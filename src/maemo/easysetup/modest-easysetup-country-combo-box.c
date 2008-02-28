/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* So we can use the getline() function, which is a convenient GNU extension. */
#endif

#include <stdio.h>

#include "../modest-maemo-utils.h"
#include "modest-easysetup-country-combo-box.h"
#include <gtk/gtkliststore.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */

#include <libintl.h> /* For dgettext(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MAX_LINE_LEN 128 /* max length of a line in MCC file */

G_DEFINE_TYPE (EasysetupCountryComboBox, easysetup_country_combo_box, GTK_TYPE_COMBO_BOX);

typedef struct
{
	GtkTreeModel *model;
} ModestEasysetupCountryComboBoxPrivate;

#define MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
							MODEST_EASYSETUP_TYPE_COUNTRY_COMBO_BOX, \
							ModestEasysetupCountryComboBoxPrivate))

static void
easysetup_country_combo_box_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
easysetup_country_combo_box_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
easysetup_country_combo_box_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (easysetup_country_combo_box_parent_class)->dispose)
		G_OBJECT_CLASS (easysetup_country_combo_box_parent_class)->dispose (object);
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* string */
	MODEL_COL_MCC  = 1 /* the 'effective mcc' for this country */
};

	
static void
easysetup_country_combo_box_finalize (GObject *object)
{
	ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));
	G_OBJECT_CLASS (easysetup_country_combo_box_parent_class)->finalize (object);
}

static void
easysetup_country_combo_box_class_init (EasysetupCountryComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestEasysetupCountryComboBoxPrivate));

	object_class->get_property = easysetup_country_combo_box_get_property;
	object_class->set_property = easysetup_country_combo_box_set_property;
	object_class->dispose = easysetup_country_combo_box_dispose;
	object_class->finalize = easysetup_country_combo_box_finalize;
}




/* cluster mcc's, based on the list
 * http://en.wikipedia.org/wiki/Mobile_country_code
 */
static int
effective_mcc (gint mcc)
{
	switch (mcc) {
	case 405: return 404; /* india */
	case 441: return 440; /* japan */	
	case 235: return 234; /* united kingdom */
	case 311:
	case 312:
	case 313:
	case 314:
	case 315:
	case 316: return 310; /* united states */
	default:  return mcc;
	}
}


/* each line is of the form:
   xxx    logical_id
   
  NOTE: this function is NOT re-entrant, the out-param country
  are static strings that should NOT be freed. and will change when
  calling this function again

  also note, this function will return the "effective mcc", which
  is the normalized mcc for a country - ie. even if the there
  are multiple entries for the United States with various mccs,
  this function will always return 310, even if the real mcc parsed
  would be 314. see the 'effective_mcc' function above.
*/
static int
parse_mcc_mapping_line (const char* line,  char** country)
{
	int i, j;
	char mcc[4];  /* the mcc code, always 3 bytes*/
	static char my_country[128];

	if (!line) {
		*country = NULL;
		return 0;
	}
	
	for (i = 3, j = 0; i < 128; ++i) {
		char kar = line[i];
		if (kar == '\0')
			break;
		else if (kar < '_')
			continue;
		else 
			my_country [j++] = kar;
	}
	my_country[j] = '\0';

	mcc[0] = line[0];
	mcc[1] = line[1];
	mcc[2] = line[2];
	mcc[3] = '\0';
	
	*country = my_country;

	return effective_mcc ((int) strtol ((const char*)mcc, NULL, 10));
}

/** Note that the mcc_mapping file is installed 
 * by the operator-wizard-settings package.
 */
static void
load_from_file (EasysetupCountryComboBox *self)
{
	ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	
	char line[MAX_LINE_LEN];
	guint previous_mcc = 0;
	
	FILE *file = modest_maemo_open_mcc_mapping_file ();
	if (!file) {
		g_warning("Could not open mcc_mapping file");
		return;
	}

	while (fgets (line, MAX_LINE_LEN, file) != NULL) { 

		int mcc;
		char *country = NULL;
		mcc = parse_mcc_mapping_line (line, &country);
		if (!country || mcc == 0) {
			g_warning ("%s: error parsing line: '%s'", __FUNCTION__, line);
			continue;
		}

		if (mcc == previous_mcc) {
			/* g_warning ("already seen: %s", line); */
			continue;
		}
		previous_mcc = mcc;
		
		/* Get the translation for the country name:
		 * Note that the osso_countries_1.0 translation domain files are installed 
		 * by the operator-wizard-settings package. */
		/* For post-Bora, there is a separate (meta)package osso-countries-l10n-mr0 */
				
		/* Note: Even when the untranslated names are different, there may still be 
		 * duplicate translated names. They would be translation bugs.
		 */
		const gchar *name_translated = dgettext ("osso-countries", country);
		
		/* Add the row to the model: */
		GtkTreeIter iter;
		gtk_list_store_append (liststore, &iter);
		gtk_list_store_set(liststore, &iter, MODEL_COL_MCC, mcc, MODEL_COL_NAME, name_translated, -1);
	}	
	fclose (file);
	
	/* Sort the items: */
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (liststore), 
					      MODEL_COL_NAME, GTK_SORT_ASCENDING);
}

static void
easysetup_country_combo_box_init (EasysetupCountryComboBox *self)
{
	ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);
	priv->model = NULL;
}

void
easysetup_country_combo_box_load_data(EasysetupCountryComboBox *self)
{
	ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);

	/* Create a tree model for the combo box,
	 * with a string for the name, and an int for the MCC ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (2,  G_TYPE_STRING, G_TYPE_INT));
	
	/* Setup the combo box: */
	GtkComboBox *combobox = GTK_COMBO_BOX (self);
	
	/* Country column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer, 
	"text", MODEL_COL_NAME, NULL);

	
	g_assert (GTK_IS_LIST_STORE(priv->model));

	
	/* Fill the model with rows: */
	load_from_file (self);
	/* Set this _after_ loading from file, it makes loading faster */
	gtk_combo_box_set_model (combobox, priv->model);
}

EasysetupCountryComboBox*
easysetup_country_combo_box_new (void)
{
	return g_object_new (MODEST_EASYSETUP_TYPE_COUNTRY_COMBO_BOX, NULL);
}

/**
 * Returns the MCC number of the selected country, or 0 if no country was selected. 
 */
gint
easysetup_country_combo_box_get_active_country_mcc (EasysetupCountryComboBox *self)
{
	GtkTreeIter active;
	const gboolean found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (self), &active);
	if (found) {
		ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);
		gint mcc = 0;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_MCC, &mcc, -1); 
		return mcc;	
	}
	return 0; /* Failed. */
}


/**
 * Selects the MCC number of the selected country.
 * Specify 0 to select no country. 
 */
gboolean
easysetup_country_combo_box_set_active_country_mcc (EasysetupCountryComboBox *self, guint mcc)
{
	ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first (priv->model, &iter)) 
		return FALSE;
	do {
		gint current_mcc = 0;
		gtk_tree_model_get (priv->model, &iter, MODEL_COL_MCC, &current_mcc, -1);
		if (current_mcc == mcc) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self), &iter);
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (priv->model, &iter));
	
	return FALSE; /* not found */
}


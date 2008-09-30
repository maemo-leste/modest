/* Copyright (c) 2008, Nokia Corporation
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

#include "modest-maemo-utils.h"
#include "modest-country-picker.h"
#include <gtk/gtkliststore.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */
#include <langinfo.h>
#include <locale.h>
#include <libintl.h> /* For dgettext(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MAX_LINE_LEN 128 /* max length of a line in MCC file */

G_DEFINE_TYPE (ModestCountryPicker, modest_country_picker, HILDON_TYPE_PICKER_BUTTON);

typedef struct
{
	gint locale_mcc;
/* 	GtkTreeModel *model; */
} ModestCountryPickerPrivate;

#define MODEST_COUNTRY_PICKER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
									   MODEST_TYPE_COUNTRY_PICKER, \
									   ModestCountryPickerPrivate))

static void
modest_country_picker_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_country_picker_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_country_picker_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_country_picker_parent_class)->dispose)
		G_OBJECT_CLASS (modest_country_picker_parent_class)->dispose (object);
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* string */
	MODEL_COL_MCC  = 1 /* the 'effective mcc' for this country */
};

	
static void
modest_country_picker_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_country_picker_parent_class)->finalize (object);
}

static void
modest_country_picker_class_init (ModestCountryPickerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestCountryPickerPrivate));

	object_class->get_property = modest_country_picker_get_property;
	object_class->set_property = modest_country_picker_set_property;
	object_class->dispose = modest_country_picker_dispose;
	object_class->finalize = modest_country_picker_finalize;
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
load_from_file (ModestCountryPicker *self, GtkListStore *liststore)
{
	ModestCountryPickerPrivate *priv = MODEST_COUNTRY_PICKER_GET_PRIVATE (self);
	
	char line[MAX_LINE_LEN];
	guint previous_mcc = 0;
	gchar *territory, *fallback = NULL;
	gchar *current_locale;

	/* Get the territory specified for the current locale */
	territory = nl_langinfo (_NL_ADDRESS_COUNTRY_NAME);

	/* Tricky stuff, the translations of the OSSO countries does
	   not always correspond to the country names in locale
	   databases. Add all these cases here. sergio */
	if (!strcmp (territory, "United Kingdom"))
		fallback = g_strdup ("UK");

	current_locale = setlocale (LC_ALL ,NULL);

	FILE *file = modest_maemo_open_mcc_mapping_file ();
	if (!file) {
		g_warning("Could not open mcc_mapping file");
		return;
	}

	while (fgets (line, MAX_LINE_LEN, file) != NULL) { 

		int mcc;
		char *country = NULL;
		const gchar *name_translated, *english_translation;

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

		if (!priv->locale_mcc) {
			english_translation = dgettext ("osso-countries", country);
			if (!strcmp (english_translation, territory) ||
			    (fallback && !strcmp (english_translation, fallback)))
				priv->locale_mcc = mcc;
		}
		name_translated = dgettext ("osso-countries", country);
		
		/* Add the row to the model: */
		GtkTreeIter iter;
		gtk_list_store_append (liststore, &iter);
		gtk_list_store_set(liststore, &iter, MODEL_COL_MCC, mcc, MODEL_COL_NAME, name_translated, -1);
	}	
	fclose (file);

	/* Sort the items: */
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (liststore), 
					      MODEL_COL_NAME, GTK_SORT_ASCENDING);

	g_free (fallback);
}

static void
modest_country_picker_init (ModestCountryPicker *self)
{
	ModestCountryPickerPrivate *priv = MODEST_COUNTRY_PICKER_GET_PRIVATE (self);
	priv->locale_mcc = 0;
}

void
modest_country_picker_load_data(ModestCountryPicker *self)
{
	GtkListStore *model;

	/* Create a tree model for the combo box,
	 * with a string for the name, and an int for the MCC ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	model = gtk_list_store_new (2,  G_TYPE_STRING, G_TYPE_INT);
	
	/* Country column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	GtkWidget *selector = hildon_touch_selector_new ();
	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));
	hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), GTK_TREE_MODEL (model),
					     renderer, "text", MODEL_COL_NAME, NULL);

	/* Fill the model with rows: */
	load_from_file (self, model);

	/* Set this _after_ loading from file, it makes loading faster */
	hildon_touch_selector_set_model (HILDON_TOUCH_SELECTOR (selector),
					 0, GTK_TREE_MODEL (model));
}

ModestCountryPicker*
modest_country_picker_new (void)
{
	return g_object_new (MODEST_TYPE_COUNTRY_PICKER, 
			     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
			     "size", HILDON_SIZE_AUTO,
			     NULL);
}

/**
 * Returns the MCC number of the selected country, or 0 if no country was selected. 
 */
gint
modest_country_picker_get_active_country_mcc (ModestCountryPicker *self)
{
	GtkTreeIter active;
	gboolean found;

	found = hildon_touch_selector_get_selected (hildon_picker_button_get_selector
						    (HILDON_PICKER_BUTTON (self)), 0, &active);
	if (found) {
		gint mcc = 0;
		gtk_tree_model_get (hildon_touch_selector_get_model (hildon_picker_button_get_selector
								     (HILDON_PICKER_BUTTON (self)), 
								     0), 
				    &active, MODEL_COL_MCC, &mcc, -1);
		return mcc;	
	}
	return 0; /* Failed. */
}


/**
 * Selects the MCC number of the selected country.
 * Specify 0 to select no country. 
 */
gboolean
modest_country_picker_set_active_country_locale (ModestCountryPicker *self)
{
	ModestCountryPickerPrivate *priv = MODEST_COUNTRY_PICKER_GET_PRIVATE (self);
	GtkTreeIter iter;
	gint current_mcc;
	GtkTreeModel *model;

	model = hildon_touch_selector_get_model (hildon_picker_button_get_selector 
						 (HILDON_PICKER_BUTTON (self)), 0);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;
	do {
		gtk_tree_model_get (model, &iter, MODEL_COL_MCC, &current_mcc, -1);
		if (priv->locale_mcc == current_mcc) {
			hildon_touch_selector_select_iter (hildon_picker_button_get_selector 
							   (HILDON_PICKER_BUTTON (self)), 0, 
							   &iter, TRUE);
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (model, &iter));
	
	return FALSE; /* not found */
}


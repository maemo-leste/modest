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
#include <hildon/hildon-touch-selector-entry.h>
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
	char mcc[4];  /* the mcc code, always 3 bytes*/
	gchar *iter, *tab, *final;

	if (!line) {
		*country = NULL;
		return 0;
	}

	/* Go to the first tab (Country separator) */
	tab = g_utf8_strrchr (line, -1, '\t');
	*country = g_utf8_find_next_char (tab, NULL);

	/* Replace by end of string. We need to use strlen, because
	   g_utf8_strrchr expects bytes and not UTF8 characters  */
	final = g_utf8_strrchr (tab, strlen (tab) + 1, '\n');
	if (G_LIKELY (final))
		*final = '\0';
	else
		tab[strlen(tab) - 1] = '\0';

	/* Get MCC code */
	mcc[0] = g_utf8_get_char (line);
	iter = g_utf8_find_next_char (line, NULL);
	mcc[1] = g_utf8_get_char (iter);
	iter = g_utf8_find_next_char (iter, NULL);
	mcc[2] = g_utf8_get_char (iter);
	mcc[3] = '\0';

	return effective_mcc ((int) strtol ((const char*)mcc, NULL, 10));
}

/** Note that the mcc_mapping file is installed 
 * by the operator-wizard-settings package.
 */
static void
load_from_file (ModestCountryPicker *self, GtkListStore *liststore)
{
	ModestCountryPickerPrivate *priv = MODEST_COUNTRY_PICKER_GET_PRIVATE (self);
	gboolean translated;
	char line[MAX_LINE_LEN];
	guint previous_mcc = 0;
	gchar *territory;

	FILE *file = modest_maemo_open_mcc_mapping_file (&translated);
	if (!file) {
		g_warning("Could not open mcc_mapping file");
		return;
	}

	/* Get the territory specified for the current locale */
	territory = nl_langinfo (_NL_ADDRESS_COUNTRY_NAME);

	while (fgets (line, MAX_LINE_LEN, file) != NULL) {

		int mcc;
		char *country = NULL;
		const gchar *name_translated;

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
			if (translated) {
				if (!g_utf8_collate (country, territory))
					priv->locale_mcc = mcc;
			} else {
				gchar *translation = dgettext ("osso-countries", country);
				if (!g_utf8_collate (translation, territory))
					priv->locale_mcc = mcc;
			}
		}
		name_translated = dgettext ("osso-countries", country);

		/* Add the row to the model: */
		GtkTreeIter iter;
		gtk_list_store_append (liststore, &iter);
		gtk_list_store_set(liststore, &iter, MODEL_COL_MCC, mcc, MODEL_COL_NAME, name_translated, -1);
	}
	fclose (file);

	/* Fallback to Finland */
	if (!priv->locale_mcc)
		priv->locale_mcc = 244;

	/* Sort the items: */
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (liststore), 
					      MODEL_COL_NAME, GTK_SORT_ASCENDING);
}

static void
modest_country_picker_init (ModestCountryPicker *self)
{
	ModestCountryPickerPrivate *priv = MODEST_COUNTRY_PICKER_GET_PRIVATE (self);
	priv->locale_mcc = 0;
}

static gchar *
country_picker_print_func (HildonTouchSelector *selector, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *text = NULL;

	/* Always pick the selected country from the tree view and
	   never from the entry */
	model = hildon_touch_selector_get_model (selector, 0);
	if (hildon_touch_selector_get_selected (selector, 0, &iter)) {
		gint column;

		column = hildon_touch_selector_entry_get_text_column (HILDON_TOUCH_SELECTOR_ENTRY (selector));
		gtk_tree_model_get (model, &iter, column, &text, -1);
	}
	return text;
}

void
modest_country_picker_load_data(ModestCountryPicker *self)
{
	GtkCellRenderer *renderer;
	GtkWidget *selector;
	GtkListStore *model;
	HildonTouchSelectorColumn *column;

	/* Create a tree model for the combo box,
	 * with a string for the name, and an int for the MCC ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	model = gtk_list_store_new (2,  G_TYPE_STRING, G_TYPE_INT);

	/* Country column:
	 * The ID model column in not shown in the view. */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	selector = hildon_touch_selector_entry_new ();
	hildon_touch_selector_set_print_func (HILDON_TOUCH_SELECTOR (selector), (HildonTouchSelectorPrintFunc) country_picker_print_func);
	column = hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), GTK_TREE_MODEL (model),
						      renderer, "text", MODEL_COL_NAME, NULL);
	g_object_set (G_OBJECT (column), "text-column", MODEL_COL_NAME, NULL);

	/* Fill the model with rows: */
	load_from_file (self, model);

	/* Set this _after_ loading from file, it makes loading faster */
	hildon_touch_selector_set_model (HILDON_TOUCH_SELECTOR (selector),
					 0, GTK_TREE_MODEL (model));
	hildon_touch_selector_entry_set_text_column (HILDON_TOUCH_SELECTOR_ENTRY (selector),
						     MODEL_COL_NAME);
	hildon_touch_selector_entry_set_input_mode (HILDON_TOUCH_SELECTOR_ENTRY (selector),
						    HILDON_GTK_INPUT_MODE_ALPHA |
						    HILDON_GTK_INPUT_MODE_AUTOCAP);

	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));

	g_object_unref (model);
}

ModestCountryPicker*
modest_country_picker_new (HildonSizeType size,
			   HildonButtonArrangement arrangement)
{
	return g_object_new (MODEST_TYPE_COUNTRY_PICKER, 
			     "arrangement", arrangement,
			     "size", size,
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
	GtkWidget *selector;
	GtkTreeIter iter;
	gint current_mcc;
	GtkTreeModel *model;

	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
	model = hildon_touch_selector_get_model (HILDON_TOUCH_SELECTOR (selector), 0);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;
	do {
		gtk_tree_model_get (model, &iter, MODEL_COL_MCC, &current_mcc, -1);
		if (priv->locale_mcc == current_mcc) {
			hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, 
							   &iter, TRUE);
			hildon_button_set_value (HILDON_BUTTON (self), 
						 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (model, &iter));
	
	return FALSE; /* not found */
}


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

#include "modest-utils.h"
#include "modest-country-picker.h"
#include <hildon/hildon-touch-selector-entry.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */
#include <locale.h>
#include <libintl.h> /* For dgettext(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
		GtkWidget *entry;
		const gchar *entry_text;

		column = hildon_touch_selector_entry_get_text_column (HILDON_TOUCH_SELECTOR_ENTRY (selector));
		gtk_tree_model_get (model, &iter, column, &text, -1);

		entry = GTK_WIDGET (hildon_touch_selector_entry_get_entry (HILDON_TOUCH_SELECTOR_ENTRY (selector)));
		entry_text = hildon_entry_get_text (HILDON_ENTRY (entry));
		if (entry_text != NULL && text != NULL && strcmp (entry_text, text)) {
			hildon_entry_set_text (HILDON_ENTRY (entry), text);
		}
	}
	return text;
}

void
modest_country_picker_load_data(ModestCountryPicker *self)
{
	GtkCellRenderer *renderer;
	GtkWidget *selector;
	GtkTreeModel *model;
	HildonTouchSelectorColumn *column;
	ModestCountryPickerPrivate *priv;

	priv = MODEST_COUNTRY_PICKER_GET_PRIVATE (self);

	/* Create a tree model for the combo box,
	 * with a string for the name, and an int for the MCC ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	model = modest_utils_create_country_model ();

	/* Country column:
	 * The ID model column in not shown in the view. */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	selector = hildon_touch_selector_entry_new ();
	hildon_touch_selector_set_print_func (HILDON_TOUCH_SELECTOR (selector), (HildonTouchSelectorPrintFunc) country_picker_print_func);

	column = hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), model,
						      renderer, "text", MODEST_UTILS_COUNTRY_MODEL_COLUMN_NAME, NULL);
	hildon_touch_selector_entry_set_text_column (HILDON_TOUCH_SELECTOR_ENTRY (selector),
						     MODEST_UTILS_COUNTRY_MODEL_COLUMN_NAME);
	modest_utils_fill_country_model (model, &(priv->locale_mcc));

	/* Set this _after_ loading from file, it makes loading faster */
	hildon_touch_selector_set_model (HILDON_TOUCH_SELECTOR (selector),
					 0, model);
	hildon_touch_selector_entry_set_input_mode (HILDON_TOUCH_SELECTOR_ENTRY (selector),
						    HILDON_GTK_INPUT_MODE_ALPHA |
						    HILDON_GTK_INPUT_MODE_SPECIAL |
						    HILDON_GTK_INPUT_MODE_AUTOCAP);


	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));

	g_object_unref (model);
}

ModestCountryPicker*
modest_country_picker_new (HildonSizeType size,
			   HildonButtonArrangement arrangement)
{
	ModestCountryPicker *picker = g_object_new (MODEST_TYPE_COUNTRY_PICKER, 
						    "arrangement", arrangement,
						    "size", size,
						    NULL);

	/* For theming purpouses. Widget name must end in Button-finger */
	gtk_widget_set_name ((GtkWidget *) picker, "ModestCountryPickerButton-finger");

	return picker;
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
				    &active, MODEST_UTILS_COUNTRY_MODEL_COLUMN_MCC, &mcc, -1);
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
		gtk_tree_model_get (model, &iter, MODEST_UTILS_COUNTRY_MODEL_COLUMN_MCC, &current_mcc, -1);
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


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

#include <modest-utils.h>
#include "modest-easysetup-country-combo-box.h"
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

#if MODEST_HILDON_API < 2
G_DEFINE_TYPE (EasysetupCountryComboBox, easysetup_country_combo_box, GTK_TYPE_COMBO_BOX);
#else
G_DEFINE_TYPE (EasysetupCountryComboBox, easysetup_country_combo_box, HILDON_TYPE_PICKER_BUTTON);
#endif

typedef struct
{
	gint locale_mcc;
/* 	GtkTreeModel *model; */
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


static void
easysetup_country_combo_box_init (EasysetupCountryComboBox *self)
{
	ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);
	priv->locale_mcc = 0;
}

void
easysetup_country_combo_box_load_data(EasysetupCountryComboBox *self)
{
	ModestEasysetupCountryComboBoxPrivate *priv;
	GtkTreeModel *model;

	priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);
	/* Create a tree model for the combo box,
	 * with a string for the name, and an int for the MCC ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	model = modest_utils_create_country_model ();
	
	/* Country column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

#if MODEST_HILDON_API < 2
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (self), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self), renderer, 
					"text", MODEST_UTILS_COUNTRY_MODEL_COLUMN_NAME, NULL);
#else
	GtkWidget *selector = hildon_touch_selector_new ();
	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));
	hildon_touch_selector_append_column (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)),
					     model,
					     renderer, "text", MODEST_UTILS_COUNTRY_MODEL_COLUMN_NAME, NULL);
#endif

	modest_utils_fill_country_model (model, &(priv->locale_mcc));

	/* Set this _after_ loading from file, it makes loading faster */
#if MODEST_HILDON_API < 2
	gtk_combo_box_set_model (GTK_COMBO_BOX (self), model);
#else
	hildon_touch_selector_set_model (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)),
					 0, model);
#endif
}

EasysetupCountryComboBox*
easysetup_country_combo_box_new (void)
{
#if MODEST_HILDON_API >= 2
	return g_object_new (MODEST_EASYSETUP_TYPE_COUNTRY_COMBO_BOX, 
			     "arrangement", HILDON_BUTTON_ARRANGEMENT_VERTICAL,
			     "size", HILDON_SIZE_AUTO,
			     NULL);
#else
	return g_object_new (MODEST_EASYSETUP_TYPE_COUNTRY_COMBO_BOX, 
			     NULL);
#endif
}

/**
 * Returns the MCC number of the selected country, or 0 if no country was selected. 
 */
gint
easysetup_country_combo_box_get_active_country_mcc (EasysetupCountryComboBox *self)
{
	GtkTreeIter active;
	gboolean found;

#if MODEST_HILDON_API < 2
	found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (self), &active);
#else
	found = hildon_touch_selector_get_selected (hildon_picker_button_get_selector
						    (HILDON_PICKER_BUTTON (self)), 0, &active);
#endif
	if (found) {
		gint mcc = 0;
#if MODEST_HILDON_API < 2
		gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (self)), 
				    &active, MODEST_UTILS_COUNTRY_MODEL_COLUMN_MCC, &mcc, -1);
#else
		gtk_tree_model_get (hildon_touch_selector_get_model (hildon_picker_button_get_selector
								     (HILDON_PICKER_BUTTON (self)), 
								     0), 
				    &active, MODEST_UTILS_COUNTRY_MODEL_COLUMN_MCC, &mcc, -1);
#endif
		return mcc;	
	}
	return 0; /* Failed. */
}


/**
 * Selects the MCC number of the selected country.
 * Specify 0 to select no country. 
 */
gboolean
easysetup_country_combo_box_set_active_country_locale (EasysetupCountryComboBox *self)
{
	ModestEasysetupCountryComboBoxPrivate *priv = MODEST_EASYSETUP_COUNTRY_COMBO_BOX_GET_PRIVATE (self);
	GtkTreeIter iter;
	gint current_mcc;
	GtkTreeModel *model;

#if MODEST_HILDON_API < 2
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (self));
	g_debug ("%s: HILDON < 2", __FUNCTION__);
#else
	model = hildon_touch_selector_get_model (hildon_picker_button_get_selector 
						 (HILDON_PICKER_BUTTON (self)), 0);
	g_debug ("%s: HILDON >= 2", __FUNCTION__);
#endif
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;
	do {
		gtk_tree_model_get (model, &iter, MODEST_UTILS_COUNTRY_MODEL_COLUMN_MCC, &current_mcc, -1);
		if (priv->locale_mcc == current_mcc) {
#if MODEST_HILDON_API < 2
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self), &iter);
#else
			hildon_touch_selector_select_iter (hildon_picker_button_get_selector 
							   (HILDON_PICKER_BUTTON (self)), 0, 
							   &iter, TRUE);
#endif
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (model, &iter));
	
	return FALSE; /* not found */
}


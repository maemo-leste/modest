/* Copyright (c) 2007, 2008, Nokia Corporation
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

#include "modest-limit-retrieve-picker.h"
#include <gtk/gtkliststore.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

G_DEFINE_TYPE (ModestLimitRetrievePicker, modest_limit_retrieve_picker, HILDON_TYPE_PICKER_BUTTON);

#define MODEST_LIMIT_RETRIEVE_PICKER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_LIMIT_RETRIEVE_PICKER, ModestLimitRetrievePickerPrivate))

typedef struct _ModestLimitRetrievePickerPrivate ModestLimitRetrievePickerPrivate;

struct _ModestLimitRetrievePickerPrivate
{
	GtkTreeModel *model;
};

static void
modest_limit_retrieve_picker_finalize (GObject *object)
{
	ModestLimitRetrievePickerPrivate *priv = MODEST_LIMIT_RETRIEVE_PICKER_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (modest_limit_retrieve_picker_parent_class)->finalize (object);
}

static void
modest_limit_retrieve_picker_class_init (ModestLimitRetrievePickerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestLimitRetrievePickerPrivate));

	object_class->finalize = modest_limit_retrieve_picker_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* a string */
	MODEL_COL_NUM = 1 /* an int */
};

static void modest_limit_retrieve_picker_fill (ModestLimitRetrievePicker *picker);

static gchar *
touch_selector_print_func (HildonTouchSelector *selector, gpointer userdata)
{
	GtkTreeIter iter;
	if (hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &iter)) {
		GtkTreeModel *model;
		GValue value = {0,};
		
		model = hildon_touch_selector_get_model (HILDON_TOUCH_SELECTOR (selector), 0);
		gtk_tree_model_get_value (model, &iter, MODEL_COL_NAME, &value);
		return g_value_dup_string (&value);
	}
	return NULL;
}

static void
modest_limit_retrieve_picker_init (ModestLimitRetrievePicker *self)
{
	ModestLimitRetrievePickerPrivate *priv = MODEST_LIMIT_RETRIEVE_PICKER_GET_PRIVATE (self);

	priv->model = NULL;
}

ModestLimitRetrievePicker*
modest_limit_retrieve_picker_new (HildonSizeType size,
				  HildonButtonArrangement arrangement)
{
	ModestLimitRetrievePicker *self;
	ModestLimitRetrievePickerPrivate *priv;
	GtkCellRenderer *renderer;
	GtkWidget *selector;

	self = g_object_new (MODEST_TYPE_LIMIT_RETRIEVE_PICKER, 
			     "arrangement", arrangement,
			     "size", size,
			     NULL);
	priv = MODEST_LIMIT_RETRIEVE_PICKER_GET_PRIVATE (self);

	/* Create a tree model,
	 * with a string for the name, and an ID for the servertype.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	selector = hildon_touch_selector_new ();
	hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), GTK_TREE_MODEL (priv->model),
					     renderer, "text", MODEL_COL_NAME, NULL);

	hildon_touch_selector_set_model (HILDON_TOUCH_SELECTOR (selector), 0, GTK_TREE_MODEL (priv->model));
	hildon_touch_selector_set_print_func (HILDON_TOUCH_SELECTOR (selector), (HildonTouchSelectorPrintFunc) touch_selector_print_func);

	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));

	modest_limit_retrieve_picker_fill (self);

	/* For theming purpouses. Widget name must end in Button-finger */
	gtk_widget_set_name ((GtkWidget *) self, "ModestLimitRetrievePickerButton-finger");

	return self;
}

/* Fill the picker box with appropriate choices.
 * #picker: The picker box.
 * @protocol: IMAP or POP.
 */
static void modest_limit_retrieve_picker_fill (ModestLimitRetrievePicker *picker)
{	
	ModestLimitRetrievePickerPrivate *priv = MODEST_LIMIT_RETRIEVE_PICKER_GET_PRIVATE (picker);
	
	/* Remove any existing rows: */
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);
	
	GtkTreeIter iter;
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 0, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_nolimit"), -1);
	
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 200, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_200"), -1);
	
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 100, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_100"), -1);

	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 50, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_50"), -1);

	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 20, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_20"), -1);
}

/**
 * Returns the selected limit_retrieve, 
 * or 0 if no limit_retrieve was selected.
 */
gint
modest_limit_retrieve_picker_get_active_limit_retrieve (ModestLimitRetrievePicker *picker)
{
	GtkTreeIter active;
	gboolean found;
	GtkWidget *selector;

	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (picker)));
	found = hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &active);
	if (found) {
		ModestLimitRetrievePickerPrivate *priv = MODEST_LIMIT_RETRIEVE_PICKER_GET_PRIVATE (picker);

		gint limit_retrieve = 0;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_NUM, &limit_retrieve, -1);
		return limit_retrieve;	
	}

	return 0; /* Failed. */
}

/* This allows us to pass more than one piece of data to the signal handler,
 * and get a result: */
typedef struct 
{
		ModestLimitRetrievePicker* self;
		gint num;
		gboolean found;
} ForEachData;

static gboolean
on_model_foreach_select_id(GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	ForEachData *state = (ForEachData*)(user_data);
	
	gboolean result = FALSE;
	
	/* Select the item if it has the matching name: */
	gint num = 0;
	gtk_tree_model_get (model, iter, MODEL_COL_NUM, &num, -1); 
	if(num == state->num) {
		GtkWidget *selector;
		selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (state->self)));
		hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, iter, TRUE);
		hildon_button_set_value (HILDON_BUTTON (state->self),
					 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
		
		state->found = TRUE;
		return TRUE; /* Stop walking the tree. */
	}
	
	return result; /* Whether we keep walking the tree. */
}

/**
 * Selects the specified limit_retrieve, 
 * or FALSE if no limit_retrieve was selected.
 */
gboolean
modest_limit_retrieve_picker_set_active_limit_retrieve (ModestLimitRetrievePicker *picker, gint limit_retrieve)
{
	ModestLimitRetrievePickerPrivate *priv = MODEST_LIMIT_RETRIEVE_PICKER_GET_PRIVATE (picker);
	
	/* Create a state instance so we can send two items of data to the signal handler: */
	ForEachData *state = g_new0 (ForEachData, 1);
	state->self = picker;
	state->num = limit_retrieve;
	state->found = FALSE;
	
	/* Look at each item, and select the one with the correct ID: */
	gtk_tree_model_foreach (priv->model, &on_model_foreach_select_id, state);

	const gboolean result = state->found;
	
	/* Free the state instance: */
	g_free(state);
	
	return result;
}


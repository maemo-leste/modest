/* Copyright (c) 2006, 2008 Nokia Corporation
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

#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkliststore.h>
#include "modest-selector-picker.h"

#define TOUCH_SELECTOR_PICKER "calling-picker"

/* 'private'/'protected' functions */
static void modest_selector_picker_class_init (ModestSelectorPickerClass *klass);
static void modest_selector_picker_init       (ModestSelectorPicker *obj);
static void modest_selector_picker_finalize   (GObject *obj);

enum {
	COLUMN_ID,
	COLUMN_DISPLAY_NAME,
	COLUMN_NUM
};

typedef struct _ModestSelectorPickerPrivate ModestSelectorPickerPrivate;
struct _ModestSelectorPickerPrivate {
	GEqualFunc id_equal_func;

	gint value_max_chars;
	
};
#define MODEST_SELECTOR_PICKER_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										MODEST_TYPE_SELECTOR_PICKER, \
										ModestSelectorPickerPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_selector_picker_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestSelectorPickerClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_selector_picker_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestSelectorPicker),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_selector_picker_init,
			NULL
		};
		my_type = g_type_register_static (HILDON_TYPE_PICKER_BUTTON,
		                                  "ModestSelectorPicker",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_selector_picker_class_init (ModestSelectorPickerClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_selector_picker_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestSelectorPickerPrivate));

}

static void
modest_selector_picker_init (ModestSelectorPicker *obj)
{
}

static void
modest_selector_picker_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static gchar *
touch_selector_print_func (HildonTouchSelector *selector, gpointer userdata)
{
	GtkTreeIter iter;
	ModestSelectorPicker *picker;

	picker = MODEST_SELECTOR_PICKER (g_object_get_data (G_OBJECT (selector), TOUCH_SELECTOR_PICKER));
	g_return_val_if_fail (MODEST_IS_SELECTOR_PICKER (picker), NULL);
	if (hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &iter)) {
		GtkTreeModel *model;
		ModestSelectorPickerPrivate *priv;
		GValue value = {0,};
		
		priv = MODEST_SELECTOR_PICKER_GET_PRIVATE (picker);
		model = hildon_touch_selector_get_model (HILDON_TOUCH_SELECTOR (selector), 0);
		gtk_tree_model_get_value (model, &iter, COLUMN_DISPLAY_NAME, &value);
		if (priv->value_max_chars == -1) {
			return g_value_dup_string (&value);
		} else {
			const gchar *str;
			str = g_value_get_string (&value);
			if (g_utf8_strlen (str, -1) > priv->value_max_chars) {
				const gchar *end;
				end = g_utf8_offset_to_pointer (str, priv->value_max_chars);
				return g_strndup (str, end - str);
			} else {
				return g_strdup (str);
			}
		}
	}
	return NULL;
}

static GtkTreeModel*
get_model (ModestPairList *pairs)
{
	GtkTreeIter iter;
	GtkListStore *store;
	GSList *cursor;
	
	store = gtk_list_store_new (2,
				    G_TYPE_POINTER, /* the id */
				    G_TYPE_STRING); /* the display name */
	cursor = pairs;
	while (cursor) {
		ModestPair *pair = (ModestPair*)cursor->data;
		gtk_list_store_insert_with_values (store, &iter, G_MAXINT,
						   COLUMN_ID,           pair->first,
						   COLUMN_DISPLAY_NAME, pair->second,
						   -1);
		cursor = cursor->next;
	}
	
	return GTK_TREE_MODEL (store);
}

static GtkWidget *
create_touch_selector (ModestSelectorPicker *self, GtkTreeModel *model)
{
	GtkCellRenderer *renderer;
	GtkWidget *selector;

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	selector = hildon_touch_selector_new ();
	hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), GTK_TREE_MODEL (model),
					     renderer, "text", COLUMN_DISPLAY_NAME, NULL);

	hildon_touch_selector_set_model (HILDON_TOUCH_SELECTOR(selector), 0, model);
	g_object_set_data (G_OBJECT (selector), TOUCH_SELECTOR_PICKER, (gpointer) self);
	hildon_touch_selector_set_print_func (HILDON_TOUCH_SELECTOR (selector), (HildonTouchSelectorPrintFunc) touch_selector_print_func);

	return selector;
}

void
modest_selector_picker_set_pair_list (ModestSelectorPicker *self, ModestPairList *pairs)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *selector;

	model = get_model (pairs);

	selector = create_touch_selector (self, model);
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(model), &iter))
		hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, &iter, TRUE);
	g_object_unref (model);

	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));
}



GtkWidget*
modest_selector_picker_new (HildonSizeType size,
			    HildonButtonArrangement arrangement,
			    ModestPairList *pairs, GEqualFunc id_equal_func)
{
	GtkTreeModel *model;
	GObject *obj;
	ModestSelectorPickerPrivate *priv;
	GtkTreeIter iter;

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_SELECTOR_PICKER,
				     "size", size,
				     "arrangement", arrangement,
				     NULL));

	priv = MODEST_SELECTOR_PICKER_GET_PRIVATE(obj);
	priv->value_max_chars = -1;
	
	model = get_model (pairs);
	if (model) {
		GtkWidget *selector;

		selector = create_touch_selector (MODEST_SELECTOR_PICKER (obj), model);
		if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter)) {
			hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, &iter, TRUE);
		}
		g_object_unref (model);

		hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (obj), HILDON_TOUCH_SELECTOR (selector));
	}

	if (id_equal_func)
		priv->id_equal_func = id_equal_func;
	else
		priv->id_equal_func = g_direct_equal; /* compare the ptr values */

	/* For theming purpouses. Widget name must end in Button-finger */
	gtk_widget_set_name ((GtkWidget *) obj, "ModestSelectorPickerButton-finger");

	return GTK_WIDGET(obj);
}



static void
get_active (ModestSelectorPicker *self, GValue *val, gint column)
{
	GtkTreeIter iter;
	GtkWidget *selector;
	g_return_if_fail (self);

	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));

	if (hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &iter)) {
		GtkTreeModel *model;
		
		model = hildon_touch_selector_get_model (HILDON_TOUCH_SELECTOR (selector), 0);
		gtk_tree_model_get_value (model, &iter, column, val);
	}
}

gpointer
modest_selector_picker_get_active_id (ModestSelectorPicker *self)
{
	GValue val = {0,};

	g_return_val_if_fail (self, NULL);
	
	/* Do not unset the GValue */
	get_active (self, &val, COLUMN_ID);

	return g_value_get_pointer (&val);
}


void
modest_selector_picker_set_active_id (ModestSelectorPicker *self, gpointer id)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	ModestSelectorPickerPrivate *priv;
	gboolean found = FALSE;
	GtkWidget *selector;
	
	g_return_if_fail (self);

	priv = MODEST_SELECTOR_PICKER_GET_PRIVATE(self);
	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
	
	model = hildon_touch_selector_get_model (HILDON_TOUCH_SELECTOR(selector), 0);
	if (!gtk_tree_model_get_iter_first (model, &iter))
		return; /* empty list */

	do {
		gpointer row_id;
		gtk_tree_model_get (model, &iter, COLUMN_ID, &row_id, -1);
		if ((priv->id_equal_func)(id, row_id)) {
			hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0,
							   &iter, TRUE);
			
			hildon_button_set_value (HILDON_BUTTON (self), 
						 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
			found = TRUE;
		}
	} while (!found && gtk_tree_model_iter_next (model, &iter));

	if (!found)
		g_printerr ("modest: could not set the active id\n"); 
}



const gchar*
modest_selector_picker_get_active_display_name (ModestSelectorPicker *self)
{
	const GValue val = {0,};
	const gchar *retval;

	g_return_val_if_fail (self, NULL);

	/* Do not unset the GValue */
	get_active (self, (GValue *)&val, COLUMN_DISPLAY_NAME);
	retval = g_value_get_string (&val);

	return retval;
}

void
modest_selector_picker_set_value_max_chars (ModestSelectorPicker *self, 
					    gint value_max_chars)
{
	ModestSelectorPickerPrivate *priv;

	g_return_if_fail (self);
	priv = MODEST_SELECTOR_PICKER_GET_PRIVATE(self);

	if (value_max_chars != priv->value_max_chars) {
		GtkWidget *selector;

		priv->value_max_chars = value_max_chars;
		selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
		hildon_touch_selector_set_print_func (HILDON_TOUCH_SELECTOR (selector), (HildonTouchSelectorPrintFunc) touch_selector_print_func);
	}
}

gint
modest_selector_picker_get_value_max_chars (ModestSelectorPicker *self)
{
	ModestSelectorPickerPrivate *priv;
	g_return_val_if_fail (self, -1);

	priv = MODEST_SELECTOR_PICKER_GET_PRIVATE(self);

	return priv->value_max_chars;
}

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

#include <modest-runtime.h>
#include "modest-servertype-picker.h"
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

G_DEFINE_TYPE (ModestServertypePicker, modest_servertype_picker, HILDON_TYPE_PICKER_BUTTON);

#define MODEST_SERVERTYPE_PICKER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_SERVERTYPE_PICKER, ModestServertypePickerPrivate))

typedef struct _ModestServertypePickerPrivate ModestServertypePickerPrivate;

struct _ModestServertypePickerPrivate
{
	GtkTreeModel *model;
};

static void
modest_servertype_picker_finalize (GObject *object)
{
	ModestServertypePickerPrivate *priv = MODEST_SERVERTYPE_PICKER_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (modest_servertype_picker_parent_class)->finalize (object);
}

static void
modest_servertype_picker_class_init (ModestServertypePickerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestServertypePickerPrivate));

	object_class->finalize = modest_servertype_picker_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* a string */
	MODEL_COL_ID = 1 /* an int. */
};

static void
modest_servertype_picker_init (ModestServertypePicker *self)
{
	ModestServertypePickerPrivate *priv;

	priv = MODEST_SERVERTYPE_PICKER_GET_PRIVATE (self);
	priv->model = NULL;

}

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
modest_servertype_picker_fill (ModestServertypePicker *self,
				     gboolean filter_providers)
{	
	ModestServertypePickerPrivate *priv;
	GtkListStore *liststore;
	ModestProtocolRegistry *protocol_registry;
	GSList *remote_protocols, *node;
	GtkTreeIter iter;
	GtkWidget *selector;
	
	/* Remove any existing rows: */
	priv = MODEST_SERVERTYPE_PICKER_GET_PRIVATE (self);
	protocol_registry = modest_runtime_get_protocol_registry ();
	remote_protocols = modest_protocol_registry_get_by_tag (protocol_registry, MODEST_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS);

	liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);

	for (node = remote_protocols; node != NULL; node = g_slist_next (node)) {
		ModestProtocol* protocol;
		gboolean add = TRUE;

		protocol = (ModestProtocol *) node->data;

		/* Do not include the protocols that would be listed
		   in the providers combo */
		if (filter_providers)
			if (modest_protocol_registry_protocol_type_is_provider (protocol_registry,
										modest_protocol_get_type_id (protocol))) {
				add = FALSE;
			}
		
		if (add) {
			gtk_list_store_append (liststore, &iter);
			gtk_list_store_set (liststore, &iter, 
					    MODEL_COL_ID, 
					    modest_protocol_get_type_id (protocol),
					    MODEL_COL_NAME, 
					    modest_protocol_get_display_name (protocol),
					    -1);
		}
	}
	
	g_slist_free (remote_protocols);

	/* Choose first in list */
	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->model), &iter)) {
		hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, &iter, TRUE);
	}
}

ModestServertypePicker*
modest_servertype_picker_new (HildonSizeType size,
			      HildonButtonArrangement arrangement,
			      gboolean filter_providers)
{
	ModestServertypePicker *self;
	ModestServertypePickerPrivate *priv;
	GtkCellRenderer *renderer;
	GtkWidget *selector;

	self = g_object_new (MODEST_TYPE_SERVERTYPE_PICKER, 
			     "arrangement", arrangement,
			     "size", size,
			     NULL);
	priv = MODEST_SERVERTYPE_PICKER_GET_PRIVATE (self);

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

	/* Fill the model */	
	modest_servertype_picker_fill (self, filter_providers);

	/* For theming purpouses. Widget name must end in Button-finger */
	gtk_widget_set_name ((GtkWidget *) self, "ModestServertypePickerButton-finger");

	return self;
}

/**
 * Returns the selected servertype, 
 * or MODEST_PROTOCOL_REGISTRY_TYPE_INVALID if no servertype was selected.
 */
ModestProtocolType
modest_servertype_picker_get_active_servertype (ModestServertypePicker *self)
{
	GtkTreeIter active;
	gboolean found;
	GtkWidget *selector;

	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
	found = hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &active);
	if (found) {
		ModestServertypePickerPrivate *priv;
		ModestProtocolType servertype;

		priv = MODEST_SERVERTYPE_PICKER_GET_PRIVATE (self);

		servertype = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_ID, &servertype, -1);
		return servertype;	
	}

	return MODEST_PROTOCOL_REGISTRY_TYPE_INVALID; /* Failed. */
}

/* This allows us to pass more than one piece of data to the signal handler,
 * and get a result: */
typedef struct 
{
		ModestServertypePicker* self;
		ModestProtocolType id;
		gboolean found;
} ForEachData;

static gboolean
on_model_foreach_select_id(GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	ModestProtocolType id = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	ForEachData *state = (ForEachData*)(user_data);
	GtkWidget *selector;
	
	/* Select the item if it has the matching ID: */
	gtk_tree_model_get (model, iter, MODEL_COL_ID, &id, -1); 
	if(id == state->id) {
		selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (state->self)));
		hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, iter, TRUE);
		hildon_button_set_value (HILDON_BUTTON (state->self),
					 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
		
		state->found = TRUE;
		return TRUE; /* Stop walking the tree. */
	}
	
	return FALSE; /* Keep walking the tree. */
}

/**
 * Selects the specified servertype, 
 * or MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN if no servertype was selected.
 */
gboolean
modest_servertype_picker_set_active_servertype (ModestServertypePicker *picker, ModestProtocolType servertype)
{
	ModestServertypePickerPrivate *priv;
	ForEachData *state;
	gboolean result;
	
	priv = MODEST_SERVERTYPE_PICKER_GET_PRIVATE (picker);

	/* Create a state instance so we can send two items of data to the signal handler: */
	state = g_new0 (ForEachData, 1);
	state->self = picker;
	state->id = servertype;
	state->found = FALSE;
	
	/* Look at each item, and select the one with the correct ID: */
	gtk_tree_model_foreach (priv->model, &on_model_foreach_select_id, state);

	result = state->found;
	
	/* Free the state instance: */
	g_free(state);
	
	return result;
}


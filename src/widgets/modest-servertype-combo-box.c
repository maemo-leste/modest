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

#include <modest-runtime.h>
#include "modest-servertype-combo-box.h"
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

G_DEFINE_TYPE (ModestServertypeComboBox, modest_servertype_combo_box, GTK_TYPE_COMBO_BOX);

#define MODEST_SERVERTYPE_COMBO_BOX_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_SERVERTYPE_COMBO_BOX, ModestServertypeComboBoxPrivate))

typedef struct _ModestServertypeComboBoxPrivate ModestServertypeComboBoxPrivate;

struct _ModestServertypeComboBoxPrivate
{
	GtkTreeModel *model;
};

static void
modest_servertype_combo_box_get_property (GObject *object, guint property_id,
					  GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_servertype_combo_box_set_property (GObject *object, guint property_id,
					  const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_servertype_combo_box_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_servertype_combo_box_parent_class)->dispose)
		G_OBJECT_CLASS (modest_servertype_combo_box_parent_class)->dispose (object);
}

static void
modest_servertype_combo_box_finalize (GObject *object)
{
	ModestServertypeComboBoxPrivate *priv = MODEST_SERVERTYPE_COMBO_BOX_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (modest_servertype_combo_box_parent_class)->finalize (object);
}

static void
modest_servertype_combo_box_class_init (ModestServertypeComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestServertypeComboBoxPrivate));

	object_class->get_property = modest_servertype_combo_box_get_property;
	object_class->set_property = modest_servertype_combo_box_set_property;
	object_class->dispose = modest_servertype_combo_box_dispose;
	object_class->finalize = modest_servertype_combo_box_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* a string */
	MODEL_COL_ID = 1 /* an int. */
};

static void
modest_servertype_combo_box_init (ModestServertypeComboBox *self)
{
	ModestServertypeComboBoxPrivate *priv = MODEST_SERVERTYPE_COMBO_BOX_GET_PRIVATE (self);

	/* Create a tree model for the combo box,
	 * with a string for the name, and an ID for the servertype.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));

	/* Setup the combo box: */
	GtkComboBox *combobox = GTK_COMBO_BOX (self);
	gtk_combo_box_set_model (combobox, priv->model);

	/* Servertype column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer, 
					"text", MODEL_COL_NAME, NULL);
}

static void 
modest_servertype_combo_box_fill (ModestServertypeComboBox *combobox,
				     gboolean filter_providers)
{	
	ModestServertypeComboBoxPrivate *priv;
	GtkListStore *liststore;
	ModestProtocolRegistry *protocol_registry;
	GSList *remote_protocols, *node;
	GtkTreeIter iter;
	
	/* Remove any existing rows: */
	priv = MODEST_SERVERTYPE_COMBO_BOX_GET_PRIVATE (combobox);
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
}

ModestServertypeComboBox*
modest_servertype_combo_box_new (gboolean filter_providers)
{
	ModestServertypeComboBox *combo;

	combo = g_object_new (MODEST_TYPE_SERVERTYPE_COMBO_BOX, NULL);

	/* Fill the combo */	
	modest_servertype_combo_box_fill (combo, filter_providers);

	return combo;
}

/**
 * Returns the selected servertype, 
 * or MODEST_PROTOCOL_REGISTRY_TYPE_INVALID if no servertype was selected.
 */
ModestProtocolType
modest_servertype_combo_box_get_active_servertype (ModestServertypeComboBox *combobox)
{
	GtkTreeIter active;
	gboolean found;

	found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &active);
	if (found) {
		ModestServertypeComboBoxPrivate *priv;
		ModestProtocolType servertype;

		priv = MODEST_SERVERTYPE_COMBO_BOX_GET_PRIVATE (combobox);

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
		ModestServertypeComboBox* self;
		ModestProtocolType id;
		gboolean found;
} ForEachData;

static gboolean
on_model_foreach_select_id(GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	ModestProtocolType id = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	ForEachData *state = (ForEachData*)(user_data);
	
	/* Select the item if it has the matching ID: */
	gtk_tree_model_get (model, iter, MODEL_COL_ID, &id, -1); 
	if(id == state->id) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (state->self), iter);
		
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
modest_servertype_combo_box_set_active_servertype (ModestServertypeComboBox *combobox, ModestProtocolType servertype)
{
	ModestServertypeComboBoxPrivate *priv;
	ForEachData *state;
	gboolean result;
	
	priv = MODEST_SERVERTYPE_COMBO_BOX_GET_PRIVATE (combobox);

	/* Create a state instance so we can send two items of data to the signal handler: */
	state = g_new0 (ForEachData, 1);
	state->self = combobox;
	state->id = servertype;
	state->found = FALSE;
	
	/* Look at each item, and select the one with the correct ID: */
	gtk_tree_model_foreach (priv->model, &on_model_foreach_select_id, state);

	result = state->found;
	
	/* Free the state instance: */
	g_free(state);
	
	return result;
}


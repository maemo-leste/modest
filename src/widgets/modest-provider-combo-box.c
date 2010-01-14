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

#include "modest-provider-combo-box.h"
#include <gtk/gtkliststore.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>
#include <glib/gi18n.h>
#include <modest-text-utils.h>
#include "modest-protocol-registry.h"
#include "modest-runtime.h"
#include <modest-account-protocol.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

G_DEFINE_TYPE (ModestProviderComboBox, modest_provider_combo_box, GTK_TYPE_COMBO_BOX);

#define MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_PROVIDER_COMBO_BOX, ModestProviderComboBoxPrivate))

typedef struct _ModestProviderComboBoxPrivate ModestProviderComboBoxPrivate;

struct _ModestProviderComboBoxPrivate
{
	GtkTreeModel *model;
	GHashTable *enabled_plugin_ids;
};

static void
modest_provider_combo_box_get_property (GObject *object, guint property_id,
					GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_provider_combo_box_set_property (GObject *object, guint property_id,
					const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_provider_combo_box_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_provider_combo_box_parent_class)->dispose)
		G_OBJECT_CLASS (modest_provider_combo_box_parent_class)->dispose (object);
}

static void
modest_provider_combo_box_finalize (GObject *object)
{
	ModestProviderComboBoxPrivate *priv = MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	g_hash_table_destroy (priv->enabled_plugin_ids);
	
	G_OBJECT_CLASS (modest_provider_combo_box_parent_class)->finalize (object);
}

static void
modest_provider_combo_box_class_init (ModestProviderComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestProviderComboBoxPrivate));

	object_class->get_property = modest_provider_combo_box_get_property;
	object_class->set_property = modest_provider_combo_box_set_property;
	object_class->dispose = modest_provider_combo_box_dispose;
	object_class->finalize = modest_provider_combo_box_finalize;
}

enum MODEL_COLS {
	MODEL_COL_ID, /* a string, not an int. */
	MODEL_COL_NAME,
	MODEL_COL_ID_TYPE
};


/*
 * strictly, we should sort providers with mcc=0 after the other ones.... but, we don't have
 * that info here, so ignoring for now.
 */
static gint
provider_sort_func (GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer user_data)
{
	gchar *prov1, *prov2;
	gint retval;
	
	gtk_tree_model_get (model, iter1, MODEL_COL_NAME, &prov1, -1);
	gtk_tree_model_get (model, iter2, MODEL_COL_NAME, &prov2, -1);

	if (strcmp (prov1, prov2) == 0) 
		retval = 0;
	else if (strcmp (_("mcen_va_serviceprovider_other"), prov1) == 0)
		retval = -1;
	else if (strcmp (_("mcen_va_serviceprovider_other"), prov2) == 0)
		retval = 1;
	else
		retval = modest_text_utils_utf8_strcmp (prov1, prov2, TRUE);
	
	g_free (prov1);
	g_free (prov2);

	return retval;
}

static void
modest_provider_combo_box_init (ModestProviderComboBox *self)
{
	ModestProviderComboBoxPrivate *priv = MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE (self);

	/* Create a tree model for the combo box,
	 * with a string for the name, and a string for the ID (e.g. "vodafone.it"), and the mcc
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT));
	priv->enabled_plugin_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
							  g_free, NULL);

	/* Setup the combo box: */
	GtkComboBox *combobox = GTK_COMBO_BOX (self);
	gtk_combo_box_set_model (combobox, priv->model);

	/* Provider column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer,  "text", MODEL_COL_NAME, NULL);
	
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(priv->model),
					      MODEL_COL_NAME, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(priv->model),
					  MODEL_COL_NAME,
					  (GtkTreeIterCompareFunc)provider_sort_func,
					  NULL, NULL);
}

ModestProviderComboBox*
modest_provider_combo_box_new (void)
{
	ModestProviderComboBox *self;

	self =  g_object_new (MODEST_TYPE_PROVIDER_COMBO_BOX, NULL);
	modest_provider_combo_box_set_others_provider (MODEST_PROVIDER_COMBO_BOX (self));

	return self;
}

void
modest_provider_combo_box_fill (ModestProviderComboBox *combobox, 
				ModestPresets *presets,
				gint mcc)
{	
	GtkTreeIter other_iter;
	ModestProviderComboBoxPrivate *priv;
	GtkListStore *liststore;	
	GSList *provider_ids_used_already = NULL, *provider_protos, *tmp;
	gchar ** provider_ids = NULL;
	gchar ** provider_names;	
	gchar ** iter_provider_names;
	gchar ** iter_provider_ids;
	ModestProtocolRegistry *registry;

	g_return_if_fail (MODEST_IS_PROVIDER_COMBO_BOX(combobox));

	priv = MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE (combobox);
	liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);
	provider_names = modest_presets_get_providers (presets, mcc, TRUE, &provider_ids);

	iter_provider_names = provider_names;
	iter_provider_ids = provider_ids;

	g_hash_table_remove_all (priv->enabled_plugin_ids);

 	while(iter_provider_names && *iter_provider_names && iter_provider_ids && *iter_provider_ids) {
		const gchar* provider_name = *iter_provider_names;
		const gchar* provider_id = *iter_provider_ids;

		/* Prevent duplicate providers: */
		if (g_slist_find_custom (provider_ids_used_already, 
					 provider_id, (GCompareFunc)strcmp) == NULL) {
			/* printf("debug: provider_name=%s\n", provider_name); */

			/* Add the row: */
			GtkTreeIter iter;
			gtk_list_store_append (liststore, &iter);
			
			gtk_list_store_set(liststore, &iter, 
					   MODEL_COL_ID, provider_id, 
					   MODEL_COL_NAME, provider_name, 
					   MODEL_COL_ID_TYPE, MODEST_PROVIDER_COMBO_BOX_ID_PROVIDER,
					   -1);
	
			provider_ids_used_already = g_slist_prepend (
				provider_ids_used_already, (gpointer)g_strdup (provider_id));
		}
		
		++iter_provider_names;
		++iter_provider_ids;
	}
	
	/* Free the result of modest_presets_get_providers()
	 * as specified by its documentation: */
	g_strfreev (provider_names);
	g_strfreev (provider_ids);

	/* Add the provider protocols */
	registry = modest_runtime_get_protocol_registry ();
	provider_protos = modest_protocol_registry_get_by_tag (registry, 
							       MODEST_PROTOCOL_REGISTRY_PROVIDER_PROTOCOLS);
	tmp = provider_protos;
	while (tmp) {
		GtkTreeIter iter;
		ModestProtocol *proto = MODEST_PROTOCOL (tmp->data);

		/* only add store protocols, no need to duplicate them */
		if (modest_protocol_registry_protocol_type_has_tag (registry, 
								    modest_protocol_get_type_id (proto),
								    MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS)) {
			gboolean supported;

			supported = modest_account_protocol_is_supported (MODEST_ACCOUNT_PROTOCOL (proto));

			if (supported) {
				const gchar *name = modest_protocol_get_display_name (proto);

				gtk_list_store_append (liststore, &iter);
				gtk_list_store_set (liststore, &iter,
						    MODEL_COL_ID, modest_protocol_get_name (proto),
						    MODEL_COL_NAME, name,
						    MODEL_COL_ID_TYPE, MODEST_PROVIDER_COMBO_BOX_ID_PLUGIN_PROTOCOL,
						    -1);
				g_hash_table_insert (priv->enabled_plugin_ids, g_strdup (modest_protocol_get_name (proto)), NULL);
			}
		}
		tmp = g_slist_next (tmp);
	}
	g_slist_free (provider_protos);
	
	/* Add the "Other" item: */
	/* Note that ID 0 means "Other" for us: */
	gtk_list_store_prepend (liststore, &other_iter);
	gtk_list_store_set (liststore, &other_iter,
			    MODEL_COL_ID, 0,
			    MODEL_COL_NAME, _("mcen_va_serviceprovider_other"),
			    MODEL_COL_ID_TYPE, MODEST_PROVIDER_COMBO_BOX_ID_OTHER,
			    -1);

	/* Select the "Other" item: */
	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &other_iter);
	
	g_slist_foreach (provider_ids_used_already, (GFunc)g_free, NULL);
	g_slist_free (provider_ids_used_already);
}

void
modest_provider_combo_box_refresh (ModestProviderComboBox *self)
{	
	ModestProviderComboBoxPrivate *priv;
	GtkListStore *liststore;	
	GSList *provider_protos, *tmp;
	ModestProtocolRegistry *registry;

	g_return_if_fail (MODEST_IS_PROVIDER_COMBO_BOX(self));

	priv = MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE (self);
	liststore = GTK_LIST_STORE (priv->model);
	/* Add the provider protocols */
	registry = modest_runtime_get_protocol_registry ();
	provider_protos = modest_protocol_registry_get_by_tag (registry, 
							       MODEST_PROTOCOL_REGISTRY_PROVIDER_PROTOCOLS);
	for (tmp = provider_protos; tmp != NULL; tmp = g_slist_next (tmp)) {

		GtkTreeIter iter;
		ModestProtocol *proto = MODEST_PROTOCOL (tmp->data);
		const gchar *name = modest_protocol_get_display_name (proto);
		gboolean provider_exists;

		/* only add store protocols, no need to duplicate them */
		if (!modest_protocol_registry_protocol_type_has_tag (registry, 
								     modest_protocol_get_type_id (proto),
								     MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS))
			continue;
		  
		if (modest_protocol_registry_protocol_type_has_tag 
		    (registry,
		     modest_protocol_get_type_id (proto),
		     MODEST_PROTOCOL_REGISTRY_SINGLETON_PROVIDER_PROTOCOLS)) {
			/* Check if there's already an account configured with this account type */
			if (modest_account_mgr_singleton_protocol_exists (modest_runtime_get_account_mgr (),
									  modest_protocol_get_type_id (proto)))
				continue;
		}

		provider_exists = g_hash_table_lookup_extended (priv->enabled_plugin_ids, modest_protocol_get_name (proto),
								NULL, NULL);

		if (MODEST_ACCOUNT_PROTOCOL (proto) && 
		    !modest_account_protocol_is_supported (MODEST_ACCOUNT_PROTOCOL (proto))) {

			if (provider_exists) {
				GtkTreeIter iter;

				if (!gtk_tree_model_get_iter_first (priv->model, &iter))
					continue;

				do {
					const gchar *id;
					gtk_tree_model_get (priv->model, &iter, 
							    MODEL_COL_ID, id,
							    -1);

					if (g_strcmp0 (id, modest_protocol_get_name (proto)) == 0) {
						gtk_list_store_remove (GTK_LIST_STORE (priv->model), &iter);
						break;
					}

				} while (gtk_tree_model_iter_next (priv->model, &iter));
			}

			continue;
		}

		if (!provider_exists) {
			gtk_list_store_append (liststore, &iter);
			gtk_list_store_set (liststore, &iter,
					    MODEL_COL_ID, modest_protocol_get_name (proto),
					    MODEL_COL_NAME, name,
					    MODEL_COL_ID_TYPE, MODEST_PROVIDER_COMBO_BOX_ID_PLUGIN_PROTOCOL,
					    -1);
			g_hash_table_insert (priv->enabled_plugin_ids, g_strdup (modest_protocol_get_name (proto)), NULL);
		}
	}
	g_slist_free (provider_protos);
	
}

/**
 * Returns the MCC number of the selected provider, 
 * or NULL if no provider was selected, or "Other" was selected. 
 */
gchar*
modest_provider_combo_box_get_active_provider_id (ModestProviderComboBox *combobox)
{
	GtkTreeIter active;

	g_return_val_if_fail (MODEST_IS_PROVIDER_COMBO_BOX(combobox), NULL);

	const gboolean found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &active);
	if (found) {
		ModestProviderComboBoxPrivate *priv = MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE (combobox);

		gchar *id = NULL;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_ID, &id, -1);
		return g_strdup(id);	
	}

	return NULL; /* Failed. */
}

gchar*
modest_provider_combo_box_get_active_provider_label (ModestProviderComboBox *combobox)
{
	GtkTreeIter active;

	g_return_val_if_fail (MODEST_IS_PROVIDER_COMBO_BOX(combobox), NULL);

	const gboolean found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &active);
	if (found) {
		ModestProviderComboBoxPrivate *priv = MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE (combobox);

		gchar *label = NULL;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_NAME, &label, -1);
		return g_strdup(label);	
	}

	return NULL; /* Failed. */
}

void 
modest_provider_combo_box_set_others_provider (ModestProviderComboBox *combobox)
{
	GtkTreeModel *model;
	GtkTreeIter others_iter;

	g_return_if_fail (MODEST_IS_PROVIDER_COMBO_BOX(combobox));
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combobox));
	
	if (gtk_tree_model_get_iter_first (model, &others_iter))
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &others_iter);
}

ModestProviderComboBoxIdType 
modest_provider_combo_box_get_active_id_type (ModestProviderComboBox *combobox)
{
	GtkTreeIter active;

	g_return_val_if_fail (MODEST_IS_PROVIDER_COMBO_BOX (combobox), 
			      MODEST_PROVIDER_COMBO_BOX_ID_OTHER);

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &active)) {
		ModestProviderComboBoxPrivate *priv = MODEST_PROVIDER_COMBO_BOX_GET_PRIVATE (combobox);
		ModestProviderComboBoxIdType id_type;

		gtk_tree_model_get (priv->model, &active, MODEL_COL_ID_TYPE, &id_type, -1);
		return id_type;	
	} else {
		/* Fallback to other */
		return MODEST_PROVIDER_COMBO_BOX_ID_OTHER;
	}
}


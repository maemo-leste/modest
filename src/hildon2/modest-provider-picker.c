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

#include "modest-provider-picker.h"
#include <hildon/hildon-touch-selector-entry.h>
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

G_DEFINE_TYPE (ModestProviderPicker, modest_provider_picker, HILDON_TYPE_PICKER_BUTTON);

#define MODEST_PROVIDER_PICKER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_PROVIDER_PICKER, ModestProviderPickerPrivate))

typedef struct _ModestProviderPickerPrivate ModestProviderPickerPrivate;

struct _ModestProviderPickerPrivate
{
	GtkTreeModel *model;
};
static void
modest_provider_picker_finalize (GObject *object)
{
	ModestProviderPickerPrivate *priv = MODEST_PROVIDER_PICKER_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (modest_provider_picker_parent_class)->finalize (object);
}

static void
modest_provider_picker_class_init (ModestProviderPickerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestProviderPickerPrivate));

	object_class->finalize = modest_provider_picker_finalize;
}

enum MODEL_COLS {
	MODEL_COL_ID, /* a string, not an int. */
	MODEL_COL_NAME,
	MODEL_COL_ID_TYPE
};

static gint 
id_type_index (ModestProviderPickerIdType id_type)
{
	switch (id_type) {
	case MODEST_PROVIDER_PICKER_ID_OTHER:
		return 2;
		break;
	case MODEST_PROVIDER_PICKER_ID_PLUGIN_PROTOCOL:
		return 1;
		break;
	case MODEST_PROVIDER_PICKER_ID_PROVIDER:
	default:
		return 0;
	}
}

/*
 * strictly, we should sort providers with mcc=0 after the other ones.... but, we don't have
 * that info here, so ignoring for now.
 */
static gint
provider_sort_func (GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer user_data)
{
	gchar *prov1, *prov2;
	ModestProviderPickerIdType id_type1, id_type2;
	gint retval;
	
	gtk_tree_model_get (model, iter1, 
			    MODEL_COL_NAME, &prov1, 
			    MODEL_COL_ID_TYPE, &id_type1,
			    -1);
	gtk_tree_model_get (model, iter2, 
			    MODEL_COL_NAME, &prov2, 
			    MODEL_COL_ID_TYPE, &id_type2,
			    -1);

	retval = id_type_index (id_type2) - id_type_index (id_type1);
	if (retval != 0)
		goto end;

	if (strcmp (prov1, prov2) == 0) 
		retval = 0;
	else if (strcmp (_("mcen_va_serviceprovider_other"), prov1) == 0)
		retval = -1;
	else if (strcmp (_("mcen_va_serviceprovider_other"), prov2) == 0)
		retval = 1;
	else
		retval = modest_text_utils_utf8_strcmp (prov1, prov2, TRUE);
end:
	g_free (prov1);
	g_free (prov2);

	return retval;
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
modest_provider_picker_init (ModestProviderPicker *self)
{
	ModestProviderPickerPrivate *priv;

	priv = MODEST_PROVIDER_PICKER_GET_PRIVATE (self);
	priv->model = NULL;
}

ModestProviderPicker*
modest_provider_picker_new (HildonSizeType size,
			    HildonButtonArrangement arrangement)
{
	ModestProviderPickerPrivate *priv;
	ModestProviderPicker *self;
	GtkCellRenderer *renderer;
	GtkWidget *selector;
	HildonTouchSelectorColumn *column;

	self = g_object_new (MODEST_TYPE_PROVIDER_PICKER, 
			     "arrangement", arrangement,
			     "size", size,
			     NULL);
	priv = MODEST_PROVIDER_PICKER_GET_PRIVATE (self);

	/* Create the tree model for the selector,
	 * with a string for the name, and a string for the ID (e.g. "vodafone.it"), and the mcc
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(priv->model),
					      MODEL_COL_NAME, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(priv->model),
					  MODEL_COL_NAME,
					  (GtkTreeIterCompareFunc)provider_sort_func,
					  NULL, NULL);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	selector = hildon_touch_selector_entry_new ();
	hildon_touch_selector_set_print_func (HILDON_TOUCH_SELECTOR (selector), 
					      (HildonTouchSelectorPrintFunc) touch_selector_print_func);
	column = hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), GTK_TREE_MODEL (priv->model),
						      renderer, "text", MODEL_COL_NAME, NULL);
	hildon_touch_selector_entry_set_text_column (HILDON_TOUCH_SELECTOR_ENTRY (selector),
						     MODEL_COL_NAME);

	/* Set this _after_ loading from file, it makes loading faster */
	hildon_touch_selector_set_model (HILDON_TOUCH_SELECTOR (selector), 0, GTK_TREE_MODEL (priv->model));
	hildon_touch_selector_entry_set_input_mode (HILDON_TOUCH_SELECTOR_ENTRY (selector),
						    HILDON_GTK_INPUT_MODE_ALPHA |
						    HILDON_GTK_INPUT_MODE_SPECIAL |
						    HILDON_GTK_INPUT_MODE_NUMERIC |
						    HILDON_GTK_INPUT_MODE_AUTOCAP);


	hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (self), HILDON_TOUCH_SELECTOR (selector));
	modest_provider_picker_set_others_provider (MODEST_PROVIDER_PICKER (self));

	/* For theming purpouses. Widget name must end in Button-finger */
	gtk_widget_set_name ((GtkWidget *) self, "ModestProviderPickerButton-finger");

	return self;
}

void
modest_provider_picker_fill (ModestProviderPicker *self, 
			     ModestPresets *presets,
			     gint mcc)
{	
	GtkTreeIter other_iter;
	ModestProviderPickerPrivate *priv;
	GtkListStore *liststore;	
	GSList *provider_ids_used_already = NULL, *provider_protos, *tmp;
	gchar ** provider_ids = NULL;
	gchar ** provider_names;	
	gchar ** iter_provider_names;
	gchar ** iter_provider_ids;
	ModestProtocolRegistry *registry;
	GtkWidget *selector;

	g_return_if_fail (MODEST_IS_PROVIDER_PICKER(self));

	priv = MODEST_PROVIDER_PICKER_GET_PRIVATE (self);
	liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);
	provider_names = modest_presets_get_providers (presets, mcc, TRUE, &provider_ids);

	iter_provider_names = provider_names;
	iter_provider_ids = provider_ids;

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
					   MODEL_COL_ID_TYPE, MODEST_PROVIDER_PICKER_ID_PROVIDER,
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
	for (tmp = provider_protos; tmp != NULL; tmp = g_slist_next (tmp)) {

		GtkTreeIter iter;
		ModestProtocol *proto = MODEST_PROTOCOL (tmp->data);
		const gchar *name = modest_protocol_get_display_name (proto);

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

		if (MODEST_ACCOUNT_PROTOCOL (proto) && 
		    !modest_account_protocol_is_supported (MODEST_ACCOUNT_PROTOCOL (proto))) {
			continue;
		}

		gtk_list_store_append (liststore, &iter);
		gtk_list_store_set (liststore, &iter,
				    MODEL_COL_ID, modest_protocol_get_name (proto),
				    MODEL_COL_NAME, name,
				    MODEL_COL_ID_TYPE, MODEST_PROVIDER_PICKER_ID_PLUGIN_PROTOCOL,
				    -1);
	}
	g_slist_free (provider_protos);
	
	g_slist_foreach (provider_ids_used_already, (GFunc)g_free, NULL);
	g_slist_free (provider_ids_used_already);

	/* Add the "Other" item: */
	/* Note that ID 0 means "Other" for us: */
	gtk_list_store_prepend (liststore, &other_iter);
	gtk_list_store_set (liststore, &other_iter,
			    MODEL_COL_ID, 0,
			    MODEL_COL_NAME, _("mcen_va_serviceprovider_other"),
			    MODEL_COL_ID_TYPE, MODEST_PROVIDER_PICKER_ID_OTHER,
			    -1);

	/* Select the "Other" item: */
	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
	hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, &other_iter, TRUE);
	hildon_button_set_value (HILDON_BUTTON (self),
				 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
	
}

/**
 * Returns the MCC number of the selected provider, 
 * or NULL if no provider was selected, or "Other" was selected. 
 */
gchar*
modest_provider_picker_get_active_provider_id (ModestProviderPicker *self)
{
	GtkTreeIter active;
	gboolean found;
	GtkWidget *selector;

	g_return_val_if_fail (MODEST_IS_PROVIDER_PICKER(self), NULL);

	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
	found = hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &active);
	if (found) {
		ModestProviderPickerPrivate *priv = MODEST_PROVIDER_PICKER_GET_PRIVATE (self);

		gchar *id = NULL;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_ID, &id, -1);
		return g_strdup(id);	
	}

	return NULL; /* Failed. */
}

void 
modest_provider_picker_set_others_provider (ModestProviderPicker *self)
{
	GtkTreeModel *model;
	GtkTreeIter others_iter;
	GtkWidget *selector;

	g_return_if_fail (MODEST_IS_PROVIDER_PICKER(self));
	
	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (self)));
	model = hildon_touch_selector_get_model (HILDON_TOUCH_SELECTOR (selector), 0);
	
	if (gtk_tree_model_get_iter_first (model, &others_iter)) {
		hildon_touch_selector_select_iter (HILDON_TOUCH_SELECTOR (selector), 0, &others_iter, TRUE);
		hildon_button_set_value (HILDON_BUTTON (self),
					 hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
	}
}

ModestProviderPickerIdType 
modest_provider_picker_get_active_id_type (ModestProviderPicker *self)
{
	GtkTreeIter active;
	GtkWidget *selector;

	g_return_val_if_fail (MODEST_IS_PROVIDER_PICKER (self), 
			      MODEST_PROVIDER_PICKER_ID_OTHER);

	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON(self)));

	if (hildon_touch_selector_get_selected (HILDON_TOUCH_SELECTOR (selector), 0, &active)) {
		ModestProviderPickerPrivate *priv = MODEST_PROVIDER_PICKER_GET_PRIVATE (self);
		ModestProviderPickerIdType id_type;

		gtk_tree_model_get (priv->model, &active, MODEL_COL_ID_TYPE, &id_type, -1);
		return id_type;	
	} else {
		/* Fallback to other */
		return MODEST_PROVIDER_PICKER_ID_OTHER;
	}
}

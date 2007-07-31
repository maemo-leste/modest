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

#include "modest-easysetup-provider-combo-box.h"
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

G_DEFINE_TYPE (EasysetupProviderComboBox, easysetup_provider_combo_box, GTK_TYPE_COMBO_BOX);

#define PROVIDER_COMBO_BOX_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EASYSETUP_TYPE_PROVIDER_COMBO_BOX, EasysetupProviderComboBoxPrivate))

typedef struct _EasysetupProviderComboBoxPrivate EasysetupProviderComboBoxPrivate;

struct _EasysetupProviderComboBoxPrivate
{
	GtkTreeModel *model;
};

static void
easysetup_provider_combo_box_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
easysetup_provider_combo_box_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
easysetup_provider_combo_box_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (easysetup_provider_combo_box_parent_class)->dispose)
		G_OBJECT_CLASS (easysetup_provider_combo_box_parent_class)->dispose (object);
}

static void
easysetup_provider_combo_box_finalize (GObject *object)
{
	EasysetupProviderComboBoxPrivate *priv = PROVIDER_COMBO_BOX_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (easysetup_provider_combo_box_parent_class)->finalize (object);
}

static void
easysetup_provider_combo_box_class_init (EasysetupProviderComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EasysetupProviderComboBoxPrivate));

	object_class->get_property = easysetup_provider_combo_box_get_property;
	object_class->set_property = easysetup_provider_combo_box_set_property;
	object_class->dispose = easysetup_provider_combo_box_dispose;
	object_class->finalize = easysetup_provider_combo_box_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0,
	MODEL_COL_ID = 1 /* a string, not an int. */
};



static void
easysetup_provider_combo_box_init (EasysetupProviderComboBox *self)
{
	EasysetupProviderComboBoxPrivate *priv = PROVIDER_COMBO_BOX_GET_PRIVATE (self);

	/* Create a tree model for the combo box,
	 * with a string for the name, and a string for the ID (e.g. "vodafone.it").
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING));

	/* Setup the combo box: */
	GtkComboBox *combobox = GTK_COMBO_BOX (self);
	gtk_combo_box_set_model (combobox, priv->model);

	/* Provider column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer, 
	"text", MODEL_COL_NAME, NULL);
	
	/* The application should call easysetup_provider_combo_box_fill()
	 * to actually add some rows. */
}

EasysetupProviderComboBox*
easysetup_provider_combo_box_new (void)
{
	return g_object_new (EASYSETUP_TYPE_PROVIDER_COMBO_BOX, NULL);
}

void easysetup_provider_combo_box_fill (EasysetupProviderComboBox *combobox, ModestPresets *presets, GSList * list_country_id)
{	
	/* If the list is empty then use mmc=0 to get the providers for all countries: */
	GSList *list = list_country_id;
	GSList *fake_list = NULL;
	if (!list_country_id) {
		fake_list = g_slist_append(fake_list, GUINT_TO_POINTER(0));
		list = fake_list;
	}
	
		
	EasysetupProviderComboBoxPrivate *priv = PROVIDER_COMBO_BOX_GET_PRIVATE (combobox);
	
	/* Remove any existing rows: */
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);
	
	GSList *provider_ids_used_already = NULL;
	
	GSList *iter_ids = list;
	while (iter_ids) {
		const guint country_id = GPOINTER_TO_UINT (iter_ids->data);
		
		/* Add the appropriate rows for this country, from the presets file: */
		gchar ** provider_ids = NULL;
		gchar ** provider_names = modest_presets_get_providers (presets, country_id, 
			TRUE /* include_globals */, &provider_ids);
		
		gchar ** iter_provider_names = provider_names;
		gchar ** iter_provider_ids = provider_ids;
		while(iter_provider_names && *iter_provider_names && iter_provider_ids && *iter_provider_ids)
		{
			const gchar* provider_name = *iter_provider_names;
			if(!provider_name)
				continue;
				
			const gchar* provider_id = *iter_provider_ids;
			if(!provider_id)
				continue;
			
			/* Prevent duplicate providers: */
			if (g_slist_find_custom (provider_ids_used_already, 
				provider_id, (GCompareFunc)strcmp) == NULL) {
				/* printf("debug: provider_name=%s\n", provider_name); */
			
				/* Add the row: */
				GtkTreeIter iter;
				gtk_list_store_append (liststore, &iter);
				
				gtk_list_store_set(liststore, &iter, 
					MODEL_COL_ID, provider_id, 
					MODEL_COL_NAME, provider_name, -1);
				
				provider_ids_used_already = g_slist_append (
					provider_ids_used_already, (gpointer)g_strdup (provider_id));
			}
				
			++iter_provider_names;
			++iter_provider_ids;	
		}
		
		/* Free the result of modest_presets_get_providers()
		 * as specified by its documentation: */
		g_strfreev (provider_names);
		g_strfreev (provider_ids);
		
		iter_ids = g_slist_next (iter_ids);
	}
	
	/* Add the "Other" item: */
	/* Note that ID 0 means "Other" for us: */
	/* TODO: We need a Logical ID for this text. */
	GtkTreeIter iter;
	gtk_list_store_prepend (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_ID, 0, MODEL_COL_NAME, _("mcen_va_serviceprovider_other"), -1);
	
	/* Select the "Other" item: */
	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &iter);
	
	g_slist_foreach (provider_ids_used_already, (GFunc)g_free, NULL);
	g_slist_free (provider_ids_used_already);
	
	if (fake_list)
		g_slist_free (fake_list);
}

/**
 * Returns the MCC number of the selected provider, 
 * or NULL if no provider was selected, or "Other" was selected. 
 */
gchar*
easysetup_provider_combo_box_get_active_provider_id (EasysetupProviderComboBox *combobox)
{
	GtkTreeIter active;
	const gboolean found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &active);
	if (found) {
		EasysetupProviderComboBoxPrivate *priv = PROVIDER_COMBO_BOX_GET_PRIVATE (combobox);

		gchar *id = NULL;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_ID, &id, -1);
		return g_strdup(id);	
	}

	return NULL; /* Failed. */
}

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

#include <glib/gi18n.h>
#include "modest-account-view.h"

#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-text-utils.h>

#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkliststore.h>

/* 'private'/'protected' functions */
static void modest_account_view_class_init    (ModestAccountViewClass *klass);
static void modest_account_view_init          (ModestAccountView *obj);
static void modest_account_view_finalize      (GObject *obj);


typedef enum {
	MODEST_ACCOUNT_VIEW_NAME_COLUMN,
	MODEST_ACCOUNT_VIEW_DISPLAY_NAME_COLUMN,
	MODEST_ACCOUNT_VIEW_IS_ENABLED_COLUMN,
	MODEST_ACCOUNT_VIEW_IS_DEFAULT_COLUMN,
	MODEST_ACCOUNT_VIEW_PROTO_COLUMN,
	MODEST_ACCOUNT_VIEW_LAST_UPDATED_COLUMN,

	MODEST_ACCOUNT_VIEW_COLUMN_NUM
} AccountViewColumns;


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountViewPrivate ModestAccountViewPrivate;
struct _ModestAccountViewPrivate {
	ModestAccountMgr *account_mgr;
	gulong sig1, sig2;
	
};
#define MODEST_ACCOUNT_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_ACCOUNT_VIEW, \
                                                 ModestAccountViewPrivate))
/* globals */
static GtkTreeViewClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_account_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_account_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_account_view_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestAccountView",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_view_class_init (ModestAccountViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_view_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountViewPrivate));
}

static void
modest_account_view_init (ModestAccountView *obj)
{
 	ModestAccountViewPrivate *priv;

	priv = MODEST_ACCOUNT_VIEW_GET_PRIVATE(obj);
	
	priv->account_mgr = NULL; 
	priv->sig1 = 0;
	priv->sig2 = 0;
}

static void
modest_account_view_finalize (GObject *obj)
{
	ModestAccountViewPrivate *priv;

	priv = MODEST_ACCOUNT_VIEW_GET_PRIVATE(obj);

	if (priv->account_mgr) {
		if (priv->sig1)
			g_signal_handler_disconnect (priv->account_mgr, priv->sig1);

		if (priv->sig2)
			g_signal_handler_disconnect (priv->account_mgr, priv->sig2);

		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL; 
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



static void
update_account_view (ModestAccountMgr *account_mgr, ModestAccountView *view)
{
	GSList *account_names, *cursor;
	GtkListStore *model;
		
	model = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(view)));	
	gtk_list_store_clear (model);

	cursor = account_names = modest_account_mgr_account_names (account_mgr);
	
	if(account_names == NULL)
	{
	  printf ("debug: modest_account_mgr_account_names() returned  NULL\n");
	}

	while (cursor) {
		gchar *account_name;
		ModestAccountData *account_data;
		
		account_name = (gchar*)cursor->data;
		
		account_data = modest_account_mgr_get_account_data (account_mgr, account_name);
		if (!account_data) {
			g_printerr ("modest: failed to get account data for %s\n", account_name);
			continue;
		}

		/* don't display accounts without stores */
		if (account_data->store_account) {

			GtkTreeIter iter;
			time_t last_updated; 
			gchar *last_updated_string;
			
			/* FIXME: let's assume that 'last update' applies to the store account... */
			last_updated = account_data->store_account->last_updated;
			if (last_updated > 0) 
				last_updated_string = modest_text_utils_get_display_date(last_updated);
			else
				last_updated_string = g_strdup (_("Never"));
			
			gtk_list_store_insert_with_values (
				model, &iter, 0,
				MODEST_ACCOUNT_VIEW_NAME_COLUMN,          account_name,
				MODEST_ACCOUNT_VIEW_DISPLAY_NAME_COLUMN,  account_data->display_name,
				MODEST_ACCOUNT_VIEW_IS_ENABLED_COLUMN,    account_data->is_enabled,
				MODEST_ACCOUNT_VIEW_IS_DEFAULT_COLUMN,    account_data->is_default,

				MODEST_ACCOUNT_VIEW_PROTO_COLUMN,
				modest_protocol_info_get_protocol_name  (account_data->store_account->proto),

				MODEST_ACCOUNT_VIEW_LAST_UPDATED_COLUMN,  last_updated_string,
				-1);
			g_free (last_updated_string);
		}

		modest_account_mgr_free_account_data (account_mgr, account_data);
		cursor = cursor->next;
	}
	g_slist_free (account_names);
}


static void
on_account_changed (ModestAccountMgr *account_mgr,
		    const gchar* account, const gchar* key,
		    gboolean server_account, ModestAccountView *self)
{	
	update_account_view (account_mgr, self);
}


static void
on_account_removed (ModestAccountMgr *account_mgr,
		    const gchar* account, gboolean server_account,
		    ModestAccountView *self)
{
	on_account_changed (account_mgr, account, NULL, server_account, self);
}




static void
on_account_enable_toggled (GtkCellRendererToggle *cell_renderer, gchar *path,
			   ModestAccountView *self)
{
	GtkTreeIter iter;
	ModestAccountViewPrivate *priv;
	GtkTreeModel *model;
	gchar *account_name;
	gboolean enabled;
	
	priv = MODEST_ACCOUNT_VIEW_GET_PRIVATE(self);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(self));
	
	if (!gtk_tree_model_get_iter_from_string (model, &iter, path)) {
		g_printerr ("modest: cannot find iterator\n");
		return;
	}
	gtk_tree_model_get (model, &iter, MODEST_ACCOUNT_VIEW_IS_ENABLED_COLUMN, &enabled,
			    MODEST_ACCOUNT_VIEW_NAME_COLUMN, &account_name,
			    -1);
	
	/* toggle enabled / disabled */
	modest_account_mgr_set_enabled (priv->account_mgr, account_name, !enabled);
	g_free (account_name);
}


void
bold_if_default_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
			    GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data)
{
	gboolean is_default;
	gtk_tree_model_get (tree_model, iter, MODEST_ACCOUNT_VIEW_IS_DEFAULT_COLUMN,
			    &is_default, -1);
	g_object_set (G_OBJECT(renderer),
		      "weight", is_default ? 800: 400,
		      NULL);
}

static void
init_view (ModestAccountView *self)
{
	ModestAccountViewPrivate *priv;
	GtkCellRenderer *toggle_renderer, *text_renderer;
	GtkListStore *model;
	GtkTreeViewColumn *column;
	
	priv = MODEST_ACCOUNT_VIEW_GET_PRIVATE(self);
		
	model = gtk_list_store_new (6,
				    G_TYPE_STRING,  /* account name */
				    G_TYPE_STRING,  /* account display name */
				    G_TYPE_BOOLEAN, /* is-enabled */
				    G_TYPE_BOOLEAN, /* is-default */
				    G_TYPE_STRING,  /* account proto (pop, imap,...) */
				    G_TYPE_STRING   /* last updated (time_t) */
		); 

	gtk_tree_view_set_model (GTK_TREE_VIEW(self), GTK_TREE_MODEL(model));
	g_object_unref (G_OBJECT (model));

	toggle_renderer = gtk_cell_renderer_toggle_new ();
	text_renderer = gtk_cell_renderer_text_new ();

	/* the is_enabled column */
	g_object_set (G_OBJECT(toggle_renderer), "activatable", TRUE,"radio", FALSE, NULL);
	g_signal_connect (G_OBJECT(toggle_renderer), "toggled", G_CALLBACK(on_account_enable_toggled),
			  self);
	gtk_tree_view_append_column (GTK_TREE_VIEW(self),
				     gtk_tree_view_column_new_with_attributes (
					     _("Enabled"), toggle_renderer,
					     "active", MODEST_ACCOUNT_VIEW_IS_ENABLED_COLUMN, NULL));
	
	/* account name */
	column =  gtk_tree_view_column_new_with_attributes (_("Account"), text_renderer,"text",
							    MODEST_ACCOUNT_VIEW_DISPLAY_NAME_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(self),column);
	gtk_tree_view_column_set_cell_data_func(column, text_renderer, bold_if_default_cell_data,
						NULL, NULL);

	/* account type */
	column =  gtk_tree_view_column_new_with_attributes (_("Type"), text_renderer,"text",
							    MODEST_ACCOUNT_VIEW_PROTO_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(self),column);
	gtk_tree_view_column_set_cell_data_func(column, text_renderer, bold_if_default_cell_data,
						NULL, NULL);

	/* last update for this account */
	column =  gtk_tree_view_column_new_with_attributes (_("Last update"), text_renderer,"text",
							    MODEST_ACCOUNT_VIEW_LAST_UPDATED_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(self),column);
	gtk_tree_view_column_set_cell_data_func(column, text_renderer, bold_if_default_cell_data,
						NULL, NULL);

	priv->sig1 = g_signal_connect (G_OBJECT(priv->account_mgr),"account_removed",
				       G_CALLBACK(on_account_removed), self);
	priv->sig2 = g_signal_connect (G_OBJECT(priv->account_mgr), "account_changed",
				       G_CALLBACK(on_account_changed), self);
}



ModestAccountView*
modest_account_view_new (ModestAccountMgr *account_mgr)
{
	GObject *obj;
	ModestAccountViewPrivate *priv;
	
	g_return_val_if_fail (account_mgr, NULL);
	
	obj  = g_object_new(MODEST_TYPE_ACCOUNT_VIEW, NULL);
	priv = MODEST_ACCOUNT_VIEW_GET_PRIVATE(obj);
	
	g_object_ref (G_OBJECT (account_mgr));
	priv->account_mgr = account_mgr;

	init_view (MODEST_ACCOUNT_VIEW (obj));
	update_account_view (account_mgr, MODEST_ACCOUNT_VIEW (obj));
	
	return MODEST_ACCOUNT_VIEW (obj);
}

gchar *
modest_account_view_get_selected_account (ModestAccountView *self)
{
	gchar *account_name = NULL;
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_VIEW (self), NULL);
	
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
		gtk_tree_model_get (model, &iter, 
				    MODEST_ACCOUNT_VIEW_NAME_COLUMN, 
				    &account_name, -1);
	}

	return account_name;
}

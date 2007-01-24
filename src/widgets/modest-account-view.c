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

#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkliststore.h>

/* 'private'/'protected' functions */
static void modest_account_view_class_init    (ModestAccountViewClass *klass);
static void modest_account_view_init          (ModestAccountView *obj);
static void modest_account_view_finalize      (GObject *obj);


enum _AccountViewColumns {
	ENABLED_COLUMN,
	NAME_COLUMN,
	PROTO_COLUMN,
	N_COLUMNS
};
typedef enum _AccountViewColumns AccountViewColumns;


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
}

static void
modest_account_view_finalize (GObject *obj)
{
	ModestAccountViewPrivate *priv;

	priv = MODEST_ACCOUNT_VIEW_GET_PRIVATE(obj);

	g_signal_handler_disconnect (G_OBJECT(priv->account_mgr),
				     priv->sig1);
	g_signal_handler_disconnect (G_OBJECT(priv->account_mgr),
				     priv->sig2);

	if (priv->account_mgr) {
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

	cursor = account_names =
		modest_account_mgr_account_names (account_mgr, NULL);

	while (cursor) {
		gchar    *proto = NULL;
		gchar    *store, *account_name, *display_name;
		gboolean enabled;

		account_name = (gchar*)cursor->data;
		
		display_name = modest_account_mgr_get_string (account_mgr,
							      account_name,
							      MODEST_ACCOUNT_DISPLAY_NAME,
							      FALSE, NULL);
		/* don't display accounts without stores */
		if (display_name) {
			store = modest_account_mgr_get_string (account_mgr,
							       account_name,
							       MODEST_ACCOUNT_STORE_ACCOUNT,
							       FALSE, NULL);
			if (store) {
				proto = modest_account_mgr_get_string (account_mgr,
								       store,
								       MODEST_ACCOUNT_PROTO,
								       TRUE, NULL);
				g_free(store);
			}
		
			enabled = modest_account_mgr_account_get_enabled (account_mgr, account_name);
			gtk_list_store_insert_with_values (
				model, NULL, 0,
				ENABLED_COLUMN, enabled,
				NAME_COLUMN,  display_name,
				PROTO_COLUMN, proto,
				-1);
		}
		g_free (display_name);
		g_free (account_name);
		g_free (proto);
		
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
	
	gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &enabled,
			    NAME_COLUMN, &account_name,
			    -1);
	
	/* toggle enabled / disabled */
	modest_account_mgr_account_set_enabled (priv->account_mgr, account_name, !enabled);
	g_free (account_name);
}

static void
init_view (ModestAccountView *self)
{
	ModestAccountViewPrivate *priv;
	GtkCellRenderer *renderer;
	GtkListStore *model;
	
	priv = MODEST_ACCOUNT_VIEW_GET_PRIVATE(self);
		
	model = gtk_list_store_new (3,
				    G_TYPE_BOOLEAN, /* checkbox */
				    G_TYPE_STRING,  /* account name */
				    G_TYPE_STRING); /* account type (pop, imap,...) */

	gtk_tree_view_set_model (GTK_TREE_VIEW(self), GTK_TREE_MODEL(model));

	renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (G_OBJECT(renderer), "activatable", TRUE,"radio", FALSE, NULL);

	g_signal_connect (G_OBJECT(renderer), "toggled",
			  G_CALLBACK(on_account_enable_toggled),
			  self);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW(self),
				     gtk_tree_view_column_new_with_attributes (
					     _("Enabled"), renderer,
					     "active", ENABLED_COLUMN, NULL));
	gtk_tree_view_append_column (GTK_TREE_VIEW(self),
				     gtk_tree_view_column_new_with_attributes (
					     _("Account"),
					     gtk_cell_renderer_text_new (),
					     "text", NAME_COLUMN, NULL));
	gtk_tree_view_append_column (GTK_TREE_VIEW(self),
				     gtk_tree_view_column_new_with_attributes (
					     _("Type"),
					     gtk_cell_renderer_text_new (),
					     "text", PROTO_COLUMN, NULL));

	priv->sig1 = g_signal_connect (G_OBJECT(priv->account_mgr),
				       "account_removed",
				       G_CALLBACK(on_account_removed), self);
	priv->sig2 = g_signal_connect (G_OBJECT(priv->account_mgr),
				       "account_changed",
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
	
	g_object_ref (G_OBJECT(account_mgr));
	priv->account_mgr = account_mgr;

	init_view (MODEST_ACCOUNT_VIEW (obj));
	update_account_view (account_mgr, MODEST_ACCOUNT_VIEW(obj));
	
	return MODEST_ACCOUNT_VIEW(obj);
}

const gchar *
modest_account_view_get_selected_account (ModestAccountView *self)
{
	const gchar *account_name = NULL;
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_VIEW (self), NULL);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    NAME_COLUMN, &account_name,
				    -1);
	}

	return account_name;
}

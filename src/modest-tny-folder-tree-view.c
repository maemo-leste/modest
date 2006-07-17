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
#include <string.h>

#include <tny-account-tree-model.h>
#include <tny-account-store-iface.h>
#include <tny-account-iface.h>
#include <tny-msg-folder-iface.h>
#include <tny-summary-window-iface.h>

#include "modest-tny-folder-tree-view.h"

#include <modest-icon-names.h>
#include "modest-icon-factory.h"


/* 'private'/'protected' functions */
static void modest_tny_folder_tree_view_class_init  (ModestTnyFolderTreeViewClass *klass);
static void modest_tny_folder_tree_view_init        (ModestTnyFolderTreeView *obj);
static void modest_tny_folder_tree_view_finalize    (GObject *obj);

//static void modest_tny_folder_tree_view_iface_init   (gpointer iface, gpointer data);
static void modest_tny_folder_tree_view_set_account_store (TnySummaryWindowIface *self,
							   TnyAccountStoreIface *account_store);
static gboolean update_model (ModestTnyFolderTreeView *self,TnyAccountStoreIface *iface);
static gboolean update_model_empty (ModestTnyFolderTreeView *self);

static void selection_changed (GtkTreeSelection *sel, gpointer data);

enum {
	FOLDER_SELECTED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyFolderTreeViewPrivate ModestTnyFolderTreeViewPrivate;
struct _ModestTnyFolderTreeViewPrivate {

	TnyAccountStoreIface *tny_account_store;
	TnyMsgFolderIface *cur_folder;
	gboolean view_is_empty;

	GMutex *lock;
};
#define MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(o)			\
	(G_TYPE_INSTANCE_GET_PRIVATE((o),				\
				     MODEST_TYPE_TNY_FOLDER_TREE_VIEW,	\
				     ModestTnyFolderTreeViewPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0}; 

GType
modest_tny_folder_tree_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyFolderTreeViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_folder_tree_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyFolderTreeView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_folder_tree_view_init,
		};
				
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestTnyFolderTreeView",
		                                  &my_info, 0);		
	}
	return my_type;
}

static void
modest_tny_folder_tree_view_class_init (ModestTnyFolderTreeViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_folder_tree_view_finalize;
	
	klass->update_model = modest_tny_folder_tree_view_update_model;

	g_type_class_add_private (gobject_class,
				  sizeof(ModestTnyFolderTreeViewPrivate));
	
 	signals[FOLDER_SELECTED_SIGNAL] = 
		g_signal_new ("folder_selected",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestTnyFolderTreeViewClass,
					       folder_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER); 
}



static void
text_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		   GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer data)
{
	GObject *rendobj;
	gchar *fname;
	gint unread;
	TnyMsgFolderType type;
	
	gtk_tree_model_get (tree_model, iter,
			    TNY_ACCOUNT_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_ACCOUNT_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_ACCOUNT_TREE_MODEL_UNREAD_COLUMN, &unread, -1);
	rendobj = G_OBJECT(renderer);

	if (unread > 0) {
		gchar *folder_title = g_strdup_printf ("%s (%d)", fname, unread);
		g_object_set (rendobj,"text", folder_title,  "weight", 800, NULL);
		g_free (folder_title);
	} else 
		g_object_set (rendobj,"text", fname, "weight", 400, NULL);
		
	g_free (fname);
}

/* FIXME: move these to TnyMail */
enum {

	TNY_MSG_FOLDER_TYPE_NOTES = TNY_MSG_FOLDER_TYPE_SENT + 1, /* urgh */
	TNY_MSG_FOLDER_TYPE_DRAFTS,
	TNY_MSG_FOLDER_TYPE_CONTACTS,
	TNY_MSG_FOLDER_TYPE_CALENDAR
};
	
static TnyMsgFolderType
guess_folder_type (const gchar* name)
{
	TnyMsgFolderType type;
	gchar *folder;

	g_return_val_if_fail (name, TNY_MSG_FOLDER_TYPE_NORMAL);
	
	type = TNY_MSG_FOLDER_TYPE_NORMAL;
	folder = g_utf8_strdown (name, strlen(name));

	if (strcmp (folder, "inbox") == 0 ||
	    strcmp (folder, _("inbox")) == 0)
		type = TNY_MSG_FOLDER_TYPE_INBOX;
	else if (strcmp (folder, "outbox") == 0 ||
		 strcmp (folder, _("outbox")) == 0)
		type = TNY_MSG_FOLDER_TYPE_OUTBOX;
	else if (g_str_has_prefix(folder, "junk") ||
		 g_str_has_prefix(folder, _("junk")))
		type = TNY_MSG_FOLDER_TYPE_JUNK;
	else if (g_str_has_prefix(folder, "trash") ||
		 g_str_has_prefix(folder, _("trash")))
		type = TNY_MSG_FOLDER_TYPE_JUNK;
	else if (g_str_has_prefix(folder, "sent") ||
		 g_str_has_prefix(folder, _("sent")))
		type = TNY_MSG_FOLDER_TYPE_SENT;

	/* these are not *really* TNY_ types */
	else if (g_str_has_prefix(folder, "draft") ||
		 g_str_has_prefix(folder, _("draft")))
		type = TNY_MSG_FOLDER_TYPE_DRAFTS;
	else if (g_str_has_prefix(folder, "notes") ||
		 g_str_has_prefix(folder, _("notes")))
		type = TNY_MSG_FOLDER_TYPE_NOTES;
	else if (g_str_has_prefix(folder, "contacts") ||
		 g_str_has_prefix(folder, _("contacts")))
		type = TNY_MSG_FOLDER_TYPE_CONTACTS;
	else if (g_str_has_prefix(folder, "calendar") ||
		 g_str_has_prefix(folder, _("calendar")))
		type = TNY_MSG_FOLDER_TYPE_CALENDAR;
	
	g_free (folder);
	return type;
}


static void
icon_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,  GtkTreeIter *iter, gpointer data)
{
	GObject *rendobj;
	GdkPixbuf *pixbuf;
	TnyMsgFolderType type;
	gchar *fname = NULL;
	gint unread;
	
	rendobj = G_OBJECT(renderer);
	gtk_tree_model_get (tree_model, iter,
			    TNY_ACCOUNT_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_ACCOUNT_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_ACCOUNT_TREE_MODEL_UNREAD_COLUMN, &unread, -1);
	rendobj = G_OBJECT(renderer);
	
	if (type == TNY_MSG_FOLDER_TYPE_NORMAL)
		type = guess_folder_type (fname);
	
	if (fname);
		g_free (fname);

	switch (type) {
        case TNY_MSG_FOLDER_TYPE_INBOX:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_INBOX);
                break;
        case TNY_MSG_FOLDER_TYPE_OUTBOX:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_OUTBOX);
                break;
        case TNY_MSG_FOLDER_TYPE_JUNK:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_JUNK);
                break;
        case TNY_MSG_FOLDER_TYPE_SENT:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_SENT);
                break;
	case TNY_MSG_FOLDER_TYPE_DRAFTS:
		pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_DRAFTS);
                break;
	case TNY_MSG_FOLDER_TYPE_NOTES:
		pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_NOTES);
                break;
	case TNY_MSG_FOLDER_TYPE_CALENDAR:
		pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_CALENDAR);
                break;
	case TNY_MSG_FOLDER_TYPE_CONTACTS:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_CONTACTS);
                break;
	case TNY_MSG_FOLDER_TYPE_NORMAL:
        default:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_NORMAL);
                break;
        }

	g_object_set (rendobj,
		      "pixbuf-expander-open",
		      modest_icon_factory_get_icon (MODEST_FOLDER_ICON_OPEN),
		      "pixbuf-expander-closed",
		      modest_icon_factory_get_icon (MODEST_FOLDER_ICON_CLOSED),
		      "pixbuf", pixbuf,
		      NULL);
}

static void
modest_tny_folder_tree_view_init (ModestTnyFolderTreeView *obj)
{
	ModestTnyFolderTreeViewPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;
	
	priv =	MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(obj);
	
	priv->view_is_empty     = TRUE;
	priv->tny_account_store = NULL;
	priv->cur_folder = NULL;

	priv->lock = g_mutex_new ();
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column,
					_("All Mail Folders"));
	
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj),
				     column);
	
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						icon_cell_data, NULL, NULL);
	
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						text_cell_data, NULL, NULL);
	
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(obj));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

	gtk_tree_view_column_set_spacing (column, 2);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_fixed_width (column, TRUE);		
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(obj), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(obj), FALSE);

}


static void
modest_tny_folder_tree_view_finalize (GObject *obj)
{
	ModestTnyFolderTreeViewPrivate *priv;

	g_return_if_fail (obj);
	
	priv =	MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(obj);
	if (priv->tny_account_store) {
		g_object_unref (G_OBJECT(priv->tny_account_store));
		priv->tny_account_store = NULL;
	}


	if (priv->lock) {
		g_mutex_free (priv->lock);
		priv->lock = NULL;
	}
	
	(*parent_class->finalize)(obj);
}


static void
modest_tny_folder_tree_view_set_account_store (TnySummaryWindowIface *self,
					       TnyAccountStoreIface *account_store)
{
	ModestTnyFolderTreeViewPrivate *priv;

	g_return_if_fail (self);
	g_return_if_fail (account_store);
	
	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);
	if (priv->tny_account_store) {
		g_object_unref (priv->tny_account_store);
		priv->tny_account_store = NULL;
	}

	g_object_ref (G_OBJECT(priv->tny_account_store = account_store));
}



static void
on_accounts_update (TnyAccountStoreIface *account_store, const gchar *account,
		    gpointer user_data)
{
	update_model_empty (MODEST_TNY_FOLDER_TREE_VIEW(user_data));
	
	if (!update_model (MODEST_TNY_FOLDER_TREE_VIEW(user_data), account_store))
		g_printerr ("modest: failed to update model for changes in '%s'",
			    account);
}


GtkWidget*
modest_tny_folder_tree_view_new (TnyAccountStoreIface *account_store)
{
	GObject *self;
	ModestTnyFolderTreeViewPrivate *priv;
	GtkTreeSelection *sel;

	self = G_OBJECT(g_object_new(MODEST_TYPE_TNY_FOLDER_TREE_VIEW, NULL));
	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);

	g_return_val_if_fail (account_store, NULL);
	
	if (!update_model (MODEST_TNY_FOLDER_TREE_VIEW(self), account_store))
		g_printerr ("modest: failed to update model");

	g_signal_connect (G_OBJECT(account_store), "update_accounts",
			  G_CALLBACK (on_accounts_update), self);
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	g_signal_connect (sel, "changed",
			  G_CALLBACK(selection_changed), self);
		
	return self;
}




static gboolean
update_model_empty (ModestTnyFolderTreeView *self)
{
	GtkTreeIter  iter;
	GtkTreeStore *store;
	ModestTnyFolderTreeViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	
	store = gtk_tree_store_new (1, G_TYPE_STRING);
	gtk_tree_store_append (store, &iter, NULL);

	gtk_tree_store_set (store, &iter, 0,
			    _("(empty)"), -1);

	gtk_tree_view_set_model (GTK_TREE_VIEW(self),
				 GTK_TREE_MODEL(store));
	g_object_unref (store);

	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);
	priv->view_is_empty = TRUE;
	
	return TRUE;
}


static gboolean
update_model (ModestTnyFolderTreeView *self, TnyAccountStoreIface *account_store)
{
	ModestTnyFolderTreeViewPrivate *priv;
	TnyListIface     *account_list;
	GtkTreeModel     *model, *sortable;
	
	g_return_val_if_fail (account_store, FALSE);
	
	priv =	MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);

	model        = GTK_TREE_MODEL(tny_account_tree_model_new ());
	account_list = TNY_LIST_IFACE(model);

	update_model_empty (self); /* cleanup */
	priv->view_is_empty = TRUE;
	
	tny_account_store_iface_get_accounts (account_store, account_list,
					      TNY_ACCOUNT_STORE_IFACE_STORE_ACCOUNTS);
	if (!account_list) /* no store accounts found */ 
		return TRUE;
	
	sortable = gtk_tree_model_sort_new_with_model (model);
	gtk_tree_view_set_model (GTK_TREE_VIEW(self), sortable);

	priv->view_is_empty = FALSE;	
	g_object_unref (model);

	return TRUE;
} 


void
selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel            *model;
	TnyMsgFolderIface       *folder = NULL;
	GtkTreeIter             iter;
	ModestTnyFolderTreeView *tree_view;
	ModestTnyFolderTreeViewPrivate *priv;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);

	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(user_data);

	/* is_empty means that there is only the 'empty' item */
	if (priv->view_is_empty)
		return;
	
	/* folder was _un_selected if true */
	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		priv->cur_folder = NULL;
		return; 
	}

	tree_view = MODEST_TNY_FOLDER_TREE_VIEW (user_data);

	gtk_tree_model_get (model, &iter,
			    TNY_ACCOUNT_TREE_MODEL_INSTANCE_COLUMN,
			    &folder, -1);

	if (priv->cur_folder) 
		tny_msg_folder_iface_expunge (priv->cur_folder);
	priv->cur_folder = folder;

	/* folder will not be defined if you click eg. on the root node */
	if (folder)
		g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTED_SIGNAL], 0,
			       folder);
}


gboolean
modest_tny_folder_tree_view_update_model(ModestTnyFolderTreeView *self, 
                                         TnyAccountStoreIface *iface)
{
	g_return_val_if_fail (MODEST_IS_TNY_FOLDER_TREE_VIEW (self), FALSE);
	
	return update_model (self, iface);
}

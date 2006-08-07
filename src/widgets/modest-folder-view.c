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
#include <tny-folder-iface.h>
#include <modest-icon-names.h>
#include <modest-icon-factory.h>
#include <modest-tny-account-store.h>

#include "modest-folder-view.h"


/* 'private'/'protected' functions */
static void modest_folder_view_class_init  (ModestFolderViewClass *klass);
static void modest_folder_view_init        (ModestFolderView *obj);
static void modest_folder_view_finalize    (GObject *obj);

static gboolean update_model (ModestFolderView *self,
			      ModestTnyAccountStore *account_store);
static gboolean update_model_empty (ModestFolderView *self);
static void on_selection_changed (GtkTreeSelection *sel, gpointer data);
static gboolean modest_folder_view_update_model (ModestFolderView *self,
						 TnyAccountStoreIface *account_store);

enum {
	FOLDER_SELECTED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestFolderViewPrivate ModestFolderViewPrivate;
struct _ModestFolderViewPrivate {

	TnyAccountStoreIface *account_store;
	TnyFolderIface       *cur_folder;
	gboolean             view_is_empty;

	gulong               sig1, sig2;
	GMutex               *lock;
};
#define MODEST_FOLDER_VIEW_GET_PRIVATE(o)			        \
	(G_TYPE_INSTANCE_GET_PRIVATE((o),				\
				     MODEST_TYPE_FOLDER_VIEW,	        \
				     ModestFolderViewPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0}; 

GType
modest_folder_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestFolderViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_folder_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestFolderView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_folder_view_init,
			NULL
		};
				
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestFolderView",
		                                  &my_info, 0);		
	}
	return my_type;
}

static void
modest_folder_view_class_init (ModestFolderViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_folder_view_finalize;
	
	klass->update_model = modest_folder_view_update_model;

	g_type_class_add_private (gobject_class,
				  sizeof(ModestFolderViewPrivate));
	
 	signals[FOLDER_SELECTED_SIGNAL] = 
		g_signal_new ("folder_selected",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestFolderViewClass,
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
	TnyFolderType type;
	
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

	TNY_FOLDER_TYPE_NOTES = TNY_FOLDER_TYPE_SENT + 1, /* urgh */
	TNY_FOLDER_TYPE_DRAFTS,
	TNY_FOLDER_TYPE_CONTACTS,
	TNY_FOLDER_TYPE_CALENDAR
};
	
static TnyFolderType
guess_folder_type (const gchar* name)
{
	TnyFolderType type;
	gchar *folder;

	g_return_val_if_fail (name, TNY_FOLDER_TYPE_NORMAL);
	
	type = TNY_FOLDER_TYPE_NORMAL;
	folder = g_utf8_strdown (name, strlen(name));

	if (strcmp (folder, "inbox") == 0 ||
	    strcmp (folder, _("inbox")) == 0)
		type = TNY_FOLDER_TYPE_INBOX;
	else if (strcmp (folder, "outbox") == 0 ||
		 strcmp (folder, _("outbox")) == 0)
		type = TNY_FOLDER_TYPE_OUTBOX;
	else if (g_str_has_prefix(folder, "junk") ||
		 g_str_has_prefix(folder, _("junk")))
		type = TNY_FOLDER_TYPE_JUNK;
	else if (g_str_has_prefix(folder, "trash") ||
		 g_str_has_prefix(folder, _("trash")))
		type = TNY_FOLDER_TYPE_JUNK;
	else if (g_str_has_prefix(folder, "sent") ||
		 g_str_has_prefix(folder, _("sent")))
		type = TNY_FOLDER_TYPE_SENT;

	/* these are not *really* TNY_ types */
	else if (g_str_has_prefix(folder, "draft") ||
		 g_str_has_prefix(folder, _("draft")))
		type = TNY_FOLDER_TYPE_DRAFTS;
	else if (g_str_has_prefix(folder, "notes") ||
		 g_str_has_prefix(folder, _("notes")))
		type = TNY_FOLDER_TYPE_NOTES;
	else if (g_str_has_prefix(folder, "contacts") ||
		 g_str_has_prefix(folder, _("contacts")))
		type = TNY_FOLDER_TYPE_CONTACTS;
	else if (g_str_has_prefix(folder, "calendar") ||
		 g_str_has_prefix(folder, _("calendar")))
		type = TNY_FOLDER_TYPE_CALENDAR;
	
	g_free (folder);
	return type;
}


static void
icon_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,  GtkTreeIter *iter, gpointer data)
{
	GObject *rendobj;
	GdkPixbuf *pixbuf;
	TnyFolderType type;
	gchar *fname = NULL;
	gint unread;
	
	rendobj = G_OBJECT(renderer);
	gtk_tree_model_get (tree_model, iter,
			    TNY_ACCOUNT_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_ACCOUNT_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_ACCOUNT_TREE_MODEL_UNREAD_COLUMN, &unread, -1);
	rendobj = G_OBJECT(renderer);
	
	if (type == TNY_FOLDER_TYPE_NORMAL)
		type = guess_folder_type (fname);
	
	if (fname)
		g_free (fname);

	switch (type) {
        case TNY_FOLDER_TYPE_INBOX:
                pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_INBOX);
                break;
        case TNY_FOLDER_TYPE_OUTBOX:
                pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_OUTBOX);
                break;
        case TNY_FOLDER_TYPE_JUNK:
                pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_JUNK);
                break;
        case TNY_FOLDER_TYPE_SENT:
                pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_SENT);
                break;
	case TNY_FOLDER_TYPE_DRAFTS:
		pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_DRAFTS);
                break;
	case TNY_FOLDER_TYPE_NOTES:
		pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_NOTES);
                break;
	case TNY_FOLDER_TYPE_CALENDAR:
		pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_CALENDAR);
                break;
	case TNY_FOLDER_TYPE_CONTACTS:
                pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_CONTACTS);
                break;
	case TNY_FOLDER_TYPE_NORMAL:
        default:
                pixbuf = modest_icon_factory_get_small_icon (MODEST_FOLDER_ICON_NORMAL);
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
modest_folder_view_init (ModestFolderView *obj)
{
	ModestFolderViewPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;
	
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(obj);
	
	priv->view_is_empty     = TRUE;
	priv->account_store = NULL;
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
modest_folder_view_finalize (GObject *obj)
{
	ModestFolderViewPrivate *priv;
	GtkTreeSelection    *sel;
	
	g_return_if_fail (obj);
	
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(obj);
	if (priv->account_store) {
		g_signal_handler_disconnect (G_OBJECT(priv->account_store),
					     priv->sig1);
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}

	if (priv->lock) {
		g_mutex_free (priv->lock);
		priv->lock = NULL;
	}

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(obj));
	if (sel)
		g_signal_handler_disconnect (G_OBJECT(sel), priv->sig2);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void
on_account_update (TnyAccountStoreIface *account_store, const gchar *account,
		   gpointer user_data)
{
	update_model_empty (MODEST_FOLDER_VIEW(user_data));
	
	if (!update_model (MODEST_FOLDER_VIEW(user_data), 
			   MODEST_TNY_ACCOUNT_STORE(account_store)))
		g_printerr ("modest: failed to update model for changes in '%s'",
			    account);
}


GtkWidget*
modest_folder_view_new (ModestTnyAccountStore *account_store)
{
	GObject *self;
	ModestFolderViewPrivate *priv;
	GtkTreeSelection *sel;
	
	g_return_val_if_fail (account_store, NULL);
	
	self = G_OBJECT(g_object_new(MODEST_TYPE_FOLDER_VIEW, NULL));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);
	
	if (!update_model (MODEST_FOLDER_VIEW(self), 
			   MODEST_TNY_ACCOUNT_STORE(account_store)))
		g_printerr ("modest: failed to update model");
	
	priv->sig1 = g_signal_connect (G_OBJECT(account_store), "account_update",
				       G_CALLBACK (on_account_update), self);	
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	priv->sig2 = g_signal_connect (sel, "changed",
				       G_CALLBACK(on_selection_changed), self);
				     	
	return GTK_WIDGET(self);
}




static gboolean
update_model_empty (ModestFolderView *self)
{
	GtkTreeIter  iter;
	GtkTreeStore *store;
	ModestFolderViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	
	store = gtk_tree_store_new (1, G_TYPE_STRING);
	gtk_tree_store_append (store, &iter, NULL);

	gtk_tree_store_set (store, &iter, 0,
			    _("(empty)"), -1);

	gtk_tree_view_set_model (GTK_TREE_VIEW(self),
				 GTK_TREE_MODEL(store));
	g_object_unref (store);

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);
	priv->view_is_empty = TRUE;

	g_signal_emit (G_OBJECT(self), signals[FOLDER_SELECTED_SIGNAL], 0,
		       NULL);
	
	return TRUE;
}


static gboolean
update_model (ModestFolderView *self, ModestTnyAccountStore *account_store)
{
	ModestFolderViewPrivate *priv;
	TnyListIface     *account_list;
	GtkTreeModel     *model, *sortable;
	
	g_return_val_if_fail (account_store, FALSE);
	
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	model        = GTK_TREE_MODEL(tny_account_tree_model_new ());
	account_list = TNY_LIST_IFACE(model);

	update_model_empty (self); /* cleanup */
	priv->view_is_empty = TRUE;
	
	tny_account_store_iface_get_accounts (TNY_ACCOUNT_STORE_IFACE(account_store),
					      account_list,
					      TNY_ACCOUNT_STORE_IFACE_STORE_ACCOUNTS);
	if (!account_list) /* no store accounts found */ 
		return TRUE;
	
	sortable = gtk_tree_model_sort_new_with_model (model);
	gtk_tree_view_set_model (GTK_TREE_VIEW(self), sortable);

	priv->view_is_empty = FALSE;	
	g_object_unref (model);

	return TRUE;
} 


static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel            *model;
	TnyFolderIface       *folder = NULL;
	GtkTreeIter             iter;
	ModestFolderView *tree_view;
	ModestFolderViewPrivate *priv;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(user_data);

	/* is_empty means that there is only the 'empty' item */
	if (priv->view_is_empty)
		return;
	
	/* folder was _un_selected if true */
	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		priv->cur_folder = NULL; /* FIXME: need this? */
		return; 
	}

	tree_view = MODEST_FOLDER_VIEW (user_data);

	gtk_tree_model_get (model, &iter,
			    TNY_ACCOUNT_TREE_MODEL_INSTANCE_COLUMN,
			    &folder, -1);

	if (priv->cur_folder) 
		tny_folder_iface_expunge (priv->cur_folder);
	priv->cur_folder = folder;

	/* folder will not be defined if you click eg. on the root node */
	if (folder)
		g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTED_SIGNAL], 0,
			       folder);
}


static gboolean
modest_folder_view_update_model (ModestFolderView *self, TnyAccountStoreIface *account_store)
{
	gboolean retval;
	
	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);
	retval = update_model (self, MODEST_TNY_ACCOUNT_STORE(account_store)); /* ugly */

	g_signal_emit (G_OBJECT(self), signals[FOLDER_SELECTED_SIGNAL],
		       0, NULL);

	return retval;
}

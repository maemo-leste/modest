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
#include <gdk/gdkkeysyms.h>
#include <tny-account-store-view.h>
#include <tny-gtk-account-list-model.h>
#include <tny-gtk-folder-store-tree-model.h>
#include <tny-gtk-header-list-model.h>
#include <tny-folder.h>
#include <tny-folder-store-observer.h>
#include <tny-account-store.h>
#include <tny-account.h>
#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-simple-list.h>
#include <modest-tny-account.h>
#include <modest-tny-folder.h>
#include <modest-tny-local-folders-account.h>
#include <modest-tny-outbox-account.h>
#include <modest-marshal.h>
#include <modest-icon-names.h>
#include <modest-tny-account-store.h>
#include <modest-text-utils.h>
#include <modest-runtime.h>
#include "modest-folder-view.h"
#include <modest-dnd.h>
#include <modest-platform.h>
#include <modest-widget-memory.h>
#include <modest-ui-actions.h>

/* 'private'/'protected' functions */
static void modest_folder_view_class_init  (ModestFolderViewClass *klass);
static void modest_folder_view_init        (ModestFolderView *obj);
static void modest_folder_view_finalize    (GObject *obj);

static void         tny_account_store_view_init (gpointer g, 
						 gpointer iface_data);

static void         modest_folder_view_set_account_store (TnyAccountStoreView *self, 
							  TnyAccountStore     *account_store);

static void         on_selection_changed   (GtkTreeSelection *sel, gpointer data);

static void         on_account_removed     (TnyAccountStore *self, 
					    TnyAccount *account,
					    gpointer user_data);

static void         on_account_inserted    (TnyAccountStore *self, 
					    TnyAccount *account,
					    gpointer user_data);

static void         on_account_changed    (TnyAccountStore *self, 
					    TnyAccount *account,
					    gpointer user_data);

static gint         cmp_rows               (GtkTreeModel *tree_model, 
					    GtkTreeIter *iter1, 
					    GtkTreeIter *iter2,
					    gpointer user_data);

static gboolean     filter_row             (GtkTreeModel *model,
					    GtkTreeIter *iter,
					    gpointer data);

static gboolean     on_key_pressed         (GtkWidget *self,
					    GdkEventKey *event,
					    gpointer user_data);

static void         on_configuration_key_changed  (ModestConf* conf, 
						   const gchar *key, 
						   ModestConfEvent event,
						   ModestConfNotificationId notification_id, 
						   ModestFolderView *self);

/* DnD functions */
static void         on_drag_data_get       (GtkWidget *widget, 
					    GdkDragContext *context, 
					    GtkSelectionData *selection_data, 
					    guint info, 
					    guint time, 
					    gpointer data);

static void         on_drag_data_received  (GtkWidget *widget, 
					    GdkDragContext *context, 
					    gint x, 
					    gint y, 
					    GtkSelectionData *selection_data, 
					    guint info, 
					    guint time, 
					    gpointer data);

static gboolean     on_drag_motion         (GtkWidget      *widget,
					    GdkDragContext *context,
					    gint            x,
					    gint            y,
					    guint           time,
					    gpointer        user_data);

static void         expand_root_items (ModestFolderView *self);

static gint         expand_row_timeout     (gpointer data);

static void         setup_drag_and_drop    (GtkTreeView *self);

static gboolean     _clipboard_set_selected_data (ModestFolderView *folder_view, 
						  gboolean delete);

static void         _clear_hidding_filter (ModestFolderView *folder_view);

static void         on_row_inserted_maybe_select_folder (GtkTreeModel     *tree_model, 
							 GtkTreePath      *path, 
							 GtkTreeIter      *iter,
							 ModestFolderView *self);

enum {
	FOLDER_SELECTION_CHANGED_SIGNAL,
	FOLDER_DISPLAY_NAME_CHANGED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestFolderViewPrivate ModestFolderViewPrivate;
struct _ModestFolderViewPrivate {
	TnyAccountStore      *account_store;
	TnyFolderStore       *cur_folder_store;

	TnyFolder            *folder_to_select; /* folder to select after the next update */

	ModestConfNotificationId notification_id;

	gulong                changed_signal;
	gulong                account_inserted_signal;
	gulong                account_removed_signal;
	gulong		      account_changed_signal;
	gulong                conf_key_signal;
	
	/* not unref this object, its a singlenton */
	ModestEmailClipboard *clipboard;

	/* Filter tree model */
	gchar **hidding_ids;
	guint n_selected;

	TnyFolderStoreQuery  *query;
	guint                 timer_expander;

	gchar                *local_account_name;
	gchar                *visible_account_id;
	ModestFolderViewStyle style;

	gboolean  reselect; /* we use this to force a reselection of the INBOX */
	gboolean  show_non_move;
	gboolean  reexpand; /* next time we expose, we'll expand all root folders */
};
#define MODEST_FOLDER_VIEW_GET_PRIVATE(o)			\
	(G_TYPE_INSTANCE_GET_PRIVATE((o),			\
				     MODEST_TYPE_FOLDER_VIEW,	\
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

		static const GInterfaceInfo tny_account_store_view_info = {
			(GInterfaceInitFunc) tny_account_store_view_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

				
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestFolderView",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, 
					     TNY_TYPE_ACCOUNT_STORE_VIEW, 
					     &tny_account_store_view_info);
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

	g_type_class_add_private (gobject_class,
				  sizeof(ModestFolderViewPrivate));
	
 	signals[FOLDER_SELECTION_CHANGED_SIGNAL] = 
		g_signal_new ("folder_selection_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestFolderViewClass,
					       folder_selection_changed),
			      NULL, NULL,
			      modest_marshal_VOID__POINTER_BOOLEAN,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_BOOLEAN);

	/*
	 * This signal is emitted whenever the currently selected
	 * folder display name is computed. Note that the name could
	 * be different to the folder name, because we could append
	 * the unread messages count to the folder name to build the
	 * folder display name
	 */
 	signals[FOLDER_DISPLAY_NAME_CHANGED_SIGNAL] = 
		g_signal_new ("folder-display-name-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestFolderViewClass,
					       folder_display_name_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
}

/* Simplify checks for NULLs: */
static gboolean
strings_are_equal (const gchar *a, const gchar *b)
{
	if (!a && !b)
		return TRUE;
	if (a && b)
	{
		return (strcmp (a, b) == 0);
	}
	else
		return FALSE;
}

static gboolean
on_model_foreach_set_name(GtkTreeModel *model, GtkTreePath *path,  GtkTreeIter *iter, gpointer data)
{
	GObject *instance = NULL;
	
	gtk_tree_model_get (model, iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &instance,
			    -1);
			    
	if (!instance)
		return FALSE; /* keep walking */
			
	if (!TNY_IS_ACCOUNT (instance)) {
		g_object_unref (instance);
		return FALSE; /* keep walking */	
	}    
	
	/* Check if this is the looked-for account: */
	TnyAccount *this_account = TNY_ACCOUNT (instance);
	TnyAccount *account = TNY_ACCOUNT (data);
	
	const gchar *this_account_id = tny_account_get_id(this_account);
	const gchar *account_id = tny_account_get_id(account);
	g_object_unref (instance);
	instance = NULL;

	/* printf ("DEBUG: %s: this_account_id=%s, account_id=%s\n", __FUNCTION__, this_account_id, account_id); */
	if (strings_are_equal(this_account_id, account_id)) {
		/* Tell the model that the data has changed, so that
	 	 * it calls the cell_data_func callbacks again: */
		/* TODO: This does not seem to actually cause the new string to be shown: */
		gtk_tree_model_row_changed (model, path, iter);
		
		return TRUE; /* stop walking */
	}
	
	return FALSE; /* keep walking */
}

typedef struct 
{
	ModestFolderView *self;
	gchar *previous_name;
} GetMmcAccountNameData;

static void
on_get_mmc_account_name (TnyStoreAccount* account, gpointer user_data)
{
	/* printf ("DEBU1G: %s: account name=%s\n", __FUNCTION__, tny_account_get_name (TNY_ACCOUNT(account))); */

	GetMmcAccountNameData *data = (GetMmcAccountNameData*)user_data;
	
	if (!strings_are_equal (
		tny_account_get_name(TNY_ACCOUNT(account)), 
		data->previous_name)) {
	
		/* Tell the model that the data has changed, so that 
		 * it calls the cell_data_func callbacks again: */
		ModestFolderView *self = data->self;
		GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	 	if (model)
			gtk_tree_model_foreach(model, on_model_foreach_set_name, account);
	}

	g_free (data->previous_name);
	g_slice_free (GetMmcAccountNameData, data);
}

static void
text_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer data)
{
	ModestFolderViewPrivate *priv;
	GObject *rendobj;
	gchar *fname = NULL;
	gint unread = 0;
	gint all = 0;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	GObject *instance = NULL;

	g_return_if_fail (column);
	g_return_if_fail (tree_model);
	g_return_if_fail (iter != NULL);

	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_ALL_COLUMN, &all,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN, &unread,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &instance,
			    -1);
	rendobj = G_OBJECT(renderer);

	if (!fname)
		return;

	if (!instance) {
		g_free (fname);
		return;
	}

	ModestFolderView *self = MODEST_FOLDER_VIEW (data);
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE (self);
	
	gchar *item_name = NULL;
	gint item_weight = 400;
	
	if (type != TNY_FOLDER_TYPE_ROOT) {
		gint number = 0;
		
		if (modest_tny_folder_is_local_folder (TNY_FOLDER (instance)) ||
		    modest_tny_folder_is_memory_card_folder (TNY_FOLDER (instance))) {
			type = modest_tny_folder_get_local_or_mmc_folder_type (TNY_FOLDER (instance));
			if (type != TNY_FOLDER_TYPE_UNKNOWN) {
				g_free (fname);
				fname = g_strdup(modest_local_folder_info_get_type_display_name (type));
			}
		}

		/* Select the number to show: the unread or unsent messages */
		if ((type == TNY_FOLDER_TYPE_DRAFTS) || (type == TNY_FOLDER_TYPE_OUTBOX))
			number = all;
		else
			number = unread;
		
		/* Use bold font style if there are unread or unset messages */
		if (number > 0) {
			item_name = g_strdup_printf ("%s (%d)", fname, number);
			item_weight = 800;
		} else {
			item_name = g_strdup (fname);
			item_weight = 400;
		}
		
	} else if (TNY_IS_ACCOUNT (instance)) {
		/* If it's a server account */
		if (modest_tny_account_is_virtual_local_folders (
				TNY_ACCOUNT (instance))) {
			item_name = g_strdup (priv->local_account_name);
			item_weight = 800;
		} else if (modest_tny_account_is_memory_card_account (
				TNY_ACCOUNT (instance))) {
			/* fname is only correct when the items are first 
			 * added to the model, not when the account is 
			 * changed later, so get the name from the account
			 * instance: */
			item_name = g_strdup (tny_account_get_name (TNY_ACCOUNT (instance)));
			item_weight = 800;
		} else {
			item_name = g_strdup (fname);
			item_weight = 800;
		}
	}
	
	if (!item_name)
		item_name = g_strdup ("unknown");
			
	if (item_name && item_weight) {
		/* Set the name in the treeview cell: */
		g_object_set (rendobj,"text", item_name, "weight", item_weight, NULL);
		
		/* Notify display name observers */
		/* TODO: What listens for this signal, and how can it use only the new name? */
		if (G_OBJECT (priv->cur_folder_store) == instance) {
			g_signal_emit (G_OBJECT(self),
					       signals[FOLDER_DISPLAY_NAME_CHANGED_SIGNAL], 0,
					       item_name);
		}
		g_free (item_name);
		
	}
	
	/* If it is a Memory card account, make sure that we have the correct name.
	 * This function will be trigerred again when the name has been retrieved: */
	if (TNY_IS_STORE_ACCOUNT (instance) && 
		modest_tny_account_is_memory_card_account (TNY_ACCOUNT (instance))) {

		/* Get the account name asynchronously: */
		GetMmcAccountNameData *callback_data = 
			g_slice_new0(GetMmcAccountNameData);
		callback_data->self = self;

		const gchar *name = tny_account_get_name (TNY_ACCOUNT(instance));
		if (name)
			callback_data->previous_name = g_strdup (name); 

		modest_tny_account_get_mmc_account_name (TNY_STORE_ACCOUNT (instance), 
							 on_get_mmc_account_name, callback_data);
	}
 			
	g_object_unref (G_OBJECT (instance));
	g_free (fname);
}

static void
icon_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,  GtkTreeIter *iter, gpointer data)
{
	GObject *rendobj = NULL, *instance = NULL;
	GdkPixbuf *pixbuf = NULL;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	const gchar *account_id = NULL;
	gboolean has_children;

	rendobj = G_OBJECT(renderer);
	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &instance,
			    -1);
	has_children = gtk_tree_model_iter_has_child (tree_model, iter);

	if (!instance) 
		return;

	/* MERGE is not needed anymore as the folder now has the correct type jschmid */
	/* We include the MERGE type here because it's used to create
	   the local OUTBOX folder */
	if (type == TNY_FOLDER_TYPE_NORMAL || 
	    type == TNY_FOLDER_TYPE_UNKNOWN) {
		type = modest_tny_folder_guess_folder_type (TNY_FOLDER (instance));
	}

	switch (type) {
	case TNY_FOLDER_TYPE_ROOT:
		if (TNY_IS_ACCOUNT (instance)) {
			
			if (modest_tny_account_is_virtual_local_folders (
				TNY_ACCOUNT (instance))) {
				pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_LOCAL_FOLDERS);
			}
			else {
				account_id = tny_account_get_id (TNY_ACCOUNT (instance));
				
				if (!strcmp (account_id, MODEST_MMC_ACCOUNT_ID))
					pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_MMC);
				else
					pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_ACCOUNT);
			}
		}
		break;
	case TNY_FOLDER_TYPE_INBOX:
	    pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_INBOX);
	    break;
	case TNY_FOLDER_TYPE_OUTBOX:
		pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_OUTBOX);
		break;
	case TNY_FOLDER_TYPE_JUNK:
		pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_JUNK);
		break;
	case TNY_FOLDER_TYPE_SENT:
		pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_SENT);
		break;
	case TNY_FOLDER_TYPE_TRASH:
		pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_TRASH);
		break;
	case TNY_FOLDER_TYPE_DRAFTS:
		pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_DRAFTS);
		break;
	case TNY_FOLDER_TYPE_NORMAL:
	default:
		pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_NORMAL);
		break;
	}
	
	g_object_unref (G_OBJECT (instance));

	/* Set pixbuf */
	g_object_set (rendobj, "pixbuf", pixbuf, NULL);
	if (has_children && (pixbuf != NULL)) {
		GdkPixbuf *open_pixbuf, *closed_pixbuf;
		GdkPixbuf *open_emblem, *closed_emblem;
		open_pixbuf = gdk_pixbuf_copy (pixbuf);
		closed_pixbuf = gdk_pixbuf_copy (pixbuf);
		open_emblem = modest_platform_get_icon ("qgn_list_gene_fldr_exp");
		closed_emblem = modest_platform_get_icon ("qgn_list_gene_fldr_clp");

		if (open_emblem) {
			gdk_pixbuf_composite (open_emblem, open_pixbuf, 0, 0, 
					      MIN (gdk_pixbuf_get_width (open_emblem), 
						   gdk_pixbuf_get_width (open_pixbuf)),
					      MIN (gdk_pixbuf_get_height (open_emblem), 
						   gdk_pixbuf_get_height (open_pixbuf)),
					      0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
			g_object_set (rendobj, "pixbuf-expander-open", open_pixbuf, NULL);
			g_object_unref (open_emblem);
		}
		if (closed_emblem) {
			gdk_pixbuf_composite (closed_emblem, closed_pixbuf, 0, 0, 
					      MIN (gdk_pixbuf_get_width (closed_emblem), 
						   gdk_pixbuf_get_width (closed_pixbuf)),
					      MIN (gdk_pixbuf_get_height (closed_emblem), 
						   gdk_pixbuf_get_height (closed_pixbuf)),
					      0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
			g_object_set (rendobj, "pixbuf-expander-closed", closed_pixbuf, NULL);
			g_object_unref (closed_emblem);
		}
		if (closed_pixbuf)
			g_object_unref (closed_pixbuf);
		if (open_pixbuf)
			g_object_unref (open_pixbuf);
	}

	if (pixbuf != NULL)
		g_object_unref (pixbuf);
}

static void
add_columns (GtkWidget *treeview)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;

	/* Create column */
	column = gtk_tree_view_column_new ();	
	
	/* Set icon and text render function */
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						icon_cell_data, treeview, NULL);
	
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						text_cell_data, treeview, NULL);
	
	/* Set selection mode */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

	/* Set treeview appearance */
	gtk_tree_view_column_set_spacing (column, 2);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_fixed_width (column, TRUE);		
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(treeview), FALSE);

	/* Add column */
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview),column);
}

static void
modest_folder_view_init (ModestFolderView *obj)
{
	ModestFolderViewPrivate *priv;
	ModestConf *conf;
	
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(obj);
	
	priv->timer_expander = 0;
	priv->account_store  = NULL;
	priv->query          = NULL;
	priv->style          = MODEST_FOLDER_VIEW_STYLE_SHOW_ALL;
	priv->cur_folder_store   = NULL;
	priv->visible_account_id = NULL;
	priv->folder_to_select = NULL;

	priv->reexpand = TRUE;

	/* Initialize the local account name */
	conf = modest_runtime_get_conf();
	priv->local_account_name = modest_conf_get_string (conf, MODEST_CONF_DEVICE_NAME, NULL);

	/* Init email clipboard */
	priv->clipboard = modest_runtime_get_email_clipboard ();
	priv->hidding_ids = NULL;
	priv->n_selected = 0;
	priv->reselect = FALSE;
	priv->show_non_move = TRUE;

	/* Build treeview */
	add_columns (GTK_WIDGET (obj));

	/* Setup drag and drop */
	setup_drag_and_drop (GTK_TREE_VIEW(obj));

	/* Connect signals */
	g_signal_connect (G_OBJECT (obj), 
			  "key-press-event", 
			  G_CALLBACK (on_key_pressed), NULL);

	/*
	 * Track changes in the local account name (in the device it
	 * will be the device name)
	 */
	priv->notification_id = modest_conf_listen_to_namespace (conf, 
								 MODEST_CONF_NAMESPACE);
	priv->conf_key_signal = g_signal_connect (G_OBJECT(conf), 
						  "key_changed",
						  G_CALLBACK(on_configuration_key_changed), 
						  obj);
}

static void
tny_account_store_view_init (gpointer g, gpointer iface_data)
{
	TnyAccountStoreViewIface *klass = (TnyAccountStoreViewIface *)g;

	klass->set_account_store_func = modest_folder_view_set_account_store;

	return;
}

static void
modest_folder_view_finalize (GObject *obj)
{
	ModestFolderViewPrivate *priv;
	GtkTreeSelection    *sel;
	
	g_return_if_fail (obj);
	
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(obj);

	if (priv->notification_id) {
		modest_conf_forget_namespace (modest_runtime_get_conf (),
					      MODEST_CONF_NAMESPACE,
					      priv->notification_id);
	}

	if (priv->timer_expander != 0) {
		g_source_remove (priv->timer_expander);
		priv->timer_expander = 0;
	}

	if (priv->account_store) {
		g_signal_handler_disconnect (G_OBJECT(priv->account_store),
					     priv->account_inserted_signal);
		g_signal_handler_disconnect (G_OBJECT(priv->account_store),
					     priv->account_removed_signal);
		g_signal_handler_disconnect (G_OBJECT(priv->account_store),
					     priv->account_changed_signal);
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}

	if (priv->query) {
		g_object_unref (G_OBJECT (priv->query));
		priv->query = NULL;
	}

/* 	modest_folder_view_disable_next_folder_selection (MODEST_FOLDER_VIEW(obj)); */
	if (priv->folder_to_select) {
		g_object_unref (G_OBJECT(priv->folder_to_select));
	    	priv->folder_to_select = NULL;
	}
   
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(obj));
	if (sel)
		g_signal_handler_disconnect (G_OBJECT(sel), priv->changed_signal);

	g_free (priv->local_account_name);
	g_free (priv->visible_account_id);
	
	if (priv->conf_key_signal) {
		g_signal_handler_disconnect (modest_runtime_get_conf (),
					     priv->conf_key_signal);
		priv->conf_key_signal = 0;
	}

	if (priv->cur_folder_store) {
		if (TNY_IS_FOLDER(priv->cur_folder_store))
			tny_folder_sync (TNY_FOLDER(priv->cur_folder_store), FALSE, NULL);
		        /* FALSE --> expunge the message */

		g_object_unref (priv->cur_folder_store);
		priv->cur_folder_store = NULL;
	}

	/* Clear hidding array created by cut operation */
	_clear_hidding_filter (MODEST_FOLDER_VIEW (obj));

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void
modest_folder_view_set_account_store (TnyAccountStoreView *self, TnyAccountStore *account_store)
{
	ModestFolderViewPrivate *priv;
	TnyDevice *device;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	g_return_if_fail (TNY_IS_ACCOUNT_STORE (account_store));

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);
	device = tny_account_store_get_device (account_store);

	if (G_UNLIKELY (priv->account_store)) {

		if (g_signal_handler_is_connected (G_OBJECT (priv->account_store),
						   priv->account_inserted_signal))
			g_signal_handler_disconnect (G_OBJECT (priv->account_store),
						     priv->account_inserted_signal);
		if (g_signal_handler_is_connected (G_OBJECT (priv->account_store), 
						   priv->account_removed_signal))
			g_signal_handler_disconnect (G_OBJECT (priv->account_store), 
						     priv->account_removed_signal);
		if (g_signal_handler_is_connected (G_OBJECT (priv->account_store), 
						   priv->account_changed_signal))
			g_signal_handler_disconnect (G_OBJECT (priv->account_store), 
						     priv->account_changed_signal);
		g_object_unref (G_OBJECT (priv->account_store));
	}

	priv->account_store = g_object_ref (G_OBJECT (account_store));

	priv->account_removed_signal = 
		g_signal_connect (G_OBJECT(account_store), "account_removed",
				  G_CALLBACK (on_account_removed), self);

	priv->account_inserted_signal =
		g_signal_connect (G_OBJECT(account_store), "account_inserted",
				  G_CALLBACK (on_account_inserted), self);

	priv->account_changed_signal =
		g_signal_connect (G_OBJECT(account_store), "account_changed",
				  G_CALLBACK (on_account_changed), self);

	modest_folder_view_update_model (MODEST_FOLDER_VIEW (self), account_store);
	
	g_object_unref (G_OBJECT (device));
}

static void
on_account_inserted (TnyAccountStore *account_store, 
		     TnyAccount *account,
		     gpointer user_data)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *sort_model, *filter_model;

	/* Ignore transport account insertions, we're not showing them
	   in the folder view */
	if (TNY_IS_TRANSPORT_ACCOUNT (account))
		return;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (user_data);

	/* If we're adding a new account, and there is no previous
	   one, we need to select the visible server account */
	if (priv->style == MODEST_FOLDER_VIEW_STYLE_SHOW_ONE &&
	    !priv->visible_account_id)
		modest_widget_memory_restore (modest_runtime_get_conf(), 
					      G_OBJECT (user_data),
					      MODEST_CONF_FOLDER_VIEW_KEY);

	/* Get the inner model */
	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
	sort_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

	/* Insert the account in the model */
	tny_list_append (TNY_LIST (gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model))),
			 G_OBJECT (account));

}


static void
on_account_changed (TnyAccountStore *account_store, 
		    TnyAccount *tny_account,
		    gpointer user_data)
{
	/* do nothing */
	ModestFolderViewPrivate *priv;
	GtkTreeModel *sort_model, *filter_model;

	/* Ignore transport account insertions, we're not showing them
	   in the folder view */
	if (TNY_IS_TRANSPORT_ACCOUNT (tny_account))
		return;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (user_data);

	/* Get the inner model */
	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));
	sort_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

	/* Remove the account from the model */
	tny_list_remove (TNY_LIST (gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model))),
			 G_OBJECT (tny_account));

	/* Insert the account in the model */
	tny_list_append (TNY_LIST (gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model))),
			 G_OBJECT (tny_account));
}



static void
on_account_removed (TnyAccountStore *account_store, 
		    TnyAccount *account,
		    gpointer user_data)
{
	ModestFolderView *self = NULL;
	ModestFolderViewPrivate *priv;
	GtkTreeModel *sort_model, *filter_model;
	GtkTreeSelection *sel = NULL;

	/* Ignore transport account removals, we're not showing them
	   in the folder view */
	if (TNY_IS_TRANSPORT_ACCOUNT (account))
		return;

	self = MODEST_FOLDER_VIEW (user_data);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	/* Invalidate the cur_folder_store only if the selected folder
	   belongs to the account that is being removed */
	if (priv->cur_folder_store) {
		TnyAccount *selected_folder_account = NULL;

		if (TNY_IS_FOLDER (priv->cur_folder_store)) {
			selected_folder_account = 
				tny_folder_get_account (TNY_FOLDER (priv->cur_folder_store));
		} else {
			selected_folder_account = 
				TNY_ACCOUNT (g_object_ref (priv->cur_folder_store));
		}

		if (selected_folder_account == account) {
			sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
			gtk_tree_selection_unselect_all (sel);
		}
		g_object_unref (selected_folder_account);
	}

	/* Invalidate row to select only if the folder to select
	   belongs to the account that is being removed*/
	if (priv->folder_to_select) {
		TnyAccount *folder_to_select_account = NULL;

		folder_to_select_account = tny_folder_get_account (priv->folder_to_select);
		if (folder_to_select_account == account) {
/* 			modest_folder_view_disable_next_folder_selection (self); */
			g_object_unref (priv->folder_to_select);
			priv->folder_to_select = NULL;
		}
		g_object_unref (folder_to_select_account);
	}

	/* Remove the account from the model */
	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	sort_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));
	tny_list_remove (TNY_LIST (gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model))),
			 G_OBJECT (account));

	/* If the removed account is the currently viewed one then
	   clear the configuration value. The new visible account will be the default account */
	if (priv->visible_account_id &&
	    !strcmp (priv->visible_account_id, tny_account_get_id (account))) {

		/* Clear the current visible account_id */
		modest_folder_view_set_account_id_of_visible_server_account (self, NULL);

		/* Call the restore method, this will set the new visible account */
		modest_widget_memory_restore (modest_runtime_get_conf(), G_OBJECT(self),
					      MODEST_CONF_FOLDER_VIEW_KEY);
	}

	/* Select the INBOX */
	modest_folder_view_select_first_inbox_or_local (self);
}

void
modest_folder_view_set_title (ModestFolderView *self, const gchar *title)
{
	GtkTreeViewColumn *col;
	
	g_return_if_fail (self);

	col = gtk_tree_view_get_column (GTK_TREE_VIEW(self), 0);
	if (!col) {
		g_printerr ("modest: failed get column for title\n");
		return;
	}

	gtk_tree_view_column_set_title (col, title);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(self),
					   title != NULL);
}

static gboolean
modest_folder_view_on_map (ModestFolderView *self, 
			   GdkEventExpose *event,
			   gpointer data)
{
	ModestFolderViewPrivate *priv;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	/* This won't happen often */
	if (G_UNLIKELY (priv->reselect)) {
		/* Select the first inbox or the local account if not found */

		/* TODO: this could cause a lock at startup, so we
		   comment it for the moment. We know that this will
		   be a bug, because the INBOX is not selected, but we
		   need to rewrite some parts of Modest to avoid the
		   deathlock situation */
		/* TODO: check if this is still the case */
		priv->reselect = FALSE;
 		modest_folder_view_select_first_inbox_or_local (self);
		/* Notify the display name observers */
		g_signal_emit (G_OBJECT(self),
			       signals[FOLDER_DISPLAY_NAME_CHANGED_SIGNAL], 0,
			       NULL);
	}

	if (priv->reexpand) {
		expand_root_items (self); 
		priv->reexpand = FALSE;
	}

	return FALSE;
}

GtkWidget*
modest_folder_view_new (TnyFolderStoreQuery *query)
{
	GObject *self;
	ModestFolderViewPrivate *priv;
	GtkTreeSelection *sel;
	
	self = G_OBJECT (g_object_new (MODEST_TYPE_FOLDER_VIEW, NULL));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	if (query)
		priv->query = g_object_ref (query);
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	priv->changed_signal = g_signal_connect (sel, "changed",
						 G_CALLBACK (on_selection_changed), self);

	g_signal_connect (self, "expose-event", G_CALLBACK (modest_folder_view_on_map), NULL);

 	return GTK_WIDGET(self);
}

/* this feels dirty; any other way to expand all the root items? */
static void
expand_root_items (ModestFolderView *self)
{
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	path = gtk_tree_path_new_first ();

	/* all folders should have child items, so.. */
	do {
		gtk_tree_view_expand_row (GTK_TREE_VIEW(self), path, FALSE);
		gtk_tree_path_next (path);
	} while (gtk_tree_model_get_iter (model, &iter, path));
	
	gtk_tree_path_free (path);
}

/*
 * We use this function to implement the
 * MODEST_FOLDER_VIEW_STYLE_SHOW_ONE style. We only show the default
 * account in this case, and the local folders.
 */
static gboolean 
filter_row (GtkTreeModel *model,
	    GtkTreeIter *iter,
	    gpointer data)
{
	ModestFolderViewPrivate *priv;
	gboolean retval = TRUE;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	GObject *instance = NULL;
	const gchar *id = NULL;
	guint i;
	gboolean found = FALSE;
	gboolean cleared = FALSE;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (data), FALSE);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (data);

	gtk_tree_model_get (model, iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &instance,
			    -1);

	/* Do not show if there is no instance, this could indeed
	   happen when the model is being modified while it's being
	   drawn. This could occur for example when moving folders
	   using drag&drop */
	if (!instance)
		return FALSE;

	if (type == TNY_FOLDER_TYPE_ROOT) {
		/* TNY_FOLDER_TYPE_ROOT means that the instance is an
		   account instead of a folder. */
		if (TNY_IS_ACCOUNT (instance)) {
			TnyAccount *acc = TNY_ACCOUNT (instance);
			const gchar *account_id = tny_account_get_id (acc);
	
			/* If it isn't a special folder, 
			 * don't show it unless it is the visible account: */
			if (priv->style == MODEST_FOLDER_VIEW_STYLE_SHOW_ONE &&
			    !modest_tny_account_is_virtual_local_folders (acc) &&
			    strcmp (account_id, MODEST_MMC_ACCOUNT_ID)) {
				
				/* Show only the visible account id */
				if (priv->visible_account_id) {
					if (strcmp (account_id, priv->visible_account_id))
						retval = FALSE;
				} else {
					retval = FALSE;
				}				
			}
			
			/* Never show these to the user. They are merged into one folder 
			 * in the local-folders account instead: */
			if (retval && MODEST_IS_TNY_OUTBOX_ACCOUNT (acc))
				retval = FALSE;
		}
	}

	/* Check hiding (if necessary) */
	cleared = modest_email_clipboard_cleared (priv->clipboard);  	       
	if ((retval) && (!cleared) && (TNY_IS_FOLDER (instance))) {
		id = tny_folder_get_id (TNY_FOLDER(instance));
		if (priv->hidding_ids != NULL)
			for (i=0; i < priv->n_selected && !found; i++)
				if (priv->hidding_ids[i] != NULL && id != NULL)
					found = (!strcmp (priv->hidding_ids[i], id));
		
		retval = !found;
	}
	
	
	/* If this is a move to dialog, hide Sent, Outbox and Drafts
	folder as no message can be move there according to UI specs */
	if (!priv->show_non_move)
	{
		switch (type)
		{
			case TNY_FOLDER_TYPE_OUTBOX:
			case TNY_FOLDER_TYPE_SENT:
			case TNY_FOLDER_TYPE_DRAFTS:
				retval = FALSE;
				break;
			case TNY_FOLDER_TYPE_UNKNOWN:
			case TNY_FOLDER_TYPE_NORMAL:
				type = modest_tny_folder_guess_folder_type(TNY_FOLDER(instance));
				if (type == TNY_FOLDER_TYPE_OUTBOX || type == TNY_FOLDER_TYPE_SENT
						|| type == TNY_FOLDER_TYPE_DRAFTS)
				{
					retval = FALSE;
				}
				break;
			default:
				break;	
		}	
	}
	
	/* Free */
	g_object_unref (instance);

	return retval;
}


gboolean
modest_folder_view_update_model (ModestFolderView *self,
				 TnyAccountStore *account_store)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *model /* , *old_model */;
	/* TnyAccount *local_account; */
	TnyList *model_as_list;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);
	g_return_val_if_fail (account_store, FALSE);

	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(self);
	
	/* Notify that there is no folder selected */
	g_signal_emit (G_OBJECT(self), 
		       signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       NULL, FALSE);
	if (priv->cur_folder_store) {
		g_object_unref (priv->cur_folder_store);
		priv->cur_folder_store = NULL;
	}

	/* FIXME: the local accounts are not shown when the query
	   selects only the subscribed folders. */
/* 	model        = tny_gtk_folder_store_tree_model_new (TRUE, priv->query); */
	model        = tny_gtk_folder_store_tree_model_new (NULL);
	
	/* Deal with the model via its TnyList Interface,
	 * filling the TnyList via a get_accounts() call: */
	model_as_list = TNY_LIST(model);

	/* Get the accounts: */
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(account_store),
					model_as_list,
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
	g_object_unref (model_as_list);
	model_as_list = NULL;	
                                                     
	GtkTreeModel *filter_model = NULL, *sortable = NULL;

	sortable = gtk_tree_model_sort_new_with_model (model);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(sortable),
					      TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, 
					      GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
					 TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN,
					 cmp_rows, NULL, NULL);

	/* Create filter model */
	filter_model = gtk_tree_model_filter_new (sortable, NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
						filter_row,
						self,
						NULL);

	/* Set new model */
	gtk_tree_view_set_model (GTK_TREE_VIEW(self), filter_model);
	g_signal_connect (G_OBJECT(filter_model), "row-inserted",
			  (GCallback) on_row_inserted_maybe_select_folder, self);


	g_object_unref (model);
	g_object_unref (filter_model);		
	g_object_unref (sortable);
	
	/* Force a reselection of the INBOX next time the widget is shown */
	priv->reselect = TRUE;
			
	return TRUE;
}


static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel *model = NULL;
	TnyFolderStore *folder = NULL;
	GtkTreeIter iter;
	ModestFolderView *tree_view = NULL;
	ModestFolderViewPrivate *priv = NULL;
	gboolean selected = FALSE;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(user_data);

	selected = gtk_tree_selection_get_selected (sel, &model, &iter);

	/* Notify the display name observers */
	g_signal_emit (G_OBJECT(user_data),
		       signals[FOLDER_DISPLAY_NAME_CHANGED_SIGNAL], 0,
		       NULL);

	tree_view = MODEST_FOLDER_VIEW (user_data);

	if (selected) {
		gtk_tree_model_get (model, &iter,
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder,
				    -1);

		/* If the folder is the same do not notify */
		if (priv->cur_folder_store == folder && folder) {
			g_object_unref (folder);
			return;
		}
	}
	
	/* Current folder was unselected */
	if (priv->cur_folder_store) {
		g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       priv->cur_folder_store, FALSE);

		if (TNY_IS_FOLDER(priv->cur_folder_store))
			tny_folder_sync_async (TNY_FOLDER(priv->cur_folder_store),
					       FALSE, NULL, NULL, NULL);

		/* FALSE --> don't expunge the messages */

		g_object_unref (priv->cur_folder_store);
		priv->cur_folder_store = NULL;
	}

	/* New current references */
	priv->cur_folder_store = folder;

	/* New folder has been selected */
	g_signal_emit (G_OBJECT(tree_view),
		       signals[FOLDER_SELECTION_CHANGED_SIGNAL],
		       0, priv->cur_folder_store, TRUE);
}

TnyFolderStore *
modest_folder_view_get_selected (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);
	if (priv->cur_folder_store)
		g_object_ref (priv->cur_folder_store);

	return priv->cur_folder_store;
}

static gint
get_cmp_rows_type_pos (GObject *folder)
{
	/* Remote accounts -> Local account -> MMC account .*/
	/* 0, 1, 2 */
	
	if (TNY_IS_ACCOUNT (folder) && 
		modest_tny_account_is_virtual_local_folders (
			TNY_ACCOUNT (folder))) {
		return 1;
	} else if (TNY_IS_ACCOUNT (folder)) {
		TnyAccount *account = TNY_ACCOUNT (folder);
		const gchar *account_id = tny_account_get_id (account);
		if (!strcmp (account_id, MODEST_MMC_ACCOUNT_ID))
			return 2;
		else
			return 0;
	}
	else {
		printf ("DEBUG: %s: unexpected type.\n", __FUNCTION__);
		return -1; /* Should never happen */
	}
}

static gint
get_cmp_subfolder_type_pos (TnyFolderType t)
{
	/* Inbox, Outbox, Drafts, Sent, User */
	/* 0, 1, 2, 3, 4 */

	switch (t) {
	case TNY_FOLDER_TYPE_INBOX:
		return 0;
		break;
	case TNY_FOLDER_TYPE_OUTBOX:
		return 1;
		break;
	case TNY_FOLDER_TYPE_DRAFTS:
		return 2;
		break;
	case TNY_FOLDER_TYPE_SENT:
		return 3;
		break;
	default:
		return 4;
	}
}

/*
 * This function orders the mail accounts according to these rules:
 * 1st - remote accounts
 * 2nd - local account
 * 3rd - MMC account
 */
static gint
cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
	  gpointer user_data)
{
	gint cmp = 0;
	gchar *name1 = NULL;
	gchar *name2 = NULL;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	TnyFolderType type2 = TNY_FOLDER_TYPE_UNKNOWN;
	GObject *folder1 = NULL;
	GObject *folder2 = NULL;

	gtk_tree_model_get (tree_model, iter1,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &name1,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder1,
			    -1);
	gtk_tree_model_get (tree_model, iter2,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &name2,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type2,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder2,
			    -1);

	/* Return if we get no folder. This could happen when folder
	   operations are happening. The model is updated after the
	   folder copy/move actually occurs, so there could be
	   situations where the model to be drawn is not correct */
	if (!folder1 || !folder2)
		goto finish;

	if (type == TNY_FOLDER_TYPE_ROOT) {
		/* Compare the types, so that 
		 * Remote accounts -> Local account -> MMC account .*/
		const gint pos1 = get_cmp_rows_type_pos (folder1);
		const gint pos2 = get_cmp_rows_type_pos (folder2);
		/* printf ("DEBUG: %s:\n  type1=%s, pos1=%d\n  type2=%s, pos2=%d\n", 
			__FUNCTION__, G_OBJECT_TYPE_NAME(folder1), pos1, G_OBJECT_TYPE_NAME(folder2), pos2); */
		if (pos1 <  pos2)
			cmp = -1;
		else if (pos1 > pos2)
			cmp = 1;
		else {
			/* Compare items of the same type: */
			
			TnyAccount *account1 = NULL;
			if (TNY_IS_ACCOUNT (folder1))
				account1 = TNY_ACCOUNT (folder1);
				
			TnyAccount *account2 = NULL;
			if (TNY_IS_ACCOUNT (folder2))
				account2 = TNY_ACCOUNT (folder2);
				
			const gchar *account_id = account1 ? tny_account_get_id (account1) : NULL;
			const gchar *account_id2 = account2 ? tny_account_get_id (account2) : NULL;
	
			if (!account_id && !account_id2) {
				cmp = 0;
			} else if (!account_id) {
				cmp = -1;
			} else if (!account_id2) {
				cmp = +1;
			} else if (!strcmp (account_id, MODEST_MMC_ACCOUNT_ID)) {
				cmp = +1;
			} else {
				cmp = modest_text_utils_utf8_strcmp (name1, name2, TRUE);
			}
		}
	} else {
		gint cmp1 = 0, cmp2 = 0;
		/* get the parent to know if it's a local folder */

		GtkTreeIter parent;
		gboolean has_parent;
		has_parent = gtk_tree_model_iter_parent (tree_model, &parent, iter1);
		if (has_parent) {
			GObject *parent_folder;
			TnyFolderType parent_type = TNY_FOLDER_TYPE_UNKNOWN;
			gtk_tree_model_get (tree_model, &parent, 
					    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &parent_type,
					    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &parent_folder,
					    -1);
			if ((parent_type == TNY_FOLDER_TYPE_ROOT) &&
			    TNY_IS_ACCOUNT (parent_folder) &&
			    modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (parent_folder))) {
				cmp1 = get_cmp_subfolder_type_pos (modest_tny_folder_get_local_or_mmc_folder_type (TNY_FOLDER (folder1)));
				cmp2 = get_cmp_subfolder_type_pos (modest_tny_folder_get_local_or_mmc_folder_type (TNY_FOLDER (folder2)));
			}
			g_object_unref (parent_folder);
		}

		/* if they are not local folders */
 		if (cmp1 == cmp2) {
			cmp1 = get_cmp_subfolder_type_pos (tny_folder_get_folder_type (TNY_FOLDER (folder1)));
			cmp2 = get_cmp_subfolder_type_pos (tny_folder_get_folder_type (TNY_FOLDER (folder2)));
		}

		if (cmp1 == cmp2)
			cmp = modest_text_utils_utf8_strcmp (name1, name2, TRUE);
		else 
			cmp = (cmp1 - cmp2);
	}

finish:	
	if (folder1)
		g_object_unref(G_OBJECT(folder1));
	if (folder2)
		g_object_unref(G_OBJECT(folder2));

	g_free (name1);
	g_free (name2);

	return cmp;	
}

/*****************************************************************************/
/*                        DRAG and DROP stuff                                */
/*****************************************************************************/

/*
 * This function fills the #GtkSelectionData with the row and the
 * model that has been dragged. It's called when this widget is a
 * source for dnd after the event drop happened
 */
static void
on_drag_data_get (GtkWidget *widget, 
		  GdkDragContext *context, 
		  GtkSelectionData *selection_data, 
		  guint info, 
		  guint time, 
		  gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *source_row;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	source_row = gtk_tree_model_get_path (model, &iter);

	gtk_tree_set_row_drag_data (selection_data,
				    model,
				    source_row);

	gtk_tree_path_free (source_row);
}

typedef struct _DndHelper {
	gboolean delete_source;
	GtkTreePath *source_row;
	GdkDragContext *context;
	guint time;
} DndHelper;


/*
 * This function is the callback of the
 * modest_mail_operation_xfer_msgs () and
 * modest_mail_operation_xfer_folder() calls. We check here if the
 * message/folder was correctly asynchronously transferred. The reason
 * to use the same callback is that the code is the same, it only has
 * to check that the operation went fine and then finalize the drag
 * and drop action
 */
static void
on_progress_changed (ModestMailOperation *mail_op, 
		     ModestMailOperationState *state,
		     gpointer user_data)
{
	gboolean success;
	DndHelper *helper;

	helper = (DndHelper *) user_data;

	if (!state->finished)
		return;

	if (state->status == MODEST_MAIL_OPERATION_STATUS_SUCCESS) {
		success = TRUE;
	} else {
		success = FALSE;
	}

	/* Notify the drag source. Never call delete, the monitor will
	   do the job if needed */
	gtk_drag_finish (helper->context, success, FALSE, helper->time);

	/* Free the helper */
	gtk_tree_path_free (helper->source_row);
	g_slice_free (DndHelper, helper);
}


/* get the folder for the row the treepath refers to. */
/* folder must be unref'd */
static TnyFolder*
tree_path_to_folder (GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	TnyFolder *folder = NULL;
	
	if (gtk_tree_model_get_iter (model,&iter, path))
		gtk_tree_model_get (model, &iter,
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder,
				    -1);
	return folder;
}

static void 
show_banner_move_target_error ()
{
	ModestWindow *main_window;

	main_window = modest_window_mgr_get_main_window(
			modest_runtime_get_window_mgr());
				
	modest_platform_information_banner(GTK_WIDGET(main_window),
			NULL, _("mail_in_ui_folder_move_target_error"));
}

/*
 * This function is used by drag_data_received_cb to manage drag and
 * drop of a header, i.e, and drag from the header view to the folder
 * view.
 */
static void
drag_and_drop_from_header_view (GtkTreeModel *source_model,
				GtkTreeModel *dest_model,
				GtkTreePath  *dest_row,
				DndHelper    *helper)
{
	TnyList *headers = NULL;
	TnyHeader *header = NULL;
	TnyFolder *folder = NULL;
	ModestMailOperation *mail_op = NULL;
	GtkTreeIter source_iter;
	ModestWindowMgr *mgr = NULL; /*no need for unref*/
	ModestWindow *main_win = NULL; /*no need for unref*/

	g_return_if_fail (GTK_IS_TREE_MODEL(source_model));
	g_return_if_fail (GTK_IS_TREE_MODEL(dest_model));
	g_return_if_fail (dest_row);
	g_return_if_fail (helper);

	/* Get header */
	gtk_tree_model_get_iter (source_model, &source_iter, helper->source_row);
	gtk_tree_model_get (source_model, &source_iter, 
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
			    &header, -1);
	if (!TNY_IS_HEADER(header)) {
		g_warning ("BUG: %s could not get a valid header", __FUNCTION__);
		goto cleanup;
	}
	
	/* Check if the selected message is in msg-view. If it is than
	 * do not enable drag&drop on that. */
	mgr = modest_runtime_get_window_mgr ();
	if (modest_window_mgr_find_registered_header(mgr, header, NULL))
		goto cleanup;

	/* Get Folder */
	folder = tree_path_to_folder (dest_model, dest_row);
	if (!TNY_IS_FOLDER(folder)) {
		g_warning ("BUG: %s could not get a valid folder", __FUNCTION__);
		show_banner_move_target_error();
		goto cleanup;
	}
	if (modest_tny_folder_get_rules(folder) & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE) {
		g_debug ("folder rules: cannot write to that folder");
		goto cleanup;
	}
	
	headers = tny_simple_list_new ();
	tny_list_append (headers, G_OBJECT (header));

	main_win = modest_window_mgr_get_main_window(mgr);
	if(msgs_move_to_confirmation(GTK_WINDOW(main_win), folder, TRUE, headers)
			== GTK_RESPONSE_CANCEL)
		goto cleanup;

	/* Transfer message */
	mail_op = modest_mail_operation_new_with_error_handling (MODEST_MAIL_OPERATION_TYPE_RECEIVE, 
								 NULL,
								 modest_ui_actions_move_folder_error_handler,
								 NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);
	g_signal_connect (G_OBJECT (mail_op), "progress-changed",
			  G_CALLBACK (on_progress_changed), helper);

	modest_mail_operation_xfer_msgs (mail_op, 
					 headers, 
					 folder, 
					 helper->delete_source, 
					 NULL, NULL);
	
	/* Frees */
cleanup:
	if (G_IS_OBJECT(mail_op))
		g_object_unref (G_OBJECT (mail_op));
	if (G_IS_OBJECT(header))
		g_object_unref (G_OBJECT (header));
	if (G_IS_OBJECT(folder))
		g_object_unref (G_OBJECT (folder));
	if (G_IS_OBJECT(headers))
		g_object_unref (headers);
}

/*
 * This function is used by drag_data_received_cb to manage drag and
 * drop of a folder, i.e, and drag from the folder view to the same
 * folder view.
 */
static void
drag_and_drop_from_folder_view (GtkTreeModel     *source_model,
				GtkTreeModel     *dest_model,
				GtkTreePath      *dest_row,
				GtkSelectionData *selection_data,
				DndHelper        *helper)
{
	ModestMailOperation *mail_op = NULL;
	GtkTreeIter dest_iter, iter;
	TnyFolderStore *dest_folder = NULL;
	TnyFolder *folder = NULL;
	gboolean forbidden = FALSE;

	if (!forbidden) {
		/* check the folder rules for the destination */
		folder = tree_path_to_folder (dest_model, dest_row);
		if (TNY_IS_FOLDER(folder)) {
			ModestTnyFolderRules rules =
					modest_tny_folder_get_rules (folder);
			forbidden = rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;

			if (forbidden)
				g_debug ("folder rules: cannot write to that folder");
		} else if (TNY_IS_FOLDER_STORE(folder)){
			/* enable local root as destination for folders */
			if (!MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (folder)
					&& TNY_IS_ACCOUNT (folder))
				forbidden = TRUE;
		}
		g_object_unref (folder);
	}
	if (!forbidden) {
		/* check the folder rules for the source */
		folder = tree_path_to_folder (source_model, helper->source_row);
		if (TNY_IS_FOLDER(folder)) {
			ModestTnyFolderRules rules =
					modest_tny_folder_get_rules (folder);
			forbidden = rules & MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE;
			if (forbidden)
				g_debug ("folder rules: cannot move that folder");
		} else
			forbidden = TRUE;
		g_object_unref (folder);
	}

	
	/* Check if the drag is possible */
	if (forbidden || !gtk_tree_path_compare (helper->source_row, dest_row)) {
		gtk_drag_finish (helper->context, FALSE, FALSE, helper->time);
		gtk_tree_path_free (helper->source_row);	
		g_slice_free (DndHelper, helper);
		return;
	}

	/* Get data */
	gtk_tree_model_get_iter (dest_model, &dest_iter, dest_row);
	gtk_tree_model_get (dest_model, &dest_iter, 
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
			    &dest_folder, -1);
	gtk_tree_model_get_iter (source_model, &iter, helper->source_row);
	gtk_tree_model_get (source_model, &iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN,
			    &folder, -1);

	/* Offer the connection dialog if necessary, for the destination parent folder and source folder: */
	if (modest_platform_connect_and_wait_if_network_folderstore (
				NULL, dest_folder) && 
			modest_platform_connect_and_wait_if_network_folderstore (
				NULL, TNY_FOLDER_STORE (folder))) {
		/* Do the mail operation */
		mail_op = modest_mail_operation_new_with_error_handling (
				MODEST_MAIL_OPERATION_TYPE_RECEIVE, 
				NULL,
				modest_ui_actions_move_folder_error_handler,
				NULL);
		modest_mail_operation_queue_add (
				modest_runtime_get_mail_operation_queue (), 
				mail_op);
		g_signal_connect (
				G_OBJECT (mail_op),
				"progress-changed",
				G_CALLBACK (on_progress_changed),
				helper);

		modest_mail_operation_xfer_folder (mail_op, 
				folder, 
				dest_folder,
				helper->delete_source,
				NULL,
				NULL);

		g_object_unref (G_OBJECT (mail_op));	
	}
	
	/* Frees */
	g_object_unref (G_OBJECT (dest_folder));
	g_object_unref (G_OBJECT (folder));
}

/*
 * This function receives the data set by the "drag-data-get" signal
 * handler. This information comes within the #GtkSelectionData. This
 * function will manage both the drags of folders of the treeview and
 * drags of headers of the header view widget.
 */
static void 
on_drag_data_received (GtkWidget *widget, 
		       GdkDragContext *context, 
		       gint x, 
		       gint y, 
		       GtkSelectionData *selection_data, 
		       guint target_type, 
		       guint time, 
		       gpointer data)
{
	GtkWidget *source_widget;
	GtkTreeModel *dest_model, *source_model;
 	GtkTreePath *source_row, *dest_row;
	GtkTreeViewDropPosition pos;
	gboolean success = FALSE, delete_source = FALSE;
	DndHelper *helper = NULL; 

	/* Do not allow further process */
	g_signal_stop_emission_by_name (widget, "drag-data-received");
	source_widget = gtk_drag_get_source_widget (context);

	/* Get the action */
	if (context->action == GDK_ACTION_MOVE) {
		delete_source = TRUE;

		/* Notify that there is no folder selected. We need to
		   do this in order to update the headers view (and
		   its monitors, because when moving, the old folder
		   won't longer exist. We can not wait for the end of
		   the operation, because the operation won't start if
		   the folder is in use */
		if (source_widget == widget) {
			ModestFolderViewPrivate *priv;

			priv = MODEST_FOLDER_VIEW_GET_PRIVATE (widget);
			if (priv->cur_folder_store) {
				g_object_unref (priv->cur_folder_store);
				priv->cur_folder_store = NULL;
			}

			g_signal_emit (G_OBJECT (widget), 
				       signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0, NULL, FALSE);
		}
	}

	/* Check if the get_data failed */
	if (selection_data == NULL || selection_data->length < 0)
		gtk_drag_finish (context, success, FALSE, time);

	/* Get the models */
	gtk_tree_get_row_drag_data (selection_data,
				    &source_model,
				    &source_row);

	/* Select the destination model */
	if (source_widget == widget) {
		dest_model = source_model;
	} else {
		dest_model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	}

	/* Get the path to the destination row. Can not call
	   gtk_tree_view_get_drag_dest_row() because the source row
	   is not selected anymore */
	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget), x, y,
					   &dest_row, &pos);

	/* Only allow drops IN other rows */
	if (!dest_row || pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_AFTER)
		gtk_drag_finish (context, success, FALSE, time);

	/* Create the helper */
	helper = g_slice_new0 (DndHelper);
	helper->delete_source = delete_source;
	helper->source_row = gtk_tree_path_copy (source_row);
	helper->context = context;
	helper->time = time;

	/* Drags from the header view */
	if (source_widget != widget) {

		drag_and_drop_from_header_view (source_model,
						dest_model,
						dest_row,
						helper);
	} else {


		drag_and_drop_from_folder_view (source_model,
						dest_model,
						dest_row,
						selection_data, 
						helper);
	}

	/* Frees */
	gtk_tree_path_free (source_row);
	gtk_tree_path_free (dest_row);
}

/*
 * We define a "drag-drop" signal handler because we do not want to
 * use the default one, because the default one always calls
 * gtk_drag_finish and we prefer to do it in the "drag-data-received"
 * signal handler, because there we have all the information available
 * to know if the dnd was a success or not.
 */
static gboolean
drag_drop_cb (GtkWidget      *widget,
	      GdkDragContext *context,
	      gint            x,
	      gint            y,
	      guint           time,
	      gpointer        user_data) 
{
	gpointer target;

	if (!context->targets)
		return FALSE;

	/* Check if we're dragging a folder row */
	target = gtk_drag_dest_find_target (widget, context, NULL);

	/* Request the data from the source. */
	gtk_drag_get_data(widget, context, target, time);

    return TRUE;
}

/*
 * This function expands a node of a tree view if it's not expanded
 * yet. Not sure why it needs the threads stuff, but gtk+`example code
 * does that, so that's why they're here.
 */
static gint
expand_row_timeout (gpointer data)
{
	GtkTreeView *tree_view = data;
	GtkTreePath *dest_path = NULL;
	GtkTreeViewDropPosition pos;
	gboolean result = FALSE;
	
	GDK_THREADS_ENTER ();
	
	gtk_tree_view_get_drag_dest_row (tree_view,
					 &dest_path,
					 &pos);
	
	if (dest_path &&
	    (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
	     pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)) {
		gtk_tree_view_expand_row (tree_view, dest_path, FALSE);
		gtk_tree_path_free (dest_path);
	}
	else {
		if (dest_path)
			gtk_tree_path_free (dest_path);
		
		result = TRUE;
	}
	
	GDK_THREADS_LEAVE ();

	return result;
}

/*
 * This function is called whenever the pointer is moved over a widget
 * while dragging some data. It installs a timeout that will expand a
 * node of the treeview if not expanded yet. This function also calls
 * gdk_drag_status in order to set the suggested action that will be
 * used by the "drag-data-received" signal handler to know if we
 * should do a move or just a copy of the data.
 */
static gboolean
on_drag_motion (GtkWidget      *widget,
		GdkDragContext *context,
		gint            x,
		gint            y,
		guint           time,
		gpointer        user_data)  
{
	GtkTreeViewDropPosition pos;
	GtkTreePath *dest_row;
	ModestFolderViewPrivate *priv;
	GdkDragAction suggested_action;
	gboolean valid_location = FALSE;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (widget);

	if (priv->timer_expander != 0) {
		g_source_remove (priv->timer_expander);
		priv->timer_expander = 0;
	}

	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget),
					   x, y,
					   &dest_row,
					   &pos);

	/* Do not allow drops between folders */
	if (!dest_row ||
	    pos == GTK_TREE_VIEW_DROP_BEFORE ||
	    pos == GTK_TREE_VIEW_DROP_AFTER) {
		gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW (widget), NULL, 0);
		gdk_drag_status(context, 0, time);
		valid_location = FALSE;
		goto out;
	} else {
		valid_location = TRUE;
	}

	/* Expand the selected row after 1/2 second */
	if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), dest_row)) {
		gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (widget), dest_row, pos);
		priv->timer_expander = g_timeout_add (500, expand_row_timeout, widget);
	}

	/* Select the desired action. By default we pick MOVE */
	suggested_action = GDK_ACTION_MOVE;

        if (context->actions == GDK_ACTION_COPY)
            gdk_drag_status(context, GDK_ACTION_COPY, time);
	else if (context->actions == GDK_ACTION_MOVE)
            gdk_drag_status(context, GDK_ACTION_MOVE, time);
	else if (context->actions & suggested_action)
            gdk_drag_status(context, suggested_action, time);
	else
            gdk_drag_status(context, GDK_ACTION_DEFAULT, time);

 out:
	if (dest_row)
		gtk_tree_path_free (dest_row);
	g_signal_stop_emission_by_name (widget, "drag-motion");
	return valid_location;
}


/* Folder view drag types */
const GtkTargetEntry folder_view_drag_types[] =
{
	{ "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, MODEST_FOLDER_ROW },
	{ "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_APP,    MODEST_HEADER_ROW }
};

/*
 * This function sets the treeview as a source and a target for dnd
 * events. It also connects all the requirede signals.
 */
static void
setup_drag_and_drop (GtkTreeView *self)
{
	/* Set up the folder view as a dnd destination. Set only the
	   highlight flag, otherwise gtk will have a different
	   behaviour */
	gtk_drag_dest_set (GTK_WIDGET (self),
			   GTK_DEST_DEFAULT_HIGHLIGHT,
			   folder_view_drag_types,
			   G_N_ELEMENTS (folder_view_drag_types),
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (self),
			  "drag_data_received",
			  G_CALLBACK (on_drag_data_received),
			  NULL);


	/* Set up the treeview as a dnd source */
	gtk_drag_source_set (GTK_WIDGET (self),
			     GDK_BUTTON1_MASK,
			     folder_view_drag_types,
			     G_N_ELEMENTS (folder_view_drag_types),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (self),
			  "drag_motion",
			  G_CALLBACK (on_drag_motion),
			  NULL);
	
	g_signal_connect (G_OBJECT (self),
			  "drag_data_get",
			  G_CALLBACK (on_drag_data_get),
			  NULL);

	g_signal_connect (G_OBJECT (self),
			  "drag_drop",
			  G_CALLBACK (drag_drop_cb),
			  NULL);
}

/*
 * This function manages the navigation through the folders using the
 * keyboard or the hardware keys in the device
 */
static gboolean
on_key_pressed (GtkWidget *self,
		GdkEventKey *event,
		gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean retval = FALSE;

	/* Up and Down are automatically managed by the treeview */
	if (event->keyval == GDK_Return) {
		/* Expand/Collapse the selected row */
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			GtkTreePath *path;

			path = gtk_tree_model_get_path (model, &iter);

			if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (self), path))
				gtk_tree_view_collapse_row (GTK_TREE_VIEW (self), path);
			else
				gtk_tree_view_expand_row (GTK_TREE_VIEW (self), path, FALSE);
			gtk_tree_path_free (path);
		}
		/* No further processing */
		retval = TRUE;
	}

	return retval;
}

/*
 * We listen to the changes in the local folder account name key,
 * because we want to show the right name in the view. The local
 * folder account name corresponds to the device name in the Maemo
 * version. We do this because we do not want to query gconf on each
 * tree view refresh. It's better to cache it and change whenever
 * necessary.
 */
static void 
on_configuration_key_changed (ModestConf* conf, 
			      const gchar *key, 
			      ModestConfEvent event,
			      ModestConfNotificationId id, 
			      ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;


	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	/* Do not listen for changes in other namespaces */
	if (priv->notification_id != id)
		 return;
	 
	if (!strcmp (key, MODEST_CONF_DEVICE_NAME)) {
		g_free (priv->local_account_name);

		if (event == MODEST_CONF_EVENT_KEY_UNSET)
			priv->local_account_name = g_strdup (MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME);
		else
			priv->local_account_name = modest_conf_get_string (modest_runtime_get_conf(),
									   MODEST_CONF_DEVICE_NAME, NULL);

		/* Force a redraw */
#if GTK_CHECK_VERSION(2, 8, 0) /* gtk_tree_view_column_queue_resize is only available in GTK+ 2.8 */
		GtkTreeViewColumn * tree_column = gtk_tree_view_get_column (GTK_TREE_VIEW (self), 
									    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN);
		gtk_tree_view_column_queue_resize (tree_column);
#endif
	}
}

void
modest_folder_view_set_style (ModestFolderView *self,
			      ModestFolderViewStyle style)
{
	ModestFolderViewPrivate *priv;

	g_return_if_fail (self);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	priv->style = style;
}

void
modest_folder_view_set_account_id_of_visible_server_account (ModestFolderView *self,
							     const gchar *account_id)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *model;

	g_return_if_fail (self);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	/* This will be used by the filter_row callback,
	 * to decided which rows to show: */
	if (priv->visible_account_id) {
		g_free (priv->visible_account_id);
		priv->visible_account_id = NULL;
	}
	if (account_id)
		priv->visible_account_id = g_strdup (account_id);

	/* Refilter */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (GTK_IS_TREE_MODEL_FILTER (model))
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));

	/* Save settings to gconf */
	modest_widget_memory_save (modest_runtime_get_conf (), G_OBJECT(self),
				   MODEST_CONF_FOLDER_VIEW_KEY);
}

const gchar *
modest_folder_view_get_account_id_of_visible_server_account (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	return (const gchar *) priv->visible_account_id;
}

static gboolean
find_inbox_iter (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *inbox_iter)
{
	do {
		GtkTreeIter child;
		TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;

		gtk_tree_model_get (model, iter, 
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, 
				    &type, -1);
			
		gboolean result = FALSE;
		if (type == TNY_FOLDER_TYPE_INBOX) {
			result = TRUE;
		}		
		if (result) {
			*inbox_iter = *iter;
			return TRUE;
		}

		if (gtk_tree_model_iter_children (model, &child, iter))	{
			if (find_inbox_iter (model, &child, inbox_iter))
				return TRUE;
		}

	} while (gtk_tree_model_iter_next (model, iter));

	return FALSE;
}




void 
modest_folder_view_select_first_inbox_or_local (ModestFolderView *self)
{
	GtkTreeModel *model;
	GtkTreeIter iter, inbox_iter;
	GtkTreeSelection *sel;
	GtkTreePath *path = NULL;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (!model)
		return;

	expand_root_items (self);
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));

	gtk_tree_model_get_iter_first (model, &iter);

	if (find_inbox_iter (model, &iter, &inbox_iter))
		path = gtk_tree_model_get_path (model, &inbox_iter);
	else
		path = gtk_tree_path_new_first ();

	/* Select the row and free */
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (self), path, NULL, FALSE);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (self), path, NULL, FALSE, 0.0, 0.0);
	gtk_tree_path_free (path);

	/* set focus */
	gtk_widget_grab_focus (GTK_WIDGET(self));
}


/* recursive */
static gboolean
find_folder_iter (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *folder_iter, 
		  TnyFolder* folder)
{
	do {
		GtkTreeIter child;
		TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
		TnyFolder* a_folder;
		gchar *name = NULL;
		
		gtk_tree_model_get (model, iter, 
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &a_folder,
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &name,
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type, 
				    -1);		
		g_free (name);

		if (folder == a_folder) {
			g_object_unref (a_folder);
			*folder_iter = *iter;
			return TRUE;
		}
		g_object_unref (a_folder);
		
		if (gtk_tree_model_iter_children (model, &child, iter))	{
			if (find_folder_iter (model, &child, folder_iter, folder)) 
				return TRUE;
		}

	} while (gtk_tree_model_iter_next (model, iter));

	return FALSE;
}


static void
on_row_inserted_maybe_select_folder (GtkTreeModel *tree_model, GtkTreePath  *path, GtkTreeIter *iter,
				     ModestFolderView *self)
{
	ModestFolderViewPrivate *priv = NULL;
	GtkTreeSelection *sel;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	GObject *instance = NULL;

	if (!MODEST_IS_FOLDER_VIEW(self))
		return;
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	priv->reexpand = TRUE;

	gtk_tree_model_get (tree_model, iter, 
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &instance,
			    -1);
	if (type == TNY_FOLDER_TYPE_INBOX && priv->folder_to_select == NULL) {
		priv->folder_to_select = g_object_ref (instance);
	}
	g_object_unref (instance);

	
	if (priv->folder_to_select) {
		
		if (!modest_folder_view_select_folder (self, priv->folder_to_select,
						       FALSE)) {
			GtkTreePath *path;
			path = gtk_tree_model_get_path (tree_model, iter);
			gtk_tree_view_expand_to_path (GTK_TREE_VIEW(self), path);
			
			sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));

			gtk_tree_selection_select_iter (sel, iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW(self), path, NULL, FALSE);

			gtk_tree_path_free (path);
		
		}

		/* Disable next */
		modest_folder_view_disable_next_folder_selection (self);
/* 		g_object_unref (priv->folder_to_select); */
/* 		priv->folder_to_select = NULL; */
	}
}


void
modest_folder_view_disable_next_folder_selection (ModestFolderView *self) 
{
	ModestFolderViewPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	if (priv->folder_to_select)
		g_object_unref(priv->folder_to_select);
	
	priv->folder_to_select = NULL;
}

gboolean
modest_folder_view_select_folder (ModestFolderView *self, TnyFolder *folder, 
				  gboolean after_change)
{
	GtkTreeModel *model;
	GtkTreeIter iter, folder_iter;
	GtkTreeSelection *sel;
	ModestFolderViewPrivate *priv = NULL;
	
	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);	
	g_return_val_if_fail (TNY_IS_FOLDER (folder), FALSE);	
		
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	if (after_change) {

		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
		gtk_tree_selection_unselect_all (sel);

		if (priv->folder_to_select)
			g_object_unref(priv->folder_to_select);
		priv->folder_to_select = TNY_FOLDER(g_object_ref(folder));
		return TRUE;
	}
		
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (!model)
		return FALSE;

		
	gtk_tree_model_get_iter_first (model, &iter);
	if (find_folder_iter (model, &iter, &folder_iter, folder)) {
		GtkTreePath *path;

		path = gtk_tree_model_get_path (model, &folder_iter);
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW(self), path);

		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
		gtk_tree_selection_select_iter (sel, &folder_iter);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW(self), path, NULL, FALSE);

		gtk_tree_path_free (path);
		return TRUE;
	}
	return FALSE;
}


void 
modest_folder_view_copy_selection (ModestFolderView *folder_view)
{
	/* Copy selection */
	_clipboard_set_selected_data (folder_view, FALSE);
}

void 
modest_folder_view_cut_selection (ModestFolderView *folder_view)
{
	ModestFolderViewPrivate *priv = NULL;
	GtkTreeModel *model = NULL;
	const gchar **hidding = NULL;
	guint i, n_selected;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (folder_view));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (folder_view);

	/* Copy selection */
	if (!_clipboard_set_selected_data (folder_view, TRUE))
		return;

	/* Get hidding ids */
	hidding = modest_email_clipboard_get_hidding_ids (priv->clipboard, &n_selected); 
	
	/* Clear hidding array created by previous cut operation */
	_clear_hidding_filter (MODEST_FOLDER_VIEW (folder_view));

	/* Copy hidding array */
	priv->n_selected = n_selected;
	priv->hidding_ids = g_malloc0(sizeof(gchar *) * n_selected);
	for (i=0; i < n_selected; i++) 
		priv->hidding_ids[i] = g_strdup(hidding[i]); 		

	/* Hide cut folders */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (folder_view));
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
}

void
modest_folder_view_copy_model (ModestFolderView *folder_view_src,
			       ModestFolderView *folder_view_dst)
{
	GtkTreeModel *filter_model = NULL;
	GtkTreeModel *model = NULL;
	GtkTreeModel *new_filter_model = NULL;
	
	g_return_if_fail (MODEST_IS_FOLDER_VIEW (folder_view_src));
	g_return_if_fail (MODEST_IS_FOLDER_VIEW (folder_view_dst));

	/* Get src model*/
	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (folder_view_src));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER(filter_model));

	/* Build new filter model */
	new_filter_model = gtk_tree_model_filter_new (model, NULL);	
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (new_filter_model),
						filter_row,
						folder_view_dst,
						NULL);
	/* Set copied model */
	gtk_tree_view_set_model (GTK_TREE_VIEW (folder_view_dst), new_filter_model);

	/* Free */
	g_object_unref (new_filter_model);
}

void
modest_folder_view_show_non_move_folders (ModestFolderView *folder_view,
					  gboolean show)
{
	GtkTreeModel *model = NULL;
	ModestFolderViewPrivate* priv = MODEST_FOLDER_VIEW_GET_PRIVATE(folder_view);
	priv->show_non_move = show;
/* 	modest_folder_view_update_model(folder_view, */
/* 					TNY_ACCOUNT_STORE(modest_runtime_get_account_store())); */

	/* Hide special folders */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (folder_view));
	if (GTK_IS_TREE_MODEL_FILTER (model)) {
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
	}
}

/* Returns FALSE if it did not selected anything */
static gboolean
_clipboard_set_selected_data (ModestFolderView *folder_view,
			      gboolean delete)
{
	ModestFolderViewPrivate *priv = NULL;
	TnyFolderStore *folder = NULL;
	gboolean retval = FALSE;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (folder_view), FALSE);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (folder_view);
		
	/* Set selected data on clipboard   */
	g_return_val_if_fail (MODEST_IS_EMAIL_CLIPBOARD (priv->clipboard), FALSE);
	folder = modest_folder_view_get_selected (folder_view);

	/* Do not allow to select an account */
	if (TNY_IS_FOLDER (folder)) {
		modest_email_clipboard_set_data (priv->clipboard, TNY_FOLDER(folder), NULL, delete);
		retval = TRUE;
	}

	/* Free */
	g_object_unref (folder);

	return retval;
}

static void
_clear_hidding_filter (ModestFolderView *folder_view) 
{
	ModestFolderViewPrivate *priv;
	guint i;
	
	g_return_if_fail (MODEST_IS_FOLDER_VIEW (folder_view));	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(folder_view);

	if (priv->hidding_ids != NULL) {
		for (i=0; i < priv->n_selected; i++) 
			g_free (priv->hidding_ids[i]);
		g_free(priv->hidding_ids);
	}	
}



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
#include <tny-account-store.h>
#include <tny-account.h>
#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-simple-list.h>
#include <tny-merge-folder.h>
#include <modest-tny-folder.h>
#include <modest-tny-simple-folder-store.h>
#include <modest-marshal.h>
#include <modest-icon-names.h>
#include <modest-tny-account-store.h>
#include <modest-tny-outbox-account.h>
#include <modest-text-utils.h>
#include <modest-runtime.h>
#include "modest-folder-view.h"
#include <modest-dnd.h>
#include <modest-platform.h>
#include <modest-account-mgr-helpers.h>
#include <modest-widget-memory.h>


/* 'private'/'protected' functions */
static void modest_folder_view_class_init  (ModestFolderViewClass *klass);
static void modest_folder_view_init        (ModestFolderView *obj);
static void modest_folder_view_finalize    (GObject *obj);

static void         tny_account_store_view_init (gpointer g, 
						 gpointer iface_data);

static void         modest_folder_view_set_account_store (TnyAccountStoreView *self, 
							  TnyAccountStore     *account_store);

static gboolean     update_model           (ModestFolderView *self,
					    ModestTnyAccountStore *account_store);

static void         on_selection_changed   (GtkTreeSelection *sel, gpointer data);

static void         on_account_update      (TnyAccountStore *account_store, 
					    const gchar *account,
					    gpointer user_data);

static void         on_accounts_reloaded   (TnyAccountStore *store, 
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

static void         on_configuration_key_changed         (ModestConf* conf, 
							  const gchar *key, 
							  ModestConfEvent event, 
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

static gint         expand_row_timeout     (gpointer data);

static void         setup_drag_and_drop    (GtkTreeView *self);

enum {
	FOLDER_SELECTION_CHANGED_SIGNAL,
	FOLDER_DISPLAY_NAME_CHANGED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestFolderViewPrivate ModestFolderViewPrivate;
struct _ModestFolderViewPrivate {
	TnyAccountStore      *account_store;
	TnyFolderStore       *cur_folder_store;

	gulong                account_update_signal;
	gulong                changed_signal;
	gulong                accounts_reloaded_signal;
	
	GtkTreeSelection     *cur_selection;
	TnyFolderStoreQuery  *query;
	guint                 timer_expander;

	gchar                *local_account_name;
	gchar                *visible_account_id;
	ModestFolderViewStyle style;
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

static void
text_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer data)
{
	ModestFolderViewPrivate *priv;
	GObject *rendobj;
	gchar *fname = NULL;
	gint unread, all;
	TnyFolderType type;
	GObject *instance = NULL;
	
	g_return_if_fail (column);
	g_return_if_fail (tree_model);

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

	
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE (data);
	
	gchar *item_name = NULL;
	gint item_weight = 400;
	
	if (type != TNY_FOLDER_TYPE_ROOT) {
		gint number = 0;
		
		if (modest_tny_folder_is_local_folder (TNY_FOLDER (instance))) {
			TnyFolderType type;
			type = modest_tny_folder_get_local_folder_type (TNY_FOLDER (instance));
			if (type != TNY_FOLDER_TYPE_UNKNOWN) {
				g_free (fname);
				fname = g_strdup(modest_local_folder_info_get_type_display_name (type));
			}
		}

		/* Select the number to show */
		if ((type == TNY_FOLDER_TYPE_DRAFTS) || (type == TNY_FOLDER_TYPE_OUTBOX))
			number = all;
		else
			number = unread;

		/* Use bold font style if there are unread messages */
		if (unread > 0) {
			item_name = g_strdup_printf ("%s (%d)", fname, unread);
			item_weight = 800;
		} else {
			item_name = g_strdup (fname);
			item_weight = 400;
		}

	} else if (TNY_IS_ACCOUNT (instance)) {
		/* If it's a server account */
		const gchar * account_id = tny_account_get_id (TNY_ACCOUNT (instance));
		if (!strcmp (account_id, MODEST_ACTUAL_LOCAL_FOLDERS_ACCOUNT_ID)) {
			item_name = g_strdup (priv->local_account_name);
		} else {
			if (!strcmp (account_id, MODEST_MMC_ACCOUNT_ID)) {
				/* TODO: get MMC card name */
				item_name = g_strdup (_("MMC"));
			} else {
				item_name = g_strdup (fname);
			}
		}

		item_weight = 800;
	} else if (modest_tny_folder_store_is_virtual_local_folders (
		TNY_FOLDER_STORE(instance)))
	{
		/* We use ModestTnySimpleFolder store to group the outboxes and 
		 * the other local folders together: */
		item_name = g_strdup (_("Local Folders"));
		item_weight = 400;
	}
	
	if (!item_name)
		item_name = g_strdup ("unknown");
			
	if (item_name && item_weight) {
		/* Set the name in the treeview cell: */
		g_object_set (rendobj,"text", item_name, "weight", item_weight, NULL);
		
		/* Notify display name observers */
		if (G_OBJECT (priv->cur_folder_store) == instance) {
			g_signal_emit (G_OBJECT(data),
					       signals[FOLDER_DISPLAY_NAME_CHANGED_SIGNAL], 0,
					       item_name);
		}
		
		g_free (item_name);
		
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
	TnyFolderType type;
	gchar *fname = NULL;
	const gchar *account_id = NULL;
	gint unread;
	
	rendobj = G_OBJECT(renderer);
	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN, &unread,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &instance,
			    -1);

	if (!fname)
		return;

	if (!instance) {
		g_free (fname);
		return;
	}
	
	if (type == TNY_FOLDER_TYPE_NORMAL || type == TNY_FOLDER_TYPE_UNKNOWN) {
		type = modest_tny_folder_guess_folder_type_from_name (fname);
	}

	switch (type) {
	case TNY_FOLDER_TYPE_ROOT:
		if (TNY_IS_ACCOUNT (instance)) {
			account_id = tny_account_get_id (TNY_ACCOUNT (instance));
			/*
			if (!strcmp (account_id, MODEST_ACTUAL_LOCAL_FOLDERS_ACCOUNT_ID)) {
				pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_LOCAL_FOLDERS);
			} else {
			*/
				if (!strcmp (account_id, MODEST_MMC_ACCOUNT_ID))
					pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_MMC);
				else
					pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_ACCOUNT);
			/*
			}
			*/
		}
		else if (modest_tny_folder_store_is_virtual_local_folders (
			TNY_FOLDER_STORE (instance))) {
			pixbuf = modest_platform_get_icon (MODEST_FOLDER_ICON_LOCAL_FOLDERS);
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
	g_free (fname);

	/* Set pixbuf */
	g_object_set (rendobj, "pixbuf", pixbuf, NULL);

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

	/* Initialize the local account name */
	conf = modest_runtime_get_conf();
	priv->local_account_name = modest_conf_get_string (conf, MODEST_CONF_DEVICE_NAME, NULL);

	/* Build treeview */
	add_columns (GTK_WIDGET (obj));

	/* Setup drag and drop */
	setup_drag_and_drop (GTK_TREE_VIEW(obj));

	/* Restore conf */
	modest_widget_memory_restore (conf, G_OBJECT (obj), MODEST_CONF_FOLDER_VIEW_KEY);

	/* Connect signals */
	g_signal_connect (G_OBJECT (obj), 
			  "key-press-event", 
			  G_CALLBACK (on_key_pressed), NULL);

	/*
	 * Track changes in the local account name (in the device it
	 * will be the device name)
	 */
	g_signal_connect (G_OBJECT(conf), 
			  "key_changed",
			  G_CALLBACK(on_configuration_key_changed), obj);

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

	if (priv->timer_expander != 0) {
		g_source_remove (priv->timer_expander);
		priv->timer_expander = 0;
	}

	if (priv->account_store) {
		g_signal_handler_disconnect (G_OBJECT(priv->account_store),
					     priv->account_update_signal);
		g_signal_handler_disconnect (G_OBJECT(priv->account_store),
					     priv->accounts_reloaded_signal);
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}

	if (priv->query) {
		g_object_unref (G_OBJECT (priv->query));
		priv->query = NULL;
	}

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(obj));
	if (sel)
		g_signal_handler_disconnect (G_OBJECT(sel), priv->changed_signal);

	g_free (priv->local_account_name);
	g_free (priv->visible_account_id);
	
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
						   priv->account_update_signal))
			g_signal_handler_disconnect (G_OBJECT (priv->account_store), 
						     priv->account_update_signal);
		if (g_signal_handler_is_connected (G_OBJECT (priv->account_store), 
						   priv->accounts_reloaded_signal))
			g_signal_handler_disconnect (G_OBJECT (priv->account_store), 
						     priv->accounts_reloaded_signal);

		g_object_unref (G_OBJECT (priv->account_store));
	}

	priv->account_store = g_object_ref (G_OBJECT (account_store));

	priv->account_update_signal = 
		g_signal_connect (G_OBJECT(account_store), "account_update",
				  G_CALLBACK (on_account_update), self);

	priv->accounts_reloaded_signal = 
		g_signal_connect (G_OBJECT(account_store), "accounts_reloaded",
				  G_CALLBACK (on_accounts_reloaded), self);
	
	if (!update_model (MODEST_FOLDER_VIEW (self),
			   MODEST_TNY_ACCOUNT_STORE (priv->account_store)))
		g_printerr ("modest: failed to update model\n");

	g_object_unref (G_OBJECT (device));
}

static void
on_account_update (TnyAccountStore *account_store, const gchar *account,
		   gpointer user_data)
{
	if (!update_model (MODEST_FOLDER_VIEW(user_data), 
			   MODEST_TNY_ACCOUNT_STORE(account_store)))
		g_printerr ("modest: failed to update model for changes in '%s'",
			    account);
}

static void 
on_accounts_reloaded   (TnyAccountStore *account_store, 
			gpointer user_data)
{
	update_model (MODEST_FOLDER_VIEW (user_data), 
		      MODEST_TNY_ACCOUNT_STORE(account_store));
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

	return GTK_WIDGET(self);
}

/* this feels dirty; any other way to expand all the root items? */
static void
expand_root_items (ModestFolderView *self)
{
	GtkTreePath *path;
	path = gtk_tree_path_new_first ();

	/* all folders should have child items, so.. */
	while (gtk_tree_view_expand_row (GTK_TREE_VIEW(self), path, FALSE))
		gtk_tree_path_next (path);
	
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
	gboolean retval = TRUE;
	gint type = 0;
	GObject *instance = NULL;

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
		/* TNY_FOLDER_TYPE_ROOT means that the instance is an account instead of a folder. */
		if (TNY_IS_ACCOUNT (instance)) {
			TnyAccount *acc = TNY_ACCOUNT (instance);
			const gchar *account_id = tny_account_get_id (acc);
			
			/* If it isn't a special folder, 
			 * don't show it unless it is the visible account: */
			if (strcmp (account_id, MODEST_MMC_ACCOUNT_ID)) { 
				/* Show only the visible account id */
				ModestFolderViewPrivate *priv = 
					MODEST_FOLDER_VIEW_GET_PRIVATE (data);
				if (priv->visible_account_id && strcmp (account_id, priv->visible_account_id))
					retval = FALSE;
			}
		}
	}
	
	/* The virtual local-folders folder store is also shown by default. */

	g_object_unref (instance);

	return retval;
}

/*
static void on_tnylist_accounts_debug_print(gpointer data,  gpointer user_data)
{
	TnyAccount* account = TNY_ACCOUNT(data);
	const gchar *prefix = (const gchar*)(user_data);
	
	printf("%s account id=%s\n", prefix, tny_account_get_id (account));
}
*/

static void
add_account_folders_to_merged_folder (TnyAccount *account, TnyMergeFolder* merge_folder)
{
	const gchar* account_id = tny_account_get_id (account);
	const gboolean is_actual_local_folders_account = account_id && 
		(strcmp (account_id, MODEST_ACTUAL_LOCAL_FOLDERS_ACCOUNT_ID) == 0);
		
	TnyList *list_outbox_folders = tny_simple_list_new ();
	tny_folder_store_get_folders (TNY_FOLDER_STORE (account), 
		list_outbox_folders, NULL, NULL);
		
	TnyIterator*  iter =  tny_list_create_iterator (list_outbox_folders);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
		
		if (folder) {
			gboolean add = TRUE;
			/* TODO: Do not add outboxes that are inside local-folders/, 
			 * because these are just left-over from earlier Modest versions 
			 * that put the outbox there: */
			if (is_actual_local_folders_account) {
				const TnyFolderType type = modest_tny_folder_get_local_folder_type (folder);
				if (type == TNY_FOLDER_TYPE_OUTBOX) {
					add = FALSE;
				}
			}
			
			if (add)
				tny_merge_folder_add_folder (merge_folder, folder);
				
			g_object_unref (folder);	
		}
		
		tny_iterator_next (iter);
	}
	
	g_object_unref (list_outbox_folders);
}


static void
add_account_folders_to_simple_folder_store (TnyAccount *account, ModestTnySimpleFolderStore* store)
{
	g_return_if_fail (account);
	g_return_if_fail (store);
		
	TnyList *list_outbox_folders = tny_simple_list_new ();
	tny_folder_store_get_folders (TNY_FOLDER_STORE (account), 
		list_outbox_folders, NULL, NULL);
	
	/* Special handling for the .modest/local-folders account,
	 * to avoid adding unwanted folders.
	 * We cannot prevent them from being in the TnyAccount without 
	 * changing the libtinymail-camel. */
	const gchar* account_id = tny_account_get_id (account);
	const gboolean is_actual_local_folders_account = account_id && 
		(strcmp (account_id, MODEST_ACTUAL_LOCAL_FOLDERS_ACCOUNT_ID) == 0);
	
	TnyIterator*  iter =  tny_list_create_iterator (list_outbox_folders);
	while (!tny_iterator_is_done (iter))
	{
		TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
		
		if (folder) {
			gboolean add = TRUE;
			/* TODO: Do not add outboxes that are inside local-folders/, 
			 * because these are just left-over from earlier Modest versions 
			 * that put the outbox there: */
			if (is_actual_local_folders_account) {
				const TnyFolderType type = modest_tny_folder_get_local_folder_type (folder);
				if (type == TNY_FOLDER_TYPE_OUTBOX) {
					add = FALSE;
				}
			}
			
			if (add)
				modest_tny_simple_folder_store_add_folder (store, folder);
				
			g_object_unref (folder);	
		}
		
		tny_iterator_next (iter);
	}
	
	g_object_unref (list_outbox_folders);
}

static gboolean
update_model (ModestFolderView *self, ModestTnyAccountStore *account_store)
{
	ModestFolderViewPrivate *priv;

	GtkTreeModel     *model;

	g_return_val_if_fail (account_store, FALSE);

	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(self);
	
	/* Notify that there is no folder selected */
	g_signal_emit (G_OBJECT(self), 
		       signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       NULL, TRUE);
	
	/* FIXME: the local accounts are not shown when the query
	   selects only the subscribed folders. */
/* 	model        = tny_gtk_folder_store_tree_model_new (TRUE, priv->query); */
	model        = tny_gtk_folder_store_tree_model_new (TRUE, NULL);
	
	/* Deal with the model via its TnyList Interface,
	 * filling the TnyList via a get_accounts() call: */
	TnyList *model_as_list = TNY_LIST(model);

	/* Create a virtual local-folders folder store, 
	 * containing the real local folders and the (merged) various per-account 
	 * outbox folders:
	 */
	ModestTnySimpleFolderStore *store = modest_tny_simple_folder_store_new ();

	/* Get the accounts: */
	TnyList *account_list = tny_simple_list_new ();
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(account_store),
					account_list,
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
	TnyIterator* iter =  tny_list_create_iterator (account_list);
	
	/* All per-account outbox folders are merged into one folders
	 * so that they appear as one outbox to the user: */
	TnyMergeFolder *merged_outbox = TNY_MERGE_FOLDER (tny_merge_folder_new());
	
	while (!tny_iterator_is_done (iter))
	{
		GObject *cur = tny_iterator_get_current (iter);
		TnyAccount *account = TNY_ACCOUNT (cur);
		if (account) {
			/* Add both outbox account and local-folders account folders
			 * to our one combined account:
			 */
			if (MODEST_IS_TNY_OUTBOX_ACCOUNT (account)) {
				/* Add the folder to the merged folder.
				 * We will add it later to the virtual local-folders store: */
				add_account_folders_to_merged_folder (account, merged_outbox);
			} else {
				const gchar *account_id = tny_account_get_id (account);
				if (account_id && !strcmp (account_id, MODEST_ACTUAL_LOCAL_FOLDERS_ACCOUNT_ID)) {
					/* Add the folders to the virtual local-folders store: */
					add_account_folders_to_simple_folder_store (account, store);
				}
				else {
					/* Just add the account: */
					tny_list_append (model_as_list, G_OBJECT (account));
				}
			}
		}
	   
		g_object_unref (cur);
		tny_iterator_next (iter);
	}
	
	/* Add the merged outbox folder to the virtual local-folders store: */
	modest_tny_simple_folder_store_add_folder (store, TNY_FOLDER(merged_outbox));
	g_object_unref (merged_outbox);
	merged_outbox = NULL;
	
	/* Add the virtual local-folders store to the model: */
	tny_list_append (model_as_list, G_OBJECT (store));
	
	
	g_object_unref (account_list);
	account_list = NULL;
	
	g_object_unref (model_as_list);
	model_as_list = NULL;	
		
	/* tny_list_foreach (account_list, on_tnylist_accounts_debug_print, "update_model: "); */
                                                     
	GtkTreeModel *filter_model = NULL, *sortable = NULL;

	sortable = gtk_tree_model_sort_new_with_model (model);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(sortable),
					      TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, 
					      GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
					 TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN,
					 cmp_rows, NULL, NULL);

	/* Create filter model */
	if (priv->style == MODEST_FOLDER_VIEW_STYLE_SHOW_ONE) {
		filter_model = gtk_tree_model_filter_new (sortable, NULL);
		gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
							filter_row,
							self,
							NULL);
	}

	/* Set new model */
	gtk_tree_view_set_model (GTK_TREE_VIEW(self), 
				 (filter_model) ? filter_model : sortable);
	expand_root_items (self); /* expand all account folders */
	
	return TRUE;
}


static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel            *model;
	TnyFolderStore          *folder = NULL;
	GtkTreeIter             iter;
	ModestFolderView        *tree_view;
	ModestFolderViewPrivate *priv;
	gint                    type;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(user_data);
	priv->cur_selection = sel;
	
	/* folder was _un_selected if true */
	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		if (priv->cur_folder_store)
			g_object_unref (priv->cur_folder_store);
		priv->cur_folder_store = NULL;

		/* Notify the display name observers */
		g_signal_emit (G_OBJECT(user_data),
			       signals[FOLDER_DISPLAY_NAME_CHANGED_SIGNAL], 0,
			       NULL);
		return;
	}

	tree_view = MODEST_FOLDER_VIEW (user_data);

	gtk_tree_model_get (model, &iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder,
			    -1);

	/* If the folder is the same do not notify */
	if (priv->cur_folder_store == folder) {
		g_object_unref (folder);
		return;
	}
	
	/* Current folder was unselected */
	if (priv->cur_folder_store) {
		g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
			       priv->cur_folder_store, FALSE);
		g_object_unref (priv->cur_folder_store);
	}

	/* New current references */
	priv->cur_folder_store = folder;

	/* New folder has been selected */
	g_signal_emit (G_OBJECT(tree_view),
		       signals[FOLDER_SELECTION_CHANGED_SIGNAL],
		       0, folder, TRUE);
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
	
	if (TNY_IS_FOLDER_STORE (folder) && 
		modest_tny_folder_store_is_virtual_local_folders (
			TNY_FOLDER_STORE (folder))) {
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
	gint cmp;
	gchar         *name1, *name2;
	TnyFolderType type;
	GObject *folder1 = NULL;
	GObject *folder2 = NULL;

	gtk_tree_model_get (tree_model, iter1,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &name1,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder1,
			    -1);
	gtk_tree_model_get (tree_model, iter2,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &name2,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder2,
			    -1);

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
	
			if (!account_id && !account_id2)
				return 0;
			else if (!account_id)
				return -1;
			else if (!account_id2)
				return +1;
			else if (!strcmp (account_id, MODEST_MMC_ACCOUNT_ID))
				cmp = +1;
			else
				cmp = modest_text_utils_utf8_strcmp (name1, name2, TRUE);
		}
	} else {
		cmp = modest_text_utils_utf8_strcmp (name1, name2, TRUE);
	}
	
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
	TnyList *headers;
	TnyHeader *header;
	TnyFolder *folder;
	ModestMailOperation *mail_op;
	GtkTreeIter source_iter, dest_iter;

	/* Get header */
	gtk_tree_model_get_iter (source_model, &source_iter, helper->source_row);
	gtk_tree_model_get (source_model, &source_iter, 
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
			    &header, -1);

	/* Get Folder */
	gtk_tree_model_get_iter (dest_model, &dest_iter, dest_row);
	gtk_tree_model_get (dest_model, &dest_iter, 
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
			    &folder, -1);

	/* Transfer message */
	mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_TYPE_RECEIVE, NULL);

	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);
	g_signal_connect (G_OBJECT (mail_op), "progress-changed",
			  G_CALLBACK (on_progress_changed), helper);

	/* FIXME: I replaced this because the API changed, but D&D
	   should be reviewed in order to allow multiple drags*/
	headers = tny_simple_list_new ();
	tny_list_append (headers, G_OBJECT (header));
	modest_mail_operation_xfer_msgs (mail_op, headers, folder, helper->delete_source, NULL, NULL);

	/* Frees */
	g_object_unref (G_OBJECT (mail_op));
	g_object_unref (G_OBJECT (header));
	g_object_unref (G_OBJECT (folder));
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
	ModestMailOperation *mail_op;
	GtkTreeIter parent_iter, iter;
	TnyFolderStore *parent_folder;
	TnyFolder *folder;

	/* Check if the drag is possible */
/* 	if (!gtk_tree_path_compare (helper->source_row, dest_row) || */
/* 	    !gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (dest_model), */
/* 						   dest_row, */
/* 						   selection_data)) { */
	if (!gtk_tree_path_compare (helper->source_row, dest_row)) {

		gtk_drag_finish (helper->context, FALSE, FALSE, helper->time);
		gtk_tree_path_free (helper->source_row);	
		g_slice_free (DndHelper, helper);
		return;
	}

	/* Get data */
	gtk_tree_model_get_iter (source_model, &parent_iter, dest_row);
	gtk_tree_model_get_iter (source_model, &iter, helper->source_row);
	gtk_tree_model_get (source_model, &parent_iter, 
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
			    &parent_folder, -1);
	gtk_tree_model_get (source_model, &iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN,
			    &folder, -1);

	/* Do the mail operation */
	mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_TYPE_RECEIVE, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), 
					 mail_op);
	g_signal_connect (G_OBJECT (mail_op), "progress-changed",
			  G_CALLBACK (on_progress_changed), helper);

	modest_mail_operation_xfer_folder (mail_op, 
					   folder, 
					   parent_folder,
					   helper->delete_source);
	
	/* Frees */
	g_object_unref (G_OBJECT (parent_folder));
	g_object_unref (G_OBJECT (folder));
	g_object_unref (G_OBJECT (mail_op));
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
		if (source_widget == widget)
			g_signal_emit (G_OBJECT (widget), 
				       signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0, NULL, TRUE);
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
			      ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	if (!key)
		return;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

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
	ModestConf *conf;
	GtkTreeModel *model;

	g_return_if_fail (self);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	/* This will be used by the filter_row callback,
	 * to decided which rows to show: */
	if (priv->visible_account_id)
		g_free (priv->visible_account_id);
	priv->visible_account_id = g_strdup (account_id);

	/* Save preferences */
	conf = modest_runtime_get_conf ();
	modest_widget_memory_save (conf, G_OBJECT (self), MODEST_CONF_FOLDER_VIEW_KEY);

	/* Refilter */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (GTK_IS_TREE_MODEL_FILTER (model))
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
}

const gchar *
modest_folder_view_get_account_id_of_visible_server_account (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	return (const gchar *) priv->visible_account_id;
}

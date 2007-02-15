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

#include <tny-account-store-view.h>
#include <tny-gtk-account-list-model.h>
#include <tny-gtk-folder-store-tree-model.h>
#include <tny-gtk-header-list-model.h>
#include <tny-folder.h>
#include <tny-folder-monitor.h>
#include <tny-account-store.h>
#include <tny-account.h>
#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-simple-list.h>
#include <modest-tny-folder.h>
#include <modest-marshal.h>
#include <modest-icon-names.h>
#include <modest-tny-account-store.h>
#include <modest-text-utils.h>
#include <modest-runtime.h>
#include "modest-folder-view.h"

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

static gboolean     update_model_empty     (ModestFolderView *self);

static void         on_selection_changed   (GtkTreeSelection *sel, gpointer data);

static void         on_account_update      (TnyAccountStore *account_store, 
					    const gchar *account,
					    gpointer user_data);

static gint         cmp_rows               (GtkTreeModel *tree_model, 
					    GtkTreeIter *iter1, 
					    GtkTreeIter *iter2,
					    gpointer user_data);

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
	TARGET_TREE_ROW,
};

static const GtkTargetEntry drag_types[] =
{
	{ "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, TARGET_TREE_ROW },
};

#define ROW_REF_DATA_NAME "row-ref"

enum {
	FOLDER_SELECTION_CHANGED_SIGNAL,
	FOLDER_MOVED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestFolderViewPrivate ModestFolderViewPrivate;
struct _ModestFolderViewPrivate {
	TnyAccountStore     *account_store;
	TnyFolder           *cur_folder;
	GtkTreeRowReference *cur_row;

	gulong               sig1, sig2;
	GMutex              *lock;
	
	GtkTreeSelection    *cur_selection;
	TnyFolderStoreQuery *query;
	guint                timer_expander;

	TnyFolderMonitor    *monitor;
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
	
	klass->update_model = modest_folder_view_update_model;

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

	/**
	 * ModestFolderView::folder-moved
	 * @self: the #MailOperation that emits the signal
	 * @folder: the #TnyFolder that is going to be moved
	 * @parent: then #TnyFolderStore that is going to be the new parent
	 * @done: indicates if the folder move was correctly completed or not
	 * @user_data: user data set when the signal handler was connected
	 *
	 * Emitted when a the user wants to move a folder through a
	 * drag and drop action
	 */
 	signals[FOLDER_MOVED_SIGNAL] = 
		g_signal_new ("folder_moved",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestFolderViewClass, folder_moved),
			      NULL, NULL,
			      modest_marshal_VOID__POINTER_POINTER_POINTER,
			      G_TYPE_NONE, 3, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);
}



static void
text_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer data)
{
	GObject *rendobj;
	gchar *fname;
	gint unread;
	TnyFolderType type;
	TnyFolder *folder;
	
	g_return_if_fail (column);
	g_return_if_fail (tree_model);

	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN, &unread,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder,
			    -1);
	rendobj = G_OBJECT(renderer);

	if (!fname)
		return;
	
	if (folder && type != TNY_FOLDER_TYPE_ROOT) { /* FIXME: tnymail bug? crashes with root folders */
		if (modest_tny_folder_is_local_folder (folder)) {
			TnyFolderType type;
			type = modest_tny_folder_get_local_folder_type (folder);
			if (type != TNY_FOLDER_TYPE_UNKNOWN) {
				g_free (fname);
				fname = g_strdup(modest_local_folder_info_get_type_display_name (type));
			}
		}
	} else if (folder && type == TNY_FOLDER_TYPE_ROOT) {
		if (strcmp (fname, MODEST_LOCAL_FOLDERS_ACCOUNT_NAME) == 0) {/* FIXME: hack */
			g_free (fname);
			fname = g_strdup (MODEST_LOCAL_FOLDERS_DISPLAY_NAME);
		}
	}
			
	if (unread > 0) {
		gchar *folder_title = g_strdup_printf ("%s (%d)", fname, unread);
		g_object_set (rendobj,"text", folder_title,  "weight", 800, NULL);
		g_free (folder_title);
	} else 
		g_object_set (rendobj,"text", fname, "weight", 400, NULL);
		
	g_free (fname);
	if (folder)
		g_object_unref (G_OBJECT(folder));
	
}


static GdkPixbuf*
get_cached_icon (const gchar *name)
{
	GError *err = NULL;
	gpointer pixbuf;
	gpointer orig_key;
	static GHashTable *icon_cache = NULL;
	
	g_return_val_if_fail (name, NULL);

	if (G_UNLIKELY(!icon_cache))
		icon_cache = modest_cache_mgr_get_cache (modest_runtime_get_cache_mgr(),
							 MODEST_CACHE_MGR_CACHE_TYPE_PIXBUF);
	
	if (!icon_cache || !g_hash_table_lookup_extended (icon_cache, name, &orig_key, &pixbuf)) {
		pixbuf = (gpointer)gdk_pixbuf_new_from_file (name, &err);
		if (!pixbuf) {
			g_printerr ("modest: error in icon factory while loading '%s': %s\n",
				    name, err->message);
			g_error_free (err);
		}
		/* if we cannot find it, we still insert (if we have a cache), so we get the error
		 * only once */
		if (icon_cache)
			g_hash_table_insert (icon_cache, g_strdup(name),(gpointer)pixbuf);
	}
	return GDK_PIXBUF(pixbuf);
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
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN, &unread, -1);
	rendobj = G_OBJECT(renderer);
	
	if (type == TNY_FOLDER_TYPE_NORMAL || type == TNY_FOLDER_TYPE_UNKNOWN) {
		type = modest_tny_folder_guess_folder_type_from_name (fname);
	}
	g_free (fname);

	switch (type) {
	case TNY_FOLDER_TYPE_ROOT:
		pixbuf = get_cached_icon (MODEST_FOLDER_ICON_ACCOUNT);
                break;
	case TNY_FOLDER_TYPE_INBOX:
                pixbuf = get_cached_icon (MODEST_FOLDER_ICON_INBOX);
                break;
        case TNY_FOLDER_TYPE_OUTBOX:
                pixbuf = get_cached_icon (MODEST_FOLDER_ICON_OUTBOX);
                break;
        case TNY_FOLDER_TYPE_JUNK:
                pixbuf = get_cached_icon (MODEST_FOLDER_ICON_JUNK);
                break;
        case TNY_FOLDER_TYPE_SENT:
                pixbuf = get_cached_icon (MODEST_FOLDER_ICON_SENT);
                break;
	case TNY_FOLDER_TYPE_TRASH:
		pixbuf = get_cached_icon (MODEST_FOLDER_ICON_TRASH);
                break;
	case TNY_FOLDER_TYPE_DRAFTS:
		pixbuf = get_cached_icon (MODEST_FOLDER_ICON_DRAFTS);
                break;
	case TNY_FOLDER_TYPE_NOTES:
		pixbuf = get_cached_icon (MODEST_FOLDER_ICON_NOTES);
                break;
	case TNY_FOLDER_TYPE_CALENDAR:
		pixbuf = get_cached_icon (MODEST_FOLDER_ICON_CALENDAR);
                break;
	case TNY_FOLDER_TYPE_CONTACTS:
                pixbuf = get_cached_icon (MODEST_FOLDER_ICON_CONTACTS);
                break;
	case TNY_FOLDER_TYPE_NORMAL:
        default:
                pixbuf = get_cached_icon (MODEST_FOLDER_ICON_NORMAL);
		break;
        }
	g_object_set (rendobj, "pixbuf", pixbuf, NULL);
}

static void
modest_folder_view_init (ModestFolderView *obj)
{
	ModestFolderViewPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;
	
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(obj);
	
	priv->timer_expander = 0;
	priv->account_store  = NULL;
	priv->cur_folder     = NULL;
	priv->cur_row        = NULL;
	priv->query          = NULL;
	priv->monitor	     = NULL;

	priv->lock           = g_mutex_new ();

	column = gtk_tree_view_column_new ();	
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj),column);
	
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
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(obj), FALSE);
	gtk_tree_view_set_enable_search     (GTK_TREE_VIEW(obj), FALSE);

	setup_drag_and_drop (GTK_TREE_VIEW(obj));
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
					     priv->sig1);
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}

	if (priv->lock) {
		g_mutex_free (priv->lock);
		priv->lock = NULL;
	}

	if (priv->query) {
		g_object_unref (G_OBJECT (priv->query));
		priv->query = NULL;
	}

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(obj));
	if (sel)
		g_signal_handler_disconnect (G_OBJECT(sel), priv->sig2);
	
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
		TnyDevice *old_device; 
		
		old_device = tny_account_store_get_device (priv->account_store);

		if (g_signal_handler_is_connected (G_OBJECT (old_device), priv->sig1))
			g_signal_handler_disconnect (G_OBJECT (old_device), 
						     priv->sig1);

		g_object_unref (G_OBJECT (priv->account_store));
		g_object_unref (G_OBJECT (old_device));
	}

	priv->account_store = g_object_ref (G_OBJECT (account_store));

	priv->sig1 = g_signal_connect (G_OBJECT(account_store), "account_update",
				       G_CALLBACK (on_account_update), self);
	
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

	priv->query = g_object_ref (query);
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	priv->sig2 = g_signal_connect (sel, "changed",
				       G_CALLBACK (on_selection_changed), self);
	return GTK_WIDGET(self);
}


static gboolean
update_model_empty (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	g_mutex_lock (priv->lock);
	{
		if (priv->monitor) {
			tny_folder_monitor_stop (priv->monitor);
			g_object_unref(G_OBJECT(priv->monitor));
			priv->monitor = NULL;
		}
	}
	g_mutex_unlock (priv->lock);
	
	g_signal_emit (G_OBJECT(self), signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       NULL, TRUE);
	return TRUE;
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

static gboolean
update_model (ModestFolderView *self, ModestTnyAccountStore *account_store)
{
	ModestFolderViewPrivate *priv;

	TnyList          *account_list;
	GtkTreeModel     *model, *sortable;

	g_return_val_if_fail (account_store, FALSE);

	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(self);
	
	/* Notify that there is no folder selected */
	update_model_empty (self);
	
	/* FIXME: the local accounts are not shown when the query
	   selects only the subscribed folders. */
/* 	model        = tny_gtk_folder_store_tree_model_new (TRUE, priv->query); */
	model        = tny_gtk_folder_store_tree_model_new (TRUE, NULL);
	account_list = TNY_LIST(model);

	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(account_store),
					account_list,
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);	
	if (account_list) {
		sortable = gtk_tree_model_sort_new_with_model (model);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(sortable),
						      TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, 
						      GTK_SORT_ASCENDING);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
						 TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN,
						 cmp_rows, NULL, NULL);

		/* Set new model */
		gtk_tree_view_set_model (GTK_TREE_VIEW(self), sortable);
		expand_root_items (self); /* expand all account folders */
		g_object_unref (account_list);
	}
	
	//if (model)
	//	g_object_unref (model);
		
	return TRUE;
}


static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel            *model_sort, *model;
	TnyFolder               *folder = NULL;
	GtkTreeIter             iter, iter_sort;
	GtkTreePath            *path;
	ModestFolderView        *tree_view;
	ModestFolderViewPrivate *priv;
	gint                    type;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(user_data);
	priv->cur_selection = sel;
	
	/* folder was _un_selected if true */
	if (!gtk_tree_selection_get_selected (sel, &model_sort, &iter_sort)) {
		priv->cur_folder = NULL; /* FIXME: need this? */
		priv->cur_row = NULL; /* FIXME: need this? */
		return; 
	}

	model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model_sort));
	gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (model_sort),
							&iter,
							&iter_sort);

	tree_view = MODEST_FOLDER_VIEW (user_data);

	gtk_tree_model_get (model, &iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder,
			    -1);

	if (type == TNY_FOLDER_TYPE_ROOT)
		return;
	
	/* emit 2 signals: one for the unselection of the old one,
	 * and one for the selection of the new on */
	g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       priv->cur_folder, FALSE);

	if (priv->cur_folder) {
/* 		tny_folder_sync (priv->cur_folder, TRUE, NULL); /\* FIXME *\/ */
		gtk_tree_row_reference_free (priv->cur_row);
	}
	priv->cur_folder = folder;
	g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       folder, TRUE);
 
	path = gtk_tree_model_get_path (model_sort, &iter_sort);
	priv->cur_row = gtk_tree_row_reference_new (model_sort, path);
	gtk_tree_path_free (path);
}

gboolean
modest_folder_view_update_model (ModestFolderView *self, TnyAccountStore *account_store)
{
	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);

	g_signal_emit (G_OBJECT(self), signals[FOLDER_SELECTION_CHANGED_SIGNAL],
		       0, NULL, TRUE);
	
	return update_model (self, MODEST_TNY_ACCOUNT_STORE(account_store)); /* ugly */
}

TnyFolder *
modest_folder_view_get_selected (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);
	if (priv->cur_folder)
		g_object_ref (priv->cur_folder);

	return priv->cur_folder;
}

static gboolean
get_model_iter (ModestFolderView *self, 
		GtkTreeModel **model, 
		GtkTreeIter *iter)
{
	GtkTreeModel *model_sort;
	GtkTreeIter iter_sort;
	GtkTreePath *path;
	ModestFolderViewPrivate *priv;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	if (!priv->cur_folder)
		return FALSE;

	if (!gtk_tree_row_reference_valid (priv->cur_row))
		return FALSE;

	model_sort = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	*model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model_sort));

	/* Get path to retrieve iter */
	path = gtk_tree_row_reference_get_path (priv->cur_row);
	if (!gtk_tree_model_get_iter (model_sort, &iter_sort, path))
		return FALSE;

	gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (model_sort),
							iter,
							&iter_sort);
	return TRUE;
}

gboolean 
modest_folder_view_rename (ModestFolderView *self)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	ModestFolderViewPrivate *priv;
	gchar *old_name;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);

	if (!get_model_iter (self, &model, &iter))
		return FALSE;

	/* Remove old name */
	gtk_tree_model_get (model, &iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, 
			    &old_name, -1);
	g_free (old_name);
		
	/* Set new name */
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN,
			    tny_folder_get_name (priv->cur_folder), -1);

	/* Invalidate selection */
	g_signal_emit (G_OBJECT(self), signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       priv->cur_folder, TRUE);

	return TRUE;
}

gboolean 
modest_folder_view_add_subfolder (ModestFolderView *self, TnyFolder *folder)
{
	GtkTreeModel *model;
	GtkTreeIter iter, child;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);

	if (!get_model_iter (self, &model, &iter))
		return FALSE;

	/* Append a new child to the folder */
	gtk_tree_store_append (GTK_TREE_STORE (model), &child, &iter);
	gtk_tree_store_set (GTK_TREE_STORE (model), &child,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, 
			    tny_folder_get_name (TNY_FOLDER (folder)),
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN, 
			    tny_folder_get_unread_count (TNY_FOLDER (folder)),
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN,
			    tny_folder_get_folder_type (TNY_FOLDER (folder)),
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN,
			    folder, -1);

	return TRUE;
}

static gint
cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
	  gpointer user_data)
{
	gint cmp;
	gchar         *name1, *name2;
	TnyFolderType type;
	TnyFolder     *folder1, *folder2;
	
	gtk_tree_model_get (tree_model, iter1,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &name1,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder1,
			    -1);
	gtk_tree_model_get (tree_model, iter2,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN, &name2,
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder2,
			    -1);

	/* local_folders should be the last one */
	if (type == TNY_FOLDER_TYPE_ROOT) {
		/* the account name is also the name of the root folder
		 * in case of local folders */
		if (name1 && strcmp (name1, MODEST_LOCAL_FOLDERS_ACCOUNT_NAME) == 0)
			cmp = +1;
		else if (name2 && strcmp (name2, MODEST_LOCAL_FOLDERS_ACCOUNT_NAME) == 0)
			cmp = -1;
		else 
			cmp = modest_text_utils_utf8_strcmp (name1, name2, TRUE);
	} else 
		cmp = modest_text_utils_utf8_strcmp (name1, name2, TRUE);

	
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
	GtkTreeModel *model_sort, *model;
	GtkTreeIter iter;
	GtkTreePath *source_row_sort;
	GtkTreePath *source_row;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	gtk_tree_selection_get_selected (selection, &model_sort, &iter);
	source_row_sort = gtk_tree_model_get_path (model_sort, &iter);

	/* Get the unsorted path and model */
	model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model_sort));
	source_row = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (model_sort),
								     source_row_sort);

	gtk_tree_set_row_drag_data (selection_data,
				    model,
				    source_row);

	gtk_tree_path_free (source_row_sort);
	gtk_tree_path_free (source_row);
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
	GtkTreeModel *model_sort, *dest_model, *source_model;
 	GtkTreePath *source_row, *dest_row, *child_dest_row;
	GtkTreeViewDropPosition pos;
	gboolean success = FALSE, delete_source = FALSE;

	/* Do not allow further process */
	g_signal_stop_emission_by_name (widget, "drag-data-received");

	/* Check if the get_data failed */
	if (selection_data == NULL || selection_data->length < 0)
		return;

	/* Get the action */
	if (context->action == GDK_ACTION_MOVE)
		delete_source = TRUE;

	/* Get the models */
	source_widget = gtk_drag_get_source_widget (context);
	model_sort = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

	gtk_tree_get_row_drag_data (selection_data,
				    &source_model,
				    &source_row);

	/* Select the destination model */
	if (source_widget == widget)
		dest_model = source_model;
	else
		dest_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model_sort));


	/* Get the path to the destination row. Can not call
	   gtk_tree_view_get_drag_dest_row() because the source row
	   it's not selected anymore */
	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget),
					   x, y,
					   &dest_row,
					   &pos);

	/* Only allow drops IN other rows */
	if (!dest_row || 
	    pos == GTK_TREE_VIEW_DROP_BEFORE ||
	    pos == GTK_TREE_VIEW_DROP_AFTER)
		goto finish;

	/* Get the destination row in the usorted model */
	child_dest_row = 
		gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (model_sort),
								dest_row);
	gtk_tree_path_free (dest_row);

	/* Drags from the header view */
	if ((target_type == TARGET_TREE_ROW) && (source_widget != widget)) {

		/* TODO: do the mail operation */

		success = TRUE;
	} else {
		GtkTreeRowReference *source_row_reference;
		GtkTreeIter parent_iter, iter;
		TnyFolder *folder;
		TnyFolderStore *parent_folder;
		gboolean done;

		/* Check if the drag is possible */
		if (!gtk_tree_path_compare (source_row, child_dest_row))
			goto out;

		if (!gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (dest_model),
							   child_dest_row,
							   selection_data))
			goto out;

		/* Do the mail operation */
		gtk_tree_model_get_iter (source_model, &parent_iter, child_dest_row);
		gtk_tree_model_get_iter (source_model, &iter, source_row);
		gtk_tree_model_get (source_model, &parent_iter, 
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				    &parent_folder, -1);
		gtk_tree_model_get (source_model, &iter, 
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, 
				    &folder, -1);
		
		g_signal_emit (G_OBJECT (widget), signals[FOLDER_MOVED_SIGNAL], 0,
			       folder, parent_folder, &done);
		if (!done)
			goto out;

		/* Get a row reference to the source path because the path
		   could change after the insertion. The gtk_drag_finish() is
		   not able to delete the source because that, so we have to
		   do it manually */
		source_row_reference = gtk_tree_row_reference_new (source_model, source_row);
		gtk_tree_path_free (source_row);

		/* Insert the dragged row as a child of the dest row */
		gtk_tree_path_down (child_dest_row);
		if (gtk_tree_drag_dest_drag_data_received (GTK_TREE_DRAG_DEST (dest_model),
							   child_dest_row,
							   selection_data)) {

			source_row = gtk_tree_row_reference_get_path (source_row_reference);

			success = TRUE;
		}
		gtk_tree_row_reference_free (source_row_reference);
		
		if (folder)
			g_object_unref (G_OBJECT(folder));
		if (parent_folder)
			g_object_unref (G_OBJECT(parent_folder));
		
	}
 out:
	gtk_tree_path_free (child_dest_row);

	/* Save the new path, will be used by the
	   drag-data-delete handler */
	if (success)
		g_object_set_data (G_OBJECT (source_widget),
				   ROW_REF_DATA_NAME,
				   gtk_tree_path_copy (source_row));

 finish:
	gtk_drag_finish (context, success, (success && delete_source), time);
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
	if (!context->targets)
		return FALSE;

	/* Request the data from the source. */
	gtk_drag_get_data(widget, 
			  context, 
			  GDK_POINTER_TO_ATOM (context->targets->data), 
			  time);

    return TRUE;
}

/*
 * This function deletes the data that has been dragged from its
 * source widget. Since is a function received by the source of the
 * drag, this function only deletes rows of the folder view
 * widget. The header view widget will need to define its own one.
 */
static void 
drag_data_delete_cb (GtkWidget      *widget,
		     GdkDragContext *context,
		     gpointer        user_data)
{
	GtkTreePath *source_row;
	GtkTreeModel *model_sort, *model;

	/* Clean dest row */
	gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (widget),
					 NULL,
					 GTK_TREE_VIEW_DROP_BEFORE);

	model_sort = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model_sort));
	source_row = g_object_steal_data (G_OBJECT (widget), ROW_REF_DATA_NAME);

	/* Delete the source row */
	gtk_tree_drag_source_drag_data_delete (GTK_TREE_DRAG_SOURCE (model),
					       source_row);

	gtk_tree_path_free (source_row);
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

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (widget);

	if (priv->timer_expander != 0) {
		g_source_remove (priv->timer_expander);
		priv->timer_expander = 0;
	}

	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget),
					   x, y,
					   &dest_row,
					   &pos);

	if (!dest_row)
		return FALSE;

	/* Expand the selected row after 1/2 second */
	if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), dest_row)) {
		gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (widget), dest_row, pos);
		priv->timer_expander = g_timeout_add (500, expand_row_timeout, widget);
	}
	gtk_tree_path_free (dest_row);

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

	return TRUE;
}

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
			   drag_types,
			   G_N_ELEMENTS (drag_types),
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);

	gtk_signal_connect(GTK_OBJECT (self),
			   "drag_data_received",
			   GTK_SIGNAL_FUNC(on_drag_data_received),
			   NULL);


	/* Set up the treeview as a dnd source */
	gtk_drag_source_set (GTK_WIDGET (self),
			     GDK_BUTTON1_MASK,
			     drag_types,
			     G_N_ELEMENTS (drag_types),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);

	gtk_signal_connect(GTK_OBJECT (self),
			   "drag_data_delete",
			   GTK_SIGNAL_FUNC(drag_data_delete_cb),
			   NULL);

	gtk_signal_connect(GTK_OBJECT (self),
			   "drag_motion",
			   GTK_SIGNAL_FUNC(on_drag_motion),
			   NULL);


	gtk_signal_connect(GTK_OBJECT (self),
			   "drag_data_get",
			   GTK_SIGNAL_FUNC(on_drag_data_get),
			   NULL);

	gtk_signal_connect(GTK_OBJECT (self),
			   "drag_drop",
			   GTK_SIGNAL_FUNC(drag_drop_cb),
			   NULL);
}

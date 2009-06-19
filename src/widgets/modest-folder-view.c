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
#include <tny-gtk-folder-list-store.h>
#include <tny-gtk-folder-store-tree-model.h>
#include <tny-gtk-header-list-model.h>
#include <tny-merge-folder.h>
#include <tny-folder.h>
#include <tny-folder-store-observer.h>
#include <tny-account-store.h>
#include <tny-account.h>
#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-simple-list.h>
#include <tny-camel-account.h>
#include <modest-defs.h>
#include <modest-tny-account.h>
#include <modest-tny-folder.h>
#include <modest-tny-local-folders-account.h>
#include <modest-tny-outbox-account.h>
#include <modest-marshal.h>
#include <modest-icon-names.h>
#include <modest-tny-account-store.h>
#include <modest-tny-local-folders-account.h>
#include <modest-text-utils.h>
#include <modest-runtime.h>
#include "modest-folder-view.h"
#include <modest-platform.h>
#include <modest-widget-memory.h>
#include <modest-ui-actions.h>
#include "modest-dnd.h"
#include "modest-ui-constants.h"
#include "widgets/modest-window.h"
#include <modest-account-protocol.h>

/* Folder view drag types */
const GtkTargetEntry folder_view_drag_types[] =
{
	{ "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, MODEST_FOLDER_ROW },
	{ GTK_TREE_PATH_AS_STRING_LIST, GTK_TARGET_SAME_APP, MODEST_HEADER_ROW }
};

/* Default icon sizes for Fremantle style are different */
#ifdef MODEST_TOOLKIT_HILDON2
#define FOLDER_ICON_SIZE MODEST_ICON_SIZE_BIG
#else
#define FOLDER_ICON_SIZE MODEST_ICON_SIZE_SMALL
#endif

/* Column names depending on we use list store or tree store */
#ifdef MODEST_TOOLKIT_HILDON2
#define NAME_COLUMN TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN
#define UNREAD_COLUMN TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN
#define ALL_COLUMN TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN
#define TYPE_COLUMN TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN
#define INSTANCE_COLUMN TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN
#else
#define NAME_COLUMN TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN
#define UNREAD_COLUMN TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN
#define ALL_COLUMN TNY_GTK_FOLDER_STORE_TREE_MODEL_ALL_COLUMN
#define TYPE_COLUMN TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN
#define INSTANCE_COLUMN TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN
#endif

/* 'private'/'protected' functions */
static void modest_folder_view_class_init  (ModestFolderViewClass *klass);
static void modest_folder_view_init        (ModestFolderView *obj);
static void modest_folder_view_finalize    (GObject *obj);
static void modest_folder_view_dispose     (GObject *obj);

static void         tny_account_store_view_init (gpointer g,
						 gpointer iface_data);

static void         modest_folder_view_set_account_store (TnyAccountStoreView *self,
							  TnyAccountStore     *account_store);

static void         on_selection_changed   (GtkTreeSelection *sel,
					    gpointer data);

static void         on_row_activated       (GtkTreeView *treeview,
					    GtkTreePath *path,
					    GtkTreeViewColumn *column,
					    gpointer userdata);

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

#ifndef MODEST_TOOLKIT_HILDON2
static void         on_row_inserted_maybe_select_folder (GtkTreeModel     *tree_model,
							 GtkTreePath      *path,
							 GtkTreeIter      *iter,
							 ModestFolderView *self);
#endif

static void         on_display_name_changed (ModestAccountMgr *self,
					     const gchar *account,
					     gpointer user_data);
static void         update_style (ModestFolderView *self);
static void         on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata);
static gint         get_cmp_pos (TnyFolderType t, TnyFolder *folder_store);
static gboolean     inbox_is_special (TnyFolderStore *folder_store);

static gboolean     get_inner_models        (ModestFolderView *self,
					     GtkTreeModel **filter_model,
					     GtkTreeModel **sort_model,
					     GtkTreeModel **tny_model);
#ifdef MODEST_TOOLKIT_HILDON2
static void on_activity_changed (TnyGtkFolderListStore *store,
				 gboolean activity,
				 ModestFolderView *folder_view);
#endif

enum {
	FOLDER_SELECTION_CHANGED_SIGNAL,
	FOLDER_DISPLAY_NAME_CHANGED_SIGNAL,
	FOLDER_ACTIVATED_SIGNAL,
	VISIBLE_ACCOUNT_CHANGED_SIGNAL,
	ACTIVITY_CHANGED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestFolderViewPrivate ModestFolderViewPrivate;
struct _ModestFolderViewPrivate {
	TnyAccountStore      *account_store;
	TnyFolderStore       *cur_folder_store;

	TnyFolder            *folder_to_select; /* folder to select after the next update */

	/* not unref this object, its a singlenton */
	ModestEmailClipboard *clipboard;

	/* Filter tree model */
	gchar **hidding_ids;
	guint n_selected;
	ModestFolderViewFilter filter;

	TnyFolderStoreQuery  *query;
	gboolean              do_refresh;
	guint                 timer_expander;

	gchar                *local_account_name;
	gchar                *visible_account_id;
	gchar                *mailbox;
	ModestFolderViewStyle style;
	ModestFolderViewCellStyle cell_style;
	gboolean show_message_count;

	gboolean  reselect; /* we use this to force a reselection of the INBOX */
	gboolean  show_non_move;
	TnyList   *list_to_move;
	gboolean  reexpand; /* next time we expose, we'll expand all root folders */

	GtkCellRenderer *messages_renderer;

	GSList   *signal_handlers;
	GdkColor active_color;
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
	GtkTreeViewClass *treeview_class;
	gobject_class = (GObjectClass*) klass;
	treeview_class = (GtkTreeViewClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_folder_view_finalize;
	gobject_class->finalize = modest_folder_view_dispose;

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

 	signals[FOLDER_ACTIVATED_SIGNAL] =
		g_signal_new ("folder_activated",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestFolderViewClass,
					       folder_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

	/*
	 * Emitted whenever the visible account changes
	 */
	signals[VISIBLE_ACCOUNT_CHANGED_SIGNAL] =
		g_signal_new ("visible-account-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestFolderViewClass,
					       visible_account_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	/*
	 * Emitted when the underlying GtkListStore is updating data
	 */
	signals[ACTIVITY_CHANGED_SIGNAL] =
		g_signal_new ("activity-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestFolderViewClass,
					       activity_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	treeview_class->select_cursor_parent = NULL;

#ifdef MODEST_TOOLKIT_HILDON2
	gtk_rc_parse_string ("class \"ModestFolderView\" style \"fremantle-touchlist\"");
	
#endif

}

/* Retrieves the filter, sort and tny models of the folder view. If
   any of these does not exist then it returns FALSE */
static gboolean
get_inner_models (ModestFolderView *self, 
		  GtkTreeModel **filter_model,
		  GtkTreeModel **sort_model,
		  GtkTreeModel **tny_model)
{
	GtkTreeModel *s_model, *f_model, *t_model;

	f_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (!GTK_IS_TREE_MODEL_FILTER(f_model)) {
		g_debug ("%s: emtpy model or not filter model", __FUNCTION__);
		return FALSE;
	}

	s_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (f_model));
	if (!GTK_IS_TREE_MODEL_SORT(s_model)) {
		g_warning ("BUG: %s: not a valid sort model", __FUNCTION__);
		return FALSE;
	}

	t_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (s_model));

	/* Assign values */
	if (filter_model)
		*filter_model = f_model;
	if (sort_model)
		*sort_model = s_model;
	if (tny_model)
		*tny_model = t_model;

	return TRUE;
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
			    INSTANCE_COLUMN, &instance,
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
convert_parent_folders_to_dots (gchar **item_name)
{
	gint n_parents = 0;
	gint n_inbox_parents = 0;
	gchar *c;
	gchar *path_start;
	gchar *last_separator;

	if (item_name == NULL)
		return;

	path_start = *item_name;
	for (c = *item_name; *c != '\0'; c++) {
		if (g_str_has_prefix (c, MODEST_FOLDER_PATH_SEPARATOR)) {
			gchar *compare;
			if (c != path_start) {
				compare = g_strndup (path_start, c - path_start);
				compare = g_strstrip (compare);
				if (g_ascii_strcasecmp (compare, "inbox") == 0) {
					n_inbox_parents++;
				}
				g_free (compare);
			}
			n_parents++;
			path_start = c + 1;
		}
	}

	last_separator = g_strrstr (*item_name, MODEST_FOLDER_PATH_SEPARATOR);
	if (last_separator != NULL) {
		last_separator = last_separator + strlen (MODEST_FOLDER_PATH_SEPARATOR);
	}

	if (n_parents > 0) {
		GString *buffer;
		gint i;

		buffer = g_string_new ("");
		for (i = 0; i < n_parents - n_inbox_parents; i++) {
			buffer = g_string_append (buffer, MODEST_FOLDER_DOT);
		}
		buffer = g_string_append (buffer, last_separator);
		g_free (*item_name);
		*item_name = g_string_free (buffer, FALSE);
	}

}

static void
format_compact_style (gchar **item_name,
		      GObject *instance,
		      const gchar *mailbox,
		      gboolean bold,
		      gboolean multiaccount,
		      gboolean *use_markup)
{
	TnyFolder *folder;
	gboolean is_special;
	TnyFolderType folder_type;

	if (!TNY_IS_FOLDER (instance))
		return;

	folder = (TnyFolder *) instance;

	folder_type = tny_folder_get_folder_type (folder);
	is_special = (get_cmp_pos (folder_type, folder)!= 4);

	if (mailbox) {
		/* Remove mailbox prefix if any */
		gchar *prefix = g_strconcat (mailbox, MODEST_FOLDER_PATH_SEPARATOR, NULL);
		if (g_str_has_prefix (*item_name, prefix)) {
			gchar *new_item_name = g_strdup (*item_name + strlen (prefix));
			g_free (*item_name);
			*item_name = new_item_name;
		}
	}

	if (!is_special || multiaccount) {
		TnyAccount *account = tny_folder_get_account (folder);
		const gchar *folder_name;
		gboolean concat_folder_name = FALSE;
		GString *buffer;

		/* Should not happen */
		if (account == NULL)
			return;

		/* convert parent folders to dots */
		convert_parent_folders_to_dots  (item_name);

		folder_name = tny_folder_get_name (folder);
		if (g_str_has_suffix (*item_name, folder_name)) {
			gchar *offset = g_strrstr (*item_name, folder_name);
			*offset = '\0';
			concat_folder_name = TRUE;
		}

		buffer = g_string_new ("");

		buffer = g_string_append (buffer, *item_name);
		if (concat_folder_name) {
			buffer = g_string_append (buffer, folder_name);
		}
		g_free (*item_name);
		g_object_unref (account);

		*item_name = g_string_free (buffer, FALSE);
		*use_markup = FALSE;
	} else {
		*use_markup = FALSE;
	}
}

static void
text_cell_data  (GtkTreeViewColumn *column,
		 GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,
		 GtkTreeIter *iter,
		 gpointer data)
{
	ModestFolderViewPrivate *priv;
	GObject *rendobj = (GObject *) renderer;
	gchar *fname = NULL;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	GObject *instance = NULL;
	gboolean use_markup = FALSE;

	gtk_tree_model_get (tree_model, iter,
			    NAME_COLUMN, &fname,
			    TYPE_COLUMN, &type,
			    INSTANCE_COLUMN, &instance,
			    -1);
	if (!fname || !instance)
		goto end;

	ModestFolderView *self = MODEST_FOLDER_VIEW (data);
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	gchar *item_name = NULL;
	gint item_weight = 400;

	if (type != TNY_FOLDER_TYPE_ROOT) {
		gint number = 0;
		gboolean drafts;
		gboolean is_local;

		is_local = modest_tny_folder_is_local_folder (TNY_FOLDER (instance)) ||
			modest_tny_folder_is_memory_card_folder (TNY_FOLDER (instance));

		if (is_local) {
			type = modest_tny_folder_get_local_or_mmc_folder_type (TNY_FOLDER (instance));
			if (type != TNY_FOLDER_TYPE_UNKNOWN) {
				g_free (fname);
				fname = g_strdup (modest_local_folder_info_get_type_display_name (type));
			}
		} else {
			/* Sometimes an special folder is reported by the server as
			   NORMAL, like some versions of Dovecot */
			if (type == TNY_FOLDER_TYPE_NORMAL ||
			    type == TNY_FOLDER_TYPE_UNKNOWN) {
				type = modest_tny_folder_guess_folder_type (TNY_FOLDER (instance));
			}
		}

		/* note: we cannot reliably get the counts from the
		 * tree model, we need to use explicit calls on
		 * tny_folder for some reason. Select the number to
		 * show: the unread or unsent messages. in case of
		 * outbox/drafts, show all */
		if (is_local && ((type == TNY_FOLDER_TYPE_DRAFTS) ||
				 (type == TNY_FOLDER_TYPE_OUTBOX) ||
				 (type == TNY_FOLDER_TYPE_MERGE))) { /* _OUTBOX actually returns _MERGE... */
			number = tny_folder_get_all_count (TNY_FOLDER(instance));
			drafts = TRUE;
		} else {
			number = tny_folder_get_unread_count (TNY_FOLDER(instance));
			drafts = FALSE;
		}

		if (priv->cell_style == MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT) {
			item_name = g_strdup (fname);
			if (number > 0) {
				item_weight = 800;
			} else {
				item_weight = 400;
			}
		} else {
			/* Use bold font style if there are unread or unset messages */
			if (number > 0) {
				if (priv->show_message_count) {
					item_name = g_strdup_printf ("%s (%d)", fname, number);
				} else {
					item_name = g_strdup (fname);
				}
				item_weight = 800;
			} else {
				item_name = g_strdup (fname);
				item_weight = 400;
			}
		}

	} else if (TNY_IS_ACCOUNT (instance)) {
		/* If it's a server account */
		if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (instance))) {
			item_name = g_strdup (priv->local_account_name);
			item_weight = 800;
		} else if (modest_tny_account_is_memory_card_account (TNY_ACCOUNT (instance))) {
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

	/* Convert INBOX */
	if (type == TNY_FOLDER_TYPE_INBOX &&
	    !g_ascii_strcasecmp (fname, "Inbox")) {
		g_free (item_name);
		item_name = g_strdup (_("mcen_me_folder_inbox"));
	}

	if (!item_name)
		item_name = g_strdup ("unknown");

	if (priv->cell_style == MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT) {
		gboolean multiaccount;

		multiaccount = (priv->style == MODEST_FOLDER_VIEW_STYLE_SHOW_ALL);
		/* Convert item_name to markup */
		format_compact_style (&item_name, instance, priv->mailbox,
				      item_weight == 800, 
				      multiaccount, &use_markup);
	}

	if (item_name && item_weight) {
		/* Set the name in the treeview cell: */
		if (priv->cell_style == MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT && item_weight == 800 && 
		    (priv->active_color.red != 0 || priv->active_color.blue != 0 || priv->active_color.green != 0)) {
			g_object_set (rendobj, 
				      "text", item_name, 
				      "weight-set", FALSE,
				      "foreground-set", TRUE,
				      "foreground-gdk", &(priv->active_color),
				      NULL);
		} else {
			g_object_set (rendobj, 
				      "text", item_name,
				      "foreground-set", FALSE,
				      "weight-set", TRUE, 
				      "weight", item_weight,
				      NULL);
		}

		/* Notify display name observers */
		/* TODO: What listens for this signal, and how can it use only the new name? */
		if (((GObject *) priv->cur_folder_store) == instance) {
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
 end:
	if (instance)
		g_object_unref (G_OBJECT (instance));
	if (fname)
		g_free (fname);
}

static void
messages_cell_data  (GtkTreeViewColumn *column,
		 GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,
		 GtkTreeIter *iter,
		 gpointer data)
{
	ModestFolderView *self; 
	ModestFolderViewPrivate *priv;
	GObject *rendobj = (GObject *) renderer;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	GObject *instance = NULL;
	gchar *item_name = NULL;

	gtk_tree_model_get (tree_model, iter,
			    TYPE_COLUMN, &type,
			    INSTANCE_COLUMN, &instance,
			    -1);
	if (!instance)
		goto end;

	self = MODEST_FOLDER_VIEW (data);
	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE (self);


	if (type != TNY_FOLDER_TYPE_ROOT) {
		gint number = 0;
		gboolean drafts;
		gboolean is_local;

		is_local = modest_tny_folder_is_local_folder (TNY_FOLDER (instance)) ||
			modest_tny_folder_is_memory_card_folder (TNY_FOLDER (instance));

		if (is_local) {
			type = modest_tny_folder_get_local_or_mmc_folder_type (TNY_FOLDER (instance));
		} else {
			/* Sometimes an special folder is reported by the server as
			   NORMAL, like some versions of Dovecot */
			if (type == TNY_FOLDER_TYPE_NORMAL ||
			    type == TNY_FOLDER_TYPE_UNKNOWN) {
				type = modest_tny_folder_guess_folder_type (TNY_FOLDER (instance));
			}
		}

		/* note: we cannot reliably get the counts from the tree model, we need
		 * to use explicit calls on tny_folder for some reason.
		 */
		/* Select the number to show: the unread or unsent messages. in case of outbox/drafts, show all */
		if (is_local && ((type == TNY_FOLDER_TYPE_DRAFTS) ||
				 (type == TNY_FOLDER_TYPE_OUTBOX) ||
				 (type == TNY_FOLDER_TYPE_MERGE))) { /* _OUTBOX actually returns _MERGE... */
			number = tny_folder_get_all_count (TNY_FOLDER(instance));
			drafts = TRUE;
		} else {
			number = tny_folder_get_unread_count (TNY_FOLDER(instance));
			drafts = FALSE;
		}

		if ((priv->cell_style == MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT) && (number > 0)) {
			item_name =
				g_strdup_printf (ngettext ((drafts) ? "mcen_ti_message" : "mcen_va_new_message",
							   (drafts) ? "mcen_ti_messages" : "mcen_va_new_messages",
							   number), number);
		}
	}

	if (!item_name)
		item_name = g_strdup ("");

	if (item_name) {
		/* Set the name in the treeview cell: */
		g_object_set (rendobj,"text", item_name, NULL);

		g_free (item_name);

	}

 end:
	if (instance)
		g_object_unref (G_OBJECT (instance));
}


typedef struct {
	GdkPixbuf *pixbuf;
	GdkPixbuf *pixbuf_open;
	GdkPixbuf *pixbuf_close;
} ThreePixbufs;


static inline GdkPixbuf *
get_composite_pixbuf (const gchar *icon_name,
		      const gint size,
		      GdkPixbuf *base_pixbuf)
{
	GdkPixbuf *emblem, *retval = NULL;

	emblem = modest_platform_get_icon (icon_name, size);
	if (emblem) {
		retval = gdk_pixbuf_copy (base_pixbuf);
		gdk_pixbuf_composite (emblem, retval, 0, 0,
				      MIN (gdk_pixbuf_get_width (emblem),
					   gdk_pixbuf_get_width (retval)),
				      MIN (gdk_pixbuf_get_height (emblem),
					   gdk_pixbuf_get_height (retval)),
						  0, 0, 1, 1, GDK_INTERP_NEAREST, 255);
		g_object_unref (emblem);
	}
	return retval;
}

static inline ThreePixbufs *
get_composite_icons (const gchar *icon_code,
		     GdkPixbuf **pixbuf,
		     GdkPixbuf **pixbuf_open,
		     GdkPixbuf **pixbuf_close)
{
	ThreePixbufs *retval;

	if (!*pixbuf) {
		GdkPixbuf *icon;
		icon = modest_platform_get_icon (icon_code, FOLDER_ICON_SIZE);
		if (icon) {
			*pixbuf = gdk_pixbuf_copy (icon);
		} else {
			*pixbuf = NULL;
		}
	}

	if (!*pixbuf_open && pixbuf && *pixbuf)
		*pixbuf_open = get_composite_pixbuf ("qgn_list_gene_fldr_exp",
						     FOLDER_ICON_SIZE,
						     *pixbuf);

	if (!*pixbuf_close && pixbuf && *pixbuf)
		*pixbuf_close = get_composite_pixbuf ("qgn_list_gene_fldr_clp",
						      FOLDER_ICON_SIZE,
						      *pixbuf);

	retval = g_slice_new0 (ThreePixbufs);
	if (*pixbuf)
		retval->pixbuf = g_object_ref (*pixbuf);
	if (*pixbuf_open)
		retval->pixbuf_open = g_object_ref (*pixbuf_open);
	if (*pixbuf_close)
		retval->pixbuf_close = g_object_ref (*pixbuf_close);

	return retval;
}

static inline ThreePixbufs *
get_account_protocol_pixbufs (ModestFolderView *folder_view,
			      ModestProtocolType protocol_type,
			      GObject *object)
{
	ModestProtocol *protocol;
	const GdkPixbuf *pixbuf = NULL;
	ModestFolderViewPrivate *priv;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (folder_view);

	protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (),
								  protocol_type);

	if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
		pixbuf = modest_account_protocol_get_icon (MODEST_ACCOUNT_PROTOCOL (protocol), 
							   priv->filter & MODEST_FOLDER_VIEW_FILTER_SHOW_ONLY_MAILBOXES?
							   MODEST_ACCOUNT_PROTOCOL_ICON_MAILBOX:
							   MODEST_ACCOUNT_PROTOCOL_ICON_FOLDER,
							   object, FOLDER_ICON_SIZE);
	}

	if (pixbuf) {
		ThreePixbufs *retval;
		retval = g_slice_new0 (ThreePixbufs);
		retval->pixbuf = g_object_ref ((GObject *) pixbuf);
		retval->pixbuf_open = g_object_ref ((GObject *) pixbuf);
		retval->pixbuf_close = g_object_ref ((GObject *) pixbuf);
		return retval;
	} else {
		return NULL;
	}
}

static inline ThreePixbufs*
get_folder_icons (ModestFolderView *folder_view, TnyFolderType type, GObject *instance)
{
	TnyAccount *account = NULL;
	static GdkPixbuf *inbox_pixbuf = NULL, *outbox_pixbuf = NULL,
		*junk_pixbuf = NULL, *sent_pixbuf = NULL,
		*trash_pixbuf = NULL, *draft_pixbuf = NULL,
		*normal_pixbuf = NULL, *anorm_pixbuf = NULL, *mmc_pixbuf = NULL,
		*ammc_pixbuf = NULL, *avirt_pixbuf = NULL;

	static GdkPixbuf *inbox_pixbuf_open = NULL, *outbox_pixbuf_open = NULL,
		*junk_pixbuf_open = NULL, *sent_pixbuf_open = NULL,
		*trash_pixbuf_open = NULL, *draft_pixbuf_open = NULL,
		*normal_pixbuf_open = NULL, *anorm_pixbuf_open = NULL, *mmc_pixbuf_open = NULL,
		*ammc_pixbuf_open = NULL, *avirt_pixbuf_open = NULL;

	static GdkPixbuf *inbox_pixbuf_close = NULL, *outbox_pixbuf_close = NULL,
		*junk_pixbuf_close = NULL, *sent_pixbuf_close = NULL,
		*trash_pixbuf_close = NULL, *draft_pixbuf_close = NULL,
		*normal_pixbuf_close = NULL, *anorm_pixbuf_close = NULL, *mmc_pixbuf_close = NULL,
		*ammc_pixbuf_close = NULL, *avirt_pixbuf_close = NULL;

	ThreePixbufs *retval = NULL;

	if (TNY_IS_ACCOUNT (instance)) {
		account = g_object_ref (instance);
	} else if (TNY_IS_FOLDER (instance) && !TNY_IS_MERGE_FOLDER (instance)) {
		account = tny_folder_get_account (TNY_FOLDER (instance));
	}

	if (account) {
		ModestProtocolType account_store_protocol;

		account_store_protocol = modest_tny_account_get_protocol_type (account);
		retval = get_account_protocol_pixbufs (folder_view, account_store_protocol, instance);
		g_object_unref (account);
	}

	if (retval)
		return retval;

	/* Sometimes an special folder is reported by the server as
	   NORMAL, like some versions of Dovecot */
	if (type == TNY_FOLDER_TYPE_NORMAL ||
	    type == TNY_FOLDER_TYPE_UNKNOWN) {
		type = modest_tny_folder_guess_folder_type (TNY_FOLDER (instance));
	}

	/* It's not enough with check the folder type. We need to
	   ensure that we're not giving a special folder icon to a
	   normal folder with the same name than a special folder */
	if (TNY_IS_FOLDER (instance) &&
	    get_cmp_pos (type, TNY_FOLDER (instance)) ==  4)
		type = TNY_FOLDER_TYPE_NORMAL;

	/* Remote folders should not be treated as special folders */
	if (TNY_IS_FOLDER_STORE (instance) &&
	    !TNY_IS_ACCOUNT (instance) &&
	    type != TNY_FOLDER_TYPE_INBOX &&
	    modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (instance))) {
#ifdef MODEST_TOOLKIT_HILDON2
		return get_composite_icons (MODEST_FOLDER_ICON_REMOTE_FOLDER,
					    &anorm_pixbuf,
					    &anorm_pixbuf_open,
					    &anorm_pixbuf_close);
#else
		return get_composite_icons (MODEST_FOLDER_ICON_NORMAL,
					    &normal_pixbuf,
					    &normal_pixbuf_open,
					    &normal_pixbuf_close);
#endif
	}

	switch (type) {

	case TNY_FOLDER_TYPE_INVALID:
		g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
		break;

	case TNY_FOLDER_TYPE_ROOT:
		if (TNY_IS_ACCOUNT (instance)) {

			if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (instance))) {
				retval = get_composite_icons (MODEST_FOLDER_ICON_LOCAL_FOLDERS,
							      &avirt_pixbuf,
							      &avirt_pixbuf_open,
							      &avirt_pixbuf_close);
			} else {
				const gchar *account_id = tny_account_get_id (TNY_ACCOUNT (instance));

				if (!strcmp (account_id, MODEST_MMC_ACCOUNT_ID)) {
					retval = get_composite_icons (MODEST_FOLDER_ICON_MMC,
								      &ammc_pixbuf,
								      &ammc_pixbuf_open,
								      &ammc_pixbuf_close);
				} else {
					retval = get_composite_icons (MODEST_FOLDER_ICON_ACCOUNT,
								      &anorm_pixbuf,
								      &anorm_pixbuf_open,
								      &anorm_pixbuf_close);
				}
			}
		}
		break;
	case TNY_FOLDER_TYPE_INBOX:
		retval = get_composite_icons (MODEST_FOLDER_ICON_INBOX,
					      &inbox_pixbuf,
					      &inbox_pixbuf_open,
					      &inbox_pixbuf_close);
		break;
	case TNY_FOLDER_TYPE_OUTBOX:
		retval = get_composite_icons (MODEST_FOLDER_ICON_OUTBOX,
					      &outbox_pixbuf,
					      &outbox_pixbuf_open,
					      &outbox_pixbuf_close);
		break;
	case TNY_FOLDER_TYPE_JUNK:
		retval = get_composite_icons (MODEST_FOLDER_ICON_JUNK,
					      &junk_pixbuf,
					      &junk_pixbuf_open,
					      &junk_pixbuf_close);
		break;
	case TNY_FOLDER_TYPE_SENT:
		retval = get_composite_icons (MODEST_FOLDER_ICON_SENT,
					      &sent_pixbuf,
					      &sent_pixbuf_open,
					      &sent_pixbuf_close);
		break;
	case TNY_FOLDER_TYPE_TRASH:
		retval = get_composite_icons (MODEST_FOLDER_ICON_TRASH,
					      &trash_pixbuf,
					      &trash_pixbuf_open,
					      &trash_pixbuf_close);
		break;
	case TNY_FOLDER_TYPE_DRAFTS:
		retval = get_composite_icons (MODEST_FOLDER_ICON_DRAFTS,
					      &draft_pixbuf,
					      &draft_pixbuf_open,
					      &draft_pixbuf_close);
		break;
	case TNY_FOLDER_TYPE_ARCHIVE:
		retval = get_composite_icons (MODEST_FOLDER_ICON_MMC_FOLDER,
					      &mmc_pixbuf,
					      &mmc_pixbuf_open,
					      &mmc_pixbuf_close);
		break;
	case TNY_FOLDER_TYPE_NORMAL:
	default:
		/* Memory card folders could have an special icon */
		if (modest_tny_folder_is_memory_card_folder (TNY_FOLDER (instance))) {
			retval = get_composite_icons (MODEST_FOLDER_ICON_MMC_FOLDER,
						      &mmc_pixbuf,
						      &mmc_pixbuf_open,
						      &mmc_pixbuf_close);
		} else {
			retval = get_composite_icons (MODEST_FOLDER_ICON_NORMAL,
						      &normal_pixbuf,
						      &normal_pixbuf_open,
						      &normal_pixbuf_close);
		}
		break;
	}

	return retval;
}

static void
free_pixbufs (ThreePixbufs *pixbufs)
{
	if (pixbufs->pixbuf)
		g_object_unref (pixbufs->pixbuf);
	if (pixbufs->pixbuf_open)
		g_object_unref (pixbufs->pixbuf_open);
	if (pixbufs->pixbuf_close)
		g_object_unref (pixbufs->pixbuf_close);
	g_slice_free (ThreePixbufs, pixbufs);
}

static void
icon_cell_data  (GtkTreeViewColumn *column,
		 GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,
		 GtkTreeIter *iter,
		 gpointer data)
{
	GObject *rendobj = NULL, *instance = NULL;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	gboolean has_children;
	ThreePixbufs *pixbufs;
	ModestFolderView *folder_view = (ModestFolderView *) data;

	rendobj = (GObject *) renderer;

	gtk_tree_model_get (tree_model, iter,
			    TYPE_COLUMN, &type,
			    INSTANCE_COLUMN, &instance,
			    -1);

	if (!instance)
		return;

	has_children = gtk_tree_model_iter_has_child (tree_model, iter);
	pixbufs = get_folder_icons (folder_view, type, instance);
	g_object_unref (instance);

	/* Set pixbuf */
	g_object_set (rendobj, "pixbuf", pixbufs->pixbuf, NULL);

	if (has_children) {
		g_object_set (rendobj, "pixbuf-expander-open", pixbufs->pixbuf_open, NULL);
		g_object_set (rendobj, "pixbuf-expander-closed", pixbufs->pixbuf_close, NULL);
	}

	free_pixbufs (pixbufs);
}

static void
add_columns (GtkWidget *treeview)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;
	ModestFolderViewPrivate *priv;

	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(treeview);

	/* Create column */
	column = gtk_tree_view_column_new ();

	/* Set icon and text render function */
	renderer = gtk_cell_renderer_pixbuf_new();
#ifdef MODEST_TOOLKIT_HILDON2
	g_object_set (renderer,
		      "xpad", MODEST_MARGIN_DEFAULT,
		      "ypad", MODEST_MARGIN_DEFAULT,
		      NULL);
#endif
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						icon_cell_data, treeview, NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer, 
#ifdef MODEST_TOOLKIT_HILDON2
		      "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
		      "ypad", MODEST_MARGIN_DEFAULT,
		      "xpad", MODEST_MARGIN_DEFAULT,
#else
		      "ellipsize", PANGO_ELLIPSIZE_END,
#endif
		      "ellipsize-set", TRUE, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						text_cell_data, treeview, NULL);

	priv->messages_renderer = gtk_cell_renderer_text_new ();
	g_object_set (priv->messages_renderer, 
#ifdef MODEST_TOOLKIT_HILDON2
		      "yalign", 0.5,
		      "ypad", MODEST_MARGIN_DEFAULT,
		      "xpad", MODEST_MARGIN_DOUBLE,
#else
		      "scale", PANGO_SCALE_X_SMALL,
		      "scale-set", TRUE,
#endif
		      "alignment", PANGO_ALIGN_RIGHT,
		      "align-set", TRUE,
		      "xalign", 1.0,
		      NULL);
	gtk_tree_view_column_pack_start (column, priv->messages_renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, priv->messages_renderer,
						messages_cell_data, treeview, NULL);

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
	priv->do_refresh     = TRUE;
	priv->style          = MODEST_FOLDER_VIEW_STYLE_SHOW_ALL;
	priv->cur_folder_store   = NULL;
	priv->visible_account_id = NULL;
	priv->mailbox = NULL;
	priv->folder_to_select = NULL;
	priv->reexpand = TRUE;
	priv->signal_handlers = 0;

	/* Initialize the local account name */
	conf = modest_runtime_get_conf();
	priv->local_account_name = modest_conf_get_string (conf, MODEST_CONF_DEVICE_NAME, NULL);

	/* Init email clipboard */
	priv->clipboard = modest_runtime_get_email_clipboard ();
	priv->hidding_ids = NULL;
	priv->n_selected = 0;
	priv->filter = MODEST_FOLDER_VIEW_FILTER_NONE;
	priv->reselect = FALSE;
	priv->show_non_move = TRUE;
	priv->list_to_move = NULL;
	priv->show_message_count = TRUE;

	/* Build treeview */
	add_columns (GTK_WIDGET (obj));

	/* Setup drag and drop */
	setup_drag_and_drop (GTK_TREE_VIEW(obj));

	/* Connect signals */
	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT (obj), "key-press-event",
							   G_CALLBACK (on_key_pressed), NULL);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   (GObject*) modest_runtime_get_account_mgr (),
							   "display_name_changed",
							   G_CALLBACK (on_display_name_changed),
							   obj);

	/*
	 * Track changes in the local account name (in the device it
	 * will be the device name)
	 */
	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT(conf),
							   "key_changed",
							   G_CALLBACK(on_configuration_key_changed),
							   obj);

	gdk_color_parse ("000", &priv->active_color);

	update_style (obj);
	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT (obj), "notify::style",
							   G_CALLBACK (on_notify_style), (gpointer) obj);
}

static void
tny_account_store_view_init (gpointer g, gpointer iface_data)
{
	TnyAccountStoreViewIface *klass = (TnyAccountStoreViewIface *)g;

	klass->set_account_store = modest_folder_view_set_account_store;
}

static void
modest_folder_view_dispose (GObject *obj)
{
	static gboolean disposed = FALSE;
	ModestFolderViewPrivate *priv;

	if (disposed)
		return;

	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE (obj);

#ifdef MODEST_TOOLKIT_HILDON2
	modest_signal_mgr_disconnect_all_and_destroy (priv->signal_handlers);
#endif

	/* Free external references */
	if (priv->account_store) {
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}

	if (priv->query) {
		g_object_unref (G_OBJECT (priv->query));
		priv->query = NULL;
	}

	if (priv->folder_to_select) {
		g_object_unref (G_OBJECT(priv->folder_to_select));
	    	priv->folder_to_select = NULL;
	}

	if (priv->cur_folder_store) {
		g_object_unref (priv->cur_folder_store);
		priv->cur_folder_store = NULL;
	}

	if (priv->list_to_move) {
		g_object_unref (priv->list_to_move);
		priv->list_to_move = NULL;
	}
}

static void
modest_folder_view_finalize (GObject *obj)
{
	ModestFolderViewPrivate *priv;

	g_return_if_fail (obj);

	priv =	MODEST_FOLDER_VIEW_GET_PRIVATE(obj);

	if (priv->timer_expander != 0) {
		g_source_remove (priv->timer_expander);
		priv->timer_expander = 0;
	}

	g_free (priv->local_account_name);
	g_free (priv->visible_account_id);
	g_free (priv->mailbox);

	/* Clear hidding array created by cut operation */
	_clear_hidding_filter (MODEST_FOLDER_VIEW (obj));

	gdk_color_parse ("000", &priv->active_color);

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

		if (modest_signal_mgr_is_connected (priv->signal_handlers,
						    G_OBJECT (priv->account_store),
						    "account_inserted"))
			priv->signal_handlers = modest_signal_mgr_disconnect (priv->signal_handlers,
									      G_OBJECT (priv->account_store),
									      "account_inserted");
		if (modest_signal_mgr_is_connected (priv->signal_handlers,
						    G_OBJECT (priv->account_store),
						    "account_removed"))
			priv->signal_handlers = modest_signal_mgr_disconnect (priv->signal_handlers,
									      G_OBJECT (priv->account_store),
									      "account_removed");
		if (modest_signal_mgr_is_connected (priv->signal_handlers,
						    G_OBJECT (priv->account_store),
						    "account_changed"))
			priv->signal_handlers = modest_signal_mgr_disconnect (priv->signal_handlers,
									      G_OBJECT (priv->account_store),
									      "account_changed");
		g_object_unref (G_OBJECT (priv->account_store));
	}

	priv->account_store = g_object_ref (G_OBJECT (account_store));

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT(account_store), "account_removed",
							   G_CALLBACK (on_account_removed), self);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT(account_store), "account_inserted",
							   G_CALLBACK (on_account_inserted), self);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT(account_store), "account_changed",
							   G_CALLBACK (on_account_changed), self);

	modest_folder_view_update_model (MODEST_FOLDER_VIEW (self), account_store);
	priv->reselect = FALSE;
	modest_folder_view_select_first_inbox_or_local (MODEST_FOLDER_VIEW (self));

	g_object_unref (G_OBJECT (device));
}

static void
on_outbox_deleted_cb (ModestTnyLocalFoldersAccount *local_account,
		      gpointer user_data)
{
	ModestFolderView *self;
	GtkTreeModel *model, *filter_model;
	TnyFolder *outbox;

	self = MODEST_FOLDER_VIEW (user_data);

	if (!get_inner_models (self, &filter_model, NULL, &model))
		return;

	/* Remove outbox from model */
	outbox = modest_tny_local_folders_account_get_merged_outbox (local_account);
	tny_list_remove (TNY_LIST (model), G_OBJECT (outbox));
	g_object_unref (outbox);

	/* Refilter view */
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));
}

static void
on_account_inserted (TnyAccountStore *account_store,
		     TnyAccount *account,
		     gpointer user_data)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *model, *filter_model;

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


	/* Get models */
	if (!get_inner_models (MODEST_FOLDER_VIEW (user_data),
			       &filter_model, NULL, &model))
		return;

	/* Insert the account in the model */
	tny_list_append (TNY_LIST (model), G_OBJECT (account));

	/* When the model is a list store (plain representation) the
	   outbox is not a child of any account so we have to manually
	   delete it because removing the local folders account won't
	   delete it (because tny_folder_get_account() is not defined
	   for a merge folder */
	if (TNY_IS_GTK_FOLDER_LIST_STORE (model) &&
	    MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (account)) {
		priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
								   (GObject*) account, "outbox-deleted",
								   G_CALLBACK (on_outbox_deleted_cb),
								   user_data);
	}

	/* Refilter the model */
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));
}


static gboolean
same_account_selected (ModestFolderView *self,
		       TnyAccount *account)
{
	ModestFolderViewPrivate *priv;
	gboolean same_account = FALSE;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	if (priv->cur_folder_store) {
		TnyAccount *selected_folder_account = NULL;

		if (TNY_IS_FOLDER (priv->cur_folder_store)) {
			selected_folder_account =
				modest_tny_folder_get_account (TNY_FOLDER (priv->cur_folder_store));
		} else {
			selected_folder_account =
				TNY_ACCOUNT (g_object_ref (priv->cur_folder_store));
		}

		if (selected_folder_account == account)
			same_account = TRUE;

		g_object_unref (selected_folder_account);
	}
	return same_account;
}

/**
 *
 * Selects the first inbox or the local account in an idle
 */
static gboolean
on_idle_select_first_inbox_or_local (gpointer user_data)
{
	ModestFolderView *self = MODEST_FOLDER_VIEW (user_data);

	gdk_threads_enter ();
	modest_folder_view_select_first_inbox_or_local (self);
	gdk_threads_leave ();

	return FALSE;
}

static void
on_account_changed (TnyAccountStore *account_store,
		    TnyAccount *tny_account,
		    gpointer user_data)
{
	ModestFolderView *self;
	ModestFolderViewPrivate *priv;
	GtkTreeModel *model, *filter_model;
	GtkTreeSelection *sel;
	gboolean same_account;

	/* Ignore transport account insertions, we're not showing them
	   in the folder view */
	if (TNY_IS_TRANSPORT_ACCOUNT (tny_account))
		return;

	self = MODEST_FOLDER_VIEW (user_data);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (user_data);

	/* Get the inner model */
	if (!get_inner_models (MODEST_FOLDER_VIEW (user_data),
			       &filter_model, NULL, &model))
		return;

	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));

	/* Invalidate the cur_folder_store only if the selected folder
	   belongs to the account that is being removed */
	same_account = same_account_selected (self, tny_account);
	if (same_account) {
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
		gtk_tree_selection_unselect_all (sel);
	}

	/* Remove the account from the model */
	tny_list_remove (TNY_LIST (model), G_OBJECT (tny_account));

	/* Insert the account in the model */
	tny_list_append (TNY_LIST (model), G_OBJECT (tny_account));

	/* Refilter the model */
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));

	/* Select the first INBOX if the currently selected folder
	   belongs to the account that is being deleted */
	if (same_account && !MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (tny_account))
		g_idle_add (on_idle_select_first_inbox_or_local, self);
}

static void
on_account_removed (TnyAccountStore *account_store,
		    TnyAccount *account,
		    gpointer user_data)
{
	ModestFolderView *self = NULL;
	ModestFolderViewPrivate *priv;
	GtkTreeModel *model, *filter_model;
	GtkTreeSelection *sel = NULL;
	gboolean same_account = FALSE;

	/* Ignore transport account removals, we're not showing them
	   in the folder view */
	if (TNY_IS_TRANSPORT_ACCOUNT (account))
		return;

	if (!MODEST_IS_FOLDER_VIEW(user_data)) {
		g_warning ("BUG: %s: not a valid folder view", __FUNCTION__);
		return;
	}

	self = MODEST_FOLDER_VIEW (user_data);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	/* Invalidate the cur_folder_store only if the selected folder
	   belongs to the account that is being removed */
	same_account = same_account_selected (self, account);
	if (same_account) {
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
		gtk_tree_selection_unselect_all (sel);
	}

	/* Invalidate row to select only if the folder to select
	   belongs to the account that is being removed*/
	if (priv->folder_to_select) {
		TnyAccount *folder_to_select_account = NULL;

		folder_to_select_account = tny_folder_get_account (priv->folder_to_select);
		if (folder_to_select_account == account) {
			modest_folder_view_disable_next_folder_selection (self);
			g_object_unref (priv->folder_to_select);
			priv->folder_to_select = NULL;
		}
		g_object_unref (folder_to_select_account);
	}

	if (!get_inner_models (MODEST_FOLDER_VIEW (user_data),
			       &filter_model, NULL, &model))
		return;

	/* Disconnect the signal handler */
	if (TNY_IS_GTK_FOLDER_LIST_STORE (model) &&
	    MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (account)) {
		if (modest_signal_mgr_is_connected (priv->signal_handlers, (GObject*) account, "outbox-deleted"))
			priv->signal_handlers = modest_signal_mgr_disconnect (priv->signal_handlers,
									      (GObject *) account,
									      "outbox-deleted");
	}

	/* Remove the account from the model */
	tny_list_remove (TNY_LIST (model), G_OBJECT (account));

	/* If the removed account is the currently viewed one then
	   clear the configuration value. The new visible account will be the default account */
	if (priv->visible_account_id &&
	    !strcmp (priv->visible_account_id, tny_account_get_id (account))) {

		/* Clear the current visible account_id */
		modest_folder_view_set_account_id_of_visible_server_account (self, NULL);
		modest_folder_view_set_mailbox (self, NULL);

		/* Call the restore method, this will set the new visible account */
		modest_widget_memory_restore (modest_runtime_get_conf(), G_OBJECT(self),
					      MODEST_CONF_FOLDER_VIEW_KEY);
	}

	/* Refilter the model */
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));

	/* Select the first INBOX if the currently selected folder
	   belongs to the account that is being deleted */
	if (same_account)
		g_idle_add (on_idle_select_first_inbox_or_local, self);
}

void
modest_folder_view_set_title (ModestFolderView *self, const gchar *title)
{
	GtkTreeViewColumn *col;

	g_return_if_fail (self && MODEST_IS_FOLDER_VIEW(self));

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
	return modest_folder_view_new_full (query, TRUE);
}

GtkWidget*
modest_folder_view_new_full (TnyFolderStoreQuery *query, gboolean do_refresh)
{
	GObject *self;
	ModestFolderViewPrivate *priv;
	GtkTreeSelection *sel;

	self = G_OBJECT (g_object_new (MODEST_TYPE_FOLDER_VIEW, 
#ifdef MODEST_TOOLKIT_HILDON2
				       "hildon-ui-mode", HILDON_UI_MODE_NORMAL,
#endif
				       NULL));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	if (query)
		priv->query = g_object_ref (query);

	priv->do_refresh = do_refresh;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   (GObject*) sel, "changed",
							   G_CALLBACK (on_selection_changed), self);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   self, "row-activated",
							   G_CALLBACK (on_row_activated), self);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   self, "expose-event", 
							   G_CALLBACK (modest_folder_view_on_map), NULL);

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

static gboolean
is_parent_of (TnyFolder *a, TnyFolder *b)
{
	const gchar *a_id;
	gboolean retval = FALSE;

	a_id = tny_folder_get_id (a);
	if (a_id) {
		gchar *string_to_match;
		const gchar *b_id;

		string_to_match = g_strconcat (a_id, "/", NULL);
		b_id = tny_folder_get_id (b);
		retval = g_str_has_prefix (b_id, string_to_match);
		g_free (string_to_match);
	}
	
	return retval;
}

typedef struct _ForeachFolderInfo {
	gchar *needle;
	gboolean found;
} ForeachFolderInfo;

static gboolean 
foreach_folder_with_id (GtkTreeModel *model,
			GtkTreePath *path,
			GtkTreeIter *iter,
			gpointer data)
{
	ForeachFolderInfo *info;
	GObject *instance;

	info = (ForeachFolderInfo *) data;
	gtk_tree_model_get (model, iter,
			    INSTANCE_COLUMN, &instance,
			    -1);

	if (TNY_IS_FOLDER (instance)) {
		const gchar *id;
		gchar *collate;
		id = tny_folder_get_id (TNY_FOLDER (instance));
		if (id) {
			collate = g_utf8_collate_key (id, -1);
			info->found = !strcmp (info->needle, collate);
			g_free (collate);
		}
	}

	if (instance)
		g_object_unref (instance);

	return info->found;
	
}


static gboolean
has_folder_with_id (ModestFolderView *self, const gchar *id)
{
	GtkTreeModel *model;
	ForeachFolderInfo info = {NULL, FALSE};

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	info.needle = g_utf8_collate_key (id, -1);
	
	gtk_tree_model_foreach (model, foreach_folder_with_id, &info);
	g_free (info.needle);

	return info.found;
}

static gboolean
has_child_with_name_of (ModestFolderView *self, TnyFolder *a, TnyFolder *b)
{
	const gchar *a_id;
	gboolean retval = FALSE;

	a_id = tny_folder_get_id (a);
	if (a_id) {
		const gchar *b_id;
		b_id = tny_folder_get_id (b);
		
		if (b_id) {
			const gchar *last_bar;
			gchar *string_to_match;
			last_bar = g_strrstr (b_id, "/");
			if (last_bar)
				last_bar++;
			else
				last_bar = b_id;
			string_to_match = g_strconcat (a_id, "/", last_bar, NULL);
			retval = has_folder_with_id (self, string_to_match);
			g_free (string_to_match);
		}
	}

	return retval;
}

static gboolean
check_move_to_this_folder_valid (ModestFolderView *self, TnyFolder *folder)
{
	ModestFolderViewPrivate *priv;
	TnyIterator *iterator;
	gboolean retval = TRUE;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	for (iterator = tny_list_create_iterator (priv->list_to_move);
	     retval && !tny_iterator_is_done (iterator);
	     tny_iterator_next (iterator)) {
		GObject *instance;
		instance = tny_iterator_get_current (iterator);
		if (instance == (GObject *) folder) {
			retval = FALSE;
		} else if (TNY_IS_FOLDER (instance)) {
			retval = !is_parent_of (TNY_FOLDER (instance), folder);
			if (retval) {
				retval = !has_child_with_name_of (self, folder, TNY_FOLDER (instance));
			}
		}
		g_object_unref (instance);
	}
	g_object_unref (iterator);

	return retval;
}


/*
 * We use this function to implement the
 * MODEST_FOLDER_VIEW_STYLE_SHOW_ONE style. We only show the default
 * account in this case, and the local folders.
 */
static gboolean
filter_row (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	ModestFolderViewPrivate *priv;
	gboolean retval = TRUE;
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	GObject *instance = NULL;
	const gchar *id = NULL;
	guint i;
	gboolean found = FALSE;
	gboolean cleared = FALSE;
	ModestTnyFolderRules rules = 0;
	gchar *fname;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (data), FALSE);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (data);

	gtk_tree_model_get (model, iter,
			    NAME_COLUMN, &fname,
			    TYPE_COLUMN, &type,
			    INSTANCE_COLUMN, &instance,
			    -1);

	/* Do not show if there is no instance, this could indeed
	   happen when the model is being modified while it's being
	   drawn. This could occur for example when moving folders
	   using drag&drop */
	if (!instance) {
		g_free (fname);
		return FALSE;
	}

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
	} else {
		if (priv->style == MODEST_FOLDER_VIEW_STYLE_SHOW_ONE) {
			/* Only show special folders for current account if needed */
			if (TNY_IS_FOLDER (instance) && !TNY_IS_MERGE_FOLDER (instance)) {
				TnyAccount *account;

				account = tny_folder_get_account (TNY_FOLDER (instance));

				if (TNY_IS_ACCOUNT (account)) {
					const gchar *account_id = tny_account_get_id (account);

					if (!modest_tny_account_is_virtual_local_folders (account) &&
					    strcmp (account_id, MODEST_MMC_ACCOUNT_ID)) {
						/* Show only the visible account id */
						if (priv->visible_account_id) {
						  if (strcmp (account_id, priv->visible_account_id)) {
							  retval = FALSE;
						  } else if (priv->mailbox) {
							  /* Filter mailboxes */
							  if (!g_str_has_prefix (fname, priv->mailbox)) {
								  retval = FALSE;
							  } else if (!strcmp (fname, priv->mailbox)) {
								  /* Hide mailbox parent */
								  retval = FALSE;
							  }
						  }
						}
					}
						g_object_unref (account);
				}
			}

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
	if (retval && !priv->show_non_move) {
		if (priv->list_to_move && 
		    tny_list_get_length (priv->list_to_move) > 0 &&
		    TNY_IS_FOLDER (instance)) {
			retval = check_move_to_this_folder_valid (MODEST_FOLDER_VIEW (data), TNY_FOLDER (instance));
		}
		if (retval && TNY_IS_FOLDER (instance) && 
		    modest_tny_folder_is_local_folder (TNY_FOLDER (instance))) {
			switch (type) {
			case TNY_FOLDER_TYPE_OUTBOX:
			case TNY_FOLDER_TYPE_SENT:
			case TNY_FOLDER_TYPE_DRAFTS:
				retval = FALSE;
				break;
			case TNY_FOLDER_TYPE_UNKNOWN:
			case TNY_FOLDER_TYPE_NORMAL:
				type = modest_tny_folder_guess_folder_type(TNY_FOLDER(instance));
				if (type == TNY_FOLDER_TYPE_INVALID)
					g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
				
				if (type == TNY_FOLDER_TYPE_OUTBOX ||
				    type == TNY_FOLDER_TYPE_SENT
				    || type == TNY_FOLDER_TYPE_DRAFTS)
					retval = FALSE;
				break;
			default:
				break;
			}
		}
		if (retval && TNY_IS_ACCOUNT (instance) &&
		    modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (instance))) {
			ModestProtocolType protocol_type;

			protocol_type = modest_tny_account_get_protocol_type (TNY_ACCOUNT (instance));
			retval  = !modest_protocol_registry_protocol_type_has_tag 
				(modest_runtime_get_protocol_registry (),
				 protocol_type,
				 MODEST_PROTOCOL_REGISTRY_STORE_FORBID_MESSAGE_ADD);
		}
	}

	/* apply special filters */
	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_HIDE_ACCOUNTS)) {
		if (TNY_IS_ACCOUNT (instance))
			return FALSE;
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_HIDE_FOLDERS)) {
		if (TNY_IS_FOLDER (instance))
			return FALSE;
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_HIDE_LOCAL_FOLDERS)) {
		if (TNY_IS_ACCOUNT (instance)) {
			if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (instance)))
				return FALSE;
		} else if (TNY_IS_FOLDER (instance)) {
			if (modest_tny_folder_is_local_folder (TNY_FOLDER (instance)))
				return FALSE;
		}
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_HIDE_MCC_FOLDERS)) {
		if (TNY_IS_ACCOUNT (instance)) {
			if (modest_tny_account_is_memory_card_account (TNY_ACCOUNT (instance)))
				return FALSE;
		} else if (TNY_IS_FOLDER (instance)) {
			if (modest_tny_folder_is_memory_card_folder (TNY_FOLDER (instance)))
				return FALSE;
		}
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_SHOW_ONLY_MAILBOXES)) {
		/* A mailbox is a fake folder with an @ in the middle of the name */
		if (!TNY_IS_FOLDER (instance) ||
		    !(tny_folder_get_caps (TNY_FOLDER (instance)) & TNY_FOLDER_CAPS_NOSELECT)) {
			return FALSE;
		} else {
			const gchar *folder_name;
			folder_name = tny_folder_get_name (TNY_FOLDER (instance));
			if (!folder_name || strchr (folder_name, '@') == NULL)
				return FALSE;
		}
		
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_CAN_HAVE_FOLDERS)) {
		if (TNY_IS_FOLDER (instance)) {
			/* Check folder rules */
			ModestTnyFolderRules rules;

			rules = modest_tny_folder_get_rules (TNY_FOLDER (instance));
			retval = !(rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE);
		} else if (TNY_IS_ACCOUNT (instance)) {
			if (modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (instance))) {
				retval = FALSE;
			} else {
				retval = TRUE;
			}
		}
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_HIDE_MANDATORY_FOLDERS)) {
		if (TNY_IS_FOLDER (instance)) {
			TnyFolderType guess_type;

			if (TNY_FOLDER_TYPE_NORMAL) {
				guess_type = modest_tny_folder_guess_folder_type (TNY_FOLDER (instance));
			} else {
				guess_type = type;
			}

			switch (type) {
			case TNY_FOLDER_TYPE_OUTBOX:
			case TNY_FOLDER_TYPE_SENT:
			case TNY_FOLDER_TYPE_DRAFTS:
			case TNY_FOLDER_TYPE_ARCHIVE:
			case TNY_FOLDER_TYPE_INBOX:
				retval = FALSE;
				break;
			case TNY_FOLDER_TYPE_UNKNOWN:
			case TNY_FOLDER_TYPE_NORMAL:
				break;
			default:
				break;
			}

		} else if (TNY_IS_ACCOUNT (instance)) {
			retval = FALSE;
		}
	}

	if (retval && TNY_IS_FOLDER (instance)) {
		rules = modest_tny_folder_get_rules (TNY_FOLDER (instance));
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_DELETABLE)) {
		if (TNY_IS_FOLDER (instance)) {
			retval = !(rules & MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE);
		} else if (TNY_IS_ACCOUNT (instance)) {
			retval = FALSE;
		}
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_RENAMEABLE)) {
		if (TNY_IS_FOLDER (instance)) {
			retval = !(rules & MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE);
		} else if (TNY_IS_ACCOUNT (instance)) {
			retval = FALSE;
		}
	}

	if (retval && (priv->filter & MODEST_FOLDER_VIEW_FILTER_MOVEABLE)) {
		if (TNY_IS_FOLDER (instance)) {
			retval = !(rules & MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE);
		} else if (TNY_IS_ACCOUNT (instance)) {
			retval = FALSE;
		}
	}

	/* Free */
	g_object_unref (instance);
        g_free (fname);

	return retval;
}


gboolean
modest_folder_view_update_model (ModestFolderView *self,
				 TnyAccountStore *account_store)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *model;
	GtkTreeModel *filter_model = NULL, *sortable = NULL, *old_tny_model;

	g_return_val_if_fail (self && MODEST_IS_FOLDER_VIEW (self), FALSE);
	g_return_val_if_fail (account_store && MODEST_IS_TNY_ACCOUNT_STORE(account_store),
			      FALSE);

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
	   selects only the subscribed folders */
#ifdef MODEST_TOOLKIT_HILDON2
	TnyGtkFolderListStoreFlags flags;
	flags = TNY_GTK_FOLDER_LIST_STORE_FLAG_SHOW_PATH;
	if (priv->do_refresh)
		flags |= TNY_GTK_FOLDER_LIST_STORE_FLAG_DELAYED_REFRESH;
	else
		flags |= TNY_GTK_FOLDER_LIST_STORE_FLAG_NO_REFRESH;
	model = tny_gtk_folder_list_store_new_with_flags (NULL, 
							  flags);
	tny_gtk_folder_list_store_set_path_separator (TNY_GTK_FOLDER_LIST_STORE (model),
						      MODEST_FOLDER_PATH_SEPARATOR);
#else
	model = tny_gtk_folder_store_tree_model_new (NULL);
#endif

	/* When the model is a list store (plain representation) the
	   outbox is not a child of any account so we have to manually
	   delete it because removing the local folders account won't
	   delete it (because tny_folder_get_account() is not defined
	   for a merge folder */
	if (TNY_IS_GTK_FOLDER_LIST_STORE (model)) {
		TnyAccount *account;
		ModestTnyAccountStore *acc_store;

		acc_store = modest_runtime_get_account_store ();
		account = modest_tny_account_store_get_local_folders_account (acc_store);

		if (modest_signal_mgr_is_connected (priv->signal_handlers, (GObject *) account,
						    "outbox-deleted"))
			priv->signal_handlers = modest_signal_mgr_disconnect (priv->signal_handlers,
									      (GObject *) account,
									      "outbox-deleted");

		priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
								   (GObject*) account, "outbox-deleted",
								   G_CALLBACK (on_outbox_deleted_cb),
								   self);
		g_object_unref (account);
	}

	/* Get the accounts: */
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(account_store),
					TNY_LIST (model),
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	sortable = gtk_tree_model_sort_new_with_model (model);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(sortable),
					      NAME_COLUMN,
					      GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
					 NAME_COLUMN,
					 cmp_rows, NULL, NULL);

	/* Create filter model */
	filter_model = gtk_tree_model_filter_new (sortable, NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
						filter_row,
						self,
						NULL);

	if (get_inner_models (self, NULL, NULL, &old_tny_model)) {
		if (modest_signal_mgr_is_connected (priv->signal_handlers, (GObject *) old_tny_model,
						    "activity-changed"))
			priv->signal_handlers = modest_signal_mgr_disconnect (priv->signal_handlers,
									      G_OBJECT (old_tny_model),
									      "activity-changed");
	}

	/* Set new model */
	gtk_tree_view_set_model (GTK_TREE_VIEW(self), filter_model);
#ifndef MODEST_TOOLKIT_HILDON2
	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT(filter_model), "row-inserted",
							   (GCallback) on_row_inserted_maybe_select_folder, self);
#endif

#ifdef MODEST_TOOLKIT_HILDON2
	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT (model),
							   "activity-changed",
							   G_CALLBACK (on_activity_changed), 
							   self);
#endif

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

	tree_view = MODEST_FOLDER_VIEW (user_data);

	if (selected) {
		gtk_tree_model_get (model, &iter,
				    INSTANCE_COLUMN, &folder,
				    -1);

		/* If the folder is the same do not notify */
		if (folder && priv->cur_folder_store == folder) {
			g_object_unref (folder);
			return;
		}
	}

	/* Current folder was unselected */
	if (priv->cur_folder_store) {
		/* We must do this firstly because a libtinymail-camel
		   implementation detail. If we issue the signal
		   before doing the sync_async, then that signal could
		   cause (and it actually does it) a free of the
		   summary of the folder (because the main window will
		   clear the headers view */
#ifndef MODEST_TOOLKIT_HILDON2
		if (TNY_IS_FOLDER(priv->cur_folder_store))
			tny_folder_sync_async (TNY_FOLDER(priv->cur_folder_store),
					       FALSE, NULL, NULL, NULL);
#endif

		g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTION_CHANGED_SIGNAL], 0,
		       priv->cur_folder_store, FALSE);

		g_object_unref (priv->cur_folder_store);
		priv->cur_folder_store = NULL;
	}

	/* New current references */
	priv->cur_folder_store = folder;

	/* New folder has been selected. Do not notify if there is
	   nothing new selected */
	if (selected) {
		g_signal_emit (G_OBJECT(tree_view),
			       signals[FOLDER_SELECTION_CHANGED_SIGNAL],
			       0, priv->cur_folder_store, TRUE);
	}
}

static void
on_row_activated (GtkTreeView *treeview,
		  GtkTreePath *treepath,
		  GtkTreeViewColumn *column,
		  gpointer user_data)
{
	GtkTreeModel *model = NULL;
	TnyFolderStore *folder = NULL;
	GtkTreeIter iter;
	ModestFolderView *self = NULL;
	ModestFolderViewPrivate *priv = NULL;

	g_return_if_fail (treeview);
	g_return_if_fail (user_data);

	self = MODEST_FOLDER_VIEW (user_data);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(user_data);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));

	if (!gtk_tree_model_get_iter (model, &iter, treepath))
		return;

	gtk_tree_model_get (model, &iter,
			    INSTANCE_COLUMN, &folder,
			    -1);

	g_signal_emit (G_OBJECT(self),
		       signals[FOLDER_ACTIVATED_SIGNAL],
		       0, folder);

#ifdef MODEST_TOOLKIT_HILDON2
	HildonUIMode ui_mode;
	g_object_get (G_OBJECT (self), "hildon-ui-mode", &ui_mode, NULL);
	if (ui_mode == HILDON_UI_MODE_NORMAL) {
		if (priv->cur_folder_store)
			g_object_unref (priv->cur_folder_store);
		priv->cur_folder_store = g_object_ref (folder);
	}
#endif

	g_object_unref (folder);
}

TnyFolderStore *
modest_folder_view_get_selected (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_FOLDER_VIEW(self), NULL);

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

static gboolean
inbox_is_special (TnyFolderStore *folder_store)
{
	gboolean is_special = TRUE;

	if (TNY_IS_FOLDER (folder_store)) {
		const gchar *id;
		gchar *downcase;
		gchar *last_bar;
		gchar *last_inbox_bar;

		id = tny_folder_get_id (TNY_FOLDER (folder_store));
		downcase = g_utf8_strdown (id, -1);
		last_bar = g_strrstr (downcase, "/");
		if (last_bar) {
			last_inbox_bar = g_strrstr  (downcase, "inbox/");
			if ((last_inbox_bar == NULL) || (last_inbox_bar + 5 != last_bar))
				is_special = FALSE;
		} else {
			is_special = FALSE;
		}
		g_free (downcase);
	}
	return is_special;
}

static gint
get_cmp_pos (TnyFolderType t, TnyFolder *folder_store)
{
	TnyAccount *account;
	gboolean is_special;
	/* Inbox, Outbox, Drafts, Sent, User */
 	/* 0, 1, 2, 3, 4 */

	if (!TNY_IS_FOLDER (folder_store))
		return 4;
	switch (t) {
	case TNY_FOLDER_TYPE_INBOX:
	{
		account = tny_folder_get_account (folder_store);
		is_special = (get_cmp_rows_type_pos (G_OBJECT (account)) == 0);

		/* In inbox case we need to know if the inbox is really the top
		 * inbox of the account, or if it's a submailbox inbox. To do
		 * this we'll apply an heuristic rule: Find last "/" and check
		 * if it's preceeded by another Inbox */
		is_special = is_special && !inbox_is_special (TNY_FOLDER_STORE (folder_store));
		g_object_unref (account);
		return is_special?0:4;
	}
	break;
	case TNY_FOLDER_TYPE_OUTBOX:
		return (TNY_IS_MERGE_FOLDER (folder_store))?2:4;
		break;
	case TNY_FOLDER_TYPE_DRAFTS:
	{
		account = tny_folder_get_account (folder_store);
		is_special = (get_cmp_rows_type_pos (G_OBJECT (account)) == 1);
		g_object_unref (account);
		return is_special?1:4;
	}
	break;
	case TNY_FOLDER_TYPE_SENT:
	{
		account = tny_folder_get_account (folder_store);
		is_special = (get_cmp_rows_type_pos (G_OBJECT (account)) == 1);
		g_object_unref (account);
		return is_special?3:4;
	}
	break;
	default:
		return 4;
	}
}

static gint
compare_account_names (TnyAccount *a1, TnyAccount *a2)
{
	const gchar *a1_name, *a2_name;

	a1_name = tny_account_get_name (a1);
	a2_name = tny_account_get_name (a2);

	return modest_text_utils_utf8_strcmp (a1_name, a2_name, TRUE);
}

static gint
compare_accounts (TnyFolderStore *s1, TnyFolderStore *s2)
{
	TnyAccount *a1 = NULL, *a2 = NULL;
	gint cmp;

	if (TNY_IS_ACCOUNT (s1)) {
		a1 = TNY_ACCOUNT (g_object_ref (s1));
	} else if (!TNY_IS_MERGE_FOLDER (s1)) {
		a1 = tny_folder_get_account (TNY_FOLDER (s1));
	}

	if (TNY_IS_ACCOUNT (s2)) {
		a2 = TNY_ACCOUNT (g_object_ref (s2));
	} else  if (!TNY_IS_MERGE_FOLDER (s2)) {
		a2 = tny_folder_get_account (TNY_FOLDER (s2));
	}

	if (!a1 || !a2) {
		if (!a1 && !a2)
			cmp = 0;
		else if (!a1)
			cmp = 1;
		else
			cmp = -1;
		goto finish;
	}

	if (a1 == a2) {
		cmp = 0;
		goto finish;
	}
	/* First we sort with the type of account */
	cmp = get_cmp_rows_type_pos (G_OBJECT (a1)) - get_cmp_rows_type_pos (G_OBJECT (a2));
	if (cmp != 0)
		goto finish;

	cmp = compare_account_names (a1, a2);

finish:
	if (a1)
		g_object_unref (a1);
	if (a2)
		g_object_unref (a2);

	return cmp;
}

static gint
compare_accounts_first (TnyFolderStore *s1, TnyFolderStore *s2)
{
	gint is_account1, is_account2;

	is_account1 = TNY_IS_ACCOUNT (s1)?1:0;
	is_account2 = TNY_IS_ACCOUNT (s2)?1:0;

	return is_account2 - is_account1;
}

static gint
compare_folders (const gchar *name1, const gchar *name2)
{
	const gchar *separator1, *separator2;
	const gchar *next1, *next2;
	gchar *top1, *top2;
	gint cmp;

	if (name1 == NULL || name1[0] == '\0')
		return -1;
	if (name2 == NULL || name2[0] == '\0')
		return 1;

	separator1 = strstr (name1, MODEST_FOLDER_PATH_SEPARATOR);
	if (separator1) {
		top1 = g_strndup (name1, separator1 - name1);
	} else {
		top1 = g_strdup (name1);
	}

	separator2 = strstr (name2, MODEST_FOLDER_PATH_SEPARATOR);
	if (separator2) {
		top2 = g_strndup (name2, separator2 - name2);
	} else {
		top2 = g_strdup (name2);
	}


	cmp = modest_text_utils_utf8_strcmp (top1, top2, TRUE);
	g_free (top1);
	g_free (top2);

	if (cmp != 0)
		return cmp;

	if (separator1 == NULL && separator2 == NULL)
		return 0;

	next1 = (separator1 != NULL)?separator1 + strlen (MODEST_FOLDER_PATH_SEPARATOR):NULL;
	next2 = (separator2 != NULL)?separator2 + strlen (MODEST_FOLDER_PATH_SEPARATOR):NULL;

	return compare_folders (next1, next2);
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
			    NAME_COLUMN, &name1,
			    TYPE_COLUMN, &type,
			    INSTANCE_COLUMN, &folder1,
			    -1);
	gtk_tree_model_get (tree_model, iter2,
			    NAME_COLUMN, &name2,
			    TYPE_COLUMN, &type2,
			    INSTANCE_COLUMN, &folder2,
			    -1);

	/* Return if we get no folder. This could happen when folder
	   operations are happening. The model is updated after the
	   folder copy/move actually occurs, so there could be
	   situations where the model to be drawn is not correct */
	if (!folder1 || !folder2)
		goto finish;

	/* Sort by type. First the special folders, then the archives */
	cmp = get_cmp_pos (type, (TnyFolder *) folder1) - get_cmp_pos (type2, (TnyFolder *) folder2);
	if (cmp != 0)
		goto finish;

	/* Now we sort using the account of each folder */
	if (TNY_IS_FOLDER_STORE (folder1) && 
	    TNY_IS_FOLDER_STORE (folder2)) {
		cmp = compare_accounts (TNY_FOLDER_STORE (folder1), TNY_FOLDER_STORE (folder2));
		if (cmp != 0)
			goto finish;

		/* Each group is preceeded by its account */
		cmp = compare_accounts_first (TNY_FOLDER_STORE (folder1), TNY_FOLDER_STORE (folder2));
		if (cmp != 0)
			goto finish;
	}

	/* Pure sort by name */
	cmp = compare_folders (name1, name2);
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
on_drag_data_get (GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data,
		  guint info, guint time, gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *source_row;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

		source_row = gtk_tree_model_get_path (model, &iter);
		gtk_tree_set_row_drag_data (selection_data,
					    model,
					    source_row);

		gtk_tree_path_free (source_row);
	}
}

typedef struct _DndHelper {
	ModestFolderView *folder_view;
	gboolean delete_source;
	GtkTreePath *source_row;
} DndHelper;

static void
dnd_helper_destroyer (DndHelper *helper)
{
	/* Free the helper */
	gtk_tree_path_free (helper->source_row);
	g_slice_free (DndHelper, helper);
}

static void
xfer_folder_cb (ModestMailOperation *mail_op,
		TnyFolder *new_folder,
		gpointer user_data)
{
	if (new_folder) {
		/* Select the folder */
		modest_folder_view_select_folder (MODEST_FOLDER_VIEW (user_data),
						  new_folder, FALSE);
	}
}


/* get the folder for the row the treepath refers to. */
/* folder must be unref'd */
static TnyFolderStore *
tree_path_to_folder (GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	TnyFolderStore *folder = NULL;

	if (gtk_tree_model_get_iter (model,&iter, path))
		gtk_tree_model_get (model, &iter,
				    INSTANCE_COLUMN, &folder,
				    -1);
	return folder;
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
				GtkSelectionData *selection_data)
{
	TnyList *headers = NULL;
	TnyFolder *folder = NULL, *src_folder = NULL;
	TnyFolderType folder_type;
	GtkTreeIter source_iter, dest_iter;
	ModestWindowMgr *mgr = NULL;
	ModestWindow *main_win = NULL;
	gchar **uris, **tmp;

	/* Build the list of headers */
	mgr = modest_runtime_get_window_mgr ();
	headers = tny_simple_list_new ();
	uris = modest_dnd_selection_data_get_paths (selection_data);
	tmp = uris;

	while (*tmp != NULL) {
		TnyHeader *header;
		GtkTreePath *path;
		gboolean first = TRUE;

		/* Get header */
		path = gtk_tree_path_new_from_string (*tmp);
		gtk_tree_model_get_iter (source_model, &source_iter, path);
		gtk_tree_model_get (source_model, &source_iter,
				    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				    &header, -1);

		/* Do not enable d&d of headers already opened */
		if (!modest_window_mgr_find_registered_header(mgr, header, NULL))
			tny_list_append (headers, G_OBJECT (header));

		if (G_UNLIKELY (first)) {
			src_folder = tny_header_get_folder (header);
			first = FALSE;
		}

		/* Free and go on */
		gtk_tree_path_free (path);
		g_object_unref (header);
		tmp++;
	}
	g_strfreev (uris);

	/* This could happen ig we perform a d&d very quickly over the
	   same row that row could dissapear because message is
	   transferred */
	if (!TNY_IS_FOLDER (src_folder))
		goto cleanup;

	/* Get the target folder */
	gtk_tree_model_get_iter (dest_model, &dest_iter, dest_row);
	gtk_tree_model_get (dest_model, &dest_iter,
			    INSTANCE_COLUMN,
			    &folder, -1);

	if (!folder || !TNY_IS_FOLDER(folder)) {
/* 		g_warning ("%s: not a valid target folder (%p)", __FUNCTION__, folder); */
		goto cleanup;
	}

	folder_type = modest_tny_folder_guess_folder_type (folder);
	if (folder_type == TNY_FOLDER_TYPE_INVALID) {
/* 		g_warning ("%s: invalid target folder", __FUNCTION__); */
		goto cleanup;  /* cannot move messages there */
	}

	if (modest_tny_folder_get_rules((TNY_FOLDER(folder))) & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE) {
/* 		g_warning ("folder not writable"); */
		goto cleanup; /* verboten! */
	}

	/* Ask for confirmation to move */
	main_win = modest_window_mgr_get_main_window (mgr, FALSE); /* don't create */
	if (!main_win) {
		g_warning ("%s: BUG: no main window found", __FUNCTION__);
		goto cleanup;
	}

	/* Transfer messages */
	modest_ui_actions_transfer_messages_helper (GTK_WINDOW (main_win), src_folder,
						    headers, folder);

	/* Frees */
cleanup:
	if (G_IS_OBJECT (src_folder))
		g_object_unref (src_folder);
	if (G_IS_OBJECT(folder))
		g_object_unref (G_OBJECT (folder));
	if (G_IS_OBJECT(headers))
		g_object_unref (headers);
}

typedef struct {
	TnyFolderStore *src_folder;
	TnyFolderStore *dst_folder;
	ModestFolderView *folder_view;
	DndHelper *helper;
} DndFolderInfo;

static void
dnd_folder_info_destroyer (DndFolderInfo *info)
{
	if (info->src_folder)
		g_object_unref (info->src_folder);
	if (info->dst_folder)
		g_object_unref (info->dst_folder);
	g_slice_free (DndFolderInfo, info);
}

static void
dnd_on_connection_failed_destroyer (DndFolderInfo *info,
				    GtkWindow *parent_window,
				    TnyAccount *account)
{
	/* Show error */
	modest_ui_actions_on_account_connection_error (parent_window, account);

	/* Free the helper & info */
	dnd_helper_destroyer (info->helper);
	dnd_folder_info_destroyer (info);
}

static void
drag_and_drop_from_folder_view_src_folder_performer (gboolean canceled,
						     GError *err,
						     GtkWindow *parent_window,
						     TnyAccount *account,
						     gpointer user_data)
{
	DndFolderInfo *info = NULL;
	ModestMailOperation *mail_op;

	info = (DndFolderInfo *) user_data;

	if (err || canceled) {
		dnd_on_connection_failed_destroyer (info, parent_window, account);
		return;
	}

	/* Do the mail operation */
	mail_op = modest_mail_operation_new_with_error_handling ((GObject *) parent_window,
								 modest_ui_actions_move_folder_error_handler,
								 info->src_folder, NULL);

	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);

	/* Transfer the folder */
	modest_mail_operation_xfer_folder (mail_op,
					   TNY_FOLDER (info->src_folder),
					   info->dst_folder,
					   info->helper->delete_source,
					   xfer_folder_cb,
					   info->helper->folder_view);

	/* Frees */
	g_object_unref (G_OBJECT (mail_op));
	dnd_helper_destroyer (info->helper);
	dnd_folder_info_destroyer (info);
}


static void
drag_and_drop_from_folder_view_dst_folder_performer (gboolean canceled,
						     GError *err,
						     GtkWindow *parent_window,
						     TnyAccount *account,
						     gpointer user_data)
{
	DndFolderInfo *info = NULL;

	info = (DndFolderInfo *) user_data;

	if (err || canceled) {
		dnd_on_connection_failed_destroyer (info, parent_window, account);
		return;
	}

	/* Connect to source folder and perform the copy/move */
	modest_platform_connect_if_remote_and_perform (NULL, TRUE,
						       info->src_folder,
						       drag_and_drop_from_folder_view_src_folder_performer,
						       info);
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
	GtkTreeIter dest_iter, iter;
	TnyFolderStore *dest_folder = NULL;
	TnyFolderStore *folder = NULL;
	gboolean forbidden = FALSE;
	ModestWindow *win;
	DndFolderInfo *info = NULL;

	win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(), FALSE); /* don't create */
	if (!win) {
		g_warning ("%s: BUG: no main window", __FUNCTION__);
		dnd_helper_destroyer (helper);
		return;
	}

	if (!forbidden) {
		/* check the folder rules for the destination */
		folder = tree_path_to_folder (dest_model, dest_row);
		if (TNY_IS_FOLDER(folder)) {
			ModestTnyFolderRules rules =
				modest_tny_folder_get_rules (TNY_FOLDER (folder));
			forbidden = rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;
		} else if (TNY_IS_FOLDER_STORE(folder)) {
			/* enable local root as destination for folders */
			if (!MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (folder) &&
			    !modest_tny_account_is_memory_card_account (TNY_ACCOUNT (folder)))
				forbidden = TRUE;
		}
		g_object_unref (folder);
	}
	if (!forbidden) {
		/* check the folder rules for the source */
		folder = tree_path_to_folder (source_model, helper->source_row);
		if (TNY_IS_FOLDER(folder)) {
			ModestTnyFolderRules rules =
				modest_tny_folder_get_rules (TNY_FOLDER (folder));
			forbidden = rules & MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE;
		} else
			forbidden = TRUE;
		g_object_unref (folder);
	}


	/* Check if the drag is possible */
	if (forbidden || !gtk_tree_path_compare (helper->source_row, dest_row)) {
		/* Show error */
		modest_platform_run_information_dialog ((GtkWindow *) win, 
							_("mail_in_ui_folder_move_target_error"), 
							FALSE);
		/* Restore the previous selection */
		folder = tree_path_to_folder (source_model, helper->source_row);
		if (folder) {
			if (TNY_IS_FOLDER (folder))
				modest_folder_view_select_folder (helper->folder_view, 
								  TNY_FOLDER (folder), FALSE);
			g_object_unref (folder);
		}
		dnd_helper_destroyer (helper);
		return;
	}

	/* Get data */
	gtk_tree_model_get_iter (dest_model, &dest_iter, dest_row);
	gtk_tree_model_get (dest_model, &dest_iter,
			    INSTANCE_COLUMN,
			    &dest_folder, -1);
	gtk_tree_model_get_iter (source_model, &iter, helper->source_row);
	gtk_tree_model_get (source_model, &iter,
			    INSTANCE_COLUMN,
			    &folder, -1);

	/* Create the info for the performer */
	info = g_slice_new0 (DndFolderInfo);
	info->src_folder = g_object_ref (folder);
	info->dst_folder = g_object_ref (dest_folder);
	info->helper = helper;

	/* Connect to the destination folder and perform the copy/move */
	modest_platform_connect_if_remote_and_perform (GTK_WINDOW (win), TRUE,
						       dest_folder,
						       drag_and_drop_from_folder_view_dst_folder_performer,
						       info);

	/* Frees */
	g_object_unref (dest_folder);
	g_object_unref (folder);
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
	gboolean delete_source = FALSE;
	gboolean success = FALSE;

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
			GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
			gtk_tree_selection_unselect_all (sel);
		}
	}

	/* Check if the get_data failed */
	if (selection_data == NULL || selection_data->length < 0)
		goto end;

	/* Select the destination model */
	dest_model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));

	/* Get the path to the destination row. Can not call
	   gtk_tree_view_get_drag_dest_row() because the source row
	   is not selected anymore */
	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (widget), x, y,
					   &dest_row, &pos);

	/* Only allow drops IN other rows */
	if (!dest_row ||
	    pos == GTK_TREE_VIEW_DROP_BEFORE ||
	    pos == GTK_TREE_VIEW_DROP_AFTER)
		goto end;

	success = TRUE;
	/* Drags from the header view */
	if (source_widget != widget) {
		source_model = gtk_tree_view_get_model (GTK_TREE_VIEW (source_widget));

		drag_and_drop_from_header_view (source_model,
						dest_model,
						dest_row,
						selection_data);
	} else {
		DndHelper *helper = NULL;

		/* Get the source model and row */
		gtk_tree_get_row_drag_data (selection_data,
					    &source_model,
					    &source_row);

		/* Create the helper */
		helper = g_slice_new0 (DndHelper);
		helper->delete_source = delete_source;
		helper->source_row = gtk_tree_path_copy (source_row);
		helper->folder_view = MODEST_FOLDER_VIEW (widget);

		drag_and_drop_from_folder_view (source_model,
						dest_model,
						dest_row,
						selection_data,
						helper);

		gtk_tree_path_free (source_row);
	}

	/* Frees */
	gtk_tree_path_free (dest_row);

 end:
	/* Finish the drag and drop */
	gtk_drag_finish (context, success, FALSE, time);
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

	gdk_threads_enter ();

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

	gdk_threads_leave ();

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
	GtkTreeModel *dest_model;
	ModestFolderViewPrivate *priv;
	GdkDragAction suggested_action;
	gboolean valid_location = FALSE;
	TnyFolderStore *folder = NULL;

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

	/* Check that the destination folder is writable */
	dest_model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	folder = tree_path_to_folder (dest_model, dest_row);
	if (folder && TNY_IS_FOLDER (folder)) {
		ModestTnyFolderRules rules = modest_tny_folder_get_rules(TNY_FOLDER (folder));

		if (rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE) {
			valid_location = FALSE;
			goto out;
		}
	}

	/* Expand the selected row after 1/2 second */
	if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), dest_row)) {
		priv->timer_expander = g_timeout_add (500, expand_row_timeout, widget);
	}
	gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (widget), dest_row, pos);

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
	if (folder)
		g_object_unref (folder);
	if (dest_row) {
		gtk_tree_path_free (dest_row);
	}
	g_signal_stop_emission_by_name (widget, "drag-motion");

	return valid_location;
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
#ifdef MODEST_TOOLKIT_HILDON2
	return;
#endif
	ModestFolderViewPrivate *priv;

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	gtk_drag_dest_set (GTK_WIDGET (self),
			   GTK_DEST_DEFAULT_HIGHLIGHT,
			   folder_view_drag_types,
			   G_N_ELEMENTS (folder_view_drag_types),
			   GDK_ACTION_MOVE | GDK_ACTION_COPY);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT (self), "drag_data_received",
							   G_CALLBACK (on_drag_data_received), NULL);


	/* Set up the treeview as a dnd source */
	gtk_drag_source_set (GTK_WIDGET (self),
			     GDK_BUTTON1_MASK,
			     folder_view_drag_types,
			     G_N_ELEMENTS (folder_view_drag_types),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT (self), "drag_motion",
							   G_CALLBACK (on_drag_motion), NULL);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT (self), "drag_data_get",
							   G_CALLBACK (on_drag_data_get), NULL);

	priv->signal_handlers = modest_signal_mgr_connect (priv->signal_handlers,
							   G_OBJECT (self), "drag_drop",
							   G_CALLBACK (drag_drop_cb), NULL);
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

	if (!strcmp (key, MODEST_CONF_DEVICE_NAME)) {
		g_free (priv->local_account_name);

		if (event == MODEST_CONF_EVENT_KEY_UNSET)
			priv->local_account_name = g_strdup (MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME);
		else
			priv->local_account_name = modest_conf_get_string (modest_runtime_get_conf(),
									   MODEST_CONF_DEVICE_NAME, NULL);

		/* Force a redraw */
#if GTK_CHECK_VERSION(2, 8, 0)
		GtkTreeViewColumn * tree_column;

		tree_column = gtk_tree_view_get_column (GTK_TREE_VIEW (self),
							NAME_COLUMN);
		gtk_tree_view_column_queue_resize (tree_column);
#else
		gtk_widget_queue_draw (GTK_WIDGET (self));
#endif
	}
}

void
modest_folder_view_set_style (ModestFolderView *self,
			      ModestFolderViewStyle style)
{
	ModestFolderViewPrivate *priv;

	g_return_if_fail (self && MODEST_IS_FOLDER_VIEW(self));
	g_return_if_fail (style == MODEST_FOLDER_VIEW_STYLE_SHOW_ALL ||
			  style == MODEST_FOLDER_VIEW_STYLE_SHOW_ONE);

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(self);


	priv->style = style;
}

void
modest_folder_view_set_account_id_of_visible_server_account (ModestFolderView *self,
							     const gchar *account_id)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *model;

	g_return_if_fail (self && MODEST_IS_FOLDER_VIEW(self));

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

	/* Notify observers */
	g_signal_emit (G_OBJECT(self),
		       signals[VISIBLE_ACCOUNT_CHANGED_SIGNAL], 0,
		       account_id);
}

const gchar *
modest_folder_view_get_account_id_of_visible_server_account (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_FOLDER_VIEW(self), NULL);

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
				    TYPE_COLUMN,
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
#ifndef MODEST_TOOLKIT_HILDON2
	GtkTreeModel *model;
	GtkTreeIter iter, inbox_iter;
	GtkTreeSelection *sel;
	GtkTreePath *path = NULL;

	g_return_if_fail (self && MODEST_IS_FOLDER_VIEW(self));

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (!model)
		return;

	expand_root_items (self);
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));

	if (!gtk_tree_model_get_iter_first (model, &iter)) {
		g_warning ("%s: model is empty", __FUNCTION__);
		return;
	}

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
#endif
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
				    INSTANCE_COLUMN, &a_folder,
				    NAME_COLUMN, &name,
				    TYPE_COLUMN, &type,
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

#ifndef MODEST_TOOLKIT_HILDON2
static void
on_row_inserted_maybe_select_folder (GtkTreeModel *tree_model,
				     GtkTreePath *path,
				     GtkTreeIter *iter,
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
			    TYPE_COLUMN, &type,
			    INSTANCE_COLUMN, &instance,
			    -1);

	if (!instance)
		return;

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

		/* Refilter the model */
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (tree_model));
	}
}
#endif

void
modest_folder_view_disable_next_folder_selection (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_if_fail (self && MODEST_IS_FOLDER_VIEW(self));

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

	g_return_val_if_fail (self && MODEST_IS_FOLDER_VIEW (self), FALSE);
	g_return_val_if_fail (folder && TNY_IS_FOLDER (folder), FALSE);

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


	/* Refilter the model, before selecting the folder */
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));

	if (!gtk_tree_model_get_iter_first (model, &iter)) {
		g_warning ("%s: model is empty", __FUNCTION__);
		return FALSE;
	}

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
modest_folder_view_copy_selection (ModestFolderView *self)
{
	g_return_if_fail (self && MODEST_IS_FOLDER_VIEW(self));

	/* Copy selection */
	_clipboard_set_selected_data (self, FALSE);
}

void
modest_folder_view_cut_selection (ModestFolderView *folder_view)
{
	ModestFolderViewPrivate *priv = NULL;
	GtkTreeModel *model = NULL;
	const gchar **hidding = NULL;
	guint i, n_selected;

	g_return_if_fail (folder_view && MODEST_IS_FOLDER_VIEW (folder_view));
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
	GtkTreeModel *old_tny_model = NULL;
	GtkTreeModel *new_tny_model = NULL;
	ModestFolderViewPrivate *dst_priv;

	g_return_if_fail (folder_view_src && MODEST_IS_FOLDER_VIEW (folder_view_src));
	g_return_if_fail (folder_view_dst && MODEST_IS_FOLDER_VIEW (folder_view_dst));

	dst_priv = MODEST_FOLDER_VIEW_GET_PRIVATE (folder_view_dst);
	if (!get_inner_models (folder_view_src, NULL, NULL, &new_tny_model))
		new_tny_model = NULL;

	/* Get src model*/
	if (get_inner_models (folder_view_dst, NULL, NULL, &old_tny_model)) {
		modest_signal_mgr_disconnect (dst_priv->signal_handlers,
					      G_OBJECT (old_tny_model),
					      "activity-changed");
	}
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
#ifndef MODEST_TOOLKIT_HILDON2
	dst_priv->signal_handlers = modest_signal_mgr_connect (dst_priv->signal_handlers,
							       G_OBJECT(new_filter_model), "row-inserted",
							       (GCallback) on_row_inserted_maybe_select_folder,
							       folder_view_dst);
#endif
#ifdef MODEST_TOOLKIT_HILDON2
	if (new_tny_model) {
		dst_priv->signal_handlers = modest_signal_mgr_connect (dst_priv->signal_handlers,
								       G_OBJECT (new_tny_model),
								       "activity-changed",
								       G_CALLBACK (on_activity_changed),
								       folder_view_dst);
	}
#endif

	/* Free */
	g_object_unref (new_filter_model);
}

void
modest_folder_view_show_non_move_folders (ModestFolderView *folder_view,
					  gboolean show)
{
	GtkTreeModel *model = NULL;
	ModestFolderViewPrivate* priv;

	g_return_if_fail (folder_view && MODEST_IS_FOLDER_VIEW (folder_view));

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(folder_view);
	priv->show_non_move = show;
/* 	modest_folder_view_update_model(folder_view, */
/* 					TNY_ACCOUNT_STORE(modest_runtime_get_account_store())); */

	/* Hide special folders */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (folder_view));
	if (GTK_IS_TREE_MODEL_FILTER (model)) {
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
	}
}

void
modest_folder_view_show_message_count (ModestFolderView *folder_view,
					  gboolean show)
{
	ModestFolderViewPrivate* priv;

	g_return_if_fail (folder_view && MODEST_IS_FOLDER_VIEW (folder_view));

	priv = MODEST_FOLDER_VIEW_GET_PRIVATE(folder_view);
	priv->show_message_count = show;

	g_object_set (G_OBJECT (priv->messages_renderer),
		      "visible", (priv->cell_style == MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT && priv->show_message_count),
		      NULL);
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


static void
on_display_name_changed (ModestAccountMgr *mgr,
			 const gchar *account,
			 gpointer user_data)
{
	ModestFolderView *self;

	self = MODEST_FOLDER_VIEW (user_data);

	/* Force a redraw */
#if GTK_CHECK_VERSION(2, 8, 0)
	GtkTreeViewColumn * tree_column;

	tree_column = gtk_tree_view_get_column (GTK_TREE_VIEW (self),
						NAME_COLUMN);
	gtk_tree_view_column_queue_resize (tree_column);
#else
	gtk_widget_queue_draw (GTK_WIDGET (self));
#endif
}

void 
modest_folder_view_set_cell_style (ModestFolderView *self,
				   ModestFolderViewCellStyle cell_style)
{
	ModestFolderViewPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	priv->cell_style = cell_style;

	g_object_set (G_OBJECT (priv->messages_renderer),
		      "visible", (cell_style == MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT && priv->show_message_count),
		      NULL);
	
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
update_style (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;
	GdkColor style_color, style_active_color;
	PangoAttrList *attr_list;
	GtkStyle *style;
	PangoAttribute *attr;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	/* Set color */

	attr_list = pango_attr_list_new ();
	if (!gtk_style_lookup_color (GTK_WIDGET (self)->style, "SecondaryTextColor", &style_color)) {
		gdk_color_parse ("grey", &style_color);
	}
	attr = pango_attr_foreground_new (style_color.red, style_color.green, style_color.blue);
	pango_attr_list_insert (attr_list, attr);
	
	/* set font */
	style = gtk_rc_get_style_by_paths (gtk_widget_get_settings
					   (GTK_WIDGET(self)),
					   "SmallSystemFont", NULL,
					   G_TYPE_NONE);
	if (style) {
		attr = pango_attr_font_desc_new (pango_font_description_copy
						 (style->font_desc));
		pango_attr_list_insert (attr_list, attr);

		g_object_set (G_OBJECT (priv->messages_renderer),
			      "foreground-gdk", &style_color,
			      "foreground-set", TRUE,
			      "attributes", attr_list,
			      NULL);
		pango_attr_list_unref (attr_list);
	}

	if (gtk_style_lookup_color (GTK_WIDGET (self)->style, "ActiveTextColor", &style_active_color)) {
		priv->active_color = style_active_color;
	} else {
		gdk_color_parse ("000", &(priv->active_color));
	}
}

static void 
on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata)
{
	if (strcmp ("style", spec->name) == 0) {
		update_style (MODEST_FOLDER_VIEW (obj));
		gtk_widget_queue_draw (GTK_WIDGET (obj));
	} 
}

void 
modest_folder_view_set_filter (ModestFolderView *self,
			       ModestFolderViewFilter filter)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *filter_model;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	priv->filter |= filter;

	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (GTK_IS_TREE_MODEL_FILTER(filter_model)) {
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));	
	}
}

void 
modest_folder_view_unset_filter (ModestFolderView *self,
				 ModestFolderViewFilter filter)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *filter_model;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	priv->filter &= ~filter;

	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (GTK_IS_TREE_MODEL_FILTER(filter_model)) {
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));	
	}
}

gboolean
modest_folder_view_any_folder_fulfils_rules (ModestFolderView *self,
					     ModestTnyFolderRules rules)
{
	GtkTreeModel *filter_model;
	GtkTreeIter iter;
	gboolean fulfil = FALSE;

	if (!get_inner_models (self, &filter_model, NULL, NULL))
		return FALSE;

	if (!gtk_tree_model_get_iter_first (filter_model, &iter))
		return FALSE;

	do {
		TnyFolderStore *folder;

		gtk_tree_model_get (filter_model, &iter, INSTANCE_COLUMN, &folder, -1);
		if (folder) {
			if (TNY_IS_FOLDER (folder)) {
				ModestTnyFolderRules folder_rules = modest_tny_folder_get_rules (TNY_FOLDER (folder));
				/* Folder rules are negative: non_writable, non_deletable... */
				if (!(folder_rules & rules))
					fulfil = TRUE;
			}
			g_object_unref (folder);
		}

	} while (gtk_tree_model_iter_next (filter_model, &iter) && !fulfil);

	return fulfil;
}

void 
modest_folder_view_set_list_to_move (ModestFolderView *self,
				     TnyList *list)
{
	ModestFolderViewPrivate *priv;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	if (priv->list_to_move)
		g_object_unref (priv->list_to_move);

	if (list)
		g_object_ref (list);

	priv->list_to_move = list;
}

void
modest_folder_view_set_mailbox (ModestFolderView *self, const gchar *mailbox)
{
	ModestFolderViewPrivate *priv;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (self));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	if (priv->mailbox)
		g_free (priv->mailbox);

	priv->mailbox = g_strdup (mailbox);

	/* Notify observers */
	g_signal_emit (G_OBJECT(self),
		       signals[VISIBLE_ACCOUNT_CHANGED_SIGNAL], 0,
		       priv->visible_account_id);
}

const gchar *
modest_folder_view_get_mailbox (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), NULL);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);

	return (const gchar *) priv->mailbox;
}

gboolean 
modest_folder_view_get_activity (ModestFolderView *self)
{
	ModestFolderViewPrivate *priv;
	GtkTreeModel *inner_model;

	g_return_val_if_fail (MODEST_IS_FOLDER_VIEW (self), FALSE);
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (self);
	g_return_val_if_fail (get_inner_models (self, NULL, NULL, &inner_model), FALSE);

	if (TNY_IS_GTK_FOLDER_LIST_STORE (inner_model)) {
		return tny_gtk_folder_list_store_get_activity (TNY_GTK_FOLDER_LIST_STORE (inner_model));
	} else {
		return FALSE;
	}
}

#ifdef MODEST_TOOLKIT_HILDON2
static void
on_activity_changed (TnyGtkFolderListStore *store,
		     gboolean activity,
		     ModestFolderView *folder_view)
{
	ModestFolderViewPrivate *priv;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW (folder_view));
	g_return_if_fail (TNY_IS_GTK_FOLDER_LIST_STORE (store));
	priv = MODEST_FOLDER_VIEW_GET_PRIVATE (folder_view);

	g_signal_emit (G_OBJECT (folder_view), signals[ACTIVITY_CHANGED_SIGNAL], 0,
		       activity);
}
#endif

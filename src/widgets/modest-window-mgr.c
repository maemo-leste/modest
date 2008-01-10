/* Copyright (c) 2006,2007 Nokia Corporation
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

#include <string.h>
#include "modest-window-mgr.h"
#include "modest-runtime.h"
#include "modest-tny-folder.h"
#include "modest-ui-actions.h"
#include "modest-platform.h"
#include "widgets/modest-main-window.h"
#include "widgets/modest-msg-edit-window.h"
#include "widgets/modest-msg-view-window.h"
#include "modest-debug.h"


/* 'private'/'protected' functions */
static void modest_window_mgr_class_init (ModestWindowMgrClass *klass);
static void modest_window_mgr_init       (ModestWindowMgr *obj);
static void modest_window_mgr_finalize   (GObject *obj);

static gboolean on_window_destroy        (ModestWindow *window,
					  GdkEvent *event,
					  ModestWindowMgr *self);

static gboolean on_modal_window_close    (GtkWidget *widget,
					  GdkEvent *event,
					  gpointer user_data);

static void     on_modal_dialog_close    (GtkDialog *dialog,
					  gint arg1,
					  gpointer user_data);

static const gchar* get_show_toolbar_key (GType window_type,
					  gboolean fullscreen);

/* list my signals  */
enum {
	WINDOW_LIST_EMPTY_SIGNAL,
	NUM_SIGNALS
};

typedef struct _ModestWindowMgrPrivate ModestWindowMgrPrivate;
struct _ModestWindowMgrPrivate {
	GList        *window_list;
	guint         banner_counter;

	ModestWindow *main_window;

	GMutex       *queue_lock;
	GQueue       *modal_windows;
	
	gboolean     fullscreen_mode;
	
	GSList       *windows_that_prevent_hibernation;
	GSList       *preregistered_uids;
	GHashTable   *destroy_handlers;
	GHashTable   *viewer_handlers;
	
	guint        closing_time;

	GSList       *modal_handler_uids;
	GtkWidget    *cached_view;
	GtkWidget    *cached_editor;
	guint        idle_load_view_id;
	guint        idle_load_editor_id;
};
#define MODEST_WINDOW_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                               MODEST_TYPE_WINDOW_MGR, \
                                               ModestWindowMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[NUM_SIGNALS] = {0};

GType
modest_window_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestWindowMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_window_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestWindowMgr),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_window_mgr_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestWindowMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_window_mgr_class_init (ModestWindowMgrClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_window_mgr_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestWindowMgrPrivate));


	/**
	 * ModestWindowMgr::window-list-empty
	 * @self: the #ModestWindowMgr that emits the signal
	 * @user_data: user data set when the signal handler was connected
	 *
	 * Issued whenever the window list becomes empty
	 */
	signals[WINDOW_LIST_EMPTY_SIGNAL] =
		g_signal_new ("window-list-empty",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestWindowMgrClass, window_list_empty),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static gboolean
idle_load_view (ModestWindowMgr *mgr)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (mgr);
	
	priv->cached_view = g_object_new (MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	priv->idle_load_view_id = 0;
	return FALSE;
}

static gboolean
idle_load_editor (ModestWindowMgr *mgr)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (mgr);
	
	priv->cached_editor = g_object_new (MODEST_TYPE_MSG_EDIT_WINDOW, NULL);
	priv->idle_load_editor_id = 0;
	return FALSE;
}

static void
load_new_view (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	if ((priv->cached_view == NULL) && (priv->idle_load_view_id == 0))
		priv->idle_load_view_id = g_idle_add ((GSourceFunc) idle_load_view, self);
}

static void
load_new_editor (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	if ((priv->cached_editor == NULL) && (priv->idle_load_editor_id == 0))
		priv->idle_load_editor_id = g_idle_add ((GSourceFunc) idle_load_editor, self);
}

static void
modest_window_mgr_init (ModestWindowMgr *obj)
{
	ModestWindowMgrPrivate *priv;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE(obj);
	priv->window_list = NULL;
	priv->banner_counter = 0;
	priv->main_window = NULL;
	priv->fullscreen_mode = FALSE;

	priv->modal_windows = g_queue_new ();
	priv->queue_lock = g_mutex_new ();
	
	priv->preregistered_uids = NULL;

	/* Could not initialize it from gconf, singletons are not
	   ready yet */
	priv->destroy_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);	
	priv->viewer_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

	priv->closing_time = 0;

	priv->modal_handler_uids = NULL;
	priv->cached_view = NULL;
	priv->cached_editor = NULL;
}

static void
modest_window_mgr_finalize (GObject *obj)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE(obj);

	if (priv->idle_load_view_id > 0) {
		g_source_remove (priv->idle_load_view_id);
		priv->idle_load_view_id = 0;
	}
	
	if (priv->idle_load_editor_id > 0) {
		g_source_remove (priv->idle_load_editor_id);
		priv->idle_load_editor_id = 0;
	}
	
	if (priv->cached_view) {
		gtk_widget_destroy (priv->cached_view);
		priv->cached_view = NULL;
	}
	if (priv->cached_editor) {
		gtk_widget_destroy (priv->cached_editor);
		priv->cached_editor = NULL;
	}

	if (priv->window_list) {
		GList *iter = priv->window_list;
		/* unregister pending windows */
		while (iter) {
			modest_window_mgr_unregister_window (MODEST_WINDOW_MGR (obj), 
							     MODEST_WINDOW (iter->data));
			iter = g_list_next (iter);
		}
		g_list_free (priv->window_list);
		priv->window_list = NULL;
	}

	g_slist_foreach (priv->preregistered_uids, (GFunc)g_free, NULL);
	g_slist_free (priv->preregistered_uids);

	
	/* Free the hash table with the handlers */
	if (priv->destroy_handlers) {
		g_hash_table_destroy (priv->destroy_handlers);
		priv->destroy_handlers = NULL;
	}

	if (priv->viewer_handlers) {
		g_hash_table_destroy (priv->viewer_handlers);
		priv->viewer_handlers = NULL;
	}

	modest_signal_mgr_disconnect_all_and_destroy (priv->modal_handler_uids);

	if (priv->modal_windows) {
		g_mutex_lock (priv->queue_lock);
		g_queue_free (priv->modal_windows);
		priv->modal_windows = NULL;
		g_mutex_unlock (priv->queue_lock);
	}
	g_mutex_free (priv->queue_lock);
	
	/* Do not unref priv->main_window because it does not hold a
	   new reference */

	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestWindowMgr*
modest_window_mgr_new (void)
{
	return MODEST_WINDOW_MGR(g_object_new(MODEST_TYPE_WINDOW_MGR, NULL));
}




/* do we have uid? */
static gboolean
has_uid (GSList *list, const gchar *uid)
{
	GSList *cursor = list;

	if (!uid)
		return FALSE;
	
	while (cursor) {
		if (cursor->data && strcmp (cursor->data, uid) == 0)
			return TRUE;
		cursor = g_slist_next (cursor);
	}
	return FALSE;
}


/* remove all from the list have have uid = uid */
static GSList*
remove_uid (GSList *list, const gchar *uid)
{
	GSList *cursor = list, *start = list;
	
	if (!uid)
		return FALSE;
	
	while (cursor) {
		GSList *next = g_slist_next (cursor);
		if (cursor->data && strcmp (cursor->data, uid) == 0) {
			g_free (cursor->data);
			start = g_slist_delete_link (start, cursor);
		}
		cursor = next;
	}
	return start;
}


static GSList *
append_uid (GSList *list, const gchar *uid)
{
	return g_slist_append (list, g_strdup(uid));
}



void 
modest_window_mgr_register_header (ModestWindowMgr *self,  TnyHeader *header, const gchar *alt_uid)
{
	ModestWindowMgrPrivate *priv;
	gchar* uid;
	
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	g_return_if_fail (TNY_IS_HEADER(header));
		
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	uid = modest_tny_folder_get_header_unique_id (header);

	if (uid == NULL)
		uid = g_strdup (alt_uid);
	
	if (!has_uid (priv->preregistered_uids, uid)) {
		MODEST_DEBUG_BLOCK(g_debug ("registering new uid %s", uid););
		priv->preregistered_uids = append_uid (priv->preregistered_uids, uid);
	} else
		MODEST_DEBUG_BLOCK(g_debug ("already had uid %s", uid););
	
	g_free (uid);
}

void 
modest_window_mgr_unregister_header (ModestWindowMgr *self,  TnyHeader *header)
{
	ModestWindowMgrPrivate *priv;
	gchar* uid;
	
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	g_return_if_fail (TNY_IS_HEADER(header));
		
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	uid = modest_tny_folder_get_header_unique_id (header);

	if (!has_uid (priv->preregistered_uids, uid)) {
		MODEST_DEBUG_BLOCK(g_debug ("trying to unregister non-existing uid %s", uid););
		priv->preregistered_uids = append_uid (priv->preregistered_uids, uid);
	} else
		MODEST_DEBUG_BLOCK(g_debug ("unregistering uid %s", uid););
	
	if (has_uid (priv->preregistered_uids, uid)) {
		priv->preregistered_uids = remove_uid (priv->preregistered_uids, uid);
		if (has_uid (priv->preregistered_uids, uid))
			g_debug ("BUG: uid %s NOT removed", uid);
		else
			MODEST_DEBUG_BLOCK(g_debug ("uid %s removed", uid););
	}
		
	g_free (uid);
}


#define MODEST_WINDOW_HELP_ID_PARAM "help-id"

void
modest_window_mgr_register_help_id (ModestWindowMgr *self, GtkWindow *win, const gchar* help_id)
{
	/* we don't need 'self', but for API consistency... */
	g_return_if_fail (self && MODEST_IS_WINDOW_MGR(self));

	g_return_if_fail (win && GTK_IS_WINDOW(win));
	g_return_if_fail (help_id);
	
	g_object_set_data_full (G_OBJECT(win), MODEST_WINDOW_HELP_ID_PARAM,
				g_strdup(help_id), g_free);
}


const gchar*
modest_window_mgr_get_help_id (ModestWindowMgr *self, GtkWindow *win)
{
	const gchar* help_id = NULL;

	/* we don't need 'self', but for API consistency... */
	g_return_val_if_fail (self && MODEST_IS_WINDOW_MGR(self), NULL);
	
	g_return_val_if_fail (win, NULL);
	g_return_val_if_fail (GTK_IS_WINDOW(win), NULL);
	
	if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *folder_view;
		TnyFolderStore *folder_store;
		
		/* Get selected folder */
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		folder_store = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

		/* Switch help_id */
		if (folder_store && TNY_IS_FOLDER (folder_store)) {
			help_id = modest_tny_folder_get_help_id (TNY_FOLDER (folder_store));
			if (!help_id)
				g_warning ("%s: BUG: did not get a valid help_id", __FUNCTION__);
		}
		if (folder_store)
			g_object_unref (folder_store);
	}

	if (!help_id)
		help_id = g_object_get_data (G_OBJECT(win), MODEST_WINDOW_HELP_ID_PARAM);
		
	return help_id;
}

static gint
compare_msguids (ModestWindow *win,
		 const gchar *uid)
{
	const gchar *msg_uid;

	if ((!MODEST_IS_MSG_EDIT_WINDOW (win)) && (!MODEST_IS_MSG_VIEW_WINDOW (win)))
		return 1;

	/* Get message uid from msg window */
	if (MODEST_IS_MSG_EDIT_WINDOW (win)) {
		msg_uid = modest_msg_edit_window_get_message_uid (MODEST_MSG_EDIT_WINDOW (win));
		if (msg_uid && uid &&!strcmp (msg_uid, uid))
			return 0;
	} else {
		msg_uid = modest_msg_view_window_get_message_uid (MODEST_MSG_VIEW_WINDOW (win));
	}
	
	if (msg_uid && uid &&!strcmp (msg_uid, uid))
		return 0;
	else
		return 1;
}

void
modest_window_mgr_close_all_windows (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv = NULL;
	gboolean ret_value = FALSE;
	
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	
	/* If there is a main window then try to close it, and it will
	   close the others if needed */
	if (priv->main_window) {
		g_signal_emit_by_name (priv->main_window, "delete-event", NULL, &ret_value);
	} else {
		GList *wins = NULL, *next = NULL;

		/* delete-event handler actually removes window_list item, */
		wins = priv->window_list;
		while (wins) {
			next = g_list_next (wins);
			g_signal_emit_by_name (G_OBJECT (wins->data), "delete-event", NULL, &ret_value);
			wins = next;
		}
	}
}


gboolean
modest_window_mgr_find_registered_header (ModestWindowMgr *self, TnyHeader *header,
					  ModestWindow **win)
{
	ModestWindowMgrPrivate *priv = NULL;
	gchar* uid = NULL;
	gboolean has_header, has_window = FALSE;
	GList *item = NULL;

	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (TNY_IS_HEADER(header), FALSE);
	
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	uid = modest_tny_folder_get_header_unique_id (header);
	
	if (win)
		*win = NULL;

	has_header = has_uid (priv->preregistered_uids, uid);
	
	item = g_list_find_custom (priv->window_list, uid, (GCompareFunc) compare_msguids);
	if (item) {
		has_window = TRUE;
		if (win) {
			if ((!MODEST_IS_MSG_VIEW_WINDOW(item->data)) && 
			    (!MODEST_IS_MSG_EDIT_WINDOW (item->data)))
				g_debug ("not a valid window!");
			else {
				g_debug ("found a window");
				*win = MODEST_WINDOW (item->data);
			}
		}
	}
	g_free (uid);
	
	return has_header || has_window;
}

static const gchar *
get_show_toolbar_key (GType window_type,
		      gboolean fullscreen)
{
	const gchar *key = NULL;

	if (window_type == MODEST_TYPE_MAIN_WINDOW)
		key = (fullscreen) ? 
			MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR_FULLSCREEN :
			MODEST_CONF_MAIN_WINDOW_SHOW_TOOLBAR;
	else if (window_type == MODEST_TYPE_MSG_VIEW_WINDOW)
		key = (fullscreen) ? 
			MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR_FULLSCREEN :
			MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR;
	else if (window_type ==  MODEST_TYPE_MSG_EDIT_WINDOW)
		key = (fullscreen) ? 
			MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR_FULLSCREEN :
			MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR;
	else
		g_return_val_if_reached (NULL);

	return key;
}

void 
modest_window_mgr_register_window (ModestWindowMgr *self, 
				   ModestWindow *window)
{
	GList *win;
	ModestWindowMgrPrivate *priv;
	gint *handler_id;
	const gchar *key;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	g_return_if_fail (GTK_IS_WINDOW (window));

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	win = g_list_find (priv->window_list, window);
	if (win) {
		/* this is for the case we want to register the window and it was already
		 * registered. We leave silently.
		 */
		return;
	}
	
	/* Check that it's not a second main window */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		if (priv->main_window) {
			g_warning ("%s: trying to register a second main window",
				   __FUNCTION__);
			return;
		} else {
			priv->main_window = window;
			load_new_view (self);
			load_new_editor (self);
		}
	}

	/* remove from the list of pre-registered uids */
	if (MODEST_IS_MSG_VIEW_WINDOW(window)) {
		const gchar *uid = modest_msg_view_window_get_message_uid
			(MODEST_MSG_VIEW_WINDOW (window));

		if (!has_uid (priv->preregistered_uids, uid)) 
			g_debug ("weird: no uid for window (%s)", uid);
		
		MODEST_DEBUG_BLOCK(g_debug ("registering window for %s", uid ? uid : "<none>"););
		
		priv->preregistered_uids = 
			remove_uid (priv->preregistered_uids,
				    modest_msg_view_window_get_message_uid
				    (MODEST_MSG_VIEW_WINDOW (window)));
	} else if (MODEST_IS_MSG_EDIT_WINDOW(window)) {
		const gchar *uid = modest_msg_edit_window_get_message_uid
			(MODEST_MSG_EDIT_WINDOW (window));
		
		MODEST_DEBUG_BLOCK(g_debug ("registering window for %s", uid););

		priv->preregistered_uids = 
			remove_uid (priv->preregistered_uids,
				    modest_msg_edit_window_get_message_uid
				    (MODEST_MSG_EDIT_WINDOW (window)));
	}
	
	/* Add to list. Keep a reference to the window */
	g_object_ref (window);
	priv->window_list = g_list_prepend (priv->window_list, window);

	/* Listen to object destruction */
	handler_id = g_malloc0 (sizeof (gint));
	*handler_id = g_signal_connect (window, "delete-event", G_CALLBACK (on_window_destroy), self);
	g_hash_table_insert (priv->destroy_handlers, window, handler_id);

	/* If there is a msg view window, let the main window listen the msg-changed signal */
	if (MODEST_IS_MSG_VIEW_WINDOW(window) && priv->main_window) {
		gulong *handler;
		handler = g_malloc0 (sizeof (gulong));
		*handler = g_signal_connect (window, "msg-changed", 
					     G_CALLBACK (modest_main_window_on_msg_view_window_msg_changed), 
					     priv->main_window);
		g_hash_table_insert (priv->viewer_handlers, window, handler);
	}

	/* Put into fullscreen if needed */
	if (priv->fullscreen_mode)
		gtk_window_fullscreen (GTK_WINDOW (window));

	/* Show/hide toolbar & fullscreen */	
	key = get_show_toolbar_key (G_TYPE_FROM_INSTANCE (window), priv->fullscreen_mode);
	modest_window_show_toolbar (window, modest_conf_get_bool (modest_runtime_get_conf (), key, NULL));
}

static gboolean
on_window_destroy (ModestWindow *window, 
		   GdkEvent *event,
		   ModestWindowMgr *self)
{
	gint dialog_response = GTK_RESPONSE_NONE;

	/* Specific stuff first */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		ModestWindowMgrPrivate *priv;
		priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

		/* If more than one window already opened */
		if (g_list_length (priv->window_list) > 1) {

			/* Present the window if it's not visible now */
			if (!gtk_window_has_toplevel_focus (GTK_WINDOW (window)))
				gtk_window_present (GTK_WINDOW (window));
			/* Create the confirmation dialog MSG-NOT308 */
			dialog_response = modest_platform_run_confirmation_dialog (
					GTK_WINDOW (window), _("emev_nc_close_windows"));

			/* If the user wants to close all the windows */
			if ((dialog_response == GTK_RESPONSE_OK) 
					|| (dialog_response == GTK_RESPONSE_ACCEPT) 
					|| (dialog_response == GTK_RESPONSE_YES))
				{
					GList *iter = priv->window_list;
					do {
						if (iter->data != window) {
							GList *tmp = iter->next;
							on_window_destroy (MODEST_WINDOW (iter->data),
									event,
									self);
							iter = tmp;
						} else {
							iter = g_list_next (iter);
						}
					} while (iter);
				}
			else
				{
					return TRUE;
				}
		}
	}
	else {
		if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
			gboolean sent = FALSE;
			gint response = GTK_RESPONSE_ACCEPT;
			sent = modest_msg_edit_window_get_sent (MODEST_MSG_EDIT_WINDOW (window));
			/* Save currently edited message to Drafts if it was not sent */
			if (!sent && modest_msg_edit_window_is_modified (MODEST_MSG_EDIT_WINDOW (window))) {

				/* Raise the window if it's minimized */
				if (!gtk_window_has_toplevel_focus (GTK_WINDOW (window)))
					gtk_window_present (GTK_WINDOW (window));
				
				response =
					modest_platform_run_confirmation_dialog (GTK_WINDOW (window),
										 _("mcen_nc_no_email_message_modified_save_changes"));
				/* Save to drafts */
				if (response != GTK_RESPONSE_CANCEL)
					modest_ui_actions_on_save_to_drafts (NULL, MODEST_MSG_EDIT_WINDOW (window));				
			}
		}
	}

	/* Unregister window */
	modest_window_mgr_unregister_window (self, window);
	
	return FALSE;
}

static void
disconnect_msg_changed (gpointer key, 
			gpointer value, 
			gpointer user_data)
{
	guint handler_id;
	handler_id = GPOINTER_TO_UINT(value);
	
	if (key && G_IS_OBJECT(key))
		g_signal_handler_disconnect (G_OBJECT (key), handler_id);
}

void 
modest_window_mgr_unregister_window (ModestWindowMgr *self, 
				     ModestWindow *window)
{
	GList *win;
	ModestWindowMgrPrivate *priv;
	gulong *tmp, handler_id;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	g_return_if_fail (MODEST_IS_WINDOW (window));

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	win = g_list_find (priv->window_list, window);
	if (!win) {
		g_warning ("Trying to unregister a window that has not being registered yet");
		return;
	}

	/* If it's the main window unset it */
	if (priv->main_window == window) {
		priv->main_window = NULL;

		/* Disconnect all emissions of msg-changed */
		if (priv->viewer_handlers) {
			g_hash_table_foreach (priv->viewer_handlers, 
					      disconnect_msg_changed, 
					      NULL);
			g_hash_table_destroy (priv->viewer_handlers);
			priv->viewer_handlers = NULL;
		}
	}

	/* Remove the viewer window handler from the hash table. The
	   HashTable could not exist if the main window was closeed
	   when there were other windows remaining */
	if (MODEST_IS_MSG_VIEW_WINDOW (window) && priv->viewer_handlers) {
		tmp = (gulong *) g_hash_table_lookup (priv->viewer_handlers, window);
		/* If the viewer was created without a main window
		   (for example when opening a message through D-Bus
		   the viewer handlers was not registered */
		if (tmp) {
			g_signal_handler_disconnect (window, *tmp);
			g_hash_table_remove (priv->viewer_handlers, window);
		}
	}

	/* Save state */
	modest_window_save_state (window);

	/* Remove from list & hash table */
	priv->window_list = g_list_remove_link (priv->window_list, win);
	tmp = g_hash_table_lookup (priv->destroy_handlers, window);
	handler_id = *tmp;
	g_hash_table_remove (priv->destroy_handlers, window);

	/* cancel open and receive operations */
	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		ModestMailOperationTypeOperation type;
		GSList* pending_ops = NULL;
		GSList* tmp_list = NULL;
		pending_ops = modest_mail_operation_queue_get_by_source (
				modest_runtime_get_mail_operation_queue (), 
				G_OBJECT (window));
		while (pending_ops != NULL) {
			type = modest_mail_operation_get_type_operation (MODEST_MAIL_OPERATION (pending_ops->data));
			if (type == MODEST_MAIL_OPERATION_TYPE_RECEIVE || type == MODEST_MAIL_OPERATION_TYPE_OPEN) {
				modest_mail_operation_cancel (pending_ops->data);
			}
			g_object_unref (G_OBJECT (pending_ops->data));
			tmp_list = pending_ops;
			pending_ops = g_slist_next (pending_ops);
			g_slist_free_1 (tmp_list);
		}
	}
	
	/* Disconnect the "delete-event" handler, we won't need it anymore */
	g_signal_handler_disconnect (window, handler_id);

	/* Disconnect all the window signals */
	modest_window_disconnect_signals (window);
	
	/* Destroy the window */
	gtk_widget_destroy (win->data);
	
	/* If there are no more windows registered emit the signal */
	if (priv->window_list == NULL && priv->banner_counter == 0)
		g_signal_emit (self, signals[WINDOW_LIST_EMPTY_SIGNAL], 0);
}



void
modest_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
				       gboolean on)
{
	ModestWindowMgrPrivate *priv;
	GList *win = NULL;
	ModestConf *conf;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	/* If there is no change do nothing */
	if (priv->fullscreen_mode == on)
		return;

	priv->fullscreen_mode = on;

	conf = modest_runtime_get_conf ();

	/* Update windows */
	win = priv->window_list;
	while (win) {
		gboolean show;
		const gchar *key = NULL;

		/* Getting this from gconf everytime is not that
		   expensive, we'll do it just a few times */
		key = get_show_toolbar_key (G_TYPE_FROM_INSTANCE (win->data), on);
		show = modest_conf_get_bool (conf, key, NULL);

		/* Set fullscreen/unfullscreen */
		if (on)
			gtk_window_fullscreen (GTK_WINDOW (win->data));
		else
			gtk_window_unfullscreen (GTK_WINDOW (win->data));

		/* Show/Hide toolbar */
		modest_window_show_toolbar (MODEST_WINDOW (win->data), show);

		win = g_list_next (win);
	}
}

gboolean
modest_window_mgr_get_fullscreen_mode (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return priv->fullscreen_mode;
}

void 
modest_window_mgr_show_toolbars (ModestWindowMgr *self,
				 GType window_type,
				 gboolean show_toolbars,
				 gboolean fullscreen)
{
	ModestWindowMgrPrivate *priv;
	ModestConf *conf;
	const gchar *key = NULL;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	conf = modest_runtime_get_conf ();

	/* If nothing changes then return */
	key = get_show_toolbar_key (window_type, fullscreen);
	conf = modest_runtime_get_conf ();
	if (modest_conf_get_bool (conf, key, NULL) == show_toolbars)
		return;

	/* Save in conf */
	modest_conf_set_bool (conf, key, show_toolbars, NULL);

	/* Apply now if the view mode is the right one */
	if ((fullscreen && priv->fullscreen_mode) ||
	    (!fullscreen && !priv->fullscreen_mode)) {

		GList *win = priv->window_list;

		while (win) {
			if (G_TYPE_FROM_INSTANCE (win->data) == window_type)
				modest_window_show_toolbar (MODEST_WINDOW (win->data),
							    show_toolbars);
			win = g_list_next (win);
		}
	}
}

ModestWindow*  
modest_window_mgr_get_main_window (ModestWindowMgr *self, gboolean create)
{
	ModestWindowMgrPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), NULL);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	
	/* create the main window, if it hasn't been created yet */
	if (!priv->main_window && create) {
		/* modest_window_mgr_register_window will set priv->main_window */
		modest_window_mgr_register_window (self, modest_main_window_new ());
		MODEST_DEBUG_BLOCK(
			g_debug ("%s: created main window: %p\n", __FUNCTION__, priv->main_window);
		);
	}
	
	return priv->main_window;
}


gboolean
modest_window_mgr_main_window_exists  (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return priv->main_window != NULL;
}


GtkWindow *
modest_window_mgr_get_modal (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), NULL);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return g_queue_peek_head (priv->modal_windows);
}


void
modest_window_mgr_set_modal (ModestWindowMgr *self, 
			     GtkWindow *window)
{
	GtkWindow *old_modal;
	ModestWindowMgrPrivate *priv;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	g_return_if_fail (GTK_IS_WINDOW (window));

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	g_mutex_lock (priv->queue_lock);
	old_modal = g_queue_peek_head (priv->modal_windows);
	g_mutex_unlock (priv->queue_lock);

	if (!old_modal) {	
		gtk_window_set_modal (window, TRUE);
	} else {
		/* un-modalize the old one; the one on top should be the
		 * modal one */
		gtk_window_set_transient_for (window, GTK_WINDOW(old_modal));	
		gtk_window_set_modal (window, TRUE);
	}

	/* this will be the new modal window */
	g_mutex_lock (priv->queue_lock);
	g_queue_push_head (priv->modal_windows, window);
	g_mutex_unlock (priv->queue_lock);

	if (GTK_IS_DIALOG (window))
		/* Note that response is not always enough because it
		   could be captured and removed easily by dialogs but
		   works for most of situations */
		priv->modal_handler_uids = 
			modest_signal_mgr_connect (priv->modal_handler_uids, 
						   G_OBJECT (window), 
						   "response",
						   G_CALLBACK (on_modal_dialog_close), 
						   self);
	else
		priv->modal_handler_uids = 
			modest_signal_mgr_connect (priv->modal_handler_uids, 
						   G_OBJECT (window), 
						   "delete-event",
						   G_CALLBACK (on_modal_window_close), 
						   self);
}


static void
on_nonhibernating_window_hide(GtkWidget *widget, gpointer user_data)
{
	ModestWindowMgr *self = MODEST_WINDOW_MGR (user_data);
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	
	/* Forget this window,
	 * so hibernation will be allowed again if no windows are remembered: */
	priv->windows_that_prevent_hibernation =
		g_slist_remove (priv->windows_that_prevent_hibernation, GTK_WINDOW(widget));
}

static void
on_nonhibernating_window_show(GtkWidget *widget, gpointer user_data)
{
	ModestWindowMgr *self = MODEST_WINDOW_MGR (user_data);
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	
	GtkWindow *window = GTK_WINDOW (widget);
	
	priv->windows_that_prevent_hibernation = 
			g_slist_append (priv->windows_that_prevent_hibernation, window);
	
	/* Allow hibernation again when the window has been hidden: */
	g_signal_connect (window, "hide", 
		G_CALLBACK (on_nonhibernating_window_hide), self);
}

void
modest_window_mgr_prevent_hibernation_while_window_is_shown (ModestWindowMgr *self,
								  GtkWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	
	if (GTK_WIDGET_VISIBLE(window)) {
		on_nonhibernating_window_show (GTK_WIDGET (window), self);
	} else {
		/* Wait for it to be shown: */
		g_signal_connect (window, "show", 
			G_CALLBACK (on_nonhibernating_window_show), self);	
	}
}

gboolean
modest_window_mgr_get_hibernation_is_prevented (ModestWindowMgr *self)
{
	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);
	
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	
	/* Prevent hibernation if any open windows are currently 
	 * preventing hibernation: */
	return (g_slist_length (priv->windows_that_prevent_hibernation) > 0);
}


void
modest_window_mgr_save_state_for_all_windows (ModestWindowMgr *self)
{
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	/* Iterate over all windows */
	GList *win = priv->window_list;
	while (win) {
		ModestWindow *window = MODEST_WINDOW (win->data);
		if (window) {
			/* This calls the vfunc, 
			 * so each window can do its own thing: */
			modest_window_save_state (window);
		}	
		
		win = g_list_next (win);
	}
}

static gboolean
idle_top_modal (gpointer data)
{
	ModestWindowMgr *self = MODEST_WINDOW_MGR (data);
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	GtkWindow *topmost;

	/* Get the top modal */
	g_mutex_lock (priv->queue_lock);
	topmost = (GtkWindow *) g_queue_peek_head (priv->modal_windows);
	g_mutex_unlock (priv->queue_lock);

	/* Show it */
	if (topmost)
		gtk_window_present (topmost);

	return FALSE;
}

static void
remove_modal_from_queue (GtkWidget *widget,
			 ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	GList *item = NULL;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	/* Remove from queue. We don't use remove, because we want to
	   exit if the widget does not belong to the queue */
	g_mutex_lock (priv->queue_lock);
	item = g_queue_find (priv->modal_windows, widget);
	if (!item) {
		g_warning ("Trying to remove a modal window that is not registered");
		g_mutex_unlock (priv->queue_lock);
		return;
	}
	g_queue_unlink (priv->modal_windows, item);
	g_mutex_unlock (priv->queue_lock);

	/* Disconnect handler */
	priv->modal_handler_uids = 
		modest_signal_mgr_disconnect (priv->modal_handler_uids, 
					      G_OBJECT (widget),
					      GTK_IS_DIALOG (widget) ? 
					      "response" : 
					      "destroy-event");

	/* Schedule the next one for being shown */
	g_idle_add (idle_top_modal, self);
}

static gboolean
on_modal_window_close (GtkWidget *widget,
		       GdkEvent *event,
		       gpointer user_data)
{
	ModestWindowMgr *self = MODEST_WINDOW_MGR (user_data);

	/* Remove modal window from queue */
	remove_modal_from_queue (widget, self);

	/* Continue */
	return FALSE;
}

static void
on_modal_dialog_close (GtkDialog *dialog,
		       gint arg1,
		       gpointer user_data)
{
	ModestWindowMgr *self = MODEST_WINDOW_MGR (user_data);

	/* Remove modal window from queue */
	remove_modal_from_queue (GTK_WIDGET (dialog), self);
}

gint 
modest_window_mgr_num_windows (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	gint num_windows = 0;

	g_return_val_if_fail (self && MODEST_IS_WINDOW_MGR(self), -1);
	
	priv =  MODEST_WINDOW_MGR_GET_PRIVATE(self);
	
	if (priv->window_list)
		num_windows = g_list_length (priv->window_list);

	return num_windows + priv->banner_counter;
}

GtkWidget *   
modest_window_mgr_get_msg_edit_window (ModestWindowMgr *self)
{
	GtkWidget *result;
	ModestWindowMgrPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_WINDOW_MGR(self), NULL);

	priv = MODEST_WINDOW_MGR_GET_PRIVATE(self);
		
	if (priv->cached_editor) {
		result = priv->cached_editor;
		priv->cached_editor = NULL;
		load_new_editor (self);
	} else {
		result = g_object_new (MODEST_TYPE_MSG_EDIT_WINDOW, NULL);
	}

	return result;
}

GtkWidget *   
modest_window_mgr_get_msg_view_window (ModestWindowMgr *self)
{
	GtkWidget *result;
	ModestWindowMgrPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_WINDOW_MGR(self), NULL);
	
	priv = MODEST_WINDOW_MGR_GET_PRIVATE(self);

	if (priv->cached_view) {
		result = priv->cached_view;
		priv->cached_view = NULL;
		load_new_view (self);
	} else {
		result = g_object_new (MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	}

	return result;
}

void           
modest_window_mgr_register_banner (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	priv->banner_counter++;
	g_message ("REGISTER BANNER -> %d", priv->banner_counter);
	
}

void           
modest_window_mgr_unregister_banner (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	priv->banner_counter--;
	g_message ("UNREGISTER BANNER -> %d", priv->banner_counter);
	if (priv->window_list == NULL && priv->banner_counter == 0) {
		g_signal_emit (self, signals[WINDOW_LIST_EMPTY_SIGNAL], 0);
		g_message ("WINDOW LIST EMPTY");
	}

}

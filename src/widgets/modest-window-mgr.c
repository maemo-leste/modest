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
#include "modest-defs.h"
#include "widgets/modest-main-window.h"
#include "widgets/modest-msg-edit-window.h"
#include "widgets/modest-msg-view-window.h"
#include "modest-debug.h"
#include <tny-simple-list.h>


/* 'private'/'protected' functions */
static void modest_window_mgr_class_init (ModestWindowMgrClass *klass);
static void modest_window_mgr_init       (ModestWindowMgr *obj);
static void modest_window_mgr_finalize   (GObject *obj);

static gboolean modest_window_mgr_register_window_default (ModestWindowMgr *self, 
							   ModestWindow *window,
							   ModestWindow *parent);
static void modest_window_mgr_unregister_window_default (ModestWindowMgr *self, 
							 ModestWindow *window);
static void modest_window_mgr_set_fullscreen_mode_default (ModestWindowMgr *self,
							   gboolean on);
static gboolean modest_window_mgr_get_fullscreen_mode_default (ModestWindowMgr *self);
static void modest_window_mgr_show_toolbars_default (ModestWindowMgr *self,
						     GType window_type,
						     gboolean show_toolbars,
						     gboolean fullscreen);
#ifndef MODEST_TOOLKIT_HILDON2
static ModestWindow* modest_window_mgr_get_main_window_default (ModestWindowMgr *self, gboolean show);
#endif
static GtkWindow *modest_window_mgr_get_modal_default (ModestWindowMgr *self);
static void modest_window_mgr_set_modal_default (ModestWindowMgr *self, 
						 GtkWindow *window,
						 GtkWindow *parent);
static gboolean modest_window_mgr_close_all_windows_default (ModestWindowMgr *self);
static gboolean modest_window_mgr_find_registered_header_default (ModestWindowMgr *self, 
								  TnyHeader *header,
								  ModestWindow **win);
static gboolean modest_window_mgr_find_registered_message_uid_default (ModestWindowMgr *self, 
								       const gchar *msg_uid,
								       ModestWindow **win);
static GList *modest_window_mgr_get_window_list_default (ModestWindowMgr *self);
static ModestWindow *modest_window_mgr_show_initial_window_default (ModestWindowMgr *self);
static ModestWindow *modest_window_mgr_get_current_top_default (ModestWindowMgr *self);
static gboolean modest_window_mgr_screen_is_on_default (ModestWindowMgr *self);
static ModestWindow *modest_window_mgr_get_folder_window_default (ModestWindowMgr *self);
static void modest_window_mgr_create_caches_default (ModestWindowMgr *self);
static void modest_window_mgr_on_queue_changed (ModestMailOperationQueue *queue,
						ModestMailOperation *mail_op,
						ModestMailOperationQueueNotification type,
						ModestWindowMgr *self);
static void on_mail_operation_started (ModestMailOperation *mail_op,
				       gpointer user_data);
static void on_mail_operation_finished (ModestMailOperation *mail_op,
					gpointer user_data);
static gboolean modest_window_mgr_close_all_but_initial_default (ModestWindowMgr *self);

/* list my signals  */
enum {
	WINDOW_LIST_EMPTY_SIGNAL,
	PROGRESS_LIST_CHANGED_SIGNAL,
	NUM_SIGNALS
};

typedef struct _ModestWindowMgrPrivate ModestWindowMgrPrivate;
struct _ModestWindowMgrPrivate {
	guint         banner_counter;

#ifndef MODEST_TOOLKIT_HILDON2
	ModestWindow *main_window;
#endif
	GSList       *windows_that_prevent_hibernation;
	GSList       *preregistered_uids;

	guint        closing_time;

	GtkWidget    *cached_view;
	GtkWidget    *cached_editor;
	guint        idle_load_view_id;
	guint        idle_load_editor_id;

	guint        queue_change_handler;
	TnyList      *progress_operations;
	GSList       *sighandlers;
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
	ModestWindowMgrClass *mgr_class;

	gobject_class = (GObjectClass*) klass;
	mgr_class = (ModestWindowMgrClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_window_mgr_finalize;
	mgr_class->register_window = modest_window_mgr_register_window_default;
	mgr_class->unregister_window = modest_window_mgr_unregister_window_default;
	mgr_class->set_fullscreen_mode = modest_window_mgr_set_fullscreen_mode_default;
	mgr_class->get_fullscreen_mode = modest_window_mgr_get_fullscreen_mode_default;
	mgr_class->show_toolbars = modest_window_mgr_show_toolbars_default;
#ifndef MODEST_TOOLKIT_HILDON2
	mgr_class->get_main_window = modest_window_mgr_get_main_window_default;
#endif
	mgr_class->get_modal = modest_window_mgr_get_modal_default;
	mgr_class->set_modal = modest_window_mgr_set_modal_default;
	mgr_class->close_all_windows = modest_window_mgr_close_all_windows_default;
	mgr_class->find_registered_header = modest_window_mgr_find_registered_header_default;
	mgr_class->find_registered_message_uid = modest_window_mgr_find_registered_message_uid_default;
	mgr_class->get_window_list = modest_window_mgr_get_window_list_default;
	mgr_class->show_initial_window = modest_window_mgr_show_initial_window_default;
	mgr_class->get_current_top = modest_window_mgr_get_current_top_default;
	mgr_class->screen_is_on = modest_window_mgr_screen_is_on_default;
	mgr_class->create_caches = modest_window_mgr_create_caches_default;
	mgr_class->close_all_but_initial = modest_window_mgr_close_all_but_initial_default;
	mgr_class->get_folder_window = modest_window_mgr_get_folder_window_default;

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

	/**
	 * ModestWindowMgr::progress-list-changed
	 * @self: the #ModestWindowMgr that emits the signal
	 * @user_data: user data set when the signal handler was connected
	 *
	 * Issued whenever the progress mail operations list becomes changed
	 */
	signals[PROGRESS_LIST_CHANGED_SIGNAL] =
		g_signal_new ("progress-list-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestWindowMgrClass, progress_list_changed),
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
		priv->idle_load_view_id = g_timeout_add (2500, (GSourceFunc) idle_load_view, self);
}

static void
load_new_editor (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	if ((priv->cached_editor == NULL) && (priv->idle_load_editor_id == 0))
		priv->idle_load_editor_id = g_timeout_add (5000, (GSourceFunc) idle_load_editor, self);
}

static void
modest_window_mgr_init (ModestWindowMgr *obj)
{
	ModestWindowMgrPrivate *priv;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE(obj);
	priv->banner_counter = 0;
#ifndef MODEST_TOOLKIT_HILDON2
	priv->main_window = NULL;
#endif
	priv->preregistered_uids = NULL;

	priv->closing_time = 0;

	priv->cached_view = NULL;
	priv->cached_editor = NULL;

	priv->windows_that_prevent_hibernation = NULL;

	priv->queue_change_handler = 0;
	priv->progress_operations = TNY_LIST (tny_simple_list_new ());
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

	if (priv->windows_that_prevent_hibernation) {
		g_slist_free (priv->windows_that_prevent_hibernation);
		priv->cached_editor = NULL;
	}

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;

	if (priv->queue_change_handler > 0) {
		g_signal_handler_disconnect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
					     priv->queue_change_handler);
		priv->queue_change_handler = 0;
	}

	if (priv->progress_operations) {
		g_object_unref (priv->progress_operations);
		priv->progress_operations = NULL;
	}

	g_slist_foreach (priv->preregistered_uids, (GFunc)g_free, NULL);
	g_slist_free (priv->preregistered_uids);

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
		return list;
	
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
	if (alt_uid != NULL) {
		uid = g_strdup (alt_uid);
	} else {
		uid = modest_tny_folder_get_header_unique_id (header);
	}

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
	/* we don't need 'self', but for API consistency... */
	g_return_val_if_fail (self && MODEST_IS_WINDOW_MGR (self), NULL);
	g_return_val_if_fail (win, NULL);

	return g_object_get_data (G_OBJECT(win), MODEST_WINDOW_HELP_ID_PARAM);
}

gboolean
modest_window_mgr_close_all_windows (ModestWindowMgr *self)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->close_all_windows (self);
}

static gboolean
modest_window_mgr_close_all_windows_default (ModestWindowMgr *self)
{
	return TRUE;
}


gboolean
modest_window_mgr_find_registered_header (ModestWindowMgr *self, TnyHeader *header,
						  ModestWindow **win)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->find_registered_header (self, header, win);
}

static gboolean
modest_window_mgr_find_registered_header_default (ModestWindowMgr *self, TnyHeader *header,
						  ModestWindow **win)
{
	gchar* uid = NULL;

	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (TNY_IS_HEADER(header), FALSE);

	uid = modest_tny_folder_get_header_unique_id (header);

	if (uid)
		return modest_window_mgr_find_registered_message_uid (self, uid, win);
	else
		return FALSE;
}

gboolean
modest_window_mgr_find_registered_message_uid (ModestWindowMgr *self, const gchar *msg_uid,
					       ModestWindow **win)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->find_registered_message_uid (self, msg_uid, win);
}

static gboolean
modest_window_mgr_find_registered_message_uid_default (ModestWindowMgr *self, const gchar *msg_uid,
						       ModestWindow **win)
{
	ModestWindowMgrPrivate *priv = NULL;
	gchar* uid = NULL;
	gboolean has_header = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (msg_uid && msg_uid[0] != '\0', FALSE);

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	if (win)
		*win = NULL;

	has_header = has_uid (priv->preregistered_uids, msg_uid);
	g_free (uid);

	return has_header;
}

GList *
modest_window_mgr_get_window_list (ModestWindowMgr *self)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->get_window_list (self);
}

static GList *
modest_window_mgr_get_window_list_default (ModestWindowMgr *self)
{
	return NULL;
}

gboolean
modest_window_mgr_register_window (ModestWindowMgr *self, 
				   ModestWindow *window,
				   ModestWindow *parent)
{
	/* If this is the first registered window then reset the
	   status of the TnyDevice as it might be forced to be offline
	   when modest is running in the background (see
	   modest_tny_account_store_new()) */
	if (modest_window_mgr_get_num_windows (self) == 0) {
		if (tny_device_is_forced (modest_runtime_get_device ()))
			tny_device_reset (modest_runtime_get_device ());
	}

	return MODEST_WINDOW_MGR_GET_CLASS (self)->register_window (self, window, parent);
}

static gboolean
modest_window_mgr_register_window_default (ModestWindowMgr *self, 
					   ModestWindow *window,
					   ModestWindow *parent)
{
	ModestWindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (GTK_IS_WINDOW (window), FALSE);

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	/* We set up the queue change handler */
	if (priv->queue_change_handler == 0) {
		priv->queue_change_handler = g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
							       "queue-changed",
							       G_CALLBACK (modest_window_mgr_on_queue_changed),
							       self);
	}

#ifndef MODEST_TOOLKIT_HILDON2
	/* Check that it's not a second main window */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		if (priv->main_window) {
			g_warning ("%s: trying to register a second main window",
				   __FUNCTION__);
			return FALSE;
		} else {
			priv->main_window = window;
		}
	}
#endif

	/* remove from the list of pre-registered uids */
	if (MODEST_IS_MSG_VIEW_WINDOW(window)) {
		const gchar *uid = modest_msg_view_window_get_message_uid
			(MODEST_MSG_VIEW_WINDOW (window));

		MODEST_DEBUG_BLOCK(g_debug ("registering window for %s", uid ? uid : "<none>"););

		if (has_uid (priv->preregistered_uids, uid)) {
			priv->preregistered_uids = 
				remove_uid (priv->preregistered_uids,
					    modest_msg_view_window_get_message_uid
					    (MODEST_MSG_VIEW_WINDOW (window)));
		}
	} else if (MODEST_IS_MSG_EDIT_WINDOW(window)) {
		const gchar *uid = modest_msg_edit_window_get_message_uid
			(MODEST_MSG_EDIT_WINDOW (window));

		MODEST_DEBUG_BLOCK(g_debug ("registering window for %s", uid););

		priv->preregistered_uids = 
			remove_uid (priv->preregistered_uids,
				    modest_msg_edit_window_get_message_uid
				    (MODEST_MSG_EDIT_WINDOW (window)));
	}

	return TRUE;
}

void 
modest_window_mgr_unregister_window (ModestWindowMgr *self, 
				     ModestWindow *window)
{
	MODEST_WINDOW_MGR_GET_CLASS (self)->unregister_window (self, window);
}

static void 
modest_window_mgr_unregister_window_default (ModestWindowMgr *self, 
					     ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Save state */
	modest_window_save_state (window);

	/* Disconnect all the window signals */
	modest_window_disconnect_signals (window);
	
	/* Destroy the window */
	gtk_widget_destroy (GTK_WIDGET (window));
}



void
modest_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
				       gboolean on)
{
	MODEST_WINDOW_MGR_GET_CLASS (self)->set_fullscreen_mode (self, on);
}

static void
modest_window_mgr_set_fullscreen_mode_default (ModestWindowMgr *self,
					       gboolean on)
{
	return;
}

gboolean
modest_window_mgr_get_fullscreen_mode (ModestWindowMgr *self)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->get_fullscreen_mode (self);
}

static gboolean
modest_window_mgr_get_fullscreen_mode_default (ModestWindowMgr *self)
{
	return FALSE;
}

void
modest_window_mgr_show_toolbars (ModestWindowMgr *self,
				 GType window_type,
				 gboolean show_toolbars,
				 gboolean fullscreen)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->show_toolbars (self, window_type, show_toolbars, fullscreen);
}

static void
modest_window_mgr_show_toolbars_default (ModestWindowMgr *self,
					 GType window_type,
					 gboolean show_toolbars,
					 gboolean fullscreen)
{
	return;
}

#ifndef MODEST_TOOLKIT_HILDON2
void
modest_window_mgr_set_main_window (ModestWindowMgr *self, ModestWindow *win)
{
	ModestWindowMgrPrivate *priv;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	priv->main_window = win;
}

ModestWindow*
modest_window_mgr_get_main_window (ModestWindowMgr *self, gboolean show)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->get_main_window (self, show);
}

static ModestWindow*
modest_window_mgr_get_main_window_default (ModestWindowMgr *self, gboolean show)
{
	ModestWindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), NULL);

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	if (priv->main_window)
		return priv->main_window;

	if (show)
		return modest_main_window_new ();
	else return NULL;
}


gboolean
modest_window_mgr_main_window_exists  (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), FALSE);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return priv->main_window != NULL;
}
#endif

GtkWindow *
modest_window_mgr_get_modal (ModestWindowMgr *self)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->get_modal (self);
}

static GtkWindow *
modest_window_mgr_get_modal_default (ModestWindowMgr *self)
{
	return NULL;
}


void
modest_window_mgr_set_modal (ModestWindowMgr *self, 
			     GtkWindow *window,
			     GtkWindow *parent)
{
	MODEST_WINDOW_MGR_GET_CLASS (self)->set_modal (self, window, parent);
}

static void
modest_window_mgr_set_modal_default (ModestWindowMgr *self, 
				     GtkWindow *window,
				     GtkWindow *parent)
{
	return;
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
	GList *window_list;
	GList *node;
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	
	/* Iterate over all windows */
	window_list = modest_window_mgr_get_window_list (self);
	node = window_list;
	while (node) {
		ModestWindow *window = MODEST_WINDOW (node->data);
		if (window) {
			/* This calls the vfunc, 
			 * so each window can do its own thing: */
			modest_window_save_state (window);
		}

		node = g_list_next (node);
	}
	g_list_free (window_list);
}

guint
modest_window_mgr_get_num_windows (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	gint num_windows = 0;
	GList *window_list;

	g_return_val_if_fail (self && MODEST_IS_WINDOW_MGR(self), -1);

	priv =  MODEST_WINDOW_MGR_GET_PRIVATE(self);

	window_list = modest_window_mgr_get_window_list (self);

	if (window_list) {
		num_windows = g_list_length (window_list);
		g_list_free (window_list);
	}

#ifndef MODEST_TOOLKIT_HILDON2
	/* Do not take into account the main window if it was hidden */
	if (num_windows && priv->main_window && !GTK_WIDGET_VISIBLE (priv->main_window))
		num_windows--;
#endif

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
}

void
modest_window_mgr_unregister_banner (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	priv->banner_counter--;
	if (modest_window_mgr_get_num_windows (self) == 0)
		g_signal_emit (self, signals[WINDOW_LIST_EMPTY_SIGNAL], 0);
}

ModestWindow *
modest_window_mgr_show_initial_window (ModestWindowMgr *self)
{
	ModestWindow *window = NULL;

	/* Call the children */
	window = MODEST_WINDOW_MGR_GET_CLASS (self)->show_initial_window (self);

	if (window) {
		ModestAccountMgr *mgr;

		/* Show the initial window */
		gtk_widget_show (GTK_WIDGET (window));

		/* If there are no accounts then show the account wizard */
		mgr = modest_runtime_get_account_mgr();
		if (!modest_account_mgr_has_accounts (mgr, TRUE)) {
			if (!modest_ui_actions_run_account_setup_wizard (window)) {
				g_debug ("%s: couldn't show account setup wizard", __FUNCTION__);
			}
		}
	}

	return window;
}

static ModestWindow *
modest_window_mgr_show_initial_window_default (ModestWindowMgr *self)
{
#ifndef MODEST_TOOLKIT_HILDON2
	/* By default it returns the main window creating it if
	   needed */
	return modest_window_mgr_get_main_window (self, TRUE);
#else
	return NULL;
#endif
}


ModestWindow *
modest_window_mgr_get_current_top (ModestWindowMgr *self) 
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->get_current_top (self);
}


static ModestWindow *
modest_window_mgr_get_current_top_default (ModestWindowMgr *self)
{
	g_return_val_if_reached (NULL);
}

gboolean
modest_window_mgr_screen_is_on (ModestWindowMgr *self)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->screen_is_on (self);
}

static gboolean
modest_window_mgr_screen_is_on_default (ModestWindowMgr *self)
{
	/* Default implementation is assuming screen is always on */

	return TRUE;
}

void
modest_window_mgr_create_caches (ModestWindowMgr *mgr)
{
	MODEST_WINDOW_MGR_GET_CLASS (mgr)->create_caches (mgr);
}

static void
modest_window_mgr_create_caches_default (ModestWindowMgr *self)
{
	load_new_editor (self);
	load_new_view (self);
}

static gboolean
tny_list_find (TnyList *list, GObject *item)
{
	TnyIterator *iterator;
	gboolean found = FALSE;

	for (iterator = tny_list_create_iterator (list);
	     !tny_iterator_is_done (iterator) && !found;
	     tny_iterator_next (iterator)) {
		GObject *current = tny_iterator_get_current (iterator);
		if (current == item)
			found = TRUE;
		g_object_unref (current);
	}
	g_object_unref (iterator);
	
	return found;
}

static void
modest_window_mgr_on_queue_changed (ModestMailOperationQueue *queue,
				    ModestMailOperation *mail_op,
				    ModestMailOperationQueueNotification type,
				    ModestWindowMgr *self)
{	
	ModestWindowMgrPrivate *priv;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	/* We register to track progress */
	if (type == MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED) {
		priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
							       G_OBJECT (mail_op),
							       "operation-started",
							       G_CALLBACK (on_mail_operation_started),
							       self);
		priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
							       G_OBJECT (mail_op),
							       "operation-finished",
							       G_CALLBACK (on_mail_operation_finished),
							       self);
	} else if (type == MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED) {
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers,
								  G_OBJECT (mail_op),
								  "operation-started");
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers,
								  G_OBJECT (mail_op),
								  "operation-finished");
		if (tny_list_find  (priv->progress_operations, G_OBJECT (mail_op))) {
			tny_list_remove (priv->progress_operations, G_OBJECT (mail_op));
			g_signal_emit (self, signals[PROGRESS_LIST_CHANGED_SIGNAL], 0);
		}
	}
}

static void 
on_mail_operation_started (ModestMailOperation *mail_op,
			   gpointer user_data)
{
	ModestWindowMgr *self;
	ModestWindowMgrPrivate *priv;
	ModestMailOperationTypeOperation op_type;

	self = MODEST_WINDOW_MGR (user_data);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	/* First we check if the operation is a send receive operation,
	 * If now, we don't handle this */
	op_type = modest_mail_operation_get_type_operation (mail_op);
	if (op_type != MODEST_MAIL_OPERATION_TYPE_SEND &&
	    op_type != MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE) {
		return;
	}

	if (!tny_list_find (priv->progress_operations, G_OBJECT (mail_op))) {
		tny_list_prepend (priv->progress_operations, G_OBJECT (mail_op));
		g_signal_emit (self, signals[PROGRESS_LIST_CHANGED_SIGNAL], 0);
	}
}

static void 
on_mail_operation_finished (ModestMailOperation *mail_op,
			    gpointer user_data)
{
	ModestWindowMgr *self;
	ModestWindowMgrPrivate *priv;

	self = MODEST_WINDOW_MGR (user_data);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	if (tny_list_find (priv->progress_operations, G_OBJECT (mail_op))) {
		tny_list_remove (priv->progress_operations, G_OBJECT (mail_op));
		g_signal_emit (self, signals[PROGRESS_LIST_CHANGED_SIGNAL], 0);
	}
}

TnyList *
modest_window_mgr_get_progress_operations (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return tny_list_copy (priv->progress_operations);
}

gboolean 
modest_window_mgr_has_progress_operation (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return tny_list_get_length (priv->progress_operations) > 0;
}

gboolean 
modest_window_mgr_has_progress_operation_on_account (ModestWindowMgr *self,
						     const gchar *account_name)
{
	ModestWindowMgrPrivate *priv;
	gint account_ops = 0;
	TnyIterator *iterator;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	if (account_name == NULL)
		return 0;

	for (iterator = tny_list_create_iterator (priv->progress_operations);
	     !tny_iterator_is_done (iterator);
	     tny_iterator_next (iterator)) {
		ModestMailOperation *mail_op; 
		TnyAccount *account;

		mail_op= MODEST_MAIL_OPERATION (tny_iterator_get_current (iterator));
		account = modest_mail_operation_get_account (mail_op);

		if (account != NULL) {
			const gchar *current_name;

			current_name = tny_account_get_id (account);
			if (current_name && strcmp (current_name, account_name) == 0)
				account_ops ++;
			g_object_unref (account);
		}

		g_object_unref (mail_op);
	}
	g_object_unref (iterator);

	return account_ops;
}

/* 'Protected method' must be only called by children */
gboolean
_modest_window_mgr_close_active_modals (ModestWindowMgr *self)
{
	GtkWidget *modal;

	/* Exit if there are no windows */
	if (!modest_window_mgr_get_num_windows (self)) {
		g_warning ("%s: there are no windows to close", __FUNCTION__);
		return FALSE;
	}

	/* Check that there is no active modal dialog */
	modal = (GtkWidget *) modest_window_mgr_get_modal (self);
	while (modal && GTK_IS_DIALOG (modal)) {
		GtkWidget *parent;

#if defined(MODEST_TOOLKIT_HILDON2) || defined(MODEST_TOOLKIT_HILDON)
#include <hildon/hildon.h>
		/* If it's a hildon note then don't try to close it as
		   this is the default behaviour of WM, delete event
		   is not issued for this kind of notes as we want the
		   user to always click on a button */
		if (HILDON_IS_NOTE (modal)) {
			gtk_window_present (GTK_WINDOW (modal));
			return FALSE;
		}
#endif

		/* Get the parent */
		parent = (GtkWidget *) gtk_window_get_transient_for (GTK_WINDOW (modal));

		/* Try to close it */
		gtk_dialog_response (GTK_DIALOG (modal), GTK_RESPONSE_DELETE_EVENT);

		/* Maybe the dialog was not closed, because a close
		   confirmation dialog for example. Then ignore the
		   register process */
		if (GTK_IS_WINDOW (modal)) {
			gtk_window_present (GTK_WINDOW (modal));
			return FALSE;
		}

		/* Get next modal */
		modal = parent;
	}
	return TRUE;
}

gboolean
modest_window_mgr_close_all_but_initial (ModestWindowMgr *self)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->close_all_but_initial (self);
}

static gboolean
modest_window_mgr_close_all_but_initial_default (ModestWindowMgr *self)
{
	/* Empty default implementation */
	return FALSE;
}

ModestWindow *
modest_window_mgr_get_folder_window (ModestWindowMgr *self)
{
	return MODEST_WINDOW_MGR_GET_CLASS (self)->get_folder_window (self);
}

static ModestWindow *
modest_window_mgr_get_folder_window_default (ModestWindowMgr *self)
{
	/* Default implementation returns NULL always */

	return NULL;
}


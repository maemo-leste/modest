/* Copyright (c) 2008, Nokia Corporation
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
#include "modest-hildon1-window-mgr.h"
#include "modest-msg-edit-window.h"
#include "modest-main-window.h"
#include "modest-conf.h"
#include "modest-defs.h"
#include "modest-signal-mgr.h"
#include "modest-runtime.h"
#include "modest-platform.h"
#include "modest-ui-actions.h"
#include "modest-debug.h"
#include "modest-tny-folder.h"

/* 'private'/'protected' functions */
static void modest_hildon1_window_mgr_class_init (ModestHildon1WindowMgrClass *klass);
static void modest_hildon1_window_mgr_instance_init (ModestHildon1WindowMgr *obj);
static void modest_hildon1_window_mgr_finalize   (GObject *obj);

static gboolean on_window_destroy        (ModestWindow *window,
					  GdkEvent *event,
					  ModestHildon1WindowMgr *self);

static gboolean on_modal_window_close    (GtkWidget *widget,
					  GdkEvent *event,
					  gpointer user_data);

static void on_modal_dialog_destroy      (GtkObject *object,
					  gpointer   user_data);

static void     on_modal_dialog_close    (GtkDialog *dialog,
					  gint arg1,
					  gpointer user_data);

static const gchar* get_show_toolbar_key (GType window_type,
					  gboolean fullscreen);

static gboolean modest_hildon1_window_mgr_register_window (ModestWindowMgr *self, 
							   ModestWindow *window,
							   ModestWindow *parent);
static void modest_hildon1_window_mgr_unregister_window (ModestWindowMgr *self, 
							 ModestWindow *window);
static void modest_hildon1_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
							   gboolean on);
static gboolean modest_hildon1_window_mgr_get_fullscreen_mode (ModestWindowMgr *self);
static void modest_hildon1_window_mgr_show_toolbars (ModestWindowMgr *self,
						     GType window_type,
						     gboolean show_toolbars,
						     gboolean fullscreen);
static ModestWindow* modest_hildon1_window_mgr_get_main_window (ModestWindowMgr *self, gboolean show);
static GtkWindow *modest_hildon1_window_mgr_get_modal (ModestWindowMgr *self);
static void modest_hildon1_window_mgr_set_modal (ModestWindowMgr *self, 
						 GtkWindow *window,
						 GtkWindow *parent);
static gboolean modest_hildon1_window_mgr_find_registered_header (ModestWindowMgr *self, 
								  TnyHeader *header,
								  ModestWindow **win);
static GList *modest_hildon1_window_mgr_get_window_list (ModestWindowMgr *self);
static gboolean modest_hildon1_window_mgr_close_all_windows (ModestWindowMgr *self);

typedef struct _ModestHildon1WindowMgrPrivate ModestHildon1WindowMgrPrivate;
struct _ModestHildon1WindowMgrPrivate {
	GList        *window_list;
	GMutex       *queue_lock;
	GQueue       *modal_windows;
	
	gboolean     fullscreen_mode;
	
	GHashTable   *destroy_handlers;
	GHashTable   *viewer_handlers;
	GSList       *window_state_uids;
	
	guint        closing_time;

	GSList       *modal_handler_uids;
	ModestWindow *current_top;
};
#define MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										   MODEST_TYPE_HILDON1_WINDOW_MGR, \
										   ModestHildon1WindowMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_hildon1_window_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestHildon1WindowMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_hildon1_window_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestHildon1WindowMgr),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_hildon1_window_mgr_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW_MGR,
		                                  "ModestHildon1WindowMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_hildon1_window_mgr_class_init (ModestHildon1WindowMgrClass *klass)
{
	GObjectClass *gobject_class;
	ModestWindowMgrClass *mgr_class;

	gobject_class = (GObjectClass*) klass;
	mgr_class = (ModestWindowMgrClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_hildon1_window_mgr_finalize;
	mgr_class->register_window = modest_hildon1_window_mgr_register_window;
	mgr_class->unregister_window = modest_hildon1_window_mgr_unregister_window;
	mgr_class->set_fullscreen_mode = modest_hildon1_window_mgr_set_fullscreen_mode;
	mgr_class->get_fullscreen_mode = modest_hildon1_window_mgr_get_fullscreen_mode;
	mgr_class->show_toolbars = modest_hildon1_window_mgr_show_toolbars;
	mgr_class->get_main_window = modest_hildon1_window_mgr_get_main_window;
	mgr_class->get_modal = modest_hildon1_window_mgr_get_modal;
	mgr_class->set_modal = modest_hildon1_window_mgr_set_modal;
	mgr_class->find_registered_header = modest_hildon1_window_mgr_find_registered_header;
	mgr_class->get_window_list = modest_hildon1_window_mgr_get_window_list;
	mgr_class->close_all_windows = modest_hildon1_window_mgr_close_all_windows;

	g_type_class_add_private (gobject_class, sizeof(ModestHildon1WindowMgrPrivate));

}

static void
modest_hildon1_window_mgr_instance_init (ModestHildon1WindowMgr *obj)
{
	ModestHildon1WindowMgrPrivate *priv;

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE(obj);
	priv->window_list = NULL;
	priv->fullscreen_mode = FALSE;
	priv->current_top = NULL;
	priv->window_state_uids = NULL;

	priv->modal_windows = g_queue_new ();
	priv->queue_lock = g_mutex_new ();
	
	/* Could not initialize it from gconf, singletons are not
	   ready yet */
	priv->destroy_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);	
	priv->viewer_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

	priv->closing_time = 0;

	priv->modal_handler_uids = NULL;
}

static void
modest_hildon1_window_mgr_finalize (GObject *obj)
{
	ModestHildon1WindowMgrPrivate *priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE(obj);

	modest_signal_mgr_disconnect_all_and_destroy (priv->window_state_uids);
	priv->window_state_uids = NULL;

	if (priv->window_list) {
		GList *iter = priv->window_list;
		/* unregister pending windows */
		while (iter) {
			ModestWindow *window = (ModestWindow *) iter->data;
			iter = g_list_next (iter);
			modest_window_mgr_unregister_window (MODEST_WINDOW_MGR (obj), window);
		}
		g_list_free (priv->window_list);
		priv->window_list = NULL;
	}

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
	priv->modal_handler_uids = NULL;

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
modest_hildon1_window_mgr_new (void)
{
	return MODEST_WINDOW_MGR(g_object_new(MODEST_TYPE_HILDON1_WINDOW_MGR, NULL));
}

static gboolean
modest_hildon1_window_mgr_close_all_windows (ModestWindowMgr *self)
{
	ModestHildon1WindowMgrPrivate *priv = NULL;
	gboolean ret_value = FALSE;
	ModestWindow *main_window;
	
	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self), FALSE);
	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);
	
	/* If there is a main window then try to close it, and it will
	   close the others if needed */
	main_window = modest_window_mgr_get_main_window (self, FALSE);
	if (main_window) {
		g_signal_emit_by_name (main_window, "delete-event", NULL, &ret_value);
		return ret_value;
	} else {
		GList *wins = NULL, *next = NULL;

		/* delete-event handler actually removes window_list item, */
		wins = priv->window_list;
		while (wins) {
			next = g_list_next (wins);
			g_signal_emit_by_name (G_OBJECT (wins->data), "delete-event", NULL, &ret_value);
			if (ret_value)
				break;
			wins = next;
		}
	}

	return ret_value;
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

static gboolean
modest_hildon1_window_mgr_find_registered_header (ModestWindowMgr *self, TnyHeader *header,
						  ModestWindow **win)
{
	ModestHildon1WindowMgrPrivate *priv = NULL;
	gchar* uid = NULL;
	gboolean has_header, has_window = FALSE;
	GList *item = NULL;

	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (TNY_IS_HEADER(header), FALSE);
	
	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	has_header = MODEST_WINDOW_MGR_CLASS (parent_class)->find_registered_header (self, header, win);
	
	uid = modest_tny_folder_get_header_unique_id (header);
	
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

static GList *
modest_hildon1_window_mgr_get_window_list (ModestWindowMgr *self)
{
	ModestHildon1WindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self), NULL);
	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	return g_list_copy (priv->window_list);
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

#ifndef MODEST_TOOLKIT_GTK
static void
on_window_is_topmost (GObject    *gobject,
		      GParamSpec *arg1,
		      gpointer    user_data)
{
	ModestHildon1WindowMgr *self;
	ModestHildon1WindowMgrPrivate *priv;
	ModestWindow *win = (ModestWindow *) gobject;

	g_return_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (user_data));

	self = MODEST_HILDON1_WINDOW_MGR (user_data);
	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	if (hildon_window_get_is_topmost (HILDON_WINDOW (win)))
		priv->current_top = win;
}

#else
static gboolean
on_window_state_event (GtkWidget           *widget,
		       GdkEventWindowState *event,
		       gpointer             user_data)
{
	ModestHildon1WindowMgr *self;
	ModestHildon1WindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (user_data), FALSE);

	self = MODEST_HILDON1_WINDOW_MGR (user_data);
	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	MODEST_DEBUG_BLOCK (
			    if (event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN)
				    g_print ("GDK_WINDOW_STATE_WITHDRAWN\n");
			    if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
				    g_print ("GDK_WINDOW_STATE_ICONIFIED\n");
			    if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED)
				    g_print ("GDK_WINDOW_STATE_MAXIMIZED\n");
			    if (event->changed_mask & GDK_WINDOW_STATE_STICKY)
				    g_print ("GDK_WINDOW_STATE_STICKY\n");
			    if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
				    g_print ("GDK_WINDOW_STATE_FULLSCREEN\n");
			    if (event->changed_mask & GDK_WINDOW_STATE_ABOVE)
				    g_print ("GDK_WINDOW_STATE_ABOVE\n");
			    if (event->changed_mask & GDK_WINDOW_STATE_BELOW)
				    g_print ("GDK_WINDOW_STATE_BELOW\n");
			    );
	if (event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN ||
	    event->changed_mask & GDK_WINDOW_STATE_ABOVE) {
		priv->current_top = MODEST_WINDOW (widget);
	}
	
	return FALSE;
}
#endif

static gboolean
modest_hildon1_window_mgr_register_window (ModestWindowMgr *self, 
					   ModestWindow *window,
					   ModestWindow *parent)
{
	GList *win;
	ModestHildon1WindowMgrPrivate *priv;
	gint *handler_id;
	const gchar *key;
	ModestWindow *main_window;

	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (GTK_IS_WINDOW (window), FALSE);

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	win = g_list_find (priv->window_list, window);
	if (win) {
		/* this is for the case we want to register the window and it was already
		 * registered. We leave silently.
		 */
		return FALSE;
	}
	
	if (!MODEST_WINDOW_MGR_CLASS (parent_class)->register_window (self, window, parent))
		return FALSE;
	
	/* Add to list. Keep a reference to the window */
	g_object_ref (window);
	priv->window_list = g_list_prepend (priv->window_list, window);

	/* Listen to window state changes. Unfortunately
	   window-state-event does not properly work for the Maemo
	   version, so we need to use is-topmost and the ifdef */
#ifndef MODEST_TOOLKIT_GTK
	priv->window_state_uids = 
		modest_signal_mgr_connect (priv->window_state_uids, 
					   G_OBJECT (window), 
					   "notify::is-topmost",
					   G_CALLBACK (on_window_is_topmost), 
					   self);
#else
	priv->window_state_uids = 
		modest_signal_mgr_connect (priv->window_state_uids, 
					   G_OBJECT (window), 
					   "window-state-event",
					   G_CALLBACK (on_window_state_event), 
					   self);
#endif
	/* Listen to object destruction */
	handler_id = g_malloc0 (sizeof (gint));
	*handler_id = g_signal_connect (window, "delete-event", G_CALLBACK (on_window_destroy), self);
	g_hash_table_insert (priv->destroy_handlers, window, handler_id);

	main_window = modest_window_mgr_get_main_window (self, FALSE);
	/* If there is a msg view window, let the main window listen the msg-changed signal */
	if (MODEST_IS_MSG_VIEW_WINDOW(window) && main_window) {
		gulong *handler;
		handler = g_malloc0 (sizeof (gulong));
		*handler = g_signal_connect (window, "msg-changed", 
					     G_CALLBACK (modest_main_window_on_msg_view_window_msg_changed), 
					     main_window);
		g_hash_table_insert (priv->viewer_handlers, window, handler);
	}

	/* Put into fullscreen if needed */
	if (priv->fullscreen_mode)
		gtk_window_fullscreen (GTK_WINDOW (window));

	/* Show/hide toolbar & fullscreen */	
	key = get_show_toolbar_key (G_TYPE_FROM_INSTANCE (window), priv->fullscreen_mode);
	modest_window_show_toolbar (window, modest_conf_get_bool (modest_runtime_get_conf (), key, NULL));

	return TRUE;
}

static void
cancel_window_operations (ModestWindow *window)
{
	GSList* pending_ops = NULL;

	/* cancel open and receive operations */
	pending_ops = modest_mail_operation_queue_get_by_source (modest_runtime_get_mail_operation_queue (), 
								 G_OBJECT (window));
	while (pending_ops != NULL) {
		ModestMailOperationTypeOperation type;
		GSList* tmp_list = NULL;

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



static gboolean
on_window_destroy (ModestWindow *window, 
		   GdkEvent *event,
		   ModestHildon1WindowMgr *self)
{
	gint dialog_response = GTK_RESPONSE_NONE;
	gboolean no_propagate = FALSE;

	/* Specific stuff first */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		ModestHildon1WindowMgrPrivate *priv;
		priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

		/* If more than one window already opened */
		if (g_list_length (priv->window_list) > 1) {

			/* Present the window if it's not visible now */
			if (!gtk_window_has_toplevel_focus (GTK_WINDOW (window))) {
				gtk_window_present (GTK_WINDOW (window));
				priv->current_top = window;
			}
			/* Create the confirmation dialog MSG-NOT308 */
			dialog_response = modest_platform_run_confirmation_dialog (
					GTK_WINDOW (window), _("emev_nc_close_windows"));

			/* If the user wants to close all the windows */
			if ((dialog_response == GTK_RESPONSE_OK) 
			    || (dialog_response == GTK_RESPONSE_ACCEPT) 
			    || (dialog_response == GTK_RESPONSE_YES)) {
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
			} else {
				return TRUE;
			}
		}

		/* Do not unregister it, just hide */
		gtk_widget_hide_all (GTK_WIDGET (window));

		/* Cancel pending operations */
		cancel_window_operations (window);

		/* Fake the window system, make it think that there is no window */
		if (modest_window_mgr_num_windows (MODEST_WINDOW_MGR (self)) == 0)
			g_signal_emit_by_name (self, "window-list-empty");

		no_propagate = TRUE;
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
				if (response == GTK_RESPONSE_OK)
					if (!modest_ui_actions_on_save_to_drafts (NULL, MODEST_MSG_EDIT_WINDOW (window)))
						return TRUE;
			}
		}
		/* Unregister window */
		modest_window_mgr_unregister_window (MODEST_WINDOW_MGR (self), window);
		no_propagate = TRUE;
	}

	return no_propagate;
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

static void 
modest_hildon1_window_mgr_unregister_window (ModestWindowMgr *self, 
					     ModestWindow *window)
{
	GList *win;
	ModestHildon1WindowMgrPrivate *priv;
	gulong *tmp, handler_id;

	g_return_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self));
	g_return_if_fail (MODEST_IS_WINDOW (window));

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	win = g_list_find (priv->window_list, window);
	if (!win) {
		g_warning ("Trying to unregister a window that has not being registered yet");
		return;
	}

	/* If it's the main window unset it */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		modest_window_mgr_set_main_window (self, NULL);

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
	   HashTable could not exist if the main window was closed
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

	/* Remove from list & hash table */
	priv->window_list = g_list_remove_link (priv->window_list, win);
	tmp = g_hash_table_lookup (priv->destroy_handlers, window);
	handler_id = *tmp;

	g_hash_table_remove (priv->destroy_handlers, window);

	/* cancel open and receive operations */
	cancel_window_operations (window);

	/* Check if it's the topmost window, and remove the window from the stack.
	 * This is needed for the cases there's no other topmost window that will
	 * replace it in topmost handler.
	 */
	 if (window == priv->current_top)
		 priv->current_top = NULL;

	/* Disconnect the "window-state-event" handler, we won't need it anymore */
	if (priv->window_state_uids) {
#ifndef MODEST_TOOLKIT_GTK
		priv->window_state_uids = 
			modest_signal_mgr_disconnect (priv->window_state_uids, 
						      G_OBJECT (window), 
						      "notify::is-topmost");
#else
		priv->window_state_uids = 
			modest_signal_mgr_disconnect (priv->window_state_uids, 
						      G_OBJECT (window), 
						      "window-state-event");
#endif
	}
	
	/* Disconnect the "delete-event" handler, we won't need it anymore */
	g_signal_handler_disconnect (window, handler_id);

	/* Destroy the window */
	g_object_unref (win->data);
	g_list_free (win);
	
	MODEST_WINDOW_MGR_CLASS (parent_class)->unregister_window (self, window);

	/* If there are no more windows registered emit the signal */
	if (modest_window_mgr_num_windows (self) == 0)
		g_signal_emit_by_name (self, "window-list-empty");
}



static void
modest_hildon1_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
					       gboolean on)
{
	ModestHildon1WindowMgrPrivate *priv;
	GList *win = NULL;
	ModestConf *conf;

	g_return_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self));

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

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

static gboolean
modest_hildon1_window_mgr_get_fullscreen_mode (ModestWindowMgr *self)
{
	ModestHildon1WindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self), FALSE);

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	return priv->fullscreen_mode;
}

static void 
modest_hildon1_window_mgr_show_toolbars (ModestWindowMgr *self,
					 GType window_type,
					 gboolean show_toolbars,
					 gboolean fullscreen)
{
	ModestHildon1WindowMgrPrivate *priv;
	ModestConf *conf;
	const gchar *key = NULL;

	g_return_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self));

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);
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

static ModestWindow*  
modest_hildon1_window_mgr_get_main_window (ModestWindowMgr *self, gboolean show)
{
	ModestHildon1WindowMgrPrivate *priv;
	ModestWindow *result;
	
	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self), NULL);
	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);
	
	result = MODEST_WINDOW_MGR_CLASS (parent_class)->get_main_window (self, FALSE);
	/* create the main window, if it hasn't been created yet */
	if (!result && show) {
		/* modest_window_mgr_register_window will set priv->main_window */
		result = modest_main_window_new ();
		modest_window_mgr_register_window (self, result, NULL);
		gtk_widget_show_all (GTK_WIDGET (result));
		gtk_window_present (GTK_WINDOW (result));
		MODEST_DEBUG_BLOCK(
			g_debug ("%s: created main window: %p\n", __FUNCTION__, result);
		);
	}
	
	return result;
}


static GtkWindow *
modest_hildon1_window_mgr_get_modal (ModestWindowMgr *self)
{
	ModestHildon1WindowMgrPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self), NULL);
	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

	return g_queue_peek_head (priv->modal_windows);
}


static void
modest_hildon1_window_mgr_set_modal (ModestWindowMgr *self, 
				     GtkWindow *window,
				     GtkWindow *parent)
{
	GtkWindow *old_modal;
	ModestHildon1WindowMgrPrivate *priv;

	g_return_if_fail (MODEST_IS_HILDON1_WINDOW_MGR (self));
	g_return_if_fail (GTK_IS_WINDOW (window));

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);
	g_mutex_lock (priv->queue_lock);
	old_modal = g_queue_peek_head (priv->modal_windows);
	g_mutex_unlock (priv->queue_lock);

	if (!old_modal) {	
		/* make us transient wrt the main window then */
		if (priv->current_top && ((GtkWindow *) priv->current_top != window)) {
			gtk_window_set_transient_for (window, GTK_WINDOW(priv->current_top));
		} else {
			ModestWindow *main_win = modest_window_mgr_get_main_window (self, FALSE);
			if (GTK_WINDOW(main_win) != window) /* they should not be the same */
				gtk_window_set_transient_for (window, GTK_WINDOW(main_win));
		}
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

	if (GTK_IS_DIALOG (window)) {
		/* Note that response is not always enough because it
		   could be captured and removed easily by dialogs but
		   works for most of situations */
		priv->modal_handler_uids = 
			modest_signal_mgr_connect (priv->modal_handler_uids, 
						   G_OBJECT (window), 
						   "response",
						   G_CALLBACK (on_modal_dialog_close), 
						   self);
		/* We need this as well because dialogs are often
		   destroyed with gtk_widget_destroy and this one will
		   prevent response from happening */
		priv->modal_handler_uids = 
			modest_signal_mgr_connect (priv->modal_handler_uids, 
						   G_OBJECT (window), 
						   "destroy",
						   G_CALLBACK (on_modal_dialog_destroy),
						   self);
	} else {
		priv->modal_handler_uids = 
			modest_signal_mgr_connect (priv->modal_handler_uids, 
						   G_OBJECT (window), 
						   "delete-event",
						   G_CALLBACK (on_modal_window_close), 
						   self);
	}
	/* Destroy width parent */
	gtk_window_set_destroy_with_parent (window, TRUE);
}



static gboolean
idle_top_modal (gpointer data)
{
	ModestWindowMgr *self = MODEST_WINDOW_MGR (data);
	ModestHildon1WindowMgrPrivate *priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);
	GtkWindow *topmost;

	/* Get the top modal */
	g_mutex_lock (priv->queue_lock);
	topmost = (GtkWindow *) g_queue_peek_head (priv->modal_windows);
	g_mutex_unlock (priv->queue_lock);

	/* Show it */
	if (topmost) {
		gdk_threads_enter ();
		gtk_window_present (topmost);
		/* It seems that the window looses modality if some
		   other is shown on top of it after set_transient_for
		   and set_parent */
		gtk_window_set_modal (topmost, TRUE);
		gdk_threads_leave ();
	}

	return FALSE;
}

static void
remove_modal_from_queue (GtkWidget *widget,
			 ModestWindowMgr *self)
{
	ModestHildon1WindowMgrPrivate *priv;
	GList *item = NULL;

	priv = MODEST_HILDON1_WINDOW_MGR_GET_PRIVATE (self);

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
					      "delete-event");
	if (GTK_IS_DIALOG (widget))
		priv->modal_handler_uids =
			modest_signal_mgr_disconnect (priv->modal_handler_uids,
						      G_OBJECT (widget),
						      "destroy");

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

	/* Remove modal window from queue. Note that if "destroy"
	   signal was invoked before the response the window could be
	   already deleted */
	remove_modal_from_queue (GTK_WIDGET (dialog), self);
}

static void
on_modal_dialog_destroy (GtkObject *object,
			 gpointer   user_data)
{
	ModestWindowMgr *self = MODEST_WINDOW_MGR (user_data);

	/* Remove modal window from queue */
	remove_modal_from_queue (GTK_WIDGET (object), self);
}


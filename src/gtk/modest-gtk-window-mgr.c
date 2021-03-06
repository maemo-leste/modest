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
#include "modest-window.h"
#include "modest-gtk-window-mgr.h"
#include "modest-msg-edit-window.h"
#include "modest-mailboxes-window.h"
#include "modest-header-window.h"
#include "modest-window-mgr-priv.h"
#include "modest-conf.h"
#include "modest-defs.h"
#include "modest-signal-mgr.h"
#include "modest-runtime.h"
#include "modest-platform.h"
#include "modest-ui-actions.h"
#include "modest-debug.h"
#include "modest-tny-folder.h"
#include "modest-folder-window.h"
#include "modest-accounts-window.h"
#include "modest-utils.h"
#include "modest-tny-msg.h"
#include "modest-tny-account.h"
#include <modest-shell.h>
#include <tny-merge-folder.h>

/* 'private'/'protected' functions */
static void modest_gtk_window_mgr_class_init (ModestGtkWindowMgrClass *klass);
static void modest_gtk_window_mgr_instance_init (ModestGtkWindowMgr *obj);
static void modest_gtk_window_mgr_finalize   (GObject *obj);

static gboolean on_window_destroy        (ModestWindow *window,
					  GdkEvent *event,
					  ModestGtkWindowMgr *self);

static gboolean modest_gtk_window_mgr_register_window (ModestWindowMgr *self, 
						       ModestWindow *window,
						       ModestWindow *parent);
static void modest_gtk_window_mgr_unregister_window (ModestWindowMgr *self, 
						     ModestWindow *window);
static void modest_gtk_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
						       gboolean on);
static gboolean modest_gtk_window_mgr_get_fullscreen_mode (ModestWindowMgr *self);
static void modest_gtk_window_mgr_show_toolbars (ModestWindowMgr *self,
						 GType window_type,
						 gboolean show_toolbars,
						 gboolean fullscreen);
static GtkWindow *modest_gtk_window_mgr_get_modal (ModestWindowMgr *self);
static void modest_gtk_window_mgr_set_modal (ModestWindowMgr *self, 
					     GtkWindow *window,
					     GtkWindow *parent);
static gboolean modest_gtk_window_mgr_find_registered_header (ModestWindowMgr *self, 
							      TnyHeader *header,
							      ModestWindow **win);
static gboolean modest_gtk_window_mgr_find_registered_message_uid (ModestWindowMgr *self, 
								   const gchar *msg_uid,
								   ModestWindow **win);
static GList *modest_gtk_window_mgr_get_window_list (ModestWindowMgr *self);
static gboolean modest_gtk_window_mgr_close_all_windows (ModestWindowMgr *self);
static gboolean modest_gtk_window_mgr_close_all_but_initial (ModestWindowMgr *self);
static gboolean shell_has_modals (ModestShell *window);
static ModestWindow *modest_gtk_window_mgr_show_initial_window (ModestWindowMgr *self);
static ModestWindow *modest_gtk_window_mgr_get_current_top (ModestWindowMgr *self);
static gboolean modest_gtk_window_mgr_screen_is_on (ModestWindowMgr *self);
static void modest_gtk_window_mgr_create_caches (ModestWindowMgr *self);
static void on_account_removed (TnyAccountStore *acc_store, 
				TnyAccount *account,
				gpointer user_data);
static ModestWindow *modest_gtk_window_mgr_get_folder_window (ModestWindowMgr *self);

typedef struct _ModestGtkWindowMgrPrivate ModestGtkWindowMgrPrivate;
struct _ModestGtkWindowMgrPrivate {
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

	gulong        accounts_handler;
	GtkWidget    *shell;

	gboolean      fullscreen;
};
#define MODEST_GTK_WINDOW_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										   MODEST_TYPE_GTK_WINDOW_MGR, \
										   ModestGtkWindowMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_gtk_window_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestGtkWindowMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_gtk_window_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestGtkWindowMgr),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_gtk_window_mgr_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW_MGR,
		                                  "ModestGtkWindowMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_gtk_window_mgr_class_init (ModestGtkWindowMgrClass *klass)
{
	GObjectClass *gobject_class;
	ModestWindowMgrClass *mgr_class;

	gobject_class = (GObjectClass*) klass;
	mgr_class = (ModestWindowMgrClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_gtk_window_mgr_finalize;
	mgr_class->register_window = modest_gtk_window_mgr_register_window;
	mgr_class->unregister_window = modest_gtk_window_mgr_unregister_window;
	mgr_class->set_fullscreen_mode = modest_gtk_window_mgr_set_fullscreen_mode;
	mgr_class->get_fullscreen_mode = modest_gtk_window_mgr_get_fullscreen_mode;
	mgr_class->show_toolbars = modest_gtk_window_mgr_show_toolbars;
	mgr_class->get_modal = modest_gtk_window_mgr_get_modal;
	mgr_class->set_modal = modest_gtk_window_mgr_set_modal;
	mgr_class->find_registered_header = modest_gtk_window_mgr_find_registered_header;
	mgr_class->find_registered_message_uid = modest_gtk_window_mgr_find_registered_message_uid;
	mgr_class->get_window_list = modest_gtk_window_mgr_get_window_list;
	mgr_class->close_all_windows = modest_gtk_window_mgr_close_all_windows;
	mgr_class->close_all_but_initial = modest_gtk_window_mgr_close_all_but_initial;
	mgr_class->show_initial_window = modest_gtk_window_mgr_show_initial_window;
	mgr_class->get_current_top = modest_gtk_window_mgr_get_current_top;
	mgr_class->screen_is_on = modest_gtk_window_mgr_screen_is_on;
	mgr_class->create_caches = modest_gtk_window_mgr_create_caches;
	mgr_class->get_folder_window = modest_gtk_window_mgr_get_folder_window;

	g_type_class_add_private (gobject_class, sizeof(ModestGtkWindowMgrPrivate));

}

static void
modest_gtk_window_mgr_instance_init (ModestGtkWindowMgr *obj)
{
	ModestGtkWindowMgrPrivate *priv;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE(obj);
	priv->window_list = NULL;
	priv->fullscreen_mode = FALSE;
	priv->window_state_uids = NULL;

	priv->modal_windows = g_queue_new ();
	priv->queue_lock = g_mutex_new ();
	priv->fullscreen = FALSE;

	/* Could not initialize it from gconf, singletons are not
	   ready yet */
	priv->destroy_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
	priv->viewer_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

	priv->closing_time = 0;

	priv->modal_handler_uids = NULL;
	priv->shell = modest_shell_new ();
	gtk_widget_show (priv->shell);
}

static void
modest_gtk_window_mgr_finalize (GObject *obj)
{
	ModestGtkWindowMgrPrivate *priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE(obj);
	ModestTnyAccountStore *acc_store;

	modest_signal_mgr_disconnect_all_and_destroy (priv->window_state_uids);
	priv->window_state_uids = NULL;

	acc_store = modest_runtime_get_account_store ();
	if (acc_store && g_signal_handler_is_connected (acc_store, priv->accounts_handler))
		g_signal_handler_disconnect (acc_store, priv->accounts_handler);

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

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestWindowMgr*
modest_gtk_window_mgr_new (void)
{
	return MODEST_WINDOW_MGR(g_object_new(MODEST_TYPE_GTK_WINDOW_MGR, NULL));
}

static gboolean
modest_gtk_window_mgr_close_all_windows (ModestWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv = NULL;
	gboolean ret_value = FALSE;
	ModestWindow *window;
	gboolean failed = FALSE;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), FALSE);
	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	while ((window = modest_shell_peek_window (MODEST_SHELL (priv->shell))) != NULL) {
		ret_value = modest_shell_delete_window (MODEST_SHELL (priv->shell), window);
		if (ret_value == TRUE) {
			failed = TRUE;
			break;
		}
	}

	return !failed;
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

static gint
compare_headers (ModestWindow *win,
		 TnyHeader *header)
{
	TnyHeader *my_header;
	gint result = 1;

	if (!MODEST_IS_MSG_VIEW_WINDOW (win))
		return 1;

	/* Get message uid from msg window */
	my_header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));
	if (my_header) {
		if (my_header == header)
			result = 0;
		g_object_unref (my_header);
	}
	return result;
}


static gboolean
modest_gtk_window_mgr_find_registered_header (ModestWindowMgr *self, TnyHeader *header,
					      ModestWindow **win)
{
	ModestGtkWindowMgrPrivate *priv = NULL;
	gchar* uid = NULL;
	gboolean has_header, has_window = FALSE;
	GList *item = NULL;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (TNY_IS_HEADER(header), FALSE);
	
	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

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

static gboolean
modest_gtk_window_mgr_find_registered_message_uid (ModestWindowMgr *self, const gchar *msg_uid,
						       ModestWindow **win)
{
	ModestGtkWindowMgrPrivate *priv = NULL;
	gboolean has_header, has_window = FALSE;
	GList *item = NULL;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (msg_uid && msg_uid[0] != '\0', FALSE);
	
	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	has_header = MODEST_WINDOW_MGR_CLASS (parent_class)->find_registered_message_uid (self, msg_uid, win);
	
	item = g_list_find_custom (priv->window_list, msg_uid, (GCompareFunc) compare_msguids);
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
	
	return has_header || has_window;
}

static GList *
modest_gtk_window_mgr_get_window_list (ModestWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), NULL);
	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	return g_list_copy (priv->window_list);
}

static gboolean
modest_gtk_window_mgr_register_window (ModestWindowMgr *self, 
					   ModestWindow *window,
					   ModestWindow *parent)
{
	GList *win;
	ModestGtkWindowMgrPrivate *priv;
	gint *handler_id;
	gboolean nested_msg = FALSE;
	ModestWindow *current_top;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), FALSE);

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	/* Try to close active modal dialogs */
	if (modest_window_mgr_get_num_windows (self) &&
	    !_modest_window_mgr_close_active_modals (self))
		return FALSE;

	current_top = (ModestWindow *) modest_shell_peek_window (MODEST_SHELL (priv->shell));

	win = g_list_find (priv->window_list, window);
	if (win) {
		/* this is for the case we want to register the window
		   and it was already registered */
		gtk_window_present (GTK_WINDOW (priv->shell));
		return FALSE;
	}

	/* Do not allow standalone editors or standalone viewers */
	if (!current_top &&
	    (MODEST_IS_MSG_VIEW_WINDOW (window) ||
	     MODEST_IS_MSG_EDIT_WINDOW (window)))
		modest_window_mgr_show_initial_window (self);

	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		gchar *uid;
		TnyHeader *header;

		uid = g_strdup (modest_msg_view_window_get_message_uid (MODEST_MSG_VIEW_WINDOW (window)));
		
		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (window));

		if (uid == NULL)
			uid = modest_tny_folder_get_header_unique_id (header);
		/* Embedded messages do not have uid */
		if (uid) {
			if (g_list_find_custom (priv->window_list, uid, (GCompareFunc) compare_msguids)) {
				g_debug ("%s found another view window showing the same header", __FUNCTION__);
				g_free (uid);
				g_object_unref (header);
				return FALSE;
			}
			g_free (uid);
		} else if (header) {
			if (g_list_find_custom (priv->window_list, header, (GCompareFunc) compare_headers)) {
				g_debug ("%s found another view window showing the same header", __FUNCTION__);
				g_object_unref (header);
				return FALSE;
			}
		}
		if (header)
			g_object_unref (header);
	}

	/* Do not go backwards */
	if ((MODEST_IS_MSG_VIEW_WINDOW (current_top) ||
	     MODEST_IS_MSG_EDIT_WINDOW (current_top) ||
	     MODEST_IS_HEADER_WINDOW (current_top)) &&
	    (MODEST_IS_FOLDER_WINDOW (window) ||
	     MODEST_IS_ACCOUNTS_WINDOW (window) ||
	     MODEST_IS_MAILBOXES_WINDOW (window))) {
		gtk_window_present (GTK_WINDOW (priv->shell));
		return FALSE;
	}

	if (MODEST_IS_FOLDER_WINDOW (current_top) && MODEST_IS_FOLDER_WINDOW (window)) {
		gboolean retval;

		retval = modest_shell_delete_window (MODEST_SHELL (priv->shell), MODEST_WINDOW (window));

		if (retval) {
			gtk_window_present (GTK_WINDOW (priv->shell));
			return FALSE;
		}
		current_top = (ModestWindow *) modest_shell_peek_window (MODEST_SHELL (priv->shell));
	}

	if (MODEST_IS_MAILBOXES_WINDOW (current_top) && MODEST_IS_MAILBOXES_WINDOW (window)) {
		gtk_window_present (GTK_WINDOW (priv->shell));
		return FALSE;
	}

	/* Mailboxes window can not replace folder windows */
	if (MODEST_IS_FOLDER_WINDOW (current_top) && MODEST_IS_MAILBOXES_WINDOW (window)) {
		gtk_window_present (GTK_WINDOW (priv->shell));
		return FALSE;
	}

	/* Trying to open a folders window and a mailboxes window at
	   the same time from the accounts window is not allowed */
	if (MODEST_IS_MAILBOXES_WINDOW (current_top) &&
	    MODEST_IS_FOLDER_WINDOW (window) &&
	    MODEST_IS_ACCOUNTS_WINDOW (parent)) {
		gtk_window_present (GTK_WINDOW (priv->shell));
		return FALSE;
	}

	if (MODEST_IS_HEADER_WINDOW (current_top) && MODEST_IS_HEADER_WINDOW (window)) {
		g_debug ("Trying to register a second header window is not allowed");
		gtk_window_present (GTK_WINDOW (priv->shell));
		return FALSE;
	}

	if (!MODEST_WINDOW_MGR_CLASS (parent_class)->register_window (self, window, parent))
		goto fail;

	/* Add to list. Keep a reference to the window */
	g_object_ref (window);
	priv->window_list = g_list_prepend (priv->window_list, window);

	nested_msg = MODEST_IS_MSG_VIEW_WINDOW (window) &&
		MODEST_IS_MSG_VIEW_WINDOW (parent);

	/* Close views if they're being shown. Nevertheless we must
	   allow nested messages */
	if (!nested_msg &&
	    (MODEST_IS_MSG_EDIT_WINDOW (current_top) ||
	     MODEST_IS_MSG_VIEW_WINDOW (current_top))) {
		gboolean retval;

		/* If the current view has modal dialogs then
		   we fail to register the new view */
		if ((current_top != NULL) &&
		    shell_has_modals (MODEST_SHELL (priv->shell))) {
			/* Window on top but it has opened dialogs */
			goto fail;
		}

		/* Close the current view */
		retval = modest_shell_delete_window (MODEST_SHELL (priv->shell), current_top);
		if (retval) {
			/* Cancelled closing top window, then we fail to register */
			goto fail;
		}
	}

	/* Listen to object destruction */
	handler_id = g_malloc0 (sizeof (gint));
	*handler_id = g_signal_connect (window, "delete-event", G_CALLBACK (on_window_destroy), self);
	g_hash_table_insert (priv->destroy_handlers, window, handler_id);

	/* Show toolbar always */
	modest_window_show_toolbar (window, TRUE);

	modest_shell_add_window (MODEST_SHELL (priv->shell), window);

	return TRUE;
fail:
	/* Add to list. Keep a reference to the window */
	priv->window_list = g_list_remove (priv->window_list, window);
	g_object_unref (window);
	current_top = (ModestWindow *) modest_shell_peek_window (MODEST_SHELL (priv->shell));
	if (current_top)
		gtk_window_present (GTK_WINDOW (priv->shell));
	return FALSE;
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
		if (type == MODEST_MAIL_OPERATION_TYPE_RECEIVE ||
		    type == MODEST_MAIL_OPERATION_TYPE_OPEN ||
		    type == MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE) {
			modest_mail_operation_cancel (pending_ops->data);
		}
		g_object_unref (G_OBJECT (pending_ops->data));
		tmp_list = pending_ops;
		pending_ops = g_slist_next (pending_ops);
		g_slist_free_1 (tmp_list);
	}
}

static gboolean
shell_has_modals (ModestShell *shell)
{
	GList *toplevels;
	GList *node;
	gboolean retvalue = FALSE;

	/* First we fetch all toplevels */
	toplevels = gtk_window_list_toplevels ();
	for (node = toplevels; node != NULL; node = g_list_next (node)) {
		if (GTK_IS_WINDOW (node->data) &&
		    gtk_window_get_transient_for (GTK_WINDOW (node->data)) == GTK_WINDOW (shell) &&
		    GTK_WIDGET_VISIBLE (node->data)) {
			retvalue = TRUE;
			break;
		}
	}
	g_list_free (toplevels);
	return retvalue;
}

static gboolean
on_window_destroy (ModestWindow *window, 
		   GdkEvent *event,
		   ModestGtkWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv;
	gboolean no_propagate = FALSE;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	/* Do not close the window if it has modals on top */
	if (!MODEST_IS_MSG_EDIT_WINDOW (window) && shell_has_modals (MODEST_SHELL (priv->shell)))
		return TRUE;

	if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
		gboolean sent = FALSE;
		sent = modest_msg_edit_window_get_sent (MODEST_MSG_EDIT_WINDOW (window));
		/* Save currently edited message to Drafts if it was not sent */
		if (!sent && modest_msg_edit_window_is_modified (MODEST_MSG_EDIT_WINDOW (window))) {
			ModestMsgEditWindow *edit_window;
			MsgData *data;

			edit_window = MODEST_MSG_EDIT_WINDOW (window);
			data = modest_msg_edit_window_get_msg_data (edit_window);

			if (data) {
				gint parts_count;
				guint64 parts_size, available_size, expected_size;

				available_size = modest_utils_get_available_space (NULL);
				modest_msg_edit_window_get_parts_size (edit_window, &parts_count, &parts_size);
				expected_size = modest_tny_msg_estimate_size (data->plain_body,
									      data->html_body,
									      parts_count,
									      parts_size);
				modest_msg_edit_window_free_msg_data (edit_window, data);
				data = NULL;

				/* If there is not enough space
				   available for saving the message
				   then show an error and close the
				   window without saving */
				if (expected_size >= available_size) {
					modest_platform_run_information_dialog (GTK_WINDOW (edit_window),
										_("mail_in_ui_save_error"),
										FALSE);
				} else {
					if (!modest_ui_actions_on_save_to_drafts (NULL, MODEST_MSG_EDIT_WINDOW (window)))
						return TRUE;
				}
			} else {
				g_warning ("Edit window without message data. This is probably a bug");
			}
		}
	}

	/* Unregister window */
	modest_window_mgr_unregister_window (MODEST_WINDOW_MGR (self), window);
	no_propagate = FALSE;

	return no_propagate;
}

static void
modest_gtk_window_mgr_unregister_window (ModestWindowMgr *self, 
					 ModestWindow *window)
{
	GList *win;
	ModestGtkWindowMgrPrivate *priv;
	gulong *tmp, handler_id;
	guint num_windows;

	g_return_if_fail (MODEST_IS_GTK_WINDOW_MGR (self));
	g_return_if_fail (MODEST_IS_WINDOW (window));

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	win = g_list_find (priv->window_list, window);
	if (!win) {
		g_debug ("Trying to unregister a window that has not being registered yet");
		return;
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

	/* Disconnect the "delete-event" handler, we won't need it anymore */
	g_signal_handler_disconnect (window, handler_id);

	/* Destroy the window */
	g_object_unref (win->data);
	g_list_free (win);

	MODEST_WINDOW_MGR_CLASS (parent_class)->unregister_window (self, window);

	/* We have to get the number of windows here in order not to
	   emit the signal too many times */
	num_windows = modest_window_mgr_get_num_windows (self);

	/* If there are no more windows registered emit the signal */
	if (num_windows == 0)
		g_signal_emit_by_name (self, "window-list-empty");
}


static void
modest_gtk_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
					       gboolean on)
{
	g_return_if_fail (MODEST_IS_GTK_WINDOW_MGR (self));

	ModestGtkWindowMgrPrivate *priv = NULL;
	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);
	
	priv->fullscreen = on;

	if (on) {
		gtk_window_fullscreen (GTK_WINDOW (priv->shell));
	} else {
		gtk_window_unfullscreen (GTK_WINDOW (priv->shell));
	}
	return;
}

static gboolean
modest_gtk_window_mgr_get_fullscreen_mode (ModestWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv = NULL;
	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);
	
	return priv->fullscreen;
}

static void 
modest_gtk_window_mgr_show_toolbars (ModestWindowMgr *self,
					 GType window_type,
					 gboolean show_toolbars,
					 gboolean fullscreen)
{
	g_return_if_fail (MODEST_IS_GTK_WINDOW_MGR (self));

	return;
}

static gint
look_for_transient (gconstpointer a,
		    gconstpointer b)
{
	GtkWindow *win, *child;

	if (a == b)
		return 1;

	child = (GtkWindow *) b;
	win = (GtkWindow *) a;

	if ((gtk_window_get_transient_for (win) == child) &&
	    GTK_WIDGET_VISIBLE (win))
		return 0;
	else
		return 1;
}

static GtkWindow *
modest_gtk_window_mgr_get_modal (ModestWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv;
	GList *toplevel_list;
	GtkWidget *toplevel;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), NULL);
	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	/* Get current top */
	toplevel = priv->shell;
	toplevel_list = gtk_window_list_toplevels ();

	while (toplevel) {
		GList *parent_link;

		parent_link = g_list_find_custom (toplevel_list, toplevel, look_for_transient);
		if (parent_link)
			toplevel = (GtkWidget *) parent_link->data;
		else
			break;
	}

	if (toplevel && GTK_WIDGET_VISIBLE (toplevel) && gtk_window_get_modal ((GtkWindow *) toplevel))
		return (GtkWindow *) toplevel;
	else
		return NULL;
}


static void
modest_gtk_window_mgr_set_modal (ModestWindowMgr *self, 
				     GtkWindow *window,
				     GtkWindow *parent)
{
	ModestGtkWindowMgrPrivate *priv;

	g_return_if_fail (MODEST_IS_GTK_WINDOW_MGR (self));
	g_return_if_fail (GTK_IS_WINDOW (window));

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	gtk_window_set_modal (window, TRUE);

	if (GTK_IS_WINDOW (parent)) {
		gtk_window_set_transient_for (window, parent);
	} else {
		gtk_window_set_transient_for (window, GTK_WINDOW (priv->shell));
	}
	gtk_window_set_destroy_with_parent (window, TRUE);
}

static void
close_all_but_first (gpointer data)
{
	gint num_windows, i;
	gboolean retval;
	ModestGtkWindowMgrPrivate *priv;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE(data);
	num_windows = modest_shell_count_windows (MODEST_SHELL (priv->shell));

	for (i = 0; i < (num_windows - 1); i++) {
		ModestWindow *current_top;

		/* Close window */
		current_top = modest_shell_peek_window (MODEST_SHELL (priv->shell));
		retval = modest_shell_delete_window (MODEST_SHELL (priv->shell), current_top);
	}
}

static gboolean
on_idle_close_all_but_first (gpointer data)
{
	gdk_threads_enter ();
	close_all_but_first (data);
	gdk_threads_leave ();

	return FALSE;
}

static void
on_account_removed (TnyAccountStore *acc_store,
		    TnyAccount *account,
		    gpointer user_data)
{
	ModestWindow *current_top;
	ModestGtkWindowMgrPrivate *priv;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (user_data);

	/* Ignore transport account removals */
	if (TNY_IS_TRANSPORT_ACCOUNT (account))
		return;

	current_top = (ModestWindow *) modest_shell_peek_window (MODEST_SHELL (priv->shell));

	/* if we're showing the header view of the currently deleted
	   account, or the outbox and we deleted the last account,
	   then close the window */
	if (current_top &&
	    (MODEST_IS_HEADER_WINDOW (current_top) ||
	     MODEST_IS_FOLDER_WINDOW (current_top))) {
		    const gchar *acc_name;

		    acc_name = modest_tny_account_get_parent_modest_account_name_for_server_account (account);

		    /* We emit it in an idle, because sometimes this
		       function could called when the account settings
		       dialog is about to close but still there. That
		       modal dialog would otherwise, prevent the
		       windows from being closed */
		    if (!strcmp (acc_name, modest_window_get_active_account (current_top)))
			    g_idle_add (on_idle_close_all_but_first, (gpointer) user_data);
	}
}

static ModestWindow *
modest_gtk_window_mgr_show_initial_window (ModestWindowMgr *self)
{
	ModestWindow *initial_window = NULL;
	ModestGtkWindowMgrPrivate *priv;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	/* Connect to the account store "account-removed" signal". We
	   do this here because in the init the singletons are still
	   not initialized properly */
	if (!g_signal_handler_is_connected (modest_runtime_get_account_store (),
					    priv->accounts_handler)) {
		priv->accounts_handler = g_signal_connect (modest_runtime_get_account_store (),
							   "account-removed",
							   G_CALLBACK (on_account_removed),
							   self);
	}

	/* Return accounts window */
	initial_window = MODEST_WINDOW (modest_accounts_window_new ());
	modest_window_mgr_register_window (self, initial_window, NULL);

	return initial_window;
}


static ModestWindow *
modest_gtk_window_mgr_get_current_top (ModestWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);
	return (ModestWindow *) modest_shell_peek_window (MODEST_SHELL (priv->shell));
}

static gint 
find_folder_window (gconstpointer a,
		    gconstpointer b)
{
	return (MODEST_IS_FOLDER_WINDOW (a)) ? 0 : 1;
}

static ModestWindow *
modest_gtk_window_mgr_get_folder_window (ModestWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv;
	GList *window;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), NULL);

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	window = g_list_find_custom (priv->window_list,
				     NULL,
				     find_folder_window);

	return (window != NULL) ? MODEST_WINDOW (window->data) : NULL;
}

static gboolean
modest_gtk_window_mgr_screen_is_on (ModestWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_GTK_WINDOW_MGR (self), FALSE);

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE (self);

	return TRUE;
}

static void
modest_gtk_window_mgr_create_caches (ModestWindowMgr *self)
{
	g_return_if_fail (MODEST_IS_GTK_WINDOW_MGR (self));

	modest_accounts_window_pre_create ();

	MODEST_WINDOW_MGR_CLASS(parent_class)->create_caches (self);
}

static gboolean
modest_gtk_window_mgr_close_all_but_initial (ModestWindowMgr *self)
{
	ModestWindow *top;
	ModestGtkWindowMgrPrivate *priv;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE(self);

	/* Exit if there are no windows */
	if (!modest_window_mgr_get_num_windows (self)) {
		g_warning ("%s: unable to close, there are no windows", __FUNCTION__);
		return FALSE;
	}

	/* Close active modals */
	if (!_modest_window_mgr_close_active_modals (self)) {
		g_debug ("%s: unable to close some dialogs", __FUNCTION__);
		return FALSE;
	}

	/* Close all but first */
	top = modest_window_mgr_get_current_top (self);
	if (!MODEST_IS_ACCOUNTS_WINDOW (top))
		close_all_but_first ((gpointer) priv->shell);

	/* If some cannot be closed return */
	top = modest_window_mgr_get_current_top (self);
	if (!MODEST_IS_ACCOUNTS_WINDOW (top)) {
		g_debug ("%s: could not close some windows", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

GtkWidget *
modest_gtk_window_mgr_get_shell (ModestGtkWindowMgr *self)
{
	ModestGtkWindowMgrPrivate *priv;

	priv = MODEST_GTK_WINDOW_MGR_GET_PRIVATE(self);

	return priv->shell;
}

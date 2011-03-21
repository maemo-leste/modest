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
#include <hildon/hildon.h>
#include "modest-hildon2-window.h"
#include "modest-hildon2-window-mgr.h"
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
#include "modest-maemo-utils.h"
#include "modest-utils.h"
#include "modest-tny-msg.h"
#include "modest-tny-account.h"
#include <tny-merge-folder.h>
#include <modest-account-mgr-helpers.h>

/* 'private'/'protected' functions */
static void modest_hildon2_window_mgr_class_init (ModestHildon2WindowMgrClass *klass);
static void modest_hildon2_window_mgr_instance_init (ModestHildon2WindowMgr *obj);
static void modest_hildon2_window_mgr_finalize   (GObject *obj);

static gboolean on_window_destroy        (ModestWindow *window,
					  GdkEvent *event,
					  ModestHildon2WindowMgr *self);

static gboolean modest_hildon2_window_mgr_register_window (ModestWindowMgr *self, 
							   ModestWindow *window,
							   ModestWindow *parent);
static void modest_hildon2_window_mgr_unregister_window (ModestWindowMgr *self, 
							 ModestWindow *window);
static void modest_hildon2_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
							   gboolean on);
static gboolean modest_hildon2_window_mgr_get_fullscreen_mode (ModestWindowMgr *self);
static void modest_hildon2_window_mgr_show_toolbars (ModestWindowMgr *self,
						     GType window_type,
						     gboolean show_toolbars,
						     gboolean fullscreen);
static GtkWindow *modest_hildon2_window_mgr_get_modal (ModestWindowMgr *self);
static void modest_hildon2_window_mgr_set_modal (ModestWindowMgr *self, 
						 GtkWindow *window,
						 GtkWindow *parent);
static gboolean modest_hildon2_window_mgr_find_registered_header (ModestWindowMgr *self, 
								  TnyHeader *header,
								  ModestWindow **win);
static gboolean modest_hildon2_window_mgr_find_registered_message_uid (ModestWindowMgr *self, 
								       const gchar *msg_uid,
								       ModestWindow **win);
static GList *modest_hildon2_window_mgr_get_window_list (ModestWindowMgr *self);
static gboolean modest_hildon2_window_mgr_close_all_windows (ModestWindowMgr *self);
static gboolean modest_hildon2_window_mgr_close_all_but_initial (ModestWindowMgr *self);
static gboolean window_has_modals (ModestWindow *window);
static ModestWindow *modest_hildon2_window_mgr_show_initial_window (ModestWindowMgr *self);
static ModestWindow *modest_hildon2_window_mgr_get_current_top (ModestWindowMgr *self);
static gboolean modest_hildon2_window_mgr_screen_is_on (ModestWindowMgr *self);
static void modest_hildon2_window_mgr_create_caches (ModestWindowMgr *self);
static void osso_display_event_cb (osso_display_state_t state, 
				   gpointer data);
static void on_account_removed (TnyAccountStore *acc_store, 
				TnyAccount *account,
				gpointer user_data);
static ModestWindow *modest_hildon2_window_mgr_get_folder_window (ModestWindowMgr *self);

typedef struct _ModestHildon2WindowMgrPrivate ModestHildon2WindowMgrPrivate;
struct _ModestHildon2WindowMgrPrivate {
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

	/* Display state */
	osso_display_state_t display_state;
};
#define MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										   MODEST_TYPE_HILDON2_WINDOW_MGR, \
										   ModestHildon2WindowMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_hildon2_window_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestHildon2WindowMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_hildon2_window_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestHildon2WindowMgr),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_hildon2_window_mgr_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW_MGR,
		                                  "ModestHildon2WindowMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_hildon2_window_mgr_class_init (ModestHildon2WindowMgrClass *klass)
{
	GObjectClass *gobject_class;
	ModestWindowMgrClass *mgr_class;

	gobject_class = (GObjectClass*) klass;
	mgr_class = (ModestWindowMgrClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_hildon2_window_mgr_finalize;
	mgr_class->register_window = modest_hildon2_window_mgr_register_window;
	mgr_class->unregister_window = modest_hildon2_window_mgr_unregister_window;
	mgr_class->set_fullscreen_mode = modest_hildon2_window_mgr_set_fullscreen_mode;
	mgr_class->get_fullscreen_mode = modest_hildon2_window_mgr_get_fullscreen_mode;
	mgr_class->show_toolbars = modest_hildon2_window_mgr_show_toolbars;
	mgr_class->get_modal = modest_hildon2_window_mgr_get_modal;
	mgr_class->set_modal = modest_hildon2_window_mgr_set_modal;
	mgr_class->find_registered_header = modest_hildon2_window_mgr_find_registered_header;
	mgr_class->find_registered_message_uid = modest_hildon2_window_mgr_find_registered_message_uid;
	mgr_class->get_window_list = modest_hildon2_window_mgr_get_window_list;
	mgr_class->close_all_windows = modest_hildon2_window_mgr_close_all_windows;
	mgr_class->close_all_but_initial = modest_hildon2_window_mgr_close_all_but_initial;
	mgr_class->show_initial_window = modest_hildon2_window_mgr_show_initial_window;
	mgr_class->get_current_top = modest_hildon2_window_mgr_get_current_top;
	mgr_class->screen_is_on = modest_hildon2_window_mgr_screen_is_on;
	mgr_class->create_caches = modest_hildon2_window_mgr_create_caches;
	mgr_class->get_folder_window = modest_hildon2_window_mgr_get_folder_window;

	g_type_class_add_private (gobject_class, sizeof(ModestHildon2WindowMgrPrivate));

}

static void
modest_hildon2_window_mgr_instance_init (ModestHildon2WindowMgr *obj)
{
	ModestHildon2WindowMgrPrivate *priv;

	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE(obj);
	priv->window_list = NULL;
	priv->fullscreen_mode = FALSE;
	priv->window_state_uids = NULL;

	priv->modal_windows = g_queue_new ();
	priv->queue_lock = g_mutex_new ();

	/* Could not initialize it from gconf, singletons are not
	   ready yet */
	priv->destroy_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
	priv->viewer_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

	priv->closing_time = 0;

	priv->modal_handler_uids = NULL;
	priv->display_state = OSSO_DISPLAY_ON;

	/* Listen for changes in the screen, we don't want to show a
	   led pattern when the display is on for example */
	osso_hw_set_display_event_cb (modest_maemo_utils_get_osso_context (),
				      osso_display_event_cb,
				      obj); 

}

static void
modest_hildon2_window_mgr_finalize (GObject *obj)
{
	ModestHildon2WindowMgrPrivate *priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE(obj);
	ModestTnyAccountStore *acc_store;

	modest_signal_mgr_disconnect_all_and_destroy (priv->window_state_uids);
	priv->window_state_uids = NULL;

	osso_hw_set_display_event_cb (modest_maemo_utils_get_osso_context (),
				      NULL, NULL);

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
modest_hildon2_window_mgr_new (void)
{
	return MODEST_WINDOW_MGR(g_object_new(MODEST_TYPE_HILDON2_WINDOW_MGR, NULL));
}

static gboolean
modest_hildon2_window_mgr_close_all_windows (ModestWindowMgr *self)
{
	ModestHildon2WindowMgrPrivate *priv = NULL;
	gboolean ret_value = FALSE;
	GtkWidget *window;
	HildonWindowStack *stack;
	gboolean failed = FALSE;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), FALSE);
	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

	stack = hildon_window_stack_get_default ();

	while ((window = hildon_window_stack_peek (stack)) != NULL) {
		g_signal_emit_by_name (G_OBJECT (window), "delete-event", NULL, &ret_value);
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
modest_hildon2_window_mgr_find_registered_header (ModestWindowMgr *self, TnyHeader *header,
						  ModestWindow **win)
{
	ModestHildon2WindowMgrPrivate *priv = NULL;
	gchar* uid = NULL;
	gboolean has_header, has_window = FALSE;
	GList *item = NULL;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (TNY_IS_HEADER(header), FALSE);
	
	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

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
modest_hildon2_window_mgr_find_registered_message_uid (ModestWindowMgr *self, const gchar *msg_uid,
						       ModestWindow **win)
{
	ModestHildon2WindowMgrPrivate *priv = NULL;
	gboolean has_header, has_window = FALSE;
	GList *item = NULL;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (msg_uid && msg_uid[0] != '\0', FALSE);
	
	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

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
modest_hildon2_window_mgr_get_window_list (ModestWindowMgr *self)
{
	ModestHildon2WindowMgrPrivate *priv;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), NULL);
	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

	return g_list_copy (priv->window_list);
}

static gint window_precedence (GtkWindow *window)
{
	if (MODEST_IS_ACCOUNTS_WINDOW (window)) {
		return 10;
	} else if (MODEST_IS_MAILBOXES_WINDOW (window)) {
		return 20;
	} else if (MODEST_IS_FOLDER_WINDOW (window)) {
		return 30;
	} else if (MODEST_IS_HEADER_WINDOW (window)) {
		return 40;
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		return 50;
	} else if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
		return 60;
	} else {
		return 100;
	}
}

static gboolean
modest_hildon2_window_mgr_register_window (ModestWindowMgr *self, 
					   ModestWindow *window,
					   ModestWindow *parent)
{
	GList *win;
	ModestHildon2WindowMgrPrivate *priv;
	gint *handler_id;
	HildonWindowStack *stack;
	gboolean nested_msg = FALSE;
	ModestWindow *current_top;
	const gchar *acc_name, *toplevel_acc_name;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), FALSE);
	g_return_val_if_fail (GTK_IS_WINDOW (window), FALSE);

	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

	/* Try to close active modal dialogs */
	if (modest_window_mgr_get_num_windows (self) &&
	    !_modest_window_mgr_close_active_modals (self))
		return FALSE;

	stack = hildon_window_stack_get_default ();
	current_top = (ModestWindow *) hildon_window_stack_peek (stack);

	win = g_list_find (priv->window_list, window);
	if (win) {
		/* this is for the case we want to register the window
		   and it was already registered */
		gtk_window_present (GTK_WINDOW (window));
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

	/* Rules
	 *  * toplevel = msg edit -> if not modified, close, if modified, stay
	 *  * same account -> no action
	 *  * window = accounts -> no action
	 *  * window = folders, mailboxes, headers: close all up to accounts window
	 */

	if (MODEST_IS_MSG_EDIT_WINDOW (current_top) && 
	    !modest_msg_edit_window_is_modified (MODEST_MSG_EDIT_WINDOW (current_top))) {
		gboolean retval;

		g_signal_emit_by_name (G_OBJECT (current_top), "delete-event", NULL, &retval);
		current_top = (ModestWindow *) hildon_window_stack_peek (stack);
	}

	if (MODEST_IS_MSG_EDIT_WINDOW (current_top) ||
	    (current_top && MODEST_IS_ACCOUNTS_WINDOW (window))) {
		gtk_window_present (GTK_WINDOW (current_top));
		return FALSE;
	}

	acc_name = modest_window_get_active_account (window);

	if (MODEST_IS_MSG_VIEW_WINDOW (current_top) ||
	    MODEST_IS_HEADER_WINDOW (current_top) ||
	    MODEST_IS_FOLDER_WINDOW (current_top) ||
	    MODEST_IS_MAILBOXES_WINDOW (current_top)) {
		gboolean tree_view = modest_conf_get_bool (modest_runtime_get_conf (),MODEST_CONF_TREE_VIEW, NULL);
		toplevel_acc_name = modest_window_get_active_account (current_top);

		if (tree_view || (acc_name != NULL && g_strcmp0 (toplevel_acc_name, acc_name) == 0)) {
			/* Same account or tree view, no action */

			if (window_precedence (GTK_WINDOW (current_top)) >= window_precedence (GTK_WINDOW (window))) {
				if (!(MODEST_IS_MSG_VIEW_WINDOW (current_top) && MODEST_IS_MSG_VIEW_WINDOW (window))) {
					gtk_window_present (GTK_WINDOW (current_top));
					return FALSE;
				}
			}
		} else {
			while (current_top && !MODEST_IS_ACCOUNTS_WINDOW (current_top)) {
				gboolean retval;
				g_signal_emit_by_name (G_OBJECT (current_top), "delete-event", NULL, &retval);

				if (retval) {
					gtk_window_present (GTK_WINDOW (current_top));
					return FALSE;
				}
				current_top = (ModestWindow *) hildon_window_stack_peek (stack);
			}
		}
	}

	/* Trying to open a folders window and a mailboxes window at
	   the same time from the accounts window is not allowed */
	if (MODEST_IS_MAILBOXES_WINDOW (current_top) &&
	    MODEST_IS_FOLDER_WINDOW (window) &&
	    MODEST_IS_ACCOUNTS_WINDOW (parent)) {
		gtk_window_present (GTK_WINDOW (window));
		return FALSE;
	}

	if (MODEST_IS_HEADER_WINDOW (current_top) && MODEST_IS_HEADER_WINDOW (window)) {
		g_debug ("Trying to register a second header window is not allowed");
		gtk_window_present (GTK_WINDOW (current_top));
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
		    window_has_modals (MODEST_WINDOW (current_top))) {
			/* Window on top but it has opened dialogs */
			goto fail;
		}

		/* Close the current view */
		g_signal_emit_by_name (G_OBJECT (current_top), "delete-event", NULL, &retval);
		if (retval) {
			/* Cancelled closing top window, then we fail to register */
			goto fail;
		}
	}

	/* Listen to object destruction */
	handler_id = g_malloc0 (sizeof (gint));
	*handler_id = g_signal_connect (window, "delete-event", G_CALLBACK (on_window_destroy), self);
	g_hash_table_insert (priv->destroy_handlers, window, handler_id);

	if (!MODEST_IS_MSG_EDIT_WINDOW (window) &&
	    !MODEST_IS_ACCOUNTS_WINDOW (window)) {
		acc_name = modest_window_get_active_account (window);

		if (acc_name) {
			modest_platform_remove_new_mail_notifications (FALSE, acc_name);
			modest_account_mgr_set_has_new_mails (modest_runtime_get_account_mgr (),
							      acc_name,
							      FALSE);
		}
	}

	/* Show toolbar always */
	if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
		gboolean show_toolbar;
		show_toolbar = modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_EDIT_WINDOW_SHOW_TOOLBAR, NULL);
		modest_window_show_toolbar (window, show_toolbar);
	} else {
		modest_window_show_toolbar (window, TRUE);
	}

	return TRUE;
fail:
	/* Add to list. Keep a reference to the window */
	priv->window_list = g_list_remove (priv->window_list, window);
	g_object_unref (window);
	current_top = (ModestWindow *) hildon_window_stack_peek (stack);
	if (current_top)
		gtk_window_present (GTK_WINDOW (current_top));
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
window_has_modals (ModestWindow *window)
{
	GList *toplevels;
	GList *node;
	gboolean retvalue = FALSE;

	/* First we fetch all toplevels */
	toplevels = gtk_window_list_toplevels ();
	for (node = toplevels; node != NULL; node = g_list_next (node)) {
		if (GTK_IS_WINDOW (node->data) &&
		    gtk_window_get_transient_for (GTK_WINDOW (node->data)) == GTK_WINDOW (window) &&
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
		   ModestHildon2WindowMgr *self)
{
	gboolean no_propagate = FALSE;

	/* Do not close the window if it has modals on top */
	if (!MODEST_IS_MSG_EDIT_WINDOW (window) && window_has_modals (window))
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
modest_hildon2_window_mgr_unregister_window (ModestWindowMgr *self, 
					     ModestWindow *window)
{
	GList *win;
	ModestHildon2WindowMgrPrivate *priv;
	gulong *tmp, handler_id;
	guint num_windows;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self));
	g_return_if_fail (MODEST_IS_WINDOW (window));

	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

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

	/* Disconnect the "window-state-event" handler, we won't need it anymore */
	if (priv->window_state_uids) {
		priv->window_state_uids = 
			modest_signal_mgr_disconnect (priv->window_state_uids, 
						      G_OBJECT (window), 
						      "notify::is-topmost");
	}

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
modest_hildon2_window_mgr_set_fullscreen_mode (ModestWindowMgr *self,
					       gboolean on)
{
	g_return_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self));

	return;
}

static gboolean
modest_hildon2_window_mgr_get_fullscreen_mode (ModestWindowMgr *self)
{
	return FALSE;
}

static void 
modest_hildon2_window_mgr_show_toolbars (ModestWindowMgr *self,
					 GType window_type,
					 gboolean show_toolbars,
					 gboolean fullscreen)
{
	g_return_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self));

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
modest_hildon2_window_mgr_get_modal (ModestWindowMgr *self)
{
	ModestHildon2WindowMgrPrivate *priv;
	GList *toplevel_list;
	GtkWidget *current_top, *toplevel;
	HildonWindowStack *stack;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), NULL);
	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

	/* Get current top */
	stack = hildon_window_stack_get_default ();
	current_top = hildon_window_stack_peek (stack);
	toplevel = current_top;
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
modest_hildon2_window_mgr_set_modal (ModestWindowMgr *self, 
				     GtkWindow *window,
				     GtkWindow *parent)
{
	g_return_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self));
	g_return_if_fail (GTK_IS_WINDOW (window));

	gtk_window_set_modal (window, TRUE);
	gtk_window_set_transient_for (window, parent);
	gtk_window_set_destroy_with_parent (window, TRUE);

	if (!gtk_window_has_toplevel_focus (window)) {
		GList *toplevels, *node;

		toplevels = gtk_window_list_toplevels ();
		for (node = toplevels; node != NULL; node = g_list_next (node)) {
			if (gtk_window_has_toplevel_focus (GTK_WINDOW (node->data))) {
				if (GTK_IS_DIALOG (node->data)) {					
					gtk_window_set_transient_for (window, GTK_WINDOW (node->data));
				}
				break;
			}
		}
	}
}

static void
close_all_but_first (gpointer data)
{
	gint num_windows, i;
	gboolean retval;
	HildonWindowStack *stack;

	stack = hildon_window_stack_get_default ();
	g_return_if_fail (stack);

	num_windows = hildon_window_stack_size (stack);

	for (i = 0; i < (num_windows - 1); i++) {
		GtkWidget *current_top;

		/* Close window */
		current_top = hildon_window_stack_peek (stack);
		g_signal_emit_by_name (G_OBJECT (current_top), "delete-event", NULL, &retval);
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
	HildonWindowStack *stack;
	ModestWindow *current_top;

	/* Ignore transport account removals */
	if (TNY_IS_TRANSPORT_ACCOUNT (account))
		return;

	stack = hildon_window_stack_get_default ();
	current_top = (ModestWindow *) hildon_window_stack_peek (stack);

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
			    g_idle_add (on_idle_close_all_but_first, NULL);
	}
}

static ModestWindow *
modest_hildon2_window_mgr_show_initial_window (ModestWindowMgr *self)
{
	ModestWindow *initial_window = NULL;
	ModestHildon2WindowMgrPrivate *priv;

	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

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

	/* Return initial window */
	if (!modest_conf_get_bool (modest_runtime_get_conf (), 
				   MODEST_CONF_TREE_VIEW, NULL))
		initial_window = MODEST_WINDOW (modest_accounts_window_new ());
	else {
		initial_window = MODEST_WINDOW (modest_folder_window_new (NULL)); 
		modest_window_set_active_account(MODEST_WINDOW(initial_window), 
				 modest_account_mgr_get_default_account(modest_runtime_get_account_mgr()));
	}
	modest_window_mgr_register_window (self, initial_window, NULL);

	return initial_window;
}


static ModestWindow *
modest_hildon2_window_mgr_get_current_top (ModestWindowMgr *self)
{
	HildonWindowStack *stack;
	stack = hildon_window_stack_get_default ();
	return (ModestWindow *) hildon_window_stack_peek (stack);
}

static gint 
find_folder_window (gconstpointer a,
		    gconstpointer b)
{
	return (MODEST_IS_FOLDER_WINDOW (a)) ? 0 : 1;
}

static ModestWindow *
modest_hildon2_window_mgr_get_folder_window (ModestWindowMgr *self)
{
	ModestHildon2WindowMgrPrivate *priv;
	GList *window;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), NULL);

	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);

	window = g_list_find_custom (priv->window_list,
				     NULL,
				     find_folder_window);

	return (window != NULL) ? MODEST_WINDOW (window->data) : NULL;
}

static gboolean
modest_hildon2_window_mgr_screen_is_on (ModestWindowMgr *self)
{
	ModestHildon2WindowMgrPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self), FALSE);

	priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (self);
	
	return (priv->display_state == OSSO_DISPLAY_ON) ? TRUE : FALSE;
}

static void
modest_hildon2_window_mgr_create_caches (ModestWindowMgr *self)
{
	g_return_if_fail (MODEST_IS_HILDON2_WINDOW_MGR (self));

	if (modest_conf_get_bool (modest_runtime_get_conf (), 
				  MODEST_CONF_TREE_VIEW, NULL))
		modest_folder_window_pre_create();
	else
		modest_accounts_window_pre_create ();

	MODEST_WINDOW_MGR_CLASS(parent_class)->create_caches (self);
}

static void
osso_display_event_cb (osso_display_state_t state,
		       gpointer data)
{
	ModestHildon2WindowMgrPrivate *priv = MODEST_HILDON2_WINDOW_MGR_GET_PRIVATE (data);

	priv->display_state = state;

	/* Stop blinking if the screen becomes on */
	if (priv->display_state == OSSO_DISPLAY_ON)
		modest_platform_remove_new_mail_notifications (TRUE, NULL);
}

static gboolean
modest_hildon2_window_mgr_close_all_but_initial (ModestWindowMgr *self)
{
	ModestWindow *top;

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
		close_all_but_first (NULL);

	/* If some cannot be closed return */
	top = modest_window_mgr_get_current_top (self);
	if (!MODEST_IS_ACCOUNTS_WINDOW (top)) {
		g_debug ("%s: could not close some windows", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

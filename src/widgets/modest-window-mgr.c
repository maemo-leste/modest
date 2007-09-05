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


/* 'private'/'protected' functions */
static void modest_window_mgr_class_init (ModestWindowMgrClass *klass);
static void modest_window_mgr_init       (ModestWindowMgr *obj);
static void modest_window_mgr_finalize   (GObject *obj);

static gboolean on_window_destroy        (ModestWindow *window,
					  GdkEvent *event,
					  ModestWindowMgr *self);

static const gchar* get_show_toolbar_key (GType window_type,
					  gboolean fullscreen);

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestWindowMgrPrivate ModestWindowMgrPrivate;
struct _ModestWindowMgrPrivate {
	GList        *window_list;

	ModestWindow *main_window;
	GtkDialog    *easysetup_dialog;
	
	gboolean     fullscreen_mode;
	
	GSList       *windows_that_prevent_hibernation;
	GSList       *preregistered_uids;
	GHashTable   *destroy_handlers;
	GHashTable   *viewer_handlers;
	
	guint        closing_time;
};
#define MODEST_WINDOW_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                               MODEST_TYPE_WINDOW_MGR, \
                                               ModestWindowMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

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
}

static void
modest_window_mgr_init (ModestWindowMgr *obj)
{
	ModestWindowMgrPrivate *priv;

	priv = MODEST_WINDOW_MGR_GET_PRIVATE(obj);
	priv->window_list = NULL;
	priv->main_window = NULL;
	priv->fullscreen_mode = FALSE;
	priv->easysetup_dialog = NULL;
	
	priv->preregistered_uids = NULL;

	/* Could not initialize it from gconf, singletons are not
	   ready yet */
	priv->destroy_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);	
	priv->viewer_handlers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

	priv->closing_time = 0;
}

static void
modest_window_mgr_finalize (GObject *obj)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE(obj);

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

	if (priv->easysetup_dialog) {
		g_warning ("%s: forgot to destroy an easysetup dialog somewhere",
			   __FUNCTION__);
		gtk_widget_destroy (GTK_WIDGET(priv->easysetup_dialog));
		priv->easysetup_dialog = NULL;
	}
	
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
modest_window_mgr_register_header (ModestWindowMgr *self,  TnyHeader *header)
{
	ModestWindowMgrPrivate *priv;
	gchar* uid;
	
	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	g_return_if_fail (TNY_IS_HEADER(header));
		
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	uid = modest_tny_folder_get_header_unique_id (header);


	if (!has_uid (priv->preregistered_uids, uid)) {
		g_debug ("registering new uid %s", uid);
		priv->preregistered_uids = append_uid (priv->preregistered_uids, uid);
	} else
		g_debug ("already had uid %s", uid);

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
		g_debug ("trying to unregister non-existing uid %s", uid);
		priv->preregistered_uids = append_uid (priv->preregistered_uids, uid);
	} else
		g_debug ("unregistering uid %s", uid);

	if (has_uid (priv->preregistered_uids, uid)) {
		priv->preregistered_uids = remove_uid (priv->preregistered_uids, uid);
		if (has_uid (priv->preregistered_uids, uid))
			g_debug ("BUG: uid %s NOT removed", uid);
		else
			g_debug ("uid %s removed", uid);
	}

	g_free (uid);
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
	GList *wins = NULL;
	gboolean ret_value = FALSE;

	g_return_if_fail (MODEST_IS_WINDOW_MGR (self));
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	
	/* delete-event handler already removes window_list item, */
	/* so no next its required on this loop  */
	wins = priv->window_list;
	while (wins) {		
		g_signal_emit_by_name (G_OBJECT (wins->data), "delete-event", NULL, &ret_value);

		wins = priv->window_list;
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
		g_warning ("%s: trying to re-register a window",
			   __FUNCTION__);
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
		}
	}

	/* remove from the list of pre-registered uids */
	if (MODEST_IS_MSG_VIEW_WINDOW(window)) {
		const gchar *uid = modest_msg_view_window_get_message_uid
			(MODEST_MSG_VIEW_WINDOW (window));
		
		if (!has_uid (priv->preregistered_uids, uid)) 
			g_debug ("weird: no uid for window (%s)", uid);
		
		g_debug ("registering window for %s", uid ? uid : "<none>");

		priv->preregistered_uids = 
			remove_uid (priv->preregistered_uids,
				    modest_msg_view_window_get_message_uid
				    (MODEST_MSG_VIEW_WINDOW (window)));

	} else if (MODEST_IS_MSG_EDIT_WINDOW(window)) {
		const gchar *uid = modest_msg_edit_window_get_message_uid
			(MODEST_MSG_EDIT_WINDOW (window));
		
		g_debug ("registering window for %s", uid);

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
	/* Specific stuff first */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		ModestWindowMgrPrivate *priv;
		priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

		/* If more than one window already opened */
		if (g_list_length (priv->window_list) > 1) {

			/* If the user wants to close all the windows */
			if (modest_main_window_close_all (MODEST_MAIN_WINDOW (window))) {
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
		}
	}
	else {
		if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
			gboolean sent = FALSE;
			gint response = GTK_RESPONSE_ACCEPT;
			sent = modest_msg_edit_window_get_sent (MODEST_MSG_EDIT_WINDOW (window));
			/* Save currently edited message to Drafts if it was not sent */
			if (!sent && modest_msg_edit_window_is_modified (MODEST_MSG_EDIT_WINDOW (window))) {
				
				response =
					modest_platform_run_confirmation_dialog (GTK_WINDOW (window),
										 _("mcen_nc_no_email_message_modified_save_changes"));
				/* Save to drafts */
				if (response != GTK_RESPONSE_CANCEL) 				
					modest_ui_actions_on_save_to_drafts (NULL, MODEST_MSG_EDIT_WINDOW (window));
				
			} 
		}
	}

	/* Save configuration state (TODO: why edit window does not require this function ?) */
	if (!MODEST_IS_MSG_EDIT_WINDOW (window)) 
		modest_window_save_state (MODEST_WINDOW(window));


	/* Unregister window */
	modest_window_mgr_unregister_window (self, window);
	
	return FALSE;
}

static void
disconnect_msg_changed (gpointer key, 
			gpointer value, 
			gpointer user_data)
{
	gulong *handler_id;

	handler_id = (gulong *) value;
	g_signal_handler_disconnect (G_OBJECT (key), *handler_id);
}



/* interval before retrying to close the application */
#define CLOSING_RETRY_INTERVAL 3000 
/* interval before cancel whatever is left in the queue, and closing anyway */
#define MAX_WAIT_FOR_CLOSING 30 * 1000 

static gboolean
on_quit_maybe (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	guint queue_num;
	
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	/* it seems, in the meantime some windows were
	 * created. in that case, stop  'on_quit_maybe' */
	if (priv->window_list) {
		priv->closing_time = 0;
		return FALSE;
	}

	if (priv->closing_time >= MAX_WAIT_FOR_CLOSING) {
		/* we waited long enough: cancel all remaining operations */
		g_debug ("%s: we waited long enough (%ds), cancelling queue and quiting",
			 __FUNCTION__, priv->closing_time/1000);
		/* FIXME: below gives me a lot of:
		 * GLIB CRITICAL ** default - modest_mail_operation_cancel:
		 *                     assertion `priv->account' failed
		 * which means there is no account for the given operation
		 * so, we're not trying to be nice, we're just quiting.
		 */
		//modest_mail_operation_queue_cancel_all
		//	(modest_runtime_get_mail_operation_queue());
	} else {
	
		/* if there is anything left in our operation queue,
		 * wait another round
		 */
		queue_num = modest_mail_operation_queue_num_elements
			(modest_runtime_get_mail_operation_queue()); 
		if  (queue_num > 0) {
			g_debug ("%s: waiting, there are still %d operation(s) queued",
				 __FUNCTION__, queue_num);
			priv->closing_time += CLOSING_RETRY_INTERVAL;
			return TRUE;
		}
	}
	
	/* so: no windows left, nothing in the queue: quit */
	priv->closing_time = 0;
	gtk_main_quit ();
	return FALSE;
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
		g_signal_handler_disconnect (window, *tmp);
		g_hash_table_remove (priv->viewer_handlers, window);
	}

	/* Save state */
	modest_window_save_state (window);

	/* Remove from list & hash table */
	priv->window_list = g_list_remove_link (priv->window_list, win);
	tmp = g_hash_table_lookup (priv->destroy_handlers, window);
	handler_id = *tmp;
	g_hash_table_remove (priv->destroy_handlers, window);

	/* Disconnect the "delete-event" handler, we won't need it anymore */
	g_signal_handler_disconnect (window, handler_id);

	/* Disconnect all the window signals */
	modest_window_disconnect_signals (window);
	
	/* Destroy the window */
	gtk_widget_destroy (win->data);
	
	/* If there are no more windows registered then exit program */
	if (priv->window_list == NULL)
		g_timeout_add (CLOSING_RETRY_INTERVAL,
			       (GSourceFunc)on_quit_maybe, self);
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
modest_window_mgr_get_main_window (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), NULL);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);
	
	/* create the main window, if it hasn't been created yet */
	if (!priv->main_window) {
		/* modest_window_mgr_register_window will set priv->main_window */
		modest_window_mgr_register_window (self, modest_main_window_new ());
		g_debug ("%s: created main window: %p\n", __FUNCTION__, priv->main_window);
	}
	
	return priv->main_window;
}


GtkDialog*
modest_window_mgr_get_easysetup_dialog (ModestWindowMgr *self)
{
	ModestWindowMgrPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), NULL);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return priv->easysetup_dialog;
}


GtkDialog*
modest_window_mgr_set_easysetup_dialog (ModestWindowMgr *self, GtkDialog *dialog)
{
	ModestWindowMgrPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_WINDOW_MGR (self), NULL);
	priv = MODEST_WINDOW_MGR_GET_PRIVATE (self);

	return priv->easysetup_dialog = dialog;
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

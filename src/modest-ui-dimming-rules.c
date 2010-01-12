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
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <string.h>
#include "modest-ui-dimming-rules.h"
#include "modest-ui-actions.h"
#include "modest-account-mgr-helpers.h"
#include "modest-dimming-rule.h"
#include "modest-debug.h"
#include "modest-tny-folder.h"
#include "modest-tny-account.h"
#include "modest-tny-msg.h"
#include "modest-tny-mime-part.h"
#include "modest-text-utils.h"
#include "widgets/modest-folder-view.h"
#include "modest-address-book.h"
#include <widgets/modest-attachments-view.h>
#include <modest-runtime.h>
#include <tny-simple-list.h>
#include <tny-merge-folder.h>
#include <widgets/modest-recpt-editor.h>
#include <gtkhtml/gtkhtml.h>
#include <modest-runtime.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-header-window.h>
#include <modest-folder-window.h>
#endif


static gboolean _folder_is_any_of_type (TnyFolder *folder, TnyFolderType types[], guint ntypes);
static gboolean _invalid_msg_selected (ModestMainWindow *win, gboolean unique, ModestDimmingRule *rule);
static gboolean _invalid_attach_selected (ModestWindow *win, 
					  gboolean unique, gboolean for_view, gboolean for_remove,
					  ModestDimmingRule *rule);
static gboolean _purged_attach_selected (ModestWindow *win, gboolean all, ModestDimmingRule *rule);
static gboolean _clipboard_is_empty (ModestWindow *win);
static gboolean _invalid_clipboard_selected (ModestWindow *win, ModestDimmingRule *rule);
static gboolean _selected_folder_not_writeable (ModestMainWindow *win, gboolean for_paste);
static gboolean _selected_folder_not_moveable (ModestMainWindow *win);
static gboolean _selected_folder_not_renameable (ModestMainWindow *win);
static gboolean _selected_folder_not_deletable (ModestMainWindow *win);
static gboolean _selected_folder_is_any_of_type (ModestWindow *win, TnyFolderType types[], guint ntypes);
static gboolean _selected_folder_is_root_or_inbox (ModestMainWindow *win);
static gboolean _selected_folder_is_root (ModestMainWindow *win);
static gboolean _header_view_is_all_selected (ModestMainWindow *win);
static gboolean _selected_folder_is_empty (ModestMainWindow *win);
static gboolean _folder_view_has_focus (ModestWindow *win);
static gboolean _selected_folder_is_same_as_source (ModestWindow *win);
static gboolean _msg_download_in_progress (ModestWindow *win);
static gboolean _msg_download_completed (ModestMainWindow *win);
static gboolean _selected_msg_sent_in_progress (ModestWindow *win);
static gboolean _invalid_folder_for_purge (ModestWindow *win, ModestDimmingRule *rule);
static gboolean _transfer_mode_enabled (ModestWindow *win);
static gboolean _selected_folder_has_subfolder_with_same_name (ModestWindow *win);
static void fill_list_of_caches (gpointer key, gpointer value, gpointer userdata);
static gboolean _send_receive_in_progress (ModestWindow *win);
static gboolean _msgs_send_in_progress (void);
static gboolean _all_msgs_in_sending_status (ModestHeaderView *header_view) G_GNUC_UNUSED;
static gboolean _forbid_outgoing_xfers (ModestWindow *window);

static DimmedState *
_define_main_window_dimming_state (ModestMainWindow *window)
{
	DimmedState *state = NULL;
	GtkWidget *focused_widget = NULL;
	GtkWidget *header_view = NULL;
	GtkTreeModel *model = NULL;
	TnyList *selected_headers = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	ModestCacheMgr *cache_mgr = NULL;
	GHashTable *send_queue_cache = NULL;
	ModestTnySendQueue *send_queue = NULL;
	GSList *send_queues = NULL, *node = NULL;
	ModestWindowMgr *mgr = NULL;
	gboolean found = FALSE;
	gchar *msg_uid = NULL;
	TnyHeaderFlags flags;
	gboolean all_deleted = TRUE;
	gboolean all_seen = TRUE;
	gboolean all_cached = TRUE;
	gboolean all_has_attach = TRUE;
	TnyFolder *folder = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(window), NULL);

	/* Init state */
	state = g_slice_new0 (DimmedState);
	state->n_selected = 0;
	state->already_opened_msg = 0;
	state->any_marked_as_deleted = FALSE;
	state->all_marked_as_deleted = FALSE;
	state->any_marked_as_seen = FALSE;
	state->all_marked_as_seen = FALSE;
	state->any_marked_as_cached = FALSE;
	state->all_marked_as_cached = FALSE;
	state->any_has_attachments = FALSE;
	state->all_has_attachments = FALSE;
	state->sent_in_progress = FALSE;
	state->all_selected = FALSE;

	/* Get focused widget */
	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));
	if (MODEST_IS_FOLDER_VIEW (focused_widget)) {
		state->n_selected++;
		return state;
	} else if (MODEST_IS_HEADER_VIEW (focused_widget)) {
		header_view = focused_widget;		
	} else {
		header_view = modest_main_window_get_child_widget (window, MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
	}
	
	/* Get header view and selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
	if (!selected_headers) 
		return state;

	/* Examine selected headers */
	iter = tny_list_create_iterator (selected_headers);
	while (!tny_iterator_is_done (iter)) {
		header = TNY_HEADER (tny_iterator_get_current (iter));
		flags = tny_header_get_flags (header);
		
		/* No selected */
		state->n_selected++;
		
		/* Already opened */
		mgr = modest_runtime_get_window_mgr ();
		if (modest_window_mgr_find_registered_header (mgr, header, NULL))
			state->already_opened_msg++;
		

		/* Mark as deleted */		
		all_deleted = all_deleted && (flags & TNY_HEADER_FLAG_DELETED);
		state->all_marked_as_deleted = all_deleted;
		if (!state->any_marked_as_deleted)
			state->any_marked_as_deleted = flags & TNY_HEADER_FLAG_DELETED;
		
		/* Mark as seen */
		all_seen = all_seen && (flags & TNY_HEADER_FLAG_SEEN);
		state->all_marked_as_seen = all_seen;
		if (!state->any_marked_as_seen)
			state->any_marked_as_seen = flags & TNY_HEADER_FLAG_SEEN;
		
		/* Mark as cached */
		all_cached = all_cached && (flags & TNY_HEADER_FLAG_CACHED);
		state->all_marked_as_cached = all_cached;
		if (!state->any_marked_as_cached)
			state->any_marked_as_cached = flags & TNY_HEADER_FLAG_CACHED;
		
		/* Mark has_attachments */
		all_has_attach = all_has_attach && (flags & TNY_HEADER_FLAG_ATTACHMENTS);
		state->all_has_attachments = all_has_attach;
		if (!state->any_has_attachments)
			state->any_has_attachments = flags & TNY_HEADER_FLAG_ATTACHMENTS;
	
		/* sent in progress */
		folder = tny_header_get_folder (header);
		if (folder) {
			if (modest_tny_folder_guess_folder_type (folder) == TNY_FOLDER_TYPE_OUTBOX) {
				msg_uid = modest_tny_send_queue_get_msg_id (header);
				if (!state->sent_in_progress) {
					cache_mgr = modest_runtime_get_cache_mgr ();
					send_queue_cache = modest_cache_mgr_get_cache (cache_mgr,
										       MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);
			
					g_hash_table_foreach (send_queue_cache, (GHFunc) fill_list_of_caches, &send_queues);
					
					for (node = send_queues; node != NULL && !found; node = g_slist_next (node)) {
						send_queue = MODEST_TNY_SEND_QUEUE (node->data);
						
						/* Check if msg uid is being processed inside send queue */
						found = modest_tny_send_queue_msg_is_being_sent (send_queue, msg_uid);		
					}
					state->sent_in_progress = found;
				}
			}
			g_object_unref (folder);
		}
		tny_iterator_next (iter);
		g_object_unref (header);
	}

	/* check if all the headers are selected or not */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(header_view));
	if (model != NULL){
		gint count;
		count = gtk_tree_model_iter_n_children(model, NULL);
		if(state->n_selected == count)
			state->all_selected = TRUE;
	}

	/* Free */
	g_free(msg_uid);
	g_object_unref(selected_headers);
	g_object_unref(iter);
	g_slist_free (send_queues);
	
	return state;
}

static DimmedState *
_define_msg_view_window_dimming_state (ModestMsgViewWindow *window)
{
	DimmedState *state = NULL;
	TnyHeader *header = NULL;
	ModestCacheMgr *cache_mgr = NULL;
	GHashTable *send_queue_cache = NULL;
	ModestTnySendQueue *send_queue = NULL;
	GSList *send_queues = NULL, *node = NULL;
	gboolean found = FALSE;
	gchar *msg_uid = NULL;
	TnyHeaderFlags flags;
	gboolean all_deleted = TRUE;
	gboolean all_seen = TRUE;
	gboolean all_cached = TRUE;
	gboolean all_has_attach = TRUE;
			
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(window), NULL);

	/* Init state */
	state = g_slice_new0 (DimmedState);
	state->n_selected = 0;
	state->already_opened_msg = 0;
	state->any_marked_as_deleted = FALSE;
	state->all_marked_as_deleted = FALSE;
	state->any_marked_as_seen = FALSE;
	state->all_marked_as_seen = FALSE;
	state->any_marked_as_cached = FALSE;
	state->all_marked_as_cached = FALSE;
	state->any_has_attachments = FALSE;
	state->all_has_attachments = FALSE;
	state->sent_in_progress = FALSE;

	header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW(window));
	if (header == NULL)
		return state;
	g_return_val_if_fail (TNY_IS_HEADER(header), state);
	flags = tny_header_get_flags (header);

	/* Selected */
	state->n_selected++;

	/* Mark as deleted */		
	all_deleted = all_deleted && (flags & TNY_HEADER_FLAG_DELETED);
	state->all_marked_as_deleted = all_deleted;
	if (!state->any_marked_as_deleted)
		state->any_marked_as_deleted = flags & TNY_HEADER_FLAG_DELETED;
	
	/* Mark as seen */
	all_seen = all_seen && (flags & TNY_HEADER_FLAG_SEEN);
	state->all_marked_as_seen = all_seen;
	if (!state->any_marked_as_seen)
		state->any_marked_as_seen = flags & TNY_HEADER_FLAG_SEEN;
	
	/* Mark as cached */
	all_cached = all_cached && (flags & TNY_HEADER_FLAG_CACHED);
	state->all_marked_as_cached = all_cached;
	if (!state->any_marked_as_cached)
		state->any_marked_as_cached = flags & TNY_HEADER_FLAG_CACHED;
	
	/* Mark has_attachments */
	all_has_attach = all_has_attach && (flags & TNY_HEADER_FLAG_ATTACHMENTS);
	state->all_has_attachments = all_has_attach;
	if (!state->any_has_attachments)
		state->any_has_attachments = (flags & TNY_HEADER_FLAG_ATTACHMENTS)?1:0;
	
	/* sent in progress */
	msg_uid = modest_tny_send_queue_get_msg_id (header);
	if (!state->sent_in_progress) {
		cache_mgr = modest_runtime_get_cache_mgr ();
		send_queue_cache = modest_cache_mgr_get_cache (cache_mgr,
							       MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);
		
		g_hash_table_foreach (send_queue_cache, (GHFunc) fill_list_of_caches, &send_queues);
		
		for (node = send_queues; node != NULL && !found; node = g_slist_next (node)) {
			send_queue = MODEST_TNY_SEND_QUEUE (node->data);
			
			/* Check if msg uid is being processed inside send queue */
			found = modest_tny_send_queue_msg_is_being_sent (send_queue, msg_uid);		
		}
		state->sent_in_progress = found;
	}
	
	/* Free */
	g_free(msg_uid);
	g_object_unref (header);
	g_slist_free (send_queues);

	return state;
}

   
DimmedState *
modest_ui_dimming_rules_define_dimming_state (ModestWindow *window)
{
	DimmedState *state = NULL;

	g_return_val_if_fail (MODEST_IS_WINDOW(window), NULL);
	
	if (MODEST_IS_MAIN_WINDOW (window)) 
		state = _define_main_window_dimming_state (MODEST_MAIN_WINDOW(window));
	else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		state = _define_msg_view_window_dimming_state (MODEST_MSG_VIEW_WINDOW(window));		
	}
	
	return state;
}

gboolean 
modest_ui_dimming_rules_on_new_msg (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
		
	/* Check dimmed rule */	
	if (MODEST_IS_MSG_VIEW_WINDOW(win)) {
#ifndef MODEST_TOOLKIT_HILDON2
		dimmed = _msg_download_in_progress (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
#endif
	} else if (MODEST_IS_MAIN_WINDOW(win)) {
		dimmed = !modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), 
							  TRUE);	
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mcen_nc_no_email_acnts_defined"));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_new_folder (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	dimmed = _transfer_mode_enabled (win);
	if (dimmed) {
		modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_create_error"));
		return dimmed;
	}

	/* Get selected folder as parent of new folder to create */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	if (!folder_view) /* folder view may not have been created yet */
		return TRUE;	

	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!parent_folder)
		return TRUE;
	
	if (TNY_IS_ACCOUNT (parent_folder)) {
		/* If it's the local account then do not dim */
		if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (parent_folder))) {
			dimmed = FALSE;
		} else {
			ModestProtocolType protocol_type = modest_tny_account_get_protocol_type (TNY_ACCOUNT (parent_folder));
			if (protocol_type != MODEST_PROTOCOL_REGISTRY_TYPE_INVALID) {
				ModestProtocolRegistry *protocol_registry;

				protocol_registry = modest_runtime_get_protocol_registry ();
				/* If account does not support folders (pop) then dim */
				dimmed = (!modest_protocol_registry_protocol_type_has_tag (protocol_registry, protocol_type,
											   MODEST_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS));
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_create_error"));
			}
		}
	} else {	
		TnyFolderType types[3];
				
		types[0] = TNY_FOLDER_TYPE_DRAFTS; 
		types[1] = TNY_FOLDER_TYPE_OUTBOX;
		types[2] = TNY_FOLDER_TYPE_SENT;

		/* Apply folder rules */	
		if (!dimmed) {
			dimmed = _selected_folder_not_writeable (MODEST_MAIN_WINDOW(win), FALSE);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_create_error"));
		}
		if (!dimmed) {
			dimmed = _selected_folder_is_any_of_type (win, types, 3);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_create_error"));
		}
	}

	/* if not the folder is selected then dim */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	if (folder_view && !gtk_widget_is_focus (folder_view)) 
		dimmed = TRUE;

	g_object_unref (parent_folder);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_delete (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	GtkWidget *folder_view = NULL;
	GtkWidget *header_view = NULL;	
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	
	if (MODEST_IS_MAIN_WINDOW (win)) {
		/* Get the folder view */
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		
		/* Get header view */
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

		if (header_view && gtk_widget_is_focus (header_view)) 
			dimmed = modest_ui_dimming_rules_on_delete_msg (win, rule);

		if (folder_view && gtk_widget_is_focus (folder_view)) 
			dimmed = modest_ui_dimming_rules_on_delete_folder (win, rule);

		if (header_view && folder_view &&
		    !gtk_widget_is_focus (header_view) &&
		    !gtk_widget_is_focus (folder_view)) {
			dimmed = TRUE;
			modest_dimming_rule_set_notification (rule, _CS("ckct_ib_nothing_to_delete"));
		}

#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_FOLDER_WINDOW (win)) {
		dimmed = modest_ui_dimming_rules_on_folder_window_delete (win, user_data);
	} else if (MODEST_IS_HEADER_WINDOW (win)) {

		if (!dimmed)
			dimmed = _transfer_mode_enabled (win);

		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));

		if (!dimmed) {
			GtkWidget *header_view;
			TnyFolder *folder;

			header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (win)));
			folder = modest_header_view_get_folder (MODEST_HEADER_VIEW (header_view));
			if (folder) {
				dimmed = (tny_folder_get_all_count (TNY_FOLDER (folder)) == 0) ||
					modest_header_view_is_empty (MODEST_HEADER_VIEW (header_view));

				if (!dimmed &&
				    (tny_folder_get_folder_type (TNY_FOLDER (folder)) == TNY_FOLDER_TYPE_OUTBOX)) {
					dimmed = _all_msgs_in_sending_status (MODEST_HEADER_VIEW (header_view));;
				}
				g_object_unref (folder);
			}
		}

#endif
	} else {
		dimmed = modest_ui_dimming_rules_on_delete_folder (win, rule);
	}

	return dimmed;
}



gboolean 
modest_ui_dimming_rules_on_delete_folder (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	TnyFolderType types[6];
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	types[3] = TNY_FOLDER_TYPE_INBOX;
	types[4] = TNY_FOLDER_TYPE_ROOT;
	types[5] = TNY_FOLDER_TYPE_ARCHIVE;

		
	/* Check dimmed rule */	
	dimmed = _selected_folder_not_deletable (MODEST_MAIN_WINDOW(win));
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_delete_error"));
	if (!dimmed) {
		dimmed = _selected_folder_is_any_of_type (win, types, 6);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_delete_error"));
	}
	if (!dimmed) {
		dimmed = _selected_folder_is_root_or_inbox (MODEST_MAIN_WINDOW(win));
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_delete_error"));
	}
	if (!dimmed) {
		dimmed = _transfer_mode_enabled (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _CS("ckct_ib_unable_to_delete"));
	}

	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_sort (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

#ifdef MODEST_TOOLKIT_HILDON2
	if (MODEST_IS_HEADER_WINDOW (win)) {
		return FALSE;
	}
#endif		

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	dimmed = _selected_folder_is_root (MODEST_MAIN_WINDOW(win));

	if (!dimmed)
		dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));

	return dimmed;
	
}

gboolean 
modest_ui_dimming_rules_on_rename_folder (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	TnyFolderType types[4];
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	types[0] = TNY_FOLDER_TYPE_DRAFTS;
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	types[3] = TNY_FOLDER_TYPE_ARCHIVE;

	/* Check dimmed rule */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		dimmed = _selected_folder_not_renameable (MODEST_MAIN_WINDOW(win));
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
		if (!dimmed) {
			dimmed = _selected_folder_is_root_or_inbox (MODEST_MAIN_WINDOW(win));
			if (dimmed)
				modest_dimming_rule_set_notification (rule, "");
		}
	}

#ifdef MODEST_TOOLKIT_HILDON2
	if (MODEST_IS_FOLDER_WINDOW (win)) {
		ModestFolderView *folder_view;
		folder_view = modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (win));
		dimmed = !modest_folder_view_any_folder_fulfils_rules (folder_view,
								       MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE);
	}
#endif

	if (!dimmed) {
		dimmed = _selected_folder_is_any_of_type (win, types, 4);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_open_msg (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	const DimmedState *state = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);		

	/* Check dimmed rule */	
	dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE, user_data);	
	if (!dimmed) {
		if (state)
			dimmed = state->any_marked_as_deleted;
		if (dimmed) {
			gchar *msg = modest_ui_actions_get_msg_already_deleted_error_msg (win);
			modest_dimming_rule_set_notification (rule, msg);
			g_free (msg);
		}
	}
	if (!dimmed) {
		dimmed = _selected_msg_sent_in_progress (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_open_while_sent"));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_reply_msg (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	TnyFolderType types[3];

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	types[0] = TNY_FOLDER_TYPE_DRAFTS;
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_ROOT;

	/* Check dimmed rule */
	dimmed = _selected_folder_is_any_of_type (win, types, 3);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_reply"));

	/* main window dimming rules */
	if (MODEST_IS_MAIN_WINDOW(win)) {

		if (!dimmed) {
			dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_nothing_to_reply"));
		}
		if (!dimmed) {
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE, rule);
		}
	/* msg view window dimming rules */
	} else if (MODEST_IS_MSG_VIEW_WINDOW(win)) {

		/* This could happen if we load the msg view window with a
		   preview before loading the full message */
		TnyMsg *msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
		if (!msg) {
			dimmed = TRUE;
		} else {
			g_object_unref (msg);
		}

		if (!dimmed) {
			dimmed = _transfer_mode_enabled (win);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));
		}
		if (!dimmed) {
			dimmed = _msg_download_in_progress (win);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_reply"));
		}
	}

	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_contents_msg (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
		
	/* Check dimmed rule */	
	dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);	
	if (!dimmed) {
		dimmed = _msg_download_completed (MODEST_MAIN_WINDOW(win));
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_always_dimmed (ModestWindow *win, gpointer user_data)
{
	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);

	return TRUE;
}

static gboolean
_message_already_sent (ModestMsgViewWindow *view_window)
{
	TnyHeader *header;
	TnyFolder *folder;
	gboolean already_sent = FALSE;

	header = modest_msg_view_window_get_header (view_window);
	if (header) {
		folder = tny_header_get_folder (header);
		if (folder) {
			if (modest_tny_folder_guess_folder_type (folder) ==
			    TNY_FOLDER_TYPE_OUTBOX) {
				ModestTnySendQueueStatus status = 
					modest_tny_all_send_queues_get_msg_status (header);
				if (status == MODEST_TNY_SEND_QUEUE_UNKNOWN ||
				    status == MODEST_TNY_SEND_QUEUE_SENDING)
					already_sent = TRUE;
			}
			g_object_unref (folder);
		}
		g_object_unref (header);
	}
	return already_sent;
}


gboolean 
modest_ui_dimming_rules_on_delete_msg (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);

	/* If we're in transfer mode then do not allow to delete messages */
	dimmed = _transfer_mode_enabled (win);
	if (dimmed) {
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));
		return dimmed;
	}

	/* Check dimmed rule */	
	if (MODEST_IS_MAIN_WINDOW (win)) {
		dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _CS("ckct_ib_nothing_to_delete"));
		if (!dimmed) {
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);
		}
		if (!dimmed) {
			if (state)
				dimmed = state->sent_in_progress;
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _CS("ckct_ib_unable_to_delete"));
		}
		if (!dimmed) {
			if (state)
				dimmed = state->any_marked_as_deleted;
			if (dimmed) {
				gchar *msg = modest_ui_actions_get_msg_already_deleted_error_msg (win);
				modest_dimming_rule_set_notification (rule, msg);
				g_free (msg);
			}
		}
		if (!dimmed) {
			if (state) {
				dimmed = (state->already_opened_msg > 0) ? TRUE : FALSE;
				if (dimmed) {
					gchar *message = NULL;

					message = g_strdup_printf(_("mcen_nc_unable_to_delete_n_messages"),
								  state->already_opened_msg);
					modest_dimming_rule_set_notification (rule, message);
					g_free(message);
				}
			}
		}
	}
	else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		/* This could happen if we load the msg view window with a
		   preview before loading the full message */
		TnyMsg *msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
		if (!msg) {
			dimmed = TRUE;
		} else {
			g_object_unref (msg);
		}

		if (!dimmed) {
			if (state)
				dimmed = state->any_marked_as_deleted;
			if (dimmed) {
				gchar *msg = modest_ui_actions_get_msg_already_deleted_error_msg (win);
				modest_dimming_rule_set_notification (rule, msg);
				g_free (msg);
			}
		}
		if (!dimmed) {
			if (state)
				dimmed = state->sent_in_progress;
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _CS("ckct_ib_unable_to_delete"));
		}

		/* This could happen if we're viewing a message of the
		   outbox that has been already sent */
		if (!dimmed)
			dimmed = _message_already_sent (MODEST_MSG_VIEW_WINDOW(win));

		/* The delete button should be dimmed when viewing an attachment,
		 * but should be enabled when viewing a message from the list, 
		 * or when viewing a search result.
		 */
		if (!dimmed) {
			TnyMsg *top_msg = NULL;
			top_msg = modest_msg_view_window_get_top_message ((ModestMsgViewWindow *) win);
			if (top_msg != NULL) {
				g_object_unref (top_msg);
				dimmed = TRUE;
			}
			if (dimmed) {
				modest_dimming_rule_set_notification (rule, _CS("ckct_ib_unable_to_delete"));
			}
		}
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_details (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	GtkWidget *header_view = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	
	/* main window dimming rules */
	if (MODEST_IS_MAIN_WINDOW(win)) {
				
		/* Check dimmed rule */
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		
		/* If the header view has the focus: */
		if (header_view && gtk_widget_is_focus (header_view)) {
			/* Check dimmed rule */	
			if (!dimmed)
				dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE, user_data);
		}
		else {
			/* If the folder view has the focus: */
			GtkWidget *folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
				MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
			if (folder_view && gtk_widget_is_focus (folder_view)) {
				TnyFolderStore *folder_store
					= modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));
				if (folder_store) {
					/* Only enable for folders, not accounts,
					 * though the UI spec is not clear about that.
					 * If we enable this for accounts then we must 
					 * add code to handle them in modest_ui_actions_on_details(). */
					if (!TNY_IS_FOLDER(folder_store)) {
						dimmed = TRUE;
						modest_dimming_rule_set_notification (rule, "");
					}

					g_object_unref (folder_store);
				} else {
					dimmed = TRUE;
					modest_dimming_rule_set_notification (rule, "");
				}
				if (!dimmed) {
					dimmed = _msg_download_in_progress (win);
					if (dimmed)
						modest_dimming_rule_set_notification (rule, "");
				}

			} else {
				return TRUE;
			}
		}

	/* msg view window dimming rules */
	} else {
		/* Check dimmed rule */	
		if (MODEST_IS_MSG_VIEW_WINDOW (win))
			dimmed = _msg_download_in_progress (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_fetch_images (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	ModestDimmingRule *rule = NULL;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	dimmed = !modest_msg_view_window_has_blocked_external_images (MODEST_MSG_VIEW_WINDOW (win));

	if (!dimmed) {
		dimmed = _transfer_mode_enabled (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));
	}
	if (!dimmed) {
		dimmed = _msg_download_in_progress (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_reply"));
	}

	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_mark_as_read_msg_in_view (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	TnyHeader *header;
	TnyHeaderFlags flags;
	gboolean dimmed = FALSE;


	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));
	if (!header) {
		dimmed = TRUE;
	}

	/* If the viewer is showing a message sent as attachment */
	if (!dimmed)
		dimmed = !modest_msg_view_window_has_headers_model (MODEST_MSG_VIEW_WINDOW (win));

	if (!dimmed) {
		flags = tny_header_get_flags (header);
		if (flags & TNY_HEADER_FLAG_SEEN)
			dimmed = TRUE;
	}

	if (header)
		g_object_unref (header);
	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_mark_as_unread_msg_in_view (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	TnyHeader *header;
	TnyHeaderFlags flags;
	gboolean dimmed = FALSE;
	

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	
	header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));
	if (!header) {
		dimmed = TRUE;
	}

	/* If the viewer is showing a message sent as attachment */
	if (!dimmed)
		dimmed = !modest_msg_view_window_has_headers_model (MODEST_MSG_VIEW_WINDOW (win));

	if (!dimmed) {
		flags = tny_header_get_flags (header);
		if (!(flags & TNY_HEADER_FLAG_SEEN))
			dimmed = TRUE;
	}

	if (header)
		g_object_unref (header);
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_mark_as_read_msg (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	TnyHeaderFlags flags;
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);		
	
	flags = TNY_HEADER_FLAG_SEEN; 

	/* Check dimmed rule */	
	dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);	
	if (!dimmed) {
		if (state)
			dimmed = state->all_marked_as_seen;
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}	

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_mark_as_unread_msg (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	TnyHeaderFlags flags;
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);		
	
	flags = TNY_HEADER_FLAG_SEEN; 

	/* Check dimmed rule */	
	dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);
	if (!dimmed) {
		if (state)
			dimmed = !state->any_marked_as_seen;
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_move_to (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	if (MODEST_IS_MAIN_WINDOW (win)) 
		dimmed = modest_ui_dimming_rules_on_main_window_move_to (win, user_data);
#ifdef MODEST_TOOLKIT_HILDON2
	else if (MODEST_IS_HEADER_WINDOW (win))
		dimmed = modest_ui_dimming_rules_on_header_window_move_to (win, user_data);
	else if (MODEST_IS_FOLDER_WINDOW (win))
		dimmed = modest_ui_dimming_rules_on_folder_window_move_to (win, user_data);
#endif
	else if (MODEST_IS_MSG_VIEW_WINDOW (win)) 
		 dimmed = modest_ui_dimming_rules_on_view_window_move_to (win, user_data);

	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_main_window_move_to (ModestWindow *win, gpointer user_data)
{
	GtkWidget *folder_view = NULL;
	ModestDimmingRule *rule = NULL;
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), TRUE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);		
	
	/* Get the folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);

	if (folder_view) {
		TnyFolderStore *selected = modest_folder_view_get_selected ((ModestFolderView *)folder_view);
		if (selected) {
			TnyAccount *account = NULL;

			if (TNY_IS_ACCOUNT (selected)) {
				account = g_object_ref (selected);
			} else if (!TNY_IS_MERGE_FOLDER (selected)){
				account = tny_folder_get_account (TNY_FOLDER (selected));
			}
			if (account) {
				ModestProtocolType protocol_type;

				protocol_type = modest_tny_account_get_protocol_type (account);
				dimmed  = modest_protocol_registry_protocol_type_has_tag
					(modest_runtime_get_protocol_registry (),
					 protocol_type,
					 MODEST_PROTOCOL_REGISTRY_STORE_FORBID_OUTGOING_XFERS);

				g_object_unref (account);
			}
			g_object_unref (selected);
		}
	}

	/* Check diming rules for folders transfer  */
	if (!dimmed && folder_view && gtk_widget_is_focus (folder_view)) {
		TnyFolderType types[5];

		types[0] = TNY_FOLDER_TYPE_DRAFTS; 
		types[1] = TNY_FOLDER_TYPE_OUTBOX;
		types[2] = TNY_FOLDER_TYPE_SENT;
		types[3] = TNY_FOLDER_TYPE_ROOT; 
		types[4] = TNY_FOLDER_TYPE_INBOX; 
		
		/* Apply folder rules */	
		dimmed = _selected_folder_not_moveable (MODEST_MAIN_WINDOW(win));
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("emev_nc_unabletomove_items"));		
		if (!dimmed) {
			dimmed = _selected_folder_is_any_of_type (win, types, 5);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("emev_nc_unabletomove_items"));
		}
	}
	
	/* Check diming rules for messages transfer  */
	if (!dimmed) {
		if (state) {
			dimmed = (state->already_opened_msg > 0) ? TRUE : FALSE;
			if (dimmed) {
				gchar *message = g_strdup_printf(_("emev_nc_unabletomove_items"),
								 state->already_opened_msg);
				modest_dimming_rule_set_notification (rule, message);
				g_free(message);
			}
		}
	}
	if (!dimmed) {
		if (!(folder_view && gtk_widget_is_focus (folder_view)))
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);

	}
	if (!dimmed) {
		dimmed = _selected_msg_sent_in_progress (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("sfil_ib_unable_to_move_selected_items"));
	}

	return dimmed;
}

static gboolean
_forbid_outgoing_xfers (ModestWindow *window)
{
	const gchar *account_name = NULL;
	TnyAccount *account = NULL;
	gboolean dimmed = FALSE;

#ifdef MODEST_TOOLKIT_HILDON2
	/* We cannot just get the active account because the active
	   account of a header window that shows the headers of a
	   local account is the ID of the remote account */
	if (MODEST_IS_HEADER_WINDOW (window)) {
		ModestHeaderView *header_view;
		TnyFolder *folder;

		header_view = modest_header_window_get_header_view ((ModestHeaderWindow *) window);
		folder = modest_header_view_get_folder (header_view);

		if (folder) {
			account = modest_tny_folder_get_account (folder);
			g_object_unref (folder);
		}
	}
#endif

	if (!account) {
		account_name = modest_window_get_active_account (window);
		account = modest_tny_account_store_get_server_account (modest_runtime_get_account_store (),
								       account_name,
								       TNY_ACCOUNT_TYPE_STORE);
	}

	if (account) {
		ModestProtocolType protocol_type;

		protocol_type = modest_tny_account_get_protocol_type (account);
		dimmed  = modest_protocol_registry_protocol_type_has_tag
			(modest_runtime_get_protocol_registry (),
			 protocol_type,
			 MODEST_PROTOCOL_REGISTRY_STORE_FORBID_OUTGOING_XFERS);

		g_object_unref (account);
	}
	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_view_window_move_to (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* This could happen if we load the msg view window with a
	   preview before loading the full message */
	TnyMsg *msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
	if (!msg) {
		return TRUE;
	} else {
		g_object_unref (msg);
	}

	/* Check dimmed rule */
	dimmed = _transfer_mode_enabled (win);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));

	if (!dimmed)
		dimmed = _forbid_outgoing_xfers (win);

	if (!dimmed) {
		const DimmedState *state = modest_window_get_dimming_state (win);
		if (state) {
			dimmed = state->any_marked_as_deleted;
			if (dimmed) {
				gchar *msg = modest_ui_actions_get_msg_already_deleted_error_msg (win);
				modest_dimming_rule_set_notification (rule, msg);
				g_free (msg);
			}
		}
	}

	if (!dimmed) {
		dimmed = _selected_msg_sent_in_progress (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("emev_nc_unabletomove_item"));
	}

	/* This could happen if we're viewing a message of the outbox
	   that has been already sent */
	if (!dimmed)
		dimmed = _message_already_sent (MODEST_MSG_VIEW_WINDOW(win));

	if (!dimmed) {
		if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
			/* The move_to button should be dimmed when viewing an attachment,
			 * but should be enabled when viewing a message from the list, 
			 * or when viewing a search result.
			 */
			if (!modest_msg_view_window_is_search_result (MODEST_MSG_VIEW_WINDOW(win))) {
				dimmed = !modest_msg_view_window_has_headers_model (MODEST_MSG_VIEW_WINDOW (win));
			}
		}
		if (dimmed) 
			modest_dimming_rule_set_notification (rule, _("emev_nc_unabletomove_item"));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_find_in_msg (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* This could happen if we load the msg view window with a
	   preview before loading the full message */
	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		TnyMsg *msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
		if (!msg) {
			return TRUE;
		} else {
			g_object_unref (msg);
		}
	}

	/* Check dimmed rule */	
	dimmed = _transfer_mode_enabled (win);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));	

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_paste (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	TnyFolderType types[3];
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	
	/* Check dimmed rule */	
	dimmed = _clipboard_is_empty (win);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, 
						      _CS("ecoc_ib_edwin_nothing_to_paste"));
	if (!dimmed) {
		dimmed = _selected_folder_is_any_of_type (win, types, 3);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, 
							      _CS("ckct_ib_unable_to_paste_here"));
	}
	if (!dimmed) {
		dimmed = !_folder_view_has_focus (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule,
					_CS("ckct_ib_unable_to_paste_here"));
	}
	if (!dimmed) {
		dimmed = _selected_folder_not_writeable (MODEST_MAIN_WINDOW(win), TRUE);
		if (dimmed) 
			modest_dimming_rule_set_notification (rule, 
							      _CS("ckct_ib_unable_to_paste_here"));
	}
	if (!dimmed) {
		dimmed = _selected_folder_is_same_as_source (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_copy_target_error"));
	}
	if (!dimmed) {
		dimmed = _selected_folder_has_subfolder_with_same_name (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_in_ui_folder_copy_target_error"));
	}
	
	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_select_all (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	GtkWidget *focused = NULL;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	focused = gtk_window_get_focus (GTK_WINDOW (win));

	/* Main window dimming rules */	
	if (MODEST_IS_MAIN_WINDOW (win))
		dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));

	if (!dimmed && MODEST_IS_MAIN_WINDOW (win))
		dimmed = _header_view_is_all_selected (MODEST_MAIN_WINDOW(win));

	if (!dimmed && GTK_IS_ENTRY (focused)) {
		const gchar *current_text;
		current_text = gtk_entry_get_text (GTK_ENTRY (focused));
		dimmed = ((current_text == NULL) || (current_text[0] == '\0'));
	}

	if (!dimmed && GTK_IS_TEXT_VIEW (focused)) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused));
		dimmed = (gtk_text_buffer_get_char_count (buffer) < 1);
	}

	if (dimmed && MODEST_IS_ATTACHMENTS_VIEW (focused))
		dimmed = FALSE;
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_view_attachments (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	dimmed = _invalid_attach_selected (win, TRUE, TRUE, FALSE, rule);			

	if (!dimmed) {
		dimmed = _purged_attach_selected (win, FALSE, NULL);
		if (dimmed) {
			modest_dimming_rule_set_notification (rule, _("mail_ib_attach_not_local"));
		}
	}
		
	return dimmed;
}

#ifdef MODEST_TOOLKIT_HILDON2
static gboolean
_not_valid_attachments (ModestWindow *win, gboolean save_not_remove)
{
	gint n_attachments;
	TnyList *attachments;
	gboolean result = FALSE;

	/* Get atachments */
	attachments = modest_msg_view_window_get_attachments (MODEST_MSG_VIEW_WINDOW(win));
	n_attachments = tny_list_get_length (attachments);

	/* Check unique */		
	if (!result) {
		result = n_attachments < 1;
	}
		
	/* Check attached type (view operation not required) */
	if (!result)  {
		gint n_valid = 0;

		TnyIterator *iter;
		iter = tny_list_create_iterator (attachments);
		while (!tny_iterator_is_done (iter)) {
			gboolean is_valid = TRUE;
			TnyMimePart *mime_part = TNY_MIME_PART (tny_iterator_get_current (iter));
			TnyList *nested_list = tny_simple_list_new ();
			tny_mime_part_get_parts (mime_part, nested_list);

			if (tny_mime_part_is_purged (mime_part)) {
				is_valid = FALSE;
			}
			
			if (is_valid && modest_tny_mime_part_is_msg (mime_part)) {
				TnyMsg *window_msg;
				window_msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
				if (window_msg) {
					if (save_not_remove && (TnyMimePart *) window_msg != mime_part) {
						is_valid = FALSE;
					}
					g_object_unref (window_msg);
				}
				if (is_valid && save_not_remove && tny_list_get_length (nested_list) > 0) {
					is_valid = FALSE;
				}
			}
			g_object_unref (nested_list);
			g_object_unref (mime_part);
			tny_iterator_next (iter);

			if (is_valid) 
				n_valid++;
		}
		g_object_unref (iter);
		result = (n_valid == 0);
	}
	g_object_unref (attachments);
	return result;

}
#endif

gboolean 
modest_ui_dimming_rules_on_save_attachments (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	

#ifdef MODEST_TOOLKIT_HILDON2
	dimmed = _not_valid_attachments (win, TRUE);
#else
	dimmed = _invalid_attach_selected (win, FALSE, FALSE, FALSE, rule);

	if (!dimmed) {
		dimmed = _purged_attach_selected (win, TRUE, NULL);
		if (dimmed) {
			modest_dimming_rule_set_notification (rule, _("mail_ib_attach_not_local"));
		}
	}
#endif
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_remove_attachments (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);		

	/* Check in main window if there's only one message selected */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW (win), TRUE, rule);
	}

	/* Check in view window if there's any attachment selected */
	if (!dimmed && MODEST_IS_MSG_VIEW_WINDOW (win)) {
		dimmed = _invalid_attach_selected (win, FALSE, FALSE, TRUE, NULL);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("FIXME:no attachment selected"));
	}

	if (!dimmed) {

		dimmed = _selected_msg_sent_in_progress (win);
		if (dimmed) {
			modest_dimming_rule_set_notification (rule, _("mail_ib_unable_to_purge_attachments"));
		}
	}

	/* cannot purge in editable drafts nor pop folders */
	if (!dimmed) {
		dimmed = _invalid_folder_for_purge (win, rule);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_ib_unable_to_purge_attachments"));
	}

	/* Check if the selected message in main window has attachments */
	if (!dimmed && MODEST_IS_MAIN_WINDOW (win)) {
		if (state)
			dimmed = !(state->any_has_attachments);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_ib_unable_to_purge_attachments"));
	}

	/* Check if all attachments are already purged */
	if (!dimmed) {
		dimmed = _purged_attach_selected (win, TRUE, rule);
	}

	/* Check if the message is already downloaded */
	if (!dimmed && MODEST_IS_MAIN_WINDOW (win)) {
		dimmed = !_msg_download_completed (MODEST_MAIN_WINDOW (win));
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mail_ib_attach_not_local"));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_undo (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	if (MODEST_IS_MAIN_WINDOW (win)) {
		dimmed = _clipboard_is_empty (win); 
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	if (!dimmed && MODEST_IS_MSG_EDIT_WINDOW (win)) {
		dimmed = !modest_msg_edit_window_can_undo (MODEST_MSG_EDIT_WINDOW (win));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_redo (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	if (MODEST_IS_MSG_EDIT_WINDOW (win)) {
		dimmed = !modest_msg_edit_window_can_redo (MODEST_MSG_EDIT_WINDOW (win));
	}
				
	return dimmed;	
}

gboolean 
modest_ui_dimming_rules_on_cut (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);

	/* Check common dimming rules */
	dimmed = _invalid_clipboard_selected (win, rule);

	/* Check window specific dimming rules */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		/* Get focused widget */
		GtkWidget *focused = gtk_window_get_focus (GTK_WINDOW (win));
		
		if (MODEST_IS_HEADER_VIEW (focused)) {
			if (!dimmed) { 
				dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));			
				if (dimmed)
					modest_dimming_rule_set_notification (rule, "");
			}
			if (!dimmed) {
				dimmed = _selected_msg_sent_in_progress (win);
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_cut_mess"));
			}
			if (!dimmed) {
				if (state)
					dimmed = (state->already_opened_msg > 0) ? TRUE : FALSE;
				if(dimmed)
					modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_cut_mess"));
			}
		}
		else if (MODEST_IS_FOLDER_VIEW (focused)) {
			TnyFolderType types[3];
			
			types[0] = TNY_FOLDER_TYPE_DRAFTS; 
			types[1] = TNY_FOLDER_TYPE_OUTBOX;
			types[2] = TNY_FOLDER_TYPE_SENT;
			
			/* Apply folder rules */	
			if (!dimmed) {
				dimmed = _selected_folder_not_deletable (MODEST_MAIN_WINDOW(win));
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _("emev_nc_unabletomove_items"));
			}
			if (!dimmed) {
				dimmed = _selected_folder_is_root_or_inbox (MODEST_MAIN_WINDOW(win));
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _("emev_nc_unabletomove_items"));
			}
			if (!dimmed) {
				dimmed = _selected_folder_is_any_of_type (win, types, 3);
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _("emev_nc_unabletomove_items"));
			}
		}
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_copy (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);

	/* Check common dimming rules */
	dimmed = _invalid_clipboard_selected (win, rule);	
	
	/* Check window specific dimming rules */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		/* Get focused widget */
		GtkWidget *focused = gtk_window_get_focus (GTK_WINDOW (win));
		
		if (MODEST_IS_HEADER_VIEW (focused)) {
			if (!dimmed) {
				dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));			
				if (dimmed)
					modest_dimming_rule_set_notification (rule, "");
			}		
			if (!dimmed) {
				dimmed = _selected_msg_sent_in_progress (win);
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _(""));
			}
			if (!dimmed) {
				if (state)
					dimmed = (state->already_opened_msg > 0) ? TRUE : FALSE;
				if(dimmed)
					modest_dimming_rule_set_notification (rule, _(""));
			}
		}
		else if (MODEST_IS_FOLDER_VIEW (focused)) {
			TnyFolderType types[3];
			
			types[0] = TNY_FOLDER_TYPE_DRAFTS; 
			types[1] = TNY_FOLDER_TYPE_OUTBOX;
			types[2] = TNY_FOLDER_TYPE_SENT;

			if (!dimmed) {
				dimmed = _selected_folder_is_root (MODEST_MAIN_WINDOW(win));
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _(""));
			}
			if (!dimmed) {
				dimmed = _selected_folder_is_any_of_type (win, types, 3);
				if (dimmed)
					modest_dimming_rule_set_notification (rule, _(""));
			}
		}
	}
		
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_set_style (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (win), TRUE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check common dimming rules */
	ModestMsgEditFormat format;
	format = modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW (win));

	dimmed = (format != MODEST_MSG_EDIT_FORMAT_HTML);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mcen_ib_item_unavailable_plaintext"));

	if (!dimmed) {
		GtkWidget *body;
		body = modest_msg_edit_window_get_child_widget (MODEST_MSG_EDIT_WINDOW (win),
								MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BODY);
		
		dimmed = ((body == NULL)||(!gtk_widget_is_focus (body)));
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_move_cursor_to_message"));
	}
	       
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_zoom (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (win), TRUE);
	rule = MODEST_DIMMING_RULE (user_data);

	GtkWidget *body;
	body = modest_msg_edit_window_get_child_widget (MODEST_MSG_EDIT_WINDOW (win),
							MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BODY);
	
	dimmed = ((body == NULL)||(!gtk_widget_is_focus (body)));
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mcen_ib_move_cursor_to_message"));
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_editor_paste (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	GtkWidget *focused = NULL;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (win), TRUE);
	rule = MODEST_DIMMING_RULE (user_data);

	focused = gtk_window_get_focus (GTK_WINDOW (win));

	dimmed = MODEST_IS_ATTACHMENTS_VIEW (focused);
	
	if (!dimmed) {
		dimmed = GTK_IS_TOGGLE_BUTTON (focused);
	}

	if (!dimmed) {
		ModestEmailClipboard *e_clipboard = modest_runtime_get_email_clipboard ();
		const gchar *clipboard_text = modest_msg_edit_window_get_clipboard_text (MODEST_MSG_EDIT_WINDOW (win));

		dimmed = modest_email_clipboard_cleared (e_clipboard) && 
		  ((clipboard_text == NULL) || (clipboard_text[0] == '\0'));
	}
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_editor_remove_attachment (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	TnyList *selected_attachments = NULL;
	gint n_att_selected = 0;
	GtkWidget *attachments_view;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (win), TRUE);
	rule = MODEST_DIMMING_RULE (user_data);

	attachments_view = modest_msg_edit_window_get_child_widget (
								    MODEST_MSG_EDIT_WINDOW (win),
								    MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_ATTACHMENTS);
	
#ifdef MODEST_TOOLKIT_HILDON2
	selected_attachments = modest_attachments_view_get_attachments (
								      MODEST_ATTACHMENTS_VIEW (attachments_view));
#else
	selected_attachments = modest_attachments_view_get_selection (
								      MODEST_ATTACHMENTS_VIEW (attachments_view));
#endif
	n_att_selected = tny_list_get_length (selected_attachments);
	g_object_unref (selected_attachments);
	
	dimmed = (n_att_selected < 1);	
	
	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_send (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	GtkWidget *body_field;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (win), TRUE);
	rule = MODEST_DIMMING_RULE (user_data);

	body_field = modest_msg_edit_window_get_child_widget (MODEST_MSG_EDIT_WINDOW (win),
							      MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BODY);

	if (!dimmed) {
		GtkWidget *to_field, *cc_field, *bcc_field;
		GtkTextBuffer * to_buffer, *cc_buffer, *bcc_buffer;
		cc_field = modest_msg_edit_window_get_child_widget (MODEST_MSG_EDIT_WINDOW (win),
								    MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_CC);
		to_field = modest_msg_edit_window_get_child_widget (MODEST_MSG_EDIT_WINDOW (win),
								    MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_TO);
		bcc_field = modest_msg_edit_window_get_child_widget (MODEST_MSG_EDIT_WINDOW (win),
								     MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BCC);
		to_buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (to_field));
		cc_buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (cc_field));
		bcc_buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (bcc_field));

		dimmed = ((gtk_text_buffer_get_char_count (to_buffer) +
			   gtk_text_buffer_get_char_count (cc_buffer) +
			   gtk_text_buffer_get_char_count (bcc_buffer)) == 0);

		if (!dimmed) {
			if (modest_text_utils_no_recipient (to_buffer) &&
			    modest_text_utils_no_recipient (cc_buffer) &&
			    modest_text_utils_no_recipient (bcc_buffer))
				dimmed = TRUE;
		}

		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_add_recipients_first"));
	}
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_view_previous (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);

	/* Check dimmed rule */
	dimmed = _transfer_mode_enabled (win);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));
	
	if (!dimmed) {
		dimmed = modest_msg_view_window_first_message_selected (
				MODEST_MSG_VIEW_WINDOW(win));
		modest_dimming_rule_set_notification (rule, NULL);
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_view_next (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	dimmed = _transfer_mode_enabled (win);			
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));
	
	if (!dimmed) {
		dimmed = modest_msg_view_window_last_message_selected (MODEST_MSG_VIEW_WINDOW (win));
		modest_dimming_rule_set_notification (rule, NULL);
	}

	return dimmed;
}


gboolean
modest_ui_dimming_rules_on_tools_smtp_servers (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed;
	ModestAccountMgr *mgr;

	mgr = modest_runtime_get_account_mgr();
	dimmed = !modest_account_mgr_has_accounts(mgr, TRUE);

	/* Dimm it if we only have metaaccounts */
	if (!dimmed) {
		ModestProtocolRegistry *reg = modest_runtime_get_protocol_registry ();
		GSList *account_names = modest_account_mgr_account_names (mgr, TRUE);

		if (account_names) {
			ModestProtocolType store_protocol;
			gboolean found = FALSE;
			GSList *iter = account_names;
			const gchar *tag = MODEST_PROTOCOL_REGISTRY_MULTI_MAILBOX_PROVIDER_PROTOCOLS;

			while (iter && !found) {
				gchar* account_name;

				account_name = (gchar *) iter->data;
				store_protocol = modest_account_mgr_get_store_protocol (mgr, account_name);

				if (!modest_protocol_registry_protocol_type_has_tag (reg, store_protocol, tag))
					found = TRUE;
				else
					iter = g_slist_next (iter);
			}
			modest_account_mgr_free_account_names (account_names);
			dimmed = !found;
		}
	}

	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_cancel_sending (ModestWindow *win, gpointer user_data)
{
 	ModestDimmingRule *rule = NULL;
	TnyFolderType types[1];
	const DimmedState *state = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	state = modest_window_get_dimming_state (win);

	types[0] = TNY_FOLDER_TYPE_OUTBOX; 

	/* Check dimmed rules */	
	dimmed = !_selected_folder_is_any_of_type (win, types, 1);
	if (dimmed) 
		modest_dimming_rule_set_notification (rule, ""); 	
	if (!dimmed) {
		if (state)
			dimmed = !state->sent_in_progress;
 		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_cancel_sending_all (ModestWindow *win, gpointer user_data)
{
	/* We dim if no msg send is in progress (and then cancelling send all has no
	 * effect */
	return !_msgs_send_in_progress ();
}

gboolean 
modest_ui_dimming_rules_on_send_receive (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	ModestAccountMgr *mgr;
	const gchar* account_name;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	mgr = modest_runtime_get_account_mgr();

	/* Check dimmed rule */
	account_name = modest_window_get_active_account (win);

	if (account_name)
		dimmed = modest_account_mgr_account_is_busy (mgr, account_name);
	else
		dimmed = TRUE;

	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mcen_nc_no_email_acnts_defined"));

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_send_receive_all (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
 
	/* Check dimmed rule */	
	GSList *account_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr (), TRUE);
	if (g_slist_length (account_names) < 1)
		dimmed = TRUE;
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mcen_nc_no_email_acnts_defined"));

	modest_account_mgr_free_account_names (account_names);

	if (!dimmed) {
		dimmed = _send_receive_in_progress (win);
	}

	return dimmed;
}

#ifdef MODEST_TOOLKIT_HILDON2
gboolean
modest_ui_dimming_rules_on_add_to_contacts (ModestWindow *win, gpointer user_data)
{
	GSList *recipients = NULL;
	gboolean has_recipients_to_add;

 	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);

	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		TnyMsg *msg;

		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));

		/* Message is loaded asynchronously, so this could happen */
		if (!msg) {
			TnyHeader *header;

			header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));
			if (!header)
				return TRUE;

			recipients = modest_tny_msg_header_get_all_recipients_list (header);
			g_object_unref (header);
		} else {
			recipients = modest_tny_msg_get_all_recipients_list (msg);
			g_object_unref (msg);
		}
	} else if (MODEST_IS_MSG_EDIT_WINDOW (win)) {
		/* Check if there are pending addresses to add */
		return !modest_msg_edit_window_has_pending_addresses ((ModestMsgEditWindow *) win);
	}

	has_recipients_to_add = FALSE;

	if (recipients) {
		GSList *node;
		for (node = recipients; node != NULL; node = g_slist_next (node)) {
			const gchar *recipient = (const gchar *) node->data;
			if (modest_text_utils_validate_recipient (recipient, NULL)) {
				if (!modest_address_book_has_address (recipient)) {
					has_recipients_to_add = TRUE;
					break;
				}
			}
		}
		g_slist_foreach (recipients, (GFunc) g_free, NULL);
		g_slist_free (recipients);
	}

	return !has_recipients_to_add;
}
#else
gboolean
modest_ui_dimming_rules_on_add_to_contacts (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	GtkWidget *focused = NULL;

 	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	focused = gtk_window_get_focus (GTK_WINDOW (win));

	dimmed = !focused;

	if (!dimmed) {
		gchar *selection = NULL;
		if (GTK_IS_TEXT_VIEW (focused)) {
			GtkTextIter start, end;
			GtkTextBuffer *buffer = NULL;
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused));
			if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end)) {
				selection = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
			}
		} else if (GTK_IS_LABEL (focused)) {
			selection = modest_text_utils_label_get_selection (GTK_LABEL (focused));
		} else {
			gboolean do_check = TRUE;
			GtkClipboard *clipboard;
			if (GTK_IS_HTML (focused)) {
				const gchar *sel;
				int len = -1;
				sel = gtk_html_get_selection_html (GTK_HTML (focused), &len);
				do_check = !((sel == NULL) || (sel[0] == '\0'));
			} else if (MODEST_IS_ATTACHMENTS_VIEW (focused)) {
				do_check = FALSE;
			}
			if (do_check) {
				clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
				selection = gtk_clipboard_wait_for_text (clipboard);
			} else {
				selection = NULL;
			}
		}
		dimmed = !((selection != NULL) && (modest_text_utils_validate_recipient (selection, NULL)));
	}

	return dimmed;
}
#endif

/* *********************** static utility functions ******************** */


static gboolean
_selected_folder_not_writeable (ModestMainWindow *win,
				gboolean for_paste)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	ModestEmailClipboard *clipboard = NULL;
	ModestTnyFolderRules rules;
	gboolean is_local_acc = FALSE;
	gboolean xfer_folders = FALSE;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(parent_folder && TNY_IS_FOLDER(parent_folder))) {
		/* If it's the local account and its transfering folders, then do not dim */		
		if (TNY_IS_ACCOUNT (parent_folder)) {
			is_local_acc = modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (parent_folder));
			if (for_paste) {
				clipboard = modest_runtime_get_email_clipboard ();
				xfer_folders = modest_email_clipboard_folder_copied (clipboard);
			}
		}

		if (for_paste) 
			result = !(is_local_acc && xfer_folders); 
		else
			result = !is_local_acc;
		goto frees;		
	}
	
	/* Check dimmed rule */	
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent_folder));
	result = rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;

	/* free */
 frees:
	if (parent_folder != NULL)
		g_object_unref (parent_folder);

	return result;
}

static gboolean
_selected_folder_not_deletable (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	ModestTnyFolderRules rules;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(parent_folder && TNY_IS_FOLDER(parent_folder))) {
		result = TRUE;
		goto frees;		
	}
	
	/* Check dimmed rule */	
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent_folder));
	result = rules & MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE;

	/* free */
 frees:
	if (parent_folder != NULL)
		g_object_unref (parent_folder);

	return result;
}

static gboolean
_selected_folder_not_moveable (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	ModestTnyFolderRules rules;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(parent_folder && TNY_IS_FOLDER(parent_folder))) {
		result = TRUE;
		goto frees;		
	}
	
	/* Check dimmed rule */	
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent_folder));
	result = rules & MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE;

	/* free */
 frees:
	if (parent_folder != NULL)
		g_object_unref (parent_folder);

	return result;
}

static gboolean
_selected_folder_not_renameable (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	ModestTnyFolderRules rules;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(parent_folder && TNY_IS_FOLDER(parent_folder))) {
		result = TRUE;
		goto frees;		
	}
	
	/* Check dimmed rule */	
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent_folder));
	result = rules & MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE;

	/* free */
 frees:
	if (parent_folder != NULL)
		g_object_unref (parent_folder);

	return result;
}

static gboolean
_selected_folder_is_root_or_inbox (ModestMainWindow *win)
{
	TnyFolderType types[2];
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_ROOT; 
	types[1] = TNY_FOLDER_TYPE_INBOX; 

	/* Check folder type */
	result = _selected_folder_is_any_of_type (MODEST_WINDOW(win), types, 2);

	/* Check pop and MMC accounts */
	if (!result) {
		result = _selected_folder_is_root (win);
	}
		
	return result;
}


static gboolean
_selected_folder_is_root (ModestMainWindow *win)
{
	TnyFolderType types[1];
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* All accounts are root items: */
	GtkWidget *folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	if (folder_view) {					
		gboolean is_account = FALSE;
		TnyFolderStore *folder_store = 
			modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
		if (folder_store) {
			is_account = TNY_IS_ACCOUNT (folder_store);

			g_object_unref (folder_store);
			folder_store = NULL;
		}
		
		if (is_account)
			return TRUE;
	}
		
	/* Try something more precise: */
	types[0] = TNY_FOLDER_TYPE_ROOT; 

	/* Check folder type */
	result = _selected_folder_is_any_of_type (MODEST_WINDOW(win), types, 1);
		
	return result;
}

static gboolean
_header_view_is_all_selected (ModestMainWindow *win)
{
	const DimmedState *state = NULL;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);

	state = modest_window_get_dimming_state (MODEST_WINDOW(win));

	return (state) ? state->all_selected : TRUE;
}

static gboolean
_selected_folder_is_empty (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL, *header_view = NULL;
	TnyFolderStore *folder = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);

	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

	/* If no folder view, always dimmed */
	if (!folder_view || !header_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(folder && TNY_IS_FOLDER(folder))) {
		if (folder)
			g_object_unref (folder);
		return TRUE;
	}
	
	/* Check folder type */
	if (modest_header_view_is_empty (MODEST_HEADER_VIEW (header_view)) ||
	    tny_folder_get_all_count (TNY_FOLDER (folder)) == 0)
		result = TRUE;

	/* free */
	g_object_unref (folder);

	return result;
}

static gboolean
_folder_view_has_focus (ModestWindow *win)
{
	GtkWidget *folder_view = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	if (!folder_view)
		return FALSE;
	
	if (gtk_widget_is_focus(folder_view))
		return TRUE;

	return FALSE;
}

static gboolean
_selected_folder_is_same_as_source (ModestWindow *win)
{
	ModestEmailClipboard *clipboard = NULL;
	GtkWidget *folder_view = NULL;
	TnyFolderStore *folder = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	if (!folder_view)
		return FALSE;
	
	/* Get selected folder as destination folder */
	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(folder && TNY_IS_FOLDER(folder))) {
		result = FALSE;
		goto frees;
	}
	
	/* Check clipboard is cleared */
	clipboard = modest_runtime_get_email_clipboard ();
	if (modest_email_clipboard_cleared (clipboard)) {
		result = FALSE;
		goto frees;
	}
		
	/* Check source folder */
	result = modest_email_clipboard_check_source_folder (clipboard, TNY_FOLDER (folder));
	
	/* Free */
 frees:
	g_object_unref (folder);
	
	return result;
}

static gboolean
_selected_folder_is_any_of_type (ModestWindow *win,
				 TnyFolderType types[], 
				 guint ntypes)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *folder = NULL;
	TnyFolderType folder_type;
	guint i=0;
	gboolean result = FALSE;

	/*Get current folder */
	if (MODEST_IS_MAIN_WINDOW(win)) {

		/* Get folder view */
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		/* If no folder view, always dimmed */
		if (!folder_view)
			return FALSE;
	
		/* Get selected folder as parent of new folder to create */
		folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));

		if (!(folder && TNY_IS_FOLDER(folder))) {
			if (folder)
				g_object_unref (folder);
			return FALSE;
		}
		
		/* Check folder type */
		result = _folder_is_any_of_type (TNY_FOLDER(folder), types, ntypes);
	}
	else if (MODEST_IS_MSG_VIEW_WINDOW(win)) {
		folder_type = modest_msg_view_window_get_folder_type (MODEST_MSG_VIEW_WINDOW (win));
		for (i=0; i < ntypes; i++) {
			result = result || folder_type == types[i];
		}
	}


	/* free */
	if (folder)
		g_object_unref (folder);

	return result;	
}

static gboolean
_folder_is_any_of_type (TnyFolder *folder,
			TnyFolderType types[], 
			guint ntypes)
{
	TnyFolderType folder_type;
	gboolean result = FALSE;
	guint i;

	g_return_val_if_fail (TNY_IS_FOLDER(folder), FALSE);

	/* Get folder type */
	folder_type = modest_tny_folder_guess_folder_type (folder);
	if (folder_type == TNY_FOLDER_TYPE_INVALID)
		g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
	
	/* Check foler type */
	for (i=0; i < ntypes; i++) {
		result = result || folder_type == types[i];
	}

	return result;
}

static gboolean
_clipboard_is_empty (ModestWindow *win)
{
	gboolean result = FALSE;
	
	if (MODEST_IS_MAIN_WINDOW (win)) {
		ModestEmailClipboard *clipboard = NULL;
		clipboard = modest_runtime_get_email_clipboard ();
		if (modest_email_clipboard_cleared (clipboard)) 
		 result = TRUE;	 
	}

	return result;
}

static gboolean
_invalid_clipboard_selected (ModestWindow *win,
			     ModestDimmingRule *rule) 
{
	gboolean result = FALSE;
	GtkWidget *focused = NULL;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);

	/* Get focuesed widget */
	focused = gtk_window_get_focus (GTK_WINDOW (win));

	if (MODEST_IS_MSG_EDIT_WINDOW (win)) {
		gboolean has_selection = FALSE;
		if (GTK_IS_TEXT_VIEW (focused)) {
			GtkTextBuffer *buffer = NULL;
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused));
			has_selection = modest_text_utils_buffer_selection_is_valid (buffer);
		} else if (GTK_IS_EDITABLE (focused)) {
			has_selection = gtk_editable_get_selection_bounds (GTK_EDITABLE (focused), NULL, NULL);
		}
		result = !has_selection;
	} else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		if (focused) {
			MODEST_DEBUG_BLOCK (g_debug ("FOCUSED %s", g_type_name (G_TYPE_FROM_INSTANCE (focused))););
			if (GTK_IS_LABEL (focused) && 
			    !gtk_label_get_selection_bounds (GTK_LABEL (focused), NULL, NULL)) {
				result = TRUE;
			} else if (GTK_IS_TEXT_VIEW (focused)) {
				GtkTextBuffer *buffer;
				buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused));
				result = !gtk_text_buffer_get_has_selection (buffer);
			} else if (GTK_IS_HTML (focused)) {
				const gchar *sel;
				int len = -1;
				sel = gtk_html_get_selection_html (GTK_HTML (focused), &len);
				result = ((sel == NULL) || (sel[0] == '\0'));
			} else if (MODEST_IS_ATTACHMENTS_VIEW (focused)) {
				result = TRUE;
			} else {
				GtkClipboard *clipboard;
				gchar *selection;

				clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
				/* Get clipboard selection*/
				selection = gtk_clipboard_wait_for_text (clipboard);
				/* Check dimming */
				result = (selection == NULL);
				g_free (selection);
			} 
		} else {
			result = TRUE;
		}
		
		if (result)
			modest_dimming_rule_set_notification (rule, "");
		
	}		
	else if (MODEST_IS_MAIN_WINDOW (win)) {
		const DimmedState *state = NULL;

		/* Check dimming */
		state = modest_window_get_dimming_state (win);
		if (state)
			result = state->n_selected == 0;
		if (result)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_no_message_selected"));			
	}
	
	return result;
}


static gboolean
_invalid_attach_selected (ModestWindow *win,
			  gboolean unique,
			  gboolean for_view,
			  gboolean for_remove,
			  ModestDimmingRule *rule) 
{
	TnyList *attachments;
	gint n_selected;
	TnyHeaderFlags flags;
	gboolean nested_attachments = FALSE;
	gboolean selected_messages = FALSE;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);

	if (MODEST_IS_MAIN_WINDOW (win)) {
		flags = TNY_HEADER_FLAG_ATTACHMENTS;
		if (!result) {
			const DimmedState *state = NULL;
			state = modest_window_get_dimming_state (win);
			result = !state->any_has_attachments;
		}
	}
	else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		
		/* Get selected atachments */
		attachments = modest_msg_view_window_get_attachments (MODEST_MSG_VIEW_WINDOW(win));
		n_selected = tny_list_get_length (attachments);

		/* Check unique */		
		if (!result) {
			if (unique) 
				result = n_selected != 1;
			else
				
				result = n_selected < 1;
		}
		
		/* Check attached type (view operation not required) */
		if (!result && !for_view)  {
			TnyIterator *iter;
			iter = tny_list_create_iterator (attachments);
			while (!tny_iterator_is_done (iter) && !result) {
#ifdef MODEST_TOOLKIT_HILDON2
				gboolean not_selectable = FALSE;
#endif
				TnyMimePart *mime_part = TNY_MIME_PART (tny_iterator_get_current (iter));
				TnyList *nested_list = tny_simple_list_new ();
				tny_mime_part_get_parts (mime_part, nested_list);

				if (!for_remove && modest_tny_mime_part_is_msg (mime_part)) {
					TnyMsg *window_msg;
					window_msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
					if ((TnyMimePart *) window_msg != mime_part) {
						selected_messages = TRUE;
#ifdef MODEST_TOOLKIT_HILDON2
						not_selectable = TRUE;
#else
						result = TRUE;
#endif
					}
					g_object_unref (window_msg);
				}
				if (!for_remove && tny_list_get_length (nested_list) > 0) {
					nested_attachments = TRUE;
#ifdef MODEST_TOOLKIT_HILDON2
					not_selectable = TRUE;
#else
					result = TRUE;
#endif
				}
#ifdef MODEST_TOOLKIT_HILDON2
				if (not_selectable)
					n_selected --;
#endif
				g_object_unref (nested_list);
				g_object_unref (mime_part);
				tny_iterator_next (iter);
			}
			g_object_unref (iter);
		}

		/* No valid attachment available */
		if (n_selected == 0)
			result = TRUE;
		
		/* Set notifications */
		if (result && rule != NULL) {
			if (selected_messages) {
				modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_save_attach_mail"));
			} else if (nested_attachments) {
				modest_dimming_rule_set_notification (rule, _("FIXME:unable to save attachments with nested elements"));
			} else if (n_selected == 0) {
				modest_dimming_rule_set_notification (rule, _("FIXME:no attachment selected"));
			} else if (unique) {
				modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_display_more"));
			}
		}
		
		/* Free */
		g_object_unref (attachments);
	}

	return result;
}

static gboolean
_purged_attach_selected (ModestWindow *win, gboolean all, ModestDimmingRule *rule) 
{
	TnyList *attachments = NULL;
	TnyIterator *iter;
	gint purged = 0;
	gint n_attachments = 0;
	gboolean result = FALSE;

	/* This should check if _all_ the attachments are already purged. If only some
	 * of them are purged, then it does not cause dim as there's a confirmation dialog
	 * for removing only local attachments */

	/* Get selected atachments */
	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		attachments = modest_msg_view_window_get_attachments (MODEST_MSG_VIEW_WINDOW(win));
	} else if (MODEST_IS_MAIN_WINDOW (win)) {
		/* If we're in main window, we won't know if there are already purged attachments */
		return FALSE;
	}

	if (attachments == NULL)
		return FALSE;

	if (tny_list_get_length (attachments) == 0) {
		g_object_unref (attachments);
		return FALSE;
	}

	iter = tny_list_create_iterator (attachments);
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *mime_part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_purged (mime_part)) {
			purged++;
		}
		n_attachments++;
		g_object_unref (mime_part);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
		
	/* Free */
	g_object_unref (attachments);

	if (all)
		result = (purged == n_attachments);
	else
		result = (purged > 0);

	/* This string no longer exists, refer to NB#75415 for more info
	if (result && (rule != NULL))
		modest_dimming_rule_set_notification (rule, _("mail_ib_attachment_already_purged"));
	*/

	return result;
}

static gboolean
_invalid_msg_selected (ModestMainWindow *win,
		       gboolean unique,
		       ModestDimmingRule *rule) 
{
	GtkWidget *folder_view = NULL;
	const DimmedState *state = NULL;
	gboolean result = FALSE;
	gint n_selected = 0;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (rule), FALSE);

	state = modest_window_get_dimming_state (MODEST_WINDOW(win));
	if (state)
		n_selected = state->n_selected;

	/* Get folder view to check focus */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);

	/* Check dimmed rule (TODO: check focus on widgets */	
	if (!result) {
		result = ((n_selected == 0 ) ||
			  (gtk_widget_is_focus (folder_view)));
		if (result)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_no_message_selected"));
	}
	if (!result && unique) {
		result = n_selected > 1;
		if (result)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_select_one_message"));
	}

	return result;
}


static gboolean
_msg_download_in_progress (ModestWindow *win)
{
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW (win), FALSE);

	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		result = modest_msg_view_window_toolbar_on_transfer_mode (MODEST_MSG_VIEW_WINDOW(win));
	}
	else if (MODEST_IS_MAIN_WINDOW (win)) {
		result = modest_main_window_transfer_mode_enabled (MODEST_MAIN_WINDOW(win));
	}

	return result;
}

static gboolean
_msg_download_completed (ModestMainWindow *win)
{
	const DimmedState *state = modest_window_get_dimming_state (MODEST_WINDOW(win));
	return (state) ? state->any_marked_as_cached : TRUE;
}

static void 
fill_list_of_caches (gpointer key, gpointer value, gpointer userdata)
{
	GSList **send_queues = (GSList **) userdata;
	*send_queues = g_slist_prepend (*send_queues, value);
}

static gboolean
_selected_msg_sent_in_progress (ModestWindow *win)
{
	const DimmedState *state = modest_window_get_dimming_state (win);
	return (state) ? state->sent_in_progress : TRUE;
}


static gboolean
_invalid_folder_for_purge (ModestWindow *win, 
			   ModestDimmingRule *rule)
{
	TnyFolder *folder = NULL;
	TnyAccount *account = NULL;
	gboolean result = FALSE;

	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {

		/* Get folder and account of message */
		TnyMsg *msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
		g_return_val_if_fail(msg != NULL, TRUE); 			
		folder = tny_msg_get_folder (msg);	
		g_object_unref (msg);
		if (folder == NULL) {
			result = TRUE;
			goto frees;
		}
	} else if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
									      MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		if (!folder_view)
			return FALSE;
		folder = (TnyFolder *) modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));
		/* Could be a folder store */
		if (folder == NULL || ! TNY_IS_FOLDER (folder))
			goto frees;		
	} else {
		g_return_val_if_reached (FALSE);
	}
	account = modest_tny_folder_get_account (folder);
	if (account == NULL) goto frees; 			
		
	/* Check account */
	if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (account))) {
		TnyFolderType types[2];
		types[0] = TNY_FOLDER_TYPE_DRAFTS;
		types[1] = TNY_FOLDER_TYPE_OUTBOX;
		
		if (_selected_folder_is_any_of_type (win, types, 2)) {
			result = TRUE;
		}
	} else {
		ModestProtocolType protocol_type = modest_tny_account_get_protocol_type (TNY_ACCOUNT (account));
		/* If it's a remote folder then dim */
		if (modest_protocol_registry_protocol_type_has_tag (modest_runtime_get_protocol_registry (),
								    protocol_type,
								    MODEST_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS)) {
			result = TRUE;
		}
	}
	
frees:
	if (folder != NULL)
		g_object_unref (folder);
	if (account != NULL)
		g_object_unref (account);
	
	return result;
}

static gboolean
_transfer_mode_enabled (ModestWindow *win)
{
	gboolean result = FALSE;

        /* Check dimming */
        if (MODEST_IS_MSG_VIEW_WINDOW(win)) {
                result = modest_msg_view_window_transfer_mode_enabled (MODEST_MSG_VIEW_WINDOW (win));
        } else if (MODEST_IS_MAIN_WINDOW(win)) {
                result = modest_main_window_transfer_mode_enabled (MODEST_MAIN_WINDOW (win));
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_FOLDER_WINDOW (win)) {
		result = modest_folder_window_transfer_mode_enabled (MODEST_FOLDER_WINDOW (win));
	} else if (MODEST_IS_HEADER_WINDOW (win)) {
		result = modest_header_window_transfer_mode_enabled (MODEST_HEADER_WINDOW (win));
#endif
        } else {
                g_warning("_transfer_mode_enabled called with wrong window type");
        }

	return result;
}

static gboolean
_selected_folder_has_subfolder_with_same_name (ModestWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *folder = NULL;
	ModestEmailClipboard *clipboard = NULL;
	const gchar *folder_name = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), TRUE);

	/*Get current parent folder */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view) return FALSE;
	
	/* Get selected folder as parent of new folder to create */
	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));	
	if (!(folder && TNY_IS_FOLDER(folder))) goto frees;
	
	/* get modest clipboard and source folder */
	clipboard = modest_runtime_get_email_clipboard ();
	folder_name = modest_email_clipboard_get_folder_name (clipboard);
	if (folder_name == NULL) goto frees;

	/* Check source subfolders names */
	result = modest_tny_folder_has_subfolder_with_name (folder, folder_name,
							    TRUE);
		
	/* Free */
 frees:
	if (folder != NULL) 
		g_object_unref (folder);


	return result;
}

static gboolean
_all_msgs_in_sending_status (ModestHeaderView *header_view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean all_sending = TRUE;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			TnyHeader *header;

			gtk_tree_model_get (model, &iter,
					    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header,
					    -1);

			if (header) {
				if (modest_tny_all_send_queues_get_msg_status (header) !=
				    MODEST_TNY_SEND_QUEUE_SENDING)
					all_sending = FALSE;
				g_object_unref (header);
			}

		} while (all_sending && gtk_tree_model_iter_next (model, &iter));
	}
	return all_sending;
}

gboolean 
modest_ui_dimming_rules_on_save_to_drafts (ModestWindow *win, 
					   gpointer user_data)
{
	ModestDimmingRule *rule = NULL;

	g_return_val_if_fail (MODEST_MSG_EDIT_WINDOW (win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	return !modest_msg_edit_window_is_modified (MODEST_MSG_EDIT_WINDOW (win));
}

gboolean
modest_ui_dimming_rules_on_insert_image (ModestWindow *win,
					 gpointer user_data)
{
	g_return_val_if_fail (MODEST_MSG_EDIT_WINDOW (win), FALSE);

	ModestMsgEditFormat format =
	  modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW (win));

	return (format != MODEST_MSG_EDIT_FORMAT_HTML);
}

static gboolean 
_send_receive_in_progress (ModestWindow *win)
{
	ModestMailOperationQueue *queue;
	GSList *op_list, *node;
	gboolean found_send_receive;

	queue = modest_runtime_get_mail_operation_queue ();
	op_list = modest_mail_operation_queue_get_by_source (queue, G_OBJECT (win));

	found_send_receive = FALSE;
	for (node = op_list; node != NULL; node = g_slist_next (node)) {
		ModestMailOperation *op;

		op = (ModestMailOperation *) node->data;
		if (modest_mail_operation_get_type_operation (op) == MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE) {
			found_send_receive = TRUE;
			break;
		}
	}

	if (op_list) {
		g_slist_foreach (op_list, (GFunc) g_object_unref, NULL);
		g_slist_free (op_list);
	}

	return found_send_receive;
}

static gboolean
_msgs_send_in_progress (void)
{
	ModestCacheMgr *cache_mgr;
	GHashTable *send_queue_cache;
	ModestTnySendQueue *send_queue;
	GSList *send_queues = NULL, *node = NULL;
	gboolean found = FALSE;

	cache_mgr = modest_runtime_get_cache_mgr ();
	send_queue_cache = modest_cache_mgr_get_cache (cache_mgr,
						       MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);

	g_hash_table_foreach (send_queue_cache, (GHFunc) fill_list_of_caches, &send_queues);

	for (node = send_queues; node != NULL && !found; node = g_slist_next (node)) {
		send_queue = MODEST_TNY_SEND_QUEUE (node->data);

		/* Check if msg uid is being processed inside send queue */
		if (modest_tny_send_queue_sending_in_progress (send_queue)) {
			found = TRUE;
			break;
		}
	}

	g_slist_free (send_queues);

	return found;
}

/*****************************************************************************/
/********************** HILDON2 only dimming rules ***************************/
/*****************************************************************************/

#ifdef MODEST_TOOLKIT_HILDON2
gboolean 
modest_ui_dimming_rules_on_header_window_move_to (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_HEADER_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	dimmed = _transfer_mode_enabled (win);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));

	if (!dimmed)
		dimmed = _forbid_outgoing_xfers (win);

	if (!dimmed) {
		GtkWidget *header_view;
		TnyFolder *folder;

		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (win)));
		folder = modest_header_view_get_folder (MODEST_HEADER_VIEW (header_view));
		if (folder) {
			dimmed = (tny_folder_get_all_count (TNY_FOLDER (folder)) == 0) ||
				modest_header_view_is_empty (MODEST_HEADER_VIEW (header_view));

			if (!dimmed &&
			    (tny_folder_get_folder_type (TNY_FOLDER (folder)) == TNY_FOLDER_TYPE_OUTBOX)) {
				dimmed = _all_msgs_in_sending_status (MODEST_HEADER_VIEW (header_view));;
			}
			g_object_unref (folder);
		}
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_folder_window_move_to (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_FOLDER_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	dimmed = _transfer_mode_enabled (win);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));

	if (!dimmed)
		dimmed = _forbid_outgoing_xfers (win);

	if (!dimmed && MODEST_IS_FOLDER_WINDOW (win)) {
		ModestFolderView *folder_view;
		folder_view = modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (win));
		dimmed = !modest_folder_view_any_folder_fulfils_rules (folder_view,
								       MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE);
	}

	if (!dimmed) {
		dimmed = _transfer_mode_enabled (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_folder_window_delete (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_FOLDER_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	dimmed = _transfer_mode_enabled (win);
	if (dimmed)
		modest_dimming_rule_set_notification (rule, _("mail_ib_notavailable_downloading"));

	if (MODEST_IS_FOLDER_WINDOW (win)) {
		ModestFolderView *folder_view;
		folder_view = modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (win));
		dimmed = !modest_folder_view_any_folder_fulfils_rules (folder_view,
								       MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE);
	}

	if (!dimmed) {
		dimmed = _transfer_mode_enabled (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, "");
	}

	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_edit_accounts (ModestWindow *win, gpointer user_data)
{
	return !modest_account_mgr_has_accounts (modest_runtime_get_account_mgr (), TRUE);
}
#endif

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
#include "modest-dimming-rule.h"
#include "modest-tny-folder.h"
#include "modest-text-utils.h"
#include <widgets/modest-attachments-view.h>
#include <modest-runtime.h>
#include <tny-simple-list.h>


static gboolean _folder_is_any_of_type (TnyFolder *folder, TnyFolderType types[], guint ntypes);
static gboolean _invalid_msg_selected (ModestMainWindow *win, gboolean unique, ModestDimmingRule *rule);
static gboolean _invalid_attach_selected (ModestWindow *win, gboolean unique, ModestDimmingRule *rule);
static gboolean _invalid_clipboard_selected (ModestMsgViewWindow *win);
static gboolean _already_opened_msg (ModestWindow *win);
static gboolean _selected_msg_marked_as (ModestWindow *win, TnyHeaderFlags mask, gboolean opposite);
static gboolean _selected_folder_not_writeable (ModestMainWindow *win);
static gboolean _selected_folder_is_any_of_type (ModestWindow *win, TnyFolderType types[], guint ntypes);
static gboolean _selected_folder_is_root_or_inbox (ModestMainWindow *win);
static gboolean _selected_folder_is_MMC_or_POP_root (ModestMainWindow *win);
static gboolean _selected_folder_is_root (ModestMainWindow *win);
static gboolean _selected_folder_is_empty (ModestMainWindow *win);
static gboolean _msg_download_in_progress (ModestMsgViewWindow *win);
static gboolean _msg_download_completed (ModestMainWindow *win);
static gboolean _selected_msg_sent_in_progress (ModestWindow *win);
static gboolean _sending_in_progress (ModestWindow *win);
static gboolean _marked_as_deleted (ModestWindow *win);


gboolean 
modest_ui_dimming_rules_on_new_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _msg_download_in_progress (MODEST_MSG_VIEW_WINDOW(win));

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_new_folder (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);


	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!parent_folder)
		return TRUE;
	
	if (TNY_IS_ACCOUNT (parent_folder)) {
		/* If it's the local account then do not dim */
		if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (parent_folder))) {
			dimmed = FALSE;
		} else {
			const gchar *proto_str = tny_account_get_proto (TNY_ACCOUNT (parent_folder));
			/* If it's POP then dim */
			dimmed = (modest_protocol_info_get_transport_store_protocol (proto_str) == 
				  MODEST_PROTOCOL_STORE_POP) ? TRUE : FALSE;
		}
	} else {
		/* TODO: the specs say that only one level of subfolder is allowed, is this true ? */
		
		TnyFolderType types[3];
				
		types[0] = TNY_FOLDER_TYPE_DRAFTS; 
		types[1] = TNY_FOLDER_TYPE_OUTBOX;
		types[2] = TNY_FOLDER_TYPE_SENT;

		/* Apply folder rules */	
		if (!dimmed)
			dimmed = _selected_folder_not_writeable (MODEST_MAIN_WINDOW(win));
		if (!dimmed)
			dimmed = _selected_folder_is_any_of_type (win, types, 3);
	}
	g_object_unref (parent_folder);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_delete_folder (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	TnyFolderType types[3];

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_not_writeable (MODEST_MAIN_WINDOW(win));
	if (!dimmed)
		dimmed = _selected_folder_is_root_or_inbox (MODEST_MAIN_WINDOW(win));
	if (!dimmed)
		dimmed = _selected_folder_is_MMC_or_POP_root (MODEST_MAIN_WINDOW(win));
	if (!dimmed)
		dimmed = _selected_folder_is_any_of_type (win, types, 3);

	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_sort (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_is_root (MODEST_MAIN_WINDOW(win));

	return dimmed;
	
}

gboolean 
modest_ui_dimming_rules_on_rename_folder (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	TnyFolderType types[3];

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_not_writeable (MODEST_MAIN_WINDOW(win));
	if (!dimmed)
		dimmed = _selected_folder_is_root_or_inbox (MODEST_MAIN_WINDOW(win));
	if (!dimmed)
		dimmed = _selected_folder_is_any_of_type (win, types, 3);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_open_msg (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE, user_data);
	if (!dimmed) {
		dimmed = _selected_msg_sent_in_progress (win);
		if (dimmed)
			modest_dimming_rule_set_notification (rule, _("TEST"));
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

	/* main window dimming rules */
	if (MODEST_IS_MAIN_WINDOW(win)) {
		
		types[0] = TNY_FOLDER_TYPE_DRAFTS; 
		types[1] = TNY_FOLDER_TYPE_OUTBOX;
		types[2] = TNY_FOLDER_TYPE_ROOT;
		
		/* Check dimmed rule */	
		if (!dimmed) {
			dimmed = _selected_folder_is_any_of_type (win, types, 3);			
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_reply"));
		}
		if (!dimmed) {
			dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));			
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_nothing_to_reply"));
		}
		if (!dimmed)
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE, rule);

	/* msg view window dimming rules */
	} else if (MODEST_IS_MSG_VIEW_WINDOW(win)) {
		
		/* Check dimmed rule */	
		if (!dimmed)
			dimmed = _msg_download_in_progress (MODEST_MSG_VIEW_WINDOW(win));
	}
	
	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_contents_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);
	if (!dimmed)
		dimmed = _msg_download_completed (MODEST_MAIN_WINDOW(win));

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_always_dimmed (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);
		
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = TRUE;

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_delete_msg (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
	
	/* Check dimmed rule */		
	if (MODEST_IS_MAIN_WINDOW (win)) {
		if (!dimmed) {
			dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));			
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_nothing_to_del"));
		}
		if (!dimmed) {
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_no_message_selected"));
		}
		if (!dimmed) {
			dimmed = _already_opened_msg (win);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_nc_unable_to_delete_n_messages"));
		}
		if (!dimmed) {
			dimmed = _marked_as_deleted (win);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_message_unableto_delete"));
		}
		if (!dimmed) {
			dimmed = _selected_msg_sent_in_progress (win);
			if (dimmed)
				modest_dimming_rule_set_notification (rule, _("mcen_ib_message_unableto_delete"));
		}
	} 
	else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		if (!dimmed) {
			dimmed = !modest_msg_view_window_has_headers_model (MODEST_MSG_VIEW_WINDOW(win));
		}
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_details (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	GtkWidget *header_view = NULL;
	
	/* main window dimming rules */
	if (MODEST_IS_MAIN_WINDOW(win)) {
				
		/* Check dimmed rule */
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
								   MODEST_WIDGET_TYPE_HEADER_VIEW);
		
		/* If the header view does not have the focus then do
		   not apply msg dimming rules because the action will
		   show the folder details that have no dimming
		   rule */
		if (gtk_widget_is_focus (header_view)) {
			/* Check dimmed rule */	
			if (!dimmed)
				dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), TRUE, user_data);
		}

	/* msg view window dimming rules */
	} else {

		/* Check dimmed rule */	
		if (!dimmed)
			dimmed = _msg_download_in_progress (MODEST_MSG_VIEW_WINDOW(win));
	}

	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_mark_as_read_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	TnyHeaderFlags flags;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	
	flags = TNY_HEADER_FLAG_SEEN; 

	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);
	if (!dimmed) 
		dimmed = _selected_msg_marked_as (win, flags, FALSE);
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_mark_as_unread_msg (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;
	TnyHeaderFlags flags;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	
	flags = TNY_HEADER_FLAG_SEEN; 

	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);
	if (!dimmed) 
		dimmed = _selected_msg_marked_as (win, flags, TRUE);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_move_to (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	if (MODEST_IS_MAIN_WINDOW (win)) 
		dimmed = modest_ui_dimming_rules_on_main_window_move_to (win, user_data);
	else if (MODEST_IS_MSG_VIEW_WINDOW (win)) 
		 dimmed = modest_ui_dimming_rules_on_view_window_move_to (win, user_data);

	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_main_window_move_to (ModestWindow *win, gpointer user_data)
{
	GtkWidget *folder_view = NULL;
	GtkWidget *header_view = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), TRUE);
	
	/* Get the folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);

	/* Get header view */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Check common diming rules */

	/* Check diming rules for folder transfer  */
	if (gtk_widget_is_focus (folder_view)) {
		if (!dimmed) 
			dimmed = _selected_folder_not_writeable(MODEST_MAIN_WINDOW(win));
	}
	/* Check diming rules for msg transfer  */
	else if (gtk_widget_is_focus (header_view)) {
		if (!dimmed)
			dimmed = _invalid_msg_selected (MODEST_MAIN_WINDOW(win), FALSE, user_data);
		
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_view_window_move_to (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);

	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = !modest_msg_view_window_has_headers_model (MODEST_MSG_VIEW_WINDOW(win));
	
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_paste_msgs (ModestWindow *win, gpointer user_data)
{
	TnyFolderType types[3];
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_is_any_of_type (win, types, 3);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_delete_msgs (ModestWindow *win, gpointer user_data)
{
	TnyFolderType types[5];
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_DRAFTS; 
	types[1] = TNY_FOLDER_TYPE_OUTBOX;
	types[2] = TNY_FOLDER_TYPE_SENT;
	types[3] = TNY_FOLDER_TYPE_INBOX;
	types[4] = TNY_FOLDER_TYPE_ROOT;
	
	/* Check dimmed rule */	
	if (!dimmed)
		dimmed = _selected_folder_is_any_of_type (win, types, 5);

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_select_all (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Check dimmed rule */	
	if (!dimmed) 
		dimmed = _selected_folder_is_empty (MODEST_MAIN_WINDOW(win));			
		
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
	if (!dimmed) 
		dimmed = _invalid_attach_selected (win, TRUE, rule);			
		
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_save_attachments (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	if (!dimmed) 
		dimmed = _invalid_attach_selected (win, FALSE, rule);			
		
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_remove_attachments (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */	
	if (!dimmed) {
		dimmed = _invalid_attach_selected (win, FALSE, NULL);			
		modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_display_more"));
	}

	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_copy (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);

	/* Check dimmed rule */	
	if (!dimmed) 
		dimmed = _invalid_clipboard_selected (MODEST_MSG_VIEW_WINDOW(win));			
		
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_view_previous (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);

	/* Check dimmed rule */	
	if (!dimmed) 
		dimmed = modest_msg_view_window_first_message_selected (MODEST_MSG_VIEW_WINDOW(win));
		
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_view_next (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win), FALSE);

	/* Check dimmed rule */	
	if (!dimmed) 
		dimmed = modest_msg_view_window_last_message_selected (MODEST_MSG_VIEW_WINDOW(win));
		
	return dimmed;
}


gboolean 
modest_ui_dimming_rules_on_tools_smtp_servers (ModestWindow *win, gpointer user_data)
{
	const gboolean dimmed = 
		!modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), 
			TRUE);	
		
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_cancel_sending (ModestWindow *win, gpointer user_data)
{
	gboolean dimmed = FALSE;

	/* Check dimmed rule */	
	if (!dimmed) 
		dimmed = !_sending_in_progress (win);
		
	return dimmed;
}

gboolean 
modest_ui_dimming_rules_on_send_receive (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;
	
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);
 
	/* Check dimmed rule */	
	if (!dimmed) {
		dimmed = !modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), 
							  TRUE);	
		modest_dimming_rule_set_notification (rule, _("mcen_nc_no_email_acnts_defined"));
	}

	return dimmed;
}

gboolean
modest_ui_dimming_rules_on_add_to_contacts (ModestWindow *win, gpointer user_data)
{
	ModestDimmingRule *rule = NULL;
	gboolean dimmed = FALSE;

	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (user_data), FALSE);
	rule = MODEST_DIMMING_RULE (user_data);

	/* Check dimmed rule */
	if (!dimmed) {
		GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
		gchar *selection = NULL;
		selection = gtk_clipboard_wait_for_text (clipboard);

		dimmed = !((selection != NULL) && (modest_text_utils_validate_recipient (selection)));
	}

	return dimmed;
}

/* *********************** static utility functions ******************** */

static gboolean 
_marked_as_deleted (ModestWindow *win)
{
	gboolean result = FALSE;
	TnyHeaderFlags flags;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	
	flags = TNY_HEADER_FLAG_DELETED; 

	/* Check dimmed rule */	
	result = _selected_msg_marked_as (win, flags, FALSE);
	
	return result;
}

static gboolean
_selected_folder_not_writeable (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	ModestTnyFolderRules rules;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(parent_folder && TNY_IS_FOLDER(parent_folder))) {
		if (parent_folder)
			g_object_unref (parent_folder);
		return TRUE;
	}
	
	/* Check dimmed rule */	
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent_folder));
	result = rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;

	/* free */
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
		result = _selected_folder_is_MMC_or_POP_root (win);
	}
		
	return result;
}


static gboolean
_selected_folder_is_root (ModestMainWindow *win)
{
	TnyFolderType types[1];
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	types[0] = TNY_FOLDER_TYPE_ROOT; 

	/* Check folder type */
	result = _selected_folder_is_any_of_type (MODEST_WINDOW(win), types, 1);
		
	/* Check pop and MMC accounts */
	if (!result) {
		result = _selected_folder_is_MMC_or_POP_root (win);
	}

	return result;
}

static gboolean
_selected_folder_is_MMC_or_POP_root (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *parent_folder = NULL;
	gboolean result = FALSE;

	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* Get selected folder as parent of new folder to create */
	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!parent_folder)
		return TRUE;
	
	if (TNY_IS_ACCOUNT (parent_folder)) {
		/* If it's the local account then do not dim */
		if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (parent_folder))) {
			result = FALSE;
		} else {
				/* If it's the MMC root folder then dim it */
			if (!strcmp (tny_account_get_id (TNY_ACCOUNT (parent_folder)), MODEST_MMC_ACCOUNT_ID)) {
					result = TRUE;
			} else {
				const gchar *proto_str = tny_account_get_proto (TNY_ACCOUNT (parent_folder));
				/* If it's POP then dim */
				result = (modest_protocol_info_get_transport_store_protocol (proto_str) == 
						  MODEST_PROTOCOL_STORE_POP) ? TRUE : FALSE;
			}
		}
	}
	g_object_unref (parent_folder);

	return result;
}




static gboolean
_selected_folder_is_empty (ModestMainWindow *win)
{
	GtkWidget *folder_view = NULL;
	TnyFolderStore *folder = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);

	/* Get folder view */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	/* If no folder view, always dimmed */
	if (!folder_view)
		return TRUE;
	
	/* Get selected folder as parent of new folder to create */
	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	if (!(folder && TNY_IS_FOLDER(folder))) {
		if (folder)
			g_object_unref (folder);
		return TRUE;
	}
	
	/* Check folder type */
	result = tny_folder_get_all_count (TNY_FOLDER (folder)) == 0;

	/* free */
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

	/*Get curent folder */
	if (MODEST_IS_MAIN_WINDOW(win)) {

		/* Get folder view */
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
								   MODEST_WIDGET_TYPE_FOLDER_VIEW);
		/* If no folder view, always dimmed */
		if (!folder_view)
			return TRUE;
	
		/* Get selected folder as parent of new folder to create */
		folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));

		if (!(folder && TNY_IS_FOLDER(folder))) {
			if (folder)
				g_object_unref (folder);
			return TRUE;
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
	if (modest_tny_folder_is_local_folder (folder))
		folder_type = modest_tny_folder_get_local_folder_type (folder);		
	else 
		folder_type = modest_tny_folder_guess_folder_type (folder);		
	
	/* Check foler type */
	for (i=0; i < ntypes; i++) {
		result = result || folder_type == types[i];
	}

	return result;
}



static gboolean
_invalid_clipboard_selected (ModestMsgViewWindow *win)
{
	GtkClipboard *clipboard = NULL;
	gchar *selection = NULL;
	GtkWidget *focused = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win), TRUE);

	/* Get clipboard selection*/
	clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	selection = gtk_clipboard_wait_for_text (clipboard);

	/* Get focuesed widget */
	focused = gtk_window_get_focus (GTK_WINDOW (win));

	/* Check dimming */
	result = ((selection == NULL) || 
		  (MODEST_IS_ATTACHMENTS_VIEW (focused)));
		  
	return result;
}

static gboolean
_invalid_attach_selected (ModestWindow *win,
			  gboolean unique,
			  ModestDimmingRule *rule) 
{
	GList *attachments, *node;
	gint n_selected;
	TnyHeaderFlags flags;
	gboolean nested_attachments = FALSE;
	gboolean selected_messages = FALSE;
	gboolean result = FALSE;

	if (MODEST_IS_MAIN_WINDOW (win)) {
		flags = TNY_HEADER_FLAG_ATTACHMENTS;
		if (!result)
			result = _selected_msg_marked_as (win, flags, TRUE);
		
	}
	else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		
		/* Get selected atachments */
		attachments = modest_msg_view_window_get_attachments (MODEST_MSG_VIEW_WINDOW(win));
		n_selected = g_list_length (attachments);
		for (node = attachments; node != NULL; node = g_list_next (node)) {
			TnyMimePart *mime_part = TNY_MIME_PART (node->data);
			TnyList *nested_list = tny_simple_list_new ();
			if (!tny_mime_part_is_attachment (mime_part)) {
				selected_messages = TRUE;
				break;
			}
			tny_mime_part_get_parts (mime_part, nested_list);
			if (tny_list_get_length (nested_list) > 0)
				nested_attachments = TRUE;
			g_object_unref (nested_list);
		}
		
		/* Check unique */
		if (unique) 
			result = n_selected != 1;
		else
			
			result = n_selected < 1;
		
		/* Set notifications */
		if (!result && rule != NULL) {
			if (selected_messages) {
				modest_dimming_rule_set_notification (rule, _("mcen_ib_unable_to_save_attach_mail"));
			} else if (nested_attachments) {
				modest_dimming_rule_set_notification (rule, _("FIXME:unable to save attachments with nested elements"));
			} else if (n_selected == 0) {
				modest_dimming_rule_set_notification (rule, _("FIXME:no attachment selected"));
			}
		}
		
		/* Free */
		g_list_free (attachments);
	}

	return result;
}

static gboolean
_invalid_msg_selected (ModestMainWindow *win,
		       gboolean unique,
		       ModestDimmingRule *rule) 
{
	GtkWidget *header_view = NULL;		
	GtkWidget *folder_view = NULL;
	TnyList *selected_headers = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
	g_return_val_if_fail (MODEST_IS_DIMMING_RULE (rule), FALSE);
		
	/* Get header view to check selected messages */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);
	
	/* Get folder view to check focus */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);

	/* Get selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));

	/* Check dimmed rule (TODO: check focus on widgets */	
	if (!result) {
		result = (selected_headers == NULL);
/* 		result = ((selected_headers == NULL) ||  */
/* 			  (GTK_WIDGET_HAS_FOCUS (folder_view))); */
		if (result)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_no_message_selected"));
	}
	if (!result && unique) {
		result = tny_list_get_length (selected_headers) > 1;
		if (result)
			modest_dimming_rule_set_notification (rule, _("mcen_ib_select_one_message"));
	}

	/* free */
	if (selected_headers != NULL) 
		g_object_unref (selected_headers);

	return result;
}

static gboolean
_already_opened_msg (ModestWindow *win)
{
	ModestWindow *window = NULL;
	ModestWindowMgr *mgr = NULL;
	GtkWidget *header_view = NULL;		
	TnyList *selected_headers = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	gboolean result = TRUE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(win), FALSE);
		
	/* Get header view to check selected messages */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Get selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
	if (selected_headers == NULL) 
		return FALSE;

	/* Check dimmed rule (TODO: check focus on widgets */	
	mgr = modest_runtime_get_window_mgr ();
	iter = tny_list_create_iterator (selected_headers);
	while (!tny_iterator_is_done (iter) && result) {
		header = TNY_HEADER (tny_iterator_get_current (iter));
		window = modest_window_mgr_find_window_by_header (mgr, header);
		result = result && (window != NULL);
			
		g_object_unref (header);
		tny_iterator_next (iter);
	}
	
	/* free */
	if (selected_headers != NULL) 
		g_object_unref (selected_headers);
	if (iter != NULL)
		g_object_unref (iter);
		
	return result;
}

static gboolean
_selected_msg_marked_as (ModestWindow *win, 
			 TnyHeaderFlags mask, 
			 gboolean opposite)
{
	GtkWidget *header_view = NULL;
	TnyList *selected_headers = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	TnyHeaderFlags flags;
	gboolean result = FALSE;

	/* Get header view to check selected messages */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Get selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
	if (selected_headers == NULL) 
		return TRUE;
	
	/* Call the function for each header */
	iter = tny_list_create_iterator (selected_headers);
	while (!tny_iterator_is_done (iter) && !result) {
		header = TNY_HEADER (tny_iterator_get_current (iter));

		flags = tny_header_get_flags (header);
		if (opposite)
			result = (flags & mask) == 0; 
		else
			result = (flags & mask) != 0; 

		g_object_unref (header);
		tny_iterator_next (iter);
	}

	/* free */
	if (selected_headers != NULL) 
		g_object_unref (selected_headers);
	if (iter != NULL)
		g_object_unref (iter);

	return result;
}

static gboolean
_msg_download_in_progress (ModestMsgViewWindow *win)
{
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win), FALSE);

	result = modest_msg_view_window_toolbar_on_transfer_mode (win);

	return result;
}

static gboolean
_msg_download_completed (ModestMainWindow *win)
{
	GtkWidget *header_view = NULL;
	TnyList *selected_headers = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	TnyHeaderFlags flags;
	gboolean result = FALSE;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (win), FALSE);


	/* Get header view to check selected messages */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Get selected headers */
	selected_headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
	if (selected_headers == NULL) 
		return TRUE;

	/* Check dimmed rule  */	
	result = TRUE;
	iter = tny_list_create_iterator (selected_headers);
	while (!tny_iterator_is_done (iter) && result) {
		header = TNY_HEADER (tny_iterator_get_current (iter));
			
		flags = tny_header_get_flags (header);
		/* TODO: is this the right flag?, it seems that some
		   headers that have been previously downloaded do not
		   come with it */
		result = (flags & TNY_HEADER_FLAG_CACHED);

		g_object_unref (header);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);

	return result;
}

static gboolean
_selected_msg_sent_in_progress (ModestWindow *win)
{
	ModestTnySendQueue *send_queue = NULL;
	GtkWidget *header_view = NULL;
	ModestTnyAccountStore *acc_store = NULL;
	TnyAccount *account = NULL;
	TnyList *header_list = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	const gchar *account_name = NULL;
	gboolean result = FALSE;
	gchar *id = NULL;
	
	/* Get transport account */
	acc_store = modest_runtime_get_account_store();
	account_name = modest_window_get_active_account (win);
	
	/* If no account defined, this action must be always dimmed  */
	if (account_name == NULL) return FALSE;
	account = modest_tny_account_store_get_transport_account_for_open_connection (acc_store, account_name);
	if (!TNY_IS_TRANSPORT_ACCOUNT (account)) return FALSE;

	/* Get send queue for current ransport account */
	send_queue = modest_runtime_get_send_queue (TNY_TRANSPORT_ACCOUNT(account));
	g_return_val_if_fail (MODEST_IS_TNY_SEND_QUEUE (send_queue), FALSE);

	if (MODEST_IS_MAIN_WINDOW(win)) {
		
		/* Get header view to check selected messages */
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
								   MODEST_WIDGET_TYPE_HEADER_VIEW);
		
		/* Get selected headers */
		header_list = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));

		/* Get message header */
		if (!header_list) return FALSE;
		iter = tny_list_create_iterator (header_list);
		header = TNY_HEADER (tny_iterator_get_current (iter));

		/* Get message id */
		id = g_strdup(tny_header_get_message_id (header));
		
        } else if (MODEST_IS_MSG_VIEW_WINDOW(win)) {
		
		/* Get message header */
		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW(win));

		/* Get message id */
		id = g_strdup(tny_header_get_message_id (header));
	}

	/* Check if msg id is being processed inside send queue */
	result = (modest_tny_send_queue_get_msg_status (send_queue, tny_header_get_message_id(header)) == MODEST_TNY_SEND_QUEUE_SENDING);

	/* Free */
	g_free(id);
	g_object_unref (header);
	g_object_unref(header_list);
	g_object_unref(iter);

	return result;
}


static gboolean
_sending_in_progress (ModestWindow *win)
{
	ModestTnySendQueue *send_queue = NULL;
	ModestTnyAccountStore *acc_store = NULL;
	TnyAccount *account = NULL;
	const gchar *account_name = NULL;
	gboolean result = FALSE;
	
	/* Get transport account */
	acc_store = modest_runtime_get_account_store();
	account_name = modest_window_get_active_account (win);

	/* If no account defined, this action must be always dimmed  */
	if (account_name == NULL) return FALSE;
	account = modest_tny_account_store_get_transport_account_for_open_connection (acc_store, account_name);
	if (!TNY_IS_TRANSPORT_ACCOUNT (account)) return FALSE;

	/* Get send queue for current ransport account */
	send_queue = modest_runtime_get_send_queue (TNY_TRANSPORT_ACCOUNT(account));
	g_return_val_if_fail (MODEST_IS_TNY_SEND_QUEUE (send_queue), FALSE);

	/* Check if send queue is perfimring any send operation */
	result = modest_tny_send_queue_sending_in_progress (send_queue);

	return result;
}

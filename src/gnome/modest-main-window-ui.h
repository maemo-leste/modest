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

#ifndef __MODEST_MAIN_WINDOW_UI_PRIV_H__
#define __MODEST_MAIN_WINDOW_UI_PRIV_H__

#include <glib/gi18n.h>
#include "modest-icon-names.h"
#include "modest-ui-actions.h"

G_BEGIN_DECLS


/* Action entries */
static const GtkActionEntry modest_action_entries [] = {

	/* Toplevel menus */
	{ "Email",       NULL, N_("mcen_me_inbox_email"),        NULL, NULL, NULL },
	{ "Edit",        NULL, N_("mcen_me_inbox_edit"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_edit_menu_activated) },
	{ "View",        NULL, N_("mcen_me_inbox_view"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_view_menu_activated) },
	{ "Tools",       NULL, N_("mcen_me_inbox_tools"),        NULL, NULL, G_CALLBACK (modest_ui_actions_on_tools_menu_activated) },
	{ "Attachments", NULL, N_("mcen_me_viewer_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_attachment_menu_activated) },
	{ "Options", NULL, N_("_Options") },
	{ "Zoom",        NULL, N_("Zoom") },
	{ "Help", NULL, N_("_Help") },

	/* Email */
	{ "EmailNew", NULL, N_("mcen_me_inbox_new") }, /* submenu */
	{ "EmailNewMessage",  NULL,  N_("mcen_me_inbox_message"),      "<CTRL>N", NULL,   G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewDefault", NULL, N_("mcen_me_viewer_newemail"), "<CTRL>N", NULL, G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewFolder",   NULL,  N_("mcen_me_inbox_folder"),       NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "EmailOpen",        NULL,  N_("mcen_me_inbox_open"),	        "<CTRL>O", NULL,   G_CALLBACK (modest_ui_actions_on_open) },
	{ "EmailReply",       NULL,  N_("mcen_me_inbox_reply"),         "<CTRL>R",      NULL,   G_CALLBACK (modest_ui_actions_on_reply) },
	{ "EmailReplyAll",    NULL,  N_("mcen_me_inbox_replytoall"),      NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "EmailForward",     NULL,  N_("mcen_me_inbox_forward"),      "<CTRL>D",      NULL,   G_CALLBACK (modest_ui_actions_on_forward) },
	{ "EmailRenameFolder", NULL, N_("mcen_me_user_renamefolder"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_rename_folder) },
	{ "EmailDelete",      NULL,  N_("mcen_me_inbox_delete"),    "BackSpace",      NULL,   G_CALLBACK (modest_ui_actions_on_delete_message_or_folder) },
	{ "EmailContents",    NULL,  N_("mcen_me_inbox_retrieve_contents"), NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_retrieve_msg_contents) },
	{ "EmailDetails",     NULL,  N_("mcen_me_inbox_messagedetails"),        NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_details) },
	{ "EmailPurgeAttachments", NULL, N_("mcen_me_inbox_remove_attachments"), NULL,  NULL,   G_CALLBACK (modest_ui_actions_remove_attachments) },
	{ "CloseWindow",          NULL,     N_("mcen_me_inbox_close_window"), "<CTRL>W", NULL,  G_CALLBACK (modest_ui_actions_on_close_window) },
	{ "CloseAllWindows",      NULL,     N_("mcen_me_inbox_close_windows"), "<CTRL>Q", NULL,  G_CALLBACK (modest_ui_actions_on_quit) },

	/* EDIT */
	{ "EditCut",         NULL,      N_("mcen_me_inbox_cut"),          "<CTRL>X",    NULL, G_CALLBACK (modest_ui_actions_on_cut) },
	{ "EditCopy",        NULL,      N_("mcen_me_inbox_copy"),         "<CTRL>C",    NULL, G_CALLBACK (modest_ui_actions_on_copy) },
	{ "EditPaste",       NULL,      N_("mcen_me_inbox_paste"),        "<CTRL>V",    NULL, G_CALLBACK (modest_ui_actions_on_paste) },
	{ "EditSelectAll",   NULL,      N_("mcen_me_viewer_selectall"),    "<CTRL>A",        NULL, G_CALLBACK (modest_ui_actions_on_select_all) },
	{ "EditMarkAsRead", NULL,      N_("mcen_me_inbox_mark_as_read"),       NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_mark_as_read) },
	{ "EditMarkAsUnread", NULL,      N_("mcen_me_inbox_mark_as_unread"),       NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_mark_as_unread) },
	{ "EditMoveTo",      NULL, 	N_("mcen_me_inbox_moveto"),    NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_move_to) },

	/* View */
	{ "ViewSort",            NULL,        N_("mcen_me_inbox_sort"),     NULL,      NULL,  G_CALLBACK (modest_ui_actions_on_sort) },
	{ "ViewPreviousMessage", NULL,    N_("mcen_me_viewer_previousmessage"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ViewNextMessage", NULL, N_("mcen_me_viewer_nextmessage"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },

	/* VIEW */
/* 	{ "ToggleView",        GTK_STOCK_CDROM,   N_("_Toggle view"), NULL,        N_("Toggle the list view"),  G_CALLBACK(modest_ui_actions_toggle_header_list_view) }, */

	/* GOTO */
	{ "GotoPrevious", GTK_STOCK_GO_BACK, N_("Previous"), NULL, N_("Go to previous message"), G_CALLBACK (modest_ui_actions_on_prev) },
	{ "GotoNext",     GTK_STOCK_GO_FORWARD, N_("Next"),     NULL, N_("Go to next message"), G_CALLBACK (modest_ui_actions_on_next) },

	/* Tools */
	{ "ToolsSettings",        NULL,      N_("mcen_me_inbox_options"),	              NULL, NULL, G_CALLBACK (modest_ui_actions_on_settings) },
	{ "ToolsAccounts",        NULL,      N_("mcen_me_inbox_accounts"),                NULL, NULL,  G_CALLBACK(modest_ui_actions_on_accounts) },
	{ "ToolsSMTPServers",     NULL,      N_("mcen_me_inbox_globalsmtpservers"),                NULL, NULL,  G_CALLBACK(modest_ui_actions_on_smtp_servers) },
	{ "ToolsSendReceive", NULL, N_("mcen_me_inbox_sendandreceive") }, /* submenu */
	{ "ToolsSendReceiveAll",    NULL,      N_("mcen_me_inbox_sendandreceive_all"),          NULL, NULL, G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "ToolsSendReceiveCancelSending",  NULL,      N_("mcen_me_outbox_cancelsend"),        NULL, NULL,  G_CALLBACK (modest_ui_actions_cancel_send) },
	{ "ToolsContacts",            NULL,      N_("mcen_me_inbox_open_addressbook"),                      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_open_addressbook) },
	{ "ToolsAddToContacts",            NULL,      N_("mcen_me_viewer_addtocontacts"),                      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_add_to_contacts) },
	{ "ToolsSearchMessages",            NULL,      N_("mcen_me_inbox_search"),  "<CTRL>E", NULL,  G_CALLBACK (modest_ui_actions_on_search_messages) },
	{ "ToolsHelp", NULL, N_("mcen_me_inbox_help"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_help) },

	/* Attachments */
	{ "ViewAttachment", NULL, N_("mcen_me_viewer_view_attachment"), NULL, NULL, G_CALLBACK (modest_ui_actions_view_attachment) },
	{ "SaveAttachment", NULL, N_("mcen_me_viewer_save_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_save_attachments) },
	{ "RemoveAttachment", NULL, N_("mcen_me_inbox_remove_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_remove_attachments) },
	
	/* OPTIONS */
	{ "OptionsAddToContacts", NULL, N_("A_dd to accounts"), NULL, N_("Add selection to accounts"), G_CALLBACK (modest_ui_actions_on_add_to_contacts) },
	{ "OptionsAccounts",  NULL, N_("_Accounts"), NULL, N_("Manage accounts"), G_CALLBACK (modest_ui_actions_on_accounts) },
	{ "OptionsContacts",  NULL, N_("_Contacts"), NULL, N_("Manage contacts"), NULL },
	{ "OptionsSettings",        NULL,      N_("mcen_me_inbox_options"),	              NULL, NULL, G_CALLBACK (modest_ui_actions_on_settings) },

	/* HELP */
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("About"), NULL, N_("About Modest"), G_CALLBACK (modest_ui_actions_on_about) },

	/* Headers Toolbar */
	{ "ToolbarMessageNew",        MODEST_STOCK_NEW_MAIL,     N_("mcen_me_inbox_new"), NULL, NULL,  G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "ToolbarMessageReply",      MODEST_STOCK_REPLY,     N_("mcen_me_inbox_reply"),      "<CTRL>R", NULL,  G_CALLBACK (modest_ui_actions_on_reply) },
	{ "ToolbarMessageReplyAll",   MODEST_STOCK_REPLY_ALL,     N_("mcen_me_inbox_replytoall"),         NULL, NULL,  G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "ToolbarMessageForward",    MODEST_STOCK_FORWARD,     N_("mcen_me_inbox_forward"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_forward) },
	{ "ToolbarSendReceive",       GTK_STOCK_REFRESH,   N_("mcen_me_inbox_sendandreceive"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "ToolbarDeleteMessage",     MODEST_STOCK_DELETE,     N_("mcen_me_inbox_delete"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_delete_message_or_folder) },
	{ "ToolbarMessageBack",       GTK_STOCK_GO_BACK,    N_("qgn_toolb_gene_back"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ToolbarMessageNext",    GTK_STOCK_GO_FORWARD, N_("qgn_toolb_gene_forward"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },

};

static const GtkToggleActionEntry modest_toggle_action_entries [] = {
	{ "ViewToggleFullscreen",     GTK_STOCK_FULLSCREEN, N_("mcen_me_inbox_fullscreen"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_fullscreen), FALSE },
};


G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

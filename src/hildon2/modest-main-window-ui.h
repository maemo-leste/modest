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
	{ "Email",       NULL, N_("mcen_me_inbox_email"),        NULL, NULL, G_CALLBACK (modest_ui_actions_on_email_menu_activated) },
	{ "Edit",        NULL, N_("mcen_me_inbox_edit"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_edit_menu_activated) },
	{ "View",        NULL, N_("mcen_me_inbox_view"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_view_menu_activated) },
	{ "Accounts",        NULL, N_("mcen_me_inbox_accounts"),         NULL, NULL, NULL },
	{ "Tools",       NULL, N_("mcen_me_inbox_tools"),        NULL, NULL, G_CALLBACK (modest_ui_actions_on_tools_menu_activated) },
	{ "Attachments", NULL, N_("mcen_me_viewer_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_attachment_menu_activated) },
	{ "Close",       NULL, N_("mcen_me_inbox_close") },
	{ "Zoom",        NULL, N_("mcen_me_viewer_zoom") },

	/* Zoom and fullscreen keyboard actions*/
	{ "ZoomPlus", NULL, N_("Zoom +"), "F7", NULL, G_CALLBACK (modest_ui_actions_on_zoom_plus) },
	{ "ZoomMinus", NULL, N_("Zoom -"), "F8", NULL, G_CALLBACK (modest_ui_actions_on_zoom_minus) },
	{ "ToggleFullscreen", NULL, N_("mcen_me_inbox_fullscreen"), "F6", NULL, G_CALLBACK (modest_ui_actions_on_change_fullscreen) },

	/* Email */
	{ "EmailNew", NULL, N_("mcen_me_inbox_new") }, /* submenu */
	{ "EmailNewMessage",  NULL,  N_("mcen_me_inbox_message"),      "<CTRL>N", NULL,   G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewDefault", NULL, N_("mcen_me_new_message"), "<CTRL>N", NULL, G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewFolder",   NULL,  N_("mcen_me_inbox_folder"),       NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "EmailOpen",        NULL,  N_("mcen_me_inbox_open"),	        "<CTRL>O", NULL,   G_CALLBACK (modest_ui_actions_on_open) },
	{ "EmailReply",       NULL,  N_("mcen_me_inbox_reply"),         "<CTRL>R",      NULL,   G_CALLBACK (modest_ui_actions_on_reply) },
	{ "EmailReplyAll",    NULL,  N_("mcen_me_inbox_replytoall"),      NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "EmailForward",     NULL,  N_("mcen_me_inbox_forward"),      "<CTRL>D",      NULL,   G_CALLBACK (modest_ui_actions_on_forward) },
	{ "EmailRenameFolder", NULL, N_("mcen_me_rename_folder"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_rename_folder) },
	{ "EmailDelete",      NULL,  N_("mcen_me_delete_messages"),    "BackSpace",      NULL,   G_CALLBACK (modest_ui_actions_on_delete_message_or_folder) },
	{ "EmailContents",    NULL,  N_("mcen_me_inbox_retrieve_contents"), NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_retrieve_msg_contents) },
	{ "EmailDetails",     NULL,  N_("mcen_me_inbox_messagedetails"),        NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_details) },
	{ "EmailPurgeAttachments", NULL, N_("mcen_me_inbox_remove_attachments"), NULL,  NULL,   G_CALLBACK (modest_ui_actions_remove_attachments) },

	/* Edit */
	{ "EditCut",         NULL,      N_("mcen_me_inbox_cut"),          "<CTRL>X",    NULL, G_CALLBACK (modest_ui_actions_on_cut) },
	{ "EditCopy",        NULL,      N_("mcen_me_inbox_copy"),         "<CTRL>C",    NULL, G_CALLBACK (modest_ui_actions_on_copy) },
	{ "EditPaste",       NULL,      N_("mcen_me_inbox_paste"),        "<CTRL>V",    NULL, G_CALLBACK (modest_ui_actions_on_paste) },
	{ "EditSelectAll",   NULL,      N_("mcen_me_viewer_selectall"),    "<CTRL>A",        NULL, G_CALLBACK (modest_ui_actions_on_select_all) },
	{ "EditMarkAsRead", NULL,      N_("mcen_me_inbox_mark_as_read"),       NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_mark_as_read) },
	{ "EditMarkAsUnread", NULL,      N_("mcen_me_inbox_mark_as_unread"),       NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_mark_as_unread) },
	{ "EditMoveTo",      NULL, 	N_("mcen_me_inbox_moveto"),    NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_move_to) },

	/* View */
	{ "ViewPreviousMessage", NULL,    N_("mcen_me_viewer_previousmessage"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ViewNextMessage", NULL, N_("mcen_me_viewer_nextmessage"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },

	/* Attachments */
	{ "SaveAttachment", NULL, N_("mcen_me_viewer_save_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_save_attachments) },
	{ "RemoveAttachment", NULL, N_("mcen_me_inbox_remove_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_remove_attachments) },

	/* Tools */
	{ "ToolsSettings",        NULL,      N_("mcen_me_inbox_options"),	              NULL, NULL, G_CALLBACK (modest_ui_actions_on_settings) },
	{ "ToolsAccounts",        NULL,      N_("mcen_me_inbox_accounts"),                NULL, NULL,  G_CALLBACK(modest_ui_actions_on_accounts) },
	{ "ToolsSMTPServers",     NULL,      N_("mcen_me_inbox_globalsmtpservers"),                NULL, NULL,  G_CALLBACK(modest_ui_actions_on_smtp_servers) },
	{ "ToolsSendReceive", NULL, N_("mcen_me_inbox_sendandreceive") }, /* submenu */
	{ "ToolsSendReceiveAll",    NULL,      N_("mcen_me_inbox_sendandreceive_all"),          NULL, NULL, G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "ToolsSendReceiveCancelSending",  NULL,      N_("mcen_me_outbox_cancelsend"),        NULL, NULL,  G_CALLBACK (modest_ui_actions_cancel_send) },
	{ "ToolsContacts",            NULL,      N_("mcen_me_inbox_open_addressbook"),                      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_open_addressbook) },
	{ "ToolsAddToContacts",            NULL,      N_("mcen_me_viewer_addtocontacts"),                      NULL, NULL,  G_CALLBACK (modest_ui_actions_add_to_contacts) },

	/* Close */
	{ "CloseWindow",          NULL,     N_("mcen_me_inbox_close_window"), "<CTRL>W", NULL,  G_CALLBACK (modest_ui_actions_on_close_window) },
	{ "CloseAllWindows",      NULL,     N_("mcen_me_inbox_close_windows"), "<CTRL>Q", NULL,  G_CALLBACK (modest_ui_actions_on_quit) },


	/* Toolbar items; they is some overlap with the menu items,
	 * but we need to specificy them differently, they have icons for example
	 */
	/* Headers Toolbar */
	{ "ToolbarMessageNew",        MODEST_STOCK_NEW_MAIL,     N_("qgn_toolb_messagin_new"), NULL, NULL,  G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "ToolbarMessageReply",      MODEST_STOCK_REPLY,     N_("mcen_me_inbox_reply"),      "<CTRL>R", NULL,  G_CALLBACK (modest_ui_actions_on_reply) },
	{ "ToolbarMessageReplyAll",   MODEST_STOCK_REPLY_ALL,     N_("mcen_me_inbox_replytoall"),         NULL, NULL,  G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "ToolbarMessageForward",    MODEST_STOCK_FORWARD,     N_("mcen_me_inbox_forward"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_forward) },
	{ "ToolbarSendReceive",       MODEST_STOCK_REFRESH,   N_("qgn_toolb_gene_refresh"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "ToolbarDeleteMessage",     MODEST_STOCK_DELETE,     N_("qgn_toolb_gene_deletebutton"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_delete_message_or_folder) },
	{ "ToolbarSort",     MODEST_STOCK_SORT,     N_("qgn_list_sort"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_sort) },
	{ "ToolbarFindInMessage",     MODEST_TOOLBAR_ICON_FIND,       N_("qgn_toolb_gene_find"),         NULL, NULL, NULL },
	{ "ToolbarMessageBack",       MODEST_TOOLBAR_ICON_PREV,    N_("qgn_toolb_gene_back"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ToolbarMessageNext",    MODEST_TOOLBAR_ICON_NEXT, N_("qgn_toolb_gene_forward"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },
	{ "ToolbarMessageMoveTo",     MODEST_TOOLBAR_ICON_MOVE_TO_FOLDER, N_("qgn_toolb_gene_movetofldr"),   NULL, NULL, G_CALLBACK(modest_ui_actions_on_move_to) },
	{ "ToolbarCancel",       GTK_STOCK_STOP,   "",      NULL, NULL,  NULL },
	{ "ToolbarDownloadExternalImages", MODEST_TOOLBAR_ICON_DOWNLOAD_IMAGES, N_("mail_bd_external_images"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_fetch_images) },
};

G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

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
	{ "Email", NULL, N_("mcen_me_inbox_email") },
	{ "Edit",    NULL, N_("mcen_me_inbox_edit") },
	{ "View",    NULL, N_("mcen_me_inbox_view") },
	{ "Tools",   NULL, N_("mcen_me_inbox_tools") },
	{ "Attachments", NULL, N_("mcen_me_viewer_attachments") },
	{ "Close",   NULL, N_("mcen_me_inbox_close") },
	{ "Zoom",   NULL, N_("Zoom") },

	/* Zoom and fullscreen keyboard actions*/
	{ "ZoomPlus", NULL, N_("Zoom +"), "F7", NULL, G_CALLBACK (modest_ui_actions_on_zoom_plus) },
	{ "ZoomMinus", NULL, N_("Zoom -"), "F8", NULL, G_CALLBACK (modest_ui_actions_on_zoom_minus) },
	{ "ToggleFullscreen", NULL, N_("Toggle fullscreen"), "F6", NULL, G_CALLBACK (modest_ui_actions_on_change_fullscreen) },

	/* Email */
	{ "EmailNew", NULL, N_("mcen_me_inbox_new") }, /* submenu */
	{ "EmailNewMessage",  NULL,  N_("mcen_me_inbox_message"),      "<CTRL>N", NULL,   G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewDefault", NULL, N_("mcen_me_inbox_new"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewFolder",   NULL,  N_("mcen_me_inbox_folder"),       NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "EmailOpen",        NULL,  N_("mcen_me_inbox_open"),	        "<CTRL>O", NULL,   G_CALLBACK (modest_ui_actions_on_open) },
	{ "EmailReply",       NULL,  N_("mcen_me_inbox_reply"),            NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply) },
	{ "EmailReplyAll",    NULL,  N_("mcen_me_inbox_replytoall"),      NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "EmailForward",     NULL,  N_("mcen_me_inbox_forward"),          NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_forward) },
	{ "EmailDelete",      NULL,  N_("mcen_me_inbox_delete"),    NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_delete) },
	{ "EmailContents",    NULL,  N_("mcen_me_inbox_retrieve_contents"), NULL,      NULL,   NULL },
	{ "EmailDetails",     NULL,  N_("mcen_me_inbox_messagedetails"),        NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_details) },
	{ "EmailPurgeAttachments", NULL, N_("mcen_me_inbox_remove_attachments"), NULL,  NULL,   NULL },
	

	/* Edit */
	{ "EditUndo",        NULL,      N_("mcen_me_inbox_undo"),        "<CTRL>Z",    NULL, NULL },
	{ "EditCut",         NULL,      N_("mcen_me_inbox_cut"),          "<CTRL>X",    NULL, G_CALLBACK (modest_ui_actions_on_cut) },
	{ "EditCopy",        NULL,      N_("mcen_me_inbox_copy"),         "<CTRL>C",    NULL, G_CALLBACK (modest_ui_actions_on_copy) },
	{ "EditPaste",       NULL,      N_("mcen_me_inbox_paste"),        "<CTRL>V",    NULL, G_CALLBACK (modest_ui_actions_on_paste) },
	{ "EditSelectAll",   NULL,      N_("mcen_me_viewer_selectall"),    NULL,        NULL, G_CALLBACK (modest_ui_actions_on_select_all) },
	{ "EditMarkAsRead", NULL,      N_("mcen_me_inbox_mark_as_read"),       NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_mark_as_read) },
	{ "EditMarkAsUnread", NULL,      N_("mcen_me_inbox_mark_as_unread"),       NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_mark_as_unread) },
	{ "EditMoveTo",      NULL, 	N_("mcen_me_inbox_moveto"),    NULL,	  NULL, G_CALLBACK (modest_ui_actions_on_move_to) },
	
	/* View */
	{ "ViewSort",            NULL,        N_("mcen_me_inbox_sort"),     NULL,      NULL,  G_CALLBACK (modest_ui_actions_on_sort) },
	{ "ViewShowToolbar", NULL, N_("mcen_me_inbox_toolbar") }, /* submenu */
	{ "ViewPreviousMessage", NULL,    N_("qgn_toolb_gene_back"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ViewNextMessage", NULL, N_("qgn_toolb_gene_forward"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },

	/* Attachments */
	{ "ViewAttachment", NULL, N_("mcen_me_viewer_view_attachment"), NULL, NULL, G_CALLBACK (modest_ui_actions_view_attachment) },
	{ "SaveAttachment", NULL, N_("mcen_me_viewer_save_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_save_attachments) },
	{ "RemoveAttachment", NULL, N_("mcen_me_viewer_remove_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_remove_attachments) },
	
	/* Tools */
	{ "ToolsSettings",        NULL,      N_("mcen_me_inbox_options"),	              NULL, NULL, G_CALLBACK (modest_ui_actions_on_settings) },
	{ "ToolsAccounts",        NULL,      N_("mcen_me_inbox_accounts"),                NULL, NULL,  G_CALLBACK(modest_ui_actions_on_accounts) },
	{ "ToolsSMTPServers",     NULL,      N_("mcen_me_inbox_globalsmtpservers"),                NULL, NULL,  NULL },
	{ "ToolsSendReceive", NULL, N_("mcen_me_inbox_sendandreceive") }, /* submenu */
	{ "ToolsSendReceiveAll",    NULL,      N_("mcen_me_inbox_sendandreceive_all"),          NULL, NULL, G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "ToolsSendReceiveCancelSending",  NULL,      N_("mcen_me_outbox_cancelsend"),        NULL, NULL,  NULL },
	{ "ToolsContacts",            NULL,      N_("mcen_me_inbox_open_addressbook"),                      NULL, NULL,  NULL },
	{ "ToolsAddToContacts",            NULL,      N_("mcen_me_viewer_addtocontacts"),                      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_add_to_contacts) },
	{ "ToolsSearchMessages",            NULL,      N_("mcen_me_inbox_search"),                      NULL, NULL,  NULL },
	{ "ToolsHelp",            NULL,      N_("mcen_me_inbox_help"),                      NULL, NULL,  NULL },

	/* Close */
	{ "CloseWindow",          NULL,     N_("mcen_me_inbox_close_window"),               NULL, NULL,  G_CALLBACK (modest_ui_actions_on_close_window) },
	{ "CloseAllWindows",      NULL,     N_("mcen_me_inbox_close_windows"),          NULL, NULL,  G_CALLBACK (modest_ui_actions_on_quit) },


	/* Toolbar items; they is some overlap with the menu items,
	 * but we need to specificy them differently, they have icons for example
	 */
	/* Headers Toolbar */
	{ "ToolbarMessageNew",        MODEST_STOCK_NEW_MAIL,     N_("qgn_toolb_messagin_new"), NULL, NULL,  G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "ToolbarMessageReply",      MODEST_STOCK_REPLY,     N_("qgn_toolb_messagin_reply"),          NULL, NULL,  G_CALLBACK (modest_ui_actions_on_reply) },
	{ "ToolbarMessageReplyAll",   MODEST_STOCK_REPLY_ALL,     N_("qgn_toolb_messagin_replyall"),         NULL, NULL,  G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "ToolbarMessageForward",    MODEST_STOCK_FORWARD,     N_("qgn_toolb_messagin_forward"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_forward) },
	{ "ToolbarSendReceive",       MODEST_STOCK_REFRESH,   N_("qgn_toolb_gene_refresh"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send_receive) },
/* 	{ "ToolbarToggleView",        MODEST_STOCK_SPLIT_VIEW,      N_("gqn_toolb_rss_fldonoff"),                "<CTRL>t", NULL,  G_CALLBACK (modest_ui_actions_toggle_folders_view) }, */
	{ "ToolbarDeleteMessage",     MODEST_STOCK_DELETE,     N_("qgn_toolb_gene_deletebutton"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_delete) },
	{ "ToolbarSort",     MODEST_STOCK_SORT,     N_("qgn_list_sort"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_sort) },
	{ "ToolbarFindInMessage",     GTK_STOCK_FIND,       N_("qgn_toolb_gene_find"),         NULL, NULL, NULL },
	{ "ToolbarMessageBack",       GTK_STOCK_GO_BACK,    N_("qgn_toolb_gene_back"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ToolbarMessageNext",    GTK_STOCK_GO_FORWARD, N_("qgn_toolb_gene_forward"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },
	{ "ToolbarMessageMoveTo",     MODEST_TOOLBAR_ICON_MOVE_TO_FOLDER, N_("qgn_toolb_gene_movetofldr"),   NULL, NULL, G_CALLBACK(modest_ui_actions_on_move_to) },
	{ "ToolbarCancel",       GTK_STOCK_STOP,   "",      NULL, NULL,  NULL },
};

static const GtkToggleActionEntry modest_toggle_action_entries [] = {
	{ "ViewToggleFolders",     NULL, N_("mcen_me_inbox_hidefolders"), NULL, NULL, G_CALLBACK (modest_ui_actions_toggle_folders_view), TRUE },
	{ "ViewToggleFullscreen",     GTK_STOCK_FULLSCREEN, N_("mcen_me_inbox_fullscreen"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_fullscreen), FALSE },
	{ "ViewShowToolbarNormalScreen", NULL, N_("mcen_me_inbox_normalview"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_toolbar), TRUE },
	{ "ViewShowToolbarFullScreen", NULL, N_("mcen_me_inbox_optimizedview"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_toolbar), TRUE },
};

G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

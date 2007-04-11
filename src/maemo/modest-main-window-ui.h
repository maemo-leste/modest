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


#ifndef GTK_STOCK_FULLSCREEN
#define GTK_STOCK_FULLSCREEN NULL
#endif /*GTK_STOCK_FULLSCREEN*/


/* Action entries */
static const GtkActionEntry modest_action_entries [] = {

	/* Toplevel menus */
	{ "Email", NULL, N_("mcen_me_inbox_email") },
	{ "Edit",    NULL, N_("mcen_me_inbox_edit") },
	{ "View",    NULL, N_("mcen_me_inbox_view") },
	{ "Tools",   NULL, N_("mcen_me_inbox_tools") },
	{ "Close",   NULL, N_("mcen_me_inbox_close") },
	{ "Zoom",   NULL, N_("Zoom") },

	/* Email */
	{ "EmailNew", NULL, N_("mcen_me_inbox_new") }, /* submenu */
	{ "EmailNewMessage",  NULL,  N_("mcen_me_inbox_message"),      "<CTRL>N", NULL,   G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewFolder",   NULL,  N_("mcen_me_inbox_folder"),       NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "EmailOpen",        NULL,  N_("mcen_me_inbox_open"),	        "<CTRL>O", NULL,   NULL },
	{ "EmailReply",       NULL,  N_("mcen_me_inbox_reply"),            NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply) },
	{ "EmailReplyAll",    NULL,  N_("mcen_me_inbox_replytoall"),      NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "EmailForward",     NULL,  N_("mcen_me_inbox_forward"),          NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_forward) },
	{ "EmailDelete",      NULL,  N_("mcen_me_inbox_delete"),    NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_delete) },
	{ "EmailContents",    NULL,  N_("mcen_me_inbox_retrieve_contents"), NULL,      NULL,   NULL },
	{ "EmailDetails",     NULL,  N_("mcen_me_inbox_messagedetails"),        NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_message_details) },
	{ "EmailPurgeAttachments", NULL, N_("FIXME: Purge attachments"), NULL,  NULL,   NULL },
	

	/* Edit */
	{ "EditUndo",        NULL,      N_("mcen_me_inbox_undo"),        "<CTRL>Z",    NULL, NULL },
	{ "EditCut",         NULL,      N_("mcen_me_inbox_cut"),          "<CTRL>X",    NULL, G_CALLBACK (modest_ui_actions_on_cut) },
	{ "EditCopy",        NULL,      N_("mcen_me_inbox_copy"),         "<CTRL>C",    NULL, G_CALLBACK (modest_ui_actions_on_copy) },
	{ "EditPaste",       NULL,      N_("mcen_me_inbox_paste"),        "<CTRL>V",    NULL, G_CALLBACK (modest_ui_actions_on_paste) },
	{ "EditSelectAll",   NULL,      N_("mcen_me_inbox_selectall"),    NULL,        NULL, G_CALLBACK (modest_ui_actions_on_select_all) },
	{ "EditMarkAsRead", NULL,      N_("mcen_me_inbox_mark_as_read"),       NULL,	  NULL, NULL },
	{ "EditMarkAsUnread", NULL,      N_("mcen_me_inbox_mark_as_unread"),       NULL,	  NULL, NULL },
	{ "EditMoveTo",      NULL, 	N_("mcen_me_inbox_moveto"),    NULL,	  NULL, NULL },
	
	/* View */
	{ "ViewSort",            NULL,        N_("mcen_me_inbox_sort"),     NULL,      NULL,  NULL },
	{ "ViewFolders",         NULL,        N_("mcen_me_inbox_hidefolders"),     NULL,     NULL,  NULL },
	{ "ViewFullscreen",      NULL,        N_("mcen_me_inbox_fullscreen"),  NULL,     NULL,  NULL },
	{ "ViewShowToolbar", NULL, N_("mcen_me_inbox_toolbar") }, /* submenu */
	{ "ViewShowToolbarNormalScreen",         NULL,        N_("mcen_me_inbox_normalview"),     NULL,     NULL,  NULL },
	{ "ViewShowToolbarFullScreen",      NULL,        N_("mcen_me_inbox_optimizedview"),  NULL,     NULL,  NULL },
	{ "ViewPreviousMessage", NULL,    N_("qgn_toolb_gene_back"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ViewNextMessage", NULL, N_("qgn_toolb_gene_forward"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },
	
	/* Tools */
	{ "ToolsSettings",        NULL,      N_("mcen_me_inbox_options"),	              NULL, NULL,  NULL },
	{ "ToolsAccounts",        NULL,      N_("mcen_me_inbox_accounts"),                NULL, NULL,  G_CALLBACK(modest_ui_actions_on_accounts) },
	{ "ToolsSMTPServers",     NULL,      N_("mcen_me_inbox_globalsmtpservers"),                NULL, NULL,  NULL },
	{ "ToolsSendReceive", NULL, N_("mcen_me_inbox_sendandreceive") }, /* submenu */
	{ "ToolsSendReceiveAll",    NULL,      N_("mcen_me_inbox_sendandreceive_all"),          NULL, NULL,  NULL },
	{ "ToolsSendReceiveCancelSending",  NULL,      N_("mcen_me_inbox_cancelsend"),        NULL, NULL,  NULL },
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
	{ "ToolbarToggleView",        MODEST_STOCK_SPLIT_VIEW,      N_("gqn_toolb_rss_fldonoff"),                "<CTRL>t", NULL,  G_CALLBACK (modest_ui_actions_toggle_view) },
	{ "ToolbarDeleteMessage",     MODEST_STOCK_DELETE,     N_("qgn_toolb_gene_deletebutton"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_delete) },
	{ "ToolbarSort",     MODEST_STOCK_SORT,     N_("qgn_list_sort"),             NULL, NULL, NULL },
	{ "ToolbarFindInMessage",     GTK_STOCK_FIND,       N_("qgn_toolb_gene_find"),         NULL, NULL, NULL },
	{ "ToolbarMessageBack",       GTK_STOCK_GO_BACK,    N_("qgn_toolb_gene_back"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ToolbarMessageForward",    GTK_STOCK_GO_FORWARD, N_("qgn_toolb_gene_forward"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },
	{ "ToolbarMessageMoveTo",     MODEST_TOOLBAR_ICON_MOVE_TO_FOLDER, N_("qgn_toolb_gene_movetofldr"),   NULL, NULL, NULL },
};

static const GtkToggleActionEntry modest_toggle_action_entries [] = {
	{ "ShowToggleFullscreen",     GTK_STOCK_FULLSCREEN, N_("Full screen"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_fullscreen), FALSE },
};


G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

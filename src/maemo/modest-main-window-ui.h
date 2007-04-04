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
	{ "Edit",    NULL, N_("Edit") },
	{ "View",    NULL, N_("View") },
	{ "Tools",   NULL, N_("Tools") },
	{ "Close",   NULL, N_("Close") },

	/* Email */
	{ "EmailNew", NULL, N_("mcen_me_inbox_new") }, /* submenu */
	{ "EmailNewMessage",  NULL,  N_("mcen_me_inbox_new"),      "<CTRL>N", NULL,   G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "EmailNewFolder",   NULL,  N_("mcen_me_inbox_folder"),       NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "EmailOpen",        NULL,  N_("mcen_me_inbox_open"),	        "<CTRL>O", NULL,   NULL },
/* 	{ "MessageCancelSending",  NULL,  N_(""),	NULL,      NULL,   NULL }, */
/* 	{ "MessageSend",        NULL,  N_("Send"),	        NULL,      NULL,   NULL }, */
/* 	{ "MessageSendNow",        NULL,  N_("Send now"),	        NULL,      NULL,   NULL }, */
	{ "EmailReply",       NULL,  N_("mcen_me_inbox_reply"),            NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply) },
	{ "EmailReplyAll",    NULL,  N_("mcen_me_inbox_replytoall"),      NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "EmailForward",     NULL,  N_("mcen_me_inbox_forward"),          NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_forward) },
	{ "EmailDelete",      NULL,  N_("mcen_me_inbox_delete"),    NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_delete) },
/* 	{ "MessageSendReceive", NULL,  N_("Send and receive"),  NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_send_receive) }, */
	{ "EmailContents",    NULL,  N_("mcen_me_inbox_retrieve_contents"), NULL,      NULL,   NULL },
	{ "EmailDetails",     NULL,  N_("mcen_me_inbox_messagedetails"),        NULL,      NULL,   NULL },
	{ "EmailPurgeAttachments", NULL, N_("FIXME: Purge attachments"), NULL,  NULL,   NULL },
	

	/* Edit */
	{ "EditUndo",        NULL,      N_("_Undo"),        "<CTRL>Z",    NULL, NULL },
	{ "EditCut",         NULL,      N_("Cut"),          "<CTRL>X",    NULL, G_CALLBACK (modest_ui_actions_on_cut) },
	{ "EditCopy",        NULL,      N_("Copy"),         "<CTRL>C",    NULL, G_CALLBACK (modest_ui_actions_on_copy) },
	{ "EditPaste",       NULL,      N_("Paste"),        "<CTRL>V",    NULL, G_CALLBACK (modest_ui_actions_on_paste) },
	{ "EditSelectAll",   NULL,      N_("Select all"),    NULL,        NULL, G_CALLBACK (modest_ui_actions_on_select_all) },
	{ "EditDelete",      NULL,      N_("_Delete"),       NULL,	  NULL, NULL },
	{ "EditMarkAsRead", NULL,      N_("Mark as read"),       NULL,	  NULL, NULL },
	{ "EditMarkAsUnread", NULL,      N_("Mark as unread"),       NULL,	  NULL, NULL },
	{ "EditSelect",      NULL, 	N_("Select..."),     NULL,	  NULL, NULL },   /* submenu */
	{ "EditMoveTo",      NULL, 	N_("Move to..."),    NULL,	  NULL, NULL },
	
	/* View */
	{ "ViewSort",            NULL,        N_("Sort..."),     NULL,      NULL,  NULL },
	{ "ViewFolders",         NULL,        N_("Folders"),     NULL,     NULL,  NULL },
	{ "ViewFullscreen",      NULL,        N_("Fullscreen"),  NULL,     NULL,  NULL },
	{ "ViewShowToolbar", NULL, N_("Show toolbar") }, /* submenu */
	{ "ViewShowToolbarNormalScreen",         NULL,        N_("Normal screen"),     NULL,     NULL,  NULL },
	{ "ViewShowToolbarFullScreen",      NULL,        N_("Full screen"),  NULL,     NULL,  NULL },
	
	/* Tools */
	{ "ToolsSettings",        NULL,      N_("Settings..."),	              NULL, NULL,  NULL },
	{ "ToolsAccounts",        NULL,      N_("Accounts..."),                NULL, NULL,  NULL },
	{ "ToolsSMTPServers",     NULL,      N_("SMTP servers..."),                NULL, NULL,  NULL },
	{ "ToolsSendReceive", NULL, N_("Send & receive") }, /* submenu */
	{ "ToolsSendReceiveAll",    NULL,      N_("All"),          NULL, NULL,  NULL },
	{ "ToolsSendReceiveCancelSending",  NULL,      N_("Cancel sending"),        NULL, NULL,  NULL },
	{ "ToolsContacts",            NULL,      N_("Contacts..."),                      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_add_to_contacts) },
	{ "ToolsSearchMessages",            NULL,      N_("Search messages..."),                      NULL, NULL,  NULL },
	{ "ToolsHelp",            NULL,      N_("Help"),                      NULL, NULL,  NULL },

	/* Close */
	{ "CloseWindow",          NULL,     N_("Close window"),               NULL, NULL,  NULL },
	{ "CloseAllWindows",      NULL,     N_("Close all windows"),          NULL, NULL,  G_CALLBACK (modest_ui_actions_on_quit) },


	/* Toolbar items; they is some overlap with the menu items,
	 * but we need to specificy them differently, they have icons for example
	 */
	/* Headers Toolbar */
	{ "ToolbarMessageNew",        MODEST_STOCK_NEW_MAIL,     N_("Compose a new message"), NULL, NULL,  G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "ToolbarMessageReply",      MODEST_STOCK_REPLY,     N_("Reply a message"),          NULL, NULL,  G_CALLBACK (modest_ui_actions_on_reply) },
	{ "ToolbarMessageReplyAll",   MODEST_STOCK_REPLY_ALL,     N_("Reply to all"),         NULL, NULL,  G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "ToolbarMessageForward",    MODEST_STOCK_FORWARD,     N_("Forward a message"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_forward) },
	{ "ToolbarSendReceive",       MODEST_STOCK_REFRESH,   N_("Send & receive"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "ToolbarToggleView",        MODEST_STOCK_SPLIT_VIEW,      N_("Toggle view"),                "<CTRL>t", NULL,  G_CALLBACK (modest_ui_actions_toggle_view) },
	{ "ToolbarDeleteMessage",     MODEST_STOCK_DELETE,     N_("Delete message"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_delete) },
	{ "ToolbarSort",     MODEST_STOCK_SORT,     N_("Sort mail"),             NULL, NULL, NULL },
};


G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

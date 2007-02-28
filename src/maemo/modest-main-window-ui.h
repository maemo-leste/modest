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
	{ "Message", NULL, N_("Message") },
	{ "Edit",    NULL, N_("Edit") },
	{ "View",    NULL, N_("View") },
	{ "Folders", NULL, N_("Folders") },
	{ "Accounts",NULL, N_("Accounts") },
	{ "Tools",   NULL, N_("Tools") },
	{ "Close",   NULL, N_("Close") },

	/* Message */
	{ "MessageNew",         NULL,  N_("_New message"),      "<CTRL>N", NULL,   G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "MessageOpen",        NULL,  N_("_Open"),	        "<CTRL>O", NULL,   NULL },
	{ "MessageCancelSending",  NULL,  N_("Cancel sending"),	NULL,      NULL,   NULL },
	{ "MessageSend",        NULL,  N_("Send"),	        NULL,      NULL,   NULL },
	{ "MessageReply",       NULL,  N_("_Reply"),            NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply) },
	{ "MessageReplyAll",    NULL,  N_("Reply to all"),      NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "MessageForward",     NULL,  N_("_Forward"),          NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_forward) },
	{ "MessageDelete",      NULL,  N_("Delete message"),    NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_delete) },
	{ "MessageSendReceive", NULL,  N_("Send and receive"),  NULL,      NULL,   G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "MessageContents",    NULL,  N_("Retrieve contents"), NULL,      NULL,   NULL },
	{ "MessageDetails",     NULL,  N_("Details..."),        NULL,      NULL,   NULL },
	

	/* Edit */
	{ "EditUndo",        NULL,      N_("_Undo"),        "<CTRL>Z",    NULL, NULL },
	{ "EditCut",         NULL,      N_("Cut"),          "<CTRL>X",    NULL, NULL },
	{ "EditCopy",        NULL,      N_("Copy"),         "<CTRL>C",    NULL, NULL },
	{ "EditPaste",       NULL,      N_("Paste"),        "<CTRL>V",    NULL, NULL },
	{ "EditDelete",      NULL,      N_("_Delete"),       NULL,	  NULL, NULL },
	{ "EditSelect",      NULL, 	N_("Select..."),     NULL,	  NULL, NULL },   /* submenu */
	{ "EditMoveTo",      NULL, 	N_("Move to..."),    NULL,	  NULL, NULL },
	
	/* View */
	{ "ViewSort",            NULL,        N_("Sort..."),     NULL,      NULL,  NULL },
	{ "ViewFolders",         NULL,        N_("Folders"),     NULL,     NULL,  NULL },
	{ "ViewFullscreen",      NULL,        N_("Fullscreen"),  NULL,     NULL,  NULL },
	{ "ViewDetails",         NULL,        N_("Details"),     NULL,     NULL,  NULL },
	{ "ViewThumbnails",      NULL,        N_("Thumbnails"),  NULL,     NULL,  NULL },
	{ "ViewShowToolbar",     NULL,        N_("Show toolbar"), NULL,   NULL,  NULL },    /* submenu */
		
	/* Folders */
	{ "FoldersNew",          NULL,       N_("New folder"),       NULL, NULL, G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "FoldersManage",       NULL,       N_("Manage..."),        NULL, NULL, NULL },
	{ "FoldersDetails",      NULL,       N_("Details..."),       NULL, NULL, NULL },
	{ "FoldersDelete",       NULL,       N_("Delete"),           NULL, NULL, G_CALLBACK (modest_ui_actions_on_delete_folder) },
	{ "FoldersRename",       NULL,       N_("Rename"),           NULL, NULL, G_CALLBACK (modest_ui_actions_on_rename_folder) },
	{ "FoldersMoveToTrash",  NULL,       N_("Move to trash"),    NULL, NULL, G_CALLBACK (modest_ui_actions_on_move_folder_to_trash_folder) },

	/* Accounts */
	{ "AccountsNew",          NULL,     N_("_New account..."),	      NULL, NULL,  NULL },
	{ "AccountsManage",        NULL,    N_("Manage..."),                  NULL, NULL,  G_CALLBACK (modest_ui_actions_on_accounts) },
	{ "AccountsConfigureSMTP", NULL,    N_("Configure SMTP servers..."),  NULL, NULL,  NULL },
	
	/* Tools */
	{ "ToolsSettings",        NULL,      N_("Settings..."),	              NULL, NULL,  NULL },
	{ "ToolsContacts",        NULL,      N_("Contact..."),                NULL, NULL,  NULL },
	{ "ToolsFontSettings",    NULL,      N_("Font settings..."),          NULL, NULL,  NULL },
	{ "ToolsSearchMessages",  NULL,      N_("Search messages..."),        NULL, NULL,  NULL },
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
	{ "ToolbarSendReceive",       GTK_STOCK_REFRESH,   N_("Send & receive"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send_receive) },
	{ "ToolbarToggleView",        GTK_STOCK_CDROM,      N_("Toggle view"),                "<CTRL>t", NULL,  G_CALLBACK (modest_ui_actions_toggle_view) },
	{ "ToolbarDeleteMessage",     GTK_STOCK_DELETE,     N_("Delete message"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_delete) },
};


G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

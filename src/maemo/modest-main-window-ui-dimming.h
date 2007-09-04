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


#ifndef __MODEST_MAIN_WINDOW_UI_DIMMING_PRIV_H__
#define __MODEST_MAIN_WINDOW_UI_DIMMING_PRIV_H__

#include "modest-dimming-rules-group.h"
#include "modest-ui-dimming-rules.h"

G_BEGIN_DECLS


/* Menu Dimming rules entries */
static const ModestDimmingEntry modest_main_window_menu_dimming_entries [] = {

	/* Email Menu */
	{ "/MenuBar/EmailMenu/EmailNewMainMenu", NULL },
	{ "/MenuBar/EmailMenu/EmailNewMainMenu/EmailNewMessageMenu", G_CALLBACK(modest_ui_dimming_rules_on_new_msg) },
	{ "/MenuBar/EmailMenu/EmailNewMainMenu/EmailNewFolderMenu", G_CALLBACK(modest_ui_dimming_rules_on_new_folder) },
	{ "/MenuBar/EmailMenu/EmailOpenMenu", G_CALLBACK(modest_ui_dimming_rules_on_open_msg) },
	{ "/MenuBar/EmailMenu/EmailReplyMenu", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/MenuBar/EmailMenu/EmailReplyAllMenu", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/MenuBar/EmailMenu/EmailForwardMenu",  G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/MenuBar/EmailMenu/EmailContentsMenu", G_CALLBACK(modest_ui_dimming_rules_on_contents_msg) },
	{ "/MenuBar/EmailMenu/EmailPurgeAttachmentsMenu", G_CALLBACK(modest_ui_dimming_rules_on_remove_attachments) },
	{ "/MenuBar/EmailMenu/EmailRenameMenu",  G_CALLBACK(modest_ui_dimming_rules_on_rename_folder) },
	{ "/MenuBar/EmailMenu/EmailDeleteMenu",  G_CALLBACK(modest_ui_dimming_rules_on_delete) },
	{ "/MenuBar/EmailMenu/EmailDetailsMenu", G_CALLBACK(modest_ui_dimming_rules_on_details) },

	/* Edit Menu */
	{ "/MenuBar/EditMenu", NULL },
	{ "/MenuBar/EditMenu/EditSelectAllMenu", G_CALLBACK(modest_ui_dimming_rules_on_select_all)},
	{ "/MenuBar/EditMenu/EditMarkAsReadMenu", G_CALLBACK(modest_ui_dimming_rules_on_mark_as_read_msg) },
	{ "/MenuBar/EditMenu/EditMarkAsUnreadMenu", G_CALLBACK(modest_ui_dimming_rules_on_mark_as_unread_msg) },
	{ "/MenuBar/EditMenu/EditMoveToMenu", G_CALLBACK(modest_ui_dimming_rules_on_move_to) },

	/* View Menu */
	{ "/MenuBar/ViewMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewSortMenu", G_CALLBACK(modest_ui_dimming_rules_on_sort) },
	{ "/MenuBar/ViewMenu/ViewToggleFoldersMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewToggleFullscreenMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewShowToolbarMainMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewShowToolbarMainMenu/ViewShowToolbarNormalScreenMenu", NULL },
	{ "/MenuBar/ViewMenu/ViewShowToolbarMainMenu/ViewShowToolbarFullScreenMenu", NULL },
	
	/* Tools Menu */
	{ "/MenuBar/ToolsMenu", NULL },
	{ "/MenuBar/ToolsMenu/ToolsSettingsMenu", NULL },
	{ "/MenuBar/ToolsMenu/ToolsAccountsMenu", NULL },
	{ "/MenuBar/ToolsMenu/ToolsSMTPServersMenu", G_CALLBACK(modest_ui_dimming_rules_on_tools_smtp_servers) },
	{ "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu", NULL },
	{ "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveAllMenu", G_CALLBACK(modest_ui_dimming_rules_on_send_receive) },
	{ "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveCancelSendingMenu", G_CALLBACK(modest_ui_dimming_rules_on_cancel_sending) },
	{ "/MenuBar/ToolsMenu/ToolsContactsMenu", NULL },
	{ "/MenuBar/ToolsMenu/ToolsSearchMessagesMenu", NULL },
	{ "/MenuBar/ToolsMenu/ToolsHelpMenu", NULL },

	/* Close Menu */
	{ "/MenuBar/CloseMenu", NULL },
	{ "/MenuBar/ToolsMenu/CloseWindowMenu", NULL },
	{ "/MenuBar/ToolsMenu/CloseAllWindowsMenu", NULL },

	/* Contextual Menus (Header View) */
	{ "/HeaderViewCSM/HeaderViewCSMOpen", G_CALLBACK(modest_ui_dimming_rules_on_open_msg) },
	{ "/HeaderViewCSM/HeaderViewCSMReply", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/HeaderViewCSM/HeaderViewCSMReplyAll", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/HeaderViewCSM/HeaderViewCSMForward", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/HeaderViewCSM/HeaderViewCSMDelete", G_CALLBACK(modest_ui_dimming_rules_on_delete_msg) },
	{ "/HeaderViewCSM/HeaderViewCSMCancelSending", G_CALLBACK(modest_ui_dimming_rules_on_csm_cancel_sending) },
	{ "/HeaderViewCSM/HeaderViewCSMHelp", NULL },

	/* Contextual Menus (Folder View) */
	{ "/FolderViewCSM/FolderViewCSMNewFolder", G_CALLBACK(modest_ui_dimming_rules_on_new_folder) },
	{ "/FolderViewCSM/FolderViewCSMRenameFolder", G_CALLBACK(modest_ui_dimming_rules_on_rename_folder) },
	{ "/FolderViewCSM/FolderViewCSMPasteMsgs", G_CALLBACK(modest_ui_dimming_rules_on_paste) },
	{ "/FolderViewCSM/FolderViewCSMDeleteFolder", G_CALLBACK(modest_ui_dimming_rules_on_delete_folder) },
	{ "/FolderViewCSM/FolderViewCSMSearchMessages", NULL },
	{ "/FolderViewCSM/FolderViewCSMHelp", NULL },

	/* Contextual Menus (Toolbar) */
	{ "/ToolbarReplyCSM/ToolbarMessageForward", NULL },
	{ "/ToolbarReplyCSM/ToolbarMessageReplyAll", NULL },
	{ "/ToolbarReplyCSM/ToolbarMessageReply", NULL },
	
};

/* Toolbar Dimming rules entries */
static const ModestDimmingEntry modest_main_window_toolbar_dimming_entries [] = {

	/* Toolbar */
	{ "/ToolBar/ToolbarMessageNew", G_CALLBACK(modest_ui_dimming_rules_on_new_msg) },
	{ "/ToolBar/ToolbarMessageReply", G_CALLBACK(modest_ui_dimming_rules_on_reply_msg) },
	{ "/ToolBar/ToolbarDeleteMessage", G_CALLBACK(modest_ui_dimming_rules_on_delete) },
	{ "/ToolBar/ToolbarToggleView", NULL },
	{ "/ToolBar/ToolbarSort", G_CALLBACK(modest_ui_dimming_rules_on_sort) },
	{ "/ToolBar/ToolbarSendReceive", G_CALLBACK(modest_ui_dimming_rules_on_send_receive) },
	{ "/ToolBar/ToolbarCancel", NULL },
};

G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

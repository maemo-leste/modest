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
	{ "File", NULL, N_("_File") },
	{ "Edit", NULL, N_("_Edit") },
	{ "Actions", NULL, N_("_Actions") },
	{ "Options", NULL, N_("_Options") },
	{ "Help", NULL, N_("_Help") },

	/* FILE */
	{ "FileNew",    GTK_STOCK_NEW,     N_("_New"),	   "<CTRL>N", N_("Compose new message"),  G_CALLBACK (_modest_ui_actions_on_new_msg) },
	{ "FileOpen",   GTK_STOCK_OPEN,    N_("_Open"),	   "<CTRL>O", N_("Open a message"),       NULL },
	{ "FileSave",   GTK_STOCK_SAVE,    N_("_Save"),	   "<CTRL>S", N_("Save a message"),       NULL },
	{ "FileSaveAs", GTK_STOCK_SAVE_AS, N_("Save _As"), NULL,      N_("Save a message as"),    NULL },
	{ "FileQuit",   GTK_STOCK_QUIT,    N_("_Quit"),	   "<CTRL>Q", N_("Exit the application"), G_CALLBACK (_modest_ui_actions_on_quit) },

	/* EDIT */
	{ "EditUndo",        GTK_STOCK_UNDO,   N_("_Undo"), "<CTRL>Z",        N_("Undo last action"),  NULL },
	{ "EditRedo",        GTK_STOCK_REDO,   N_("_Redo"), "<shift><CTRL>Z", N_("Redo previous action"),  NULL },
	{ "EditCut",         GTK_STOCK_CUT,    N_("Cut"),   "<CTRL>X",        N_("_Cut"), NULL   },
	{ "EditCopy",        GTK_STOCK_COPY,   N_("Copy"),  "<CTRL>C",        N_("Copy"), NULL },
	{ "EditPaste",       GTK_STOCK_PASTE,  N_("Paste"), "<CTRL>V",        N_("Paste"), NULL },
	{ "EditDelete",      GTK_STOCK_DELETE, N_("_Delete"),      "<CTRL>Q",	      N_("Delete"), NULL },
	{ "EditSelectAll",   NULL, 	       N_("Select all"),   "<CTRL>A",	      N_("Select all"), NULL },
	{ "EditDeselectAll", NULL,             N_("Deselect all"), "<Shift><CTRL>A",  N_("Deselect all"), NULL },

	/* VIEW */
	{ "ToggleView",        GTK_STOCK_CDROM,   N_("_Toggle view"), NULL,        N_("Toggle the list view"),  G_CALLBACK(_modest_ui_actions_toggle_view) },
	
	/* ACTIONS */
	{ "ActionsNew",         MODEST_STOCK_NEW_MAIL, N_("_New Message"),   NULL, N_("Compose a new message"), G_CALLBACK (_modest_ui_actions_on_new_msg) },
	{ "ActionsReply",       MODEST_STOCK_REPLY, N_("_Reply"),         NULL, N_("Reply to a message"), G_CALLBACK (_modest_ui_actions_on_reply) },
	{ "ActionsReplyAll",    MODEST_STOCK_REPLY_ALL, N_("Reply to all"),   NULL, N_("Reply to all"), G_CALLBACK (_modest_ui_actions_on_reply_all) },
	{ "ActionsForward",     MODEST_STOCK_FORWARD, N_("_Forward"),       NULL, N_("Forward a message"), G_CALLBACK (_modest_ui_actions_on_forward) },
	{ "ActionsBounce",      NULL, N_("_Bounce"),        NULL, N_("Bounce a message"),          NULL },
	{ "ActionsSendReceive", MODEST_STOCK_SEND_RECEIVE, N_("Send/Receive"),   NULL, N_("Send and receive messages"), NULL },
	{ "ActionsDelete",      MODEST_STOCK_DELETE, N_("Delete message"), NULL, N_("Delete messages"), G_CALLBACK (_modest_ui_actions_on_delete) },
	{ "ActionsFolderNew",   NULL, N_("New Folder"),   NULL, N_("Create a new folder"), G_CALLBACK (_modest_ui_actions_on_new_folder) },
	{ "ActionsFolderDelete",   NULL, N_("Delete Folder"),   NULL, N_("Delete the folder"), G_CALLBACK (_modest_ui_actions_on_delete_folder) },
	{ "ActionsFolderRename",   NULL, N_("Rename Folder"),   NULL, N_("Rename the folder"), G_CALLBACK (_modest_ui_actions_on_rename_folder) },
	{ "ActionsFolderMoveToTrash",   NULL, N_("Move Folder to Trash"),   NULL, N_("Move folder to Trash"), G_CALLBACK (_modest_ui_actions_on_move_to_trash_folder) },


	/* GOTO */
	{ "GotoPrevious", MODEST_STOCK_PREV, N_("Previous"), NULL, N_("Go to previous message"), NULL },
	{ "GotoNext",     MODEST_STOCK_NEXT, N_("Next"),     NULL, N_("Go to next message"), G_CALLBACK (_modest_ui_actions_on_next) },

	/* OPTIONS */
	{ "OptionsAccounts",  NULL, N_("_Accounts"), NULL, N_("Manage accounts"), G_CALLBACK (_modest_ui_actions_on_accounts) },
	{ "OptionsContacts",  NULL, N_("_Contacts"), NULL, N_("Manage contacts"), NULL },

	/* HELP */
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("About"), NULL, N_("About Modest"), G_CALLBACK (_modest_ui_actions_on_about) },
};


G_END_DECLS
#endif /* __MODEST_MAIN_WINDOW_UI_PRIV_H__ */

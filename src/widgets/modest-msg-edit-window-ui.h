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

#ifndef __MODEST_MSG_EDIT_WINDOW_UI_H__
#define __MODEST_MSG_EDIT_WINDOW_UI_H__

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "modest-icon-names.h"
#include "modest-ui-actions.h"
#include "modest-defs.h"

G_BEGIN_DECLS

static const GtkActionEntry modest_msg_edit_action_entries [] = {
	/* Toplevel menus */
	{ "Email", NULL, N_("mcen_me_inbox_email") , NULL, NULL, G_CALLBACK (modest_ui_actions_on_email_menu_activated) },
	{ "View", NULL, N_("mcen_me_inbox_view") , NULL, NULL, G_CALLBACK (modest_ui_actions_on_view_menu_activated)},
	{ "Edit", NULL, N_("mcen_me_inbox_edit") , NULL, NULL, G_CALLBACK (modest_ui_actions_on_edit_menu_activated)},
	{ "Format", NULL, N_("mcen_me_editor_format") , NULL, NULL, G_CALLBACK (modest_ui_actions_on_format_menu_activated)},
	{ "Alignment", NULL, N_("mcen_me_editor_align") },
	{ "Attachments", NULL, N_("mcen_me_viewer_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_attachment_menu_activated) },
	{ "MessagePriority", NULL, N_("mcen_me_editor_message_priority") },
	{ "FileFormat", NULL, N_("mcen_me_editor_file_format") },
	{ "Tools", NULL, N_("mcen_me_inbox_tools") },
	{ "ShowToolbar", NULL, N_("mcen_me_inbox_toolbar") },
	{ "Close", NULL, N_("mcen_me_inbox_close") },

	/* ACTIONS */
	/* The logical id is different */
#ifdef MODEST_TOOLKIT_HILDON2
	{ "ActionsNewMessage", NULL, N_("mcen_me_new_message"), "<CTRL>N", NULL, G_CALLBACK (modest_ui_actions_on_new_msg) },
#else
	{ "ActionsNewMessage", NULL, N_("mcen_me_viewer_newemail"), "<CTRL>N", NULL, G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "ActionsSaveToDrafts", NULL, N_("mcen_me_editor_save_as_draft"), "<CTRL>S", NULL, G_CALLBACK (modest_ui_actions_on_save_to_drafts) },
#endif
#ifdef MODEST_TOOLKIT_HILDON2
	{ "ActionsDelete", NULL, N_("mcen_me_delete_messages"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_delete_message) },
#else
	{ "ActionsDelete", NULL, N_("mcen_me_inbox_delete"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_delete_message) },
#endif
	{ "ActionsSend", NULL, N_("mcen_me_editor_send"), NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send) },
#ifndef MODEST_TOOLKIT_HILDON2
 	{ "ActionsFontColor", GTK_STOCK_SELECT_COLOR, N_("Color"), NULL, N_("Change text color"), G_CALLBACK (modest_ui_actions_on_select_editor_color)},
 	{ "BackgroundColor", GTK_STOCK_SELECT_COLOR, N_("Background color"), NULL, N_("Change background color"), G_CALLBACK (modest_ui_actions_on_select_editor_background_color)},
#endif
	{ "InsertImage", NULL, N_("mcen_me_editor_attach_inlineimage"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_insert_image)},
#ifndef MODEST_TOOLKIT_HILDON2
	{ "AttachFile", NULL, N_("mcen_me_editor_attachfile"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_attach_file)},
#endif
	{ "RemoveAttachments", NULL, N_("mcen_me_inbox_remove_attachments"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_remove_attachments) },
	{ "Undo", NULL, N_("mcen_me_inbox_undo"), "<CTRL>Z", NULL, G_CALLBACK (modest_ui_actions_on_undo)},
#ifndef MODEST_TOOLKIT_HILDON2
	{ "Redo", NULL, N_("mcen_me_inbox_redo"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_redo)},
	{ "Cut", NULL, N_("mcen_me_inbox_cut"),  "<CTRL>X", NULL, G_CALLBACK (modest_ui_actions_on_cut)},
	{ "Copy", NULL, N_("mcen_me_inbox_copy"),  "<CTRL>C", NULL, G_CALLBACK (modest_ui_actions_on_copy)},
	{ "Paste", NULL, N_("mcen_me_inbox_paste"),  "<CTRL>V", NULL, G_CALLBACK (modest_ui_actions_on_paste)},
	{ "SelectAll", NULL, N_("mcen_me_viewer_selectall"),  "<CTRL>A", NULL, G_CALLBACK (modest_ui_actions_on_select_all)},
#endif
	{ "SelectFont", NULL, N_("mcen_me_editor_font"), NULL, NULL, G_CALLBACK (modest_ui_actions_msg_edit_on_select_font)},
#ifndef MODEST_TOOLKIT_HILDON2
	{ "SelectContacts", NULL, N_("mcen_me_editor_selectrecipients"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_select_contacts)},
#endif
	{ "CheckNames", NULL, N_("mcen_me_editor_checknames"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_check_names)},
	{ "CloseWindow", NULL, N_("mcen_me_inbox_close_window"), "<CTRL>W", NULL, G_CALLBACK (modest_ui_actions_on_close_window)},
	{ "CloseAllWindows", NULL, N_("mcen_me_inbox_close_windows"), "<CTRL>Q", NULL, G_CALLBACK (modest_ui_actions_on_quit) },
	{ "Help", NULL, N_("mcen_me_inbox_help"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_help) },
#ifndef MODEST_TOOLKIT_HILDON2
        { "SearchMessages", NULL, N_("mcen_me_inbox_search"), "<CTRL>E", NULL,  G_CALLBACK (modest_ui_actions_on_search_messages) },
#endif
	/* KEY ACCELERATOR ACTIONS */
	{ "ZoomPlus", NULL, N_("Zoom +"), "F7", NULL, G_CALLBACK (modest_ui_actions_on_zoom_plus) },
	{ "ZoomMinus", NULL, N_("Zoom -"), "F8", NULL, G_CALLBACK (modest_ui_actions_on_zoom_minus) },
	{ "SendShortcut", NULL, N_("Send email"), "<CTRL>KP_Enter", NULL, G_CALLBACK (modest_ui_actions_on_send) },
	{ "SendShortcut2", NULL, N_("Send email"), "<CTRL>Return", NULL, G_CALLBACK (modest_ui_actions_on_send) },
	{ "ToggleFullscreen", NULL, N_("Toggle fullscreen"), "F6", NULL, G_CALLBACK (modest_ui_actions_on_change_fullscreen) },
	{ "CloseWindowShortcut", NULL, NULL, "Escape", NULL, NULL },

	/* TOOLBAR ACTIONS */
#ifndef MODEST_TOOLKIT_HILDON2
	{ "ToolbarSend", MODEST_TOOLBAR_ICON_MAIL_SEND, N_("mcen_me_editor_send"),  "<Control>Return", NULL,  G_CALLBACK (modest_ui_actions_on_send) },
#endif
#ifdef MODEST_TOOLKIT_GTK
	{ "ToolbarAttach", "stock_attach", N_("mcen_me_editor_attachfile"),  NULL, NULL,  G_CALLBACK (modest_ui_actions_on_attach_file) },
#endif
};

static const GtkToggleActionEntry modest_msg_edit_toggle_action_entries [] = {

	/* VIEW */
#ifdef MODEST_TOOLKIT_HILDON2
	{ "ViewCcField",   NULL,    N_("mcen_me_editor_showcc"),  NULL, NULL,  G_CALLBACK (modest_ui_actions_on_toggle_show_cc), FALSE },
	{ "ViewBccField",  NULL,    N_("mcen_me_editor_showbcc"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_show_bcc), FALSE },
#else
	{ "ViewCcField",   NULL,    N_("mcen_me_editor_showcc"),  NULL, NULL,  G_CALLBACK (modest_ui_actions_on_toggle_show_cc), TRUE },
	{ "ViewBccField",  NULL,    N_("mcen_me_editor_showbcc"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_show_bcc), TRUE },
#endif

	/* Fullscreen toggle */
	{ "ViewToggleFullscreen", NULL, N_("mcen_me_inbox_fullscreen"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_fullscreen), FALSE},
	/* Toolbar visibility */
	{ "ViewShowToolbarNormalScreen", NULL, N_("mcen_me_inbox_normalview"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_toolbar), TRUE },
	{ "ViewShowToolbarFullScreen", NULL, N_("mcen_me_inbox_fullscreen"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_toolbar), TRUE },

	/* Rich text editor functions */
	{ "ActionsBulletedList", NULL, N_("mcen_me_editor_bullets"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_bullets), FALSE },

	/* Toolbar buttons */
	{ "ActionsBold", MODEST_TOOLBAR_ICON_BOLD, MODEST_TOOLBAR_ICON_BOLD, NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_bold), FALSE },
	{ "ActionsItalics", MODEST_TOOLBAR_ICON_ITALIC, MODEST_TOOLBAR_ICON_ITALIC, NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_italics), FALSE },

	/* Find in page */
	{ "FindInMessage", NULL, N_("mcen_me_viewer_find"), "<CTRL>F", NULL, G_CALLBACK (modest_ui_actions_on_toggle_find_in_page), FALSE },
};

static const GtkRadioActionEntry modest_msg_edit_alignment_radio_action_entries [] = {
	{ "AlignmentLeft", NULL, N_("mcen_me_editor_align_left"), NULL, NULL, GTK_JUSTIFY_LEFT },
	{ "AlignmentCenter", NULL, N_("mcen_me_editor_align_centred"), NULL, NULL, GTK_JUSTIFY_CENTER },
	{ "AlignmentRight", NULL, N_("mcen_me_editor_align_right"), NULL, NULL, GTK_JUSTIFY_RIGHT },
};

static const GtkRadioActionEntry modest_msg_edit_priority_action_entries [] = {
	{ "MessagePriorityHigh", NULL, N_("mcen_me_editor_priority_high"), NULL, NULL, TNY_HEADER_FLAG_HIGH_PRIORITY },
	{ "MessagePriorityNormal", NULL, N_("mcen_me_editor_priority_normal"), NULL, NULL, TNY_HEADER_FLAG_NORMAL_PRIORITY },
	{ "MessagePriorityLow", NULL, N_("mcen_me_editor_priority_low"), NULL, NULL, TNY_HEADER_FLAG_LOW_PRIORITY },
};

static const GtkRadioActionEntry modest_msg_edit_file_format_action_entries [] = {
	{ "FileFormatPlainText", NULL, N_("mcen_me_editor_plain_text"), NULL, NULL, MODEST_FILE_FORMAT_PLAIN_TEXT },
	{ "FileFormatFormattedText", NULL, N_("mcen_me_editor_formatted_text"), NULL, NULL, MODEST_FILE_FORMAT_FORMATTED_TEXT },
};

G_END_DECLS
#endif /* __MODEST_MSG_EDIT_WINDOW_UI_H__ */

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

G_BEGIN_DECLS

static const GtkActionEntry modest_msg_edit_action_entries [] = {

	/* Toplevel menus */
	{ "View", NULL, N_("mcen_me_inbox_view") },
	{ "Edit", NULL, N_("mcen_me_inbox_edit") },
	{ "Format", NULL, N_("mcen_me_editor_format") },
	{ "Alignment", NULL, N_("mcen_me_editor_align") },
	{ "Attachments", NULL, N_("mcen_me_viewer_attachments") },
	{ "Zoom", NULL, N_("mcen_me_viewer_zoom") },
	{ "MessagePriority", NULL, N_("mcen_me_editor_message_priority") },
	{ "FileFormat", NULL, N_("mcen_me_editor_file_format") },
	{ "Tools", NULL, N_("mcen_me_inbox_tools") },
	{ "ShowToolbar", NULL, N_("mcen_me_inbox_toolbar") },

	/* ACTIONS */
	{ "ActionsSend", NULL, N_("mcen_me_editor_send"),  NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send) },
/* 	{ "ActionsFontColor", GTK_STOCK_SELECT_COLOR, N_("Color"), NULL, N_("Change text color"), G_CALLBACK (modest_ui_actions_on_select_editor_color)}, */
/* 	{ "BackgroundColor", GTK_STOCK_SELECT_COLOR, N_("Background color"), NULL, N_("Change background color"), G_CALLBACK (modest_ui_actions_on_select_editor_background_color)}, */
	{ "InsertImage", NULL, N_("mcen_me_editor_attach_inlineimage"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_insert_image)},
	{ "Undo", NULL, N_("mcen_me_inbox_undo"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_undo)},
	{ "Cut", NULL, N_("mcen_me_inbox_cut"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_cut)},
	{ "Copy", NULL, N_("mcen_me_inbox_copy"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_copy)},
	{ "Paste", NULL, N_("mcen_me_inbox_paste"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_paste)},
	{ "SelectAll", NULL, N_("mcen_me_viewer_selectall"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_select_all)},
	{ "SelectFont", NULL, N_("mcen_me_editor_font"), NULL, NULL, G_CALLBACK (modest_ui_actions_msg_edit_on_select_font)},

	/* KEY ACCELERATOR ACTIONS */
	{ "ZoomPlus", NULL, N_("Zoom +"), "F7", NULL, G_CALLBACK (modest_ui_actions_on_zoom_plus) },
	{ "ZoomMinus", NULL, N_("Zoom -"), "F8", NULL, G_CALLBACK (modest_ui_actions_on_zoom_minus) },
 	{ "ToggleFullscreen", NULL, N_("Toggle fullscreen"), "F6", NULL, G_CALLBACK (modest_ui_actions_on_change_fullscreen) },

	/* TOOLBAR ACTIONS */
	{ "ToolbarSend", MODEST_STOCK_MAIL_SEND, N_("qgn_toolb_messagin_send"),  NULL, NULL,  G_CALLBACK (modest_ui_actions_on_send) },

};

static const GtkToggleActionEntry modest_msg_edit_toggle_action_entries [] = {

	/* VIEW */
	{ "ViewCcField",   NULL,    N_("mcen_me_editor_showcc"),  NULL, NULL,  G_CALLBACK (modest_ui_actions_on_toggle_show_cc), TRUE  },
	{ "ViewBccField",  NULL,    N_("mcen_me_editor_showbcc"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_show_bcc), TRUE },

	/* Fullscreen toggle */
	{ "ViewToggleFullscreen", NULL, N_("mcen_me_inbox_fullscreen"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_fullscreen), FALSE},
	/* Toolbar visibility */
	{ "ViewShowToolbarNormalScreen", NULL, N_("mcen_me_inbox_normalview"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_toolbar), TRUE },
	{ "ViewShowToolbarFullScreen", NULL, N_("mcen_me_inbox_optimizedview"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_toolbar), TRUE },

	/* Rich text editor functions */
	{ "ActionsBulletedList", NULL, N_("mcen_me_editor_bullets"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_bullets), FALSE },

	/* Toolbar buttons */
	{ "ActionsBold", GTK_STOCK_BOLD, NULL, NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_bold), FALSE },
	{ "ActionsItalics", GTK_STOCK_ITALIC, NULL, NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_italics), FALSE },

};

static const GtkRadioActionEntry modest_msg_edit_alignment_radio_action_entries [] = {
	{ "AlignmentLeft", NULL, N_("mcen_me_editor_align_left"), NULL, NULL, GTK_JUSTIFY_LEFT },
	{ "AlignmentCenter", NULL, N_("mcen_me_editor_align_centred"), NULL, NULL, GTK_JUSTIFY_CENTER },
	{ "AlignmentRight", NULL, N_("mcen_me_editor_align_right"), NULL, NULL, GTK_JUSTIFY_RIGHT },
};

static const GtkRadioActionEntry modest_msg_edit_zoom_action_entries [] = {
	{ "Zoom50", NULL, N_("mcen_me_viewer_50"), NULL, NULL, 50 },
	{ "Zoom100", NULL, N_("mcen_me_viewer_100"), NULL, NULL, 100 },
	{ "Zoom150", NULL, N_("mcen_me_viewer_150"), NULL, NULL, 150 },
	{ "Zoom200", NULL, N_("mcen_me_viewer_200"), NULL, NULL, 200 }
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

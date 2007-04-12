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
	{ "View", NULL, N_("_View") },
	{ "Edit", NULL, N_("_Edit") },
	{ "Insert", NULL, N_("_Insert") },
	{ "Format", NULL, N_("For_mat") },
	{ "Alignment", NULL, N_("_Alignment") },
	{ "Attachments", NULL, N_("Attachments") },
	{ "Zoom", NULL, N_("Zoom") },

	/* ACTIONS */
	{ "ActionsSend", MODEST_STOCK_MAIL_SEND, N_("Send"),  NULL, N_("Send a message"),  G_CALLBACK (modest_ui_actions_on_send) },
	{ "ActionsFontColor", GTK_STOCK_SELECT_COLOR, N_("Color"), NULL, N_("Change text color"), G_CALLBACK (modest_ui_actions_on_select_editor_color)},
	{ "BackgroundColor", GTK_STOCK_SELECT_COLOR, N_("Background color"), NULL, N_("Change background color"), G_CALLBACK (modest_ui_actions_on_select_editor_background_color)},
	{ "InsertImage", NULL, N_("Insert image..."), NULL, N_("Insert image"), G_CALLBACK (modest_ui_actions_on_insert_image)},
	{ "Cut", GTK_STOCK_CUT, N_("Cut"), NULL, N_("Cut selection"), G_CALLBACK (modest_ui_actions_on_cut)},
	{ "Copy", GTK_STOCK_COPY, N_("Copy"), NULL, N_("Copy selection"), G_CALLBACK (modest_ui_actions_on_copy)},
	{ "Paste", GTK_STOCK_PASTE, N_("Paste"), NULL, N_("Paste selection"), G_CALLBACK (modest_ui_actions_on_paste)},
	{ "SelectAll", NULL, N_("Select all"), NULL, N_("Select all"), G_CALLBACK (modest_ui_actions_on_select_all)},

	/* KEY ACCELERATOR ACTIONS */
	{ "ZoomPlus", NULL, N_("Zoom +"), "F7", NULL, G_CALLBACK (modest_ui_actions_on_zoom_plus) },
	{ "ZoomMinus", NULL, N_("Zoom -"), "F8", NULL, G_CALLBACK (modest_ui_actions_on_zoom_minus) },
 	{ "ToggleFullscreen", NULL, N_("Toggle fullscreen"), "F6", NULL, G_CALLBACK (modest_ui_actions_on_change_fullscreen) },

};

static const GtkToggleActionEntry modest_msg_edit_toggle_action_entries [] = {

	/* VIEW */
	{ "ViewCcField",   NULL,    N_("Cc: field"),  NULL, N_("Shows the Cc: field"),  G_CALLBACK (modest_ui_actions_on_toggle_show_cc), TRUE  },
	{ "ViewBccField",  NULL,    N_("Bcc: filed"), NULL, N_("Shows the Bcc: field"), G_CALLBACK (modest_ui_actions_on_toggle_show_bcc), TRUE },

	/* Fullscreen toggle */
	{ "ShowToggleFullscreen", GTK_STOCK_FULLSCREEN, N_("Show fullscreen"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_toggle_fullscreen), FALSE},

	/* Rich text editor functions */
	{ "ActionsBold", GTK_STOCK_BOLD, N_("Bold"), NULL, N_("Use bold"), G_CALLBACK (modest_ui_actions_on_toggle_bold), FALSE },
	{ "ActionsItalics", GTK_STOCK_ITALIC, N_("Italics"), NULL, N_("Use italics"), G_CALLBACK (modest_ui_actions_on_toggle_italics), FALSE },
	{ "ActionsBulletedList", MODEST_TOOLBAR_ICON_FORMAT_BULLETS, N_("Bullet list"), NULL, N_("Add a bullet list"), G_CALLBACK (modest_ui_actions_on_toggle_bullets), FALSE },
};

static const GtkRadioActionEntry modest_msg_edit_alignment_radio_action_entries [] = {
	{ "AlignmentLeft", NULL, N_("Left"), NULL, N_("Align to the left"), GTK_JUSTIFY_LEFT },
	{ "AlignmentCenter", NULL, N_("Center"), NULL, N_("Align to the center"), GTK_JUSTIFY_CENTER },
	{ "AlignmentRight", NULL, N_("Right"), NULL, N_("Align to the right"), GTK_JUSTIFY_RIGHT },
};

static const GtkRadioActionEntry modest_msg_edit_zoom_action_entries [] = {
	{ "Zoom50", NULL, N_("mcen_me_viewer_50"), NULL, NULL, 50 },
	{ "Zoom100", NULL, N_("mcen_me_viewer_100"), NULL, NULL, 100 },
	{ "Zoom150", NULL, N_("mcen_me_viewer_150"), NULL, NULL, 150 },
	{ "Zoom200", NULL, N_("mcen_me_viewer_200"), NULL, NULL, 200 }
};



G_END_DECLS
#endif /* __MODEST_MSG_EDIT_WINDOW_UI_H__ */

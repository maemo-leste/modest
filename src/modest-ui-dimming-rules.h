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

#ifndef __MODEST_UI_DIMMING_RULES_H__
#define __MODEST_UI_DIMMING_RULES_H__

#include <widgets/modest-main-window.h>
#include <widgets/modest-msg-edit-window.h>
#include <widgets/modest-recpt-view.h>

G_BEGIN_DECLS

/* Window dimming state */
DimmedState *modest_ui_dimming_rules_define_dimming_state (ModestWindow *window);

/* Menu & toolbar dimming rules */
gboolean modest_ui_dimming_rules_on_new_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_new_folder (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_new_msg_or_folder (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_delete (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_delete_folder (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_delete_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_rename_folder (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_sort (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_open_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_reply_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_contents_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_always_dimmed (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_details (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_fetch_images (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_mark_as_read_msg_in_view (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_mark_as_unread_msg_in_view (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_mark_as_read_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_mark_as_unread_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_move_to (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_main_window_move_to (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_view_window_move_to (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_paste (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_delete_msgs (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_select_all (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_view_attachments (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_save_attachments (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_remove_attachments (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_undo (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_redo (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_copy (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_cut (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_view_previous (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_view_next (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_tools_smtp_servers (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_cancel_sending (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_cancel_sending_all (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_send_receive (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_send_receive_all (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_add_to_contacts (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_find_in_msg (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_set_style (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_editor_show_toolbar (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_zoom (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_send (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_editor_remove_attachment (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_editor_paste (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_editor_paste_show_menu (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_save_to_drafts (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_insert_image (ModestWindow *win, gpointer user_data);
#ifdef MODEST_TOOLKIT_HILDON2
gboolean modest_ui_dimming_rules_on_header_window_move_to (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_folder_window_move_to (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_folder_window_delete (ModestWindow *win, gpointer user_data);
gboolean modest_ui_dimming_rules_on_edit_accounts (ModestWindow *win, gpointer user_data);
#endif

G_END_DECLS
#endif 

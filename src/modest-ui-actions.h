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

#ifndef __MODEST_UI_ACTIONS_H__
#define __MODEST_UI_ACTIONS_H__

#include <widgets/modest-main-window.h>
#include <widgets/modest-msg-edit-window.h>
#include <widgets/modest-recpt-view.h>

G_BEGIN_DECLS

/* Menu & toolbar actions */
void     modest_ui_actions_on_about         (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_delete        (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_quit          (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_close_window  (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_new_account      (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_accounts      (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_add_to_contacts   (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_new_msg       (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_open           (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_reply         (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_forward       (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_reply_all     (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_next          (GtkAction *action, ModestWindow *main_window);

void     modest_ui_actions_on_prev          (GtkAction *action, ModestWindow *main_window);

void     modest_ui_actions_on_message_details (GtkAction *action, ModestWindow *win);

void	 modest_ui_actions_toggle_view	     (GtkAction *action, ModestMainWindow *main_window);

/* Widget actions */
void     modest_ui_actions_on_header_selected          (ModestHeaderView *folder_view, 
							TnyHeader *header,
							ModestMainWindow *main_window);
void     modest_ui_actions_on_header_activated         (ModestHeaderView *folder_view, 
						         TnyHeader *header,
						         ModestMainWindow *main_window);

void     modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
							 TnyFolder *folder, 
							 gboolean selected,
							 ModestMainWindow *main_window);

void     modest_ui_actions_on_online_toggle_toggled    (GtkToggleButton *toggle,
							 ModestMainWindow *main_window);

void     modest_ui_actions_on_item_not_found           (ModestHeaderView *header_view,
							 ModestItemType type,
							 ModestWindow *window);

void     modest_ui_actions_on_msg_link_hover           (ModestMsgView *msgview, const gchar* link,
							ModestWindow *win);

void     modest_ui_actions_on_msg_link_clicked         (ModestMsgView *msgview, const gchar* link,
							ModestWindow *win);

void     modest_ui_actions_on_msg_link_contextual      (ModestMsgView *msgview, const gchar* link,
							ModestWindow *win);

void     modest_ui_actions_on_msg_attachment_clicked   (ModestMsgView *msgview, TnyMimePart *mime_part,
							ModestWindow *win);

void     modest_ui_actions_on_msg_recpt_activated   (ModestMsgView *msgview, const gchar *address,
						     ModestWindow *win);

void     modest_ui_actions_on_send                     (GtkWidget *widget,
							ModestMsgEditWindow *edit_window);

void     modest_ui_actions_on_toggle_bold              (GtkToggleAction *action,
							ModestMsgEditWindow *window);

void     modest_ui_actions_on_toggle_italics           (GtkToggleAction *action,
							ModestMsgEditWindow *window);

void     modest_ui_actions_on_toggle_bullets           (GtkToggleAction *action,
							ModestMsgEditWindow *window);

void     modest_ui_actions_on_change_justify      (GtkRadioAction *action,
						   GtkRadioAction *selected,
						   ModestMsgEditWindow *window);

void     modest_ui_actions_on_select_editor_color      (GtkAction *action,
							ModestMsgEditWindow *window);

void     modest_ui_actions_on_select_editor_background_color      (GtkAction *action,
								   ModestMsgEditWindow *window);

void     modest_ui_actions_on_insert_image             (GtkAction *action,
							ModestMsgEditWindow *window);

void    modest_ui_actions_on_send_receive              (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_new_folder               (GtkAction *action,
							ModestMainWindow *main_window);

void     modest_ui_actions_on_rename_folder            (GtkAction *action,
							ModestMainWindow *main_window);

void     modest_ui_actions_on_delete_folder            (GtkAction *action,
							 ModestMainWindow *main_window);

void     modest_ui_actions_on_move_folder_to_trash_folder     (GtkAction *action,
							       ModestMainWindow *main_window);

void     modest_ui_actions_on_connection_changed    (TnyDevice *device, gboolean online,
						     ModestMainWindow *main_window);

void     modest_ui_actions_on_password_requested (TnyAccountStore *account_store,
						  const gchar* account_name,
						  gchar **password,  gboolean *cancel, 
						  gboolean *remember, ModestMainWindow *main_window);

void     modest_ui_actions_on_cut (GtkAction *action,
				   ModestWindow *window);
void     modest_ui_actions_on_copy (GtkAction *action,
				    ModestWindow *window);
void     modest_ui_actions_on_paste (GtkAction *action,
				     ModestWindow *window);
void     modest_ui_actions_on_select_all (GtkAction *action,
					  ModestWindow *window);

void     modest_ui_actions_on_change_zoom (GtkRadioAction *action,
					   GtkRadioAction *selected,
					   ModestWindow *window);

void     modest_ui_actions_on_toggle_fullscreen    (GtkToggleAction *toggle,
						    ModestWindow *window);


G_END_DECLS
#endif /* __MODEST_UI_ACTIONS_H__ */

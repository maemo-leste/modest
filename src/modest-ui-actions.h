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

G_BEGIN_DECLS

/* Menu & toolbar actions */
void     modest_ui_actions_on_about         (GtkWidget *widget, ModestWindow *win);

void     modest_ui_actions_on_delete        (GtkWidget *widget, ModestMainWindow *main_window);

void     modest_ui_actions_on_quit          (GtkWidget *widget, ModestWindow *win);

void     modest_ui_actions_on_accounts      (GtkWidget *widget, ModestWindow *win);

void     modest_ui_actions_on_new_msg       (GtkWidget *widget, ModestWindow *win);

void     modest_ui_actions_on_open           (GtkWidget *widget, ModestWindow *win);

void     modest_ui_actions_on_reply         (GtkWidget *widget, ModestMainWindow *main_window);

void     modest_ui_actions_on_forward       (GtkWidget *widget, ModestMainWindow *main_window);

void     modest_ui_actions_on_reply_all     (GtkWidget *widget, ModestMainWindow *main_window);

void     modest_ui_actions_on_next          (GtkWidget *widget, ModestMainWindow *main_window);

void     modest_ui_actions_on_prev          (GtkWidget *widget, ModestMainWindow *main_window);

void	 modest_ui_actions_toggle_view	     (GtkWidget *widget, ModestMainWindow *main_window);

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

void     modest_ui_actions_on_header_status_update     (ModestHeaderView *header_view, 
							 const gchar *msg,
							 gint num, 
							 gint total, 
							 ModestMainWindow *main_window);

void     modest_ui_actions_on_msg_link_hover           (ModestMsgView *msgview, const gchar* link,
							ModestWindow *win);

void     modest_ui_actions_on_msg_link_clicked         (ModestMsgView *msgview, const gchar* link,
							ModestWindow *win);

void     modest_ui_actions_on_msg_attachment_clicked   (ModestMsgView *msgview, int index,
							ModestWindow *win);

void     modest_ui_actions_on_send                     (GtkWidget *widget,
							ModestMsgEditWindow *edit_window);

void     modest_ui_actions_on_new_folder               (GtkWidget *widget,
							 ModestMainWindow *main_window);

void     modest_ui_actions_on_rename_folder            (GtkWidget *widget,
							 ModestMainWindow *main_window);

void     modest_ui_actions_on_delete_folder            (GtkWidget *widget,
							 ModestMainWindow *main_window);

void     modest_ui_actions_on_move_folder_to_trash_folder     (GtkWidget *widget,
							       ModestMainWindow *main_window);

void     modest_ui_actions_on_connection_changed    (TnyDevice *device, gboolean online,
						     ModestMainWindow *main_window);


void     modest_ui_actions_on_accounts_reloaded     (TnyAccountStore *store, 
						     gpointer user_data);

void     modest_ui_actions_on_folder_moved          (ModestFolderView *folder_view,
						     TnyFolder        *folder, 
						     TnyFolderStore   *parent,
						     gboolean         *done,
						     gpointer          user_data);
G_END_DECLS
#endif /* __MODEST_UI_ACTIONS_H__ */

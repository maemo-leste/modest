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
#include "modest-mail-operation.h"
#include "modest-tny-send-queue.h"
#include "modest-plugin-ui-actions.h"

G_BEGIN_DECLS

/* Menu & toolbar actions */
void     modest_ui_actions_on_about         (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_refresh_message_window_after_delete (ModestMsgViewWindow* win);

void     modest_ui_actions_on_delete_message     (GtkAction *action, ModestWindow *win);

gboolean modest_ui_actions_on_edit_mode_delete_message (ModestWindow *win);

gboolean modest_ui_actions_on_edit_mode_delete_folder (ModestWindow *win);

void     modest_ui_actions_on_delete_message_or_folder (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_quit          (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_close_window  (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_new_account      (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_accounts      (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_smtp_servers  (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_add_to_contacts   (GtkAction *action, ModestWindow *win);
void     modest_ui_actions_on_add_to_contacts   (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_select_contacts (GtkAction *action, ModestMsgEditWindow *win);

void     modest_ui_actions_on_open_addressbook (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_check_names (GtkAction *action, ModestMsgEditWindow *win);

void     modest_ui_actions_on_new_msg       (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_new_msg_or_folder (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_open           (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_reply         (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_reply_calendar (ModestWindow *win, TnyMsg *msg, TnyList *header_pairs);

void     modest_ui_actions_on_forward       (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_sort          (GtkAction *action, ModestWindow *window);

void     modest_ui_actions_on_reply_all     (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_next          (GtkAction *action, ModestWindow *main_window);

void     modest_ui_actions_on_prev          (GtkAction *action, ModestWindow *main_window);

void     modest_ui_actions_on_details       (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_limit_error   (GtkAction *action, ModestWindow *win);

gboolean     modest_ui_actions_on_edit_mode_move_to       (ModestWindow *win);

void     modest_ui_actions_on_move_to       (GtkAction *action, ModestWindow *win);

void     modest_ui_actions_on_settings      (GtkAction *action, ModestWindow *win);

/**
 * modest_ui_actions_on_help:
 * @action: a #GtkAction
 * @win: a #ModestWindow
 * 
 * Shows the help dialog
 **/
void     modest_ui_actions_on_help          (GtkAction *action, GtkWindow *win);


/**
 * modest_ui_actions_on_csm_elp:
 * @action: a #GtkAction
 * @win: a #ModestWindow
 * 
 * Shows the help dialog for folder view CSM. It shows the help ID
 * which refers to the currently selected folder if any
 **/
void     modest_ui_actions_on_csm_help      (GtkAction *action, GtkWindow *win);

/**
 * modest_ui_actions_toggle_folders_view:
 * @action: the #GtkAction
 * @main_window: the #ModestMainWindow
 * 
 * this action switches between split view (with a folders pane at the
 * left) and simple view without the left pane with the folders and
 * accounts. Maybe it's only useful for Maemo code, but as it uses
 * generic code could be used also by the GNOME UI
 **/
void	 modest_ui_actions_toggle_folders_view	     (GtkAction *action, 
						      ModestMainWindow *main_window);

/**
 * modest_ui_actions_toggle_folders_view:
 * @action: the #GtkAction
 * @main_window: the #ModestMainWindow
 * 
 * this action shows or hides the column titles of the header list
 * view. It also enables the two lines rendering for the treeview rows
 **/
void	 modest_ui_actions_toggle_header_list_view    (GtkAction *action, 
						       ModestMainWindow *main_window);

/* Widget actions */
void     modest_ui_actions_on_header_selected          (ModestHeaderView *folder_view, 
							TnyHeader *header,
							ModestMainWindow *main_window);
void     modest_ui_actions_on_header_activated         (ModestHeaderView *folder_view, 
							TnyHeader *header,
							GtkTreePath *path,
							ModestWindow *main_window);

void     modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
							 TnyFolderStore *folder_store, 
							 gboolean selected,
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

gboolean modest_ui_actions_on_send                     (GtkWidget *widget,
							ModestMsgEditWindow *edit_window);
gboolean modest_ui_actions_on_save_to_drafts           (GtkWidget *widget, 
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

void     modest_ui_actions_on_insert_image             (GObject *object,
							ModestMsgEditWindow *window);

void     modest_ui_actions_on_attach_file             (GtkAction *action,
						       ModestMsgEditWindow *window);

void     modest_ui_actions_on_remove_attachments      (GtkAction *action,
						       ModestMsgEditWindow *window);
void     modest_ui_actions_on_mark_as_read            (GtkAction *action,
						       ModestWindow *window);

void     modest_ui_actions_on_mark_as_unread            (GtkAction *action,
							 ModestWindow *window);

void     modest_ui_actions_cancel_send (GtkAction *action,  ModestWindow *win);

/**
 * modest_ui_actions_do_send_receive_all:
 * @win: the window that will be used as source of the refresh mail operation
 * @force_connection: whether or not the code should try to force a new connection if we're offline
 * @poke_status: wheter ot not we want to poke the status of all mail folders
 * @interactive: is coming from an interactive send receive.
 * 
 * Refreshes all the accounts
 **/
void    modest_ui_actions_do_send_receive_all          (ModestWindow *win,
							gboolean force_connection,
							gboolean poke_status,
							gboolean interactive);

/**
 * modest_ui_actions_do_send_receive:
 * @account_name: the name of the Modest account or NULL
 * @force_connection: whether or not the code should try to force a new connection if we're offline
 * @poke_status: wheter ot not we want to poke the status of all mail folders
 * @interactive: is coming from an interactive send receive
 * @win: the window that will be used as source of the refresh mail operation
 * 
 * Refreshes the Modest account whose name is passed as argument. If
 * NULL is passed as #account_name then this function refreses the
 * active account, if no active account is defined then it picks the
 * default account
 **/
void    modest_ui_actions_do_send_receive              (const gchar *account_name,
							gboolean force_connection,
							gboolean poke_status,
							gboolean interactive,
							ModestWindow *win);

/**
 * modest_ui_actions_on_send_receive:
 * @action: a #GtkAction
 * @win: the Window that contains the action
 * 
 * Handles the activation of the send receive action, for example
 * clicks on Send&Receive button in the main toolbar
 **/
void    modest_ui_actions_on_send_receive              (GtkAction *action, 
							ModestWindow *win);

void     modest_ui_actions_on_new_folder               (GtkAction *action,
							ModestWindow *main_window);

gboolean     modest_ui_actions_on_edit_mode_rename_folder            (ModestWindow *window);

void     modest_ui_actions_on_rename_folder            (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_delete_folder            (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_move_folder_to_trash_folder     (GtkAction *action,
							       ModestMainWindow *main_window);

void     modest_ui_actions_on_password_requested (TnyAccountStore *account_store,
						  const gchar* server_account_name,
						  gchar **username, gchar **password, gboolean *cancel, 
						  gboolean *remember, ModestMainWindow *main_window);

void     modest_ui_actions_on_undo                     (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_redo                     (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_cut                      (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_copy                     (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_zoom_plus (GtkAction *action,
					 ModestWindow *window);

void     modest_ui_actions_on_zoom_minus (GtkAction *action,
					  ModestWindow *window);

void     modest_ui_actions_on_toggle_fullscreen    (GtkToggleAction *toggle,
						    ModestWindow *window);

void     modest_ui_actions_on_change_fullscreen    (GtkAction *action,
						    ModestWindow *window);

void     modest_ui_actions_on_paste                    (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_select_all               (GtkAction *action,
							ModestWindow *window);

void     modest_ui_actions_on_toggle_show_cc (GtkToggleAction *toggle,
					      ModestMsgEditWindow *window);

void     modest_ui_actions_on_toggle_show_bcc (GtkToggleAction *toggle,
					      ModestMsgEditWindow *window);

void     modest_ui_actions_on_change_zoom              (GtkRadioAction *action,
							GtkRadioAction *selected,
							ModestWindow *window);

void     modest_ui_actions_msg_edit_on_change_priority (GtkRadioAction *action,
							GtkRadioAction *selected,
							ModestWindow *window);

void     modest_ui_actions_msg_edit_on_change_file_format (GtkRadioAction *action,
							   GtkRadioAction *selected,
							   ModestWindow *window);

void     modest_ui_actions_msg_edit_on_select_font (GtkAction *action,
						    ModestMsgEditWindow *window);

/**
 * modest_ui_actions_on_toggle_toolbar:
 * @toggle: 
 * @window: 
 * 
 * Hides/Shows the toolbars of the #ModestWindow instances
 **/
void     modest_ui_actions_on_toggle_toolbar           (GtkToggleAction *toggle, 
							ModestWindow *window);

/**
 * modest_ui_actions_on_folder_display_name_changed:
 * @folder_view: a #ModestFolderView
 * @display_name: the new window title
 * @window: a #GtkWindow
 * 
 * Sets the title of the window to the value specified by
 * display_name. This function is used typically as a callback to the
 * "folder-display-name-changed" signal from the #ModestFolderView
 **/
void     modest_ui_actions_on_folder_display_name_changed (ModestFolderView *folder_view,
							   const gchar *display_name,
							   GtkWindow *window);

void     modest_ui_actions_view_attachment                (GtkAction *action,
							   ModestWindow *window);

void     modest_ui_actions_save_attachments               (GtkAction *action,
							   ModestWindow *window);

void     modest_ui_actions_remove_attachments             (GtkAction *action,
							   ModestWindow *window);

/**
 * modest_ui_actions_on_retrieve_msg_contents:
 * @action: the #GtkAction
 * @window: the #ModestWindow that issues the action
 * 
 * Retrieve the contents of the selected messages in the header view
 **/
void     modest_ui_actions_on_retrieve_msg_contents       (GtkAction *action,
							   ModestWindow *window);

void
modest_ui_actions_on_email_menu_activated (GtkAction *action,
					  ModestWindow *window);

void
modest_ui_actions_on_edit_menu_activated (GtkAction *action,
					  ModestWindow *window);

void
modest_ui_actions_on_format_menu_activated (GtkAction *action,
					    ModestWindow *window);

void
modest_ui_actions_on_view_menu_activated (GtkAction *action,
					  ModestWindow *window);

void
modest_ui_actions_on_tools_menu_activated (GtkAction *action,
					  ModestWindow *window);

void
modest_ui_actions_on_attachment_menu_activated (GtkAction *action,
						ModestWindow *window);

void
modest_ui_actions_on_toolbar_csm_menu_activated (GtkAction *action,
						 ModestWindow *window);

void
modest_ui_actions_on_folder_view_csm_menu_activated (GtkAction *action,
						     ModestWindow *window);

void
modest_ui_actions_on_header_view_csm_menu_activated (GtkAction *action,
						     ModestWindow *window);

void
modest_ui_actions_check_toolbar_dimming_rules (ModestWindow *window);

void
modest_ui_actions_check_menu_dimming_rules (ModestWindow *window);

/* Dimming rules groups */
#define MODEST_DIMMING_RULES_TOOLBAR "ModestToolbarDimmingRules"
#define MODEST_DIMMING_RULES_MENU "ModestMenuDimmingRules"
#define MODEST_DIMMING_RULES_CLIPBOARD "ModestClipboardDimmingRules"

/**
 * modest_ui_actions_move_folder_error_handler:
 * @mail_op: a #ModestMailOperation
 * @user_data: user data
 * 
 * manages an error in a mail operation that tries to move a folder
 **/
void     modest_ui_actions_move_folder_error_handler      (ModestMailOperation *mail_op, 
							   gpointer user_data);
/**
 * modest_ui_actions_send-receive_error_handler:
 * @mail_op: a #ModestMailOperation
 * @user_data: user data
 * 
 * manages an error in a mail operation that tries to execute
 * a send&receive operation.
 **/
void     modest_ui_actions_send_receive_error_handler      (ModestMailOperation *mail_op, 
							    gpointer user_data);

/**
 * modest_ui_actions_on_search_messages:
 * @action: a #GtkAction
 * @window: a #ModestWindow
 *
 * Shows the search messages dialog
 **/
void     modest_ui_actions_on_search_messages             (GtkAction *action,
							   ModestWindow *window);

/**
 * modest_ui_actions_on_find_in_page:
 * @action: a #GtkAction
 * @window: a #ModestWindow
 *
 * Toggles the visibility of the find in page toolbar
 **/
void     modest_ui_actions_on_toggle_find_in_page             (GtkAction *action,
							       ModestWindow *window);

/**
 * modest_ui_actions_msg_retrieval_check
 * @mail_op: a #ModestMailOperation
 * @header: a #TnyHeader
 * @msg: a #TnyMsg
 *
 * This function checks that the message has been retrieved
 * successfully. It it was not the case it unregisters the header from
 * the window manager because it won't do it automatically unless the
 * operation run fine
 *
 * Returns: TRUE if the operation was OK, otherwise FALSE
 **/
gboolean modest_ui_actions_msg_retrieval_check                (ModestMailOperation *mail_op, 
							       TnyHeader *header,
							       TnyMsg *msg);


/**
 * modest_ui_actions_disk_operations_error_handler
 * @mail_op: a #ModestMailOperation
 *
 * Error handler for retrieval operations like
 * modest_mail_operation_get_msg or
 * modest_mail_operation_get_msgs_full
 **/
void     modest_ui_actions_disk_operations_error_handler      (ModestMailOperation *mail_op,
							       gpointer user_data);

/* Show the account creation wizard dialog.
 * returns: TRUE if an account was created. FALSE if the user cancelled.
 */
gboolean modest_ui_actions_run_account_setup_wizard (ModestWindow *win);

gint modest_ui_actions_msgs_move_to_confirmation (ModestWindow *win,
						  TnyFolder *dest_folder,
						  gboolean delete,
						  TnyList *headers);

/*
 * modest_ui_actions_on_send_queue_error_happened:
 *
 * Method for handling errors in send queues
 */
void modest_ui_actions_on_send_queue_error_happened (TnySendQueue *self, 
						     TnyHeader *header, 
						     TnyMsg *msg, 
						     GError *err, 
						     gpointer user_data);

/*
 * modest_ui_actions_on_send_queue_status_changed:
 *
 * Method for handling changes in the status of the messages in the send queues
 */
void modest_ui_actions_on_send_queue_status_changed (ModestTnySendQueue *send_queue,
						     gchar *msg_id, 
						     guint status,
						     gpointer user_data);

/**
 * modest_ui_actions_compose_msg
 * @win: Modest main window (can be NULL)
 * @to_str: "To:" header, or NULL
 * @cc_str: "Cc:" header, or NULL
 * @bcc_str: "Bcc:" header, or NULL
 * @subject_str: Subject of the message, or NULL
 * @body_str: Body of the message (without signature), or NULL
 * @attachments: attachments List of file URIs to attach
 * @set_as_modified: wheter or not the message is set initially as modified or not
 *
 * Opens a new message editor for composing
 */
void modest_ui_actions_compose_msg (ModestWindow *win,
				    const gchar *to_str,
				    const gchar *cc_str,
				    const gchar *bcc_str,
				    const gchar *subject_str,
				    const gchar *body_str,
				    GSList *attachments,
				    gboolean set_as_modified);

void modest_ui_actions_on_account_connection_error (GtkWindow *parent_window,
						    TnyAccount *account);

gchar *modest_ui_actions_get_msg_already_deleted_error_msg (ModestWindow *win);

void modest_ui_actions_transfer_messages_helper (GtkWindow *win,
						 TnyFolder *src_folder,
						 TnyList *headers,
						 TnyFolder *dst_folder);

void modest_ui_actions_on_fetch_images (GtkAction *action,
					ModestWindow *window);

/**
 * modest_ui_actions_check_for_active_account:
 * @win: a #ModestWindow
 * @account_name: the account name of the account to check
 *
 * Check whether any connections are active, and cancel them if
 * the user wants.
 *
 * Returns: Returns TRUE if there was no problem, or if an operation
 * was cancelled so we can continue. Returns FALSE if the user chose
 * to cancel his request instead.
 **/
gboolean modest_ui_actions_check_for_active_account (ModestWindow *win,
						     const gchar* account_name);

G_END_DECLS
#endif /* __MODEST_UI_ACTIONS_H__ */

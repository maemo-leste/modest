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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <string.h>
#include <modest-runtime.h>
#include <modest-defs.h>
#include <modest-tny-folder.h>
#include <modest-tny-msg.h>
#include <modest-tny-account.h>
#include <modest-address-book.h>
#include "modest-error.h"
#include "modest-ui-actions.h"
#include "modest-tny-platform-factory.h"
#include "modest-platform.h"
#include "modest-debug.h"
#include <tny-mime-part.h>
#include <tny-error.h>
#include <tny-camel-folder.h>
#include <tny-camel-imap-folder.h>
#include <tny-camel-pop-folder.h>
#include <widgets/modest-header-window.h>
#include <widgets/modest-folder-window.h>
#include <widgets/modest-accounts-window.h>

#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-gtk.h>
#include <modest-maemo-utils.h>
#else
#include <gtk/modest-shell-window.h>
#endif
#include "modest-utils.h"
#include "widgets/modest-connection-specific-smtp-window.h"
#include "widgets/modest-ui-constants.h"
#include <widgets/modest-msg-view-window.h>
#include <widgets/modest-account-view-window.h>
#include <widgets/modest-details-dialog.h>
#include <widgets/modest-attachments-view.h>
#include "widgets/modest-folder-view.h"
#include "widgets/modest-global-settings-dialog.h"
#include "modest-account-mgr-helpers.h"
#include "modest-mail-operation.h"
#include "modest-text-utils.h"
#include <modest-widget-memory.h>
#include <tny-error.h>
#include <tny-simple-list.h>
#include <tny-msg-view.h>
#include <tny-device.h>
#include <tny-merge-folder.h>
#include <widgets/modest-toolkit-utils.h>
#include <tny-camel-bs-msg.h>
#include <tny-camel-bs-mime-part.h>

#include <gtk/gtk.h>
#include <gtkhtml/gtkhtml.h>

#define MODEST_MOVE_TO_DIALOG_FOLDER_VIEW "move-to-dialog-folder-view"

typedef struct _GetMsgAsyncHelper {
	ModestWindow *window;
	ModestMailOperation *mail_op;
	TnyIterator *iter;
	guint num_ops;
	GFunc func;
	gpointer user_data;
} GetMsgAsyncHelper;

typedef enum _ReplyForwardAction {
	ACTION_REPLY,
	ACTION_REPLY_TO_ALL,
	ACTION_FORWARD
} ReplyForwardAction;

typedef struct _ReplyForwardHelper {
	guint reply_forward_type;
	ReplyForwardAction action;
	gchar *account_name;
	gchar *mailbox;
	GtkWidget *parent_window;
	TnyHeader *header;
	TnyHeader *top_header;
	TnyMsg    *msg_part;
	TnyList *parts;
} ReplyForwardHelper;

typedef struct _MoveToHelper {
	GtkTreeRowReference *reference;
	GtkWidget *banner;
} MoveToHelper;

typedef struct _PasteAsAttachmentHelper {
	ModestMsgEditWindow *window;
	GtkWidget *banner;
} PasteAsAttachmentHelper;

typedef struct {
	TnyList *list;
	ModestWindow *win;
} MoveToInfo;

/*
 * The do_headers_action uses this kind of functions to perform some
 * action to each member of a list of headers
 */
typedef void (*HeadersFunc) (TnyHeader *header, ModestWindow *win, gpointer user_data);

static void     do_headers_action     (ModestWindow *win,
				       HeadersFunc func,
				       gpointer user_data);

static void     open_msg_cb            (ModestMailOperation *mail_op,
					TnyHeader *header,
					gboolean canceled,
					TnyMsg *msg,
					GError *err,
					gpointer user_data);

static void     reply_forward_cb       (ModestMailOperation *mail_op,
					TnyHeader *header,
					gboolean canceled,
					TnyMsg *msg,
					GError *err,
					gpointer user_data);

static void     reply_forward          (ReplyForwardAction action, ModestWindow *win);

static gint header_list_count_uncached_msgs (TnyList *header_list);

static gboolean connect_to_get_msg (ModestWindow *win,
				    gint num_of_uncached_msgs,
				    TnyAccount *account);

static gboolean remote_folder_has_leave_on_server (TnyFolderStore *folder);

static void     do_create_folder (ModestWindow *window,
				  TnyFolderStore *parent_folder,
				  const gchar *suggested_name);

static TnyAccount *get_account_from_folder_store (TnyFolderStore *folder_store);

static void modest_ui_actions_on_folder_window_move_to (GtkWidget *folder_view,
							TnyFolderStore *dst_folder,
							TnyList *selection,
							ModestWindow *win);

static void modest_ui_actions_on_window_move_to (GtkAction *action,
						 TnyList *list_to_move,
						 TnyFolderStore *dst_folder,
						 ModestWindow *win);

static void
modest_ui_actions_send_receive_offline (ModestMailOperation *mail_op, gpointer user_data);

/*
 * This function checks whether a TnyFolderStore is a pop account
 */
static gboolean
remote_folder_has_leave_on_server (TnyFolderStore *folder)
{
        TnyAccount *account;
	gboolean result;

        g_return_val_if_fail (TNY_IS_FOLDER_STORE (folder), FALSE);

	account = get_account_from_folder_store (folder);
	result = (modest_protocol_registry_protocol_type_has_leave_on_server (modest_runtime_get_protocol_registry (),
									      modest_tny_account_get_protocol_type (account)));
        g_object_unref (account);

	return result;
}

/* FIXME: this should be merged with the similar code in modest-account-view-window */
/* Show the account creation wizard dialog.
 * returns: TRUE if an account was created. FALSE if the user cancelled.
 */
gboolean
modest_ui_actions_run_account_setup_wizard (ModestWindow *win)
{
	gboolean result = FALSE;
	GtkWindow *wizard;
	gint dialog_response;

	/* there is no such wizard yet */
	wizard = GTK_WINDOW (modest_platform_get_account_settings_wizard ());
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr(), GTK_WINDOW (wizard), (GtkWindow *) win);

	if (!win) {
		GList *window_list;
		ModestWindowMgr *mgr;

		mgr = modest_runtime_get_window_mgr ();

		window_list = modest_window_mgr_get_window_list (mgr);
		if (window_list == NULL) {
			win = MODEST_WINDOW (modest_accounts_window_new ());
			if (modest_window_mgr_register_window (mgr, win, NULL)) {
				gtk_widget_show_all (GTK_WIDGET (win));
			} else {
				gtk_widget_destroy (GTK_WIDGET (win));
				win = NULL;
			}

		} else {
			g_list_free (window_list);
		}
	}

	if (win) {
		GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
		gtk_window_set_transient_for (GTK_WINDOW (wizard), toplevel);
	}

	/* make sure the mainwindow is visible. We need to present the
	   wizard again to give it the focus back. show_all are needed
	   in order to get the widgets properly drawn (MainWindow main
	   paned won't be in its right position and the dialog will be
	   missplaced */

	dialog_response = gtk_dialog_run (GTK_DIALOG (wizard));
	gtk_widget_destroy (GTK_WIDGET (wizard));
	if (gtk_events_pending ())
		gtk_main_iteration ();

	if (dialog_response == GTK_RESPONSE_CANCEL) {
		result = FALSE;
	} else {
		/* Check whether an account was created: */
		result = modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), TRUE);
	}
	return result;
}


void
modest_ui_actions_on_about (GtkAction *action, ModestWindow *win)
{
	GtkWindow *toplevel;
	GtkWidget *about;
	const gchar *authors[] = {
		"Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>",
		NULL
	};
	about = gtk_about_dialog_new ();
	gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG(about), PACKAGE_NAME);
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about),PACKAGE_VERSION);
	gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG(about),
					_("Copyright (c) 2006, Nokia Corporation\n"
					  "All rights reserved."));
	gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG(about),
				       _("a modest e-mail client\n\n"
					 "design and implementation: Dirk-Jan C. Binnema\n"
					 "contributions from the fine people at KC and Ig\n"
					 "uses the tinymail email framework written by Philip van Hoof"));
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(about), authors);
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(about), "http://modest.garage.maemo.org");

	toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
	gtk_window_set_transient_for (GTK_WINDOW (about), toplevel);
	gtk_window_set_modal (GTK_WINDOW (about), TRUE);

	gtk_dialog_run (GTK_DIALOG (about));
	gtk_widget_destroy(about);
}

/*
 * Gets the list of currently selected messages. If the win is the
 * main window, then it returns a newly allocated list of the headers
 * selected in the header view. If win is the msg view window, then
 * the value returned is a list with just a single header.
 *
 * The caller of this funcion must free the list.
 */
static TnyList *
get_selected_headers (ModestWindow *win)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		/* for MsgViewWindows, we simply return a list with one element */
		TnyHeader *header;
		TnyList *list = NULL;

		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));
		if (header != NULL) {
			list = tny_simple_list_new ();
			tny_list_prepend (list, G_OBJECT(header));
			g_object_unref (G_OBJECT(header));
		}

		return list;
	} else if (MODEST_IS_HEADER_WINDOW (win)) {
		GtkWidget *header_view;

		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (win)));
		return modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
	} else {
		return NULL;
	}
}

static void
headers_action_mark_as_read (TnyHeader *header,
			     ModestWindow *win,
			     gpointer user_data)
{
	TnyHeaderFlags flags;
	gchar *uid;

	g_return_if_fail (TNY_IS_HEADER(header));

	flags = tny_header_get_flags (header);
	if (flags & TNY_HEADER_FLAG_SEEN) return;
	tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);
	uid = modest_tny_folder_get_header_unique_id (header);
	modest_platform_emit_msg_read_changed_signal (uid, TRUE);
	g_free (uid);
}

static void
headers_action_mark_as_unread (TnyHeader *header,
			       ModestWindow *win,
			       gpointer user_data)
{
	TnyHeaderFlags flags;

	g_return_if_fail (TNY_IS_HEADER(header));

	flags = tny_header_get_flags (header);
	if (flags & TNY_HEADER_FLAG_SEEN)  {
		gchar *uid;
		uid = modest_tny_folder_get_header_unique_id (header);
		tny_header_unset_flag (header, TNY_HEADER_FLAG_SEEN);
		modest_platform_emit_msg_read_changed_signal (uid, FALSE);
	}
}

/** After deleting a message that is currently visible in a window,
 * show the previoues (newer) message from the list, or close the 
 * window if there are no more messages.
 **/
void
modest_ui_actions_refresh_message_window_after_delete (ModestMsgViewWindow* win)
{
	/* Close msg view window or select previous (newer) */
	if (!modest_msg_view_window_select_previous_message (win) &&
	    !modest_msg_view_window_select_next_message (win)) {
		gboolean ret_value;
		g_signal_emit_by_name (G_OBJECT (win), "delete-event", NULL, &ret_value);
	}
}


void
modest_ui_actions_on_delete_message (GtkAction *action, ModestWindow *win)
{
	modest_ui_actions_on_edit_mode_delete_message (win);
}

gboolean
modest_ui_actions_on_edit_mode_delete_message (ModestWindow *win)
{
	TnyList *header_list = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	gchar *message = NULL;
	gchar *desc = NULL;
	gint response;
	ModestWindowMgr *mgr;
	gboolean retval = TRUE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);

	/* Get the headers, either from the header view (if win is the main window),
	 * or from the message view window: */
	header_list = get_selected_headers (win);
	if (!header_list) return FALSE;

	/* Check if any of the headers are already opened, or in the process of being opened */

	/* Select message */
	if (tny_list_get_length(header_list) == 1) {
		iter = tny_list_create_iterator (header_list);
		header = TNY_HEADER (tny_iterator_get_current (iter));
		if (header) {
			gchar *subject;
			subject = tny_header_dup_subject (header);
			if (!subject)
				subject = g_strdup (_("mail_va_no_subject"));
			desc = g_strdup_printf ("%s", subject);
			g_free (subject);
			g_object_unref (header);
		}

		g_object_unref (iter);
	}
	message = g_strdup_printf(ngettext("emev_nc_delete_message", "emev_nc_delete_messages",
					   tny_list_get_length(header_list)), desc);

	/* Confirmation dialog */
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (win))),
							    message);

	if (response == GTK_RESPONSE_OK) {
		GtkTreeSelection *sel = NULL;
		GList *sel_list = NULL;
		ModestMailOperation *mail_op = NULL;

		/* Find last selected row */

		/* Disable window dimming management */
		modest_window_disable_dimming (win);

		/* Remove each header. If it's a view window header_view == NULL */
		mail_op = modest_mail_operation_new ((GObject *) win);
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
						 mail_op);
		modest_mail_operation_remove_msgs (mail_op, header_list, FALSE);
		g_object_unref (mail_op);

		/* Enable window dimming management */
		if (sel != NULL) {
			gtk_tree_selection_unselect_all (sel);
		}
		modest_window_enable_dimming (win);

		if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
			modest_ui_actions_refresh_message_window_after_delete (MODEST_MSG_VIEW_WINDOW (win));

			/* Get main window */
			mgr = modest_runtime_get_window_mgr ();
		}

		/* Update toolbar dimming state */
		modest_ui_actions_check_menu_dimming_rules (win);
		modest_ui_actions_check_toolbar_dimming_rules (win);

		/* Free */
		g_list_foreach (sel_list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (sel_list);
		retval = TRUE;
	} else {
		retval = FALSE;
	}

	/* Free*/
	g_free(message);
	g_free(desc);
	g_object_unref (header_list);

	return retval;
}




/* delete either message or folder, based on where we are */
void
modest_ui_actions_on_delete_message_or_folder (GtkAction *action, ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_WINDOW(win));

	/* Check first if the header view has the focus */
	modest_ui_actions_on_delete_message (action, win);
}

void
modest_ui_actions_on_quit (GtkAction *action, ModestWindow *win)
{
	ModestWindowMgr *mgr = NULL;

#ifdef MODEST_PLATFORM_MAEMO
	modest_window_mgr_save_state_for_all_windows (modest_runtime_get_window_mgr ());
#endif /* MODEST_PLATFORM_MAEMO */

	g_debug ("closing down, clearing %d item(s) from operation queue",
		 modest_mail_operation_queue_num_elements
		 (modest_runtime_get_mail_operation_queue()));

	/* cancel all outstanding operations */
	modest_mail_operation_queue_cancel_all
		(modest_runtime_get_mail_operation_queue());

	g_debug ("queue has been cleared");


	/* Check if there are opened editing windows */
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_close_all_windows (mgr);

	/* note: when modest-tny-account-store is finalized,
	   it will automatically set all network connections
	   to offline */

/* 	gtk_main_quit (); */
}

void
modest_ui_actions_on_close_window (GtkAction *action, ModestWindow *win)
{
	gboolean ret_value;

	g_signal_emit_by_name (G_OBJECT (win), "delete-event", NULL, &ret_value);

/* 	if (MODEST_IS_MSG_VIEW_WINDOW (win)) { */
/* 		gtk_widget_destroy (GTK_WIDGET (win)); */
/* 	} else if (MODEST_IS_MSG_EDIT_WINDOW (win)) { */
/* 		gboolean ret_value; */
/* 		g_signal_emit_by_name (G_OBJECT (win), "delete-event", NULL, &ret_value); */
/* 	} else if (MODEST_IS_WINDOW (win)) { */
/* 		gtk_widget_destroy (GTK_WIDGET (win)); */
/* 	} else { */
/* 		g_return_if_reached (); */
/* 	} */
}

void
modest_ui_actions_add_to_contacts (GtkAction *action, ModestWindow *win)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (win))
		modest_msg_view_window_add_to_contacts (MODEST_MSG_VIEW_WINDOW (win));
	else if (MODEST_IS_MSG_EDIT_WINDOW (win))
		modest_msg_edit_window_add_to_contacts (MODEST_MSG_EDIT_WINDOW (win));
}

void
modest_ui_actions_on_add_to_contacts (GtkAction *action, ModestWindow *win)
{
	GtkClipboard *clipboard = NULL;
	gchar *selection = NULL;

	clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	selection = gtk_clipboard_wait_for_text (clipboard);

	if (selection) {
		modest_address_book_add_address (selection, (GtkWindow *) win);
		g_free (selection);
	}
}

void
modest_ui_actions_on_new_account (GtkAction *action,
				  ModestWindow *window)
{
	if (!modest_ui_actions_run_account_setup_wizard (window)) {
		g_debug ("%s: wizard was already running", __FUNCTION__);
	}
}

void
modest_ui_actions_on_accounts (GtkAction *action,
			       ModestWindow *win)
{
	/* This is currently only implemented for Maemo */
	if (!modest_account_mgr_has_accounts (modest_runtime_get_account_mgr(), TRUE)) {
		if (!modest_ui_actions_run_account_setup_wizard (win))
			g_debug ("%s: wizard was already running", __FUNCTION__);

		return;
	} else {
		/* Show the list of accounts */
		GtkWindow *win_toplevel, *acc_toplevel;
		GtkWidget *account_win;

		account_win = modest_account_view_window_new ();
		acc_toplevel = (GtkWindow *) gtk_widget_get_toplevel (account_win);

		/* The accounts dialog must be modal */
		win_toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), acc_toplevel, win_toplevel);
		modest_utils_show_dialog_and_forget (win_toplevel, GTK_DIALOG (account_win));
	}
}

void
modest_ui_actions_on_smtp_servers (GtkAction *action, ModestWindow *win)
{
	/* Create the window if necessary: */
	GtkWidget *specific_window = GTK_WIDGET (modest_connection_specific_smtp_window_new ());
	modest_connection_specific_smtp_window_fill_with_connections (
		MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (specific_window),
		modest_runtime_get_account_mgr());

	/* Show the window: */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (specific_window), (GtkWindow *) win);
    	gtk_widget_show (specific_window);
}

static guint64
count_part_size (const gchar *part)
{
	GFile *file = g_file_new_for_uri(part);
	GFileInfo *info;
	guint64 result;

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE ","
				  G_FILE_ATTRIBUTE_STANDARD_SIZE,
				  G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_object_unref(file);

	/* Estimation of attachment size if we cannot get it from file info */
	result = 32768;

	g_return_val_if_fail (info != NULL, result);
	
	if (g_file_info_has_attribute (
		    info, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE) &&
	    g_file_info_has_attribute (
		    info, G_FILE_ATTRIBUTE_STANDARD_SIZE)) {
		result = g_file_info_get_attribute_uint64 (
				 info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
	}

	g_object_unref (info);

	return result;
}

static guint64 
count_parts_size (GSList *parts)
{
	GSList *node;
	guint64 result = 0;

	for (node = parts; node != NULL; node = g_slist_next (node)) {
		result += count_part_size ((const gchar *) node->data);
	}

	return result;
}

void
modest_ui_actions_compose_msg(ModestWindow *win,
			      const gchar *to_str,
			      const gchar *cc_str,
			      const gchar *bcc_str,
			      const gchar *subject_str,
			      const gchar *body_str,
			      GSList *attachments,
			      gboolean set_as_modified)
{
	gchar *account_name = NULL;
	const gchar *mailbox;
	TnyMsg *msg = NULL;
	TnyAccount *account = NULL;
	TnyFolder *folder = NULL;
	gchar *from_str = NULL, *signature = NULL, *body = NULL, *recipient = NULL, *tmp = NULL;
	gboolean use_signature = FALSE;
	ModestWindow *msg_win = NULL;
	ModestAccountMgr *mgr = modest_runtime_get_account_mgr();
	ModestTnyAccountStore *store = modest_runtime_get_account_store();
	guint64 total_size, allowed_size;
	guint64 available_disk, expected_size, parts_size;
	guint parts_count;

	/* we check for low-mem */
	if (modest_platform_check_memory_low (win, TRUE))
		goto cleanup;

	available_disk = modest_utils_get_available_space (NULL);
	parts_count = g_slist_length (attachments);
	parts_size = count_parts_size (attachments);
	expected_size = modest_tny_msg_estimate_size (body, NULL, parts_count, parts_size);

	/* Double check: disk full condition or message too big */
	if (available_disk < MODEST_TNY_ACCOUNT_STORE_MIN_FREE_SPACE ||
	    expected_size > available_disk) {
		gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
		modest_platform_system_banner (NULL, NULL, msg);
		g_free (msg);

		return;
	}

	if (expected_size > MODEST_MAX_ATTACHMENT_SIZE) {
		GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
		modest_platform_run_information_dialog (toplevel,
							_("mail_ib_error_attachment_size"),
							TRUE);
		return;
	}


	if (win)
		account_name = g_strdup (modest_window_get_active_account(win));
	if (!account_name) {
		account_name = modest_account_mgr_get_default_account(mgr);
	}
	if (!account_name) {
		g_printerr ("modest: no account found\n");
		goto cleanup;
	}

	if (win)
		mailbox = modest_window_get_active_mailbox (win);
	else
		mailbox = NULL;
	account = modest_tny_account_store_get_server_account (store, account_name, TNY_ACCOUNT_TYPE_STORE);
	if (!account) {
		g_printerr ("modest: failed to get tnyaccount for '%s'\n", account_name);
		goto cleanup;
	}
	folder = modest_tny_account_get_special_folder (account, TNY_FOLDER_TYPE_DRAFTS);
	if (!folder) {
		g_printerr ("modest: failed to find Drafts folder\n");
		goto cleanup;
	}
	from_str = modest_account_mgr_get_from_string (mgr, account_name, mailbox);
	if (!from_str) {
		g_printerr ("modest: failed get from string for '%s'\n", account_name);
		goto cleanup;
	}


	recipient = modest_text_utils_get_email_address (from_str);
	tmp = modest_account_mgr_get_signature_from_recipient (modest_runtime_get_account_mgr (),
							       recipient,
							       &use_signature);
	signature = modest_text_utils_create_colored_signature (tmp);
	g_free (tmp);
	g_free (recipient);

	body = use_signature ? g_strconcat ((body_str) ? body_str : "", signature, NULL) :
		g_strdup(body_str);

	msg = modest_tny_msg_new_html_plain (to_str, from_str, cc_str, bcc_str, subject_str,
					     NULL, NULL, body, NULL, NULL, NULL, NULL, NULL);
	if (!msg) {
		g_printerr ("modest: failed to create new msg\n");
		goto cleanup;
	}

	/* Create and register edit window */
	/* This is destroyed by TODO. */
	total_size = 0;
	allowed_size = MODEST_MAX_ATTACHMENT_SIZE;
	msg_win = modest_msg_edit_window_new (msg, account_name, mailbox, FALSE);

	if (!modest_window_mgr_register_window (modest_runtime_get_window_mgr(), msg_win, win)) {
		gtk_widget_destroy (GTK_WIDGET (msg_win));
		goto cleanup;
	}
	modest_msg_edit_window_set_modified (MODEST_MSG_EDIT_WINDOW (msg_win), set_as_modified);
	gtk_widget_show_all (GTK_WIDGET (msg_win));

	while (attachments) {
		guint64 att_size;
		att_size =
			modest_msg_edit_window_attach_file_one((ModestMsgEditWindow *)msg_win,
							       attachments->data, allowed_size);
		total_size += att_size;

		if (att_size > allowed_size) {
			g_debug ("%s: total size: %u",
				 __FUNCTION__, (unsigned int)total_size);
			break;
		}
		allowed_size -= att_size;

		attachments = g_slist_next(attachments);
	}

cleanup:
	g_free (from_str);
	g_free (signature);
	g_free (body);
	g_free (account_name);
	if (account)
		g_object_unref (G_OBJECT(account));
	if (folder)
		g_object_unref (G_OBJECT(folder));
	if (msg)
		g_object_unref (G_OBJECT(msg));
}

void
modest_ui_actions_on_new_msg (GtkAction *action, ModestWindow *win)
{
	ModestAccountMgr *account_mgr;

	account_mgr = modest_runtime_get_account_mgr();
	/* if there are no accounts yet, just show the wizard */
	if (!modest_account_mgr_has_accounts (account_mgr, TRUE))
		if (!modest_ui_actions_run_account_setup_wizard (win))
			return;

	/* If clicked from a folder window in tree view mode, set the default account first */
	if (MODEST_IS_FOLDER_WINDOW(win) &&
	    modest_conf_get_bool (modest_runtime_get_conf(), MODEST_CONF_TREE_VIEW, NULL)) {
		gchar *account_name = modest_account_mgr_get_default_account(account_mgr);
		modest_window_set_active_account(MODEST_WINDOW(win), account_name);
	}

	modest_ui_actions_compose_msg(win, NULL, NULL, NULL, NULL, NULL, NULL, FALSE);
}


gboolean
modest_ui_actions_msg_retrieval_check (ModestMailOperation *mail_op,
				       TnyHeader *header,
				       TnyMsg *msg)
{
	ModestMailOperationStatus status;

	/* If there is no message or the operation was not successful */
	status = modest_mail_operation_get_status (mail_op);
	if (!msg || status != MODEST_MAIL_OPERATION_STATUS_SUCCESS) {
		const GError *error;

		/* If it's a memory low issue, then show a banner */
		error = modest_mail_operation_get_error (mail_op);
		if (error && error->domain == MODEST_MAIL_OPERATION_ERROR &&
		    error->code == MODEST_MAIL_OPERATION_ERROR_LOW_MEMORY) {
			GtkWindow *toplevel = NULL;
			GObject *source = modest_mail_operation_get_source (mail_op);

			toplevel = (GtkWindow *) gtk_widget_get_toplevel (GTK_WIDGET (source));
			modest_platform_run_information_dialog (toplevel,
								_KR("memr_ib_operation_disabled"),
								TRUE);
			g_object_unref (source);
		}

		if (error && ((error->code == TNY_SERVICE_ERROR_NO_SUCH_MESSAGE) ||
			      error->code == TNY_SERVICE_ERROR_MESSAGE_NOT_AVAILABLE)) {
			gchar *subject, *msg, *format = NULL;
			TnyAccount *account;

			subject = (header) ? tny_header_dup_subject (header) : NULL;
			if (!subject)
				subject = g_strdup (_("mail_va_no_subject"));

			account = modest_mail_operation_get_account (mail_op);
			if (account) {
				ModestProtocolType proto = modest_tny_account_get_protocol_type (account);
				ModestProtocol *protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (), proto);

				if (protocol) {
					if (tny_account_get_connection_status (account) ==
					    TNY_CONNECTION_STATUS_CONNECTED) {
						if (header) {
							format = modest_protocol_get_translation (protocol,
												  MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE,
												  subject);
						} else {
							format = modest_protocol_get_translation (protocol,
												  MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE_LOST_HEADER);
						}
					} else {
						format = g_strdup_printf (_("mail_ib_backend_server_invalid"),
									  tny_account_get_hostname (account));
					}
				}
				g_object_unref (account);
			}

			if (!format) {
				if (header) {
					format = g_strdup (_("emev_ni_ui_imap_message_not_available_in_server"));
				} else {
					format = g_strdup (_("emev_ni_ui_pop3_msg_recv_error"));
				}
			}

			msg = g_strdup_printf (format, subject);
			modest_platform_run_information_dialog (NULL, msg, FALSE);
			g_free (msg);
			g_free (format);
			g_free (subject);
		}

		/* Remove the header from the preregistered uids */
		modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (),
						     header);

		return FALSE;
	}

	return TRUE;
}

typedef struct {
	guint idle_handler;
	gchar *message;
	GtkWidget *banner;
} OpenMsgBannerInfo;

typedef struct {
	GtkTreeModel *model;
	TnyHeader *header;
	ModestWindow *caller_window;
	OpenMsgBannerInfo *banner_info;
	GtkTreeRowReference *rowref;
} OpenMsgHelper;

gboolean
open_msg_banner_idle (gpointer userdata)
{
	OpenMsgBannerInfo *banner_info = (OpenMsgBannerInfo *) userdata;

	gdk_threads_enter ();
	banner_info->idle_handler = 0;
	banner_info->banner = modest_platform_animation_banner (NULL, NULL, banner_info->message);
	if (banner_info->banner)
		g_object_ref (banner_info->banner);

	gdk_threads_leave ();

	return FALSE;
}

static GtkWidget *
get_header_view_from_window (ModestWindow *window)
{
	GtkWidget *header_view;

	if (MODEST_IS_HEADER_WINDOW (window)){
		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (window)));
	} else {
		header_view = NULL;
	}

	return header_view;
}

static gchar *
get_info_from_header (TnyHeader *header, gboolean *is_draft, gboolean *can_open)
{
	TnyFolder *folder;
	gchar *account = NULL;
	TnyFolderType folder_type = TNY_FOLDER_TYPE_UNKNOWN;

	*is_draft = FALSE;
	*can_open = TRUE;

	folder = tny_header_get_folder (header);
	/* Gets folder type (OUTBOX headers will be opened in edit window */
	if (modest_tny_folder_is_local_folder (folder)) {
		folder_type = modest_tny_folder_get_local_or_mmc_folder_type (folder);
		if (folder_type == TNY_FOLDER_TYPE_INVALID)
			g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
	}

	if (folder_type == TNY_FOLDER_TYPE_OUTBOX) {
		TnyTransportAccount *traccount = NULL;
		ModestTnyAccountStore *accstore = modest_runtime_get_account_store();
		traccount = modest_tny_account_store_get_transport_account_from_outbox_header(accstore, header);
		if (traccount) {
			ModestTnySendQueue *send_queue = NULL;
			ModestTnySendQueueStatus status;
			gchar *msg_id;
			account = g_strdup(modest_tny_account_get_parent_modest_account_name_for_server_account(
						   TNY_ACCOUNT(traccount)));
			send_queue = modest_runtime_get_send_queue(traccount, TRUE);
			if (TNY_IS_SEND_QUEUE (send_queue)) {
				msg_id = modest_tny_send_queue_get_msg_id (header);
				status = modest_tny_send_queue_get_msg_status(send_queue, msg_id);
				g_free (msg_id);
				/* Only open messages in outbox with the editor if they are in Failed state */
				if (status == MODEST_TNY_SEND_QUEUE_FAILED) {
					*is_draft = TRUE;
				}
				else {
					/* In Fremantle we can not
					   open any message from
					   outbox which is not in
					   failed state */
					*can_open = FALSE;
                                }
			}
			g_object_unref(traccount);
		} else {
			g_warning("Cannot get transport account for message in outbox!!");
		}
	} else if (folder_type == TNY_FOLDER_TYPE_DRAFTS) {
		*is_draft = TRUE; /* Open in editor if the message is in the Drafts folder */
	}

	if (!account) {
		TnyAccount *acc = tny_folder_get_account (folder);
		if (acc) {
			account =
				g_strdup (modest_tny_account_get_parent_modest_account_name_for_server_account (acc));
			g_object_unref (acc);
		}
	}

	g_object_unref (folder);

	return account;
}

static void
open_msg_cb (ModestMailOperation *mail_op,
	     TnyHeader *header,
	     gboolean canceled,
	     TnyMsg *msg,
	     GError *err,
	     gpointer user_data)
{
	ModestWindowMgr *mgr = NULL;
	ModestWindow *parent_win = NULL;
	ModestWindow *win = NULL;
	gchar *account = NULL;
	gboolean open_in_editor = FALSE;
	gboolean can_open;
	OpenMsgHelper *helper = (OpenMsgHelper *) user_data;

	/* Do nothing if there was any problem with the mail
	   operation. The error will be shown by the error_handler of
	   the mail operation */
	if (!modest_ui_actions_msg_retrieval_check (mail_op, header, msg))
		return;

	parent_win = (ModestWindow *) modest_mail_operation_get_source (mail_op);

	/* Mark header as read */
	headers_action_mark_as_read (header, MODEST_WINDOW(parent_win), NULL);

	account = get_info_from_header (header, &open_in_editor, &can_open);

	/* Get account */
	if (!account)
		account = g_strdup (modest_window_get_active_account (MODEST_WINDOW (parent_win)));
	if (!account)
		account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());

	if (open_in_editor) {
		ModestAccountMgr *mgr = modest_runtime_get_account_mgr ();
		gchar *from_header = NULL, *acc_name;
		gchar *mailbox = NULL;

		from_header = tny_header_dup_from (header);

		/* we cannot edit without a valid account... */
		if (!modest_account_mgr_has_accounts(mgr, TRUE)) {
			if (!modest_ui_actions_run_account_setup_wizard(parent_win)) {
				modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (),
								     header);
				g_free (from_header);
				goto cleanup;
			}
		}

		acc_name = modest_utils_get_account_name_from_recipient (from_header, &mailbox);
		g_free (from_header);
		if (acc_name) {
			g_free (account);
			account = acc_name;
		}

		win = modest_msg_edit_window_new (msg, account, mailbox, TRUE);
		if (mailbox)
			g_free (mailbox);
	} else {
		gchar *uid = modest_tny_folder_get_header_unique_id (header);
		const gchar *mailbox = NULL;

		if (parent_win && MODEST_IS_WINDOW (parent_win))
			mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (parent_win));

		if (helper->rowref && helper->model) {
			win = modest_msg_view_window_new_with_header_model (msg, account, mailbox, (const gchar*) uid,
									    helper->model, helper->rowref);
		} else {
			win = modest_msg_view_window_new_for_attachment (msg, NULL, account, mailbox, (const gchar*) uid);
		}
		g_free (uid);
	}

	/* Register and show new window */
	if (win != NULL) {
		mgr = modest_runtime_get_window_mgr ();
		if (!modest_window_mgr_register_window (mgr, win, NULL)) {
			gtk_widget_destroy (GTK_WIDGET (win));
			goto cleanup;
		}
		gtk_widget_show_all (GTK_WIDGET(win));
	}


cleanup:
	/* Free */
	g_free(account);
	g_object_unref (parent_win);
}

void
modest_ui_actions_disk_operations_error_handler (ModestMailOperation *mail_op,
						 gpointer user_data)
{
	const GError *error;
	GObject *win = NULL;
	ModestMailOperationStatus status;

	win = modest_mail_operation_get_source (mail_op);
	error = modest_mail_operation_get_error (mail_op);
	status = modest_mail_operation_get_status (mail_op);

	/* If the mail op has been cancelled then it's not an error:
	   don't show any message */
	if (status != MODEST_MAIL_OPERATION_STATUS_CANCELED) {
		TnyAccount *account = modest_mail_operation_get_account (mail_op);
		if (modest_tny_account_store_is_disk_full_error (modest_runtime_get_account_store(),
								 (GError *) error, account)) {
			gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
			modest_platform_information_banner ((GtkWidget *) win, NULL, msg);
			g_free (msg);
		} else if (error->code == TNY_SYSTEM_ERROR_MEMORY) {
			modest_platform_information_banner ((GtkWidget *) win,
							    NULL, _("emev_ui_imap_inbox_select_error"));
		} else if (error->domain == MODEST_MAIL_OPERATION_ERROR &&
			   error->code == MODEST_MAIL_OPERATION_ERROR_FILE_IO) {
			modest_platform_information_banner ((GtkWidget *) win,
							    NULL, _CS_UNABLE_TO_OPEN_FILE_NOT_FOUND);
		} else if (user_data) {
			modest_platform_information_banner ((GtkWidget *) win,
							    NULL, user_data);
		}
		if (account)
			g_object_unref (account);
	}

	if (win)
		g_object_unref (win);
}

/**
 * Returns the account a list of headers belongs to. It returns a
 * *new* reference so don't forget to unref it
 */
static TnyAccount*
get_account_from_header_list (TnyList *headers)
{
	TnyAccount *account = NULL;

	if (tny_list_get_length (headers) > 0) {
		TnyIterator *iter = tny_list_create_iterator (headers);
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
		TnyFolder *folder = tny_header_get_folder (header);

		if (!folder) {
			g_object_unref (header);

			while (!tny_iterator_is_done (iter)) {
				header = TNY_HEADER (tny_iterator_get_current (iter));
				folder = tny_header_get_folder (header);
				if (folder)
					break;
				g_object_unref (header);
				header = NULL;
				tny_iterator_next (iter);
			}
		}

		if (folder) {
			account = tny_folder_get_account (folder);
			g_object_unref (folder);
		}

		if (header)
			g_object_unref (header);

		g_object_unref (iter);
	}
	return account;
}

static TnyAccount*
get_account_from_header (TnyHeader *header)
{
	TnyAccount *account = NULL;
	TnyFolder *folder;

	folder = tny_header_get_folder (header);

	if (folder) {
		account = tny_folder_get_account (folder);
		g_object_unref (folder);
	}
	return account;
}

static void
caller_win_destroyed (OpenMsgHelper *helper, GObject *object)
{
	if (helper->caller_window)
		helper->caller_window = NULL;
}

static void
open_msg_helper_destroyer (gpointer user_data)
{
	OpenMsgHelper *helper = (OpenMsgHelper *) user_data;

	if (helper->caller_window) {
		g_object_weak_unref ((GObject *) helper->caller_window, (GWeakNotify) caller_win_destroyed, helper);
		helper->caller_window = NULL;
	}

	if (helper->banner_info) {
		g_free (helper->banner_info->message);
		if (helper->banner_info->idle_handler > 0) {
			g_source_remove (helper->banner_info->idle_handler);
			helper->banner_info->idle_handler = 0;
		}
		if (helper->banner_info->banner != NULL) {
			gtk_widget_destroy (helper->banner_info->banner);
			g_object_unref (helper->banner_info->banner);
			helper->banner_info->banner = NULL;
		}
		g_slice_free (OpenMsgBannerInfo, helper->banner_info);
		helper->banner_info = NULL;
	}
	g_object_unref (helper->model);
	g_object_unref (helper->header);
	gtk_tree_row_reference_free (helper->rowref);
	g_slice_free (OpenMsgHelper, helper);
}

static void
open_msg_performer(gboolean canceled,
		    GError *err,
		    ModestWindow *parent_window,
		    TnyAccount *account,
		    gpointer user_data)
{
	ModestMailOperation *mail_op = NULL;
	gchar *error_msg = NULL;
	ModestProtocolType proto;
	TnyConnectionStatus status;
	OpenMsgHelper *helper = NULL;
	ModestProtocol *protocol;
	ModestProtocolRegistry *protocol_registry;
	gchar *subject;

	helper = (OpenMsgHelper *) user_data;

	status = tny_account_get_connection_status (account);
	if (err || canceled || helper->caller_window == NULL) {
		modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (), helper->header);
		/* Free the helper */
		open_msg_helper_destroyer (helper);

		/* In disk full conditions we could get this error here */
		modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								(GtkWidget *) parent_window, err,
								account, NULL);

		goto clean;
	}

	/* Get the error message depending on the protocol */
	proto = modest_tny_account_get_protocol_type (account);
	if (proto == MODEST_PROTOCOL_REGISTRY_TYPE_INVALID) {
		proto = MODEST_PROTOCOLS_STORE_MAILDIR;
	}

	protocol_registry = modest_runtime_get_protocol_registry ();
	subject = tny_header_dup_subject (helper->header);

	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry, proto);
	error_msg = modest_protocol_get_translation (protocol, MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE, subject);
	if (subject)
		g_free (subject);

	if (error_msg == NULL) {
		error_msg = g_strdup (_("mail_ni_ui_folder_get_msg_folder_error"));
	}

	gboolean is_draft;
	gboolean can_open;
	gchar *account_name = get_info_from_header (helper->header, &is_draft, &can_open);

	if (!g_strcmp0 (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) ||
	    !g_strcmp0 (account_name, MODEST_MMC_ACCOUNT_ID)) {
		g_free (account_name);
		account_name = g_strdup (modest_window_get_active_account (MODEST_WINDOW (parent_window)));
	}

	if (!can_open) {
		modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (), helper->header);
		g_free (account_name);
		open_msg_helper_destroyer (helper);
		goto clean;
	}

	if (!is_draft) {
		ModestWindow *window;
		GtkWidget *header_view;
		gchar *uid;

		header_view = get_header_view_from_window (parent_window);
		uid = modest_tny_folder_get_header_unique_id (helper->header);
		if (header_view) {
			const gchar *mailbox = NULL;
			mailbox = modest_window_get_active_mailbox (parent_window);
			window = modest_msg_view_window_new_from_header_view 
				(MODEST_HEADER_VIEW (header_view), account_name, mailbox, uid, helper->rowref);
			if (window != NULL) {
				if (!modest_window_mgr_register_window (modest_runtime_get_window_mgr (),
									window, NULL)) {
					gtk_widget_destroy (GTK_WIDGET (window));
				} else {
					gtk_widget_show_all (GTK_WIDGET(window));
				}
			}
		}
		g_free (account_name);
		g_free (uid);
		open_msg_helper_destroyer (helper);
		goto clean;
	}
	g_free (account_name);
	/* Create the mail operation */
	mail_op =
		modest_mail_operation_new_with_error_handling ((GObject *) parent_window,
							       modest_ui_actions_disk_operations_error_handler,
							       g_strdup (error_msg), g_free);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);



	TnyList *headers;
	headers = TNY_LIST (tny_simple_list_new ());
	tny_list_prepend (headers, G_OBJECT (helper->header));
	modest_mail_operation_get_msgs_full (mail_op,
					     headers,
					     open_msg_cb,
					     helper,
					     open_msg_helper_destroyer);
	g_object_unref (headers);

	/* Frees */
 clean:
	if (error_msg)
		g_free (error_msg);
	if (mail_op)
		g_object_unref (mail_op);
	g_object_unref (account);
}

/*
 * This function is used by both modest_ui_actions_on_open and
 * modest_ui_actions_on_header_activated. This way we always do the
 * same when trying to open messages.
 */
static void
open_msg_from_header (TnyHeader *header, GtkTreeRowReference *rowref, ModestWindow *win)
{
	ModestWindowMgr *mgr = NULL;
	TnyAccount *account;
	gboolean cached = FALSE;
	gboolean found;
	GtkWidget *header_view = NULL;
	OpenMsgHelper *helper;
	ModestWindow *window;

	g_return_if_fail (header != NULL && rowref != NULL && gtk_tree_row_reference_valid (rowref));

	mgr = modest_runtime_get_window_mgr ();

        /* get model */
	header_view = get_header_view_from_window (MODEST_WINDOW (win));
	if (header_view == NULL)
		return;

	/* Get the account */
	account = get_account_from_header (header);
	if (!account)
		return;

	window = NULL;
	found = modest_window_mgr_find_registered_header (mgr, header, &window);

	/* Do not open again the message and present the
	   window to the user */
	if (found) {
		if (window) {
#ifndef MODEST_TOOLKIT_HILDON2
			GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) window);
			gtk_window_present (toplevel);
#endif
		} else {
			/* the header has been registered already, we don't do
			 * anything but wait for the window to come up*/
			g_debug ("header %p already registered, waiting for window", header);
		}
		goto cleanup;
	}

	/* Open each message */
	cached = tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED;
	if (!cached) {
		/* Allways download if we are online. */
		if (!tny_device_is_online (modest_runtime_get_device ())) {
			gint response;
			GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);

			/* If ask for user permission to download the messages */
			response = modest_platform_run_confirmation_dialog (toplevel,
									    _("mcen_nc_get_msg"));

			/* End if the user does not want to continue */
			if (response == GTK_RESPONSE_CANCEL) {
				goto cleanup;
			}
		}
	}

	/* We register the window for opening */
	modest_window_mgr_register_header (mgr, header, NULL);

	/* Create the helper. We need to get a reference to the model
	   here because it could change while the message is readed
	   (the user could switch between folders) */
	helper = g_slice_new (OpenMsgHelper);
	helper->model = g_object_ref (gtk_tree_view_get_model (GTK_TREE_VIEW (header_view)));
	helper->caller_window = win;
	g_object_weak_ref ((GObject *) helper->caller_window, (GWeakNotify) caller_win_destroyed, helper);
	helper->header = g_object_ref (header);
	helper->rowref = gtk_tree_row_reference_copy (rowref);
	helper->banner_info = NULL;

	/* Connect to the account and perform */
	if (!cached) {
		modest_platform_connect_and_perform (win, TRUE, g_object_ref (account),
						     open_msg_performer, helper);
	} else {
		/* Call directly the performer, do not need to connect */
		open_msg_performer (FALSE, NULL, win,
				    g_object_ref (account), helper);
	}
cleanup:
	/* Clean */
	if (account)
		g_object_unref (account);
}

void
modest_ui_actions_on_open (GtkAction *action, ModestWindow *win)
{
	TnyList *headers;
	TnyHeader *header;
	gint headers_count;
	TnyIterator *iter;

	/* we check for low-mem; in that case, show a warning, and don't allow
	 * opening
	 */
	if (modest_platform_check_memory_low (MODEST_WINDOW(win), TRUE))
		return;

	/* Get headers */
	headers = get_selected_headers (win);
	if (!headers)
		return;

	headers_count = tny_list_get_length (headers);
	if (headers_count != 1) {
		if (headers_count > 1) {
			/* Don't allow activation if there are more than one message selected */
			modest_platform_information_banner (NULL, NULL, _("mcen_ib_select_one_message"));
		}

		g_object_unref (headers);
		return;
	}

	iter = tny_list_create_iterator (headers);
	header = TNY_HEADER (tny_iterator_get_current (iter));
	g_object_unref (iter);

	/* Open them */
	if (header) {
		open_msg_from_header (header, NULL, win);
		g_object_unref (header);
	}

	g_object_unref(headers);
}

static void
rf_helper_window_closed (gpointer data,
			 GObject *object)
{
	ReplyForwardHelper *helper = (ReplyForwardHelper *) data;

	helper->parent_window = NULL;
}

static ReplyForwardHelper*
create_reply_forward_helper (ReplyForwardAction action,
			     ModestWindow *win,
			     guint reply_forward_type,
			     TnyHeader *header,
			     TnyMsg *msg_part,
			     TnyHeader *top_header,
			     TnyList *parts)
{
	ReplyForwardHelper *rf_helper = NULL;
	const gchar *active_acc = modest_window_get_active_account (win);
	const gchar *active_mailbox = modest_window_get_active_mailbox (win);

	rf_helper = g_slice_new0 (ReplyForwardHelper);
	rf_helper->reply_forward_type = reply_forward_type;
	rf_helper->action = action;
	rf_helper->parent_window = (MODEST_IS_WINDOW (win)) ? GTK_WIDGET (win) : NULL;
	rf_helper->header = (header) ? g_object_ref (header) : NULL;
	rf_helper->top_header = (top_header) ? g_object_ref (top_header) : NULL;
	rf_helper->msg_part = (msg_part) ? g_object_ref (msg_part) : NULL;
	rf_helper->account_name = (active_acc) ?
		g_strdup (active_acc) :
		modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());
	rf_helper->mailbox = g_strdup (active_mailbox);
	if (parts)
		rf_helper->parts = g_object_ref (parts);
	else
		rf_helper->parts = NULL;

	/* Note that window could be destroyed just AFTER calling
	   register_window so we must ensure that this pointer does
	   not hold invalid references */
	if (rf_helper->parent_window)
		g_object_weak_ref (G_OBJECT (rf_helper->parent_window),
				   rf_helper_window_closed, rf_helper);

	return rf_helper;
}

static void
free_reply_forward_helper (gpointer data)
{
	ReplyForwardHelper *helper;

	helper = (ReplyForwardHelper *) data;
	g_free (helper->account_name);
	g_free (helper->mailbox);
	if (helper->header)
		g_object_unref (helper->header);
	if (helper->top_header)
		g_object_unref (helper->top_header);
	if (helper->msg_part)
		g_object_unref (helper->msg_part);
	if (helper->parts)
		g_object_unref (helper->parts);
	if (helper->parent_window)
		g_object_weak_unref (G_OBJECT (helper->parent_window),
				     rf_helper_window_closed, helper);
	g_slice_free (ReplyForwardHelper, helper);
}

static void
reply_forward_cb (ModestMailOperation *mail_op,
		  TnyHeader *header,
		  gboolean canceled,
		  TnyMsg *msg,
		  GError *err,
		  gpointer user_data)
{
	TnyMsg *new_msg = NULL;
	ReplyForwardHelper *rf_helper;
	ModestWindow *msg_win = NULL;
	ModestEditType edit_type;
	gchar *from = NULL;
	TnyAccount *account = NULL;
	ModestWindowMgr *mgr = NULL;
	gchar *signature = NULL, *recipient = NULL;
	gboolean use_signature;

	/* If there was any error. The mail operation could be NULL,
	   this means that we already have the message downloaded and
	   that we didn't do a mail operation to retrieve it */
	rf_helper = (ReplyForwardHelper *) user_data;
	if (mail_op && !modest_ui_actions_msg_retrieval_check (mail_op, header, msg))
		goto cleanup;

	from = modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
						   rf_helper->account_name, rf_helper->mailbox);

	recipient = modest_text_utils_get_email_address (from);
	signature = modest_account_mgr_get_signature_from_recipient (modest_runtime_get_account_mgr (),
								     recipient,
								     &use_signature);
	g_free (recipient);

	/* Create reply mail */
	switch (rf_helper->action) {
		/* Use the msg_header to ensure that we have all the
		   information. The summary can lack some data */
		TnyHeader *msg_header;
	case ACTION_REPLY:
		msg_header = tny_msg_get_header (rf_helper->msg_part?rf_helper->msg_part:msg);
		new_msg =
			modest_tny_msg_create_reply_msg (rf_helper->msg_part?rf_helper->msg_part:msg, msg_header, from,
							 (use_signature) ? signature : NULL,
							 rf_helper->reply_forward_type,
							 MODEST_TNY_MSG_REPLY_MODE_SENDER);
		g_object_unref (msg_header);
		break;
	case ACTION_REPLY_TO_ALL:
		msg_header = tny_msg_get_header (rf_helper->msg_part?rf_helper->msg_part:msg);
		new_msg =
			modest_tny_msg_create_reply_msg (rf_helper->msg_part?rf_helper->msg_part:msg, msg_header, from,
							 (use_signature) ? signature : NULL,
							 rf_helper->reply_forward_type,
							 MODEST_TNY_MSG_REPLY_MODE_ALL);
		edit_type = MODEST_EDIT_TYPE_REPLY;
		g_object_unref (msg_header);
		break;
	case ACTION_FORWARD:
		new_msg =
			modest_tny_msg_create_forward_msg (rf_helper->msg_part?rf_helper->msg_part:msg, from, 
							   (use_signature) ? signature : NULL,
							   rf_helper->reply_forward_type);
		edit_type = MODEST_EDIT_TYPE_FORWARD;
		break;
	default:
		modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (),
						     header);
		g_return_if_reached ();
		return;
	}

	g_free (from);
	g_free (signature);

	if (!new_msg) {
		g_warning ("%s: failed to create message\n", __FUNCTION__);
		goto cleanup;
	}

	account = modest_tny_account_store_get_server_account (modest_runtime_get_account_store(),
								       rf_helper->account_name,
								       TNY_ACCOUNT_TYPE_STORE);
	if (!account) {
		g_warning ("%s: failed to get tnyaccount for '%s'\n", __FUNCTION__, rf_helper->account_name);
		goto cleanup;
	}

	/* Create and register the windows */
	msg_win = modest_msg_edit_window_new (new_msg, rf_helper->account_name, rf_helper->mailbox, FALSE);
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_register_window (mgr, msg_win, (ModestWindow *) rf_helper->parent_window);

	/* Note that register_window could have deleted the account */
	if (MODEST_IS_WINDOW (rf_helper->parent_window)) {
		gdouble parent_zoom;

		parent_zoom = modest_window_get_zoom (MODEST_WINDOW (rf_helper->parent_window));
		modest_window_set_zoom (msg_win, parent_zoom);
	}

	/* Show edit window */
	gtk_widget_show_all (GTK_WIDGET (msg_win));

cleanup:
	/* We always unregister the header because the message is
	   forwarded or replied so the original one is no longer
	   opened */
	modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (),
					     header);
	if (new_msg)
		g_object_unref (G_OBJECT (new_msg));
	if (account)
		g_object_unref (G_OBJECT (account));
	free_reply_forward_helper (rf_helper);
}

/* Checks a list of headers. If any of them are not currently
 * downloaded (CACHED) then returns TRUE else returns FALSE.
 */
static gint
header_list_count_uncached_msgs (TnyList *header_list)
{
	TnyIterator *iter;
	gint uncached_messages = 0;

	iter = tny_list_create_iterator (header_list);
	while (!tny_iterator_is_done (iter)) {
		TnyHeader *header;

		header = TNY_HEADER (tny_iterator_get_current (iter));
		if (header) {
			if (!(tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED))
				uncached_messages ++;
			g_object_unref (header);
		}

		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	return uncached_messages;
}

/* Returns FALSE if the user does not want to download the
 * messages. Returns TRUE if the user allowed the download.
 */
static gboolean
connect_to_get_msg (ModestWindow *win,
		    gint num_of_uncached_msgs,
		    TnyAccount *account)
{
	GtkResponseType response;
	GtkWindow *toplevel;

	/* Allways download if we are online. */
	if (tny_device_is_online (modest_runtime_get_device ()))
		return TRUE;

	/* If offline, then ask for user permission to download the messages */
	toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
	response = modest_platform_run_confirmation_dialog (toplevel,
							    ngettext("mcen_nc_get_msg",
								     "mcen_nc_get_msgs",
								     num_of_uncached_msgs));

	if (response == GTK_RESPONSE_CANCEL)
		return FALSE;

	return modest_platform_connect_and_wait(toplevel, account);
}

static void
reply_forward_performer (gboolean canceled,
			 GError *err,
			 ModestWindow *parent_window,
			 TnyAccount *account,
			 gpointer user_data)
{
	ReplyForwardHelper *rf_helper = NULL;
	ModestMailOperation *mail_op;

	rf_helper = (ReplyForwardHelper *) user_data;

	if (canceled || err) {
		free_reply_forward_helper (rf_helper);
		return;
	}

	/* Retrieve the message */
	modest_window_mgr_register_header (modest_runtime_get_window_mgr (), rf_helper->header, NULL);
	mail_op = modest_mail_operation_new_with_error_handling (G_OBJECT (parent_window),
								 modest_ui_actions_disk_operations_error_handler,
								 NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	modest_mail_operation_get_msg_and_parts (mail_op, rf_helper->top_header, rf_helper->parts, TRUE, reply_forward_cb, rf_helper);

	/* Frees */
	g_object_unref(mail_op);
}

static gboolean
all_parts_retrieved (TnyMimePart *part)
{
	if (!TNY_IS_CAMEL_BS_MIME_PART (part)) {
		return TRUE;
	} else {
		TnyList *pending_parts;
		TnyIterator *iterator;
		gboolean all_retrieved = TRUE;

		pending_parts = TNY_LIST (tny_simple_list_new ());
		tny_mime_part_get_parts (part, pending_parts);
		iterator = tny_list_create_iterator (pending_parts);
		while (all_retrieved && !tny_iterator_is_done (iterator)) {
			TnyMimePart *child;

			child = TNY_MIME_PART (tny_iterator_get_current (iterator));

			if (tny_camel_bs_mime_part_is_fetched (TNY_CAMEL_BS_MIME_PART (child))) {
				all_retrieved = all_parts_retrieved (TNY_MIME_PART (child));
			} else {
				all_retrieved = FALSE;
			}

			g_object_unref (child);
			tny_iterator_next (iterator);
		}
		g_object_unref (iterator);
		g_object_unref (pending_parts);
		return all_retrieved;
	}
}

static void
forward_pending_parts_helper (TnyMimePart *part, TnyList *list)
{
	TnyList *parts;
	TnyIterator *iterator;

	if (!tny_camel_bs_mime_part_is_fetched (TNY_CAMEL_BS_MIME_PART (part))) {
		tny_list_append (list, G_OBJECT (part));
	}
	parts = TNY_LIST (tny_simple_list_new ());
	tny_mime_part_get_parts (part, parts);
	for (iterator = tny_list_create_iterator (parts); 
	     !tny_iterator_is_done (iterator);
	     tny_iterator_next (iterator)) {
		TnyMimePart *child;

		child = TNY_MIME_PART (tny_iterator_get_current (iterator));
		forward_pending_parts_helper (child, list);
		g_object_unref (child);
	}
	g_object_unref (iterator);
	g_object_unref (parts);
}

static TnyList *
forward_pending_parts (TnyMsg *msg)
{
	TnyList *result = TNY_LIST (tny_simple_list_new ());
	if (TNY_IS_CAMEL_BS_MIME_PART (msg)) {
		forward_pending_parts_helper (TNY_MIME_PART (msg), result);
	}

	return result;
}

/*
 * Common code for the reply and forward actions
 */
static void
reply_forward (ReplyForwardAction action, ModestWindow *win)
{
	ReplyForwardHelper *rf_helper = NULL;
	guint reply_forward_type;

	g_return_if_fail (win && MODEST_IS_WINDOW(win));

	/* we check for low-mem; in that case, show a warning, and don't allow
	 * reply/forward (because it could potentially require a lot of memory */
	if (modest_platform_check_memory_low (MODEST_WINDOW(win), TRUE))
		return;


	/* we need an account when editing */
	if (!modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), TRUE)) {
		if (!modest_ui_actions_run_account_setup_wizard (win))
			return;
	}

	reply_forward_type =
		modest_conf_get_int (modest_runtime_get_conf (),
				     (action == ACTION_FORWARD) ?
				     MODEST_CONF_FORWARD_TYPE :
				     MODEST_CONF_REPLY_TYPE,
				     NULL);

	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		TnyMsg *msg = NULL;
		TnyMsg *top_msg = NULL;
		TnyHeader *header = NULL;
		/* Get header and message. Do not free them here, the
		   reply_forward_cb must do it */
		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW(win));
		top_msg = modest_msg_view_window_get_top_message (MODEST_MSG_VIEW_WINDOW(win));
		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));

		if (msg && header && (action != ACTION_FORWARD || all_parts_retrieved (TNY_MIME_PART (msg)))) {
			/* Create helper */
			rf_helper = create_reply_forward_helper (action, win,
								 reply_forward_type, header, NULL, NULL, NULL);
			reply_forward_cb (NULL, header, FALSE, msg, NULL, rf_helper);
		} else {
			gboolean do_download = TRUE;

			if (msg && header && action == ACTION_FORWARD) {
				if (top_msg == NULL)
					top_msg = g_object_ref (msg);
				/* Not all parts retrieved. Then we have to retrieve them all before
				 * creating the forward message */
				if (!tny_device_is_online (modest_runtime_get_device ())) {
					gint response;
					GtkWindow *toplevel;

					/* If ask for user permission to download the messages */
					toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
					response = modest_platform_run_confirmation_dialog (toplevel,
											    ngettext("mcen_nc_get_msg",
												     "mcen_nc_get_msgs",
												     1));

					/* End if the user does not want to continue */
					if (response == GTK_RESPONSE_CANCEL)
						do_download = FALSE;
				}

				if (do_download) {
					TnyList *pending_parts;
					TnyFolder *folder;
					TnyAccount *account;
					TnyHeader *top_header;

					/* Create helper */
					top_header = tny_msg_get_header (top_msg);
					pending_parts = forward_pending_parts (top_msg);
					rf_helper = create_reply_forward_helper (action, win,
										 reply_forward_type, header, msg, top_header, pending_parts);
					g_object_unref (pending_parts);

					folder = tny_header_get_folder (top_header);
					account = tny_folder_get_account (folder);
					modest_platform_connect_and_perform (win,
									     TRUE, account,
									     reply_forward_performer,
									     rf_helper);
					if (folder) g_object_unref (folder);
					g_object_unref (account);
					if (top_header) g_object_unref (top_header);
				}

			} else {
				g_warning("%s: no message or header found in viewer\n", __FUNCTION__);
			}
		}

		if (msg)
			g_object_unref (msg);
		if (top_msg)
			g_object_unref (top_msg);
 		if (header)
			g_object_unref (header);
	} else {
		TnyHeader *header = NULL;
		TnyIterator *iter;
		gboolean do_retrieve = TRUE;
		TnyList *header_list = NULL;

		header_list = get_selected_headers (win);
		if (!header_list)
			return;
		/* Check that only one message is selected for replying */
		if (tny_list_get_length (header_list) != 1) {
			modest_platform_information_banner ((win) ? GTK_WIDGET (win) : NULL,
							    NULL, _("mcen_ib_select_one_message"));
			g_object_unref (header_list);
			return;
		}

		/* Only reply/forward to one message */
		iter = tny_list_create_iterator (header_list);
		header = TNY_HEADER (tny_iterator_get_current (iter));
		g_object_unref (iter);

		/* Retrieve messages */
		do_retrieve = (action == ACTION_FORWARD) ||
			(reply_forward_type != MODEST_TNY_MSG_REPLY_TYPE_CITE);

		if (do_retrieve) {
			TnyAccount *account = NULL;
			TnyFolder *folder = NULL;
			gdouble download = TRUE;
			guint uncached_msgs = 0;

			folder = tny_header_get_folder (header);
			if (!folder)
				goto do_retrieve_frees;
			account = tny_folder_get_account (folder);
			if (!account)
				goto do_retrieve_frees;

			uncached_msgs = header_list_count_uncached_msgs (header_list);

			if (uncached_msgs > 0) {
				/* Allways download if we are online. */
				if (!tny_device_is_online (modest_runtime_get_device ())) {
					gint response;
					GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);

					/* If ask for user permission to download the messages */
					response = modest_platform_run_confirmation_dialog (toplevel,
											    ngettext("mcen_nc_get_msg",
												     "mcen_nc_get_msgs",
												     uncached_msgs));

					/* End if the user does not want to continue */
					if (response == GTK_RESPONSE_CANCEL)
						download = FALSE;
				}
			}

			if (download) {
				/* Create helper */
				rf_helper = create_reply_forward_helper (action, win,
									 reply_forward_type, header, NULL, NULL, NULL);
				if (uncached_msgs > 0) {
					modest_platform_connect_and_perform (win,
									     TRUE, account,
									     reply_forward_performer,
									     rf_helper);
				} else {
					reply_forward_performer (FALSE, NULL, win,
								 account, rf_helper);
				}
			}
		do_retrieve_frees:
			if (account)
				g_object_unref (account);
			if (folder)
				g_object_unref (folder);
		} else {
			reply_forward_cb (NULL, header, FALSE, NULL, NULL, NULL);
		}
		/* Frees */
		g_object_unref (header_list);
		g_object_unref (header);
	}
}

void
modest_ui_actions_reply_calendar (ModestWindow *win, TnyList *header_pairs)
{
	modest_ui_actions_reply_calendar_with_subject (win, NULL, header_pairs);
}

void
modest_ui_actions_reply_calendar_with_subject (ModestWindow *win, const gchar *custom_subject, TnyList *header_pairs)
{
	gchar *from;
	gchar *recipient;
	gchar *signature;
	gboolean use_signature;
	TnyMsg *new_msg;
	GtkWidget *msg_win;
	const gchar *account_name;
	const gchar *mailbox;
	TnyHeader *msg_header;
	ModestWindowMgr *mgr;
	TnyMsg *msg;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW(win));

	/* we check for low-mem; in that case, show a warning, and don't allow
	 * reply/forward (because it could potentially require a lot of memory */
	if (modest_platform_check_memory_low (MODEST_WINDOW(win), TRUE))
		return;

	account_name = modest_window_get_active_account (MODEST_WINDOW (win));
	mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (win));
	from = modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
						   account_name, mailbox);
	recipient = modest_text_utils_get_email_address (from);
	signature = modest_account_mgr_get_signature_from_recipient (modest_runtime_get_account_mgr(), 
								     recipient, 
								     &use_signature);
	g_free (recipient);

	msg = modest_msg_view_window_get_message(MODEST_MSG_VIEW_WINDOW(win));
	g_return_if_fail(msg);

	msg_header = tny_msg_get_header (msg);
	new_msg =
		modest_tny_msg_create_reply_calendar_msg (msg, msg_header, from,
							  (use_signature) ? signature : NULL,
							  header_pairs);
	g_object_unref (msg_header);

	g_free (from);
	g_free (signature);

	if (!new_msg) {
		g_warning ("%s: failed to create message\n", __FUNCTION__);
		goto cleanup;
	}

	if (custom_subject) {
		TnyHeader *new_msg_header;

		new_msg_header = tny_msg_get_header (new_msg);
		tny_header_set_subject (new_msg_header, custom_subject);
		g_object_unref (new_msg_header);
	}

	msg_win = (GtkWidget *) modest_msg_edit_window_new (new_msg, account_name, mailbox, FALSE);
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_register_window (mgr, MODEST_WINDOW (msg_win), (ModestWindow *) win);

	/* Show edit window */
	gtk_widget_show_all (GTK_WIDGET (msg_win));

cleanup:
	if (new_msg)
		g_object_unref (G_OBJECT (new_msg));
}

void
modest_ui_actions_on_reply (GtkAction *action, ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_WINDOW(win));

	reply_forward (ACTION_REPLY, win);
}

void
modest_ui_actions_on_forward (GtkAction *action, ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_WINDOW(win));

	reply_forward (ACTION_FORWARD, win);
}

void
modest_ui_actions_on_reply_all (GtkAction *action, ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_WINDOW(win));

	reply_forward (ACTION_REPLY_TO_ALL, win);
}

void
modest_ui_actions_on_next (GtkAction *action,
			   ModestWindow *window)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		modest_msg_view_window_select_next_message (
				MODEST_MSG_VIEW_WINDOW (window));
	} else {
		g_return_if_reached ();
	}
}

void
modest_ui_actions_on_prev (GtkAction *action,
			   ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW(window));

	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		modest_msg_view_window_select_previous_message (MODEST_MSG_VIEW_WINDOW (window));
	} else {
		g_return_if_reached ();
	}
}

void
modest_ui_actions_on_sort (GtkAction *action,
			   ModestWindow *window)
{
	GtkWidget *header_view = NULL;

	g_return_if_fail (MODEST_IS_WINDOW(window));

	if (MODEST_IS_HEADER_WINDOW (window)) {
		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (window)));
	}

	if (!header_view) {
		modest_platform_information_banner (NULL, NULL, _CS_NOTHING_TO_SORT);

		return;
	}

	/* Show sorting dialog */
	modest_utils_run_sort_dialog (MODEST_WINDOW (window), MODEST_SORT_HEADERS);
}

static void
sync_folder_cb (ModestMailOperation *mail_op,
		TnyFolder *folder,
		gpointer user_data)
{
	ModestHeaderView *header_view = (ModestHeaderView *) user_data;

	if (modest_mail_operation_get_status (mail_op) == MODEST_MAIL_OPERATION_STATUS_SUCCESS) {
		ModestWindow *parent = (ModestWindow *) modest_mail_operation_get_source (mail_op);

		/* We must clear first, because otherwise set_folder will ignore */
		/*    the change as the folders are the same */
		modest_header_view_clear (header_view);
		modest_header_view_set_folder (header_view, folder, TRUE, parent, NULL, NULL);

		g_object_unref (parent);
	}

	g_object_unref (header_view);
}

static gboolean
idle_refresh_folder (gpointer source)
{
	ModestHeaderView *header_view = NULL;

	/* If the window still exists */
	if (!GTK_IS_WIDGET (source) ||
	    !GTK_WIDGET_VISIBLE (source))
		return FALSE;

	/* Refresh the current view */
	if (MODEST_IS_HEADER_WINDOW (source))
		header_view = modest_header_window_get_header_view ((ModestHeaderWindow *) source);
	if (header_view) {
		TnyFolder *folder = modest_header_view_get_folder (header_view);
		if (folder) {
			/* Sync the folder status */
			ModestMailOperation *mail_op = modest_mail_operation_new (source);
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
			modest_mail_operation_sync_folder (mail_op, folder, FALSE, sync_folder_cb, g_object_ref (header_view));
			g_object_unref (folder);
			g_object_unref (mail_op);
		}
	}

	return FALSE;
}

static void
update_account_cb (ModestMailOperation *self,
		   TnyList *new_headers,
		   gpointer user_data)
{
	ModestWindow *top;
	gboolean show_visual_notifications;

	top = modest_window_mgr_get_current_top (modest_runtime_get_window_mgr ());
	show_visual_notifications = (top) ? FALSE : TRUE;

	/* Notify new messages have been downloaded. If the
	   send&receive was invoked by the user then do not show any
	   visual notification, only play a sound and activate the LED
	   (for the Maemo version) */
	if (TNY_IS_LIST(new_headers) && (tny_list_get_length (new_headers)) > 0) {

		/* We only notify about really new messages (not seen) we get */
		TnyList *actually_new_list;
		TnyIterator *iterator;
		actually_new_list = TNY_LIST (tny_simple_list_new ());
		for (iterator = tny_list_create_iterator (new_headers);
		     !tny_iterator_is_done (iterator);
		     tny_iterator_next (iterator)) {
			TnyHeader *header;
			TnyHeaderFlags flags;
			header = TNY_HEADER (tny_iterator_get_current (iterator));
			flags = tny_header_get_flags (header);

			if (!(flags & TNY_HEADER_FLAG_SEEN)) {
				/* Messages are ordered from most
				   recent to oldest. But we want to
				   show notifications starting from
				   the oldest message. That's why we
				   reverse the list */
				tny_list_prepend (actually_new_list, G_OBJECT (header));
			}
			g_object_unref (header);
		}
		g_object_unref (iterator);

		if (tny_list_get_length (actually_new_list) > 0) {
			GList *new_headers_list = NULL;

			new_headers_list = modest_utils_create_notification_list_from_header_list (actually_new_list);

			/* Send notifications */
			if (new_headers_list) {
				modest_platform_on_new_headers_received (new_headers_list,
									 show_visual_notifications);
				/* Free the list */
				modest_utils_free_notification_list (new_headers_list);
			}
		}
		g_object_unref (actually_new_list);
	}

	if (top) {
		/* Refresh the current folder in an idle. We do this
		   in order to avoid refresh cancelations if the
		   currently viewed folder is the inbox */
		g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
				 idle_refresh_folder,
				 g_object_ref (top),
				 g_object_unref);
	}
}

typedef struct {
	TnyAccount *account;
	ModestWindow *win;
	gchar *account_name;
	gboolean poke_status;
	gboolean interactive;
	ModestMailOperation *mail_op;
	ModestMailOperation *disconnect_op;
	GtkWindow *parent_window;
	gboolean force_connection;
} SendReceiveInfo;

static void
do_send_receive_performer (gboolean canceled,
			   GError *err,
			   ModestWindow *parent_window,
			   TnyAccount *account,
			   gpointer user_data)
{
	SendReceiveInfo *info;

	info = (SendReceiveInfo *) user_data;

	if (err || canceled) {
		/* In disk full conditions we could get this error here */
		modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								(GtkWidget *) parent_window, err,
								account, NULL);

		if (info->mail_op) {
			modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (),
							    info->mail_op);
		}
		goto clean;
	}


	/* Send & receive. */
	modest_mail_operation_update_account (info->mail_op, info->account_name,
					      info->poke_status, info->interactive,
					      update_account_cb, info->win);

 clean:
	/* Frees */
	if (info->mail_op)
		g_object_unref (G_OBJECT (info->mail_op));
	if (info->account_name)
		g_free (info->account_name);
	if (info->win)
		g_object_unref (info->win);
	if (info->account)
		g_object_unref (info->account);
	g_slice_free (SendReceiveInfo, info);
}

/*
 * This function performs the send & receive required actions. The
 * window is used to create the mail operation. Typically it should
 * always be the main window, but we pass it as argument in order to
 * be more flexible.
 */
void
modest_ui_actions_do_send_receive (const gchar *account_name,
				   gboolean force_connection,
				   gboolean poke_status,
				   gboolean interactive,
				   ModestWindow *win)
{
	gchar *acc_name = NULL;
	SendReceiveInfo *info;
	ModestProtocolType account_type;
	ModestTnyAccountStore *acc_store;
	TnyAccount *account;

	/* If no account name was provided then get the current account, and if
	   there is no current account then pick the default one: */
	if (!account_name) {
		if (win)
			acc_name = g_strdup (modest_window_get_active_account (win));
		if (!acc_name)
			acc_name  = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());
		if (!acc_name) {
			modest_platform_information_banner (NULL, NULL, _("emev_ni_internal_error"));
			return;
		}
	} else {
		acc_name = g_strdup (account_name);
	}

	acc_store = modest_runtime_get_account_store ();
	account = modest_tny_account_store_get_server_account (acc_store, acc_name, TNY_ACCOUNT_TYPE_STORE);

	if (!account) {
		g_free (acc_name);
		modest_platform_information_banner (NULL, NULL, _("emev_ni_internal_error"));
		return;
	}

	/* Do not automatically refresh accounts that are flagged as
	   NO_AUTO_UPDATE. This could be useful for accounts that
	   handle their own update times */
	if (!interactive) {
		ModestProtocolType proto = modest_tny_account_get_protocol_type (account);
		if (proto != MODEST_PROTOCOL_REGISTRY_TYPE_INVALID) {
			const gchar *tag = MODEST_PROTOCOL_REGISTRY_NO_AUTO_UPDATE_PROTOCOLS;
			ModestProtocolRegistry *registry = modest_runtime_get_protocol_registry ();

			if (modest_protocol_registry_protocol_type_has_tag (registry, proto, tag)) {
				g_debug ("%s no auto update allowed for account %s", __FUNCTION__, account_name);
				g_object_unref (account);
				g_free (acc_name);
				return;
			}
		}
	}

	/* Create the info for the connect and perform */
	info = g_slice_new (SendReceiveInfo);
	info->account_name = acc_name;
	info->win = (win) ? g_object_ref (win) : NULL;
	info->poke_status = poke_status;
	info->interactive = interactive;
	info->account = account;
	info->parent_window = (win ? GTK_WINDOW (win) : NULL);
	info->force_connection = force_connection;
	/* We need to create the operation here, because otherwise it
	   could happen that the queue emits the queue-empty signal
	   while we're trying to connect the account */
	info->mail_op = modest_mail_operation_new_with_error_handling ((info->win) ? G_OBJECT (info->win) : NULL,
								       modest_ui_actions_disk_operations_error_handler,
								       NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), info->mail_op);

	/* for POP3 account we should go offline before we can sync account */
	account_type = modest_tny_account_get_protocol_type (account);
	if (MODEST_PROTOCOLS_STORE_POP == account_type) {
		info->disconnect_op =
			modest_mail_operation_new ((info->win) ? G_OBJECT (info->win) : NULL);
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
			info->disconnect_op);
		g_signal_connect (G_OBJECT (info->disconnect_op), "operation-finished",
			G_CALLBACK (modest_ui_actions_send_receive_offline), info);
		modest_mail_operation_disconnect_account (info->disconnect_op, account);
	}
	else {
		/* Invoke the connect and perform */
		modest_platform_connect_and_perform (info->parent_window,
		force_connection, info->account, do_send_receive_performer, info);
		info->disconnect_op = NULL;
	}
}

static void
modest_ui_actions_send_receive_offline (ModestMailOperation *mail_op, gpointer user_data)
{
	SendReceiveInfo *info;

	info = (SendReceiveInfo *)user_data;

	/* release the 'disconnect' mail operation */
	modest_mail_operation_queue_remove (
		modest_runtime_get_mail_operation_queue (), info->disconnect_op);
	g_object_unref (info->disconnect_op);
	info->disconnect_op = NULL;

	/* Invoke the connect and perform */
	modest_platform_connect_and_perform (
		info->parent_window ? info->parent_window : NULL,
		info->force_connection, info->account, do_send_receive_performer, info);
}

static void
modest_ui_actions_do_cancel_send (const gchar *account_name,
				  ModestWindow *win)
{
	TnyTransportAccount *transport_account;
	TnySendQueue *send_queue = NULL;
	GError *error = NULL;

	/* Get transport account */
	transport_account =
		TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_server_account
				      (modest_runtime_get_account_store(),
				       account_name,
				       TNY_ACCOUNT_TYPE_TRANSPORT));
	if (!transport_account) {
		g_printerr ("modest: no transport account found for '%s'\n", account_name);
		goto frees;
	}

	/* Get send queue*/
	send_queue = TNY_SEND_QUEUE (modest_runtime_get_send_queue (transport_account, TRUE));
	if (!TNY_IS_SEND_QUEUE(send_queue)) {
		g_set_error (&error, MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
			     "modest: could not find send queue for account\n");
	} else {
		/* Cancel the current send */
		tny_account_cancel (TNY_ACCOUNT (transport_account));

		/* Suspend all pending messages */
		tny_send_queue_cancel (send_queue, TNY_SEND_QUEUE_CANCEL_ACTION_SUSPEND, &error);
	}

 frees:
	if (transport_account != NULL)
		g_object_unref (G_OBJECT (transport_account));
}

static void
modest_ui_actions_cancel_send_all (ModestWindow *win)
{
	GSList *account_names, *iter;

	account_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr(),
							  TRUE);

	iter = account_names;
	while (iter) {
		modest_ui_actions_do_cancel_send ((const char*) iter->data, win);
		iter = g_slist_next (iter);
	}

	modest_account_mgr_free_account_names (account_names);
	account_names = NULL;
}

void
modest_ui_actions_cancel_send (GtkAction *action,  ModestWindow *win)

{
	/* Check if accounts exist */
	gboolean accounts_exist =
		modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), TRUE);

	/* If not, allow the user to create an account before trying to send/receive. */
	if (!accounts_exist)
		modest_ui_actions_on_accounts (NULL, win);

	/* Cancel all sending operaitons */
	modest_ui_actions_cancel_send_all (win);
}

/*
 * Refreshes all accounts. This function will be used by automatic
 * updates
 */
void
modest_ui_actions_do_send_receive_all (ModestWindow *win,
				       gboolean force_connection,
				       gboolean poke_status,
				       gboolean interactive)
{
	GSList *account_names, *iter;

	account_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr(),
							  TRUE);

	iter = account_names;
	while (iter) {
		modest_ui_actions_do_send_receive ((const char*) iter->data,
						   force_connection,
						   poke_status, interactive, win);
		iter = g_slist_next (iter);
	}

	modest_account_mgr_free_account_names (account_names);
	account_names = NULL;
}

/*
 * Handler of the click on Send&Receive button in the main toolbar
 */
void
modest_ui_actions_on_send_receive (GtkAction *action, ModestWindow *win)
{
	/* Check if accounts exist */
	gboolean accounts_exist;

	accounts_exist =
		modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), TRUE);

	/* If not, allow the user to create an account before trying to send/receive. */
	if (!accounts_exist)
		modest_ui_actions_on_accounts (NULL, win);

	/* Refresh the current folder. The if is always TRUE it's just an extra check */
	if (MODEST_IS_ACCOUNTS_WINDOW (win) || 
	    (MODEST_IS_FOLDER_WINDOW (win) && 
		modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_TREE_VIEW, NULL))) {
		modest_ui_actions_do_send_receive_all (win, TRUE, TRUE, TRUE);
	} else {
		const gchar *active_account;
		active_account = modest_window_get_active_account (MODEST_WINDOW (win));

		modest_ui_actions_do_send_receive (active_account, TRUE, TRUE, TRUE, win);
	}

}


void
modest_ui_actions_on_header_activated (ModestHeaderView *header_view,
				       TnyHeader *header,
				       GtkTreePath *path,
				       ModestWindow *window)
{
	GtkTreeRowReference *rowref;

	g_return_if_fail (MODEST_IS_WINDOW(window));
	g_return_if_fail (MODEST_IS_HEADER_VIEW (header_view));
	g_return_if_fail (TNY_IS_HEADER (header));

	if (modest_header_view_count_selected_headers (header_view) > 1) {
		/* Don't allow activation if there are more than one message selected */
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_select_one_message"));
		return;
	}

	/* we check for low-mem; in that case, show a warning, and don't allow
	 * activating headers
	 */
	if (modest_platform_check_memory_low (MODEST_WINDOW(window), TRUE))
		return;


	rowref = gtk_tree_row_reference_new (gtk_tree_view_get_model (GTK_TREE_VIEW (header_view)), path);
	open_msg_from_header (header, rowref, MODEST_WINDOW (window));
	gtk_tree_row_reference_free (rowref);
}

void
modest_ui_actions_on_item_not_found (ModestHeaderView *header_view,ModestItemType type,
				     ModestWindow *win)
{
	GtkWidget *dialog;
	gchar *txt, *item;
	gboolean online;
	GtkWindow *toplevel;

	toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
	item = (type == MODEST_ITEM_TYPE_FOLDER) ? "folder" : "message";

	online = tny_device_is_online (modest_runtime_get_device());

	if (online) {
		/* already online -- the item is simply not there... */
		dialog = gtk_message_dialog_new (toplevel,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_NONE,
						 _("The %s you selected cannot be found"),
						 item);
		gtk_dialog_add_button (GTK_DIALOG (dialog),_("mcen_bd_dialog_ok"), GTK_RESPONSE_ACCEPT);
		gtk_dialog_run (GTK_DIALOG(dialog));
	} else {
		dialog = gtk_dialog_new_with_buttons (_("Connection requested"),
						      toplevel,
						      GTK_DIALOG_MODAL,
						      _("mcen_bd_dialog_cancel"),
						      GTK_RESPONSE_REJECT,
						      _("mcen_bd_dialog_ok"),
						      GTK_RESPONSE_ACCEPT,
						      NULL);
		txt = g_strdup_printf (_("This %s is not available in offline mode.\n"
					 "Do you want to get online?"), item);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox),
				    gtk_label_new (txt), FALSE, FALSE, 0);
		gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
		g_free (txt);

		gtk_window_set_default_size ((GtkWindow *) dialog, 300, 300);
		if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			/* TODO: Comment about why is this commented out: */
			/* modest_platform_connect_and_wait (); */
		}
	}
	gtk_widget_destroy (dialog);
}

void
modest_ui_actions_on_msg_link_hover (ModestMsgView *msgview, const gchar* link,
				     ModestWindow *win)
{
	/* g_debug ("%s %s", __FUNCTION__, link); */
}


void
modest_ui_actions_on_msg_link_clicked (ModestMsgView *msgview, const gchar* link,
					ModestWindow *win)
{
	modest_platform_activate_uri (link);
}

void
modest_ui_actions_on_msg_link_contextual (ModestMsgView *msgview, const gchar* link,
					  ModestWindow *win)
{
	modest_platform_show_uri_popup (link);
}

void
modest_ui_actions_on_msg_attachment_clicked (ModestMsgView *msgview, TnyMimePart *mime_part,
					     ModestWindow *win)
{
	/* we check for low-mem; in that case, show a warning, and don't allow
	 * viewing attachments
	 */
	if (modest_platform_check_memory_low (MODEST_WINDOW(win), TRUE))
		return;

	modest_msg_view_window_view_attachment (MODEST_MSG_VIEW_WINDOW (win), mime_part);
}

void
modest_ui_actions_on_msg_recpt_activated (ModestMsgView *msgview,
					  const gchar *address,
					  ModestWindow *win)
{
	/* g_debug ("%s %s", __FUNCTION__, address); */
}

static void
on_save_to_drafts_cb (ModestMailOperation *mail_op,
		      TnyMsg *saved_draft,
		      gpointer user_data)
{
	ModestMsgEditWindow *edit_window;

	/* TODO: in hildon 2 we have to dim and undim the header views while we're saving */

	edit_window = MODEST_MSG_EDIT_WINDOW (user_data);

	/* Set draft is there was no error */
	if (!modest_mail_operation_get_error (mail_op))
		modest_msg_edit_window_set_draft (edit_window, saved_draft);

	g_object_unref(edit_window);
}

static gboolean
enough_space_for_message (ModestMsgEditWindow *edit_window,
			  MsgData *data)
{
	guint64 available_disk, expected_size;
	gint parts_count;
	guint64 parts_size;

	/* Check size */
	available_disk = modest_utils_get_available_space (NULL);
	modest_msg_edit_window_get_parts_size (edit_window, &parts_count, &parts_size);
	expected_size = modest_tny_msg_estimate_size (data->plain_body,
						      data->html_body,
						      parts_count,
						      parts_size);

	/* Double check: disk full condition or message too big */
	if (available_disk < MODEST_TNY_ACCOUNT_STORE_MIN_FREE_SPACE ||
	    expected_size > available_disk) {
		gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
		modest_platform_information_banner (NULL, NULL, msg);
		g_free (msg);

		return FALSE;
	}

	/*
	 * djcb: if we're in low-memory state, we only allow for
	 * saving messages smaller than
	 * MODEST_MAX_LOW_MEMORY_MESSAGE_SIZE (see modest-defs.h) this
	 * should still allow for sending anything critical...
	 */
	if ((expected_size > MODEST_MAX_LOW_MEMORY_MESSAGE_SIZE) &&
	    modest_platform_check_memory_low (MODEST_WINDOW(edit_window), TRUE))
		return FALSE;

	/*
	 * djcb: we also make sure that the attachments are smaller than the max size
	 * this is for the case where we'd try to forward a message with attachments
	 * bigger than our max allowed size, or sending an message from drafts which
	 * somehow got past our checks when attaching.
	 */
	if (expected_size > MODEST_MAX_ATTACHMENT_SIZE) {
		GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) edit_window);
		modest_platform_run_information_dialog (toplevel,
							_("mail_ib_error_attachment_size"),
							TRUE);
		return FALSE;
	}

	return TRUE;
}

gboolean
modest_ui_actions_on_save_to_drafts (GtkWidget *widget, ModestMsgEditWindow *edit_window)
{
	TnyTransportAccount *transport_account;
	ModestMailOperation *mail_operation;
	MsgData *data;
	gchar *account_name;
	ModestAccountMgr *account_mgr;
	gboolean had_error = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW(edit_window), FALSE);

	data = modest_msg_edit_window_get_msg_data (edit_window);

	/* Check size */
	if (!enough_space_for_message (edit_window, data)) {
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return FALSE;
	}

	account_name = g_strdup (data->account_name);
	account_mgr = modest_runtime_get_account_mgr();
	if (!account_name)
		account_name = g_strdup(modest_window_get_active_account (MODEST_WINDOW(edit_window)));
	if (!account_name)
		account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return FALSE;
	}

	if (!strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID)) {
		account_name = g_strdup (data->account_name);
	}

	transport_account =
		TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_server_account
				      (modest_runtime_get_account_store (),
				       account_name,
				       TNY_ACCOUNT_TYPE_TRANSPORT));
	if (!transport_account) {
		g_printerr ("modest: no transport account found for '%s'\n", account_name);
		g_free (account_name);
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return FALSE;
	}

	/* Create the mail operation */
	mail_operation = modest_mail_operation_new_with_error_handling (NULL, modest_ui_actions_disk_operations_error_handler,
									NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);

	modest_mail_operation_save_to_drafts (mail_operation,
					      transport_account,
					      data->draft_msg,
					      data->from,
					      data->to, 
					      data->cc, 
					      data->bcc,
					      data->subject,
					      data->plain_body,
					      data->html_body,
					      data->attachments,
					      data->images,
					      data->priority_flags,
					      data->references,
					      data->in_reply_to,
					      on_save_to_drafts_cb,
					      g_object_ref(edit_window));

	/* In hildon2 we always show the information banner on saving to drafts.
	 * It will be a system information banner in this case.
	 */
	gchar *text = g_strdup_printf (_("mail_va_saved_to_drafts"), _("mcen_me_folder_drafts"));
	modest_platform_information_banner (NULL, NULL, text);
	g_free (text);
	modest_msg_edit_window_set_modified (edit_window, FALSE);

	/* Frees */
	g_free (account_name);
	g_object_unref (G_OBJECT (transport_account));
	g_object_unref (G_OBJECT (mail_operation));

	modest_msg_edit_window_free_msg_data (edit_window, data);


	return !had_error;
}

/* For instance, when clicking the Send toolbar button when editing a message: */
gboolean
modest_ui_actions_on_send (GtkWidget *widget, ModestMsgEditWindow *edit_window)
{
	TnyTransportAccount *transport_account = NULL;
	gboolean result = TRUE, add_to_contacts;
	MsgData *data;
	ModestAccountMgr *account_mgr;
	gchar *account_name;
	gchar *recipients;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW(edit_window), TRUE);

	/* Check whether to automatically add new contacts to addressbook or not */
	add_to_contacts = modest_conf_get_bool (modest_runtime_get_conf (),
						MODEST_CONF_AUTO_ADD_TO_CONTACTS, NULL);
	/* first validate the names, this resolves the original email addresses */
	if (!modest_msg_edit_window_check_names (edit_window, FALSE)) {
		return TRUE;
	}
	if (add_to_contacts) {
		/* now, add the resolved addresses to the address book */
		if (!modest_msg_edit_window_check_names (edit_window, TRUE)) {
			return TRUE;
		}
	}

	data = modest_msg_edit_window_get_msg_data (edit_window);

	recipients = g_strconcat (data->to?data->to:"", 
				  data->cc?data->cc:"",
				  data->bcc?data->bcc:"",
				  NULL);
	if (recipients == NULL || recipients[0] == '\0') {
		/* Empty subject -> no send */
		g_free (recipients);
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return FALSE;
	}
	g_free (recipients);

	/* Check size */
	if (!enough_space_for_message (edit_window, data)) {
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return FALSE;
	}

	account_mgr = modest_runtime_get_account_mgr();
	account_name = g_strdup (data->account_name);
	if (!account_name)
		account_name = g_strdup(modest_window_get_active_account (MODEST_WINDOW(edit_window)));

	if (!account_name)
		account_name = modest_account_mgr_get_default_account (account_mgr);

	if (!account_name) {
		modest_msg_edit_window_free_msg_data (edit_window, data);
		/* Run account setup wizard */
		if (!modest_ui_actions_run_account_setup_wizard (MODEST_WINDOW(edit_window))) {
			return TRUE;
		}
	}

	/* Get the currently-active transport account for this modest account: */
	if (account_name && strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) != 0) {
		transport_account =
			TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_server_account
					      (modest_runtime_get_account_store (),
					       account_name, TNY_ACCOUNT_TYPE_TRANSPORT));
	}

	if (!transport_account) {
		modest_msg_edit_window_free_msg_data (edit_window, data);
		/* Run account setup wizard */
		if (!modest_ui_actions_run_account_setup_wizard(MODEST_WINDOW(edit_window)))
			return TRUE;
	}

	result = modest_ui_actions_send_msg_with_transport (transport_account,
							    data->draft_msg,
							    data->from,
							    data->to,
							    data->cc,
							    data->bcc,
							    data->subject,
							    data->plain_body,
							    data->html_body,
							    data->attachments,
							    data->images,
							    data->references,
							    data->in_reply_to,
							    data->priority_flags);


	/* Free data: */
	g_free (account_name);
	g_object_unref (G_OBJECT (transport_account));

	modest_msg_edit_window_free_msg_data (edit_window, data);

	if (result) {
		modest_msg_edit_window_set_sent (edit_window, TRUE);

		/* Save settings and close the window: */
		modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (edit_window));
	}

	return result;
}

/* For instance, when clicking the Send toolbar button when editing a message: */
gboolean
modest_ui_actions_on_send_custom_msg (const gchar *account_name, 
				      const gchar *from, const gchar *to, const gchar *cc, const gchar *bcc,
				      const gchar *subject,
				      const gchar *plain_body, const gchar *html_body,
				      const GList *attachments_list, const GList *images_list,
				      const gchar *references, const gchar *in_reply_to,
				      TnyHeaderFlags priority_flags)
{
	TnyTransportAccount *transport_account = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (account_name, FALSE);

	transport_account =
	  TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_server_account
				(modest_runtime_get_account_store (),
				 account_name, TNY_ACCOUNT_TYPE_TRANSPORT));

	g_return_val_if_fail (transport_account, FALSE);

	result = modest_ui_actions_send_msg_with_transport (transport_account,
							    NULL /*draft msg*/,
							    from, to, cc, bcc,
							    subject,
							    plain_body, html_body,
							    attachments_list, images_list,
							    references, in_reply_to,
							    priority_flags);

	/* Free data: */
	g_object_unref (G_OBJECT (transport_account));

	return result;
}

gboolean
modest_ui_actions_send_msg_with_transport (TnyTransportAccount *transport_account, 
					   TnyMsg *draft_msg,
					   const gchar *from, const gchar *to, const gchar *cc, const gchar *bcc,
					   const gchar *subject,
					   const gchar *plain_body, const gchar *html_body,
					   const GList *attachments_list, const GList *images_list,
					   const gchar *references, const gchar *in_reply_to,
					   TnyHeaderFlags priority_flags)
{
	gboolean had_error = FALSE;
	ModestMailOperation *mail_operation;

	g_return_val_if_fail (transport_account, FALSE);

	/* Create the mail operation */
	mail_operation = modest_mail_operation_new_with_error_handling (NULL, modest_ui_actions_disk_operations_error_handler, NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);

	modest_mail_operation_send_new_mail (mail_operation,
					     transport_account,
					     draft_msg,
					     from,
					     to,
					     cc,
					     bcc,
					     subject,
					     plain_body,
					     html_body,
					     attachments_list,
					     images_list,
					     references,
					     in_reply_to,
					     priority_flags);

	if (modest_mail_operation_get_status (mail_operation) == MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS)
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_outbox_waiting_to_be_sent"));

	if (modest_mail_operation_get_error (mail_operation) != NULL) {
		const GError *error = modest_mail_operation_get_error (mail_operation);
		if (error->domain == MODEST_MAIL_OPERATION_ERROR &&
		    error->code == MODEST_MAIL_OPERATION_ERROR_INSTANCE_CREATION_FAILED) {
			g_warning ("%s failed: %s\n", __FUNCTION__, (modest_mail_operation_get_error (mail_operation))->message);
			modest_platform_information_banner (NULL, NULL, _CS_NOT_ENOUGH_MEMORY);
			had_error = TRUE;
		}
	}

	/* Free data: */
	g_object_unref (G_OBJECT (mail_operation));

	return !had_error;
}

gboolean
modest_ui_actions_on_send_msg (ModestWindow *window,
			       TnyMsg *msg)
{
	TnyTransportAccount *transport_account = NULL;
	gboolean had_error = FALSE;
	ModestAccountMgr *account_mgr;
	gchar *account_name;
	ModestMailOperation *mail_operation;

	account_mgr = modest_runtime_get_account_mgr();
	account_name = g_strdup(modest_window_get_active_account (MODEST_WINDOW(window)));

	if (!account_name)
		account_name = modest_account_mgr_get_default_account (account_mgr);

	/* Get the currently-active transport account for this modest account: */
	if (account_name && strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) != 0) {
		transport_account =
			TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_server_account
					      (modest_runtime_get_account_store (),
					       account_name, TNY_ACCOUNT_TYPE_TRANSPORT));
	}

	/* Create the mail operation */
	mail_operation = modest_mail_operation_new_with_error_handling (NULL, modest_ui_actions_disk_operations_error_handler, NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);

	modest_mail_operation_send_mail (mail_operation,
					 transport_account,
					 msg);

	if (modest_mail_operation_get_status (mail_operation) == MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS)
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_outbox_waiting_to_be_sent"));

	if (modest_mail_operation_get_error (mail_operation) != NULL) {
		const GError *error = modest_mail_operation_get_error (mail_operation);
		if (error->domain == MODEST_MAIL_OPERATION_ERROR &&
		    error->code == MODEST_MAIL_OPERATION_ERROR_INSTANCE_CREATION_FAILED) {
			g_warning ("%s failed: %s\n", __FUNCTION__, (modest_mail_operation_get_error (mail_operation))->message);
			modest_platform_information_banner (NULL, NULL, _CS("sfil_ni_not_enough_memory"));
			had_error = TRUE;
		}
	}

	/* Free data: */
	g_free (account_name);
	g_object_unref (G_OBJECT (transport_account));
	g_object_unref (G_OBJECT (mail_operation));

	return !had_error;
}

void
modest_ui_actions_on_toggle_bold (GtkToggleAction *action,
				  ModestMsgEditWindow *window)
{
	ModestMsgEditFormatState *format_state = NULL;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_TOGGLE_ACTION (action));

	if (modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW (window)) == MODEST_MSG_EDIT_FORMAT_TEXT)
		return;

	format_state = modest_msg_edit_window_get_format_state (window);
	g_return_if_fail (format_state != NULL);

	format_state->bold = gtk_toggle_action_get_active (action);
	modest_msg_edit_window_set_format_state (window, format_state);
	g_free (format_state);

}

void
modest_ui_actions_on_toggle_italics (GtkToggleAction *action,
				     ModestMsgEditWindow *window)
{
	ModestMsgEditFormatState *format_state = NULL;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_TOGGLE_ACTION (action));

	if (modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW(window)) == MODEST_MSG_EDIT_FORMAT_TEXT)
		return;

	format_state = modest_msg_edit_window_get_format_state (window);
	g_return_if_fail (format_state != NULL);

	format_state->italics = gtk_toggle_action_get_active (action);
	modest_msg_edit_window_set_format_state (window, format_state);
	g_free (format_state);

}

void
modest_ui_actions_on_toggle_bullets (GtkToggleAction *action,
				     ModestMsgEditWindow *window)
{
	ModestMsgEditFormatState *format_state = NULL;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_TOGGLE_ACTION (action));

	if (modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW (window)) == MODEST_MSG_EDIT_FORMAT_TEXT)
		return;

	format_state = modest_msg_edit_window_get_format_state (window);
	g_return_if_fail (format_state != NULL);

	format_state->bullet = gtk_toggle_action_get_active (action);
	modest_msg_edit_window_set_format_state (window, format_state);
	g_free (format_state);

}

void
modest_ui_actions_on_change_justify (GtkRadioAction *action,
				     GtkRadioAction *selected,
				     ModestMsgEditWindow *window)
{
	ModestMsgEditFormatState *format_state = NULL;
	GtkJustification value;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	if (modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW(window)) == MODEST_MSG_EDIT_FORMAT_TEXT)
		return;

	value = gtk_radio_action_get_current_value (selected);

	format_state = modest_msg_edit_window_get_format_state (window);
	g_return_if_fail (format_state != NULL);

	format_state->justification = value;
	modest_msg_edit_window_set_format_state (window, format_state);
	g_free (format_state);
}

void
modest_ui_actions_on_select_editor_color (GtkAction *action,
					  ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_ACTION (action));

	if (modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW(window)) == MODEST_MSG_EDIT_FORMAT_TEXT)
		return;

	modest_msg_edit_window_select_color (window);
}

void
modest_ui_actions_on_select_editor_background_color (GtkAction *action,
						     ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_ACTION (action));

	if (modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW(window)) == MODEST_MSG_EDIT_FORMAT_TEXT)
		return;

}

void
modest_ui_actions_on_insert_image (GObject *object,
				   ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));


	if (modest_platform_check_memory_low (MODEST_WINDOW(window), TRUE))
		return;

	if (modest_msg_edit_window_get_format (MODEST_MSG_EDIT_WINDOW(window)) == MODEST_MSG_EDIT_FORMAT_TEXT)
		return;

	modest_msg_edit_window_insert_image (window);
}

void
modest_ui_actions_on_attach_file (GtkAction *action,
				  ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_ACTION (action));

	if (modest_platform_check_memory_low (MODEST_WINDOW(window), TRUE))
		return;

	modest_msg_edit_window_offer_attach_file (window);
}

void
modest_ui_actions_on_remove_attachments (GtkAction *action,
					 ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	modest_msg_edit_window_remove_attachments (window, NULL);
}

static void
do_create_folder_cb (ModestMailOperation *mail_op,
		     TnyFolderStore *parent_folder,
		     TnyFolder *new_folder,
		     gpointer user_data)
{
	gchar *suggested_name = (gchar *) user_data;
	GtkWindow *source_win = (GtkWindow *) modest_mail_operation_get_source (mail_op);
	const GError *error;

	error = modest_mail_operation_get_error (mail_op);
	if (error) {
		gboolean disk_full = FALSE;
		TnyAccount *account;
		/* Show an error. If there was some problem writing to
		   disk, show it, otherwise show the generic folder
		   create error. We do it here and not in an error
		   handler because the call to do_create_folder will
		   stop the main loop in a gtk_dialog_run and then,
		   the message won't be shown until that dialog is
		   closed */
		account = modest_mail_operation_get_account (mail_op);
		if (account) {
			disk_full =
				modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
										(GtkWidget *) source_win,
										(GError *) error,
										account,
										_("mail_in_ui_folder_create_error_memory"));
			g_object_unref (account);
		}
		if (!disk_full) {
			/* Show an error and try again if there is no
			   full memory condition */
			modest_platform_information_banner ((GtkWidget *) source_win, NULL,
							    _("mail_in_ui_folder_create_error"));
			do_create_folder ((ModestWindow *) source_win,
					  parent_folder, (const gchar *) suggested_name);
		}

	} else {
		/* the 'source_win' is either the ModestWindow, or the 'Move to folder'-dialog
		 * FIXME: any other? */
		GtkWidget *folder_view;

			folder_view = GTK_WIDGET(g_object_get_data (G_OBJECT (source_win),
								    MODEST_MOVE_TO_DIALOG_FOLDER_VIEW));

		/* Select the newly created folder. It could happen
		   that the widget is no longer there (i.e. the window
		   has been destroyed, so we need to check this */
		if (folder_view)
			modest_folder_view_select_folder (MODEST_FOLDER_VIEW (folder_view),
							  new_folder, FALSE);
		g_object_unref (new_folder);
	}
	/* Free. Note that the first time it'll be NULL so noop */
	g_free (suggested_name);
	g_object_unref (source_win);
}

typedef struct {
	gchar *folder_name;
	TnyFolderStore *parent;
} CreateFolderConnect;

static void
do_create_folder_performer (gboolean canceled,
			    GError *err,
			    ModestWindow *parent_window,
			    TnyAccount *account,
			    gpointer user_data)
{
	CreateFolderConnect *helper = (CreateFolderConnect *) user_data;
	ModestMailOperation *mail_op;

	if (canceled || err) {
		/* In disk full conditions we could get this error here */
		modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								(GtkWidget *) parent_window, err,
								NULL, _("mail_in_ui_folder_create_error_memory"));

		/* This happens if we have selected the outbox folder
		   as the parent */
		if (err && err->code == TNY_SERVICE_ERROR_UNKNOWN &&
		    TNY_IS_MERGE_FOLDER (helper->parent)) {
			/* Show an error and retry */
			modest_platform_information_banner ((GtkWidget *) parent_window,
							    NULL,
							    _("mail_in_ui_folder_create_error"));

			do_create_folder (parent_window, helper->parent, helper->folder_name);
		}

		goto frees;
	}

	mail_op  = modest_mail_operation_new ((GObject *) parent_window);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);
	modest_mail_operation_create_folder (mail_op,
					     helper->parent,
					     (const gchar *) helper->folder_name,
					     do_create_folder_cb,
					     g_strdup (helper->folder_name));
	g_object_unref (mail_op);

 frees:
	if (helper->parent)
		g_object_unref (helper->parent);
	if (helper->folder_name)
		g_free (helper->folder_name);
	g_slice_free (CreateFolderConnect, helper);
}


static void
do_create_folder (ModestWindow *parent_window,
		  TnyFolderStore *suggested_parent,
		  const gchar *suggested_name)
{
	gint result;
	gchar *folder_name = NULL;
	TnyFolderStore *parent_folder = NULL;
	GtkWindow *toplevel;

	toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) parent_window);
	result = modest_platform_run_new_folder_dialog (toplevel,
							suggested_parent,
							(gchar *) suggested_name,
							&folder_name,
							&parent_folder);

	if (result == GTK_RESPONSE_ACCEPT && parent_folder) {
		CreateFolderConnect *helper = (CreateFolderConnect *) g_slice_new0 (CreateFolderConnect);
		helper->folder_name = g_strdup (folder_name);
		helper->parent = g_object_ref (parent_folder);

		modest_platform_connect_if_remote_and_perform (parent_window,
							       TRUE,
							       parent_folder,
							       do_create_folder_performer,
							       helper);
	}

	if (folder_name)
		g_free (folder_name);
	if (parent_folder)
		g_object_unref (parent_folder);
}

static void
modest_ui_actions_create_folder(GtkWindow *parent_window,
                                GtkWidget *folder_view,
				TnyFolderStore *parent_folder)
{
	if (!parent_folder) {
		ModestTnyAccountStore *acc_store;

		acc_store = modest_runtime_get_account_store ();

		parent_folder = (TnyFolderStore *)
			modest_tny_account_store_get_local_folders_account (acc_store);
	}

	if (parent_folder) {
		do_create_folder (MODEST_WINDOW (parent_window), parent_folder, NULL);
		g_object_unref (parent_folder);
	}
}

void
modest_ui_actions_on_new_folder (GtkAction *action, ModestWindow *window)
{

	g_return_if_fail (MODEST_IS_WINDOW(window));

	if (MODEST_IS_FOLDER_WINDOW (window)) {
		GtkWidget *folder_view;
		GtkWindow *toplevel;

		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (window)));
		toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) window);
		modest_ui_actions_create_folder (toplevel, folder_view, NULL);
	} else {
		g_assert_not_reached ();
	}
}

static void
modest_ui_actions_rename_folder_error_handler (ModestMailOperation *mail_op,
					       gpointer user_data)
{
	const GError *error = NULL;
	gchar *message = NULL;
	gboolean mem_full;
	TnyAccount *account = modest_mail_operation_get_account (mail_op);

	/* Get error message */
	error = modest_mail_operation_get_error (mail_op);
	if (!error)
		g_return_if_reached ();

	mem_full = modest_tny_account_store_is_disk_full_error (modest_runtime_get_account_store(),
								(GError *) error, account);
	if (mem_full) {
		message = g_strdup_printf (_KR("cerm_device_memory_full"), "");
	} else if (error->domain == MODEST_MAIL_OPERATION_ERROR &&
		   error->code == MODEST_MAIL_OPERATION_ERROR_FOLDER_EXISTS) {
		message = _CS_FOLDER_ALREADY_EXISTS;
	} else if (error->domain == TNY_ERROR_DOMAIN &&
		   error->code == TNY_SERVICE_ERROR_STATE) {
		/* This means that the folder is already in use (a
		   message is opened for example */
		message = _("emev_ni_internal_error");
	} else {
		message = _CS_UNABLE_TO_RENAME;
	}

	/* We don't set a parent for the dialog because the dialog
	   will be destroyed so the banner won't appear */
	modest_platform_information_banner (NULL, NULL, message);

	if (account)
		g_object_unref (account);
	if (mem_full)
		g_free (message);
}

typedef struct {
	TnyFolderStore *folder;
	gchar *new_name;
} RenameFolderInfo;

static void
on_rename_folder_cb (ModestMailOperation *mail_op,
		     TnyFolder *new_folder,
		     gpointer user_data)
{
	ModestFolderView *folder_view;

	/* If the window was closed when renaming a folder, or if
	 * it's not a main window this will happen */
	if (!MODEST_IS_FOLDER_VIEW (user_data))
		return;

	folder_view = MODEST_FOLDER_VIEW (user_data);
	/* Note that if the rename fails new_folder will be NULL */
	if (new_folder) {
		modest_folder_view_select_folder (folder_view, new_folder, FALSE);
	}
	gtk_widget_grab_focus (GTK_WIDGET (folder_view));
}

static void
on_rename_folder_performer (gboolean canceled,
			    GError *err,
			    ModestWindow *parent_window,
			    TnyAccount *account,
			    gpointer user_data)
{
	ModestMailOperation *mail_op = NULL;
	GtkTreeSelection *sel = NULL;
	GtkWidget *folder_view = NULL;
	RenameFolderInfo *data = (RenameFolderInfo*)user_data;

	if (canceled || err) {
		/* In disk full conditions we could get this error here */
		modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								(GtkWidget *) parent_window, err,
								account, NULL);
	} else {

		mail_op =
			modest_mail_operation_new_with_error_handling (G_OBJECT(parent_window),
					modest_ui_actions_rename_folder_error_handler,
					parent_window, NULL);

		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
				mail_op);
		if (MODEST_IS_FOLDER_WINDOW (parent_window)) {
			ModestFolderWindow *folder_window = (ModestFolderWindow *) parent_window;
			folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (folder_window));
		}

		/* Clear the folders view */
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_view));
		gtk_tree_selection_unselect_all (sel);

		/* Actually rename the folder */
		modest_mail_operation_rename_folder (mail_op,
						     TNY_FOLDER (data->folder),
						     (const gchar *) (data->new_name),
						     on_rename_folder_cb,
						     folder_view);
		g_object_unref (mail_op);
	}

	g_object_unref (data->folder);
	g_free (data->new_name);
	g_free (data);
}

void
modest_ui_actions_on_rename_folder (GtkAction *action,
				     ModestWindow *window)
{
	modest_ui_actions_on_edit_mode_rename_folder (window);
}

gboolean
modest_ui_actions_on_edit_mode_rename_folder (ModestWindow *window)
{
	TnyFolderStore *folder;
	GtkWidget *folder_view;
	gboolean do_rename = TRUE;

	g_return_val_if_fail (MODEST_IS_WINDOW(window), FALSE);

	if (MODEST_IS_FOLDER_WINDOW (window)) {
		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (window)));
	} else {
		return FALSE;
	}

	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));

	if (!folder)
		return FALSE;

	if (TNY_IS_FOLDER (folder)) {
		gchar *folder_name = NULL;
		gint response;
		const gchar *current_name;
		TnyFolderStore *parent;

		current_name = tny_folder_get_name (TNY_FOLDER (folder));
		parent = tny_folder_get_folder_store (TNY_FOLDER (folder));
		response = modest_platform_run_rename_folder_dialog (MODEST_WINDOW (window),
								     parent, current_name,
								     &folder_name);
		g_object_unref (parent);

		if (response != GTK_RESPONSE_ACCEPT || strlen (folder_name) == 0) {
			do_rename = FALSE;
		} else {
			RenameFolderInfo *rename_folder_data = g_new0 (RenameFolderInfo, 1);
			rename_folder_data->folder = g_object_ref (folder);
			rename_folder_data->new_name = folder_name;
			modest_platform_connect_if_remote_and_perform (window, TRUE,
					folder, on_rename_folder_performer, rename_folder_data);
		}
	}
	g_object_unref (folder);
	return do_rename;
}

static void
modest_ui_actions_delete_folder_error_handler (ModestMailOperation *mail_op,
					       gpointer user_data)
{
	GObject *win = modest_mail_operation_get_source (mail_op);
	GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel (GTK_WIDGET (win));

	modest_platform_run_information_dialog (toplevel,
						_("mail_in_ui_folder_delete_error"),
						FALSE);
	g_object_unref (win);
}

typedef struct {
	TnyFolderStore *folder;
	gboolean move_to_trash;
} DeleteFolderInfo;

static void
on_delete_folder_cb (gboolean canceled,
		     GError *err,
		     ModestWindow *parent_window,
		     TnyAccount *account,
		     gpointer user_data)
{
	DeleteFolderInfo *info = (DeleteFolderInfo*) user_data;
	GtkWidget *folder_view;
	ModestMailOperation *mail_op;
	GtkTreeSelection *sel;
	ModestWindow *modest_window;

#ifdef MODEST_TOOLKIT_HILDON2
	modest_window = (ModestWindow*) parent_window;
#else
	if (MODEST_IS_SHELL (parent_window)) {
		modest_window = modest_shell_peek_window (MODEST_SHELL (parent_window));
	} else {
		modest_window = NULL;
	}
#endif

	if (!MODEST_IS_WINDOW(modest_window) || canceled || (err!=NULL)) {
		/* Note that the connection process can fail due to
		   memory low conditions as it can not successfully
		   store the summary */
		if (!modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								     (GtkWidget*) parent_window, err,
								     account, NULL))
			g_debug ("Error connecting when trying to delete a folder");
		g_object_unref (G_OBJECT (info->folder));
		g_free (info);
		return;
	}

	if (MODEST_IS_FOLDER_WINDOW (modest_window)) {
		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (modest_window)));
	} else {
		g_object_unref (G_OBJECT (info->folder));
		g_free (info);
		return;
	}

	/* Unselect the folder before deleting it to free the headers */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_view));
	gtk_tree_selection_unselect_all (sel);

	/* Create the mail operation */
	mail_op =
		modest_mail_operation_new_with_error_handling (G_OBJECT(parent_window),
				modest_ui_actions_delete_folder_error_handler,
				NULL, NULL);

	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
			mail_op);
	modest_mail_operation_remove_folder (mail_op, TNY_FOLDER (info->folder), info->move_to_trash);

	g_object_unref (mail_op);
	g_object_unref (info->folder);
	g_free (info);
}

static gboolean
delete_folder (ModestWindow *window, gboolean move_to_trash)
{
	TnyFolderStore *folder;
	GtkWidget *folder_view;
	gint response;
	gchar *message;

	g_return_val_if_fail (MODEST_IS_WINDOW(window), FALSE);

	if (MODEST_IS_FOLDER_WINDOW (window)) {
		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (window)));
	} else {
		return FALSE;
	}
	if (!folder_view)
		return FALSE;

	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

	if (!folder)
		return FALSE;

	/* Show an error if it's an account */
	if (!TNY_IS_FOLDER (folder)) {
		modest_platform_run_information_dialog (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (window))),
							_("mail_in_ui_folder_delete_error"),
							FALSE);
		g_object_unref (G_OBJECT (folder));
		return FALSE;
	}

	/* Ask the user */
	message =  g_strdup_printf (_("mcen_nc_delete_folder_text"),
				    tny_folder_get_name (TNY_FOLDER (folder)));
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (window))),
							    (const gchar *) message);
	g_free (message);

	if (response == GTK_RESPONSE_OK) {
		TnyAccount *account = NULL;
		DeleteFolderInfo *info = NULL;
		info = g_new0(DeleteFolderInfo, 1);
		info->folder = g_object_ref (folder);
		info->move_to_trash = move_to_trash;

		account = tny_folder_get_account (TNY_FOLDER (folder));
		modest_platform_connect_if_remote_and_perform (window,
							       TRUE,
							       TNY_FOLDER_STORE (account),
							       on_delete_folder_cb, info);
		g_object_unref (account);
		g_object_unref (folder);
		return TRUE;
	} else {
		return FALSE;
	}
}

void
modest_ui_actions_on_delete_folder (GtkAction *action,
				    ModestWindow *window)
{
	modest_ui_actions_on_edit_mode_delete_folder (window);
}

gboolean
modest_ui_actions_on_edit_mode_delete_folder (ModestWindow *window)
{
	g_return_val_if_fail (MODEST_IS_WINDOW(window), TRUE);

	return delete_folder (window, FALSE);
}


typedef struct _PasswordDialogFields {
	GtkWidget *username;
	GtkWidget *password;
	GtkWidget *dialog;
} PasswordDialogFields;

static void
password_dialog_check_field (GtkEditable *editable,
			     PasswordDialogFields *fields)
{
	const gchar *value;
	gboolean any_value_empty = FALSE;

	value = modest_entry_get_text (fields->username);
	if ((value == NULL) || value[0] == '\0') {
		any_value_empty = TRUE;
	}
	value = modest_entry_get_text (fields->password);
	if ((value == NULL) || value[0] == '\0') {
		any_value_empty = TRUE;
	}
	gtk_dialog_set_response_sensitive (GTK_DIALOG (fields->dialog), GTK_RESPONSE_ACCEPT, !any_value_empty);
}

void
modest_ui_actions_on_password_requested (TnyAccountStore *account_store,
					 const gchar* server_account_name,
					 gchar **username,
					 gchar **password,
					 gboolean *cancel,
					 gboolean *remember,
					 ModestWindow *window)
{
	g_return_if_fail(server_account_name);
	gboolean completed = FALSE;
	PasswordDialogFields *fields = NULL;

	/* Initalize output parameters: */
	if (cancel)
		*cancel = FALSE;

	if (remember)
		*remember = TRUE;

#ifndef MODEST_TOOLKIT_GTK
	/* Maemo uses a different (awkward) button order,
	 * It should probably just use gtk_alternative_dialog_button_order ().
	 */
#ifdef MODEST_TOOLKIT_HILDON2
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons (_("mail_ti_password_protected"),
					     NULL,
					     GTK_DIALOG_MODAL,
					     _HL_DONE,
					     GTK_RESPONSE_ACCEPT,
					     NULL);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
					HILDON_MARGIN_DOUBLE);
#else
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons (_("mail_ti_password_protected"),
					     NULL,
					     GTK_DIALOG_MODAL,
					     _("mcen_bd_dialog_ok"),
					     GTK_RESPONSE_ACCEPT,
					     _("mcen_bd_dialog_cancel"),
					     GTK_RESPONSE_REJECT,
					     NULL);
#endif /* MODEST_TOOLKIT_HILDON2 */
#else
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons (_("mail_ti_password_protected"),
					     NULL,
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_REJECT,
					     GTK_STOCK_OK,
					     GTK_RESPONSE_ACCEPT,
					     NULL);
#endif /* MODEST_TOOLKIT_GTK */

	modest_window_mgr_set_modal (modest_runtime_get_window_mgr(), GTK_WINDOW (dialog), NULL);

	gchar *server_name = modest_account_mgr_get_server_account_hostname (
		modest_runtime_get_account_mgr(), server_account_name);
	if (!server_name) {/* This happened once, though I don't know why. murrayc. */
		g_warning("%s: Could not get server name for server account '%s'", __FUNCTION__, server_account_name);
		if (cancel)
			*cancel = TRUE;
		gtk_widget_destroy (dialog);
		return;
	}

	gchar *txt = g_strdup_printf (_("mail_ia_password_info"), server_name);
	GtkWidget *label = gtk_label_new (txt);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	g_free (txt);
	g_free (server_name);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
			    FALSE, FALSE, 0);
	server_name = NULL;

	/* username: */
	gchar *initial_username = modest_account_mgr_get_server_account_username (
		modest_runtime_get_account_mgr(), server_account_name);

	GtkWidget *entry_username = modest_toolkit_factory_create_entry (modest_runtime_get_toolkit_factory ());
	if (initial_username)
		modest_entry_set_text (entry_username, initial_username);

	/* Dim this if a connection has ever succeeded with this username,
	 * as per the UI spec: */
	/* const gboolean username_known =  */
	/* 	modest_account_mgr_get_server_account_username_has_succeeded( */
	/* 		modest_runtime_get_account_mgr(), server_account_name); */
	/* gtk_widget_set_sensitive (entry_username, !username_known); */

	/* We drop the username sensitive code and disallow changing it here
	 * as tinymail does not support really changing the username in the callback
	 */
	gtk_widget_set_sensitive (entry_username, FALSE);

	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (entry_username), HILDON_GTK_INPUT_MODE_FULL);
#endif

	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	GtkWidget *caption = modest_toolkit_utils_create_captioned (sizegroup, NULL,
								    _("mail_fi_username"), FALSE,
								    entry_username);
	gtk_widget_show (entry_username);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), caption,
		FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);

	/* password: */
	GtkWidget *entry_password = modest_toolkit_factory_create_entry (modest_runtime_get_toolkit_factory ());
	gtk_entry_set_visibility (GTK_ENTRY(entry_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY(entry_password), "*"); */

	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (entry_password),
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);
#endif

	caption = modest_toolkit_utils_create_captioned (sizegroup, NULL,
							 _("mail_fi_password"), FALSE,
							 entry_password);
	gtk_widget_show (entry_password);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), caption,
		FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	g_object_unref (sizegroup);

	if (initial_username != NULL)
		gtk_widget_grab_focus (GTK_WIDGET (entry_password));

/* This is not in the Maemo UI spec:
	remember_pass_check = gtk_check_button_new_with_label (_("Remember password"));
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), remember_pass_check,
			    TRUE, FALSE, 0);
*/

	fields = g_slice_new0 (PasswordDialogFields);
	fields->username = entry_username;
	fields->password = entry_password;
	fields->dialog = dialog;

	g_signal_connect (entry_username, "changed", G_CALLBACK (password_dialog_check_field), fields);
	g_signal_connect (entry_password, "changed", G_CALLBACK (password_dialog_check_field), fields);
	password_dialog_check_field (NULL, fields);

	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));

	while (!completed) {

		if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			if (username) {
				*username = g_strdup (modest_entry_get_text (entry_username));

				/* Note that an empty field becomes the "" string */
				if (*username && strlen (*username) > 0) {
					modest_account_mgr_set_server_account_username (modest_runtime_get_account_mgr(),
											server_account_name,
											*username);
					completed = TRUE;

					const gboolean username_was_changed =
						(strcmp (*username, initial_username) != 0);
					if (username_was_changed) {
						g_warning ("%s: tinymail does not yet support changing the "
							   "username in the get_password() callback.\n", __FUNCTION__);
					}
				} else {
					g_free (*username);
					*username = NULL;
					/* Show error */
					modest_platform_information_banner (GTK_WIDGET (dialog), NULL,
									    _("mcen_ib_username_pw_incorrect"));
					completed = FALSE;
				}
			}

			if (password) {
				*password = g_strdup (modest_entry_get_text (entry_password));

				/* We do not save the password in the configuration,
				 * because this function is only called for passwords that should
				 * not be remembered:
				 modest_server_account_set_password (
				 modest_runtime_get_account_mgr(), server_account_name,
				 *password);
				 */
			}
			if (cancel)
				*cancel   = FALSE;
		} else {
			completed = TRUE;
			if (username)
				*username = NULL;
			if (password)
				*password = NULL;
			if (cancel)
				*cancel   = TRUE;
		}
	}

/* This is not in the Maemo UI spec:
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (remember_pass_check)))
		*remember = TRUE;
	else
		*remember = FALSE;
*/

	g_free (initial_username);
	gtk_widget_destroy (dialog);
	g_slice_free (PasswordDialogFields, fields);

	/* printf ("DEBUG: %s: cancel=%d\n", __FUNCTION__, *cancel); */
}

void
modest_ui_actions_on_cut (GtkAction *action,
			  ModestWindow *window)
{
	GtkWidget *focused_widget;
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	focused_widget = gtk_container_get_focus_child ((GtkContainer *) window);
	if (GTK_IS_EDITABLE (focused_widget)) {
		gtk_editable_cut_clipboard (GTK_EDITABLE(focused_widget));
		gtk_clipboard_set_can_store (clipboard, NULL, 0);
		gtk_clipboard_store (clipboard);
	} else if (GTK_IS_TEXT_VIEW (focused_widget)) {
		GtkTextBuffer *buffer;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused_widget));
		if (modest_text_utils_buffer_selection_is_valid (buffer)) {
			gtk_text_buffer_cut_clipboard (buffer, clipboard, TRUE);
			gtk_clipboard_set_can_store (clipboard, NULL, 0);
			gtk_clipboard_store (clipboard);
		}
	} else if (MODEST_IS_HEADER_VIEW (focused_widget)) {
		TnyList *header_list = modest_header_view_get_selected_headers (
				MODEST_HEADER_VIEW (focused_widget));
		gboolean continue_download = FALSE;
		gint num_of_unc_msgs;

		num_of_unc_msgs = header_list_count_uncached_msgs(header_list);

		if (num_of_unc_msgs) {
			TnyAccount *account = get_account_from_header_list (header_list);
			if (account) {
				continue_download = connect_to_get_msg (window, num_of_unc_msgs, account);
				g_object_unref (account);
			}
		}

		if (num_of_unc_msgs == 0 || continue_download) {
/*			modest_platform_information_banner (
					NULL, NULL, _CS("mcen_ib_getting_items"));*/
			modest_header_view_cut_selection (
					MODEST_HEADER_VIEW (focused_widget));
		}

		g_object_unref (header_list);
	} else if (MODEST_IS_FOLDER_VIEW (focused_widget)) {
 		modest_folder_view_cut_selection (MODEST_FOLDER_VIEW (focused_widget));
	}
}

void
modest_ui_actions_on_copy (GtkAction *action,
			   ModestWindow *window)
{
	GtkClipboard *clipboard;
	GtkWidget *focused_widget;
	gboolean copied = TRUE;

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	focused_widget = gtk_container_get_focus_child ((GtkContainer *) window);

	if (GTK_IS_LABEL (focused_widget)) {
		gchar *selection;
		selection = modest_text_utils_label_get_selection (GTK_LABEL (focused_widget));
		gtk_clipboard_set_text (clipboard, selection, -1);
		g_free (selection);
		gtk_clipboard_set_can_store (clipboard, NULL, 0);
		gtk_clipboard_store (clipboard);
	} else if (GTK_IS_EDITABLE (focused_widget)) {
		gtk_editable_copy_clipboard (GTK_EDITABLE(focused_widget));
		gtk_clipboard_set_can_store (clipboard, NULL, 0);
		gtk_clipboard_store (clipboard);
	} else if (GTK_IS_HTML (focused_widget)) {
		const gchar *sel;
		int len = -1;
		sel = gtk_html_get_selection_html (GTK_HTML (focused_widget), &len);
		if ((sel == NULL) || (sel[0] == '\0')) {
			copied = FALSE;
		} else {
			gtk_html_copy (GTK_HTML (focused_widget));
			gtk_clipboard_set_can_store (clipboard, NULL, 0);
			gtk_clipboard_store (clipboard);
		}
	} else if (GTK_IS_TEXT_VIEW (focused_widget)) {
		GtkTextBuffer *buffer;
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused_widget));
		if (modest_text_utils_buffer_selection_is_valid (buffer)) {
			gtk_text_buffer_copy_clipboard (buffer, clipboard);
			gtk_clipboard_set_can_store (clipboard, NULL, 0);
			gtk_clipboard_store (clipboard);
		}
	} else if (MODEST_IS_HEADER_VIEW (focused_widget)) {
		TnyList *header_list = modest_header_view_get_selected_headers (
				MODEST_HEADER_VIEW (focused_widget));
		gboolean continue_download = FALSE;
		gint num_of_unc_msgs;

		num_of_unc_msgs = header_list_count_uncached_msgs(header_list);

		if (num_of_unc_msgs) {
			TnyAccount *account = get_account_from_header_list (header_list);
			if (account) {
				continue_download = connect_to_get_msg (window, num_of_unc_msgs, account);
				g_object_unref (account);
			}
		}

		if (num_of_unc_msgs == 0 || continue_download) {
			modest_platform_information_banner (
					NULL, NULL, _CS_GETTING_ITEMS);
			modest_header_view_copy_selection (
					MODEST_HEADER_VIEW (focused_widget));
		} else
			copied = FALSE;

		g_object_unref (header_list);

	} else if (MODEST_IS_FOLDER_VIEW (focused_widget)) {
 		modest_folder_view_copy_selection (MODEST_FOLDER_VIEW (focused_widget));
	}

	/* Show information banner if there was a copy to clipboard */
	if(copied)
		modest_platform_information_banner (
				NULL, NULL, _CS_COPIED);
}

void
modest_ui_actions_on_undo (GtkAction *action,
			   ModestWindow *window)
{
	if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
		modest_msg_edit_window_undo (MODEST_MSG_EDIT_WINDOW (window));
	} else {
		g_return_if_reached ();
	}
}

void
modest_ui_actions_on_redo (GtkAction *action,
			   ModestWindow *window)
{
	if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
		modest_msg_edit_window_redo (MODEST_MSG_EDIT_WINDOW (window));
	}
	else {
		g_return_if_reached ();
	}
}


static void
destroy_information_note (ModestMailOperation *mail_op,
			  gpointer user_data)
{
	/* destroy information note */
	gtk_widget_destroy (GTK_WIDGET(user_data));
}

static void
destroy_folder_information_note (ModestMailOperation *mail_op,
				 TnyFolder *new_folder,
				 gpointer user_data)
{
	/* destroy information note */
	gtk_widget_destroy (GTK_WIDGET(user_data));
}


static void
paste_as_attachment_free (gpointer data)
{
	PasteAsAttachmentHelper *helper = (PasteAsAttachmentHelper *) data;

	if (helper->banner) {
		gtk_widget_destroy (helper->banner);
		g_object_unref (helper->banner);
	}
	g_free (helper);
}

static void
paste_msg_as_attachment_cb (ModestMailOperation *mail_op,
			    TnyHeader *header,
			    TnyMsg *msg,
			    gpointer userdata)
{
	PasteAsAttachmentHelper *helper = (PasteAsAttachmentHelper *) userdata;
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (helper->window));

	if (msg == NULL)
		return;

	modest_msg_edit_window_add_part (MODEST_MSG_EDIT_WINDOW (helper->window), TNY_MIME_PART (msg));

}

void
modest_ui_actions_on_paste (GtkAction *action,
			    ModestWindow *window)
{
	GtkWidget *focused_widget = NULL;
	GtkWidget *inf_note = NULL;
	ModestMailOperation *mail_op = NULL;

	focused_widget = gtk_container_get_focus_child ((GtkContainer *) window);
	if (GTK_IS_EDITABLE (focused_widget)) {
		gtk_editable_paste_clipboard (GTK_EDITABLE(focused_widget));
	} else if (GTK_IS_TEXT_VIEW (focused_widget)) {
		ModestEmailClipboard *e_clipboard = NULL;
		e_clipboard = modest_runtime_get_email_clipboard ();
		if (modest_email_clipboard_cleared (e_clipboard)) {
			GtkTextBuffer *buffer;
			GtkClipboard *clipboard;

			clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused_widget));
			gtk_text_buffer_paste_clipboard (buffer, clipboard, NULL, TRUE);
		} else if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
			ModestMailOperation *mail_op;
			TnyFolder *src_folder = NULL;
			TnyList *data = NULL;
			gboolean delete;
			PasteAsAttachmentHelper *helper = g_new0 (PasteAsAttachmentHelper, 1);
			helper->window = MODEST_MSG_EDIT_WINDOW (window);
			helper->banner = modest_platform_animation_banner (GTK_WIDGET (window), NULL,
									   _CS_PASTING);
			modest_email_clipboard_get_data (e_clipboard, &src_folder, &data, &delete);
			mail_op = modest_mail_operation_new (G_OBJECT (window));
			if (helper->banner != NULL) {
				g_object_ref (G_OBJECT (helper->banner));
				gtk_widget_show (GTK_WIDGET (helper->banner));
			}

			if (data != NULL) {
				modest_mail_operation_get_msgs_full (mail_op,
								     data,
								     (GetMsgAsyncUserCallback) paste_msg_as_attachment_cb,
								     helper,
								     paste_as_attachment_free);
			}
			/* Free */
			if (data)
				g_object_unref (data);
			if (src_folder)
				g_object_unref (src_folder);

		}
	} else if (MODEST_IS_FOLDER_VIEW (focused_widget)) {
		ModestEmailClipboard *clipboard = NULL;
		TnyFolder *src_folder = NULL;
		TnyFolderStore *folder_store = NULL;
		TnyList *data = NULL;
		gboolean delete = FALSE;

		/* Check clipboard source */
		clipboard = modest_runtime_get_email_clipboard ();
		if (modest_email_clipboard_cleared (clipboard))
			return;

		/* Get elements to paste */
		modest_email_clipboard_get_data (clipboard, &src_folder, &data, &delete);

		/* Create a new mail operation */
		mail_op = modest_mail_operation_new (G_OBJECT(window));

		/* Get destination folder */
		folder_store = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (focused_widget));

		/* transfer messages  */
		if (data != NULL) {
			gint response = 0;

			/* Ask for user confirmation */
			response =
				modest_ui_actions_msgs_move_to_confirmation (window,
									     TNY_FOLDER (folder_store),
									     delete,
									     data);

			if (response == GTK_RESPONSE_OK) {
				/* Launch notification */
				inf_note = modest_platform_animation_banner (GTK_WIDGET (window), NULL,
									     _CS_PASTING);
				if (inf_note != NULL)  {
					gtk_window_set_modal (GTK_WINDOW(inf_note), FALSE);
					gtk_widget_show (GTK_WIDGET(inf_note));
				}

				modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
				modest_mail_operation_xfer_msgs (mail_op,
								 data,
								 TNY_FOLDER (folder_store),
								 delete,
								 destroy_information_note,
								 inf_note);
			} else {
				g_object_unref (mail_op);
			}

		} else if (src_folder != NULL) {
			/* Launch notification */
			inf_note = modest_platform_animation_banner (GTK_WIDGET (window), NULL,
								     _CS_PASTING);
			if (inf_note != NULL)  {
				gtk_window_set_modal (GTK_WINDOW(inf_note), FALSE);
				gtk_widget_show (GTK_WIDGET(inf_note));
			}

			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
			modest_mail_operation_xfer_folder (mail_op,
							   src_folder,
							   folder_store,
							   delete,
							   destroy_folder_information_note,
							   inf_note);
		}

		/* Free */
		if (data != NULL)
			g_object_unref (data);
		if (src_folder != NULL)
			g_object_unref (src_folder);
		if (folder_store != NULL)
			g_object_unref (folder_store);
	}
}


void
modest_ui_actions_on_select_all (GtkAction *action,
				 ModestWindow *window)
{
	GtkWidget *focused_widget;

	focused_widget = gtk_container_get_focus_child ((GtkContainer *) window);
	if (MODEST_IS_ATTACHMENTS_VIEW (focused_widget)) {
		modest_attachments_view_select_all (MODEST_ATTACHMENTS_VIEW (focused_widget));
	} else if (GTK_IS_LABEL (focused_widget)) {
		gtk_label_select_region (GTK_LABEL (focused_widget), 0, -1);
	} else if (GTK_IS_EDITABLE (focused_widget)) {
		gtk_editable_select_region (GTK_EDITABLE(focused_widget), 0, -1);
	} else if (GTK_IS_TEXT_VIEW (focused_widget)) {
		GtkTextBuffer *buffer;
		GtkTextIter start, end;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused_widget));
		gtk_text_buffer_get_start_iter (buffer, &start);
		gtk_text_buffer_get_end_iter (buffer, &end);
		gtk_text_buffer_select_range (buffer, &start, &end);
	} else if (GTK_IS_HTML (focused_widget)) {
		gtk_html_select_all (GTK_HTML (focused_widget));
	}

}

void
modest_ui_actions_on_mark_as_read (GtkAction *action,
				   ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW(window));

	/* Mark each header as read */
	do_headers_action (window, headers_action_mark_as_read, NULL);
}

void
modest_ui_actions_on_mark_as_unread (GtkAction *action,
				     ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW(window));

	/* Mark each header as read */
	do_headers_action (window, headers_action_mark_as_unread, NULL);
}

void
modest_ui_actions_on_change_zoom (GtkRadioAction *action,
				  GtkRadioAction *selected,
				  ModestWindow *window)
{
	gint value;

	value = gtk_radio_action_get_current_value (selected);
	if (MODEST_IS_WINDOW (window)) {
		modest_window_set_zoom (MODEST_WINDOW (window), ((gdouble)value)/100);
	}
}

void
modest_ui_actions_msg_edit_on_change_priority (GtkRadioAction *action,
					       GtkRadioAction *selected,
					       ModestWindow *window)
{
	TnyHeaderFlags flags;
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	flags = gtk_radio_action_get_current_value (selected);
	modest_msg_edit_window_set_priority_flags (MODEST_MSG_EDIT_WINDOW (window), flags);
}

void
modest_ui_actions_msg_edit_on_change_file_format (GtkRadioAction *action,
						  GtkRadioAction *selected,
						  ModestWindow *window)
{
	gint file_format;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	file_format = gtk_radio_action_get_current_value (selected);
	modest_msg_edit_window_set_file_format (MODEST_MSG_EDIT_WINDOW (window), file_format);
}


void
modest_ui_actions_on_zoom_plus (GtkAction *action,
				ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	modest_window_zoom_plus (MODEST_WINDOW (window));
}

void
modest_ui_actions_on_zoom_minus (GtkAction *action,
				 ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	modest_window_zoom_minus (MODEST_WINDOW (window));
}

void
modest_ui_actions_on_toggle_fullscreen    (GtkToggleAction *toggle,
					   ModestWindow *window)
{
	ModestWindowMgr *mgr;
	gboolean fullscreen, active;
	g_return_if_fail (MODEST_IS_WINDOW (window));

	mgr = modest_runtime_get_window_mgr ();

	active = (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (toggle)))?1:0;
	fullscreen = modest_window_mgr_get_fullscreen_mode (mgr);

	if (active != fullscreen) {
		modest_window_mgr_set_fullscreen_mode (mgr, active);
#ifndef MODEST_TOOLKIT_HILDON2
		GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) window);
		gtk_window_present (toplevel);
#endif
	}
}

void
modest_ui_actions_on_change_fullscreen (GtkAction *action,
					ModestWindow *window)
{
	ModestWindowMgr *mgr;
	gboolean fullscreen;

	g_return_if_fail (MODEST_IS_WINDOW (window));

	mgr = modest_runtime_get_window_mgr ();
	fullscreen = modest_window_mgr_get_fullscreen_mode (mgr);
	modest_window_mgr_set_fullscreen_mode (mgr, !fullscreen);

}

/*
 * Used by modest_ui_actions_on_details to call do_headers_action
 */
static void
headers_action_show_details (TnyHeader *header,
			     ModestWindow *window,
			     gpointer user_data)

{
	gboolean async_retrieval;
	GtkWindow *toplevel;
	TnyMsg *msg = NULL;

	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		async_retrieval = TRUE;
		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (window));
		async_retrieval = !TNY_IS_CAMEL_BS_MSG (msg);
	} else {
		async_retrieval = FALSE;
	}
	toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) window);
	modest_platform_run_header_details_dialog (toplevel, header, async_retrieval, msg);
	if (msg)
		g_object_unref (msg);
}

/*
 * Show the header details in a ModestDetailsDialog widget
 */
void
modest_ui_actions_on_details (GtkAction *action,
			      ModestWindow *win)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		TnyMsg *msg;
		TnyHeader *header;

		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
		if (!msg)
			return;

		header = tny_msg_get_header (msg);
		if (header) {
			headers_action_show_details (header, win, NULL);
			g_object_unref (header);
		}
		g_object_unref (msg);
	} else if (MODEST_IS_HEADER_WINDOW (win)) {
		TnyFolder *folder;
		GtkWidget *header_view;

		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (win)));
		folder = modest_header_view_get_folder (MODEST_HEADER_VIEW (header_view));
		if (folder) {
			GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);

			modest_platform_run_folder_details_dialog (toplevel, folder);
			g_object_unref (folder);
		}
	}
}

void
modest_ui_actions_on_limit_error (GtkAction *action,
				  ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win));

	modest_platform_information_banner ((GtkWidget *) win, NULL, _CS_MAXIMUM_CHARACTERS_REACHED);

}

void
modest_ui_actions_on_toggle_show_cc (GtkToggleAction *toggle,
				     ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	modest_msg_edit_window_show_cc (window, gtk_toggle_action_get_active (toggle));
}

void
modest_ui_actions_on_toggle_show_bcc (GtkToggleAction *toggle,
				      ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	modest_msg_edit_window_show_bcc (window, gtk_toggle_action_get_active (toggle));
}


void
modest_ui_actions_on_toggle_toolbar (GtkToggleAction *toggle,
				     ModestWindow *window)
{
	gboolean active, fullscreen = FALSE;
	ModestWindowMgr *mgr;

	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (toggle));

	/* Check if we want to toggle the toolbar view in fullscreen
	   or normal mode */
	if (!strcmp (gtk_action_get_name (GTK_ACTION (toggle)),
		     "ViewShowToolbarFullScreen")) {
		fullscreen = TRUE;
	}

	/* Toggle toolbar */
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_show_toolbars (mgr, G_TYPE_FROM_INSTANCE (window), active, fullscreen);
}

void
modest_ui_actions_msg_edit_on_select_font (GtkAction *action,
					   ModestMsgEditWindow *window)
{
	modest_msg_edit_window_select_font (window);
}


void
modest_ui_actions_on_folder_display_name_changed (ModestFolderView *folder_view,
						  const gchar *display_name,
						  GtkWindow *window)
{
	/* don't update the display name if it was already set;
	 * updating the display name apparently is expensive */
	const gchar* old_name = gtk_window_get_title (window);

	if (display_name == NULL)
		display_name = " ";

	if (old_name && display_name && strcmp (old_name, display_name) == 0)
		return; /* don't do anything */

	/* This is usually used to change the title of the main window, which
	 * is the one that holds the folder view. Note that this change can
	 * happen even when the widget doesn't have the focus. */
	gtk_window_set_title (window, display_name);

}

void
modest_ui_actions_on_select_contacts (GtkAction *action, ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	modest_msg_edit_window_select_contacts (window);
}

void
modest_ui_actions_on_check_names (GtkAction *action, ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	modest_msg_edit_window_check_names (window, FALSE);
}


static void
on_move_to_dialog_response (GtkDialog *dialog,
			    gint       response,
			    gpointer   user_data)
{
	GtkWidget *parent_win;
	MoveToInfo *helper = NULL;
	ModestFolderView *folder_view;
	gboolean unset_edit_mode = FALSE;

	helper = (MoveToInfo *) user_data;

	parent_win = (GtkWidget *) helper->win;
	folder_view = MODEST_FOLDER_VIEW (g_object_get_data (G_OBJECT (dialog),
							     MODEST_MOVE_TO_DIALOG_FOLDER_VIEW));
	switch (response) {
		TnyFolderStore *dst_folder;
		TnyFolderStore *selected;

	case MODEST_GTK_RESPONSE_NEW_FOLDER:
		selected = modest_folder_view_get_selected (folder_view);
		modest_ui_actions_create_folder ((GtkWindow *) dialog, GTK_WIDGET (folder_view), selected);
		g_object_unref (selected);
		return;
	case GTK_RESPONSE_NONE:
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_DELETE_EVENT:
		break;
	case GTK_RESPONSE_OK:
		dst_folder = modest_folder_view_get_selected (folder_view);

		if (MODEST_IS_FOLDER_WINDOW (parent_win)) {
			/* Clean list to move used for filtering */
			modest_folder_view_set_list_to_move (folder_view, NULL);

			modest_ui_actions_on_folder_window_move_to (GTK_WIDGET (folder_view),
								    dst_folder,
								    helper->list,
								    MODEST_WINDOW (parent_win));
		} else {
			/* if the user selected a root folder
			   (account) then do not perform any action */
			if (TNY_IS_ACCOUNT (dst_folder)) {
				g_signal_stop_emission_by_name (dialog, "response");
				return;
			}

			/* Clean list to move used for filtering */
			modest_folder_view_set_list_to_move (folder_view, NULL);

			/* Moving from headers window in edit mode */
			modest_ui_actions_on_window_move_to (NULL, helper->list,
							     dst_folder,
							     MODEST_WINDOW (parent_win));
		}

		if (dst_folder)
			g_object_unref (dst_folder);

		unset_edit_mode = TRUE;
		break;
	default:
		g_warning ("%s unexpected response id %d", __FUNCTION__, response);
	}

	/* Free the helper and exit */
	if (helper->list)
		g_object_unref (helper->list);
	if (unset_edit_mode) {
#ifdef MODEST_TOOLKIT_HILDON2
		modest_hildon2_window_unset_edit_mode (MODEST_HILDON2_WINDOW (helper->win));
#endif
	}
	g_slice_free (MoveToInfo, helper);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkWidget*
create_move_to_dialog (GtkWindow *win,
		       GtkWidget *folder_view,
		       TnyList *list_to_move)
{
	GtkWidget *dialog, *tree_view = NULL;

	dialog = modest_platform_create_move_to_dialog (win, &tree_view);


	/* It could happen that we're trying to move a message from a
	   window (msg window for example) after the main window was
	   closed, so we can not just get the model of the folder
	   view */
	if (MODEST_IS_FOLDER_VIEW (folder_view)) {
		const gchar *visible_id = NULL;

		modest_folder_view_set_style (MODEST_FOLDER_VIEW (tree_view),
					      MODEST_FOLDER_VIEW_STYLE_SHOW_ALL);
		modest_folder_view_copy_model (MODEST_FOLDER_VIEW(folder_view),
					       MODEST_FOLDER_VIEW(tree_view));

		visible_id =
			modest_folder_view_get_account_id_of_visible_server_account (MODEST_FOLDER_VIEW(folder_view));

		/* Show the same account than the one that is shown in the main window */
		modest_folder_view_set_account_id_of_visible_server_account (MODEST_FOLDER_VIEW(tree_view),
									     visible_id);
	} else {
		const gchar *active_account_name = NULL;
		ModestAccountMgr *mgr = NULL;
		ModestAccountSettings *settings = NULL;
		ModestServerAccountSettings *store_settings = NULL;
		ModestWindow *modest_window;

		modest_folder_view_set_style (MODEST_FOLDER_VIEW (tree_view),
					      MODEST_FOLDER_VIEW_STYLE_SHOW_ALL);

#ifdef MODEST_TOOLKIT_HILDON2
		modest_window = (ModestWindow *) win;
#else
		modest_window = modest_shell_peek_window (MODEST_SHELL (win));
#endif
		active_account_name = modest_window_get_active_account (modest_window);
		mgr = modest_runtime_get_account_mgr ();
		settings = modest_account_mgr_load_account_settings (mgr, active_account_name);

		if (settings) {
			const gchar *store_account_name;
			store_settings = modest_account_settings_get_store_settings (settings);
			store_account_name = modest_server_account_settings_get_account_name (store_settings);

			modest_folder_view_set_account_id_of_visible_server_account (MODEST_FOLDER_VIEW (tree_view),
										     store_account_name);
			g_object_unref (store_settings);
			g_object_unref (settings);
		}
	}

	/* we keep a pointer to the embedded folder view, so we can
	 *   retrieve it with get_folder_view_from_move_to_dialog (see
	 *   above) later (needed for focus handling)
	 */
	g_object_set_data (G_OBJECT(dialog), MODEST_MOVE_TO_DIALOG_FOLDER_VIEW, tree_view);

	/* Hide special folders */
	if (list_to_move)
		modest_folder_view_set_list_to_move (MODEST_FOLDER_VIEW (tree_view), list_to_move);

	gtk_widget_show (GTK_WIDGET (tree_view));

	return dialog;
}

/*
 * Shows a confirmation dialog to the user when we're moving messages
 * from a remote server to the local storage. Returns the dialog
 * response. If it's other kind of movement then it always returns
 * GTK_RESPONSE_OK
 *
 * This one is used by the next functions:
 *	modest_ui_actions_on_paste			- commented out
 *	drag_and_drop_from_header_view (for d&d in modest_folder_view.c)
 */
gint
modest_ui_actions_msgs_move_to_confirmation (ModestWindow *win,
					     TnyFolder *dest_folder,
					     gboolean delete,
					     TnyList *headers)
{
	gint response = GTK_RESPONSE_OK;
	TnyAccount *account = NULL;
	TnyFolder *src_folder = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;

	/* return with OK if the destination is a remote folder */
	if (modest_tny_folder_is_remote_folder (dest_folder))
		return GTK_RESPONSE_OK;

	/* Get source folder */
	iter = tny_list_create_iterator (headers);
	header = TNY_HEADER (tny_iterator_get_current (iter));
	if (header) {
		src_folder = tny_header_get_folder (header);
		g_object_unref (header);
	}
	g_object_unref (iter);

	/* if no src_folder, message may be an attahcment */
	if (src_folder == NULL)
		return GTK_RESPONSE_CANCEL;

	/* If the source is a local or MMC folder */
	if (!modest_tny_folder_is_remote_folder (src_folder)) {
		g_object_unref (src_folder);
		return GTK_RESPONSE_OK;
	}

	/* Get the account */
	account = tny_folder_get_account (src_folder);

	/* now if offline we ask the user */
	if(connect_to_get_msg (win, tny_list_get_length (headers), account))
		response = GTK_RESPONSE_OK;
	else
		response = GTK_RESPONSE_CANCEL;

	/* Frees */
	g_object_unref (src_folder);
	g_object_unref (account);

	return response;
}

static void
move_to_helper_destroyer (gpointer user_data)
{
	MoveToHelper *helper = (MoveToHelper *) user_data;

	/* Close the "Pasting" information banner */
	if (helper->banner) {
		gtk_widget_destroy (GTK_WIDGET (helper->banner));
		g_object_unref (helper->banner);
	}
	if (gtk_tree_row_reference_valid (helper->reference)) {
		gtk_tree_row_reference_free (helper->reference);
		helper->reference = NULL;
	}
	g_free (helper);
}

static void
move_to_cb (ModestMailOperation *mail_op,
	    gpointer user_data)
{
	MoveToHelper *helper = (MoveToHelper *) user_data;
	GObject *object = modest_mail_operation_get_source (mail_op);

	/* Note that the operation could have failed, in that case do
	   nothing */
	if (modest_mail_operation_get_status (mail_op) !=
	    MODEST_MAIL_OPERATION_STATUS_SUCCESS)
		goto frees;

	if (MODEST_IS_MSG_VIEW_WINDOW (object)) {
		ModestMsgViewWindow *self = MODEST_MSG_VIEW_WINDOW (object);

		if (!modest_msg_view_window_select_previous_message (self) &&
		    !modest_msg_view_window_select_next_message (self)) {
			/* No more messages to view, so close this window */
			modest_ui_actions_on_close_window (NULL, MODEST_WINDOW(self));
		}
	}
	g_object_unref (object);

 frees:
	/* Destroy the helper */
	move_to_helper_destroyer (helper);
}

static void
folder_move_to_cb (ModestMailOperation *mail_op,
		   TnyFolder *new_folder,
		   gpointer user_data)
{
	GObject *object;

	object = modest_mail_operation_get_source (mail_op);
	{
		move_to_cb (mail_op, user_data);
	}
}

static void
msgs_move_to_cb (ModestMailOperation *mail_op,
		 gpointer user_data)
{
	move_to_cb (mail_op, user_data);
}

void
modest_ui_actions_move_folder_error_handler (ModestMailOperation *mail_op,
					     gpointer user_data)
{
	GObject *win = NULL;
	const GError *error;
	TnyAccount *account = NULL;

	win = modest_mail_operation_get_source (mail_op);
	error = modest_mail_operation_get_error (mail_op);

	if (TNY_IS_FOLDER (user_data))
		account = modest_tny_folder_get_account (TNY_FOLDER (user_data));
	else if (TNY_IS_ACCOUNT (user_data))
		account = g_object_ref (user_data);

	/* If it's not a disk full error then show a generic error */
	if (!modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
							     (GtkWidget *) win, (GError *) error,
							     account, NULL))
		modest_platform_run_information_dialog ((GtkWindow *) win,
							_("mail_in_ui_folder_move_target_error"),
							FALSE);
	if (account)
		g_object_unref (account);
	if (win)
		g_object_unref (win);
}


/*
 * Checks if we need a connection to do the transfer and if the user
 * wants to connect to complete it
 */
static void
modest_ui_actions_xfer_messages_check (ModestWindow *parent_window,
				       TnyFolderStore *src_folder,
				       TnyList *headers,
				       TnyFolder *dst_folder,
				       gboolean delete_originals,
				       gboolean *need_connection,
				       gboolean *do_xfer)
{
	TnyAccount *src_account;
	gint uncached_msgs = 0;

	/* We don't need any further check if
	 *
	 * 1- the source folder is local OR
	 * 2- the device is already online
	 */
	if (!modest_tny_folder_store_is_remote (src_folder) ||
	    tny_device_is_online (modest_runtime_get_device())) {
		*need_connection = FALSE;
		*do_xfer = TRUE;
		return;
	}

	/* We must ask for a connection when
	 *
	 *   - the message(s) is not already cached   OR
	 *   - the message(s) is cached but the leave_on_server setting
	 * is FALSE (because we need to sync the source folder to
	 * delete the message from the server (for IMAP we could do it
	 * offline, it'll take place the next time we get a
	 * connection)
	 */
	uncached_msgs = header_list_count_uncached_msgs (headers);
	src_account = get_account_from_folder_store (src_folder);
	if (uncached_msgs > 0) {
		guint num_headers;
		const gchar *msg;
		GtkWindow *toplevel;

		*need_connection = TRUE;
		num_headers = tny_list_get_length (headers);
		msg = ngettext ("mcen_nc_get_msg", "mcen_nc_get_msgs", num_headers);
		toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) parent_window);

		if (modest_platform_run_confirmation_dialog (toplevel, msg) ==
		    GTK_RESPONSE_CANCEL) {
			*do_xfer = FALSE;
		} else {
			*do_xfer = TRUE;
		}
	} else {
		/* The transfer is possible and the user wants to */
		*do_xfer = TRUE;

		if (remote_folder_has_leave_on_server (src_folder) && delete_originals) {
			const gchar *account_name;
			gboolean leave_on_server;

			account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (src_account);
			leave_on_server = modest_account_mgr_get_leave_on_server (modest_runtime_get_account_mgr (),
										  account_name);

			if (leave_on_server == TRUE) {
				*need_connection = FALSE;
			} else {
				*need_connection = TRUE;
			}
		} else {
			*need_connection = FALSE;
		}
	}

	/* Frees */
	g_object_unref (src_account);
}

static void
xfer_messages_error_handler (ModestMailOperation *mail_op,
			     gpointer user_data)
{
	GObject *win;
	const GError *error;
	TnyAccount *account;

	win = modest_mail_operation_get_source (mail_op);
	error = modest_mail_operation_get_error (mail_op);

	/* We cannot get the account from the mail op as that is the
	   source account and for checking memory full conditions we
	   need the destination one */
	account = TNY_ACCOUNT (user_data);

	if (error &&
	    !modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
							     (GtkWidget *) win, (GError*) error,
							     account, _KR("cerm_memory_card_full"))) {
		modest_platform_run_information_dialog ((GtkWindow *) win,
							_("mail_in_ui_folder_move_target_error"),
							FALSE);
	}
	if (win)
		g_object_unref (win);
}

typedef struct {
	TnyFolderStore *dst_folder;
	TnyList *headers;
} XferMsgsHelper;

/**
 * Utility function that transfer messages from both the main window
 * and the msg view window when using the "Move to" dialog
 */
static void
xfer_messages_performer  (gboolean canceled,
			  GError *err,
			  ModestWindow *parent_window,
			  TnyAccount *account,
			  gpointer user_data)
{
	TnyAccount *dst_account = NULL;
	gboolean dst_forbids_message_add = FALSE;
	XferMsgsHelper *helper;
	MoveToHelper *movehelper;
	ModestMailOperation *mail_op;

	helper = (XferMsgsHelper *) user_data;

	if (canceled || err) {
		if (!modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								     (GtkWidget *) parent_window, err,
								     account, NULL)) {
			/* Show the proper error message */
			modest_ui_actions_on_account_connection_error (parent_window, account);
		}
		goto end;
	}

	dst_account = tny_folder_get_account (TNY_FOLDER (helper->dst_folder));

	/* tinymail will return NULL for local folders it seems */
	dst_forbids_message_add = modest_protocol_registry_protocol_type_has_tag (modest_runtime_get_protocol_registry (),
										  modest_tny_account_get_protocol_type (dst_account),
										  MODEST_PROTOCOL_REGISTRY_STORE_FORBID_INCOMING_XFERS);

	if (dst_forbids_message_add) {
		modest_platform_information_banner (GTK_WIDGET (parent_window),
						    NULL,
						    ngettext("mail_in_ui_folder_move_target_error",
							     "mail_in_ui_folder_move_targets_error",
							     tny_list_get_length (helper->headers)));
		goto end;
	}

	movehelper = g_new0 (MoveToHelper, 1);


	/* Perform the mail operation */
	mail_op = modest_mail_operation_new_with_error_handling (G_OBJECT(parent_window),
								 xfer_messages_error_handler,
								 g_object_ref (dst_account),
								 g_object_unref);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);

	modest_mail_operation_xfer_msgs (mail_op,
					 helper->headers,
					 TNY_FOLDER (helper->dst_folder),
					 TRUE,
					 msgs_move_to_cb,
					 movehelper);

	g_object_unref (G_OBJECT (mail_op));
 end:
	if (dst_account)
		g_object_unref (dst_account);
	g_object_unref (helper->dst_folder);
	g_object_unref (helper->headers);
	g_slice_free (XferMsgsHelper, helper);
}

typedef struct {
	TnyFolder *src_folder;
	TnyFolderStore *dst_folder;
	gboolean delete_original;
	GtkWidget *folder_view;
} MoveFolderInfo;

static void
on_move_folder_cb (gboolean canceled,
		   GError *err,
		   ModestWindow *parent_window,
		   TnyAccount *account,
		   gpointer user_data)
{
	MoveFolderInfo *info = (MoveFolderInfo*)user_data;
	GtkTreeSelection *sel;
	ModestMailOperation *mail_op = NULL;

	if (canceled || err || !MODEST_IS_WINDOW (parent_window)) {
		/* Note that the connection process can fail due to
		   memory low conditions as it can not successfully
		   store the summary */
		if (!modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								     (GtkWidget*) parent_window, err,
								     account, NULL))
			g_debug ("Error connecting when trying to move a folder");

		g_object_unref (G_OBJECT (info->src_folder));
		g_object_unref (G_OBJECT (info->dst_folder));
		g_free (info);
		return;
	}

	MoveToHelper *helper = g_new0 (MoveToHelper, 1);
#ifndef MODEST_TOOLKIT_HILDON2
	helper->banner = modest_platform_animation_banner (GTK_WIDGET (parent_window), NULL,
			_CS_PASTING);
	if (helper->banner != NULL)  {
		g_object_ref (helper->banner);
		gtk_widget_show (GTK_WIDGET(helper->banner));
	}
#endif
	/* Clean folder on header view before moving it */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (info->folder_view));
	gtk_tree_selection_unselect_all (sel);

	/* Let gtk events run. We need that the folder
	   view frees its reference to the source
	   folder *before* issuing the mail operation
	   so we need the signal handler of selection
	   changed to happen before the mail
	   operation
	while (gtk_events_pending ())
		gtk_main_iteration ();   */

	mail_op =
		modest_mail_operation_new_with_error_handling (G_OBJECT(parent_window),
							       modest_ui_actions_move_folder_error_handler,
							       g_object_ref (info->dst_folder), g_object_unref);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);

	modest_mail_operation_xfer_folder (mail_op,
			TNY_FOLDER (info->src_folder),
			info->dst_folder,
			info->delete_original,
			folder_move_to_cb,
			helper);
	g_object_unref (G_OBJECT (info->src_folder));

	/* if (modest_mail_operation_get_status (mail_op) == MODEST_MAIL_OPERATION_STATUS_SUCCESS) {        */
	/* } */

	/* Unref mail operation */
	g_object_unref (G_OBJECT (mail_op));
	g_object_unref (G_OBJECT (info->dst_folder));
	g_free (user_data);
}

static TnyAccount *
get_account_from_folder_store (TnyFolderStore *folder_store)
{
	if (TNY_IS_ACCOUNT (folder_store))
		return g_object_ref (folder_store);
	else
		return tny_folder_get_account (TNY_FOLDER (folder_store));
}

/*
 * UI handler for the "Move to" action when invoked from the
 * ModestFolderWindow
 */
static void
modest_ui_actions_on_folder_window_move_to (GtkWidget *folder_view,
					    TnyFolderStore *dst_folder,
					    TnyList *selection,
					    ModestWindow *win)
{
	TnyFolderStore *src_folder = NULL;
	TnyIterator *iterator;

	if (tny_list_get_length (selection) != 1)
		return;

	iterator = tny_list_create_iterator (selection);
	src_folder = TNY_FOLDER_STORE (tny_iterator_get_current (iterator));
	g_object_unref (iterator);


	gboolean do_xfer = TRUE;

	/* Allow only to transfer folders to the local root folder */
	if (TNY_IS_ACCOUNT (dst_folder) &&
	    !MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (dst_folder) &&
	    !modest_tny_account_is_memory_card_account (TNY_ACCOUNT (dst_folder))) {
		GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);

		do_xfer = FALSE;
		/* Show an error */
		modest_platform_run_information_dialog (toplevel,
							_("mail_in_ui_folder_move_target_error"),
							FALSE);
	} else if (!TNY_IS_FOLDER (src_folder)) {
		g_warning ("%s: src_folder is not a TnyFolder.\n", __FUNCTION__);
		do_xfer = FALSE;
	}

	if (do_xfer) {
		MoveFolderInfo *info = g_new0 (MoveFolderInfo, 1);
		DoubleConnectionInfo *connect_info = g_slice_new (DoubleConnectionInfo);

		info->src_folder = g_object_ref (src_folder);
		info->dst_folder = g_object_ref (dst_folder);
		info->delete_original = TRUE;
		info->folder_view = folder_view;

		connect_info->callback = on_move_folder_cb;
		connect_info->dst_account = get_account_from_folder_store (TNY_FOLDER_STORE (dst_folder));
		connect_info->data = info;

		modest_platform_double_connect_and_perform(win, TRUE,
							   TNY_FOLDER_STORE (src_folder),
							   connect_info);
	}

	/* Frees */
	g_object_unref (src_folder);
}


void
modest_ui_actions_transfer_messages_helper (ModestWindow *win,
					    TnyFolder *src_folder,
					    TnyList *headers,
					    TnyFolder *dst_folder)
{
	gboolean need_connection = TRUE;
	gboolean do_xfer = TRUE;
	XferMsgsHelper *helper;

	g_return_if_fail (TNY_IS_FOLDER (src_folder));
	g_return_if_fail (TNY_IS_FOLDER (dst_folder));
	g_return_if_fail (TNY_IS_LIST (headers));

	modest_ui_actions_xfer_messages_check (win, TNY_FOLDER_STORE (src_folder),
					       headers, TNY_FOLDER (dst_folder),
					       TRUE, &need_connection,
					       &do_xfer);

	/* If we don't want to transfer just return */
	if (!do_xfer)
		return;

	/* Create the helper */
	helper = g_slice_new (XferMsgsHelper);
	helper->dst_folder = g_object_ref (dst_folder);
	helper->headers = g_object_ref (headers);

	if (need_connection) {
		DoubleConnectionInfo *connect_info = g_slice_new (DoubleConnectionInfo);
		connect_info->callback = xfer_messages_performer;
		connect_info->dst_account = tny_folder_get_account (TNY_FOLDER (dst_folder));
		connect_info->data = helper;

		modest_platform_double_connect_and_perform(win, TRUE,
							   TNY_FOLDER_STORE (src_folder),
							   connect_info);
	} else {
		TnyAccount *src_account = get_account_from_folder_store (TNY_FOLDER_STORE (src_folder));
		xfer_messages_performer (FALSE, NULL, win,
					 src_account, helper);
		g_object_unref (src_account);
	}
}

/*
 * UI handler for the "Move to" action when invoked from the
 * ModestMsgViewWindow
 */
static void
modest_ui_actions_on_window_move_to (GtkAction *action,
				     TnyList *headers,
				     TnyFolderStore *dst_folder,
				     ModestWindow *win)
{
	TnyFolder *src_folder = NULL;

	g_return_if_fail (TNY_IS_FOLDER (dst_folder));

	if (headers) {
		TnyHeader *header = NULL;
		TnyIterator *iter;

		iter = tny_list_create_iterator (headers);
		header = (TnyHeader *) tny_iterator_get_current (iter);
		src_folder = tny_header_get_folder (header);

		/* Transfer the messages */
		modest_ui_actions_transfer_messages_helper (win, src_folder,
							    headers,
							    TNY_FOLDER (dst_folder));

		/* Frees */
		g_object_unref (header);
		g_object_unref (iter);
		g_object_unref (src_folder);
	}
}

void
modest_ui_actions_on_move_to (GtkAction *action,
			      ModestWindow *win)
{
	modest_ui_actions_on_edit_mode_move_to (win);
}

gboolean
modest_ui_actions_on_edit_mode_move_to (ModestWindow *win)
{
	GtkWidget *dialog = NULL;
	GtkWindow *toplevel = NULL;
	MoveToInfo *helper = NULL;
	TnyList *list_to_move;

	g_return_val_if_fail (MODEST_IS_WINDOW (win), FALSE);


	list_to_move = modest_platform_get_list_to_move (MODEST_WINDOW (win));

	if (!list_to_move)
		return FALSE;

	if (tny_list_get_length (list_to_move) < 1) {
		g_object_unref (list_to_move);
		return FALSE;
	}

	/* Create and run the dialog */
	toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);
	dialog = create_move_to_dialog (toplevel, NULL, list_to_move);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (dialog),
				     toplevel);

	/* Create helper */
	helper = g_slice_new0 (MoveToInfo);
	helper->list = list_to_move;
	helper->win = win;

	/* Listen to response signal */
	g_signal_connect (dialog, "response", G_CALLBACK (on_move_to_dialog_response), helper);

	/* Show the dialog */
	gtk_widget_show (dialog);

	return FALSE;
}

/*
 * Calls #HeadersFunc for each header already selected in the main
 * window or the message currently being shown in the msg view window
 */
static void
do_headers_action (ModestWindow *win,
		   HeadersFunc func,
		   gpointer user_data)
{
	TnyList *headers_list = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	TnyFolder *folder = NULL;

	/* Get headers */
	headers_list = get_selected_headers (win);
	if (!headers_list)
		return;

	/* Get the folder */
	iter = tny_list_create_iterator (headers_list);
	header = TNY_HEADER (tny_iterator_get_current (iter));
	if (header) {
		folder = tny_header_get_folder (header);
		g_object_unref (header);
	}

	/* Call the function for each header */
	while (!tny_iterator_is_done (iter)) {
		header = TNY_HEADER (tny_iterator_get_current (iter));
		func (header, win, user_data);
		g_object_unref (header);
		tny_iterator_next (iter);
	}

	/* Trick: do a poke status in order to speed up the signaling
	   of observers */
	if (folder) {
		tny_folder_poke_status (folder);
		g_object_unref (folder);
	}

	/* Frees */
	g_object_unref (iter);
	g_object_unref (headers_list);
}

void
modest_ui_actions_view_attachment (GtkAction *action,
				   ModestWindow *window)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		modest_msg_view_window_view_attachment (MODEST_MSG_VIEW_WINDOW (window), NULL);
	} else {
		/* not supported window for this action */
		g_return_if_reached ();
	}
}

void
modest_ui_actions_save_attachments (GtkAction *action,
				    ModestWindow *window)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {

		if (modest_platform_check_memory_low (MODEST_WINDOW(window), TRUE))
			return;

		modest_msg_view_window_save_attachments (MODEST_MSG_VIEW_WINDOW (window), NULL);
	} else {
		/* not supported window for this action */
		g_return_if_reached ();
	}
}

void
modest_ui_actions_remove_attachments (GtkAction *action,
				      ModestWindow *window)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		modest_msg_view_window_remove_attachments (MODEST_MSG_VIEW_WINDOW (window), FALSE);
	} else {
		/* not supported window for this action */
		g_return_if_reached ();
	}
}

void
modest_ui_actions_on_settings (GtkAction *action,
			       ModestWindow *win)
{
	GtkWidget *dialog;
	GtkWindow *toplevel;
	gboolean tree_view;

	/* We need to monitor tree view mode changes to recreate the window */
        tree_view = modest_conf_get_bool (modest_runtime_get_conf (),
        	MODEST_CONF_TREE_VIEW, NULL);

	dialog = modest_platform_get_global_settings_dialog ();
	toplevel = (GtkWindow *) gtk_widget_get_toplevel (GTK_WIDGET (win));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), toplevel);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_widget_show_all (dialog);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	if (tree_view != modest_conf_get_bool (modest_runtime_get_conf (),
        				      MODEST_CONF_TREE_VIEW, NULL)) {
#ifdef MODEST_TOOLKIT_HILDON2
  	        hildon_gtk_window_take_screenshot (toplevel, FALSE);
#endif
		ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();
		modest_window_mgr_unregister_window (mgr, MODEST_WINDOW(toplevel));
		modest_window_mgr_show_initial_window (mgr);
	}
}

void
modest_ui_actions_on_help (GtkAction *action,
			   GtkWindow *win)
{
	/* Help app is not available at all in fremantle */
#ifndef MODEST_TOOLKIT_HILDON2
	const gchar *help_id;

	g_return_if_fail (win && GTK_IS_WINDOW(win));

	help_id = modest_window_mgr_get_help_id (modest_runtime_get_window_mgr(), win);

        if (help_id)
                modest_platform_show_help (win, help_id);
#endif
}

void
modest_ui_actions_on_csm_help (GtkAction *action,
			       GtkWindow *win)
{
	/* Help app is not available at all in fremantle */
}

static void
retrieve_contents_cb (ModestMailOperation *mail_op,
		      TnyHeader *header,
		      gboolean canceled,
		      TnyMsg *msg,
		      GError *err,
		      gpointer user_data)
{
	/* We only need this callback to show an error in case of
	   memory low condition */
	if (!modest_ui_actions_msg_retrieval_check (mail_op, header, msg)) {
		g_debug ("%s: message failed to retrieve. Memory low?", __FUNCTION__);
	}
}

static void
retrieve_msg_contents_performer (gboolean canceled,
				 GError *err,
				 ModestWindow *parent_window,
				 TnyAccount *account,
				 gpointer user_data)
{
	ModestMailOperation *mail_op;
	TnyList *headers = TNY_LIST (user_data);

	if (err || canceled) {
		modest_tny_account_store_check_disk_full_error (modest_runtime_get_account_store(),
								(GtkWidget *) parent_window, err,
								account, NULL);
		goto out;
	}

	/* Create mail operation */
	mail_op = modest_mail_operation_new_with_error_handling ((GObject *) parent_window,
								 modest_ui_actions_disk_operations_error_handler,
								 NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	modest_mail_operation_get_msgs_full (mail_op, headers, retrieve_contents_cb, NULL, NULL);

	/* Frees */
	g_object_unref (mail_op);
 out:
	g_object_unref (headers);
	g_object_unref (account);
}

void
modest_ui_actions_on_retrieve_msg_contents (GtkAction *action,
					    ModestWindow *window)
{
	TnyList *headers = NULL;
	TnyAccount *account = NULL;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	TnyFolder *folder = NULL;

	/* Get headers */
	headers = get_selected_headers (window);
	if (!headers)
		return;

	/* Pick the account */
	iter = tny_list_create_iterator (headers);
	header = TNY_HEADER (tny_iterator_get_current (iter));
	folder = tny_header_get_folder (header);
	account = tny_folder_get_account (folder);
	g_object_unref (folder);
	g_object_unref (header);
	g_object_unref (iter);

	/* Connect and perform the message retrieval */
	modest_platform_connect_and_perform (window, TRUE,
					     g_object_ref (account),
					     retrieve_msg_contents_performer,
					     g_object_ref (headers));

	/* Frees */
	g_object_unref (account);
	g_object_unref (headers);
}

void
modest_ui_actions_check_toolbar_dimming_rules (ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_window_check_dimming_rules_group (window, MODEST_DIMMING_RULES_TOOLBAR);
}

void
modest_ui_actions_check_menu_dimming_rules (ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_window_check_dimming_rules_group (window, MODEST_DIMMING_RULES_MENU);
}

void
modest_ui_actions_on_email_menu_activated (GtkAction *action,
					  ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_edit_menu_activated (GtkAction *action,
					  ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_view_menu_activated (GtkAction *action,
					  ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_format_menu_activated (GtkAction *action,
					    ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_tools_menu_activated (GtkAction *action,
					  ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_attachment_menu_activated (GtkAction *action,
					  ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_toolbar_csm_menu_activated (GtkAction *action,
						 ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_folder_view_csm_menu_activated (GtkAction *action,
						     ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_header_view_csm_menu_activated (GtkAction *action,
						     ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* Update dimmed */
	modest_ui_actions_check_menu_dimming_rules (window);
}

void
modest_ui_actions_on_search_messages (GtkAction *action, ModestWindow *window)
{
	GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) window);

	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* we check for low-mem; in that case, show a warning, and don't allow
	 * searching
	 */
	if (modest_platform_check_memory_low (window, TRUE))
		return;

	modest_platform_show_search_messages (toplevel);
}

void
modest_ui_actions_on_open_addressbook (GtkAction *action, ModestWindow *win)
{
	GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) win);

	g_return_if_fail (MODEST_IS_WINDOW (win));

	/* we check for low-mem; in that case, show a warning, and don't allow
	 * for the addressbook
	 */
	if (modest_platform_check_memory_low (win, TRUE))
		return;

	modest_platform_show_addressbook (toplevel);
}


void
modest_ui_actions_on_toggle_find_in_page (GtkAction *action,
					  ModestWindow *window)
{
	gboolean active;
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	if (GTK_IS_TOGGLE_ACTION (action))
		active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	else
		active = TRUE;

	modest_msg_edit_window_toggle_isearch_toolbar (MODEST_MSG_EDIT_WINDOW (window),
						       active);
}


void
modest_ui_actions_on_send_queue_error_happened (TnySendQueue *self,
						TnyHeader *header,
						TnyMsg *msg,
						GError *err,
						gpointer user_data)
{
	const gchar* server_name = NULL;
	TnyTransportAccount *transport;
	gchar *message = NULL;
	ModestProtocol *protocol;

	/* Don't show anything if the user cancelled something or the
	 * send receive request is not interactive. Authentication
	 * errors are managed by the account store so no need to show
	 * a dialog here again */
	if (err->code == TNY_SYSTEM_ERROR_CANCEL ||
	    err->code == TNY_SERVICE_ERROR_AUTHENTICATE ||
	    !modest_tny_send_queue_get_requested_send_receive (MODEST_TNY_SEND_QUEUE (self)))
		return;


	/* Get the server name. Note that we could be using a
	   connection specific transport account */
	transport = (TnyTransportAccount *)
		tny_camel_send_queue_get_transport_account (TNY_CAMEL_SEND_QUEUE (self));
	if (transport) {
		ModestTnyAccountStore *acc_store;
		const gchar *acc_name;
		TnyTransportAccount *conn_specific;

		acc_store = modest_runtime_get_account_store();
		acc_name = modest_tny_account_get_parent_modest_account_name_for_server_account (TNY_ACCOUNT (transport));
		conn_specific = (TnyTransportAccount *)
			modest_tny_account_store_get_transport_account_for_open_connection (acc_store, acc_name);
		if (conn_specific) {
			server_name = tny_account_get_hostname (TNY_ACCOUNT (conn_specific));
			g_object_unref (conn_specific);
		} else {
			server_name = tny_account_get_hostname (TNY_ACCOUNT (transport));
		}
		g_object_unref (transport);
	}

	/* Get protocol */
	protocol = modest_protocol_registry_get_protocol_by_name (modest_runtime_get_protocol_registry (),
								  MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS,
								  tny_account_get_proto (TNY_ACCOUNT (transport)));
	if (!protocol) {
		g_warning ("%s: Account with no proto", __FUNCTION__);
		return;
	}

	/* Show the appropriate message text for the GError: */
	switch (err->code) {
	case TNY_SERVICE_ERROR_CONNECT:
		message = modest_protocol_get_translation (protocol,
							   MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR,
							   server_name);
		break;
	case TNY_SERVICE_ERROR_SEND:
		message = g_strdup (_CS_UNABLE_TO_SEND);
		break;
	case TNY_SERVICE_ERROR_UNAVAILABLE:
		message = modest_protocol_get_translation (protocol,
							   MODEST_PROTOCOL_TRANSLATION_CONNECT_ERROR,
							   server_name);
		break;
	default:
		g_warning ("%s: unexpected ERROR %d",
			   __FUNCTION__, err->code);
		message = g_strdup (_CS_UNABLE_TO_SEND);
		break;
	}

	modest_platform_run_information_dialog (NULL, message, FALSE);
	g_free (message);
}

void
modest_ui_actions_on_send_queue_status_changed (ModestTnySendQueue *send_queue,
						gchar *msg_id,
						guint status,
						gpointer user_data)
{
	ModestWindow *top_window = NULL;
	ModestWindowMgr *mgr = NULL;
	GtkWidget *header_view = NULL;
	TnyFolder *selected_folder = NULL;
	TnyFolderType folder_type;

	mgr = modest_runtime_get_window_mgr ();
	top_window = modest_window_mgr_get_current_top (mgr);

	if (!top_window)
		return;

	if (MODEST_IS_HEADER_WINDOW (top_window)) {
		header_view = (GtkWidget *)
			modest_header_window_get_header_view (MODEST_HEADER_WINDOW (top_window));
	}

	/* Get selected folder */
	if (header_view)
		selected_folder = modest_header_view_get_folder (MODEST_HEADER_VIEW (header_view));
	if (!selected_folder)
		return;

	/* gtk_tree_view_column_queue_resize is only available in GTK+ 2.8 */
#if GTK_CHECK_VERSION(2, 8, 0)
	folder_type = modest_tny_folder_guess_folder_type (selected_folder);
	if (folder_type ==  TNY_FOLDER_TYPE_OUTBOX) {
		GtkTreeViewColumn *tree_column;

		tree_column = gtk_tree_view_get_column (GTK_TREE_VIEW (header_view),
							TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN);
		if (tree_column)
			gtk_tree_view_column_queue_resize (tree_column);
		}
#else /* #if GTK_CHECK_VERSION(2, 8, 0) */
	gtk_widget_queue_draw (header_view);
#endif

#ifndef MODEST_TOOLKIT_HILDON2
	/* Rerun dimming rules, because the message could become deletable for example */
	modest_window_check_dimming_rules_group (MODEST_WINDOW (top_window),
						 MODEST_DIMMING_RULES_TOOLBAR);
	modest_window_check_dimming_rules_group (MODEST_WINDOW (top_window),
						 MODEST_DIMMING_RULES_MENU);
#endif

	/* Free */
	g_object_unref (selected_folder);
}

void
modest_ui_actions_on_account_connection_error (ModestWindow *parent_window,
					       TnyAccount *account)
{
	ModestProtocolType protocol_type;
	ModestProtocol *protocol;
	gchar *error_note = NULL;

	protocol_type = modest_tny_account_get_protocol_type (account);
	protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (),
								  protocol_type);

	error_note = modest_protocol_get_translation (protocol, MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR, tny_account_get_hostname (account));
	if (error_note == NULL) {
		g_warning ("%s: This should not be reached", __FUNCTION__);
	} else {
		GtkWindow *toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) parent_window);
		modest_platform_run_information_dialog (toplevel, error_note, FALSE);
		g_free (error_note);
	}
}

gchar *
modest_ui_actions_get_msg_already_deleted_error_msg (ModestWindow *win)
{
	gchar *msg = NULL;
	gchar *subject;
	TnyFolderStore *folder = NULL;
	TnyAccount *account = NULL;
	ModestProtocolType proto;
	ModestProtocol *protocol;
	TnyHeader *header = NULL;

	if (MODEST_IS_HEADER_WINDOW (win)) {
		GtkWidget *header_view;
		TnyList* headers = NULL;
		TnyIterator *iter;
		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (win)));
		headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW (header_view));
		if (!headers || tny_list_get_length (headers) == 0) {
			if (headers)
				g_object_unref (headers);
			return NULL;
		}
		iter = tny_list_create_iterator (headers);
		header = TNY_HEADER (tny_iterator_get_current (iter));
		if (header) {
			folder = TNY_FOLDER_STORE (tny_header_get_folder (header));
		} else {
			g_warning ("List should contain headers");
		}
		g_object_unref (iter);
		g_object_unref (headers);
	} else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));
		if (header)
			folder = TNY_FOLDER_STORE (tny_header_get_folder (header));
	}

	if (!header || !folder)
		goto frees;

	/* Get the account type */
	account = tny_folder_get_account (TNY_FOLDER (folder));
	proto = modest_tny_account_get_protocol_type (account);
	protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (),
								  proto);

	subject = tny_header_dup_subject (header);
	msg = modest_protocol_get_translation (protocol, MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE, subject);
	if (subject)
		g_free (subject);
	if (msg == NULL) {
		msg = g_strdup_printf (_("mail_ni_ui_folder_get_msg_folder_error"));
	}

 frees:
	/* Frees */
	if (account)
		g_object_unref (account);
	if (folder)
		g_object_unref (folder);
	if (header)
		g_object_unref (header);

	return msg;
}

gboolean
modest_ui_actions_on_delete_account (GtkWindow *parent_window,
				     const gchar *account_name,
				     const gchar *account_title)
{
	ModestAccountMgr *account_mgr;
	gchar *txt = NULL;
	gint response;
	ModestProtocol *protocol;
	gboolean removed = FALSE;

	g_return_val_if_fail (account_name, FALSE);
	g_return_val_if_fail (account_title, FALSE);

	account_mgr = modest_runtime_get_account_mgr();

	/* The warning text depends on the account type: */
	protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (),
								  modest_account_mgr_get_store_protocol (account_mgr,
													 account_name));
	txt = modest_protocol_get_translation (protocol,
					       MODEST_PROTOCOL_TRANSLATION_DELETE_MAILBOX,
					       account_title);
	if (txt == NULL)
		txt = g_strdup_printf (_("emev_nc_delete_mailbox"), account_title);

	response = modest_platform_run_confirmation_dialog (parent_window, txt);
	g_free (txt);
	txt = NULL;

	if (response == GTK_RESPONSE_OK) {
		/* Remove account. If it succeeds then it also removes
		   the account from the ModestAccountView: */
		gboolean is_default = FALSE;
		gchar *default_account_name = modest_account_mgr_get_default_account (account_mgr);
		if (default_account_name && (strcmp (default_account_name, account_name) == 0))
			is_default = TRUE;
		g_free (default_account_name);

		removed = modest_account_mgr_remove_account (account_mgr, account_name);
		if (removed) {
#ifdef MODEST_TOOLKIT_HILDON2
			hildon_gtk_window_take_screenshot (parent_window, FALSE);
#endif
			/* Close all email notifications, we cannot
			   distinguish if the notification belongs to
			   this account or not, so for safety reasons
			   we remove them all */
			modest_platform_remove_new_mail_notifications (FALSE, account_name);
		} else {
			g_warning ("%s: modest_account_mgr_remove_account() failed.\n", __FUNCTION__);
		}
	}
	return removed;
}

static void
on_fetch_images_performer (gboolean canceled,
			   GError *err,
			   ModestWindow *parent_window,
			   TnyAccount *account,
			   gpointer user_data)
{
	if (err || canceled) {
		/* Show an unable to retrieve images ??? */
		return;
	}

	/* Note that the user could have closed the window while connecting */
	if (GTK_WIDGET_VISIBLE (parent_window))
		modest_msg_view_window_fetch_images ((ModestMsgViewWindow *) parent_window);
	g_object_unref ((GObject *) user_data);
}

void
modest_ui_actions_on_fetch_images (GtkAction *action,
				   ModestWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));

	modest_platform_connect_and_perform (window, TRUE,
					     NULL,
					     on_fetch_images_performer,
					     g_object_ref (window));
}

void
modest_ui_actions_on_reload_message (const gchar *msg_id)
{
	ModestWindow *window = NULL;

	g_return_if_fail (msg_id && msg_id[0] != '\0');
	if (!modest_window_mgr_find_registered_message_uid (modest_runtime_get_window_mgr (),
							    msg_id,
							    &window))
		return;


	if (window == NULL || !MODEST_IS_MSG_VIEW_WINDOW (window))
		return;

	modest_msg_view_window_reload (MODEST_MSG_VIEW_WINDOW (window));
}

/** Check whether any connections are active, and cancel them if 
 * the user wishes.
 * Returns TRUE is there was no problem, 
 * or if an operation was cancelled so we can continue.
 * Returns FALSE if the user chose to cancel his request instead.
 */

gboolean
modest_ui_actions_check_for_active_account (ModestWindow *self,
					    const gchar* account_name)
{
	ModestTnySendQueue *send_queue;
	ModestTnyAccountStore *acc_store;
	ModestMailOperationQueue* queue;
	TnyConnectionStatus store_conn_status;
	TnyAccount *store_account = NULL, *transport_account = NULL;
	gboolean retval = TRUE, sending = FALSE;

	acc_store = modest_runtime_get_account_store ();
	queue = modest_runtime_get_mail_operation_queue ();

	store_account = 
		modest_tny_account_store_get_server_account (acc_store,
							     account_name,
							     TNY_ACCOUNT_TYPE_STORE);

	/* This could happen if the account was deleted before the
	   call to this function */
	if (!store_account)
		return FALSE;

	transport_account = 
		modest_tny_account_store_get_server_account (acc_store,
							     account_name,
							     TNY_ACCOUNT_TYPE_TRANSPORT);

	/* This could happen if the account was deleted before the
	   call to this function */
	if (!transport_account) {
		g_object_unref (store_account);
		return FALSE;
	}

	/* If the transport account was not used yet, then the send
	   queue could not exist (it's created on demand) */
	send_queue = modest_runtime_get_send_queue (TNY_TRANSPORT_ACCOUNT (transport_account), FALSE);
	if (TNY_IS_SEND_QUEUE (send_queue))
		sending = modest_tny_send_queue_sending_in_progress (send_queue);

	store_conn_status = tny_account_get_connection_status (store_account);
	if (store_conn_status == TNY_CONNECTION_STATUS_CONNECTED || sending) {
		gint response;

		response = modest_platform_run_confirmation_dialog (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))), 
								    _("emev_nc_disconnect_account"));
		if (response == GTK_RESPONSE_OK) {
			retval = TRUE;
		} else {
			retval = FALSE;
		}
	}

	if (retval) {

		/* FIXME: We should only cancel those of this account */
		modest_mail_operation_queue_cancel_all (queue);

		/* Also disconnect the account */
		if ((tny_account_get_connection_status (store_account) != TNY_CONNECTION_STATUS_DISCONNECTED) &&
		    (tny_account_get_connection_status (store_account) != TNY_CONNECTION_STATUS_DISCONNECTED_BROKEN)) {
			tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (store_account),
						      FALSE, NULL, NULL);
		}
		if (sending) {
			tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (transport_account),
						      FALSE, NULL, NULL);
		}
	}
		
	/* Frees */
	g_object_unref (store_account);
	g_object_unref (transport_account);
	
	return retval;
}

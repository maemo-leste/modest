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
#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-accounts-window.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-gtk.h>
#include <modest-header-window.h>
#include <modest-folder-window.h>
#include <modest-maemo-utils.h>
#endif

#ifdef MODEST_PLATFORM_MAEMO
#include "maemo/modest-osso-state-saving.h"
#endif /* MODEST_PLATFORM_MAEMO */
#ifndef MODEST_TOOLKIT_GTK
#include "maemo/modest-hildon-includes.h"
#include "maemo/modest-connection-specific-smtp-window.h"
#endif /* !MODEST_TOOLKIT_GTK */
#include <modest-utils.h>

#include "widgets/modest-ui-constants.h"
#include <widgets/modest-main-window.h>
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

#include <gtkhtml/gtkhtml.h>

#define MIN_FREE_SPACE 5 * 1024 * 1024
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

static void     folder_refreshed_cb    (ModestMailOperation *mail_op,
					TnyFolder *folder,
					gpointer user_data);

static void     on_send_receive_finished (ModestMailOperation  *mail_op,
					  gpointer user_data);

static gint header_list_count_uncached_msgs (TnyList *header_list);

static gboolean connect_to_get_msg (ModestWindow *win,
				    gint num_of_uncached_msgs,
				    TnyAccount *account);

static gboolean remote_folder_has_leave_on_server (TnyFolderStore *folder);

static void     do_create_folder (GtkWindow *window,
				  TnyFolderStore *parent_folder,
				  const gchar *suggested_name);

static TnyAccount *get_account_from_folder_store (TnyFolderStore *folder_store);

static void modest_ui_actions_on_main_window_move_to (GtkAction *action,
						      GtkWidget *folder_view,
						      TnyFolderStore *dst_folder,
						      ModestMainWindow *win);
#ifdef MODEST_TOOLKIT_HILDON2
static void modest_ui_actions_on_folder_window_move_to (GtkWidget *folder_view,
							TnyFolderStore *dst_folder,
							TnyList *selection,
							GtkWindow *win);
#endif

static void modest_ui_actions_on_window_move_to (GtkAction *action,
						 TnyList *list_to_move,
						 TnyFolderStore *dst_folder,
						 ModestWindow *win);

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

#ifndef MODEST_TOOLKIT_HILDON2
	/* always present a main window in the background
	 * we do it here, so we cannot end up with two wizards (as this
	 * function might be called in modest_window_mgr_get_main_window as well */
	if (!win)
		win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(),
							 TRUE);  /* create if not existent */
#else
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
#endif

	if (win)
		gtk_window_set_transient_for (GTK_WINDOW (wizard), GTK_WINDOW (win));

	/* make sure the mainwindow is visible. We need to present the
	   wizard again to give it the focus back. show_all are needed
	   in order to get the widgets properly drawn (MainWindow main
	   paned won't be in its right position and the dialog will be
	   missplaced */
#ifndef MODEST_TOOLKIT_HILDON2
	gtk_widget_show_all (GTK_WIDGET (win));
	gtk_widget_show_all (GTK_WIDGET (wizard));
	gtk_window_present (GTK_WINDOW (win));
	gtk_window_present (GTK_WINDOW (wizard));
#endif

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
	GtkWidget *about;
	const gchar *authors[] = {
		"Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>",
		NULL
	};
	about = gtk_about_dialog_new ();
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG(about), PACKAGE_NAME);
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
	gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW (win));
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
	if (MODEST_IS_MAIN_WINDOW(win)) {
		GtkWidget *header_view;

		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		return modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));

	} else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
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

#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_HEADER_WINDOW (win)) {
		GtkWidget *header_view;

		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (win)));
		return modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
#endif
	} else
		return NULL;
}

static GtkTreeRowReference *
get_next_after_selected_headers (ModestHeaderView *header_view)
{
	GtkTreeSelection *sel;
	GList *selected_rows, *node;
	GtkTreePath *path;
	GtkTreeRowReference *result;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (header_view));
	selected_rows = gtk_tree_selection_get_selected_rows (sel, NULL);

	if (selected_rows == NULL)
		return NULL;

	node = g_list_last (selected_rows);
	path = gtk_tree_path_copy ((GtkTreePath *) node->data);
	gtk_tree_path_next (path);

	result = gtk_tree_row_reference_new (model, path);

	gtk_tree_path_free (path);
	g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected_rows);

	return result;
}

static void
headers_action_mark_as_read (TnyHeader *header,
			     ModestWindow *win,
			     gpointer user_data)
{
	TnyHeaderFlags flags;

	g_return_if_fail (TNY_IS_HEADER(header));

	flags = tny_header_get_flags (header);
	if (flags & TNY_HEADER_FLAG_SEEN) return;
	tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);
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
		tny_header_unset_flag (header, TNY_HEADER_FLAG_SEEN);
	}
}

/** After deleing a message that is currently visible in a window,
 * show the next message from the list, or close the window if there are no more messages.
 **/
void
modest_ui_actions_refresh_message_window_after_delete (ModestMsgViewWindow* win)
{
	/* Close msg view window or select next */
	if (!modest_msg_view_window_select_next_message (win) &&
	    !modest_msg_view_window_select_previous_message (win)) {
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
	GtkWidget *header_view = NULL;
	gboolean retval = TRUE;

	g_return_val_if_fail (MODEST_IS_WINDOW(win), FALSE);

	/* Check first if the header view has the focus */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		header_view =
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		if (!gtk_widget_is_focus (header_view))
			return FALSE;
	}

	/* Get the headers, either from the header view (if win is the main window),
	 * or from the message view window: */
	header_list = get_selected_headers (win);
	if (!header_list) return FALSE;

	/* Check if any of the headers are already opened, or in the process of being opened */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		gint opened_headers = 0;

		iter = tny_list_create_iterator (header_list);
		mgr = modest_runtime_get_window_mgr ();
		while (!tny_iterator_is_done (iter)) {
			header = TNY_HEADER (tny_iterator_get_current (iter));
			if (header) {
				if (modest_window_mgr_find_registered_header (mgr, header, NULL))
					opened_headers++;
				g_object_unref (header);
			}
			tny_iterator_next (iter);
		}
		g_object_unref (iter);

		if (opened_headers > 0) {
			gchar *msg;

			msg = g_strdup_printf (_("mcen_nc_unable_to_delete_n_messages"),
					       opened_headers);

			modest_platform_run_information_dialog (GTK_WINDOW (win), (const gchar *) msg, FALSE);

			g_free (msg);
			g_object_unref (header_list);
			return FALSE;
		}
	}

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
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (win),
							    message);


	if (response == GTK_RESPONSE_OK) {
		ModestWindow *main_window = NULL;
		ModestWindowMgr *mgr = NULL;
		GtkTreeModel *model = NULL;
		GtkTreeSelection *sel = NULL;
		GList *sel_list = NULL, *tmp = NULL;
		GtkTreeRowReference *next_row_reference = NULL;
		GtkTreeRowReference *prev_row_reference = NULL;
		GtkTreePath *next_path = NULL;
		GtkTreePath *prev_path = NULL;
		ModestMailOperation *mail_op = NULL;

		/* Find last selected row */
		if (MODEST_IS_MAIN_WINDOW (win)) {
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
			sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (header_view));
			sel_list = gtk_tree_selection_get_selected_rows (sel, &model);
			for (tmp=sel_list; tmp; tmp=tmp->next) {
				if (tmp->next == NULL) {
					prev_path = gtk_tree_path_copy((GtkTreePath *) tmp->data);
					next_path = gtk_tree_path_copy((GtkTreePath *) tmp->data);

					gtk_tree_path_prev (prev_path);
					gtk_tree_path_next (next_path);

					prev_row_reference = gtk_tree_row_reference_new (model, prev_path);
					next_row_reference = gtk_tree_row_reference_new (model, next_path);
				}
			}
		}

		/* Disable window dimming management */
		modest_window_disable_dimming (MODEST_WINDOW(win));

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
		modest_window_enable_dimming (MODEST_WINDOW(win));

		if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
			modest_ui_actions_refresh_message_window_after_delete (MODEST_MSG_VIEW_WINDOW (win));

			/* Get main window */
			mgr = modest_runtime_get_window_mgr ();
			main_window = modest_window_mgr_get_main_window (mgr, FALSE); /* don't create */
		} else if (MODEST_IS_MAIN_WINDOW (win)) {
			/* Move cursor to next row */
			main_window = win;

			/* Select next or previous row */
			if (gtk_tree_row_reference_valid (next_row_reference)) {
				gtk_tree_selection_select_path (sel, next_path);
			}
			else if (gtk_tree_row_reference_valid (prev_row_reference)) {
				gtk_tree_selection_select_path (sel, prev_path);
			}

			/* Free */
			if (gtk_tree_row_reference_valid (next_row_reference))
				gtk_tree_row_reference_free (next_row_reference);
			if (next_path != NULL)
				gtk_tree_path_free (next_path);
			if (gtk_tree_row_reference_valid (prev_row_reference))
				gtk_tree_row_reference_free (prev_row_reference);
			if (prev_path != NULL)
				gtk_tree_path_free (prev_path);
		}

		/* Update toolbar dimming state */
		if (main_window) {
			modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (main_window));
			modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (main_window));
		}

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
	if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *w;
		w = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							 MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		if (gtk_widget_is_focus (w)) {
			modest_ui_actions_on_delete_folder (action, MODEST_WINDOW(win));
			return;
		}
	}
	modest_ui_actions_on_delete_message (action, win);
}

void
modest_ui_actions_on_quit (GtkAction *action, ModestWindow *win)
{
	ModestWindowMgr *mgr = NULL;

#ifdef MODEST_PLATFORM_MAEMO
	modest_osso_save_state();
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
       g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win));

       modest_msg_view_window_add_to_contacts (MODEST_MSG_VIEW_WINDOW (win));
}

void
modest_ui_actions_on_add_to_contacts (GtkAction *action, ModestWindow *win)
{
	GtkClipboard *clipboard = NULL;
	gchar *selection = NULL;

	clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	selection = gtk_clipboard_wait_for_text (clipboard);

	/* Question: why is the clipboard being used here?
	 * It doesn't really make a lot of sense. */

	if (selection)
	{
		modest_address_book_add_address (selection);
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
		GtkWindow *account_win = GTK_WINDOW (modest_account_view_window_new ());

		/* The accounts dialog must be modal */
		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), GTK_WINDOW (account_win), (GtkWindow *) win);
		modest_utils_show_dialog_and_forget (GTK_WINDOW (win), GTK_DIALOG (account_win));
	}
}

void
modest_ui_actions_on_smtp_servers (GtkAction *action, ModestWindow *win)
{
	/* This is currently only implemented for Maemo,
	 * because it requires an API (libconic) to detect different connection
	 * possiblities.
	 */
#ifndef MODEST_TOOLKIT_GTK /* Defined in config.h */

	/* Create the window if necessary: */
	GtkWidget *specific_window = GTK_WIDGET (modest_connection_specific_smtp_window_new ());
	modest_connection_specific_smtp_window_fill_with_connections (
		MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (specific_window),
		modest_runtime_get_account_mgr());

	/* Show the window: */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (specific_window), (GtkWindow *) win);
    	gtk_widget_show (specific_window);
#endif /* !MODEST_TOOLKIT_GTK */
}

static guint64
count_part_size (const gchar *part)
{
	GnomeVFSURI *vfs_uri;
	gchar *escaped_filename;
	gchar *filename;
	GnomeVFSFileInfo *info;
	guint64 result;

	/* Estimation of attachment size if we cannot get it from file info */
	result = 32768;

	vfs_uri = gnome_vfs_uri_new (part);

	escaped_filename = g_path_get_basename (gnome_vfs_uri_get_path (vfs_uri));
	filename = gnome_vfs_unescape_string_for_display (escaped_filename);
	g_free (escaped_filename);
	gnome_vfs_uri_unref (vfs_uri);

	info = gnome_vfs_file_info_new ();
	
	if (gnome_vfs_get_file_info (part, 
				     info, 
				     GNOME_VFS_FILE_INFO_GET_MIME_TYPE)
	    == GNOME_VFS_OK) {
		if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SIZE) {
			result = info->size;
		}
	}
	g_free (filename);
	gnome_vfs_file_info_unref (info);

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
	gchar *from_str = NULL, *signature = NULL, *body = NULL;
	gchar *recipient = NULL;
	gboolean use_signature = FALSE;
	ModestWindow *msg_win = NULL;
	ModestAccountMgr *mgr = modest_runtime_get_account_mgr();
	ModestTnyAccountStore *store = modest_runtime_get_account_store();
	GnomeVFSFileSize total_size, allowed_size;
	guint64 available_disk, expected_size, parts_size;
	guint parts_count;

	/* we check for low-mem */
	if (modest_platform_check_memory_low (win, TRUE))
		goto cleanup;

	available_disk = modest_utils_get_available_space (NULL);
	parts_count = g_slist_length (attachments);
	parts_size = count_parts_size (attachments);
	expected_size = modest_tny_msg_estimate_size (body, NULL, parts_count, parts_size);

	/* Double check: memory full condition or message too big */
	if (available_disk < MIN_FREE_SPACE ||
	    expected_size > available_disk) {
		gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
		modest_platform_system_banner (NULL, NULL, msg);
		g_free (msg);

		return;
	}

	if (expected_size > MODEST_MAX_ATTACHMENT_SIZE) {
		modest_platform_run_information_dialog (
			GTK_WINDOW(win),
			_KR("memr_ib_operation_disabled"),
			TRUE);
		return;
	}


#ifdef MODEST_TOOLKIT_HILDON2
	if (win)
		account_name = g_strdup (modest_window_get_active_account(win));
#endif
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
	signature = modest_account_mgr_get_signature_from_recipient (mgr, recipient, &use_signature);
	g_free (recipient);
	if (body_str != NULL) {
		body = use_signature ? g_strconcat(body_str, "\n--\n", signature, NULL) : g_strdup(body_str);
	} else {
		body = use_signature ? g_strconcat("\n--\n", signature, NULL) : g_strdup("");
	}

	msg = modest_tny_msg_new (to_str, from_str, cc_str, bcc_str, subject_str, NULL, NULL, body, NULL, NULL, NULL);
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
		GnomeVFSFileSize att_size;
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
	/* if there are no accounts yet, just show the wizard */
	if (!modest_account_mgr_has_accounts (modest_runtime_get_account_mgr(), TRUE))
		if (!modest_ui_actions_run_account_setup_wizard (win))
			return;

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
			GObject *source = modest_mail_operation_get_source (mail_op);
			modest_platform_run_information_dialog (GTK_IS_WINDOW (source) ? GTK_WINDOW (source) : NULL,
								_KR("memr_ib_operation_disabled"),
								TRUE);
			g_object_unref (source);
		}

		if (error && ((error->code == TNY_SERVICE_ERROR_NO_SUCH_MESSAGE) ||
			      error->code == TNY_SERVICE_ERROR_MESSAGE_NOT_AVAILABLE)) {
			gchar *subject, *msg, *format = NULL;
			TnyAccount *account;
			subject = tny_header_dup_subject (header);
			if (!subject)
				subject = g_strdup (_("mail_va_no_subject"));

			account = modest_mail_operation_get_account (mail_op);
			if (account) {
				ModestProtocol *protocol;
				ModestProtocolType proto;
				proto = modest_tny_account_get_protocol_type (account);
				protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (), proto);
				if (protocol)
				  format = modest_protocol_get_translation (protocol, MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE, subject);
				g_object_unref (account);
			}

			if (!format)
				format = g_strdup (_("emev_ni_ui_imap_message_not_available_in_server"));

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

	if (MODEST_IS_MAIN_WINDOW (window)) {
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_HEADER_WINDOW (window)){
		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (window)));
#endif
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
#ifdef MODEST_TOOLKIT_HILDON2
				else {
					/* In Fremantle we can not
					   open any message from
					   outbox which is not in
					   failed state */
					*can_open = FALSE;
                                }
#endif
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
			win = modest_msg_view_window_new_for_attachment (msg, account, mailbox, (const gchar*) uid);
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

	/* Update toolbar dimming state */
	if (MODEST_IS_MAIN_WINDOW (parent_win)) {
		modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (parent_win));
	}

cleanup:
	/* Free */
	g_free(account);
	g_object_unref (parent_win);
}

static gboolean
is_memory_full_error (GError *error, ModestMailOperation *mail_op, gboolean *is_mmc)
{
	gboolean enough_free_space = TRUE;
	GnomeVFSURI *cache_dir_uri;
	const gchar *cache_dir = NULL;
	GnomeVFSFileSize free_space;
	TnyAccountStore *acc_store;

	acc_store = TNY_ACCOUNT_STORE (modest_runtime_get_account_store ());

	if (is_mmc)
		*is_mmc = FALSE;

	/* Cache dir is different in case we're using an external storage (like MMC account) */
	if (mail_op) {
		TnyAccount *account = modest_mail_operation_get_account (mail_op);
		if (account) {
			if (modest_tny_account_is_memory_card_account (account)) {
				if (is_mmc)
					*is_mmc = TRUE;
				cache_dir = g_getenv (MODEST_MMC1_VOLUMEPATH_ENV);
			}
			g_object_unref (account);
		}
	}

	/* Get the default local cache dir */
	if (!cache_dir)
		cache_dir = tny_account_store_get_cache_dir (acc_store);

	cache_dir_uri = gnome_vfs_uri_new (cache_dir);
	if (cache_dir_uri) {
		if (gnome_vfs_get_volume_free_space (cache_dir_uri, &free_space) == GNOME_VFS_OK) {
			if (free_space < MIN_FREE_SPACE)
				enough_free_space = FALSE;
		}
		gnome_vfs_uri_unref (cache_dir_uri);
	}

	if ((error->code == TNY_SYSTEM_ERROR_MEMORY ||
	     /* When asking for a mail and no space left on device
		tinymail returns this error */
	     error->code == TNY_SERVICE_ERROR_MESSAGE_NOT_AVAILABLE ||
	     /* When the folder summary could not be read or
		written */
	     error->code == TNY_IO_ERROR_WRITE ||
	     error->code == TNY_IO_ERROR_READ) &&
	    !enough_free_space) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/* Shows an error if we're in memory full conditions. The #alternate
   parametter contains the error that will be shown if the memory full
   conditions happen in the mmc external storage */
static gboolean
check_memory_full_error (GtkWidget *parent_window, GError *err, const gchar *alternate)
{
	gboolean is_mcc;

	if (err == NULL)
		return FALSE;

	if (is_memory_full_error (err, NULL, &is_mcc)) {
		if (is_mcc && alternate) {
			modest_platform_information_banner (parent_window, NULL, alternate);
		} else {
			gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
			modest_platform_information_banner (parent_window, NULL, msg);
			g_free (msg);
		}
	} else if (err->code == TNY_SYSTEM_ERROR_MEMORY)
		/* If the account was created in memory full
		   conditions then tinymail won't be able to
		   connect so it'll return this error code */
		modest_platform_information_banner (parent_window,
						    NULL, _("emev_ui_imap_inbox_select_error"));
	else
		return FALSE;

	return TRUE;
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
		if (is_memory_full_error ((GError *) error, mail_op, NULL)) {
			gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
			modest_platform_information_banner ((GtkWidget *) win, NULL, msg);
			g_free (msg);
		} else if (error->code == TNY_SYSTEM_ERROR_MEMORY) {
			modest_platform_information_banner ((GtkWidget *) win,
							    NULL, _("emev_ui_imap_inbox_select_error"));
		} else if (error->domain == MODEST_MAIL_OPERATION_ERROR &&
			   error->code == MODEST_MAIL_OPERATION_ERROR_FILE_IO) {
			modest_platform_information_banner ((GtkWidget *) win,
							    NULL, _CS ("sfil_ni_unable_to_open_file_not_found"));
		} else if (user_data) {
			modest_platform_information_banner ((GtkWidget *) win,
							    NULL, user_data);
		}
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
open_msg_helper_destroyer (gpointer user_data)
{
	OpenMsgHelper *helper = (OpenMsgHelper *) user_data;

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
		    GtkWindow *parent_window,
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
	if (err || canceled) {
		modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (), helper->header);
		/* Free the helper */
		open_msg_helper_destroyer (helper);

		/* In memory full conditions we could get this error here */
		check_memory_full_error ((GtkWidget *) parent_window, err, NULL);

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

#ifndef MODEST_TOOLKIT_HILDON2
	gboolean show_open_draft = FALSE;
	if (modest_protocol_registry_protocol_type_has_tag (protocol_registry,
							    proto,
							    MODEST_PROTOCOL_REGISTRY_LOCAL_STORE_PROTOCOLS)) {
		TnyFolder *folder;
		TnyFolderType folder_type;

		folder = tny_header_get_folder (helper->header);
		folder_type = modest_tny_folder_get_local_or_mmc_folder_type (folder);
		show_open_draft = (folder_type == TNY_FOLDER_TYPE_DRAFTS);
		g_object_unref (folder);
	}
#endif

#ifdef MODEST_TOOLKIT_HILDON2
	gboolean is_draft;
	gboolean can_open;
	gchar *account_name = get_info_from_header (helper->header, &is_draft, &can_open);

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

		header_view = get_header_view_from_window (MODEST_WINDOW (parent_window));
		uid = modest_tny_folder_get_header_unique_id (helper->header);
		if (header_view) {
			const gchar *mailbox = NULL;
			mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (parent_window));
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
#endif
	/* Create the mail operation */
	mail_op =
		modest_mail_operation_new_with_error_handling ((GObject *) parent_window,
							       modest_ui_actions_disk_operations_error_handler,
							       g_strdup (error_msg), g_free);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);


#ifndef MODEST_TOOLKIT_HILDON2
	if (show_open_draft) {
		helper->banner_info = g_slice_new (OpenMsgBannerInfo);
		helper->banner_info->message = g_strdup (_("mail_ib_opening_draft_message"));
		helper->banner_info->banner = NULL;
		helper->banner_info->idle_handler = g_timeout_add (500, open_msg_banner_idle,
								   helper->banner_info);
	}
#endif


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

	g_return_if_fail (header != NULL && rowref != NULL);

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
			gtk_window_present (GTK_WINDOW (window));
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

			/* If ask for user permission to download the messages */
			response = modest_platform_run_confirmation_dialog (GTK_WINDOW (win),
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
	helper->header = g_object_ref (header);
	helper->rowref = gtk_tree_row_reference_copy (rowref);
	helper->banner_info = NULL;

	/* Connect to the account and perform */
	if (!cached) {
		modest_platform_connect_and_perform ((GtkWindow *) win, TRUE, g_object_ref (account),
						     open_msg_performer, helper);
	} else {
		/* Call directly the performer, do not need to connect */
		open_msg_performer (FALSE, NULL, (GtkWindow *) win,
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
			     TnyHeader *header)
{
	ReplyForwardHelper *rf_helper = NULL;
	const gchar *active_acc = modest_window_get_active_account (win);
	const gchar *active_mailbox = modest_window_get_active_mailbox (win);

	rf_helper = g_slice_new0 (ReplyForwardHelper);
	rf_helper->reply_forward_type = reply_forward_type;
	rf_helper->action = action;
	rf_helper->parent_window = (MODEST_IS_WINDOW (win)) ? GTK_WIDGET (win) : NULL;
	rf_helper->header = (header) ? g_object_ref (header) : NULL;
	rf_helper->account_name = (active_acc) ?
		g_strdup (active_acc) :
		modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());
	rf_helper->mailbox = g_strdup (active_mailbox);

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
	gchar *signature = NULL;
	gboolean use_signature;
	gchar *recipient;

	/* If there was any error. The mail operation could be NULL,
	   this means that we already have the message downloaded and
	   that we didn't do a mail operation to retrieve it */
	rf_helper = (ReplyForwardHelper *) user_data;
	if (mail_op && !modest_ui_actions_msg_retrieval_check (mail_op, header, msg))
		goto cleanup;

	from = modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
						   rf_helper->account_name, rf_helper->mailbox);
	recipient = modest_text_utils_get_email_address (from);
	signature = modest_account_mgr_get_signature_from_recipient (modest_runtime_get_account_mgr(), 
								     recipient, 
								     &use_signature);
	g_free (recipient);

	/* Create reply mail */
	switch (rf_helper->action) {
	case ACTION_REPLY:
		new_msg =
			modest_tny_msg_create_reply_msg (msg, header, from,
							 (use_signature) ? signature : NULL,
							 rf_helper->reply_forward_type,
							 MODEST_TNY_MSG_REPLY_MODE_SENDER);
		break;
	case ACTION_REPLY_TO_ALL:
		new_msg =
			modest_tny_msg_create_reply_msg (msg, header, from,
							 (use_signature) ? signature : NULL,
							 rf_helper->reply_forward_type,
							 MODEST_TNY_MSG_REPLY_MODE_ALL);
		edit_type = MODEST_EDIT_TYPE_REPLY;
		break;
	case ACTION_FORWARD:
		new_msg =
			modest_tny_msg_create_forward_msg (msg, from, (use_signature) ? signature : NULL,
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

	/* Allways download if we are online. */
	if (tny_device_is_online (modest_runtime_get_device ()))
		return TRUE;

	/* If offline, then ask for user permission to download the messages */
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (win),
			ngettext("mcen_nc_get_msg",
			"mcen_nc_get_msgs",
			num_of_uncached_msgs));

	if (response == GTK_RESPONSE_CANCEL)
		return FALSE;

	return modest_platform_connect_and_wait((GtkWindow *) win, account);
}

static void
reply_forward_performer (gboolean canceled,
			 GError *err,
			 GtkWindow *parent_window,
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
	modest_mail_operation_get_msg (mail_op, rf_helper->header, TRUE, reply_forward_cb, rf_helper);

	/* Frees */
	g_object_unref(mail_op);
}

/*
 * Common code for the reply and forward actions
 */
static void
reply_forward (ReplyForwardAction action, ModestWindow *win)
{
	ReplyForwardHelper *rf_helper = NULL;
	guint reply_forward_type;

	g_return_if_fail (MODEST_IS_WINDOW(win));

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
		TnyHeader *header = NULL;
		/* Get header and message. Do not free them here, the
		   reply_forward_cb must do it */
		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW(win));
		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (win));

		if (msg && header) {
			/* Create helper */
			rf_helper = create_reply_forward_helper (action, win,
								 reply_forward_type, header);
			reply_forward_cb (NULL, header, FALSE, msg, NULL, rf_helper);
		} else {
			g_warning("%s: no message or header found in viewer\n", __FUNCTION__);
		}

		if (msg)
			g_object_unref (msg);
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

					/* If ask for user permission to download the messages */
					response = modest_platform_run_confirmation_dialog (GTK_WINDOW (win),
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
									 reply_forward_type, header);
				if (uncached_msgs > 0) {
					modest_platform_connect_and_perform (GTK_WINDOW (win),
									     TRUE, account,
									     reply_forward_performer,
									     rf_helper);
				} else {
					reply_forward_performer (FALSE, NULL, GTK_WINDOW (win),
								 account, rf_helper);
				}
			}
		do_retrieve_frees:
			if (account)
				g_object_unref (account);
			if (folder)
				g_object_unref (folder);
		} else {
			reply_forward_cb (NULL, header, FALSE, NULL, NULL, rf_helper);
		}
		/* Frees */
		g_object_unref (header_list);
		g_object_unref (header);
	}
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
	if (MODEST_IS_MAIN_WINDOW (window)) {
		GtkWidget *header_view;

		header_view = modest_main_window_get_child_widget (
				MODEST_MAIN_WINDOW(window),
				MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		if (!header_view)
			return;

		modest_header_view_select_next (
				MODEST_HEADER_VIEW(header_view));
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
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

	if (MODEST_IS_MAIN_WINDOW (window)) {
		GtkWidget *header_view;
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		if (!header_view)
			return;

		modest_header_view_select_prev (MODEST_HEADER_VIEW(header_view));
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
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

	if (MODEST_IS_MAIN_WINDOW (window)) {
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_HEADER_WINDOW (window)) {
		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (window)));
#endif
	}

	if (!header_view) {
		modest_platform_information_banner (NULL, NULL, _CS("ckdg_ib_nothing_to_sort"));

		return;
	}

	/* Show sorting dialog */
	modest_utils_run_sort_dialog (GTK_WINDOW (window), MODEST_SORT_HEADERS);
}

static void
new_messages_arrived (ModestMailOperation *self,
		      TnyList *new_headers,
		      gpointer user_data)
{
	GObject *source;
	gboolean show_visual_notifications;

	source = modest_mail_operation_get_source (self);
	show_visual_notifications = (source) ? FALSE : TRUE;
	if (source)
		g_object_unref (source);

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
				tny_list_append (actually_new_list, G_OBJECT (header));
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

}

gboolean
retrieve_all_messages_cb (GObject *source,
			  guint num_msgs,
			  guint retrieve_limit)
{
	GtkWindow *window;
	gchar *msg;
	gint response;

	window = GTK_WINDOW (source);
	msg = g_strdup_printf (_("mail_nc_msg_count_limit_exceeded"),
			       num_msgs, retrieve_limit);

	/* Ask the user if they want to retrieve all the messages */
	response =
		modest_platform_run_confirmation_dialog_with_buttons (window, msg,
								      _("mcen_bd_get_all"),
								      _("mcen_bd_newest_only"));
	/* Free and return */
	g_free (msg);
	return (response == GTK_RESPONSE_ACCEPT) ? TRUE : FALSE;
}

typedef struct {
	TnyAccount *account;
	ModestWindow *win;
	gchar *account_name;
	gboolean poke_status;
	gboolean interactive;
	ModestMailOperation *mail_op;
} SendReceiveInfo;

static void
do_send_receive_performer (gboolean canceled,
			   GError *err,
			   GtkWindow *parent_window,
			   TnyAccount *account,
			   gpointer user_data)
{
	SendReceiveInfo *info;

	info = (SendReceiveInfo *) user_data;

	if (err || canceled) {
		/* In memory full conditions we could get this error here */
		check_memory_full_error ((GtkWidget *) parent_window, err, NULL);

		if (info->mail_op) {
			modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (),
							    info->mail_op);
		}
		goto clean;
	}

	/* Set send/receive operation in progress */
	if (info->win && MODEST_IS_MAIN_WINDOW (info->win)) {
		modest_main_window_notify_send_receive_initied (MODEST_MAIN_WINDOW (info->win));
	}

	if (info->win && MODEST_IS_MAIN_WINDOW (info->win))
		g_signal_connect (G_OBJECT (info->mail_op), "operation-finished",
				  G_CALLBACK (on_send_receive_finished),
				  info->win);

	/* Send & receive. */
	modest_mail_operation_update_account (info->mail_op, info->account_name, info->poke_status, info->interactive,
					      (info->win) ? retrieve_all_messages_cb : NULL,
					      new_messages_arrived, info->win);

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
			g_printerr ("modest: cannot get default account\n");
			return;
		}
	} else {
		acc_name = g_strdup (account_name);
	}

	acc_store = modest_runtime_get_account_store ();
	account = modest_tny_account_store_get_server_account (acc_store, acc_name, TNY_ACCOUNT_TYPE_STORE);

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
	/* We need to create the operation here, because otherwise it
	   could happen that the queue emits the queue-empty signal
	   while we're trying to connect the account */
	info->mail_op = modest_mail_operation_new_with_error_handling ((info->win) ? G_OBJECT (info->win) : NULL,
								       modest_ui_actions_disk_operations_error_handler,
								       NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), info->mail_op);

	/* Invoke the connect and perform */
	modest_platform_connect_and_perform ((win) ? GTK_WINDOW (win) : NULL,
					     force_connection, info->account,
					     do_send_receive_performer, info);
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
	if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *folder_view;
		TnyFolderStore *folder_store;

		folder_view =
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		if (!folder_view)
			return;

		folder_store = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

		if (folder_store)
			g_object_unref (folder_store);
		/* Refresh the active account. Force the connection if needed
		   and poke the status of all folders */
		modest_ui_actions_do_send_receive (NULL, TRUE, TRUE, TRUE, win);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_ACCOUNTS_WINDOW (win)) {
		modest_ui_actions_do_send_receive_all (win, TRUE, TRUE, TRUE);
#endif
	} else {
		const gchar *active_account;
		active_account = modest_window_get_active_account (MODEST_WINDOW (win));

		modest_ui_actions_do_send_receive (active_account, TRUE, TRUE, TRUE, win);
	}

}


void
modest_ui_actions_toggle_header_list_view (GtkAction *action, ModestMainWindow *main_window)
{
	ModestConf *conf;
	GtkWidget *header_view;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	header_view = modest_main_window_get_child_widget (main_window,
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
	if (!header_view)
		return;

	conf = modest_runtime_get_conf ();

	/* what is saved/restored is depending on the style; thus; we save with
	 * old style, then update the style, and restore for this new style
	 */
	modest_widget_memory_save (conf, G_OBJECT(header_view), MODEST_CONF_HEADER_VIEW_KEY);

	if (modest_header_view_get_style
	    (MODEST_HEADER_VIEW(header_view)) == MODEST_HEADER_VIEW_STYLE_DETAILS)
		modest_header_view_set_style (MODEST_HEADER_VIEW(header_view),
					      MODEST_HEADER_VIEW_STYLE_TWOLINES);
	else
		modest_header_view_set_style (MODEST_HEADER_VIEW(header_view),
					      MODEST_HEADER_VIEW_STYLE_DETAILS);

	modest_widget_memory_restore (conf, G_OBJECT(header_view),
				      MODEST_CONF_HEADER_VIEW_KEY);
}


void
modest_ui_actions_on_header_selected (ModestHeaderView *header_view,
				      TnyHeader *header,
				      ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	g_return_if_fail (MODEST_IS_HEADER_VIEW (header_view));

	/* in the case the folder is empty, show the empty folder message and focus
	 * folder view */
	if (!header && gtk_widget_is_focus (GTK_WIDGET (header_view))) {
		if (modest_header_view_is_empty (header_view)) {
			TnyFolder *folder = modest_header_view_get_folder (header_view);
			GtkWidget *folder_view =
				modest_main_window_get_child_widget (main_window,
								     MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
			if (folder != NULL) {
				modest_folder_view_select_folder (MODEST_FOLDER_VIEW (folder_view), folder, FALSE);
				g_object_unref (folder);
			}
			gtk_widget_grab_focus (GTK_WIDGET (folder_view));
			return;
		}
	}
	/* If no header has been selected then exit */
	if (!header)
		return;

	/* Update focus */
	if (!gtk_widget_is_focus (GTK_WIDGET(header_view)))
	    gtk_widget_grab_focus (GTK_WIDGET(header_view));

	/* Update toolbar dimming state */
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (main_window));
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (main_window));
}

void
modest_ui_actions_on_header_activated (ModestHeaderView *header_view,
				       TnyHeader *header,
				       GtkTreePath *path,
				       ModestWindow *window)
{
	GtkWidget *open_widget;
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

	if (MODEST_IS_MAIN_WINDOW (window)) {
		modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
		open_widget = modest_window_get_action_widget (MODEST_WINDOW (window), "/MenuBar/EmailMenu/EmailOpenMenu");
		if (!GTK_WIDGET_IS_SENSITIVE (open_widget))
			return;
	}

	rowref = gtk_tree_row_reference_new (gtk_tree_view_get_model (GTK_TREE_VIEW (header_view)), path);
	open_msg_from_header (header, rowref, MODEST_WINDOW (window));
	gtk_tree_row_reference_free (rowref);
}

static void
set_active_account_from_tny_account (TnyAccount *account,
				     ModestWindow *window)
{
	const gchar *server_acc_name = tny_account_get_id (account);

	/* We need the TnyAccount provided by the
	   account store because that is the one that
	   knows the name of the Modest account */
	TnyAccount *modest_server_account =
		modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store (),
							     MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
							     server_acc_name);
	if (!modest_server_account) {
		g_warning ("%s: could not get tny account\n", __FUNCTION__);
		return;
	}

	/* Update active account, but only if it's not a pseudo-account */
	if ((!modest_tny_account_is_virtual_local_folders(modest_server_account)) &&
	    (!modest_tny_account_is_memory_card_account(modest_server_account))) {
		const gchar *modest_acc_name =
			modest_tny_account_get_parent_modest_account_name_for_server_account (modest_server_account);
		if (modest_acc_name)
			modest_window_set_active_account (window, modest_acc_name);
	}

	g_object_unref (modest_server_account);
}


static void
folder_refreshed_cb (ModestMailOperation *mail_op,
		     TnyFolder *folder,
		     gpointer user_data)
{
	ModestMainWindow *win = NULL;
	GtkWidget *folder_view, *header_view;
	const GError *error;

	g_return_if_fail (TNY_IS_FOLDER (folder));

	win = MODEST_MAIN_WINDOW (user_data);

	/* Check if the operation failed due to memory low conditions */
	error = modest_mail_operation_get_error (mail_op);
	if (error && error->domain == MODEST_MAIL_OPERATION_ERROR &&
	    error->code == MODEST_MAIL_OPERATION_ERROR_LOW_MEMORY) {
		modest_platform_run_information_dialog (GTK_WINDOW (win),
							_KR("memr_ib_operation_disabled"),
							TRUE);
		return;
	}

	folder_view =
		modest_main_window_get_child_widget(win, MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	header_view =
		modest_main_window_get_child_widget(win, MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

	if (folder_view) {
		TnyFolderStore *current_folder;

		current_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));
		if (current_folder) {
			gboolean different = ((TnyFolderStore *) folder != current_folder);
			g_object_unref (current_folder);
			if (different)
				return;
		}
	}

	/* Check if folder is empty and set headers view contents style */
	if ((tny_folder_get_all_count (folder) == 0) ||
	    modest_header_view_is_empty (MODEST_HEADER_VIEW (header_view)))
		modest_main_window_set_contents_style (win,
						       MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY);
}

void
modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
					       TnyFolderStore *folder_store,
					       gboolean selected,
					       ModestMainWindow *main_window)
{
	GtkWidget *header_view;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	header_view = modest_main_window_get_child_widget(main_window,
							  MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
	if (!header_view)
		return;


	if (TNY_IS_ACCOUNT (folder_store)) {
		if (selected) {
			set_active_account_from_tny_account (TNY_ACCOUNT (folder_store), MODEST_WINDOW (main_window));

			/* Show account details */
			modest_main_window_set_contents_style (main_window, MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS);
		}
	} else {
		if (TNY_IS_FOLDER (folder_store) && selected) {
			TnyAccount *account;

			/* Update the active account */
			account = modest_tny_folder_get_account (TNY_FOLDER (folder_store));
			if (account) {
				set_active_account_from_tny_account (account, MODEST_WINDOW (main_window));
				g_object_unref (account);
				account = NULL;
			}

			/* Set the header style by default, it could
			   be changed later by the refresh callback to
			   empty */
			modest_main_window_set_contents_style (main_window,
							       MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS);

			/* Set folder on header view. This function
			   will call tny_folder_refresh_async so we
			   pass a callback that will be called when
			   finished. We use that callback to set the
			   empty view if there are no messages */
			modest_header_view_set_folder (MODEST_HEADER_VIEW(header_view),
						       TNY_FOLDER (folder_store),
						       TRUE,
						       MODEST_WINDOW (main_window),
						       folder_refreshed_cb,
						       main_window);

			/* Restore configuration. We need to do this
			   *after* the set_folder because the widget
			   memory asks the header view about its
			   folder  */
			modest_widget_memory_restore (modest_runtime_get_conf (),
						      G_OBJECT(header_view),
						      MODEST_CONF_HEADER_VIEW_KEY);
		} else {
			/* No need to save the header view
			   configuration for Maemo because it only
			   saves the sorting stuff and that it's
			   already being done by the sort
			   dialog. Remove it when the GNOME version
			   has the same behaviour */
#ifdef MODEST_TOOLKIT_GTK
			if (modest_main_window_get_contents_style (main_window) ==
			    MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS)
				modest_widget_memory_save (modest_runtime_get_conf (), 
							   G_OBJECT (header_view),
							   MODEST_CONF_HEADER_VIEW_KEY);
#endif
			modest_header_view_clear (MODEST_HEADER_VIEW(header_view));
		}
	}

	/* Update dimming state */
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (main_window));
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (main_window));
}

void
modest_ui_actions_on_item_not_found (ModestHeaderView *header_view,ModestItemType type,
				     ModestWindow *win)
{
	GtkWidget *dialog;
	gchar *txt, *item;
	gboolean online;

	item = (type == MODEST_ITEM_TYPE_FOLDER) ? "folder" : "message";

	online = tny_device_is_online (modest_runtime_get_device());

	if (online) {
		/* already online -- the item is simply not there... */
		dialog = gtk_message_dialog_new (GTK_WINDOW (win),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_NONE,
						 _("The %s you selected cannot be found"),
						 item);
		gtk_dialog_add_button (GTK_DIALOG (dialog),_("mcen_bd_dialog_ok"), GTK_RESPONSE_ACCEPT);
		gtk_dialog_run (GTK_DIALOG(dialog));
	} else {
		dialog = gtk_dialog_new_with_buttons (_("Connection requested"),
						      GTK_WINDOW (win),
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

		gtk_window_set_default_size (GTK_WINDOW(dialog), 300, 300);
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
#ifndef MODEST_TOOLKIT_HILDON2
	ModestMainWindow *win;

	/* FIXME. Make the header view sensitive again. This is a
	 * temporary hack. See modest_ui_actions_on_save_to_drafts()
	 * for details */
	win = MODEST_MAIN_WINDOW(modest_window_mgr_get_main_window(
					 modest_runtime_get_window_mgr(), FALSE));
	if (win != NULL) {
		GtkWidget *hdrview = modest_main_window_get_child_widget(
			win, MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		if (hdrview) gtk_widget_set_sensitive(hdrview, TRUE);
	}
#endif

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

	/* Double check: memory full condition or message too big */
	if (available_disk < MIN_FREE_SPACE ||
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
		modest_platform_run_information_dialog (
			GTK_WINDOW(edit_window),
			_KR("memr_ib_operation_disabled"),
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
	ModestMainWindow *win = NULL;

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

#ifdef MODEST_TOOLKIT_HILDON2
	/* In hildon2 we always show the information banner on saving to drafts.
	 * It will be a system information banner in this case.
	 */
	gchar *text = g_strdup_printf (_("mail_va_saved_to_drafts"), _("mcen_me_folder_drafts"));
	modest_platform_information_banner (NULL, NULL, text);
	g_free (text);
#else
	/* Use the main window as the parent of the banner, if the
	   main window does not exist it won't be shown, if the parent
	   window exists then it's properly shown. We don't use the
	   editor window because it could be closed (save to drafts
	   could happen after closing the window */
	win = (ModestMainWindow *)
		modest_window_mgr_get_main_window( modest_runtime_get_window_mgr(), FALSE);
	if (win) {
		gchar *text = g_strdup_printf (_("mail_va_saved_to_drafts"), _("mcen_me_folder_drafts"));
		modest_platform_information_banner (GTK_WIDGET (win), NULL, text);
		g_free (text);
	}
#endif
	modest_msg_edit_window_set_modified (edit_window, FALSE);

	/* Frees */
	g_free (account_name);
	g_object_unref (G_OBJECT (transport_account));
	g_object_unref (G_OBJECT (mail_operation));

	modest_msg_edit_window_free_msg_data (edit_window, data);

	/* ** FIXME **
	 * If the drafts folder is selected then make the header view
	 * insensitive while the message is being saved to drafts
	 * (it'll be sensitive again in on_save_to_drafts_cb()). This
	 * is not very clean but it avoids letting the drafts folder
	 * in an inconsistent state: the user could edit the message
	 * being saved and undesirable things would happen.
	 * In the average case the user won't notice anything at
	 * all. In the worst case (the user is editing a really big
	 * file from Drafts) the header view will be insensitive
	 * during the saving process (10 or 20 seconds, depending on
	 * the message). Anyway this is just a quick workaround: once
	 * we find a better solution it should be removed
	 * See NB#65125 (commend #18) for details.
	 */
	if (!had_error && win != NULL) {
		ModestFolderView *view = MODEST_FOLDER_VIEW(modest_main_window_get_child_widget(
			win, MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW));
		if (view != NULL) {
			TnyFolder *folder = TNY_FOLDER(modest_folder_view_get_selected(view));
			if (folder) {
				if (modest_tny_folder_is_local_folder(folder)) {
					TnyFolderType folder_type;
					folder_type = modest_tny_folder_get_local_or_mmc_folder_type(folder);
					if (folder_type == TNY_FOLDER_TYPE_DRAFTS) {
						GtkWidget *hdrview = modest_main_window_get_child_widget(
							win, MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
						if (hdrview) gtk_widget_set_sensitive(hdrview, FALSE);
					}
				}
			}
			if (folder != NULL) g_object_unref(folder);
		}
	}

	return !had_error;
}

/* For instance, when clicking the Send toolbar button when editing a message: */
gboolean
modest_ui_actions_on_send (GtkWidget *widget, ModestMsgEditWindow *edit_window)
{
	TnyTransportAccount *transport_account = NULL;
	gboolean had_error = FALSE;
	MsgData *data;
	ModestAccountMgr *account_mgr;
	gchar *account_name;
	ModestMailOperation *mail_operation;
	gchar *recipients;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW(edit_window), TRUE);

	if (!modest_msg_edit_window_check_names (edit_window, TRUE))
		return TRUE;

	data = modest_msg_edit_window_get_msg_data (edit_window);

	if (data->subject == NULL || data->subject[0] == '\0') {
		/* Empty subject -> no send */
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return FALSE;
	}

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
	if (strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) != 0) {
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


	/* Create the mail operation */
	mail_operation = modest_mail_operation_new_with_error_handling (NULL, modest_ui_actions_disk_operations_error_handler, NULL, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);

	modest_mail_operation_send_new_mail (mail_operation,
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
					     data->references,
					     data->in_reply_to,
					     data->priority_flags);

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

	modest_msg_edit_window_free_msg_data (edit_window, data);

	if (!had_error) {
		modest_msg_edit_window_set_sent (edit_window, TRUE);

		/* Save settings and close the window: */
		modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (edit_window));
	}

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

	modest_msg_edit_window_select_background_color (window);
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

		/* Show an error. If there was some problem writing to
		   disk, show it, otherwise show the generic folder
		   create error. We do it here and not in an error
		   handler because the call to do_create_folder will
		   stop the main loop in a gtk_dialog_run and then,
		   the message won't be shown until that dialog is
		   closed */
		if (!check_memory_full_error ((GtkWidget *) source_win, (GError *) error,
					      _("mail_in_ui_folder_create_error_memory"))) {
			/* Show an error and try again if there is no
			   full memory condition */
			modest_platform_information_banner ((GtkWidget *) source_win, NULL,
							    _("mail_in_ui_folder_create_error"));
			do_create_folder (source_win, parent_folder, (const gchar *) suggested_name);
		}
	} else {
		/* the 'source_win' is either the ModestMainWindow, or the 'Move to folder'-dialog
		 * FIXME: any other? */
		GtkWidget *folder_view;

		if (MODEST_IS_MAIN_WINDOW(source_win))
			folder_view =
				modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (source_win),
								     MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		else
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
			 GtkWindow *parent_window,
			 TnyAccount *account,
			 gpointer user_data)
{
	CreateFolderConnect *helper = (CreateFolderConnect *) user_data;
	ModestMailOperation *mail_op;

	if (canceled || err) {
		/* In memory full conditions we could get this error here */
		check_memory_full_error ((GtkWidget *) parent_window, err, 
					 _("mail_in_ui_folder_create_error_memory"));

		/* This happens if we have selected the outbox folder
		   as the parent */
		if (err->code == TNY_SERVICE_ERROR_UNKNOWN &&
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
do_create_folder (GtkWindow *parent_window,
		  TnyFolderStore *suggested_parent,
		  const gchar *suggested_name)
{
	gint result;
	gchar *folder_name = NULL;
	TnyFolderStore *parent_folder = NULL;

	result = modest_platform_run_new_folder_dialog (GTK_WINDOW (parent_window),
							suggested_parent,
							(gchar *) suggested_name,
							&folder_name,
							&parent_folder);

	if (result == GTK_RESPONSE_ACCEPT && parent_folder) {
		CreateFolderConnect *helper = (CreateFolderConnect *) g_slice_new0 (CreateFolderConnect);
		helper->folder_name = g_strdup (folder_name);
		helper->parent = g_object_ref (parent_folder);

		modest_platform_connect_if_remote_and_perform (GTK_WINDOW (parent_window),
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
modest_ui_actions_create_folder(GtkWidget *parent_window,
                                GtkWidget *folder_view,
				TnyFolderStore *parent_folder)
{
	if (!parent_folder) {
#ifdef MODEST_TOOLKIT_HILDON2
		ModestTnyAccountStore *acc_store;

		acc_store = modest_runtime_get_account_store ();

		parent_folder = (TnyFolderStore *)
			modest_tny_account_store_get_local_folders_account (acc_store);
#else
		parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
#endif
	}

	if (parent_folder) {
		do_create_folder (GTK_WINDOW (parent_window), parent_folder, NULL);
		g_object_unref (parent_folder);
	}
}

void
modest_ui_actions_on_new_folder (GtkAction *action, ModestWindow *window)
{

	g_return_if_fail (MODEST_IS_WINDOW(window));

	if (MODEST_IS_MAIN_WINDOW (window)) {
		GtkWidget *folder_view;

		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		if (!folder_view)
			return;

		modest_ui_actions_create_folder (GTK_WIDGET (window), folder_view, NULL);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_FOLDER_WINDOW (window)) {
		GtkWidget *folder_view;

		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (window)));
		modest_ui_actions_create_folder (GTK_WIDGET (window), folder_view, NULL);
#endif
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

	/* Get error message */
	error = modest_mail_operation_get_error (mail_op);
	if (!error)
		g_return_if_reached ();

	mem_full = is_memory_full_error ((GError *) error, mail_op, NULL);
	if (mem_full) {
		message = g_strdup_printf (_KR("cerm_device_memory_full"), "");
	} else if (error->domain == MODEST_MAIL_OPERATION_ERROR &&
		   error->code == MODEST_MAIL_OPERATION_ERROR_FOLDER_EXISTS) {
		message = _CS("ckdg_ib_folder_already_exists");
	} else if (error->domain == TNY_ERROR_DOMAIN &&
		   error->code == TNY_SERVICE_ERROR_STATE) {
		/* This means that the folder is already in use (a
		   message is opened for example */
		message = _("emev_ni_internal_error");
	} else {
		message = _CS("ckdg_ib_unable_to_rename");
	}

	/* We don't set a parent for the dialog because the dialog
	   will be destroyed so the banner won't appear */
	modest_platform_information_banner (NULL, NULL, message);

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
	} else {
		modest_folder_view_select_first_inbox_or_local (folder_view);
	}
	gtk_widget_grab_focus (GTK_WIDGET (folder_view));
}

static void
on_rename_folder_performer (gboolean canceled,
			    GError *err,
			    GtkWindow *parent_window,
			    TnyAccount *account,
			    gpointer user_data)
{
	ModestMailOperation *mail_op = NULL;
	GtkTreeSelection *sel = NULL;
	GtkWidget *folder_view = NULL;
	RenameFolderInfo *data = (RenameFolderInfo*)user_data;

	if (canceled || err) {
		/* In memory full conditions we could get this error here */
		check_memory_full_error ((GtkWidget *) parent_window, err, NULL);
	} else {

		mail_op =
			modest_mail_operation_new_with_error_handling (G_OBJECT(parent_window),
					modest_ui_actions_rename_folder_error_handler,
					parent_window, NULL);

		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
				mail_op);

		if (MODEST_IS_MAIN_WINDOW(parent_window)) {

			folder_view = modest_main_window_get_child_widget (
				MODEST_MAIN_WINDOW (parent_window),
				MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		}
#ifdef MODEST_TOOLKIT_HILDON2
		else if (MODEST_IS_FOLDER_WINDOW (parent_window)) {
			ModestFolderWindow *folder_window = (ModestFolderWindow *) parent_window;
			folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (folder_window));
		}
#endif

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

	if (MODEST_IS_MAIN_WINDOW (window)) {
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		if (!folder_view)
			return FALSE;

#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_FOLDER_WINDOW (window)) {
		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (window)));
#endif
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
		response = modest_platform_run_rename_folder_dialog (GTK_WINDOW (window),
								     parent, current_name,
								     &folder_name);
		g_object_unref (parent);

		if (response != GTK_RESPONSE_ACCEPT || strlen (folder_name) == 0) {
			do_rename = FALSE;
		} else {
			RenameFolderInfo *rename_folder_data = g_new0 (RenameFolderInfo, 1);
			rename_folder_data->folder = g_object_ref (folder);
			rename_folder_data->new_name = folder_name;
			modest_platform_connect_if_remote_and_perform (GTK_WINDOW(window), TRUE,
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

	modest_platform_run_information_dialog ((win) ? GTK_WINDOW (win) : NULL,
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
		  GtkWindow *parent_window,
		  TnyAccount *account,
		  gpointer user_data)
{
	DeleteFolderInfo *info = (DeleteFolderInfo*) user_data;
	GtkWidget *folder_view;
	ModestMailOperation *mail_op;
	GtkTreeSelection *sel;

	if (!MODEST_IS_WINDOW(parent_window) || canceled || (err!=NULL)) {
		g_object_unref (G_OBJECT (info->folder));
		g_free (info);
		return;
	}

	if (MODEST_IS_MAIN_WINDOW (parent_window)) {
		folder_view = modest_main_window_get_child_widget (
			MODEST_MAIN_WINDOW (parent_window),
			MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_FOLDER_WINDOW (parent_window)) {
		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (parent_window)));
#endif
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

	modest_folder_view_select_first_inbox_or_local (MODEST_FOLDER_VIEW (folder_view));

	g_object_unref (G_OBJECT (mail_op));
	g_object_unref (G_OBJECT (info->folder));
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

	if (MODEST_IS_MAIN_WINDOW (window)) {

		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_FOLDER_WINDOW (window)) {
		folder_view = GTK_WIDGET (modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (window)));
#endif
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
		modest_platform_run_information_dialog (GTK_WINDOW (window),
							_("mail_in_ui_folder_delete_error"),
							FALSE);
		g_object_unref (G_OBJECT (folder));
		return FALSE;
	}

	/* Ask the user */
	message =  g_strdup_printf (_("mcen_nc_delete_folder_text"),
				    tny_folder_get_name (TNY_FOLDER (folder)));
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (window),
							    (const gchar *) message);
	g_free (message);

	if (response == GTK_RESPONSE_OK) {
		DeleteFolderInfo *info;
		info = g_new0(DeleteFolderInfo, 1);
		info->folder = folder;
		info->move_to_trash = move_to_trash;
		g_object_ref (G_OBJECT (info->folder));
		TnyAccount *account = tny_folder_get_account (TNY_FOLDER (folder));
		modest_platform_connect_if_remote_and_perform (GTK_WINDOW (window),
							       TRUE,
							       TNY_FOLDER_STORE (account),
							       on_delete_folder_cb, info);
		g_object_unref (account);
		return TRUE;
	} else {
		return FALSE;
	}
	g_object_unref (G_OBJECT (folder));
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

void
modest_ui_actions_on_move_folder_to_trash_folder (GtkAction *action, ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	delete_folder (MODEST_WINDOW (main_window), TRUE);
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

#ifdef MODEST_TOOLKIT_HILDON2
	value = hildon_entry_get_text (HILDON_ENTRY (fields->username));
#else
	value = gtk_entry_get_text (GTK_ENTRY (fields->username));
#endif
	if ((value == NULL) || value[0] == '\0') {
		any_value_empty = TRUE;
	}
#ifdef MODEST_TOOLKIT_HILDON2
	value = hildon_entry_get_text (HILDON_ENTRY (fields->password));
#else
	value = gtk_entry_get_text (GTK_ENTRY (fields->password));
#endif
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
					 ModestMainWindow *main_window)
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
					     _HL("wdgt_bd_done"),
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

#ifdef MODEST_TOOLKIT_HILDON2
	GtkWidget *entry_username = hildon_entry_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
	if (initial_username)
		hildon_entry_set_text (HILDON_ENTRY (entry_username), initial_username);
#else
	GtkWidget *entry_username = gtk_entry_new ();
	if (initial_username)
		gtk_entry_set_text (GTK_ENTRY (entry_username), initial_username);
#endif
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

#ifndef MODEST_TOOLKIT_GTK
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (entry_username), HILDON_GTK_INPUT_MODE_FULL);

	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

#ifdef MODEST_TOOLKIT_HILDON2
	GtkWidget *caption = modest_maemo_utils_create_captioned (sizegroup, NULL,
								  _("mail_fi_username"), FALSE,
								  entry_username);
#else
	GtkWidget *caption = hildon_caption_new (sizegroup,
		_("mail_fi_username"), entry_username, NULL, HILDON_CAPTION_MANDATORY);
#endif
	gtk_widget_show (entry_username);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), caption,
		FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
#else
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), entry_username,
			    TRUE, FALSE, 0);
#endif /* !MODEST_TOOLKIT_GTK */

	/* password: */
#ifdef MODEST_TOOLKIT_HILDON2
	GtkWidget *entry_password = hildon_entry_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
#else
	GtkWidget *entry_password = gtk_entry_new ();
#endif
	gtk_entry_set_visibility (GTK_ENTRY(entry_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY(entry_password), "*"); */

#ifndef MODEST_TOOLKIT_GTK
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (entry_password),
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);

#ifdef MODEST_TOOLKIT_HILDON2
	caption = modest_maemo_utils_create_captioned (sizegroup, NULL,
						       _("mail_fi_password"), FALSE,
						       entry_password);
#else
	caption = hildon_caption_new (sizegroup,
		_("mail_fi_password"), entry_password, NULL, HILDON_CAPTION_MANDATORY);
#endif
	gtk_widget_show (entry_password);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), caption,
		FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	g_object_unref (sizegroup);
#else
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), entry_password,
			    TRUE, FALSE, 0);
#endif /* !MODEST_TOOLKIT_GTK */

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
#ifdef MODEST_TOOLKIT_HILDON2
				*username = g_strdup (hildon_entry_get_text (HILDON_ENTRY(entry_username)));
#else
				*username = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry_username)));
#endif

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
#ifdef MODEST_TOOLKIT_HILDON2
				*password = g_strdup (hildon_entry_get_text (HILDON_ENTRY(entry_password)));
#else
				*password = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry_password)));
#endif

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
#ifndef MODEST_TOOLKIT_HILDON2
			/* Set parent to NULL or the banner will disappear with its parent dialog */
			modest_platform_information_banner(NULL, NULL, _("mail_ib_login_cancelled"));
#endif
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
	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));
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
	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));

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
					NULL, NULL, _CS("mcen_ib_getting_items"));
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
				NULL, NULL, _CS("ecoc_ib_edwin_copied"));
}

void
modest_ui_actions_on_undo (GtkAction *action,
			   ModestWindow *window)
{
	ModestEmailClipboard *clipboard = NULL;

	if (MODEST_IS_MSG_EDIT_WINDOW (window)) {
		modest_msg_edit_window_undo (MODEST_MSG_EDIT_WINDOW (window));
	} else if (MODEST_IS_MAIN_WINDOW (window)) {
		/* Clear clipboard source */
		clipboard = modest_runtime_get_email_clipboard ();
		modest_email_clipboard_clear (clipboard);
	}
	else {
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

	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));
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
									   _CS("ckct_nw_pasting"));
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
									     _CS("ckct_nw_pasting"));
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
								     _CS("ckct_nw_pasting"));
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

	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));
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
	} else if (MODEST_IS_MAIN_WINDOW (window)) {
		GtkWidget *header_view = focused_widget;
 		GtkTreeSelection *selection = NULL;

		if (!(MODEST_IS_HEADER_VIEW (focused_widget))) {
			header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
									   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		}

		/* Disable window dimming management */
		modest_window_disable_dimming (MODEST_WINDOW(window));

		/* Select all messages */
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(header_view));
		gtk_tree_selection_select_all (selection);

		/* Set focuse on header view */
		gtk_widget_grab_focus (header_view);

		/* Enable window dimming management */
		modest_window_enable_dimming (MODEST_WINDOW(window));
		modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
		modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
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
		gtk_window_present (GTK_WINDOW (window));
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

#ifndef MODEST_TOOLKIT_HILDON2
	gtk_window_present (GTK_WINDOW (window));
#endif
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
	TnyMsg *msg = NULL;

	if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		async_retrieval = TRUE;
		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (window));
	} else {
		async_retrieval = FALSE;
	}
	modest_platform_run_header_details_dialog (GTK_WINDOW (window), header, async_retrieval, msg);
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

	} else if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *folder_view, *header_view;

		/* Check which widget has the focus */
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
								    MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		if (gtk_widget_is_focus (folder_view)) {
			TnyFolderStore *folder_store
				= modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));
			if (!folder_store) {
				g_warning ("%s: No item was selected.\n", __FUNCTION__);
				return;
			}
			/* Show only when it's a folder */
			/* This function should not be called for account items,
			 * because we dim the menu item for them. */
			if (TNY_IS_FOLDER (folder_store)) {
				modest_platform_run_folder_details_dialog (GTK_WINDOW (win),
									   TNY_FOLDER (folder_store));
			}

			g_object_unref (folder_store);

		} else {
			header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
									   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
			/* Show details of each header */
			do_headers_action (win, headers_action_show_details, header_view);
		}
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_HEADER_WINDOW (win)) {
		TnyFolder *folder;
		GtkWidget *header_view;

		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (win)));
		folder = modest_header_view_get_folder (MODEST_HEADER_VIEW (header_view));
		if (folder) {
			modest_platform_run_folder_details_dialog (GTK_WINDOW (win),
								   folder);
			g_object_unref (folder);
		}
#endif
	}
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
modest_ui_actions_toggle_folders_view (GtkAction *action,
				       ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
		modest_main_window_set_style (main_window, MODEST_MAIN_WINDOW_STYLE_SPLIT);
	else
		modest_main_window_set_style (main_window, MODEST_MAIN_WINDOW_STYLE_SIMPLE);
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

#ifndef MODEST_TOOLKIT_HILDON2
/*
 * This function is used to track changes in the selection of the
 * folder view that is inside the "move to" dialog to enable/disable
 * the OK button because we do not want the user to select a disallowed
 * destination for a folder.
 * The user also not desired to be able to use NEW button on items where
 * folder creation is not possibel.
 */
static void
on_move_to_dialog_folder_selection_changed (ModestFolderView* self,
					    TnyFolderStore *folder_store,
					    gboolean selected,
					    gpointer user_data)
{
	GtkWidget *dialog = NULL;
	gboolean ok_sensitive = TRUE, new_sensitive = TRUE;
	gboolean moving_folder = FALSE;
	gboolean is_local_account = TRUE;
	GtkWidget *folder_view = NULL;
	ModestTnyFolderRules rules;

	g_return_if_fail (MODEST_IS_FOLDER_VIEW(self));

	if (!selected)
		return;

	dialog = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_DIALOG);
	if (!dialog)
		return;

	/* check if folder_store is an remote account */
	if (TNY_IS_ACCOUNT (folder_store)) {
		TnyAccount *local_account = NULL;
		TnyAccount *mmc_account = NULL;
		ModestTnyAccountStore *account_store = NULL;

		account_store = modest_runtime_get_account_store ();
		local_account = modest_tny_account_store_get_local_folders_account (account_store);
		mmc_account = modest_tny_account_store_get_mmc_folders_account (account_store);

		if ((gpointer) local_account != (gpointer) folder_store &&
		    (gpointer) mmc_account != (gpointer) folder_store) {
			ModestProtocolType proto;
			proto = modest_tny_account_get_protocol_type (TNY_ACCOUNT (folder_store));
			if (proto == MODEST_PROTOCOL_REGISTRY_TYPE_INVALID) {
				proto = MODEST_PROTOCOLS_STORE_MAILDIR;
			}
			is_local_account = FALSE;
			/* New button should be dimmed on remote
			   POP account root */
			new_sensitive = (modest_protocol_registry_protocol_type_has_tag (modest_runtime_get_protocol_registry (),
											 proto,
											 MODEST_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS));
		}
		g_object_unref (local_account);

		/* It could not exist */
		if (mmc_account)
			g_object_unref (mmc_account);
	}

	/* Check the target folder rules */
	if (TNY_IS_FOLDER (folder_store)) {
		rules = modest_tny_folder_get_rules (TNY_FOLDER (folder_store));
		if (rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE) {
			ok_sensitive = FALSE;
			new_sensitive = FALSE;
			goto end;
		}
	}

	/* Check if we're moving a folder */
	if (MODEST_IS_MAIN_WINDOW (user_data)) {
		/* Get the widgets */
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (user_data),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		if (gtk_widget_is_focus (folder_view))
			moving_folder = TRUE;
	}

	if (moving_folder) {
		TnyFolderStore *moved_folder = NULL, *parent = NULL;

		/* Get the folder to move */
		moved_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

		/* Check that we're not moving to the same folder */
		if (TNY_IS_FOLDER (moved_folder)) {
			parent = tny_folder_get_folder_store (TNY_FOLDER (moved_folder));
			if (parent == folder_store)
				ok_sensitive = FALSE;
			g_object_unref (parent);
		}

		if (ok_sensitive && TNY_IS_ACCOUNT (folder_store)) {
			/* Do not allow to move to an account unless it's the
			   local folders account */
			if (!is_local_account)
				ok_sensitive = FALSE;
		}

		if (ok_sensitive && (moved_folder == folder_store)) {
			/* Do not allow to move to itself */
			ok_sensitive = FALSE;
		}
		g_object_unref (moved_folder);
	} else {
		TnyFolder *src_folder = NULL;

		/* Moving a message */
		if (MODEST_IS_MSG_VIEW_WINDOW (user_data)) {

			TnyHeader *header = NULL;
			header = modest_msg_view_window_get_header
				(MODEST_MSG_VIEW_WINDOW (user_data));
			if (!TNY_IS_HEADER(header))
				g_warning ("%s: could not get source header", __FUNCTION__);
			else
				src_folder = tny_header_get_folder (header);

			if (header)
				g_object_unref (header);
		} else {
			src_folder =
				TNY_FOLDER (modest_folder_view_get_selected
					    (MODEST_FOLDER_VIEW (folder_view)));
		}

		if (TNY_IS_FOLDER(src_folder)) {
			/* Do not allow to move the msg to the same folder */
			/* Do not allow to move the msg to an account */
			if ((gpointer) src_folder == (gpointer) folder_store ||
			    TNY_IS_ACCOUNT (folder_store))
				ok_sensitive = FALSE;
			g_object_unref (src_folder);
		} else
			g_warning ("%s: could not get source folder", __FUNCTION__);
	}

 end:
	/* Set sensitivity of the OK and NEW button */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, ok_sensitive);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), MODEST_GTK_RESPONSE_NEW_FOLDER, new_sensitive);
}
#endif

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
		modest_ui_actions_create_folder (GTK_WIDGET (dialog), GTK_WIDGET (folder_view), selected);
		g_object_unref (selected);
		return;
	case GTK_RESPONSE_NONE:
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_DELETE_EVENT:
		break;
	case GTK_RESPONSE_OK:
		dst_folder = modest_folder_view_get_selected (folder_view);

		if (MODEST_IS_MAIN_WINDOW (parent_win)) {
			/* Clean list to move used for filtering */
			modest_folder_view_set_list_to_move (folder_view, NULL);

			modest_ui_actions_on_main_window_move_to (NULL,
								  GTK_WIDGET (folder_view),
								  dst_folder,
								  MODEST_MAIN_WINDOW (parent_win));
#ifdef MODEST_TOOLKIT_HILDON2
		} else if (MODEST_IS_FOLDER_WINDOW (parent_win)) {
			/* Clean list to move used for filtering */
			modest_folder_view_set_list_to_move (folder_view, NULL);

			modest_ui_actions_on_folder_window_move_to (GTK_WIDGET (folder_view),
								    dst_folder,
								    helper->list,
								    GTK_WINDOW (parent_win));
#endif
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

#ifndef MODEST_TOOLKIT_HILDON2
	/* Track changes in the selection to
	 * disable the OK button whenever "Move to" is not possible
	 * disbale NEW button whenever New is not possible */
	g_signal_connect (tree_view,
			  "folder_selection_changed",
			  G_CALLBACK (on_move_to_dialog_folder_selection_changed),
			  win);
#endif

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

		modest_folder_view_set_style (MODEST_FOLDER_VIEW (tree_view),
					      MODEST_FOLDER_VIEW_STYLE_SHOW_ALL);
		modest_folder_view_update_model (MODEST_FOLDER_VIEW (tree_view),
						 TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

		active_account_name = modest_window_get_active_account (MODEST_WINDOW (win));
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
#ifndef MODEST_TOOLKIT_HILDON2
	modest_folder_view_show_non_move_folders (MODEST_FOLDER_VIEW (tree_view), FALSE);
#endif
	if (list_to_move)
		modest_folder_view_set_list_to_move (MODEST_FOLDER_VIEW (tree_view), list_to_move);
#ifndef MODEST_TOOLKIT_HILDON2
	modest_folder_view_select_first_inbox_or_local (MODEST_FOLDER_VIEW (tree_view));
#endif

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

		if (!modest_msg_view_window_select_next_message (self) &&
		    !modest_msg_view_window_select_previous_message (self)) {
			/* No more messages to view, so close this window */
			modest_ui_actions_on_close_window (NULL, MODEST_WINDOW(self));
		}
	} else if (MODEST_IS_MAIN_WINDOW (object) &&
		   gtk_tree_row_reference_valid (helper->reference)) {
		GtkWidget *header_view;
		GtkTreePath *path;
		GtkTreeSelection *sel;

		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(object),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (header_view));
		path = gtk_tree_row_reference_get_path (helper->reference);
		/* We need to unselect the previous one
		   because we could be copying instead of
		   moving */
		gtk_tree_selection_unselect_all (sel);
		gtk_tree_selection_select_path (sel, path);
		gtk_tree_path_free (path);
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
	GtkWidget *folder_view;
	GObject *object;

	object = modest_mail_operation_get_source (mail_op);
	if (MODEST_IS_MAIN_WINDOW (object)) {
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(object),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		g_object_ref (folder_view);
		g_object_unref (object);
		move_to_cb (mail_op, user_data);
		modest_folder_view_select_folder (MODEST_FOLDER_VIEW (folder_view), new_folder, FALSE);
		g_object_unref (folder_view);
	} else {
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

#ifndef MODEST_TOOLKIT_HILDON2
	ModestWindow *main_window = NULL;

	/* Disable next automatic folder selection */
	main_window = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (),
							 FALSE); /* don't create */
	if (main_window) {
		GtkWidget *folder_view = NULL;

		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (main_window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		modest_folder_view_disable_next_folder_selection (MODEST_FOLDER_VIEW(folder_view));

		if (user_data && TNY_IS_FOLDER (user_data)) {
			modest_folder_view_select_folder (MODEST_FOLDER_VIEW (folder_view),
							  TNY_FOLDER (user_data), FALSE);
		}
	}
#endif
	/* Show notification dialog only if the main window exists */
	win = modest_mail_operation_get_source (mail_op);
	modest_platform_run_information_dialog ((GtkWindow *) win,
						_("mail_in_ui_folder_move_target_error"),
						FALSE);
	if (win)
		g_object_unref (win);
}

static void
open_msg_for_purge_cb (ModestMailOperation *mail_op,
		       TnyHeader *header,
		       gboolean canceled,
		       TnyMsg *msg,
		       GError *err,
		       gpointer user_data)
{
	TnyList *parts;
	TnyIterator *iter;
	gint pending_purges = 0;
	gboolean some_purged = FALSE;
	ModestWindow *win = MODEST_WINDOW (user_data);
	ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();

	/* If there was any error */
	if (!modest_ui_actions_msg_retrieval_check (mail_op, header, msg)) {
		modest_window_mgr_unregister_header (mgr, header);
		return;
	}

	/* Once the message has been retrieved for purging, we check if
	 * it's all ok for purging */

	parts = tny_simple_list_new ();
	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);
	iter = tny_list_create_iterator (parts);

	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part;
		part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (part && (tny_mime_part_is_attachment (part) || TNY_IS_MSG (part))) {
			if (tny_mime_part_is_purged (part))
				some_purged = TRUE;
			else
				pending_purges++;
		}

		if (part)
			g_object_unref (part);

		tny_iterator_next (iter);
	}
	g_object_unref (iter);


	if (pending_purges>0) {
		gint response;
		response = modest_platform_run_confirmation_dialog (GTK_WINDOW (win),_("mcen_nc_purge_file_text_inbox"));

		if (response == GTK_RESPONSE_OK) {
			GtkWidget *info;
			info =
				modest_platform_animation_banner (GTK_WIDGET (win), NULL, _("mcen_me_inbox_remove_attachments"));
			iter = tny_list_create_iterator (parts);
			while (!tny_iterator_is_done (iter)) {
				TnyMimePart *part;

				part = TNY_MIME_PART (tny_iterator_get_current (iter));
				if (part && (tny_mime_part_is_attachment (part) || TNY_IS_MSG (part)))
					tny_mime_part_set_purged (part);

				if (part)
					g_object_unref (part);

				tny_iterator_next (iter);
			}
			g_object_unref (iter);

			tny_msg_rewrite_cache (msg);

			gtk_widget_destroy (info);
		}
	}

	modest_window_mgr_unregister_header (mgr, header);

	g_object_unref (parts);
}

static void
modest_ui_actions_on_main_window_remove_attachments (GtkAction *action,
						     ModestMainWindow *win)
{
	GtkWidget *header_view;
	TnyList *header_list;
	TnyHeader *header;
	TnyHeaderFlags flags;
	ModestWindow *msg_view_window =  NULL;
	gboolean found;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (win));

	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

	header_list = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW (header_view));
	if (!header_list) {
		g_warning ("%s: no header selected", __FUNCTION__);
		return;
	}

	if (tny_list_get_length (header_list) == 1) {
		TnyIterator *iter = tny_list_create_iterator (header_list);
		header = TNY_HEADER (tny_iterator_get_current (iter));
		g_object_unref (iter);
	} else
		return;

	if (!header || !TNY_IS_HEADER(header)) {
		g_warning ("%s: header is not valid", __FUNCTION__);
		return;
	}

	found = modest_window_mgr_find_registered_header (modest_runtime_get_window_mgr (),
							  header, &msg_view_window);
	flags = tny_header_get_flags (header);
	if (!(flags & TNY_HEADER_FLAG_CACHED))
		return;
	if (found) {
		if (msg_view_window != NULL)
			modest_msg_view_window_remove_attachments (MODEST_MSG_VIEW_WINDOW (msg_view_window), TRUE);
		else {
			/* do nothing; uid was registered before, so window is probably on it's way */
			g_debug ("header %p has already been registered", header);
		}
	} else {
		ModestMailOperation *mail_op = NULL;
		modest_window_mgr_register_header (modest_runtime_get_window_mgr (), header, NULL);
		mail_op = modest_mail_operation_new_with_error_handling (G_OBJECT (win),
									 modest_ui_actions_disk_operations_error_handler,
									 NULL, NULL);
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
		modest_mail_operation_get_msg (mail_op, header, FALSE, open_msg_for_purge_cb, win);

		g_object_unref (mail_op);
	}
	if (header)
		g_object_unref (header);
	if (header_list)
		g_object_unref (header_list);
}

/*
 * Checks if we need a connection to do the transfer and if the user
 * wants to connect to complete it
 */
static void
modest_ui_actions_xfer_messages_check (GtkWindow *parent_window,
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

		*need_connection = TRUE;
		num_headers = tny_list_get_length (headers);
		msg = ngettext ("mcen_nc_get_msg", "mcen_nc_get_msgs", num_headers);

		if (modest_platform_run_confirmation_dialog (parent_window, msg) ==
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

	win = modest_mail_operation_get_source (mail_op);
	error = modest_mail_operation_get_error (mail_op);

	if (error && is_memory_full_error ((GError *) error, mail_op, NULL)) {
		gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
		modest_platform_information_banner ((GtkWidget *) win, NULL, msg);
		g_free (msg);
	} else {
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
			  GtkWindow *parent_window,
			  TnyAccount *account,
			  gpointer user_data)
{
	ModestWindow *win = MODEST_WINDOW (parent_window);
	TnyAccount *dst_account = NULL;
	gboolean dst_forbids_message_add = FALSE;
	XferMsgsHelper *helper;
	MoveToHelper *movehelper;
	ModestMailOperation *mail_op;

	helper = (XferMsgsHelper *) user_data;

	if (canceled || err) {
		if (!check_memory_full_error ((GtkWidget *) parent_window, err, NULL)) {
			/* Show the proper error message */
			modest_ui_actions_on_account_connection_error (parent_window, account);
		}
		goto end;
	}

	dst_account = tny_folder_get_account (TNY_FOLDER (helper->dst_folder));

	/* tinymail will return NULL for local folders it seems */
	dst_forbids_message_add = modest_protocol_registry_protocol_type_has_tag (modest_runtime_get_protocol_registry (),
										  modest_tny_account_get_protocol_type (dst_account),
										  MODEST_PROTOCOL_REGISTRY_STORE_FORBID_MESSAGE_ADD);
	g_object_unref (dst_account);

	if (dst_forbids_message_add) {
		modest_platform_information_banner (GTK_WIDGET (win),
						    NULL,
						    ngettext("mail_in_ui_folder_move_target_error",
							     "mail_in_ui_folder_move_targets_error",
							     tny_list_get_length (helper->headers)));
		goto end;
	}

	movehelper = g_new0 (MoveToHelper, 1);

#ifndef MODEST_TOOLKIT_HILDON2
	movehelper->banner = modest_platform_animation_banner (GTK_WIDGET (win), NULL,
							       _CS("ckct_nw_pasting"));
	if (movehelper->banner != NULL)  {
		g_object_ref (movehelper->banner);
		gtk_widget_show (GTK_WIDGET (movehelper->banner));
	}
#endif

	if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *header_view =
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		movehelper->reference = get_next_after_selected_headers (MODEST_HEADER_VIEW (header_view));
	}

	/* Perform the mail operation */
	mail_op = modest_mail_operation_new_with_error_handling (G_OBJECT(win),
								 xfer_messages_error_handler,
								 movehelper, NULL);
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
on_move_folder_cb (gboolean canceled, GError *err, GtkWindow *parent_window,
		TnyAccount *account, gpointer user_data)
{
	MoveFolderInfo *info = (MoveFolderInfo*)user_data;
	GtkTreeSelection *sel;
	ModestMailOperation *mail_op = NULL;

	if (canceled || err || !MODEST_IS_WINDOW (parent_window)) {
		g_object_unref (G_OBJECT (info->src_folder));
		g_object_unref (G_OBJECT (info->dst_folder));
		g_free (info);
		return;
	}

	MoveToHelper *helper = g_new0 (MoveToHelper, 1);
#ifndef MODEST_TOOLKIT_HILDON2
	helper->banner = modest_platform_animation_banner (GTK_WIDGET (parent_window), NULL,
			_CS("ckct_nw_pasting"));
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
				info->src_folder, NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
			mail_op);

	/* Select *after* the changes */
	/* TODO: this function hangs UI after transfer */
	/* 			modest_folder_view_select_folder (MODEST_FOLDER_VIEW(folder_view), */
	/* 							  TNY_FOLDER (src_folder), TRUE); */

	if (MODEST_IS_MAIN_WINDOW (parent_window)) {
		modest_folder_view_select_folder (MODEST_FOLDER_VIEW(info->folder_view),
						  TNY_FOLDER (info->dst_folder), TRUE);
	}
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
 * ModestMainWindow
 */
static void
modest_ui_actions_on_main_window_move_to (GtkAction *action,
					  GtkWidget *folder_view,
					  TnyFolderStore *dst_folder,
					  ModestMainWindow *win)
{
	ModestHeaderView *header_view = NULL;
	TnyFolderStore *src_folder = NULL;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (win));

	/* Get the source folder */
	src_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

	/* Get header view */
	header_view = (ModestHeaderView *)
		modest_main_window_get_child_widget (win, MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

	/* Get folder or messages to transfer */
	if (gtk_widget_is_focus (folder_view)) {
		gboolean do_xfer = TRUE;

		/* Allow only to transfer folders to the local root folder */
		if (TNY_IS_ACCOUNT (dst_folder) &&
		    !MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (dst_folder) &&
		    !modest_tny_account_is_memory_card_account (TNY_ACCOUNT (dst_folder))) {
			do_xfer = FALSE;
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

			modest_platform_double_connect_and_perform(GTK_WINDOW (win), TRUE,
								   TNY_FOLDER_STORE (src_folder),
								   connect_info);
		}
	} else if (gtk_widget_is_focus (GTK_WIDGET(header_view))) {
		TnyList *headers;

		headers = modest_header_view_get_selected_headers(header_view);

		/* Transfer the messages */
		modest_ui_actions_transfer_messages_helper (GTK_WINDOW (win), TNY_FOLDER (src_folder),
							    headers, TNY_FOLDER (dst_folder));

		g_object_unref (headers);
	}

	/* Frees */
	g_object_unref (src_folder);
}

#ifdef MODEST_TOOLKIT_HILDON2
/*
 * UI handler for the "Move to" action when invoked from the
 * ModestFolderWindow
 */
static void
modest_ui_actions_on_folder_window_move_to (GtkWidget *folder_view,
					    TnyFolderStore *dst_folder,
					    TnyList *selection,
					    GtkWindow *win)
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
		do_xfer = FALSE;
		/* Show an error */
		modest_platform_run_information_dialog (win,
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

		modest_platform_double_connect_and_perform(GTK_WINDOW (win), TRUE,
							   TNY_FOLDER_STORE (src_folder),
							   connect_info);
	}

	/* Frees */
	g_object_unref (src_folder);
}
#endif


void
modest_ui_actions_transfer_messages_helper (GtkWindow *win,
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

		modest_platform_double_connect_and_perform(GTK_WINDOW (win), TRUE,
							   TNY_FOLDER_STORE (src_folder),
							   connect_info);
	} else {
		TnyAccount *src_account = get_account_from_folder_store (TNY_FOLDER_STORE (src_folder));
		xfer_messages_performer (FALSE, NULL, GTK_WINDOW (win),
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
		modest_ui_actions_transfer_messages_helper (GTK_WINDOW (win), src_folder,
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
	MoveToInfo *helper = NULL;
	TnyList *list_to_move;

	g_return_val_if_fail (MODEST_IS_WINDOW (win), FALSE);

#ifndef MODEST_TOOLKIT_HILDON2
	/* Get the main window if exists */
	ModestMainWindow *main_window;
	if (MODEST_IS_MAIN_WINDOW (win))
		main_window = MODEST_MAIN_WINDOW (win);
	else
		main_window =
			MODEST_MAIN_WINDOW (modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (),
									       FALSE)); /* don't create */
#endif

	list_to_move = modest_platform_get_list_to_move (MODEST_WINDOW (win));

	if (!list_to_move)
		return FALSE;

	if (tny_list_get_length (list_to_move) < 1) {
		g_object_unref (list_to_move);
		return FALSE;
	}

	/* Create and run the dialog */
	dialog = create_move_to_dialog (GTK_WINDOW (win), NULL, list_to_move);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (dialog),
				     (GtkWindow *) win);

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
	if (MODEST_IS_MAIN_WINDOW (window)) {
		modest_ui_actions_on_main_window_remove_attachments (action, MODEST_MAIN_WINDOW (window));
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
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

	dialog = modest_platform_get_global_settings_dialog ();
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (win));
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_widget_show_all (dialog);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
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
                modest_platform_show_help (GTK_WINDOW (win), help_id);
#endif
}

void
modest_ui_actions_on_csm_help (GtkAction *action,
			       GtkWindow *win)
{
	/* Help app is not available at all in fremantle */
#ifndef MODEST_TOOLKIT_HILDON2

	const gchar* help_id = NULL;
	GtkWidget *folder_view;
	TnyFolderStore *folder_store;

	g_return_if_fail (win && MODEST_IS_MAIN_WINDOW (win));

	/* Get selected folder */
	folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	folder_store = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

	/* Switch help_id */
	if (folder_store && TNY_IS_FOLDER (folder_store))
		help_id = modest_tny_folder_get_help_id (TNY_FOLDER (folder_store));

	if (folder_store)
		g_object_unref (folder_store);

        if (help_id)
                modest_platform_show_help (GTK_WINDOW (win), help_id);
	else
		modest_ui_actions_on_help (action, win);
#endif
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
				 GtkWindow *parent_window,
				 TnyAccount *account,
				 gpointer user_data)
{
	ModestMailOperation *mail_op;
	TnyList *headers = TNY_LIST (user_data);

	if (err || canceled) {
		check_memory_full_error ((GtkWidget *) parent_window, err, NULL);
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
	modest_platform_connect_and_perform ((GtkWindow *) window, TRUE,
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
	g_return_if_fail (MODEST_IS_WINDOW (window));

	/* we check for low-mem; in that case, show a warning, and don't allow
	 * searching
	 */
	if (modest_platform_check_memory_low (window, TRUE))
		return;

	modest_platform_show_search_messages (GTK_WINDOW (window));
}

void
modest_ui_actions_on_open_addressbook (GtkAction *action, ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_WINDOW (win));


	/* we check for low-mem; in that case, show a warning, and don't allow
	 * for the addressbook
	 */
	if (modest_platform_check_memory_low (win, TRUE))
		return;


	modest_platform_show_addressbook (GTK_WINDOW (win));
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

	modest_msg_edit_window_toggle_find_toolbar (MODEST_MSG_EDIT_WINDOW (window),
						    active);
}

static void
on_send_receive_finished (ModestMailOperation  *mail_op,
			   gpointer user_data)
{
	GtkWidget *header_view, *folder_view;
	TnyFolderStore *folder_store;
	ModestMainWindow *main_win = MODEST_MAIN_WINDOW (user_data);

	/* Set send/receive operation finished */
	modest_main_window_notify_send_receive_completed (main_win);

	/* Don't refresh the current folder if there were any errors */
	if (modest_mail_operation_get_status (mail_op) !=
	    MODEST_MAIL_OPERATION_STATUS_SUCCESS)
		return;

	/* Refresh the current folder if we're viewing a window. We do
	   this because the user won't be able to see the new mails in
	   the selected folder after a Send&Receive because it only
	   performs a poke_status, i.e, only the number of read/unread
	   messages is updated, but the new headers are not
	   downloaded */
	folder_view = modest_main_window_get_child_widget (main_win,
							   MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
	if (!folder_view)
		return;

	folder_store = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

	/* Do not need to refresh INBOX again because the
	   update_account does it always automatically */
	if (folder_store && TNY_IS_FOLDER (folder_store) &&
	    tny_folder_get_folder_type (TNY_FOLDER (folder_store)) != TNY_FOLDER_TYPE_INBOX) {
		ModestMailOperation *refresh_op;

		header_view = modest_main_window_get_child_widget (main_win,
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

		/* We do not need to set the contents style
		   because it hasn't changed. We also do not
		   need to save the widget status. Just force
		   a refresh */
		refresh_op = modest_mail_operation_new (G_OBJECT (main_win));
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), refresh_op);
		modest_mail_operation_refresh_folder (refresh_op, TNY_FOLDER (folder_store),
						      folder_refreshed_cb, main_win);
		g_object_unref (refresh_op);
	}

	if (folder_store)
		g_object_unref (folder_store);
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
		message = g_strdup (_CS("sfil_ib_unable_to_send"));
		break;
	case TNY_SERVICE_ERROR_UNAVAILABLE:
		message = modest_protocol_get_translation (protocol,
							   MODEST_PROTOCOL_TRANSLATION_CONNECT_ERROR,
							   server_name);
		break;
	default:
		g_warning ("%s: unexpected ERROR %d",
			   __FUNCTION__, err->code);
		message = g_strdup (_CS("sfil_ib_unable_to_send"));
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

#ifndef MODEST_TOOLKIT_HILDON2
	if (MODEST_IS_MAIN_WINDOW (top_window)) {
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (top_window),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
	}
#else
	if (MODEST_IS_HEADER_WINDOW (top_window)) {
		header_view = (GtkWidget *)
			modest_header_window_get_header_view (MODEST_HEADER_WINDOW (top_window));
	}
#endif

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
modest_ui_actions_on_account_connection_error (GtkWindow *parent_window,
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
		modest_platform_run_information_dialog (parent_window, error_note, FALSE);
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

	if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *header_view;
		TnyList* headers = NULL;
		TnyIterator *iter;
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
								   MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);
		headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW (header_view));
		if (!headers || tny_list_get_length (headers) == 0) {
			if (headers)
				g_object_unref (headers);
			return NULL;
		}
		iter = tny_list_create_iterator (headers);
		header = TNY_HEADER (tny_iterator_get_current (iter));
		folder = TNY_FOLDER_STORE (tny_header_get_folder (header));
		g_object_unref (iter);
		g_object_unref (headers);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_HEADER_WINDOW (win)) {
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
#endif
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
		if (!removed)
			g_warning ("%s: modest_account_mgr_remove_account() failed.\n", __FUNCTION__);
	}
	return removed;
}

static void
on_fetch_images_performer (gboolean canceled,
			   GError *err,
			   GtkWindow *parent_window,
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

	modest_platform_connect_and_perform ((GtkWindow *) window, TRUE, 
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

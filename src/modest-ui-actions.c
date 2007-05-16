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
#include <string.h>
#include <modest-runtime.h>
#include <modest-tny-folder.h>
#include <modest-tny-msg.h>
#include <modest-tny-account.h>
#include <modest-address-book.h>

#include "modest-ui-actions.h"

#include "modest-tny-platform-factory.h"
#include "modest-platform.h"

#ifdef MODEST_PLATFORM_MAEMO
#include "maemo/modest-osso-state-saving.h"
#endif /* MODEST_PLATFORM_MAEMO */

#include "widgets/modest-ui-constants.h"
#include <widgets/modest-main-window.h>
#include <widgets/modest-msg-view-window.h>
#include <widgets/modest-account-view-window.h>
#include <widgets/modest-details-dialog.h>
#include <widgets/modest-attachments-view.h>
#include "widgets/modest-global-settings-dialog.h"
#include "modest-account-mgr-helpers.h"
#include "modest-mail-operation.h"
#include "modest-text-utils.h"

#ifdef MODEST_HAVE_EASYSETUP
#include "easysetup/modest-easysetup-wizard.h"
#endif /* MODEST_HAVE_EASYSETUP */

#include <modest-widget-memory.h>
#include <tny-error.h>
#include <tny-simple-list.h>
#include <tny-msg-view.h>
#include <tny-device.h>

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
	guint pending_ops;
} ReplyForwardHelper;

typedef struct _HeaderActivatedHelper {
	GtkTreeModel *model;
	GtkTreeRowReference *row_reference;
	TnyFolder *folder;
	TnyHeader *header;
} HeaderActivatedHelper;

/*
 * The do_headers_action uses this kind of functions to perform some
 * action to each member of a list of headers
 */
typedef void (*HeadersFunc) (TnyHeader *header, ModestWindow *win, gpointer user_data);

static void
do_headers_action (ModestWindow *win, 
		   HeadersFunc func,
		   gpointer user_data);


static void     open_msg_func          (const GObject *obj, const TnyMsg *msg, gpointer user_data);

static void     reply_forward_func     (const GObject *obj, const TnyMsg *msg, gpointer user_data);

static void     reply_forward          (ReplyForwardAction action, ModestWindow *win);

static gchar*   ask_for_folder_name    (GtkWindow *parent_window, const gchar *title);

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
	
	gtk_dialog_run (GTK_DIALOG (about));
	gtk_widget_destroy(about);
}


static TnyList *
get_selected_headers (ModestWindow *win)
{
	if (MODEST_IS_MAIN_WINDOW(win)) {
		GtkWidget *header_view;		
		
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(win),
								   MODEST_WIDGET_TYPE_HEADER_VIEW);
		return modest_header_view_get_selected_headers (MODEST_HEADER_VIEW(header_view));
		
	} else if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		/* for MsgViewWindows, we simply return a list with one element */
		TnyMsg *msg;
		TnyHeader *header;
		TnyList *list = NULL;
		
		msg  = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW(win));
		if (msg) {
			header = tny_msg_get_header (msg);
			list = tny_simple_list_new ();
			tny_list_prepend (list, G_OBJECT(header));
			g_object_unref (G_OBJECT(header));
		}
		return list;

	} else
		return NULL;
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
	tny_header_set_flags (header, TNY_HEADER_FLAG_SEEN);
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
		tny_header_unset_flags (header, TNY_HEADER_FLAG_SEEN);
	}
}


static void
headers_action_delete (TnyHeader *header,
		       ModestWindow *win,
		       gpointer user_data)
{
	ModestMailOperation *mail_op;

	/* TODO: add confirmation dialog */
	mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_DELETE, G_OBJECT(win));
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);

	/* Always delete. TODO: Move to trash still not supported */
	modest_mail_operation_remove_msg (mail_op, header, FALSE);
	g_object_unref (G_OBJECT (mail_op));
}

void
modest_ui_actions_on_delete (GtkAction *action, ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_WINDOW(win));

	if (MODEST_IS_MSG_EDIT_WINDOW (win)) {
		gboolean ret_value;
		g_signal_emit_by_name (G_OBJECT (win), "delete-event", NULL, &ret_value);
		return;
	}
		
	/* Remove each header */
	do_headers_action (win, headers_action_delete, NULL);

	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		gtk_widget_destroy (GTK_WIDGET(win));
	} 
}


void
modest_ui_actions_on_quit (GtkAction *action, ModestWindow *win)
{
	#ifdef MODEST_PLATFORM_MAEMO
	modest_osso_save_state();
	#endif /* MODEST_PLATFORM_MAEMO */
	
	gtk_main_quit ();
}

void
modest_ui_actions_on_close_window (GtkAction *action, ModestWindow *win)
{
	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		gtk_widget_destroy (GTK_WIDGET (win));
	} else if (MODEST_IS_MSG_EDIT_WINDOW (win)) {
		gboolean ret_value;
		g_signal_emit_by_name (G_OBJECT (win), "delete-event", NULL, &ret_value);
	} else if (MODEST_IS_WINDOW (win)) {
		gtk_widget_destroy (GTK_WIDGET (win));
	} else {
		g_return_if_reached ();
	}
}

void
modest_ui_actions_on_add_to_contacts (GtkAction *action, ModestWindow *win)
{
	GtkClipboard *clipboard = NULL;
	gchar *selection = NULL;

	clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	selection = gtk_clipboard_wait_for_text (clipboard);

	modest_address_book_add_address (selection);
	g_free (selection);
}

void
modest_ui_actions_on_accounts (GtkAction *action, ModestWindow *win)
{
	
	/* This is currently only implemented for Maemo,
	 * because it requires a providers preset file which is not publically available.
	 */
#ifdef MODEST_PLATFORM_MAEMO /* Defined in config.h */
	GSList *account_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr(), 
				TRUE /* enabled accounts only */);
	gboolean accounts_exist = account_names != NULL;
	g_slist_free (account_names);
	
	if (!accounts_exist) {
		/* If there are no accounts yet, just show the easy-setup wizard, as per the UI spec: */
		ModestEasysetupWizardDialog *wizard = modest_easysetup_wizard_dialog_new ();
		gtk_window_set_transient_for (GTK_WINDOW (wizard), GTK_WINDOW (win));
		gtk_dialog_run (GTK_DIALOG (wizard));
		gtk_widget_destroy (GTK_WIDGET (wizard));
	} else 	{
		/* Show the list of accounts: */
		GtkDialog *account_win = GTK_DIALOG(modest_account_view_window_new ());
		gtk_window_set_transient_for (GTK_WINDOW (account_win), GTK_WINDOW(win));
		gtk_dialog_run (account_win);
		gtk_widget_destroy (GTK_WIDGET(account_win));
	}
#else
	GtkWidget *dialog, *label;
	
	/* Create the widgets */
	
	dialog = gtk_dialog_new_with_buttons ("Message",
					      GTK_WINDOW(win),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_NONE,
					      NULL);
	label = gtk_label_new ("Hello World!");
	
	/* Ensure that the dialog box is destroyed when the user responds. */
	
	g_signal_connect_swapped (dialog, "response", 
				  G_CALLBACK (gtk_widget_destroy),
				  dialog);
	
	/* Add the label, and show everything we've added to the dialog. */
	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
			   label);
	gtk_widget_show_all (dialog);
#endif /* MODEST_PLATFORM_MAEMO */
}

void
modest_ui_actions_on_new_msg (GtkAction *action, ModestWindow *win)
{
	ModestWindow *msg_win;
	TnyMsg *msg = NULL;
	TnyFolder *folder = NULL;
	gchar *account_name = NULL;
	gchar *from_str = NULL;
/* 	GError *err = NULL; */
	TnyAccount *account = NULL;
	ModestWindowMgr *mgr;
	gchar *signature = NULL;
	
	account_name = g_strdup(modest_window_get_active_account (win));
	if (!account_name)
		account_name = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());
	if (!account_name) {
		g_printerr ("modest: no account found\n");
		goto cleanup;
	}
	
	account = modest_tny_account_store_get_tny_account_by_account (modest_runtime_get_account_store(),
								       account_name,
								       TNY_ACCOUNT_TYPE_STORE);
	if (!account) {
		g_printerr ("modest: failed to get tnyaccount for '%s'\n", account_name);
		goto cleanup;
	}

	from_str = modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(), account_name);
	if (!from_str) {
		g_printerr ("modest: failed get from string for '%s'\n", account_name);
		goto cleanup;
	}

	if (modest_account_mgr_get_bool (modest_runtime_get_account_mgr (), account_name,
					 MODEST_ACCOUNT_USE_SIGNATURE, FALSE)) {
		signature = modest_account_mgr_get_string (modest_runtime_get_account_mgr (), account_name,
							   MODEST_ACCOUNT_SIGNATURE, FALSE);
	} else {
		signature = g_strdup ("");
	}

	msg = modest_tny_msg_new ("", from_str, "", "", "", signature, NULL);
	if (!msg) {
		g_printerr ("modest: failed to create new msg\n");
		goto cleanup;
	}
	
	folder = modest_tny_account_get_special_folder (account, TNY_FOLDER_TYPE_DRAFTS);
	if (!folder) {
		g_printerr ("modest: failed to find Drafts folder\n");
		goto cleanup;
	}
	
/* 	tny_folder_add_msg (folder, msg, &err); */
/* 	if (err) { */
/* 		g_printerr ("modest: error adding msg to Drafts folder: %s", */
/* 			    err->message); */
/* 		g_error_free (err); */
/* 		goto cleanup; */
/* 	} */

	/* Create and register edit window */
	msg_win = modest_msg_edit_window_new (msg, account_name);
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_register_window (mgr, msg_win);

	if (win)
		gtk_window_set_transient_for (GTK_WINDOW (msg_win),
					      GTK_WINDOW (win));	
	gtk_widget_show_all (GTK_WIDGET (msg_win));

cleanup:
	g_free (account_name);
	g_free (from_str);
	g_free (signature);
	if (account)
		g_object_unref (G_OBJECT(account));
	if (msg)
		g_object_unref (G_OBJECT(msg));
	if (folder)
		g_object_unref (G_OBJECT(folder));
}

static void
headers_action_open (TnyHeader *header, 
		     ModestWindow *win, 
		     gpointer user_data)
{
	modest_ui_actions_on_header_activated (MODEST_HEADER_VIEW (user_data), 
					       header,
					       MODEST_MAIN_WINDOW (win));
}

void
modest_ui_actions_on_open (GtkAction *action, ModestWindow *win)
{
	GtkWidget *header_view;

	/* Get header view */
	header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							   MODEST_WIDGET_TYPE_HEADER_VIEW);

	/* Open each message */
	do_headers_action (win, headers_action_open, header_view);
}


static void
open_msg_func (const GObject *obj, const TnyMsg *msg, gpointer user_data)
{
	ModestWindowMgr *mgr = NULL;
	ModestWindow *parent_win = NULL;
	ModestWindow *win = NULL;
	HeaderActivatedHelper *helper = NULL;
	TnyFolderType folder_type = TNY_FOLDER_TYPE_UNKNOWN;
	gchar *account = NULL;
	
	g_return_if_fail (MODEST_IS_WINDOW(obj));
	g_return_if_fail (user_data != NULL);

	/* TODO: Show an error? (review the specs) */
	if (!msg)
		return;

	parent_win = MODEST_WINDOW(obj);
	helper = (HeaderActivatedHelper *) user_data;

	/* Mark header as read */
	headers_action_mark_as_read (helper->header, MODEST_WINDOW(parent_win), NULL);

	/* Get account */
	account =  g_strdup(modest_window_get_active_account(MODEST_WINDOW(parent_win)));
	if (!account)
		account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());
	
	/* Gets foldert type (OUTBOX headers will be opened in edit window */
	if (modest_tny_folder_is_local_folder (helper->folder))
		folder_type = modest_tny_folder_get_local_folder_type (helper->folder);

	switch (folder_type) {
	case TNY_FOLDER_TYPE_DRAFTS:
		win = modest_msg_edit_window_new ((TnyMsg *) msg, account);
		break;
	default:
		if (helper->model != NULL)
			win = modest_msg_view_window_new_with_header_model ((TnyMsg *) msg, account, helper->model, helper->row_reference);
		else
			win = modest_msg_view_window_new ((TnyMsg *) msg, account);
	}
	
	/* Register and show new window */
	if (win != NULL) {
		mgr = modest_runtime_get_window_mgr ();
		modest_window_mgr_register_window (mgr, win);
		gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (parent_win));
		gtk_widget_show_all (GTK_WIDGET(win));
	}

	/* Free */
	g_free(account);
/* 	g_object_unref (G_OBJECT(msg)); */
	g_object_unref (G_OBJECT(helper->folder));
	g_object_unref (G_OBJECT(helper->header));
	gtk_tree_row_reference_free (helper->row_reference);
	g_slice_free (HeaderActivatedHelper, helper);
}

static void
reply_forward_func (const GObject *obj, const TnyMsg *msg, gpointer user_data)
{
	TnyMsg *new_msg;
	ReplyForwardHelper *rf_helper;
	ModestWindow *msg_win;
	ModestEditType edit_type;
	gchar *from;
	GError *err = NULL;
	TnyFolder *folder = NULL;
	TnyAccount *account = NULL;
	ModestWindowMgr *mgr;
	gchar *signature = NULL;
			
	g_return_if_fail (user_data != NULL);
	rf_helper = (ReplyForwardHelper *) user_data;

	rf_helper->pending_ops--;

	from = modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
						   rf_helper->account_name);
	if (modest_account_mgr_get_bool (modest_runtime_get_account_mgr(),
					 rf_helper->account_name,
					 MODEST_ACCOUNT_USE_SIGNATURE, FALSE)) {
		signature = modest_account_mgr_get_string (modest_runtime_get_account_mgr (),
							   rf_helper->account_name,
							   MODEST_ACCOUNT_SIGNATURE, FALSE);
	}

	/* Create reply mail */
	switch (rf_helper->action) {
	case ACTION_REPLY:
		new_msg = 
			modest_tny_msg_create_reply_msg ((TnyMsg *) msg,  from, signature,
							 rf_helper->reply_forward_type,
							 MODEST_TNY_MSG_REPLY_MODE_SENDER);
		break;
	case ACTION_REPLY_TO_ALL:
		new_msg = 
			modest_tny_msg_create_reply_msg ((TnyMsg *) msg, from, signature, rf_helper->reply_forward_type,
							 MODEST_TNY_MSG_REPLY_MODE_ALL);
		edit_type = MODEST_EDIT_TYPE_REPLY;
		break;
	case ACTION_FORWARD:
		new_msg = 
			modest_tny_msg_create_forward_msg ((TnyMsg *) msg, from, signature, rf_helper->reply_forward_type);
		edit_type = MODEST_EDIT_TYPE_FORWARD;
		break;
	default:
		g_return_if_reached ();
		return;
	}

	g_free (signature);

	if (!new_msg) {
		g_printerr ("modest: failed to create message\n");
		goto cleanup;
	}

	account = modest_tny_account_store_get_tny_account_by_account (modest_runtime_get_account_store(),
								       rf_helper->account_name,
								       TNY_ACCOUNT_TYPE_STORE);
	if (!account) {
		g_printerr ("modest: failed to get tnyaccount for '%s'\n", rf_helper->account_name);
		goto cleanup;
	}

	folder = modest_tny_account_get_special_folder (account, TNY_FOLDER_TYPE_DRAFTS);
	if (!folder) {
		g_printerr ("modest: failed to find Drafts folder\n");
		goto cleanup;
	}
	
	tny_folder_add_msg (folder, (TnyMsg *) msg, &err);
	if (err) {
		g_printerr ("modest: error adding msg to Drafts folder: %s",
			    err->message);
		g_error_free (err);
		goto cleanup;
	}	

	/* Create and register the windows */			
	msg_win = modest_msg_edit_window_new (new_msg, rf_helper->account_name);
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_register_window (mgr, msg_win);

	/* Show edit window */
	gtk_widget_show_all (GTK_WIDGET (msg_win));

cleanup:
	if (new_msg)
		g_object_unref (G_OBJECT (new_msg));
	if (folder)
		g_object_unref (G_OBJECT (folder));
	if (account)
		g_object_unref (G_OBJECT (account));
	
	if (rf_helper->pending_ops == 0) {
		g_free (rf_helper->account_name);
		g_slice_free (ReplyForwardHelper, rf_helper);
	}
}
/*
 * Common code for the reply and forward actions
 */
static void
reply_forward (ReplyForwardAction action, ModestWindow *win)
{
	ModestMailOperation *mail_op = NULL;
	TnyList *header_list = NULL;
	ReplyForwardHelper *rf_helper = NULL;
	guint reply_forward_type;
	
	g_return_if_fail (MODEST_IS_WINDOW(win));

	header_list = get_selected_headers (win);
	if (!header_list)
		return;
	
	reply_forward_type = modest_conf_get_int (modest_runtime_get_conf (),
						  (action == ACTION_FORWARD) ? MODEST_CONF_FORWARD_TYPE : MODEST_CONF_REPLY_TYPE,
						  NULL);
	/* We assume that we can only select messages of the
	   same folder and that we reply all of them from the
	   same account. In fact the interface currently only
	   allows single selection */
	
	/* Fill helpers */
	rf_helper = g_slice_new0 (ReplyForwardHelper);
	rf_helper->reply_forward_type = reply_forward_type;
	rf_helper->action = action;
	rf_helper->pending_ops = tny_list_get_length (header_list);
	rf_helper->account_name = g_strdup (modest_window_get_active_account (win));
	if (!rf_helper->account_name)
		rf_helper->account_name =
			modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());

	if (MODEST_IS_MSG_VIEW_WINDOW(win)) {
		TnyMsg *msg;
		msg = modest_msg_view_window_get_message(MODEST_MSG_VIEW_WINDOW(win));
		if (!msg) {
			g_printerr ("modest: no message found\n");
			return;
		} else
			reply_forward_func (G_OBJECT(win), msg, rf_helper);
	} else {
				
		mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_RECEIVE, G_OBJECT(win));
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
		modest_mail_operation_process_msg (mail_op, header_list, reply_forward_func, rf_helper);

		/* Clean */
		g_object_unref(mail_op);
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

		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(window),
								   MODEST_WIDGET_TYPE_HEADER_VIEW);
		if (!header_view)
			return;
	
		modest_header_view_select_next (MODEST_HEADER_VIEW(header_view)); 
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		modest_msg_view_window_select_next_message (MODEST_MSG_VIEW_WINDOW (window));
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
								   MODEST_WIDGET_TYPE_HEADER_VIEW);
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
	g_return_if_fail (MODEST_IS_WINDOW(window));

	if (MODEST_IS_MAIN_WINDOW (window)) {
		GtkWidget *header_view;
		header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(window),
								   MODEST_WIDGET_TYPE_HEADER_VIEW);
		if (!header_view)
			return;

		/* Show sorting dialog */
		modest_platform_run_sort_dialog (GTK_WINDOW (window), MODEST_SORT_HEADERS);	
	}
}


static gboolean
action_send (const gchar* account_name)
{
	TnyAccount *tny_account;
	ModestTnySendQueue *send_queue;

	g_return_val_if_fail (account_name, FALSE);

	/* Get the transport account according to the open connection, 
	 * because the account might specify connection-specific SMTP servers.
	 */
	tny_account = 
		modest_tny_account_store_get_transport_account_for_open_connection (modest_runtime_get_account_store(),
								     account_name);
	if (!tny_account) {
		g_printerr ("modest: cannot get tny transport account for %s\n", account_name);
		return FALSE;
	}
	
	send_queue = modest_tny_send_queue_new (TNY_CAMEL_TRANSPORT_ACCOUNT(tny_account));
	if (!send_queue) {
		g_object_unref (G_OBJECT(tny_account));
		g_printerr ("modest: cannot get send queue for %s\n", account_name);
		return FALSE;
	}
	
	modest_tny_send_queue_flush (send_queue);

	g_object_unref (G_OBJECT(send_queue));
	g_object_unref (G_OBJECT(tny_account));

	return TRUE;
}


static gboolean
action_receive (const gchar* account_name, 
		ModestWindow *win)
{
	ModestMailOperation *mail_op;

	g_return_val_if_fail (account_name, FALSE);

	/* Create the mail operation */
	/* TODO: The spec wants us to first do any pending deletions, before receiving. */
	mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_RECEIVE, G_OBJECT(win));
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	modest_mail_operation_update_account (mail_op, account_name);

	g_object_unref (G_OBJECT (mail_op));
		
	return TRUE;
}

/** Check that an appropriate connection is open.
 */
gboolean check_for_connection (const gchar *account_name)
{
	TnyDevice *device = modest_runtime_get_device ();

/*
	g_assert (TNY_IS_MAEMO_CONIC_DEVICE (device));
	
	TnyMaemoConicDevice *maemo_device = TNY_MAEMO_CONIC_DEVICE (device);
*/
	
	if (tny_device_is_online (device))
		return TRUE;
	else {
		modest_platform_connect_and_wait (NULL);
		
		/* TODO: Wait until a result. */
		return TRUE;
	}
}

/*
 * This function performs the send & receive required actions. The
 * window it's used to create the mail operation. Tipically it should
 * be allways the main window, but we pass it as argument in order to
 * be more flexible.
 */
void
modest_ui_actions_do_send_receive (const gchar *account_name, ModestWindow *win)
{
	gchar *acc_name = NULL;

	/* If no account name was provided get the current account, if
	   there is none either then pick the default one */
	if (!account_name) {
		acc_name = g_strdup (modest_window_get_active_account(win));
		if (!acc_name)
			acc_name  = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr());
		if (!acc_name) {
			g_printerr ("modest: cannot get default account\n");
			return;
		}
	} else {
		acc_name = g_strdup (account_name);
	}

	/* Send & receive. Do not continue if no suitable connection
	   is open */
	if (check_for_connection (acc_name)) {
		/* As per the UI spec,
		 * for POP accounts, we should receive,
		 * for IMAP we should synchronize everything, including receiving,
		 * for SMTP we should send,
		 * first receiving, then sending:
		 */
		if (!action_receive(acc_name, win))
			g_printerr ("modest: failed to receive\n");
		if (!action_send(acc_name))
			g_printerr ("modest: failed to send\n");
	}
	/* Free */
	g_free (acc_name);
}

/*
 * Refreshes all accounts. This function will be used by automatic
 * updates
 */
void
modest_ui_actions_do_send_receive_all (ModestWindow *win)
{
	GSList *account_names, *iter;

	account_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr(), 
							  TRUE);

	iter = account_names;
	while (iter) {			
		modest_ui_actions_do_send_receive ((const char*) iter->data, win);
		iter = g_slist_next (iter);
	}
	
	g_slist_foreach (account_names, (GFunc) g_free, NULL);
	g_slist_free (account_names);
}

/*
 * Handler of the click on Send&Receive button in the main toolbar
 */
void
modest_ui_actions_on_send_receive (GtkAction *action,  ModestWindow *win)
{
	/* Check that at least one account exists: */
	GSList *account_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr(),
				TRUE /* enabled accounts only */);
	gboolean accounts_exist = account_names != NULL;
	g_slist_free (account_names);
	
	/* If not, allow the user to create an account before trying to send/receive. */
	if (!accounts_exist)
		modest_ui_actions_on_accounts (NULL, win);
	
	/* Refresh the active account */
	modest_ui_actions_do_send_receive (NULL, win);
}


void
modest_ui_actions_toggle_header_list_view (GtkAction *action, ModestMainWindow *main_window)
{
	ModestConf *conf;
	GtkWidget *header_view;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	header_view = modest_main_window_get_child_widget (main_window,
							   MODEST_WIDGET_TYPE_HEADER_VIEW);
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

	/* If no header has been selected then exit */
	if (!header)
		return;

	/* Update Main window title */
	if (GTK_WIDGET_HAS_FOCUS (header_view)) {
		const gchar *subject = tny_header_get_subject (header);
		if (subject && strcmp (subject, ""))
			gtk_window_set_title (GTK_WINDOW (main_window), subject);
		else
			gtk_window_set_title (GTK_WINDOW (main_window), _("mail_va_no_subject"));
	}
}

void
modest_ui_actions_on_header_activated (ModestHeaderView *header_view,
				       TnyHeader *header,
				       ModestMainWindow *main_window)
{
	ModestMailOperation *mail_op = NULL;
	HeaderActivatedHelper *helper = NULL;
	ModestWindowMgr *mgr = NULL;
	ModestWindow *win = NULL;
	GtkTreeModel *model = NULL;
	GtkTreeSelection *sel = NULL;
	GList *sel_list = NULL;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	if (!header)
		return;

	/* Look if we already have a message view for that header */
	mgr = modest_runtime_get_window_mgr ();
	win = modest_window_mgr_find_window_by_msguid (mgr, tny_header_get_uid (header));
	if (win) return;

	/* Build helper */
	helper = g_slice_new0 (HeaderActivatedHelper);
	helper->folder = tny_header_get_folder (header);
	helper->header = g_object_ref(header);
	helper->model = NULL;

	/* Get headers tree model and selected row reference to build message view */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (header_view));
	sel_list = gtk_tree_selection_get_selected_rows (sel, &model);
	if (sel_list != NULL) {
		
		/* Fill helpers */
 		helper->model = model;
		helper->row_reference = gtk_tree_row_reference_new (model, (GtkTreePath *) sel_list->data);

		g_list_foreach (sel_list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (sel_list);
	}

	/* New mail operation */
	mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_RECEIVE, G_OBJECT(main_window));
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	modest_mail_operation_get_msg (mail_op, header, open_msg_func, helper);

	/* Free */
	g_object_unref (mail_op);
}

static void
set_active_account_from_tny_account (TnyAccount *account,
				     ModestWindow *window)
{
	TnyAccount *modest_server_account;
	const gchar *server_acc_name;
	gchar *modest_acc_name;

	server_acc_name = tny_account_get_id (account);
	/* We need the TnyAccount provided by the
	   account store because that is the one that
	   knows the name of the Modest account */
	modest_server_account = 
		modest_tny_account_store_get_tny_account_by_id  (modest_runtime_get_account_store (), 
								 server_acc_name);
	modest_acc_name = (gchar *) g_object_get_data (G_OBJECT (modest_server_account), "modest_account");
	modest_window_set_active_account (window, modest_acc_name);
	g_object_unref (modest_server_account);
}

void 
modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
					       TnyFolderStore *folder_store, 
					       gboolean selected,
					       ModestMainWindow *main_window)
{
	ModestConf *conf;
	GtkWidget *header_view;
	TnyAccount *account;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	header_view = modest_main_window_get_child_widget(main_window,
							  MODEST_WIDGET_TYPE_HEADER_VIEW);
	if (!header_view)
		return;
	
	conf = modest_runtime_get_conf ();

	if (TNY_IS_FOLDER (folder_store)) {

		if (selected) {
			/* Update the active account */
			account = tny_folder_get_account (TNY_FOLDER (folder_store));
			set_active_account_from_tny_account (account, MODEST_WINDOW (main_window));
			g_object_unref (account);

			/* Set folder on header view */
			modest_main_window_set_contents_style (main_window, 
							       MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS);
			modest_header_view_set_folder (MODEST_HEADER_VIEW(header_view),
						       TNY_FOLDER (folder_store));
			modest_widget_memory_restore (conf, G_OBJECT(header_view),
						      MODEST_CONF_HEADER_VIEW_KEY);
		} else {
			/* Update the active account */
			modest_window_set_active_account (MODEST_WINDOW (main_window), NULL);
			/* Do not show folder */
			modest_widget_memory_save (conf, G_OBJECT (header_view), MODEST_CONF_HEADER_VIEW_KEY);
			modest_header_view_set_folder (MODEST_HEADER_VIEW(header_view), NULL);
		}
	} else if (TNY_IS_ACCOUNT (folder_store)) {
		/* Update active account */
		set_active_account_from_tny_account (TNY_ACCOUNT (folder_store), MODEST_WINDOW (main_window));
		/* Show account details */
		modest_main_window_set_contents_style (main_window, MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS);
	}
}

void 
modest_ui_actions_on_item_not_found (ModestHeaderView *header_view,ModestItemType type,
				     ModestWindow *win)
{
	GtkWidget *dialog;
	gchar *txt, *item;
	gboolean online;

	item = (type == MODEST_ITEM_TYPE_FOLDER) ? "folder" : "message";
	
	if (g_main_depth > 0)	
		gdk_threads_enter ();
	online = tny_device_is_online (modest_runtime_get_device());

	if (online) {
		/* already online -- the item is simply not there... */
		dialog = gtk_message_dialog_new (GTK_WINDOW (win),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_OK,
						 _("The %s you selected cannot be found"),
						 item);
		gtk_dialog_run (GTK_DIALOG(dialog));
	} else {
		dialog = gtk_dialog_new_with_buttons (_("Connection requested"),
						      GTK_WINDOW (win),
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_REJECT,
						      GTK_STOCK_OK,
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
//			modest_platform_connect_and_wait ();;
		}
	}
	gtk_widget_destroy (dialog);
	if (g_main_depth > 0)	
		gdk_threads_leave ();
}

void
modest_ui_actions_on_msg_link_hover (ModestMsgView *msgview, const gchar* link,
				     ModestWindow *win)
{
	/* g_message ("%s %s", __FUNCTION__, link); */
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
	modest_msg_view_window_view_attachment (MODEST_MSG_VIEW_WINDOW (win), mime_part);
}

void
modest_ui_actions_on_msg_recpt_activated (ModestMsgView *msgview,
					  const gchar *address,
					  ModestWindow *win)
{
	/* g_message ("%s %s", __FUNCTION__, address); */
}

void
modest_ui_actions_on_save_to_drafts (GtkWidget *widget, ModestMsgEditWindow *edit_window)
{
	TnyTransportAccount *transport_account;
	ModestMailOperation *mail_operation;
	MsgData *data;
	gchar *account_name, *from;
	ModestAccountMgr *account_mgr;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW(edit_window));
	
	data = modest_msg_edit_window_get_msg_data (edit_window);

	account_mgr = modest_runtime_get_account_mgr();
	account_name = g_strdup(modest_window_get_active_account (MODEST_WINDOW(edit_window)));
	if (!account_name) 
		account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return;
	}
	transport_account =
		TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_tny_account_by_account
				      (modest_runtime_get_account_store(),
				       account_name,
				       TNY_ACCOUNT_TYPE_TRANSPORT));
	if (!transport_account) {
		g_printerr ("modest: no transport account found for '%s'\n", account_name);
		g_free (account_name);
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return;
	}
	from = modest_account_mgr_get_from_string (account_mgr, account_name);

	/* Create the mail operation */		
	mail_operation = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_INFO, G_OBJECT(edit_window));
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);

	modest_mail_operation_save_to_drafts (mail_operation,
					      transport_account,
					      from,
					      data->to, 
					      data->cc, 
					      data->bcc,
					      data->subject, 
					      data->plain_body, 
					      data->html_body,
					      data->attachments,
					      data->priority_flags);
	/* Frees */
	g_free (from);
	g_free (account_name);
	g_object_unref (G_OBJECT (transport_account));
	g_object_unref (G_OBJECT (mail_operation));

	modest_msg_edit_window_free_msg_data (edit_window, data);

	/* Save settings and close the window */
	gtk_widget_destroy (GTK_WIDGET (edit_window));
}
void
modest_ui_actions_on_send (GtkWidget *widget, ModestMsgEditWindow *edit_window)
{
	TnyTransportAccount *transport_account;
	ModestMailOperation *mail_operation;
	MsgData *data;
	gchar *account_name, *from;
	ModestAccountMgr *account_mgr;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW(edit_window));

	if (!modest_msg_edit_window_check_names (edit_window))
		return;
	
	data = modest_msg_edit_window_get_msg_data (edit_window);

	/* FIXME: Code added just for testing. The final version will
	   use the send queue provided by tinymail and some
	   classifier */
	account_mgr = modest_runtime_get_account_mgr();
	account_name = g_strdup(modest_window_get_active_account (MODEST_WINDOW(edit_window)));
	if (!account_name) 
		account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return;
	}
	transport_account =
		TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_transport_account_for_open_connection
				      (modest_runtime_get_account_store(),
				       account_name));
	if (!transport_account) {
		g_printerr ("modest: no transport account found for '%s'\n", account_name);
		g_free (account_name);
		modest_msg_edit_window_free_msg_data (edit_window, data);
		return;
	}
	from = modest_account_mgr_get_from_string (account_mgr, account_name);

	/* Create the mail operation */		
	mail_operation = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_SEND, G_OBJECT(edit_window));
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);

	modest_mail_operation_send_new_mail (mail_operation,
					     transport_account,
					     from,
					     data->to, 
					     data->cc, 
					     data->bcc,
					     data->subject, 
					     data->plain_body, 
					     data->html_body,
					     data->attachments,
					     data->priority_flags);
	/* Frees */
	g_free (from);
	g_free (account_name);
	g_object_unref (G_OBJECT (transport_account));
	g_object_unref (G_OBJECT (mail_operation));

	modest_msg_edit_window_free_msg_data (edit_window, data);

	/* Save settings and close the window */
	gtk_widget_destroy (GTK_WIDGET (edit_window));
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
modest_ui_actions_on_insert_image (GtkAction *action,
				   ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_ACTION (action));

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

	modest_msg_edit_window_attach_file (window);
}

void 
modest_ui_actions_on_remove_attachments (GtkAction *action,
					 ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail (GTK_IS_ACTION (action));

	modest_msg_edit_window_remove_attachments (window, NULL);
}

/*
 * Shows a dialog with an entry that asks for some text. The returned
 * value must be freed by the caller. The dialog window title will be
 * set to @title.
 */
static gchar *
ask_for_folder_name (GtkWindow *parent_window,
		     const gchar *title)
{
	GtkWidget *dialog, *entry;
	gchar *folder_name = NULL;

	/* Ask for folder name */
	dialog = gtk_dialog_new_with_buttons (_("New Folder Name"),
					      parent_window,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    gtk_label_new(title),
			    FALSE, FALSE, 0);
		
	entry = gtk_entry_new_with_max_length (40);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    entry,
			    TRUE, FALSE, 0);	
	
	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	
	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)		
		folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	gtk_widget_destroy (dialog);

	return folder_name;
}

void 
modest_ui_actions_on_new_folder (GtkAction *action, ModestMainWindow *main_window)
{
	TnyFolderStore *parent_folder;
	GtkWidget *folder_view;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	folder_view = modest_main_window_get_child_widget (main_window,
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	if (!folder_view)
		return;

	parent_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	
	if (parent_folder) {
		gboolean finished = FALSE;
		gint result;
		gchar *folder_name = NULL, *suggested_name = NULL;

		/* Run the new folder dialog */
		while (!finished) {
			result = modest_platform_run_new_folder_dialog (GTK_WINDOW (main_window),
									parent_folder,
									suggested_name,
									&folder_name);

			if (result == GTK_RESPONSE_REJECT) {
				finished = TRUE;
			} else {
				ModestMailOperation *mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_INFO, G_OBJECT(main_window));
				TnyFolder *new_folder = NULL;

				modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), 
								 mail_op);
		
				new_folder = modest_mail_operation_create_folder (mail_op,
										  parent_folder,
										  (const gchar *) folder_name);
				if (new_folder) {
					g_object_unref (new_folder);
					finished = TRUE;
				} 
/* 				else { */
/* 					/\* TODO: check error and follow proper actions *\/ */
/* /\* 					suggested_name = X; *\/ */
/* 					/\* Show error to the user *\/ */
/* 					modest_platform_run_information_dialog (GTK_WINDOW (main_window), */
/* 										_("mail_in_ui_folder_create_error")); */
/* 				} */
				g_object_unref (mail_op);
			}
			g_free (folder_name);
			folder_name = NULL;
		}

		g_object_unref (parent_folder);
	}
}

void 
modest_ui_actions_on_rename_folder (GtkAction *action,
				     ModestMainWindow *main_window)
{
	TnyFolderStore *folder;
	GtkWidget *folder_view;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	folder_view = modest_main_window_get_child_widget (main_window,
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	if (!folder_view)
		return;
	
	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW(folder_view));
	
	if (folder && TNY_IS_FOLDER (folder)) {
		gchar *folder_name;
		folder_name = ask_for_folder_name (GTK_WINDOW (main_window),
						   _("Please enter a new name for the folder"));

		if (folder_name != NULL && strlen (folder_name) > 0) {
			ModestMailOperation *mail_op;

			mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_INFO, G_OBJECT(main_window));
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
							 mail_op);

			modest_mail_operation_rename_folder (mail_op,
							     TNY_FOLDER (folder),
							     (const gchar *) folder_name);

			g_object_unref (mail_op);
			g_free (folder_name);
		}
		g_object_unref (folder);
	}
}

static void
delete_folder (ModestMainWindow *main_window, gboolean move_to_trash) 
{
	TnyFolderStore *folder;
	GtkWidget *folder_view;
	gint response;
	gchar *message;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	folder_view = modest_main_window_get_child_widget (main_window,
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	if (!folder_view)
		return;

	folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

	/* Show an error if it's an account */
	if (!TNY_IS_FOLDER (folder)) {
		modest_platform_run_information_dialog (GTK_WINDOW (main_window),
							_("mail_in_ui_folder_delete_error"));
		return ;
	}

	/* Ask the user */	
	message =  g_strdup_printf (_("mcen_nc_delete_folder_text"), 
				    tny_folder_get_name (TNY_FOLDER (folder)));
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (main_window), 
							    (const gchar *) message);
	g_free (message);

	if (response == GTK_RESPONSE_OK) {
		ModestMailOperation *mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_DELETE, G_OBJECT(main_window));

		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
						 mail_op);
		modest_mail_operation_remove_folder (mail_op, TNY_FOLDER (folder), move_to_trash);

		/* Show error if happened */
		if (modest_mail_operation_get_error (mail_op))
			modest_platform_run_information_dialog (GTK_WINDOW (main_window),
								_("mail_in_ui_folder_delete_error"));

		g_object_unref (G_OBJECT (mail_op));
	}

	g_object_unref (G_OBJECT (folder));
}

void 
modest_ui_actions_on_delete_folder (GtkAction *action,
				     ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	delete_folder (main_window, FALSE);
}

void 
modest_ui_actions_on_move_folder_to_trash_folder (GtkAction *action, ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	delete_folder (main_window, TRUE);
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
	/* printf("DEBUG: %s: server_account_name=%s\n", __FUNCTION__, server_account_name); */
	
#ifdef MODEST_PLATFORM_MAEMO
	/* Maemo uses a different (awkward) button order,
	 * It should probably just use gtk_alternative_dialog_button_order ().
	 */
	GtkWidget *dialog = gtk_dialog_new_with_buttons (_("mail_ti_password_protected"),
					      NULL,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      NULL);
#else
	GtkWidget *dialog = gtk_dialog_new_with_buttons (_("mail_ti_password_protected"),
					      NULL,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);
#endif /* MODEST_PLATFORM_MAEMO */

	gtk_window_set_transient_for (GTK_WINDOW(dialog), GTK_WINDOW(main_window));
	
	gchar *server_name = modest_server_account_get_hostname (
		modest_runtime_get_account_mgr(), server_account_name);
	
	/* This causes a warning because the logical ID has no %s in it, 
	 * though the translation does, but there is not much we can do about that: */
	gchar *txt = g_strdup_printf (_("mail_ia_password_info"), server_name);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), gtk_label_new(txt),
			    FALSE, FALSE, 0);
	g_free (txt);
	g_free (server_name);
	server_name = NULL;

	/* username: */
	gchar *initial_username = modest_server_account_get_username (
		modest_runtime_get_account_mgr(), server_account_name);
	
	GtkWidget *entry_username = gtk_entry_new ();
	if (initial_username)
		gtk_entry_set_text (GTK_ENTRY (entry_username), initial_username);
	/* Dim this if a connection has ever succeeded with this username,
	 * as per the UI spec: */
	const gboolean username_known = 
		modest_server_account_get_username_has_succeeded(
			modest_runtime_get_account_mgr(), server_account_name);
	gtk_widget_set_sensitive (entry_username, !username_known);
	
#ifdef MODEST_PLATFORM_MAEMO
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (entry_username), HILDON_GTK_INPUT_MODE_FULL);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	
	GtkWidget *caption = hildon_caption_new (sizegroup, 
		_("mail_fi_username"), entry_username, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (entry_username);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), caption, 
		FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
#else 
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), entry_username,
			    TRUE, FALSE, 0);
#endif /* MODEST_PLATFORM_MAEMO */	
			    
	/* password: */
	GtkWidget *entry_password = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY(entry_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY(entry_password), "*"); */
	
#ifdef MODEST_PLATFORM_MAEMO
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (entry_password), 
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);
	
	caption = hildon_caption_new (sizegroup, 
		_("mail_fi_password"), entry_password, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (entry_password);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), caption, 
		FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	g_object_unref (sizegroup);
#else 
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), entry_password,
			    TRUE, FALSE, 0);
#endif /* MODEST_PLATFORM_MAEMO */	
			    	
/* This is not in the Maemo UI spec:
	remember_pass_check = gtk_check_button_new_with_label (_("Remember password"));
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), remember_pass_check,
			    TRUE, FALSE, 0);
*/

	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	
	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		if (username) {
			*username = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry_username)));
			
			modest_server_account_set_username (
				 modest_runtime_get_account_mgr(), server_account_name, 
				 *username);
				 
			const gboolean username_was_changed = 
				(strcmp (*username, initial_username) != 0);
			if (username_was_changed) {
				/* To actually use a changed username, 
				 * we must reset the connection, according to pvanhoof.
				 * This _might_ be a sensible way to do that: */
				 TnyDevice *device = modest_runtime_get_device();
				 tny_device_force_offline (device);
				 tny_device_force_online (device);
			}
		}
			
		if (password) {
			*password = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry_password)));
			
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
		if (username)
			*username = NULL;
			
		if (password)
			*password = NULL;
			
		if (cancel)
			*cancel   = TRUE;
	}

/* This is not in the Maemo UI spec:
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (remember_pass_check)))
		*remember = TRUE;
	else
		*remember = FALSE;
*/

	gtk_widget_destroy (dialog);
}

void
modest_ui_actions_on_cut (GtkAction *action,
			  ModestWindow *window)
{
	GtkWidget *focused_widget;

	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));
	if (GTK_IS_EDITABLE (focused_widget)) {
		gtk_editable_cut_clipboard (GTK_EDITABLE(focused_widget));
	} else if (GTK_IS_TEXT_VIEW (focused_widget)) {
		GtkTextBuffer *buffer;
		GtkClipboard *clipboard;

		clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused_widget));
		gtk_text_buffer_cut_clipboard (buffer, clipboard, TRUE);
	}
}

void
modest_ui_actions_on_copy (GtkAction *action,
			   ModestWindow *window)
{
	GtkClipboard *clipboard;
	GtkWidget *focused_widget;

	clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));
	if (GTK_IS_LABEL (focused_widget)) {
		gtk_clipboard_set_text (clipboard, gtk_label_get_text (GTK_LABEL (focused_widget)), -1);
	} else if (GTK_IS_EDITABLE (focused_widget)) {
		gtk_editable_copy_clipboard (GTK_EDITABLE(focused_widget));
	} else if (GTK_IS_TEXT_VIEW (focused_widget)) {
		GtkTextBuffer *buffer;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused_widget));
		gtk_text_buffer_copy_clipboard (buffer, clipboard);
	}
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
modest_ui_actions_on_paste (GtkAction *action,
			    ModestWindow *window)
{
	GtkWidget *focused_widget;

	focused_widget = gtk_window_get_focus (GTK_WINDOW (window));
	if (GTK_IS_EDITABLE (focused_widget)) {
		gtk_editable_paste_clipboard (GTK_EDITABLE(focused_widget));
	} else if (GTK_IS_TEXT_VIEW (focused_widget)) {
		GtkTextBuffer *buffer;
		GtkClipboard *clipboard;

		clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused_widget));
		gtk_text_buffer_paste_clipboard (buffer, clipboard, NULL, TRUE);
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

void     modest_ui_actions_msg_edit_on_change_priority (GtkRadioAction *action,
							GtkRadioAction *selected,
							ModestWindow *window)
{
	TnyHeaderFlags flags;
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	flags = gtk_radio_action_get_current_value (selected);
	modest_msg_edit_window_set_priority_flags (MODEST_MSG_EDIT_WINDOW (window), flags);
}

void     modest_ui_actions_msg_edit_on_change_file_format (GtkRadioAction *action,
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
		gtk_window_present (GTK_WINDOW (window));
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

	gtk_window_present (GTK_WINDOW (window));
}

/* 
 * Used by modest_ui_actions_on_details to call do_headers_action 
 */
static void
headers_action_show_details (TnyHeader *header, 
			     ModestWindow *window,
			     gpointer user_data)

{
	GtkWidget *dialog;
	
	/* Create dialog */
	dialog = modest_details_dialog_new_with_header (GTK_WINDOW (window), header);

	/* Run dialog */
	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

/*
 * Show the folder details in a ModestDetailsDialog widget
 */
static void
show_folder_details (TnyFolder *folder, 
		     GtkWindow *window)
{
	GtkWidget *dialog;
	
	/* Create dialog */
	dialog = modest_details_dialog_new_with_folder (window, folder);

	/* Run dialog */
	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

/*
 * Show the header details in a ModestDetailsDialog widget
 */
void     
modest_ui_actions_on_details (GtkAction *action, 
			      ModestWindow *win)
{
	TnyList * headers_list;
	TnyIterator *iter;
	TnyHeader *header;		

	if (MODEST_IS_MSG_VIEW_WINDOW (win)) {
		TnyMsg *msg;

		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
		if (!msg) {
			return;
		} else {
			headers_list = get_selected_headers (win);
			if (!headers_list)
				return;

			iter = tny_list_create_iterator (headers_list);

			header = TNY_HEADER (tny_iterator_get_current (iter));
			headers_action_show_details (header, win, NULL);
			g_object_unref (header);

			g_object_unref (iter);
		}
	} else if (MODEST_IS_MAIN_WINDOW (win)) {
		GtkWidget *folder_view, *header_view;

		/* Check which widget has the focus */
		folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
								    MODEST_WIDGET_TYPE_FOLDER_VIEW);
		if (gtk_widget_is_focus (folder_view)) {
			TnyFolder *folder;

			folder = (TnyFolder *) modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

			/* Show only when it's a folder */
			if (!folder || !TNY_IS_FOLDER (folder))
				return;

			show_folder_details (folder, GTK_WINDOW (win));

		} else {
			header_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
									   MODEST_WIDGET_TYPE_HEADER_VIEW);
			/* Show details of each header */
			do_headers_action (win, headers_action_show_details, header_view);
		}
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
	ModestConf *conf;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	conf = modest_runtime_get_conf ();
	
	if (modest_main_window_get_style (main_window) == MODEST_MAIN_WINDOW_STYLE_SPLIT)
		modest_main_window_set_style (main_window, MODEST_MAIN_WINDOW_STYLE_SIMPLE);
	else
		modest_main_window_set_style (main_window, MODEST_MAIN_WINDOW_STYLE_SPLIT);
}

void 
modest_ui_actions_on_toggle_toolbar (GtkToggleAction *toggle, 
				     ModestWindow *window)
{
	gboolean active, fullscreen = FALSE;
	ModestWindowMgr *mgr;

	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (toggle));

	/* Check if we want to toggle the toolbar vuew in fullscreen
	   or normal mode */
	if (!strcmp (gtk_action_get_name (GTK_ACTION (toggle)), 
		     "ViewShowToolbarFullScreen")) {
		fullscreen = TRUE;
	}

	/* Toggle toolbar */
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_show_toolbars (mgr, active, fullscreen);
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
	/* Do not change the application name if the widget has not
	   the focus. This callback could be called even if the folder
	   view has not the focus, because the handled signal could be
	   emitted when the folder view is redrawn */
	if (gtk_widget_is_focus (GTK_WIDGET (folder_view))) {
		if (display_name)
			gtk_window_set_title (window, display_name);
		else
			gtk_window_set_title (window, " ");
	}
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
	modest_msg_edit_window_check_names (window);
}


static GtkWidget*
create_move_to_dialog (ModestWindow *win,
		       GtkWidget *folder_view,
		       GtkWidget **tree_view)
{
	GtkWidget *dialog, *scroll;

	dialog = gtk_dialog_new_with_buttons (_("mcen_ti_moveto_folders_title"),
					      GTK_WINDOW (win),
					      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      NULL);

	/* Create scrolled window */
	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scroll),
					 GTK_POLICY_AUTOMATIC,
					 GTK_POLICY_AUTOMATIC);

	/* Create folder view */
	*tree_view = modest_folder_view_new (NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (*tree_view),
				 gtk_tree_view_get_model (GTK_TREE_VIEW (folder_view)));
	gtk_container_add (GTK_CONTAINER (scroll), *tree_view);

	/* Add scroll to dialog */
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
			    scroll, FALSE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));

	return dialog;
}

/*
 * Returns TRUE if at least one of the headers of the list belongs to
 * a message that has been fully retrieved.
 */
static gboolean
has_retrieved_msgs (TnyList *list)
{
	TnyIterator *iter;
	gboolean found = FALSE;

	iter = tny_list_create_iterator (list);
	while (tny_iterator_is_done (iter) && !found) {
		TnyHeader *header;
		TnyHeaderFlags flags;

		header = TNY_HEADER (tny_iterator_get_current (iter));
		flags = tny_header_get_flags (header);
		if (!(flags & TNY_HEADER_FLAG_PARTIAL))
			found = TRUE;

		if (!found)
			tny_iterator_next (iter);
	}
	g_object_unref (iter);

	return found;
}

/*
 * Shows a confirmation dialog to the user when we're moving messages
 * from a remote server to the local storage. Returns the dialog
 * response. If it's other kind of movement the it always returns
 * GTK_RESPONSE_OK
 */
static gint
msgs_move_to_confirmation (GtkWindow *win,
			   TnyFolder *dest_folder,
			   TnyList *headers)
{
	gint response = GTK_RESPONSE_OK;

	/* If the destination is a local folder */
	if (modest_tny_folder_is_local_folder (dest_folder)) {
		TnyFolder *src_folder;
		TnyIterator *iter;
		TnyHeader *header;

		/* Get source folder */
		iter = tny_list_create_iterator (headers);
		header = TNY_HEADER (tny_iterator_get_current (iter));
		src_folder = tny_header_get_folder (header);
		g_object_unref (header);
		g_object_unref (iter);

		/* If the source is a remote folder */
		if (!modest_tny_folder_is_local_folder (src_folder)) {
			const gchar *message;
			
			if (tny_list_get_length (headers) == 1)
				if (has_retrieved_msgs (headers))
					message = _("mcen_nc_move_retrieve");
				else
					message = _("mcen_nc_move_header");
			else
				if (has_retrieved_msgs (headers))
					message = _("mcen_nc_move_retrieves");
				else
					message = _("mcen_nc_move_headers");
			
			response = modest_platform_run_confirmation_dialog (GTK_WINDOW (win),
									    (const gchar *) message);
		}
	}
	return response;
}

/*
 * UI handler for the "Move to" action when invoked from the
 * ModestMainWindow
 */
static void 
modest_ui_actions_on_main_window_move_to (GtkAction *action, 
					  ModestMainWindow *win)
{
	GtkWidget *dialog, *folder_view, *tree_view = NULL;
	gint result;
	TnyFolderStore *folder_store;
	ModestMailOperation *mail_op = NULL;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (win));

	/* Get the folder view */
	folder_view = modest_main_window_get_child_widget (win,
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);

	/* Create and run the dialog */
	dialog = create_move_to_dialog (MODEST_WINDOW (win), folder_view, &tree_view);
	result = gtk_dialog_run (GTK_DIALOG(dialog));

	/* We do this to save an indentation level ;-) */
	if (result != GTK_RESPONSE_ACCEPT)
		goto end;

	folder_store = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (tree_view));

	if (TNY_IS_ACCOUNT (folder_store))
		goto end;

	/* Get folder or messages to transfer */
	if (gtk_widget_is_focus (folder_view)) {
		TnyFolderStore *src_folder;
		src_folder = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

		if (TNY_IS_FOLDER (src_folder)) {
			mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_RECEIVE, G_OBJECT(win));
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), 
							 mail_op);

			modest_mail_operation_xfer_folder (mail_op, 
							   TNY_FOLDER (src_folder),
							   folder_store,
							   TRUE);
			g_object_unref (G_OBJECT (mail_op));
		}

		/* Frees */
		g_object_unref (G_OBJECT (src_folder));
	} else {
		GtkWidget *header_view;
		header_view = modest_main_window_get_child_widget (win,
								   MODEST_WIDGET_TYPE_HEADER_VIEW);
		if (gtk_widget_is_focus (header_view)) {
			TnyList *headers;
			gint response;

			headers = modest_header_view_get_selected_headers (MODEST_HEADER_VIEW (header_view));

			/* Ask for user confirmation */
			response = msgs_move_to_confirmation (GTK_WINDOW (win), 
							      TNY_FOLDER (folder_store), 
							      headers);

			/* Transfer messages */
			if (response == GTK_RESPONSE_OK) {
				mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_RECEIVE, G_OBJECT(win));
				modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), 
								 mail_op);

				modest_mail_operation_xfer_msgs (mail_op, 
								 headers,
								 TNY_FOLDER (folder_store),
								 TRUE);
				g_object_unref (G_OBJECT (mail_op));
			}
		}
	}
	g_object_unref (folder_store);

 end:
	gtk_widget_destroy (dialog);
}


/*
 * UI handler for the "Move to" action when invoked from the
 * ModestMsgViewWindow
 */
static void 
modest_ui_actions_on_msg_view_window_move_to (GtkAction *action, 
					      ModestMsgViewWindow *win)
{
	GtkWidget *dialog, *folder_view, *tree_view = NULL;
	gint result;
	ModestMainWindow *main_window;
	TnyMsg *msg;
	TnyHeader *header;
	TnyList *headers;

	/* Get the folder view */
	main_window = MODEST_MAIN_WINDOW (modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ()));
	folder_view = modest_main_window_get_child_widget (main_window,
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);

	/* Create and run the dialog */
	dialog = create_move_to_dialog (MODEST_WINDOW (win), folder_view, &tree_view);	
	result = gtk_dialog_run (GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_ACCEPT) {
		TnyFolderStore *folder_store;
		gint response;

		folder_store = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (tree_view));

		/* Create header list */
		msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (win));
		header = tny_msg_get_header (msg);
		headers = tny_simple_list_new ();
		tny_list_prepend (headers, G_OBJECT (header));
		g_object_unref (header);
		g_object_unref (msg);

		/* Ask user for confirmation. MSG-NOT404 */
		response = msgs_move_to_confirmation (GTK_WINDOW (win), 
						      TNY_FOLDER (folder_store), 
						      headers);

		/* Transfer current msg */
		if (response == GTK_RESPONSE_OK) {
			ModestMailOperation *mail_op;

			/* Create mail op */
			mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_RECEIVE, G_OBJECT(win));
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), 
							 mail_op);
			
			/* Transfer messages */
			modest_mail_operation_xfer_msgs (mail_op, 
							 headers,
							 TNY_FOLDER (folder_store),
							 TRUE);
			g_object_unref (G_OBJECT (mail_op));
		} else {
			g_object_unref (headers);
		}
		g_object_unref (folder_store);
	}
	gtk_widget_destroy (dialog);
}

void 
modest_ui_actions_on_move_to (GtkAction *action, 
			      ModestWindow *win)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (win) ||
			  MODEST_IS_MSG_VIEW_WINDOW (win));

	if (MODEST_IS_MAIN_WINDOW (win)) 
		modest_ui_actions_on_main_window_move_to (action, 
							  MODEST_MAIN_WINDOW (win));
	else
		modest_ui_actions_on_msg_view_window_move_to (action, 
							      MODEST_MSG_VIEW_WINDOW (win));
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
	TnyList *headers_list;
	TnyIterator *iter;

	/* Get headers */
	headers_list = get_selected_headers (win);
	if (!headers_list)
		return;

	/* Call the function for each header */
	iter = tny_list_create_iterator (headers_list);
	while (!tny_iterator_is_done (iter)) {
		TnyHeader *header;

		header = TNY_HEADER (tny_iterator_get_current (iter));
		func (header, win, user_data);
		g_object_unref (header);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
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
		modest_msg_view_window_remove_attachments (MODEST_MSG_VIEW_WINDOW (window), NULL);
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
	gtk_widget_show (dialog);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
}

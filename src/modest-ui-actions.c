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
#include "modest-ui-actions.h"
#include "modest-tny-platform-factory.h"

#include <widgets/modest-main-window.h>
#include "modest-account-view-window.h"
#include <widgets/modest-msg-view-window.h>

#include "modest-account-mgr-helpers.h"
#include "modest-mail-operation.h"
#include <modest-widget-memory.h>
#include <tny-error.h>
#include <tny-simple-list.h>
#include <tny-msg-view.h>
#include <tny-device.h>


typedef struct _GetMsgAsyncHelper {
	ModestMainWindow *main_window;
	TnyIterator *iter;
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
	gchar *from;
} ReplyForwardHelper;


static void     reply_forward_func     (gpointer data, gpointer user_data);
static void     read_msg_func          (gpointer data, gpointer user_data);
static void     get_msg_cb             (TnyFolder *folder, TnyMsg *msg,	GError **err, 
					gpointer user_data);

static void     reply_forward          (GtkWidget *widget, ReplyForwardAction action,
					ModestMainWindow *main_window);

static gchar*   ask_for_folder_name    (GtkWindow *parent_window, const gchar *title);


void   
modest_ui_actions_on_about (GtkWidget *widget, ModestWindow *win)
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
					 "contributions from the fine people at KernelConcepts and Igalia\n"
					 "uses the tinymail email framework written by Philip van Hoof"));
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(about), authors);
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(about), "http://modest.garage.maemo.org");
	
	gtk_dialog_run (GTK_DIALOG (about));
	gtk_widget_destroy(about);
}

void
modest_ui_actions_on_delete (GtkWidget *widget, ModestMainWindow *main_window)
{
	TnyList *header_list;
	TnyIterator *iter;
	GtkTreeModel *model;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	if (!main_window->header_view)
		return;
	
	header_list = modest_header_view_get_selected_headers (main_window->header_view);
	
	if (header_list) {
		iter = tny_list_create_iterator (header_list);
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (main_window->header_view));
		if (GTK_IS_TREE_MODEL_SORT (model))
			model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model));
		do {
			TnyHeader *header;
			ModestMailOperation *mail_op;

			header = TNY_HEADER (tny_iterator_get_current (iter));
			/* TODO: thick grain mail operation involving
			   a list of objects. Composite pattern ??? */
			mail_op = modest_mail_operation_new ();

			/* TODO: add confirmation dialog */

			/* Move to trash */
			modest_mail_operation_remove_msg (mail_op, header, TRUE);

			/* Remove from tree model */
			if (modest_mail_operation_get_status (mail_op) == 
			    MODEST_MAIL_OPERATION_STATUS_SUCCESS)
				tny_list_remove (TNY_LIST (model), G_OBJECT (header));
			else {
				/* TODO: error handling management */
				const GError *error;
				error = modest_mail_operation_get_error (mail_op);
				g_warning (error->message);
			}

			g_object_unref (G_OBJECT (mail_op));
			g_object_unref (header);
			tny_iterator_next (iter);

		} while (!tny_iterator_is_done (iter));
	}
}


void
modest_ui_actions_on_quit (GtkWidget *widget, ModestWindow *win)
{
	/* FIXME: save size of main window */
/* 	save_sizes (main_window); */
	gtk_widget_destroy (GTK_WIDGET (win));
}

void
modest_ui_actions_on_accounts (GtkWidget *widget, ModestWindow *win)
{
	GtkWidget *account_win;
	account_win = modest_account_view_window_new ();

	if (win)
		gtk_window_set_transient_for (GTK_WINDOW (account_win), GTK_WINDOW (win));

	gtk_widget_show (account_win);
}

void
modest_ui_actions_on_new_msg (GtkWidget *widget, ModestWindow *win)
{
	ModestWindow *msg_win;
	msg_win = modest_msg_edit_window_new (MODEST_EDIT_TYPE_NEW);
	if (win)
		gtk_window_set_transient_for (GTK_WINDOW (msg_win),
					      GTK_WINDOW (win));
	
	gtk_widget_show_all (GTK_WIDGET (msg_win));
}


void
modest_ui_actions_on_open (GtkWidget *widget, ModestWindow *win)
{
	/* FIXME */
	
}


static void
reply_forward_func (gpointer data, gpointer user_data)
{
	TnyMsg *msg, *new_msg;
	GetMsgAsyncHelper *helper;
	ReplyForwardHelper *rf_helper;
	ModestWindow *msg_win;
	ModestEditType edit_type;

	msg = TNY_MSG (data);
	helper = (GetMsgAsyncHelper *) user_data;
	rf_helper = (ReplyForwardHelper *) helper->user_data;

	/* Create reply mail */
	switch (rf_helper->action) {
	case ACTION_REPLY:
		new_msg = 
			modest_mail_operation_create_reply_mail (msg, 
								 rf_helper->from, 
								 rf_helper->reply_forward_type,
								 MODEST_MAIL_OPERATION_REPLY_MODE_SENDER);
		break;
	case ACTION_REPLY_TO_ALL:
		new_msg = 
			modest_mail_operation_create_reply_mail (msg, rf_helper->from, rf_helper->reply_forward_type,
								 MODEST_MAIL_OPERATION_REPLY_MODE_ALL);
		edit_type = MODEST_EDIT_TYPE_REPLY;
		break;
	case ACTION_FORWARD:
		new_msg = 
			modest_mail_operation_create_forward_mail (msg, rf_helper->from, rf_helper->reply_forward_type);
		edit_type = MODEST_EDIT_TYPE_FORWARD;
		break;
	default:
		g_return_if_reached ();
	}

	if (!new_msg) {
		g_warning ("Unable to create a message");
		goto cleanup;
	}
		
	/* Show edit window */
	msg_win = modest_msg_edit_window_new (MODEST_EDIT_TYPE_NEW);
	modest_msg_edit_window_set_msg (MODEST_MSG_EDIT_WINDOW (msg_win),
					new_msg);
	gtk_widget_show_all (GTK_WIDGET (msg_win));
	
	/* Clean */
	g_object_unref (G_OBJECT (new_msg));

 cleanup:
	g_free (rf_helper->from);
	g_slice_free (ReplyForwardHelper, rf_helper);
}

/*
 * Common code for the reply and forward actions
 */
static void
reply_forward (GtkWidget *widget, ReplyForwardAction action,
	       ModestMainWindow *main_window)
{
	ModestAccountMgr *account_mgr;
	TnyList *header_list;
	guint reply_forward_type;
	ModestConf *conf;	
	ModestAccountData *default_account_data;
	TnyHeader *header;
	TnyFolder *folder;
	gchar *from, *key, *default_account_name;
	GetMsgAsyncHelper *helper;
	ReplyForwardHelper *rf_helper;

	if (!main_window->header_view)
		return;

	header_list = modest_header_view_get_selected_headers (main_window->header_view);	
	if (!header_list)
		return;

	conf = modest_runtime_get_conf ();
	
	/* Get reply or forward type */
	key = g_strdup_printf ("%s/%s", MODEST_CONF_NAMESPACE, 
			       (action == ACTION_FORWARD) ? MODEST_CONF_FORWARD_TYPE : MODEST_CONF_REPLY_TYPE);
	reply_forward_type = modest_conf_get_int (conf, key, NULL);
	g_free (key);

	/* We assume that we can only select messages of the
	   same folder and that we reply all of them from the
	   same account. In fact the interface currently only
	   allows single selection */
	account_mgr = modest_runtime_get_account_mgr();
	default_account_name = modest_account_mgr_get_default_account (account_mgr);
	default_account_data = 
		modest_account_mgr_get_account_data (account_mgr,
						     (const gchar*) default_account_name);
	from = g_strdup (default_account_data->email);
	modest_account_mgr_free_account_data (account_mgr, default_account_data);
	g_free (default_account_name);
	
	/* Fill helpers */
	rf_helper = g_slice_new0 (ReplyForwardHelper);
	rf_helper->reply_forward_type = reply_forward_type;
	rf_helper->action = action;
	rf_helper->from = from;
	
	helper = g_slice_new0 (GetMsgAsyncHelper);
	helper->main_window = main_window;
	helper->func = reply_forward_func;
	helper->iter = tny_list_create_iterator (header_list);
	helper->user_data = rf_helper;
	
	header = TNY_HEADER (tny_iterator_get_current (helper->iter));
	folder = tny_header_get_folder (header);
	
	/* The callback will call it per each header */
	tny_folder_get_msg_async (folder, header, get_msg_cb, helper);
	
	/* Clean */
	g_object_unref (G_OBJECT (header));
	g_object_unref (G_OBJECT (folder));
}

void
modest_ui_actions_on_reply (GtkWidget *widget, ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	reply_forward (widget, ACTION_REPLY, main_window);
}

void
modest_ui_actions_on_forward (GtkWidget *widget, ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	reply_forward (widget, ACTION_FORWARD, main_window);
}

void
modest_ui_actions_on_reply_all (GtkWidget *widget,ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	reply_forward (widget, ACTION_REPLY_TO_ALL, main_window);
}

void 
modest_ui_actions_on_next (GtkWidget *widget, 
			   ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	if (main_window->header_view)
		modest_header_view_select_next (main_window->header_view); 
}

void
modest_ui_actions_toggle_view (GtkWidget *widget, ModestMainWindow *main_window)
{
	ModestConf *conf;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	if (!main_window->header_view)
		return;
	conf = modest_runtime_get_conf ();
	
	/* what is saved/restored is depending on the style; thus; we save with
	 * old style, then update the style, and restore for this new style*/
	modest_widget_memory_save (conf, G_OBJECT(main_window->header_view), "header-view");
	
	if (modest_header_view_get_style (main_window->header_view) == MODEST_HEADER_VIEW_STYLE_DETAILS)
		modest_header_view_set_style (main_window->header_view,
					      MODEST_HEADER_VIEW_STYLE_TWOLINES);
	else
		modest_header_view_set_style (main_window->header_view,
					      MODEST_HEADER_VIEW_STYLE_DETAILS);

	modest_widget_memory_restore (conf, G_OBJECT(main_window->header_view),
				      "header-view");
}



/*
 * Marks a message as read and passes it to the msg preview widget
 */
static void
read_msg_func (gpointer data, gpointer user_data)
{
	TnyMsg *msg;
	TnyHeader *header;
	GetMsgAsyncHelper *helper;
	TnyHeaderFlags header_flags;

	msg = TNY_MSG (data);
	helper = (GetMsgAsyncHelper *) user_data;

	if (!helper->main_window->msg_preview)
		return;
	
	/* mark message as seen; _set_flags crashes, bug in tinymail? */
	header = TNY_HEADER (tny_iterator_get_current (helper->iter));
	header_flags = tny_header_get_flags (header);
	tny_header_set_flags (header, header_flags | TNY_HEADER_FLAG_SEEN);
	g_object_unref (G_OBJECT (header));

	/* Set message on msg view */
	modest_msg_view_set_message (helper->main_window->msg_preview,
				     msg);
}

/*
 * This function is a generic handler for the tny_folder_get_msg_async
 * call. It expects as user_data a #GetMsgAsyncHelper. This helper
 * contains a user provided function that is called inside this
 * method. This will allow us to use this callback in many different
 * places. This callback performs the common actions for the
 * get_msg_async call, more specific actions will be done by the user
 * function
 */
static void
get_msg_cb (TnyFolder *folder, TnyMsg *msg, GError **err, gpointer user_data)
{
	GetMsgAsyncHelper *helper;

	helper = (GetMsgAsyncHelper *) user_data;

	if ((*err && ((*err)->code == TNY_FOLDER_ERROR_GET_MSG)) || !msg) {
		ModestHeaderView *header_view =
			helper->main_window->header_view;
		if (header_view)
			modest_ui_actions_on_item_not_found (header_view,
							     MODEST_ITEM_TYPE_MESSAGE,
							     MODEST_WINDOW(helper->main_window));
		return;
	}

	/* Call user function */
	helper->func (msg, user_data);

	/* Process next element (if exists) */
	tny_iterator_next (helper->iter);
	if (tny_iterator_is_done (helper->iter)) {
		TnyList *headers;
		headers = tny_iterator_get_list (helper->iter);
		/* Free resources */
		g_object_unref (G_OBJECT (headers));
		g_object_unref (G_OBJECT (helper->iter));
		g_slice_free (GetMsgAsyncHelper, helper);
	} else {
		TnyHeader *header;
		header = TNY_HEADER (tny_iterator_get_current (helper->iter)); 
		tny_folder_get_msg_async (folder, header,			  
					  get_msg_cb, helper);
		g_object_unref (G_OBJECT(header));
	}
}

void 
modest_ui_actions_on_header_selected (ModestHeaderView *folder_view,     TnyHeader *header,
				      ModestMainWindow *main_window)
{
	TnyFolder *folder;
	GetMsgAsyncHelper *helper;
	TnyList *list;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	if (!main_window->msg_preview)
		return;
	
	/* when there's no header, clear the msgview */
	if (!header) {
		modest_msg_view_set_message (main_window->msg_preview, NULL);
		return;
	}

	folder = tny_header_get_folder (TNY_HEADER(header));

	/* Create list */
	list = tny_simple_list_new ();
	tny_list_prepend (list, G_OBJECT (header));

	/* Fill helper data */
	helper = g_slice_new0 (GetMsgAsyncHelper);
	helper->main_window = main_window;
	helper->iter = tny_list_create_iterator (list);
	helper->func = read_msg_func;

	tny_folder_get_msg_async (TNY_FOLDER(folder),
				  header, get_msg_cb,
				  helper);

	/* Frees */
	g_object_unref (G_OBJECT (folder));
}



void 
modest_ui_actions_on_header_activated (ModestHeaderView *folder_view, TnyHeader *header,
					ModestMainWindow *main_window)
{
	ModestWindow *win;
	TnyFolder *folder = NULL;
	TnyMsg    *msg    = NULL;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	if (!header)
		return;

	folder = tny_header_get_folder (header);
	if (!folder) {
		g_printerr ("modest: cannot get folder for header\n");
		goto cleanup;
	}

	/* FIXME: make async?; check error  */
	msg = tny_folder_get_msg (folder, header, NULL);
	if (!msg) {
		g_printerr ("modest: cannot get msg for header\n");
		goto cleanup;
	}

	win = modest_msg_view_window_new (msg);
	gtk_window_set_transient_for (GTK_WINDOW (win),
				      GTK_WINDOW (main_window));

	gtk_widget_show_all (GTK_WIDGET(win));
	
cleanup:
	if (folder)
		g_object_unref (G_OBJECT (folder));
	if (msg)
		g_object_unref (G_OBJECT (folder));
}





void 
modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
					       TnyFolder *folder, 
					       gboolean selected,
					       ModestMainWindow *main_window)
{
//	GtkLabel *folder_info_label;
	gchar *txt;	
	ModestConf *conf;

/* 	folder_info_label =  */
/* 		GTK_LABEL (modest_widget_factory_get_folder_info_label */
/* 			   (modest_runtime_get_widget_factory())); */

/* 	if (!folder) { */
/* 		gtk_label_set_label (GTK_LABEL(folder_info_label), ""); */
/* 		return; */
/* 	} */
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	if (!main_window->header_view)
		return;

	conf = modest_runtime_get_conf ();

	if (!selected) { /* the folder was unselected; save it's settings  */
		modest_widget_memory_save (conf, G_OBJECT (main_window->header_view),
					   "header-view");
		gtk_window_set_title (GTK_WINDOW(main_window), "Modest");
		modest_header_view_set_folder (main_window->header_view, NULL);
	} else {  /* the folder was selected */
		if (folder) { /* folder may be NULL */
			guint num, unread;
			gchar *title;

			num    = tny_folder_get_all_count    (folder);
			unread = tny_folder_get_unread_count (folder);
			
			title = g_strdup_printf ("Modest: %s",
						 tny_folder_get_name (folder));
			
			gtk_window_set_title (GTK_WINDOW(main_window), title);
			g_free (title);
			
			txt = g_strdup_printf (_("%d %s, %d unread"),
					       num, num==1 ? _("item") : _("items"), unread);		
			//gtk_label_set_label (GTK_LABEL(folder_info_label), txt);
			g_free (txt);
		}
		modest_header_view_set_folder (main_window->header_view, folder);
		modest_widget_memory_restore (conf, G_OBJECT(main_window->header_view),
					      "header-view");
	}
}


/****************************************************/
/*
 * below some stuff to clearup statusbar messages after 1,5 seconds....
 */
static gboolean
progress_bar_clean (GtkWidget *bar)
{
	if (GTK_IS_PROGRESS_BAR(bar)) {
		gtk_progress_bar_set_text     (GTK_PROGRESS_BAR(bar), "");
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(bar), 1.0);
	}
	return FALSE;
}

static gboolean
statusbar_clean (GtkWidget *bar)
{
	if (GTK_IS_STATUSBAR(bar))
		gtk_statusbar_push (GTK_STATUSBAR(bar), 0, "");
	return FALSE;
}


static void
statusbar_push (ModestMainWindow *main_window, guint context_id, const gchar *msg)
{
	if (!msg)
		return;

	if (main_window->progress_bar) {
		gtk_widget_show (main_window->progress_bar);
		g_timeout_add (3000, (GSourceFunc)progress_bar_clean,
			       main_window->progress_bar);
	}
	
	if (main_window->status_bar) {
		gtk_widget_show (main_window->status_bar);
		gtk_statusbar_push (GTK_STATUSBAR(main_window->status_bar), 0, msg);
		g_timeout_add (1500, (GSourceFunc)statusbar_clean, main_window->status_bar);
	}

}
/****************************************************************************/

void 
modest_ui_actions_on_item_not_found (ModestHeaderView *header_view,ModestItemType type,
				     ModestWindow *win)
{
	GtkWidget *dialog;
	gchar *txt, *item;
	gboolean online;
	TnyDevice *device;
	TnyAccountStore *account_store;

	item = (type == MODEST_ITEM_TYPE_FOLDER) ? "folder" : "message";
	
	/* Get device. Do not ask the platform factory for it, because
	   it returns always a new one */
	account_store = TNY_ACCOUNT_STORE (modest_runtime_get_account_store ());
	device = tny_account_store_get_device (account_store);

	if (g_main_depth > 0)	
		gdk_threads_enter ();
	online = tny_device_is_online (device);

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
			tny_device_force_online (device);
		}
	}
	gtk_widget_destroy (dialog);
	if (g_main_depth > 0)	
		gdk_threads_leave ();
}



void
modest_ui_actions_on_header_status_update (ModestHeaderView *header_view, 
					    const gchar *msg, gint num, 
					    gint total,  ModestMainWindow *main_window)
{
	char* txt;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	if (!main_window->progress_bar)
		return;

	if (total != 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(main_window->progress_bar),
					       (gdouble)num/(gdouble)total);
	else
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR(main_window->progress_bar));

	txt = g_strdup_printf (_("Downloading %d of %d"), num, total);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR(main_window->progress_bar), txt);
	g_free (txt);
	
	statusbar_push (main_window, 0, msg);
}


void
modest_ui_actions_on_msg_link_hover (ModestMsgView *msgview, const gchar* link,
				     ModestWindow *win)
{
	g_warning (__FUNCTION__);
}	


void
modest_ui_actions_on_msg_link_clicked (ModestMsgView *msgview, const gchar* link,
					ModestWindow *win)
{
	g_warning (__FUNCTION__);
}

void
modest_ui_actions_on_msg_attachment_clicked (ModestMsgView *msgview, int index,
					     ModestWindow *win)
{
	g_warning (__FUNCTION__);
	
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
	
	data = modest_msg_edit_window_get_msg_data (edit_window);

	/* FIXME: Code added just for testing. The final version will
	   use the send queue provided by tinymail and some
	   classifier */
	account_mgr = modest_runtime_get_account_mgr();
	account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no default account found\n");
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
		
	mail_operation = modest_mail_operation_new ();
	modest_mail_operation_send_new_mail (mail_operation,
					     transport_account,
					     from,
					     data->to, 
					     data->cc, 
					     data->bcc,
					     data->subject, 
					     data->body, 
					     NULL);
	/* Frees */
	g_free (from);
	g_free (account_name);
	g_object_unref (G_OBJECT (mail_operation));
	g_object_unref (G_OBJECT (transport_account));

	modest_msg_edit_window_free_msg_data (edit_window, data);

	/* Save settings and close the window */
	/* save_settings (edit_window) */
	gtk_widget_destroy (GTK_WIDGET (edit_window));
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
modest_ui_actions_on_new_folder (GtkWidget *widget, ModestMainWindow *main_window)
{
	TnyFolder *parent_folder;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	if (!main_window->folder_view)
		return;

	parent_folder = modest_folder_view_get_selected (main_window->folder_view);
	
	if (parent_folder) {
		gchar *folder_name;

		folder_name = ask_for_folder_name (GTK_WINDOW (main_window),
						   _("Please enter a name for the new folder"));

		if (folder_name != NULL && strlen (folder_name) > 0) {
			TnyFolder *new_folder;
			ModestMailOperation *mail_op;

			mail_op = modest_mail_operation_new ();
			new_folder = modest_mail_operation_create_folder (mail_op,
									  TNY_FOLDER_STORE (parent_folder),
									  (const gchar *) folder_name);
			if (new_folder) {
				/* TODO: tinymail should do this. 
				   Update view */
				modest_folder_view_add_subfolder (main_window->folder_view, new_folder);

				/* Free new folder */
				g_object_unref (new_folder);
			}
			g_object_unref (mail_op);
		}
		g_object_unref (parent_folder);
	}
}

void 
modest_ui_actions_on_rename_folder (GtkWidget *widget,
				     ModestMainWindow *main_window)
{
	TnyFolder *folder;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	if (!main_window->folder_view)
		return;
	
	folder = modest_folder_view_get_selected (main_window->folder_view);

	if (folder) {
		gchar *folder_name;

		folder_name = ask_for_folder_name (GTK_WINDOW (main_window),
						   _("Please enter a new name for the folder"));

		if (folder_name != NULL && strlen (folder_name) > 0) {
			ModestMailOperation *mail_op;
			const GError *error;

			mail_op = modest_mail_operation_new ();
			modest_mail_operation_rename_folder (mail_op,
							     folder,
							     (const gchar *) folder_name);

			error = modest_mail_operation_get_error (mail_op);
			if (!error)
				/* TODO: tinymail should do this. 
				   Update view */
				modest_folder_view_rename (main_window->folder_view);

			/* TODO: else ? notify error ? */

			g_object_unref (mail_op);
		}
		g_object_unref (folder);
	}
}

static void
delete_folder (ModestMainWindow *main_window, gboolean move_to_trash) 
{
	TnyFolder *folder;
	ModestMailOperation *mail_op;
	
	if (!main_window->folder_view)
		return;

	folder = modest_folder_view_get_selected (main_window->folder_view);

	mail_op = modest_mail_operation_new ();
	modest_mail_operation_remove_folder (mail_op, folder, move_to_trash);
	g_object_unref (mail_op);
}

void 
modest_ui_actions_on_delete_folder (GtkWidget *widget,
				     ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));

	delete_folder (main_window, FALSE);
}

void 
modest_ui_actions_on_move_folder_to_trash_folder (GtkWidget *widget, ModestMainWindow *main_window)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW(main_window));
	
	delete_folder (main_window, TRUE);
}

void
modest_ui_actions_on_accounts_reloaded (TnyAccountStore *store, gpointer user_data)
{
	/* FIXME */
	/* ModestFolderView *folder_view; */
	
/* 	folder_view = modest_widget_factory_get_folder_view (modest_runtime_get_widget_factory()); */
/* 	modest_folder_view_update_model (folder_view, store); */
}

void 
modest_ui_actions_on_folder_moved (ModestFolderView *folder_view,  TnyFolder *folder, 
				    TnyFolderStore *parent,  gboolean *done,
				    gpointer user_data)
{
	ModestMailOperation *mail_op;
	const GError *error;

	*done = TRUE;

	/* Try to move the folder */	
	mail_op = modest_mail_operation_new ();
	modest_mail_operation_move_folder (mail_op, folder, parent);

	error = modest_mail_operation_get_error (mail_op);
	if (error)
		*done = FALSE;

	g_object_unref (G_OBJECT (mail_op));
}

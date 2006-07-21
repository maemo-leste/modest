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


#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

/* TODO: put in auto* */
#include <tny-text-buffer-stream.h>
#include <tny-list.h>
#include <tny-msg-folder.h>

#include "../modest-ui.h"
#include "../modest-window-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-identity-mgr.h"

#include "../modest-tny-account-store.h"
#include "../widgets/modest-folder-view.h"
#include "../widgets/modest-header-view.h"
#include "../widgets/modest-msg-view.h"
#include "../modest-tny-transport-actions.h"
#include "../modest-tny-store-actions.h"

#include "../modest-text-utils.h"
#include "../modest-tny-msg-actions.h"

#include "../modest-editor-window.h"

#include "modest-ui-glade.h"
#include "modest-ui-wizard.h"
#include "modest-ui-account-setup.h"

#include "modest-ui-main-view.h"
#include "modest-ui-message-editor.h"
#include "modest-ui-message-viewer.h"


static GtkWidget* modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr,
						  TnyAccountStoreIface *account_store);

static GtkWidget* modest_main_window_header_tree (TnyMsgFolderIface *folder);

static void on_folder_clicked (ModestFolderView *folder_tree,
			       TnyMsgFolderIface *folder,
			       gpointer data);

static void on_message_clicked (ModestFolderView *folder_tree,
				TnyMsgIface *message,
				gpointer data);

static void on_reply_clicked (GtkWidget *widget, gpointer user_data);

static void on_forward_clicked (GtkWidget *widget, gpointer user_data);

static void on_delete_clicked (GtkWidget *widget, gpointer user_data);

static void on_sendreceive_button_clicked (GtkWidget *widget, gpointer user_data);

static void on_forward_attached_activated (GtkWidget *widget, gpointer user_data);

static void on_headers_status_update (GtkWidget *header_view, const gchar *msg, gint status,
			  gpointer user_data);
static void register_toolbar_callbacks (ModestUI *modest_ui);


static void
modest_ui_main_view_destroy (GtkWidget *win, GdkEvent *event, gpointer data)
{
	ModestUIPrivate *priv;

	g_return_if_fail (data);
	g_return_if_fail(MODEST_IS_UI(data));
	priv = MODEST_UI_GET_PRIVATE((ModestUI *)data);
	g_return_if_fail(priv);
	if (!modest_window_mgr_unregister (priv->modest_window_mgr, G_OBJECT(win)))
		g_warning ("modest window mgr: failed to unregister %p",
			   G_OBJECT(win));
	else
		gtk_widget_hide(win);
}
gboolean
modest_ui_show_main_window (ModestUI *modest_ui)
{
	GtkWidget       *win;
	gint              height, width;
	ModestUIPrivate *priv;
	GtkWidget     *folder_view, *header_view;
	GtkWidget     *message_view;
	GtkWidget     *account_settings_item;
	GtkWidget     *delete_item;
	GtkWidget     *open_item;
	GtkWidget     *forward_attached_menu_item;
	GtkWidget     *view_attachments_item;
	GtkWidget     *new_account_item;
	
	GtkWidget  *folder_view_holder,
		*header_view_holder,
		*mail_paned;

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	
	height = modest_conf_get_int (priv->modest_conf,
				      MODEST_MAIN_WINDOW_HEIGHT,NULL);
	width  = modest_conf_get_int (priv->modest_conf,
				      MODEST_MAIN_WINDOW_WIDTH,NULL);

	win = glade_xml_get_widget (priv->glade_xml, "main");
	if (!win) {
		g_warning ("could not create main window");
		return FALSE;
	}

	folder_view = GTK_WIDGET(modest_main_window_folder_tree(priv->modest_acc_mgr,
								priv->account_store));
	priv->folder_view = folder_view;
	folder_view_holder = glade_xml_get_widget (priv->glade_xml, "folders");
	if (!folder_view||!folder_view_holder) {
		g_warning ("failed to create folder tree");
		return FALSE;
	}
	gtk_container_add (GTK_CONTAINER(folder_view_holder), folder_view);
	
	header_view  =  GTK_WIDGET(modest_main_window_header_tree (NULL));
	priv->header_view = header_view;
	header_view_holder = glade_xml_get_widget (priv->glade_xml, "mail_list");
	if (!header_view) {
		g_warning ("failed to create header tree");
		return FALSE;
	}
	gtk_container_add (GTK_CONTAINER(header_view_holder), header_view);

	g_signal_connect (G_OBJECT(folder_view), "folder_selected",
 			  G_CALLBACK(on_folder_clicked), modest_ui);
	
	message_view  = GTK_WIDGET(modest_msg_view_new (NULL));
	priv->message_view = message_view;
	if (!message_view) {
		g_warning ("failed to create message view");
		return FALSE;
	}
	//g_signal_connect(G_OBJECT(message_view), "on_mailto_clicked",
        //            G_CALLBACK(ui_on_mailto_clicked), modest_ui);

	mail_paned = glade_xml_get_widget (priv->glade_xml, "mail_paned");
	gtk_paned_add2 (GTK_PANED(mail_paned), message_view);

	g_signal_connect (header_view, "message_selected",
			  G_CALLBACK(on_message_clicked),
                          modest_ui);

	g_signal_connect (header_view, "status_update",
			  G_CALLBACK(on_headers_status_update), modest_ui);

	g_signal_connect (header_view, "row-activated",
			  G_CALLBACK(on_message_activated),
                          modest_ui);

	account_settings_item = glade_xml_get_widget (priv->glade_xml, "AccountSettingsMenuItem");
	if (!account_settings_item) {
		g_warning ("The account settings item isn't available!\n");
		return FALSE;
	}

        g_signal_connect (account_settings_item, "activate",
                          G_CALLBACK(account_settings), modest_ui);

	new_account_item = glade_xml_get_widget (priv->glade_xml, "NewAccountWizardMenuItem");
	if (!new_account_item) {
		g_warning ("The new account item isn't available!\n");
		return FALSE;
	}

	g_signal_connect (new_account_item, "activate",
		G_CALLBACK(new_wizard_account), modest_ui);

	open_item = glade_xml_get_widget (priv->glade_xml, "open1");
	if (!open_item) {
		g_warning ("The open item isn't available!\n");
		return FALSE;
	}
	g_signal_connect (open_item, "activate", G_CALLBACK(on_open_message_clicked),
			  modest_ui);

	delete_item = glade_xml_get_widget (priv->glade_xml, "delete1");
	if (!delete_item) {
		g_warning ("The delete item isn't available!\n");
		return FALSE;
	}
	g_signal_connect (delete_item, "activate", G_CALLBACK(on_delete_clicked),
			  modest_ui);

	view_attachments_item = glade_xml_get_widget (priv->glade_xml, "menu_view_attachments");
	if (!view_attachments_item) {
		g_warning ("The view_attachments_item isn't available!");
		return FALSE;
	}

	forward_attached_menu_item = glade_xml_get_widget (priv->glade_xml, "forward_attached");
	if (!forward_attached_menu_item) {
		g_warning ("The forward_attached_menu_item isn't available!");
		return FALSE;
	}
	g_signal_connect (forward_attached_menu_item, "activate", G_CALLBACK(on_forward_attached_activated),
			  modest_ui);

	register_toolbar_callbacks (modest_ui);

	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_MAIN_WINDOW, 0);
	g_signal_connect (win, "destroy-event", G_CALLBACK(modest_ui_main_view_destroy),
			  modest_ui);
	g_signal_connect (win, "delete-event", G_CALLBACK(modest_ui_main_view_destroy),
			  modest_ui);
	gtk_widget_set_usize (GTK_WIDGET(win), width, height);
	gtk_window_set_title (GTK_WINDOW(win), PACKAGE_STRING);

	gtk_widget_show_all (win);
	return TRUE;
}


static void
register_toolbar_callbacks (ModestUI *modest_ui)
{
	ModestUIPrivate *priv;
	GtkWidget *button;

	g_return_if_fail (modest_ui);

	priv = MODEST_UI_GET_PRIVATE (modest_ui);

	button = glade_xml_get_widget (priv->glade_xml, "toolb_new_mail");
	if (button)
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_new_mail_clicked), modest_ui);

	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply");
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_reply_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply_all");
	if (button) {
		//g_signal_connect (button, "clicked",
		//		  G_CALLBACK(on_reply_all_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_forward");
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_forward_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_move_to");
	if (button) {
		//g_signal_connect (button, "clicked",
		//		  G_CALLBACK(on_move_to_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_delete");
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_delete_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_send_receive");
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_sendreceive_button_clicked), modest_ui);
		gtk_widget_set_sensitive(button, TRUE);
	}
}


static void
on_folder_clicked (ModestFolderView *folder_tree,
		   TnyMsgFolderIface *folder,
		   gpointer data)
{
	GtkWidget *win;
	GtkWidget *button;
	ModestHeaderView *tree_view;
	ModestMsgView *msg_view;
	ModestUIPrivate *priv;
	GtkWidget *scrollview;

	g_return_if_fail (folder);
	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE(data);
	scrollview = glade_xml_get_widget (priv->glade_xml,"mail_list");

	tree_view = MODEST_HEADER_VIEW (priv->header_view);

	win = glade_xml_get_widget (priv->glade_xml, "main");
	gtk_window_set_title (GTK_WINDOW(win),
			      tny_msg_folder_iface_get_name(folder));

	modest_header_view_set_folder (tree_view, folder);
	priv->current_folder = folder;

	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply");
	if (button) {
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_forward");
	if (button) {
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_delete");
	if (button) {
		gtk_widget_set_sensitive(button, FALSE);
	}

	msg_view = MODEST_MSG_VIEW (priv->message_view);
	g_return_if_fail (msg_view);

	modest_msg_view_set_message  (msg_view, NULL);
}


static void
on_message_clicked (ModestFolderView *folder_tree,
		    TnyMsgIface *message,
		    gpointer data)
{
	GtkWidget *button;
	ModestMsgView *msg_view;
	ModestUIPrivate *priv;

	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE (data);
	msg_view = MODEST_MSG_VIEW (priv->message_view);

	modest_msg_view_set_message (msg_view, message);
	
	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply");
	if (button) {
		gtk_widget_set_sensitive(button, TRUE);
	}
	button = glade_xml_get_widget (priv->glade_xml, "toolb_forward");
	if (button) {
		gtk_widget_set_sensitive(button, TRUE);
	}
	button = glade_xml_get_widget (priv->glade_xml, "toolb_delete");
	if (button) {
		gtk_widget_set_sensitive(button, TRUE);
	}
}


static GtkWidget*
modest_main_window_header_tree (TnyMsgFolderIface *folder)
{
	int i;
	GSList *columns = NULL;
	GtkWidget *header_tree;
	ModestHeaderViewColumn cols[] = {
		MODEST_HEADER_VIEW_COLUMN_MSGTYPE,
		MODEST_HEADER_VIEW_COLUMN_ATTACH,
		MODEST_HEADER_VIEW_COLUMN_FROM,
		MODEST_HEADER_VIEW_COLUMN_SUBJECT,
		MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE
	};

	for (i = 0 ; i != sizeof(cols) / sizeof(ModestHeaderViewColumn); ++i)
		columns = g_slist_append (columns, GINT_TO_POINTER(cols[i]));

	header_tree = GTK_WIDGET(modest_header_view_new(folder, columns,
							MODEST_HEADER_VIEW_STYLE_NORMAL));
	g_slist_free (columns);

	if (!header_tree) {
		g_warning ("could not create header tree");
		return NULL;
	}

	return GTK_WIDGET(header_tree);
}


static GtkWidget*
modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr,
				TnyAccountStoreIface *account_store)
{
	GtkWidget *folder_tree;

	folder_tree = GTK_WIDGET (modest_folder_view_new (account_store));
	if (!folder_tree) {
		g_warning ("could not create folder list");
		return NULL;
	}

	return folder_tree;
}


static void
on_reply_clicked (GtkWidget *widget, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *)user_data;

	quoted_send_msg (modest_ui, QUOTED_SEND_REPLY);
}


static void
on_forward_clicked (GtkWidget *widget, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *)user_data;

	quoted_send_msg (modest_ui, QUOTED_SEND_FORWARD);
}


static void
on_delete_clicked (GtkWidget *widget, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *)user_data;
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeModel *mymodel;

	ModestHeaderView *header_view;
	ModestMsgView *msg_view;
	ModestUIPrivate *priv;

	g_return_if_fail (modest_ui);

	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	msg_view = MODEST_MSG_VIEW(priv->message_view);
	g_return_if_fail (msg_view);

	header_view = MODEST_HEADER_VIEW(priv->header_view);
	g_return_if_fail (header_view);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(header_view));
	g_return_if_fail (sel);

	/* get all selected mails */
	if (G_LIKELY (gtk_tree_selection_get_selected (sel, &model, &iter))) {
		TnyMsgHeaderIface *header;

		gtk_tree_model_get (model, &iter, TNY_MSG_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			&header, -1);

		if (G_LIKELY (header)) {
			TnyMsgFolderIface *folder;
			// const TnyMsgIface *msg;

			if (GTK_IS_TREE_MODEL_SORT (model)) {
				mymodel = gtk_tree_model_sort_get_model
					(GTK_TREE_MODEL_SORT (model));
			} else
				mymodel = model;

			folder = (TnyMsgFolderIface*)tny_msg_header_iface_get_folder (header);

			/* this will make the message as deleted */
			/* 	tny_msg_folder_iface_expunge (folder); will finally delete messages */
			if (TNY_IS_MSG_FOLDER (folder))
				tny_msg_folder_iface_remove_message (folder, header);
			gtk_widget_queue_draw (GTK_WIDGET (header_view));
		}
	}
}


static void
on_sendreceive_button_clicked (GtkWidget *widget, gpointer user_data)
{
	ModestUI *modest_ui;
	ModestUIPrivate *priv;

	TnyListIface *store_accounts;
	TnyIteratorIface *iter;
	
	g_return_if_fail (user_data);

	modest_ui = MODEST_UI(user_data);
	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	store_accounts = tny_list_new (); 
	tny_account_store_iface_get_accounts (priv->account_store, store_accounts,
					      TNY_ACCOUNT_STORE_IFACE_STORE_ACCOUNTS);
	iter = tny_list_iface_create_iterator (store_accounts);
	
	if (tny_iterator_iface_has_first (iter)) {
		while (1) {
			TnyStoreAccountIface *account;
			account = TNY_STORE_ACCOUNT_IFACE(tny_iterator_iface_current(iter));
			modest_tny_store_actions_update_folders (account);
			
			if (!tny_iterator_iface_has_next (iter))
				break;
			
			tny_iterator_iface_next (iter);	
		}
		
	}
	g_object_unref (G_OBJECT(iter));
	g_object_unref (G_OBJECT(store_accounts));
		
	if (priv->header_view && priv->current_folder) {
		
		modest_header_view_set_folder (MODEST_HEADER_VIEW(priv->header_view),
					       priv->current_folder);
		gtk_widget_queue_draw (priv->header_view);
	}
}


static void
on_forward_attached_activated (GtkWidget *widget, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *)user_data;

	quoted_send_msg (modest_ui, QUOTED_SEND_FORWARD_ATTACHED);
}

static void
on_headers_status_update (GtkWidget *header_view, const gchar *msg, gint status_id,
			  gpointer user_data)
{
	ModestUIPrivate *priv;
	ModestUI *modest_ui;

	GtkStatusbar *status_bar;
	GtkProgressBar *progress_bar;
	GtkWidget    *status_box;
	
	modest_ui = MODEST_UI (user_data);
	priv      = MODEST_UI_GET_PRIVATE(modest_ui);

	progress_bar = GTK_PROGRESS_BAR(glade_xml_get_widget
					(priv->glade_xml, "progressbar"));
	status_bar   = GTK_STATUSBAR  (glade_xml_get_widget
				       (priv->glade_xml, "statusbar"));

	
	status_box   = glade_xml_get_widget (priv->glade_xml, "statusbox");
	
	
	if (!status_bar || !progress_bar) {
		g_warning ("failed to find status / progress bar");
		return;
	}

	if (msg && status_id) {
		gtk_widget_show (status_box);
		gtk_progress_bar_pulse (progress_bar);
		gtk_statusbar_push (status_bar, status_id, msg);
	} else {
		gtk_widget_hide   (status_box);
		gtk_statusbar_pop (status_bar, status_id);
	}
}		


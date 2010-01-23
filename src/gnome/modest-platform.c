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

#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomeui/gnome-icon-lookup.h>
#include <tny-gnome-device.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <tny-simple-list.h>
#include <tny-error.h>
#include <tny-merge-folder.h>
#include "modest-platform.h"
#include "modest-mail-operation-queue.h"
#include "modest-runtime.h"
#include "gnome/modest-gnome-global-settings-dialog.h"
#include "widgets/modest-default-account-settings-dialog.h"
#include "gnome/modest-account-assistant.h"
#include "gnome/modest-gnome-sort-dialog.h"
#include "widgets/modest-details-dialog.h"
#include "widgets/modest-main-window.h"

gboolean
modest_platform_init (int argc, char *argv[])
{	
	return TRUE; /* nothing to do */
}


gboolean modest_platform_uninit (void)
{
	return TRUE; /*nothing to do */
}


TnyDevice*
modest_platform_get_new_device (void)
{
	return TNY_DEVICE (tny_gnome_device_new ());
}


gchar*
modest_platform_get_file_icon_name (const gchar* name, const gchar* mime_type,
					  gchar **effective_mime_type)
{
	GString *mime_str = NULL;
	gchar *icon_name  = NULL;
	gchar *uri;
	const static gchar* octet_stream = "application/octet-stream";
	
	g_return_val_if_fail (name || mime_type, NULL);

	if (!mime_type || g_ascii_strcasecmp (mime_type, octet_stream)) 
		mime_str = g_string_new(gnome_vfs_mime_type_from_name_or_default
					(name, "application/octet-stream"));
	else {
		mime_str = g_string_new (mime_type);
		g_string_ascii_down (mime_str);
	}

	uri = g_strconcat ("file:///", name ? name : "dummy", NULL);
	icon_name  = gnome_icon_lookup (gtk_icon_theme_get_default(), NULL,
					uri, NULL, NULL, mime_str->str, 0, 0);
	g_free (uri);

	if (effective_mime_type)
		*effective_mime_type = g_string_free (mime_str, FALSE);
	else
		g_string_free (mime_str, TRUE);

	return icon_name;
}

gboolean 
modest_platform_activate_uri (const gchar *uri)
{
	g_warning ("NOT IMPLEMENTED");;
	return FALSE;
}

gboolean 
modest_platform_activate_file (const gchar *path, const gchar *mime_type)
{
	g_warning ("NOT IMPLEMENTED");;
	return FALSE;
}

gboolean 
modest_platform_show_uri_popup (const gchar *uri)
{
	g_warning ("NOT IMPLEMENTED");;
	return FALSE;
}

GdkPixbuf*
modest_platform_get_icon (const gchar *name, guint icon_size)
{
	GError *err = NULL;
	GdkPixbuf* pixbuf;

	g_return_val_if_fail (name, NULL);

	pixbuf = gdk_pixbuf_new_from_file (name, &err);

	if (!pixbuf) {
/* 		g_printerr ("modest: error while loading icon '%s': %s\n", */
/* 			    name, err->message); */
		g_error_free (err);
		err = NULL;

		pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), name, icon_size, 0, &err);
		if (!pixbuf) {
			g_error_free (err);
		}
	}
	
	return pixbuf;
}


const gchar*
modest_platform_get_app_name (void)
{
	return ("Modest");
}

gint 
modest_platform_run_new_folder_dialog (GtkWindow *parent_window,
				       TnyFolderStore *suggested_parent,
				       gchar *suggested_name,
				       gchar **folder_name,
				       TnyFolderStore **parent_folder)
{
	GtkWidget *dialog, *entry;
	gint result;

	/* Ask the user for the folder name */
	dialog = gtk_dialog_new_with_buttons (_("New Folder Name"),
					      parent_window,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    gtk_label_new (_("Please enter a name for the new folder")),
			    FALSE, FALSE, 0);
		
	entry = gtk_entry_new_with_max_length (40);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    entry,
			    TRUE, FALSE, 0);
	
	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));

	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT)
		*folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	gtk_widget_destroy (dialog);

	if (parent_folder != NULL) {
		parent_folder = suggested_parent?g_object_ref (suggested_parent): NULL;
	}

	return result;
}


gint
modest_platform_run_confirmation_dialog (GtkWindow *parent_window,
					 const gchar *msg)
{
	gint response;
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent_window,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_OK_CANCEL,
					 msg);

	response = gtk_dialog_run (GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);
	
	return response;
}

gint
modest_platform_run_confirmation_dialog_with_buttons (GtkWindow *parent_window,
						      const gchar *message,
						      const gchar *button_accept,
						      const gchar *button_cancel)
{
	gint response;
	GtkWidget *dialog;

	dialog = gtk_dialog_new_with_buttons (message,
					      parent_window,
					      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					      button_accept,
					      GTK_RESPONSE_ACCEPT,
					      button_cancel,
					      GTK_RESPONSE_CANCEL,
					      NULL);

	response = gtk_dialog_run (GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);
	
	return response;
}

void
modest_platform_run_information_dialog (GtkWindow *parent_window,
					const gchar *message,
					gboolean block)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent_window,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_INFO,
					 GTK_BUTTONS_OK,
					 message);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
}

gboolean modest_platform_connect_and_wait (GtkWindow *parent_window, TnyAccount *account)
{
	/* TODO: Do something with network-manager? 
	   Otherwise, maybe it is safe to assume that we would already be online if we could be. */
	return TRUE;
}

gboolean modest_platform_connect_and_wait_if_network_account (GtkWindow *parent_window, TnyAccount *account)
{
	/* TODO: Do something with network-manager? 
	   Otherwise, maybe it is safe to assume that we would already be online if we could be. */
	return TRUE;
}


void
modest_platform_connect_if_remote_and_perform (GtkWindow *parent_window, 
					       gboolean force,
					       TnyFolderStore *folder_store, 
					       ModestConnectedPerformer callback, 
					       gpointer user_data)
{
	TnyAccount *account = NULL;

	if (!folder_store ||
	    (TNY_IS_MERGE_FOLDER (folder_store) &&
	     (tny_folder_get_folder_type (TNY_FOLDER(folder_store)) == TNY_FOLDER_TYPE_OUTBOX))) {

 		/* We promise to instantly perform the callback, so ... */
 		if (callback) {
			GError *error = NULL;
			g_set_error (&error, TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_UNKNOWN,
				     "Unable to move or not found folder");
 			callback (FALSE, error, parent_window, NULL, user_data);
			g_error_free (error);
		}
 		return;

 	} else if (TNY_IS_FOLDER (folder_store)) {
 		/* Get the folder's parent account: */
 		account = tny_folder_get_account(TNY_FOLDER (folder_store));
 	} else if (TNY_IS_ACCOUNT (folder_store)) {
 		/* Use the folder store as an account: */
 		account = TNY_ACCOUNT (folder_store);
 	}
 
	if (tny_account_get_account_type (account) == TNY_ACCOUNT_TYPE_STORE) {
 		if (!TNY_IS_CAMEL_POP_STORE_ACCOUNT (account) &&
 		    !TNY_IS_CAMEL_IMAP_STORE_ACCOUNT (account)) {

 			/* This IS a local account like a maildir account, which does not require 
 			 * a connection. (original comment had a vague assumption in its language
 			 * usage. There's no assuming needed, this IS what it IS: a local account), */
 
 			/* We promise to instantly perform the callback, so ... */
 			if (callback) {
 				callback (FALSE, NULL, parent_window, account, user_data);
 			}
 			
 			return;
 		}
 	}
 
 	modest_platform_connect_and_perform (parent_window, force, account, callback, user_data);
 
 	return;
}


gboolean modest_platform_set_update_interval (guint minutes)
{
	/* TODO. */
	return FALSE;
}

GtkWidget *
modest_platform_create_sort_dialog (GtkWindow *parent_window)
{
	return modest_gnome_sort_dialog_new (parent_window);
}

GtkWidget *
modest_platform_get_global_settings_dialog ()
{
	return modest_gnome_global_settings_dialog_new ();
}

void
modest_platform_push_email_notification(void)
{
	/* TODO: implement this */
	g_print ("--------------- NEW MESSAGE ARRIVED ---------------\n");
}

void 
modest_platform_on_new_headers_received (GList *URI_list,
					 gboolean show_visual)
{
	/* TODO: implement this */
	g_print ("--------------- NEW MESSAGE ARRIVED ---------------\n");
}



void
modest_platform_show_help (GtkWindow *parent_window, const gchar *help_id)
{
	return; /* TODO */
}

void 
modest_platform_information_banner (GtkWidget *widget,
				    const gchar *icon_name,
				    const gchar *text)
{
	g_warning ("NOT IMPLEMENTED");;
}

void 
modest_platform_system_banner (GtkWidget *widget,
			       const gchar *icon_name,
			       const gchar *text)
{
	g_warning ("NOT IMPLEMENTED");;
}

void
modest_platform_information_banner_with_timeout (GtkWidget *widget,
						 const gchar *icon_name,
						 const gchar *text,
						 gint timeout)
{
	g_warning ("NOT IMPLEMENTED");;
}

GtkWidget *
modest_platform_animation_banner (GtkWidget *widget,
				  const gchar *icon_name,
				  const gchar *text)
{
	g_warning ("NOT IMPLEMENTED");
	return NULL;
}


void
modest_platform_show_search_messages (GtkWindow *parent_window)
{
	g_warning ("NOT IMPLEMENTED");;
}

GtkWidget *
modest_platform_create_folder_view (TnyFolderStoreQuery *query)
{
	GtkWidget *widget = modest_folder_view_new (query);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (widget), FALSE);
	/* Show all accounts by default */
	modest_folder_view_set_style (MODEST_FOLDER_VIEW (widget),
				      MODEST_FOLDER_VIEW_STYLE_SHOW_ALL);

	return widget;
}

gboolean
modest_platform_run_alert_dialog (const gchar* prompt, 
				  gboolean is_question)
{
	/* TODO */
	return TRUE;
}

void 
modest_platform_connect_and_perform (GtkWindow *parent_window, 
				     gboolean force,
				     TnyAccount *account, 
				     ModestConnectedPerformer callback, 
				     gpointer user_data)
{
	if (callback)
		callback (FALSE, NULL, parent_window, account, user_data);
}

void 
modest_platform_double_connect_and_perform (GtkWindow *parent_window, 
					    gboolean force,
					    TnyFolderStore *folder_store,
					    DoubleConnectionInfo *connect_info)
{
	if (connect_info->callback)
		connect_info->callback (FALSE, NULL, parent_window, TNY_ACCOUNT (folder_store), connect_info->data);
}

void 
modest_platform_connect_and_perform_if_network_account (GtkWindow *parent_window, 
							gboolean force,
							TnyAccount *account,
							ModestConnectedPerformer callback, 
							gpointer user_data)
{
	if (callback)
		callback (FALSE, NULL, parent_window, account, user_data);
}

void
modest_platform_connect_and_perform_if_network_folderstore (GtkWindow *parent_window, 
							    TnyFolderStore *folder_store, 
							    ModestConnectedPerformer callback, 
							    gpointer user_data)
{
	if (callback)
		callback (FALSE, NULL, parent_window, NULL, user_data);
}


void 
modest_platform_remove_new_mail_notifications (gboolean only_visuals, const gchar *acc_name)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
}

gboolean 
modest_platform_check_and_wait_for_account_is_online(TnyAccount *account)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return TRUE;
}

gboolean 
modest_platform_run_certificate_confirmation_dialog (const gchar* server_name,
						     const gchar *certificate)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return TRUE;
}

gint
modest_platform_run_rename_folder_dialog (GtkWindow *parent_window,
                                          TnyFolderStore *parent_folder,
                                          const gchar *suggested_name,
                                          gchar **folder_name)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return GTK_RESPONSE_CANCEL;
}

void 
modest_platform_show_addressbook (GtkWindow *parent_window)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
}

GtkWidget *
modest_platform_get_account_settings_dialog (ModestAccountSettings *settings)
{
	ModestAccountSettingsDialog *dialog = modest_default_account_settings_dialog_new ();

	modest_account_settings_dialog_load_settings (dialog, settings);
	return GTK_WIDGET (dialog);
}

GtkWidget *
modest_platform_get_account_settings_wizard (void)
{
	GtkWidget *widget = modest_account_assistant_new (modest_runtime_get_account_mgr ());
	return widget;
}



gboolean modest_platform_check_memory_low (ModestWindow *win,
					   gboolean visuals)
{
	g_debug ("%s not implemented", __FUNCTION__);
	return FALSE;
}

void 
modest_platform_run_folder_details_dialog (GtkWindow *parent_window,
					   TnyFolder *folder)
{
	GtkWidget *dialog;
	
	/* Create dialog */
	dialog = modest_details_dialog_new_with_folder (parent_window, folder);

	/* Run dialog */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), 
				     parent_window);
	gtk_widget_show_all (dialog);

	g_signal_connect_swapped (dialog, "response", 
				  G_CALLBACK (gtk_widget_destroy),
				  dialog);
}

void
modest_platform_run_header_details_dialog (GtkWindow *parent_window,
					   TnyHeader *header,
					   gboolean async_get_size, 
					   TnyMsg *msg)
{
	GtkWidget *dialog;
	
	/* Create dialog */
	dialog = modest_details_dialog_new_with_header (parent_window, header, TRUE);

	/* Run dialog */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog),
				     parent_window);
	gtk_widget_show_all (dialog);

	g_signal_connect_swapped (dialog, "response", 
				  G_CALLBACK (gtk_widget_destroy),
				  dialog);
}

GtkWidget* 
modest_platform_create_move_to_dialog (GtkWindow *parent_window,
				       GtkWidget **folder_view)
{
	GtkWidget *dialog, *folder_view_container;

	dialog = gtk_dialog_new_with_buttons (_("mcen_ti_moveto_folders_title"),
					      GTK_WINDOW (parent_window),
					      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR |
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      _("mcen_bd_dialog_ok"), GTK_RESPONSE_OK,
					      _("mcen_bd_new"), MODEST_GTK_RESPONSE_NEW_FOLDER,
					      _("mcen_bd_dialog_cancel"), GTK_RESPONSE_CANCEL,
	                                      NULL);

	/* Create folder view */
	*folder_view = modest_platform_create_folder_view (NULL);

	/* Create pannable and add it to the dialog */
	folder_view_container = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (folder_view_container),
					 GTK_POLICY_AUTOMATIC,
					 GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), folder_view_container);
	gtk_container_add (GTK_CONTAINER (folder_view_container), *folder_view);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 300);

	return dialog;
}

TnyList *
modest_platform_get_list_to_move (ModestWindow *window)
{
	TnyList *list = NULL;

	/* If it's a main window then it could be that we're moving a
	   folder or a set of messages */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		ModestHeaderView *header_view = NULL;
		ModestFolderView *folder_view = NULL;

		folder_view = (ModestFolderView *)
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		header_view = (ModestHeaderView *)
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

		/* Get folder or messages to transfer */
		if (gtk_widget_is_focus (GTK_WIDGET (folder_view))) {
			TnyFolderStore *src_folder;

			src_folder = modest_folder_view_get_selected (folder_view);
			if (src_folder) {
				list = tny_simple_list_new ();
				tny_list_prepend (list, G_OBJECT (src_folder));
				g_object_unref (src_folder);
			}
		} else if (gtk_widget_is_focus (GTK_WIDGET(header_view))) {
			list = modest_header_view_get_selected_headers(header_view);
		}
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		TnyHeader *header = NULL;

		/* We simply return the currently viewed message */
		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (window));
		if (header) {
			list = tny_simple_list_new ();
			tny_list_prepend (list, G_OBJECT (header));
			g_object_unref (header);
		}
	} else {
		g_return_val_if_reached (NULL);
	}

	return list;
}

DBusConnection*
modest_platform_get_dbus_connection (void)
{
	return NULL;
}

void
modest_platform_emit_folder_updated_signal (const gchar *account_id, const gchar *folder_id)
{
	return;
}


void
modest_platform_emit_account_created_signal (const gchar *account_id)
{
	return;
}

void
modest_platform_emit_account_removed_signal (const gchar *account_id)
{
	return;
}

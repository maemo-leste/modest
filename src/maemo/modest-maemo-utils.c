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

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif /*DBUS_API_SUBJECT_TO_CHANGE*/

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
#include <modest-runtime.h>
#include <libgnomevfs/gnome-vfs.h>
#include <tny-fs-stream.h>
#include <tny-camel-account.h>
#include <tny-status.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include "modest-hildon-includes.h"

#include "modest-maemo-utils.h"

/*
 * For getting and tracking the Bluetooth name
 */
#define BTNAME_SERVICE                  "org.bluez"
#define BTNAME_REQUEST_IF               "org.bluez.Adapter"
#define BTNAME_SIGNAL_IF                "org.bluez.Adapter"
#define BTNAME_REQUEST_PATH             "/org/bluez/hci0"
#define BTNAME_SIGNAL_PATH              "/org/bluez/hci0"

#define BTNAME_REQ_GET                  "GetName"
#define BTNAME_SIG_CHANGED              "NameChanged"

#define BTNAME_MATCH_RULE "type='signal',interface='" BTNAME_SIGNAL_IF \
                          "',member='" BTNAME_SIG_CHANGED "'"


GtkWidget*
modest_maemo_utils_menubar_to_menu (GtkUIManager *ui_manager)
{
	GtkWidget *main_menu;
	GtkWidget *menubar;
	GList *iter;

	g_return_val_if_fail (ui_manager, NULL);
	
	/* Create new main menu */
	main_menu = gtk_menu_new();

	/* Get the menubar from the UI manager */
	menubar = gtk_ui_manager_get_widget (ui_manager, "/MenuBar");

	iter = gtk_container_get_children (GTK_CONTAINER (menubar));
	while (iter) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent(menu, main_menu);

		iter = g_list_next (iter);
	}
	return main_menu;
}


static void
update_device_name_from_msg (DBusMessage *message)
{
	DBusError error;
	DBusMessageIter iter;

	dbus_error_init (&error);

	if (dbus_set_error_from_message (&error, message)) {
		g_printerr ("modest: failed to get bluetooth name: %s\n", error.message);
		dbus_error_free (&error);
	} else {
		const gchar *device_name;
		if (!dbus_message_iter_init (message, &iter)) {
			g_printerr ("modest: message did not have argument\n");
			return;
		}
		dbus_message_iter_get_basic (&iter, &device_name);
		g_warning ("update device name: %s", device_name);
		modest_conf_set_string (modest_runtime_get_conf(),
					MODEST_CONF_DEVICE_NAME, device_name,
					NULL);
	}
}


static void
on_device_name_received (DBusPendingCall *call, void *user_data)
{
	DBusMessage *message;
	
	g_return_if_fail (dbus_pending_call_get_completed (call));
	
	message = dbus_pending_call_steal_reply (call);
	if (!message) {
		g_printerr ("modest: no reply on device name query\n");
		return;
	}

	update_device_name_from_msg (message);
	dbus_message_unref (message);
}


static DBusHandlerResult
handle_dbus_signal (DBusConnection *conn, DBusMessage *msg, gpointer data)
{
	if (dbus_message_is_signal(msg, BTNAME_SIGNAL_IF, BTNAME_SIG_CHANGED))
		update_device_name_from_msg (msg);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static void
get_device_name_from_dbus ()
{
	static DBusConnection *conn = NULL;
	DBusMessage *request;
	DBusError error;
	DBusPendingCall *call = NULL;
	
	dbus_error_init (&error);
	if (!conn) {
		conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
		if (!conn) {
			g_printerr ("modest: cannot get on the dbus: %s: %s\n",
				    error.name, error.message);
			dbus_error_free (&error);
			return;
		}
	}
	
	request = dbus_message_new_method_call (BTNAME_SERVICE, BTNAME_REQUEST_PATH,
						BTNAME_REQUEST_IF, BTNAME_REQ_GET);
	if (!request) {
		/* should we free the connection? */
		g_printerr ("modest: dbus_message_new_method_call failed\n");
		return;
	}
	dbus_message_set_auto_start (request, TRUE);
	if (dbus_connection_send_with_reply (conn, request, &call, -1)) {
		dbus_pending_call_set_notify (call, on_device_name_received,
					      NULL, NULL);
		dbus_pending_call_unref (call);
	}
	dbus_message_unref (request);
	
	dbus_connection_setup_with_g_main (conn, NULL);
	dbus_bus_add_match (conn, BTNAME_MATCH_RULE, &error);
	if (dbus_error_is_set(&error)) {
		g_printerr ("modest: dbus_bus_add_match failed: %s\n", error.message);
		dbus_error_free (&error);
	}

	if (!dbus_connection_add_filter(conn, handle_dbus_signal, NULL, NULL))
		g_printerr ("modest: dbus_connection_add_filter failed\n");
}


void
modest_maemo_utils_get_device_name (void)
{
	get_device_name_from_dbus ();
}

gboolean 
modest_maemo_utils_folder_writable (const gchar *filename)
{
	if (g_strncasecmp (filename, "obex", 4) != 0) {
		GnomeVFSFileInfo folder_info;
		gchar *folder;
		folder = g_path_get_dirname (filename);
		gnome_vfs_get_file_info (folder, &folder_info,
					 GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);
		g_free (folder);
		if (!((folder_info.permissions & GNOME_VFS_PERM_ACCESS_WRITABLE) ||
		      (folder_info.permissions & GNOME_VFS_PERM_USER_WRITE))) {
			return FALSE;
		}
	}
	return TRUE;
}

gboolean 
modest_maemo_utils_file_exists (const gchar *filename)
{
	GnomeVFSURI *uri = NULL;
	gboolean result = FALSE;

	uri = gnome_vfs_uri_new (filename);
	if (uri) {
		result = gnome_vfs_uri_exists (uri);
		gnome_vfs_uri_unref (uri);
	}
	return result;
}

TnyFsStream *
modest_maemo_utils_create_temp_stream (const gchar *extension, gchar **path)
{
	TnyStream *tmp_fs_stream = NULL;
	gint fd;
	gchar *filepath = NULL;
	gchar *template = NULL;

	if (extension != NULL)
		template = g_strdup_printf ("XXXXXX.%s", extension);

	fd = g_file_open_tmp (template, &filepath, NULL);
	g_free (template);
	if (path != NULL)
		*path = filepath;
	if (fd == -1) {
		g_message ("TODO BANNER: Error saving stream");
		return NULL;
	}
	tmp_fs_stream = tny_fs_stream_new (fd);
	
	return TNY_FS_STREAM (tmp_fs_stream);
}

typedef struct 
{
	gboolean finished;
	gboolean cancel;
	GList *result;
	GtkWidget* dialog;
  GtkWidget* progress;
} ModestGetSupportedAuthInfo;

static void on_camel_account_get_supported_secure_authentication_status (
	GObject *self, TnyStatus *status, gpointer user_data)
{
	/*ModestGetSupportedAuthInfo* info = (ModestGetSupportedAuthInfo*) user_data;*/
}

static void
on_camel_account_get_supported_secure_authentication (
  TnyCamelAccount *self, gboolean cancelled,
  TnyList *auth_types, GError **err, 
  gpointer user_data)
{
		
	ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
	g_return_if_fail (info);
	
	/* Free everything if the actual action was canceled */
	if (info->cancel)
	{
		g_slice_free (ModestGetSupportedAuthInfo, info);
		info = NULL;
		return;
	}

	if (!auth_types) {
		printf ("DEBUG: %s: auth_types is NULL.\n", __FUNCTION__);
		info->finished = TRUE; /* We are blocking, waiting for this. */
		return;
	}
		
	ModestPairList* pairs = modest_protocol_info_get_auth_protocol_pair_list ();
  
	/* Get the enum value for the strings: */
	GList *result = NULL;
	TnyIterator* iter = tny_list_create_iterator(auth_types);
	while (!tny_iterator_is_done(iter)) {
		const gchar *auth_name = tny_pair_get_name(TNY_PAIR(tny_iterator_get_current(iter)));
		printf("DEBUG: %s: auth_name=%s\n", __FUNCTION__, auth_name);
		ModestPair *matching = modest_pair_list_find_by_first_as_string (pairs, 
			auth_name);
		if (matching)
    {
      result = g_list_append (result, GINT_TO_POINTER((ModestConnectionProtocol)matching->first));
    }
		tny_iterator_next(iter);
	}
	
  g_object_unref(auth_types);
	
	modest_pair_list_free (pairs);
	
	info->result = result;
  printf("DEBUG: finished\n");
	info->finished = TRUE; /* We are blocking, waiting for this. */
}

static void on_secure_auth_cancel(GtkWidget* dialog, int response, gpointer user_data)
{
	ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
	g_return_if_fail(info);
	/* We are blocking */
	info->result = NULL;
	info->cancel = TRUE;
}

GList* modest_maemo_utils_get_supported_secure_authentication_methods (ModestTransportStoreProtocol proto, 
	const gchar* hostname, gint port, GtkWindow *parent_window)
{
	g_return_val_if_fail (proto != MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN, NULL);
	
	/*
	result = g_list_append (result, GINT_TO_POINTER (MODEST_PROTOCOL_AUTH_CRAMMD5));
	*/
	
	/* Create a TnyCamelAccount so we can use 
	 * tny_camel_account_get_supported_secure_authentication(): */
	TnyAccount * tny_account = NULL;
	switch (proto) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		tny_account = TNY_ACCOUNT(tny_camel_transport_account_new ()); break;
	case MODEST_PROTOCOL_STORE_POP:
		tny_account = TNY_ACCOUNT(tny_camel_pop_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_IMAP:
		tny_account = TNY_ACCOUNT(tny_camel_imap_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		tny_account = TNY_ACCOUNT(tny_camel_store_account_new()); break;
	default:
		tny_account = NULL;
	}
	
	if (!tny_account) {
		g_printerr ("%s could not create tny account.", __FUNCTION__);
		return NULL;
	}
	
	/* Set proto, so that the prepare_func() vfunc will work when we call 
	 * set_session(): */
	 /* TODO: Why isn't this done in account_new()? */
	tny_account_set_proto (tny_account,
			       modest_protocol_info_get_transport_store_protocol_name(proto));
			       
	/* Set the session for the account, so we can use it: */
	ModestTnyAccountStore *account_store = modest_runtime_get_account_store ();
	TnySessionCamel *session = 
		modest_tny_account_store_get_session (TNY_ACCOUNT_STORE (account_store));
	g_return_val_if_fail (session, NULL);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	tny_account_set_hostname (tny_account, hostname);
	
	if(port > 0)
		tny_account_set_port (tny_account, port);
		

	/* Ask camel to ask the server, asynchronously: */
	ModestGetSupportedAuthInfo *info = g_slice_new (ModestGetSupportedAuthInfo);
	info->finished = FALSE;
	info->result = NULL;
	info->cancel = FALSE;
	info->progress = gtk_progress_bar_new();
  info->dialog = gtk_dialog_new_with_buttons(_("Authentication"),
																parent_window, GTK_DIALOG_MODAL,
																GTK_STOCK_CANCEL,
																GTK_RESPONSE_REJECT,
																NULL);
	//gtk_window_set_default_size(GTK_WINDOW(info->dialog), 300, 100);
	
	g_signal_connect(G_OBJECT(info->dialog), "response", G_CALLBACK(on_secure_auth_cancel), info);
	
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox),
										gtk_label_new("Checking for supported authentication types..."));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox), info->progress);
	gtk_widget_show_all(info->dialog);
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(info->progress));
	
	printf ("DEBUG: %s: STARTING.\n", __FUNCTION__);
	tny_camel_account_get_supported_secure_authentication (
		TNY_CAMEL_ACCOUNT (tny_account),
		on_camel_account_get_supported_secure_authentication,
		on_camel_account_get_supported_secure_authentication_status,
		info);
		
	/* Block until the callback has been called,
	 * driving the main context, so that the (idle handler) callback can be 
	 * called, and so that our dialog is clickable: */
	while (!(info->finished) && !(info->cancel)) {
    gtk_main_iteration_do(FALSE); 
	}
	
  gtk_widget_destroy(info->dialog);
		
	GList *result = info->result;
	if (!info->cancel)
	{
		g_slice_free (ModestGetSupportedAuthInfo, info);
		info = NULL;
	}
	return result;
}

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
#include <glib/gstdio.h>
#include <errno.h>
#include <string.h> /* for strlen */
#include <modest-runtime.h>
#include <libgnomevfs/gnome-vfs.h>
#include <tny-fs-stream.h>
#include <tny-camel-account.h>
#include <tny-status.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include "modest-hildon-includes.h"

#include <modest-defs.h>
#include "modest-maemo-utils.h"
#include "modest-text-utils.h"
#include "modest-platform.h"

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


static osso_context_t *__osso_context = NULL; /* urgh global */

osso_context_t *
modest_maemo_utils_get_osso_context (void)
{
	if (!__osso_context) 
		g_warning ("%s: __osso_context == NULL", __FUNCTION__);
	
	return __osso_context;
}

void
modest_maemo_utils_set_osso_context (osso_context_t *osso_context)
{
	g_return_if_fail (osso_context);
	__osso_context = osso_context;
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

void
modest_maemo_utils_setup_images_filechooser (GtkFileChooser *chooser)
{
	gchar *images_folder;
	GtkFileFilter *file_filter;
	GList *image_mimetypes_list;
	GList *node;

	g_return_if_fail (GTK_IS_FILE_CHOOSER (chooser));

	/* Set the default folder to images folder */
	images_folder = g_build_filename (g_get_home_dir (), 
					  MODEST_MAEMO_UTILS_MYDOCS_FOLDER,
					  MODEST_MAEMO_UTILS_DEFAULT_IMAGE_FOLDER, NULL);
	gtk_file_chooser_set_current_folder (chooser, images_folder);
	g_free (images_folder);

	/* Set the images mime filter */
	file_filter = gtk_file_filter_new ();
	image_mimetypes_list = hildon_mime_get_mime_types_for_category (HILDON_MIME_CATEGORY_IMAGES);
	for (node = image_mimetypes_list; node != NULL; node = g_list_next (node)) {
		gtk_file_filter_add_mime_type (file_filter, node->data);
	}
	gtk_file_chooser_set_filter (chooser, file_filter);
	hildon_mime_types_list_free (image_mimetypes_list);
}

void
modest_maemo_set_thumbable_scrollbar (GtkScrolledWindow *win, 
				      gboolean thumbable)
{
	g_return_if_fail (GTK_IS_SCROLLED_WINDOW(win));
#ifdef MODEST_HAVE_HILDON1_WIDGETS		
	hildon_helper_set_thumb_scrollbar (win, thumbable);
#endif /* MODEST_HAVE_HILDON1_WIDGETS */
}

GtkWidget *
modest_maemo_utils_get_manager_menubar_as_menu (GtkUIManager *manager,
						const gchar *item_name)
{
	GtkWidget *new_menu;
	GtkWidget *menubar;
	GList *children, *iter;

	menubar = gtk_ui_manager_get_widget (manager, item_name);
	new_menu = gtk_menu_new ();

	children = gtk_container_get_children (GTK_CONTAINER (menubar));
	for (iter = children; iter != NULL; iter = g_list_next (iter)) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent (menu, new_menu);
	}
	
	g_list_free (children);

	return new_menu;
}

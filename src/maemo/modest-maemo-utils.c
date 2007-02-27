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


static void
update_device_name_from_msg (DBusMessage *message)
{
	DBusError error;
	DBusMessageIter iter;

	dbus_error_init (&error);

	if (dbus_set_error_from_message (&error, message)) {
		g_printerr ("modest: failed to get bt name: %s\n", error.message);
		dbus_error_free (&error);
		modest_conf_set_string (modest_runtime_get_conf(),
					MODEST_CONF_DEVICE_NAME,
					MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME,
					NULL);
					
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

	g_warning ("get device name from dbus");
	
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




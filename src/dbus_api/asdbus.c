/* Copyright (c) 2006, 2008 Nokia Corporation
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

#include "asdbus.h"
#include "asdbus-bindings.h"
#include <glib-object.h>
#include "modest-runtime.h"
#include "modest-protocol.h"
#include "modest-protocol-registry.h"
#include "modest-account-mgr.h"


static
void recipient_iter (const GValue *value, gpointer user_data)
{
	AsDbusRecipient *recipient = g_new0 (AsDbusRecipient, 1);
	g_return_if_fail (dbus_g_type_is_struct (G_VALUE_TYPE (value)));
	g_return_if_fail (3 >= dbus_g_type_get_struct_size (G_VALUE_TYPE (value)));
	g_return_if_fail (G_TYPE_STRING == dbus_g_type_get_struct_member_type (G_VALUE_TYPE (value), 1));
	g_return_if_fail (G_TYPE_STRING == dbus_g_type_get_struct_member_type (G_VALUE_TYPE (value), 2));
	g_return_if_fail (dbus_g_type_struct_get (value,
			1, &recipient->display_name,
			2, &recipient->email_address,
			G_MAXUINT));
	*((GList **)user_data) = g_list_append (*((GList **)user_data), recipient);
}


static
void response_iter (const GValue *value, gpointer user_data)
{
	GValue recipients = {0,};
	g_return_if_fail (dbus_g_type_is_struct (G_VALUE_TYPE (value)));
	g_return_if_fail (4 >= dbus_g_type_get_struct_size (G_VALUE_TYPE (value)));
	g_return_if_fail (dbus_g_type_is_collection (dbus_g_type_get_struct_member_type (G_VALUE_TYPE (value), 3)));
	g_return_if_fail (dbus_g_type_struct_get_member (value, 3, g_value_init (&recipients, dbus_g_type_get_struct_member_type (G_VALUE_TYPE (value), 3))));
	dbus_g_type_collection_value_iterate (&recipients, recipient_iter, user_data);
}


/* assuming g_type_init is already called */
GList * asdbus_resolve_recipients (const gchar *name)
{
	GList *result = NULL;
	DBusGConnection *bus = NULL;
	GError *error = NULL;
	DBusGProxy *asdbus = NULL;
	GValueArray* reply = NULL;
	ModestProtocol *protocol = NULL;
	ModestProtocolType protocol_type;

	protocol = modest_protocol_registry_get_protocol_by_name (
			modest_runtime_get_protocol_registry (),
			MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS,  // MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS ?
			"activesync");
	protocol_type = modest_protocol_get_type_id (protocol);
    if (!modest_account_mgr_singleton_protocol_exists(modest_runtime_get_account_mgr(), protocol_type)) {
		return NULL;
	}

	const gchar *names[2];
	names[0] = name;
	names[1] = NULL;

	bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (bus == NULL) {
		/** FIXME: proper log */
		g_printerr ("Failed to open connection to bus: %s\n", error->message);
		g_error_free (error);
		goto CLEANUP;
	}

	asdbus = dbus_g_proxy_new_for_name (bus, "com.nokia.asdbus", "/com/nokia/asdbus", "com.nokia.asdbus");

	if (!com_nokia_asdbus_resolve_recipients (asdbus, names, &reply, &error)) {
		g_printerr ("com_nokia_asdbus_many_args failed: %s\n", error->message);
		g_error_free (error);
		goto CLEANUP;
	}

	if (!reply) goto CLEANUP;
	if (2 != reply->n_values) goto CLEANUP;
	if (G_TYPE_INT != G_VALUE_TYPE (&reply->values[0])) goto CLEANUP;
	if (!dbus_g_type_is_collection (G_VALUE_TYPE (&reply->values[1]))) goto CLEANUP;

	dbus_g_type_collection_value_iterate (&reply->values[1], response_iter, &result);

CLEANUP:
	g_value_array_free (reply);
	g_object_unref (asdbus);

	return result;
}

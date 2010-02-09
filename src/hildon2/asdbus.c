#include "asdbus.h"
#include "asdbus-bindings.h"

#include <glib-object.h>
#include <asconfig.h>


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

GList * asdbus_resolve_recipients (const gchar *name)
{
	GList *result = NULL;
	DBusGConnection *bus = NULL;
	GError *error = NULL;
	DBusGProxy *asdbus = NULL;
	GValueArray* reply = NULL;


	///g_type_init ();

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

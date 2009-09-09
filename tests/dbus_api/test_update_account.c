#include <libmodest-dbus-client/libmodest-dbus-api.h>
#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
	osso_context_t *osso_context;

	osso_context = osso_initialize ("test_update_account",
					"0.0.1",
					TRUE,
					NULL);
	       
	if (osso_context == NULL) {
		g_printerr ("osso_initialize() failed.\n");
	    return -1;
	}

	return libmodest_dbus_client_send_and_receive (osso_context);
}

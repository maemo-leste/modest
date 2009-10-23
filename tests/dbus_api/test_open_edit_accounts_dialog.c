#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
	osso_context_t *osso_context;
	gboolean ret;

	osso_context = osso_initialize ("test_open_default_inbox",
					"0.0.1",
					TRUE,
					NULL);

	if (osso_context == NULL) {
		g_printerr ("osso_initialize() failed.\n");
	    return -1;
	}

	ret = libmodest_dbus_client_open_edit_accounts_dialog (osso_context);

	if (!ret) {
		g_printerr ("libmodest_dbus_client_open_default_inbox() failed.\n");
	} else {
		g_print ("libmodest_dbus_client_open_default_inbox() succeeded.\n");
	}

    return ret ? 0 : -1;
}

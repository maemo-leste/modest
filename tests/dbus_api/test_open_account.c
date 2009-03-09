#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
	osso_context_t *osso_context;
	const char *url = NULL;
      	gboolean ret;

	osso_context = osso_initialize ("test_open_account",
					"0.0.1",
					TRUE,
					NULL);

	if (osso_context == NULL) {
		g_printerr ("osso_initialize() failed.\n");
	    return -1;
	}

	if (argc == 2) {
		url = argv[1];
	} else {
		g_printerr ("No account ID argument supplied on the command line.\n");
		return -1;
	}

	g_print ("Trying to open msg: %s\n", url);
	ret = libmodest_dbus_client_open_account (osso_context, url);

	if (!ret) {
		g_printerr ("libmodest_dbus_client_open_account () failed.\n");
	} else {
		g_print ("libmodest_dbus_client_open_account () succeeded.\n");
	}

    return ret ? 0 : -1;
}

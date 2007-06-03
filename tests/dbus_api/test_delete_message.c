#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
	osso_context_t *osso_context;
	const char *url;
      	gboolean ret; 

	osso_context = osso_initialize ("test_delete_msg",
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
		url = "local://???FIXME???";
	}

	g_print ("Trying to delete msg: %s\n", url);
	ret = libmodest_dbus_client_delete_message (osso_context,
						    url);

	if (!ret) {
		g_printerr ("libmodest_dbus_client_delete_message() failed.\n");
	} else {
		g_print ("libmodest_dbus_client_delete_message() succeeded.\n");
	}
		
    return ret ? 0 : -1;

}

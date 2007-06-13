#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
	osso_context_t *osso_context;
	const char *url;
      	gboolean ret; 

	osso_context = osso_initialize ("test_open_msg",
					"0.0.1",
					TRUE,
					NULL);
	       
	if (osso_context == NULL) {
		g_printerr ("osso_initialize() failed.\n");
	    return -1;
	}
	
	/* For instance, 
	 * "pop://murray.cumming%40gmail.com@pop.gmail.com:995/;use_ssl=wrapped/inbox/GmailId112e166949157685"
	 */
	if (argc == 2) {
		url = argv[1];
	} else {
		/* TODO: Add some test DBus method to get a valid URL for a message, 
		 * just so we can test this method. */
		g_printerr ("No email URL argument supplied on the command line.\n");
		return -1;
	}

	g_print ("Trying to open msg: %s\n", url);
	ret = libmodest_dbus_client_open_message (osso_context,
						  url);

	if (!ret) {
		g_printerr ("libmodest_dbus_client_open_message() failed.\n");
	} else {
		g_print ("libmodest_dbus_client_open_message() succeeded.\n");
	}
		
    return ret ? 0 : -1;

}

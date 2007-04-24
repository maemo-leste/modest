#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	/* Initialize maemo application */
	osso_context_t * osso_context = osso_initialize(
	    "test_hello", "0.0.1", TRUE, NULL);
	       
	/* Check that initialization was ok */
	if (osso_context == NULL)
	{
		printf("osso_initialize() failed.\n");
	    return OSSO_ERROR;
	}
	
	/* Call the function in libmodest-dbus-client: */
	const gboolean ret = libmodest_dbus_client_call_helloworld (osso_context);
	if (!ret) {
			printf("libmodest_dbus_client_call_helloworld() failed.\n");
		return OSSO_ERROR;
	} else {
		printf("libmodest_dbus_client_call_helloworld() succeeded.\n");
	}
		
    /* Exit */
    return 0;
}

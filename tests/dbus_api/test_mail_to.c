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
	const gboolean ret = libmodest_dbus_client_mail_to (osso_context,
		"mailto:nomail@nodomain.com?subject=test%20subject%20via%20dbus&body=test%20body%20via%20dbus");
	if (!ret) {
		printf("libmodest_dbus_client_mail_to() failed.\n");
		return OSSO_ERROR;
	} else {
		printf("libmodest_dbus_client_mail_to() succeeded.\n");
	}
		
    /* Exit */
    return 0;
}

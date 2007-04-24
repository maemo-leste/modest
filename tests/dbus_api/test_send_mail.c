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
	const gboolean ret = libmodfest_dbus_client_send_mail (osso_context,
		"murrayc@murrayc.com", /* to */
		NULL, /* cc */
		NULL, /* bcc */
		"test subject", /* subject */
		"test body\nline two", /* body */
		NULL);
	if (!ret) {
			printf("libmodest_dbus_client_call_helloworld() failed.\n");
		return OSSO_ERROR;
	} else {
		printf("libmodest_dbus_client_call_helloworld() succeeded.\n");
	}
		
    /* Exit */
    return 0;
}

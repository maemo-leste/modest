#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
	GSList *attachments = NULL;
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
	
	attachments = g_slist_append(attachments, "/usr/include/math.h,/usr/include/malloc.h");
	
	const gboolean ret = libmodest_dbus_client_compose_mail (
		osso_context,
		"modesttest@openismus.com", /* to */
		"cc test", /* cc */
		"bcc test", /* bcc */
		"test subject", /* subject */
		"test body\nline two", /* body */
		attachments);
		
	
	if (!ret) {
			printf("libmodest_dbus_client_open_message() failed.\n");
		return OSSO_ERROR;
	} else {
		printf("libmodest_dbus_client_open_message() succeeded!\n");
	}
		
    /* Exit */
    return 0;
}

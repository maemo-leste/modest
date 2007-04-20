#include <libosso.h>
#include <stdio.h>

#define MODEST_DBUS_NAME    "modestemail"
#define MODEST_DBUS_EXAMPLE_MESSAGE "HelloWorld"



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
	
	osso_rpc_t retval;
	osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_EXAMPLE_MESSAGE, &retval, DBUS_TYPE_INVALID);
	if (ret != OSSO_OK) {
			printf("osso_rpc_run() failed.\n");
		return OSSO_ERROR;
	} else {
		printf("osso_rpc_run() succeeded.\n");
	}
		
	osso_rpc_free_val(&retval);
		
    /* Exit */
    return 0;
}

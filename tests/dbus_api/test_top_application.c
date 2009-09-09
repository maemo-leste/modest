#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <libmodest-dbus-client/libmodest-dbus-api.h>
#include <libosso.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
      	
	osso_context_t * osso_context = osso_initialize ("test_open_default_inbox",
					"0.0.1",
					TRUE,
					NULL);
	       
	if (osso_context == NULL) {
		g_printerr ("osso_initialize() failed.\n");
	    return -1;
	}

	osso_rpc_t retval;
	const osso_return_t ret = osso_rpc_run_with_defaults(osso_context, 
		   MODEST_DBUS_NAME, 
		   MODEST_DBUS_METHOD_TOP_APPLICATION, &retval, 
		   DBUS_TYPE_INVALID);
		
	if (ret != OSSO_OK) {
		printf("debug: %s: osso_rpc_run() failed.\n", __FUNCTION__);
		return FALSE;
	} else {
		printf("debug: %s: osso_rpc_run() succeeded.\n", __FUNCTION__);
	}
	
	osso_rpc_free_val(&retval);
	
	return TRUE;
		
	return ret ? 0 : -1;
}

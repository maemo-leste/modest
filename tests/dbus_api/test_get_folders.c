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
	
	GList *list = NULL;
	const gboolean ret = libmodest_dbus_client_get_folders (
		osso_context, &list);
		
	if (!ret) {
		printf("libmodest_dbus_client_get_folders() failed.\n");
		return OSSO_ERROR;
	} else {
		printf("libmodest_dbus_client_get_folders() succeeded\n");
	}
	
	if (list) {
		GList *iter = NULL;
		for (iter = list; iter; iter = iter->next) {
			ModestFolderResult *item = (ModestFolderResult*)iter->data;	
			if (item) {
				printf("  Folder name=%s\n", item->folder_name);
			}
		}
	
		modest_folder_result_list_free (list);
	} else {
		printf("  The list of folders was empty.\n");	
	}	
	
    /* Exit */
    return 0;
}

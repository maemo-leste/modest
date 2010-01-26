#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

int main (int argc, char *argv[])
{
	osso_context_t *osso_context;
	gboolean res;
	GList *hits, *iter;
	gint number;

	osso_context = osso_initialize ("test_search",
					"0.0.1",
					TRUE,
					NULL);


	/* Check that initialization was ok */
	if (osso_context == NULL) {
		g_printerr ("osso_initialize() failed.\n");
		return OSSO_ERROR;
	}

	hits = NULL;

	if (argc == 2) {
	  number = strtol (argv[1], NULL, 10);
	} else {
		number = 10;
	}

	g_print ("Starting get_unread_messages)...\n");

	res = libmodest_dbus_client_get_unread_messages (osso_context,
							 number,
							 &hits);

	g_print ("Search done. (success: %s)\n", res ? "yes" : "no");

	for (iter = hits; iter; iter = iter->next) {
		ModestAccountHits *hits = (ModestAccountHits *) iter->data;
		GList *header_node;

		g_print ("Account: id: %s name: %s\n", hits->account_id, hits->account_name);
		for (header_node = hits->hits; header_node != NULL; header_node = g_list_next (header_node)) {
			ModestGetUnreadMessagesHit *hit = (ModestGetUnreadMessagesHit *) header_node->data;

			g_print ("    %s\n    %s\n", hit->subject, ctime (&(hit->timestamp)));
		}
	}
	modest_account_hits_list_free (hits);

	return res ? 0 : -1;
}

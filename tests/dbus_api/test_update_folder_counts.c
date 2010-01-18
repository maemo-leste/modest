#include <libmodest-dbus-client/libmodest-dbus-client.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

static gchar *account_id = NULL;

static GOptionEntry option_entries [] =
{
	{ "account", 'a', 0, G_OPTION_ARG_STRING, &account_id, "Account to perform update folder accounts into", NULL },
	{ NULL }
};


int
main (int argc, char *argv[])
{
	osso_context_t *osso_context;
	GOptionContext *context;
	gboolean result;
	GError *error = NULL;

	context = g_option_context_new ("- Modest email client");
	g_option_context_add_main_entries (context, option_entries, NULL);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_print ("option parsing failed: %s\n", error->message);
		g_option_context_free (context);
		exit (1);
	}
	g_option_context_free (context);

	osso_context = osso_initialize ("test_update_account",
					"0.0.1",
					TRUE,
					NULL);
	       
	if (osso_context == NULL) {
		g_printerr ("osso_initialize() failed.\n");
	    return -1;
	}

	if (account_id == NULL) {
		g_printerr ("Provide an account id\n");
		return -1;
	}

	result = libmodest_dbus_client_update_folder_counts (osso_context, account_id);
	g_free (account_id);

	return result;

}

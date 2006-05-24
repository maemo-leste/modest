/* modest-main.c -- part of modest */
#include <glib.h>

#include "modest-conf.h"
#include "modest-account-mgr.h"
#include "modest-ui.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <gtk/gtk.h>

static void install_basic_conf_settings (ModestConf *conf);
static void install_test_account        (ModestConf *conf);


int
main (int argc, char *argv[])
{
	GOptionContext   *context        = NULL;
	ModestConf       *modest_conf    = NULL;
	ModestUI         *modest_ui      = NULL;

	GError *err = NULL;
	int retval  = 0;

	static gboolean update, debug, reinstall;
	static gchar *mailto, *subject, *bcc, *cc, *body;

	static GOptionEntry options[] = {
		{ "debug",  'd', 0, G_OPTION_ARG_NONE, &debug,
		  "Run in debug mode" },
		{ "update", 'u', 0, G_OPTION_ARG_NONE, &update,
		  "Send/receive all accounts and exit"},
		{ "mailto", 'm', 0, G_OPTION_ARG_STRING, &mailto,
		  "Start writing a new email to <addresses>"},
		{ "subject", 's', 0, G_OPTION_ARG_STRING, &subject,
		  "Subject for a new mail"},
		{ "body", 'b', 0, G_OPTION_ARG_STRING, &body,
		  "Body for a new email"},
		{ "cc",  0, 0, G_OPTION_ARG_STRING, &cc,
		  "CC-addresses for a new mail (comma-separated)"},
		{ "bcc", 0, 0, G_OPTION_ARG_STRING, &bcc,
		  "BCC-adresses for a new mail (comma-separated)"},
		{ "reinstall-factory-settings", 0, 0, G_OPTION_ARG_NONE, &reinstall,
		  "Delete all settings and start over (*DESTRUCTIVE*)"
		},
		{ NULL }
	};


	g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, options, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &err)) {
		g_printerr ("modest: error in command line parameter(s): %s\n",
			 err ? err->message : "");
		retval = 1;
		goto cleanup;
	}

	if (debug) {
		g_log_set_always_fatal (G_LOG_LEVEL_WARNING);
	}

	modest_conf = MODEST_CONF(modest_conf_new());
	if (!modest_conf) {
		g_warning ("failed to initialize config system");
		goto cleanup;
	}

	if (reinstall) {
		modest_conf_remove_key (modest_conf, MODEST_CONF_NAMESPACE, NULL);
		install_basic_conf_settings (modest_conf);
		install_test_account (modest_conf);
		goto cleanup;
	}



	gtk_init (&argc, &argv);
	modest_ui = MODEST_UI(modest_ui_new (modest_conf));
	if (!modest_ui) {
		g_warning ("failed to initialize ui");
		goto cleanup;
	}

	{
		gboolean ok;
		gtk_init (&argc, &argv);

		if (mailto||cc||bcc||subject||body)
			ok = modest_ui_show_edit_window (modest_ui,
							 mailto,  /* to */
							 cc,      /* cc */
							 bcc,     /* bcc */
							 subject,    /* subject */
							 body,    /* body */
							 NULL);   /* attachments */
		else
			ok = modest_ui_show_main_window (modest_ui);

		if (!ok)
			g_warning ("showing window failed");
		else
			gtk_main();
	}


cleanup:
	if (err)
		g_error_free (err);

	if (context)
		g_option_context_free (context);

	if (modest_ui)
		g_object_unref (modest_ui);

	if (modest_conf)
		g_object_unref (modest_conf);

	return retval;
}



static void
install_basic_conf_settings (ModestConf *conf)
{
	g_return_if_fail (conf);

	/* main window size */
	modest_conf_set_int (conf, MODEST_CONF_MAIN_WINDOW_WIDTH,
			     MODEST_CONF_MAIN_WINDOW_WIDTH_DEFAULT, NULL);
	modest_conf_set_int (conf, MODEST_CONF_MAIN_WINDOW_HEIGHT,
			     MODEST_CONF_MAIN_WINDOW_HEIGHT_DEFAULT, NULL);

	/* edit window size */
	modest_conf_set_int (conf, MODEST_CONF_EDIT_WINDOW_WIDTH,
			     MODEST_CONF_EDIT_WINDOW_WIDTH_DEFAULT, NULL);
	modest_conf_set_int (conf, MODEST_CONF_EDIT_WINDOW_HEIGHT,
			     MODEST_CONF_EDIT_WINDOW_HEIGHT_DEFAULT, NULL);

	g_print ("modest: returned to factory settings\n");
}


static void
install_test_account (ModestConf *conf)
{
	ModestAccountMgr *acc_mgr;
	const gchar *acc_name = "test";
	g_return_if_fail (conf);

	acc_mgr = MODEST_ACCOUNT_MGR(modest_account_mgr_new (conf));
	if (!acc_mgr) {
		g_warning ("failed to instantiate account mgr");
		return;
	}

	if (modest_account_mgr_account_exists (acc_mgr, acc_name, NULL)) {
		if (!modest_account_mgr_remove_account(acc_mgr, acc_name, NULL)) {
			g_warning ("could not delete existing account");
		}
	}

	if (!modest_account_mgr_add_account (acc_mgr, acc_name, "mystore", "mytransport", NULL))
		g_warning ("failed to add test account");
	else
	{
		modest_account_mgr_add_server_account (acc_mgr, "mystore", "localhost", "djcb",
						       NULL, "imap");
		modest_account_mgr_add_server_account (acc_mgr, "mytransport", "localhost", NULL,
						       NULL, "smtp");
		
	}
	if (modest_account_mgr_identity_exists(acc_mgr, "myidentity", NULL)) {
		if (!modest_account_mgr_remove_identity(acc_mgr, "myidentity", NULL)) {
			g_warning ("could not delete existing identity");
		}
	}
	if (!modest_account_mgr_add_identity (acc_mgr, "myidentity", "user@localhost",
		                       "", "", FALSE, NULL, FALSE ))
		g_warning ("failed to add test account");
	g_object_unref (G_OBJECT(acc_mgr));
}

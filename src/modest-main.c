/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <tny-list.h>
#include <tny-transport-account.h>
#include <tny-account-store.h>
#include <tny-list.h>
#include <tny-simple-list.h>

#include "config.h"
#include "modest-conf.h"
#include "modest-account-mgr.h"
#include "modest-ui.h"
#include "modest-icon-factory.h"
#include "modest-tny-account-store.h"
#include "modest-tny-platform-factory.h"
#include "modest-mail-operation.h"


#ifdef MODEST_ENABLE_HILDON /* Hildon includes */
#include <libosso.h>
#endif /* MODEST_ENABLE_HILDON */

/* return values */
#define MODEST_ERR_NONE    0
#define MODEST_ERR_OPTIONS 1
#define MODEST_ERR_CONF    2
#define MODEST_ERR_UI      3
#define MODEST_ERR_HILDON  4
#define MODEST_ERR_RUN     5
#define MODEST_ERR_SEND    6

static gboolean hildon_init (); /* NOP if HILDON is not defined */

static int start_ui (const gchar* mailto, const gchar *cc,
		     const gchar *bcc, const gchar* subject, const gchar *body);

static int send_mail (const gchar* mailto, const gchar *cc, const gchar *bcc,
		      const gchar* subject, const gchar *body);

int
main (int argc, char *argv[])
{
	GOptionContext   *context        = NULL;
	TnyPlatformFactory *fact         = NULL;
	ModestConf       *modest_conf    = NULL;

	GError *err = NULL;
	int retval  = MODEST_ERR_NONE;
		
	static gboolean debug=FALSE, batch=FALSE;
	static gchar    *mailto, *subject, *bcc, *cc, *body;

	static GOptionEntry options[] = {
		{ "debug",  'd', 0, G_OPTION_ARG_NONE, &debug,
		  "Run in debug mode", NULL},
		{ "mailto", 'm', 0, G_OPTION_ARG_STRING, &mailto,
		  "New email to <addresses> (comma-separated)", NULL},
		{ "subject", 's', 0, G_OPTION_ARG_STRING, &subject,
		  "Subject for a new mail", NULL},
		{ "body", 'b', 0, G_OPTION_ARG_STRING, &body,
		  "Body for a new email", NULL},
		{ "cc",  'c', 0, G_OPTION_ARG_STRING, &cc,
		  "Cc: addresses for a new mail (comma-separated)", NULL},
		{ "bcc", 'x', 0, G_OPTION_ARG_STRING, &bcc,
		  "Bcc: addresses for a new mail (comma-separated)", NULL},
		{ "batch", 'y', 0, G_OPTION_ARG_NONE, &batch,
		  "Run in batch mode (don't show UI)", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL }
	};

	bindtextdomain (GETTEXT_PACKAGE, MODESTLOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, options, NULL);
	
	if (!g_option_context_parse (context, &argc, &argv, &err)) {
		g_printerr ("modest: error in command line parameter(s): '%s', exiting\n",
			    err ? err->message : "");
		g_error_free (err);
		retval = MODEST_ERR_OPTIONS;
		goto cleanup;
	}
	g_option_context_free (context);
	
	fact = modest_tny_platform_factory_get_instance ();
	modest_conf = modest_tny_platform_factory_get_modest_conf_instance (fact);
	if (!modest_conf) {
		g_printerr ("modest: failed to initialize config system, exiting\n");
		retval = MODEST_ERR_CONF;
		goto cleanup;
	}

	if (debug)
		g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING);
	
	if (!getenv("DISPLAY"))
		batch = TRUE; 
	
	if (!batch) {
		gtk_init (&argc, &argv);
		retval = start_ui (mailto, cc, bcc, subject, body);
	} else 
		retval = send_mail (mailto, cc, bcc, subject, body);
		
	
cleanup:
	
	return retval;
}


static int
start_ui (const gchar* mailto, const gchar *cc, const gchar *bcc,
	  const gchar* subject, const gchar *body)
{

	ModestUI *modest_ui;
	gint retval = 0;

	#ifndef OLD_UI_STUFF
	GtkWidget *win;
	#endif
	
	modest_ui = MODEST_UI(modest_ui_new ());
	if (!modest_ui) {
		g_printerr ("modest: failed to initialize ui, exiting\n");
		retval = MODEST_ERR_UI;
		goto cleanup;
	}
	
	modest_icon_factory_init ();	

	if (!hildon_init ()) { /* NOP  if hildon is not defined */
		g_printerr ("modest: failed to initialize hildon, exiting\n");
		retval = MODEST_ERR_HILDON;
		goto cleanup;
	}

	if (mailto||cc||bcc||subject||body) {

/* 		ok = modest_ui_new_edit_window (modest_ui, */
/* 						mailto,  /\* to *\/ */
/* 						cc,      /\* cc *\/ */
/* 						bcc,     /\* bcc *\/ */
/* 						subject,    /\* subject *\/ */
/* 		 				body,    /\* body *\/ */
/* 						NULL);   /\* attachments *\/ */
	} else
#ifndef OLD_UI_STUFF
	win = modest_ui_main_window (modest_ui);
	gtk_widget_show (win);
#else
	modest_ui_show_main_window (modest_ui);
#endif
	gtk_main();
	
cleanup:
	if (modest_ui)
		g_object_unref (modest_ui);

	modest_icon_factory_uninit ();
	return retval;
}


static gboolean
hildon_init ()
{
#ifdef MODEST_ENABLE_HILDON

	osso_context_t *osso_context =
		osso_initialize(PACKAGE, PACKAGE_VERSION,
				TRUE, NULL);	
	if (!osso_context) {
		g_printerr ("modest: failed to aquire osso context, exiting\n");

		return FALSE;
		
	}
#endif /* MODEST_ENABLE_HILDON */

	return TRUE;
}



static int
send_mail (const gchar* mailto, const gchar *cc, const gchar *bcc,
	   const gchar* subject, const gchar *body)
{
	ModestAccountMgr *acc_mgr = NULL;
	TnyPlatformFactory *fact = NULL;
	TnyAccountStore *acc_store = NULL;
	ModestMailOperation *mail_operation;

	TnyList *accounts = NULL;
	TnyIterator *iter = NULL;
	TnyTransportAccount *account = NULL;	
	int retval;

	fact = modest_tny_platform_factory_get_instance ();
	acc_mgr = modest_tny_platform_factory_get_modest_account_mgr_instance (fact);
	acc_store = tny_platform_factory_new_account_store (fact);	

	accounts = TNY_LIST(tny_simple_list_new ());
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(acc_store), accounts,
					      TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);

	iter = tny_list_create_iterator(accounts);
	tny_iterator_first (iter);
	if (tny_iterator_is_done (iter)) {
		g_printerr("modest: no transport accounts defined\n");
		retval = MODEST_ERR_SEND;
		goto cleanup;
	}

	account = TNY_TRANSPORT_ACCOUNT (tny_iterator_get_current(iter));

	mail_operation = modest_mail_operation_new (TNY_ACCOUNT (account));

	modest_mail_operation_send_new_mail (mail_operation,
					     "djcb@djcbsoftware.nl", mailto, cc, bcc, 
					     subject, body, NULL);


	if (modest_mail_operation_get_status (mail_operation) == 
	    MODEST_MAIL_OPERATION_STATUS_FAILED) {
		retval = MODEST_ERR_SEND;
		goto cleanup;
	} else
		retval = MODEST_ERR_NONE; /* hurray! */

cleanup:
	if (iter) g_object_unref (G_OBJECT (iter));
	if (accounts) g_object_unref (G_OBJECT (accounts));
	if (mail_operation) g_object_unref (G_OBJECT (mail_operation));

	return retval;
}


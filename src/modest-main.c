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

#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtkwidget.h>

#include <tny-list.h>
#include <tny-transport-account.h>
#include <tny-account-store.h>
#include <tny-list.h>
#include <tny-simple-list.h>

#include <modest-runtime.h>
#include <modest-defs.h>
#include <modest-ui.h>
#include <modest-icon-factory.h>
#include <modest-tny-account-store.h>
#include <modest-tny-platform-factory.h>
#include <modest-mail-operation.h>


static int start_ui (const gchar* mailto, const gchar *cc, const gchar *bcc,
		     const gchar* subject, const gchar *body);
static int send_mail (const gchar* mailto, const gchar *cc, const gchar *bcc,
		      const gchar* subject, const gchar *body);

int
main (int argc, char *argv[])
{
	GOptionContext     *context       = NULL;
	
	GError *err = NULL;
	int retval  = MODEST_ERR_NONE;
		
	static gboolean batch=FALSE, factory_settings=FALSE;
	static gchar    *mailto, *subject, *bcc, *cc, *body, *account;

	static GOptionEntry options[] = {
		{ "mailto", 'm', 0, G_OPTION_ARG_STRING, &mailto,
		  N_("New email to <addresses> (comma-separated)"), NULL},
		{ "subject", 's', 0, G_OPTION_ARG_STRING, &subject,
		  N_("Subject for a new mail"), NULL},
		{ "body", 'b', 0, G_OPTION_ARG_STRING, &body,
		  N_("Body for a new email"), NULL},
		{ "cc",  'c', 0, G_OPTION_ARG_STRING, &cc,
		  N_("Cc: addresses for a new mail (comma-separated)"), NULL},
		{ "bcc", 'x', 0, G_OPTION_ARG_STRING, &bcc,
		  N_("Bcc: addresses for a new mail (comma-separated)"), NULL},
		{ "account", 'a', 0, G_OPTION_ARG_STRING, &account,
		  N_("Account to use (if specified, default account is used)"), NULL},
		{ "batch", 'y', 0, G_OPTION_ARG_NONE, &batch,
		  N_("Run in batch mode (don't show UI)"), NULL},
		{ "factory-settings", 0, 0, G_OPTION_ARG_NONE, &factory_settings,
		  N_("return to factory settings"), NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL }
	};

	if (!modest_runtime_init ()) {
		g_printerr ("modest: cannot init runtime\n");
		return MODEST_ERR_INIT;
	}
	
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
	
	if (!getenv("DISPLAY"))
		batch = TRUE; 
	
	if (!batch) {
		if (!gtk_init_check(&argc, &argv)) {
			g_printerr ("modest: failed to start graphical ui\n");
			goto cleanup;
		}
		retval = start_ui (mailto, cc, bcc, subject, body);

	} else 
		retval = send_mail (mailto, cc, bcc, subject, body);
	
cleanup:
	if (!modest_runtime_uninit ()) 
		g_printerr ("modest: modest_runtime_uninit failed\n");

	return retval;
}


static int
start_ui (const gchar* mailto, const gchar *cc, const gchar *bcc,
	  const gchar* subject, const gchar *body)
{
	ModestWindow *win = NULL;
	ModestUI *modest_ui = NULL;
	
	gint retval = 0;

	modest_ui = modest_ui_new ();

	if (mailto||cc||bcc||subject||body) {
		g_warning ("FIXME: implement this");
/* 		ok = modest_ui_new_edit_window (modest_ui, */
/* 						mailto,  /\* to *\/ */
/* 						cc,      /\* cc *\/ */
/* 						bcc,     /\* bcc *\/ */
/* 						subject,    /\* subject *\/ */
/* 		 				body,    /\* body *\/ */
/* 						NULL);   /\* attachments *\/ */
	} else 
		win = modest_ui_main_window (modest_ui);
	
	if (win) {
		gtk_widget_show_all (GTK_WIDGET (win));
		gtk_main();
	}
	if (modest_ui)
		g_object_unref (G_OBJECT(modest_ui));
	
	return retval;
}



static int
send_mail (const gchar* mailto, const gchar *cc, const gchar *bcc,
	   const gchar* subject, const gchar *body)
{
	ModestMailOperation *mail_operation = NULL;
	TnyList *accounts = NULL;
	TnyIterator *iter = NULL;
	TnyTransportAccount *account = NULL;	
	int retval;

	accounts = TNY_LIST(tny_simple_list_new ());
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(modest_runtime_get_account_store()),
					accounts,
					TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);

	iter = tny_list_create_iterator(accounts);
	tny_iterator_first (iter);
	if (tny_iterator_is_done (iter)) {
		g_printerr("modest: no transport accounts defined\n");
		retval = MODEST_ERR_SEND;
		goto cleanup;
	}

	account = TNY_TRANSPORT_ACCOUNT (tny_iterator_get_current(iter));
	mail_operation = modest_mail_operation_new ();

	modest_mail_operation_send_new_mail (mail_operation,
					     account, "test@example.com",
					     mailto, cc, bcc, 
					     subject, body, NULL);
		
	if (modest_mail_operation_get_status (mail_operation) == 
	    MODEST_MAIL_OPERATION_STATUS_FAILED) {
		retval = MODEST_ERR_SEND;
		goto cleanup;
	} else
		retval = MODEST_ERR_NONE; /* hurray! */
cleanup:
	if (iter)
		g_object_unref (G_OBJECT (iter));
	if (accounts)
		g_object_unref (G_OBJECT (accounts));
	if (mail_operation)
		g_object_unref (G_OBJECT (mail_operation));
	return retval;
}


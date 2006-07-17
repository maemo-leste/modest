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

#include <tny-account-store-iface.h>
#include <tny-list-iface.h>

#include "modest-conf.h"
#include "modest-account-mgr.h"
#include "modest-ui.h"
#include "modest-icon-factory.h"
#include "modest-tny-transport-actions.h"
#include "modest-tny-account-store.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

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

static int start_ui (ModestConf *conf, const gchar* mailto, const gchar *cc,
		     const gchar *bcc, const gchar* subject, const gchar *body);

static int send_mail (ModestConf *conf, const gchar* mailto, const gchar *cc, const gchar *bcc,
		      const gchar* subject, const gchar *body);

int
main (int argc, char *argv[])
{
	GOptionContext   *context        = NULL;
	ModestConf       *modest_conf    = NULL;
	ModestUI         *modest_ui      = NULL;

	GError *err = NULL;
	int retval  = MODEST_ERR_NONE;
		
	static gboolean debug=FALSE, batch=FALSE;
	static gchar    *mailto, *subject, *bcc, *cc, *body;

	static GOptionEntry options[] = {
		{ "debug",  'd', 0, G_OPTION_ARG_NONE, &debug,
		  "Run in debug mode" },
		{ "mailto", 'm', 0, G_OPTION_ARG_STRING, &mailto,
		  "New email to <addresses> (comma-separated)"},
		{ "subject", 's', 0, G_OPTION_ARG_STRING, &subject,
		  "Subject for a new mail"},
		{ "body", 'b', 0, G_OPTION_ARG_STRING, &body,
		  "Body for a new email"},
		{ "cc",  'c', 0, G_OPTION_ARG_STRING, &cc,
		  "Cc: addresses for a new mail (comma-separated)"},
		{ "bcc", 'x', 0, G_OPTION_ARG_STRING, &bcc,
		  "Bcc: addresses for a new mail (comma-separated)"},
		{ "batch", 'y', 0, G_OPTION_ARG_NONE, &batch,
		  "Run in batch mode (don't show UI)"},
		{ NULL }
	};

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
	
	modest_conf = MODEST_CONF(modest_conf_new());
	if (!modest_conf) {
		g_printerr ("modest: failed to initialize config system, exiting\n");
		retval = MODEST_ERR_CONF;
		goto cleanup;
	}

	if (debug)
		g_log_set_always_fatal (G_LOG_LEVEL_WARNING);
	
	if (!batch) {
		gtk_init (&argc, &argv);
		retval = start_ui (modest_conf, mailto, cc, bcc, subject, body);
	} else 
		retval = send_mail (modest_conf, mailto, cc, bcc, subject, body);
		
	
cleanup:
	if (modest_conf)
		g_object_unref (G_OBJECT(modest_conf));
	
	return retval;
}


static int
start_ui (ModestConf *conf, const gchar* mailto, const gchar *cc, const gchar *bcc,
	  const gchar* subject, const gchar *body)
{

	GtkWidget *win;
	ModestUI *modest_ui;
	gint ok, retval = 0;

	modest_ui = MODEST_UI(modest_ui_new (conf));
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
send_mail (ModestConf *conf, const gchar* mailto, const gchar *cc, const gchar *bcc,
	   const gchar* subject, const gchar *body)
{
	ModestAccountMgr *acc_mgr = NULL;
	ModestTnyTransportActions *transport = NULL;
	ModestTnyAccountStore *acc_store = NULL;

	TnyListIface *accounts = NULL;
	TnyIteratorIface *iter = NULL;

	TnyTransportAccountIface *account = NULL;
	
	int retval;
	int i = 0;
	
	acc_mgr   = modest_account_mgr_new (conf);
	acc_store = modest_tny_account_store_new (acc_mgr);	
	transport = modest_tny_transport_actions_new ();

	accounts = TNY_LIST_IFACE(tny_list_new ());
	tny_account_store_iface_get_accounts (TNY_ACCOUNT_STORE_IFACE(acc_store), accounts,
					      TNY_ACCOUNT_STORE_IFACE_TRANSPORT_ACCOUNTS);

	iter = tny_list_iface_create_iterator(accounts);
	tny_iterator_iface_first (iter);
	if (tny_iterator_iface_is_done (iter)) {
		g_printerr("modest: no transport accounts defined");
		retval = MODEST_ERR_SEND;
		goto cleanup;
	}

	account = TNY_TRANSPORT_ACCOUNT_IFACE (tny_iterator_iface_current(iter));

	if (!modest_tny_transport_actions_send_message (transport, account,
							"<>", mailto, cc, bcc, subject, body,
							NULL)) {
		retval = MODEST_ERR_SEND;
		goto cleanup;
	} else
		retval = MODEST_ERR_NONE; /* hurray! */
							 
cleanup:
	if (iter)
		g_object_unref (G_OBJECT(iter));
	if (accounts)
		g_object_unref (G_OBJECT(accounts));
	if (transport)
		g_object_unref (G_OBJECT(transport));
	if (acc_store)
		g_object_unref (G_OBJECT(acc_store));
	if (acc_mgr)
		g_object_unref (G_OBJECT(acc_mgr));
	
	return retval;
}


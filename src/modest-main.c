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
#include <modest-tny-account-store.h>
#include <modest-tny-platform-factory.h>
#include <modest-mail-operation.h>
#include <modest-tny-account.h>
#include <modest-tny-msg.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>

#include <widgets/modest-main-window.h>
#include <widgets/modest-msg-edit-window.h>

typedef enum {
	MODEST_ERR_NONE    = 0,   /* no error */
	MODEST_ERR_OPTIONS = 1,   /* error in the options */
	MODEST_ERR_CONF    = 2,   /* error getting confuration db */
	MODEST_ERR_UI      = 3,   /* error in the UI */
	MODEST_ERR_HILDON  = 4,   /* error with Hildon (maemo-only) */
	MODEST_ERR_RUN     = 5,   /* error running */
	MODEST_ERR_SEND    = 6,   /* error sending mail */
	MODEST_ERR_PARAM   = 7,   /* error in one or more of the parameters */
	MODEST_ERR_INIT    = 8    /* error in initialization */
} ModestErrorCode;

static gchar*           check_account (const gchar *account);

static ModestErrorCode  start_ui      (const gchar *account,
				       const gchar* mailto, const gchar *cc,
				       const gchar *bcc, const gchar* subject, const gchar *body);

static ModestErrorCode  send_mail     (const gchar* account,
				       const gchar* mailto, const gchar *cc, const gchar *bcc,
				       const gchar* subject, const gchar *body);
int
main (int argc, char *argv[])
{
	GOptionContext     *context = NULL;
	
	GError *err = NULL;
	int retval  = MODEST_ERR_NONE;
		
	static gboolean batch = FALSE;
	static gchar    *mailto=NULL, *subject=NULL, *bcc=NULL,
		*cc=NULL, *body=NULL, *account=NULL;
	gchar *account_or_default;
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
		  N_("Account to use (if none specified, the default account will be used)"), NULL},
		{ "batch", 'y', 0, G_OPTION_ARG_NONE, &batch,
		  N_("Run in batch mode (don't show UI)"), NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL }
	};

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, options, NULL);
	
	if (!g_option_context_parse (context, &argc, &argv, &err)) {
		g_printerr ("modest: error in command line parameter(s): '%s', exiting\n",
			    err ? err->message : "");
		g_error_free (err);
		g_option_context_free (context);
		retval = MODEST_ERR_OPTIONS;
		goto cleanup;
	}
	g_option_context_free (context);
	
	if (!modest_runtime_init ()) {
		g_printerr ("modest: cannot init runtime\n");
		return MODEST_ERR_INIT;
		
	}
	
	account_or_default = check_account (account);
	g_free (account);
	
	if (!batch) {
		if (!modest_runtime_init_ui (argc, argv)) {
			g_printerr ("modest: cannot start ui\n");
			retval = MODEST_ERR_UI;
			goto cleanup;
		} else {
			if (modest_conf_get_bool (modest_runtime_get_conf(),
						  MODEST_CONF_CONNECT_AT_STARTUP, NULL))
				tny_device_force_online (modest_runtime_get_device());
			
			retval = start_ui (account_or_default,
					   mailto, cc, bcc, subject, body);
		}
	} else {
		if (!account_or_default) {
			g_printerr ("modest: no account has been defined yet\n");
			retval = MODEST_ERR_CONF;
			goto cleanup;
		}	
		retval = send_mail (account_or_default,
				    mailto, cc, bcc, subject, body);
	}
	
cleanup:
	g_free (mailto);
	g_free (subject);
	g_free (bcc);
	g_free (cc);
	g_free (body);
	g_free (account);

	if (!modest_runtime_uninit ()) 
		g_printerr ("modest: modest_runtime_uninit failed\n");

	return retval;
}


static ModestErrorCode 
start_ui (const gchar *account_name, const gchar* mailto, const gchar *cc, const gchar *bcc,
	  const gchar* subject, const gchar *body)
{
	ModestWindow *win = NULL;

	if (mailto||cc||bcc||subject||body) {		
		gchar *from;
		TnyMsg     *msg;
		TnyFolder  *folder;
		TnyAccount *account;
		
		if (!account_name) {
			g_printerr ("modest: no valid account provided, "
				    "nor is default one available\n");
			return MODEST_ERR_PARAM;
		}
		from = modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
							   account_name);
		msg  = modest_tny_msg_new (mailto,from,cc,bcc,subject,body,NULL);
		if (!msg) {
			g_printerr ("modest: failed to create message\n");
			g_free (from);
			return MODEST_ERR_SEND;
		}

		account = modest_tny_account_store_get_tny_account_by_account (
			modest_runtime_get_account_store(), account_name,
			TNY_ACCOUNT_TYPE_TRANSPORT);
		if (!account) {
			g_printerr ("modest: failed to get tny account folder\n");
			g_free (from);
			g_object_unref (G_OBJECT(msg));
			return MODEST_ERR_SEND;
		}
		
		folder = modest_tny_account_get_special_folder (account,
								TNY_FOLDER_TYPE_DRAFTS);
		if (!folder) {
			g_printerr ("modest: failed to find Drafts folder\n");
			g_free (from);
			g_object_unref (G_OBJECT(msg));
			g_object_unref (G_OBJECT(account));
			return MODEST_ERR_SEND;
		}
		tny_folder_add_msg (folder, msg, NULL); /* FIXME: check err */

		win = modest_msg_edit_window_new (msg, account_name);
		
		g_object_unref (G_OBJECT(msg));
		g_object_unref (G_OBJECT(account));
		g_object_unref (G_OBJECT(folder));
		g_free (from);
	} else 
		win = modest_main_window_new ();

	if (!win) {
		g_printerr ("modest: failed to create window\n");
		return MODEST_ERR_UI;
	} else {
		ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();
		modest_window_mgr_register_window (mgr, win);
	}
	
	gtk_widget_show_all (GTK_WIDGET (win));
	gtk_main();
	
	return MODEST_ERR_NONE;
}

static gchar*
check_account (const gchar* account)
{
	gchar *retval;
	ModestAccountMgr *account_mgr;
	
	account_mgr = modest_runtime_get_account_mgr();
	
	if (!account)
		retval = modest_account_mgr_get_default_account (account_mgr);
	else
		retval = g_strdup (account);

	/* sanity check */
	if (!account || !modest_account_mgr_account_exists (account_mgr, account, FALSE)) {
		g_free (retval);
		retval = NULL;
	}
	return retval;
}

static ModestErrorCode
send_mail (const gchar* account_name,
	   const gchar* mailto, const gchar *cc, const gchar *bcc,
	   const gchar* subject, const gchar *body)
{
	int retval;
	TnyTransportAccount *account;
	ModestMailOperation *mail_operation = NULL;
	gchar               *from_string;
	
	g_return_val_if_fail (account_name, MODEST_ERR_SEND);

	////////////////////// FIXME ////////
	modest_runtime_not_implemented (NULL);
	return MODEST_ERR_NONE;
	//////////////////////////////////////
	
	account = TNY_TRANSPORT_ACCOUNT (modest_tny_account_store_get_tny_account_by_account
					 (modest_runtime_get_account_store(), account_name,
					  TNY_ACCOUNT_TYPE_TRANSPORT));
	if (!account) {
		g_printerr ("modest: no transport defined account for %s\n",
			    account_name);
		return MODEST_ERR_SEND;
	}
	from_string = modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
							  account_name);

	mail_operation = modest_mail_operation_new ();
	modest_mail_operation_send_new_mail (mail_operation, account,
					     from_string, mailto,
					     cc, bcc, subject, body, NULL,
					     NULL, 0);
	if (modest_mail_operation_get_status (mail_operation) == 
	    MODEST_MAIL_OPERATION_STATUS_FAILED) {
		retval = MODEST_ERR_SEND;
	} else
		retval = MODEST_ERR_NONE; /* hurray! */
	
	g_object_unref (G_OBJECT(account));
	g_object_unref (G_OBJECT(mail_operation));
	g_free (from_string);
	
	return retval;
}

	

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

#include <modest-tny-platform-factory.h>
#include <modest-tny-account.h>
#include <modest-tny-account-store.h>
#include <modest-runtime.h>
#include <tny-simple-list.h>
#include <modest-tny-folder.h>
#include <modest-account-mgr-helpers.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <tny-folder-stats.h>

/* for now, ignore the account ===> the special folders are the same,
 * local folders for all accounts
 * this might change, ie, IMAP might have server-side sent-items
 */
TnyFolder *
modest_tny_account_get_special_folder (TnyAccount *account,
				       TnyFolderType special_type)
{
	TnyList *folders;
	TnyIterator *iter;
	TnyFolder *special_folder = NULL;
	TnyAccount *local_account;
	
	g_return_val_if_fail (account, NULL);
	g_return_val_if_fail (0 <= special_type && special_type < TNY_FOLDER_TYPE_NUM,
			      NULL);
	
	local_account = modest_tny_account_store_get_tny_account_by_id (modest_runtime_get_account_store(),
									MODEST_LOCAL_FOLDERS_ACCOUNT_ID);
	if (!local_account) {
		g_printerr ("modest: cannot get local account\n");
		return NULL;
	}

	folders = TNY_LIST (tny_simple_list_new ());

	/* no need to do this _async, as these are local folders */
	tny_folder_store_get_folders (TNY_FOLDER_STORE (local_account),
				      folders, NULL, NULL);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder =
			TNY_FOLDER (tny_iterator_get_current (iter));
		if (modest_tny_folder_get_local_folder_type (folder) == special_type) {
			special_folder = folder;
			break;
		}
		g_object_unref (G_OBJECT(folder));
		tny_iterator_next (iter);
	}
	g_object_unref (G_OBJECT (folders));
	g_object_unref (G_OBJECT (iter));
	g_object_unref (G_OBJECT (local_account));

	return special_folder;
}

/* Camel options: */

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-store.c 
 */
#define MODEST_ACCOUNT_OPTION_SSL "use_ssl"
#define MODEST_ACCOUNT_OPTION_SSL_NEVER "never"
#define MODEST_ACCOUNT_OPTION_SSL_ALWAYS "always"
#define MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE "when-possible"

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-provider.c 
 */
#define MODEST_ACCOUNT_OPTION_USE_LSUB "use_lsub" /* Show only subscribed folders */
#define MODEST_ACCOUNT_OPTION_CHECK_ALL "check_all" /* Check for new messages in all folders */


/* Posssible values for tny_account_set_secure_auth_mech().
 * These might be camel-specific.
 * Really, tinymail should use an enum.
 * camel_sasl_authtype() seems to list some possible values.
 */
 
/* Note that evolution does not offer these for IMAP: */
#define MODEST_ACCOUNT_AUTH_PLAIN "PLAIN"
#define MODEST_ACCOUNT_AUTH_ANONYMOUS "ANONYMOUS"

/* Caeml's IMAP uses NULL instead for "Password".
 * Also, not that Evolution offers "Password" for IMAP, but "Login" for SMTP.*/
#define MODEST_ACCOUNT_AUTH_PASSWORD "LOGIN" 
#define MODEST_ACCOUNT_AUTH_CRAMMD5 "CRAM-MD5"


/**
 * modest_tny_account_new_from_server_account:
 * @account_mgr: a valid account mgr instance
 * @account_name: the server account name for which to create a corresponding tny account
 * @type: the type of account to create (TNY_ACCOUNT_TYPE_STORE or TNY_ACCOUNT_TYPE_TRANSPORT)
 * 
 * get a tnyaccount corresponding to the server_accounts (store or transport) for this account.
 * NOTE: this function does not set the camel session or the get/forget password functions
 * 
 * Returns: a new TnyAccount or NULL in case of error.
 */
static TnyAccount*
modest_tny_account_new_from_server_account (ModestAccountMgr *account_mgr,
					    ModestServerAccountData *account_data)
{	
	gchar *url;

	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (account_data, NULL);

	/* sanity checks */
	if (account_data->proto == MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN) {
		g_printerr ("modest: '%s' does not provide a protocol\n",
			    account_data->account_name);
		return NULL;
	}

	TnyAccount *tny_account = NULL;
	
	switch (account_data->proto) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		tny_account = TNY_ACCOUNT(tny_camel_transport_account_new ()); break;
	case MODEST_PROTOCOL_STORE_POP:
		tny_account = TNY_ACCOUNT(tny_camel_pop_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_IMAP:
		tny_account = TNY_ACCOUNT(tny_camel_imap_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		tny_account = TNY_ACCOUNT(tny_camel_store_account_new()); break;
	default:
		g_return_val_if_reached (NULL);
	}
	if (!tny_account) {
		g_printerr ("modest: could not create tny account for '%s'\n",
			    account_data->account_name);
		return NULL;
	}
	tny_account_set_id (tny_account, account_data->account_name);

	/* Proto */
	const gchar* proto_name =
		modest_protocol_info_get_transport_store_protocol_name(account_data->proto);
	tny_account_set_proto (tny_account, proto_name);

	       
	/* mbox and maildir accounts use a URI instead of the rest: */
	if (account_data->uri)  {
		/* printf("DEBUG: %s: Using URI=%s\n", __FUNCTION__, account_data->uri); */
		tny_account_set_url_string (TNY_ACCOUNT(tny_account), account_data->uri);
	}
	else {
		/* Set camel-specific options: */
		
		/* Enable secure connection settings: */
		/* printf("DEBUG: %s: security=%d\n", __FUNCTION__, account_data->security); */
		const gchar* option_security = NULL;
		switch (account_data->security) {
		case MODEST_PROTOCOL_CONNECTION_NORMAL:
			option_security = MODEST_ACCOUNT_OPTION_SSL "=" MODEST_ACCOUNT_OPTION_SSL_NEVER;
			break;
		case MODEST_PROTOCOL_CONNECTION_SSL:
		case MODEST_PROTOCOL_CONNECTION_TLS:
			option_security = MODEST_ACCOUNT_OPTION_SSL "=" MODEST_ACCOUNT_OPTION_SSL_ALWAYS;;
			break;
		case MODEST_PROTOCOL_CONNECTION_TLS_OP:
			option_security = MODEST_ACCOUNT_OPTION_SSL "=" MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE;
			break;
		default:
			break;
		}
		
		if(option_security)
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      option_security);
		
		/* Secure authentication: */
		/* printf("DEBUG: %s: secure-auth=%d\n", __FUNCTION__, account_data->secure_auth); */
		const gchar* auth_mech_name = NULL;
		switch (account_data->secure_auth) {
		case MODEST_PROTOCOL_AUTH_NONE:
			/* IMAP and POP need at least a password,
			 * which camel uses if we specify NULL.
			 * This setting should never happen anyway. */
			if (account_data->proto == MODEST_PROTOCOL_STORE_IMAP ||
			    account_data->proto == MODEST_PROTOCOL_STORE_POP)
				auth_mech_name = NULL;
			else if (account_data->proto == MODEST_PROTOCOL_TRANSPORT_SMTP)
				auth_mech_name = MODEST_ACCOUNT_AUTH_ANONYMOUS;
			else
				auth_mech_name = MODEST_ACCOUNT_AUTH_PLAIN;
			break;
			
		case MODEST_PROTOCOL_AUTH_PASSWORD:
			/* Camel use a password for IMAP or POP if we specify NULL,
			 * For IMAP, at least it will report an error if we use "Password", "Login" or "Plain".
			 * (POP is know to report an error for Login too. Probably Password and Plain too.) */
			if (account_data->proto == MODEST_PROTOCOL_STORE_IMAP)
				auth_mech_name = NULL;
			else if (account_data->proto == MODEST_PROTOCOL_STORE_POP)
				auth_mech_name = NULL;
			else
				auth_mech_name = MODEST_ACCOUNT_AUTH_PASSWORD;
			break;
			
		case MODEST_PROTOCOL_AUTH_CRAMMD5:
			auth_mech_name = MODEST_ACCOUNT_AUTH_CRAMMD5;
			break;
			
		default:
			g_warning ("%s: Unhandled secure authentication setting %d for "
				"account=%s (%s)", __FUNCTION__, account_data->secure_auth,
				   account_data->account_name, account_data->hostname);
			break;
		}
		
		if(auth_mech_name) 
			tny_account_set_secure_auth_mech (tny_account, auth_mech_name);
		
		if (modest_protocol_info_protocol_is_store(account_data->proto) && 
			(account_data->proto == MODEST_PROTOCOL_STORE_IMAP) ) {
			/* Other connection options, needed for IMAP. */
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      MODEST_ACCOUNT_OPTION_USE_LSUB);
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      MODEST_ACCOUNT_OPTION_CHECK_ALL);
		}
		
		if (account_data->username) 
			tny_account_set_user (tny_account, account_data->username);
		if (account_data->hostname)
			tny_account_set_hostname (tny_account, account_data->hostname);
		 
		/* Set the port: */
		if (account_data->port)
			tny_account_set_port (tny_account, account_data->port);
	}

	/* FIXME: for debugging */
	url = tny_account_get_url_string (TNY_ACCOUNT(tny_account));
	g_message ("modest: account-url: %s", url);
	g_free (url);
	/***********************/
	
	return tny_account;
}


/* we need these dummy functions, or tinymail will complain */
static gchar*
get_pass_dummy (TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	return NULL;
}
static void
forget_pass_dummy (TnyAccount *account)
{
	/* intentionally left blank */
}

TnyAccount*
modest_tny_account_new_from_account (ModestAccountMgr *account_mgr, const gchar *account_name,
				     TnyAccountType type,
				     TnySessionCamel *session,
				     TnyGetPassFunc get_pass_func,
				     TnyForgetPassFunc forget_pass_func) 
{
	TnyAccount *tny_account = NULL;
	ModestAccountData *account_data = NULL;
	ModestServerAccountData *server_data = NULL;

	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (account_name, NULL);

	account_data = modest_account_mgr_get_account_data (account_mgr, account_name);
	if (!account_data) {
		g_printerr ("modest: cannot get account data for account %s\n", account_name);
		return NULL;
	}

	if (type == TNY_ACCOUNT_TYPE_STORE && account_data->store_account)
		server_data = account_data->store_account;
	else if (type == TNY_ACCOUNT_TYPE_TRANSPORT && account_data->transport_account)
		server_data = account_data->transport_account;
	if (!server_data) {
		g_printerr ("modest: no %s account defined for '%s'\n",
			    type == TNY_ACCOUNT_TYPE_STORE ? "store" : "transport",
			    account_data->display_name);
		modest_account_mgr_free_account_data (account_mgr, account_data);
		return NULL;
	}
	
	tny_account = modest_tny_account_new_from_server_account (account_mgr, server_data);
	if (!tny_account) { 
		g_printerr ("modest: failed to create tny account for %s (%s)\n",
			    account_data->account_name, server_data->account_name);
		modest_account_mgr_free_account_data (account_mgr, account_data);
		return NULL;
	}
	
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	tny_account_set_forget_pass_func (tny_account,
					  forget_pass_func ? forget_pass_func : forget_pass_dummy);
	tny_account_set_pass_func (tny_account,
				   get_pass_func ? get_pass_func: get_pass_dummy);

	/* This name is what shows up in the folder view -- so for some POP/IMAP/... server
	 * account, we set its name to the account of which it is part. */
	if (account_data->display_name)
		tny_account_set_name (tny_account, account_data->display_name); 

	g_object_set_data_full (G_OBJECT(tny_account), "modest_account",
				(gpointer*) g_strdup (account_name), g_free);
	
	modest_account_mgr_free_account_data (account_mgr, account_data);

	return tny_account;
}


TnyAccount*
modest_tny_account_new_for_local_folders (ModestAccountMgr *account_mgr, TnySessionCamel *session)
{
	TnyStoreAccount *tny_account;
	CamelURL *url;
	gchar *maildir, *url_string;

	g_return_val_if_fail (account_mgr, NULL);
	
	tny_account = tny_camel_store_account_new ();
	if (!tny_account) {
		g_printerr ("modest: cannot create account for local folders");
		return NULL;
	}
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	maildir = modest_local_folder_info_get_maildir_path ();
	url = camel_url_new ("maildir:", NULL);
	camel_url_set_path (url, maildir);
	/* Needed by tinymail's DBC assertions */
 	camel_url_set_host (url, "localhost");
	url_string = camel_url_to_string (url, 0);
	
	tny_account_set_url_string (TNY_ACCOUNT(tny_account), url_string);

	tny_account_set_name (TNY_ACCOUNT(tny_account), MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME); 
	tny_account_set_id (TNY_ACCOUNT(tny_account), MODEST_LOCAL_FOLDERS_ACCOUNT_ID); 
        tny_account_set_forget_pass_func (TNY_ACCOUNT(tny_account), forget_pass_dummy);
	tny_account_set_pass_func (TNY_ACCOUNT(tny_account), get_pass_dummy);
	
	g_object_set_data (G_OBJECT(tny_account), "modest_account",
			   (gpointer*)MODEST_LOCAL_FOLDERS_ACCOUNT_ID);
	
	camel_url_free (url);
	g_free (maildir);
	g_free (url_string);

	return TNY_ACCOUNT(tny_account);
}

typedef gint (*TnyStatsFunc) (TnyFolderStats *stats);

typedef struct _RecurseFoldersHelper {
	TnyStatsFunc function;
	guint sum;
	guint folders;
} RecurseFoldersHelper;

static void
recurse_folders (TnyFolderStore *store, 
		 TnyFolderStoreQuery *query, 
		 RecurseFoldersHelper *helper)
{
	TnyIterator *iter;
	TnyList *folders = tny_simple_list_new ();

	tny_folder_store_get_folders (store, folders, query, NULL);
	iter = tny_list_create_iterator (folders);

	helper->folders += tny_list_get_length (folders);

	while (!tny_iterator_is_done (iter)) {
		TnyFolderStats *stats;
		TnyFolder *folder;

		folder = TNY_FOLDER (tny_iterator_get_current (iter));
		stats = tny_folder_get_stats (folder);

		/* initially, we sometimes get -1 from tinymail; ignore that */
		if (helper->function && helper->function (stats) > 0)
			helper->sum += helper->function (stats);

		recurse_folders (TNY_FOLDER_STORE (folder), query, helper);
	    
 		g_object_unref (folder);
 		g_object_unref (stats);
		tny_iterator_next (iter);
	}
	 g_object_unref (G_OBJECT (iter));
	 g_object_unref (G_OBJECT (folders));
}

gint 
modest_tny_account_get_folder_count (TnyAccount *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_ACCOUNT (self), -1);

	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->function = NULL;
	helper->sum = 0;
	helper->folders = 0;

	recurse_folders (TNY_FOLDER_STORE (self), NULL, helper);

	retval = helper->folders;

	g_free (helper);

	return retval;
}

gint
modest_tny_account_get_message_count (TnyAccount *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_ACCOUNT (self), -1);
	
	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->function = (TnyStatsFunc) tny_folder_stats_get_all_count;
	helper->sum = 0;

	recurse_folders (TNY_FOLDER_STORE (self), NULL, helper);

	retval = helper->sum;

	g_free (helper);

	return retval;
}

gint 
modest_tny_account_get_local_size (TnyAccount *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_ACCOUNT (self), -1);

	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->function = (TnyStatsFunc) tny_folder_stats_get_local_size;
	helper->sum = 0;

	recurse_folders (TNY_FOLDER_STORE (self), NULL, helper);

	retval = helper->sum;

	g_free (helper);

	return retval;
}

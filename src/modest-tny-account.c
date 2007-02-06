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

	return special_folder;
}


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
	TnyAccount *tny_account;
		
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (account_data, NULL);

	/* sanity checks */
	if (account_data->proto == MODEST_PROTOCOL_UNKNOWN) {
		g_printerr ("modest: '%s' does not provide a protocol\n",
			    account_data->account_name);
		return NULL;
	}

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
	tny_account_set_proto (tny_account,
			       modest_protocol_info_get_protocol_name(account_data->proto));
	if (account_data->uri) 
		tny_account_set_url_string (TNY_ACCOUNT(tny_account), account_data->uri);
	else {
		if (account_data->options) {
			GSList *options = account_data->options;
			while (options) {
				tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
							      options->data);
				options = g_slist_next (options);
			}
		}
		if (account_data->username) 
			tny_account_set_user (tny_account, account_data->username);
		if (account_data->hostname)
			tny_account_set_hostname (tny_account, account_data->hostname);
	}
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

	/* this name is what shows up in the folder view -- so for some POP/IMAP/... server
	 * account, we set its name to the acount of which it is part */
	if (account_data->display_name)
		tny_account_set_name (tny_account, account_data->display_name); 

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
	url_string = camel_url_to_string (url, 0);
	
	tny_account_set_url_string (TNY_ACCOUNT(tny_account), url_string);
	tny_account_set_name (TNY_ACCOUNT(tny_account), MODEST_LOCAL_FOLDERS_ACCOUNT_NAME); 
	tny_account_set_id (TNY_ACCOUNT(tny_account), MODEST_LOCAL_FOLDERS_ACCOUNT_ID); 
        tny_account_set_forget_pass_func (TNY_ACCOUNT(tny_account), forget_pass_dummy);
	tny_account_set_pass_func (TNY_ACCOUNT(tny_account), get_pass_dummy);

	camel_url_free (url);
	g_free (maildir);
	g_free (url_string);

	return TNY_ACCOUNT(tny_account);
}



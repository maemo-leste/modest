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

#include <modest-account-mgr-helpers.h>
#include <modest-account-mgr-priv.h>
#include <tny-simple-list.h>
#include <modest-runtime.h>
#include <string.h>

gboolean
modest_account_mgr_set_enabled (ModestAccountMgr *self, const gchar* name,
					gboolean enabled)
{
	return modest_account_mgr_set_bool (self, name,
					    MODEST_ACCOUNT_ENABLED, enabled,
					    FALSE, NULL);
}


gboolean
modest_account_mgr_get_enabled (ModestAccountMgr *self, const gchar* name)
{
	return modest_account_mgr_get_bool (self, name,
					    MODEST_ACCOUNT_ENABLED, FALSE,
					    NULL);
}


static ModestServerAccountData*
modest_account_mgr_get_server_account_data (ModestAccountMgr *self, const gchar* name)
{
	ModestServerAccountData *data;
	gchar *proto;
	
	g_return_val_if_fail (modest_account_mgr_account_exists (self, name,
								 TRUE, NULL), NULL);	
	data = g_slice_new0 (ModestServerAccountData);
	
	data->account_name = g_strdup (name);
	data->hostname     = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_HOSTNAME,
							    TRUE, NULL);
	data->username     = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_USERNAME,
							    TRUE, NULL);
	
	proto        = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_PROTO,
						      TRUE, NULL);
	data->proto  = modest_protocol_info_get_protocol (proto);
	g_free (proto);
	
	data->password     = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_PASSWORD,
							    TRUE, NULL);

	data->options = modest_account_mgr_get_list (self, name,
						     MODEST_ACCOUNT_OPTIONS,
						     MODEST_CONF_VALUE_STRING,
						     TRUE, NULL);
	return data;
}


static void
modest_account_mgr_free_server_account_data (ModestAccountMgr *self,
					     ModestServerAccountData* data)
{
	g_return_if_fail (self);

	if (!data)
		return; /* not an error */

	g_free (data->account_name);
	data->account_name = NULL;
	
	g_free (data->hostname);
	data->hostname = NULL;
	
	g_free (data->username);
	data->username = NULL;

	g_free (data->password);
	data->password = NULL;
	
	if (data->options) {
		GSList *tmp = data->options;
		while (tmp) {
			g_free (tmp->data);
			tmp = g_slist_next (tmp);
		}
		g_slist_free (data->options);
	}

	g_slice_free (ModestServerAccountData, data);
}

ModestAccountData*
modest_account_mgr_get_account_data     (ModestAccountMgr *self, const gchar* name)
{
	ModestAccountData *data;
	gchar *server_account;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (modest_account_mgr_account_exists (self, name,
								 FALSE, NULL), NULL);	
	data = g_slice_new0 (ModestAccountData);

	data->account_name = g_strdup (name);

	data->display_name = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_DISPLAY_NAME,
							    FALSE, NULL);
 	data->fullname      = modest_account_mgr_get_string (self, name,
							      MODEST_ACCOUNT_FULLNAME,
							       FALSE, NULL);
	data->email        = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_EMAIL,
							    FALSE, NULL);
	data->enabled      = modest_account_mgr_get_enabled (self, name);

	/* store */
	server_account     = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_STORE_ACCOUNT,
							    FALSE, NULL);
	if (server_account) {
		data->store_account =
			modest_account_mgr_get_server_account_data (self,
								    server_account);
		g_free (server_account);
	}

	/* transport */
	server_account = modest_account_mgr_get_string (self, name,
							MODEST_ACCOUNT_TRANSPORT_ACCOUNT,
							FALSE, NULL);
	if (server_account) {
		data->transport_account =
			modest_account_mgr_get_server_account_data (self,
								    server_account);
		g_free (server_account);
	}

	return data;
}


void
modest_account_mgr_free_account_data (ModestAccountMgr *self, ModestAccountData *data)
{
	g_return_if_fail (self);

	if (!data) /* not an error */ 
		return;

	g_free (data->account_name);
	g_free (data->display_name);
	g_free (data->fullname);
	g_free (data->email);

	modest_account_mgr_free_server_account_data (self, data->store_account);
	modest_account_mgr_free_server_account_data (self, data->transport_account);
	
	g_slice_free (ModestAccountData, data);
}


gchar*
modest_account_mgr_get_default_account  (ModestAccountMgr *self)
{
	gchar *account;	
	ModestConf *conf;
	
	g_return_val_if_fail (self, NULL);

	conf = MODEST_ACCOUNT_MGR_GET_PRIVATE (self)->modest_conf;
	account = modest_conf_get_string (conf, MODEST_CONF_DEFAULT_ACCOUNT,
					  NULL);
	
	/* it's not really an error if there is no default account */
	if (!account) 
		return NULL;

	/* sanity check */
	if (!modest_account_mgr_account_exists (self, account, FALSE, NULL)) {
		g_printerr ("modest: default account does not exist\n");
		g_free (account);
		return NULL;
	}

	return account;
}


gboolean
modest_account_mgr_set_default_account  (ModestAccountMgr *self, const gchar* account)
{
	ModestConf *conf;
	
	g_return_val_if_fail (self,    FALSE);
	g_return_val_if_fail (account, FALSE);
	g_return_val_if_fail (modest_account_mgr_account_exists (self, account, FALSE, NULL),
			      FALSE);
	
	conf = MODEST_ACCOUNT_MGR_GET_PRIVATE (self)->modest_conf;
		
	return modest_conf_set_string (conf, MODEST_CONF_DEFAULT_ACCOUNT,
				       account, NULL);

}


TnyAccount*
modest_account_mgr_get_tny_account (ModestAccountMgr *self, const gchar* account_name,
					TnyAccountType type)
{
	TnyAccount      *account = NULL;
	TnyList         *accounts;
	TnyIterator     *iter;
	gchar           *server_account;
	const gchar     *conf_key;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_name, NULL);

	switch (type) {
	case TNY_ACCOUNT_TYPE_STORE:
		conf_key = MODEST_ACCOUNT_STORE_ACCOUNT; break;
	case TNY_ACCOUNT_TYPE_TRANSPORT:
		conf_key = MODEST_ACCOUNT_TRANSPORT_ACCOUNT; break;
	default:
		g_return_val_if_reached (NULL);
	}
	
	server_account = modest_account_mgr_get_string (self, account_name, conf_key, FALSE, NULL);
	if (!server_account) {
		g_printerr ("modest: no %s account specified for %s\n",
			    type == TNY_ACCOUNT_TYPE_TRANSPORT ? "transport" : "store", account_name);
		return NULL;
	}
	
	accounts = tny_simple_list_new ();
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(modest_runtime_get_account_store()),
					accounts, type);	
	iter = tny_list_create_iterator (accounts);	
	while (tny_iterator_is_done (iter)) {
		account = TNY_ACCOUNT(tny_iterator_get_current(iter));
		if (strcmp (tny_account_get_id (account), server_account) == 0)
			break;
	}
	
	g_object_unref (G_OBJECT(iter));
	g_object_unref (G_OBJECT(accounts));
	
	if (!account)
		g_printerr ("modest: no tny %s account found for %s\n",
			    type == TNY_ACCOUNT_TYPE_TRANSPORT ? "transport" : "store", account_name);
	else {
		/* sanity check */
		if ((type == TNY_ACCOUNT_TYPE_TRANSPORT && !TNY_IS_TRANSPORT_ACCOUNT(account)) ||
		    (type == TNY_ACCOUNT_TYPE_STORE && !TNY_IS_STORE_ACCOUNT(account))) {
			g_printerr ("modest: tny %s acccount found for %s, but was expecting %s account\n",
				    type == TNY_ACCOUNT_TYPE_TRANSPORT ? "transport" : "store", account_name,
				    type == TNY_ACCOUNT_TYPE_TRANSPORT ? "store" : "transport");
			g_object_unref (G_OBJECT(account));
			account = NULL;
		}
	}
	return account;
}


gchar*
modest_account_mgr_get_from_string (ModestAccountMgr *self, const gchar* name)
{
	gchar *fullname, *email, *from;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);

	fullname      = modest_account_mgr_get_string (self, name,MODEST_ACCOUNT_FULLNAME,
						       FALSE, NULL);
	email         = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_EMAIL,
						       FALSE, NULL);
	from = g_strdup_printf ("%s <%s>",
				fullname ? fullname : "",
				email    ? email    : "");
	g_free (fullname);
	g_free (email);

	return from;
}

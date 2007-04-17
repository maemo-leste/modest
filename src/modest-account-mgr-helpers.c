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
	return modest_account_mgr_set_bool (self, name, MODEST_ACCOUNT_ENABLED, enabled,FALSE);
}


gboolean
modest_account_mgr_get_enabled (ModestAccountMgr *self, const gchar* name)
{
	return modest_account_mgr_get_bool (self, name, MODEST_ACCOUNT_ENABLED, FALSE);
}

static gint
compare_option_strings_for_name (const gchar* a, const gchar* b)
{
	/* printf("  debug: compare_option_strings_for_name():a=%s, b=%s\n", a, b); */
	const gchar* sep = strchr(a, '=');
	if (!sep)
		return -1;
		
	gint len = sep - a;
	if(len <= 0)
		return -1;
		
	/* Get the part of the string before the =.
	 * Note that this allocation is inefficient just so we can do a strcmp. */
	gchar* name = g_malloc (len+1);
	memcpy(name, a, len);
	name[len] = 0; /* Null-termination. */
	
	/* printf("    debug: name=%s\n", name); */

	gint result = strcmp (name, b);
	
	g_free (name);
	
	return result;
}
           
gchar*
modest_server_account_data_get_option_value (GSList* options_list, const gchar* option_name)
{
	if (!options_list)
		return NULL;
	
	gchar *result = NULL;
	GSList* option = g_slist_find_custom(options_list, option_name, (GCompareFunc)compare_option_strings_for_name);
	if(option) {
		/* Get the value part of the key=value pair: */
		const gchar* pair = (const gchar*)option->data;
		
		const gchar* sep = strchr(pair, '=');
		if (sep) {
			gint len = sep - pair;
			if(len > 0) {
				result = g_strdup(sep+1);
				
				/* Avoid returning an empty string instead of NULL. */
				if(result && strlen(result) == 0) {
					g_free(result);
					result = NULL;
				}
			}
		}
	}
		
	return result;
}

gboolean
modest_server_account_data_get_option_bool (GSList* options_list, const gchar* option_name)
{
	if (!options_list)
		return FALSE;
	
	gboolean result = FALSE;
	GSList* option = g_slist_find_custom(options_list, option_name, (GCompareFunc)strcmp);
	if(option) {
		return TRUE;
	}
		
	return result;
}

ModestProtocol
modest_server_account_data_get_option_secure_auth (ModestServerAccountData *account_data)
{
	ModestProtocol result = MODEST_PROTOCOL_AUTH_NONE;
	gchar* value = modest_server_account_data_get_option_value (account_data->options, 
		MODEST_ACCOUNT_OPTION_SECURE_AUTH);
	if (value) {
		if (strcmp(value, MODEST_ACCOUNT_OPTION_SECURE_AUTH_VALUE_NONE) == 0)
			result = MODEST_PROTOCOL_AUTH_NONE;
		else if (strcmp(value, MODEST_ACCOUNT_OPTION_SECURE_AUTH_VALUE_PASSWORD) == 0)
			result = MODEST_PROTOCOL_AUTH_PASSWORD;
		else if (strcmp(value, MODEST_ACCOUNT_OPTION_SECURE_AUTH_VALUE_CRAMMD5) == 0)
			result = MODEST_PROTOCOL_AUTH_CRAMMD5;
			
		g_free (value);
	}
	
	return result;
}

void
modest_server_account_set_option_secure_auth (ModestAccountMgr *self, 
	const gchar* account_name, ModestProtocol secure_auth)
{
	GSList *options_list = modest_account_mgr_get_list (self, account_name, MODEST_ACCOUNT_OPTIONS,
						     MODEST_CONF_VALUE_STRING, TRUE);
	if(options_list) {
		/* Remove the item if it exists already: */
		GSList* option = NULL;
		do {
			option = g_slist_find_custom(options_list, MODEST_ACCOUNT_OPTION_SECURE_AUTH, (GCompareFunc)compare_option_strings_for_name);
			if(option)
				options_list = g_slist_remove (options_list, option->data);
		} while (option);
	}					     
	
	/* Add the new item to the list: */
	const gchar* str_value = NULL;
	if (secure_auth == MODEST_PROTOCOL_AUTH_NONE)
		str_value = MODEST_ACCOUNT_OPTION_SECURE_AUTH_VALUE_NONE;
	else if (secure_auth == MODEST_PROTOCOL_AUTH_PASSWORD)
		str_value = MODEST_ACCOUNT_OPTION_SECURE_AUTH_VALUE_PASSWORD;
	else if (secure_auth == MODEST_PROTOCOL_AUTH_CRAMMD5)
		str_value = MODEST_ACCOUNT_OPTION_SECURE_AUTH_VALUE_CRAMMD5;
	
	if (str_value) {
		gchar* pair = g_strdup_printf(MODEST_ACCOUNT_OPTION_SECURE_AUTH "=%s", str_value);
		options_list = g_slist_append(options_list, pair);
	}
	
	/* Set it in the configuration: */
	modest_account_mgr_set_list (self, account_name, MODEST_ACCOUNT_OPTIONS, options_list,
						     MODEST_CONF_VALUE_STRING, TRUE);
	
	/* TODO: Should we free the items too, or just the list? */
	g_slist_free (options_list);
}

ModestProtocol
modest_server_account_data_get_option_security (ModestServerAccountData *account_data)
{
	ModestProtocol result = MODEST_PROTOCOL_SECURITY_NONE;
	gchar* value = modest_server_account_data_get_option_value (account_data->options, 
		MODEST_ACCOUNT_OPTION_SECURITY);
	if (value) {
		if (strcmp(value, MODEST_ACCOUNT_OPTION_SECURITY_VALUE_NONE) == 0)
			result = MODEST_PROTOCOL_SECURITY_NONE;
		else if (strcmp(value, MODEST_ACCOUNT_OPTION_SECURITY_VALUE_NORMAL) == 0)
			result = MODEST_PROTOCOL_SECURITY_TLS;
		else if (strcmp(value, MODEST_ACCOUNT_OPTION_SECURITY_VALUE_SSL) == 0)
			result = MODEST_PROTOCOL_SECURITY_SSL;
			
		g_free (value);
	}
	
	return result;
}

void
modest_server_account_set_option_security (ModestAccountMgr *self, 
	const gchar* account_name, ModestProtocol security)
{
	GSList *options_list = modest_account_mgr_get_list (self, account_name, MODEST_ACCOUNT_OPTIONS,
						     MODEST_CONF_VALUE_STRING, TRUE);

	if(options_list) {
		/* Remove the item if it exists already: */
		GSList* option = NULL;
		do {
			g_slist_find_custom(options_list, MODEST_ACCOUNT_OPTION_SECURITY, (GCompareFunc)compare_option_strings_for_name);
			if(option)
				options_list = g_slist_remove (options_list, option->data);
		} while(option);
	}
		
	/* Add the new item to the list: */
	const gchar* str_value = NULL;
	if (security == MODEST_PROTOCOL_SECURITY_NONE)
		str_value = MODEST_ACCOUNT_OPTION_SECURITY_VALUE_NONE;
	else if (security == MODEST_PROTOCOL_SECURITY_TLS)
		str_value = MODEST_ACCOUNT_OPTION_SECURITY_VALUE_NORMAL;
	else if (security == MODEST_PROTOCOL_SECURITY_SSL)
		str_value = MODEST_ACCOUNT_OPTION_SECURITY_VALUE_SSL;
	
	if (str_value) {
		gchar* pair = g_strdup_printf(MODEST_ACCOUNT_OPTION_SECURITY "=%s", str_value);
		options_list = g_slist_append(options_list, pair);
	}
	
	/* Set it in the configuration: */
	modest_account_mgr_set_list (self, account_name, MODEST_ACCOUNT_OPTIONS, options_list,
						     MODEST_CONF_VALUE_STRING, TRUE);
	
	/* TODO: Should we free the items too, or just the list? */
	g_slist_free (options_list);
}
	                                  
gchar*
modest_account_mgr_get_server_account_option (ModestAccountMgr *self, 
	const gchar* account_name, const gchar* option_name)
{
	GSList *option_list = modest_account_mgr_get_list (self, account_name, MODEST_ACCOUNT_OPTIONS,
						     MODEST_CONF_VALUE_STRING, TRUE);
	if (!option_list)
		return NULL;
		
	gchar *result = modest_server_account_data_get_option_value (option_list, option_name);
	
	/* TODO: Should we free the items too, or just the list? */
	g_slist_free (option_list);
		
	return result;
}

static ModestServerAccountData*
modest_account_mgr_get_server_account_data (ModestAccountMgr *self, const gchar* name)
{
	ModestServerAccountData *data;
	gchar *proto;
	
	g_return_val_if_fail (modest_account_mgr_account_exists (self, name, TRUE), NULL);	
	data = g_slice_new0 (ModestServerAccountData);
	
	data->account_name = g_strdup (name);
	data->hostname     = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_HOSTNAME,TRUE);
	data->username     = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_USERNAME,TRUE);	
	proto              = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_PROTO, TRUE);
	data->proto        = modest_protocol_info_get_protocol (proto);
	g_free (proto);

	data->last_updated = modest_account_mgr_get_int    (self, name, MODEST_ACCOUNT_LAST_UPDATED,TRUE);
	
	data->password     = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_PASSWORD, TRUE);
	data->uri          = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_URI,TRUE);
	data->options = modest_account_mgr_get_list (self, name, MODEST_ACCOUNT_OPTIONS,
						     MODEST_CONF_VALUE_STRING, TRUE);
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

/** You must use modest_account_mgr_free_account_data() on the result.
 */
ModestAccountData*
modest_account_mgr_get_account_data     (ModestAccountMgr *self, const gchar* name)
{
	ModestAccountData *data;
	gchar *server_account;
	gchar *default_account;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (modest_account_mgr_account_exists (self, name,FALSE), NULL);	
	data = g_slice_new0 (ModestAccountData);
	
	data->account_name = g_strdup (name);

	data->display_name = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_DISPLAY_NAME,
							    FALSE);
 	data->fullname     = modest_account_mgr_get_string (self, name,
							      MODEST_ACCOUNT_FULLNAME,
							       FALSE);
	data->email        = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_EMAIL,
							    FALSE);
	data->is_enabled   = modest_account_mgr_get_enabled (self, name);

	default_account    = modest_account_mgr_get_default_account (self);
	data->is_default   = (default_account && strcmp (default_account, name) == 0);
	g_free (default_account);

	/* store */
	server_account     = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_STORE_ACCOUNT,
							    FALSE);
	if (server_account) {
		data->store_account =
			modest_account_mgr_get_server_account_data (self, server_account);
		g_free (server_account);
	}

	/* transport */
	server_account = modest_account_mgr_get_string (self, name,
							MODEST_ACCOUNT_TRANSPORT_ACCOUNT,
							FALSE);
	if (server_account) {
		data->transport_account =
			modest_account_mgr_get_server_account_data (self, server_account);
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
	GError *err = NULL;
	
	g_return_val_if_fail (self, NULL);

	conf = MODEST_ACCOUNT_MGR_GET_PRIVATE (self)->modest_conf;
	account = modest_conf_get_string (conf, MODEST_CONF_DEFAULT_ACCOUNT, &err);
	
	if (err) {
		g_printerr ("modest: failed to get '%s': %s\n",
			    MODEST_CONF_DEFAULT_ACCOUNT, err->message);
		g_error_free (err);
		g_free (account);
		return  NULL;
	}
	
	/* it's not really an error if there is no default account */
	if (!account) 
		return NULL;

	/* sanity check */
	if (!modest_account_mgr_account_exists (self, account, FALSE)) {
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
	g_return_val_if_fail (modest_account_mgr_account_exists (self, account, FALSE),
			      FALSE);
	
	conf = MODEST_ACCOUNT_MGR_GET_PRIVATE (self)->modest_conf;
		
	return modest_conf_set_string (conf, MODEST_CONF_DEFAULT_ACCOUNT,
				       account, NULL);

}

gchar*
modest_account_mgr_get_from_string (ModestAccountMgr *self, const gchar* name)
{
	gchar *fullname, *email, *from;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);

	fullname      = modest_account_mgr_get_string (self, name,MODEST_ACCOUNT_FULLNAME,
						       FALSE);
	email         = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_EMAIL,
						       FALSE);
	from = g_strdup_printf ("%s <%s>",
				fullname ? fullname : "",
				email    ? email    : "");
	g_free (fullname);
	g_free (email);

	return from;
}

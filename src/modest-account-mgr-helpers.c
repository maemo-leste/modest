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

gboolean modest_account_mgr_set_signature (ModestAccountMgr *self, const gchar* name, 
	const gchar* signature, gboolean use_signature)
{
	gboolean result = modest_account_mgr_set_bool (self, name, MODEST_ACCOUNT_USE_SIGNATURE, 
		use_signature, FALSE);
	result = result && modest_account_mgr_set_string (self, name, MODEST_ACCOUNT_SIGNATURE, 
		signature, FALSE);
	return result;
}

gchar* modest_account_mgr_get_display_name (ModestAccountMgr *self, 
	const gchar* name)
{
	return modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_DISPLAY_NAME, FALSE);
}



gchar* modest_account_mgr_get_signature (ModestAccountMgr *self, const gchar* name, 
	gboolean* use_signature)
{
	if (use_signature) {
		*use_signature = 
			modest_account_mgr_get_bool (self, name, MODEST_ACCOUNT_USE_SIGNATURE, FALSE);
	}
	
	return modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_SIGNATURE, FALSE);
}
	

gboolean modest_account_mgr_set_connection_specific_smtp (ModestAccountMgr *self, 
	const gchar* account_name,
	const gchar* connection_name, const gchar* server_account_name)
{
	modest_account_mgr_remove_connection_specific_smtp (self, account_name, connection_name);
	
	GSList *list = modest_account_mgr_get_list (self, account_name, 
							MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST,
						    MODEST_CONF_VALUE_STRING, FALSE);
		
	/* The server account is in the item after the connection name: */
	GSList *list_connection = g_slist_append (list, (gpointer)connection_name);
	list_connection = g_slist_append (list_connection, (gpointer)server_account_name);
	
	/* Reset the changed list: */
	modest_account_mgr_set_list (self, account_name, 
							MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST, list_connection,
						    MODEST_CONF_VALUE_STRING, FALSE);
				
	/* TODO: Should we free the items too, or just the list? */
	g_slist_free (list);
	
	return TRUE;
}

/**
 * modest_account_mgr_remove_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @name: the account name
 * @connection_name: A libconic IAP connection name
 * 
 * Disassacoiate a server account to use with the specific connection for this account.
 *
 * Returns: TRUE if it worked, FALSE otherwise
 */				 
gboolean modest_account_mgr_remove_connection_specific_smtp (ModestAccountMgr *self, 
	const gchar* account_name, const gchar* connection_name)
{
	GSList *list = modest_account_mgr_get_list (self, account_name, 
							MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST,
						    MODEST_CONF_VALUE_STRING, FALSE);
	if (!list)
		return FALSE;
		
	/* The server account is in the item after the connection name: */
	GSList *list_connection = g_slist_find_custom (list, connection_name, (GCompareFunc)strcmp);
	if (list_connection) {
		/* remove both items: */
		GSList *temp = g_slist_delete_link(list_connection, list_connection);
		temp = g_slist_delete_link(temp, g_slist_next(temp));
	}
	
	/* Reset the changed list: */
	modest_account_mgr_set_list (self, account_name, 
							MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST, list,
						    MODEST_CONF_VALUE_STRING, FALSE);
				
	/* TODO: Should we free the items too, or just the list? */
	g_slist_free (list);
	
	return TRUE;
}

gboolean modest_account_mgr_get_has_connection_specific_smtp (ModestAccountMgr *self, const gchar* account_name)
{
	GSList *list = modest_account_mgr_get_list (self, account_name, 
							MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST,
						    MODEST_CONF_VALUE_STRING, FALSE);
	if (!list)
		return FALSE;
	
	/* TODO: Should we free the items too, or just the list? */
	g_slist_free (list);
	
	return TRUE;
}

/**
 * modest_account_mgr_get_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @name: the account name
 * @connection_name: A libconic IAP connection name
 * 
 * Retrieve a server account to use with this specific connection for this account.
 *
 * Returns: a server account name to use for this connection, or NULL if none is specified.
 */			 
gchar* modest_account_mgr_get_connection_specific_smtp (ModestAccountMgr *self, const gchar* account_name,
					 const gchar* connection_name)
{
	gchar *result = NULL;
	
	GSList *list = modest_account_mgr_get_list (self, account_name, 
							MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST,
						    MODEST_CONF_VALUE_STRING, FALSE);
	if (!list)
		return NULL;
		
	/* The server account is in the item after the connection name: */
	GSList *list_connection = g_slist_find_custom (list, connection_name, (GCompareFunc)strcmp);
	if (list_connection) {
		GSList * list_server_account = g_slist_next(list_connection);
		if (list_server_account)
			result = g_strdup ((gchar*)(list_server_account->data));
	}
				
	/* TODO: Should we free the items too, or just the list? */
	g_slist_free (list);
	
	return result;
}
					 
gchar*
modest_server_account_get_username (ModestAccountMgr *self, const gchar* account_name)
{
	return modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_USERNAME, 
		TRUE /* server account */);
}

void
modest_server_account_set_username (ModestAccountMgr *self, const gchar* account_name, 
	const gchar* username)
{
	/* Note that this won't work properly as long as the gconf cache is broken 
	 * in Maemo Bora: */
	gchar *existing_username = modest_server_account_get_username(self, 
		account_name);
	
	modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_USERNAME, 
		username, TRUE /* server account */);
		
	/* We don't know anything about new usernames: */
	if (strcmp (existing_username, username) != 0)
		modest_server_account_set_username_has_succeeded (self, 
		account_name, FALSE);
		
	g_free (existing_username);
}

gboolean
modest_server_account_get_username_has_succeeded (ModestAccountMgr *self, const gchar* account_name)
{
	return modest_account_mgr_get_bool (self, account_name, MODEST_ACCOUNT_USERNAME_HAS_SUCCEEDED, 
		TRUE /* server account */);
}

void
modest_server_account_set_username_has_succeeded (ModestAccountMgr *self, const gchar* account_name, 
	gboolean succeeded)
{
	modest_account_mgr_set_bool (self, account_name, MODEST_ACCOUNT_USERNAME_HAS_SUCCEEDED, 
		succeeded, TRUE /* server account */);
}

void
modest_server_account_set_password (ModestAccountMgr *self, const gchar* account_name, 
	const gchar* password)
{
	modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_PASSWORD, 
		password, TRUE /* server account */);
}
	
gchar*
modest_server_account_get_hostname (ModestAccountMgr *self, const gchar* account_name)
{
	return modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_HOSTNAME, 
		TRUE /* server account */);
}
 

static ModestAuthProtocol
get_secure_auth_for_conf_string(const gchar* value)
{
	ModestAuthProtocol result = MODEST_PROTOCOL_AUTH_NONE;
	if (value) {
		if (strcmp(value, MODEST_ACCOUNT_AUTH_MECH_VALUE_NONE) == 0)
			result = MODEST_PROTOCOL_AUTH_NONE;
		else if (strcmp(value, MODEST_ACCOUNT_AUTH_MECH_VALUE_PASSWORD) == 0)
			result = MODEST_PROTOCOL_AUTH_PASSWORD;
		else if (strcmp(value, MODEST_ACCOUNT_AUTH_MECH_VALUE_CRAMMD5) == 0)
			result = MODEST_PROTOCOL_AUTH_CRAMMD5;
	}
	
	return result;
}

ModestAuthProtocol
modest_server_account_get_secure_auth (ModestAccountMgr *self, 
	const gchar* account_name)
{
	ModestAuthProtocol result = MODEST_PROTOCOL_AUTH_NONE;
	gchar* value = modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_AUTH_MECH, 
		TRUE /* server account */);
	if (value) {
		result = get_secure_auth_for_conf_string (value);
			
		g_free (value);
	}
	
	return result;
}


void
modest_server_account_set_secure_auth (ModestAccountMgr *self, 
	const gchar* account_name, ModestAuthProtocol secure_auth)
{
	/* Get the conf string for the enum value: */
	const gchar* str_value = NULL;
	if (secure_auth == MODEST_PROTOCOL_AUTH_NONE)
		str_value = MODEST_ACCOUNT_AUTH_MECH_VALUE_NONE;
	else if (secure_auth == MODEST_PROTOCOL_AUTH_PASSWORD)
		str_value = MODEST_ACCOUNT_AUTH_MECH_VALUE_PASSWORD;
	else if (secure_auth == MODEST_PROTOCOL_AUTH_CRAMMD5)
		str_value = MODEST_ACCOUNT_AUTH_MECH_VALUE_CRAMMD5;
	
	/* Set it in the configuration: */
	modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_AUTH_MECH, str_value, TRUE);
}

static ModestConnectionProtocol
get_security_for_conf_string(const gchar* value)
{
	ModestConnectionProtocol result = MODEST_PROTOCOL_CONNECTION_NORMAL;
	if (value) {
		if (strcmp(value, MODEST_ACCOUNT_SECURITY_VALUE_NONE) == 0)
			result = MODEST_PROTOCOL_CONNECTION_NORMAL;
		else if (strcmp(value, MODEST_ACCOUNT_SECURITY_VALUE_NORMAL) == 0)
			result = MODEST_PROTOCOL_CONNECTION_TLS;
		else if (strcmp(value, MODEST_ACCOUNT_SECURITY_VALUE_SSL) == 0)
			result = MODEST_PROTOCOL_CONNECTION_SSL;
	}
	
	return result;
}

ModestConnectionProtocol
modest_server_account_get_security (ModestAccountMgr *self, 
	const gchar* account_name)
{
	ModestConnectionProtocol result = MODEST_PROTOCOL_CONNECTION_NORMAL;
	gchar* value = modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_SECURITY, 
		TRUE /* server account */);
	if (value) {
		result = get_security_for_conf_string (value);
			
		g_free (value);
	}
	
	return result;
}

void
modest_server_account_set_security (ModestAccountMgr *self, 
	const gchar* account_name, ModestConnectionProtocol security)
{
	/* Get the conf string for the enum value: */
	const gchar* str_value = NULL;
	if (security == MODEST_PROTOCOL_CONNECTION_NORMAL)
		str_value = MODEST_ACCOUNT_SECURITY_VALUE_NONE;
	else if (security == MODEST_PROTOCOL_CONNECTION_TLS)
		str_value = MODEST_ACCOUNT_SECURITY_VALUE_NORMAL;
	else if (security == MODEST_PROTOCOL_CONNECTION_SSL)
		str_value = MODEST_ACCOUNT_SECURITY_VALUE_SSL;
	
	/* Set it in the configuration: */
	modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_SECURITY, str_value, TRUE);
}

ModestServerAccountData*
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
	data->proto        = modest_protocol_info_get_transport_store_protocol (proto);
	g_free (proto);

	data->port         = modest_account_mgr_get_int (self, name, MODEST_ACCOUNT_PORT, TRUE);
	
	gchar *secure_auth_str = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_AUTH_MECH, TRUE);
	data->secure_auth  = get_secure_auth_for_conf_string(secure_auth_str);
	g_free (secure_auth_str);
		
	gchar *security_str = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_SECURITY, TRUE);
	data->security     = get_security_for_conf_string(security_str);
	g_free (security_str);
	
	data->last_updated = modest_account_mgr_get_int    (self, name, MODEST_ACCOUNT_LAST_UPDATED,TRUE);
	
	data->password     = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_PASSWORD, TRUE);		   
	
	return data;
}


void
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
	
	if (!modest_account_mgr_account_exists (self, name, FALSE)) {
		/* For instance, maybe you are mistakenly checking for a server account name? */
		g_warning ("%s: Account %s does not exist.", __FUNCTION__, name);
		return NULL;
	}
	
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
	
	/* Make sure that at least one account is always the default, if possible:
	 * (It would be meaningless to have enabled accounts but no default account. */
	if (!account) {
		modest_account_mgr_set_first_account_as_default (self);
		account = modest_conf_get_string (conf, MODEST_CONF_DEFAULT_ACCOUNT, &err);
	}

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

gboolean
modest_account_mgr_unset_default_account  (ModestAccountMgr *self)
{
	ModestConf *conf;
	
	g_return_val_if_fail (self,    FALSE);

	conf = MODEST_ACCOUNT_MGR_GET_PRIVATE (self)->modest_conf;
		
	return modest_conf_remove_key (conf, MODEST_CONF_DEFAULT_ACCOUNT, NULL /* err */);

}

gint on_accounts_list_sort_by_title(gconstpointer a, gconstpointer b)
{
 	return g_utf8_collate((const gchar*)a, (const gchar*)b);
}

gboolean
modest_account_mgr_set_first_account_as_default  (ModestAccountMgr *self)
{
	gboolean result = FALSE;
	GSList *account_names = modest_account_mgr_account_names (self, TRUE /* only enabled */);
	
	/* Get the first one, alphabetically, by title: */
	GSList* list_sorted = g_slist_sort (account_names, 
		on_accounts_list_sort_by_title);
	if(list_sorted)
	{
		const gchar* account_name = (const gchar*)list_sorted->data;
		if (account_name) 
			result = modest_account_mgr_set_default_account (self, account_name);
	}
	
	/* TODO: Free the strings too? */
	g_slist_free (account_names);
	
	return result;
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

/* Add a number to the end of the text, or increment a number that is already there.
 */
static gchar*
util_increment_name (const gchar* text)
{
	/* Get the end character,
	 * also doing a UTF-8 validation which is required for using g_utf8_prev_char().
	 */
	const gchar* end = NULL;
	if (!g_utf8_validate (text, -1, &end))
		return NULL;
  
  	if (!end)
  		return NULL;
  		
  	--end; /* Go to before the null-termination. */
  		
  	/* Look at each UTF-8 characer, starting at the end: */
  	const gchar* p = end;
  	const gchar* alpha_end = NULL;
  	while (p)
  	{	
  		/* Stop when we reach the first character that is not a numeric digit: */
  		const gunichar ch = g_utf8_get_char (p);
  		if (!g_unichar_isdigit (ch)) {
  			alpha_end = p;
  			break;
  		}
  		
  		p = g_utf8_prev_char (p);	
  	}
  	
  	if(!alpha_end) {
  		/* The text must consist completely of numeric digits. */
  		alpha_end = text;
  	}
  	else
  		++alpha_end;
  	
  	/* Intepret and increment the number, if any: */
  	gint num = atol (alpha_end);
  	++num;
  	
	/* Get the name part: */
  	gint name_len = alpha_end - text;
  	gchar *name_without_number = g_malloc(name_len + 1);
  	memcpy (name_without_number, text, name_len);
  	name_without_number[name_len] = 0;\
  	
    /* Concatenate the text part and the new number: */	
  	gchar *result = g_strdup_printf("%s%d", name_without_number, num);
  	g_free (name_without_number);
  	
  	return result; 	
}

gchar*
modest_account_mgr_get_unused_account_name (ModestAccountMgr *self, const gchar* starting_name,
	gboolean server_account)
{
	gchar *account_name = g_strdup (starting_name);

	while (modest_account_mgr_account_exists (self, 
		account_name, server_account /*  server_account */)) {
			
		gchar * account_name2 = util_increment_name (account_name);
		g_free (account_name);
		account_name = account_name2;
	}
	
	return account_name;
}

gchar*
modest_account_mgr_get_unused_account_display_name (ModestAccountMgr *self, const gchar* starting_name)
{
	gchar *account_name = g_strdup (starting_name);

	while (modest_account_mgr_account_with_display_name_exists (self, account_name)) {
			
		gchar * account_name2 = util_increment_name (account_name);
		g_free (account_name);
		account_name = account_name2;
	}
	
	return account_name;
}

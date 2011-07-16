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
#include <modest-defs.h>
#include <string.h>
#include <strings.h>
#include <modest-account-protocol.h>
#include <modest-utils.h>
#include <modest-platform.h>

static const gchar * null_means_empty (const gchar * str);

static const gchar *
null_means_empty (const gchar * str)
{
	return str ? str : "";
}

gboolean
modest_account_mgr_set_enabled (ModestAccountMgr *self, const gchar* name,
					gboolean enabled)
{
	gboolean result;
	result = modest_account_mgr_set_bool (self, name, MODEST_ACCOUNT_ENABLED, enabled,FALSE);
	if (result) {
		if (enabled) {
			modest_platform_emit_account_created_signal (name);
		} else {
			modest_platform_emit_account_removed_signal (name);
		}
	}
	return result;
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
							  null_means_empty (signature), FALSE);
	return result;
}

gchar* 
modest_account_mgr_get_signature (ModestAccountMgr *self, 
				  const gchar* name, 
				  gboolean* use_signature)
{
	*use_signature = 
		modest_account_mgr_get_bool (self, name, MODEST_ACCOUNT_USE_SIGNATURE, FALSE);
	
	return modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_SIGNATURE, FALSE);
}

ModestProtocolType modest_account_mgr_get_store_protocol (ModestAccountMgr *self, const gchar* name)
{
       ModestProtocolType result = MODEST_PROTOCOLS_STORE_POP; /* Arbitrary default */
       
       gchar *server_account_name = modest_account_mgr_get_string (self, name,
								   MODEST_ACCOUNT_STORE_ACCOUNT,
								   FALSE);
       if (server_account_name) {
	       ModestServerAccountSettings* server_settings = 
                       modest_account_mgr_load_server_settings (self, server_account_name, FALSE);
               result = modest_server_account_settings_get_protocol (server_settings);
	       
               g_object_unref (server_settings);
               
               g_free (server_account_name);
       }
       
       return result;
}


gboolean 
modest_account_mgr_set_connection_specific_smtp (ModestAccountMgr *self, 
						 const gchar* connection_id, 
						 const gchar* server_account_name)
{
	modest_account_mgr_remove_connection_specific_smtp (self, connection_id);
	
	ModestConf *conf = MODEST_ACCOUNT_MGR_GET_PRIVATE (self)->modest_conf;

	gboolean result = TRUE;
	GError *err = NULL;
	GSList *list = modest_conf_get_list (conf, MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST,
						    MODEST_CONF_VALUE_STRING, &err);
	if (err) {
		g_printerr ("modest: %s: error getting list: %s.\n", __FUNCTION__, err->message);
		g_error_free (err);
		err = NULL;
		result = FALSE;
	} else {	
		/* The server account is in the item after the connection name: */
		list = g_slist_append (list, g_strdup (connection_id));
		list = g_slist_append (list, g_strdup (server_account_name));
	
		/* Reset the changed list: */
		modest_conf_set_list (conf, MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST, list,
						    MODEST_CONF_VALUE_STRING, &err);
		if (err) {
			g_printerr ("modest: %s: error setting list: %s.\n", __FUNCTION__, err->message);
			g_error_free (err);
			result = FALSE;
		}
	}
				
	/* Free the list */
	if (list) {
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);
	}
	
	return result;
}

/**
 * modest_account_mgr_remove_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @name: the account name
 * @connection_id: A libconic IAP connection id
 * 
 * Disassacoiate a server account to use with the specific connection for this account.
 *
 * Returns: TRUE if it worked, FALSE otherwise
 */				 
gboolean 
modest_account_mgr_remove_connection_specific_smtp (ModestAccountMgr *self, 
						    const gchar* connection_id)
{
	ModestAccountMgrPrivate *priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	
	gboolean result = TRUE;
	GError *err = NULL;
	GSList *list = modest_conf_get_list (priv->modest_conf, 
					     MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST,
					     MODEST_CONF_VALUE_STRING, &err);
	if (err) {
		g_printerr ("modest: %s: error getting list: %s.\n", __FUNCTION__, err->message);
		g_error_free (err);
		err = NULL;
		result = FALSE;
	}

	if (!list)
		return FALSE;
		
	/* The server account is in the item after the connection name: */
	GSList *list_connection = g_slist_find_custom (list, connection_id, (GCompareFunc)strcmp);
	if (list_connection) {
		GSList *account_node = g_slist_next (list_connection);
		/* remove both items: */
		list = g_slist_delete_link(list, list_connection);
		list = g_slist_delete_link(list, account_node);
	}
	
	/* Reset the changed list: */
	modest_conf_set_list (priv->modest_conf, MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST, list,
						    MODEST_CONF_VALUE_STRING, &err);
	if (err) {
		g_printerr ("modest: %s: error setting list: %s.\n", __FUNCTION__, err->message);
		g_error_free (err);
		result = FALSE;
	}
				
	/* Free the list */
	if (list) {
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);
	}
	
	return result;
}


gboolean modest_account_mgr_get_use_connection_specific_smtp (ModestAccountMgr *self, const gchar* account_name)
{
	return modest_account_mgr_get_bool (self, account_name, 
		MODEST_ACCOUNT_USE_CONNECTION_SPECIFIC_SMTP, FALSE);
}

gboolean modest_account_mgr_set_use_connection_specific_smtp (ModestAccountMgr *self, const gchar* account_name, 
	gboolean new_value)
{
	return modest_account_mgr_set_bool (self, account_name, MODEST_ACCOUNT_USE_CONNECTION_SPECIFIC_SMTP, 
		new_value, FALSE);
}

/**
 * modest_account_mgr_get_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @connection_id: A libconic IAP connection id
 * 
 * Retrieve a server account to use with this specific connection for this account.
 *
 * Returns: a server account name to use for this connection, or NULL if none is specified.
 */			 
gchar* modest_account_mgr_get_connection_specific_smtp (ModestAccountMgr *self,  const gchar* connection_id)
{
	gchar *result = NULL;
	
	ModestAccountMgrPrivate *priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	
	GError *err = NULL;
	GSList *list = modest_conf_get_list (priv->modest_conf, MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST,
						    MODEST_CONF_VALUE_STRING, &err);
	if (err) {
		g_printerr ("modest: %s: error getting list: %s.\n", __FUNCTION__, err->message);
		g_error_free (err);
		err = NULL;
	}

	if (!list)
		return NULL;

	/* The server account is in the item after the connection name: */
	GSList *iter = list;
	while (iter) {
		const gchar* this_connection_id = (const gchar*)(iter->data);
		if (strcmp (this_connection_id, connection_id) == 0) {
			iter = g_slist_next (iter);
			
			if (iter) {
				const gchar* account_name = (const gchar*)(iter->data);
				if (account_name) {
					result = g_strdup (account_name);
					break;
				}
			}
		}
		
		/* Skip 2 to go to the next connection in the list: */
		iter = g_slist_next (iter);
		if (iter)
			iter = g_slist_next (iter);
	}
		
	/* Free the list */
	if (list) {
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);
	}
	
	return result;
}
					 
gchar*
modest_account_mgr_get_server_account_username (ModestAccountMgr *self, const gchar* account_name)
{
	return modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_USERNAME, 
		TRUE /* server account */);
}

void
modest_account_mgr_set_server_account_username (ModestAccountMgr *self, const gchar* account_name, 
	const gchar* username)
{
	/* Note that this won't work properly as long as the gconf cache is broken 
	 * in Maemo Bora: */
	gchar *existing_username = modest_account_mgr_get_server_account_username(self, 
		account_name);
	
	modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_USERNAME, 
		username, TRUE /* server account */);
		
	/* We don't know anything about new usernames: */
	if (strcmp (existing_username, username) != 0)
		modest_account_mgr_set_server_account_username_has_succeeded (self, account_name, FALSE);
		
	g_free (existing_username);
}

gboolean
modest_account_mgr_get_server_account_username_has_succeeded (ModestAccountMgr *self, const gchar* account_name)
{
	return modest_account_mgr_get_bool (self, account_name, MODEST_ACCOUNT_USERNAME_HAS_SUCCEEDED, 
					    TRUE /* server account */);
}

void
modest_account_mgr_set_server_account_username_has_succeeded (ModestAccountMgr *self, 
						  const gchar* account_name, 
						  gboolean succeeded)
{
	modest_account_mgr_set_bool (self, account_name, MODEST_ACCOUNT_USERNAME_HAS_SUCCEEDED, 
				     succeeded, TRUE /* server account */);
}

gchar*
modest_account_mgr_get_server_account_password (ModestAccountMgr *self, const gchar* account_name)
{
	return modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_PASSWORD, 
		TRUE /* server account */);	
}

gboolean
modest_account_mgr_get_server_account_has_password (ModestAccountMgr *self, const gchar* account_name)
{
	gboolean result = FALSE;
	gchar *password = modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_PASSWORD, 
		TRUE /* server account */);
	if (password && strlen (password)) {
		result = TRUE;
	
		/* Clean password */
		bzero (password, strlen (password));
	}

	g_free (password);
	return result;
}
			 
	
gchar*
modest_account_mgr_get_server_account_hostname (ModestAccountMgr *self, 
						const gchar* account_name)
{
	return modest_account_mgr_get_string (self, 
					      account_name, 
					      MODEST_ACCOUNT_HOSTNAME, 
					      TRUE /* server account */);
}
 
void
modest_account_mgr_set_server_account_hostname (ModestAccountMgr *self, 
						const gchar *server_account_name,
						const gchar *hostname)
{
	modest_account_mgr_set_string (self, 
				       server_account_name,
				       MODEST_ACCOUNT_HOSTNAME, 
				       hostname, 
				       TRUE /* server account */);
}



ModestProtocolType
modest_account_mgr_get_server_account_secure_auth (ModestAccountMgr *self, 
	const gchar* account_name)
{
	ModestProtocolRegistry *protocol_registry;
	ModestProtocolType result = MODEST_PROTOCOLS_AUTH_NONE;
	gchar* value;

	protocol_registry = modest_runtime_get_protocol_registry ();
	value = modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_AUTH_MECH, 
					       TRUE /* server account */);
	if (value) {
		ModestProtocol *protocol;

		protocol = modest_protocol_registry_get_protocol_by_name (protocol_registry, MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS, value);
		g_free (value);

		if (protocol)
			result = modest_protocol_get_type_id (protocol);
			
	}
	
	return result;
}


void
modest_account_mgr_set_server_account_secure_auth (ModestAccountMgr *self, 
	const gchar* account_name, ModestProtocolType secure_auth)
{
	const gchar* str_value;
	ModestProtocolRegistry *protocol_registry;
	ModestProtocol *protocol;

	/* Get the conf string for the protocol: */
	protocol_registry = modest_runtime_get_protocol_registry ();
	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry, secure_auth);
	str_value = modest_protocol_get_name (protocol);
	
	/* Set it in the configuration: */
	modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_AUTH_MECH, str_value, TRUE);
}

ModestProtocolType
modest_account_mgr_get_server_account_security (ModestAccountMgr *self, 
	const gchar* account_name)
{
	ModestProtocolType result = MODEST_PROTOCOLS_CONNECTION_NONE;
	gchar* value;

	value = modest_account_mgr_get_string (self, account_name, MODEST_ACCOUNT_SECURITY, 
					       TRUE /* server account */);
	if (value) {
		ModestProtocolRegistry *protocol_registry;
		ModestProtocol *protocol;

		protocol_registry = modest_runtime_get_protocol_registry ();
		protocol = modest_protocol_registry_get_protocol_by_name (protocol_registry,
									  MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
									  value);
		g_free (value);

		if (protocol)
			result = modest_protocol_get_type_id (protocol);
	}
	
	return result;
}

void
modest_account_mgr_set_server_account_security (ModestAccountMgr *self, 
						const gchar* account_name, 
						ModestProtocolType security)
{
	const gchar* str_value;
	ModestProtocolRegistry *protocol_registry;
	ModestProtocol *protocol;

	/* Get the conf string for the protocol type: */
	protocol_registry = modest_runtime_get_protocol_registry ();
	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry, security);
	str_value = modest_protocol_get_name (protocol);
	
	/* Set it in the configuration: */
	modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_SECURITY, str_value, TRUE);
}

ModestServerAccountSettings *
modest_account_mgr_load_server_settings (ModestAccountMgr *self, const gchar* name, gboolean is_transport_and_not_store)
{
	ModestServerAccountSettings *settings = NULL;
	ModestProtocol *protocol;
	ModestProtocolRegistry *registry;
	gchar *hostname, *username, *pwd, *uri, *proto, *auth, *sec;
        gboolean offline_sync;

	if (!modest_account_mgr_account_exists (self, name, TRUE)) {
		g_warning ("%s account %s does not exist", __FUNCTION__, name);
		return NULL;
	}

	registry = modest_runtime_get_protocol_registry ();
	settings = modest_server_account_settings_new ();

	modest_server_account_settings_set_account_name (settings, name);

	proto = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_PROTO, TRUE);
	if (proto) {
		gchar *tag = NULL;
		if (is_transport_and_not_store) {
			tag = MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS;
		} else {
			tag = MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS;
		}
		protocol = modest_protocol_registry_get_protocol_by_name (registry, tag, proto);

		modest_server_account_settings_set_protocol (settings,
							     modest_protocol_get_type_id (protocol));
		g_free (proto);
	} else {
		goto on_error;
	}

	modest_server_account_settings_set_port (settings,
						 modest_account_mgr_get_int (self, name, MODEST_ACCOUNT_PORT, TRUE));

	auth = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_AUTH_MECH, TRUE);
	if (auth) {
		protocol = modest_protocol_registry_get_protocol_by_name (registry, MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS, auth);
		modest_server_account_settings_set_auth_protocol (settings,
								  modest_protocol_get_type_id (protocol));
		g_free (auth);
	} else {
		modest_server_account_settings_set_auth_protocol (settings, MODEST_PROTOCOLS_AUTH_NONE);
	}

	sec = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_SECURITY, TRUE);
	if (sec) {
		protocol = modest_protocol_registry_get_protocol_by_name (registry, MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS, sec);
		modest_server_account_settings_set_security_protocol (settings,
								      modest_protocol_get_type_id (protocol));
		g_free (sec);
	} else {
		modest_server_account_settings_set_security_protocol (settings,
								      MODEST_PROTOCOLS_CONNECTION_NONE);
	}

	/* Username, password and URI. Note that the URI could include
	   the former two, so in this case there is no need to have
	   them */
	username = modest_account_mgr_get_string (self, name,
						  MODEST_ACCOUNT_USERNAME,TRUE);
	if (username)
		modest_server_account_settings_set_username (settings, username);

	pwd = modest_account_mgr_get_string (self, name,
					     MODEST_ACCOUNT_PASSWORD, TRUE);
	if (pwd) {
		modest_server_account_settings_set_password (settings, pwd);
		g_free (pwd);
	}

	uri = modest_account_mgr_get_string (self, name,
					     MODEST_ACCOUNT_URI, TRUE);
	if (uri)
		modest_server_account_settings_set_uri (settings, uri);

	hostname = modest_account_mgr_get_string (self, name,
						  MODEST_ACCOUNT_HOSTNAME,TRUE);
	if (hostname)
		modest_server_account_settings_set_hostname (settings, hostname);

        offline_sync = modest_account_mgr_get_bool (self, name, 
                                       		    MODEST_ACCOUNT_OFFLINE_SYNC, TRUE);
        if (offline_sync)
            modest_server_account_settings_set_offline_sync (settings, TRUE);

	if (!uri) {
		if (!username || !hostname) {
			g_free (username);
			g_free (hostname);
			goto on_error;
		}
	}

	g_free (username);
	g_free (hostname);
	g_free (uri);

	return settings;

 on_error:
	if (settings)
		g_object_unref (settings);
	return NULL;
}

gboolean 
modest_account_mgr_save_server_settings (ModestAccountMgr *self,
					 ModestServerAccountSettings *settings)
{
	gboolean has_errors = FALSE;
	const gchar *account_name;
	const gchar *protocol_name;
	const gchar *uri;
	ModestProtocolRegistry *protocol_registry;
	ModestProtocol *protocol;
	
	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), FALSE);
	protocol_registry = modest_runtime_get_protocol_registry ();
	account_name = modest_server_account_settings_get_account_name (settings);

	/* if we don't have a valid account name we cannot save */
	g_return_val_if_fail (account_name, FALSE);

	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry,
								  modest_server_account_settings_get_protocol (settings));
	protocol_name = modest_protocol_get_name (protocol);
	uri = modest_server_account_settings_get_uri (settings);
	if (!uri) {
		const gchar *hostname;
		const gchar *username;
		const gchar *password;
		gint port;
		const gchar *auth_protocol_name;
		const gchar *security_name;
                gboolean offline_sync;

		hostname = null_means_empty (modest_server_account_settings_get_hostname (settings));
		username = null_means_empty (modest_server_account_settings_get_username (settings));
		password = null_means_empty (modest_server_account_settings_get_password (settings));
		port = modest_server_account_settings_get_port (settings);
		protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry,
									  modest_server_account_settings_get_auth_protocol (settings));
		auth_protocol_name = modest_protocol_get_name (protocol);
		protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry,
									  modest_server_account_settings_get_security_protocol (settings));
		security_name = modest_protocol_get_name (protocol);
                offline_sync = modest_server_account_settings_get_offline_sync (settings);

		has_errors = !modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_HOSTNAME, 
							    hostname, TRUE);
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_USERNAME,
									   username, TRUE));
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_PASSWORD,
									   password, TRUE));
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_PROTO,
									   protocol_name, TRUE));
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_int (self, account_name, MODEST_ACCOUNT_PORT,
									port, TRUE));
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_string (self, account_name, 
									   MODEST_ACCOUNT_AUTH_MECH,
									   auth_protocol_name, TRUE));		
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_SECURITY,
									   security_name,
									   TRUE));
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_bool (self, account_name, MODEST_ACCOUNT_OFFLINE_SYNC,
									    offline_sync,
									    TRUE));
	} else {
		const gchar *uri = modest_server_account_settings_get_uri (settings);
		has_errors = !modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_URI,
							    uri, TRUE);
		if (!has_errors)
			(has_errors = !modest_account_mgr_set_string (self, account_name, MODEST_ACCOUNT_PROTO,
									   protocol_name, TRUE));
	}

	return !has_errors;

}


ModestAccountSettings *
modest_account_mgr_load_account_settings (ModestAccountMgr *self, 
					  const gchar* name)
{
	ModestAccountSettings *settings;
	gchar *string;
	gchar *server_account;
	gchar *default_account;
	gboolean use_signature = FALSE;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	
	if (!modest_account_mgr_account_exists (self, name, FALSE)) {
		/* For instance, maybe you are mistakenly checking for a server account name? */
		g_warning ("%s: Account %s does not exist.", __FUNCTION__, name);
		return NULL;
	}
	
	settings = modest_account_settings_new ();

	modest_account_settings_set_account_name (settings, name);

	string = modest_account_mgr_get_string (self, name,
						MODEST_ACCOUNT_DISPLAY_NAME,
						FALSE);
	modest_account_settings_set_display_name (settings, string);
	g_free (string);

 	string = modest_account_mgr_get_string (self, name,
						MODEST_ACCOUNT_FULLNAME,
						FALSE);
	modest_account_settings_set_fullname (settings, string);
	g_free (string);

	string = modest_account_mgr_get_string (self, name,
						MODEST_ACCOUNT_EMAIL,
						FALSE);
	modest_account_settings_set_email_address (settings, string);
	g_free (string);

	modest_account_settings_set_enabled (settings, modest_account_mgr_get_enabled (self, name));
	modest_account_settings_set_retrieve_type (settings, modest_account_mgr_get_retrieve_type (self, name));
	modest_account_settings_set_retrieve_limit (settings, modest_account_mgr_get_retrieve_limit (self, name));

	default_account    = modest_account_mgr_get_default_account (self);
	modest_account_settings_set_is_default (settings,
						(default_account && strcmp (default_account, name) == 0));
	g_free (default_account);

	string = modest_account_mgr_get_signature (self, name, &use_signature);
	modest_account_settings_set_use_signature (settings, use_signature);
	modest_account_settings_set_signature (settings, string);
	g_free (string);

	modest_account_settings_set_leave_messages_on_server 
		(settings, modest_account_mgr_get_leave_on_server (self, name));
	modest_account_settings_set_use_connection_specific_smtp 
		(settings, modest_account_mgr_get_use_connection_specific_smtp (self, name));

	/* store */
	server_account     = modest_account_mgr_get_string (self, name,
							    MODEST_ACCOUNT_STORE_ACCOUNT,
							    FALSE);
	if (server_account) {
		ModestServerAccountSettings *store_settings;
		store_settings = modest_account_mgr_load_server_settings (self, server_account, FALSE);
		g_free (server_account);

		/* It could happen that the account data is corrupted
		   so it's not loaded properly */
		if (store_settings) {
			modest_account_settings_set_store_settings (settings,
								    store_settings);
			g_object_unref (store_settings);
		} else {
			g_warning ("%s can not load server settings. Account corrupted?", __FUNCTION__);
			g_object_unref (settings);
			return NULL;
		}
	}

	/* transport */
	server_account = modest_account_mgr_get_string (self, name,
							MODEST_ACCOUNT_TRANSPORT_ACCOUNT,
							FALSE);
	if (server_account) {
		ModestServerAccountSettings *transport_settings;
		transport_settings = modest_account_mgr_load_server_settings (self, server_account, TRUE);
		g_free (server_account);

		if (transport_settings) {
			modest_account_settings_set_transport_settings (settings, transport_settings);
			g_object_unref (transport_settings);
		} else {
			g_warning ("%s can not load server settings. Account corrupted?", __FUNCTION__);
			g_object_unref (settings);
			return NULL;
		}
	}

	return settings;
}

void
modest_account_mgr_save_account_settings (ModestAccountMgr *mgr,
					  ModestAccountSettings *settings)
{
	const gchar *account_name;
	ModestServerAccountSettings *store_settings;
	ModestServerAccountSettings *transport_settings;

	g_return_if_fail (MODEST_IS_ACCOUNT_MGR (mgr));
	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	account_name = modest_account_settings_get_account_name (settings);
	g_return_if_fail (account_name != NULL);

	modest_account_mgr_set_display_name (mgr, account_name,
					     modest_account_settings_get_display_name (settings));
	modest_account_mgr_set_user_fullname (mgr, account_name,
					      modest_account_settings_get_fullname (settings));
	modest_account_mgr_set_user_email (mgr, account_name,
					   modest_account_settings_get_email_address (settings));
	modest_account_mgr_set_retrieve_type (mgr, account_name,
					      modest_account_settings_get_retrieve_type (settings));
	modest_account_mgr_set_retrieve_limit (mgr, account_name,
					       modest_account_settings_get_retrieve_limit (settings));
	modest_account_mgr_set_leave_on_server (mgr, account_name,
						modest_account_settings_get_leave_messages_on_server (settings));
	modest_account_mgr_set_signature (mgr, account_name,
					  modest_account_settings_get_signature (settings),
					  modest_account_settings_get_use_signature (settings));
	modest_account_mgr_set_use_connection_specific_smtp 
		(mgr, account_name,
		 modest_account_settings_get_use_connection_specific_smtp (settings));

	store_settings = modest_account_settings_get_store_settings (settings);
	if (store_settings) {
		const gchar *store_account_name;
		store_account_name = modest_server_account_settings_get_account_name (store_settings);
		if (store_account_name)
			modest_account_mgr_set_string (mgr, account_name, MODEST_ACCOUNT_STORE_ACCOUNT, 
						       store_account_name, FALSE);
		modest_account_mgr_save_server_settings (mgr, store_settings);
		g_object_unref (store_settings);
	}

	transport_settings = modest_account_settings_get_transport_settings (settings);
	if (transport_settings) {
		const gchar *transport_account_name;
		transport_account_name = modest_server_account_settings_get_account_name (transport_settings);
		if (transport_account_name)
			modest_account_mgr_set_string (mgr, account_name, MODEST_ACCOUNT_TRANSPORT_ACCOUNT, 
						       transport_account_name, FALSE);
		modest_account_mgr_save_server_settings (mgr, transport_settings);
		g_object_unref (transport_settings);
	}
	modest_account_mgr_set_bool (mgr, account_name, MODEST_ACCOUNT_ENABLED, TRUE,FALSE);
}


gint 
on_accounts_list_sort_by_title(gconstpointer a, gconstpointer b)
{
 	return g_utf8_collate((const gchar*)a, (const gchar*)b);
}

/** Get the first one, alphabetically, by title. */
gchar* 
modest_account_mgr_get_first_account_name (ModestAccountMgr *self)
{
	const gchar* account_name = NULL;
	GSList *account_names = modest_account_mgr_account_names (self, TRUE /* only enabled */);

	/* Return TRUE if there is no account */
	if (!account_names)
		return NULL;

	/* Get the first one, alphabetically, by title: */
	/* gchar *old_default = modest_account_mgr_get_default_account (self); */
	GSList* list_sorted = g_slist_sort (account_names, on_accounts_list_sort_by_title);

	GSList* iter = list_sorted;
	gboolean found = FALSE;
	while (iter && !found) {
		account_name = (const gchar*)list_sorted->data;

		if (account_name)
			found = TRUE;

		if (!found)
			iter = g_slist_next (iter);
	}

	gchar* result = NULL;
	if (account_name)
		result = g_strdup (account_name);
		
	modest_account_mgr_free_account_names (account_names);
	account_names = NULL;

	return result;
}

gboolean
modest_account_mgr_set_first_account_as_default (ModestAccountMgr *self)
{
	gboolean result = FALSE;
	
	gchar* account_name = modest_account_mgr_get_first_account_name(self);
	if (account_name) {
		result = modest_account_mgr_set_default_account (self, account_name);
		g_free (account_name);
	}
	else
		result = TRUE; /* If there are no accounts then it's not a failure. */

	return result;
}

gchar*
modest_account_mgr_get_from_string (ModestAccountMgr *self, const gchar* name, const gchar *mailbox)
{
	gchar *from;
	gchar *transport_account;
	gchar *proto;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);

	from = NULL;

	transport_account = modest_account_mgr_get_server_account_name (self,
									name,
									TNY_ACCOUNT_TYPE_TRANSPORT);
	if (transport_account) {
		proto = modest_account_mgr_get_string (self, transport_account, MODEST_ACCOUNT_PROTO, TRUE);
		if (proto != NULL) {
			ModestProtocol *protocol = 
				modest_protocol_registry_get_protocol_by_name (modest_runtime_get_protocol_registry (),
									       MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
									       proto);
			if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
				from = modest_account_protocol_get_from (MODEST_ACCOUNT_PROTOCOL (protocol),
									 name,
									 mailbox);
			}
			g_free (proto);
		}
	}

	if (from == NULL) {
		gchar *fullname, *email;

		fullname      = modest_account_mgr_get_string (self, name,MODEST_ACCOUNT_FULLNAME,
							       FALSE);
		email         = modest_account_mgr_get_string (self, name, MODEST_ACCOUNT_EMAIL,
							       FALSE);
		from = g_strdup_printf ("%s <%s>",
					fullname ? fullname : "",
					email    ? email    : "");
		g_free (fullname);
		g_free (email);
	}

	return from;
}

/* Add a number to the end of the text, or increment a number that is already there.
 */
static gchar*
util_increment_name (const gchar* text)
{
	g_return_val_if_fail (text, NULL);

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
  		
  		p = g_utf8_find_prev_char (text, p);	
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

void 
modest_account_mgr_set_leave_on_server (ModestAccountMgr *self, 
					const gchar *account_name, 
					gboolean leave_on_server)
{
	modest_account_mgr_set_bool (self, 
				     account_name,
				     MODEST_ACCOUNT_LEAVE_ON_SERVER, 
				     leave_on_server, 
				     FALSE);
}

gboolean 
modest_account_mgr_get_leave_on_server (ModestAccountMgr *self, 
					const gchar* account_name)
{
	return modest_account_mgr_get_bool (self, 
					    account_name,
					    MODEST_ACCOUNT_LEAVE_ON_SERVER, 
					    FALSE);
}

gint 
modest_account_mgr_get_last_updated (ModestAccountMgr *self, 
				     const gchar* account_name)
{
	return modest_account_mgr_get_int (modest_runtime_get_account_mgr (), 
					   account_name, 
					   MODEST_ACCOUNT_LAST_UPDATED, 
					   TRUE);
}

void 
modest_account_mgr_set_last_updated (ModestAccountMgr *self, 
				     const gchar* account_name,
				     gint time)
{
	modest_account_mgr_set_int (self, 
				    account_name, 
				    MODEST_ACCOUNT_LAST_UPDATED, 
				    time, 
				    TRUE);

	/* if 'account_name' is not defined, use "<null>" string */
	if (!account_name) {
		account_name = "<null>";
	}

	/* send a folder-updated signal, using a dummi folder id */
	modest_platform_emit_folder_updated_signal (account_name, "inbox");
}

gboolean
modest_account_mgr_get_has_new_mails (ModestAccountMgr *self, 
				      const gchar* account_name)
{
	return modest_account_mgr_get_bool (modest_runtime_get_account_mgr (), 
					    account_name, 
					    MODEST_ACCOUNT_HAS_NEW_MAILS, 
					    FALSE);
}

void 
modest_account_mgr_set_has_new_mails (ModestAccountMgr *self, 
				      const gchar* account_name,
				      gboolean has_new_mails)
{
	modest_account_mgr_set_bool (self, 
				     account_name, 
				     MODEST_ACCOUNT_HAS_NEW_MAILS, 
				     has_new_mails, 
				     FALSE);

	/* TODO: notify about changes */
}

gint  
modest_account_mgr_get_retrieve_limit (ModestAccountMgr *self, 
				       const gchar* account_name)
{
	return modest_account_mgr_get_int (self, 
					   account_name,
					   MODEST_ACCOUNT_LIMIT_RETRIEVE, 
					   FALSE);
}

void  
modest_account_mgr_set_retrieve_limit (ModestAccountMgr *self, 
				       const gchar* account_name,
				       gint limit_retrieve)
{
	modest_account_mgr_set_int (self, 
				    account_name,
				    MODEST_ACCOUNT_LIMIT_RETRIEVE, 
				    limit_retrieve, 
				    FALSE /* not server account */);
}

gint  
modest_account_mgr_get_server_account_port (ModestAccountMgr *self, 
					    const gchar* account_name)
{
	return modest_account_mgr_get_int (self, 
					   account_name,
					   MODEST_ACCOUNT_PORT, 
					   TRUE);
}

void
modest_account_mgr_set_server_account_port (ModestAccountMgr *self, 
					    const gchar *account_name,
					    gint port_num)
{
	modest_account_mgr_set_int (self, 
				    account_name,
				    MODEST_ACCOUNT_PORT, 
				    port_num, TRUE /* server account */);
}

gchar* 
modest_account_mgr_get_server_account_name (ModestAccountMgr *self, 
					    const gchar *account_name,
					    TnyAccountType account_type)
{
	return modest_account_mgr_get_string (self, 
					      account_name,
					      (account_type == TNY_ACCOUNT_TYPE_STORE) ?
					      MODEST_ACCOUNT_STORE_ACCOUNT :
					      MODEST_ACCOUNT_TRANSPORT_ACCOUNT, 
					      FALSE);
}

static const gchar *
get_retrieve_type_name (ModestAccountRetrieveType retrieve_type)
{
	switch(retrieve_type) {
	case MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY:
		return MODEST_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY;
		break;
	case MODEST_ACCOUNT_RETRIEVE_MESSAGES:
		return MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES;
		break;
	case MODEST_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS:
		return MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES_AND_ATTACHMENTS;
		break;
	default:
		return MODEST_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY;
	};
}

static ModestAccountRetrieveType
get_retrieve_type (const gchar *name)
{
	if (!name || name[0] == 0)
		return MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY;
	if (strcmp (name, MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES) == 0) {
		return MODEST_ACCOUNT_RETRIEVE_MESSAGES;
	} else if (strcmp (name, MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES_AND_ATTACHMENTS) == 0) {
		return MODEST_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS;
	} else {
		/* we fall back to headers only */
		return MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY;
	}
}

ModestAccountRetrieveType
modest_account_mgr_get_retrieve_type (ModestAccountMgr *self, 
				      const gchar *account_name)
{
	gchar *string;
	ModestAccountRetrieveType result;

	string =  modest_account_mgr_get_string (self, 
						 account_name,
						 MODEST_ACCOUNT_RETRIEVE, 
						 FALSE /* not server account */);
	result = get_retrieve_type (string);
	g_free (string);

	return result;
}

void 
modest_account_mgr_set_retrieve_type (ModestAccountMgr *self, 
				      const gchar *account_name,
				      ModestAccountRetrieveType retrieve_type)
{
	modest_account_mgr_set_string (self, 
				       account_name,
				       MODEST_ACCOUNT_RETRIEVE, 
				       get_retrieve_type_name (retrieve_type), 
				       FALSE /* not server account */);
}


void
modest_account_mgr_set_user_fullname (ModestAccountMgr *self, 
				      const gchar *account_name,
				      const gchar *fullname)
{
	modest_account_mgr_set_string (self, 
				       account_name,
				       MODEST_ACCOUNT_FULLNAME, 
				       fullname, 
				       FALSE /* not server account */);
}

void
modest_account_mgr_set_user_email (ModestAccountMgr *self, 
				   const gchar *account_name,
				   const gchar *email)
{
	modest_account_mgr_set_string (self, 
				       account_name,
				       MODEST_ACCOUNT_EMAIL, 
				       email, 
				       FALSE /* not server account */);
}

gboolean 
modest_account_mgr_account_is_multimailbox (ModestAccountMgr *mgr,
					    const gchar *account_name, 
					    ModestProtocol **mmb_protocol)
{
	gchar *transport_account;
	gboolean result = FALSE;

	g_return_val_if_fail (account_name, FALSE);

	if (mmb_protocol)
		*mmb_protocol = NULL;

	transport_account = modest_account_mgr_get_server_account_name (mgr,
									account_name,
									TNY_ACCOUNT_TYPE_TRANSPORT);
	if (transport_account) {
		gchar *proto;
		ModestProtocolRegistry *registry;

		registry = modest_runtime_get_protocol_registry ();

		proto = modest_account_mgr_get_string (mgr, transport_account, 
						       MODEST_ACCOUNT_PROTO, TRUE);
		if (proto != NULL) {
			ModestProtocol *protocol = 
				modest_protocol_registry_get_protocol_by_name (registry,
									       MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
									       proto);
			if (protocol &&
			    modest_protocol_registry_protocol_type_has_tag 
			    (registry,
			     modest_protocol_get_type_id (protocol),
			     MODEST_PROTOCOL_REGISTRY_MULTI_MAILBOX_PROVIDER_PROTOCOLS)) {
				if (mmb_protocol)
					*mmb_protocol = protocol;
				result = TRUE;
			}
			
		}
	}

	return result;
}

gchar *
modest_account_mgr_get_signature_from_recipient (ModestAccountMgr *mgr, 
						 const gchar *current_recipient, 
						 gboolean *has_signature)
{
	gchar *result = NULL;
	gchar *mailbox = NULL;
	gchar *account_name;
	ModestProtocol *protocol = NULL;

	*has_signature = FALSE;

	account_name = modest_utils_get_account_name_from_recipient (current_recipient, &mailbox);
	if (modest_account_mgr_account_is_multimailbox (mgr, account_name, &protocol)) {
		if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
			result = modest_account_protocol_get_signature (MODEST_ACCOUNT_PROTOCOL (protocol),
									account_name, mailbox, 
									has_signature);
		}
	}

	if (result == NULL) {
		result = modest_account_mgr_get_signature (mgr, 
							   account_name, has_signature);
	}
	return result;
}

void
modest_account_mgr_get_branding_from_recipient (ModestAccountMgr *mgr, 
						const gchar *current_recipient, 
						gchar **service_name,
						const GdkPixbuf **service_icon,
						guint64 icon_size)
{
	gchar *mailbox = NULL;
	gchar *account_name;
	ModestProtocol *protocol = NULL;

	*service_name = NULL;
	*service_icon = NULL;

	account_name = modest_utils_get_account_name_from_recipient (current_recipient, &mailbox);
	if (modest_account_mgr_account_is_multimailbox (mgr, account_name, &protocol)) {
		if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
			gchar *name;
			const GdkPixbuf *icon;
			name = modest_account_protocol_get_service_name (MODEST_ACCOUNT_PROTOCOL (protocol),
									 account_name, mailbox);
			icon = modest_account_protocol_get_service_icon (MODEST_ACCOUNT_PROTOCOL (protocol),
									 account_name, mailbox, icon_size);
			*service_name = name;
			*service_icon = icon;
		}
	}

}


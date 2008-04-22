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

#include <modest-platform.h>
#include <modest-tny-platform-factory.h>
#include <modest-tny-account.h>
#include <modest-tny-account-store.h>
#include <modest-default-connection-policy.h>
#include <modest-tny-local-folders-account.h>
#include <modest-runtime.h>
#include <tny-simple-list.h>
#include <modest-tny-folder.h>
#include <modest-tny-outbox-account.h>
#include <modest-transport-account-decorator.h>
#include <modest-account-mgr-helpers.h>
#include <modest-init.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <tny-folder-stats.h>
#include <modest-debug.h>
#include <string.h>
#ifdef MODEST_PLATFORM_MAEMO
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <hildon-widgets/hildon-file-system-info.h>
#else
#include <hildon/hildon-file-system-info.h>
#endif
#endif

/* we need these dummy functions, or tinymail will complain */
static gchar *  get_pass_dummy     (TnyAccount *account, const gchar *prompt, gboolean *cancel);
static void     forget_pass_dummy  (TnyAccount *account);

TnyFolder *
modest_tny_account_get_special_folder (TnyAccount *account,
				       TnyFolderType special_type)
{
	TnyList *folders = NULL;
	TnyIterator *iter = NULL;
	TnyFolder *special_folder = NULL;
	TnyAccount *local_account  = NULL;
	GError *error = NULL;
	
	g_return_val_if_fail (account, NULL);
	g_return_val_if_fail (0 <= special_type && special_type < TNY_FOLDER_TYPE_NUM,
			      NULL);
	
	/* The accounts have already been instantiated by 
	 * modest_tny_account_store_get_accounts(), which is the 
	 * TnyAccountStore::get_accounts_func() implementation,
	 * so we just get them here.
	 */
	 
	/* Per-account outbox folders are each in their own on-disk directory: */
	if ((special_type == TNY_FOLDER_TYPE_OUTBOX) &&
	    (!modest_tny_account_is_virtual_local_folders (account))) {

		gchar *account_id;
		const gchar *modest_account_name;

		modest_account_name =
			modest_tny_account_get_parent_modest_account_name_for_server_account (account);
		if (!modest_account_name) {
			g_warning ("%s: could not get modest account name", __FUNCTION__);
			return NULL;
		}
		
		account_id = g_strdup_printf (
			MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDER_ACCOUNT_ID_PREFIX "%s", 
			modest_account_name);
		
		local_account = modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store(),
									     MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
									     account_id);
		g_free (account_id);		
	} else 
		/* Other local folders are all in one on-disk directory: */
		local_account = modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store(),
									     MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
									     MODEST_LOCAL_FOLDERS_ACCOUNT_ID);
	if (!local_account) {
		g_printerr ("modest: cannot get local account\n");
		goto cleanup;
	}

	folders = TNY_LIST (tny_simple_list_new ());
	
	/* There is no need to do this _async, as these are local folders. */
	/* TODO: However, this seems to fail sometimes when the network is busy, 
	 * returning an empty list. murrayc. */	
	tny_folder_store_get_folders (TNY_FOLDER_STORE (local_account), folders, NULL, &error);
	if (error) {
		g_warning ("%s: tny_folder_store_get_folders() failed:%s\n", __FUNCTION__, error->message);
		g_error_free (error);
		goto cleanup;
	}
				      
	if (tny_list_get_length (folders) == 0) {
		gchar* url_string = tny_account_get_url_string (local_account);
		g_printerr ("modest: %s: tny_folder_store_get_folders(%s) returned an empty list\n", 
			    __FUNCTION__, url_string);
		g_free (url_string);
		goto cleanup;
	}
	
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
		if (folder) {
			if (modest_tny_folder_get_local_or_mmc_folder_type (folder) == special_type) {
				special_folder = folder;
				break; /* Leaving a ref for the special_folder return value. */
			}
			g_object_unref (folder);
		}
		tny_iterator_next (iter);
	}
	
cleanup:
	if (folders)
		g_object_unref (folders);
	if (iter)
		g_object_unref (iter);
	if (local_account)
		g_object_unref (local_account);
	
	return special_folder;
}

/**
 * create_tny_account:
 * @session: A valid TnySessionCamel instance.
 * @account_data: the server account for which to create a corresponding tny account
 * 
 * get a tnyaccount corresponding to the server_accounts (store or transport) for this account.
 * NOTE: this function does not set the camel session or the get/forget password functions
 * 
 * Returns: a new TnyAccount or NULL in case of error.
 */
static TnyAccount*
create_tny_account (TnySessionCamel *session,
		    ModestServerAccountSettings *server_settings)
{
	TnyAccount *tny_account = NULL;
	ModestTransportStoreProtocol protocol;
	const gchar* proto_name;
	
	g_return_val_if_fail (session, NULL);
	g_return_val_if_fail (server_settings, NULL);
	protocol = modest_server_account_settings_get_protocol (server_settings);
	g_return_val_if_fail (protocol != MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN, NULL);
	
	switch (protocol) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		tny_account = TNY_ACCOUNT (modest_transport_account_decorator_new ()); break;
	case MODEST_PROTOCOL_STORE_POP:
		tny_account = TNY_ACCOUNT(tny_camel_pop_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_IMAP:
		tny_account = TNY_ACCOUNT(tny_camel_imap_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		/* Note that this is not where we create the special local folders account.
		 * That happens in modest_tny_account_new_for_local_folders() instead.
		 */
		tny_account = TNY_ACCOUNT(tny_camel_store_account_new()); break;
	default:
		g_return_val_if_reached (NULL);
	}

	if (!tny_account) {
		g_printerr ("modest: %s: could not create tny account for '%s'\n",
			    __FUNCTION__, modest_server_account_settings_get_account_name (server_settings));
		return NULL;
	}
	tny_account_set_id (tny_account, modest_server_account_settings_get_account_name (server_settings));

	/* This must be set quite early, or other set() functions will fail. */
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (tny_account), session);
    
	/* Proto */
	proto_name =  modest_protocol_info_get_transport_store_protocol_name(protocol);
	tny_account_set_proto (tny_account, proto_name);

	return tny_account;
}



/* Camel options: */

/* These seem to be listed in 
 * libtinymail-camel/camel-lite/camel/providers/imap/camel-imap-store.c 
 */
#define MODEST_ACCOUNT_OPTION_SSL "use_ssl"
#define MODEST_ACCOUNT_OPTION_SSL_NEVER "never"
/* This is a tinymail camel-lite specific option, 
 * roughly equivalent to "always" in regular camel,
 * which is appropriate for a generic "SSL" connection option: */
#define MODEST_ACCOUNT_OPTION_SSL_WRAPPED "wrapped"
/* Not used in our UI so far: */
#define MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE "when-possible"
/* This is a tinymailcamel-lite specific option that is not in regular camel. */
#define MODEST_ACCOUNT_OPTION_SSL_TLS "tls"

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
 * update_tny_account:
 * @account_mgr: a valid account mgr instance
 * @account_data: the server account for which to create a corresponding tny account
 * 
 * get a tnyaccount corresponding to the server_accounts (store or transport) for this account.
 * NOTE: this function does not set the camel session or the get/forget password functions
 * 
 * Returns: a new TnyAccount or NULL in case of error.
 */
static gboolean
update_tny_account (TnyAccount *tny_account,
		    ModestServerAccountSettings *server_settings)
{
	const gchar *account_name;
	const gchar *uri;
	g_return_val_if_fail (server_settings, FALSE);
	account_name = modest_server_account_settings_get_account_name (server_settings);
	g_return_val_if_fail (account_name, FALSE);
	g_return_val_if_fail (tny_account, FALSE);
	
	/* Do not change the id if it's not needed */
	if (tny_account_get_id (tny_account) && 
	    strcmp (tny_account_get_id (tny_account), account_name))
		tny_account_set_id (tny_account, account_name);
	
	/* mbox and maildir accounts use a URI instead of the rest:
	 * Note that this is not where we create the special local folders account.
	 * We do that in modest_tny_account_new_for_local_folders() instead. */
	uri = modest_server_account_settings_get_uri (server_settings);
	if (uri)  
		tny_account_set_url_string (TNY_ACCOUNT(tny_account), uri);
	else {
		/* Set camel-specific options: */		
		/* Enable secure connection settings: */
		TnyPair *option_security = NULL;
		const gchar* auth_mech_name = NULL;
		ModestTransportStoreProtocol protocol;
		ModestConnectionProtocol security;
		ModestAuthProtocol auth_protocol;
		const gchar *username;
		const gchar *hostname;
		guint port;

		/* First of all delete old options */
		tny_camel_account_clear_options (TNY_CAMEL_ACCOUNT (tny_account));

		protocol = modest_server_account_settings_get_protocol (server_settings);
		security = modest_server_account_settings_get_security (server_settings);
		auth_protocol = modest_server_account_settings_get_auth_protocol (server_settings);

		switch (security) {
		case MODEST_PROTOCOL_CONNECTION_NORMAL:
			option_security = tny_pair_new (MODEST_ACCOUNT_OPTION_SSL,MODEST_ACCOUNT_OPTION_SSL_NEVER);
			break;
		case MODEST_PROTOCOL_CONNECTION_SSL:
			/* Apparently, use of "IMAPS" (specified in our UI spec), implies 
			 * use of the "wrapped" option: */
			option_security = tny_pair_new (MODEST_ACCOUNT_OPTION_SSL,MODEST_ACCOUNT_OPTION_SSL_WRAPPED);
			break;
		case MODEST_PROTOCOL_CONNECTION_TLS:
			option_security = tny_pair_new (MODEST_ACCOUNT_OPTION_SSL,MODEST_ACCOUNT_OPTION_SSL_TLS);
			break;
		case MODEST_PROTOCOL_CONNECTION_TLS_OP:
			/* This is not actually in our UI: */
			option_security = tny_pair_new (MODEST_ACCOUNT_OPTION_SSL,MODEST_ACCOUNT_OPTION_SSL_WHEN_POSSIBLE);
			break;
		default:
			break;
		}
		
		if(option_security)
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account), option_security);
		
		/* Secure authentication: */
		switch (auth_protocol) {
		case MODEST_PROTOCOL_AUTH_NONE:
			/* IMAP and POP need at least a password,
			 * which camel uses if we specify NULL.
			 * This setting should never happen anyway. */
			if (protocol == MODEST_PROTOCOL_STORE_IMAP ||
			    protocol == MODEST_PROTOCOL_STORE_POP)
				auth_mech_name = NULL;
			else if (protocol == MODEST_PROTOCOL_TRANSPORT_SMTP)
				auth_mech_name = NULL; 
			else
				auth_mech_name = MODEST_ACCOUNT_AUTH_PLAIN;
			break;
			
		case MODEST_PROTOCOL_AUTH_PASSWORD:
			/* Camel use a password for IMAP or POP if we specify NULL,
			 * For IMAP, at least it will report an error if we use "Password", "Login" or "Plain".
			 * (POP is know to report an error for Login too. Probably Password and Plain too.) */
			if (protocol == MODEST_PROTOCOL_STORE_IMAP)
				auth_mech_name = NULL;
			else if (protocol == MODEST_PROTOCOL_STORE_POP)
				auth_mech_name = NULL;
			else
				auth_mech_name = MODEST_ACCOUNT_AUTH_PASSWORD;
			break;
			
		case MODEST_PROTOCOL_AUTH_CRAMMD5:
			auth_mech_name = MODEST_ACCOUNT_AUTH_CRAMMD5;
			break;
			
		default:
			g_warning ("%s: Unhandled secure authentication setting %d for "
				   "account_name=%s (%s)", __FUNCTION__, auth_protocol,
				   account_name, modest_server_account_settings_get_hostname (server_settings));
			break;
		}
		
		if(auth_mech_name) 
			tny_account_set_secure_auth_mech (tny_account, auth_mech_name);
		
		if (modest_protocol_info_protocol_is_store(protocol) && 
			(protocol == MODEST_PROTOCOL_STORE_IMAP) ) {
			/* Other connection options, needed for IMAP. */
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      tny_pair_new (MODEST_ACCOUNT_OPTION_USE_LSUB, ""));
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      tny_pair_new (MODEST_ACCOUNT_OPTION_CHECK_ALL, ""));
		}
		
		username = modest_server_account_settings_get_username (server_settings);
		if (username && strlen (username) > 0) 
			tny_account_set_user (tny_account, username);
		hostname = modest_server_account_settings_get_hostname (server_settings);
		if (hostname)
			tny_account_set_hostname (tny_account, hostname);
		 
		/* Set the port: */
		port = modest_server_account_settings_get_port (server_settings);
		if (port)
			tny_account_set_port (tny_account, port);
	}

	MODEST_DEBUG_BLOCK (
		gchar *url = tny_account_get_url_string (TNY_ACCOUNT(tny_account));
		g_debug ("%s:\n  account-url: %s\n", __FUNCTION__, url);
		g_free (url);
	);
	
	return TRUE;
}

TnyAccount*
modest_tny_account_new_from_server_account_name (ModestAccountMgr *account_mgr,
						 TnySessionCamel *session,
						 const gchar *server_account_name,
						 TnyGetPassFunc get_pass_func,
						 TnyForgetPassFunc forget_pass_func)
{
	ModestServerAccountSettings *server_settings;
	TnyAccount *tny_account;
	
	g_return_val_if_fail (session, NULL);
	g_return_val_if_fail (server_account_name, NULL);

	
	server_settings = 	modest_account_mgr_load_server_settings (account_mgr, server_account_name);
	if (!server_settings)
		return NULL;

	tny_account = TNY_ACCOUNT (tny_camel_transport_account_new ());

	if (tny_account) {
		const gchar* proto_name = NULL;
		tny_account_set_id (tny_account, server_account_name);
		tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (tny_account), session);
		proto_name = modest_protocol_info_get_transport_store_protocol_name 
				    (modest_server_account_settings_get_protocol (server_settings));
		tny_account_set_proto (tny_account, proto_name);
		modest_tny_account_set_parent_modest_account_name_for_server_account (tny_account, server_account_name);
	}

	if (!tny_account)
		g_warning ("%s: failed to create tny_account", __FUNCTION__);
	else if (!update_tny_account (tny_account, server_settings))
		g_warning ("%s: failed to initialize tny_account", __FUNCTION__);
	else {
		tny_account_set_forget_pass_func (tny_account,
						  forget_pass_func ? forget_pass_func : forget_pass_dummy);
		tny_account_set_pass_func (tny_account,
					   get_pass_func ? get_pass_func: get_pass_dummy);
	}

	tny_account_set_connection_policy (tny_account, modest_default_connection_policy_new ());

	g_object_unref (server_settings);
	
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


static void
set_online_callback (TnyCamelAccount *account, gboolean canceled, GError *err, gpointer user_data)
{
	TnyAccountStore *account_store;

	account_store = TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(account),
							     "account_store"));
	if (err && !canceled) {
		/* It seems err is forgotten here ... if the disk is full ! */
		if (account_store) {
			tny_account_store_alert (
				account_store, 
				TNY_ACCOUNT (account), TNY_ALERT_TYPE_ERROR, FALSE, 
				err);
		}
		g_warning ("err: %s", err->message);
	}
}

gboolean
modest_tny_account_update_from_account (TnyAccount *tny_account,
					TnyGetPassFunc get_pass_func,
					TnyForgetPassFunc forget_pass_func) 
{
	ModestAccountSettings *settings = NULL;
	ModestServerAccountSettings *server_settings = NULL;
	ModestAccountMgr *account_mgr;
	const gchar *account_name;
	TnyAccountType type;
	const gchar *display_name;
	TnyConnectionStatus conn_status;

	g_return_val_if_fail (tny_account, FALSE);

	account_mgr = modest_runtime_get_account_mgr ();
	account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (tny_account);
	type = tny_account_get_account_type (tny_account);
	settings = modest_account_mgr_load_account_settings (account_mgr, account_name);
	if (!settings) {
		g_printerr ("modest: %s: cannot get account data for account %s\n",
			    __FUNCTION__, account_name);
		return FALSE;
	}

	display_name = modest_account_settings_get_display_name (settings);

	if (type == TNY_ACCOUNT_TYPE_STORE)
		server_settings = modest_account_settings_get_store_settings (settings);
	else if (type == TNY_ACCOUNT_TYPE_TRANSPORT)
		server_settings = modest_account_settings_get_transport_settings (settings);

	if (modest_server_account_settings_get_account_name (server_settings) == NULL) {
		g_printerr ("modest: no %s account defined for '%s'\n",
			    type == TNY_ACCOUNT_TYPE_STORE ? "store" : "transport",
			    display_name);
		g_object_unref (server_settings);
		g_object_unref (settings);
		return FALSE;
	}
	
	update_tny_account (tny_account, server_settings);
		
	/* This name is what shows up in the folder view -- so for some POP/IMAP/... server
 	 * account, we set its name to the account of which it is part. */

	if (display_name)
		tny_account_set_name (tny_account, display_name);

	g_object_unref (server_settings);
	g_object_unref (settings);

	tny_account_set_forget_pass_func (tny_account, NULL);
	tny_account_set_pass_func (tny_account, NULL);
	tny_account_set_forget_pass_func (tny_account,
					  forget_pass_func ? forget_pass_func : forget_pass_dummy);
	tny_account_set_pass_func (tny_account,
				   get_pass_func ? get_pass_func: get_pass_dummy);

	tny_account_set_connection_policy (tny_account, modest_default_connection_policy_new ());
	
	/* The callback will have an error for you if the reconnect
	 * failed. Please handle it (this is TODO). */
	
	conn_status = tny_account_get_connection_status (tny_account);
	if (conn_status != TNY_CONNECTION_STATUS_DISCONNECTED) {
		TnyAccountStore *account_store = NULL;
	
		account_store = TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(tny_account),
								     "account_store"));
	
		if (account_store) {
			modest_tny_account_store_forget_already_asked (MODEST_TNY_ACCOUNT_STORE (account_store), 
								       tny_account);
		}
	
		tny_camel_account_set_online (TNY_CAMEL_ACCOUNT(tny_account), TRUE, 
					      set_online_callback,  "online");
	}
	
	return TRUE;
}



TnyAccount*
modest_tny_account_new_from_account (ModestAccountMgr *account_mgr,
				     const gchar *account_name,
				     TnyAccountType type,
				     TnySessionCamel *session,
				     TnyGetPassFunc get_pass_func,
				     TnyForgetPassFunc forget_pass_func) 
{
	TnyAccount *tny_account = NULL;
	ModestAccountSettings *settings = NULL;
	ModestServerAccountSettings *server_settings = NULL;
	const gchar *display_name;

	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (account_name, NULL);
	g_return_val_if_fail (session, NULL);
	g_return_val_if_fail (type == TNY_ACCOUNT_TYPE_STORE || type == TNY_ACCOUNT_TYPE_TRANSPORT,
			      NULL);

	settings = modest_account_mgr_load_account_settings (account_mgr, account_name);
	if (!settings) {
		g_printerr ("modest: %s: cannot get account data for account %s\n",
			    __FUNCTION__, account_name);
		return NULL;
	}
	display_name = modest_account_settings_get_display_name (settings);

	if (type == TNY_ACCOUNT_TYPE_STORE)
		server_settings = modest_account_settings_get_store_settings (settings);
	else if (type == TNY_ACCOUNT_TYPE_TRANSPORT)
		server_settings = modest_account_settings_get_transport_settings (settings);

	if (modest_server_account_settings_get_account_name (server_settings) == NULL) {
		g_printerr ("modest: no %s account defined for '%s'\n",
			    type == TNY_ACCOUNT_TYPE_STORE ? "store" : "transport",
			    display_name);
		g_object_unref (server_settings);
		g_object_unref (settings);
		return NULL;
	}
	
	tny_account = create_tny_account (session, server_settings);
	if (!tny_account) { 
		g_printerr ("modest: failed to create tny account for %s (%s)\n",
			    account_name, 
			    modest_server_account_settings_get_account_name (server_settings));
		g_object_unref (server_settings);
		g_object_unref (settings);
		return NULL;
	} else
		update_tny_account (tny_account, server_settings);
		
	/* This name is what shows up in the folder view -- so for some POP/IMAP/... server
 	 * account, we set its name to the account of which it is part. */
 
	if (display_name)
		tny_account_set_name (tny_account, display_name);

	tny_account_set_forget_pass_func (tny_account,
					  forget_pass_func ? forget_pass_func : forget_pass_dummy);
	tny_account_set_pass_func (tny_account,
				   get_pass_func ? get_pass_func: get_pass_dummy);

	tny_account_set_connection_policy (tny_account, modest_default_connection_policy_new ());

        modest_tny_account_set_parent_modest_account_name_for_server_account (tny_account,
									      account_name);
	g_object_unref (server_settings);
	g_object_unref (settings);

	return tny_account;
}

typedef struct
{
	TnyStoreAccount *account;
	
	ModestTnyAccountGetMmcAccountNameCallback callback;
	gpointer user_data;
} GetMmcAccountNameData;



#ifdef MODEST_PLATFORM_MAEMO
/* Gets the memory card name: */
static void 
on_modest_file_system_info (HildonFileSystemInfoHandle *handle,
			    HildonFileSystemInfo *info,
			    const GError *error, gpointer data)
{
	GetMmcAccountNameData *callback_data = (GetMmcAccountNameData*)data;

	if (error) {
		g_warning ("%s: error=%s", __FUNCTION__, error->message);
  	}
	
	TnyAccount *account = TNY_ACCOUNT (callback_data->account);
	
	const gchar *previous_display_name = NULL;
	
	const gchar *display_name = NULL;
	if (!error && info) {
		display_name = hildon_file_system_info_get_display_name(info);
		previous_display_name = tny_account_get_name (account);
	}
		 
	/* printf ("DEBUG: %s: display name=%s\n", __FUNCTION__,  display_name); */
	if (display_name && previous_display_name && 
		(strcmp (display_name, previous_display_name) != 0)) {
		tny_account_set_name (account, display_name);
	}
		
	/* Inform the application that the name is now ready: */
	if (callback_data->callback)
		(*(callback_data->callback)) (callback_data->account, 
			callback_data->user_data);
	
	g_object_unref (callback_data->account);
	g_slice_free (GetMmcAccountNameData, callback_data);
}
#endif

void modest_tny_account_get_mmc_account_name (TnyStoreAccount* self, ModestTnyAccountGetMmcAccountNameCallback callback, gpointer user_data)
{
#ifdef MODEST_PLATFORM_MAEMO
	/* Just use the hard-coded path for the single memory card,
	 * rather than try to figure out the path to the specific card by 
	 * looking at the maildir URI:
	 */
	const gchar *uri_real = MODEST_MCC1_VOLUMEPATH_URI;

	/*
	gchar* uri = tny_account_get_url_string (TNY_ACCOUNT (self));
	if (!uri)
		return;

	TODO: This gets the name of the folder, but we want the name of the volume.
	gchar *uri_real = NULL;
	const gchar* prefix = "maildir://localhost/";
	if ((strstr (uri, prefix) == uri) && (strlen(uri) > strlen(prefix)) )
		uri_real = g_strconcat ("file:///", uri + strlen (prefix), NULL);
	*/

	if (uri_real) {
		//This is freed in the callback:
		GetMmcAccountNameData * callback_data = g_slice_new0(GetMmcAccountNameData);
		callback_data->account = self;
		g_object_ref (callback_data->account); /* Unrefed when we destroy the struct. */
		callback_data->callback = callback;
		callback_data->user_data = user_data;
		
		/* TODO: gnome_vfs_volume_get_display_name() does not return 
		 * the same string. But why not? Why does hildon needs its own 
		 * function for this?
		 */
		/* printf ("DEBUG: %s Calling hildon_file_system_info_async_new() with URI=%s\n", __FUNCTION__, uri_real); */
		hildon_file_system_info_async_new(uri_real, 
			on_modest_file_system_info, callback_data /* user_data */);

		/* g_free (uri_real); */
	}

	/* g_free (uri); */
#endif
}

 				

TnyAccount*
modest_tny_account_new_for_local_folders (ModestAccountMgr *account_mgr, TnySessionCamel *session,
					  const gchar* location_filepath)
{

	
	/* Make sure that the directories exist: */
	modest_init_local_folders (location_filepath);

	TnyStoreAccount *tny_account;
	CamelURL *url;
	gchar *maildir, *url_string;

	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (session, NULL);

	
	if (!location_filepath) {
		/* A NULL filepath means that this is the special local-folders maildir 
		 * account: */
		tny_account = TNY_STORE_ACCOUNT (modest_tny_local_folders_account_new ());
	}
	else {
		/* Else, for instance, a per-account outbox maildir account: */
		tny_account = TNY_STORE_ACCOUNT (tny_camel_store_account_new ());
	}
		
	if (!tny_account) {
		g_printerr ("modest: %s: cannot create account for local folders. filepath=%s", 
			__FUNCTION__, location_filepath);
		return NULL;
	}
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	/* This path contains directories for each local folder.
	 * We have created them so that TnyCamelStoreAccount can find them 
	 * and report a folder for each directory: */
	maildir = modest_local_folder_info_get_maildir_path (location_filepath);
	url = camel_url_new ("maildir:", NULL);
	camel_url_set_path (url, maildir);
	/* Needed by tinymail's DBC assertions */
 	camel_url_set_host (url, "localhost");
	url_string = camel_url_to_string (url, 0);
	
	tny_account_set_url_string (TNY_ACCOUNT(tny_account), url_string);
/* 	printf("DEBUG: %s:\n  url=%s\n", __FUNCTION__, url_string); */

	/* TODO: Use a more generic way of identifying memory card paths, 
	 * and of marking accounts as memory card accounts, maybe
	 * via a derived TnyCamelStoreAccount ? */
	const gboolean is_mmc = 
		location_filepath && 
		(strcmp (location_filepath, MODEST_MCC1_VOLUMEPATH) == 0);
		
	/* The name of memory card locations will be updated asynchronously.
	 * This is just a default: */
	const gchar *name = is_mmc ? _("Memory Card") : 
		MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME;
	tny_account_set_name (TNY_ACCOUNT(tny_account), name); 
	
 	/* Get the correct display name for memory cards, asynchronously: */
 	if (location_filepath) {
 		GError *error = NULL;
 		gchar *uri = g_filename_to_uri(location_filepath, NULL, &error);
 		if (error) {
 			g_warning ("%s: g_filename_to_uri(%s) failed: %s", __FUNCTION__, 
 				location_filepath, error->message);
 			g_error_free (error);
 			error = NULL;	
 		} else if (uri) {
 			/* Get the account name asynchronously:
 			 * This might not happen soon enough, so some UI code might 
 			 * need to call this again, specifying a callback.
 			 */
 			modest_tny_account_get_mmc_account_name (tny_account, NULL, NULL);
 				
 			g_free (uri);
 			uri = NULL;
 		}
 	}
 	
	
	const gchar* id = is_mmc ? MODEST_MMC_ACCOUNT_ID :
		MODEST_LOCAL_FOLDERS_ACCOUNT_ID;
	tny_account_set_id (TNY_ACCOUNT(tny_account), id);
	
	tny_account_set_forget_pass_func (TNY_ACCOUNT(tny_account), forget_pass_dummy);
	tny_account_set_pass_func (TNY_ACCOUNT(tny_account), get_pass_dummy);

	tny_account_set_connection_policy (TNY_ACCOUNT (tny_account), modest_default_connection_policy_new ());	

	modest_tny_account_set_parent_modest_account_name_for_server_account (
		TNY_ACCOUNT (tny_account), id);
	
	camel_url_free (url);
	g_free (maildir);
	g_free (url_string);

	return TNY_ACCOUNT(tny_account);
}


TnyAccount*
modest_tny_account_new_for_per_account_local_outbox_folder (ModestAccountMgr *account_mgr,
							    const gchar* account_name,
							    TnySessionCamel *session)
{
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (account_name, NULL);
	g_return_val_if_fail (session, NULL);
	
	/* Notice that we create a ModestTnyOutboxAccount here, 
	 * instead of just a TnyCamelStoreAccount,
	 * so that we can later identify this as a special account for internal use only.
	 */
	TnyStoreAccount *tny_account = TNY_STORE_ACCOUNT (modest_tny_outbox_account_new ());
	if (!tny_account) {
		g_printerr ("modest: cannot create account for per-account local outbox folder.");
		return NULL;
	}
	
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	/* Make sure that the paths exists on-disk so that TnyCamelStoreAccount can 
	 * find it to create a TnyFolder for it: */
	gchar *folder_dir = modest_per_account_local_outbox_folder_info_get_maildir_path_to_outbox_folder (account_name); 
	modest_init_one_local_folder(folder_dir);
	g_free (folder_dir);
	folder_dir = NULL;

	/* This path should contain just one directory - "outbox": */
	gchar *maildir = 
		modest_per_account_local_outbox_folder_info_get_maildir_path (account_name);
			
	CamelURL *url = camel_url_new ("maildir:", NULL);
	camel_url_set_path (url, maildir);
	g_free (maildir);
	
	/* Needed by tinymail's DBC assertions */
 	camel_url_set_host (url, "localhost");
	gchar *url_string = camel_url_to_string (url, 0);
	camel_url_free (url);
	
	tny_account_set_url_string (TNY_ACCOUNT(tny_account), url_string);
/* 	printf("DEBUG: %s:\n  url=%s\n", __FUNCTION__, url_string); */
	g_free (url_string);

	/* This text should never been seen,
	 * because the per-account outbox accounts are not seen directly by the user.
	 * Their folders are merged and shown as one folder. */ 
	tny_account_set_name (TNY_ACCOUNT(tny_account), "Per-Account Outbox"); 
	
	gchar *account_id = g_strdup_printf (
		MODEST_PER_ACCOUNT_LOCAL_OUTBOX_FOLDER_ACCOUNT_ID_PREFIX "%s", 
		account_name);
	tny_account_set_id (TNY_ACCOUNT(tny_account), account_id);
	g_free (account_id);
	
	tny_account_set_forget_pass_func (TNY_ACCOUNT(tny_account), forget_pass_dummy);
	tny_account_set_pass_func (TNY_ACCOUNT(tny_account), get_pass_dummy);

	tny_account_set_connection_policy (TNY_ACCOUNT (tny_account), modest_default_connection_policy_new ());

	/* Make this think that it belongs to the modest local-folders parent account: */
	modest_tny_account_set_parent_modest_account_name_for_server_account (
		TNY_ACCOUNT (tny_account), MODEST_LOCAL_FOLDERS_ACCOUNT_ID);

	return TNY_ACCOUNT(tny_account);
}



typedef gint (*TnyStatsFunc) (TnyFolderStats *stats);
#define TASK_GET_ALL_COUNT	0
#define TASK_GET_LOCAL_SIZE	1
#define TASK_GET_FOLDER_COUNT	2

typedef struct _RecurseFoldersHelper {
	gint task;
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
		TnyFolder *folder;

		folder = TNY_FOLDER (tny_iterator_get_current (iter));
		if (folder) {
			if (helper->task == TASK_GET_ALL_COUNT)
				helper->sum += tny_folder_get_all_count (folder);

			if (helper->task == TASK_GET_LOCAL_SIZE)
				helper->sum += tny_folder_get_local_size (folder);

			if (TNY_IS_FOLDER_STORE (folder))
				recurse_folders (TNY_FOLDER_STORE (folder), query, helper);

 			g_object_unref (folder);
		}

		tny_iterator_next (iter);
	}
	 g_object_unref (G_OBJECT (iter));
	 g_object_unref (G_OBJECT (folders));
}

gint 
modest_tny_folder_store_get_folder_count (TnyFolderStore *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (self), -1);

	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->task = TASK_GET_FOLDER_COUNT;
	helper->folders = 0;

	recurse_folders (self, NULL, helper);

	retval = helper->folders;

	g_free (helper);

	return retval;
}

gint
modest_tny_folder_store_get_message_count (TnyFolderStore *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (self), -1);
	
	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->task = TASK_GET_ALL_COUNT;
	if (TNY_IS_FOLDER (self))
		helper->sum = tny_folder_get_all_count (TNY_FOLDER (self));

	recurse_folders (self, NULL, helper);

	retval = helper->sum;

	g_free (helper);

	return retval;
}

gint 
modest_tny_folder_store_get_local_size (TnyFolderStore *self)
{
	RecurseFoldersHelper *helper;
	gint retval;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (self), -1);

	/* Create helper */
	helper = g_malloc0 (sizeof (RecurseFoldersHelper));
	helper->task = TASK_GET_LOCAL_SIZE;
	if (TNY_IS_FOLDER (self))
		helper->sum = tny_folder_get_local_size (TNY_FOLDER (self));

	recurse_folders (self, NULL, helper);

	retval = helper->sum;

	g_free (helper);

	return retval;
}

const gchar* 
modest_tny_account_get_parent_modest_account_name_for_server_account (TnyAccount *self)
{
	return (const gchar *)g_object_get_data (G_OBJECT (self), "modest_account");
}

void 
modest_tny_account_set_parent_modest_account_name_for_server_account (TnyAccount *self, 
								      const gchar* parent_modest_account_name)
{
	g_object_set_data_full (G_OBJECT(self), "modest_account",
				(gpointer) g_strdup (parent_modest_account_name), g_free);
}

gboolean
modest_tny_account_is_virtual_local_folders (TnyAccount *self)
{
	/* We should make this more sophisticated if we ever use ModestTnyLocalFoldersAccount 
	 * for anything else. */
	return MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (self);
}


gboolean
modest_tny_account_is_memory_card_account (TnyAccount *self)
{
	const gchar* account_id = NULL;

	g_return_val_if_fail (TNY_ACCOUNT (self), FALSE);

	if (!self)
		return FALSE;

	account_id = tny_account_get_id (self);

	if (!account_id)
		return FALSE;
	else	
		return (strcmp (account_id, MODEST_MMC_ACCOUNT_ID) == 0);
}

gboolean 
modest_tny_folder_store_is_remote (TnyFolderStore *folder_store)
{
        TnyAccount *account = NULL;
        gboolean result = TRUE;

        g_return_val_if_fail(TNY_IS_FOLDER_STORE(folder_store), FALSE);

        if (TNY_IS_FOLDER (folder_store)) {
                /* Get the folder's parent account: */
                account = tny_folder_get_account(TNY_FOLDER(folder_store));
        } else if (TNY_IS_ACCOUNT (folder_store)) {
                account = TNY_ACCOUNT(folder_store);
                g_object_ref(account);
        }

        if (account != NULL) {
                if (tny_account_get_account_type (account) == TNY_ACCOUNT_TYPE_STORE) {
                        if (!TNY_IS_CAMEL_POP_STORE_ACCOUNT (account) &&
                            !TNY_IS_CAMEL_IMAP_STORE_ACCOUNT (account)) {
                                /* This must be a maildir account, which does
                                 * not require a connection: */
                                result = FALSE;
                        }
                }
                g_object_unref (account);
        } else {
                result = FALSE;
        }

        return result;
}


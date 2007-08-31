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


#ifndef __MODEST_ACCOUNT_MGR_HELPERS_H__
#define __MODEST_ACCOUNT_MGR_HELPERS_H__

#include <modest-account-mgr.h>
#include <modest-tny-account-store.h>

#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>

G_BEGIN_DECLS

typedef struct {
	gchar            *account_name;
	gchar            *hostname;
	gchar            *username;
	gchar	         *uri; /*< Only for mbox and maildir accounts. */
	ModestTransportStoreProtocol    proto; /*< The store or transport. Not ORed. */
	gchar            *password;
	time_t		  last_updated;
	gint              port; /* Or 0, if the default should be used. */
	ModestAuthProtocol   secure_auth;
	ModestConnectionProtocol   security;
} ModestServerAccountData;

typedef struct {
	gchar            *account_name;
	gchar            *display_name;
	gchar            *fullname;
	gchar            *email;
	gboolean         is_enabled;
	gboolean         is_default;
	ModestServerAccountData *transport_account;
	ModestServerAccountData *store_account;
} ModestAccountData;



/**
 * modest_account_mgr_get_account_data:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * 
 * get information about an account
 *
 * Returns: a ModestAccountData structure with information about the account.
 * the data should not be changed, and be freed with modest_account_mgr_free_account_data
 * The function does a sanity check, an if it's not returning NULL,
 * it is a valid account
 */
ModestAccountData *modest_account_mgr_get_account_data     (ModestAccountMgr *self,
							    const gchar* name);

/**
 * modest_account_mgr_get_default_account:
 * @self: a ModestAccountMgr instance
 * 
 * get the default account name, or NULL if none is found
 *
 * Returns: the default account name (as newly allocated string, which
 * must be g_free'd), or NULL
 */
gchar* modest_account_mgr_get_default_account  (ModestAccountMgr *self);

/**
 * modest_account_mgr_set_default_account:
 * @self: a ModestAccountMgr instance
 * @account: the name of an existing account
 * 
 * set the default account name (which must be valid account)
 *
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean modest_account_mgr_set_default_account  (ModestAccountMgr *self,
						  const gchar* account);

/**
 * modest_account_mgr_unset_default_account:
 * @self: a ModestAccountMgr instance
 * @account: the name of an account
 * 
 * Unset the default account name, so that no account is the default.
 *
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean modest_account_mgr_unset_default_account  (ModestAccountMgr *self);

/**
 * modest_account_mgr_set_first_account_as_default:
 * @self: a ModestAccountMgr instance
 * 
 * Guarantees that at least one account, if there are any accounts, is the default,
 * so that modest_account_mgr_get_default_account() will return non-NULL if there 
 * are any accounts.
 *
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean
modest_account_mgr_set_first_account_as_default  (ModestAccountMgr *self);

/** Get the first one, alphabetically, by title. */
gchar* 
modest_account_mgr_get_first_account_name (ModestAccountMgr *self);

/**
 * modest_account_mgr_free_account_data:
 * @self: a ModestAccountMgr instance
 * @data: a ModestAccountData instance
 * 
 * free the account data structure
 */
void       modest_account_mgr_free_account_data     (ModestAccountMgr *self,
						     ModestAccountData *data);

/**
 * modest_account_mgr_set_enabled
 * @self: a ModestAccountMgr instance
 * @name: the account name 
 * @enabled: if TRUE, the account will be enabled, if FALSE, it will be disabled
 * 
 * enable/disabled an account
 *
 * Returns: TRUE if it worked, FALSE otherwise
 */
gboolean modest_account_mgr_set_enabled (ModestAccountMgr *self, const gchar* name,
					 gboolean enabled);

/**
 * modest_account_mgr_get_enabled:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 *
 * check whether a certain account is enabled
 *
 * Returns: TRUE if it is enabled, FALSE otherwise
 */
gboolean modest_account_mgr_get_enabled (ModestAccountMgr *self, const gchar* name);

/**
 * modest_account_mgr_get_display_name:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 *
 * Return the human-readable account title for this account, or NULL.
 */
gchar* modest_account_mgr_get_display_name (ModestAccountMgr *self, 
	const gchar* name);


/**
 * modest_account_mgr_set_signature
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 * @signature: the signature text 
 * @use_signature: Whether the signature should be used.
 * 
 * Sets the signature text for the account.
 *
 * Returns: TRUE if it worked, FALSE otherwise
 */
gboolean modest_account_mgr_set_signature (ModestAccountMgr *self, const gchar* name, 
	const gchar* signature, gboolean use_signature);

/**
 * modest_account_mgr_get_signature:
 * @self: a ModestAccountMgr instance
 * @name: the account name
 * @use_signature: Pointer to a gboolean taht will be set to TRUE if the signature should be used.
 *
 * Gets the signature text for this account.
 *
 * Returns: The signature text, which should be freed with g_free().
 */
gchar* modest_account_mgr_get_signature (ModestAccountMgr *self, const gchar* name, 
	gboolean* use_signature);
	
/**
 * modest_account_mgr_get_store_protocol:
 * @self: a ModestAccountMgr instance
 * @name: the account name
 *
 * Gets the protocol type (For instance, POP or IMAP) used for the store server account.
 *
 * Returns: The protocol type.
 */
ModestTransportStoreProtocol modest_account_mgr_get_store_protocol (ModestAccountMgr *self, const gchar* name);

/**
 * modest_account_mgr_set_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @connection_name: A libconic IAP connection name
 * @server_account_name: a server account name to use for this connection.
 * 
 * Specify a server account to use with the specific connection for this account.
 *
 * Returns: TRUE if it worked, FALSE otherwise
 */
gboolean modest_account_mgr_set_connection_specific_smtp (ModestAccountMgr *self, 
					 const gchar* connection_name, const gchar* server_account_name);

/**
 * modest_account_mgr_remove_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @connection_name: A libconic IAP connection name
 * 
 * Disassociate a server account to use with the specific connection for this account.
 *
 * Returns: TRUE if it worked, FALSE otherwise
 */				 
gboolean modest_account_mgr_remove_connection_specific_smtp (ModestAccountMgr *self, 
	const gchar* connection_name);

/**
 * modest_account_mgr_get_use_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @account_name: the account name
 * @result: Whether this account should use connection-specific smtp server accounts.
 */
gboolean modest_account_mgr_get_use_connection_specific_smtp (ModestAccountMgr *self, const gchar* account_name);

/**
 * modest_account_mgr_set_use_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @account_name: the account name
 * @new_value: New value that indicates if if this account should use connection-specific smtp server accounts.
 * @result: TRUE if it succeeded, FALSE otherwise
 */
gboolean modest_account_mgr_set_use_connection_specific_smtp (ModestAccountMgr *self, const gchar* account_name,
	gboolean new_value);

/**
 * modest_account_mgr_get_connection_specific_smtp
 * @self: a ModestAccountMgr instance
 * @connection_name: A libconic IAP connection name
 * 
 * Retrieve a server account to use with this specific connection for this account.
 *
 * Returns: a server account name to use for this connection, or NULL if none is specified.
 */			 
gchar* modest_account_mgr_get_connection_specific_smtp (ModestAccountMgr *self, 
					 const gchar* connection_name);


/**
 * modest_server_account_get_username:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 *
 * Gets the username this server account.
 *
 * Returns: The username.
 */
gchar*
modest_server_account_get_username (ModestAccountMgr *self, const gchar* account_name);

/**
 * modest_server_account_set_username:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 * @username: The new username.
 *
 * Sets the username this server account.
 */
void
modest_server_account_set_username (ModestAccountMgr *self, const gchar* account_name, 
	const gchar* username);

/**
 * modest_server_account_get_username_has_succeeded:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 *
 * Whether a connection has ever been successfully made to this account with 
 * the current username. This can be used to avoid asking again for the username 
 * when asking a second time for a non-stored password.
 *
 * Returns: TRUE if the username is known to be correct.
 */
gboolean
modest_server_account_get_username_has_succeeded (ModestAccountMgr *self, const gchar* account_name);

/**
 * modest_server_account_set_username_has_succeeded:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 * @succeeded: Whether the username has succeeded
 *
 * Sets whether the username is known to be correct.
 */
void
modest_server_account_set_username_has_succeeded (ModestAccountMgr *self, const gchar* account_name, 
	gboolean succeeded);
	
/**
 * modest_server_account_set_password:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 * @password: The new password.
 *
 * Sets the password for this server account.
 */
void
modest_server_account_set_password (ModestAccountMgr *self, const gchar* account_name, 
	const gchar* password);
	
/**
 * modest_server_account_get_password:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 *
 * Gets the password for this server account from the account settings.
 */
gchar*
modest_server_account_get_password (ModestAccountMgr *self, const gchar* account_name);

/**
 * modest_server_account_get_has_password:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 *
 * Gets whether a password has been set for this server account in the account settings.
 */
gboolean
modest_server_account_get_has_password (ModestAccountMgr *self, const gchar* account_name);	 

/**
 * modest_server_account_modest_server_account_get_hostnameget_username:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 *
 * Gets the hostname this server account.
 *
 * Returns: The hostname.
 */
gchar*
modest_server_account_get_hostname (ModestAccountMgr *self, const gchar* account_name);


/**
 * modest_server_account_get_secure_auth:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 *
 * Gets the secure authentication method for this server account.
 *
 * Returns: The secure authentication enum value.
 */
ModestAuthProtocol
modest_server_account_get_secure_auth (ModestAccountMgr *self, const gchar* account_name);

/**
 * modest_server_account_data_get_secure_auth:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 * @secure_auth: The secure authentication enum value.
 *
 * Gets the secure authentication method for this server account.
 */
void
modest_server_account_set_secure_auth (ModestAccountMgr *self, const gchar* account_name, 
				       ModestAuthProtocol secure_auth);
	
/**
 * modest_server_account_data_get_security:
 * @self: a ModestAccountMgr instance
 * @account_name: The name of a server account.
 *
 * Gets the security method for this server account.
 *
 * Returns: The security enum value.
 */
ModestConnectionProtocol
modest_server_account_get_security (ModestAccountMgr *self, const gchar* account_name);

/**
 * modest_server_account_set_security:
 * @self: a ModestAccountMgr instance
 * @secure_auth: The security enum value.
 *
 * Gets the security method for this server account.
 */
void
modest_server_account_set_security (ModestAccountMgr *self, const gchar* account_name, 
				    ModestConnectionProtocol security);

ModestServerAccountData*
modest_account_mgr_get_server_account_data (ModestAccountMgr *self, const gchar* name);

void
modest_account_mgr_free_server_account_data (ModestAccountMgr *self,
					     ModestServerAccountData* data);

/**
 * modest_account_mgr_get_from_string
 * @self: a #ModestAccountMgr instance
 * @name: the account name
 *
 * get the From: string for some account; ie. "Foo Bar" <foo.bar@cuux.yy>"
 *
 * Returns: the newly allocated from-string, or NULL in case of error
 */
gchar * modest_account_mgr_get_from_string (ModestAccountMgr *self, const gchar* name);


/**
 * modest_account_mgr_get_unused_account_name
 * @self: a #ModestAccountMgr instance
 * @name: The initial account name
 *
 * get an unused account name, based on a starting string.
 *
 * Returns: the newly allocated name.
 */
gchar*
modest_account_mgr_get_unused_account_name (ModestAccountMgr *self, const gchar* starting_name,
	gboolean server_account);

/**
 * modest_account_mgr_get_unused_account_display name
 * @self: a #ModestAccountMgr instance
 * @name: The initial account display name
 *
 * get an unused account display name, based on a starting string.
 *
 * Returns: the newly allocated name.
 */
gchar*
modest_account_mgr_get_unused_account_display_name (ModestAccountMgr *self, const gchar* starting_name);

G_END_DECLS

#endif /* __MODEST_ACCOUNT_MGR_H__ */

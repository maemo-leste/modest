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


#ifndef __MODEST_ACCOUNT_MGR_H__
#define __MODEST_ACCOUNT_MGR_H__

#include <glib-object.h>
#include <modest-conf.h>
#include <modest-protocol-registry.h>
#include <modest-account-settings.h>
#include <tny-account.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_MGR             (modest_account_mgr_get_type())
#define MODEST_ACCOUNT_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_MGR,ModestAccountMgr))
#define MODEST_ACCOUNT_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_MGR,ModestAccountMgrClass))
#define MODEST_IS_ACCOUNT_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_MGR))
#define MODEST_IS_ACCOUNT_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_MGR))
#define MODEST_ACCOUNT_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_MGR,ModestAccountMgrClass))

typedef struct _ModestAccountMgr      ModestAccountMgr;
typedef struct _ModestAccountMgrClass ModestAccountMgrClass;

struct _ModestAccountMgr {
	 GObject parent;
};

struct _ModestAccountMgrClass {
	GObjectClass parent_class;

	void (* account_inserted)  (ModestAccountMgr *obj, 
				    const gchar* account,
				    gpointer user_data);

	void (* account_removed)   (ModestAccountMgr *obj, 
				    const gchar* account,
				    gpointer user_data);
	
	void (* account_changed)   (ModestAccountMgr *obj, 
				    const gchar* account,
				    gpointer user_data);

	void (* account_busy_changed)   (ModestAccountMgr *obj, 
					 const gchar* account,
					 gboolean busy,
					 gpointer user_data);	

	void (* default_account_changed)(ModestAccountMgr *obj, 
					 gpointer user_data);

	void (* display_name_changed)   (ModestAccountMgr *obj, 
					 const gchar *account,
					 gpointer user_data);
	
	void (* account_updated)   (ModestAccountMgr *obj, 
			 const gchar *account,
			 gpointer user_data);
};

/**
 * modest_account_mgr_get_type:
 * 
 * get the GType for ModestAccountMgr
 *  
 * Returns: the GType
 */
GType           modest_account_mgr_get_type       (void) G_GNUC_CONST;


/**
 * modest_account_mgr_new:
 * @modest_conf: a ModestConf instance 
 *  
 * Returns: a new ModestAccountMgr, or NULL in case of error
 */
ModestAccountMgr*        modest_account_mgr_new            (ModestConf *modest_conf);



/**
 * modest_account_mgr_add_account_from_settings:
 * @self: a #ModestAccountMgr instance
 * @self: a #ModestSettings
 * 
 * Create a new account from a @settings instance.
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 */
gboolean        modest_account_mgr_add_account_from_settings    (ModestAccountMgr *self,
								 ModestAccountSettings *settings);

/**
 * modest_account_mgr_add_account:
 * @self: a ModestAccountMgr instance
 * @name: name (id) of the account, which is a valid UTF8 string that does not contain '/'
 * @store_name: the store account (ie. POP/IMAP)
 * @transport_name: the transport account (ie. sendmail/SMTP)
 * @enabled: Whether the account should be enabled initially.
 * 
 * Create a new account. The account with @name should not already exist. The @name will 
 * be used as the initial display name of the new account.
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 */
gboolean        modest_account_mgr_add_account    (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *display_name,
						   const gchar *user_fullname,
						   const gchar *user_email,
						   ModestAccountRetrieveType retrieve_type,
						   const gchar* store_name,
						   const gchar* transport_name,
						   gboolean enabled);

/**
 * modest_account_mgr_add_server_account:
 * @self: a ModestAccountMgr instance
 * @name: name (id) of the account, which is a valid UTF8 string that does not contain '/'
 * @hostname: the hostname
 * @portnumber: the portnumber, or 0 for default
 * @username: the username
 * @password: the password
 * @proto:    the protocol (imap, smtp, ...) used for this account
 * @security: the security options, (SSL, TLS ...) used to access the server
 * @auth: the authentication method (password, none ...) used to access the server
 * 
 * add a server account to the configuration.
 * the server account with @name should not already exist
 * 
 * Returns: TRUE if succeeded, FALSE otherwise,
 */
gboolean modest_account_mgr_add_server_account    (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *hostname,
						   const guint portnumber,
						   const gchar *username,
						   const gchar *password,
						   ModestProtocolType proto,
						   ModestProtocolType security,
						   ModestProtocolType auth);


/**
 * modest_account_mgr_add_server_account_uri:
 * @self: a ModestAccountMgr instance
 * @name: name (id) of the account, which is a valid UTF8 string that does not contain '/'
 * @proto:    the protocol (imap, smtp, ...) used for this account
 * @uri: the URI
 * 
 * add a server account to the configuration, based on the account-URI
 * 
 * Returns: TRUE if succeeded, FALSE otherwise,
 */
gboolean modest_account_mgr_add_server_account_uri    (ModestAccountMgr *self,
						       const gchar *name,
						       ModestProtocolType proto,
						       const gchar* uri);

/**
 * modest_account_mgr_remove_account:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account to remove
 * @err: a #GError ptr, or NULL to ignore.
 * 
 * remove an existing account. the account with @name should already exist; note
 * that when deleting an account, also the corresponding server accounts will
 * be deleted
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 * @err gives details in case of error
 */
gboolean        modest_account_mgr_remove_account         (ModestAccountMgr *self,
							   const gchar* name);

/**
 * modest_account_mgr_remove_account:
 * @self: a ModestAccountMgr instance
 * @name: the name of the server account to remove
 * 
 * remove an existing server account. This is only for internal use.
 *
 * Returns: TRUE if the operation succeeded, FALSE otherwise,
 */
gboolean        modest_account_mgr_remove_server_account         (ModestAccountMgr *self,
								  const gchar* name);

/**
 * modest_account_mgr_account_names:
 * @self: a ModestAccountMgr instance
 * @only_enabled: Whether only enabled accounts should be returned.
 * 
 * list all account names
 *
 * Returns: a newly allocated list of account names, or NULL in case of error or
 * if there are no accounts. The caller must free the returned GSList.
 *
 */
GSList*	        modest_account_mgr_account_names    (ModestAccountMgr *self,
						     gboolean only_enabled);

/**
 * modest_account_mgr_free_account_names:
 * @account_name: a gslist of account names
 * 
 * list all account names
 *
 * free the list of account names
 */
void	        modest_account_mgr_free_account_names    (GSList *account_names);
							  

/**
 * modest_account_mgr_account_exists:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 * @server_account: if TRUE, this is a server account
 * 
 * check whether account @name exists. Note that this does not check the display name.
 *
 * Returns: TRUE if the account with name @name exists, FALSE otherwise (or in case of error)
 */
gboolean	modest_account_mgr_account_exists	  (ModestAccountMgr *self,
							   const gchar *name,
							   gboolean server_account);

/**
 * modest_account_mgr_account_exists:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 * 
 * check whether a non-server account with the @display_name exists.
 *
 * Returns: TRUE if the account with name @name exists, FALSE otherwise (or in case of error)
 */
gboolean	modest_account_mgr_account_with_display_name_exists (ModestAccountMgr *self,
								     const gchar *display_name);

/**
 * modest_account_mgr_check_already_configured_account:
 * @self: a #ModestAccountMgr
 * @settings: a #ModestAccountSettings *settings
 *
 * Checks if there's already an active store account with the same settings
 *
 * Returns: %TRUE if account setup exists
 */
gboolean        modest_account_mgr_check_already_configured_account (ModestAccountMgr * self,
								     ModestAccountSettings *settings);

/**
 * modest_account_mgr_unset:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to unset
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * unsets the config value of an account and all their children keys
 *
 * Returns: TRUE if unsetting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean        modest_account_mgr_unset           (ModestAccountMgr *self,
						    const gchar *name,
						    const gchar *key,
						    gboolean server_account);

/**
 * modest_account_mgr_has_accounts:
 * @self: a ModestAccountMgr instance
 * @enabled: TRUE to search for enabled accounts only
 * 
 * Checks if any accounts exist
 *
 * Returns: TRUE if accounts exist, FALSE otherwise
 */

gboolean modest_account_mgr_has_accounts (ModestAccountMgr* self, gboolean enabled);

/**
 * modest_account_mgr_set_account_busy
 * @self: a ModestAccountMgr instance
 * @account_name: name of the account
 * @busy: whether to set busy or not busy
 * 
 * Changes the busy flag of an account
 *
 */

void modest_account_mgr_set_account_busy(ModestAccountMgr* self, const gchar* account_name, 
																		gboolean busy);

/**
 * modest_account_mgr_account_is_busy
 * @self: a ModestAccountMgr instance
 * @account_name: name of the account
 * 
 * Returns: If the account is currently busy or not
 *
 */
gboolean modest_account_mgr_account_is_busy (ModestAccountMgr* self, 
					     const gchar* account_name);


void modest_account_mgr_notify_account_update (ModestAccountMgr* self, 
					       const gchar *server_account_name);

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
 * modest_account_mgr_get_display_name:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 *
 * Return the human-readable account title for this account, or NULL.
 */
gchar* modest_account_mgr_get_display_name (ModestAccountMgr *self, 
					    const gchar* name);

void  modest_account_mgr_set_display_name (ModestAccountMgr *self, 
					   const gchar *account_name,
					   const gchar *display_name);

gboolean modest_account_mgr_singleton_protocol_exists (ModestAccountMgr *mgr,
						       ModestProtocolType protocol_type);

gchar * modest_account_mgr_get_string (ModestAccountMgr *self, const gchar *name,
				       const gchar *key, gboolean server_account);
GSList * modest_account_mgr_get_list (ModestAccountMgr *self, const gchar *name,
				      const gchar *key, ModestConfValueType list_type,
				      gboolean server_account);
gboolean modest_account_mgr_set_list (ModestAccountMgr *self,
				      const gchar *name,
				      const gchar *key,
				      GSList *val,
				      ModestConfValueType list_type,
				      gboolean server_account);

gchar* modest_account_mgr_get_account_from_tny_account (ModestAccountMgr *self,
						  	TnyAccount *account);
G_END_DECLS

#endif /* __MODEST_ACCOUNT_MGR_H__ */

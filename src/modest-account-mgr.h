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
#include <modest-defs.h>
#include <modest-protocol-info.h>

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
						   const gchar* name,
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
						   ModestTransportStoreProtocol proto,
						   ModestConnectionProtocol security,
						   ModestAuthProtocol auth);


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
						       ModestTransportStoreProtocol proto,
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
gboolean	modest_account_mgr_account_with_display_name_exists	  (ModestAccountMgr *self,
							   const gchar *display_name);


/**
 * modest_account_mgr_get_string:
 * @self: self a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * 
 * get a config string from an account
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error. 
 */
gchar*	        modest_account_mgr_get_string     (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account);


/**
 * modest_account_mgr_get_password:
 * @self: self a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * 
 * get a password from an account
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error.
 */
gchar*	        modest_account_mgr_get_password     (ModestAccountMgr *self,
						     const gchar *name,
						     const gchar *key,
						     gboolean server_account);

/**
 * modest_account_mgr_get_int:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * 
 * get a config int from an account
 *
 * Returns: an integer with the value for the key, or -1 in case of
 * error (but of course -1 does not necessarily imply an error)
 */
gint	        modest_account_mgr_get_int        (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account);

/**
 * modest_account_mgr_get_bool:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * 
 * get a config boolean from an account
 *
 * Returns: an boolean with the value for the key, or FALSE in case of
 * error (but of course FALSE does not necessarily imply an error)
 */
gboolean	modest_account_mgr_get_bool       (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account);

/**
 * modest_account_mgr_get_list:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to get
 * @list_type: the type of the members of the list
 * @server_account: if TRUE, this is a server account
 * 
 * get a config list of values of type @list_type of an account
 *
 * Returns: a newly allocated list of elements
 */
GSList*	        modest_account_mgr_get_list       (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   ModestConfValueType list_type,
						   gboolean server_account);

/**
 * modest_account_mgr_set_string:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * 
 * set a config string for an account.
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 */
gboolean	modest_account_mgr_set_string     (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key, const gchar* val,
						   gboolean server_account);


/**
 * modest_account_mgr_set_password:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * 
 * set a password for an account.
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
* @err gives details in case of error
 */
gboolean	modest_account_mgr_set_password     (ModestAccountMgr *self,
						     const gchar *name,
						     const gchar *key, const gchar* val,
						     gboolean server_account);

/**
 * modest_account_mgr_set_int:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * 
 * set a config int for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 */
gboolean	modest_account_mgr_set_int        (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key, gint val,
						   gboolean server_account);

/**
 * modest_account_mgr_set_bool:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * 
 * set a config bool for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 */
gboolean	modest_account_mgr_set_bool       (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key, gboolean val,
						   gboolean server_account);

/**
 * modest_account_mgr_set_list:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the list with the values to set
 * @list_type: the type of the members of the list
 * @server_account: if TRUE, this is a server account
 *
 * * set a config list of values of type @list_type of an account
 * 
 * returns TRUE if this succeeded, FALSE otherwise 
 */
gboolean	        modest_account_mgr_set_list       (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key,
							   GSList *val,
							   ModestConfValueType list_type,
							   gboolean server_account);

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
gboolean
modest_account_mgr_account_is_busy(ModestAccountMgr* self, const gchar* account_name);


G_END_DECLS

#endif /* __MODEST_ACCOUNT_MGR_H__ */

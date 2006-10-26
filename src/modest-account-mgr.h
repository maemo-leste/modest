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
#include <modest-account-keys.h>
#include <modest-protocol-mgr.h>

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

	void (* account_removed)   (ModestAccountMgr *obj, const gchar* account,
				    gboolean server_account, gpointer user_data);
	void (* account_changed)   (ModestAccountMgr *obj, const gchar* account,
				    const gchar* key, gboolean server_account,
				    gpointer user_data);
};

/*
 * some convenience structs to get bulk data about an account 
 */
struct _ModestServerAccountData {
	gchar *account_name;
	gchar *hostname;
	gchar *username;
	gchar *proto;
	gchar *password;
};
typedef struct _ModestServerAccountData ModestServerAccountData;

struct _ModestAccountData {
	gchar *account_name;
	gchar *full_name;
	gchar *email;
	gboolean enabled;
	
	ModestServerAccountData *transport_account;
	ModestServerAccountData *store_account;
};
typedef struct _ModestAccountData ModestAccountData;



/**
 * modest_ui_get_type:
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
 * @name: the name of the account to create
 * @store_name: the store account (ie. POP/IMAP)
 * @transport_name: the transport account (ie. sendmail/SMTP)
 * @err: a GError ptr, or NULL to ignore.
 * 
 * create a new account. the account with @name should not already exist
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 * @err gives details in case of error
 */
gboolean        modest_account_mgr_add_account    (ModestAccountMgr *self,
						   const gchar* name,
						   const gchar* store_name,
						   const gchar* transport_name,
						   GError **err);


/**
 * modest_account_mgr_add_server_account:
 * @self: a ModestAccountMgr instance
 * @name: name (id) of the account
 * @hostname: the hostname
 * @username: the username
 * @password: the password
 * @proto:    the protocol (imap, smtp, ...) used for this account
 * 
 * add a server account to the configuration.
 * the server account with @name should not already exist
 * 
 * Returns: TRUE if succeeded, FALSE otherwise,
 */
gboolean modest_account_mgr_add_server_account    (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *hostname,
						   const gchar *username,
						   const gchar *password,
						   const gchar *proto);  

/**
 * modest_account_mgr_remove_account:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account to remove
 * @err: a GError ptr, or NULL to ignore.
 * 
 * remove an existing account. the account with @name should already exist
 *
 * Returns: TRUE if the creation succeeded, FALSE otherwise,
 * @err gives details in case of error
 */
gboolean        modest_account_mgr_remove_account         (ModestAccountMgr *self,
							   const gchar* name,
							   gboolean server_account,
							   GError **err);


/**
 * modest_account_mgr_account_names:
 * @self: a ModestAccountMgr instance
 * @err: a GError ptr, or NULL to ignore.
 * 
 * list all account names
 *
 * Returns: a newly allocated list of account names, or NULL in case of error or
 * if there are no accounts. The caller must free the returned GSList
 * @err gives details in case of error
 */
GSList*	        modest_account_mgr_account_names    (ModestAccountMgr *self, GError **err);


/**
 * modest_account_mgr_server_account_names:
 * @self: a ModestAccountMgr instance
 * @account_name: get only server accounts for @account_name, or NULL for any
 * @type: get only server accounts from protocol type @type, or MODEST_PROTO_TYPE_ANY
 * @proto: get only server account with protocol @proto, or NULL for any
 * @only_enabled: get only enabled server accounts if TRUE
 * 
 * list all the server account names
 *
 * Returns: a newly allocated list of server account names, or NULL in case of
 * error or if there are no server accounts. The caller must free the returned GSList
 */
GSList*  modest_account_mgr_search_server_accounts  (ModestAccountMgr *self,
						     const gchar*       account_name,
						     ModestProtocolType type,
						     const gchar*       proto);

/**
 * modest_account_mgr_account_exists:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * check whether account @name exists
 *
 * Returns: TRUE if the account with name @name exists, FALSE otherwise (or in case of error)
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_account_exists	  (ModestAccountMgr *self,
							   const gchar *name,
							   gboolean server_account,
							   GError **err);

/**
 * modest_account_mgr_get_account_data:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * 
 * get information about an account
 *
 * Returns: a ModestAccountData structure with information about the account.
 * the data should not be changed, and be freed with modest_account_mgr_free_account_data
 */
ModestAccountData *modest_account_mgr_get_account_data     (ModestAccountMgr *self,
							    const gchar* name);


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
 * modest_account_mgr_account_set_enabled
 * @self: a ModestAccountMgr instance
 * @name: the account name 
 * @enabled: if TRUE, the account will be enabled, if FALSE, it will be disabled
 * 
 * enable/disabled an account
 *
 * Returns: TRUE if it worked, FALSE otherwise
 */
gboolean modest_account_mgr_account_set_enabled (ModestAccountMgr *self, const gchar* name,
						 gboolean enabled);


/**
 * modest_account_mgr_account_get_enabled:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 *
 * check whether a certain account is enabled
 *
 * Returns: TRUE if it is enabled, FALSE otherwise
 */
gboolean modest_account_mgr_account_get_enabled (ModestAccountMgr *self, const gchar* name);


/**
 * modest_account_mgr_get_account_string:
 * @self: self a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a config string from an account
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error. @err gives details in case of error
 */
gchar*	        modest_account_mgr_get_string     (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account,
						   GError **err);


/**
 * modest_account_mgr_get_account_int:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a config int from an account
 *
 * Returns: an integer with the value for the key, or -1 in case of
 * error (but of course -1 does not necessarily imply an error)
 * @err gives details in case of error
 */
gint	        modest_account_mgr_get_int        (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account,
						   GError **err);

/**
 * modest_account_mgr_get_account_bool:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a config boolean from an account
 *
 * Returns: an boolean with the value for the key, or FALSE in case of
 * error (but of course FALSE does not necessarily imply an error)
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_get_bool       (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key,
						   gboolean server_account,
						   GError **err);

/**
 * modest_account_mgr_set_account_string:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a config string for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_set_string     (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key, const gchar* val,
						   gboolean server_account,
						   GError **err);

/**
 * modest_account_mgr_set_account_int:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a config int for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_set_int        (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key, gint val,
						   gboolean server_account,
						   GError **err);


/**
 * modest_account_mgr_set_account_bool:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @server_account: if TRUE, this is a server account
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a config bool for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_set_bool       (ModestAccountMgr *self,
						   const gchar *name,
						   const gchar *key, gboolean val,
						   gboolean server_account,
						   GError **err);



G_END_DECLS

#endif /* __MODEST_ACCOUNT_MGR_H__ */

/* modest-account-mgr.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_ACCOUNT_MGR_H__
#define __MODEST_ACCOUNT_MGR_H__

#include <glib-object.h>
#include "modest-conf.h"
#include "modest-account-keys.h"
#include "modest-proto.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_MGR             (modest_account_mgr_get_type())
#define MODEST_ACCOUNT_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_MGR,ModestAccountMgr))
#define MODEST_ACCOUNT_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_MGR,GObject))
#define MODEST_IS_ACCOUNT_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_MGR))
#define MODEST_IS_ACCOUNT_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_MGR))
#define MODEST_ACCOUNT_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_MGR,ModestAccountMgrClass))

typedef struct _ModestAccountMgr      ModestAccountMgr;
typedef struct _ModestAccountMgrClass ModestAccountMgrClass;



struct _ModestAccountMgr {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestAccountMgrClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestAccountMgr* obj); */
};


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
GObject*        modest_account_mgr_new            (ModestConf *modest_conf);



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
 * modest_account_mgr_remove_server_account:
 * @self: a ModestAccountMgr instance
 * @name: the name for the server account to remove
 * @err: a GError ptr, or NULL to ignore.
 * 
 * remove a server account from the configuration
 * the server account with @name should exist
 *
 * Returns: TRUE if the removal succeeded, FALSE otherwise,
 * @err gives details in case of error
 */
gboolean        modest_account_mgr_remove_server_account    (ModestAccountMgr *self,
							     const gchar *name,
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
GSList*  modest_account_mgr_server_account_names   (ModestAccountMgr *self,
						    const gchar*    account_name,
						    ModestProtoType type,
						    const gchar*    proto,
						    gboolean only_enabled);

/**
 * modest_account_mgr_account_exists:
 * @self: a ModestAccountMgr instance
 * @name: the account name to check
 * @err: a GError ptr, or NULL to ignore.
 * 
 * check whether account @name exists
 *
 * Returns: TRUE if the account with name @name exists, FALSE otherwise (or in case of error)
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_account_exists	  (ModestAccountMgr *self,
							   const gchar *name,
							   GError **err);
							   
gboolean	modest_account_mgr_server_account_exists	  (ModestAccountMgr *self,
								   const gchar *name,
								   GError **err);



/* account specific functions */

/**
 * modest_account_mgr_get_account_string:
 * @self: self a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a config string from an account
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error. @err gives details in case of error
 */
gchar*	        modest_account_mgr_get_account_string     (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, GError **err);

gchar*	        modest_account_mgr_get_server_account_string     (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, GError **err);

/**
 * modest_account_mgr_get_account_int:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a config int from an account
 *
 * Returns: an integer with the value for the key, or -1 in case of
 * error (but of course -1 does not necessarily imply an error)
 * @err gives details in case of error
 */
gint	        modest_account_mgr_get_account_int        (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, GError **err);

gint	        modest_account_mgr_get_server_account_int        (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, GError **err);


/**
 * modest_account_mgr_get_account_bool:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a config boolean from an account
 *
 * Returns: an boolean with the value for the key, or FALSE in case of
 * error (but of course FALSE does not necessarily imply an error)
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_get_account_bool       (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, GError **err);

gboolean	modest_account_mgr_get_server_account_bool       (ModestAccountMgr *self,
								  const gchar *name,
								  const gchar *key, GError **err);

/**
 * modest_account_mgr_set_account_string:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a config string for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_set_account_string     (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, const gchar* val,
							   GError **err);

gboolean	modest_account_mgr_set_server_account_string     (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, const gchar* val,
							   GError **err);


/**
 * modest_account_mgr_set_account_int:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a config int for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_set_account_int        (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, gint val,
							   GError **err);

gboolean	modest_account_mgr_set_server_account_int        (ModestAccountMgr *self,
								  const gchar *name,
								  const gchar *key, gint val,
								  GError **err);


/**
 * modest_account_mgr_set_account_bool:
 * @self: a ModestAccountMgr instance
 * @name: the name of the account
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a config bool for an account
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean	modest_account_mgr_set_account_bool       (ModestAccountMgr *self,
							   const gchar *name,
							   const gchar *key, gboolean val,
							   GError **err);

gboolean	modest_account_mgr_set_server_account_bool       (ModestAccountMgr *self,
								  const gchar *name,
								  const gchar *key, gboolean val,
								  GError **err);


G_END_DECLS

#endif /* __MODEST_ACCOUNT_MGR_H__ */

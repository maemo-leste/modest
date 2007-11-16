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

#ifndef __MODEST_ACCOUNT_MGR_PRIV_H__
#define __MODEST_ACCOUNT_MGR_PRIV_H__

#include <glib.h>
#include <modest-conf.h>

/*
 * private functions, only for use in modest-account-mgr and
 * modest-account-mgr-helpers
 */

G_BEGIN_DECLS

gchar* _modest_account_mgr_account_from_key (const gchar *key, gboolean *is_account_key,
					     gboolean *is_server_account);
gchar * _modest_account_mgr_get_account_keyname (const gchar *account_name, const gchar * name,
						 gboolean server_account);

/* below is especially very _private_ stuff */
typedef struct _ModestAccountMgrPrivate ModestAccountMgrPrivate;
struct _ModestAccountMgrPrivate {
	ModestConf        *modest_conf;
	
	/* We store these as they change, and send notifications every X seconds: */
	gulong key_changed_handler_uid;
	GSList* busy_accounts;

	GSList* change_queue; /* list with all accounts that are changed */
	guint timeout;
	
	GHashTable *notification_id_accounts;
	GHashTable *server_account_key_hash;
	GHashTable *account_key_hash;
	
	/* cache whether we have accounts; if this is TRUE, we have accounts, if
	 * it's FALSE we _don't know_ if we have account and need to check
	 */
	gboolean has_accounts;
	gboolean has_enabled_accounts;
};
#define MODEST_ACCOUNT_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
	         				    MODEST_TYPE_ACCOUNT_MGR, \
                                                ModestAccountMgrPrivate))
	
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

G_END_DECLS
#endif /* __MODEST_ACCOUNT_MGR_PRIV_H__ */

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

G_BEGIN_DECLS

typedef struct {
	gchar *account_name;
	gchar *hostname;
	gchar *username;
	ModestProtocol proto;
	gchar *password;
} ModestServerAccountData;

typedef struct {
	gchar *account_name;
	gchar *display_name;
	gchar *fullname;
	gchar *email;
	gboolean enabled;
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
 * modest_account_mgr_free_account_data:
 * @self: a ModestAccountMgr instance
 * @data: a ModestAccountData instance
 * 
 * free the account data structure
 */
void       modest_account_mgr_free_account_data     (ModestAccountMgr *self,
						     ModestAccountData *data);


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
						     ModestProtocol     proto);

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


G_END_DECLS

#endif /* __MODEST_ACCOUNT_MGR_H__ */

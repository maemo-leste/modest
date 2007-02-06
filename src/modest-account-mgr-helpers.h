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
	gchar	         *uri;
	ModestProtocol    proto;
	gchar            *password;
	time_t		  last_updated;
	GSList           *options;
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
 * modest_account_mgr_get_default_account:
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
 * modest_account_mgr_get_from_string
 * @self: a #ModestAccountMgr instance
 * @name: the account name
 *
 * get the From: string for some account; ie. "Foo Bar" <foo.bar@cuux.yy>"
 *
 * Returns: the newly allocated from-string, or NULL in case of error
 */
gchar * modest_account_mgr_get_from_string (ModestAccountMgr *self, const gchar* name);


G_END_DECLS

#endif /* __MODEST_ACCOUNT_MGR_H__ */

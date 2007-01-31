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


/* modest-tny-account-store.h */

#ifndef __MODEST_TNY_ACCOUNT_STORE_H__
#define __MODEST_TNY_ACCOUNT_STORE_H__

#include <glib-object.h>
#include <modest-defs.h>
#include <tny-account-store.h>
#include <tny-session-camel.h>
#include <tny-shared.h>
#include <tny-folder.h>
#include <modest-account-mgr.h>

/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_ACCOUNT_STORE             (modest_tny_account_store_get_type())
#define MODEST_TNY_ACCOUNT_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_ACCOUNT_STORE,ModestTnyAccountStore))
#define MODEST_TNY_ACCOUNT_STORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_ACCOUNT_STORE,ModestTnyAccountStoreClass))
#define MODEST_IS_TNY_ACCOUNT_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_ACCOUNT_STORE))
#define MODEST_IS_TNY_ACCOUNT_STORE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_ACCOUNT_STORE))
#define MODEST_TNY_ACCOUNT_STORE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_ACCOUNT_STORE,ModestTnyAccountStoreClass))

typedef struct _ModestTnyAccountStore      ModestTnyAccountStore;
typedef struct _ModestTnyAccountStoreClass ModestTnyAccountStoreClass;
typedef TnyGetPassFunc ModestTnyGetPassFunc;

struct _ModestTnyAccountStore {
	GObject parent;
};

struct _ModestTnyAccountStoreClass {
	GObjectClass parent_class;

	void (*account_update)    (ModestTnyAccountStore *self,
				   const gchar *account_name,
				   gpointer user_data);
};

/* member functions */

/**
 * modest_tny_account_store_get_type:
 *
 * Returns: GType of the account store
 */
GType  modest_tny_account_store_get_type   (void) G_GNUC_CONST;

/**
 * modest_tny_account_store_new:
 * @account_mgr: account manager to use for new account store
 *
 * creates new (tinymail) account store for account manager modest_acc_mgr
 *
 * Returns: newly created account store or NULL in case of error
 */
ModestTnyAccountStore*    modest_tny_account_store_new (ModestAccountMgr *account_mgr);

/**
 * tny_account_store_get_session
 * @self: a TnyAccountStore instance
 * 
 * get the tny-camel-session for this account store. Note that this function
 * does NOT have the modest_ prefix, as tinymail requires the symbol to be like this...
 *
 * Returns: a tny-camel-session
 */
TnySessionCamel*    tny_account_store_get_session    (TnyAccountStore *self);

/**
 * tny_account_store_get_local_folders_account 
 * @self: a ModestTnyAccountStore instance
 * 
 * return the account corresponding to the local folders
 *
 * Returns: the tnyaccount for the local folders
 */
TnyAccount*    modest_tny_account_store_get_local_folders_account    (ModestTnyAccountStore *self);


/**
 * modest_tny_account_store_get_account_from_server_account
 * @server_account: the server account name
 * 
 * this function gets a reference to the #TnyAccount corresponding
 * to a given server account.
 *
 * Returns: a  #TnyFolder corresponding to a given server account, or NULL in case
 * of error
 */
TnyAccount*  modest_tny_account_store_get_tny_account_from_server_account
                   (ModestTnyAccountStore *self, const gchar* server_account);
G_END_DECLS

#endif /* __MODEST_TNY_ACCOUNT_STORE_H__ */


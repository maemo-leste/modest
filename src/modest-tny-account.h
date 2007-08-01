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

/*
 * TnyAccount Decorator
 */

#ifndef __MODEST_TNY_ACCOUNT_H__
#define __MODEST_TNY_ACCOUNT_H__

#include <tny-account.h>
#include <tny-folder.h>
#include <modest-account-mgr.h>
#include <modest-local-folder-info.h>
#include <tny-session-camel.h>

G_BEGIN_DECLS





/**
 * modest_tny_account_new_from_account:
 * @account_mgr: a valid account mgr instance
 * @account_name: the modest account name for which to create a corresponding tny account of the given type.
 * @type: the type of account to create (TNY_ACCOUNT_TYPE_STORE or TNY_ACCOUNT_TYPE_TRANSPORT)
 * @session: a tny camel session
 * @get_pass_func: the get-password function
 * @forget_pass_func: the forget-password function
 * 
 * get a tnyaccount corresponding to the server_accounts (store or transport) for this account.
 * 
 * Returns: a new TnyAccount or NULL in case of error.
 */
TnyAccount*
modest_tny_account_new_from_account (ModestAccountMgr *account_mgr, const gchar *account_name,
				     TnyAccountType type,
				     TnySessionCamel *session,
				     TnyGetPassFunc get_pass_func,
				     TnyForgetPassFunc forget_pass_func);


/**
 * modest_tny_account_update_from_account:
 * @tny_account: a valid tny_account
 * @account_mgr: a valid account mgr instance
 * @account_name: the modest account name for which to create a corresponding tny account of the given type.
 * @type: the type of account to create (TNY_ACCOUNT_TYPE_STORE or TNY_ACCOUNT_TYPE_TRANSPORT)
 * 
 * update a tnyaccount corresponding to the server_accounts (store or transport) for this account.
 * 
 * Returns: a TRUE or FALSE in case of error.
 */
gboolean
modest_tny_account_update_from_account (TnyAccount *tny_account, ModestAccountMgr *account_mgr,
					const gchar *account_name, TnyAccountType type);


/**
 * modest_tny_account_new_for_local_folders:
 * @account_mgr: a valid account mgr instance
 * @session: a tny camel session
 * @location_filepath: The location at which the local-folders directory exists, or NULL to specify $HOME.
 * 
 * get the local folders (pseudo) account; you should only need one such account.
 * 
 * Returns: a new local folders TnyAccount or NULL in case of error.
 */
TnyAccount* modest_tny_account_new_for_local_folders (ModestAccountMgr *account_mgr,
						      TnySessionCamel *session,
						      const gchar* location_filepath);

/**
 * modest_tny_account_new_for_per_account_local_outbox_folder:
 * @account_mgr: a valid account mgr instance
 * @account_name: a modest account name.
 * @session: a tny camel session
 * 
 * get the per-account local outbox folder (pseudo) account.
 * 
 * Returns: a new per-account local outbox folder TnyAccount or NULL in case of error.
 */
TnyAccount* modest_tny_account_new_for_per_account_local_outbox_folder (
	ModestAccountMgr *account_mgr, const gchar* account_name,
	TnySessionCamel *session);

/**
 * modest_tny_account_new_from_server_account_name:
 * @account_mgr: a valid account mgr instance
 * @session: a valid TnySessionCamel instance.
 * @server_account_name: the name of a server account in the configuration system.
 *
 * Returns: a new TnyAccount or NULL in case of error.
 */
TnyAccount*
modest_tny_account_new_from_server_account_name (ModestAccountMgr *account_mgr, 
						 TnySessionCamel *session,
						 const gchar* server_account_name);

/**
 * modest_tny_account_new_from_server_account_name:
 * @tny_account: a valid tny account
 * @account_mgr: a valid account mgr instance
 * @server_account_name: the name of a server account in the configuration system.
 *
 * update the given tny account; note that you cannot change the protocol type
 * 
 * Returns: a new TnyAccount or NULL in case of error.
 */
gboolean modest_tny_account_update_from_server_account_name (TnyAccount *tny_account,
							     ModestAccountMgr *account_mgr,
							     const gchar *server_account_name);


/**
 * modest_tny_account_get_special_folder:
 * @self: a TnyAccount
 * @special_type: the special folder to get
 * 
 * get the special (Inbox,Outbox,Sent,Draft etc.) folder for this server account's parent modest account.
 * Note: currently, the implementation will always return a local folder for this.
 * This can be changed later to return really account-specific special folders,
 * such as (for example) server-side Sent/Junk mail for IMAP accounts 
 * 
 * Returns: the tny folder corresponding to this special folder, or NULL in case
 * of error, or if the special folder does not exist for this account. 
 * This must be unrefed with g_object_unref().
 */
TnyFolder*    modest_tny_account_get_special_folder   (TnyAccount *self,
						       TnyFolderType special_type);


/**
 * modest_tny_folder_store_get_folder_count:
 * @self: a #TnyFolderStore
 * 
 * gets the number of folders of the account
 * 
 * Returns: the number of folder, or -1 in case of error
 **/
gint          modest_tny_folder_store_get_folder_count  (TnyFolderStore *self);

/**
 * modest_tny_folder_store_get_message_count:
 * @self: 
 * 
 * gets the number of messages in the account
 * 
 * Returns: the number of messages, or -1 in case of error
 **/
gint          modest_tny_folder_store_get_message_count (TnyFolderStore *self);

/**
 * modest_tny_folder_store_get_local_size:
 * @self: 
 * 
 * gets the total size occupied by the account in the local storage
 * device
 * 
 * Returns: the total size in bytes, or -1 in case of error
 **/
gint          modest_tny_folder_store_get_local_size    (TnyFolderStore *self);

/** modest_tny_account_get_parent_modest_account_name_for_server_account:
 * Get the name of the parent modest account of which the server account is a part.
 */
const gchar* modest_tny_account_get_parent_modest_account_name_for_server_account (TnyAccount *self);

/** modest_tny_account_set_parent_modest_account_name_for_server_account:
 * Set the name of the parent modest account of which the server account is a part,
 * so it can be retrieved later with 
 * modest_tny_account_get_parent_modest_account_name_for_server_account().
 */
void modest_tny_account_set_parent_modest_account_name_for_server_account (TnyAccount *account, const gchar* parent_modest_acount_name);


typedef void (*ModestTnyAccountGetMmcAccountNameCallback) (TnyStoreAccount* self, gpointer user_data);

/** modest_tny_account_get_mmc_account_name:
 * Asnchronously get the name of a memory card account and set it in the TnyAccount,
 * calling the callback (if not NULL) to notify that the name is changed.
 * if the name was changed. The callback will not be called if the name was not changed.
 */
void modest_tny_account_get_mmc_account_name (TnyStoreAccount* self, 
					      ModestTnyAccountGetMmcAccountNameCallback callback, 
					      gpointer user_data);


/** modest_tny_account_is_virtual_local_folders:
 * @self A TnyAccount.
 * 
 * A convenience function to identify whether TnyAccount 
 * is the virtual local folders account, containing the folders from local_folders/
 * and the outboxes from outboxes/<account-name>/.
 **/
gboolean modest_tny_account_is_virtual_local_folders (TnyAccount *self);

/** modest_tny_account_is_memory_card_account:
 * @self A TnyAccount.
 * 
 * A convenience function to identify whether TnyAccount 
 * is the memory card account.
 **/
gboolean modest_tny_account_is_memory_card_account (TnyAccount *self);


G_END_DECLS

#endif /* __MODEST_TNY_ACCOUNT_H__*/

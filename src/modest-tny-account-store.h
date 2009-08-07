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
#include <gtk/gtk.h>
#include <tny-account-store.h>
#include <tny-session-camel.h>
#include <tny-shared.h>
#include <tny-folder.h>
#include <modest-account-mgr.h>
#include <modest-tny-local-folders-account.h>

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

	void (*account_update)        (ModestTnyAccountStore *self,
				      const gchar *account_name,
				      gpointer user_data);
	void (*password_requested)    (ModestTnyAccountStore *self,
				       const gchar *server_account_name,
				       gchar **username,
				       gchar **password,
				       gboolean *remember,
				       gboolean *cancel,
				       gpointer user_data);

	/* Signals */
	void (*account_changed) (TnyAccountStore *self, TnyAccount *account);
	void (*account_inserted) (TnyAccountStore *self, TnyAccount *account);
	void (*account_removed) (TnyAccountStore *self, TnyAccount *account);
};

typedef enum {
	MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
	MODEST_TNY_ACCOUNT_STORE_QUERY_URL,
} ModestTnyAccountStoreQueryType;


/* We set 5Mb as the upper limit to consider disk full conditions */
#define MODEST_TNY_ACCOUNT_STORE_MIN_FREE_SPACE 5 * 1024 * 1024

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
ModestTnyAccountStore*    modest_tny_account_store_new (ModestAccountMgr *account_mgr,
							TnyDevice *device);


/**
 * modest_tny_account_store_get_account_by 
 * @self: a ModestTnyAccountStore instance
 * @id: some ID
 * 
 * get the account with the given str or NULL if it's not found
 * 
 * Returns: the tnyaccount or NULL,
 * g_object_unref when it's no longer needed
 */
TnyAccount* modest_tny_account_store_get_tny_account_by  (ModestTnyAccountStore *self,
							  ModestTnyAccountStoreQueryType type,
							  const gchar *str);

/**
 * modest_tny_account_store_get_server_account
 * @self: a ModestTnyAccountStore instance
 * @account_name: a modest account name
 * @type: the tny account type (#TNY_ACCOUNT_TYPE_STORE or #TNY_ACCOUNT_TYPE_TRANSPORT)
 * 
 * Get the tny account corresponding to one of the server_accounts for account with @account_name
 * 
 * Returns: the tnyaccount for the server account or NULL in case it's not found or error,
 * g_object_unref when it's no longer needed. TODO: Check that callers are unreffing.
 */
TnyAccount* modest_tny_account_store_get_server_account (ModestTnyAccountStore *self,
							 const gchar *account_name,
							 TnyAccountType type);

/**
 * modest_tny_account_store_get_transport_account_for_open_connection
 * @self: a ModestTnyAccountStore instance
 * @account_name: an account name
 * 
 * Get the tny account corresponding to the transport server account for the account with @account_name,
 * returning the connection-specific SMTP-server transport server account if one is specified,
 * otherwise just returning the regular transport server account.
 * 
 * Returns: the tnyaccount for the server account or NULL in case it's not found or error,
 * g_object_unref when it's no longer needed
 */						 
TnyAccount* modest_tny_account_store_get_transport_account_for_open_connection (ModestTnyAccountStore *self,
										const gchar *account_name);

TnyAccount* modest_tny_account_store_get_smtp_specific_transport_account_for_open_connection (ModestTnyAccountStore *self,
											      const gchar *account_name);

/**
 * tny_account_store_get_session
 * @self: a TnyAccountStore instance
 * 
 * get the tny-camel-session for this account store. Note that this function
 * does NOT have the modest_ prefix, as tinymail requires the symbol to be like this...
 *
 * Returns: a tny-camel-session
 */
TnySessionCamel*    modest_tny_account_store_get_session    (TnyAccountStore *self);

/** modest_tny_account_store_get_local_folders_account:
 * @self: a TnyAccountStore instance
 * 
 * Get the user-visible local folders account. It returns a new
 * reference so the caller must unref it when no longer needed
 **/
TnyAccount * modest_tny_account_store_get_local_folders_account (ModestTnyAccountStore *self);

/**
 * modest_tny_account_store_get_mmc_folders_account:
 * @self: a TnyAccountStore instance
 * 
 * Get the mmc folders account.
 *
 * Returns: a #TnyAccount, or %NULL if no mmc account is available
 * now. It returns a new reference so the caller must unref it when no
 * longer needed
 */
TnyAccount * modest_tny_account_store_get_mmc_folders_account (ModestTnyAccountStore *self);

/** modest_tny_account_store_forget_already_asked:
  * @self: a TnyAccountStore instance
  * @account: the account to forget the already asked status for
  *
  * Forget the already asked status of an account
  **/
void modest_tny_account_store_forget_already_asked (ModestTnyAccountStore *self, TnyAccount *account);

/**
 * modest_tny_account_store_find_msg_in_outboxes:
 * @self: a #ModestTnyAccountStore
 * @uri: the uri of the message
 * @ac_out: output attribute, %NULL, or the #TnyAccount of the message
 *
 * finds a message in the set of outboxes, using the uri.
 *
 * Returns: %NULL or a #TnyMsg
 **/
TnyMsg *modest_tny_account_store_find_msg_in_outboxes (ModestTnyAccountStore *self, 
						       const gchar *uri,
						       TnyAccount **ac_out);


/**
 * modest_tny_account_store_get_transport_account_from_outbox_header:
 * @self: a #ModestTnyAccountStore
 * @header: a #TnyHeader
 *
 * Gets the transport account from a header that is in the outbox
 *
 * Returns: %NULL or a %TnyTransportAccount. Returns a new reference
 * so the caller must unref it when no longer needed
 */
TnyTransportAccount * modest_tny_account_store_get_transport_account_from_outbox_header(ModestTnyAccountStore *self,
										       TnyHeader *header);

/**
 * modest_tny_account_store_new_connection_specific_transport_account:
 * @self: a #ModestTnyAccountStore
 * @name: the name of the connection specific smtp transport account
 *
 * Creates a connection specific transport account and put it in the merged outbox
 *
 * Returns: the new #TnyTransportAccount
 */
TnyTransportAccount * modest_tny_account_store_new_connection_specific_transport_account (ModestTnyAccountStore *self,
											  const gchar *name);

typedef void (*ModestTnyAccountStoreShutdownCallback) (ModestTnyAccountStore *account_store, gpointer userdata);

/**
 * modest_tny_account_store_shutdown:
 * @self: a #ModestTnyAccountStore
 * @callback: a #ModestTnyAccountStoreShutdownCallback
 * @userdata: a #gpointer
 *
 * Disconnects all registered accounts (forcing syncs for all of them).
 */
void modest_tny_account_store_shutdown (ModestTnyAccountStore *self,
					ModestTnyAccountStoreShutdownCallback callback,
					gpointer userdata);

/**
 * modest_tny_account_store_is_shutdown:
 * @self: a #ModestTnyAccountStore
 *
 * Check if all accounts are disconnected
 *
 * Returns: %TRUE if all accounts are disconnected, %FALSE otherwise.
 */
gboolean modest_tny_account_store_is_shutdown (ModestTnyAccountStore *self);

/**
 * modest_tny_account_store_is_send_mail_blocked:
 * @self: a #ModestTnyAccountStore
 * 
 * Tells if we've blocked the send queue flush attempts temporally. This is
 * usually done when we're editing an account, to prevent sending mails as
 * it can cause problems
 *
 * Returns: %TRUE if sending mail is blocked
 */
gboolean modest_tny_account_store_is_send_mail_blocked (ModestTnyAccountStore *self);

/**
 * modest_tny_account_store_set_send_mail_blocked:
 * @self: a #ModestTnyAccountStore
 * @blocked: a #gboolean
 * 
 * Sets as blocked/non blocked the send queue flush attempts temporally. This is
 * usually done when we're editing an account, to prevent sending mails as
 * it can cause problems
 */
void modest_tny_account_store_set_send_mail_blocked (ModestTnyAccountStore *self, gboolean blocked);

guint modest_tny_account_store_get_num_remote_accounts (ModestTnyAccountStore *self);

/**
 * modest_tny_account_store_start_send_queues:
 * @self: a #ModestTnyAccountStore
 *
 * Instantiates the send queues for the available transport
 * accounts. Note that send queues will start to listen to
 * connection-changed signals to try to send pending emails ASAP
 **/
void modest_tny_account_store_start_send_queues (ModestTnyAccountStore *self);

/**
 * modest_utils_check_disk_full_error:
 * @parent_window: a #GtkWidget that will be used as the parent of information banners that will be shown
 * @err: a #GError
 * @alternate: a string that contains the error that will be shown if
 * the memory full conditions happen in the MMC external storage
 *
 * Shows an information banner if the passed #GError is a disk full error.
 *
 * Returns: TRUE if it's a disk full error, false otherwise
 **/
gboolean modest_tny_account_store_check_disk_full_error (ModestTnyAccountStore *self,
							 GtkWidget *parent_window,
							 GError *err,
							 TnyAccount *account,
							 const gchar *alternate);

/**
 * modest_utils_is_disk_full_error:
 * @error: a #GError
 * @mail_op: the #ModestMailOperation that returned the error
 * @is_mmc: returns if the disk full error happens in the external MMC or not
 *
 * This function returns if the given #GError is a disk full error or
 * not, whithout showing anything to the user. The optional #is_mmc
 * returns TRUE if the disk full conditions happen in the external MMC
 * account
 *
 * Returns: TRUE if is a memory full error, FALSE otherwise.
 **/
gboolean modest_tny_account_store_is_disk_full_error (ModestTnyAccountStore *self,
						      GError *error,
						      TnyAccount *account);


void modest_tny_account_store_reset_attempt_count (ModestTnyAccountStore *self,
						   TnyAccount *account);

G_END_DECLS

#endif /* __MODEST_TNY_ACCOUNT_STORE_H__ */

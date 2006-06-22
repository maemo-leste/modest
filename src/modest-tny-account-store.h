/* modest-tny-account-store.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_ACCOUNT_STORE_H__
#define __MODEST_TNY_ACCOUNT_STORE_H__

#include <glib-object.h>
#include <tny-account-store.h>
#include <tny-session-camel.h>
#include <tny-shared.h>

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

	void (*password_requested) (ModestTnyAccountStore *self,
				    const gchar *account_name,
				    gpointer user_data);
};

/* member functions */

/**
 * modest_tny_account_store_get_type:
 *
 * Returns: GType of account store
 */
GType        modest_tny_account_store_get_type    (void) G_GNUC_CONST;

/**
 * modest_tny_account_store_new:
 * @modest_acc_mgr: account manager to use for new account store
 *
 * creates new (tinymail) account store for account manager modest_acc_mgr
 *
 * Returns: GObject of newly created account store
 */
GObject*    modest_tny_account_store_new         (ModestAccountMgr *modest_acc_mgr);

/**
 * modest_tny_account_store_get_account_mgr:
 * @self: a TnyAccountStore instance
 *
 * retrieve the account manager associated with this account store.
 *
 * Returns: the account manager for @self.
 */
ModestAccountMgr *modest_tny_account_store_get_accout_mgr(ModestTnyAccountStore *self);

/**
 * tny_account_store_get_session:
 * @self: a TnyAccountStore instance
 *
 * retrieve current tinymail camel session
 *
 * Returns: current tinymail camel session
 */
TnySessionCamel* tny_account_store_get_session (TnyAccountStore *self);

/**
 * tny_account_store_set_get_pass_func:
 * @self: a TnyAccountStore instance
 * key: a key
 * func: a function
 *
 * set the password function to function
 */
void
modest_tny_account_store_set_get_pass_func (ModestTnyAccountStore *, ModestTnyGetPassFunc);

G_END_DECLS

#endif /* __MODEST_TNY_ACCOUNT_STORE_H__ */


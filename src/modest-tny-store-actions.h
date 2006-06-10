/* modest-tny-store-actions.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_STORE_ACTIONS_H__
#define __MODEST_TNY_STORE_ACTIONS_H__

#include <glib-object.h>
#include <tny-store-account-iface.h>

G_BEGIN_DECLS

void    modest_tny_store_actions_update_folders (TnyStoreAccountIface *storage_account);

G_END_DECLS

#endif /* __MODEST_TNY_STORE_ACTIONS_H__ */


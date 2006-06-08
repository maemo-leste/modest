/* modest-tny-store-actions.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_STORE_ACTIONS_H__
#define __MODEST_TNY_STORE_ACTIONS_H__

#include <glib-object.h>
#include <tny-store-account-iface.h>

G_BEGIN_DECLS

/* standard convenience macros */
#define MODEST_TYPE_TNY_STORE_ACTIONS             (modest_tny_store_actions_get_type())
#define MODEST_TNY_STORE_ACTIONS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_STORE_ACTIONS,ModestTnyStoreActions))
#define MODEST_TNY_STORE_ACTIONS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_STORE_ACTIONS,ModestTnyStoreActionsClass))
#define MODEST_IS_TNY_STORE_ACTIONS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_STORE_ACTIONS))
#define MODEST_IS_TNY_STORE_ACTIONS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_STORE_ACTIONS))
#define MODEST_TNY_STORE_ACTIONS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_STORE_ACTIONS,ModestTnyStoreActionsClass))

typedef struct _ModestTnyStoreActions      ModestTnyStoreActions;
typedef struct _ModestTnyStoreActionsClass ModestTnyStoreActionsClass;

struct _ModestTnyStoreActions {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestTnyStoreActionsClass {
	GObjectClass parent_class;
	
	void (* update_folders) (ModestTnyStoreActions *self,
					   TnyStoreAccountIface *storage_account);
};

/* member functions */
GType        modest_tny_store_actions_get_type    (void) G_GNUC_CONST;

GObject*    modest_tny_store_actions_new         (void);

void    modest_tny_store_actions_update_folders (ModestTnyStoreActions *self,
					   TnyStoreAccountIface *storage_account);


G_END_DECLS

#endif /* __MODEST_TNY_STORE_ACTIONS_H__ */


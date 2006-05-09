/* modest-tny-account-store.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_ACCOUNT_STORE_H__
#define __MODEST_TNY_ACCOUNT_STORE_H__

#include <glib-object.h>
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_ACCOUNT_STORE             (modest_tny_account_store_get_type())
#define MODEST_TNY_ACCOUNT_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_ACCOUNT_STORE,ModestTnyAccountStore))
#define MODEST_TNY_ACCOUNT_STORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_ACCOUNT_STORE,GObject))
#define MODEST_IS_TNY_ACCOUNT_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_ACCOUNT_STORE))
#define MODEST_IS_TNY_ACCOUNT_STORE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_ACCOUNT_STORE))
#define MODEST_TNY_ACCOUNT_STORE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_ACCOUNT_STORE,ModestTnyAccountStoreClass))

typedef struct _ModestTnyAccountStore      ModestTnyAccountStore;
typedef struct _ModestTnyAccountStoreClass ModestTnyAccountStoreClass;

struct _ModestTnyAccountStore {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestTnyAccountStoreClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestTnyAccountStore* obj); */
};

/* member functions */
GType        modest_tny_account_store_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget*, */
/*    otherwise probably a GObject*. */
GObject*    modest_tny_account_store_new         (ModestAccountMgr *modest_acc_mgr);

/* fill in other public functions, eg.: */
/* 	void       modest_tny_account_store_do_something (ModestTnyAccountStore *self, const gchar* param); */
/* 	gboolean   modest_tny_account_store_has_foo      (ModestTnyAccountStore *self, gint value); */


G_END_DECLS

#endif /* __MODEST_TNY_ACCOUNT_STORE_H__ */


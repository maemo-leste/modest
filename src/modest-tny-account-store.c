/* modest-tny-account-store.c */

/* insert (c)/licensing information) */

#include <string.h>

#include <tny-account-store-iface.h>
#include <tny-account-iface.h>
#include <tny-account-store-iface.h>

#include <tny-account-iface.h>
#include <tny-store-account-iface.h>
#include <tny-transport-account-iface.h>

#include <tny-store-account.h>
#include <tny-transport-account.h>

#include "modest-account-mgr.h"
#include "modest-tny-account-store.h"

/* 'private'/'protected' functions */
static void modest_tny_account_store_class_init   (ModestTnyAccountStoreClass *klass);
static void modest_tny_account_store_init         (ModestTnyAccountStore *obj);
static void modest_tny_account_store_finalize     (GObject *obj);

/* implementations for tny-account-store-iface */
static void    modest_tny_account_store_iface_init              (gpointer g_iface, gpointer iface_data);

static void    modest_tny_account_store_add_store_account       (TnyAccountStoreIface *self,
							       TnyStoreAccountIface *account);
static void    modest_tny_account_store_add_transport_account   (TnyAccountStoreIface *self,
								       TnyTransportAccountIface *account);
static const GList*  modest_tny_account_store_get_store_accounts      (TnyAccountStoreIface *iface);
static const GList*  modest_tny_account_store_get_transport_accounts  (TnyAccountStoreIface *iface);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {
	ModestAccountMgr *modest_acc_mgr;	
};
#define MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                      MODEST_TYPE_TNY_ACCOUNT_STORE, \
                                                      ModestTnyAccountStorePrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_account_store_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyAccountStoreClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_account_store_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyAccountStore),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_account_store_init,
		};
		
		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) modest_tny_account_store_iface_init, 
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
                };

                my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestTnyAccountStore", &my_info, 0);
		
                g_type_add_interface_static (my_type, TNY_TYPE_ACCOUNT_STORE_IFACE,
					     &iface_info);
		
	}
	return my_type;
}

static void
modest_tny_account_store_class_init (ModestTnyAccountStoreClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_account_store_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyAccountStorePrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_tny_account_store_init (ModestTnyAccountStore *obj)
{
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);
	
	priv->modest_acc_mgr         = NULL;
}

static void
modest_tny_account_store_finalize (GObject *obj)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(obj);
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (priv->modest_acc_mgr) {
		g_object_unref (G_OBJECT(priv->modest_acc_mgr));
		priv->modest_acc_mgr = NULL;
	}
}

GObject*
modest_tny_account_store_new (ModestAccountMgr *modest_acc_mgr)
{
	GObject *obj;
	ModestTnyAccountStorePrivate *priv;

	g_return_val_if_fail (modest_acc_mgr, NULL);
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_ACCOUNT_STORE, NULL));

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);
	g_object_ref(G_OBJECT(priv->modest_acc_mgr = modest_acc_mgr));
	
	return obj;
}



/* FIXME: tinymail needs to change here */
/* a gpointer arg to get_password should be enough */
static gchar*
get_password (TnyAccountIface *account, const gchar *prompt)
{
	g_warning ("%s: %s", __FUNCTION__, prompt);
	return g_strdup("djcb123");
}


static void
forget_password (TnyAccountIface *account)
{
	g_warning (__FUNCTION__);
}



static gboolean
add_account  (TnyAccountStoreIface *self, TnyAccountIface *account)
{
	TnyAccountIface       *account_iface;
	ModestTnyAccountStore *account_store; 
	ModestTnyAccountStorePrivate *priv;

	const gchar* account_name; 
	const gchar *hostname, *username, *proto;

	g_warning (__FUNCTION__);		
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (account, FALSE);
	
	account_iface  = TNY_ACCOUNT_IFACE(account);
	account_store  = MODEST_TNY_ACCOUNT_STORE(self);
	priv           = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	account_name   = tny_account_iface_get_id(account_iface);
	if (!account_name) {
		g_warning ("failed to retrieve account name");
		return FALSE;
	}

	hostname =  tny_account_iface_get_hostname(account_iface);
	username =  tny_account_iface_get_user(account_iface);
	proto    =  tny_account_iface_get_proto(account_iface);

	return modest_account_mgr_add_server_account (priv->modest_acc_mgr,
						      account_name,
						      hostname, username, NULL,
						      proto);
}



static void
modest_tny_account_store_add_store_account  (TnyAccountStoreIface *self,
					     TnyStoreAccountIface *account)
{
	if (!add_account (self, TNY_ACCOUNT_IFACE(account)))
		g_warning ("failed to add store account");
}



static void
modest_tny_account_store_add_transport_account  (TnyAccountStoreIface *self,
						 TnyTransportAccountIface *account)
{
	if (!add_account (self, TNY_ACCOUNT_IFACE(account)))
		g_warning ("failed to add transport account");
}




static TnyAccountIface*
tny_account_from_key (ModestTnyAccountStore *self, const gchar *key,
		      gboolean is_store)
{
	TnyAccountIface *tny_account;
	ModestTnyAccountStorePrivate *priv;
	gchar *val;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (key, NULL);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* is it a store or a transport? */
	if (is_store)
		tny_account = TNY_ACCOUNT_IFACE(tny_store_account_new ());
	else
		tny_account = TNY_ACCOUNT_IFACE(tny_transport_account_new ());

	tny_account_iface_set_account_store (tny_account,
					     TNY_ACCOUNT_STORE_IFACE(self));
	/* id */
	tny_account_iface_set_id (tny_account, key);

	/* hostname */
	val = modest_account_mgr_get_account_string (priv->modest_acc_mgr, key,
						     MODEST_ACCOUNT_HOSTNAME, NULL);
	g_warning (val);
	tny_account_iface_set_hostname (tny_account, val);
	g_free (val);

	/* username */
	val = modest_account_mgr_get_account_string (priv->modest_acc_mgr, key,
						     MODEST_ACCOUNT_USERNAME, NULL);
	g_warning (val);
	tny_account_iface_set_user (tny_account, val);
	g_free (val);

	/* proto */
	val = modest_account_mgr_get_account_string (priv->modest_acc_mgr, key,
						     MODEST_ACCOUNT_PROTO, NULL);
	g_warning (val);
	tny_account_iface_set_proto (tny_account, val);
	g_free (val);

	g_warning ("set_pass");
	tny_account_iface_set_pass_func (tny_account, get_password);	
	tny_account_iface_set_forget_pass_func (tny_account, forget_password);	

	return tny_account;
}


static GList*
tny_accounts_from_server_accounts (ModestTnyAccountStore *self, GSList *accounts,
				   gboolean is_store)
{
	GSList *cursor = accounts;
	GList *tny_accounts = NULL;
	
	g_return_val_if_fail (self, NULL);
	
	while (cursor) {
		TnyAccountIface *tny_account;
		tny_account = tny_account_from_key (self, (gchar*)cursor->data, is_store);
		if (!tny_account) {
			g_warning ("could not create tnyaccount for %s",
				   (gchar*)cursor->data);
		} else {
			g_warning ("added %s",(gchar*)cursor->data);
			tny_accounts =
				g_list_append (tny_accounts, tny_account);
		}
		cursor = cursor->next;
	}

	return tny_accounts;
}




static const GList*
modest_tny_account_store_get_store_accounts  (TnyAccountStoreIface *iface)
{
	ModestTnyAccountStore        *self;
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts, *cursor;
	GList                        *tny_accounts;
	
	g_return_val_if_fail (iface, NULL);

	g_warning ("i'm being called: %s", __FUNCTION__);
	
	self = MODEST_TNY_ACCOUNT_STORE(iface);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	accounts =
		modest_account_mgr_server_account_names (priv->modest_acc_mgr,
							 NULL,
							 MODEST_PROTO_TYPE_STORE,
							 NULL, FALSE);

	g_warning ("accounts: %d", g_slist_length (accounts));
	tny_accounts = tny_accounts_from_server_accounts (self, accounts, TRUE);
	g_slist_free (accounts);
	g_warning ("store accounts: %d", g_list_length (tny_accounts));	
	
	return tny_accounts; /* FIXME: who will free this? */
}
	


static const GList*
modest_tny_account_store_get_transport_accounts (TnyAccountStoreIface *iface)
{
	ModestTnyAccountStore        *self;
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts, *cursor;
	GList                        *tny_accounts;
	
	g_return_val_if_fail (iface, NULL);
	
	self = MODEST_TNY_ACCOUNT_STORE(iface);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	accounts =
		modest_account_mgr_server_account_names (priv->modest_acc_mgr,
							 NULL,
							 MODEST_PROTO_TYPE_TRANSPORT,
							 NULL, FALSE);
	tny_accounts = tny_accounts_from_server_accounts (self, accounts, FALSE);
	g_warning ("transport accounts: %d", g_list_length (tny_accounts));

	g_slist_free (accounts);

	return tny_accounts; /* FIXME: who will free this? */
}
	

/**
 * modest_tny_account_store_get_cache_dir:
 * @self: self a TnyAccountStoreIface instance
 * 
 * returns the pathname of the cache directory
 *
 * Returns: a newly allocated string with the value of the pathname
 * to the cache directory or NULL if the environment variable $HOME is
 * not set,
 * pointer has to be freed by caller
 */
static const gchar*
modest_tny_account_store_get_cache_dir (TnyAccountStoreIface *self)
{
gchar *cache_dir;

	if (g_getenv("HOME") != NULL)
		cache_dir = g_strconcat(g_getenv("HOME"), "/.modest/cache/");
	else
		cache_dir = NULL;

	return cache_dir;
}



static void
modest_tny_account_store_iface_init (gpointer g_iface, gpointer iface_data)
{
        TnyAccountStoreIfaceClass *klass;

	g_return_if_fail (g_iface);
	
	klass = (TnyAccountStoreIfaceClass *)g_iface;

        klass->add_store_account_func      =
		modest_tny_account_store_add_store_account;
        klass->get_store_accounts_func     =
		modest_tny_account_store_get_store_accounts;
        klass->add_transport_account_func  =
		modest_tny_account_store_add_transport_account;
        klass->get_transport_accounts_func =
		modest_tny_account_store_get_transport_accounts;
	klass->get_cache_dir_func =
		modest_tny_account_store_get_cache_dir;	
}


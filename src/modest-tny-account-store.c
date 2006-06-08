/* modest-tny-account-store.c */

/* insert (c)/licensing information) */

#include <string.h>

#include <tny-account-iface.h>
#include <tny-account-store-iface.h>
#include <tny-store-account-iface.h>
#include <tny-transport-account-iface.h>
#include <tny-device-iface.h>
#include <tny-device.h>
#include <tny-account-store.h>

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
	PASSWORD_REQUESTED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {

	GList *store_accounts;
	GList *transport_accounts;
	gchar *cache_dir;

	TnySessionCamel *tny_session_camel;
	TnyDeviceIface  *device;

	ModestAccountMgr *modest_acc_mgr;
};
#define MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                      MODEST_TYPE_TNY_ACCOUNT_STORE, \
                                                      ModestTnyAccountStorePrivate))
/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

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
		/* hack hack */
		my_type = g_type_register_static (TNY_TYPE_ACCOUNT_STORE,
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

	g_type_class_add_private (gobject_class,
				  sizeof(ModestTnyAccountStorePrivate));

 	signals[PASSWORD_REQUESTED_SIGNAL] =
 		g_signal_new ("password_requested",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestTnyAccountStoreClass,password_requested),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
modest_tny_account_store_init (ModestTnyAccountStore *obj)
{
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->modest_acc_mgr         = NULL;
	priv->device                 = NULL;

	priv->store_accounts         = NULL;
	priv->transport_accounts     = NULL;
	priv->cache_dir              = NULL;

	priv->tny_session_camel      = NULL;
}


static void
free_gobject (GObject *obj, gpointer user_data)
{
	if (obj)
		g_object_unref (obj);
}


static GList*
free_gobject_list (GList *list)
{
	if (list) {
		g_list_foreach (list, (GFunc)free_gobject, NULL);
		g_list_free (list);
	}
	return NULL;
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

	if (priv->tny_session_camel) {
		g_object_unref (G_OBJECT(priv->tny_session_camel));
		priv->tny_session_camel = NULL;
	}

	if (priv->device) {
		g_object_unref (G_OBJECT(priv->device));
		priv->device = NULL;
	}

	priv->store_accounts     = free_gobject_list (priv->store_accounts);
	priv->transport_accounts = free_gobject_list (priv->store_accounts);

	g_free (priv->cache_dir);
	priv->cache_dir = NULL;



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

	priv->device = (TnyDeviceIface*)tny_device_new();
	if (!priv->device) {
		g_warning ("Cannot create Device instance");
		g_object_unref (obj);
		return NULL;
	}
	priv->tny_session_camel = tny_session_camel_new
		(TNY_ACCOUNT_STORE_IFACE(obj));
	if (!priv->tny_session_camel) {
		g_warning ("Cannot create TnySessionCamel instance");
		g_object_unref (obj);
		return NULL;
	}

	return obj;
}


static gchar*
get_password (TnyAccountIface *account, const gchar *prompt, gboolean *cancel)
{
	const gchar *key;
	const TnyAccountStoreIface *account_store;
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	gchar *val;

	g_return_val_if_fail (account, NULL);

	key = tny_account_iface_get_id (account);
	account_store = tny_account_iface_get_account_store(account);

	self = MODEST_TNY_ACCOUNT_STORE (account_store);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	val = modest_account_mgr_get_server_account_string (priv->modest_acc_mgr, key,
												MODEST_ACCOUNT_PASSWORD, NULL);
	if (!val) {
		/* FIXME:
		 * append the prompt to the emitted signal,
		 * so the password dialog shows the prompt supplied by the caller of this function.
		 */
		g_signal_emit (G_OBJECT(self), signals[PASSWORD_REQUESTED_SIGNAL], 0, key);
	}

	return val;
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

	if (!tny_account) {
		g_warning ("failed to create new tny %s account",
			   is_store ? "store" : "transport");
		return NULL;
	}

	tny_account_iface_set_account_store (TNY_ACCOUNT_IFACE(tny_account),
					     TNY_ACCOUNT_STORE_IFACE(self));
	/* id */
	tny_account_iface_set_id (tny_account, key);
	tny_account_iface_set_name (tny_account, key);
	
	/* proto */
	val = modest_account_mgr_get_server_account_string (priv->modest_acc_mgr, key,
							    MODEST_ACCOUNT_PROTO, NULL);
	if (val) {
		tny_account_iface_set_proto (tny_account, val);
		g_free (val);
	} else {
		g_warning ("protocol not defined for %s", key);
		g_object_unref (G_OBJECT(tny_account));
		return NULL;
	}

	/* hostname */
	val = modest_account_mgr_get_server_account_string (priv->modest_acc_mgr, key,
							    MODEST_ACCOUNT_HOSTNAME,
							    NULL);
	if (val) {
		tny_account_iface_set_hostname (tny_account, val);
		g_free (val);
	}


	/* username */
	val = modest_account_mgr_get_server_account_string (priv->modest_acc_mgr, key,
							    MODEST_ACCOUNT_USERNAME,
							    NULL);
	if (val) {
		tny_account_iface_set_user (tny_account, val);
		g_free (val);
	}

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
		tny_account = tny_account_from_key (self, (gchar*)cursor->data,
						    is_store);
		if (!tny_account) {
			g_warning ("could not create tnyaccount for %s",
				   (gchar*)cursor->data);
		} else {
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
	GSList                       *accounts;
	GList                        *tny_accounts;

	g_return_val_if_fail (iface, NULL);

	self = MODEST_TNY_ACCOUNT_STORE(iface);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	accounts =
		modest_account_mgr_server_account_names (priv->modest_acc_mgr,
							 NULL,
							 MODEST_PROTO_TYPE_STORE,
							 NULL, FALSE);

	tny_accounts = tny_accounts_from_server_accounts (self, accounts, TRUE);
	g_slist_free (accounts);
	
	/*
	 * FIXME: after gconf notification support is added,
	 * we can simply return priv->store_account
	 */
	priv->store_accounts = free_gobject_list (priv->store_accounts);
	priv->store_accounts = tny_accounts;

	return tny_accounts;
}


static const GList*
modest_tny_account_store_get_transport_accounts (TnyAccountStoreIface *iface)
{
	ModestTnyAccountStore        *self;
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts;
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

	/*
	 * FIXME: after gconf notification support is added,
	 * we can simply return priv->store_account
	 */
	priv->transport_accounts = free_gobject_list (priv->transport_accounts);
	priv->transport_accounts = tny_accounts;

	return tny_accounts; /* FIXME: who will free this? */
}


ModestAccountMgr
*modest_tny_account_store_get_accout_mgr(ModestTnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;
	g_return_val_if_fail (self, NULL);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	return priv->modest_acc_mgr;
}


TnySessionCamel*
tny_account_store_get_session (TnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;
	g_return_val_if_fail (self, NULL);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	return priv->tny_session_camel;
}


/**
 * modest_tny_account_store_get_cache_dir:
 * @self: self a TnyAccountStoreIface instance
 *
 * returns the pathname of the cache directory
 *
 * Returns: a string with the value of the pathname
 * to the cache directory or NULL if the environment variable $HOME is
 * not set. string should _not_ be freed by caller
 */
static const gchar*
modest_tny_account_store_get_cache_dir (TnyAccountStoreIface *self)
{
	ModestTnyAccountStorePrivate *priv;
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (!priv->cache_dir) {
		if (g_getenv("HOME") != NULL)
			priv->cache_dir = g_strconcat(g_getenv("HOME"),
						      "/.modest/cache/", NULL);
	}
	return priv->cache_dir;
}


static const TnyDeviceIface*
modest_tny_account_store_get_device (TnyAccountStoreIface *self)
{
	ModestTnyAccountStorePrivate *priv;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self);

	return priv->device;
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
	klass->get_device_func =
		modest_tny_account_store_get_device;
}

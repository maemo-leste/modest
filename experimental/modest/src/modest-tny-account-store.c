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
static void          modest_tny_account_store_iface_init              (gpointer g_iface, gpointer iface_data);

static gboolean      modest_tny_account_store_add_store_account       (TnyAccountStoreIface *self,
								       TnyStoreAccountIface *account);
static gboolean      modest_tny_account_store_add_transport_account   (TnyAccountStoreIface *self,
								       TnyTransportAccountIface *account);
static const GList*  modest_tny_account_store_get_store_accounts      (TnyAccountStoreIface *iface);
static const GList*  modest_tny_account_store_get_transport_accounts  (TnyAccountStoreIface *iface);

static gboolean destroy_all_accounts (ModestTnyAccountStore *self);
static gboolean get_all_accounts     (ModestTnyAccountStore *self);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {
	ModestAccountMgr *modest_acc_mgr;	
	GList       *tny_transport_accounts;
	GList       *tny_store_accounts;
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
	priv->tny_transport_accounts = NULL;
	priv->tny_store_accounts     = NULL;
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
	
	destroy_all_accounts (self);
}

GObject*
modest_tny_account_store_new (ModestAccountMgr *modest_acc_mgr)
{
	GObject *obj;
	ModestTnyAccountStorePrivate *priv;

	g_return_val_if_fail (modest_acc_mgr, NULL);
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_ACCOUNT_STORE, NULL));
	
	if (!get_all_accounts (MODEST_TNY_ACCOUNT_STORE(obj))) {
		g_warning ("could get accounts");
		g_object_unref (obj);
		return NULL;
	}
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);
	g_object_ref(G_OBJECT(priv->modest_acc_mgr = modest_acc_mgr));

	return obj;
}


static void
destroy_account (gpointer account)
{
	g_object_unref (G_OBJECT(account));
}


static gboolean
destroy_all_accounts (ModestTnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;

	g_return_val_if_fail (self, FALSE);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* clear up old stuff first */
	if (priv->tny_transport_accounts) {
		g_list_foreach (priv->tny_transport_accounts,
				(GFunc)destroy_account, NULL);
		g_list_free (priv->tny_transport_accounts);
		priv->tny_transport_accounts = NULL;

	}
	
	if (priv->tny_store_accounts) {
		g_list_foreach (priv->tny_store_accounts,
				(GFunc)destroy_account, NULL);
		g_list_free (priv->tny_store_accounts);
		priv->tny_store_accounts = NULL;
	}

	return TRUE;
}


/* FIXME: tinymail needs to change here */
/* a gpointer arg to get_password should be enough */
static gchar*
get_password (TnyAccountIface *account, const gchar *prompt)
{
	/* don't want to create all these, but there's no other way right now */
	ModestConf       *modest_conf;
	ModestAccountMgr *modest_acc_mgr;
	gchar            *pw = NULL;
	const gchar      *account_name;

	g_return_val_if_fail (account, NULL);

	modest_conf    = MODEST_CONF(modest_conf_new ());
	if (!modest_conf) {
		g_warning ("could not create conf");
		return NULL;
	}
	
	modest_acc_mgr = MODEST_ACCOUNT_MGR(modest_account_mgr_new (modest_conf));
	if (!modest_acc_mgr) {
		g_object_unref (modest_conf);
		g_warning ("could not create acc mgr");
		return NULL;
	}

	account_name = tny_account_iface_get_id(account);
	if (!account_name) {
		g_object_unref (modest_acc_mgr);
		g_object_unref (modest_conf);
		g_warning ("could not retrieve account name");
		return NULL;
	}
	
	pw = modest_account_mgr_get_account_string (modest_acc_mgr, account_name,
						    MODEST_ACCOUNT_PASSWORD, NULL);
				     
	g_object_unref (G_OBJECT(modest_conf));
	g_object_unref (G_OBJECT(modest_acc_mgr));
	
	return pw;
}


static void
forget_password (TnyAccountIface *account)
{
	g_warning (__FUNCTION__);
}



static gboolean
add_tny_account_from_account (ModestTnyAccountStore *self, const gchar* account_name)
{
	gchar *type, *val;
	TnyAccountIface *account_iface = NULL; 
	ModestTnyAccountStorePrivate *priv;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (account_name, FALSE);
	
	priv =	MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	type = modest_account_mgr_get_account_string (priv->modest_acc_mgr,
						      account_name,
						      MODEST_ACCOUNT_TYPE, NULL);
	if (!type) {
		g_warning ("error in account %s: type not defined", account_name);
		return FALSE;
	}

	if (strcmp (type, MODEST_ACCOUNT_TYPE_STORE) == 0) {
		account_iface = TNY_ACCOUNT_IFACE (tny_store_account_new ());
		priv->tny_store_accounts = g_list_append (priv->tny_store_accounts,
							  account_iface);
	
	} else if (strcmp (type, MODEST_ACCOUNT_TYPE_TRANSPORT) == 0) {
		account_iface = TNY_ACCOUNT_IFACE (tny_transport_account_new ());
		priv->tny_transport_accounts = g_list_append (priv->tny_transport_accounts,
							      account_iface);
	} else {
		g_warning ("invalid account '%s': type: '%s'", account_name, type);
		g_free (type);
		return FALSE;
	}

	g_free (type);
	tny_account_iface_set_id(account_iface, account_name);	
	
	val = modest_account_mgr_get_account_string (priv->modest_acc_mgr, account_name,
						     MODEST_ACCOUNT_PROTO,NULL);
	tny_account_iface_set_proto(account_iface, val);
	g_free (val);
	
	val = modest_account_mgr_get_account_string (priv->modest_acc_mgr, account_name,
						     MODEST_ACCOUNT_SERVER,NULL);
	tny_account_iface_set_hostname(account_iface, val);
	g_free (val);

	val = modest_account_mgr_get_account_string (priv->modest_acc_mgr, account_name,
						     MODEST_ACCOUNT_USER,NULL);
	tny_account_iface_set_user(account_iface, val);	
	g_free (val);

	tny_account_iface_set_pass_func(account_iface, get_password);	
	tny_account_iface_set_forget_pass_func(account_iface, forget_password);	

	return TRUE;
}



static gboolean
get_all_accounts (ModestTnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;
	GSList *account_names, *cursor;

	g_return_if_fail (self);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	destroy_all_accounts (self);	
	
	cursor = account_names = modest_account_mgr_account_names (priv->modest_acc_mgr,
								   NULL);
	while (cursor) {
		const gchar *account_name = (const gchar*) cursor->data;
		if (!add_tny_account_from_account (self, account_name)) {
			g_warning ("cannot add iface for account %s",
				   account_name);
			return FALSE;
		}
		cursor = cursor->next;
	}
	return TRUE;
}



static gboolean
modest_tny_account_store_add_store_account  (TnyAccountStoreIface *self,
					     TnyStoreAccountIface *account)
{
	TnyAccountIface       *account_iface;
	ModestTnyAccountStore *account_store; 
	ModestTnyAccountStorePrivate *priv;

	const gchar* account_name; 
	const gchar *hostname, *user, *proto;
	gboolean check;
	
	g_return_if_fail (self);
	g_return_if_fail (account);
	
	account_iface  = TNY_ACCOUNT_IFACE(account);
	account_store  = MODEST_TNY_ACCOUNT_STORE(self);
	priv           = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	account_name   = tny_account_iface_get_id(account_iface);
	if (!account_name) {
		g_warning ("failed to retrieve account name");
		return FALSE;
	}

	if (!modest_account_mgr_add_account (priv->modest_acc_mgr, account_name, NULL)) {
		g_warning ("failed to add account %s", account_name);
		return FALSE;
	}

	hostname =  tny_account_iface_get_hostname(account_iface);
	user     =  tny_account_iface_get_user(account_iface);
	proto    =  tny_account_iface_get_proto(account_iface);

	if (!hostname || !user || !proto) {
		g_warning ("error in account data: hostname:%s; user:%s; proto:%s",
			   hostname ? hostname : "<none>",
			   user     ? user     : "<none>",
			   proto    ? proto    : "<none>");
		return FALSE;
	}
	
	check = modest_account_mgr_set_account_string (priv->modest_acc_mgr,
						       account_name,MODEST_ACCOUNT_SERVER,
						       hostname, NULL);
	check = check && modest_account_mgr_set_account_string (priv->modest_acc_mgr,
								account_name,MODEST_ACCOUNT_USER,
								user, NULL);
	check = check && modest_account_mgr_set_account_string (priv->modest_acc_mgr,
								account_name, MODEST_ACCOUNT_PROTO,
								proto, NULL);
	if (!check)
		g_warning ("failed to set some account data");

	return check;
}


static const GList*
modest_tny_account_store_get_store_accounts  (TnyAccountStoreIface *iface)
{
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;

	g_return_val_if_fail (iface, NULL);
	
	self = MODEST_TNY_ACCOUNT_STORE(iface);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (!priv->tny_store_accounts)
		get_all_accounts (self);
		
	return priv->tny_store_accounts;
}
	

static gboolean
modest_tny_account_store_add_transport_account  (TnyAccountStoreIface *self,
						 TnyTransportAccountIface *account)
{		
	g_return_if_fail (self);
	g_return_if_fail (account);
	
	return modest_tny_account_store_add_transport_account (self, account);
}

	
static const GList*
modest_tny_account_store_get_transport_accounts  (TnyAccountStoreIface *iface)
{
	ModestTnyAccountStore        *self;
	ModestTnyAccountStorePrivate *priv;

	g_return_if_fail (iface);
	
	self = MODEST_TNY_ACCOUNT_STORE(iface);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (!priv->tny_transport_accounts)
		get_all_accounts (self);
	
	return priv->tny_transport_accounts;
}


static void
modest_tny_account_store_iface_init (gpointer g_iface, gpointer iface_data)
{
        TnyAccountStoreIfaceClass *klass;

	g_return_if_fail (g_iface);
	
	klass = (TnyAccountStoreIfaceClass *)g_iface;

        klass->add_store_account_func      = modest_tny_account_store_add_store_account;
        klass->get_store_accounts_func     = modest_tny_account_store_get_store_accounts;
        klass->add_transport_account_func  = modest_tny_account_store_add_transport_account;
        klass->get_transport_accounts_func = modest_tny_account_store_get_transport_accounts;
}


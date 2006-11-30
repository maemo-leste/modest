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

#include <string.h>

#include <tny-account.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>
#include <tny-device.h>
#include <tny-account-store.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-store-account.h>
#include <modest-marshal.h>
#include <glib/gi18n.h>
#include "modest-account-mgr.h"
#include "modest-tny-account-store.h"
#include "modest-tny-platform-factory.h"

/* 'private'/'protected' functions */
static void modest_tny_account_store_class_init   (ModestTnyAccountStoreClass *klass);
//static void modest_tny_account_store_init         (ModestTnyAccountStore *obj);
static void modest_tny_account_store_finalize     (GObject *obj);

/* implementations for tny-account-store-iface */
static void    modest_tny_account_store_instance_init (ModestTnyAccountStore *obj);

static void    modest_tny_account_store_init              (gpointer g, gpointer iface_data);
static void    modest_tny_account_store_add_store_account       (TnyAccountStore *self,
								 TnyStoreAccount *account);
static void    modest_tny_account_store_add_transport_account   (TnyAccountStore *self,
								 TnyTransportAccount *account);
static void    modest_tny_account_store_get_accounts            (TnyAccountStore *iface, TnyList *list,
								 TnyGetAccountsRequestType type);
/* list my signals */
enum {
	PASSWORD_REQUESTED_SIGNAL,
	ACCOUNT_UPDATE_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {

	GMutex *store_lock;	
	gchar *cache_dir;
	gulong sig1, sig2;

	GHashTable *password_hash;
	
	TnySessionCamel *tny_session_camel;
	TnyDevice  *device;
	
        ModestAccountMgr *account_mgr;
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
			0,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_account_store_instance_init,
			NULL
		};

		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) modest_tny_account_store_init,
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
                };
		/* hack hack */
		my_type = g_type_register_static (G_TYPE_OBJECT,
 						  "ModestTnyAccountStore",
						  &my_info, 0);
		g_type_add_interface_static (my_type, TNY_TYPE_ACCOUNT_STORE,
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
			      G_STRUCT_OFFSET(ModestTnyAccountStoreClass, password_requested),
			      NULL, NULL,
			      modest_marshal_VOID__STRING_POINTER_POINTER,
			      G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER);
	
	signals[ACCOUNT_UPDATE_SIGNAL] =
 		g_signal_new ("account_update",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestTnyAccountStoreClass, account_update),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	
}

static void
modest_tny_account_store_instance_init (ModestTnyAccountStore *obj)
{
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->account_mgr            = NULL;
	priv->device                 = NULL;
	priv->cache_dir              = NULL;
        priv->tny_session_camel      = NULL;

	priv->password_hash          = g_hash_table_new_full (g_str_hash, g_str_equal,
							      g_free, g_free);
}


static void
on_account_removed (ModestAccountMgr *acc_mgr, const gchar *account, gboolean server_account,
		    gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);

	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
		       account);
	
}


static void
on_account_changed (ModestAccountMgr *acc_mgr, const gchar *account, gboolean server_account,
		    const gchar *key, gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	
	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
		       account);
}


static ModestTnyAccountStore*
get_account_store_for_account (TnyAccount *account)
{
	return MODEST_TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(account), "account_store"));
}



static void
set_account_store_for_account (TnyAccount *account, ModestTnyAccountStore *store)
{
	g_object_set_data (G_OBJECT(account), "account_store", (gpointer)store);
}


static gchar*
get_password (TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	const gchar *key;
	const TnyAccountStore *account_store;
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	gchar *pwd = NULL;
	gboolean already_asked;
	
	g_return_val_if_fail (account, NULL);
	
	key           = tny_account_get_id (account);
	account_store = TNY_ACCOUNT_STORE(get_account_store_for_account (account));

	self = MODEST_TNY_ACCOUNT_STORE (account_store);
        priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* is it in the hash? if it's already there, it must be wrong... */
	already_asked = g_hash_table_lookup (priv->password_hash, key) != NULL;

	/* if the password is not already there, try ModestConf */
/* 	if (!already_asked)  */
		pwd  = modest_account_mgr_get_string (priv->account_mgr,
						      key, MODEST_ACCOUNT_PASSWORD,
						      TRUE, NULL);

/* 	/\* if it was already asked, it must have been wrong, so ask again *\/ */
/* 	if (!already_asked || !pwd || strlen(pwd) == 0) { */

/* 		/\* we don't have it yet. we emit a signal to get the password somewhere *\/ */
/* 		const gchar* name = tny_account_get_name (account); */
/* 		*cancel = TRUE; */
/* 		pwd     = NULL; */
/* 		g_signal_emit (G_OBJECT(self), signals[PASSWORD_REQUESTED_SIGNAL], 0, */
/* 			       name, &pwd, cancel); */
/* 		if (!*cancel) /\* remember the password *\/ */
/* 			modest_account_mgr_set_string (priv->account_mgr, */
/* 						       key, MODEST_ACCOUNT_PASSWORD, */
/* 						       pwd, TRUE, NULL); */
/* 	} else */
/* 		*cancel = FALSE; */

/* 	g_hash_table_insert (priv->password_hash, key, pwd); */
	
	return pwd; 
}


static void
forget_password (TnyAccount *account) {

	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	const TnyAccountStore *account_store;

        account_store = TNY_ACCOUNT_STORE(get_account_store_for_account (account));
	self = MODEST_TNY_ACCOUNT_STORE (account_store);
        priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
}



/* create a tnyaccount for the server account connected to the account with name 'key'
 */
static TnyAccount*
tny_account_from_name (ModestTnyAccountStore *self, const gchar *account, 
		       const gchar *server_account, ModestProtocolType modest_type)
{
	TnyAccount *tny_account;
	ModestTnyAccountStorePrivate *priv;
	gchar *val;
	GSList *options = NULL;
	GError *error = NULL;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account, NULL);
	g_return_val_if_fail (server_account, NULL);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* is it a store or a transport? */
	if  (modest_type == MODEST_PROTOCOL_TYPE_STORE) 
		tny_account = TNY_ACCOUNT(tny_camel_store_account_new ());
	else if (modest_type == MODEST_PROTOCOL_TYPE_TRANSPORT)
		tny_account = TNY_ACCOUNT(tny_camel_transport_account_new ());
	else
		g_assert_not_reached ();

	if (!tny_account) {
		g_printerr ("modest: failed to create new tny account for '%s:%s'\n",
			    account, server_account);
		return NULL;
	}
	
	set_account_store_for_account (TNY_ACCOUNT(tny_account), self);

	/* session */
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account),
				       priv->tny_session_camel);
	
	/* id */
	tny_account_set_id   (tny_account, server_account);

	/* name */
	val = modest_account_mgr_get_string (priv->account_mgr, account,
					     MODEST_ACCOUNT_DISPLAY_NAME, FALSE, NULL);
	if (val) {
		tny_account_set_name (tny_account, val);
		g_free (val);
	} else {
		g_printerr ("modest: display name not defined for '%s:%s'\n", 
			    account, server_account);
		g_object_unref (G_OBJECT(tny_account));
		return NULL;
	}

	/* proto */
	val = modest_account_mgr_get_string (priv->account_mgr, server_account,
					     MODEST_ACCOUNT_PROTO, TRUE, NULL);
	if (val) {
		tny_account_set_proto (tny_account, val);
		g_free (val);
	} else {
		g_printerr ("modest: protocol not defined for '%s:%s'\n", 
			    account, server_account);
		g_object_unref (G_OBJECT(tny_account));
		return NULL;
	}

	/* Options */
	options = modest_account_mgr_get_list (priv->account_mgr,
					       tny_account_get_id (tny_account),
					       MODEST_ACCOUNT_OPTIONS,
					       MODEST_CONF_VALUE_STRING,
					       TRUE,
					       &error);
	
	if (error) {
		g_warning ("Error retrieving account %s options: %s",
			   tny_account_get_id (tny_account), error->message);
		g_error_free (error);
	} else {
		GSList *tmp = options;
		while (options) {
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account), options->data);
			g_free (options->data);
			options = g_slist_next (options);
		}
		g_slist_free (tmp);
	}

	/* hostname */
	val = modest_account_mgr_get_string (priv->account_mgr, server_account,
					     MODEST_ACCOUNT_HOSTNAME, TRUE,
					     NULL);
	if (val) {
		tny_account_set_hostname (tny_account, val);
		g_free (val);
	}

	/* username */
	val = modest_account_mgr_get_string (priv->account_mgr, server_account,
					     MODEST_ACCOUNT_USERNAME, TRUE,
					     NULL);
	if (val) {
		tny_account_set_user (tny_account, val);
		g_free (val);
	}

	tny_account_set_pass_func (tny_account, get_password);
        tny_account_set_forget_pass_func (tny_account, forget_password);

	return tny_account;
}



static void
modest_tny_account_store_finalize (GObject *obj)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(obj);
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (priv->account_mgr) {
		g_signal_handler_disconnect (G_OBJECT(priv->account_mgr),
					     priv->sig1);
		g_signal_handler_disconnect (G_OBJECT(priv->account_mgr),
					     priv->sig2);
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	if (priv->tny_session_camel) {
		// FIXME: how to kill a camel
		priv->tny_session_camel = NULL;
	}

	if (priv->device) {
		g_object_unref (G_OBJECT(priv->device));
		priv->device = NULL;
	}

	if (priv->store_lock)
		g_mutex_free (priv->store_lock);

	g_free (priv->cache_dir);
	priv->cache_dir = NULL;

	if (priv->password_hash) {
		g_hash_table_destroy (priv->password_hash);
		priv->password_hash = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestTnyAccountStore*
modest_tny_account_store_new (ModestAccountMgr *account_mgr) {

	GObject *obj;
	ModestTnyAccountStorePrivate *priv;
	TnyPlatformFactory *pfact;
	
	g_return_val_if_fail (account_mgr, NULL);

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_ACCOUNT_STORE, NULL));

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	g_object_ref(G_OBJECT(account_mgr));
	priv->account_mgr = account_mgr;

	priv->sig1 = g_signal_connect (G_OBJECT(account_mgr), "account_changed",
				       G_CALLBACK (on_account_changed), obj);
	priv->sig2 = g_signal_connect (G_OBJECT(account_mgr), "account_removed",
				       G_CALLBACK (on_account_removed), obj);
	
	priv->store_lock = g_mutex_new ();

	pfact = TNY_PLATFORM_FACTORY (modest_tny_platform_factory_get_instance());
	if (!pfact) {
		g_printerr ("modest: cannot create platform factory\n");
		g_object_unref (obj);
		return NULL;
	}
	
	priv->device = TNY_DEVICE(tny_platform_factory_new_device(pfact));
	if (!priv->device) {
		g_printerr ("modest: cannot create device instance\n");
		g_object_unref (obj);
		return NULL;
	}
	tny_device_force_online (priv->device);
	
	priv->tny_session_camel = tny_session_camel_new (TNY_ACCOUNT_STORE(obj));

	if (!priv->tny_session_camel) {
		g_printerr ("modest: cannot create TnySessionCamel instance\n");
		g_object_unref (obj);
		return NULL;
	}

	return MODEST_TNY_ACCOUNT_STORE(obj);
}


static gboolean
add_account  (TnyAccountStore *self, TnyAccount *account) {

	ModestTnyAccountStore *account_store;
	ModestTnyAccountStorePrivate *priv;

	const gchar *account_name;
	const gchar *hostname, *username, *proto;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (account, FALSE);

	account_store  = MODEST_TNY_ACCOUNT_STORE(self);
	priv           = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	account_name   = tny_account_get_id(account);
	if (!account_name) {
		g_printerr ("modest: failed to retrieve account name\n");
		return FALSE;
	}

	hostname =  tny_account_get_hostname(account);
	username =  tny_account_get_user(account);
	proto    =  tny_account_get_proto(account);

	return modest_account_mgr_add_server_account (priv->account_mgr,
						      account_name,
						      hostname, username, NULL,
						      proto);
}


static void
modest_tny_account_store_add_store_account  (TnyAccountStore *self,
					     TnyStoreAccount *account)
{
	ModestTnyAccountStorePrivate *priv;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(account),
				       priv->tny_session_camel);
	
	if (!add_account (self, TNY_ACCOUNT(account)))
		g_printerr ("modest: failed to add store account\n");
}


static void
modest_tny_account_store_add_transport_account  (TnyAccountStore *self,
						 TnyTransportAccount *account)
{
	ModestTnyAccountStorePrivate *priv;
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(account),
				       priv->tny_session_camel);
	
	if (!add_account (self, TNY_ACCOUNT(account)))
		g_printerr ("modest: failed to add transport account\n");
}


static gchar*
get_server_account_for_account (ModestTnyAccountStore *self, const gchar *account_name,
				ModestProtocolType modest_type)
{
	ModestTnyAccountStorePrivate *priv;
	gchar *server;
	gchar *key;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (modest_type == MODEST_PROTOCOL_TYPE_STORE)
		key = MODEST_ACCOUNT_STORE_ACCOUNT;
	else if (modest_type == MODEST_PROTOCOL_TYPE_TRANSPORT)
		key = MODEST_ACCOUNT_TRANSPORT_ACCOUNT;
	else
		g_assert_not_reached();
	
	server = modest_account_mgr_get_string (priv->account_mgr,
						account_name,
						key, FALSE, NULL);
	if (!server)
		return NULL;
	
	if (!modest_account_mgr_account_exists (priv->account_mgr,
						server, TRUE, NULL)) {
		g_free (server);
		return NULL;
	}
	return server;
}

static void
modest_tny_account_store_get_accounts  (TnyAccountStore *iface,
					TnyList *list,
					TnyGetAccountsRequestType type)
{
	ModestTnyAccountStore        *self;
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts, *cursor;
	ModestProtocolType            modest_type;
	
	g_return_if_fail (iface);

	self = MODEST_TNY_ACCOUNT_STORE(iface);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	switch (type) {
	case TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS:
		modest_type = MODEST_PROTOCOL_TYPE_TRANSPORT;
		break;
	case TNY_ACCOUNT_STORE_STORE_ACCOUNTS:
		modest_type = MODEST_PROTOCOL_TYPE_STORE;
		break;
	case TNY_ACCOUNT_STORE_BOTH:
		modest_type = MODEST_PROTOCOL_TYPE_ANY;
		break;
	default:
		g_assert_not_reached ();
	}

	cursor = accounts = modest_account_mgr_account_names (priv->account_mgr, NULL); 

	while (cursor) {
		gchar       *account_name;
		gchar       *server_account;
		TnyAccount  *account;
		gboolean     is_server_account;

		account_name      = (gchar*)cursor->data;
		account           = NULL;
		is_server_account = FALSE;
		
 		if (!modest_account_mgr_account_get_enabled (priv->account_mgr, account_name)) { 
 			g_free (account_name); 
 			cursor = cursor->next;
			continue;
 		} 
		
		if (modest_type == MODEST_PROTOCOL_TYPE_TRANSPORT || modest_type == MODEST_PROTOCOL_TYPE_ANY) {
			server_account = get_server_account_for_account (self, account_name,
									 MODEST_PROTOCOL_TYPE_TRANSPORT);
			if (server_account) {
				account = tny_account_from_name (self, account_name, 
								       server_account,
								       MODEST_PROTOCOL_TYPE_TRANSPORT);
				is_server_account = TRUE;
			}

			if (!account)
				g_printerr ("modest: no transport account for '%s'\n",
					    account_name);
			else
				tny_list_prepend (list, G_OBJECT(account));
		
			g_free (server_account);
		}
		
		if (modest_type == MODEST_PROTOCOL_TYPE_STORE || modest_type == MODEST_PROTOCOL_TYPE_ANY) {
			server_account = get_server_account_for_account (self, account_name,
									 MODEST_PROTOCOL_TYPE_STORE);
			if (server_account) {
				account = tny_account_from_name (self, account_name, 
								       server_account,
								       MODEST_PROTOCOL_TYPE_STORE);
				is_server_account = TRUE;
			}

			if (!account)
				g_printerr ("modest: no store account for '%s'\n",
					    account_name);
			else
				tny_list_prepend (list, G_OBJECT(account));
			g_free (server_account);
		}

		g_free (account_name);
		cursor = cursor->next;
	}

	g_slist_free (accounts);

	tny_session_camel_set_account_store (priv->tny_session_camel, iface);
}


/*
 * the cache dir will be ~/.modest/cache
 * might want to change this in a simple #define...
 */
static const gchar*
modest_tny_account_store_get_cache_dir (TnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (!priv->cache_dir)
		priv->cache_dir = g_build_filename (g_get_home_dir(),
						    ".modest",
						    "cache",
						    NULL);
	return priv->cache_dir;
}


/*
 * callers need to unref
 */
static TnyDevice*
modest_tny_account_store_get_device (TnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self);

	return g_object_ref (G_OBJECT(priv->device));
}



static gboolean
modest_tny_account_store_alert (TnyAccountStore *self, TnyAlertType type,
				const gchar *prompt)
{
	g_printerr ("modest: alert [%d]: %s",
		    type, prompt);

	return TRUE;
}


static void
modest_tny_account_store_init (gpointer g, gpointer iface_data)
{
        TnyAccountStoreIface *klass;

	g_return_if_fail (g);

	klass = (TnyAccountStoreIface *)g;

	klass->get_accounts_func =
		modest_tny_account_store_get_accounts;
	klass->add_transport_account_func =
		modest_tny_account_store_add_transport_account;
	klass->add_store_account_func =
		modest_tny_account_store_add_store_account;
	klass->get_cache_dir_func =
		modest_tny_account_store_get_cache_dir;
	klass->get_device_func =
		modest_tny_account_store_get_device;
	klass->alert_func =
		modest_tny_account_store_alert;
}

void
modest_tny_account_store_set_get_pass_func (ModestTnyAccountStore *self,
					    ModestTnyGetPassFunc func)
{
	g_warning (__FUNCTION__);
	return; /* not implemented, we use signals */
}


TnySessionCamel*
tny_account_store_get_session    (TnyAccountStore *self)
{
	g_return_val_if_fail (self, NULL);
	
	return MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self)->tny_session_camel;
}

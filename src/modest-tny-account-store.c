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
static void    modest_tny_account_store_get_accounts            (TnyAccountStoreIface *iface, TnyListIface *list,
								 TnyGetAccountsRequestType type);

/* list my signals */
enum {
	PASSWORD_REQUESTED_SIGNAL,
	UPDATE_ACCOUNTS_SIGNAL,
	LAST_SIGNAL
};

/* Password Status */
enum {
        PW_NOT_INVALID,
        PW_INVALID
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {

	GMutex *store_lock;	
	gchar *cache_dir;

	TnySessionCamel *tny_session_camel;
	TnyDeviceIface  *device;

        ModestAccountMgr *modest_acc_mgr;
        gint pw_invalid;
        ModestTnyGetPassFunc get_pass_func;
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
			      G_STRUCT_OFFSET(ModestTnyAccountStoreClass, password_requested),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[UPDATE_ACCOUNTS_SIGNAL] =
 		g_signal_new ("update_accounts",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestTnyAccountStoreClass, update_accounts),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	
}

static void
modest_tny_account_store_init (ModestTnyAccountStore *obj)
{
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->modest_acc_mgr         = NULL;
	priv->device                 = NULL;
	priv->cache_dir              = NULL;

        priv->tny_session_camel      = NULL;
        /* Meaning: if not indicated otherwise, we have valid password data */
        priv->pw_invalid             = PW_NOT_INVALID;
        priv->get_pass_func          = NULL;
}


static void
on_account_removed (ModestAccountMgr *acc_mgr, const gchar *account, gboolean server_account,
		    gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);

	g_signal_emit (G_OBJECT(self), signals[UPDATE_ACCOUNTS_SIGNAL], 0,
		       account);
	
}


static void
on_account_changed (ModestAccountMgr *acc_mgr, const gchar *account, gboolean server_account,
		    const gchar *key, gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	
	g_signal_emit (G_OBJECT(self), signals[UPDATE_ACCOUNTS_SIGNAL], 0,
		       account);
}



static gchar*
get_password (TnyAccountIface *account, const gchar *prompt, gboolean *cancel)
{
	
	const gchar *key;
	const TnyAccountStoreIface *account_store;
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	gchar *retval;

	g_return_val_if_fail (account, NULL);

	key = tny_account_iface_get_id (account);
	account_store = tny_account_iface_get_account_store(account);

	self = MODEST_TNY_ACCOUNT_STORE (account_store);
        priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

        if (priv->pw_invalid==PW_NOT_INVALID) {
                retval = modest_account_mgr_get_string (priv->modest_acc_mgr,
							key,
							MODEST_ACCOUNT_PASSWORD,
							TRUE,
							NULL);
        } else {
                retval = priv->get_pass_func(account, prompt, cancel);
                if (!*cancel) {
                        priv->pw_invalid=PW_NOT_INVALID;
                        modest_account_mgr_set_string(priv->modest_acc_mgr,
						      key,
						      MODEST_ACCOUNT_PASSWORD,
						      retval, TRUE, 
						      NULL);
                }
        }
        return retval;
}


static void
forget_password (TnyAccountIface *account) {

	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	const TnyAccountStoreIface *account_store;

        account_store = tny_account_iface_get_account_store(account);
	self = MODEST_TNY_ACCOUNT_STORE (account_store);
        priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

        priv->pw_invalid=PW_INVALID;
}



static TnyAccountIface*
tny_account_from_name (ModestTnyAccountStore *self, const gchar *key, ModestProtoType modest_type)
{
	TnyAccountIface *tny_account;
	ModestTnyAccountStorePrivate *priv;
	gchar *val;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (key, NULL);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* is it a store or a transport? */
	if  (modest_type == MODEST_PROTO_TYPE_STORE)
		tny_account = TNY_ACCOUNT_IFACE(tny_store_account_new ());
	else if (modest_type == MODEST_PROTO_TYPE_TRANSPORT)
		tny_account = TNY_ACCOUNT_IFACE(tny_transport_account_new ());
	else
		g_assert_not_reached ();

	if (!tny_account) {
		g_printerr ("modest: failed to create new tny account for '%s'\n",
			    key);
		return NULL;
	}
	
	tny_account_iface_set_account_store (TNY_ACCOUNT_IFACE(tny_account),
					     TNY_ACCOUNT_STORE_IFACE(self));
	/* id */
	tny_account_iface_set_id (tny_account, key);
	tny_account_iface_set_name (tny_account, key);

	/* proto */
	val = modest_account_mgr_get_string (priv->modest_acc_mgr, key,
					     MODEST_ACCOUNT_PROTO, TRUE, NULL);
	if (val) {
		tny_account_iface_set_proto (tny_account, val);
		g_free (val);
	} else {
		g_printerr ("modest: protocol not defined for '%s'\n", key);
		g_object_unref (G_OBJECT(tny_account));
		return NULL;
	}

	/* hostname */
	val = modest_account_mgr_get_string (priv->modest_acc_mgr, key,
					     MODEST_ACCOUNT_HOSTNAME, TRUE,
					     NULL);
	if (val) {
		tny_account_iface_set_hostname (tny_account, val);
		g_free (val);
	}


	/* username */
	val = modest_account_mgr_get_string (priv->modest_acc_mgr, key,
					     MODEST_ACCOUNT_USERNAME, TRUE,
					     NULL);
	if (val) {
		tny_account_iface_set_user (tny_account, val);
		g_free (val);
	}

	tny_account_iface_set_pass_func (tny_account, get_password);
        tny_account_iface_set_forget_pass_func (tny_account, forget_password);

	return tny_account;
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

	g_mutex_free (priv->store_lock);

	g_free (priv->cache_dir);
	priv->cache_dir = NULL;
}


ModestTnyAccountStore*
modest_tny_account_store_new (ModestAccountMgr *modest_acc_mgr) {

	GObject *obj;
	ModestTnyAccountStorePrivate *priv;

	g_return_val_if_fail (modest_acc_mgr, NULL);

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_ACCOUNT_STORE, NULL));

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	g_object_ref(G_OBJECT(modest_acc_mgr));
	priv->modest_acc_mgr = modest_acc_mgr;

	g_signal_connect (G_OBJECT(modest_acc_mgr), "account_changed",
			  G_CALLBACK (on_account_changed), obj);
	g_signal_connect (G_OBJECT(modest_acc_mgr), "account_removed",
			  G_CALLBACK (on_account_removed), obj);

	priv->store_lock = g_mutex_new ();

	priv->device = (TnyDeviceIface*)tny_device_new();
	if (!priv->device) {
		g_printerr ("modest: cannot create device instance\n");
		g_object_unref (obj);
		return NULL;
	}
	
	priv->tny_session_camel = tny_session_camel_new (TNY_ACCOUNT_STORE_IFACE(obj));

	if (!priv->tny_session_camel) {
		g_printerr ("modest: cannot create TnySessionCamel instance\n");
		g_object_unref (obj);
		return NULL;
	}

	return MODEST_TNY_ACCOUNT_STORE(obj);
}


static gboolean
add_account  (TnyAccountStoreIface *self, TnyAccountIface *account) {

	TnyAccountIface       *account_iface;
	ModestTnyAccountStore *account_store;
	ModestTnyAccountStorePrivate *priv;

	const gchar *account_name;
	const gchar *hostname, *username, *proto;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (account, FALSE);

	account_iface  = TNY_ACCOUNT_IFACE(account);
	account_store  = MODEST_TNY_ACCOUNT_STORE(self);
	priv           = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	account_name   = tny_account_iface_get_id(account_iface);
	if (!account_name) {
		g_printerr ("modest: failed to retrieve account name\n");
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
		g_printerr ("modest: failed to add store account\n");
}


static void
modest_tny_account_store_add_transport_account  (TnyAccountStoreIface *self,
						 TnyTransportAccountIface *account)
{
	if (!add_account (self, TNY_ACCOUNT_IFACE(account)))
		g_printerr ("modest: failed to add transport account\n");
}


static void
modest_tny_account_store_get_accounts  (TnyAccountStoreIface *iface,
					TnyListIface *list,
					TnyGetAccountsRequestType type)
{
	ModestTnyAccountStore        *self;
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts, *cursor;
	ModestProtoType              modest_type;
	
	g_return_if_fail (iface);

	self = MODEST_TNY_ACCOUNT_STORE(iface);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	switch (type) {
	case TNY_ACCOUNT_STORE_IFACE_TRANSPORT_ACCOUNTS:
		modest_type = MODEST_PROTO_TYPE_TRANSPORT;
		break;
	case TNY_ACCOUNT_STORE_IFACE_STORE_ACCOUNTS:
		modest_type = MODEST_PROTO_TYPE_STORE;
		break;
	case TNY_ACCOUNT_STORE_IFACE_BOTH:
		modest_type = MODEST_PROTO_TYPE_ANY;
		break;
	default:
		g_assert_not_reached ();
	}

	accounts = modest_account_mgr_search_server_accounts (priv->modest_acc_mgr,
							      NULL, modest_type,
							      NULL, TRUE);
	cursor = accounts;
	while (cursor) {
		gchar           *account_name;
		TnyAccountIface *account_iface;

		account_name =  (gchar*)cursor->data;
		account_iface = tny_account_from_name (self, account_name, modest_type);
		
		if (!account_iface)
			g_printerr ("modest: failed to create account iface for '%s'\n",
				    account_name);
		else
			tny_list_iface_prepend (list, account_iface);

		g_free (account_name);
		cursor = cursor->next;
	}

	g_slist_free (accounts);

	tny_session_camel_set_current_accounts (priv->tny_session_camel,
						list);
}


static const gchar*
modest_tny_account_store_get_cache_dir (TnyAccountStoreIface *self)
{
	ModestTnyAccountStorePrivate *priv;
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (!priv->cache_dir)
		priv->cache_dir = g_build_filename (g_get_home_dir(),
						    ".modest", "cache", NULL);
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

	klass->get_accounts_func =
		modest_tny_account_store_get_accounts;
	klass->add_transport_account_func  =
		modest_tny_account_store_add_transport_account;
	klass->get_cache_dir_func =
		modest_tny_account_store_get_cache_dir;
	klass->get_device_func =
		modest_tny_account_store_get_device;
}

void
modest_tny_account_store_set_get_pass_func (ModestTnyAccountStore *self,
					    ModestTnyGetPassFunc func)
{
        ModestTnyAccountStorePrivate *priv;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

        priv->get_pass_func=func;
}


TnySessionCamel*
tny_account_store_get_session    (TnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	return priv->tny_session_camel;
}

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
#include <glib/gi18n.h>

#include <tny-account.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <modest-runtime.h>
#include <modest-marshal.h>
#include <modest-protocol-info.h>
#include <modest-local-folder-info.h>
#include <modest-tny-account.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>

#include "modest-tny-account-store.h"
#include "modest-tny-platform-factory.h"
#include <tny-gtk-lockable.h>
#include <camel/camel.h>

/* 'private'/'protected' functions */
static void modest_tny_account_store_class_init   (ModestTnyAccountStoreClass *klass);
//static void modest_tny_account_store_init         (ModestTnyAccountStore *obj);
static void modest_tny_account_store_finalize     (GObject *obj);

/* implementations for tny-account-store-iface */
static void    modest_tny_account_store_instance_init (ModestTnyAccountStore *obj);
static void    modest_tny_account_store_init          (gpointer g, gpointer iface_data);


/* list my signals */
enum {
	ACCOUNT_UPDATE_SIGNAL,
	PASSWORD_REQUESTED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {
	gchar              *cache_dir;	
	GHashTable         *password_hash;
	ModestAccountMgr   *account_mgr;
	TnySessionCamel    *session;
	TnyDevice          *device;
	
	/* we cache them here */
	GSList             *store_accounts;
	GSList             *transport_accounts;
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

	 signals[ACCOUNT_UPDATE_SIGNAL] =
 		g_signal_new ("account_update",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestTnyAccountStoreClass, account_update),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	 
	 signals[PASSWORD_REQUESTED_SIGNAL] =
		 g_signal_new ("password_requested",
			       G_TYPE_FROM_CLASS (gobject_class),
			       G_SIGNAL_RUN_FIRST,
			       G_STRUCT_OFFSET(ModestTnyAccountStoreClass, password_requested),
			       NULL, NULL,
			       modest_marshal_VOID__STRING_POINTER_POINTER_POINTER,
			       G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER,
			       G_TYPE_POINTER);
}


static void
modest_tny_account_store_instance_init (ModestTnyAccountStore *obj)
{
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->cache_dir              = NULL;
	priv->account_mgr            = NULL;
	priv->session                = NULL;
	priv->device                 = NULL;
	
	priv->password_hash          = g_hash_table_new_full (g_str_hash, g_str_equal,
							      g_free, g_free);
}

static void
account_list_free (GSList *accounts)
{
	GSList *cursor = accounts;
	while (cursor) {
		g_object_unref (G_OBJECT(cursor->data));
/* 		if (G_IS_OBJECT(cursor->data)) */
/* 			g_warning ("BUG: account %s still holds refs", */
/* 				   tny_account_get_id (TNY_ACCOUNT(cursor->data))); */
		cursor = cursor->next;
	}
	g_slist_free (accounts);
}


static void
on_account_removed (ModestAccountMgr *acc_mgr, const gchar *account, gboolean server_account,
		    gpointer user_data)
{
	ModestTnyAccountStore *self        = MODEST_TNY_ACCOUNT_STORE(user_data);
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* FIXME: make this more finegrained; changes do not really affect _all_
	 * accounts, and some do not affect tny accounts at all (such as 'last_update')
	 */
	account_list_free (priv->store_accounts);
	priv->store_accounts = NULL;
	
	account_list_free (priv->transport_accounts);
	priv->transport_accounts = NULL;
	
	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
		       account);
}


static void
on_account_changed (ModestAccountMgr *acc_mgr, const gchar *account, gboolean server_account,
		    const gchar *key, gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* FIXME: make this more finegrained; changes do not really affect _all_
	 * accounts, and some do not affect tny accounts at all (such as 'last_update')
	 */
	account_list_free (priv->store_accounts);
	priv->store_accounts = NULL;
	
	account_list_free (priv->transport_accounts);
	priv->transport_accounts = NULL;

	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
		       account);
}


static ModestTnyAccountStore*
get_account_store_for_account (TnyAccount *account)
{
	return MODEST_TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(account),
							   "account_store"));
}

static gchar*
get_password (TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	const gchar *key;
	const TnyAccountStore *account_store;
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	gchar *pwd = NULL;
	gpointer pwd_ptr;
	gboolean already_asked;
	
	key           = tny_account_get_id (account);
	account_store = TNY_ACCOUNT_STORE(get_account_store_for_account (account));
	
	self = MODEST_TNY_ACCOUNT_STORE (account_store);
        priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* is it in the hash? if it's already there, it must be wrong... */
	pwd_ptr = (gpointer)&pwd; /* pwd_ptr so the compiler does not complained about
				   * type-punned ptrs...*/
	already_asked = g_hash_table_lookup_extended (priv->password_hash,
						      key,
						      NULL,
						      (gpointer*)&pwd_ptr);

	/* if the password is not already there, try ModestConf */
	if (!already_asked) {
		pwd  = modest_account_mgr_get_string (priv->account_mgr,
						      key, MODEST_ACCOUNT_PASSWORD,TRUE);
		g_hash_table_insert (priv->password_hash, g_strdup (key), g_strdup (pwd));
	}

	/* if it was already asked, it must have been wrong, so ask again */
	if (already_asked || !pwd || strlen(pwd) == 0) {

		/* we don't have it yet. Get the password from the user */
		const gchar* name = tny_account_get_name (account);
		gboolean remember = FALSE;
		pwd = NULL;
		
		g_signal_emit (G_OBJECT(self), signals[PASSWORD_REQUESTED_SIGNAL], 0,
			       name, &pwd, cancel, &remember);
		
		if (!*cancel) {
			if (remember)
				modest_account_mgr_set_string (priv->account_mgr,key,
							       MODEST_ACCOUNT_PASSWORD,
							       pwd, TRUE);
			/* We need to dup the string even knowing that
			   it's already a dup of the contents of an
			   entry, because it if it's wrong, then camel
			   will free it */
			g_hash_table_insert (priv->password_hash, g_strdup (key), g_strdup(pwd));
		} else {
			g_hash_table_remove (priv->password_hash, key);
			g_free (pwd);
			pwd = NULL;
		}
	} else
		*cancel = FALSE;
 
	return pwd;
}


static void
forget_password (TnyAccount *account)
{
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	const TnyAccountStore *account_store;
	gchar *pwd;
	const gchar *key;
	
        account_store = TNY_ACCOUNT_STORE(get_account_store_for_account (account));
	self = MODEST_TNY_ACCOUNT_STORE (account_store);
        priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	key  = tny_account_get_id (account);

	/* Do not remove the key, this will allow us to detect that we
	   have already asked for it at least once */
	pwd = g_hash_table_lookup (priv->password_hash, key);
	if (pwd) {
		memset (pwd, 0, strlen (pwd));
		g_hash_table_insert (priv->password_hash, g_strdup (key), NULL);
	}

	/* Remove from configuration system */
	modest_account_mgr_unset (priv->account_mgr,
				  key, MODEST_ACCOUNT_PASSWORD, TRUE);
}


static void
modest_tny_account_store_finalize (GObject *obj)
{
	ModestTnyAccountStore *self        = MODEST_TNY_ACCOUNT_STORE(obj);
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	g_free (priv->cache_dir);
	priv->cache_dir = NULL;
	
	if (priv->password_hash) {
		g_hash_table_destroy (priv->password_hash);
		priv->password_hash = NULL;
	}

	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	if (priv->device) {
		g_object_unref (G_OBJECT(priv->device));
		priv->device = NULL;
	}

	/* this includes the local folder */
	account_list_free (priv->store_accounts);
	priv->store_accounts = NULL;
	
	account_list_free (priv->transport_accounts);
	priv->transport_accounts = NULL;

	if (priv->session) {
		camel_object_unref (CAMEL_OBJECT(priv->session));
		priv->session = NULL;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestTnyAccountStore*
modest_tny_account_store_new (ModestAccountMgr *account_mgr, TnyDevice *device) {

	GObject *obj;
	ModestTnyAccountStorePrivate *priv;
	TnyList *list;
	
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (device, NULL);

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_ACCOUNT_STORE, NULL));
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->account_mgr = account_mgr;
	g_object_ref (G_OBJECT(priv->account_mgr));

	priv->device = device;
	g_object_ref (priv->device);
	
	priv->session = tny_session_camel_new (TNY_ACCOUNT_STORE(obj));
	
	tny_session_camel_set_ui_locker (priv->session,	 tny_gtk_lockable_new ());
	/* FIXME: unref this in the end? */

	/* force a cache fill... ugly */
	list = TNY_LIST(tny_simple_list_new());
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE(obj), list,
					TNY_ACCOUNT_STORE_BOTH);
	g_object_unref(list);
	
	/* Connect signals */
	g_signal_connect (G_OBJECT(account_mgr), "account_changed",
				       G_CALLBACK (on_account_changed), obj);
	g_signal_connect (G_OBJECT(account_mgr), "account_removed",
				       G_CALLBACK (on_account_removed), obj);

	return MODEST_TNY_ACCOUNT_STORE(obj);
}


static void
get_cached_accounts (TnyAccountStore *self, TnyList *list, TnyAccountType type)
{
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts, *cursor;
	
	priv     = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	accounts = (type == TNY_ACCOUNT_TYPE_STORE ? priv->store_accounts : priv->transport_accounts);

	cursor = accounts;
	while (cursor) {
		tny_list_prepend (list, G_OBJECT(cursor->data));
		cursor = cursor->next;
	}
}

/* this function fills the TnyList, and also returns a GSList of the accounts,
 * for caching purposes
 */
static GSList*
get_accounts  (TnyAccountStore *self, TnyList *list, TnyAccountType type)
{
	ModestTnyAccountStorePrivate *priv;
	GSList                       *account_names, *cursor;
	GSList                       *accounts = NULL;
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
 
	account_names = modest_account_mgr_account_names (priv->account_mgr);
	
	for (cursor = account_names; cursor; cursor = cursor->next) {
		
		gchar *account_name = (gchar*)cursor->data;
		
		/* only return enabled accounts */
		if (modest_account_mgr_get_enabled(priv->account_mgr, account_name)) {
			TnyAccount *tny_account = 
				modest_tny_account_new_from_account (priv->account_mgr, account_name,
								     type, priv->session, get_password,
								     forget_password);
			if (tny_account) { /* something went wrong */
				g_object_set_data (G_OBJECT(tny_account), "account_store", (gpointer)self);
				tny_list_prepend (list, G_OBJECT(tny_account));
				accounts = g_slist_append (accounts, tny_account); /* cache it */
			} else
				g_printerr ("modest: failed to create account for %s\n", account_name);
		}
		g_free (account_name);
	}
	g_slist_free (account_names);

	/* also, add the local folder pseudo-account */
	if (type == TNY_ACCOUNT_TYPE_STORE) {
		TnyAccount *tny_account =
			modest_tny_account_new_for_local_folders (priv->account_mgr, priv->session);
		tny_list_prepend (list, G_OBJECT(tny_account));
		accounts = g_slist_append (accounts, tny_account); /* cache it */
	}

	return accounts;
}
	

static void
modest_tny_account_store_get_accounts  (TnyAccountStore *self, TnyList *list,
					TnyGetAccountsRequestType request_type)
{
	ModestTnyAccountStorePrivate *priv;
	
	g_return_if_fail (self);
	g_return_if_fail (TNY_IS_LIST(list));
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (request_type == TNY_ACCOUNT_STORE_BOTH) {
		modest_tny_account_store_get_accounts (self, list, TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
		modest_tny_account_store_get_accounts (self, list, TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);
		return;
	}
	
	if (request_type == TNY_ACCOUNT_STORE_STORE_ACCOUNTS)  {
		
		if (!priv->store_accounts)
			priv->store_accounts = get_accounts (self, list, TNY_ACCOUNT_TYPE_STORE);
		else
			get_cached_accounts (self, list, TNY_ACCOUNT_TYPE_STORE);

	} else if (request_type == TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS) {

		if (!priv->transport_accounts)
			priv->transport_accounts = get_accounts (self, list, TNY_ACCOUNT_TYPE_TRANSPORT);
		else
			get_cached_accounts (self, list, TNY_ACCOUNT_TYPE_TRANSPORT);
	} else {
		g_return_if_reached (); /* incorrect req type */
		return;
	}
}


static const gchar*
modest_tny_account_store_get_cache_dir (TnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (!priv->cache_dir)
		priv->cache_dir = g_build_filename (g_get_home_dir(), 
						    MODEST_DIR,
						    MODEST_CACHE_DIR,
						    "cache", NULL);
	return priv->cache_dir;
}


/*
 * callers need to unref
 */
static TnyDevice*
modest_tny_account_store_get_device (TnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (priv->device)
		return g_object_ref (G_OBJECT(priv->device));
	else
		return NULL;
}



static gboolean
modest_tny_account_store_alert (TnyAccountStore *self, TnyAlertType type,
				const gchar *prompt)
{
	GtkMessageType gtktype;
	gboolean retval = FALSE;
	GtkWidget *dialog;

	switch (type)
	{
		case TNY_ALERT_TYPE_INFO:
		gtktype = GTK_MESSAGE_INFO;
		break;
		case TNY_ALERT_TYPE_WARNING:
		gtktype = GTK_MESSAGE_WARNING;
		break;
		case TNY_ALERT_TYPE_ERROR:
		default:
		gtktype = GTK_MESSAGE_ERROR;
		break;
	}

	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
		gtktype, GTK_BUTTONS_YES_NO, prompt);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
		retval = TRUE;

	gtk_widget_destroy (dialog);

	return retval;
}



static void
modest_tny_account_store_add_store_account  (TnyAccountStore *self,
					     TnyStoreAccount *account)
{
	/* we should not need this...*/
	g_printerr ("modest: add_store_account_func not implemented\n");
}


static void
modest_tny_account_store_add_transport_account  (TnyAccountStore *self,
						 TnyTransportAccount *account)
{	
	/* we should not need this...*/
	g_printerr ("modest: add_transport_account_func not implemented\n");
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
	/* not implemented, we use signals */
	g_printerr ("modest: set_get_pass_func not implemented\n");
}

TnySessionCamel*
tny_account_store_get_session  (TnyAccountStore *self)
{
	g_return_val_if_fail (self, NULL);	
	return MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self)->session;
}


TnyAccount*
modest_tny_account_store_get_tny_account_by_id  (ModestTnyAccountStore *self, const gchar *id)
{
	TnyAccount *account = NULL;
	ModestTnyAccountStorePrivate *priv;	
	GSList *cursor;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (id, NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	for (cursor = priv->store_accounts; cursor ; cursor = cursor->next) {
		const gchar *acc_id = tny_account_get_id (TNY_ACCOUNT(cursor->data));
		if (acc_id && strcmp (acc_id, id) == 0) {
			account = TNY_ACCOUNT(cursor->data);
			break;
		}
	}

	/* if we already found something, no need to search the transport accounts */
	for (cursor = priv->transport_accounts; !account && cursor ; cursor = cursor->next) {
		const gchar *acc_id = tny_account_get_id (TNY_ACCOUNT(cursor->data));
		if (acc_id && strcmp (acc_id, id) == 0) {
			account = TNY_ACCOUNT(cursor->data);
			break;
		}
	}

	if (account)
		g_object_ref (G_OBJECT(account));
	
	return account;
}


TnyAccount*
modest_tny_account_store_get_tny_account_by_account (ModestTnyAccountStore *self,
						     const gchar *account_name,
						     TnyAccountType type)
{
	TnyAccount *account = NULL;
	ModestAccountData *account_data;
	const gchar *id = NULL;
	ModestTnyAccountStorePrivate *priv;	

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_name, NULL);
	g_return_val_if_fail (type == TNY_ACCOUNT_TYPE_STORE || type == TNY_ACCOUNT_TYPE_TRANSPORT,
			      NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	account_data = modest_account_mgr_get_account_data (priv->account_mgr, account_name);
	if (!account_data) {
		g_printerr ("modest: cannot get account data for account '%s'\n", account_name);
		return NULL;
	}

	if (type == TNY_ACCOUNT_TYPE_STORE && account_data->store_account)
		id = account_data->store_account->account_name;
	else if (account_data->transport_account)
		id = account_data->transport_account->account_name;

	if (id) 
		account =  modest_tny_account_store_get_tny_account_by_id  (self, id);
	if (!account)
		g_printerr ("modest: could not get tny %s account for %s (id=%s)\n",
			    type == TNY_ACCOUNT_TYPE_STORE? "store" : "transport",
			    account_name, id ? id : "<none>");

	modest_account_mgr_free_account_data (priv->account_mgr, account_data);
	return account;	
}

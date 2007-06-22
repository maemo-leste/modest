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
#include <tny-error.h>
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
#include <modest-tny-local-folders-account.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <widgets/modest-window-mgr.h>
#include <modest-account-settings-dialog.h>


#include "modest-tny-account-store.h"
#include "modest-tny-platform-factory.h"
#include <tny-gtk-lockable.h>
#include <camel/camel.h>

#ifdef MODEST_PLATFORM_MAEMO
#include <tny-maemo-conic-device.h>
#ifdef MODEST_HILDON_VERSION_0
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-banner.h>
#else
#include <hildon/hildon-note.h>
#include <hildon/hildon-banner.h>
#endif
#endif

#include <libgnomevfs/gnome-vfs-volume-monitor.h>

/* 'private'/'protected' functions */
static void modest_tny_account_store_class_init   (ModestTnyAccountStoreClass *klass);
//static void modest_tny_account_store_init         (ModestTnyAccountStore *obj);
static void modest_tny_account_store_finalize     (GObject *obj);

/* implementations for tny-account-store-iface */
static void    modest_tny_account_store_instance_init (ModestTnyAccountStore *obj);
static void    modest_tny_account_store_init          (gpointer g, gpointer iface_data);


static void    get_server_accounts                    (TnyAccountStore *self, 
						       TnyList *list, 
						       TnyAccountType type);

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
	
	/* We cache the lists of accounts here.
	 * They are created in our get_accounts_func() implementation. */
	GSList             *store_accounts;
	GSList             *transport_accounts;
	
	/* This is also contained in store_accounts,
	 * but we cached it temporarily separately, 
	 * because we create this while creating the transport accounts, 
	 * but return it when requesting the store accounts: 
	 */
	GSList             *store_accounts_outboxes;
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
			       modest_marshal_VOID__STRING_POINTER_POINTER_POINTER_POINTER,
			       G_TYPE_NONE, 5, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
			       G_TYPE_POINTER);
}


     
static void
on_vfs_volume_mounted(GnomeVFSVolumeMonitor *volume_monitor, 
	GnomeVFSVolume *volume, gpointer user_data);

static void
on_vfs_volume_unmounted(GnomeVFSVolumeMonitor *volume_monitor, 
	GnomeVFSVolume *volume, gpointer user_data);

static void
modest_tny_account_store_instance_init (ModestTnyAccountStore *obj)
{
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->cache_dir              = NULL;
	priv->account_mgr            = NULL;
	priv->session                = NULL;
	priv->device                 = NULL;
	
	/* An in-memory store of passwords, 
	 * for passwords that are not remembered in the configuration,
         * so they need to be asked for from the user once in each session:
         */
	priv->password_hash          = g_hash_table_new_full (g_str_hash, g_str_equal,
							      g_free, g_free);
							      
	/* Respond to volume mounts and unmounts, such 
	 * as the insertion/removal of the memory card: */
	GnomeVFSVolumeMonitor* monitor = 
		gnome_vfs_get_volume_monitor();
	g_signal_connect (G_OBJECT(monitor), "volume-mounted",
			  G_CALLBACK(on_vfs_volume_mounted),
			  obj);
	g_signal_connect (G_OBJECT(monitor), "volume-unmounted",
			  G_CALLBACK(on_vfs_volume_unmounted),
			  obj);
}

static void
account_list_free (GSList *accounts)
{
	GSList *cursor = accounts;

	while (cursor) {
		if (G_IS_OBJECT(cursor->data)) { /* check twice... */
			const gchar *id = tny_account_get_id(TNY_ACCOUNT(cursor->data));
			modest_runtime_verify_object_last_ref(cursor->data,id);
		}			
		g_object_unref (G_OBJECT(cursor->data));
		cursor = cursor->next;
	}
	g_slist_free (accounts);
}

static void
recreate_all_accounts (ModestTnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv = 
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (priv->store_accounts_outboxes) {
		account_list_free (priv->store_accounts_outboxes);
		priv->store_accounts_outboxes = NULL;
	}
			
	if (priv->store_accounts) {
		account_list_free (priv->store_accounts);
		priv->store_accounts = NULL;
		get_server_accounts (TNY_ACCOUNT_STORE(self),
					     NULL, TNY_ACCOUNT_TYPE_STORE);
	}
	
	if (priv->transport_accounts) {
		account_list_free (priv->transport_accounts);
		priv->transport_accounts = NULL;
		get_server_accounts (TNY_ACCOUNT_STORE(self), NULL,
					     TNY_ACCOUNT_TYPE_TRANSPORT);
	}
}

static void
on_vfs_volume_mounted(GnomeVFSVolumeMonitor *volume_monitor, 
	GnomeVFSVolume *volume, gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	
	/* Check whether this was the external MMC1 card: */
	gchar *uri = gnome_vfs_volume_get_activation_uri (volume);	
	if (uri && (strcmp (uri, MODEST_MCC1_VOLUMEPATH_URI) == 0)) {
		printf ("DEBUG: %s: MMC1 card mounted.\n", __FUNCTION__);
		
		/* TODO: Just add an account and emit (and respond to) 
		 * TnyAccountStore::accountinserted signal?
		 */
		recreate_all_accounts (self);
		
		g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
			       NULL);
	}
	
	g_free (uri);
}

static void
on_vfs_volume_unmounted(GnomeVFSVolumeMonitor *volume_monitor, 
	GnomeVFSVolume *volume, gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	
	/* Check whether this was the external MMC1 card: */
	gchar *uri = gnome_vfs_volume_get_activation_uri (volume);
	if (uri && (strcmp (uri, MODEST_MCC1_VOLUMEPATH_URI) == 0)) {
		printf ("DEBUG: %s: MMC1 card unmounted.\n", __FUNCTION__);
		
		/* TODO: Just add an account and emit (and respond to) 
		 * TnyAccountStore::accountinserted signal?
		 */
		recreate_all_accounts (self);
		
		g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
			       NULL);
	}
	
	g_free (uri);
}

static void
on_account_removed (ModestAccountMgr *acc_mgr, 
		    const gchar *account,
		    gboolean server_account,
		    gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	TnyAccount *store_account;
	
	/* Clear the account cache */
	store_account = modest_tny_account_store_get_tny_account_by  (self, 
								      MODEST_TNY_ACCOUNT_STORE_QUERY_NAME, 
								      account);
	if (store_account) {
		tny_store_account_delete_cache (TNY_STORE_ACCOUNT (store_account));
		
		g_signal_emit (G_OBJECT (self), 
					 tny_account_store_signals [TNY_ACCOUNT_STORE_ACCOUNT_REMOVED], 
					 0, store_account);

		g_object_unref (store_account);
	} else
		g_printerr ("modest: cannot find server account for %s", account);
	
	/* FIXME: make this more finegrained; changes do not
	 * really affect _all_ accounts, and some do not
	 * affect tny accounts at all (such as 'last_update')
	 */
	recreate_all_accounts (self);

	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
		       account);
}

static void
on_account_changed (ModestAccountMgr *acc_mgr, const gchar *account,
		    const GSList *keys, gboolean server_account, gpointer user_data)

{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	
	/* Ignore the change if it's a change in the last_updated value */
	if (g_slist_length ((GSList *)keys) == 1 &&
	    g_str_has_suffix ((const gchar *) keys->data, MODEST_ACCOUNT_LAST_UPDATED))
		return;

	/* FIXME: make this more finegrained; changes do not really affect _all_
	 * accounts
	 */
	recreate_all_accounts (self);

	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
		       account);
}


static ModestTnyAccountStore*
get_account_store_for_account (TnyAccount *account)
{
	return MODEST_TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(account),
							   "account_store"));
}

/* This callback will be called by Tinymail when it needs the password
 * from the user, for instance if the password was not remembered.
 * It also calls forget_password() before calling this,
 * so that we clear wrong passwords out of our account settings.
 * Note that TnyAccount here will be the server account. */
static gchar*
get_password (TnyAccount *account, const gchar * prompt_not_used, gboolean *cancel)
{
	/* Initialize the output parameter: */
	if (cancel)
	  *cancel = FALSE;
	  
	const gchar *key;
	const TnyAccountStore *account_store;
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	gchar *username = NULL;
	gchar *pwd = NULL;
	gpointer pwd_ptr;
	gboolean already_asked;
	
	key           = tny_account_get_id (account);
	account_store = TNY_ACCOUNT_STORE(get_account_store_for_account (account));
	
	self = MODEST_TNY_ACCOUNT_STORE (account_store);
        priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* This hash map stores passwords, including passwords that are not stored in gconf. */
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
						      key, MODEST_ACCOUNT_PASSWORD, TRUE);
		g_hash_table_insert (priv->password_hash, g_strdup (key), g_strdup (pwd));
	}

	/* if it was already asked, it must have been wrong, so ask again */
	/* TODO: However, when we supply a wrong password to tinymail, 
	 * it seems to (at least sometimes) call our alert_func() instead of 
	 * asking for the password again.
	 */
	if (already_asked || !pwd || strlen(pwd) == 0) {
		/* we don't have it yet. Get the password from the user */
		const gchar* account_id = tny_account_get_id (account);
		gboolean remember = FALSE;
		pwd = NULL;
		
		g_signal_emit (G_OBJECT(self), signals[PASSWORD_REQUESTED_SIGNAL], 0,
			       account_id, /* server_account_name */
			       &username, &pwd, cancel, &remember);
		
		if (!*cancel) {
			/* The password will be returned as the result,
			 * but we need to tell tinymail about the username too: */
			/* TODO: This causes a crash because it frees memory that 
			 * tinymail is already using. tinymail needs to detect the change 
			 * and stop using the old username: 
			 * tny_account_set_user (account, username); */
			
			if (remember) {
				printf ("%s: Storing username=%s, password=%s\n", 
					__FUNCTION__, username, pwd);
				modest_account_mgr_set_string (priv->account_mgr,key,
							       MODEST_ACCOUNT_USERNAME,
							       username, TRUE);
				modest_account_mgr_set_string (priv->account_mgr,key,
							       MODEST_ACCOUNT_PASSWORD,
							       pwd, TRUE);
			}
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

		g_free (username);
		username = NULL;
	} else
		*cancel = FALSE;
 
    /* printf("  DEBUG: %s: returning %s\n", __FUNCTION__, pwd); */
	
	return pwd;
}

/* tinymail calls this if the connection failed due to an incorrect password.
 * And it seems to call this for any general connection failure. */
static void
forget_password (TnyAccount *account)
{
	printf ("DEBUG: %s\n", __FUNCTION__);
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
	
	//gboolean debug = modest_runtime_get_debug_flags() & MODEST_RUNTIME_DEBUG_DEBUG_OBJECTS;

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

	priv->account_mgr = g_object_ref (G_OBJECT(account_mgr));
	priv->device = g_object_ref (device);
	
	priv->session = tny_session_camel_new (TNY_ACCOUNT_STORE(obj));
	
	tny_session_camel_set_ui_locker (priv->session,	 tny_gtk_lockable_new ());
	/* FIXME: unref this in the end? */
	tny_session_camel_set_async_connecting (priv->session, TRUE);
	
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

/** Fill the TnyList from the appropriate cached GSList of accounts. */
static void
get_cached_accounts (TnyAccountStore *self, TnyList *list, TnyAccountType type)
{
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts, *cursor;
	
	priv     = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	accounts = (type == TNY_ACCOUNT_TYPE_STORE ? priv->store_accounts : priv->transport_accounts);

	cursor = accounts;
	while (cursor) {
		if (cursor->data) {
			GObject *object = G_OBJECT(cursor->data);
			tny_list_prepend (list, object);
		}
			
		cursor = cursor->next;
	}
}

static void
create_per_account_local_outbox_folders (TnyAccountStore *self)
{
	g_return_if_fail (self);
	
	ModestTnyAccountStorePrivate *priv = 
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* printf("DEBUG: %s: priv->store_accounts_outboxes = %p\n", __FUNCTION__, priv->store_accounts_outboxes); */
	
	GSList *accounts = NULL;
	
	GSList *account_names  = modest_account_mgr_account_names (priv->account_mgr, 
		TRUE /* including disabled accounts */);
	
	GSList *iter = NULL;
	for (iter = account_names; iter; iter = g_slist_next (iter)) {
		
		const gchar* account_name = (const gchar*)iter->data;
		
		/* Create a per-account local outbox folder (a _store_ account) 
		 * for each _transport_ account: */
		TnyAccount *tny_account_outbox =
			modest_tny_account_new_for_per_account_local_outbox_folder (
				priv->account_mgr, account_name, priv->session);
				
		accounts = g_slist_append (accounts, tny_account_outbox); /* cache it */
	};

	modest_account_mgr_free_account_names (account_names);
	account_names = NULL;
	
	priv->store_accounts_outboxes = accounts;
}

/* This function fills the TnyList, and also stores a GSList of the accounts,
 * for caching purposes. It creates the TnyAccount objects if necessary.
 * The @list parameter may be NULL, if you just want to fill the cache.
 */
static void
get_server_accounts  (TnyAccountStore *self, TnyList *list, TnyAccountType type)
{
	g_return_if_fail (self);
		
	ModestTnyAccountStorePrivate *priv = 
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
		
	/* Do nothing if the accounts are already cached: */
	if (type == TNY_ACCOUNT_TYPE_STORE) {
		if (priv->store_accounts)
			return;
	} else if (type == TNY_ACCOUNT_TYPE_TRANSPORT) {
		if (priv->transport_accounts)
			return;
	}
	
	GSList *account_names = NULL, *cursor = NULL;
	GSList *accounts = NULL;

	/* These are account names, not server_account names */
	account_names = modest_account_mgr_account_names (priv->account_mgr,FALSE);
		
	for (cursor = account_names; cursor; cursor = cursor->next) {
		
		gchar *account_name = (gchar*)cursor->data;
		
		/* we get the server_accounts for enabled accounts */
		if (modest_account_mgr_get_enabled(priv->account_mgr, account_name)) {
				
			/* Add the account: */
			TnyAccount *tny_account = 
				modest_tny_account_new_from_account (priv->account_mgr,
								     account_name,
								     type, priv->session,
								     get_password,
								     forget_password);
			if (tny_account) {
				g_object_set_data (G_OBJECT(tny_account), "account_store",
						   (gpointer)self);
				if (list)
					tny_list_prepend (list, G_OBJECT(tny_account));
				
				accounts = g_slist_append (accounts, tny_account); /* cache it */		
			} else
				g_printerr ("modest: failed to create account for %s\n",
					    account_name);
			}
	}
	
	if (type == TNY_ACCOUNT_TYPE_STORE) {		
		/* Also add the Memory card account if it is mounted: */
		gboolean mmc_is_mounted = FALSE;
		GnomeVFSVolumeMonitor* monitor = 
			gnome_vfs_get_volume_monitor();
		GList* list_volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);
		GList *iter = list_volumes;
		while (iter) {
			GnomeVFSVolume *volume = (GnomeVFSVolume*)iter->data;
			if (volume) {
				if (!mmc_is_mounted) {
					gchar *uri = gnome_vfs_volume_get_activation_uri (volume);
					if (uri && (strcmp (uri, MODEST_MCC1_VOLUMEPATH_URI) == 0)) {
						mmc_is_mounted = TRUE;
					}
					g_free (uri);
				}
				
				gnome_vfs_volume_unref(volume);
			}
			
			iter = g_list_next (iter);
		}
		g_list_free (list_volumes);
		
		if (mmc_is_mounted) {
			TnyAccount *tny_account =
				modest_tny_account_new_for_local_folders (priv->account_mgr, 
					priv->session, MODEST_MCC1_VOLUMEPATH);
			if (list)
				tny_list_prepend (list, G_OBJECT(tny_account));
			accounts = g_slist_append (accounts, tny_account); /* cache it */
		}
	}

	/* And add the connection-specific transport accounts, if any.
	 * Note that these server account instances might never be used 
	 * if their connections are never active: */
	/* Look at each modest account: */
	if (type == TNY_ACCOUNT_TYPE_TRANSPORT) {
		GSList *iter_account_names = account_names;
		while (iter_account_names) {
			const gchar* account_name = (const gchar*)(iter_account_names->data);
			GSList *list_specifics = modest_account_mgr_get_list (priv->account_mgr,
				account_name, 
				MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST,
				MODEST_CONF_VALUE_STRING, FALSE);
				
			/* Look at each connection-specific transport account for the 
			 * modest account: */
			GSList *iter = list_specifics;
			while (iter) {
				/* const gchar* this_connection_name = (const gchar*)(iter->data); */
				iter = g_slist_next (iter);
				if (iter) {
					const gchar* transport_account_name = (const gchar*)(iter->data);
					if (transport_account_name) {
						TnyAccount * tny_account = NULL;
						/* Add the account: */
						tny_account = modest_tny_account_new_from_server_account_name (
							priv->account_mgr, transport_account_name);
						if (tny_account) {
							g_object_set_data (G_OBJECT(tny_account), "account_store",
									   (gpointer)self);
							if (list)
								tny_list_prepend (list, G_OBJECT(tny_account));
							
							accounts = g_slist_append (accounts, tny_account); /* cache it */		
						} else
							g_printerr ("modest: failed to create smtp-specific account for %s\n",
								    transport_account_name);
					}
				}
				
				iter = g_slist_next (iter);
			}
			
			iter_account_names = g_slist_next (iter_account_names);
		}		
	}

	/* free the account_names */
	modest_account_mgr_free_account_names (account_names);
	account_names = NULL;

	/* We also create a per-account local outbox folder (a _store_ account) 
	 * for each _transport_ account. */
	if (type == TNY_ACCOUNT_TYPE_TRANSPORT) {
		/* Now would be a good time to create the per-account local outbox folder 
		 * _store_ accounts corresponding to each transport account: */
		if (!priv->store_accounts_outboxes) {
			create_per_account_local_outbox_folders	(self);
		}
	}
	
	/* But we only return the per-account local outbox folder when 
	 * _store_ accounts are requested. */
	if (type == TNY_ACCOUNT_TYPE_STORE) {
		/* Create them if necessary, 
		 * (which also requires creating the transport accounts, 
		 * if necessary.) */
		if (!priv->store_accounts_outboxes) {
			create_per_account_local_outbox_folders	(self);
		}
	
		/* Also add the local folder pseudo-account: */
		TnyAccount *tny_account =
			modest_tny_account_new_for_local_folders (priv->account_mgr, 
				priv->session, NULL);
					
		/* Add them to the TnyList: */
		if (priv->store_accounts_outboxes) {
			GSList *iter = NULL;
			for (iter = priv->store_accounts_outboxes; iter; iter = g_slist_next (iter)) {
				TnyAccount *outbox_account = (TnyAccount*)iter->data;
				if (list && outbox_account)
					tny_list_prepend (list,  G_OBJECT(outbox_account));
					
				g_object_ref (outbox_account);
				accounts = g_slist_append (accounts, outbox_account);
			}
		}
		
		/* Add a merged folder, merging all the per-account outbox folders: */
		modest_tny_local_folders_account_add_merged_outbox_folders (
			MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (tny_account), priv->store_accounts_outboxes);
			
		if (priv->store_accounts_outboxes) {
			/* We have finished with this temporary list, so free it: */
			account_list_free (priv->store_accounts_outboxes);
			priv->store_accounts_outboxes = NULL;
		}
		
		if (list)
			tny_list_prepend (list, G_OBJECT(tny_account));
		accounts = g_slist_append (accounts, tny_account); /* cache it */	
	}
		
	if (type == TNY_ACCOUNT_TYPE_STORE) {
			/* Store the cache: */
			priv->store_accounts = accounts;
	} else if (type == TNY_ACCOUNT_TYPE_TRANSPORT) {
			/* Store the cache: */
			priv->transport_accounts = accounts;
	}
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
		modest_tny_account_store_get_accounts (self, list,
						       TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
		modest_tny_account_store_get_accounts (self, list,
						       TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);
		return;
	}
	
	if (request_type == TNY_ACCOUNT_STORE_STORE_ACCOUNTS)  {
		if (!priv->store_accounts)
			get_server_accounts (self, list, TNY_ACCOUNT_TYPE_STORE);
		else
			get_cached_accounts (self, list, TNY_ACCOUNT_TYPE_STORE);
		
	} else if (request_type == TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS) {
		if (!priv->transport_accounts)
			get_server_accounts (self, list, TNY_ACCOUNT_TYPE_TRANSPORT);
		else
			get_cached_accounts (self, list, TNY_ACCOUNT_TYPE_TRANSPORT);
	} else
		g_return_if_reached (); /* incorrect req type */
}


static const gchar*
modest_tny_account_store_get_cache_dir (TnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv;
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (!priv->cache_dir)
		priv->cache_dir = g_build_filename (g_get_home_dir(), 
						    MODEST_DIR, MODEST_CACHE_DIR, NULL);
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


static TnyAccount*
modest_tny_account_store_find_account_by_url (TnyAccountStore *self, const gchar* url_string)
{
	return modest_tny_account_store_get_tny_account_by (MODEST_TNY_ACCOUNT_STORE (self), 
							    MODEST_TNY_ACCOUNT_STORE_QUERY_URL,
							    url_string);
}



static gboolean
modest_tny_account_store_alert (TnyAccountStore *self, TnyAlertType type,
				gboolean question, const GError *error)
{
	/* TODO: It would be nice to know what account caused this error. */
	
	g_return_val_if_fail (error, FALSE);

	if ((error->domain != TNY_ACCOUNT_ERROR) 
		&& (error->domain != TNY_ACCOUNT_STORE_ERROR)) {
		g_warning("%s: Unexpected error domain: != TNY_ACCOUNT_ERROR: %d, message=%s", 
			__FUNCTION__, error->domain, error->message); 
		return FALSE;
	}
	
	/* printf("DEBUG: %s: error->message=%s\n", __FUNCTION__, error->message); */
	

	/* const gchar *prompt = NULL; */
	gchar *prompt = NULL;
	switch (error->code) {
		case TNY_ACCOUNT_ERROR_TRY_CONNECT:
		/* The tinymail camel implementation just sends us this for almost 
		 * everything, so we have to guess at the cause.
		 * It could be a wrong password, or inability to resolve a hostname, 
		 * or lack of network, or incorrect authentication method, or something entirely different: */
		/* TODO: Fix camel to provide specific error codes, and then use the 
		 * specific dialog messages from Chapter 12 of the UI spec.
		 */
		case TNY_ACCOUNT_STORE_ERROR_UNKNOWN_ALERT: 
/* 		    g_debug ("%s: Handling GError domain=%d, code=%d, message=%s",  */
/* 				__FUNCTION__, error->domain, error->code, error->message); */
			
			/* TODO: Remove the internal error message for the real release.
			 * This is just so the testers can give us more information: */
			/* prompt = _("Modest account not yet fully configured."); */
			prompt = g_strdup_printf(
				"%s\n (Internal error message, often very misleading):\n%s", 
				_("Incorrect Account Settings"), 
				error->message);
				
			/* TODO: If we can ever determine that the problem is a wrong password:
			 * In this case, the UI spec wants us to show a banner, and then 
			 * open the Account Settings dialog. */
			/* Note: Sometimes, the get_password() function seems to be called again 
			 * when a password is wrong, but sometimes this alert_func is called. */
			#if 0
			GtkWidget *parent_widget = 
				GTK_WIDGET (
					modest_window_mgr_get_main_window (
						modest_runtime_get_window_mgr ()));
			
			hildon_banner_show_information (
				parent_widget,
				NULL /* icon name */,
				_("mcen_ib_username_pw_incorrect") );
				
			/* Show the Account Settings window: */
			ModestAccountSettingsDialog *dialog = modest_account_settings_dialog_new ();
			/* TODO: Get the account somehow. Maybe tinymail should send it with the signal. */
			const gchar* modest_account_name = 
				modest_tny_account_get_parent_modest_account_name_for_server_account (account);
			g_assert (modest_account_name);
			modest_account_settings_dialog_set_account_name (dialog, 
				modest_account_name);
			
			gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self));
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (GTK_WIDGET (dialog));
			#endif
			 
			break;
			
		//TODO: We have started receiving errors of 
		//domain=TNY_ACCOUNT_ERROR, code=TNY_ACCOUNT_ERROR_TRY_CONNECT, message="Canceled".
		//If this is really a result of us cancelling our own operation then 
		//a) this probably shouldn't be an error, and
		//b) should have its own error code.
		
		default:
			g_warning ("%s: Unhandled GError code: %d, message=%s", 
				__FUNCTION__, error->code, error->message);
			prompt = NULL;
		break;
	}
	
	if (!prompt)
		return FALSE;

#ifdef MODEST_PLATFORM_MAEMO
	/* The Tinymail documentation says that we should show Yes and No buttons, 
	 * when it is a question.
	 * Obviously, we need tinymail to use more specific error codes instead,
	 * so we know what buttons to show. */
	 GtkWidget *dialog = NULL;
	 if (question) {
	 	dialog = GTK_WIDGET (hildon_note_new_confirmation (NULL, 
	 		prompt));
	 } else {
	 	dialog = GTK_WIDGET (hildon_note_new_information (NULL, 
	 		prompt));
	 }
#else

	GtkMessageType gtktype = GTK_MESSAGE_ERROR;
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
	
	GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
		gtktype, GTK_BUTTONS_YES_NO, prompt);
#endif /* #ifdef MODEST_PLATFORM_MAEMO */

	gboolean retval = TRUE;
	const int response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (question) {
		retval = (response == GTK_RESPONSE_YES) ||
				 (response == GTK_RESPONSE_OK);	
	}

	gtk_widget_destroy (dialog);
	
	/* TODO: Don't free this when we no longer strdup the message for testers. */
	g_free (prompt);


	/* printf("DEBUG: %s: returning %d\n", __FUNCTION__, retval); */
	return retval;
}


static void
modest_tny_account_store_init (gpointer g, gpointer iface_data)
{
        TnyAccountStoreIface *klass;

	g_return_if_fail (g);

	klass = (TnyAccountStoreIface *)g;

	klass->get_accounts_func =
		modest_tny_account_store_get_accounts;
	klass->get_cache_dir_func =
		modest_tny_account_store_get_cache_dir;
	klass->get_device_func =
		modest_tny_account_store_get_device;
	klass->alert_func =
		modest_tny_account_store_alert;
	klass->find_account_func =
		modest_tny_account_store_find_account_by_url;
}

void
modest_tny_account_store_set_get_pass_func (ModestTnyAccountStore *self,
					    ModestTnyGetPassFunc func)
{
	/* not implemented, we use signals */
	g_printerr ("modest: set_get_pass_func not implemented\n");
}

TnySessionCamel*
modest_tny_account_store_get_session  (TnyAccountStore *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self)->session;
}


TnyAccount*
modest_tny_account_store_get_tny_account_by (ModestTnyAccountStore *self, 
					     ModestTnyAccountStoreQueryType type,
					     const gchar *str)
{
	TnyAccount *account = NULL;
	ModestTnyAccountStorePrivate *priv;	
	GSList *cursor;
	const gchar *val = NULL;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (str, NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* Search in store accounts */
	for (cursor = priv->store_accounts; cursor ; cursor = cursor->next) {
		switch (type) {
		case MODEST_TNY_ACCOUNT_STORE_QUERY_ID:
			val = tny_account_get_id (TNY_ACCOUNT(cursor->data));
			break;
		case MODEST_TNY_ACCOUNT_STORE_QUERY_NAME:
			val = modest_tny_account_get_parent_modest_account_name_for_server_account (TNY_ACCOUNT(cursor->data));
			break;
		case MODEST_TNY_ACCOUNT_STORE_QUERY_URL:
			val = tny_account_get_url_string (TNY_ACCOUNT(cursor->data));
			break;
		}
		
		if (type == MODEST_TNY_ACCOUNT_STORE_QUERY_URL && 
		    tny_account_matches_url_string (TNY_ACCOUNT(cursor->data), val)) {
			account = TNY_ACCOUNT (cursor->data);
			goto end;
		} else {
			if (strcmp (val, str) == 0) {
				account = TNY_ACCOUNT(cursor->data);
				goto end;
			}
		}
	}
		
	/* if we already found something, no need to search the transport accounts */
	for (cursor = priv->transport_accounts; !account && cursor ; cursor = cursor->next) {
		switch (type) {
		case MODEST_TNY_ACCOUNT_STORE_QUERY_ID:
			val = tny_account_get_id (TNY_ACCOUNT(cursor->data));
			break;
		case MODEST_TNY_ACCOUNT_STORE_QUERY_NAME:
			val = tny_account_get_name (TNY_ACCOUNT(cursor->data));
			break;
		case MODEST_TNY_ACCOUNT_STORE_QUERY_URL:
			val = tny_account_get_url_string (TNY_ACCOUNT(cursor->data));
			break;
		}
		
		if (type == MODEST_TNY_ACCOUNT_STORE_QUERY_URL && 
		    tny_account_matches_url_string (TNY_ACCOUNT(cursor->data), val)) {
			account = TNY_ACCOUNT (cursor->data);
			goto end;
		} else {
			if (strcmp (val, str) == 0) {
				account = TNY_ACCOUNT(cursor->data);
				goto end;
			}
		}
	}
 end:
	if (account)
		g_object_ref (G_OBJECT(account));
	
	return account;
}

TnyAccount*
modest_tny_account_store_get_server_account (ModestTnyAccountStore *self,
						     const gchar *account_name,
						     TnyAccountType type)
{
	TnyAccount *account = NULL;
	gchar *id = NULL;
	ModestTnyAccountStorePrivate *priv;	

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_name, NULL);
	g_return_val_if_fail (type == TNY_ACCOUNT_TYPE_STORE || type == TNY_ACCOUNT_TYPE_TRANSPORT,
			      NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* Special case for the local account */
	if (!strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID)) {
		if(type == TNY_ACCOUNT_TYPE_STORE)
			id = g_strdup (MODEST_LOCAL_FOLDERS_ACCOUNT_ID);
		else {
			/* The local folders modest account has no transport server account. */
			return NULL;
		}
	} else {
		ModestAccountData *account_data;
		account_data = modest_account_mgr_get_account_data (priv->account_mgr, account_name);
		if (!account_data) {
			g_printerr ("modest: %s: cannot get account data for account '%s'\n", __FUNCTION__,
				    account_name);
			return NULL;
		}

		if (type == TNY_ACCOUNT_TYPE_STORE && account_data->store_account)
			id = g_strdup (account_data->store_account->account_name);
		else if (account_data->transport_account)
			id = g_strdup (account_data->transport_account->account_name);

		modest_account_mgr_free_account_data (priv->account_mgr, account_data);
	}

	if (!id)
		g_printerr ("modest: could not get an id for account %s\n",
			    account_name);
	else 	
		account = modest_tny_account_store_get_tny_account_by (self, MODEST_TNY_ACCOUNT_STORE_QUERY_ID, id);

	if (!account)
		g_printerr ("modest: could not get tny %s account for %s (id=%s)\n",
			    type == TNY_ACCOUNT_TYPE_STORE ? "store" : "transport",
			    account_name, id ? id : "<none>");

	return account;	
}

static TnyAccount*
get_smtp_specific_transport_account_for_open_connection (ModestTnyAccountStore *self,
							 const gchar *account_name)
{
	/* Get the current connection: */
	TnyDevice *device = modest_runtime_get_device ();
	
	if (!tny_device_is_online (device))
		return NULL;

#ifdef MODEST_PLATFORM_MAEMO
	g_assert (TNY_IS_MAEMO_CONIC_DEVICE (device));
	TnyMaemoConicDevice *maemo_device = TNY_MAEMO_CONIC_DEVICE (device);	
	const gchar* iap_id = tny_maemo_conic_device_get_current_iap_id (maemo_device);
	/* printf ("DEBUG: %s: iap_id=%s\n", __FUNCTION__, iap_id); */
	if (!iap_id)
		return NULL;
		
	ConIcIap* connection = tny_maemo_conic_device_get_iap (maemo_device, iap_id);
	if (!connection)
		return NULL;
		
	const gchar *connection_name = con_ic_iap_get_name (connection);
	/* printf ("DEBUG: %s: connection_name=%s\n", __FUNCTION__, connection_name); */
	if (!connection_name)
		return NULL;
	
	/*  Get the connection-specific transport acccount, if any: */
	ModestAccountMgr *account_manager = modest_runtime_get_account_mgr ();
	gchar* server_account_name = modest_account_mgr_get_connection_specific_smtp (account_manager, 
		account_name, connection_name);

	/* printf ("DEBUG: %s: server_account_name=%s\n", __FUNCTION__, server_account_name); */
	if (!server_account_name) {
		return NULL; /* No connection-specific SMTP server was specified for this connection. */
	}
		
	TnyAccount* account = modest_tny_account_store_get_tny_account_by (self, 
									   MODEST_TNY_ACCOUNT_STORE_QUERY_ID, 
									   server_account_name);

	/* printf ("DEBUG: %s: account=%p\n", __FUNCTION__, account); */
	g_free (server_account_name);	

	/* Unref the get()ed object, as required by the tny_maemo_conic_device_get_iap() documentation. */
	g_object_unref (connection);
	
	return account;
#else
	return NULL; /* TODO: Implement this for GNOME, instead of just Maemo? */
#endif /* MODEST_PLATFORM_MAEMO */
}

								 
TnyAccount*
modest_tny_account_store_get_transport_account_for_open_connection (ModestTnyAccountStore *self,
								    const gchar *account_name)
{
	/*  Get the connection-specific transport acccount, if any: */
	TnyAccount *account = get_smtp_specific_transport_account_for_open_connection (self, account_name);
			
	/* If there is no connection-specific transport account (the common case), 
	 * just get the regular transport account: */
	if (!account) {
		/* printf("DEBUG: %s: using regular transport account for account %s.\n", __FUNCTION__, account_name); */

		/* The special local folders don't have transport accounts. */
		if (strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) == 0)
			account = NULL;
		else
			account = modest_tny_account_store_get_server_account (self, account_name, 
						     TNY_ACCOUNT_TYPE_TRANSPORT);
	}
			     
	return account;
}

gboolean
modest_tny_account_is_virtual_local_folders (TnyAccount *self)
{
	/* We should make this more sophisticated if we ever use ModestTnyLocalFoldersAccount 
	 * for anything else. */
	return MODEST_IS_TNY_LOCAL_FOLDERS_ACCOUNT (self);
}

TnyAccount*
modest_tny_account_store_get_local_folders_account (TnyAccountStore *self)
{
	TnyAccount *account = NULL;
	ModestTnyAccountStorePrivate *priv;	
	GSList *cursor;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	for (cursor = priv->store_accounts; cursor ; cursor = cursor->next) {
		TnyAccount *this_account = TNY_ACCOUNT(cursor->data);
		if (modest_tny_account_is_virtual_local_folders (this_account)) {
				 account = this_account;
				 break;
		}
	}

	if (account)
		g_object_ref (G_OBJECT(account));
	
	return account;
}

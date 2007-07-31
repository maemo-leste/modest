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
#include <maemo/modest-maemo-utils.h>


#include "modest-tny-account-store.h"
#include "modest-tny-platform-factory.h"
#include <tny-gtk-lockable.h>
#include <camel/camel.h>

#ifdef MODEST_PLATFORM_MAEMO
#include <tny-maemo-conic-device.h>
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-banner.h>
#else
#include <hildon/hildon-note.h>
#include <hildon/hildon-banner.h>
#endif
#endif

#include <libgnomevfs/gnome-vfs-volume-monitor.h>

/* 'private'/'protected' functions */
static void    modest_tny_account_store_class_init   (ModestTnyAccountStoreClass *klass);

static void    modest_tny_account_store_finalize     (GObject *obj);

static void    modest_tny_account_store_instance_init (ModestTnyAccountStore *obj);

static void    modest_tny_account_store_init          (gpointer g, gpointer iface_data);

static void    modest_tny_account_store_base_init     (gpointer g_class);

static void    on_account_inserted         (ModestAccountMgr *acc_mgr, 
					    const gchar *account,
					    gpointer user_data);

static void    add_existing_accounts       (ModestTnyAccountStore *self);

static void    insert_account              (ModestTnyAccountStore *self,
					    const gchar *account,
					    gboolean notify);

static void    on_account_removed          (ModestAccountMgr *acc_mgr, 
					    const gchar *account,
					    gpointer user_data);

static gchar*  get_password                (TnyAccount *account, 
					    const gchar * prompt_not_used, 
					    gboolean *cancel);

static void    forget_password             (TnyAccount *account);

static void    on_vfs_volume_mounted       (GnomeVFSVolumeMonitor *volume_monitor, 
					    GnomeVFSVolume *volume, 
					    gpointer user_data);

static void    on_vfs_volume_unmounted     (GnomeVFSVolumeMonitor *volume_monitor, 
					    GnomeVFSVolume *volume, 
					    gpointer user_data);

static void    modest_tny_account_store_forget_password_in_memory (ModestTnyAccountStore *self, 
								   const gchar *server_account_name);

static void    add_connection_specific_transport_accounts         (ModestTnyAccountStore *self,
								   const gchar *account_name);

/* list my signals */
enum {
	ACCOUNT_CHANGED_SIGNAL,
	ACCOUNT_INSERTED_SIGNAL,
	ACCOUNT_REMOVED_SIGNAL,

	PASSWORD_REQUESTED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {
	gchar              *cache_dir;	
	GHashTable         *password_hash;
	GHashTable         *account_settings_dialog_hash;
	ModestAccountMgr   *account_mgr;
	TnySessionCamel    *session;
	TnyDevice          *device;

	gulong acc_inserted_handler;
	gulong acc_changed_handler;
	gulong acc_removed_handler;
	gulong volume_mounted_handler;
	gulong volume_unmounted_handler;
	
	/* We cache the lists of accounts here */
	TnyList             *store_accounts;
	TnyList             *transport_accounts;
	TnyList             *store_accounts_outboxes;
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
			modest_tny_account_store_base_init,	/* base init */
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
modest_tny_account_store_base_init (gpointer g_class)
{
	static gboolean tny_account_store_initialized = FALSE;

	if (!tny_account_store_initialized) {

		signals[ACCOUNT_CHANGED_SIGNAL] =
			g_signal_new ("account_changed",
				      MODEST_TYPE_TNY_ACCOUNT_STORE,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (ModestTnyAccountStoreClass, account_changed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__OBJECT,
				      G_TYPE_NONE, 1, TNY_TYPE_ACCOUNT);

		signals[ACCOUNT_INSERTED_SIGNAL] =
			g_signal_new ("account_inserted",
				      MODEST_TYPE_TNY_ACCOUNT_STORE,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (ModestTnyAccountStoreClass, account_inserted),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__OBJECT,
				      G_TYPE_NONE, 1, TNY_TYPE_ACCOUNT);
		
		signals[ACCOUNT_REMOVED_SIGNAL] =
			g_signal_new ("account_removed",
				      MODEST_TYPE_TNY_ACCOUNT_STORE,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET (ModestTnyAccountStoreClass, account_removed),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__OBJECT,
				      G_TYPE_NONE, 1, TNY_TYPE_ACCOUNT);
		
/* 		signals[TNY_ACCOUNT_STORE_CONNECTING_FINISHED] = */
/* 			g_signal_new ("connecting_finished", */
/* 				      TNY_TYPE_ACCOUNT_STORE, */
/* 				      G_SIGNAL_RUN_FIRST, */
/* 				      G_STRUCT_OFFSET (TnyAccountStoreIface, connecting_finished), */
/* 				      NULL, NULL, */
/* 				      g_cclosure_marshal_VOID__VOID,  */
/* 				      G_TYPE_NONE, 0); */

		signals[PASSWORD_REQUESTED_SIGNAL] =
			g_signal_new ("password_requested",
				      MODEST_TYPE_TNY_ACCOUNT_STORE,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestTnyAccountStoreClass, password_requested),
				      NULL, NULL,
				      modest_marshal_VOID__STRING_POINTER_POINTER_POINTER_POINTER,
				      G_TYPE_NONE, 5, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
				      G_TYPE_POINTER);		

		tny_account_store_initialized = TRUE;
	}
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
}
     
static void
modest_tny_account_store_instance_init (ModestTnyAccountStore *obj)
{
	GnomeVFSVolumeMonitor* monitor = NULL;
	ModestTnyAccountStorePrivate *priv;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->cache_dir              = NULL;
	priv->account_mgr            = NULL;
	priv->session                = NULL;
	priv->device                 = NULL;
	
	/* An in-memory store of passwords, 
	 * for passwords that are not remembered in the configuration,
         * so they need to be asked for from the user once in each session:
         */
	priv->password_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						     g_free, g_free);
							     
	/* A hash-map of modest account names to dialog pointers,
	 * so we can avoid showing the account settings twice for the same modest account: */				      
	priv->account_settings_dialog_hash = g_hash_table_new_full (g_str_hash, g_str_equal, 
								    g_free, NULL);
							      
	/* Respond to volume mounts and unmounts, such 
	 * as the insertion/removal of the memory card: */
	monitor = gnome_vfs_get_volume_monitor();

	priv->volume_mounted_handler = g_signal_connect (G_OBJECT(monitor), 
							 "volume-mounted",
							 G_CALLBACK(on_vfs_volume_mounted),
							 obj);

	priv->volume_unmounted_handler = g_signal_connect (G_OBJECT(monitor), "volume-unmounted",
							   G_CALLBACK(on_vfs_volume_unmounted),
							   obj);
}

/* disconnect the list of TnyAccounts */
static void
foreach_account_disconnect (gpointer data, 
			    gpointer user_data)
{
	tny_camel_account_set_online (TNY_CAMEL_ACCOUNT(data), FALSE, NULL);
}


static void
foreach_account_append_to_list (gpointer data, 
				gpointer user_data)
{
	TnyList *list;

	list = TNY_LIST (user_data);
	tny_list_append (list, G_OBJECT (data));
}

/********************************************************************/
/*           Control the state of the MMC local account             */
/********************************************************************/
static void
on_vfs_volume_mounted(GnomeVFSVolumeMonitor *volume_monitor, 
		      GnomeVFSVolume *volume, 
		      gpointer user_data)
{
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
 
	gchar *uri = NULL;

	self = MODEST_TNY_ACCOUNT_STORE(user_data);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* Check whether this was the external MMC1 card: */
	uri = gnome_vfs_volume_get_activation_uri (volume);

	if (uri && (!strcmp (uri, MODEST_MCC1_VOLUMEPATH_URI))) {
		TnyAccount *mmc_account;

		mmc_account = modest_tny_account_new_for_local_folders (priv->account_mgr, 
									priv->session, 
									MODEST_MCC1_VOLUMEPATH);

		/* Add to the list of store accounts */
		tny_list_append (priv->store_accounts, G_OBJECT (mmc_account));

		g_signal_emit (G_OBJECT (self), 
			       signals [ACCOUNT_INSERTED_SIGNAL],
			       0, mmc_account);
		/* Free */
		g_object_unref (mmc_account);
	}
	g_free (uri);
}

static void
on_vfs_volume_unmounted(GnomeVFSVolumeMonitor *volume_monitor, 
			GnomeVFSVolume *volume, 
			gpointer user_data)
{
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	gchar *uri = NULL;

	self = MODEST_TNY_ACCOUNT_STORE(user_data);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* Check whether this was the external MMC1 card: */
	uri = gnome_vfs_volume_get_activation_uri (volume);
	if (uri && (strcmp (uri, MODEST_MCC1_VOLUMEPATH_URI) == 0)) {
		TnyAccount *mmc_account = NULL;
		gboolean found = FALSE;
		TnyIterator *iter = NULL;

		iter = tny_list_create_iterator (priv->store_accounts);
		while (!tny_iterator_is_done (iter) && !found) {
			TnyAccount *account;

			account = TNY_ACCOUNT (tny_iterator_get_current (iter));
			if (modest_tny_account_is_memory_card_account (account)) {
				found = TRUE;
				mmc_account = g_object_ref (account);
			}
			g_object_unref (account);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);

		if (found) {
			/* Remove from the list */
			tny_list_remove (priv->store_accounts, G_OBJECT (mmc_account));
		       
	/* Make sure that the account is reviewed from the View menu, etc: */
	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_UPDATE_SIGNAL], 0,
			       NULL);

			/* Notify observers */
			g_signal_emit (G_OBJECT (self),
				       signals [ACCOUNT_REMOVED_SIGNAL],
				       0, mmc_account);

			g_object_unref (mmc_account);
		} else {
			g_warning ("%s: there was no store account for the unmounted MMC",
				   __FUNCTION__);
		}
	}
	g_free (uri);
}

/**
 * modest_tny_account_store_forget_password_in_memory
 * @self: a TnyAccountStore instance
 * @account: A server account.
 * 
 * Forget any password stored in memory for this account.
 * For instance, this should be called when the user has changed the password in the account settings.
 */
static void
modest_tny_account_store_forget_password_in_memory (ModestTnyAccountStore *self, const gchar * server_account_name)
{
	/* printf ("DEBUG: %s\n", __FUNCTION__); */
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (server_account_name && priv->password_hash) {
		g_hash_table_remove (priv->password_hash, server_account_name);
	}
}

static void
on_account_changed (ModestAccountMgr *acc_mgr, 
		    const gchar *account,
		    const GSList *keys, 
		    gboolean server_account, 
		    gpointer user_data)
{
	printf ("DEBUG: modest: %s\n", __FUNCTION__);
	
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	
	/*
	printf ("DEBUG: %s\n", __FUNCTION__);
	const GSList *iter = keys;
	for (iter = keys; iter; iter = g_slist_next (iter)) {
		printf ("  DEBUG: %s: key=%s\n", __FUNCTION__, (const gchar*)iter->data);
	}
	*/
	
	
	/* Ignore the change if it's a change in the last_updated value */
	if (g_slist_length ((GSList *)keys) == 1 &&
		g_str_has_suffix ((const gchar *) keys->data, MODEST_ACCOUNT_LAST_UPDATED)) {
		return;
	}

	/* FIXME: make this more finegrained; changes do not really affect _all_
	 * accounts
	 */
/* 	recreate_all_accounts (self); */
	
	/* TODO: This doesn't actually work, because
	 * a) The account name is not sent correctly per key:
	 * b) We should test the end of the key, not the whole keym
	 * c) We don't seem to be getting all keys here.
	 * Instead, we just forget the password for all accounts when we create them, for now.
	 */
	#if 0
	/* If a password has changed, then forget the previously cached password for this account: */
	if (server_account && keys && g_slist_find_custom ((GSList *)keys, MODEST_ACCOUNT_PASSWORD, (GCompareFunc)strcmp)) {
		printf ("DEBUG: %s: Forgetting cached password for account ID=%s\n", __FUNCTION__, account);
		modest_tny_account_store_forget_password_in_memory (self,  account);
	}
	#endif

	g_signal_emit (G_OBJECT(self), signals[ACCOUNT_CHANGED_SIGNAL], 0, account);
}

static void 
on_account_settings_hide (GtkWidget *widget, gpointer user_data)
{
	TnyAccount *account = (TnyAccount*)user_data;
	
	/* This is easier than using a struct for the user_data: */
	ModestTnyAccountStore *self = modest_runtime_get_account_store();
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	const gchar *modest_account_name = 
			modest_tny_account_get_parent_modest_account_name_for_server_account (account);
	if (modest_account_name)
		g_hash_table_remove (priv->account_settings_dialog_hash, modest_account_name);
}

static gboolean 
on_idle_wrong_password_warning_only (gpointer user_data)
{
	gdk_threads_enter();
	
	ModestWindow *main_window = 
				modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ());
				
	/* Show an explanatory temporary banner: */
	hildon_banner_show_information ( 
		GTK_WIDGET(main_window), NULL, _("mcen_ib_username_pw_incorrect"));
		
	gdk_threads_leave();
	
	return FALSE; /* Don't show again. */
}
		
static gboolean 
on_idle_wrong_password (gpointer user_data)
{ 
	TnyAccount *account = (TnyAccount*)user_data;
	/* This is easier than using a struct for the user_data: */
	ModestTnyAccountStore *self = modest_runtime_get_account_store();
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	const gchar *modest_account_name = 
			modest_tny_account_get_parent_modest_account_name_for_server_account (account);
	if (!modest_account_name) {
		g_warning ("%s: modest_tny_account_get_parent_modest_account_name_for_server_account() failed.\n", 
			__FUNCTION__);
			
		g_object_unref (account);
		return FALSE;
	}
	
	
	/* Check whether this window is already open,
	 * for instance because of a previous get_password() call: 
	 */
	gpointer dialog_as_gpointer = NULL;
	gboolean found = FALSE;
	if (priv->account_settings_dialog_hash) {
		found = g_hash_table_lookup_extended (priv->account_settings_dialog_hash,
			modest_account_name, NULL, (gpointer*)&dialog_as_gpointer);
	}
	ModestAccountSettingsDialog *dialog = dialog_as_gpointer;
					
	ModestWindow *main_window = 
				modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ());

	gdk_threads_enter ();
	gboolean created_dialog = FALSE;
	if (!found || !dialog) {
		dialog = modest_account_settings_dialog_new ();
		modest_account_settings_dialog_set_account_name (dialog, modest_account_name);
		modest_account_settings_dialog_switch_to_user_info (dialog);
		
		g_hash_table_insert (priv->account_settings_dialog_hash, g_strdup (modest_account_name), dialog);
		
		created_dialog = TRUE;
	}
	
	/* Show an explanatory temporary banner: */
	hildon_banner_show_information ( 
		GTK_WIDGET(dialog), NULL, _("mcen_ib_username_pw_incorrect"));
		
	if (created_dialog) {
		/* Forget it when it closes: */
		g_signal_connect_object (G_OBJECT (dialog), "hide", G_CALLBACK (on_account_settings_hide), 
			account, 0);
			
		/* Show it and delete it when it closes: */
		modest_maemo_show_dialog_and_forget (GTK_WINDOW (main_window), GTK_DIALOG (dialog));
	}
	else {
		/* Just show it instead of showing it and deleting it when it closes,
		 * though it is probably open already: */
		gtk_window_present (GTK_WINDOW (dialog));
	}
	
	g_object_unref (account);
	gdk_threads_leave ();
	
	return FALSE; /* Dont' call this again. */
}

typedef struct 
{
	GMainLoop *loop;
	ModestTnyAccountStore* account_store;
	const gchar* server_account_id;
	gchar **username;
	gchar **password;
	gboolean *cancel;
	gboolean *remember;
} IdlePasswordRequest;

static gboolean 
on_idle_request_password (gpointer user_data)
{
	gdk_threads_enter();
	
	IdlePasswordRequest* info = (IdlePasswordRequest*)user_data;
	g_signal_emit (G_OBJECT(info->account_store), signals[PASSWORD_REQUESTED_SIGNAL], 0,
			       info->server_account_id, /* server_account_name */
			       info->username, info->password, info->cancel, info->remember);
			       
	if (info->loop)
		g_main_loop_quit (info->loop);
	
	gdk_threads_leave();
	
	return FALSE; /* Don't call again. */
}

static void
request_password_in_main_loop_and_wait (ModestTnyAccountStore *account_store, 
					 const gchar* server_account_id,
					 gchar **username,
					 gchar **password,
					 gboolean *cancel, 
					 gboolean *remember)
{
	IdlePasswordRequest *data = g_slice_new0 (IdlePasswordRequest);
	data->account_store = account_store;
	data->server_account_id = server_account_id;
	data->username = username;
	data->password = password;
	data->cancel = cancel;
	data->remember = remember;

	data->loop = g_main_loop_new (NULL, FALSE /* not running */);
	
	/* Cause the function to be run in an idle-handler, which is always 
	 * in the main thread:
	 */
	g_idle_add (&on_idle_request_password, data);
	
	/* This main loop will run until the idle handler has stopped it: */
	printf ("DEBUG: %s: before g_main_loop_run()\n", __FUNCTION__);
	GDK_THREADS_LEAVE();
	g_main_loop_run (data->loop);
	GDK_THREADS_ENTER();
	printf ("DEBUG: %s: after g_main_loop_run()\n", __FUNCTION__);
	printf ("DEBUG: %s: Finished\n", __FUNCTION__);
	g_main_loop_unref (data->loop);

	g_slice_free (IdlePasswordRequest, data);
}

/* This callback will be called by Tinymail when it needs the password
 * from the user or the account settings.
 * It can also call forget_password() before calling this,
 * so that we clear wrong passwords out of our account settings.
 * Note that TnyAccount here will be the server account. */
static gchar*
get_password (TnyAccount *account, const gchar * prompt_not_used, gboolean *cancel)
{
	/* TODO: Settting cancel to FALSE does not actually cancel everything.
	 * We still get multiple requests afterwards, so we end up showing the 
	 * same dialogs repeatedly.
	 */
	 
	printf ("DEBUG: modest: %s: prompt (not shown) = %s\n", __FUNCTION__, prompt_not_used);
	  
	g_return_val_if_fail (account, NULL);
	  
	const TnyAccountStore *account_store = NULL;
	ModestTnyAccountStore *self = NULL;
	ModestTnyAccountStorePrivate *priv;
	gchar *username = NULL;
	gchar *pwd = NULL;
	gpointer pwd_ptr = NULL;
	gboolean already_asked = FALSE;

	/* Initialize the output parameter: */
	if (cancel)
		*cancel = FALSE;
		
	const gchar *server_account_name = tny_account_get_id (account);
	account_store = TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(account),
							     "account_store"));

	if (!server_account_name || !account_store) {
		g_warning ("modest: %s: could not retrieve account_store for account %s",
			   __FUNCTION__, server_account_name ? server_account_name : "<NULL>");
		if (cancel)
			*cancel = TRUE;
		
		return NULL;
	}

	self = MODEST_TNY_ACCOUNT_STORE (account_store);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* This hash map stores passwords, including passwords that are not stored in gconf. */
	/* Is it in the hash? if it's already there, it must be wrong... */
	pwd_ptr = (gpointer)&pwd; /* pwd_ptr so the compiler does not complained about
				   * type-punned ptrs...*/
	already_asked = priv->password_hash && 
				g_hash_table_lookup_extended (priv->password_hash,
						      server_account_name,
						      NULL,
						      (gpointer*)&pwd_ptr);
						      
	printf ("DEBUG: modest: %s: Already asked = %d\n", __FUNCTION__, already_asked);

	/* If the password is not already there, try ModestConf */
	if (!already_asked) {
		pwd  = modest_server_account_get_password (priv->account_mgr,
						      server_account_name);
		g_hash_table_insert (priv->password_hash, g_strdup (server_account_name), g_strdup (pwd));
	}

	/* If it was already asked, it must have been wrong, so ask again */
	if (already_asked || !pwd || strlen(pwd) == 0) {
		/* As per the UI spec, if no password was set in the account settings, 
		 * ask for it now. But if the password is wrong in the account settings, 
		 * then show a banner and the account settings dialog so it can be corrected:
		 */
		const gboolean settings_have_password = 
			modest_server_account_get_has_password (priv->account_mgr, server_account_name);
		printf ("DEBUG: modest: %s: settings_have_password=%d\n", __FUNCTION__, settings_have_password);
		if (settings_have_password) {
	
			
			/* The password must be wrong, so show the account settings dialog so it can be corrected: */
			/* We show it in the main loop, because this function might not be in the main loop. */
			g_object_ref (account); /* unrefed in the idle handler. */
			g_idle_add (on_idle_wrong_password, account);
			
			if (cancel)
				*cancel = TRUE;
				
			return NULL;
		}
	
		/* we don't have it yet. Get the password from the user */
		const gchar* account_id = tny_account_get_id (account);
		gboolean remember = FALSE;
		pwd = NULL;
		
		if (already_asked) {
			/* Show an info banner, before we show the protected password dialog: */
			g_idle_add (on_idle_wrong_password_warning_only, NULL);
		}
		
		request_password_in_main_loop_and_wait (self, account_id, 
			       &username, &pwd, cancel, &remember);
		
		if (!*cancel) {
			/* The password will be returned as the result,
			 * but we need to tell tinymail about the username too: */
			tny_account_set_user (account, username);
			
			/* Do not save the password in gconf, 
			 * because the UI spec says "The password will never be saved in the account": */
			/*
			if (remember) {
				printf ("%s: Storing username=%s, password=%s\n", 
					__FUNCTION__, username, pwd);
				modest_server_account_set_username (priv->account_mgr, server_account_name,
							       username);
				modest_server_account_set_password (priv->account_mgr, server_account_name,
							       pwd);
			}
			*/

			/* We need to dup the string even knowing that
			   it's already a dup of the contents of an
			   entry, because it if it's wrong, then camel
			   will free it */
			g_hash_table_insert (priv->password_hash, g_strdup (server_account_name), g_strdup(pwd));
		} else {
			g_hash_table_remove (priv->password_hash, server_account_name);
			
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
	
        account_store = TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(account),
							     "account_store"));
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
	/*
	modest_account_mgr_unset (priv->account_mgr,
				  key, MODEST_ACCOUNT_PASSWORD, TRUE);
	*/
}

static void
modest_tny_account_store_finalize (GObject *obj)
{
	GnomeVFSVolumeMonitor *volume_monitor;
	ModestTnyAccountStore *self        = MODEST_TNY_ACCOUNT_STORE(obj);
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	g_free (priv->cache_dir);
	priv->cache_dir = NULL;
	
	if (priv->password_hash) {
		g_hash_table_destroy (priv->password_hash);
		priv->password_hash = NULL;
	}

	/* Disconnect VFS signals */
	volume_monitor = gnome_vfs_get_volume_monitor ();
	if (g_signal_handler_is_connected (volume_monitor, 
					   priv->volume_mounted_handler))
		g_signal_handler_disconnect (volume_monitor, 
					     priv->volume_mounted_handler);
	if (g_signal_handler_is_connected (volume_monitor, 
					   priv->volume_unmounted_handler))
		g_signal_handler_disconnect (volume_monitor, 
					     priv->volume_unmounted_handler);

	if (priv->account_mgr) {
		/* Disconnect signals */
		if (g_signal_handler_is_connected (priv->account_mgr, 
						   priv->acc_inserted_handler))
			g_signal_handler_disconnect (priv->account_mgr, 
						     priv->acc_inserted_handler);
		if (g_signal_handler_is_connected (priv->account_mgr, 
						   priv->acc_changed_handler))
			g_signal_handler_disconnect (priv->account_mgr, 
						     priv->acc_changed_handler);
		if (g_signal_handler_is_connected (priv->account_mgr, 
						   priv->acc_removed_handler))
			g_signal_handler_disconnect (priv->account_mgr, 
						     priv->acc_removed_handler);

		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	if (priv->device) {
		g_object_unref (G_OBJECT(priv->device));
		priv->device = NULL;
	}

	/* Destroy all accounts. Disconnect all accounts before they are destroyed */
	if (priv->store_accounts) {
		tny_list_foreach (priv->store_accounts, foreach_account_disconnect, NULL);
		g_object_unref (priv->store_accounts);
		priv->store_accounts = NULL;
	}

	if (priv->transport_accounts) {
		tny_list_foreach (priv->transport_accounts, foreach_account_disconnect, NULL);
		g_object_unref (priv->transport_accounts);
		priv->transport_accounts = NULL;
	}

	if (priv->store_accounts_outboxes) {
		g_object_unref (priv->store_accounts_outboxes);
		priv->store_accounts_outboxes = NULL;
	}
		
	if (priv->session) {
		camel_object_unref (CAMEL_OBJECT(priv->session));
		priv->session = NULL;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestTnyAccountStore*
modest_tny_account_store_new (ModestAccountMgr *account_mgr, 
			      TnyDevice *device) 
{
	GObject *obj;
	ModestTnyAccountStorePrivate *priv;
	TnyAccount *local_account = NULL;
	
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (device, NULL);

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_ACCOUNT_STORE, NULL));
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->account_mgr = g_object_ref (G_OBJECT(account_mgr));
	priv->device = g_object_ref (device);
	
	priv->session = tny_session_camel_new (TNY_ACCOUNT_STORE(obj));
	if (!priv->session) {
		g_warning ("failed to get TnySessionCamel");
		return NULL;
	}

	/* Set the ui locker */	
	tny_session_camel_set_ui_locker (priv->session,	 tny_gtk_lockable_new ());
		
	/* Connect signals */
	priv->acc_inserted_handler = g_signal_connect (G_OBJECT(account_mgr), "account_inserted",
						      G_CALLBACK (on_account_inserted), obj);
	priv->acc_changed_handler = g_signal_connect (G_OBJECT(account_mgr), "account_changed",
						      G_CALLBACK (on_account_changed), obj);
	priv->acc_removed_handler = g_signal_connect (G_OBJECT(account_mgr), "account_removed",
						      G_CALLBACK (on_account_removed), obj);

	/* Create the lists of accounts */
	priv->store_accounts = tny_simple_list_new ();
	priv->transport_accounts = tny_simple_list_new ();
	priv->store_accounts_outboxes = tny_simple_list_new ();

	/* Create the local folders account */
	local_account = 
		modest_tny_account_new_for_local_folders (priv->account_mgr, priv->session, NULL);
	tny_list_append (priv->store_accounts, G_OBJECT(local_account));
	g_object_unref (local_account);	

	/* Add the other remote accounts. Do this before adding the
	   local account, because we need to add our outboxes to the
	   global OUTBOX hosted in the local account */
	add_existing_accounts (MODEST_TNY_ACCOUNT_STORE (obj));

	return MODEST_TNY_ACCOUNT_STORE(obj);
}

static void
modest_tny_account_store_get_accounts  (TnyAccountStore *self, 
					TnyList *list,
					TnyGetAccountsRequestType request_type)
{
	ModestTnyAccountStorePrivate *priv;
	
	g_return_if_fail (self);
	g_return_if_fail (TNY_IS_LIST(list));
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	switch (request_type) {
	case TNY_ACCOUNT_STORE_BOTH:
		tny_list_foreach (priv->store_accounts, foreach_account_append_to_list, list);
		tny_list_foreach (priv->transport_accounts, foreach_account_append_to_list, list);
		break;
	case TNY_ACCOUNT_STORE_STORE_ACCOUNTS:
		tny_list_foreach (priv->store_accounts, foreach_account_append_to_list, list);
		break;
	case TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS:
		tny_list_foreach (priv->transport_accounts, foreach_account_append_to_list, list);
		break;
	default:
		g_return_if_reached ();
	}

	/* Initialize session. Why do we need this ??? */
	tny_session_camel_set_initialized (priv->session);
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
modest_tny_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type,
				gboolean question, const GError *error)
{
	g_return_val_if_fail (error, FALSE);

	if ((error->domain != TNY_ACCOUNT_ERROR) 
		&& (error->domain != TNY_ACCOUNT_STORE_ERROR)) {
		g_warning("modest: %s: Unexpected error domain: != TNY_ACCOUNT_ERROR: %d, message=%s", 
			__FUNCTION__, error->domain, error->message); 
			
		return FALSE;
	}
	
	printf("DEBUG: %s: GError code: %d, message=%s\n", 
				__FUNCTION__, error->code, error->message);
	
	/* Get the server name: */
	const gchar* server_name = NULL;
	if (account && TNY_IS_ACCOUNT (account)) {
		server_name = tny_account_get_hostname (account);
		printf ("modest: %s: account name = %s, server_name=%s\n", __FUNCTION__, 
			tny_account_get_id (account), server_name);
	}
	
	if (!server_name)
		server_name = _("Unknown Server");	
		
	ModestTransportStoreProtocol proto = MODEST_PROTOCOL_STORE_POP; /* Arbitrary default. */
	if (account) {
		const gchar *proto_name = tny_account_get_proto (account);
		if (proto_name)
			proto = modest_protocol_info_get_transport_store_protocol (proto_name);
		else {
			g_warning("modest: %s: account with id=%s has no proto.\n", __FUNCTION__, 
				tny_account_get_id (account));
		}
	}
		
	/* const gchar *prompt = NULL; */
	gchar *prompt = NULL;
	switch (error->code) {
		case TNY_ACCOUNT_STORE_ERROR_CANCEL_ALERT:
		case TNY_ACCOUNT_ERROR_TRY_CONNECT_USER_CANCEL:
			/* Don't show waste the user's time by showing him a dialog telling 
			 * him that he has just cancelled something: */
			g_debug ("%s: Handling GError domain=%d, code=%d (cancelled) without showing a dialog, message=%s", 
 				__FUNCTION__, error->domain, error->code, error->message);
			prompt = NULL;
			break;
			
		case TNY_ACCOUNT_ERROR_TRY_CONNECT_HOST_LOOKUP_FAILED:
			/* TODO: Show the appropriate message, depending on whether it's POP or IMAP: */
			g_debug ("%s: Handling GError domain=%d, code=%d (lookup failed), message=%s", 
 				__FUNCTION__, error->domain, error->code, error->message);
 				
 			switch (proto) {
 				case MODEST_PROTOCOL_STORE_POP:
					prompt = g_strdup_printf (_("emev_ni_ui_pop3_msg_connect_error"), server_name);
					break;
				case MODEST_PROTOCOL_STORE_IMAP:
					prompt = g_strdup_printf (_("emev_ni_ui_imap_connect_server_error"), server_name);
					break;
				case MODEST_PROTOCOL_TRANSPORT_SMTP:
				default: /* Arbitrary default. */
					prompt = g_strdup_printf (_("emev_ib_ui_smtp_server_invalid"), server_name);
					break;
 			}
	
			/*
			prompt = g_strdup_printf(
				_("Incorrect Account Settings:\n Host lookup failed.%s"), 
				error->message);
			*/
			break;
			
		case TNY_ACCOUNT_ERROR_TRY_CONNECT_AUTHENTICATION_NOT_SUPPORTED:
			g_debug ("%s: Handling GError domain=%d, code=%d (authentication not supported), message=%s", 
 				__FUNCTION__, error->domain, error->code, error->message);
			/* TODO: This needs a logical ID for the string: */
			prompt = g_strdup_printf(
				_("Incorrect Account Settings:\nThe secure authentication method is not supported.\n%s"), 
				error->message);
			break;
			
		case TNY_ACCOUNT_ERROR_TRY_CONNECT_CERTIFICATE:
			g_debug ("%s: Handling GError domain=%d, code=%d (certificatae), message=%s", 
 				__FUNCTION__, error->domain, error->code, error->message);
			prompt = g_strdup_printf(
				_("Certificate Problem:\n%s"), 
				error->message);
			break;
		
		case TNY_ACCOUNT_ERROR_TRY_CONNECT:
		/* The tinymail camel implementation just sends us this for almost 
		 * everything, so we have to guess at the cause.
		 * It could be a wrong password, or inability to resolve a hostname, 
		 * or lack of network, or incorrect authentication method, or something entirely different: */
		/* TODO: Fix camel to provide specific error codes, and then use the 
		 * specific dialog messages from Chapter 12 of the UI spec.
		 */
		case TNY_ACCOUNT_STORE_ERROR_UNKNOWN_ALERT: 
			/* This debug output is useful. Please keep it uncommented until 
			 * we have fixed the problems in this function: */
			g_debug ("%s: Handling GError domain=%d, code=%d, message=%s", 
 				__FUNCTION__, error->domain, error->code, error->message);
			
			/* TODO: Remove the internal error message for the real release.
			 * This is just so the testers can give us more information: */
			/* prompt = _("Modest account not yet fully configured."); */
			prompt = g_strdup_printf(
				"%s\n (Internal error message, often very misleading):\n%s", 
				_("Incorrect Account Settings"), 
				error->message);
				
			/* Note: If the password was wrong then get_password() would be called again,
			 * instead of this vfunc being called. */
			 
			break;
		
		default:
			g_warning ("%s: Unhandled GError code: %d, message=%s", 
				__FUNCTION__, error->code, error->message);
			prompt = NULL;
		break;
	}
	
	if (!prompt)
		return FALSE;

	ModestWindow *main_window = 
		modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ());
	gboolean retval = TRUE;
	if (question) {
		/* The Tinymail documentation says that we should show Yes and No buttons, 
		 * when it is a question.
		 * Obviously, we need tinymail to use more specific error codes instead,
		 * so we know what buttons to show. */
	 
	 	/* TODO: Do this in the main context: */
		GtkWidget *dialog = GTK_WIDGET (hildon_note_new_confirmation (GTK_WINDOW (main_window), 
	 		prompt));
		const int response = gtk_dialog_run (GTK_DIALOG (dialog));
		if (question) {
			retval = (response == GTK_RESPONSE_YES) ||
					 (response == GTK_RESPONSE_OK);
		}
	
		gtk_widget_destroy (dialog);
	
	 } else {
	 	/* Just show the error text and use the default response: */
	 	modest_maemo_show_information_note_in_main_context_and_forget (GTK_WINDOW (main_window), 
	 		prompt);
	 }
	
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

static TnyAccount*
get_tny_account_by (TnyList *accounts,
		    ModestTnyAccountStoreQueryType type,
		    const gchar *str)
{
	TnyIterator *iter = NULL;
	gboolean found = FALSE;
	TnyAccount *retval = NULL;

	iter = tny_list_create_iterator (accounts);
	while (!tny_iterator_is_done (iter) && !found) {
		TnyAccount *tmp_account = NULL;
		const gchar *val = NULL;

		tmp_account = TNY_ACCOUNT (tny_iterator_get_current (iter));
		switch (type) {
		case MODEST_TNY_ACCOUNT_STORE_QUERY_ID:
			val = tny_account_get_id (tmp_account);
			break;
		case MODEST_TNY_ACCOUNT_STORE_QUERY_URL:
			val = tny_account_get_url_string (tmp_account);
			break;
		}
		
		if (type == MODEST_TNY_ACCOUNT_STORE_QUERY_URL && 
		    tny_account_matches_url_string (tmp_account, str)) {
			retval = g_object_ref (tmp_account);
			found = TRUE;
		} else {
			if (strcmp (val, str) == 0) {
				retval = g_object_ref (tmp_account);
				found = TRUE;
			}
		}
		g_object_unref (tmp_account);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	return retval;
}

TnyAccount*
modest_tny_account_store_get_tny_account_by (ModestTnyAccountStore *self, 
					     ModestTnyAccountStoreQueryType type,
					     const gchar *str)
{
	TnyAccount *account = NULL;
	ModestTnyAccountStorePrivate *priv;	
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (str, NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* Search in store accounts */
	account = get_tny_account_by (priv->store_accounts, type, str);

	/* If we already found something, no need to search the transport accounts */
	if (!account) {
		account = get_tny_account_by (priv->transport_accounts, type, str);

		/* If we already found something, no need to search the
		   per-account outbox accounts */
		if (!account)
			account = get_tny_account_by (priv->store_accounts_outboxes, type, str);
	}

	/* Warn if nothing was found. This is generally unusual. */
	if (!account) {
		g_warning("%s: Failed to find account with %s=%s\n", 
			  __FUNCTION__, 
			  (type == MODEST_TNY_ACCOUNT_STORE_QUERY_ID) ? "ID" : "URL",			  
			  str);
	}

	/* Returns a new reference to the account if found */	
	return account;
}

TnyAccount*
modest_tny_account_store_get_server_account (ModestTnyAccountStore *self,
					     const gchar *account_name,
					     TnyAccountType type)
{
	ModestTnyAccountStorePrivate *priv = NULL;
	TnyAccount *retval = NULL;
	TnyList *account_list = NULL;
	TnyIterator *iter = NULL;
	gboolean found;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_name, NULL);
	g_return_val_if_fail (type == TNY_ACCOUNT_TYPE_STORE || 
			      type == TNY_ACCOUNT_TYPE_TRANSPORT,
			      NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	account_list = (type == TNY_ACCOUNT_TYPE_STORE) ? 
		priv->store_accounts : 
		priv->transport_accounts;

	if (!account_list) {
		g_printerr ("%s: No server accounts of type %s\n", __FUNCTION__, 
			(type == TNY_ACCOUNT_TYPE_STORE) ? "store" : "transport");
		return NULL;
	}
	
	/* Look for the server account */
	found = FALSE;
	iter = tny_list_create_iterator (account_list);
	while (!tny_iterator_is_done (iter) && !found) {
		const gchar *modest_acc_name;
		TnyAccount *tmp_account;

		tmp_account = TNY_ACCOUNT (tny_iterator_get_current (iter));
		modest_acc_name = 
			modest_tny_account_get_parent_modest_account_name_for_server_account (tmp_account);
		
		if (!strcmp (account_name, modest_acc_name)) {
			found = TRUE;
			retval = g_object_ref (tmp_account);
		}
		/* Free and continue */
		g_object_unref (tmp_account);
		tny_iterator_next (iter);
	}

	if (!found) {
		g_printerr ("modest: %s: could not get tny %s account for %s\n." \
			    "Number of server accounts of this type=%d\n", __FUNCTION__,
			    (type == TNY_ACCOUNT_TYPE_STORE) ? "store" : "transport",
			    account_name, tny_list_get_length (account_list));
	}

	/* Returns a new reference */
	return retval;
}

static TnyAccount*
get_smtp_specific_transport_account_for_open_connection (ModestTnyAccountStore *self,
							 const gchar *account_name)
{
	/* Get the current connection: */
	TnyDevice *device = modest_runtime_get_device ();
	
	if (!tny_device_is_online (device))
		return NULL;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_name, NULL);
	
	
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
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_name, NULL);

	if (!account_name || !self)
		return NULL;
	
	/*  Get the connection-specific transport acccount, if any: */
	TnyAccount *account =
		get_smtp_specific_transport_account_for_open_connection (self, account_name);
			
	/* If there is no connection-specific transport account (the common case), 
	 * just get the regular transport account: */
	if (!account) {
		/* The special local folders don't have transport accounts. */
		if (strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) == 0)
			account = NULL;
		else
			account = modest_tny_account_store_get_server_account (self, account_name, 
						     TNY_ACCOUNT_TYPE_TRANSPORT);
	}
			     
	return account;
}

TnyAccount*
modest_tny_account_store_get_local_folders_account (ModestTnyAccountStore *self)
{
	TnyAccount *account = NULL;
	ModestTnyAccountStorePrivate *priv;
	TnyIterator *iter;
	gboolean found;

	g_return_val_if_fail (MODEST_IS_TNY_ACCOUNT_STORE (self), NULL);
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	found = FALSE;
	iter = tny_list_create_iterator (priv->store_accounts);
	while (!tny_iterator_is_done (iter) && !found) {
		TnyAccount *tmp_account;

		tmp_account = TNY_ACCOUNT (tny_iterator_get_current (iter));
		if (modest_tny_account_is_virtual_local_folders (tmp_account)) {
			account = g_object_ref (tmp_account);
			found = TRUE;
		}
		g_object_unref (tmp_account);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	/* Returns a new reference to the account */
	return account;
}

/*********************************************************************************/
static void
add_existing_accounts (ModestTnyAccountStore *self)
{
	GSList *account_names = NULL, *iter = NULL;
	ModestTnyAccountStorePrivate *priv = NULL;
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* These are account names, not server_account names */
	account_names = modest_account_mgr_account_names (priv->account_mgr, FALSE);

	for (iter = account_names; iter != NULL; iter = g_slist_next (iter)) {
		const gchar *account_name = (const gchar*) iter->data;
		
		/* Insert all enabled accounts without notifying */
		if (modest_account_mgr_get_enabled (priv->account_mgr, account_name))
			insert_account (self, account_name, FALSE);
	}
	modest_account_mgr_free_account_names (account_names);
}

static TnyAccount*
create_tny_account (ModestTnyAccountStore *self,
		    const gchar *name,
		    TnyAccountType type)
{
	TnyAccount *account = NULL;
	ModestTnyAccountStorePrivate *priv = NULL;
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	account = modest_tny_account_new_from_account (priv->account_mgr,
						       name, type, 
						       priv->session,
						       get_password,
						       forget_password);

	if (account) {
		/* Forget any cached password for the account, so that
		   we use a new account if any */
		modest_tny_account_store_forget_password_in_memory (self, 
								    tny_account_get_id (account));
		/* Set the account store */				
		g_object_set_data (G_OBJECT(account), "account_store", self);
	} else {
		g_printerr ("modest: failed to create account for %s\n", name);
	}

	return account;
}

/*
 * This function will be used for both adding new accounts and for the
 * initialization. In the initialization we do not want to emit
 * signals so notify will be FALSE, in the case of account additions
 * we do want to notify the observers
 */
static void
insert_account (ModestTnyAccountStore *self,
		const gchar *account,
		gboolean notify)
{
	ModestTnyAccountStorePrivate *priv = NULL;
	TnyAccount *store_account = NULL, *transport_account = NULL;
	
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* Get the server and the transport account */
	store_account = create_tny_account (self, account, TNY_ACCOUNT_TYPE_STORE);
	transport_account = create_tny_account (self, account, TNY_ACCOUNT_TYPE_TRANSPORT);

	/* Add to the list, and notify the observers */
	if (store_account) {
		tny_list_append (priv->store_accounts, G_OBJECT (store_account));
		if (notify)
			g_signal_emit (G_OBJECT (self), signals [ACCOUNT_INSERTED_SIGNAL], 0, store_account);
		g_object_unref (store_account);
	}

	/* Add to the list, and notify the observers */
	if (transport_account) {
		TnyAccount *account_outbox = NULL, *local_account = NULL;
		TnyFolder *per_account_outbox = NULL;
		TnyList *folders;
		TnyIterator *iter_folders;

		/* Add account to the list */
		tny_list_append (priv->transport_accounts, G_OBJECT (transport_account));

		/* Add connection-specific transport accounts */
		add_connection_specific_transport_accounts (self, account);

		/* Create per account local outbox */
		account_outbox = 
			modest_tny_account_new_for_per_account_local_outbox_folder (priv->account_mgr, 
										    account, 
										    priv->session);
		tny_list_append (priv->store_accounts_outboxes, G_OBJECT (account_outbox));

		/* Get the outbox folder */
		folders = tny_simple_list_new ();
		tny_folder_store_get_folders (TNY_FOLDER_STORE (account_outbox),
					      folders, NULL, NULL);
		g_assert (tny_list_get_length (folders) == 1);
		iter_folders = tny_list_create_iterator (folders);
		per_account_outbox = TNY_FOLDER (tny_iterator_get_current (iter_folders));
		g_object_unref (iter_folders);
		g_object_unref (account_outbox);

		/* Add the outbox of the new per-account-local-outbox
		   account to the global local merged OUTBOX of the
		   local folders account */
		local_account = modest_tny_account_store_get_local_folders_account (MODEST_TNY_ACCOUNT_STORE (self));
		modest_tny_local_folders_account_add_folder_to_outbox (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (local_account), 
								       per_account_outbox);
		g_object_unref (local_account);
		g_object_unref (per_account_outbox);

		/* Notify the observers */
		if (notify)
			g_signal_emit (G_OBJECT (self), signals [ACCOUNT_INSERTED_SIGNAL], 0, transport_account);
		g_object_unref (transport_account);
	}
}

static void
on_account_inserted (ModestAccountMgr *acc_mgr, 
		     const gchar *account,
		     gpointer user_data)
{
	/* Insert the account and notify the observers */
	insert_account (MODEST_TNY_ACCOUNT_STORE (user_data), account, TRUE);
}

static void
on_account_removed (ModestAccountMgr *acc_mgr, 
		    const gchar *account,
		    gpointer user_data)
{
	TnyAccount *store_account = NULL, *transport_account = NULL;
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	
	/* Get the server and the transport account */
	store_account = 
		modest_tny_account_store_get_server_account (self, account, TNY_ACCOUNT_TYPE_STORE);
	transport_account = 
		modest_tny_account_store_get_server_account (self, account, TNY_ACCOUNT_TYPE_TRANSPORT);

	/* If there was any problem creating the account, for example,
	   with the configuration system this could not exist */
	if (store_account) {
		/* Clear the cache */
		tny_store_account_delete_cache (TNY_STORE_ACCOUNT (store_account));

		/* Notify the observers */
		g_signal_emit (G_OBJECT (self), signals [ACCOUNT_REMOVED_SIGNAL], 0, store_account);
		g_object_unref (store_account);
	} else {
		g_warning ("There is no store account for account %s\n", account);
	}

	/* If there was any problem creating the account, for example,
	   with the configuration system this could not exist */
	if (transport_account) {
		/* Notify the observers */
		g_signal_emit (G_OBJECT (self), signals [ACCOUNT_REMOVED_SIGNAL], 0, transport_account);
		g_object_unref (transport_account);
	} else {
		g_warning ("There is no transport account for account %s\n", account);
	}
}

static void
add_connection_specific_transport_accounts (ModestTnyAccountStore *self,
					    const gchar *account_name)
{
	ModestTnyAccountStorePrivate *priv = NULL;
	GSList *list_specifics = NULL, *iter = NULL;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	list_specifics = modest_account_mgr_get_list (priv->account_mgr,
						      account_name, 
						      MODEST_ACCOUNT_CONNECTION_SPECIFIC_SMTP_LIST,
						      MODEST_CONF_VALUE_STRING, FALSE);
				
	/* Look at each connection-specific transport account for the 
	 * modest account: */
	iter = list_specifics;
	while (iter) {
		/* The list alternates between the connection name and the transport name: */
		iter = g_slist_next (iter);
		if (iter) {
			const gchar* transport_account_name = (const gchar*) (iter->data);
			if (transport_account_name) {
				TnyAccount * tny_account = NULL;
				/* Add the account: */
				tny_account = 
					modest_tny_account_new_from_server_account_name (priv->account_mgr, 
											 priv->session, 
											 transport_account_name);
				if (tny_account) {
					modest_tny_account_set_parent_modest_account_name_for_server_account (tny_account, 
													      account_name);
					g_object_set_data (G_OBJECT(tny_account), 
							   "account_store", 
							   (gpointer)self);

					tny_list_append (priv->transport_accounts, G_OBJECT (tny_account));
				} else
					g_printerr ("modest: failed to create smtp-specific account for %s\n",
						    transport_account_name);
			}
		}				
		iter = g_slist_next (iter);
	}
}

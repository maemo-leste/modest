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

#include <tny-error.h>
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
#include <modest-tny-local-folders-account.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <widgets/modest-window-mgr.h>
#include <modest-signal-mgr.h>
#include <modest-debug.h>

#include "modest-tny-account-store.h"
#include "modest-tny-platform-factory.h"
#include <tny-gtk-lockable.h>
#include <camel/camel.h>
#include <modest-platform.h>
#include "modest-ui-actions.h"
#include <widgets/modest-account-settings-dialog.h>

#ifdef MODEST_PLATFORM_MAEMO
#include <tny-maemo-conic-device.h>
#include <maemo/modest-maemo-utils.h>
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

static void    forget_password_in_memory (ModestTnyAccountStore *self, 
					  const gchar *server_account_name);

static void    add_connection_specific_transport_accounts         (ModestTnyAccountStore *self);

static void    connection_status_changed   (TnyAccount *account, 
					    TnyConnectionStatus status, 
					    gpointer data);

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

	GSList *sighandlers;
	
	/* We cache the lists of accounts here */
	TnyList             *store_accounts;
	TnyList             *transport_accounts;
	TnyList             *store_accounts_outboxes;
	
	/* Matches transport accounts and outbox folder */
	GHashTable          *outbox_of_transport;
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
	priv->sighandlers            = NULL;
	
	priv->outbox_of_transport = g_hash_table_new_full (g_direct_hash,
							   g_direct_equal,
							   NULL,
							   NULL);

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
	/* This is a singleton, so it does not need to be unrefed. */
	monitor = gnome_vfs_get_volume_monitor();

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
						       G_OBJECT(monitor), 
						       "volume-mounted",
						       G_CALLBACK(on_vfs_volume_mounted),
						       obj);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
						       G_OBJECT(monitor), "volume-unmounted",
						       G_CALLBACK(on_vfs_volume_unmounted),
						       obj);
}

/* disconnect the list of TnyAccounts */
static void
account_disconnect (TnyAccount *account)
{
	g_return_if_fail (account && TNY_IS_ACCOUNT(account));

	if (TNY_IS_STORE_ACCOUNT (account) && 
	    !modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (account)))
		return;

	tny_camel_account_set_online (TNY_CAMEL_ACCOUNT(account), FALSE, NULL, NULL);
}


/* disconnect the list of TnyAccounts */
static void
account_verify_last_ref (TnyAccount *account, const gchar *str)
{
	gchar *txt;

	g_return_if_fail (account && TNY_IS_ACCOUNT(account));

	txt = g_strdup_printf ("%s: %s", str ? str : "?", tny_account_get_name(account));
	MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(G_OBJECT(account),txt);
	g_free (txt);
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

/** Only call this if the memory card is really mounted.
 */ 
static void
add_mmc_account(ModestTnyAccountStore *self, gboolean emit_insert_signal)
{
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	g_return_if_fail (priv->session);
	
	TnyAccount *mmc_account = modest_tny_account_new_for_local_folders (priv->account_mgr, 
									priv->session, 
									MODEST_MCC1_VOLUMEPATH);

	/* Add to the list of store accounts */
	tny_list_append (priv->store_accounts, G_OBJECT (mmc_account));

	if (emit_insert_signal) {
		g_signal_emit (G_OBJECT (self), 
			       signals [ACCOUNT_INSERTED_SIGNAL],
			       0, mmc_account);
	}

	/* Free */
	g_object_unref (mmc_account);
}

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
		add_mmc_account (self, TRUE /* emit the insert signal. */);
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
 * forget_password_in_memory
 * @self: a TnyAccountStore instance
 * @account: A server account.
 * 
 * Forget any password stored in memory for this account.
 * For instance, this should be called when the user has changed the password in the account settings.
 */
static void
forget_password_in_memory (ModestTnyAccountStore *self, 
			   const gchar * server_account_name)
{
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (server_account_name && priv->password_hash) {
		g_hash_table_remove (priv->password_hash, server_account_name);
	}
}

static void
on_account_changed (ModestAccountMgr *acc_mgr, 
		    const gchar *account_name, 
		    TnyAccountType account_type,
		    gpointer user_data)
{
	ModestTnyAccountStore *self = MODEST_TNY_ACCOUNT_STORE(user_data);
	ModestTnyAccountStorePrivate *priv;
	TnyList* account_list;
	gboolean found = FALSE;
	TnyIterator *iter = NULL;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	account_list = (account_type == TNY_ACCOUNT_TYPE_STORE ? 
			priv->store_accounts : 
			priv->transport_accounts);
	
	iter = tny_list_create_iterator (account_list);
	while (!tny_iterator_is_done (iter) && !found) {
		TnyAccount *tny_account;
		tny_account = TNY_ACCOUNT (tny_iterator_get_current (iter));
		if (tny_account) {
			if (!strcmp (tny_account_get_id (tny_account), account_name)) {
				found = TRUE;
				modest_tny_account_update_from_account (tny_account, get_password, forget_password);
				g_signal_emit (G_OBJECT(self), signals[ACCOUNT_CHANGED_SIGNAL], 0, tny_account);
			}
			g_object_unref (tny_account);
		}
		tny_iterator_next (iter);
	}

	if (iter)
		g_object_unref (iter);
}

static void 
on_account_settings_hide (GtkWidget *widget, gpointer user_data)
{
	/* This is easier than using a struct for the user_data: */
	ModestTnyAccountStore *self = modest_runtime_get_account_store();
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	gchar *account_name = (gchar *) user_data;
	if (account_name)
		g_hash_table_remove (priv->account_settings_dialog_hash, account_name);
}

static void 
show_password_warning_only (const gchar *msg)
{
	ModestWindow *main_window = 
		modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (), FALSE); /* don't create */
	
	/* Show an explanatory temporary banner: */
	if (main_window) 
		modest_platform_information_banner (NULL, NULL, msg);
}

static void 
show_wrong_password_dialog (TnyAccount *account)
{ 
	/* This is easier than using a struct for the user_data: */
	ModestTnyAccountStore *self = modest_runtime_get_account_store();
	const gchar *modest_account_name;
	GtkWidget *main_window;
	GtkWidget *dialog;

	main_window = (GtkWidget *) modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (),
								       FALSE); /* don't create */
	if (!main_window) {
		g_warning ("%s: password was wrong; ignoring because no main window", __FUNCTION__);
		return;
	}

	modest_account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (account);
	if (!modest_account_name) {
		g_warning ("%s: modest_tny_account_get_parent_modest_account_name_for_server_account() failed.\n", 
			__FUNCTION__);
	}
	
	dialog = modest_tny_account_store_show_account_settings_dialog (self, modest_account_name);
	modest_account_settings_dialog_save_password (MODEST_ACCOUNT_SETTINGS_DIALOG (dialog));
	/* Show an explanatory temporary banner: */
	modest_platform_information_banner (GTK_WIDGET(dialog), NULL, _("mcen_ib_username_pw_incorrect"));
}

/* This callback will be called by Tinymail when it needs the password
 * from the user or the account settings.
 * It can also call forget_password() before calling this,
 * so that we clear wrong passwords out of our account settings.
 * Note that TnyAccount here will be the server account. */
static gchar*
get_password (TnyAccount *account, const gchar * prompt_not_used, gboolean *cancel)
{
	ModestTnyAccountStore *self = NULL;
	ModestTnyAccountStorePrivate *priv;
	gchar *username = NULL;
	gchar *pwd = NULL;
	gpointer pwd_ptr = NULL;
	gboolean already_asked = FALSE;
	const gchar *server_account_name;
	gchar *url_string;

	g_return_val_if_fail (account, NULL);
	
	MODEST_DEBUG_BLOCK(
		g_debug ("%s: prompt (not shown) = %s\n", __FUNCTION__, prompt_not_used);
	);		

	/* Get a reference to myself */
	self = MODEST_TNY_ACCOUNT_STORE (g_object_get_data (G_OBJECT(account), "account_store"));
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* Ensure that we still have this account. It could happen
	   that a set_online was requested *before* removing an
	   account, and due to tinymail emits the get_password
	   function using a g_idle the account could be actually
	   removed *before* this function was really called */
	url_string = tny_account_get_url_string (account);
	if (url_string) {
		TnyAccount *tmp_account;

		tmp_account = tny_account_store_find_account (TNY_ACCOUNT_STORE (self), 
							      url_string);
		g_free (url_string);

		if (!tmp_account) {
			*cancel = TRUE;
			return NULL;
		}
		g_object_unref (tmp_account);
	}

	server_account_name = tny_account_get_id (account);
	if (!server_account_name || !self) {
		g_warning ("modest: %s: could not retrieve account_store for account %s",
			   __FUNCTION__, server_account_name ? server_account_name : "<NULL>");
		if (cancel)
			*cancel = TRUE;
		
		return NULL;
	}
	
	/* This hash map stores passwords, including passwords that are not stored in gconf. */
	/* Is it in the hash? if it's already there, it must be wrong... */
	pwd_ptr = (gpointer)&pwd; /* pwd_ptr so the compiler does not complained about
				   * type-punned ptrs...*/
	already_asked = priv->password_hash && 
				g_hash_table_lookup_extended (priv->password_hash,
						      server_account_name,
						      NULL,
						      (gpointer*)&pwd_ptr);
	MODEST_DEBUG_BLOCK(
		g_debug ("%s: Already asked = %d\n", __FUNCTION__, already_asked);
	);
		
	/* If the password is not already there, try ModestConf */
	if (!already_asked) {
		pwd  = modest_account_mgr_get_server_account_password (priv->account_mgr,
								       server_account_name);
		g_hash_table_insert (priv->password_hash, g_strdup (server_account_name), g_strdup (pwd));
	}

	/* If it was already asked, it must have been wrong, so ask again */
	if (already_asked || !pwd || strlen(pwd) == 0) {
		/* As per the UI spec, if no password was set in the account settings, 
		 * ask for it now. But if the password is wrong in the account settings, 
		 * then show a banner and the account settings dialog so it can be corrected:
		 */
		ModestTransportStoreProtocol proto;
		const gboolean settings_have_password = 
			modest_account_mgr_get_server_account_has_password (priv->account_mgr, server_account_name);

		/* Show an error and after that ask for a password */
		proto = modest_protocol_info_get_transport_store_protocol (tny_account_get_proto (account));
		if (proto == MODEST_PROTOCOL_TRANSPORT_SMTP) {
			gchar *username = NULL, *msg = NULL;
			username = modest_account_mgr_get_server_account_username (priv->account_mgr,
										   server_account_name);
			if (!username || strlen(username) == 0) {
				msg = g_strdup_printf (_("emev_ni_ui_smtp_userid_invalid"), 
						       tny_account_get_name (account),
						       tny_account_get_hostname (account));
			} else {
				gchar *password;
				password  = modest_account_mgr_get_server_account_password (priv->account_mgr,
											    server_account_name);
				if (!password || strlen(password) == 0)
					msg = g_strdup_printf (_("emev_ni_ui_smtp_passwd_invalid"), 
							       tny_account_get_name (account),
							       tny_account_get_hostname (account));
				else
					msg = g_strdup_printf (_("emev_ni_ui_smtp_authentication_fail_error"), 
							       tny_account_get_hostname (account));
				if (password)
					g_free (password);
			}
			if (msg) {
				modest_platform_run_information_dialog (NULL, msg, TRUE);
				g_free (msg);
			}
			if (username)
				g_free (username);
		}

		if (settings_have_password) {
			/* The password must be wrong, so show the account settings dialog so it can be corrected: */
			show_wrong_password_dialog (account);
			
			if (cancel)
				*cancel = TRUE;
				
			return NULL;
		}
	
		/* we don't have it yet. Get the password from the user */
		const gchar* account_id = tny_account_get_id (account);
		gboolean remember = FALSE;
		pwd = NULL;
		
		if (already_asked) {
			const gchar *msg;
			gboolean username_known = 
				modest_account_mgr_get_server_account_username_has_succeeded(priv->account_mgr, 
											     server_account_name);
			/* If the login has ever succeeded then show a specific message */
			if (username_known)
				msg = _("ecdg_ib_set_password_incorrect");
			else
				msg = _("mcen_ib_username_pw_incorrect");
			show_password_warning_only (msg);
		}

		/* Request password */
		g_signal_emit (G_OBJECT (self), signals[PASSWORD_REQUESTED_SIGNAL], 0,
			       account_id, /* server_account_name */
			       &username, &pwd, cancel, &remember);

		
		if (!*cancel) {
			/* The password will be returned as the result,
			 * but we need to tell tinymail about the username too: */
			tny_account_set_user (account, username);
			
			/* Do not save the password in gconf, because
			 * the UI spec says "The password will never
			 * be saved in the account": */

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
		if (cancel)
			*cancel = FALSE;	
	return pwd;
}

void 
modest_tny_account_store_forget_already_asked (ModestTnyAccountStore *self, TnyAccount *account)
{
	g_return_if_fail (account);
	  
	ModestTnyAccountStorePrivate *priv;
	gchar *pwd = NULL;
	gpointer pwd_ptr = NULL;
	gboolean already_asked = FALSE;
		
	const gchar *server_account_name = tny_account_get_id (account);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* This hash map stores passwords, including passwords that are not stored in gconf. */
	pwd_ptr = (gpointer)&pwd; /* pwd_ptr so the compiler does not complained about
				   * type-punned ptrs...*/
	already_asked = priv->password_hash && 
				g_hash_table_lookup_extended (priv->password_hash,
						      server_account_name,
						      NULL,
						      (gpointer*)&pwd_ptr);

	if (already_asked) {
		g_hash_table_remove (priv->password_hash, server_account_name);		
		g_free (pwd);
		pwd = NULL;
	}

	return;
}

/* tinymail calls this if the connection failed due to an incorrect password.
 * And it seems to call this for any general connection failure. */
static void
forget_password (TnyAccount *account)
{
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
	ModestTnyAccountStore *self        = MODEST_TNY_ACCOUNT_STORE(obj);
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	g_free (priv->cache_dir);
	priv->cache_dir = NULL;
	
	if (priv->password_hash) {
		g_hash_table_destroy (priv->password_hash);
		priv->password_hash = NULL;
	}

	if (priv->account_settings_dialog_hash) {
		g_hash_table_destroy (priv->account_settings_dialog_hash);
		priv->account_settings_dialog_hash = NULL;
	}

	if (priv->outbox_of_transport) {
		g_hash_table_destroy (priv->outbox_of_transport);
		priv->outbox_of_transport = NULL;
	}

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;	

	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	if (priv->device) {
		g_object_unref (G_OBJECT(priv->device));
		priv->device = NULL;
	}

	/* Destroy all accounts. Disconnect all accounts before they are destroyed */
	if (priv->store_accounts) {
		tny_list_foreach (priv->store_accounts, (GFunc)account_disconnect, NULL);
		tny_list_foreach (priv->store_accounts, (GFunc)account_verify_last_ref, "store");
		g_object_unref (priv->store_accounts);
		priv->store_accounts = NULL;
	}
	
	if (priv->transport_accounts) {
		tny_list_foreach (priv->transport_accounts, (GFunc)account_disconnect, NULL);
		tny_list_foreach (priv->transport_accounts, (GFunc)account_verify_last_ref, "transport");
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

	camel_shutdown ();
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

gboolean 
volume_path_is_mounted (const gchar* path)
{
	g_return_val_if_fail (path, FALSE);

	gboolean result = FALSE;
	gchar * path_as_uri = g_filename_to_uri (path, NULL, NULL);
	g_return_val_if_fail (path_as_uri, FALSE);

	/* Get the monitor singleton: */
	GnomeVFSVolumeMonitor *monitor = gnome_vfs_get_volume_monitor();

	/* This seems like a simpler way to do this, but it returns a 	
	 * GnomeVFSVolume even if the drive is not mounted: */
	/*
	GnomeVFSVolume *volume = gnome_vfs_volume_monitor_get_volume_for_path (monitor, 
		MODEST_MCC1_VOLUMEPATH);
	if (volume) {
		gnome_vfs_volume_unref(volume);
	}
	*/

	/* Get the mounted volumes from the monitor: */
	GList *list = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);
	GList *iter = list;
	for (iter = list; iter; iter = g_list_next (iter)) {
		GnomeVFSVolume *volume = (GnomeVFSVolume*)iter->data;
		if (volume) {
			/*
			char *display_name = 
				gnome_vfs_volume_get_display_name (volume);
			printf ("volume display name=%s\n", display_name);
			g_free (display_name);
			*/
			
			char *uri = 
				gnome_vfs_volume_get_activation_uri (volume);
			/* printf ("  uri=%s\n", uri); */
			if (uri && (strcmp (uri, path_as_uri) == 0))
				result = TRUE;

			g_free (uri);

			gnome_vfs_volume_unref (volume);
		}
	}

	g_list_free (list);

	g_free (path_as_uri);

	return result;
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
	priv->sighandlers  =  modest_signal_mgr_connect (priv->sighandlers,
							 G_OBJECT(account_mgr), "account_inserted",
							 G_CALLBACK (on_account_inserted), obj);
	priv->sighandlers  =  modest_signal_mgr_connect (priv->sighandlers,
							 G_OBJECT(account_mgr), "account_changed",
							 G_CALLBACK (on_account_changed), obj);
	priv->sighandlers  =  modest_signal_mgr_connect (priv->sighandlers,
							 G_OBJECT(account_mgr), "account_removed",
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

	/* Add the other remote accounts. Do this after adding the
	   local account, because we need to add our outboxes to the
	   global OUTBOX hosted in the local account */
	add_existing_accounts (MODEST_TNY_ACCOUNT_STORE (obj));
	
	/* FIXME: I'm doing this (adding an "if (FALSE)"because this
	   stuff is not working properly and could cause SIGSEVs, for
	   example one send queue will be created for each connection
	   specific SMTP server, so when tinymail asks for the outbox
	   it will return NULL because there is no outbox folder for
	   this specific transport accounts, and it's a must that the
	   send queue returns an outbox */
	if (TRUE)
		/* Add connection-specific transport accounts */
		add_connection_specific_transport_accounts (MODEST_TNY_ACCOUNT_STORE(obj));
	
	/* This is a singleton, so it does not need to be unrefed. */
	if (volume_path_is_mounted (MODEST_MCC1_VOLUMEPATH)) {
		/* It is mounted: */
		add_mmc_account (MODEST_TNY_ACCOUNT_STORE (obj), FALSE /* don't emit the insert signal. */); 
	}
	
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
modest_tny_account_store_alert (TnyAccountStore *self, 
				TnyAccount *account, 
				TnyAlertType type,
				gboolean question, 
				GError *error)
{
	ModestTransportStoreProtocol proto =
		MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN; 
	const gchar* server_name = NULL;
	gchar *prompt = NULL;
	gboolean retval;


	g_return_val_if_fail (account, FALSE);
	g_return_val_if_fail (error, FALSE);
	
	/* Get the server name: */
	server_name = tny_account_get_hostname (account);

	if (account) {
		const gchar *proto_name = tny_account_get_proto (account);
		if (proto_name)
			proto = modest_protocol_info_get_transport_store_protocol (proto_name);
		else {
			g_warning("modest: %s: account with id=%s has no proto.\n", __FUNCTION__, 
				  tny_account_get_id (account));
			return FALSE;
		}
	}

	switch (error->code) {
	case TNY_SYSTEM_ERROR_CANCEL:
		/* Don't show waste the user's time by showing him a dialog telling 
		 * him that he has just cancelled something: */
		return TRUE;

	case TNY_SERVICE_ERROR_PROTOCOL:
		/* Like a BAD from IMAP (protocol error) */
	case TNY_SERVICE_ERROR_LOST_CONNECTION:
		/* Lost the connection with the service */
	case TNY_SERVICE_ERROR_UNAVAILABLE:
		/* You must be working online for this operation */
	case TNY_SERVICE_ERROR_CONNECT:
		switch (proto) {
		case MODEST_PROTOCOL_STORE_POP:
			prompt = g_strdup_printf (_("emev_ni_ui_pop3_msg_connect_error"),
						  server_name);
			break;
		case MODEST_PROTOCOL_STORE_IMAP:
			prompt = g_strdup_printf (_("emev_ni_ui_imap_connect_server_error"),
						  server_name);
			break;
		case MODEST_PROTOCOL_TRANSPORT_SMTP:
			prompt = g_strdup_printf (_("emev_ib_ui_smtp_server_invalid"),
						  server_name);
			break;
		default:
			g_return_val_if_reached (FALSE);
		}
		break;
		
	case TNY_SERVICE_ERROR_AUTHENTICATE:
		/* It seems that there's no better error to show with
		 * POP and IMAP because TNY_SERVICE_ERROR_AUTHENTICATE
		 * may appear if there's a timeout during auth */
		switch (proto) {
		case MODEST_PROTOCOL_STORE_POP:
			prompt = g_strdup_printf (_("emev_ni_ui_pop3_msg_connect_error"),
						  server_name);
			break;
		case MODEST_PROTOCOL_STORE_IMAP:
			prompt = g_strdup_printf (_("emev_ni_ui_imap_connect_server_error"),
						  server_name);
			break;
		case MODEST_PROTOCOL_TRANSPORT_SMTP:
			prompt = g_strdup_printf (_("emev_ni_ui_smtp_authentication_fail_error"),
						  server_name);
			break;
		default:
			g_return_val_if_reached (FALSE);
		}
		break;
			
	case TNY_SERVICE_ERROR_CERTIFICATE:
		/* We'll show the proper dialog later */
		break;

	case TNY_SYSTEM_ERROR_MEMORY:
		/* Can't allocate memory for this operation */

	case TNY_SERVICE_ERROR_UNKNOWN: 
		return FALSE;			
	default:
		g_return_val_if_reached (FALSE);
	}
	

	if (error->code == TNY_SERVICE_ERROR_CERTIFICATE)
		retval = modest_platform_run_certificate_confirmation_dialog (server_name,
									      error->message);
	else
		retval = modest_platform_run_alert_dialog (prompt, question);
	
	if (prompt)
		g_free (prompt);
	
	return retval;
}


static void
modest_tny_account_store_init (gpointer g, gpointer iface_data)
{
        TnyAccountStoreIface *klass;

	g_return_if_fail (g);

	klass = (TnyAccountStoreIface *)g;

	klass->get_accounts =
		modest_tny_account_store_get_accounts;
	klass->get_cache_dir =
		modest_tny_account_store_get_cache_dir;
	klass->get_device =
		modest_tny_account_store_get_device;
	klass->alert =
		modest_tny_account_store_alert;
	klass->find_account =
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

	g_return_val_if_fail (TNY_IS_LIST(accounts), NULL);

	if (tny_list_get_length(accounts) == 0) {
		g_warning ("%s: account list is empty", __FUNCTION__);
		return NULL;
	}
	
	iter = tny_list_create_iterator (accounts);
	while (!tny_iterator_is_done (iter) && !found) {
		TnyAccount *tmp_account = NULL;
		const gchar *val = NULL;

		tmp_account = TNY_ACCOUNT (tny_iterator_get_current (iter));
		if (!TNY_IS_ACCOUNT(tmp_account)) {
			g_warning ("%s: not a valid account", __FUNCTION__);
			tmp_account = NULL;
			break;
		}

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
			if (val && str && strcmp (val, str) == 0) {
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
		
		if (account_name && modest_acc_name && !strcmp (account_name, modest_acc_name)) {
			found = TRUE;
			retval = g_object_ref (tmp_account);
		}
		/* Free and continue */
		g_object_unref (tmp_account);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	if (!found) {
		g_printerr ("modest: %s: could not get tny %s account for %s\n." \
			    "Number of server accounts of this type=%d\n", __FUNCTION__,
			    (type == TNY_ACCOUNT_TYPE_STORE) ? "store" : "transport",
			    account_name, tny_list_get_length (account_list));
	}

	/* Returns a new reference */
	return retval;
}

TnyAccount*
modest_tny_account_store_get_smtp_specific_transport_account_for_open_connection (
	ModestTnyAccountStore *self, const gchar *account_name)
{
	TnyDevice *device;

	g_return_val_if_fail (self && MODEST_IS_TNY_ACCOUNT_STORE(self), NULL);
	g_return_val_if_fail (account_name, NULL);

	/* Get the current connection: */
	device = modest_runtime_get_device ();

	if (!device) {
		g_warning ("%s: could not get device", __FUNCTION__);
		return NULL;
	}
		
	if (!tny_device_is_online (device))
		return NULL;
	
#ifdef MODEST_HAVE_CONIC
	g_return_val_if_fail (TNY_IS_MAEMO_CONIC_DEVICE (device), NULL);
	
	TnyMaemoConicDevice *maemo_device = TNY_MAEMO_CONIC_DEVICE (device);	
	const gchar* iap_id = tny_maemo_conic_device_get_current_iap_id (maemo_device);
	/* printf ("DEBUG: %s: iap_id=%s\n", __FUNCTION__, iap_id); */
	if (!iap_id)
		return NULL;
		
	ConIcIap* connection = tny_maemo_conic_device_get_iap (maemo_device, iap_id);
	if (!connection)
		return NULL;
		
	const gchar *connection_id = con_ic_iap_get_id (connection);
	/* printf ("DEBUG: %s: connection_id=%s\n", __FUNCTION__, connection_id); */
	if (!connection_id)
		return NULL;
	
	/*  Get the connection-specific transport acccount, if any: */
	ModestAccountMgr *account_manager = modest_runtime_get_account_mgr ();

	/* Check if this account has connection-specific SMTP enabled */
	if (!modest_account_mgr_get_use_connection_specific_smtp (account_manager, account_name)) {
		return NULL;
	}

	gchar* server_account_name = modest_account_mgr_get_connection_specific_smtp (account_manager, 
		connection_id);

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
#endif /* MODEST_HAVE_CONIC */
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
	/* Note: This gives us a reference: */
	TnyAccount *account =
		modest_tny_account_store_get_smtp_specific_transport_account_for_open_connection (self, account_name);
			
	/* If there is no connection-specific transport account (the common case), 
	 * just get the regular transport account: */
	if (!account) {
		/* The special local folders don't have transport accounts. */
		if (strcmp (account_name, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) == 0)
			account = NULL;
		else {
			/* Note: This gives us a reference: */
			account = modest_tny_account_store_get_server_account (self, account_name, 
						     TNY_ACCOUNT_TYPE_TRANSPORT);
		}
	}
			
	/* returns a reference. */     
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

TnyAccount*
modest_tny_account_store_get_mmc_folders_account (ModestTnyAccountStore *self)
{
	g_return_val_if_fail (MODEST_IS_TNY_ACCOUNT_STORE (self), NULL);
	
	/* New reference */
	return modest_tny_account_store_get_tny_account_by (self, MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
							    MODEST_MMC_ACCOUNT_ID);

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

static void 
connection_status_changed (TnyAccount *account, 
			   TnyConnectionStatus status, 
			   gpointer data)
{
	/* We do this here and not in the connection policy because we
	   don't want to do it for every account, just for the
	   accounts that are interactively added when modest is
	   runnning */
	if (status == TNY_CONNECTION_STATUS_CONNECTED) {
		const gchar *account_name;
		ModestWindow *main_window;
		ModestTnyAccountStorePrivate *priv = NULL;
		
		priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (data);

		/* Remove this handler */
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers, 
								  G_OBJECT (account),
								  "connection_status_changed");

		/* Perform a send receive */
		account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (account);
		main_window = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (), FALSE);
		modest_ui_actions_do_send_receive (account_name, FALSE, FALSE, FALSE, main_window);
	}
}

static TnyAccount*
create_tny_account (ModestTnyAccountStore *self,
		    const gchar *name,
		    TnyAccountType type,
		    gboolean notify)
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
		forget_password_in_memory (self, tny_account_get_id (account));

		/* Install a signal handler that will refresh the
		   account the first time it becomes online. Do this
		   only if we're adding a new account while the
		   program is running (we do not want to do this
		   allways) */
		if (type == TNY_ACCOUNT_TYPE_STORE && notify)
			priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
								       G_OBJECT (account), 
								       "connection_status_changed",
								       G_CALLBACK (connection_status_changed),
								       self);

		/* Set the account store */
		g_object_set_data (G_OBJECT(account), "account_store", self);
	} else {
		g_printerr ("modest: failed to create account for %s\n", name);
	}

	return account;
}


static void
add_outbox_from_transport_account_to_global_outbox (ModestTnyAccountStore *self,
						    const gchar *account_name,
						    TnyAccount *transport_account)
{
	TnyList *folders = NULL;
	TnyIterator *iter_folders = NULL;
	TnyAccount *local_account = NULL, *account_outbox = NULL;
	TnyFolder *per_account_outbox = NULL;
	ModestTnyAccountStorePrivate *priv = NULL;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* Create per account local outbox */
	account_outbox = 
		modest_tny_account_new_for_per_account_local_outbox_folder (priv->account_mgr, 
									    account_name, 
									    priv->session);
	tny_list_append (priv->store_accounts_outboxes, G_OBJECT (account_outbox));

	/* Get the outbox folder */
	folders = tny_simple_list_new ();
	tny_folder_store_get_folders (TNY_FOLDER_STORE (account_outbox), folders, NULL, NULL);
	if (tny_list_get_length (folders) != 1) {
		g_warning ("%s: > 1 outbox found (%d)?!", __FUNCTION__,
			   tny_list_get_length (folders));
	}
			
	iter_folders = tny_list_create_iterator (folders);
	per_account_outbox = TNY_FOLDER (tny_iterator_get_current (iter_folders));
	g_object_unref (iter_folders);
	g_object_unref (folders);
	g_object_unref (account_outbox);

	/* Add the outbox of the new per-account-local-outbox account
	   to the global local merged OUTBOX of the local folders
	   account */
	local_account = modest_tny_account_store_get_local_folders_account (self);
	modest_tny_local_folders_account_add_folder_to_outbox (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (local_account),
							       per_account_outbox);
	/* Add the pair to the hash table */
	g_hash_table_insert (priv->outbox_of_transport,
			     transport_account,
			     per_account_outbox);
	
	g_object_unref (local_account);
	g_object_unref (per_account_outbox);
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
	store_account = create_tny_account (self, account, TNY_ACCOUNT_TYPE_STORE, notify);
	if (!store_account || !TNY_IS_ACCOUNT(store_account)) {
		g_warning ("%s: failed to create store account", __FUNCTION__);
		return;
	}

	transport_account = create_tny_account (self, account, TNY_ACCOUNT_TYPE_TRANSPORT, notify);
	if (!transport_account || !TNY_IS_ACCOUNT(transport_account)) {
		g_warning ("%s: failed to create transport account", __FUNCTION__);
		g_object_unref (store_account);
		return;
	}

	/* Add accounts to the lists */
	tny_list_append (priv->store_accounts, G_OBJECT (store_account));
	tny_list_append (priv->transport_accounts, G_OBJECT (transport_account));
	
	/* Create a new pseudo-account with an outbox for this
	   transport account and add it to the global outbox
	   in the local account */
	add_outbox_from_transport_account_to_global_outbox (self, account, transport_account);
	
	/* Notify the observers. We do it after everything is
	   created */
	if (notify) {
		TnyAccount *local_account = NULL;
		
		/* Notify the observers about the new server & transport accounts */
		g_signal_emit (G_OBJECT (self), signals [ACCOUNT_INSERTED_SIGNAL], 0, store_account);	
		g_signal_emit (G_OBJECT (self), signals [ACCOUNT_INSERTED_SIGNAL], 0, transport_account);

		/* Notify that the local account changed */
		local_account = modest_tny_account_store_get_local_folders_account (self);
		g_signal_emit (G_OBJECT (self), signals [ACCOUNT_CHANGED_SIGNAL], 0, local_account);
		g_object_unref (local_account);
	}

	/* Frees */
	g_object_unref (store_account);
	g_object_unref (transport_account);
}

static void
on_account_inserted (ModestAccountMgr *acc_mgr, 
		     const gchar *account,
		     gpointer user_data)
{
	/* Insert the account and notify the observers */
	insert_account (MODEST_TNY_ACCOUNT_STORE (user_data), account, TRUE);
}

/* This is the callback of the tny_camel_account_set_online called in
   on_account_removed to disconnect the account */
static void
on_account_disconnect_when_removing (TnyCamelAccount *account, 
				     gboolean canceled, 
				     GError *err, 
				     gpointer user_data)
{
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;

	self = MODEST_TNY_ACCOUNT_STORE (user_data);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self);

	/* Remove the connection-status-changed handler if it's still there */
	if (modest_signal_mgr_is_connected (priv->sighandlers, 
					    G_OBJECT (account),
					    "connection_status_changed")) {
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers, 
								  G_OBJECT (account),
								  "connection_status_changed");
	}

	/* Cancel all pending operations */
	tny_account_cancel (TNY_ACCOUNT (account));
	
	/* Unref the extra reference added by get_server_account */
	g_object_unref (account);

	/* Clear the cache if it's an store account */
	if (TNY_IS_STORE_ACCOUNT (account))
		tny_store_account_delete_cache (TNY_STORE_ACCOUNT (account));
}

static void
on_account_removed (ModestAccountMgr *acc_mgr, 
		    const gchar *account,
		    gpointer user_data)
{
	TnyAccount *store_account = NULL, *transport_account = NULL;
	ModestTnyAccountStore *self;
	ModestTnyAccountStorePrivate *priv;
	
	self = MODEST_TNY_ACCOUNT_STORE (user_data);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self);

	/* Get the server and the transport account */
	store_account = 
		modest_tny_account_store_get_server_account (self, account, TNY_ACCOUNT_TYPE_STORE);
	transport_account = 
		modest_tny_account_store_get_server_account (self, account, TNY_ACCOUNT_TYPE_TRANSPORT);
	
	/* If there was any problem creating the account, for example,
	   with the configuration system this could not exist */
	if (store_account) {
		/* Forget any cached password for the account */
		forget_password_in_memory (self, tny_account_get_id (store_account));

		/* Remove it from the list of accounts and notify the
		   observers. Do not need to wait for account
		   disconnection */
		tny_list_remove (priv->store_accounts, (GObject *) store_account);
		g_signal_emit (G_OBJECT (self), signals [ACCOUNT_REMOVED_SIGNAL], 0, store_account);

		/* Cancel all pending operations */
		tny_account_cancel (TNY_ACCOUNT (store_account));

		/* Disconnect before deleting the cache, because the
		   disconnection will rewrite the cache to the
		   disk */
		tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (store_account), FALSE,
					      on_account_disconnect_when_removing, self);
	} else {
		g_warning ("There is no store account for account %s\n", account);
	}

	/* If there was any problem creating the account, for example,
	   with the configuration system this could not exist */
	if (transport_account) {
		TnyAccount *local_account = NULL;
		TnyFolder *outbox = NULL;

		/* Forget any cached password for the account */
		forget_password_in_memory (self, tny_account_get_id (store_account));

		/* Remove it from the list of accounts and notify the
		   observers. Do not need to wait for account
		   disconnection */
		tny_list_remove (priv->transport_accounts, (GObject *) transport_account);
		g_signal_emit (G_OBJECT (self), signals [ACCOUNT_REMOVED_SIGNAL], 0, transport_account);
	
		/* Remove the OUTBOX of the account from the global outbox */
		outbox = g_hash_table_lookup (priv->outbox_of_transport, transport_account);

		if (TNY_IS_FOLDER (outbox)) {
			TnyAccount *outbox_account = tny_folder_get_account (outbox);

			if (outbox_account) {
				tny_list_remove (priv->store_accounts_outboxes, G_OBJECT (outbox_account));
				g_object_unref (outbox_account);
			}

			local_account = modest_tny_account_store_get_local_folders_account (self);
			modest_tny_local_folders_account_remove_folder_from_outbox (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (local_account),
										    outbox);
			g_hash_table_remove (priv->outbox_of_transport, transport_account);

			/* Notify the change in the local account */
			g_signal_emit (G_OBJECT (self), signals [ACCOUNT_CHANGED_SIGNAL], 0, local_account);
			g_object_unref (local_account);
		} else {
			g_warning ("Removing a transport account that has no outbox");
		}

		/* Cancel all pending operations */
		tny_account_cancel (TNY_ACCOUNT (transport_account));

		/* Disconnect and notify the observers. The callback will free the reference */
		tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (transport_account), FALSE,
					      on_account_disconnect_when_removing, self);
	} else {
		g_warning ("There is no transport account for account %s\n", account);
	}
}

TnyTransportAccount *
modest_tny_account_store_new_connection_specific_transport_account (ModestTnyAccountStore *self,
								    const gchar *name)
{
	ModestTnyAccountStorePrivate *priv = NULL;
	TnyAccount * tny_account = NULL;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	/* Add the account: */
	tny_account = 
		modest_tny_account_new_from_server_account_name (priv->account_mgr, 
								 priv->session, 
								 name,
								 get_password,
								 forget_password);
	if (tny_account) {
		g_object_set_data (G_OBJECT(tny_account), 
				   "account_store", 
				   (gpointer)self);
		
		tny_list_append (priv->transport_accounts, G_OBJECT (tny_account));
		add_outbox_from_transport_account_to_global_outbox (self, 
								    name, 
								    tny_account);
		
	} else
		g_printerr ("modest: failed to create smtp-specific account for %s\n",
			    name);

	return TNY_TRANSPORT_ACCOUNT (tny_account);
}


static void
add_connection_specific_transport_accounts (ModestTnyAccountStore *self)
{
	ModestTnyAccountStorePrivate *priv = NULL;
	GSList *list_specifics = NULL, *iter = NULL;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	ModestConf *conf = modest_runtime_get_conf ();

	GError *err = NULL;
	list_specifics = modest_conf_get_list (conf,
					       MODEST_CONF_CONNECTION_SPECIFIC_SMTP_LIST,
					       MODEST_CONF_VALUE_STRING, &err);
	if (err) {
		g_printerr ("modest: %s: error getting list: %s\n.", __FUNCTION__, err->message);
		g_error_free (err);
		err = NULL;
	}
				
	/* Look at each connection-specific transport account for the 
	 * modest account: */
	iter = list_specifics;
	while (iter) {
		/* The list alternates between the connection name and the transport name: */
		iter = g_slist_next (iter);
		if (iter) {
			const gchar* transport_account_name = (const gchar*) (iter->data);
			TnyTransportAccount * account = NULL;
			account = modest_tny_account_store_new_connection_specific_transport_account (
				self, transport_account_name);
			if (account)
				g_object_unref (account);
		}				
		iter = g_slist_next (iter);
	}
}

TnyMsg *
modest_tny_account_store_find_msg_in_outboxes (ModestTnyAccountStore *self, 
					       const gchar *uri,
					       TnyAccount **ac_out)
{
	TnyIterator *acc_iter;
	ModestTnyAccountStorePrivate *priv;
	TnyMsg *msg = NULL;
	TnyAccount *msg_account = NULL;

	g_return_val_if_fail (MODEST_IS_TNY_ACCOUNT_STORE (self), NULL);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self);

	acc_iter = tny_list_create_iterator (priv->store_accounts_outboxes);
	while (!msg && !tny_iterator_is_done (acc_iter)) {
		TnyList *folders = tny_simple_list_new ();
		TnyAccount *account = TNY_ACCOUNT (tny_iterator_get_current (acc_iter));
		TnyIterator *folders_iter = NULL;

		tny_folder_store_get_folders (TNY_FOLDER_STORE (account), folders, NULL, NULL);
		folders_iter = tny_list_create_iterator (folders);

		while (msg == NULL && !tny_iterator_is_done (folders_iter)) {
			TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (folders_iter));
			msg = tny_folder_find_msg (folder, uri, NULL);

			if (msg)
				msg_account = g_object_ref (account);

			g_object_unref (folder);
			tny_iterator_next (folders_iter);
		}
		g_object_unref (folders_iter);

		g_object_unref (folders);
		g_object_unref (account);
		tny_iterator_next (acc_iter);
	}

	g_object_unref (acc_iter);

	if (ac_out != NULL)
		*ac_out = msg_account;

	return msg;
}

TnyTransportAccount *
modest_tny_account_store_get_transport_account_from_outbox_header(ModestTnyAccountStore *self, TnyHeader *header)
{
	TnyIterator *acc_iter;
	ModestTnyAccountStorePrivate *priv;
	TnyTransportAccount *header_acc = NULL;
	const gchar *msg_id;

	g_return_val_if_fail (MODEST_IS_TNY_ACCOUNT_STORE (self), NULL);
	g_return_val_if_fail (TNY_IS_HEADER (header), NULL);
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self);

	msg_id = modest_tny_send_queue_get_msg_id (header);
	acc_iter = tny_list_create_iterator (priv->transport_accounts);
	while (!header_acc && !tny_iterator_is_done (acc_iter)) {
		TnyTransportAccount *account = TNY_TRANSPORT_ACCOUNT (tny_iterator_get_current (acc_iter));
		ModestTnySendQueue *send_queue;
		ModestTnySendQueueStatus status;
		send_queue = modest_runtime_get_send_queue(TNY_TRANSPORT_ACCOUNT(account));
		status = modest_tny_send_queue_get_msg_status(send_queue, msg_id);
		if (status != MODEST_TNY_SEND_QUEUE_UNKNOWN) {
			header_acc = g_object_ref(account);
		}
		g_object_unref (account);
		tny_iterator_next (acc_iter);
	}
	g_object_unref(acc_iter);

	/* New reference */
	return header_acc;
}

GtkWidget *
modest_tny_account_store_show_account_settings_dialog (ModestTnyAccountStore *self,
						      const gchar *account_name)
{
	ModestTnyAccountStorePrivate *priv;
	gpointer dialog_as_gpointer = NULL;
	gboolean found;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	found = g_hash_table_lookup_extended (priv->account_settings_dialog_hash,
					      account_name, NULL, (gpointer*)&dialog_as_gpointer);

	if (found)
		return (GtkWidget *) dialog_as_gpointer;
	else {
		ModestAccountSettings *settings;
		GtkWidget *dialog;
		dialog = (GtkWidget *) modest_account_settings_dialog_new ();
		settings = modest_account_mgr_load_account_settings (priv->account_mgr, account_name);
		modest_account_settings_dialog_set_account (MODEST_ACCOUNT_SETTINGS_DIALOG (dialog), settings);
		g_object_unref (settings);
		modest_account_settings_dialog_switch_to_user_info (MODEST_ACCOUNT_SETTINGS_DIALOG (dialog));
		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), GTK_WINDOW (dialog));
		
		g_hash_table_insert (priv->account_settings_dialog_hash, g_strdup (account_name), dialog);
		
		g_signal_connect (G_OBJECT (dialog), "hide", G_CALLBACK (on_account_settings_hide), 
				  g_strdup (account_name));
			
		/* Show it and delete it when it closes: */
		g_signal_connect_swapped (dialog, 
					  "response", 
					  G_CALLBACK (gtk_widget_destroy), 
					  dialog);
		gtk_widget_show (GTK_WIDGET (dialog));

		return dialog;
	}
	
}

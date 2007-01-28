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
#include <tny-device.h>
#include <tny-account-store.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <modest-marshal.h>
#include <modest-protocol-info.h>
#include <modest-local-folder-info.h>

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
static void    modest_tny_account_store_init                     (gpointer g, gpointer iface_data);


/* list my signals */
enum {
	ACCOUNT_UPDATE_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyAccountStorePrivate ModestTnyAccountStorePrivate;
struct _ModestTnyAccountStorePrivate {

	gchar              *cache_dir;
	
	GHashTable         *password_hash;
	TnyDevice          *device;
	TnySessionCamel    *tny_session_camel;

	ModestAccountMgr   *account_mgr;
	TnyAccount         *local_folders;
};

#define MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                      MODEST_TYPE_TNY_ACCOUNT_STORE, \
                                                      ModestTnyAccountStorePrivate))

static void    on_password_requested        (ModestTnyAccountStore *account_store, 
					     const gchar* account_name,
					     gchar **password, 
					     gboolean *cancel, 
					     gboolean *remember);

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
	
}


static void
modest_tny_account_store_instance_init (ModestTnyAccountStore *obj)
{
	ModestTnyAccountStorePrivate *priv =
		MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->cache_dir              = NULL;
	priv->account_mgr            = NULL;
	priv->tny_session_camel      = NULL;
	priv->device                 = NULL;
	
	priv->password_hash          = g_hash_table_new_full (g_str_hash, g_str_equal,
							      g_free, g_free);

	priv->local_folders          = NULL;
}


/* we need these dummy functions, or tinymail will complain */
static gchar*
get_password_dummy (TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	return NULL;
}
static void
forget_password_dummy (TnyAccount *account)
{
	return;
}
	
/* create a pseudo-account for our local folders */
static TnyAccount*
get_local_folders_account (ModestTnyAccountStore *self)
{
	TnyStoreAccount *tny_account;
	CamelURL *url;
	gchar *maildir, *url_string;
	ModestTnyAccountStorePrivate *priv;

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	tny_account = tny_camel_store_account_new ();
	if (!tny_account) {
		g_printerr ("modest: cannot create account for local folders");
		return NULL;
	}
	
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account),priv->tny_session_camel);
	
	maildir = modest_local_folder_info_get_maildir_path ();
	url = camel_url_new ("maildir:", NULL);
	camel_url_set_path (url, maildir);
	url_string = camel_url_to_string (url, 0);
	
	tny_account_set_url_string (TNY_ACCOUNT(tny_account), url_string);
	tny_account_set_name (TNY_ACCOUNT(tny_account), MODEST_LOCAL_FOLDERS_ACCOUNT_NAME); 
	tny_account_set_id (TNY_ACCOUNT(tny_account), MODEST_LOCAL_FOLDERS_ACCOUNT_NAME); 
	tny_account_set_pass_func (TNY_ACCOUNT(tny_account), get_password_dummy);
        tny_account_set_forget_pass_func (TNY_ACCOUNT(tny_account), forget_password_dummy);

	camel_url_free (url);
	g_free (maildir);
	g_free (url_string);

	return TNY_ACCOUNT(tny_account);
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
	return MODEST_TNY_ACCOUNT_STORE(g_object_get_data (G_OBJECT(account),
							   "account_store"));
}



static void
set_account_store_for_account (TnyAccount *account, ModestTnyAccountStore *store)
{
	g_object_set_data (G_OBJECT(account), "account_store", (gpointer)store);
}

static void
on_password_requested (ModestTnyAccountStore *account_store, 
		       const gchar* account_name,
		       gchar **password, 
		       gboolean *cancel, 
		       gboolean *remember)
{
	gchar *txt;
	GtkWidget *dialog, *entry, *remember_pass_check;

	dialog = gtk_dialog_new_with_buttons (_("Password requested"),
					      NULL,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);

	txt = g_strdup_printf (_("Please enter your password for %s"), account_name);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), gtk_label_new(txt),
			    FALSE, FALSE, 0);
	g_free (txt);

	entry = gtk_entry_new_with_max_length (40);
	gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(entry), 0x2022); /* bullet unichar */
	
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), entry,
			    TRUE, FALSE, 0);	

	remember_pass_check = gtk_check_button_new_with_label (_("Remember password"));
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), remember_pass_check,
			    TRUE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	
	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		*password = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry)));
		*cancel   = FALSE;
	} else {
		*password = NULL;
		*cancel   = TRUE;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (remember_pass_check)))
		*remember = TRUE;
	else
		*remember = FALSE;

	gtk_widget_destroy (dialog);

	while (gtk_events_pending ())
		gtk_main_iteration ();
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
						      key, MODEST_ACCOUNT_PASSWORD,
						      TRUE, NULL);
		g_hash_table_insert (priv->password_hash, g_strdup (key), g_strdup (pwd));
	}

	/* if it was already asked, it must have been wrong, so ask again */
	if (already_asked || !pwd || strlen(pwd) == 0) {

		/* we don't have it yet. Get the password from the user */
		const gchar* name = tny_account_get_name (account);
		gboolean remember;
		pwd = NULL;

		on_password_requested (self, name, &pwd, cancel, &remember);

		if (!*cancel) {
			if (remember)
				modest_account_mgr_set_string (priv->account_mgr,
							       key, MODEST_ACCOUNT_PASSWORD,
							       pwd,
							       TRUE, NULL);
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
				  key, MODEST_ACCOUNT_PASSWORD,
				  TRUE, NULL);
}



/* instantiate the correct tny account subclass */
static TnyAccount*
tny_account_for_proto (ModestProtocol proto) 
{
	ModestProtocolType type;	
	TnyAccount *tny_account = NULL;
	
	type  = modest_protocol_info_get_protocol_type (proto);
	
	if (type == MODEST_PROTOCOL_TYPE_TRANSPORT) 
		tny_account = TNY_ACCOUNT(tny_camel_transport_account_new ());
	else if (proto == MODEST_PROTOCOL_STORE_POP)
		tny_account = TNY_ACCOUNT(tny_camel_pop_store_account_new ());
	else if (proto == MODEST_PROTOCOL_STORE_IMAP)
		tny_account = TNY_ACCOUNT(tny_camel_imap_store_account_new ());
	else 
		g_return_val_if_reached (NULL);
	
	if (tny_account)
		tny_account_set_proto (tny_account,
				       modest_protocol_info_get_protocol_name(proto));
	else
		g_printerr ("modest: could not get tny account for %d\n",
			    proto);    
	return tny_account;
}


/* create a tnyaccount for the server account connected to the account with name 'key'
 */
static TnyAccount*
get_tny_account_from_server_account (ModestTnyAccountStore *self,
				     ModestServerAccountData *account_data,
				     ModestProtocolType modest_type)
{
	TnyAccount *tny_account;
	ModestTnyAccountStorePrivate *priv;
		
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (account_data, NULL);

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	/* proto */
	if (account_data->proto == MODEST_PROTOCOL_UNKNOWN) {
		g_printerr ("modest: '%s' does not provide a protocol\n",
			    account_data->account_name);
		return NULL;
	}
		
	tny_account = tny_account_for_proto (account_data->proto);
	if (!tny_account) {
		g_printerr ("modest: could not create tny account for '%s'\n",
			    account_data->account_name);
		return NULL;
	}
	
	/* Set account store, session and id */
	set_account_store_for_account (TNY_ACCOUNT(tny_account), self);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), 	/* session */
				       priv->tny_session_camel);
	tny_account_set_id (tny_account, account_data->account_name); /* id */

	/* Options */
	if (account_data->options) {
		GSList *tmp = account_data->options;
		while (tmp) {
			tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (tny_account),
						      tmp->data);
			tmp = g_slist_next (tmp);
		}
	}
	/* Hostname & Username */
	if (account_data->username) 
		tny_account_set_user (tny_account, account_data->username);

	if (account_data->hostname)
		tny_account_set_hostname (tny_account, account_data->hostname);

	/* Password functions */
	tny_account_set_pass_func (tny_account, get_password);
        tny_account_set_forget_pass_func (tny_account, forget_password);

	return tny_account;
}



static void
modest_tny_account_store_finalize (GObject *obj)
{
	ModestTnyAccountStore *self        = MODEST_TNY_ACCOUNT_STORE(obj);
	ModestTnyAccountStorePrivate *priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);

	if (priv->tny_session_camel) {
		camel_object_unref (CAMEL_OBJECT(priv->tny_session_camel));
		priv->tny_session_camel = NULL;
	}

	g_free (priv->cache_dir);
	priv->cache_dir = NULL;

	if (priv->device) {
		g_object_unref (priv->device);
		priv->device = NULL;
	}
	
	if (priv->password_hash) {
		g_hash_table_destroy (priv->password_hash);
		priv->password_hash = NULL;
	}

	if (priv->account_mgr) {
		g_object_unref (priv->account_mgr);
		priv->account_mgr = NULL;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestTnyAccountStore*
modest_tny_account_store_new (ModestAccountMgr *account_mgr) {

	GObject *obj;
	ModestTnyAccountStorePrivate *priv;
	
	g_return_val_if_fail (account_mgr, NULL);

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_ACCOUNT_STORE, NULL));
	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(obj);

	priv->account_mgr = account_mgr;
	g_object_ref (G_OBJECT(priv->account_mgr));
	
	/* The session needs the platform factory */
	priv->tny_session_camel = tny_session_camel_new (TNY_ACCOUNT_STORE(obj));
	if (!priv->tny_session_camel) {
		g_printerr ("modest: cannot create TnySessionCamel instance\n");
		g_object_unref (obj);
		return NULL;
	}
	
	tny_session_camel_set_ui_locker (priv->tny_session_camel, tny_gtk_lockable_new ());
	/* FIXME: unref this in the end? */
	
	/* Connect signals */
	g_signal_connect (G_OBJECT(account_mgr), "account_changed",
				       G_CALLBACK (on_account_changed), obj);
	g_signal_connect (G_OBJECT(account_mgr), "account_removed",
				       G_CALLBACK (on_account_removed), obj);

	return MODEST_TNY_ACCOUNT_STORE(obj);
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



static TnyAccount*
get_tny_account_from_account (ModestTnyAccountStore *self, ModestAccountData *account_data,
			      TnyGetAccountsRequestType type) 
{
	TnyAccount *tny_account = NULL;
	ModestServerAccountData *server_account = NULL;

	if (type == TNY_ACCOUNT_STORE_STORE_ACCOUNTS && account_data->store_account)
		server_account = account_data->store_account;
	else if (type == TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS && account_data->transport_account)
		server_account = account_data->transport_account;
	
	if (!server_account) {
		g_printerr ("modest: no %s account defined for '%s'\n",
			    type == TNY_ACCOUNT_STORE_STORE_ACCOUNTS ? "store" : "transport",
			    account_data->display_name);
		return NULL;
	}
	
	tny_account = get_tny_account_from_server_account (self, server_account, type);
	if (!tny_account) { 
		g_printerr ("modest: failed to create tny account for %s\n",
			    account_data->account_name);
		return NULL;
	}
	
	if (account_data->display_name)
		tny_account_set_name (tny_account, account_data->display_name); 
	
	return tny_account;
}


static void
modest_tny_account_store_get_accounts  (TnyAccountStore *account_store, TnyList *list,
					TnyGetAccountsRequestType type)
{
	ModestTnyAccountStore        *self;
	ModestTnyAccountStorePrivate *priv;
	GSList                       *accounts, *cursor;
	
	g_return_if_fail (account_store);
	g_return_if_fail (TNY_IS_LIST(list));

	self        = MODEST_TNY_ACCOUNT_STORE(account_store);
	priv        = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self);
	
	if (type == TNY_ACCOUNT_STORE_BOTH) {
		modest_tny_account_store_get_accounts (account_store, list,
						       TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
		modest_tny_account_store_get_accounts (account_store, list,
						       TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);
	}

	accounts = modest_account_mgr_account_names (priv->account_mgr, NULL); 
	for (cursor = accounts; cursor; cursor = cursor->next) {
		TnyAccount *tny_account = NULL;
		ModestAccountData *account_data =
			modest_account_mgr_get_account_data (priv->account_mgr, 
		 					     (gchar*)cursor->data);
		if (account_data && account_data->enabled) {
			tny_account = get_tny_account_from_account (self, account_data, type);
			if (tny_account)
				tny_list_prepend (list, G_OBJECT(tny_account));
		}
		g_free (cursor->data);
		modest_account_mgr_free_account_data (priv->account_mgr, account_data);
	}
	g_slist_free (accounts);

	/* also, add the local folder pseudo-account */
	if (type != TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS) {
		if (!priv->local_folders)
			priv->local_folders = get_local_folders_account (self);
		if (!priv->local_folders)
			g_printerr ("modest: no local folders account\n");
		else
			tny_list_prepend (list, G_OBJECT(priv->local_folders));
	}
	tny_session_camel_set_account_store (priv->tny_session_camel, account_store);
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

	priv = MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self);

	if (!priv->device) 
		priv->device = tny_platform_factory_new_device
			(modest_tny_platform_factory_get_instance());
	
	return g_object_ref (G_OBJECT(priv->device));
}



static gboolean
modest_tny_account_store_alert (TnyAccountStore *self, TnyAlertType type,
				const gchar *prompt)
{
	g_printerr ("modest: alert_func not implemented (%d, %s)\n",
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
	/* not implemented, we use signals */
	g_printerr ("modest: set_get_pass_func not implemented\n");
}

TnySessionCamel*
tny_account_store_get_session  (TnyAccountStore *self)
{
	g_return_val_if_fail (self, NULL);	
	return MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE(self)->tny_session_camel;
}


TnyAccount*
modest_tny_account_store_get_local_folders_account    (ModestTnyAccountStore *self)
{
	g_return_val_if_fail (self, NULL);
	
	return MODEST_TNY_ACCOUNT_STORE_GET_PRIVATE (self)->local_folders;
}


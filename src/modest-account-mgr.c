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
#include <modest-marshal.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-priv.h>
#include <modest-account-mgr-helpers.h>

/* 'private'/'protected' functions */
static void modest_account_mgr_class_init (ModestAccountMgrClass * klass);
static void modest_account_mgr_init       (ModestAccountMgr * obj);
static void modest_account_mgr_finalize   (GObject * obj);

/* list my signals */
enum {
	ACCOUNT_CHANGED_SIGNAL,
	ACCOUNT_REMOVED_SIGNAL,
	LAST_SIGNAL
};


/* globals */
static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = {0};

static void
on_key_change (ModestConf *conf, const gchar *key, ModestConfEvent event, gpointer user_data)
{
	ModestAccountMgr *self;
	ModestAccountMgrPrivate *priv;

	gchar *account;
	gboolean is_account_key, is_server_account;
	gboolean enabled;

	self = MODEST_ACCOUNT_MGR (user_data);
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	
	account = _modest_account_mgr_account_from_key (key, &is_account_key, &is_server_account);

	/* if this is not an account-related key change, ignore */
	if (!account)
		return;

	/* account was removed -- emit this, even if the account was disabled */
	if (is_account_key && event == MODEST_CONF_EVENT_KEY_UNSET) {
		g_signal_emit (G_OBJECT(self), signals[ACCOUNT_REMOVED_SIGNAL], 0,
			       account, is_server_account);
		g_free (account);
		return;
	}

	/* is this account enabled? */
	if (is_server_account)
		enabled = TRUE;
	else 
		enabled = modest_account_mgr_get_enabled (self, account);

	/* account was changed.
	 * and always notify when enabled/disabled changes
	 */
	if (enabled || g_str_has_suffix (key, MODEST_ACCOUNT_ENABLED)) 
		g_signal_emit (G_OBJECT(self), signals[ACCOUNT_CHANGED_SIGNAL], 0,
			       account, key, is_server_account);

	g_free (account);
}


GType
modest_account_mgr_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof (ModestAccountMgrClass),
			NULL,	/* base init */
			NULL,	/* base finalize */
			(GClassInitFunc) modest_account_mgr_class_init,
			NULL,	/* class finalize */
			NULL,	/* class data */
			sizeof (ModestAccountMgr),
			1,	/* n_preallocs */
			(GInstanceInitFunc) modest_account_mgr_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestAccountMgr",
						  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_mgr_class_init (ModestAccountMgrClass * klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_mgr_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof (ModestAccountMgrPrivate));

	/* signal definitions */
	signals[ACCOUNT_REMOVED_SIGNAL] =
 		g_signal_new ("account_removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestAccountMgrClass,account_removed),
			      NULL, NULL,
			      modest_marshal_VOID__STRING_BOOLEAN,
			      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);
	signals[ACCOUNT_CHANGED_SIGNAL] =
 		g_signal_new ("account_changed",
	                       G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestAccountMgrClass,account_changed),
			      NULL, NULL,
			      modest_marshal_VOID__STRING_STRING_BOOLEAN,
			      G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
}


static void
modest_account_mgr_init (ModestAccountMgr * obj)
{
	ModestAccountMgrPrivate *priv =
		MODEST_ACCOUNT_MGR_GET_PRIVATE (obj);

	priv->modest_conf = NULL;
}

static void
modest_account_mgr_finalize (GObject * obj)
{
	ModestAccountMgrPrivate *priv =
		MODEST_ACCOUNT_MGR_GET_PRIVATE (obj);

	if (priv->modest_conf) {
		g_object_unref (G_OBJECT(priv->modest_conf));
		priv->modest_conf = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestAccountMgr *
modest_account_mgr_new (ModestConf * conf)
{
	GObject *obj;
	ModestAccountMgrPrivate *priv;

	g_return_val_if_fail (conf, NULL);

	obj = G_OBJECT (g_object_new (MODEST_TYPE_ACCOUNT_MGR, NULL));
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (obj);

	g_object_ref (G_OBJECT(conf));
	priv->modest_conf = conf;

	g_signal_connect (G_OBJECT (conf), "key_changed",
	                  G_CALLBACK (on_key_change),
			  obj);
	
	return MODEST_ACCOUNT_MGR (obj);
}


static const gchar *
null_means_empty (const gchar * str)
{
	return str ? str : "";
}


gboolean
modest_account_mgr_add_account (ModestAccountMgr *self,
				const gchar *name,
				const gchar *store_account,
				const gchar *transport_account,
				GError **err)
{
	ModestAccountMgrPrivate *priv;
	gchar *key;
	gboolean ok;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (strchr(name, '/') == NULL, FALSE);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	
	/*
	 * we create the account by adding an account 'dir', with the name <name>,
	 * and in that the 'display_name' string key
	 */
	key = _modest_account_mgr_get_account_keyname (name, MODEST_ACCOUNT_DISPLAY_NAME, FALSE);
	if (modest_account_mgr_account_exists (self, key, FALSE, err)) {
		g_printerr ("modest: account already exists\n");
		g_free (key);
		return FALSE;
	}
	
	ok = modest_conf_set_string (priv->modest_conf, key, name, err);
	g_free (key);
	if (!ok) {
		g_printerr ("modest: cannot set display name\n");
		return FALSE;
	}
	
	if (store_account) {
		key = _modest_account_mgr_get_account_keyname (name, MODEST_ACCOUNT_STORE_ACCOUNT, FALSE);
		ok = modest_conf_set_string (priv->modest_conf, key, store_account, err);
		g_free (key);
		if (!ok) {
			g_printerr ("modest: failed to set store account '%s'\n",
				store_account);
			return FALSE;
		}
	}

	if (transport_account) {
		key = _modest_account_mgr_get_account_keyname (name, MODEST_ACCOUNT_TRANSPORT_ACCOUNT, FALSE);
		ok = modest_conf_set_string (priv->modest_conf, key, transport_account, err);
		g_free (key);
		if (!ok) {
			g_printerr ("modest: failed to set transport account '%s'\n",
				transport_account);
			return FALSE;
		}
	}
	modest_account_mgr_set_enabled (self, name, TRUE);
	
	return TRUE;
}




gboolean
modest_account_mgr_add_server_account (ModestAccountMgr * self,
				       const gchar * name, const gchar *hostname,
				       const gchar * username, const gchar * password,
				       ModestProtocol proto)
{
	ModestAccountMgrPrivate *priv;
	gchar *key;
	ModestProtocolType proto_type;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (strchr(name, '/') == NULL, FALSE);

	proto_type = modest_protocol_info_get_protocol_type (proto);
	g_return_val_if_fail (proto_type == MODEST_PROTOCOL_TYPE_TRANSPORT ||
			      proto_type == MODEST_PROTOCOL_TYPE_STORE, FALSE);
			      
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	
	/* hostname */
	key = _modest_account_mgr_get_account_keyname (name, MODEST_ACCOUNT_HOSTNAME, TRUE);
	if (modest_conf_key_exists (priv->modest_conf, key, NULL)) {
		g_printerr ("modest: server account '%s' already exists", name);
		g_free (key);
		return FALSE;
	}
	
	modest_conf_set_string (priv->modest_conf, key, null_means_empty(hostname), NULL);
	g_free (key);

	/* username */
	key = _modest_account_mgr_get_account_keyname (name, MODEST_ACCOUNT_USERNAME, TRUE);
	modest_conf_set_string (priv->modest_conf, key,	null_means_empty (username), NULL);
	g_free (key);

	/* password */
	key = _modest_account_mgr_get_account_keyname (name, MODEST_ACCOUNT_PASSWORD, TRUE);
	modest_conf_set_string (priv->modest_conf, key,	null_means_empty (password), NULL);
	g_free (key);

	/* proto */
	key = _modest_account_mgr_get_account_keyname (name, MODEST_ACCOUNT_PROTO, TRUE);
	modest_conf_set_string (priv->modest_conf, key,
				modest_protocol_info_get_protocol_name(proto),
				NULL);
	g_free (key);
	
	return TRUE;
}



gboolean
modest_account_mgr_remove_account (ModestAccountMgr * self,
				   const gchar * name,
				   gboolean server_account,
				   GError ** err)
{
	ModestAccountMgrPrivate *priv;
	gchar *key;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);

	if (!modest_account_mgr_account_exists (self, name, server_account, err)) {
		g_printerr ("modest: account '%s' does not exist\n", name);
		return FALSE;
	}

	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	key = _modest_account_mgr_get_account_keyname (name, NULL, server_account);

	retval = modest_conf_remove_key (priv->modest_conf, key, NULL);

	g_free (key);
	return retval;
}



/* strip the first /n/ character from each element
 * caller must make sure all elements are strings with
 * length >= n, and also that data can be freed.
 * change is in-place
 */
static void
strip_prefix_from_elements (GSList * lst, guint n)
{
	while (lst) {
		memmove (lst->data, lst->data + n,
			 strlen(lst->data) - n + 1);
		lst = lst->next;
	}
}


GSList*
modest_account_mgr_search_server_accounts (ModestAccountMgr * self,
					   const gchar * account_name,
					   ModestProtocolType type,
					   ModestProtocol proto)
{
	GSList *accounts;
	GSList *cursor;
	ModestAccountMgrPrivate *priv;
	gchar *key;
	GError *err = NULL;
	
	g_return_val_if_fail (self, NULL);

	if (proto != MODEST_PROTOCOL_UNKNOWN) {
		ModestProtocolType proto_type;
		proto_type = modest_protocol_info_get_protocol_type (proto);
		g_return_val_if_fail (proto_type == MODEST_PROTOCOL_TYPE_TRANSPORT ||
				      proto_type == MODEST_PROTOCOL_TYPE_STORE, NULL);
	}
	
	key      = _modest_account_mgr_get_account_keyname (account_name, NULL, TRUE);
	priv     = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	
	/* get the list of all server accounts */
	accounts = modest_conf_list_subkeys (priv->modest_conf, key, &err);
	if (err) {
		g_error_free (err);
		g_printerr ("modest: failed to get subkeys for '%s'\n", key);
		return NULL;
	}
	
	/* filter out the ones with the wrong protocol */
	/* we could optimize for unknown proto / unknown type, but it will only
	 * make the code more complex */
	cursor = accounts;
	while (cursor) { 
		gchar *account   = _modest_account_mgr_account_from_key ((gchar*)cursor->data, NULL, NULL);
		gchar *acc_proto = modest_account_mgr_get_string (self, account, MODEST_ACCOUNT_PROTO,
								  TRUE, NULL);
		ModestProtocol     this_proto = modest_protocol_info_get_protocol (acc_proto);
		ModestProtocolType this_type  = modest_protocol_info_get_protocol_type (this_proto);

		if ((this_type  != MODEST_PROTOCOL_TYPE_UNKNOWN && this_type  != type) ||
		    (this_proto != MODEST_PROTOCOL_UNKNOWN      && this_proto != proto)) {
			GSList *nxt = cursor->next;
			accounts = g_slist_delete_link (accounts, cursor);
			cursor = nxt;
		} else
			cursor = cursor->next;
		
		g_free (account);
		g_free (acc_proto);
	}
	
	/* +1 because we must remove the ending '/' as well */
	strip_prefix_from_elements (accounts, strlen(key)+1);
	return accounts;	
}


GSList*
modest_account_mgr_account_names (ModestAccountMgr * self, GError ** err)
{
	GSList *accounts;
	ModestAccountMgrPrivate *priv;
	const size_t prefix_len = strlen (MODEST_ACCOUNT_NAMESPACE "/");

	g_return_val_if_fail (self, NULL);

	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);

	accounts = modest_conf_list_subkeys (priv->modest_conf,
                                             MODEST_ACCOUNT_NAMESPACE, err);
	
	strip_prefix_from_elements (accounts, prefix_len);
	return accounts;
}


gchar *
modest_account_mgr_get_string (ModestAccountMgr *self, const gchar *name,
			       const gchar *key, gboolean server_account, GError **err) {

	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gchar *retval;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (key, NULL);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	retval = modest_conf_get_string (priv->modest_conf, keyname, err);
	g_free (keyname);

	return retval;
}


gint
modest_account_mgr_get_int (ModestAccountMgr *self, const gchar *name,
			    const gchar *key, gboolean server_account, GError **err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gint retval;

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (name, -1);
	g_return_val_if_fail (key, -1);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);

	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	retval = modest_conf_get_int (priv->modest_conf, keyname, err);
	g_free (keyname);

	return retval;
}



gboolean
modest_account_mgr_get_bool (ModestAccountMgr * self, const gchar *account,
			     const gchar * key, gboolean server_account, GError ** err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (account, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = _modest_account_mgr_get_account_keyname (account, key, server_account);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	retval = modest_conf_get_bool (priv->modest_conf, keyname, err);
		
	g_free (keyname);

	return retval;
}


gboolean
modest_account_mgr_set_string (ModestAccountMgr * self, const gchar * name,
			       const gchar * key, const gchar * val,
			       gboolean server_account, GError ** err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);

	retval = modest_conf_set_string (priv->modest_conf, keyname, val,
					 err);

	g_free (keyname);
	return retval;
}


gboolean
modest_account_mgr_set_int (ModestAccountMgr * self, const gchar * name,
			    const gchar * key, int val, gboolean server_account,
			    GError ** err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);

	retval = modest_conf_set_int (priv->modest_conf, keyname, val, err);

	g_free (keyname);
	return retval;
}



gboolean
modest_account_mgr_set_bool (ModestAccountMgr * self, const gchar * name,
			     const gchar * key, gboolean val, gboolean server_account, 
			     GError ** err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);

	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);

	retval = modest_conf_set_bool (priv->modest_conf, keyname, val, err);

	g_free (keyname);
	return retval;
}

void 
modest_account_mgr_set_list (ModestAccountMgr *self,
			     const gchar *name,
			     const gchar *key,
			     GSList *val,
			     ModestConfValueType list_type,
			     gboolean server_account,
			     GError **err)
{
	ModestAccountMgrPrivate *priv;
	gchar *keyname;
	
	g_return_if_fail (self);
	g_return_if_fail (name);
	g_return_if_fail (key);
	g_return_if_fail (val);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	modest_conf_set_list (priv->modest_conf, keyname, val, list_type, err);
	g_free (keyname);
}

gboolean
modest_account_mgr_account_exists (ModestAccountMgr * self, const gchar * name,
				   gboolean server_account, GError ** err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
        g_return_val_if_fail (name, FALSE);

	keyname = _modest_account_mgr_get_account_keyname (name, NULL, server_account);

	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	retval = modest_conf_key_exists (priv->modest_conf, keyname, err);

	g_free (keyname);
	return retval;
}


GSList * 
modest_account_mgr_get_list (ModestAccountMgr *self,
			     const gchar *name,
			     const gchar *key,
			     ModestConfValueType list_type,
			     gboolean server_account,
			     GError **err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	GSList *retval;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (key, NULL);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	retval = modest_conf_get_list (priv->modest_conf, keyname, list_type, err);
	g_free (keyname);

	return retval;
}


gboolean 
modest_account_mgr_unset (ModestAccountMgr *self,
			  const gchar *name,
			  const gchar *key,
			  gboolean server_account,
			  GError **err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
        g_return_val_if_fail (name, FALSE);
        g_return_val_if_fail (key, FALSE);

	keyname = _modest_account_mgr_get_account_keyname (name, key, server_account);

	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE (self);
	retval = modest_conf_remove_key (priv->modest_conf, keyname, err);

	g_free (keyname);
	return retval;
}

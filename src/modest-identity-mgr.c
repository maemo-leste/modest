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


/* modest-identity-mgr.c */

#include <string.h>
#include "modest-marshal.h"
#include "modest-identity-mgr.h"

/* 'private'/'protected' functions */
static void modest_identity_mgr_class_init (ModestIdentityMgrClass * klass);
static void modest_identity_mgr_init (ModestIdentityMgr * obj);
static void modest_identity_mgr_finalize (GObject * obj);

static gchar *get_identity_keyname (const gchar * accname,
				    const gchar * name);

/* list my signals */
enum {
	IDENTITY_CHANGE_SIGNAL,
	IDENTITY_REMOVE_SIGNAL,
	IDENTITY_ADD_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestIdentityMgrPrivate ModestIdentityMgrPrivate;
struct _ModestIdentityMgrPrivate {
	ModestConf *modest_conf;
	GSList *current_identities;
};

#define MODEST_IDENTITY_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_IDENTITY_MGR, \
                                                ModestIdentityMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};


static GSList *
delete_account_from_list (GSList *list, const gchar *name)
{
	GSList *iter, *result;

	iter = list;
	result = list;
	while (iter) {
		if (!strcmp (name, iter->data)) {
			result = g_slist_delete_link (list, iter);
			break;
		}

		iter = g_slist_next (iter);
	}
	return result;
}

static GSList *
find_account_in_list (GSList *list, const gchar *name)
{
	GSList *iter, *result;

	iter = list;
	result = list;
	while (iter) {
		if (!strcmp (name, iter->data)) {
			return iter;
			break;
		}

		iter = g_slist_next (iter);
	}
	return NULL;
}


static void
modest_identity_mgr_check_change (ModestConf *conf,
				 const gchar *key,
				 const gchar *new_value,
				 gpointer userdata)
{
	ModestIdentityMgr *id_mgr = userdata;
	ModestIdentityMgrPrivate *priv = MODEST_IDENTITY_MGR_GET_PRIVATE(id_mgr);
	gchar *subkey;
	gchar *param;

	if ((strlen(key) > strlen(MODEST_IDENTITY_NAMESPACE "/")
	     && g_str_has_prefix(key, MODEST_IDENTITY_NAMESPACE))) {
		subkey = g_strdup(key + strlen(MODEST_IDENTITY_NAMESPACE "/"));
		if (! strstr(subkey, "/")) { /* no more '/' means an entry was modified */
			if (!new_value) {
				priv->current_identities =
					delete_account_from_list (priv->current_identities, subkey);
				g_signal_emit(id_mgr, signals[IDENTITY_REMOVE_SIGNAL], 0, subkey);
			}
		}
		else {
			param = strstr(subkey, "/");
			param[0] = 0;
			param++;

			if (!find_account_in_list(priv->current_identities, subkey)) {
				priv->current_identities =
					g_slist_prepend(priv->current_identities, g_strdup(subkey));
				g_signal_emit(id_mgr, signals[IDENTITY_ADD_SIGNAL], 0, subkey);
			}
			g_signal_emit(id_mgr, signals[IDENTITY_CHANGE_SIGNAL], 0, subkey, param, new_value);
		}
		g_free(subkey);
	}
}

GType
modest_identity_mgr_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof (ModestIdentityMgrClass),
			NULL,	/* base init */
			NULL,	/* base finalize */
			(GClassInitFunc) modest_identity_mgr_class_init,
			NULL,	/* class finalize */
			NULL,	/* class data */
			sizeof (ModestIdentityMgr),
			1,	/* n_preallocs */
			(GInstanceInitFunc) modest_identity_mgr_init,
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestIdentityMgr",
						  &my_info, 0);
	}
	return my_type;
}

static void
modest_identity_mgr_class_init (ModestIdentityMgrClass * klass)
{
	GObjectClass *gobject_class;
	GType paramtypes[3] = {G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER};

	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_identity_mgr_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof (ModestIdentityMgrPrivate));

	/* signal definitions */
	signals[IDENTITY_ADD_SIGNAL] =
 		g_signal_newv ("identity-add",
	                       G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		               NULL, NULL, NULL,
		               g_cclosure_marshal_VOID__POINTER,
		               G_TYPE_NONE, 1, paramtypes);

	signals[IDENTITY_REMOVE_SIGNAL] =
 		g_signal_newv ("identity-remove",
	                       G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		               NULL, NULL, NULL,
		               g_cclosure_marshal_VOID__POINTER,
		               G_TYPE_NONE, 1, paramtypes);
	signals[IDENTITY_CHANGE_SIGNAL] =
 		g_signal_newv ("identity-change",
	                       G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		               NULL, NULL, NULL,
		               modest_marshal_VOID__POINTER_POINTER_POINTER,
		               G_TYPE_NONE, 3, paramtypes);
}


static void
modest_identity_mgr_init (ModestIdentityMgr * obj)
{
	ModestIdentityMgrPrivate *priv =
		MODEST_IDENTITY_MGR_GET_PRIVATE (obj);
	priv->modest_conf = NULL;
}

static void
modest_identity_mgr_finalize (GObject * obj)
{
	ModestIdentityMgr *self = MODEST_IDENTITY_MGR (obj);
	ModestIdentityMgrPrivate *priv =
		MODEST_IDENTITY_MGR_GET_PRIVATE (self);

	g_object_unref (G_OBJECT (priv->modest_conf));
	priv->modest_conf = NULL;
}

GObject *
modest_identity_mgr_new (ModestConf * conf)
{
	GObject *obj;
	ModestIdentityMgrPrivate *priv;

	g_return_val_if_fail (conf, NULL);

	obj = G_OBJECT (g_object_new (MODEST_TYPE_IDENTITY_MGR, NULL));
	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (obj);

	/*
	 * increase the ref count on the modest_conf. Normally, the
	 * ModestConf should outlive the ModestIdentityMgr though
	 */
	g_object_ref (G_OBJECT (priv->modest_conf = conf));

	priv->current_identities = modest_identity_mgr_identity_names (MODEST_IDENTITY_MGR(obj), NULL);

	g_signal_connect(G_OBJECT(conf),
			 "key-changed",
			 G_CALLBACK (modest_identity_mgr_check_change),
			 obj);
	return obj;
}


gboolean
modest_identity_mgr_remove_identity (ModestIdentityMgr * self,
				     const gchar * name, GError ** err)
{
	ModestIdentityMgrPrivate *priv;
	gchar *key;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);

	if (!modest_identity_mgr_identity_exists (self, name, err)) {
		g_warning ("identity doest not exist");
		return FALSE;
	}

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);
	key = get_identity_keyname (name, NULL);

	retval = modest_conf_remove_key (priv->modest_conf, key, NULL);

	g_free (key);
	return retval;
}


static const gchar *
null_means_empty (const gchar * str)
{
	return str ? str : "";
}


gboolean
modest_identity_mgr_add_identity (ModestIdentityMgr * self,
				  const gchar    * name,
				  const gchar    * realname,
				  const gchar    * email,
				  const gchar    * replyto,
				  const gchar    * signature,
				  const gboolean use_signature,
				  const gchar    * id_via,
				  const gboolean use_id_via)
{
	ModestIdentityMgrPrivate *priv;
	gchar *id_key, *key;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);

	/* TODO: check already exists */

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);
	id_key = g_strconcat (MODEST_IDENTITY_NAMESPACE, "/", name, NULL);

	if (modest_conf_key_exists (priv->modest_conf, id_key, NULL)) {
		g_warning ("identity %s already exists", name);
		//g_free (id_key);
		//return FALSE;
	}

	/* realname */
	key = g_strconcat (id_key, "/", MODEST_IDENTITY_REALNAME, NULL);
	modest_conf_set_string (priv->modest_conf, key,
				null_means_empty (realname), NULL);
	g_free (key);

	/* email */
	key = g_strconcat (id_key, "/", MODEST_IDENTITY_EMAIL, NULL);
	modest_conf_set_string (priv->modest_conf, key,
				null_means_empty (email), NULL);
	g_free (key);

	/* replyto */
	key = g_strconcat (id_key, "/", MODEST_IDENTITY_REPLYTO, NULL);
	modest_conf_set_string (priv->modest_conf, key,
				null_means_empty (replyto), NULL);
	g_free (key);

	/* signature */
	key = g_strconcat (id_key, "/", MODEST_IDENTITY_SIGNATURE, NULL);
	modest_conf_set_string (priv->modest_conf, key,
				null_means_empty (signature), NULL);
	g_free (key);

	/* use_signature */
	key = g_strconcat (id_key, "/", MODEST_IDENTITY_USE_SIGNATURE, NULL);
	modest_conf_set_bool (priv->modest_conf, key, use_signature, NULL);
	g_free (key);

	/* id_via */
	key = g_strconcat (id_key, "/", MODEST_IDENTITY_ID_VIA, NULL);
	modest_conf_set_string (priv->modest_conf, key,
				null_means_empty (id_via), NULL);
	g_free (key);

	/* use_id_via */
	key = g_strconcat (id_key, "/", MODEST_IDENTITY_USE_ID_VIA, NULL);
	modest_conf_set_bool (priv->modest_conf, key, use_id_via, NULL);
	g_free (key);

	g_free (id_key);

	return TRUE;		/* FIXME: better error checking */
}

/* strip the first /n/ character from each element */
/* caller must make sure all elements are strings with
 * length >= n, and also that data can be freed.
 */
/* this function is copied from modest-account-mgr. Maybe it should be moved
 * to modest-list-utils or the like...
 */
static GSList *
strip_prefix_from_elements (GSList * lst, guint n)
{
	GSList *cursor = lst;

	while (cursor) {
		gchar *str = (gchar *) cursor->data;

		cursor->data = g_strdup (str + n);
		g_free (str);
		cursor = cursor->next;
	}
	return lst;
}

GSList *
modest_identity_mgr_identity_names (ModestIdentityMgr * self, GError ** err)
{
	GSList *identities;
	ModestIdentityMgrPrivate *priv;
	const size_t prefix_len = strlen (MODEST_IDENTITY_NAMESPACE "/");


	g_return_val_if_fail (self, NULL);

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);

	identities = modest_conf_list_subkeys (priv->modest_conf,
					       MODEST_IDENTITY_NAMESPACE,
					       err);
	return strip_prefix_from_elements (identities, prefix_len);
}


gchar *
modest_identity_mgr_get_identity_string (ModestIdentityMgr * self,
					 const gchar * name,
					 const gchar * key, GError ** err)
{
	ModestIdentityMgrPrivate *priv;

	gchar *keyname;
	gchar *retval;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (key, NULL);

	keyname = get_identity_keyname (name, key);

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);
	retval = modest_conf_get_string (priv->modest_conf, keyname, err);
	g_free (keyname);

	return retval;
}


gint
modest_identity_mgr_get_identity_int (ModestIdentityMgr * self,
				      const gchar * name, const gchar * key,
				      GError ** err)
{
	ModestIdentityMgrPrivate *priv;

	gchar *keyname;
	gint retval;

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (name, -1);
	g_return_val_if_fail (key, -1);

	keyname = get_identity_keyname (name, key);
	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);
	retval = modest_conf_get_int (priv->modest_conf, keyname, err);
	g_free (keyname);

	return retval;
}



gboolean
modest_identity_mgr_get_identity_bool (ModestIdentityMgr * self,
				       const gchar * name, const gchar * key,
				       GError ** err)
{
	ModestIdentityMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = get_identity_keyname (name, key);

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);
	retval = modest_conf_get_bool (priv->modest_conf, keyname, err);
	g_free (keyname);

	return retval;
}


gboolean
modest_identity_mgr_set_identity_string (ModestIdentityMgr * self,
					 const gchar * name,
					 const gchar * key, const gchar * val,
					 GError ** err)
{
	ModestIdentityMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = get_identity_keyname (name, key);

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);

	retval = modest_conf_set_string (priv->modest_conf, keyname, val,
					 err);

	g_free (keyname);
	return retval;
}


gboolean
modest_identity_mgr_set_identity_int (ModestIdentityMgr * self,
				      const gchar * name, const gchar * key,
				      const gint val, GError ** err)
{
	ModestIdentityMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = get_identity_keyname (name, key);

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);

	retval = modest_conf_set_int (priv->modest_conf, keyname, val, err);

	g_free (keyname);
	return retval;
}


gboolean
modest_identity_mgr_set_identity_bool (ModestIdentityMgr * self,
				       const gchar * name, const gchar * key,
				       gboolean val, GError ** err)
{
	ModestIdentityMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = get_identity_keyname (name, key);

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);

	retval = modest_conf_set_bool (priv->modest_conf, keyname, val, err);

	g_free (keyname);
	return retval;
}


gboolean
modest_identity_mgr_identity_exists (ModestIdentityMgr * self,
				     const gchar * name, GError ** err)
{
	ModestIdentityMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);

	keyname = get_identity_keyname (name, NULL);

	priv = MODEST_IDENTITY_MGR_GET_PRIVATE (self);
	retval = modest_conf_key_exists (priv->modest_conf, keyname, err);

	g_free (keyname);
	return retval;
}


/* must be freed by caller */
static gchar *
get_identity_keyname (const gchar * idname, const gchar * name)
{
	if (name)
		return g_strconcat
			(MODEST_IDENTITY_NAMESPACE, "/",
			 idname, "/", name, NULL);
	else
		return g_strconcat
			(MODEST_IDENTITY_NAMESPACE, "/", idname, NULL);
}

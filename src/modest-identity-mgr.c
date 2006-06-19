/* modest-identity-mgr.c */

/* insert (c)/licensing information) */

#include <string.h>
#include "modest-identity-mgr.h"

/* 'private'/'protected' functions */
static void modest_identity_mgr_class_init (ModestIdentityMgrClass * klass);
static void modest_identity_mgr_init (ModestIdentityMgr * obj);
static void modest_identity_mgr_finalize (GObject * obj);

static gchar *get_identity_keyname (const gchar * accname,
				    const gchar * name);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestIdentityMgrPrivate ModestIdentityMgrPrivate;
struct _ModestIdentityMgrPrivate {
	ModestConf *modest_conf;
};

#define MODEST_IDENTITY_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_IDENTITY_MGR, \
                                                ModestIdentityMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

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

	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_identity_mgr_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof (ModestIdentityMgrPrivate));

/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
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
modest_identy_mgr_set_identity_bool (ModestIdentityMgr * self,
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

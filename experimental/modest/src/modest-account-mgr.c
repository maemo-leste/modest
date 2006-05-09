/* modest-account-mgr.c */

/* insert (c)/licensing information) */

#include <string.h>
#include "modest-account-mgr.h"

/* 'private'/'protected' functions */
static void    modest_account_mgr_class_init    (ModestAccountMgrClass *klass);
static void    modest_account_mgr_init          (ModestAccountMgr *obj);
static void    modest_account_mgr_finalize      (GObject *obj);

static gchar*  get_account_keyname (const gchar *accname, const gchar *name);


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountMgrPrivate ModestAccountMgrPrivate;
struct _ModestAccountMgrPrivate {
	ModestConf *modest_conf;
};
#define MODEST_ACCOUNT_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_ACCOUNT_MGR, \
                                                ModestAccountMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_account_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_account_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountMgr),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_account_mgr_init,
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestAccountMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_mgr_class_init (ModestAccountMgrClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_mgr_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountMgrPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}


static void
modest_account_mgr_init (ModestAccountMgr *obj)
{
 	ModestAccountMgrPrivate *priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(obj);
 	priv->modest_conf = NULL;	
}

static void
modest_account_mgr_finalize (GObject *obj)
{
	ModestAccountMgr *self = MODEST_ACCOUNT_MGR(obj);
	ModestAccountMgrPrivate *priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);

	g_object_unref (G_OBJECT(priv->modest_conf));
	priv->modest_conf = NULL;
}

GObject*
modest_account_mgr_new (ModestConf *conf)
{
	GObject *obj;
	ModestAccountMgrPrivate *priv;
	
	g_return_val_if_fail (conf, NULL);

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_ACCOUNT_MGR, NULL));
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(obj);

	/*
	 * increase the ref count on the modest_conf. Normally, the
	 * ModestConf should outlive the ModestAccountMgr though
	 */
	g_object_ref(G_OBJECT(priv->modest_conf = conf));
	return obj;
}



gboolean
modest_account_mgr_add_account (ModestAccountMgr *self, const gchar* name,
			GError **err)
{
	ModestAccountMgrPrivate *priv;
	gchar *key;
	gboolean retval;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	
	if (modest_account_mgr_account_exists (self, name, err)) {
		g_warning ("account already exists");
		return FALSE;
	}
	/*
	 * we create the account by adding an account 'dir', with the name <name>,
	 * and in that the 'display_name' string key
	 */
	key = get_account_keyname (name, "display_name");
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	retval = modest_conf_set_string (priv->modest_conf, key, name, err);

	g_free (key);
	return retval;
}

	
gboolean
modest_account_mgr_remove_account (ModestAccountMgr *self, const gchar* name,
			   GError **err)
{
	ModestAccountMgrPrivate *priv;
	gchar *key;
	gboolean retval;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);

	if (!modest_account_mgr_account_exists (self, name, err)) {
		g_warning ("account doest not exist");
		return FALSE;
	}
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);	
	key = get_account_keyname (name, NULL);
	
	retval = modest_conf_remove_key (priv->modest_conf, key, NULL);	

	g_free (key);
	return retval;
}



GSList*
modest_account_mgr_account_names  (ModestAccountMgr *self, GError **err)
{
	GSList *accounts, *cursor;
	ModestAccountMgrPrivate *priv;
	
	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);

	cursor = accounts = modest_conf_list_subkeys (priv->modest_conf,
						      MODEST_ACCOUNT_NAMESPACE,
						      err);
	while (cursor) {
		gchar *account = cursor->data;
		cursor->data = g_strdup (account +
					 strlen(MODEST_ACCOUNT_NAMESPACE "/"));
		g_free (account);
		cursor = cursor->next;
	}
	return accounts;
}




gchar*
modest_account_mgr_get_account_string (ModestAccountMgr *self, const gchar *name,
				       const gchar *key, GError **err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gchar *retval;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (key, NULL);

	keyname = get_account_keyname (name, key);
 	g_warning ("get key: %s", keyname);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	
	retval = modest_conf_get_string (priv->modest_conf,
					 keyname, err);
	
	g_free (keyname);
	return retval;
}



gint
modest_account_mgr_get_account_int (ModestAccountMgr *self, const gchar *name,
				    const gchar *key, GError **err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gint retval;

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (name, -1);
	g_return_val_if_fail (key, -1);

	keyname = get_account_keyname (name, key);
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	retval = modest_conf_get_int (priv->modest_conf,keyname,err);

	g_free (keyname);
	return retval;
}

	
gboolean modest_account_mgr_get_account_bool (ModestAccountMgr *self, const gchar *name,
					      const gchar *key, GError **err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = get_account_keyname (name, key);
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	retval = modest_conf_get_bool (priv->modest_conf,keyname,err);

	g_free (keyname);
	return retval;
}
	


gboolean
modest_account_mgr_set_account_string  (ModestAccountMgr *self, const gchar *name,
					const gchar *key, const gchar* val,
					GError **err)
{
	ModestAccountMgrPrivate *priv;

	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = get_account_keyname (name, key);
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	retval = modest_conf_set_string (priv->modest_conf,keyname,val,err);

	g_free (keyname);
	return retval;
}



gboolean
modest_account_mgr_set_account_int  (ModestAccountMgr *self, const gchar *name,
				     const gchar *key, gint val, GError **err)
{
	ModestAccountMgrPrivate *priv;
	
	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (name, -1);
	g_return_val_if_fail (key, -1);

	keyname = get_account_keyname (name, key);
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	retval = modest_conf_set_int (priv->modest_conf,keyname,val,err);

	g_free (keyname);
	return retval;
}




gboolean
modest_account_mgr_set_account_bool  (ModestAccountMgr *self, const gchar *name,
				      const gchar *key, gboolean val, GError **err)
{
	ModestAccountMgrPrivate *priv;
	
	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	g_return_val_if_fail (key, FALSE);

	keyname = get_account_keyname (name, key);
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	retval = modest_conf_set_bool (priv->modest_conf,keyname,val,err);

	g_free (keyname);
	return retval;
}


gboolean
modest_account_mgr_account_exists (ModestAccountMgr *self, const gchar *name,
				   GError **err)
{
	ModestAccountMgrPrivate *priv;
	
	gchar *keyname;
	gboolean retval;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (name, FALSE);
	
	keyname = get_account_keyname (name, NULL);
	
	priv = MODEST_ACCOUNT_MGR_GET_PRIVATE(self);
	retval = modest_conf_key_exists (priv->modest_conf,keyname,err);

	g_free (keyname);
	return retval;
}



/* must be freed by caller */
static gchar*
get_account_keyname (const gchar *accname, const gchar *name)
{
	if (name)
		return g_strconcat
			(MODEST_ACCOUNT_NAMESPACE, "/", accname, "/", name, NULL);
	else
		return g_strconcat
			(MODEST_ACCOUNT_NAMESPACE, "/", accname, NULL);
}


/* modest-conf-gconf.c */
/* GConf implementation of ModestConf */

/* insert (c)/licensing information) */

#include "modest-conf.h"
#include "modest-marshal.h"
#include <gconf/gconf-client.h>


/* 'private'/'protected' functions */
static void   modest_conf_class_init     (ModestConfClass *klass);
static void   modest_conf_init           (ModestConf *obj);
static void   modest_conf_finalize       (GObject *obj);

static void   modest_conf_on_change	 (GConfClient *client, guint conn_id,
					  GConfEntry *entry, gpointer data);
/* list my signals */
enum {
	KEY_CHANGED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestConfPrivate ModestConfPrivate;
struct _ModestConfPrivate {
	GConfClient *gconf_client;
};
#define MODEST_CONF_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                         MODEST_TYPE_CONF, \
                                         ModestConfPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

void 
modest_conf_key_changed (ModestConf* self, const gchar *key, const gchar *new_value)
{
	g_signal_emit (self, signals[KEY_CHANGED_SIGNAL], 0, key, new_value);
}


GType
modest_conf_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestConfClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_conf_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestConf),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_conf_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestConf",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_conf_class_init (ModestConfClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	GType paramtypes[2] = {G_TYPE_POINTER, G_TYPE_POINTER};

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_conf_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestConfPrivate));
	
	klass->key_changed = modest_conf_key_changed;

	signals[KEY_CHANGED_SIGNAL] = 
 		g_signal_newv ("key-changed", 
	                       G_TYPE_FROM_CLASS (gobject_class), G_SIGNAL_RUN_LAST,
		               NULL, NULL, NULL,
		               modest_marshal_VOID__POINTER_POINTER,
		               G_TYPE_NONE, 2, paramtypes);
}

static void
modest_conf_init (ModestConf *obj)
{
	GConfClient *conf = NULL;
	ModestConfPrivate *priv = MODEST_CONF_GET_PRIVATE(obj);
	GError *err      = NULL;
	
	priv->gconf_client = NULL;
	
	conf = gconf_client_get_default ();
	if (!conf) {
		g_warning ("could not get gconf client");
		return;
	}

	/* FIXME: is PRELOAD_NONE the most efficient? */
	gconf_client_add_dir (conf, MODEST_CONF_NAMESPACE,
			      GCONF_CLIENT_PRELOAD_NONE, &err);
	if (err) {
		g_warning ("error with gconf_client_add_dir: %d:%s",
			   err->code, err->message);
		g_object_unref (conf);
		g_error_free (err);
		return;
	}

	gconf_client_notify_add (conf, MODEST_CONF_NAMESPACE,
				 modest_conf_on_change,
				 obj, NULL, &err);
	if (err) {
		g_warning ("error with gconf_client_notify_add: %d:%s",
			   err->code, err->message);
		g_object_unref (conf);
		g_error_free (err);
		return;
	}

	/* all went well! */
	priv->gconf_client = conf;
	return;
}

static void
modest_conf_finalize (GObject *obj)
{
	ModestConfPrivate *priv = MODEST_CONF_GET_PRIVATE(obj);
	if (priv->gconf_client) {
		gconf_client_suggest_sync (priv->gconf_client, NULL);
		g_object_unref (priv->gconf_client);
	}	
}

GObject*
modest_conf_new (void)
{
	ModestConf *conf = MODEST_CONF(g_object_new(MODEST_TYPE_CONF, NULL));
	if (!conf) {
		g_warning ("failed to init ModestConf (GConf)");
		return NULL;
	}

	ModestConfPrivate *priv = MODEST_CONF_GET_PRIVATE(conf);
	if (!priv->gconf_client) {
		g_warning ("failed to init gconf");
		g_object_unref (conf);
		return NULL;
	}
	
	return G_OBJECT(conf);
}


gchar*
modest_conf_get_string (ModestConf* self, const gchar* key, GError **err)
{
	ModestConfPrivate *priv;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (key,  NULL);

	priv = MODEST_CONF_GET_PRIVATE(self);
	return gconf_client_get_string (priv->gconf_client, key, err);
}


gint
modest_conf_get_int (ModestConf* self, const gchar* key, GError **err)
{
	ModestConfPrivate *priv;

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (key, -1);

	priv = MODEST_CONF_GET_PRIVATE(self);
	
	return gconf_client_get_int (priv->gconf_client, key, err);
}


gboolean
modest_conf_get_bool (ModestConf* self, const gchar* key, GError **err)
{
	ModestConfPrivate *priv;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (key, FALSE);

	priv = MODEST_CONF_GET_PRIVATE(self);
	
	return gconf_client_get_bool (priv->gconf_client, key, err);
}


gboolean
modest_conf_set_string (ModestConf* self, const gchar* key, const gchar* val,
			GError **err)
{
	ModestConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = MODEST_CONF_GET_PRIVATE(self);

	if (!gconf_client_key_is_writable(priv->gconf_client,key,err)) {
		g_warning ("'%s' is not writable", key);
		return FALSE;
	}
			
	return gconf_client_set_string (priv->gconf_client, key, val, err);	
}


gboolean
modest_conf_set_int  (ModestConf* self, const gchar* key, gint val,
		      GError **err)
{
	ModestConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = MODEST_CONF_GET_PRIVATE(self);

	if (!gconf_client_key_is_writable(priv->gconf_client,key,err)) {
		g_warning ("'%s' is not writable", key);
		return FALSE;
	}
			
	return gconf_client_set_int (priv->gconf_client, key, val, err);	
}


gboolean
modest_conf_set_bool (ModestConf* self, const gchar* key, gboolean val,
		      GError **err)
{
	ModestConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = MODEST_CONF_GET_PRIVATE(self);

	if (!gconf_client_key_is_writable(priv->gconf_client,key, err)) {
		g_warning ("'%s' is not writable", key);
		return FALSE;
	}
			
	return gconf_client_set_bool (priv->gconf_client,key,val, err);
}




GSList*
modest_conf_list_subkeys (ModestConf* self, const gchar* key, GError **err)
{
	ModestConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = MODEST_CONF_GET_PRIVATE(self);
			
	return gconf_client_all_dirs (priv->gconf_client,key,err);
}


gboolean
modest_conf_remove_key (ModestConf* self, const gchar* key, GError **err)
{
	ModestConfPrivate *priv;
		
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = MODEST_CONF_GET_PRIVATE(self);
			
	return gconf_client_recursive_unset (priv->gconf_client,key,0,err);
}




gboolean
modest_conf_key_exists (ModestConf* self, const gchar* key, GError **err)
{
	ModestConfPrivate *priv;
	
	g_return_val_if_fail (self,FALSE);
	g_return_val_if_fail (key, FALSE);
	
	priv = MODEST_CONF_GET_PRIVATE(self);
			
	return gconf_client_dir_exists (priv->gconf_client,key,err);
}





static void
modest_conf_on_change (GConfClient *client, guint conn_id, GConfEntry *entry,
			gpointer data)
{
	ModestConf *modest_conf = data;
	
	if (!entry->value) {
		g_print ("modest: key '%s' unset\n",
			 gconf_entry_get_key (entry));
		g_signal_emit (modest_conf, signals[KEY_CHANGED_SIGNAL], 0, 
		               gconf_entry_get_key (entry), NULL);
	} else {
		gchar *val = gconf_value_to_string (gconf_entry_get_value(entry));
		g_print ("modest: key '%s' set to '%s'\n",
			 gconf_entry_get_key (entry), val);
		g_signal_emit (modest_conf, signals[KEY_CHANGED_SIGNAL], 0, 
		               gconf_entry_get_key (entry), val);
		g_free (val);
	}
}

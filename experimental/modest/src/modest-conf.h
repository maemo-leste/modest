/*
 * modest-conf.h
 */

#ifndef __MODEST_CONF_H__
#define __MODEST_CONF_H__

#include <glib-object.h>
#include "modest-conf-keys.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_CONF             (modest_conf_get_type())
#define MODEST_CONF(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_CONF,ModestConf))
#define MODEST_CONF_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_CONF,GObject))
#define MODEST_IS_CONF(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_CONF))
#define MODEST_IS_CONF_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_CONF))
#define MODEST_CONF_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_CONF,ModestConfClass))

typedef struct _ModestConf      ModestConf;
typedef struct _ModestConfClass ModestConfClass;

struct _ModestConf {
	 GObject parent;
};

struct _ModestConfClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestConf* obj); */
};


/**
 * modest_conf_get_type:
 * 
 * get the GType for ModestConf
 *  
 * Returns: the GType
 */
GType        modest_conf_get_type    (void) G_GNUC_CONST;


/**
 * modest_conf_new:
 * 
 * create a new modest ModestConf object. 
 * 
 * Returns: a new ModestConf instance, or NULL in case
 * of any error
 */
GObject*     modest_conf_new         (void);


/**
 * modest_conf_get_string:
 * @self: self a ModestConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a string from the configuration system
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error. @err gives details in case of error
 */
gchar*       modest_conf_get_string  (ModestConf* self, const gchar* key, GError **err);


/** 
 * modest_conf_get_int:
 * @self: self a ModestConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get an integer from the configuration system
 *  
 * Returns: an integer with the value for the key, or -1 in case of error
 * (of course, -1 can also be returned in non-error cases).
 * @err gives details in case of error
 */
int          modest_conf_get_int     (ModestConf* self, const gchar* key, GError **err);


/** 
 * modest_conf_get_bool:
 * @self: self a ModestConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a boolean value from the configuration system
 *  
 * Returns: a boolean value with the value for the key, or -1 in case of error
 * (of course, -1 can also be returned in non-error cases).
 * @err gives details in case of error
 */
gboolean     modest_conf_get_bool    (ModestConf* self, const gchar* key, GError **err);


/**
 * modest_conf_set_string:
 * @self: a ModestConf instance
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL if not interested.
 *
 * store a string value in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     modest_conf_set_string (ModestConf* self, const gchar* key, const gchar *val,
				     GError **err);

/**
 * modest_conf_set_int:
 * @self: a ModestConf instance
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL if not interested.
 *
 * store an integer value in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     modest_conf_set_int    (ModestConf* self, const gchar* key, int val,
				     GError **err);

/**
 * modest_conf_set_bool:
 * @self: a ModestConf instance
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL if not interested.
 *
 * store a boolean value in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     modest_conf_set_bool    (ModestConf* self, const gchar* key, gboolean val,
				      GError **err);


/**
 * modest_conf_list_subkeys:
 * @self: a ModestConf instance
 * @key: the key whose subkeys will be listed
 * @err: a GError ptr, or NULL if not interested.
 *
 * list all the subkeys for a given key
 * 
 * Returns: a newly allocated list or NULL in case of error
 * the returned GSList must be freed by the caller
 * @err gives details in case of error
 */
GSList*     modest_conf_list_subkeys    (ModestConf* self, const gchar* key,
					GError **err);


/**
 * modest_conf_remove_key:
 * @self: a ModestConf instance
 * @key: the key to remove
 * @err: a GError ptr, or NULL if not interested.
 *
 * attempts to remove @key and all its subkeys
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean   modest_conf_remove_key    (ModestConf* self, const gchar* key, GError **err);


/**
 * modest_conf_key_exists:
 * @self: a ModestConf instance
 * @key: the key to remove
 * @err: a GError ptr, or NULL if not interested.
 *
 * checks if the given key exists in the configuration system
 * 
 * Returns: TRUE if exists, FALSE otherwise.
 * @err gives details in case of error
 */
gboolean   modest_conf_key_exists   (ModestConf* self, const gchar* key, GError **err);


G_END_DECLS

#endif /* __MODEST_CONF_H__ */


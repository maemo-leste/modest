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

#ifndef __MODEST_CONF_H__
#define __MODEST_CONF_H__

#include <glib-object.h>
#include "modest-defs.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_CONF             (modest_conf_get_type())
#define MODEST_CONF(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_CONF,ModestConf))
#define MODEST_CONF_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_CONF,GObject))
#define MODEST_IS_CONF(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_CONF))
#define MODEST_IS_CONF_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_CONF))
#define MODEST_CONF_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_CONF,ModestConfClass))

typedef struct _ModestConf        ModestConf;
typedef struct _ModestConfClass   ModestConfClass;

typedef guint ModestConfNotificationId; 

typedef enum {
	MODEST_CONF_VALUE_INT,
	MODEST_CONF_VALUE_BOOL,
	MODEST_CONF_VALUE_FLOAT,
	MODEST_CONF_VALUE_STRING
} ModestConfValueType;

typedef enum {
	MODEST_CONF_EVENT_KEY_CHANGED,
	MODEST_CONF_EVENT_KEY_UNSET
} ModestConfEvent;

struct _ModestConf {
	 GObject parent;
};

struct _ModestConfClass {
	GObjectClass parent_class;	
	void (* key_changed) (ModestConf* self, 
			      const gchar *key, 
			      ModestConfEvent event,
			      ModestConfNotificationId id);
};

/**
 * modest_conf_get_type:
 * 
 * get the #GType for #ModestConf
 *  
 * Returns: the #GType
 */
GType        modest_conf_get_type    (void) G_GNUC_CONST;


/**
 * modest_conf_new:
 * 
 * create a new modest #ModestConf object. 
 * 
 * Returns: a new #ModestConf instance, or NULL in case
 * of any error
 */
ModestConf*     modest_conf_new         (void);


/**
 * modest_conf_get_string:
 * @self: a ModestConf instance
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
 * @self: a ModestConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get an integer from the configuration system
 *  
 * Returns: an integer with the value for the key, or -1 in case of error
 * (of course, -1 can also be returned in non-error cases).
 * @err gives details in case of error
 */
gint         modest_conf_get_int     (ModestConf* self, const gchar* key, GError **err);


/** 
 * modest_conf_get_bool:
 * @self: a ModestConf instance
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a boolean value from the configuration system
 *  
 * Returns: a boolean value with the value for the key, or FALSE in case of error
 * (of course, FALSE can also be returned in non-error cases).
 * @err gives details in case of error
 */
gboolean     modest_conf_get_bool    (ModestConf* self, const gchar* key, GError **err);


/** 
 * modest_conf_get_list:
 * @self: a ModestConf instance
 * @key: the key of the value to retrieve
 * @list_type: the type of the elements of the list
 * @err: a GError ptr, or NULL to ignore.
 * 
 * get a list of values from the configuration system
 *  
 * Returns: a list with the values for the key, or NULL in case of error
 * @err gives details in case of error
 */
GSList*     modest_conf_get_list    (ModestConf* self, const gchar* key, 
				      ModestConfValueType list_type, GError **err);

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
 * modest_conf_set_list:
 * @self: a ModestConf instance
 * @key: the key of the value to retrieve
 * @val: the list with the values to set
 * @list_type: the type of the elements of the list
 * @err: a GError ptr, or NULL to ignore.
 * 
 * set a list of values in the configuration system
 *
 * Returns: TRUE if succeeded or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean     modest_conf_set_list    (ModestConf* self, const gchar* key, 
				      GSList *val, ModestConfValueType list_type, 
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
 * @err might give details in case of error
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
 * @err might give details in case of error
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
 * Returns: TRUE if it exists, FALSE otherwise.
 * @err gives details in case of error
 */
gboolean   modest_conf_key_exists   (ModestConf* self, const gchar* key, GError **err);



/**
 * modest_conf_key_valid:
 * @str: some key
 *
 * check whether @str is a valid key in the config system
 * This is a *class* function, and therefore does not require a ModestConf
 * instance
 * 
 * Returns: TRUE if it is valid, FALSE otherwise
 */
gboolean modest_conf_key_is_valid (const gchar* str);


/**
 * modest_conf_key_escape:
 * @str: a non-empty string to escape
 *
 * returns an escaped version of @str, ie. something suitable as a key
 * This is a *class* function, and therefore does not require a ModestConf
 * instance. Note: this for is invidual elements in a key
 * 
 * Returns: a newly allocated string with the escaped version
 */
gchar* modest_conf_key_escape (const gchar* str);


/**
 * modest_conf_key_escape:
 * @str: a string to escape
 *
 * returns an unescaped version of @str. This is a *class* function, and
 * therefore does not require a ModestConf instance
 * Note: this for is invidual elements in a key
 * 
 * Returns: a newly allocated string with the unescaped version
 */
gchar* modest_conf_key_unescape (const gchar* str);


void modest_conf_listen_to_namespace (ModestConf *self,
				      const gchar *namespace);

void modest_conf_forget_namespace    (ModestConf *self,
				      const gchar *namespace);
G_END_DECLS

#endif /* __MODEST_CONF_H__ */


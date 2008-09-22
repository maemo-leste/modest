/* Copyright (c) 2008, Nokia Corporation
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


/* modest-account-settings.h */

#ifndef __MODEST_PROTOCOL_H__
#define __MODEST_PROTOCOL_H__

#include <glib-object.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_PROTOCOL             (modest_protocol_get_type())
#define MODEST_PROTOCOL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_PROTOCOL,ModestProtocol))
#define MODEST_PROTOCOL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_PROTOCOL,ModestProtocolClass))
#define MODEST_IS_PROTOCOL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_PROTOCOL))
#define MODEST_IS_PROTOCOL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_PROTOCOL))
#define MODEST_PROTOCOL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_PROTOCOL,ModestProtocolClass))

#define MODEST_PROTOCOL_TYPE_INVALID -1

typedef gchar * (*TranslationFunc) (gpointer userdata, ...);

typedef struct _ModestProtocol      ModestProtocol;
typedef struct _ModestProtocolClass ModestProtocolClass;

typedef guint ModestProtocolType;

struct _ModestProtocol {
	GObject parent;
};

struct _ModestProtocolClass {
	GObjectClass parent_class;
};

/**
 * modest_protocol_get_type:
 *
 * Returns: GType of the account store
 */
GType  modest_protocol_get_type   (void) G_GNUC_CONST;

/**
 * modest_protocol_new:
 *
 * creates a new instance of #ModestProtocol
 *
 * Returns: a #ModestProtocol
 */
ModestProtocol*    modest_protocol_new (const gchar *name, const gchar *display_name);

/**
 * modest_protocol_get_name:
 * @self: a #ModestProtocol
 *
 * get the protocol unique name (used for storing conf and identifying the protocol with a string)
 *
 * Returns: a string
 */
const gchar* modest_protocol_get_name (ModestProtocol *self);

/**
 * modest_protocol_set_name:
 * @self: a #ModestProtocol
 * @name: the protocol unique name.
 *
 * set @name as the protocol unique name .
 */
void         modest_protocol_set_name (ModestProtocol *self,
				       const gchar *name);
/**
 * modest_protocol_get_display_name:
 * @self: a #ModestProtocol
 *
 * get the display name for the protocol
 *
 * Returns: a string
 */
const gchar* modest_protocol_get_display_name (ModestProtocol *self);

/**
 * modest_protocol_set_display_name:
 * @settings: a #ModestProtocol
 * @display_name: a string.
 *
 * set @display_name of the account.
 */
void         modest_protocol_set_display_name (ModestProtocol *protocol,
						const gchar *display_name);
/**
 * modest_protocol_get_type_id:
 * @self: a #ModestProtocol
 *
 * get the protocol type id.
 *
 * Returns: a #ModestProtocolType
 */
ModestProtocolType modest_protocol_get_type_id (ModestProtocol *self);

/**
 * modest_protocol_get:
 * @protocol: a #ModestProtocol
 * @key: a string
 *
 * obtains the value of @key for @protocol
 *
 * Returns: a string
 */
const gchar *
modest_protocol_get (ModestProtocol *protocol,
		     const gchar *key);

/**
 * modest_protocol_set:
 * @protocol: a #ModestProtocol
 * @key: a string
 * @value: a string
 *
 * sets @value as the value for @key in @protocol
 */
void
modest_protocol_set (ModestProtocol *protocol,
		     const gchar *key, const gchar *value);

/**
 * modest_protocol_set_translation:
 * @protocol: a #ModestProtocol
 * @id: the id for the translation set
 * @translation_func: the function used to obtain the translation
 *
 * sets @translation_func as the way to compose the translation for @id
 */
void
modest_protocol_set_translation (ModestProtocol *protocol,
				 const gchar *id,
				 TranslationFunc translation_func,
				 gpointer userdata,
				 GDestroyNotify data_destroy_func);

/**
 * modest_protocol_get_translation:
 * @protocol: a @ModestProtocol
 * @id: the id for the translation set
 * @...: the parameters for the translation (pritntf style)
 *
 * applies the translation with parameters to obtain the full string expected.
 *
 * Returns: a newly allocated string
 */
gchar *
modest_protocol_get_translation (ModestProtocol *protocol,
				 const gchar *id,
				 ...);

G_END_DECLS

#endif /* __MODEST_PROTOCOL_H__ */

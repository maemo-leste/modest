/* Copyright (c) 2007, Nokia Corporation
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

#ifndef __MODEST_PROTOCOL_REGISTRY_H__
#define __MODEST_PROTOCOL_REGISTRY_H__

#include <glib-object.h>
#include "modest-protocol.h"
#include "modest-pair.h"

G_BEGIN_DECLS

#define MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS "connection"
#define MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS "auth"
#define MODEST_PROTOCOL_REGISTRY_TRANSPORT_STORE_PROTOCOLS "transport-store"
#define MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS "store"
#define MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS "transport"
#define MODEST_PROTOCOL_REGISTRY_LOCAL_STORE_PROTOCOLS "local-store"
#define MODEST_PROTOCOL_REGISTRY_REMOTE_STORE_PROTOCOLS "remote-store"
#define MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS "secure"
#define MODEST_PROTOCOL_REGISTRY_HAS_LEAVE_ON_SERVER_PROTOCOLS "leave-on-server-available"
#define MODEST_PROTOCOL_REGISTRY_PROVIDER_PROTOCOLS "providers"
#define MODEST_PROTOCOL_REGISTRY_SINGLETON_PROVIDER_PROTOCOLS "singleton-providers"
#define MODEST_PROTOCOL_REGISTRY_MULTI_MAILBOX_PROVIDER_PROTOCOLS "multi-mailbox-providers"
#define MODEST_PROTOCOL_REGISTRY_USE_ALTERNATE_PORT "use-alternate-port"
#define MODEST_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS "store-has-folders"
/* Accounts that cannot be the destination of messages or folders transfers */
#define MODEST_PROTOCOL_REGISTRY_STORE_FORBID_INCOMING_XFERS "store-forbid-incoming-xfers"
#define MODEST_PROTOCOL_REGISTRY_NO_AUTO_UPDATE_PROTOCOLS "no-auto-update"

/* convenience macros */
#define MODEST_TYPE_PROTOCOL_REGISTRY             (modest_protocol_registry_get_type())
#define MODEST_PROTOCOL_REGISTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_PROTOCOL_REGISTRY,ModestProtocolRegistry))
#define MODEST_PROTOCOL_REGISTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_PROTOCOL_REGISTRY,ModestProtocolRegistryClass))
#define MODEST_IS_PROTOCOL_REGISTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_PROTOCOL_REGISTRY))
#define MODEST_IS_PROTOCOL_REGISTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_PROTOCOL_REGISTRY))
#define MODEST_PROTOCOL_REGISTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_PROTOCOL_REGISTRY,ModestProtocolRegistryClass))

/* a special type, equivalent to a NULL protocol */
#define MODEST_PROTOCOL_REGISTRY_TYPE_INVALID -1

/* The minimum priority custom protocols should take for their index */
#define MODEST_PROTOCOL_REGISTRY_USER_PRIORITY 100

/* macros to access the default configured protocols */
#define MODEST_PROTOCOLS_TRANSPORT_SMTP (modest_protocol_registry_get_smtp_type_id())
#define MODEST_PROTOCOLS_TRANSPORT_SENDMAIL (modest_protocol_registry_get_sendmail_type_id())
#define MODEST_PROTOCOLS_STORE_POP (modest_protocol_registry_get_pop_type_id())
#define MODEST_PROTOCOLS_STORE_IMAP (modest_protocol_registry_get_imap_type_id())
#define MODEST_PROTOCOLS_STORE_MAILDIR (modest_protocol_registry_get_maildir_type_id())
#define MODEST_PROTOCOLS_STORE_MBOX (modest_protocol_registry_get_mbox_type_id())
#define MODEST_PROTOCOLS_CONNECTION_NONE (modest_protocol_registry_get_none_connection_type_id ())
#define MODEST_PROTOCOLS_CONNECTION_TLS (modest_protocol_registry_get_tls_connection_type_id ())
#define MODEST_PROTOCOLS_CONNECTION_SSL (modest_protocol_registry_get_ssl_connection_type_id ())
#define MODEST_PROTOCOLS_CONNECTION_TLSOP (modest_protocol_registry_get_tlsop_connection_type_id ())
#define MODEST_PROTOCOLS_AUTH_NONE (modest_protocol_registry_get_none_auth_type_id ())
#define MODEST_PROTOCOLS_AUTH_PASSWORD (modest_protocol_registry_get_password_auth_type_id ())
#define MODEST_PROTOCOLS_AUTH_CRAMMD5 (modest_protocol_registry_get_crammd5_auth_type_id ())

/* properties available */
#define MODEST_PROTOCOL_SECURITY_ACCOUNT_OPTION "modest-security-account-option"
#define MODEST_PROTOCOL_AUTH_ACCOUNT_OPTION "modest-auth-account-option"

/* translations */
#define MODEST_PROTOCOL_TRANSLATION_DELETE_MAILBOX "translation-delete-mailbox" /* title string */
#define MODEST_PROTOCOL_TRANSLATION_CONNECT_ERROR "translation-connect-error" /* server name */
#define MODEST_PROTOCOL_TRANSLATION_AUTH_ERROR "translation-auth-error" /* server name */
#define MODEST_PROTOCOL_TRANSLATION_ACCOUNT_CONNECTION_ERROR "translation-account-connection-error" /* hostname */
#define MODEST_PROTOCOL_TRANSLATION_MSG_NOT_AVAILABLE "translation-msg-not-available" /* subject */
#define MODEST_PROTOCOL_TRANSLATION_SSL_PROTO_NAME "translation-ssl-proto-name"


typedef struct _ModestProtocolRegistry      ModestProtocolRegistry;
typedef struct _ModestProtocolRegistryClass ModestProtocolRegistryClass;

typedef guint ModestProtocolRegistryType;

struct _ModestProtocolRegistry {
	GObject parent;
};

struct _ModestProtocolRegistryClass {
	GObjectClass parent_class;
};

/**
 * modest_protocol_registry_get_type:
 *
 * Returns: GType of the account store
 */
GType  modest_protocol_registry_get_type   (void) G_GNUC_CONST;

/**
 * modest_protocol_registry_new:
 *
 * creates a new instance of #ModestProtocolRegistry
 *
 * Returns: a #ModestProtocolRegistry
 */
ModestProtocolRegistry*    modest_protocol_registry_new (void);

/**
 * modest_protocol_registry_add:
 * @self: a #ModestProtocolRegistry
 * @protocol: a #ModestProtocol
 * @priority: priority establishes the order the protocols will be shown on listings
 * @first_tag: a string
 * @...: a %NULL terminated list of strings with the tags for the protocol
 *
 * Add @protocol to the registry @self, setting the proper identifying tags
 */
void modest_protocol_registry_add (ModestProtocolRegistry *self, ModestProtocol *protocol, gint priority, const gchar *first_tag, ...);

/**
 * modest_protocol_registry_get_all:
 * @self: a #ModestProtocolRegistry
 *
 * obtains a list of all protocols registered in @self
 *
 * Returns: a newly allocated GSList of the protocols. Don't unref the protocols, only the list.
 */
GSList *modest_protocol_registry_get_all (ModestProtocolRegistry *self);

/**
 * modest_protocol_registry_get_by_tag:
 * @self: a #ModestProtocolRegistry
 * @tag: a string
 *
 * obtains a list of all protocols registered in @self tagged with @tag
 *
 * Returns: a newly allocated GSList of the protocols. Don't unref the protocol, only the list.
 */
GSList *modest_protocol_registry_get_by_tag (ModestProtocolRegistry *self, const gchar *tag);

/**
 * modest_protocol_registry_get_pair_list_by_tag:
 * @self: a #ModestProtocolRegistry
 * @tag: a string
 *
 * obtains a pair list of all protocols registered in @self tagged with @tag
 *
 * Returns: a newly allocated #ModestPairList of the protocols. Should be freed using
 * modest_pair_list_free ()
 */
ModestPairList *modest_protocol_registry_get_pair_list_by_tag (ModestProtocolRegistry *self, const gchar *tag);

/**
 * modest_protocol_registry_get_protocol_by_name:
 * @self: a #ModestProtocolRegistry
 * @tag: a string
 * @name: a string
 *
 * Obtains the protocol in registry @self, tagged with @tag and with @name
 *
 * Returns: the obtained #ModestProtocol, or %NULL if not found.
 */
ModestProtocol *modest_protocol_registry_get_protocol_by_name (ModestProtocolRegistry *self, const gchar *tag, const gchar *name);

/**
 * modest_protocol_registry_get_protocol_by_type:
 * @self: a #ModestProtocolRegistry
 * @type_id: a #ModestProtocolType
 *
 * Obtains the protocol in registry @self, identified by @type_id
 *
 * Returns: the obtained #ModestProtocol, or %NULL if not found.
 */
ModestProtocol *modest_protocol_registry_get_protocol_by_type (ModestProtocolRegistry *self, ModestProtocolType type_id);

/**
 * modest_protocol_registry_protocol_type_has_tag:
 * @self: a #ModestProtocolRegistry
 * @type_id: a #ModestProtocolType
 * @tag: a string
 *
 * Checks if a protocol identified with @type_id has a specific @tag.
 *
 * Returns: %TRUE if @type_id protocol has @tag in registry @self
 */
gboolean modest_protocol_registry_protocol_type_has_tag (ModestProtocolRegistry *self, ModestProtocolType type_id, const gchar *tag);

#define modest_protocol_registry_protocol_type_is_secure(registry,protocol_type) \
	modest_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS)

#define modest_protocol_registry_protocol_type_is_provider(registry,protocol_type) \
	modest_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							MODEST_PROTOCOL_REGISTRY_PROVIDER_PROTOCOLS)

#define modest_protocol_registry_protocol_type_is_singleton_provider(registry,protocol_type) \
	modest_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							MODEST_PROTOCOL_REGISTRY_SINGLETON_PROVIDER_PROTOCOLS)

#define modest_protocol_registry_protocol_type_has_leave_on_server(registry,protocol_type) \
	modest_protocol_registry_protocol_type_has_tag ((registry), (protocol_type), \
							MODEST_PROTOCOL_REGISTRY_HAS_LEAVE_ON_SERVER_PROTOCOLS)

/**
 * @self: a #ModestProtocolRegistry
 *
 * Set default available protocols in Modest in @self
 */
void modest_protocol_registry_set_to_default (ModestProtocolRegistry *self);

ModestProtocolType modest_protocol_registry_get_imap_type_id (void);
ModestProtocolType modest_protocol_registry_get_pop_type_id (void);
ModestProtocolType modest_protocol_registry_get_maildir_type_id (void);
ModestProtocolType modest_protocol_registry_get_mbox_type_id (void);
ModestProtocolType modest_protocol_registry_get_smtp_type_id (void);
ModestProtocolType modest_protocol_registry_get_sendmail_type_id (void);
ModestProtocolType modest_protocol_registry_get_none_connection_type_id (void);
ModestProtocolType modest_protocol_registry_get_tls_connection_type_id (void);
ModestProtocolType modest_protocol_registry_get_ssl_connection_type_id (void);
ModestProtocolType modest_protocol_registry_get_tlsop_connection_type_id (void);
ModestProtocolType modest_protocol_registry_get_none_auth_type_id (void);
ModestProtocolType modest_protocol_registry_get_password_auth_type_id (void);
ModestProtocolType modest_protocol_registry_get_crammd5_auth_type_id (void);

G_END_DECLS

#endif /* __MODEST_PROTOCOL_REGISTRY_H__ */

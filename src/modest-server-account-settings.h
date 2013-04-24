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


/* modest-server-account-settings.h */

#ifndef __MODEST_SERVER_ACCOUNT_SETTINGS_H__
#define __MODEST_SERVER_ACCOUNT_SETTINGS_H__

#include <glib-object.h>
#include <modest-protocol.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_SERVER_ACCOUNT_SETTINGS             (modest_server_account_settings_get_type())
#define MODEST_SERVER_ACCOUNT_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_SERVER_ACCOUNT_SETTINGS,ModestServerAccountSettings))
#define MODEST_SERVER_ACCOUNT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_SERVER_ACCOUNT_SETTINGS,ModestServerAccountSettingsClass))
#define MODEST_IS_SERVER_ACCOUNT_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_SERVER_ACCOUNT_SETTINGS))
#define MODEST_IS_SERVER_ACCOUNT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_SERVER_ACCOUNT_SETTINGS))
#define MODEST_SERVER_ACCOUNT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_SERVER_ACCOUNT_SETTINGS,ModestServerAccountSettingsClass))

typedef struct _ModestServerAccountSettings      ModestServerAccountSettings;
typedef struct _ModestServerAccountSettingsClass ModestServerAccountSettingsClass;

struct _ModestServerAccountSettings {
	GObject parent;
};

struct _ModestServerAccountSettingsClass {
	GObjectClass parent_class;
};


/**
 * modest_server_account_settings_get_type:
 *
 * Returns: GType of the account store
 */
GType  modest_server_account_settings_get_type   (void) G_GNUC_CONST;

/**
 * modest_server_account_settings_new:
 *
 * creates a new instance of #ModestServerAccountSettings
 *
 * Returns: a #ModestServerAccountSettings
 */
ModestServerAccountSettings*    modest_server_account_settings_new (void);

/**
 * modest_server_account_settings_get_hostname:
 * @settings: a #ModestServerAccountSettings
 *
 * get the server hostname.
 *
 * Returns: a string
 */
const gchar* modest_server_account_settings_get_hostname (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_hostname:
 * @settings: a #ModestServerAccountSettings
 * @hostname: a string.
 *
 * set @hostname as the server hostname.
 */
void         modest_server_account_settings_set_hostname (ModestServerAccountSettings *settings,
								      const gchar *hostname);

/**
 * modest_server_account_settings_get_protocol:
 * @settings: a #ModestServerAccountSettings
 *
 * get the server protocol.
 *
 * Returns: a #ModestProtocolType
 */
ModestProtocolType modest_server_account_settings_get_protocol (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_protocol:
 * @settings: a #ModestServerAccountSettings
 * @protocol: a #ModestProtocolType
 *
 * set @server_type.
 */
void                          modest_server_account_settings_set_protocol (ModestServerAccountSettings *settings,
									   ModestProtocolType protocol_type);


/**
 * modest_server_account_settings_get_uri:
 * @settings: a #ModestServerAccountSettings
 *
 * get the uri, if any. If this is set, then all the other fields are invalid. It's only valid if protocol is %NULL.
 *
 * Returns: a string
 */
const gchar *modest_server_account_settings_get_uri (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_uri:
 * @settings: a #ModestServerAccountSettings
 * @uri: a string
 *
 * set @uri. When you set an @uri, then the protocol is set to %MODEST_PROTOCOL_REGISTRY_TYPE_INVALID. This is used for setting maildir or mbox
 * accounts.
 */
void   modest_server_account_settings_set_uri (ModestServerAccountSettings *settings,
					       const gchar *uri);

/**
 * modest_server_account_settings_get_port:
 * @settings: a #ModestServerAccountSettings
 *
 * get the server port.
 *
 * Returns: a #guint
 */
guint  modest_server_account_settings_get_port (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_port:
 * @settings: a #ModestServerAccountSettings
 * @port: a #guint.
 *
 * set @port.
 */
void   modest_server_account_settings_set_port (ModestServerAccountSettings *settings,
						guint port);

/**
 * modest_server_account_settings_get_username:
 * @settings: a #ModestServerAccountSettings
 *
 * get the username.
 *
 * Returns: a string
 */
const gchar *modest_server_account_settings_get_username (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_username:
 * @settings: a #ModestServerAccountSettings
 * @username: a string
 *
 * set @username.
 */
void   modest_server_account_settings_set_username (ModestServerAccountSettings *settings,
						    const gchar *username);

/**
 * modest_server_account_settings_get_password:
 * @settings: a #ModestServerAccountSettings
 *
 * get the password.
 *
 * Returns: a string
 */
const gchar *modest_server_account_settings_get_password (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_password:
 * @settings: a #ModestServerAccountSettings
 * @password: a string
 *
 * set @password.
 */
void   modest_server_account_settings_set_password (ModestServerAccountSettings *settings,
						    const gchar *password);


/**
 * modest_server_account_settings_get_security_protocol:
 * @settings: a #ModestServerAccountSettings
 *
 * get the secure connection type, if any.
 *
 * Returns: a #ModestProtocolType
 */
ModestProtocolType modest_server_account_settings_get_security_protocol (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_security_protocol:
 * @settings: a #ModestServerAccountSettings
 * @security: a #ModestProtocolType
 *
 * set the current security connection protocol to @security.
 */
void   modest_server_account_settings_set_security_protocol (ModestServerAccountSettings *settings,
							     ModestProtocolType security_protocol);


/**
 * modest_server_account_settings_get_auth_protocol:
 * @settings: a #ModestServerAccountSettings
 *
 * get the authentication protocol
 *
 * Returns: a #ModestProtocolType
 */
ModestProtocolType modest_server_account_settings_get_auth_protocol (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_auth_protocol:
 * @settings: a #ModestServerAccountSettings
 * @auth_protocol: a #ModestProtocolType
 *
 * set the current authentication protocol to @auth_protocol.
 */
void   modest_server_account_settings_set_auth_protocol (ModestServerAccountSettings *settings,
							 ModestProtocolType auth_protocol);

/**
 * modest_server_account_settings_get_account_name:
 * @settings: a #ModestServerAccountSettings
 *
 * get the #ModestAccountMgr account name for these settings, or
 * %NULL if it's not in the manager.
 *
 * Returns: a string, or %NULL
 */
const gchar *modest_server_account_settings_get_account_name (ModestServerAccountSettings *settings);

/**
 * modest_server_account_settings_set_account_name:
 * @settings: a #ModestServerAccountSettings
 * @account_name: a string
 *
 * sets the account name that will be used to store the account settings. This should
 * only be called from #ModestAccountMgr and #ModestAccountSettings.
 */
void modest_server_account_settings_set_account_name (ModestServerAccountSettings *settings,
						      const gchar *account_name);

/**
 * modest_account_settings_get_offline_sync:
 * @settings: a #ModestAccountSettings
 *
 * obtains if we should synchronise the account for offline use
 *
 * Returns: a #gboolean
 */
gboolean modest_server_account_settings_get_offline_sync (ModestServerAccountSettings *settings);
					      
/**
 * modest_account_settings_set_offline_sync:
 * @settings: a #ModestAccountSettings
 * @offline_sync: a #gboolean
 *
 * if set, mails in these folders will be downloaded for offline reading
 */
void modest_server_account_settings_set_offline_sync (ModestServerAccountSettings *settings, 
						      gboolean offline_sync);

/**
 * modest_account_settings_get_update_all_folders:
 * @settings: a #ModestAccountSettings
 *
 * obtains if we should update all folders for the account (not only INBOX)
 *
 * Returns: a #gboolean
 */
gboolean modest_server_account_settings_get_update_all_folders (ModestServerAccountSettings *settings);

/**
 * modest_account_settings_set_update_all_folders:
 * @settings: a #ModestAccountSettings
 * @update: a #gboolean
 *
 * if set, all folders for this account will be updated (not only INBOX)
 */
void modest_server_account_settings_set_update_all_folders (ModestServerAccountSettings *settings,
						      gboolean enable);


G_END_DECLS

#endif /* __MODEST_SERVER_ACCOUNT_SETTINGS_H__ */

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

#ifndef __MODEST_ACCOUNT_SETTINGS_H__
#define __MODEST_ACCOUNT_SETTINGS_H__

#include <glib-object.h>
#include <modest-server-account-settings.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_SETTINGS             (modest_account_settings_get_type())
#define MODEST_ACCOUNT_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_SETTINGS,ModestAccountSettings))
#define MODEST_ACCOUNT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_SETTINGS,ModestAccountSettingsClass))
#define MODEST_IS_ACCOUNT_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_SETTINGS))
#define MODEST_IS_ACCOUNT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_SETTINGS))
#define MODEST_ACCOUNT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_SETTINGS,ModestAccountSettingsClass))

typedef struct _ModestAccountSettings      ModestAccountSettings;
typedef struct _ModestAccountSettingsClass ModestAccountSettingsClass;

struct _ModestAccountSettings {
	GObject parent;
};

struct _ModestAccountSettingsClass {
	GObjectClass parent_class;
};

typedef enum {
	MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY = 0,
	MODEST_ACCOUNT_RETRIEVE_MESSAGES,
	MODEST_ACCOUNT_RETRIEVE_MESSAGES_AND_ATTACHMENTS
} ModestAccountRetrieveType;


/**
 * modest_account_settings_get_type:
 *
 * Returns: GType of the account store
 */
GType  modest_account_settings_get_type   (void) G_GNUC_CONST;

/**
 * modest_account_settings_new:
 *
 * creates a new instance of #ModestAccountSettings
 *
 * Returns: a #ModestAccountSettings
 */
ModestAccountSettings*    modest_account_settings_new (void);

/**
 * modest_account_settings_get_fullname:
 * @settings: a #ModestAccountSettings
 *
 * get the user full name.
 *
 * Returns: a string
 */
const gchar* modest_account_settings_get_fullname (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_fullname:
 * @settings: a #ModestAccountSettings
 * @hostname: a string.
 *
 * set @fullname as the user full name .
 */
void         modest_account_settings_set_fullname (ModestAccountSettings *settings,
						   const gchar *fullname);
/**
 * modest_account_settings_get_email_address:
 * @settings: a #ModestAccountSettings
 *
 * get the user email address.
 *
 * Returns: a string
 */
const gchar* modest_account_settings_get_email_address (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_email_address:
 * @settings: a #ModestAccountSettings
 * @hostname: a string.
 *
 * set @email_address of the account.
 */
void         modest_account_settings_set_email_address (ModestAccountSettings *settings,
							const gchar *email_address);
/**
 * modest_account_settings_get_retrieve_type:
 * @settings: a #ModestAccountSettings
 *
 * get the account retrieve type.
 *
 * Returns: a #ModestAccountRetrieveType
 */
ModestAccountRetrieveType modest_account_settings_get_retrieve_type (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_retrieve_type:
 * @settings: a #ModestAccountSettings
 * @retrieve_type: a #ModestAccountRetrieveType.
 *
 * set @retrieve_type of the account.
 */
void         modest_account_settings_set_retrieve_type (ModestAccountSettings *settings,
							ModestAccountRetrieveType retrieve_type);

/**
 * modest_account_settings_get_retrieve_limit:
 * @settings: a #ModestAccountSettings
 *
 * get the account retrieve limit. 0 is no limit.
 *
 * Returns: a #gint
 */
gint modest_account_settings_get_retrieve_limit (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_retrieve_limit:
 * @settings: a #ModestAccountSettings
 * @retrieve_limit: a #gint.
 *
 * set @retrieve_limit of the account. 0 is no limit.
 */
void         modest_account_settings_set_retrieve_limit (ModestAccountSettings *settings,
							 gint retrieve_limit);

/**
 * modest_account_settings_get_display_name:
 * @settings: a #ModestAccountSettings
 *
 * get the visible name of the account.
 *
 * Returns: a string
 */
const gchar* modest_account_settings_get_display_name (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_display_name:
 * @settings: a #ModestAccountSettings
 * @hostname: a string.
 *
 * set @display_name as the name of the account visible to the users in UI.
 */
void         modest_account_settings_set_display_name (ModestAccountSettings *settings,
						       const gchar *display_name);

/**
 * modest_account_settings_get_account_name:
 * @settings: a #ModestAccountSettings
 *
 * get the #ModestAccountMgr account name for these settings, or
 * %NULL if it's not in the manager.
 *
 * Returns: a string, or %NULL
 */
const gchar *modest_account_settings_get_account_name (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_account_name:
 * @settings: a #ModestAccountSettings
 * @account_name: a string
 *
 * sets the account name that will be used to store the account settings. This should
 * only be called from #ModestAccountMgr and #ModestAccountSettings.
 */
void modest_account_settings_set_account_name (ModestAccountSettings *settings,
					       const gchar *account_name);

/**
 * modest_account_settings_get_enabled:
 * @settings: a #ModestAccountSettings
 *
 * obtains whether the account is enabled or not.
 *
 * Returns: a #gboolean
 */
gboolean modest_account_settings_get_enabled (ModestAccountSettings *settings);
					      
/**
 * modest_account_settings_set_enabled:
 * @settings: a #ModestAccountSettings
 * @enabled: a #gboolean
 *
 * set if @settings account is enabled or not.
 */
void modest_account_settings_set_enabled (ModestAccountSettings *settings, gboolean enabled);


/**
 * modest_account_settings_get_is_default:
 * @settings: a #ModestAccountSettings
 *
 * obtains whether the account is the default account or not.
 *
 * Returns: a #gboolean
 */
gboolean modest_account_settings_get_is_default (ModestAccountSettings *settings);
					      
/**
 * modest_account_settings_set_is_default:
 * @settings: a #ModestAccountSettings
 * @is_default: a #gboolean
 *
 * set if @settings account is the default account or not.
 */
void modest_account_settings_set_is_default (ModestAccountSettings *settings, gboolean is_default);

/**
 * modest_account_settings_get_store_settings:
 * @settings: a #ModestAccountSettings
 *
 * obtains a ref'ed instance of the store account server settings
 *
 * Returns: a ref'd #ModestServerAccountSettings. You should unreference it on finishing usage.
 */
ModestServerAccountSettings *modest_account_settings_get_store_settings (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_store_settings:
 * @settings: a #ModestAccountSettings
 *
 * sets @store_settings as the settings of the store account of @settings account.
 * @settings will keep an internal reference to it.
 */
void modest_account_settings_set_store_settings (ModestAccountSettings *settings, 
						 ModestServerAccountSettings *store_settings);

/**
 * modest_account_settings_get_transport_settings:
 * @settings: a #ModestAccountSettings
 *
 * obtains a ref'ed instance of the transport account server settings
 *
 * Returns: a ref'd #ModestServerAccountSettings. You should unreference it on finishing usage.
 */
ModestServerAccountSettings *modest_account_settings_get_transport_settings (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_transport_settings:
 * @settings: a #ModestAccountSettings
 *
 * sets @transport_settings as the settings of the transport account of @settings account.
 * @settings will keep an internal reference to it.
 */
void modest_account_settings_set_transport_settings (ModestAccountSettings *settings, 
						     ModestServerAccountSettings *transport_settings);

/**
 * modest_account_settings_get_use_signature:
 * @settings: a #ModestAccountSettings
 *
 * obtains whether the mails from this account use signature or not.
 *
 * Returns: a #gboolean
 */
gboolean modest_account_settings_get_use_signature (ModestAccountSettings *settings);
					      
/**
 * modest_account_settings_set_use_signature:
 * @settings: a #ModestAccountSettings
 * @use_signature: a #gboolean
 *
 * set if @settings mails use signature or not
 */
void modest_account_settings_set_use_signature (ModestAccountSettings *settings, gboolean use_signature);

/**
 * modest_account_settings_get_signature:
 * @settings: a #ModestAccountSettings
 *
 * get the signature.
 *
 * Returns: a string
 */
const gchar* modest_account_settings_get_signature (ModestAccountSettings *settings);

/**
 * modest_account_settings_set_signature:
 * @settings: a #ModestAccountSettings
 * @hostname: a string.
 *
 * set @signature for the account .
 */
void         modest_account_settings_set_signature (ModestAccountSettings *settings,
						   const gchar *signature);
/**
 * modest_account_settings_get_leave_messages_on_server:
 * @settings: a #ModestAccountSettings
 *
 * obtains whether messages should be left on server or not
 *
 * Returns: a #gboolean
 */
gboolean modest_account_settings_get_leave_messages_on_server (ModestAccountSettings *settings);
					      
/**
 * modest_account_settings_set_leave_messages_on_server:
 * @settings: a #ModestAccountSettings
 * @leave_messages_on_server: a #gboolean
 *
 * set if we leave the messages on server or not.
 */
void modest_account_settings_set_leave_messages_on_server (ModestAccountSettings *settings, 
							   gboolean leave_messages_on_server);


/**
 * modest_account_settings_get_use_connection_specific_smtp:
 * @settings: a #ModestAccountSettings
 *
 * obtains if we should try the connection specific smtp servers
 *
 * Returns: a #gboolean
 */
gboolean modest_account_settings_get_use_connection_specific_smtp (ModestAccountSettings *settings);
					      
/**
 * modest_account_settings_set_use_connection_specific_smtp:
 * @settings: a #ModestAccountSettings
 * @use_connection_specific_smtp: a #gboolean
 *
 * if set, mails sent from this account first try the connection specific smtp servers
 * before the transport account.
 */
void modest_account_settings_set_use_connection_specific_smtp (ModestAccountSettings *settings, 
							       gboolean use_connection_specific_smtp);

G_END_DECLS

#endif /* __MODEST_ACCOUNT_SETTINGS_H__ */

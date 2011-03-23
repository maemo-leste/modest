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

#include <modest-protocol-registry.h>
#include <modest-server-account-settings.h>
#include <strings.h>
#include <string.h>

/* 'private'/'protected' functions */
static void   modest_server_account_settings_class_init (ModestServerAccountSettingsClass *klass);
static void   modest_server_account_settings_finalize   (GObject *obj);
static void   modest_server_account_settings_instance_init (ModestServerAccountSettings *obj);

typedef struct _ModestServerAccountSettingsPrivate ModestServerAccountSettingsPrivate;
struct _ModestServerAccountSettingsPrivate {
	gchar *hostname;
	guint port;
	ModestProtocolType protocol;
	gchar *username;
	gchar *password;
	ModestProtocolType security_protocol;
	ModestProtocolType auth_protocol;
	gchar *account_name;
	gchar *uri;
	gboolean offline_sync;
};

#define MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
							   MODEST_TYPE_SERVER_ACCOUNT_SETTINGS, \
							   ModestServerAccountSettingsPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_server_account_settings_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestServerAccountSettingsClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) modest_server_account_settings_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ModestServerAccountSettings),
			0,      /* n_preallocs */
			(GInstanceInitFunc) modest_server_account_settings_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestServerAccountSettings",
						  &my_info, 0);
	}
	return my_type;
}

static void
modest_server_account_settings_class_init (ModestServerAccountSettingsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_server_account_settings_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof(ModestServerAccountSettingsPrivate));
}

static void
modest_server_account_settings_instance_init (ModestServerAccountSettings *obj)
{
	ModestServerAccountSettingsPrivate *priv;

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (obj);

	priv->hostname = NULL;
	priv->protocol = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	priv->port = 0;
	priv->username = NULL;
	priv->password = NULL;
	priv->security_protocol = MODEST_PROTOCOLS_CONNECTION_NONE;
	priv->auth_protocol = MODEST_PROTOCOLS_AUTH_NONE;
	priv->account_name = NULL;
	priv->uri = NULL;
	priv->offline_sync = FALSE;
}

static void   
modest_server_account_settings_finalize   (GObject *obj)
{
	ModestServerAccountSettings *settings = MODEST_SERVER_ACCOUNT_SETTINGS (obj);
	ModestServerAccountSettingsPrivate *priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->hostname);
	priv->hostname = NULL;
	g_free (priv->username);
	priv->username = NULL;

	if (priv->password) {
		bzero (priv->password, strlen (priv->password));
		g_free (priv->password);
	}
	priv->password = NULL;

	priv->protocol = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	priv->port = 0;
	priv->security_protocol = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	priv->auth_protocol = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	g_free (priv->account_name);
	priv->account_name = NULL;
	g_free (priv->uri);
	priv->uri = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

ModestServerAccountSettings*
modest_server_account_settings_new (void)
{
	return g_object_new (MODEST_TYPE_SERVER_ACCOUNT_SETTINGS, NULL);
}

const gchar* 
modest_server_account_settings_get_hostname (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->hostname;
}

void         
modest_server_account_settings_set_hostname (ModestServerAccountSettings *settings,
					     const gchar *hostname)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->hostname);
	priv->hostname = g_strdup (hostname);
}

const gchar* 
modest_server_account_settings_get_uri (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->uri;
}

void         
modest_server_account_settings_set_uri (ModestServerAccountSettings *settings,
					const gchar *uri)
{
	ModestServerAccountSettingsPrivate *priv;
	
	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->uri);
	priv->uri = g_strdup (uri);

}

const gchar* 
modest_server_account_settings_get_username (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->username;
}

void         
modest_server_account_settings_set_username (ModestServerAccountSettings *settings,
					     const gchar *username)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->username);
	priv->username = g_strdup (username);
}

const gchar* 
modest_server_account_settings_get_password (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->password;
}

void         
modest_server_account_settings_set_password (ModestServerAccountSettings *settings,
					     const gchar *password)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	if (priv->password) {
		bzero (priv->password, strlen (priv->password));
		g_free (priv->password);
	}
	priv->password = g_strdup (password);
}

const gchar* 
modest_server_account_settings_get_account_name (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->account_name;
}

void         
modest_server_account_settings_set_account_name (ModestServerAccountSettings *settings,
						 const gchar *account_name)
{
	ModestServerAccountSettingsPrivate *priv;

	/* be careful. This method should only be used internally in #ModestAccountMgr and
	 * #ModestAccountSettings. */

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->account_name);
	priv->account_name = g_strdup (account_name);
}

ModestProtocolType
modest_server_account_settings_get_protocol (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), MODEST_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->protocol;
}

void                          
modest_server_account_settings_set_protocol (ModestServerAccountSettings *settings,
					     ModestProtocolType protocol)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->protocol = protocol;
	
}

guint  
modest_server_account_settings_get_port (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->port;
}

void   
modest_server_account_settings_set_port (ModestServerAccountSettings *settings,
					 guint port)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->port = port;
}

ModestProtocolType
modest_server_account_settings_get_security_protocol (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), MODEST_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->security_protocol;
}

void   
modest_server_account_settings_set_security_protocol (ModestServerAccountSettings *settings,
						      ModestProtocolType security_protocol)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->security_protocol = security_protocol;
}

ModestProtocolType
modest_server_account_settings_get_auth_protocol (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), MODEST_PROTOCOL_REGISTRY_TYPE_INVALID);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->auth_protocol;
}

void   
modest_server_account_settings_set_auth_protocol (ModestServerAccountSettings *settings,
						  ModestProtocolType auth_protocol)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->auth_protocol = auth_protocol;
}

gboolean 
modest_server_account_settings_get_offline_sync (ModestServerAccountSettings *settings)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->offline_sync;
}

void   
modest_server_account_settings_set_offline_sync (ModestServerAccountSettings *settings,
						 gboolean offline_sync)
{
	ModestServerAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_SERVER_ACCOUNT_SETTINGS (settings));

	priv = MODEST_SERVER_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->offline_sync = offline_sync;
}


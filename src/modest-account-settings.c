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

#include <modest-account-settings.h>

/* 'private'/'protected' functions */
static void   modest_account_settings_class_init (ModestAccountSettingsClass *klass);
static void   modest_account_settings_finalize   (GObject *obj);
static void   modest_account_settings_instance_init (ModestAccountSettings *obj);

typedef struct _ModestAccountSettingsPrivate ModestAccountSettingsPrivate;
struct _ModestAccountSettingsPrivate {
	gchar *fullname;
	gchar *email_address;
	ModestAccountRetrieveType retrieve_type;
	gint retrieve_limit;
	gchar *display_name;
	gchar *account_name;
	ModestServerAccountSettings *store_settings;
	ModestServerAccountSettings *transport_settings;
	gboolean enabled;
	gboolean is_default;
	gboolean leave_messages_on_server;
	gboolean use_signature;
	gchar *signature;
	gboolean use_connection_specific_smtp;
};

#define MODEST_ACCOUNT_SETTINGS_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
						    MODEST_TYPE_ACCOUNT_SETTINGS, \
						    ModestAccountSettingsPrivate))

/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_account_settings_get_type (void)
{
	static GType my_type = 0;

	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountSettingsClass),
			NULL,   /* base init */
			NULL,   /* base finalize */
			(GClassInitFunc) modest_account_settings_class_init,
			NULL,   /* class finalize */
			NULL,   /* class data */
			sizeof(ModestAccountSettings),
			0,      /* n_preallocs */
			(GInstanceInitFunc) modest_account_settings_instance_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestAccountSettings",
						  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_settings_class_init (ModestAccountSettingsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_settings_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof(ModestAccountSettingsPrivate));
}

static void
modest_account_settings_instance_init (ModestAccountSettings *obj)
{
	ModestAccountSettingsPrivate *priv;

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (obj);

	priv->fullname = NULL;
	priv->email_address = NULL;
	priv->retrieve_type = MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY;
	priv->retrieve_limit = 0;
	priv->display_name = NULL;
	priv->account_name = NULL;
	priv->store_settings = NULL;
	priv->transport_settings = NULL;
	priv->enabled = TRUE;
	priv->is_default = FALSE;
	priv->leave_messages_on_server = TRUE;
	priv->use_signature = FALSE;
	priv->signature = FALSE;
	priv->use_connection_specific_smtp = FALSE;
}

static void   
modest_account_settings_finalize   (GObject *obj)
{
	ModestAccountSettings *settings = MODEST_ACCOUNT_SETTINGS (obj);
	ModestAccountSettingsPrivate *priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->fullname);
	priv->fullname = NULL;
	g_free (priv->email_address);
	priv->email_address = NULL;
	g_free (priv->display_name);
	priv->display_name = NULL;
	g_free (priv->account_name);
	priv->account_name = NULL;
	g_free (priv->signature);
	priv->signature = FALSE;
	if (priv->store_settings) {
		g_object_unref (priv->store_settings);
		priv->store_settings = NULL;
	}
	if (priv->transport_settings) {
		g_object_unref (priv->transport_settings);
		priv->transport_settings = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

ModestAccountSettings*
modest_account_settings_new (void)
{
	return g_object_new (MODEST_TYPE_ACCOUNT_SETTINGS, NULL);
}

const gchar* 
modest_account_settings_get_fullname (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->fullname;
}

void         
modest_account_settings_set_fullname (ModestAccountSettings *settings,
					     const gchar *fullname)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->fullname);
	priv->fullname = g_strdup (fullname);
}

const gchar* 
modest_account_settings_get_email_address (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->email_address;
}

void         
modest_account_settings_set_email_address (ModestAccountSettings *settings,
					     const gchar *email_address)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->email_address);
	priv->email_address = g_strdup (email_address);
}

const gchar* 
modest_account_settings_get_display_name (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->display_name;
}

void         
modest_account_settings_set_display_name (ModestAccountSettings *settings,
					     const gchar *display_name)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->display_name);
	priv->display_name = g_strdup (display_name);
}

const gchar* 
modest_account_settings_get_account_name (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->account_name;
}

void         
modest_account_settings_set_account_name (ModestAccountSettings *settings,
						 const gchar *account_name)
{
	ModestAccountSettingsPrivate *priv;

	/* be careful. This method should only be used internally in #ModestAccountMgr and
	 * #ModestAccountSettings. */

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->account_name);
	priv->account_name = g_strdup (account_name);
}

ModestAccountRetrieveType  
modest_account_settings_get_retrieve_type (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->retrieve_type;
}

void                          
modest_account_settings_set_retrieve_type (ModestAccountSettings *settings,
					   ModestAccountRetrieveType retrieve_type)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->retrieve_type = retrieve_type;
}

gint  
modest_account_settings_get_retrieve_limit (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->retrieve_limit;
}

void   
modest_account_settings_set_retrieve_limit (ModestAccountSettings *settings,
					    gint retrieve_limit)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->retrieve_limit = retrieve_limit;
}

gboolean 
modest_account_settings_get_enabled (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->enabled;
}

void   
modest_account_settings_set_enabled (ModestAccountSettings *settings,
				     gboolean enabled)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->enabled = enabled;
}

gboolean 
modest_account_settings_get_is_default (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->is_default;
}

void   
modest_account_settings_set_is_default (ModestAccountSettings *settings,
				     gboolean is_default)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->is_default = is_default;
}

ModestServerAccountSettings * 
modest_account_settings_get_store_settings (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	if (!priv->store_settings)
		priv->store_settings = modest_server_account_settings_new ();
	return g_object_ref (priv->store_settings);
}

void   
modest_account_settings_set_store_settings (ModestAccountSettings *settings,
					    ModestServerAccountSettings *store_settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);

	if (priv->store_settings) {
		g_object_unref (priv->store_settings);
		priv->store_settings = NULL;
	}

	if (MODEST_IS_SERVER_ACCOUNT_SETTINGS (store_settings))
		priv->store_settings = g_object_ref (store_settings);
}

ModestServerAccountSettings * 
modest_account_settings_get_transport_settings (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	if (!priv->transport_settings)
		priv->transport_settings = modest_server_account_settings_new ();
	return g_object_ref (priv->transport_settings);
}

void   
modest_account_settings_set_transport_settings (ModestAccountSettings *settings,
					    ModestServerAccountSettings *transport_settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);

	if (priv->transport_settings) {
		g_object_unref (priv->transport_settings);
		priv->transport_settings = NULL;
	}

	if (MODEST_IS_SERVER_ACCOUNT_SETTINGS (transport_settings))
		priv->transport_settings = g_object_ref (transport_settings);
}

gboolean 
modest_account_settings_get_use_signature (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->use_signature;
}

void   
modest_account_settings_set_use_signature (ModestAccountSettings *settings,
				     gboolean use_signature)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->use_signature = use_signature;
}

const gchar* 
modest_account_settings_get_signature (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), NULL);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);	
	return priv->signature;
}

void         
modest_account_settings_set_signature (ModestAccountSettings *settings,
					     const gchar *signature)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	g_free (priv->signature);
	priv->signature = g_strdup (signature);
}

gboolean 
modest_account_settings_get_leave_messages_on_server (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->leave_messages_on_server;
}

void   
modest_account_settings_set_leave_messages_on_server (ModestAccountSettings *settings,
				     gboolean leave_messages_on_server)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->leave_messages_on_server = leave_messages_on_server;
}

gboolean 
modest_account_settings_get_use_connection_specific_smtp (ModestAccountSettings *settings)
{
	ModestAccountSettingsPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings), 0);

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	return priv->use_connection_specific_smtp;
}

void   
modest_account_settings_set_use_connection_specific_smtp (ModestAccountSettings *settings,
							  gboolean use_connection_specific_smtp)
{
	ModestAccountSettingsPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_ACCOUNT_SETTINGS_GET_PRIVATE (settings);
	priv->use_connection_specific_smtp = use_connection_specific_smtp;
}


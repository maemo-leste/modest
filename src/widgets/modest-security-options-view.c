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

#include <string.h>
#include <gtk/gtk.h>
#include "modest-utils.h"
#include "modest-runtime.h"
#include "modest-platform.h"
#include "modest-security-options-view.h"
#include "modest-security-options-view-priv.h"
#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-hildon-includes.h>
#endif

/* list my signals */
enum {
	MISSING_MANDATORY_DATA_SIGNAL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

void 
modest_security_options_view_load_settings (ModestSecurityOptionsView* self, 
					    ModestAccountSettings *settings)
{
	ModestSecurityOptionsViewPrivate *priv;
	ModestServerAccountSettings *server_settings;
	ModestProtocolType server_proto, secure_protocol, secure_auth;

	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	/* Save initial settings */
	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING)
		server_settings = modest_account_settings_get_store_settings (settings);
	else
		server_settings = modest_account_settings_get_transport_settings (settings);

	server_proto = modest_server_account_settings_get_protocol (server_settings);
	secure_protocol = modest_server_account_settings_get_security_protocol (server_settings);
	secure_auth = modest_server_account_settings_get_auth_protocol (server_settings);

	priv->initial_state.security = secure_protocol;
	priv->initial_state.auth = secure_auth;
	priv->initial_state.port = modest_server_account_settings_get_port (server_settings);

	/* Update UI */
	modest_security_options_view_set_server_type (self, server_proto);
	modest_serversecurity_selector_set_active_serversecurity (priv->security_view, secure_protocol);

/* 		update_incoming_server_title (dialog, dialog->incoming_protocol); */

	/* Username and password */
	if (priv->full && self->type == MODEST_SECURITY_OPTIONS_OUTGOING) {
		priv->initial_state.user = 
			modest_server_account_settings_get_username (server_settings);
		priv->initial_state.pwd = 
			modest_server_account_settings_get_password (server_settings);

		if (priv->initial_state.user)
			gtk_entry_set_text(GTK_ENTRY (priv->user_entry), 
					   priv->initial_state.user);
		if (priv->initial_state.pwd)
			gtk_entry_set_text(GTK_ENTRY (priv->pwd_entry), 
					   priv->initial_state.pwd);
	}

	/* Set auth */
	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING) {
		/* Active the authentication checkbox */
		if (modest_protocol_registry_protocol_type_is_secure (modest_runtime_get_protocol_registry (), 
								      secure_auth))
			modest_togglable_set_active (priv->auth_view,
						     TRUE);
	} else {
		modest_secureauth_selector_set_active_secureauth (
			priv->auth_view, secure_auth);
	}

	MODEST_SECURITY_OPTIONS_VIEW_GET_CLASS (self)->load_settings (self, settings);

	/* Free */
	g_object_unref (server_settings);
}

void 
modest_security_options_view_save_settings (ModestSecurityOptionsView* self, 
					    ModestAccountSettings *settings)
{
	ModestServerAccountSettings *server_settings;
	ModestProtocolType security_proto, auth_protocol;
 	ModestSecurityOptionsViewPrivate *priv;
	ModestProtocolRegistry *proto_registry;

	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	proto_registry = modest_runtime_get_protocol_registry ();

	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING)
		server_settings = modest_account_settings_get_store_settings (settings);
	else
		server_settings = modest_account_settings_get_transport_settings (settings);

	/* initialize */
	security_proto = MODEST_PROTOCOLS_CONNECTION_NONE;
	auth_protocol = MODEST_PROTOCOLS_AUTH_NONE;

	/* Get data */
	security_proto = modest_serversecurity_selector_get_active_serversecurity (priv->security_view);

	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING) {
		if (modest_togglable_get_active (priv->auth_view)) {
			if (!modest_protocol_registry_protocol_type_is_secure (proto_registry,
									       security_proto)) {
				/* TODO */
				/* 		auth_protocol = check_first_supported_auth_method (self); */
				auth_protocol = MODEST_PROTOCOLS_AUTH_PASSWORD;
			} else {
				auth_protocol = MODEST_PROTOCOLS_AUTH_PASSWORD;
			}
		}
	} else {
		auth_protocol = modest_secureauth_selector_get_active_secureauth (priv->auth_view);
	}

	/* Save settings */
	modest_server_account_settings_set_security_protocol (server_settings, 
							      security_proto);
	modest_server_account_settings_set_auth_protocol (server_settings, 
							  auth_protocol);

	if (priv->full && self->type == MODEST_SECURITY_OPTIONS_OUTGOING) {
		const gchar *username, *password;

		username = gtk_entry_get_text (GTK_ENTRY (priv->user_entry));
		password = gtk_entry_get_text (GTK_ENTRY (priv->pwd_entry));

		modest_server_account_settings_set_username (server_settings, username);
		modest_server_account_settings_set_password (server_settings, password);
	}

	MODEST_SECURITY_OPTIONS_VIEW_GET_CLASS (self)->save_settings (self, settings);


	/* Free */
	g_object_unref (server_settings);
}

void 
modest_security_options_view_set_server_type (ModestSecurityOptionsView* self, 
					      ModestProtocolType server_type)
{
 	ModestSecurityOptionsViewPrivate *priv;
	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	modest_serversecurity_selector_fill (priv->security_view, server_type);
	modest_serversecurity_selector_set_active_serversecurity (priv->security_view,
								  MODEST_PROTOCOLS_CONNECTION_NONE);
}

static void
get_current_state (ModestSecurityOptionsView* self,
		   ModestSecurityOptionsState *state)
{
	ModestSecurityOptionsViewPrivate *priv;
	ModestProtocolRegistry *proto_registry;

	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	proto_registry = modest_runtime_get_protocol_registry ();

	/* Get security */
	state->security =
		modest_serversecurity_selector_get_active_serversecurity (priv->security_view);
	state->port =
		modest_serversecurity_selector_get_active_serversecurity_port (priv->security_view);

	/* Get auth */
	if (self->type == MODEST_SECURITY_OPTIONS_OUTGOING) {
		state->auth = modest_secureauth_selector_get_active_secureauth (priv->auth_view);
		if (priv->full) {
		}
	} else {
		if (modest_togglable_get_active (priv->auth_view))
			state->auth = priv->initial_state.auth;
		else
			state->auth = MODEST_PROTOCOLS_AUTH_NONE;
	}
}

gboolean 
modest_security_options_view_changed (ModestSecurityOptionsView* self,
				      ModestAccountSettings *settings)
{
	ModestSecurityOptionsViewPrivate *priv;
	ModestSecurityOptionsState state = {0};

	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	get_current_state (self, &state);

	if (state.security != priv->initial_state.security ||
	    state.auth != priv->initial_state.auth)
		return TRUE;

	if (priv->full && self->type == MODEST_SECURITY_OPTIONS_OUTGOING) {
		const gchar *username, *password;

		username = gtk_entry_get_text (GTK_ENTRY (priv->user_entry));
		password = gtk_entry_get_text (GTK_ENTRY (priv->pwd_entry));

		if (!priv->initial_state.user && strcmp (username, ""))
			return TRUE;
		if (!priv->initial_state.pwd && strcmp (password, ""))
			return TRUE;

		if ((priv->initial_state.user && 
		     strcmp (priv->initial_state.user, username)) ||
		    (priv->initial_state.pwd &&
		     strcmp (priv->initial_state.pwd, password)))
			return TRUE;
	}

	/* Check subclass */
	return 	MODEST_SECURITY_OPTIONS_VIEW_GET_CLASS (self)->changed (self, settings);
}

void 
modest_security_options_view_enable_changes (ModestSecurityOptionsView* self,
					     gboolean enable)
{
	ModestSecurityOptionsViewPrivate *priv;

	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	gtk_widget_set_sensitive (priv->port_view, enable);
	gtk_widget_set_sensitive (priv->security_view, enable);
}

gboolean 
modest_security_options_view_auth_check (ModestSecurityOptionsView* self)
{
	ModestSecurityOptionsViewPrivate *priv;
	ModestProtocolType security_incoming_type; 
	gboolean auth_active, is_secure;
	ModestProtocolRegistry *protocol_registry;

	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	protocol_registry = modest_runtime_get_protocol_registry ();

	/* Check if the server supports secure authentication */
	security_incoming_type = 
		modest_serversecurity_selector_get_active_serversecurity (priv->security_view);

	auth_active = 
		modest_togglable_get_active (priv->auth_view);
	is_secure = 
		modest_protocol_registry_protocol_type_has_tag (protocol_registry, 
								security_incoming_type, 
								MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS);

	if (auth_active && !is_secure)
		return TRUE;
	else
		return FALSE;
}

ModestProtocolType 
modest_security_options_view_get_connection_protocol (ModestSecurityOptionsView *self)
{
	ModestSecurityOptionsViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_SECURITY_OPTIONS_VIEW (self), MODEST_PROTOCOL_REGISTRY_TYPE_INVALID);
	priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	return modest_serversecurity_selector_get_active_serversecurity (priv->security_view);
}

static void 
modest_security_options_view_init (ModestSecurityOptionsView *self) 
{
	ModestSecurityOptionsViewPrivate *priv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	memset (&(priv->initial_state), 0, sizeof (ModestSecurityOptionsState));

	priv->security_view = NULL;
	priv->port_view = NULL;
	priv->auth_view = NULL;
	priv->user_entry = NULL;
	priv->pwd_entry = NULL;
	priv->full = FALSE;
	priv->changed = FALSE;
}

static void 
modest_security_options_view_class_init (ModestSecurityOptionsViewClass *klass) 
{
	GObjectClass *gobject_class = (GObjectClass*) klass;

	g_type_class_add_private (gobject_class, sizeof (ModestSecurityOptionsViewPrivate));

	/* Register signals */
	signals[MISSING_MANDATORY_DATA_SIGNAL] =
		g_signal_new ("missing_mandatory_data",
			      MODEST_TYPE_SECURITY_OPTIONS_VIEW,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestSecurityOptionsViewClass, missing_mandatory_data),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

/* Type definition */
G_DEFINE_ABSTRACT_TYPE (ModestSecurityOptionsView, 
			modest_security_options_view,
			GTK_TYPE_VBOX);

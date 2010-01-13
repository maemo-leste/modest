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

#include "modest-runtime.h"
#include "modest-security-options-view-priv.h"
#include "modest-maemo-security-options-view.h"
#include "modest-text-utils.h"
#include "modest-platform.h"
#include "modest-account-protocol.h"
#include "widgets/modest-ui-constants.h"
#include "widgets/modest-validating-entry.h"
#include "modest-maemo-utils.h"
#include "modest-hildon-includes.h"
#include <modest-toolkit-factory.h>
#include <modest-toolkit-utils.h>

#define PORT_MIN 1
#define PORT_MAX 65535

typedef struct _ModestMaemoSecurityOptionsViewPrivate ModestMaemoSecurityOptionsViewPrivate;
struct _ModestMaemoSecurityOptionsViewPrivate {
	gboolean missing_data;
};

#define MODEST_MAEMO_SECURITY_OPTIONS_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), \
				     MODEST_TYPE_MAEMO_SECURITY_OPTIONS_VIEW, \
				     ModestMaemoSecurityOptionsViewPrivate))

static void modest_maemo_security_options_view_init (ModestMaemoSecurityOptionsView *obj);
static void modest_maemo_security_options_view_finalize (GObject *obj);
static void modest_maemo_security_options_view_class_init (ModestMaemoSecurityOptionsViewClass *klass);

G_DEFINE_TYPE (ModestMaemoSecurityOptionsView, 
	       modest_maemo_security_options_view, 
	       MODEST_TYPE_SECURITY_OPTIONS_VIEW);

static void on_entry_changed (GtkEditable *editable, gpointer user_data);

#ifdef MODEST_NUMBER_ENTRY_SUPPORT_VALID_CHANGED
static void on_valid_changed (ModestNumberEditor *editor, gboolean valid, ModestSecurityOptionsView *self);
#endif

/* Tracks changes in the incoming security picker */
static void
on_security_changed (GtkWidget *widget, 
		     ModestMaemoSecurityOptionsView *self)
{
	ModestSecurityOptionsViewPrivate* ppriv;
	ModestProtocolType proto_type;
	ModestProtocolRegistry *proto_registry;
	gboolean is_secure;

	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	proto_registry = modest_runtime_get_protocol_registry ();
	proto_type = modest_serversecurity_selector_get_active_serversecurity (ppriv->security_view);

	is_secure = modest_protocol_registry_protocol_type_has_tag (proto_registry, proto_type, 
								    MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS);

	if (ppriv->full) {
		gint port_number =
			modest_serversecurity_selector_get_active_serversecurity_port (ppriv->security_view);

		if(port_number) {
			modest_number_entry_set_value (ppriv->port_view,
						       port_number);
		}
	}
}

static void
on_auth_changed (GtkWidget *widget, 
		 ModestMaemoSecurityOptionsView *self)
{
	ModestSecurityOptionsViewPrivate* ppriv;
	ModestProtocolRegistry *protocol_registry;
	ModestProtocolType auth_proto;
	gboolean secureauth_used;
	GtkWidget *user_caption, *pwd_caption;

	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	protocol_registry = modest_runtime_get_protocol_registry ();

	auth_proto = modest_secureauth_selector_get_active_secureauth (ppriv->auth_view);
	secureauth_used = modest_protocol_registry_protocol_type_is_secure (protocol_registry, 
									    auth_proto);

	/* Get captions, well dimm the whole widget */
	user_caption = gtk_widget_get_parent (ppriv->user_entry);
	pwd_caption = gtk_widget_get_parent (ppriv->pwd_entry);

	/* Enable / disable */
	gtk_widget_set_sensitive (user_caption, secureauth_used);
	gtk_widget_set_sensitive (pwd_caption, secureauth_used);

	/* Check if mandatory data is missing */
	on_entry_changed (GTK_EDITABLE (ppriv->user_entry), (gpointer) self);
}

static void
create_incoming_security (ModestSecurityOptionsView* self,
			  GtkSizeGroup *title_size_group,
			  GtkSizeGroup *value_size_group)
{
 	ModestSecurityOptionsViewPrivate *ppriv;
	GtkWidget *entry_caption = NULL;

	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	ppriv->auth_view = modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
								       _("mcen_li_emailsetup_secure_authentication"));
	gtk_box_pack_start (GTK_BOX (self), ppriv->auth_view, FALSE, FALSE, 0);

	/* Create widgets for incoming security */
	ppriv->security_view = modest_toolkit_factory_create_serversecurity_selector (modest_runtime_get_toolkit_factory ());
	modest_serversecurity_selector_fill (ppriv->security_view,
					     modest_protocol_registry_get_pop_type_id ());
	if (GTK_IS_COMBO_BOX (ppriv->security_view)) {
		GtkWidget *captioned;

		captioned = modest_toolkit_utils_create_captioned (title_size_group, value_size_group,
								   _("mcen_li_emailsetup_secure_connection"), FALSE,
								   ppriv->security_view);
		g_signal_connect (G_OBJECT (ppriv->security_view), "changed",
				  G_CALLBACK (on_security_changed), self);
		gtk_box_pack_start (GTK_BOX (self), captioned, FALSE, FALSE, 0);
		gtk_widget_show (captioned);
	} else {
		modest_toolkit_utils_set_hbutton_layout (title_size_group,
							 value_size_group,
							 _("mcen_li_emailsetup_secure_connection"), 
							 ppriv->security_view);
		g_signal_connect (G_OBJECT (ppriv->security_view), "value-changed",
				  G_CALLBACK (on_security_changed), self);
		gtk_box_pack_start (GTK_BOX (self), ppriv->security_view, FALSE, FALSE, 0);
	}

	/* Pack into container & show */
	gtk_widget_show (ppriv->security_view);
	gtk_widget_show (ppriv->auth_view);

	if (ppriv->full) {
		ppriv->port_view = modest_toolkit_factory_create_number_entry (modest_runtime_get_toolkit_factory (),
									       PORT_MIN, PORT_MAX);
		entry_caption =
			modest_toolkit_utils_create_captioned_with_size_type (title_size_group,
									      value_size_group,
									      _("mcen_fi_emailsetup_port"),
									      FALSE,
									      ppriv->port_view,
									      MODEST_EDITABLE_SIZE);
		/* Pack & show widgets */
		gtk_box_pack_start (GTK_BOX (self), entry_caption, FALSE, FALSE, 0);
		gtk_widget_show (ppriv->port_view);
		gtk_widget_show (entry_caption);

		/* Track changes in UI */
#ifdef MODEST_NUMBER_ENTRY_SUPPORT_VALID_CHANGED
		g_signal_connect (G_OBJECT (ppriv->port_view), "valid-changed",
					    G_CALLBACK (on_valid_changed), self);
#endif
	}
}

static void
on_entry_max (ModestValidatingEntry *self, 
	      gpointer user_data)
{
	modest_platform_information_banner (GTK_WIDGET (self), NULL, 
					    _CS_MAXIMUM_CHARACTERS_REACHED);
}

/*
 * TODO: call this whenever the auth picker changes. If we set it
 * explicitely at the beggining to a value then there is no need to
 * call this handler directly at the beginning
 */
static void
on_entry_changed (GtkEditable *editable, 
		  gpointer user_data)
{
	ModestSecurityOptionsView* self;
	ModestMaemoSecurityOptionsViewPrivate *priv;
 	ModestSecurityOptionsViewPrivate *ppriv;
	gboolean is_secure = FALSE;
	ModestProtocolRegistry *protocol_registry;

	self = MODEST_SECURITY_OPTIONS_VIEW (user_data);
	priv = MODEST_MAEMO_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	protocol_registry = modest_runtime_get_protocol_registry ();

	/* Check if it's a secure protocol */
	if (modest_is_secureauth_selector (ppriv->auth_view)) {
		ModestProtocolType auth_proto;
		auth_proto = modest_secureauth_selector_get_active_secureauth (ppriv->auth_view);
		is_secure = modest_protocol_registry_protocol_type_is_secure (protocol_registry,
									      auth_proto);
	} else if (modest_is_togglable (ppriv->auth_view)) {
		is_secure = modest_togglable_get_active (ppriv->auth_view);
	}

	if (is_secure &&
	    !g_strcmp0 (modest_entry_get_text (ppriv->user_entry), "")) {
		priv->missing_data = TRUE;
	} else {
		priv->missing_data = FALSE;
	}

	if (!priv->missing_data &&
	    ppriv->full &&
	    !modest_number_entry_is_valid (ppriv->port_view))
		priv->missing_data = TRUE;

	/* Emit a signal to notify if mandatory data is missing */
	g_signal_emit_by_name (G_OBJECT (self), "missing_mandatory_data",
			       priv->missing_data, NULL);
}

#ifdef MODEST_NUMBER_ENTRY_SUPPORT_VALID_CHANGED
static void
on_valid_changed (ModestNumberEditor *editor,
		  gboolean valid,
		  ModestSecurityOptionsView *self)
{
	on_entry_changed (NULL, (gpointer) self);
}
#endif

static void
create_outgoing_security (ModestSecurityOptionsView* self,
			  GtkSizeGroup *title_size_group,
			  GtkSizeGroup *value_size_group)
{
 	ModestSecurityOptionsViewPrivate *ppriv;
	GtkWidget *user_caption = NULL, *security_caption = NULL, *auth_caption = NULL;
	GtkWidget *pwd_caption = NULL, *port_caption = NULL;

	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	
	/* The secure connection widgets */
	ppriv->security_view = modest_toolkit_factory_create_serversecurity_selector (modest_runtime_get_toolkit_factory ());
	modest_serversecurity_selector_fill (ppriv->security_view,
					      MODEST_PROTOCOLS_TRANSPORT_SMTP);
	if (GTK_IS_COMBO_BOX (ppriv->security_view)) {
		security_caption = modest_toolkit_utils_create_captioned (title_size_group, value_size_group,
									  _("mcen_li_emailsetup_secure_connection"), FALSE,
									  ppriv->security_view);
		gtk_widget_show (security_caption);
	} else {
		modest_toolkit_utils_set_hbutton_layout (title_size_group,
							 value_size_group,
							 _("mcen_li_emailsetup_secure_connection"), 
							 ppriv->security_view);
		security_caption = ppriv->security_view;
	}
	
	/* The secure authentication widgets */
	ppriv->auth_view = modest_toolkit_factory_create_secureauth_selector (modest_runtime_get_toolkit_factory ());
	if (GTK_IS_COMBO_BOX (ppriv->auth_view)) {
		auth_caption = modest_toolkit_utils_create_captioned (title_size_group, value_size_group,
								      _("mcen_li_emailsetup_secure_authentication"), FALSE,
								      ppriv->auth_view);
		gtk_widget_show (auth_caption);
	} else {
		modest_toolkit_utils_set_hbutton_layout (title_size_group,
							 value_size_group,
							 _("mcen_li_emailsetup_secure_authentication"), 
							 ppriv->auth_view);
		auth_caption = ppriv->auth_view;
	}

	if (ppriv->full) {
		gchar *user_label;

		/* Username widgets */
		ppriv->user_entry = GTK_WIDGET (modest_validating_entry_new ());

		/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MODEST_TOOLKIT_HILDON2
		hildon_gtk_entry_set_input_mode (GTK_ENTRY (ppriv->user_entry), 
						 HILDON_GTK_INPUT_MODE_FULL);
#endif

		user_label = g_strdup_printf("%s*", _("mail_fi_username"));
		user_caption = modest_toolkit_utils_create_captioned_with_size_type (title_size_group,
										     value_size_group,
										     user_label,
										     FALSE,
										     ppriv->user_entry,
										     MODEST_EDITABLE_SIZE);
		g_free (user_label);
	
		/* Prevent the use of some characters. Limit the max
		   length as well */
		modest_validating_entry_set_unallowed_characters_whitespace (
		     MODEST_VALIDATING_ENTRY (ppriv->user_entry));	
		gtk_entry_set_max_length (GTK_ENTRY (ppriv->user_entry), 64);
		modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (ppriv->user_entry),
						      on_entry_max, self);
		
		/* Password widgets */
		ppriv->pwd_entry = modest_toolkit_factory_create_entry (modest_runtime_get_toolkit_factory ());

		/* Auto-capitalization is the default, so let's turn it off */
#ifdef MODEST_TOOLKIT_HILDON2
		hildon_gtk_entry_set_input_mode (GTK_ENTRY (ppriv->pwd_entry),
						 HILDON_GTK_INPUT_MODE_FULL | 
						 HILDON_GTK_INPUT_MODE_INVISIBLE);
#endif
		gtk_entry_set_visibility (GTK_ENTRY (ppriv->pwd_entry), FALSE);

		pwd_caption =
			modest_toolkit_utils_create_captioned_with_size_type (title_size_group,
									      value_size_group,
									      _("mail_fi_password"),
									      FALSE,
									      ppriv->pwd_entry,
									      MODEST_EDITABLE_SIZE);

		ppriv->port_view = modest_toolkit_factory_create_number_entry (modest_runtime_get_toolkit_factory (),
									       PORT_MIN, PORT_MAX);
		port_caption =
			modest_toolkit_utils_create_captioned_with_size_type (title_size_group,
									      value_size_group,
									      _("mcen_fi_emailsetup_port"),
									      FALSE,
									      ppriv->port_view,
									      MODEST_EDITABLE_SIZE);
	}

	/* Track changes in UI */
	if (GTK_IS_COMBO_BOX (ppriv->security_view)) {
		g_signal_connect (G_OBJECT (ppriv->security_view), "changed",
				  G_CALLBACK (on_security_changed), self);
	} else {
		g_signal_connect (G_OBJECT (ppriv->security_view), "value-changed",
				  G_CALLBACK (on_security_changed), self);
	}
	if (ppriv->full) {
		if (GTK_IS_COMBO_BOX (ppriv->auth_view)) {
			g_signal_connect (G_OBJECT (ppriv->auth_view), "changed",
					  G_CALLBACK (on_auth_changed), self);
		} else {
			g_signal_connect (G_OBJECT (ppriv->auth_view), "value-changed",
					  G_CALLBACK (on_auth_changed), self);
		}
		g_signal_connect (G_OBJECT (ppriv->user_entry), "changed",
				  G_CALLBACK (on_entry_changed), self);
#ifdef MODEST_NUMBER_ENTRY_SUPPORT_VALID_CHANGED
		g_signal_connect (G_OBJECT (ppriv->port_view), "valid-changed",
					    G_CALLBACK (on_valid_changed), self);
#endif
	}

	/* Initialize widgets */
	modest_serversecurity_selector_set_active_serversecurity (
		ppriv->security_view,
		MODEST_PROTOCOLS_CONNECTION_NONE);
	modest_secureauth_selector_set_active_secureauth (
	   ppriv->auth_view,
	   MODEST_PROTOCOLS_AUTH_NONE);

	/* Pack into container */
	if (ppriv->full) {
		gtk_box_pack_start (GTK_BOX (self), auth_caption, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (self), user_caption, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (self), pwd_caption, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (self), security_caption, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (self), port_caption, FALSE, FALSE, 0);
	} else {
		gtk_box_pack_start (GTK_BOX (self), auth_caption, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (self), security_caption, FALSE, FALSE, 0);
	}

	/* Show widgets */
	if (ppriv->full) {
		gtk_widget_show (ppriv->pwd_entry);
		gtk_widget_show (ppriv->user_entry);
		gtk_widget_show (ppriv->port_view);
		gtk_widget_show (pwd_caption);
		gtk_widget_show (user_caption);
		gtk_widget_show (port_caption);
	}
	gtk_widget_show (ppriv->security_view);
	gtk_widget_show (ppriv->auth_view);
}

GtkWidget *    
modest_maemo_security_options_view_new  (ModestSecurityOptionsType type,
					 gboolean full,
					 GtkSizeGroup *title_size_group,
					 GtkSizeGroup *value_size_group)
{
	ModestSecurityOptionsView* self;
 	ModestSecurityOptionsViewPrivate *ppriv;

	self = (ModestSecurityOptionsView *)
		g_object_new (MODEST_TYPE_MAEMO_SECURITY_OPTIONS_VIEW, NULL);
	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	ppriv->full = full;
	self->type = type;
	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING)
		create_incoming_security (self, title_size_group, value_size_group);
	else
		create_outgoing_security (self, title_size_group, value_size_group);

	return (GtkWidget *) self;
}

gboolean
modest_security_options_view_has_missing_mandatory_data (ModestSecurityOptionsView* self)
{
 	ModestMaemoSecurityOptionsViewPrivate *priv;

	priv = MODEST_MAEMO_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	return priv->missing_data;
}

static void
modest_maemo_security_options_view_load_settings (ModestSecurityOptionsView* self,
						  ModestAccountSettings *settings)
{
 	ModestSecurityOptionsViewPrivate *ppriv;
	ModestServerAccountSettings *server_settings;
	gint port_number;

	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING)
		server_settings = modest_account_settings_get_store_settings (settings);
	else
		server_settings = modest_account_settings_get_transport_settings (settings);
	port_number = modest_server_account_settings_get_port (server_settings);

	if (port_number == 0) {
		/* Show the appropriate port number */
		on_security_changed (ppriv->security_view, 
				     MODEST_MAEMO_SECURITY_OPTIONS_VIEW (self));
	} else if (ppriv->full) {
		/* Keep the user-entered port-number, or the
		 * already-appropriate automatic port number */
		modest_number_entry_set_value (ppriv->port_view,
					       port_number);
	}
	/* Frees */
	g_object_unref (server_settings);
}

static void
modest_maemo_security_options_view_save_settings (ModestSecurityOptionsView* self, 
						  ModestAccountSettings *settings)
{
	ModestServerAccountSettings *server_settings;
 	ModestSecurityOptionsViewPrivate *ppriv;
	gint server_port;

	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);

	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING)
		server_settings = modest_account_settings_get_store_settings (settings);
	else
		server_settings = modest_account_settings_get_transport_settings (settings);

	if (ppriv->full) {
		server_port = modest_number_entry_get_value (ppriv->port_view);
	} else {
		server_port = modest_serversecurity_selector_get_active_serversecurity_port (ppriv->security_view);
	}

	modest_server_account_settings_set_port (server_settings, server_port);

	/* Frees */
	g_object_unref (server_settings);
}

static gboolean 
modest_maemo_security_options_view_changed (ModestSecurityOptionsView* self,
					    ModestAccountSettings *settings)
{
	ModestServerAccountSettings *server_settings;
	ModestSecurityOptionsViewPrivate *ppriv;
	gint server_port;

	ppriv = MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE (self);
	
	/* If we're not showing the port number then it never changes */
	if (!ppriv->full)
		return FALSE;

	if (self->type == MODEST_SECURITY_OPTIONS_INCOMING)
		server_settings = modest_account_settings_get_store_settings (settings);
	else
		server_settings = modest_account_settings_get_transport_settings (settings);
	
	server_port = 
		modest_number_entry_get_value (ppriv->port_view);

	/* Frees */
	g_object_unref (server_settings);

	if (server_port != ppriv->initial_state.port)
		return TRUE;
	else
		return FALSE;
}

static void
modest_maemo_security_options_view_init (ModestMaemoSecurityOptionsView *obj)
{
}

static void
modest_maemo_security_options_view_finalize (GObject *obj)
{
	G_OBJECT_CLASS (modest_maemo_security_options_view_parent_class)->finalize (obj);
}


static void     
modest_maemo_security_options_view_class_init (ModestMaemoSecurityOptionsViewClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass*) klass;

	modest_maemo_security_options_view_parent_class = g_type_class_peek_parent (klass);

	g_type_class_add_private (gobject_class, sizeof (ModestMaemoSecurityOptionsViewPrivate));
	gobject_class->finalize = modest_maemo_security_options_view_finalize;

	MODEST_SECURITY_OPTIONS_VIEW_CLASS (klass)->load_settings = 
		modest_maemo_security_options_view_load_settings;
	MODEST_SECURITY_OPTIONS_VIEW_CLASS (klass)->save_settings = 
		modest_maemo_security_options_view_save_settings;
	MODEST_SECURITY_OPTIONS_VIEW_CLASS (klass)->changed = 
		modest_maemo_security_options_view_changed;
}

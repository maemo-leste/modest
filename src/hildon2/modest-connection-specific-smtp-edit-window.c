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

#include "modest-connection-specific-smtp-edit-window.h"
#include "widgets/modest-ui-constants.h"
#include "modest-hildon-includes.h"
#include "modest-runtime.h"

#include "modest-serversecurity-picker.h"
#include "modest-secureauth-picker.h"
#include "widgets/modest-validating-entry.h"
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkstock.h>
#include "modest-text-utils.h"
#include "modest-maemo-utils.h"

#include <glib/gi18n.h>

#define PORT_RANGE_MIN 1
#define PORT_RANGE_MAX 65535

G_DEFINE_TYPE (ModestConnectionSpecificSmtpEditWindow, modest_connection_specific_smtp_edit_window, GTK_TYPE_DIALOG);

#define CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW, ModestConnectionSpecificSmtpEditWindowPrivate))

static void on_response (GtkDialog *dialog,
			 gint arg1,
			 gpointer user_data);

typedef struct _ModestConnectionSpecificSmtpEditWindowPrivate ModestConnectionSpecificSmtpEditWindowPrivate;

struct _ModestConnectionSpecificSmtpEditWindowPrivate
{
	GtkWidget *entry_outgoingserver;
	GtkWidget *outgoing_auth_picker;
	GtkWidget *entry_user_username;
	GtkWidget *entry_user_password;
	GtkWidget *outgoing_security_picker;
	GtkWidget *entry_port;
	
	GtkWidget *button_ok;
	GtkWidget *button_cancel;

	gchar     *account_name;
	
	gboolean is_dirty;
	gboolean range_error_occured;
	guint range_error_banner_timeout;
};

static void
modest_connection_specific_smtp_edit_window_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_connection_specific_smtp_edit_window_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_connection_specific_smtp_edit_window_dispose (GObject *object)
{

	
	if (G_OBJECT_CLASS (modest_connection_specific_smtp_edit_window_parent_class)->dispose)
		G_OBJECT_CLASS (modest_connection_specific_smtp_edit_window_parent_class)->dispose (object);
}

static void
modest_connection_specific_smtp_edit_window_finalize (GObject *object)
{
	ModestConnectionSpecificSmtpEditWindow *self = (ModestConnectionSpecificSmtpEditWindow *) object;
	ModestConnectionSpecificSmtpEditWindowPrivate *priv =
       		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);

	if (priv->range_error_banner_timeout > 0) {
		g_source_remove (priv->range_error_banner_timeout);
		priv->range_error_banner_timeout = 0;
	}
	if (priv->account_name) {
		g_free (priv->account_name);
		priv->account_name = NULL;
	}
	G_OBJECT_CLASS (modest_connection_specific_smtp_edit_window_parent_class)->finalize (object);
}

static void
modest_connection_specific_smtp_edit_window_class_init (ModestConnectionSpecificSmtpEditWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestConnectionSpecificSmtpEditWindowPrivate));

	object_class->get_property = modest_connection_specific_smtp_edit_window_get_property;
	object_class->set_property = modest_connection_specific_smtp_edit_window_set_property;
	object_class->dispose = modest_connection_specific_smtp_edit_window_dispose;
	object_class->finalize = modest_connection_specific_smtp_edit_window_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0,
	MODEL_COL_SERVER_NAME = 1,
	MODEL_COL_ID = 2
};

static void
on_change(GtkWidget* widget, ModestConnectionSpecificSmtpEditWindow *self)
{
	ModestConnectionSpecificSmtpEditWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);
	priv->is_dirty = TRUE;
}

static void
on_value_changed(GtkWidget* widget, GValue* value, ModestConnectionSpecificSmtpEditWindow *self)
{
	ModestConnectionSpecificSmtpEditWindowPrivate *priv =
       		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);

	priv->range_error_occured = FALSE;
	on_change(widget, self);
}

gboolean
show_banner_handler (gpointer userdata)
{
	ModestConnectionSpecificSmtpEditWindow *self = userdata;
	ModestConnectionSpecificSmtpEditWindowPrivate *priv =
       		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);
	gchar *msg;

	msg = g_strdup_printf (dgettext("hildon-libs", "ckct_ib_set_a_value_within_range"), PORT_RANGE_MIN, PORT_RANGE_MAX);

	hildon_banner_show_information (NULL, NULL, msg);
	g_free (msg);

	priv->range_error_banner_timeout = 0;
	return FALSE;
}

static gboolean
on_range_error (GtkWidget *widget, HildonNumberEditorErrorType type, gpointer user_data)
{
	ModestConnectionSpecificSmtpEditWindow *self = user_data;
	ModestConnectionSpecificSmtpEditWindowPrivate *priv =
       		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);

	/* We want to prevent the closure of the dialog when a range error occured. The problem is that
	 * the hildon number editor already resets the value to the default value, so we have to
	 * remember that such an error occured. */
	priv->range_error_occured = TRUE;
	if (priv->range_error_banner_timeout == 0)
		priv->range_error_banner_timeout = g_timeout_add (200, show_banner_handler, self);

	/* Show error message by not returning TRUE */
	return TRUE;
}

static void
on_response (GtkDialog *dialog, int response_id, gpointer user_data)
{
	const gchar *hostname;
	ModestConnectionSpecificSmtpEditWindow *self = user_data;
	ModestConnectionSpecificSmtpEditWindowPrivate *priv =
       		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);

	hostname = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoingserver));

	if ((response_id != GTK_RESPONSE_OK) &&
	    (priv->range_error_banner_timeout > 0)) {
		g_source_remove (priv->range_error_banner_timeout);
		priv->range_error_banner_timeout = 0;
	}

	/* Don't close the dialog if a range error occured */
	if(response_id == GTK_RESPONSE_OK && priv->range_error_occured)
	{
		priv->range_error_occured = FALSE;
		g_signal_stop_emission_by_name (dialog, "response");
		gtk_widget_grab_focus (priv->entry_port);
		return;
	}

	/* Don't close the dialog if a range error occured */
	if(response_id == GTK_RESPONSE_OK) {
		if (hostname && (hostname[0] != '\0') &&
		    (!modest_text_utils_validate_domain_name (hostname))) { 
			g_signal_stop_emission_by_name (dialog, "response");
			hildon_banner_show_information (NULL, NULL, _("mcen_ib_invalid_servername"));
			gtk_widget_grab_focus (priv->entry_outgoingserver);
			gtk_editable_select_region (GTK_EDITABLE (priv->entry_outgoingserver), 0, -1);
			return;
		}
	} else {
		/* Ask user if they want to discard changes */
		if (priv->is_dirty) {
			gint response;
			response = modest_platform_run_confirmation_dialog (GTK_WINDOW (user_data), 
									    _("imum_nc_wizard_confirm_lose_changes"));
			if (response != GTK_RESPONSE_OK)
				g_signal_stop_emission_by_name (dialog, "response");
		}
	}
}

static void on_set_focus_child (GtkContainer *container, GtkWidget *widget, gpointer user_data)
{
	ModestConnectionSpecificSmtpEditWindow *self = user_data;
	ModestConnectionSpecificSmtpEditWindowPrivate *priv =
       		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);

	/* Another child gained focus. Since the number editor already reset a
	 * possible range error to the default value, we allow closure of the
	 * dialog */
	priv->range_error_occured = FALSE;
}

static void
on_security_picker_changed (HildonPickerButton *widget, gpointer user_data)
{
	ModestConnectionSpecificSmtpEditWindow *self = 
		MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (user_data);
	ModestConnectionSpecificSmtpEditWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);
	
	on_change(GTK_WIDGET(widget), self);
	
	const gint port_number = 
		modest_serversecurity_picker_get_active_serversecurity_port (
			MODEST_SERVERSECURITY_PICKER (priv->outgoing_security_picker));

	if(port_number != 0) {
		hildon_number_editor_set_value (
			HILDON_NUMBER_EDITOR (priv->entry_port), port_number);
	}		
}

static void
modest_connection_specific_smtp_edit_window_init (ModestConnectionSpecificSmtpEditWindow *self)
{
	ModestConnectionSpecificSmtpEditWindowPrivate *priv; 
	GtkWidget *dialog_box;
	GtkWidget *scrolled_window, *vbox;

	/* The title of this dialog is quite long, so make the window wide enough */
	gtk_widget_set_size_request (GTK_WIDGET (self), 600, -1);

	priv = CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);
	dialog_box = GTK_DIALOG(self)->vbox; /* gtk_vbox_new (FALSE, MODEST_MARGIN_HALF); */
	gtk_box_set_spacing (GTK_BOX (dialog_box), MODEST_MARGIN_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_box), MODEST_MARGIN_HALF);

	vbox = gtk_vbox_new (FALSE, 0);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The outgoing server widgets: */
	if (!priv->entry_outgoingserver)
		priv->entry_outgoingserver = gtk_entry_new ();
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_outgoingserver), HILDON_GTK_INPUT_MODE_FULL);
	g_signal_connect(G_OBJECT(priv->entry_outgoingserver), "changed", G_CALLBACK(on_change), self);
	
	GtkWidget *caption = hildon_caption_new (sizegroup, 
		_("mcen_li_emailsetup_smtp"), priv->entry_outgoingserver, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (priv->entry_outgoingserver);
	gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* The secure authentication widgets: */
	if (!priv->outgoing_auth_picker) {
		priv->outgoing_auth_picker = 
			GTK_WIDGET (modest_secureauth_picker_new (MODEST_EDITABLE_SIZE,
								  MODEST_EDITABLE_ARRANGEMENT));
		hildon_button_set_title (HILDON_BUTTON (priv->outgoing_auth_picker), _("mcen_li_emailsetup_secure_authentication"));
	}
	g_signal_connect (G_OBJECT (priv->outgoing_auth_picker), "value-changed", G_CALLBACK(on_change), self);
	gtk_widget_show (priv->outgoing_auth_picker);
	gtk_box_pack_start (GTK_BOX (vbox), priv->outgoing_auth_picker, FALSE, FALSE, MODEST_MARGIN_HALF);
	
	/* The username widgets: */	
	priv->entry_user_username = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_user_username), HILDON_GTK_INPUT_MODE_FULL);
	caption = hildon_caption_new (sizegroup, _("mail_fi_username"), 
		priv->entry_user_username, NULL, HILDON_CAPTION_MANDATORY);
	g_signal_connect(G_OBJECT(priv->entry_user_username), "changed", G_CALLBACK(on_change), self);
	gtk_widget_show (priv->entry_user_username);
	gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	modest_validating_entry_set_unallowed_characters_whitespace (
	 	MODEST_VALIDATING_ENTRY (priv->entry_user_username));
	
	/* Set max length as in the UI spec:
	 * TODO: The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_username), 64);
	
	/* The password widgets: */	
	priv->entry_user_password = gtk_entry_new ();
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_user_password), 
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);
	gtk_entry_set_visibility (GTK_ENTRY (priv->entry_user_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (priv->entry_user_password), '*'); */
	caption = hildon_caption_new (sizegroup, 
		_("mail_fi_password"), priv->entry_user_password, NULL, HILDON_CAPTION_OPTIONAL);
	g_signal_connect(G_OBJECT(priv->entry_user_password), "changed", G_CALLBACK(on_change), self);
	gtk_widget_show (priv->entry_user_password);
	gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* The secure connection widgets: */	
	if (!priv->outgoing_security_picker)
		priv->outgoing_security_picker = 
			GTK_WIDGET (modest_serversecurity_picker_new (MODEST_EDITABLE_SIZE,
								      MODEST_EDITABLE_ARRANGEMENT));
	modest_serversecurity_picker_fill (
		MODEST_SERVERSECURITY_PICKER (priv->outgoing_security_picker), MODEST_PROTOCOLS_TRANSPORT_SMTP);
	modest_serversecurity_picker_set_active_serversecurity (
		MODEST_SERVERSECURITY_PICKER (priv->outgoing_security_picker), MODEST_PROTOCOLS_CONNECTION_NONE);
	hildon_button_set_title (HILDON_BUTTON (priv->outgoing_security_picker), _("mcen_li_emailsetup_secure_connection"));
	gtk_widget_show (priv->outgoing_security_picker);
	gtk_box_pack_start (GTK_BOX (vbox), priv->outgoing_security_picker, FALSE, FALSE, MODEST_MARGIN_HALF);
	
	/* The port number widgets: */
	if (!priv->entry_port)
		priv->entry_port = GTK_WIDGET (hildon_number_editor_new (PORT_RANGE_MIN, PORT_RANGE_MAX));
	caption = hildon_caption_new (sizegroup, 
		_("mcen_fi_emailsetup_port"), priv->entry_port, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_add_events(GTK_WIDGET(priv->entry_port), GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(priv->entry_port), "range-error", G_CALLBACK(on_range_error), self);
	g_signal_connect(G_OBJECT(priv->entry_port), "notify::value", G_CALLBACK(on_value_changed), self);
	gtk_widget_show (priv->entry_port);
	gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Show a default port number when the security method changes, as per the UI spec: */
	g_signal_connect (G_OBJECT (priv->outgoing_security_picker), "value-changed", (GCallback)on_security_picker_changed, self);
	on_security_picker_changed (HILDON_PICKER_BUTTON (priv->outgoing_security_picker), self);
	
	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), _("mcen_bd_dialog_ok"), GTK_RESPONSE_OK);
	
	priv->is_dirty = FALSE;
	priv->range_error_occured = FALSE;
	g_signal_connect(G_OBJECT(self), "response", G_CALLBACK(on_response), self);
	g_signal_connect(G_OBJECT(vbox), "set-focus-child", G_CALLBACK(on_set_focus_child), self);

	priv->range_error_banner_timeout = 0;
	priv->account_name = NULL;

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (dialog_box), scrolled_window, TRUE, TRUE, 0);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (vbox), 
					     gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window)));
	
	gtk_widget_show_all (dialog_box);
	gtk_window_set_default_size (GTK_WINDOW (self), -1, 220);
	
	
	/* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
	modest_window_mgr_prevent_hibernation_while_window_is_shown (
		modest_runtime_get_window_mgr (), GTK_WINDOW (self)); 
	
	hildon_help_dialog_help_enable (GTK_DIALOG(self),
					"applications_email_connectionspecificsmtpconf",
					modest_maemo_utils_get_osso_context());
}

ModestConnectionSpecificSmtpEditWindow*
modest_connection_specific_smtp_edit_window_new (void)
{
	return g_object_new (MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW, NULL);
}

void
modest_connection_specific_smtp_edit_window_set_connection (
	ModestConnectionSpecificSmtpEditWindow *window, const gchar* iap_id, const gchar* iap_name,
	ModestServerAccountSettings *server_settings)
{
	ModestConnectionSpecificSmtpEditWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (window);

	/* This causes a warning because of the %s in the translation, but not in the original string: */
	gchar* title = g_strdup_printf (_("mcen_ti_connection_connection_name"), iap_name);
	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);

	if (server_settings) 
	{
		
		if (priv->account_name)
			g_free (priv->account_name);
		priv->account_name = g_strdup (modest_server_account_settings_get_account_name (server_settings));
		gtk_entry_set_text (GTK_ENTRY (priv->entry_outgoingserver), 
				    modest_server_account_settings_get_hostname (server_settings));
		gtk_entry_set_text (GTK_ENTRY (priv->entry_user_username),
				    modest_server_account_settings_get_username (server_settings));	
		gtk_entry_set_text (GTK_ENTRY (priv->entry_user_password), 
				    modest_server_account_settings_get_password (server_settings));
	
		modest_serversecurity_picker_set_active_serversecurity (
		MODEST_SERVERSECURITY_PICKER (priv->outgoing_security_picker), 
		modest_server_account_settings_get_security_protocol (server_settings));
	
		modest_secureauth_picker_set_active_secureauth (
		MODEST_SECUREAUTH_PICKER (priv->outgoing_auth_picker), 
		modest_server_account_settings_get_auth_protocol (server_settings));
		
		/* port: */
		hildon_number_editor_set_value (
			HILDON_NUMBER_EDITOR (priv->entry_port), 
			modest_server_account_settings_get_port (server_settings));
		
		
		/* This will cause changed signals so we set dirty back to FALSE */
		priv->is_dirty = FALSE;
		if (priv->range_error_banner_timeout > 0) {
			g_source_remove (priv->range_error_banner_timeout);
			priv->range_error_banner_timeout = 0;
		}
	
	}
}

ModestServerAccountSettings*
modest_connection_specific_smtp_edit_window_get_settings (ModestConnectionSpecificSmtpEditWindow *window)
{
	ModestConnectionSpecificSmtpEditWindowPrivate *priv = NULL;
	ModestServerAccountSettings *server_settings = NULL;
	const gchar *outgoing_server = NULL;

	priv = 	CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (window);
	outgoing_server = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoingserver));

	/* If the outgoing server is NULL, we are removing the connection specific
	 * settings */
	if ((outgoing_server == NULL) || (outgoing_server[0] == '\0')) {
		return NULL;
	}
	
	server_settings = modest_server_account_settings_new ();
	
	modest_server_account_settings_set_hostname (server_settings, 
						     gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoingserver)));
	modest_server_account_settings_set_protocol (server_settings,
						     MODEST_PROTOCOLS_TRANSPORT_SMTP);
	modest_server_account_settings_set_username (server_settings,
						     gtk_entry_get_text (GTK_ENTRY (priv->entry_user_username)));
	modest_server_account_settings_set_password (server_settings,
						     gtk_entry_get_text (GTK_ENTRY (priv->entry_user_password)));
	
	modest_server_account_settings_set_security_protocol (server_settings, 
						     modest_serversecurity_picker_get_active_serversecurity (
						     MODEST_SERVERSECURITY_PICKER (priv->outgoing_security_picker)));
	modest_server_account_settings_set_auth_protocol (server_settings,
							  modest_secureauth_picker_get_active_secureauth (
							  MODEST_SECUREAUTH_PICKER (priv->outgoing_auth_picker)));
	modest_server_account_settings_set_account_name (server_settings,
							 priv->account_name);
	
	/* port: */
	modest_server_account_settings_set_port (server_settings,
						 hildon_number_editor_get_value (HILDON_NUMBER_EDITOR (priv->entry_port)));
			
	return server_settings;
}

gboolean 
modest_connection_specific_smtp_edit_window_is_dirty(ModestConnectionSpecificSmtpEditWindow *window)
{
	ModestConnectionSpecificSmtpEditWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (window);
	
	return priv->is_dirty;
}

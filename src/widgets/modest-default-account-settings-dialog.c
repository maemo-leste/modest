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


#include <glib/gi18n.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkstock.h>
#include "modest-default-account-settings-dialog.h"
#include "modest-account-mgr.h"
#include "widgets/modest-validating-entry.h"
#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-account-mgr-helpers.h" /* For modest_account_mgr_get_account_data(). */
#include <modest-server-account-settings.h>
#include "modest-runtime.h" /* For modest_runtime_get_account_mgr(). */
#include "modest-connection-specific-smtp-window.h"
#include "modest-signature-editor-dialog.h"
#include <modest-utils.h>
#include <modest-defs.h>
#include "modest-ui-actions.h"
#include "widgets/modest-ui-constants.h"
#include <tny-account.h>
#include <tny-status.h>
#include <modest-scrollable.h>
#include <modest-toolkit-factory.h>
#include <modest-toolkit-utils.h>

#include <gconf/gconf-client.h>
#include <string.h> /* For strlen(). */

#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon.h>
#endif


/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define PORT_MIN 1
#define PORT_MAX 65535

#define RESPONSE_DELETE_DUMMY 1
#define RESPONSE_SIGNATURE_DUMMY 2

static void modest_account_settings_dialog_init (gpointer g, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (ModestDefaultAccountSettingsDialog, 
                        modest_default_account_settings_dialog, 
                        GTK_TYPE_DIALOG,
                        0, 
                        G_IMPLEMENT_INTERFACE (MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, 
                                               modest_account_settings_dialog_init));

#define MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_DEFAULT_ACCOUNT_SETTINGS_DIALOG, ModestDefaultAccountSettingsDialogPrivate))

typedef struct _ModestDefaultAccountSettingsDialogPrivate ModestDefaultAccountSettingsDialogPrivate;

struct _ModestDefaultAccountSettingsDialogPrivate
{
	/* Used by derived widgets to query existing accounts,
	 * and to create new accounts: */
	ModestAccountMgr *account_manager;
	ModestAccountSettings *settings;
	
	gboolean modified;
	gchar * account_name; /* This may not change. It is not user visible. */
	ModestProtocolType incoming_protocol; /* This may not change. */
	ModestProtocolType outgoing_protocol; /* This may not change. */
	gchar * original_account_title;

	ModestProtocolType protocol_authentication_incoming;
	
	GtkWidget *main_container;
	
	GtkWidget *page_account_details;
	GtkWidget *entry_account_title;
	GtkWidget *checkbox_leave_messages;
	
	GtkWidget *page_user_details;
	GtkWidget *entry_user_name;
	GtkWidget *entry_user_username;
	GtkWidget *entry_user_password;
	GtkWidget *entry_user_email;
	GtkWidget *button_signature;
	GtkWidget *button_delete;
	
	GtkWidget *page_complete_easysetup;
	
	GtkWidget *page_incoming;
	GtkWidget *caption_incoming;
	GtkWidget *entry_incomingserver;

	GtkWidget *page_outgoing;
	GtkWidget *entry_outgoingserver;
	GtkWidget *checkbox_outgoing_smtp_specific;
	GtkWidget *button_outgoing_smtp_servers;
	
	GtkWidget *signature_dialog;

	GtkWidget *incoming_security;
	GtkWidget *outgoing_security;
};

static void
enable_buttons (ModestDefaultAccountSettingsDialog *self);

static gboolean
save_configuration (ModestDefaultAccountSettingsDialog *dialog);

static void on_missing_mandatory_data (ModestSecurityOptionsView *security_view,
				       gboolean missing,
				       gpointer user_data);

static const gchar * null_means_empty (const gchar * str);

static const gchar *
null_means_empty (const gchar * str)
{
	return str ? str : "";
}

static void
modest_default_account_settings_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_default_account_settings_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (modest_default_account_settings_dialog_parent_class)->dispose (object);
}

static void
modest_default_account_settings_dialog_finalize (GObject *object)
{
	ModestDefaultAccountSettingsDialog *self;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	self = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (object);
	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	if (priv->account_name)
		g_free (priv->account_name);

	if (priv->original_account_title)
		g_free (priv->original_account_title);

	if (priv->account_manager)
		g_object_unref (G_OBJECT (priv->account_manager));

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	G_OBJECT_CLASS (modest_default_account_settings_dialog_parent_class)->finalize (object);
}

static void
set_modified (ModestDefaultAccountSettingsDialog *self, gboolean modified)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	priv->modified = modified;
}

#ifdef MODEST_TOOLKIT_HILDON2
static void
on_modified_picker_changed (GtkWidget *widget, gpointer user_data)
{
	set_modified (MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data), TRUE);
}
#endif

static void
on_modified_entry_changed (GtkEditable *editable, gpointer user_data)
{
	set_modified (MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data), TRUE);
}

static void
on_modified_checkbutton_toggled (GtkWidget *button, gpointer user_data)
{
	set_modified (MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data), TRUE);
}

static void
on_modified_number_entry_changed (GtkWidget *number_entry, gint new_value, gpointer user_data)
{
	set_modified (MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data), TRUE);
}

static void       
on_number_entry_notify (GtkWidget *entry, GParamSpec *arg1, gpointer user_data)
{
	ModestDefaultAccountSettingsDialog *dialog = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data);
	gint value = modest_number_entry_get_value (entry);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, value > 0);
}

/* Set a modified boolean whenever the widget is changed, 
 * so we can check for it later.
 */
static void
connect_for_modified (ModestDefaultAccountSettingsDialog *self, GtkWidget *widget)
{
	if (modest_is_number_entry (widget)) {
		g_signal_connect (G_OBJECT (widget), "notify::value",
			G_CALLBACK (on_modified_number_entry_changed), self);
		g_signal_connect (G_OBJECT (widget), "notify", G_CALLBACK (on_number_entry_notify), self);
	}
	else if (GTK_IS_ENTRY (widget)) {
		g_signal_connect (G_OBJECT (widget), "changed",
			G_CALLBACK (on_modified_entry_changed), self);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (HILDON_IS_PICKER_BUTTON (widget)) {
		g_signal_connect (G_OBJECT (widget), "value-changed",
				  G_CALLBACK (on_modified_picker_changed), self);
#endif
	} else if (modest_is_togglable (widget)) {
		g_signal_connect (G_OBJECT (widget), "toggled",
			G_CALLBACK (on_modified_checkbutton_toggled), self);
	}
}

static void
on_caption_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ModestDefaultAccountSettingsDialog *self = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data);
	g_assert(self);
	enable_buttons(self);
}

static GtkWidget* 
create_captioned (ModestDefaultAccountSettingsDialog *self,
		  GtkSizeGroup *title_sizegroup,
		  GtkSizeGroup *value_sizegroup,
		  const gchar *label_text,
		  gboolean use_markup,
		  GtkWidget *control)
{

	GtkWidget *result;

	result = modest_toolkit_utils_create_captioned_with_size_type (title_sizegroup, value_sizegroup,
								       label_text, use_markup, control,
								       MODEST_EDITABLE_SIZE);

	/* Connect to the appropriate changed signal for the widget, 
	 * so we can ask for the prev/next buttons to be enabled/disabled appropriately:
	 */
	if (GTK_IS_ENTRY (control)) {
		g_signal_connect (G_OBJECT (control), "changed",
        	G_CALLBACK (on_caption_entry_changed), self);
		
	}
	 
	return result;
}

static void
on_entry_invalid_account_title_character (ModestValidatingEntry *self, const gchar* character, gpointer user_data)
{
	gchar *tmp, *msg;
			
	tmp = g_strndup (account_title_forbidden_chars, ACCOUNT_TITLE_FORBIDDEN_CHARS_LENGTH);
	msg = g_strdup_printf (_CS_ILLEGAL_CHARACTERS_ENTERED, tmp);

	modest_platform_information_banner (GTK_WIDGET (self), NULL, msg);

	g_free (msg);
	g_free (tmp);
}

static void
on_entry_invalid_fullname_character (ModestValidatingEntry *self, const gchar* character, gpointer user_data)
{
	gchar *tmp, *msg;
			
	tmp = g_strndup (user_name_forbidden_chars, USER_NAME_FORBIDDEN_CHARS_LENGTH);
	msg = g_strdup_printf (_CS_ILLEGAL_CHARACTERS_ENTERED, tmp);

	modest_platform_information_banner (GTK_WIDGET (self), NULL, msg);

	g_free (msg);
	g_free (tmp);
}


static void
on_entry_max (ModestValidatingEntry *self, gpointer user_data)
{
	modest_platform_information_banner (GTK_WIDGET (self), NULL, 
					    _CS_MAXIMUM_CHARACTERS_REACHED);
}

static GtkWidget*
create_page_account_details (ModestDefaultAccountSettingsDialog *self, 
			     GtkSizeGroup *title_sizegroup,
			     GtkSizeGroup *value_sizegroup)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;
	GtkWidget *box;
	GtkWidget *hbox;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
           
	/* The description widgets: */	
	priv->entry_account_title = GTK_WIDGET (modest_validating_entry_new ());
	/* Do use auto-capitalization: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_account_title), 
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_AUTOCAP);
#endif
	GtkWidget *caption = create_captioned (self, title_sizegroup, value_sizegroup,
					       _("mcen_fi_account_title"), FALSE,
					       priv->entry_account_title);
	gtk_widget_show (priv->entry_account_title);
	connect_for_modified (self, priv->entry_account_title);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 0);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the account title, 
	 * as required by our UI specification: */
	GList *list_prevent = NULL;
	list_prevent = g_list_append (list_prevent, "\\");
	list_prevent = g_list_append (list_prevent, "/");
	list_prevent = g_list_append (list_prevent, ":");
	list_prevent = g_list_append (list_prevent, "*");
	list_prevent = g_list_append (list_prevent, "?");
	list_prevent = g_list_append (list_prevent, "\"");
	list_prevent = g_list_append (list_prevent, "<"); 
	list_prevent = g_list_append (list_prevent, ">"); 
	list_prevent = g_list_append (list_prevent, "|");
	list_prevent = g_list_append (list_prevent, "^"); 	
	modest_validating_entry_set_unallowed_characters (
	 	MODEST_VALIDATING_ENTRY (priv->entry_account_title), list_prevent);
	g_list_free (list_prevent);
	modest_validating_entry_set_func(MODEST_VALIDATING_ENTRY(priv->entry_account_title),
					 on_entry_invalid_account_title_character, self);

	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_account_title), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_account_title), 
		on_entry_max, self);

	hbox = gtk_hbox_new (TRUE, 0);

	/* The leave-messages widgets: */
	if(!priv->checkbox_leave_messages) {
		priv->checkbox_leave_messages = 
			modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
								    _("mcen_fi_advsetup_leave_on_server"));
	}
	connect_for_modified (self, priv->checkbox_leave_messages);
	gtk_box_pack_start (GTK_BOX (box), priv->checkbox_leave_messages, FALSE, FALSE, 0);
	gtk_widget_show (priv->checkbox_leave_messages);

	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static gchar*
get_entered_account_title (ModestDefaultAccountSettingsDialog *dialog)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;
	const gchar* account_title;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);
	account_title = gtk_entry_get_text (GTK_ENTRY (priv->entry_account_title));

	if (!account_title || (strlen (account_title) == 0))
		return NULL;
	else {
		/* Strip it of whitespace at the start and end: */
		gchar *result = g_strdup (account_title);
		result = g_strstrip (result);
		
		if (!result)
			return NULL;
			
		if (strlen (result) == 0) {
			g_free (result);
			return NULL;	
		}
		
		return result;
	}
}


static void
signature_button_clicked (ModestDefaultAccountSettingsDialog *self)
{
	gint response;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Create the window, if necessary: */
	if (!(priv->signature_dialog)) {
		priv->signature_dialog = GTK_WIDGET (modest_signature_editor_dialog_new ());

		gboolean use_signature = modest_account_settings_get_use_signature (priv->settings);
		const gchar *signature = modest_account_settings_get_signature(priv->settings);
		gchar* account_title = get_entered_account_title (self);
		modest_signature_editor_dialog_set_settings (
			MODEST_SIGNATURE_EDITOR_DIALOG (priv->signature_dialog), 
			use_signature, signature, account_title);

		g_free (account_title);
		account_title = NULL;
		signature = NULL;
	}

	/* Show the window: */	
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (priv->signature_dialog), GTK_WINDOW (self));

	response = gtk_dialog_run (GTK_DIALOG (priv->signature_dialog));
	gtk_widget_hide (priv->signature_dialog);
	if (response != GTK_RESPONSE_OK) {
		/* Destroy the widget now, and its data: */
		gtk_widget_destroy (priv->signature_dialog);
		priv->signature_dialog = NULL;
	} else {
		/* Mark modified, so we use the dialog's data later: */
		priv->modified = TRUE;	
	}
}

static gboolean
delete_button_clicked (ModestDefaultAccountSettingsDialog *self)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;
	gchar *account_title;
	gboolean removed;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	if (priv->modified)
		account_title = g_strdup (priv->original_account_title);
	else
		account_title = get_entered_account_title (self);

	removed = modest_ui_actions_on_delete_account (GTK_WINDOW (self),
						       priv->account_name, 
						       (const gchar *) account_title);
	g_free (account_title);

	return removed;
}

static GtkWidget*
create_page_user_details (ModestDefaultAccountSettingsDialog *self,
			  GtkSizeGroup *title_sizegroup,
			  GtkSizeGroup *value_sizegroup)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;
	GtkWidget *box;
	gchar *tmp;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
 
	/* The name widgets: */
	priv->entry_user_name = GTK_WIDGET (modest_validating_entry_new ());

	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_user_name),
					 HILDON_GTK_INPUT_MODE_FULL |
					 HILDON_GTK_INPUT_MODE_AUTOCAP);
#endif
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_name), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_user_name), 
		on_entry_max, self);
	GtkWidget *caption = 
		create_captioned (self, title_sizegroup, value_sizegroup,
				  _("mcen_li_emailsetup_name"), FALSE, priv->entry_user_name);
	gtk_widget_show (priv->entry_user_name);
	connect_for_modified (self, priv->entry_user_name);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 0);
	gtk_widget_show (caption);


	/* Prevent the use of some characters in the name, 
	 * as required by our UI specification: */
	GList *list_prevent = NULL;
	list_prevent = g_list_append (list_prevent, "<");
	list_prevent = g_list_append (list_prevent, ">");
	modest_validating_entry_set_unallowed_characters (
	 	MODEST_VALIDATING_ENTRY (priv->entry_user_name), list_prevent);
	g_list_free (list_prevent);
	modest_validating_entry_set_func(MODEST_VALIDATING_ENTRY(priv->entry_user_name),
					 on_entry_invalid_fullname_character, self);
	
	/* The username widgets: */	
	priv->entry_user_username = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_user_username), HILDON_GTK_INPUT_MODE_FULL);
#endif
	tmp = g_strconcat (_("mail_fi_username"), "*", NULL);
	caption = create_captioned (self, title_sizegroup, value_sizegroup,
				    tmp, FALSE,
				    priv->entry_user_username);
	g_free (tmp);
	gtk_widget_show (priv->entry_user_username);
	connect_for_modified (self, priv->entry_user_username);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 0);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	modest_validating_entry_set_unallowed_characters_whitespace (
	 	MODEST_VALIDATING_ENTRY (priv->entry_user_username));
	modest_validating_entry_set_func (MODEST_VALIDATING_ENTRY (priv->entry_user_username), 
					  modest_utils_on_entry_invalid_character, 
					  self);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_username), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_user_username), 
		on_entry_max, self);
	
	/* The password widgets: */	
	priv->entry_user_password = modest_toolkit_factory_create_entry (modest_runtime_get_toolkit_factory ());
	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_user_password), 
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);
#endif
	gtk_entry_set_visibility (GTK_ENTRY (priv->entry_user_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (priv->entry_user_password), '*'); */
	caption = create_captioned (self, title_sizegroup, value_sizegroup,
				    _("mail_fi_password"), FALSE, priv->entry_user_password);
	gtk_widget_show (priv->entry_user_password);
	connect_for_modified (self, priv->entry_user_password);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 0);
	gtk_widget_show (caption);
	
	/* The email address widgets: */	
	priv->entry_user_email = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_user_email), HILDON_GTK_INPUT_MODE_FULL);
#endif
	tmp = g_strconcat (_("mcen_li_emailsetup_email_address"), "*", NULL);
	caption = create_captioned (self, title_sizegroup, value_sizegroup,
				    tmp, FALSE, priv->entry_user_email);
	g_free (tmp);
	gtk_entry_set_text (GTK_ENTRY (priv->entry_user_email), MODEST_EXAMPLE_EMAIL_ADDRESS); /* Default text. */
	gtk_widget_show (priv->entry_user_email);
	connect_for_modified (self, priv->entry_user_email);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 0);
	gtk_widget_show (caption);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_email), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_user_email), 
		on_entry_max, self);
	
	/* Delete button: */
	if (!priv->button_delete)
		priv->button_delete = gtk_dialog_add_button (GTK_DIALOG (self),
							     _HL_DELETE,
							     RESPONSE_DELETE_DUMMY);

	/* Signature button: */
	if (!priv->button_signature)
		priv->button_signature = gtk_dialog_add_button (GTK_DIALOG (self),
								_("mcen_bd_email_signature"),
								RESPONSE_SIGNATURE_DUMMY);
	gtk_widget_show (priv->button_signature);

	gtk_widget_show (GTK_WIDGET (box));

	return GTK_WIDGET (box);
}

/* Change the caption title for the incoming server */
static void
update_incoming_server_title (ModestDefaultAccountSettingsDialog *self,
			      ModestProtocolType protocol_type)
{
	ModestProtocolRegistry *protocol_registry;
	ModestProtocol *protocol;
	const gchar *protocol_display_name;
	gchar* incomingserver_title;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	protocol_registry = modest_runtime_get_protocol_registry ();
	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry, protocol_type);
	protocol_display_name = modest_protocol_get_display_name (protocol);
	incomingserver_title = g_strconcat(_("mcen_li_emailsetup_servertype"), "*", 
					   "\n<small>(", protocol_display_name, ")</small>", NULL);
	
	modest_toolkit_utils_captioned_set_label (priv->caption_incoming, incomingserver_title, TRUE);
	g_free (incomingserver_title);
}

/* The size groups passed as arguments are only used by the security settings */
static GtkWidget*
create_page_incoming (ModestDefaultAccountSettingsDialog *self,
		      GtkSizeGroup *security_title_sizegroup,
		      GtkSizeGroup *security_value_sizegroup)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;
	GtkWidget *box;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	/* The incoming server widgets: */
	if(!priv->entry_incomingserver)
		priv->entry_incomingserver = modest_toolkit_factory_create_entry (modest_runtime_get_toolkit_factory ());
	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_incomingserver), HILDON_GTK_INPUT_MODE_FULL);
#endif

	if (priv->caption_incoming)
		gtk_widget_destroy (priv->caption_incoming);

	/* The caption title will be updated in update_incoming_server_title().
	 * so this default text will never be seen: */
	/* (Note: Changing the title seems pointless. murrayc) */
	priv->caption_incoming = create_captioned (self, security_title_sizegroup, security_value_sizegroup,
						   "Incoming Server", FALSE, priv->entry_incomingserver);
	gtk_widget_show (priv->entry_incomingserver);
	connect_for_modified (self, priv->entry_incomingserver);
	gtk_box_pack_start (GTK_BOX (box), priv->caption_incoming, FALSE, FALSE, 0);
	gtk_widget_show (priv->caption_incoming);

	/* Incoming security widgets */
	priv->incoming_security =
		modest_toolkit_factory_create_security_options_view (modest_runtime_get_toolkit_factory (),
								     MODEST_SECURITY_OPTIONS_INCOMING,
								     TRUE, security_title_sizegroup,
								     security_value_sizegroup);
	gtk_box_pack_start (GTK_BOX (box), priv->incoming_security,
			    FALSE, FALSE, 0);

	gtk_widget_show (priv->incoming_security);
	gtk_widget_show (GTK_WIDGET (box));
	g_signal_connect (priv->incoming_security, "missing-mandatory-data",
			  G_CALLBACK (on_missing_mandatory_data), self);

	return GTK_WIDGET (box);
}

static void
on_check_button_clicked (GtkWidget *button, gpointer user_data)
{
	GtkWidget *widget = GTK_WIDGET (user_data);
	
	/* Enable the widget only if the check button is active: */
	const gboolean enable = modest_togglable_get_active (button);
	gtk_widget_set_sensitive (widget, enable);
}

/* Make the sensitivity of a widget depend on a check button.
 */
static void
enable_widget_for_checkbutton (GtkWidget *widget, GtkWidget* button)
{
	g_signal_connect (G_OBJECT (button), "clicked",
		G_CALLBACK (on_check_button_clicked), widget);

	/* Set the starting sensitivity: */
	on_check_button_clicked (button, widget);
}

static void
on_button_outgoing_smtp_servers (GtkButton *button, gpointer user_data)
{
	ModestDefaultAccountSettingsDialog * self = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestConnectionSpecificSmtpWindow *smtp_win;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Create the window if necessary: */
	smtp_win = modest_connection_specific_smtp_window_new ();
	modest_connection_specific_smtp_window_fill_with_connections (smtp_win, priv->account_manager);

	/* Show the window: */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), GTK_WINDOW (smtp_win), GTK_WINDOW (self));
	gtk_widget_show (GTK_WIDGET (smtp_win));
}

static void
on_missing_mandatory_data (ModestSecurityOptionsView *security_view,
			   gboolean missing,
			   gpointer user_data)
{
	/* Disable the OK button */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (user_data),
					   GTK_RESPONSE_OK,
					   !missing);
}

static GtkWidget*
create_page_outgoing (ModestDefaultAccountSettingsDialog *self,
		      GtkSizeGroup *security_title_sizegroup,
		      GtkSizeGroup *security_value_sizegroup)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;
	gchar *smtp_caption_label;
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
 
	/* The outgoing server widgets: */
	if (!priv->entry_outgoingserver)
		priv->entry_outgoingserver = modest_toolkit_factory_create_entry (modest_runtime_get_toolkit_factory ());
	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->entry_outgoingserver), HILDON_GTK_INPUT_MODE_FULL);
#endif
	smtp_caption_label = g_strconcat (_("mcen_li_emailsetup_smtp"), "*\n<small>(SMTP)</small>", NULL);
	GtkWidget *caption = create_captioned (self, security_title_sizegroup, security_value_sizegroup,
					       smtp_caption_label, TRUE, priv->entry_outgoingserver);
	g_free (smtp_caption_label);
	gtk_widget_show (priv->entry_outgoingserver);
	connect_for_modified (self, priv->entry_outgoingserver);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 0);
	gtk_widget_show (caption);

	/* Outgoing security widgets */
	priv->outgoing_security = 
		modest_toolkit_factory_create_security_options_view (modest_runtime_get_toolkit_factory (),
								     MODEST_SECURITY_OPTIONS_OUTGOING,
								     TRUE, security_title_sizegroup,
								     security_value_sizegroup);
	gtk_box_pack_start (GTK_BOX (box), priv->outgoing_security, 
			    FALSE, FALSE, 0);
	gtk_widget_show (priv->outgoing_security);
	g_signal_connect (priv->outgoing_security, "missing-mandatory-data",
			  G_CALLBACK (on_missing_mandatory_data), self);

	GtkWidget *separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), separator, FALSE, FALSE, MODEST_MARGIN_DEFAULT);
#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (separator, HILDON_SIZE_AUTO);
#endif
	gtk_widget_show (separator);

	/* connection-specific checkbox: */
	if (!priv->checkbox_outgoing_smtp_specific) {
		priv->checkbox_outgoing_smtp_specific = 
			modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
								    _("mcen_fi_advsetup_connection_smtp"));
		modest_togglable_set_active (priv->checkbox_outgoing_smtp_specific,
					     FALSE);
	}
	gtk_widget_show (priv->checkbox_outgoing_smtp_specific);
	gtk_box_pack_start (GTK_BOX (box), priv->checkbox_outgoing_smtp_specific, 
			    FALSE, FALSE, 0);
	connect_for_modified (self, priv->checkbox_outgoing_smtp_specific);

	/* Connection-specific SMTP-Severs Edit button: */
	if (!priv->button_outgoing_smtp_servers)
		priv->button_outgoing_smtp_servers = gtk_button_new_with_label (_("mcen_bd_advsetup_optional_smtp"));
#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (priv->button_outgoing_smtp_servers, 
					  HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);	
#endif
	gtk_widget_show (priv->button_outgoing_smtp_servers);
	gtk_box_pack_start (GTK_BOX (box), priv->button_outgoing_smtp_servers, FALSE, FALSE, 0);

	/* Only enable the button when the checkbox is checked: */
	enable_widget_for_checkbutton (priv->button_outgoing_smtp_servers, 
		priv->checkbox_outgoing_smtp_specific);

	g_signal_connect (G_OBJECT (priv->button_outgoing_smtp_servers), "clicked",
        	G_CALLBACK (on_button_outgoing_smtp_servers), self);

	gtk_widget_show (GTK_WIDGET (box));

	return GTK_WIDGET (box);
}

static gboolean
check_data (ModestDefaultAccountSettingsDialog *self)
{
	gchar* account_title;
	const gchar* email_address; 
	const gchar* hostname;
	const gchar* hostname2;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Check that the title is not already in use: */
	account_title = get_entered_account_title (self);
	if (!account_title)
		return FALSE; /* Should be prevented already anyway. */

	if (g_strcmp0 (account_title, priv->original_account_title) != 0) {
		gboolean name_in_use; 

		/* Check the changed title: */
		name_in_use = modest_account_mgr_account_with_display_name_exists (priv->account_manager,
										   account_title);

		if (name_in_use) {
			/* Warn the user via a dialog: */
			modest_platform_information_banner(NULL, NULL, _("mail_ib_account_name_already_existing"));

			g_free (account_title);
			return FALSE;
		}
	}

	g_free (account_title);
	account_title  = NULL;

	/* Check that the email address is valid: */
	email_address = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_email));
	if ((!email_address) || (strlen(email_address) == 0)) {
		return FALSE;
	}

	if (!modest_text_utils_validate_email_address (email_address, NULL)) {
		/* Warn the user via a dialog: */
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_invalid_email"));
                                         
	        /* Return focus to the email address entry: */
        	gtk_widget_grab_focus (priv->entry_user_email);
		gtk_editable_select_region (GTK_EDITABLE (priv->entry_user_email), 0, -1);
		return FALSE;
	}

	/* make sure the domain name for the incoming server is valid */
	hostname = gtk_entry_get_text (GTK_ENTRY (priv->entry_incomingserver));
	if ((!hostname) || (strlen(hostname) == 0)) {
		return FALSE;
	}
	
	if (!modest_text_utils_validate_domain_name (hostname)) {
		/* Warn the user via a dialog: */
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_invalid_servername"));
                                         
		/* Return focus to the email address entry: */
		gtk_widget_grab_focus (priv->entry_incomingserver);
		gtk_editable_select_region (GTK_EDITABLE (priv->entry_incomingserver), 0, -1);
		return FALSE;
	}

	/* make sure the domain name for the outgoing server is valid */
	hostname2 = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoingserver));
	if ((!hostname2) || (strlen(hostname2) == 0)) {
		return FALSE;
	}
	
	if (!modest_text_utils_validate_domain_name (hostname2)) {
		/* Warn the user via a dialog: */
		modest_platform_information_banner (priv->entry_outgoingserver, NULL, _("mcen_ib_invalid_servername"));

		/* Return focus to the email address entry: */
		gtk_widget_grab_focus (priv->entry_outgoingserver);
		gtk_editable_select_region (GTK_EDITABLE (priv->entry_outgoingserver), 0, -1);
		return FALSE;
	}

	return TRUE;
}

static gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
	ModestDefaultAccountSettingsDialog *self = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;
	ModestSecurityOptionsView *incoming_sec, *outgoing_sec;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Check if security widgets changed */
	incoming_sec = MODEST_SECURITY_OPTIONS_VIEW (priv->incoming_security);
	outgoing_sec = MODEST_SECURITY_OPTIONS_VIEW (priv->outgoing_security);

	return modest_security_options_view_changed (incoming_sec, priv->settings) ||
		modest_security_options_view_changed (outgoing_sec, priv->settings) ||
		priv->modified;
}

static void
on_response (GtkDialog *wizard_dialog,
	     gint response_id,
	     gpointer user_data)
{
	ModestDefaultAccountSettingsDialog *self = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (wizard_dialog);
	ModestDefaultAccountSettingsDialogPrivate *priv;
	gboolean prevent_response = FALSE, sec_changed;
	ModestSecurityOptionsView *incoming_sec, *outgoing_sec;

	/* Dummy and delete buttons have a response id because they're
	   added with gtk_dialog_add_button in order to get the proper
	   theme */
	if (response_id == RESPONSE_SIGNATURE_DUMMY) {
		signature_button_clicked (self);
		g_signal_stop_emission_by_name (wizard_dialog, "response");
		return;
	}

	if (response_id == RESPONSE_DELETE_DUMMY) {
		if (!delete_button_clicked (self))
			g_signal_stop_emission_by_name (wizard_dialog, "response");
		return;
	}

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	enable_buttons (self);

	/* Check if security widgets changed */
	incoming_sec = MODEST_SECURITY_OPTIONS_VIEW (priv->incoming_security);
	outgoing_sec = MODEST_SECURITY_OPTIONS_VIEW (priv->outgoing_security);
	sec_changed =
		modest_security_options_view_changed (incoming_sec, priv->settings) ||
		modest_security_options_view_changed (outgoing_sec, priv->settings);

	/* Warn about unsaved changes: */
	if ((response_id == GTK_RESPONSE_CANCEL || response_id == GTK_RESPONSE_DELETE_EVENT) && 
	    (priv->modified || sec_changed)) {
		gint dialog_response = modest_platform_run_confirmation_dialog (GTK_WINDOW (self), 
										_("imum_nc_wizard_confirm_lose_changes"));
		/* TODO: These button names will be ambiguous, and not specified in the UI specification. */


		if (dialog_response != GTK_RESPONSE_OK)
			prevent_response = TRUE;
	}
	/* Check for invalid input: */
	else if (response_id != GTK_RESPONSE_CANCEL && !check_data (self)) {
		prevent_response = TRUE;
	}

	if (prevent_response) {
		/* Don't let the dialog close */
		g_signal_stop_emission_by_name (wizard_dialog, "response");
		return;
	} else {
		modest_tny_account_store_set_send_mail_blocked (modest_runtime_get_account_store (), FALSE);
	}

	if (response_id == GTK_RESPONSE_OK) {
		/* Try to save the changes if modified (NB #59251): */
		if (priv->modified || sec_changed) {
			if (save_configuration (self)) {
				/* Do not show the account-saved dialog if we are just saving this 
				 * temporarily, because from the user's point of view it will not 
				 * really be saved (saved + enabled) until later
				 */
				if (modest_account_settings_get_account_name (priv->settings) != NULL) {
					ModestServerAccountSettings *store_settings;
					ModestServerAccountSettings *transport_settings;
					const gchar *store_account_name;
					const gchar *transport_account_name;


					store_settings = modest_account_settings_get_store_settings (priv->settings);
					transport_settings = modest_account_settings_get_transport_settings (priv->settings);
					store_account_name = modest_server_account_settings_get_account_name (store_settings);
					transport_account_name = modest_server_account_settings_get_account_name (transport_settings);

					if (store_account_name) {
						modest_account_mgr_notify_account_update (priv->account_manager, 
											  store_account_name);
					}
					if (transport_account_name) {
						modest_account_mgr_notify_account_update (priv->account_manager, 
											  transport_account_name);
					}
					g_object_unref (store_settings);
					g_object_unref (transport_settings);

					modest_platform_information_banner(NULL, NULL, _("mcen_ib_advsetup_settings_saved"));
				}
			} else {
				modest_platform_information_banner (NULL, NULL, _("mail_ib_setting_failed"));
			}
		}
	}
}

static void
modest_default_account_settings_dialog_init (ModestDefaultAccountSettingsDialog *self)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;
	GtkWidget *scrollable;
	GtkWidget *separator;
	GtkWidget *align;
	GtkSizeGroup* account_title_sizegroup;
	GtkSizeGroup* account_value_sizegroup;
	GtkSizeGroup *sec_title_sizegroup, *sec_value_sizegroup;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE(self);

#ifndef MODEST_TOOLKIT_HILDON2
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
#endif

	priv->incoming_security = NULL;
	priv->outgoing_security = NULL;

	priv->main_container = gtk_vbox_new (FALSE, 0);
	priv->settings = modest_account_settings_new ();

	/* Get the account manager object, 
	 * so we can check for existing accounts,
	 * and create new accounts: */
	priv->account_manager = modest_runtime_get_account_mgr ();
	g_assert (priv->account_manager);
	g_object_ref (priv->account_manager);

	priv->protocol_authentication_incoming = MODEST_PROTOCOLS_AUTH_PASSWORD;

	/* Create the common pages */
	account_title_sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	account_value_sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	priv->page_account_details = create_page_account_details (self, account_title_sizegroup, account_value_sizegroup);
	priv->page_user_details = create_page_user_details (self, account_title_sizegroup, account_value_sizegroup);
	g_object_unref (account_title_sizegroup);
	g_object_unref (account_value_sizegroup);
	

	/* Create size groups for security settings */
	sec_title_sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	sec_value_sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* Create incoming and outgoing "pages" */
	priv->page_incoming = create_page_incoming (self, sec_title_sizegroup, sec_value_sizegroup);
	priv->page_outgoing = create_page_outgoing (self, sec_title_sizegroup, sec_value_sizegroup);
	g_object_unref (sec_title_sizegroup);
	g_object_unref (sec_value_sizegroup);

	/* Add the notebook pages: */
	gtk_box_pack_start (GTK_BOX (priv->main_container),
			    priv->page_account_details,
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->main_container),
			    priv->page_user_details,
			    FALSE, FALSE, 0);

	separator = gtk_hseparator_new ();
#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (separator, HILDON_SIZE_AUTO);
#endif
	gtk_box_pack_start (GTK_BOX (priv->main_container), separator,
			    FALSE, FALSE, MODEST_MARGIN_DEFAULT);
	gtk_widget_show (separator);
	gtk_box_pack_start (GTK_BOX (priv->main_container),
			    priv->page_incoming,
			    FALSE, FALSE, 0);
	separator = gtk_hseparator_new ();
#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (separator, HILDON_SIZE_AUTO);
#endif
	gtk_box_pack_start (GTK_BOX (priv->main_container), separator,
			    FALSE, FALSE, MODEST_MARGIN_DEFAULT);
	gtk_widget_show (separator);
	gtk_box_pack_start (GTK_BOX (priv->main_container),
			    priv->page_outgoing,
			    FALSE, FALSE, 0);
		
	GtkDialog *dialog = GTK_DIALOG (self);
	scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());

	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DOUBLE, 0);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), priv->main_container);
	
	modest_scrollable_add_with_viewport (MODEST_SCROLLABLE (scrollable), align);
	gtk_container_add (GTK_CONTAINER (dialog->vbox), GTK_WIDGET (scrollable));
	gtk_widget_show (GTK_WIDGET (priv->main_container));
	gtk_widget_show (GTK_WIDGET (scrollable));
        
    /* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG(self), _HL_SAVE, GTK_RESPONSE_OK);

    gtk_window_set_default_size (GTK_WINDOW (self), -1, MODEST_DIALOG_WINDOW_MAX_HEIGHT);

    /* Connect to the dialog's "response" and "delete-event" signals */
    g_signal_connect (G_OBJECT (self), "response", G_CALLBACK (on_response), self); 
    g_signal_connect (G_OBJECT (self), "delete-event", G_CALLBACK (on_delete_event), self); 

    priv->modified = FALSE;

    /* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
    modest_window_mgr_prevent_hibernation_while_window_is_shown (
    	modest_runtime_get_window_mgr (), GTK_WINDOW (self)); 

    /* Prevent sending mails while editing an account, to avoid hangs on unprotected locks
     * while sending messages causes an error dialog and we have a lock */
    modest_tny_account_store_set_send_mail_blocked (modest_runtime_get_account_store (), TRUE);

}

ModestAccountSettingsDialog*
modest_default_account_settings_dialog_new (void)
{
	return g_object_new (MODEST_TYPE_DEFAULT_ACCOUNT_SETTINGS_DIALOG, NULL);
}

/** Update the UI with the stored account details, so they can be edited.
 * @account_name: Name of the account, which should contain incoming and outgoing server accounts.
 */
static void 
modest_default_account_settings_dialog_load_settings (ModestAccountSettingsDialog *dialog, 
						      ModestAccountSettings *settings)
{
	ModestServerAccountSettings *incoming_account;
	ModestServerAccountSettings *outgoing_account;
	ModestProtocolRegistry *protocol_registry;
	const gchar *account_name, *server_account_name;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS_DIALOG (dialog));
	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);
	protocol_registry = modest_runtime_get_protocol_registry ();

	account_name = modest_account_settings_get_account_name (settings);

	/* Save the account name so we can refer to it later: */
	if (priv->account_name)
		g_free (priv->account_name);
	priv->account_name = g_strdup (account_name);

	if (priv->account_name)
		gtk_widget_show (priv->button_delete);
	else
		gtk_widget_hide (priv->button_delete);

	if (priv->settings)
		g_object_unref (priv->settings);
	priv->settings = g_object_ref (settings);

	/* Save the account title so we can refer to it if the user changes it: */
	if (priv->original_account_title)
		g_free (priv->original_account_title);
	priv->original_account_title = g_strdup (modest_account_settings_get_display_name (settings));

	/* Show the account data in the widgets: */

	/* Note that we never show the non-display name in the UI.
	 * (Though the display name defaults to the non-display name at the start.) */
	gtk_entry_set_text( GTK_ENTRY (priv->entry_account_title),
			    null_means_empty (modest_account_settings_get_display_name (settings)));
	gtk_entry_set_text( GTK_ENTRY (priv->entry_user_name), 
			    null_means_empty (modest_account_settings_get_fullname (settings)));
	gtk_entry_set_text( GTK_ENTRY (priv->entry_user_email), 
			    null_means_empty (modest_account_settings_get_email_address (settings)));

	modest_togglable_set_active (priv->checkbox_leave_messages,
				     modest_account_settings_get_leave_messages_on_server (settings));

	incoming_account = modest_account_settings_get_store_settings (settings);
	if (incoming_account) {
		const gchar *username, *password, *hostname, *proto_str, *account_title;
		gchar *proto_name, *title;
		ModestProtocolType incoming_protocol;

		if (!modest_protocol_registry_protocol_type_has_leave_on_server (protocol_registry,
										 modest_server_account_settings_get_protocol (incoming_account))) {
			gtk_widget_hide (priv->checkbox_leave_messages);
		} else {
			gtk_widget_show (priv->checkbox_leave_messages);
		}

		/* Remember this for later: */
		incoming_protocol = modest_server_account_settings_get_protocol (incoming_account);;

		hostname = modest_server_account_settings_get_hostname (incoming_account);
		username = modest_server_account_settings_get_username (incoming_account);
		password = modest_server_account_settings_get_password (incoming_account);
		gtk_entry_set_text( GTK_ENTRY (priv->entry_user_username),
				    null_means_empty (username));
		gtk_entry_set_text( GTK_ENTRY (priv->entry_user_password), 
				    null_means_empty (password));

		gtk_entry_set_text( GTK_ENTRY (priv->entry_incomingserver), 
				    null_means_empty (hostname));

		/* Load security settings */
		modest_security_options_view_load_settings (
			    MODEST_SECURITY_OPTIONS_VIEW (priv->incoming_security), 
			    settings);
		gtk_widget_show (priv->incoming_security);

		/* Update the incoming label */
		update_incoming_server_title (MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG (dialog), 
					      incoming_protocol);

		/* Set window title according to account */
		proto_str = modest_protocol_get_display_name (modest_protocol_registry_get_protocol_by_type (protocol_registry, incoming_protocol));
		proto_name = g_utf8_strup (proto_str, -1);
		account_title = modest_account_settings_get_display_name(settings);
		title = g_strdup_printf(_("mcen_ti_account_settings"), proto_name, account_title);

		gtk_window_set_title (GTK_WINDOW (dialog), title);

		g_free (proto_name);
		g_free (title);
		g_object_unref (incoming_account);
	}

	outgoing_account = modest_account_settings_get_transport_settings (settings);
	if (outgoing_account) {
		const gchar *hostname;
		const gchar *username;
		const gchar *password;
		ModestProtocolType outgoing_protocol;

		/* Remember this for later: */
		outgoing_protocol = 
			modest_server_account_settings_get_protocol (outgoing_account);

		hostname = modest_server_account_settings_get_hostname (outgoing_account);
		username = modest_server_account_settings_get_username (outgoing_account);
		password = modest_server_account_settings_get_password (outgoing_account);
		gtk_entry_set_text( GTK_ENTRY (priv->entry_outgoingserver), 
				    null_means_empty (hostname));

		/* Load security settings */
		modest_security_options_view_load_settings (
			    MODEST_SECURITY_OPTIONS_VIEW (priv->outgoing_security), 
			    settings);
		gtk_widget_show (priv->outgoing_security);

		const gboolean has_specific = 
			modest_account_settings_get_use_connection_specific_smtp (settings);
		modest_togglable_set_active (priv->checkbox_outgoing_smtp_specific,
					     has_specific);
		g_object_unref (outgoing_account);
	}

	/* Switch to user page */
	/* Check if we allow changes or not */
	server_account_name = modest_server_account_settings_get_account_name (incoming_account);
	if (server_account_name) {
		gboolean username_known;

		username_known =
			modest_account_mgr_get_server_account_username_has_succeeded (priv->account_manager,
										      server_account_name);
		gtk_widget_set_sensitive (priv->entry_user_username, !username_known);
		gtk_widget_set_sensitive (priv->entry_incomingserver, !username_known);
		modest_security_options_view_enable_changes (MODEST_SECURITY_OPTIONS_VIEW (priv->incoming_security),
							     !username_known);
	}

	/* Unset the modified flag so we can detect changes later: */
	priv->modified = FALSE;
}

static gboolean
save_configuration (ModestDefaultAccountSettingsDialog *dialog)
{
	const gchar* user_fullname;
	const gchar* emailaddress;
	ModestServerAccountSettings *store_settings;
	ModestServerAccountSettings *transport_settings;
	gboolean leave_on_server;
	const gchar* hostname;
	const gchar* username;
	const gchar* password;
	gchar* account_title;
	ModestDefaultAccountSettingsDialogPrivate *priv;
	const gchar* account_name;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);
	account_name = priv->account_name;

	/* Set the account data from the widgets: */
	user_fullname = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_name));
	modest_account_settings_set_fullname (priv->settings, user_fullname);
	
	emailaddress = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_email));
	modest_account_settings_set_email_address (priv->settings, emailaddress);
		
	/* Signature: */
	if (priv->signature_dialog) {
		gboolean use_signature = FALSE;
		gchar *signature;
		signature = modest_signature_editor_dialog_get_settings (MODEST_SIGNATURE_EDITOR_DIALOG (priv->signature_dialog),
									 &use_signature);
    	
		modest_account_settings_set_use_signature (priv->settings, use_signature);
		modest_account_settings_set_signature (priv->settings, signature);
	}

	leave_on_server = modest_togglable_get_active (priv->checkbox_leave_messages);
	modest_account_settings_set_leave_messages_on_server (priv->settings, leave_on_server); 

	store_settings = modest_account_settings_get_store_settings (priv->settings);
			
	hostname = gtk_entry_get_text (GTK_ENTRY (priv->entry_incomingserver));
	modest_server_account_settings_set_hostname (store_settings, hostname);
				
	username = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_username));
	modest_server_account_settings_set_username (store_settings, username);
	
	password = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_password));
	modest_server_account_settings_set_password (store_settings, password);

	/* Save security settings */
	modest_security_options_view_save_settings (MODEST_SECURITY_OPTIONS_VIEW (priv->incoming_security), 
						    priv->settings);

	g_object_unref (store_settings);
	
	/* Outgoing: */
	transport_settings = modest_account_settings_get_transport_settings (priv->settings);
	
	hostname = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoingserver));
	modest_server_account_settings_set_hostname (transport_settings, hostname);
			
	/* Save security settings */
	modest_security_options_view_save_settings (
	    MODEST_SECURITY_OPTIONS_VIEW (priv->outgoing_security), 
	    priv->settings);
	
	/* Set the changed account title last, to simplify the previous code: */
	account_title = get_entered_account_title (dialog);
	if (!account_title)
		return FALSE; /* Should be prevented already anyway. */
		
/* 	if (strcmp (account_title, account_name) != 0) { */
	modest_account_settings_set_display_name (priv->settings, account_title);
/* 	} */
	g_free (account_title);
	account_title = NULL;
	
	/* Save connection-specific SMTP server accounts: */
	modest_account_settings_set_use_connection_specific_smtp 
		(priv->settings, 
		 modest_togglable_get_active(priv->checkbox_outgoing_smtp_specific));

	/* this configuration is not persistent, we should not save */
	if (account_name != NULL)
		modest_account_mgr_save_account_settings (priv->account_manager, priv->settings);

	return TRUE;
}

static gboolean entry_is_empty (GtkWidget *entry)
{
	if (!entry)
		return FALSE;
		
	const gchar* text = gtk_entry_get_text (GTK_ENTRY (entry));
	if ((!text) || (strlen(text) == 0))
		return TRUE;
	else {
		/* Strip it of whitespace at the start and end: */
		gchar *stripped = g_strdup (text);
		stripped = g_strstrip (stripped);
		
		if (!stripped)
			return TRUE;
			
		const gboolean result = (strlen (stripped) == 0);
		
		g_free (stripped);
		return result;
	}
}

static void
enable_buttons (ModestDefaultAccountSettingsDialog *self)
{
	gboolean enable_ok = TRUE;
	ModestProtocolRegistry *protocol_registry;
	ModestSecurityOptionsView *sec_view;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* The account details title is mandatory: */
	if (entry_is_empty(priv->entry_account_title))
		enable_ok = FALSE;

	/* The user details username is mandatory: */
	if (enable_ok && entry_is_empty(priv->entry_user_username))
		enable_ok = FALSE;

	/* The user details email address is mandatory: */
	if (enable_ok && entry_is_empty (priv->entry_user_email))
		enable_ok = FALSE;

	/* The custom incoming server is mandatory: */
	if (enable_ok && entry_is_empty(priv->entry_incomingserver))
		enable_ok = FALSE;

	sec_view = MODEST_SECURITY_OPTIONS_VIEW (priv->incoming_security);
	if (enable_ok &&
	    modest_security_options_view_has_missing_mandatory_data (sec_view))
		enable_ok = FALSE;

	/* The custom outgoing server is mandatory: */
	if (enable_ok && entry_is_empty(priv->entry_outgoingserver))
		enable_ok = FALSE;

	sec_view = MODEST_SECURITY_OPTIONS_VIEW (priv->outgoing_security);
	if (enable_ok &&
	    modest_security_options_view_has_missing_mandatory_data (sec_view))
		enable_ok = FALSE;

	protocol_registry = modest_runtime_get_protocol_registry ();

	/* Enable the buttons, 
	 * identifying them via their associated response codes:
	 */
	GtkDialog *dialog_base = GTK_DIALOG (self);
	gtk_dialog_set_response_sensitive (dialog_base,
					   GTK_RESPONSE_OK,
					   enable_ok);
}

static void
modest_default_account_settings_dialog_class_init (ModestDefaultAccountSettingsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (ModestDefaultAccountSettingsDialogPrivate));

	object_class->dispose = modest_default_account_settings_dialog_dispose;
	object_class->finalize = modest_default_account_settings_dialog_finalize;
}

static void 
modest_account_settings_dialog_init (gpointer g_iface, gpointer iface_data)
{
	ModestAccountSettingsDialogClass *iface = (ModestAccountSettingsDialogClass *) g_iface;

	iface->load_settings = modest_default_account_settings_dialog_load_settings;
}

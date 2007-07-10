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
 

#include "modest-easysetup-wizard.h"
#include <glib/gi18n.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkseparator.h>
#include "maemo/easysetup/modest-easysetup-country-combo-box.h"
#include "maemo/easysetup/modest-easysetup-provider-combo-box.h"
#include "maemo/easysetup/modest-easysetup-servertype-combo-box.h"
#include "widgets/modest-serversecurity-combo-box.h"
#include "widgets/modest-secureauth-combo-box.h"
#include "widgets/modest-validating-entry.h"
#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-account-mgr-helpers.h"
#include "modest-runtime.h" /* For modest_runtime_get_account_mgr(). */
#include "maemo/modest-connection-specific-smtp-window.h"
#include "widgets/modest-ui-constants.h"
#include "maemo/modest-account-settings-dialog.h"
#include "maemo/modest-maemo-utils.h"
#include <gconf/gconf-client.h>
#include <string.h> /* For strlen(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define EXAMPLE_EMAIL_ADDRESS "first.last@provider.com"

G_DEFINE_TYPE (ModestEasysetupWizardDialog, modest_easysetup_wizard_dialog, MODEST_TYPE_WIZARD_DIALOG);

#define WIZARD_DIALOG_GET_PRIVATE(o)					\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialogPrivate))

typedef struct _ModestEasysetupWizardDialogPrivate ModestEasysetupWizardDialogPrivate;

typedef enum {
	MODEST_EASYSETUP_WIZARD_DIALOG_INCOMING_CHANGED = 0x01,
	MODEST_EASYSETUP_WIZARD_DIALOG_OUTGOING_CHANGED = 0x02
} ModestEasysetupWizardDialogServerChanges;

struct _ModestEasysetupWizardDialogPrivate
{
	ModestPresets *presets;

	/* Remember what fields the user edited manually to not prefill them
	 * again. */
	ModestEasysetupWizardDialogServerChanges server_changes;
	
	/* Check if the user changes a field to show a confirmation dialog */
	gboolean dirty;
};

static void
on_easysetup_changed(GtkWidget* widget, ModestEasysetupWizardDialog* wizard)
{
	ModestEasysetupWizardDialogPrivate* priv = WIZARD_DIALOG_GET_PRIVATE(wizard);
	g_return_if_fail (priv != NULL);
	priv->dirty = TRUE;
}

static void
modest_easysetup_wizard_dialog_get_property (GObject *object, guint property_id,
					     GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_easysetup_wizard_dialog_set_property (GObject *object, guint property_id,
					     const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_easysetup_wizard_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_easysetup_wizard_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (modest_easysetup_wizard_dialog_parent_class)->dispose (object);
}

static void
modest_easysetup_wizard_dialog_finalize (GObject *object)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (object);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	
	if (self->account_manager)
		g_object_unref (G_OBJECT (self->account_manager));
		
	if (priv->presets)
		modest_presets_destroy (priv->presets);
		
	if (self->specific_window)
	 	gtk_widget_destroy (self->specific_window);
	 	
	g_free (self->saved_account_name);
	
	G_OBJECT_CLASS (modest_easysetup_wizard_dialog_parent_class)->finalize (object);
}

static void
show_error (GtkWidget *parent_widget, const gchar* text);

static gboolean
create_account (ModestEasysetupWizardDialog *self, gboolean enabled);

static void
create_subsequent_easysetup_pages (ModestEasysetupWizardDialog *self);

static void
set_default_custom_servernames(ModestEasysetupWizardDialog *dialog);

static void on_combo_servertype_changed(GtkComboBox *combobox, gpointer user_data);

static gint get_serverport_incoming(ModestPresetsServerType servertype_incoming,
                                    ModestPresetsSecurity security_incoming)
{
	int serverport_incoming = 0;
		/* We don't check for SMTP here as that is impossible for an incoming server. */
		if (servertype_incoming == MODEST_PRESETS_SERVER_TYPE_IMAP) {
			serverport_incoming =
				(security_incoming & MODEST_PRESETS_SECURITY_SECURE_INCOMING_ALTERNATE_PORT) ? 993 : 143; 
		} else if (servertype_incoming == MODEST_PRESETS_SERVER_TYPE_POP) {
			serverport_incoming =
				(security_incoming & MODEST_PRESETS_SECURITY_SECURE_INCOMING_ALTERNATE_PORT) ? 995 : 110; 
		}
	return serverport_incoming;
}

static GList* check_for_supported_auth_methods(ModestEasysetupWizardDialog* account_wizard)
{
	GError *error = NULL;
	const ModestTransportStoreProtocol protocol = 
          easysetup_servertype_combo_box_get_active_servertype (
                                                                EASYSETUP_SERVERTYPE_COMBO_BOX (account_wizard->combo_incoming_servertype));
	const gchar* hostname = gtk_entry_get_text(GTK_ENTRY(account_wizard->entry_incomingserver));
	const gchar* username = gtk_entry_get_text(GTK_ENTRY(account_wizard->entry_user_username));
	const ModestConnectionProtocol protocol_security_incoming = 
					modest_serversecurity_combo_box_get_active_serversecurity (
					MODEST_SERVERSECURITY_COMBO_BOX (
					account_wizard->combo_incoming_security));
	int port_num = get_serverport_incoming(protocol, protocol_security_incoming); 
	GList *list_auth_methods =
          modest_maemo_utils_get_supported_secure_authentication_methods (
                                                                      protocol, 
                                                                      hostname, port_num, username, GTK_WINDOW (account_wizard), &error);
	if (list_auth_methods) {
		/* TODO: Select the correct method */
		GList* list = NULL;
		GList* method;
		for (method = list_auth_methods; method != NULL; method = g_list_next(method)) {
			ModestAuthProtocol auth = (ModestAuthProtocol) (GPOINTER_TO_INT(method->data));
			if (modest_protocol_info_auth_is_secure(auth)) {
				list = g_list_append(list, GINT_TO_POINTER(auth));
			}
		}
		g_list_free(list_auth_methods);
		if (list)
			return list;
	}

	if(error == NULL || error->domain != modest_maemo_utils_get_supported_secure_authentication_error_quark() ||
			error->code != MODEST_MAEMO_UTILS_GET_SUPPORTED_SECURE_AUTHENTICATION_ERROR_CANCELED)
	{
		show_error (GTK_WIDGET(account_wizard), _("Could not discover supported secure authentication methods."));
	}

	if(error != NULL) g_error_free(error);
	return NULL;
}

static void
invoke_enable_buttons_vfunc (ModestEasysetupWizardDialog *wizard_dialog)
{
	ModestWizardDialogClass *klass = MODEST_WIZARD_DIALOG_GET_CLASS (wizard_dialog);
	
	/* Call the vfunc, which may be overridden by derived classes: */
	if (klass->enable_buttons) {
		GtkNotebook *notebook = NULL;
		g_object_get (wizard_dialog, "wizard-notebook", &notebook, NULL);
		
		const gint current_page_num = gtk_notebook_get_current_page (notebook);
		if (current_page_num == -1)
			return;
			
		GtkWidget* current_page_widget = gtk_notebook_get_nth_page (notebook, current_page_num);
		(*(klass->enable_buttons))(MODEST_WIZARD_DIALOG (wizard_dialog), current_page_widget);
	}
}

static void
on_caption_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	g_assert(self);
	invoke_enable_buttons_vfunc(self);
}

static void
on_caption_combobox_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	g_assert(self);
	invoke_enable_buttons_vfunc(self);
}

/** This is a convenience function to create a caption containing a mandatory widget.
 * When the widget is edited, the enable_buttons() vfunc will be called.
 */
static GtkWidget* create_caption_new_with_asterisk(ModestEasysetupWizardDialog *self,
						  GtkSizeGroup *group,
						  const gchar *value,
						  GtkWidget *control,
						  GtkWidget *icon,
						  HildonCaptionStatus flag)
{
	GtkWidget *caption = NULL;
  
	/* Note: Previously, the translated strings already contained the "*",
	 * Comment out this code if they do again.
	 */
	/* Add a * character to indicate mandatory fields,
	 * as specified in our "Email UI Specification": */
	if (flag == HILDON_CAPTION_MANDATORY) {
		gchar* title = g_strdup_printf("%s*", value);
		caption = hildon_caption_new (group, title, control, icon, flag);	
		g_free(title);
	}	
	else
		caption = hildon_caption_new (group, value, control, icon, flag);

	/* Connect to the appropriate changed signal for the widget, 
	 * so we can ask for the prev/next buttons to be enabled/disabled appropriately:
	 */
	if (GTK_IS_ENTRY (control)) {
		g_signal_connect (G_OBJECT (control), "changed",
				  G_CALLBACK (on_caption_entry_changed), self);
		
	}
	else if (GTK_IS_COMBO_BOX (control)) {
		g_signal_connect (G_OBJECT (control), "changed",
				  G_CALLBACK (on_caption_combobox_changed), self);
	}
	 
	return caption;
}
           
static GtkWidget*
create_page_welcome (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *label = gtk_label_new(_("mcen_ia_emailsetup_intro"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	/* So that it is not truncated: */
	gtk_label_set_max_width_chars (GTK_LABEL (label), 40);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	gtk_widget_show (GTK_WIDGET (box));
	return GTK_WIDGET (box);
}

static void
on_combo_account_country (GtkComboBox *widget, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	g_assert(self);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	
	priv->dirty = TRUE;
	
	/* Fill the providers combo, based on the selected country: */
	GSList *list_mcc_ids = easysetup_country_combo_box_get_active_country_ids (
		EASYSETUP_COUNTRY_COMBO_BOX (self->combo_account_country));
	easysetup_provider_combo_box_fill (
		EASYSETUP_PROVIDER_COMBO_BOX (self->combo_account_serviceprovider), priv->presets, list_mcc_ids);
}

static void
on_combo_account_serviceprovider (GtkComboBox *widget, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	g_assert(self);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	
	priv->dirty = TRUE;
	
	/* Fill the providers combo, based on the selected country: */
	gchar* provider_id = easysetup_provider_combo_box_get_active_provider_id (
		EASYSETUP_PROVIDER_COMBO_BOX (self->combo_account_serviceprovider));
	
	gchar* domain_name = NULL;
	if(provider_id)
		domain_name = modest_presets_get_domain (priv->presets, provider_id);
	
	if(!domain_name)
		domain_name = g_strdup (EXAMPLE_EMAIL_ADDRESS);
		
	if (self->entry_user_email)
		gtk_entry_set_text (GTK_ENTRY (self->entry_user_email), domain_name);
		
	g_free (domain_name);
	
	g_free (provider_id);
}

static void
on_entry_max (ModestValidatingEntry *self, gpointer user_data)
{
	/* ModestEasysetupWizardDialog *dialog = MODEST_EASYSETUP_WIZARD_DIALOG (user_data); */
	show_error (GTK_WIDGET (self), _CS("ckdg_ib_maximum_characters_reached"));
}

static void
on_entry_invalid_character (ModestValidatingEntry *self, const gchar* character, gpointer user_data)
{
	/* ModestEasysetupWizardDialog *dialog = MODEST_EASYSETUP_WIZARD_DIALOG (user_data); */
	/* We could add a special case for whitespace here 
	if (character == NULL) ...
	*/
	/* TODO: Should this show just this one bad character or all the not-allowed characters? */
	gchar *message = g_strdup_printf (_CS("ckdg_ib_illegal_characters_entered"), character);
	show_error (GTK_WIDGET (self), message);
}

static GtkWidget*
create_page_account_details (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *label = gtk_label_new(_("mcen_ia_accountdetails"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_max_width_chars (GTK_LABEL (label), 40);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (label);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* The country widgets: */
	self->combo_account_country = GTK_WIDGET (easysetup_country_combo_box_new ());
	GtkWidget *caption = create_caption_new_with_asterisk (self, sizegroup, _("mcen_fi_country"), 
							      self->combo_account_country, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_account_country);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* connect to country combo's changed signal, so we can fill the provider combo: */
	g_signal_connect (G_OBJECT (self->combo_account_country), "changed",
			  G_CALLBACK (on_combo_account_country), self);
            
	GtkWidget *separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), separator, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (separator);
            
	/* The service provider widgets: */	
	self->combo_account_serviceprovider = GTK_WIDGET (easysetup_provider_combo_box_new ());
	
	caption = create_caption_new_with_asterisk (self, sizegroup, _("mcen_fi_serviceprovider"), 
						   self->combo_account_serviceprovider, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_account_serviceprovider);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* connect to providers combo's changed signal, so we can fill the email address: */
	g_signal_connect (G_OBJECT (self->combo_account_serviceprovider), "changed",
			  G_CALLBACK (on_combo_account_serviceprovider), self);
	
	/* TODO: Default to the current country somehow.
	 * But I don't know how to get the information that is specified in the 
	 * "Language and region" control panel. It does not seem be anywhere in gconf. murrayc.
	 *
	 * This is probably not the best choice of gconf key:
	 * This is the  "mcc used in the last pairing", ie. the last connection you made.
	 * set by the osso-operator-wizard package, suggested by Dirk-Jan Binnema.
	 *
	 */
	GConfClient *client = gconf_client_get_default ();
	GError *error = NULL;
	const gchar* key = "/apps/osso/operator-wizard/last_mcc";
	gint mcc_id = gconf_client_get_int(client, key, &error);
	
	if(mcc_id < 0)
		mcc_id = 0;
     
	if (error) {
		g_warning ("Error getting gconf key %s:\n%s", key, error->message);
		g_error_free (error);
		error = NULL;
     	
		mcc_id = 0;
	}
    
	/* Note that gconf_client_get_int() seems to return 0 without an error if the key is not there
	 * This might just be a Maemo bug.
	 */
	if (mcc_id == 0) 
	{
		/* For now, we default to Finland when there is nothing better: */
		mcc_id = 244;
	}
   
	easysetup_country_combo_box_set_active_country_id (
		EASYSETUP_COUNTRY_COMBO_BOX (self->combo_account_country), mcc_id);
		
	
	/* The description widgets: */	
	self->entry_account_title = GTK_WIDGET (modest_validating_entry_new ());
	g_signal_connect(G_OBJECT(self->entry_account_title), "changed",
									 G_CALLBACK(on_easysetup_changed), self);
	/* Do use auto-capitalization: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_account_title), 
					 HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_AUTOCAP);
	
	/* Set a default account title, choosing one that does not already exist: */
	/* Note that this is irrelevant to the non-user visible name, which we will create later. */
	gchar* default_account_name_start = g_strdup (_("mcen_ia_emailsetup_defaultname"));
	gchar* default_account_name = modest_account_mgr_get_unused_account_display_name (
		self->account_manager, default_account_name_start);
	g_free (default_account_name_start);
	default_account_name_start = NULL;
	
	gtk_entry_set_text( GTK_ENTRY (self->entry_account_title), default_account_name);
	g_free (default_account_name);
	default_account_name = NULL;

	caption = create_caption_new_with_asterisk (self, sizegroup, _("mcen_fi_account_title"), 
						   self->entry_account_title, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_account_title);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the account title, 
	 * as required by our UI specification: */
	GList *list_prevent = NULL;
	list_prevent = g_list_append (list_prevent, "\\");
	list_prevent = g_list_append (list_prevent, "/");
	list_prevent = g_list_append (list_prevent, ":");
	list_prevent = g_list_append (list_prevent, "*");
	list_prevent = g_list_append (list_prevent, "?");
	list_prevent = g_list_append (list_prevent, "\""); /* The UI spec mentions “, but maybe means ", maybe both. */
	list_prevent = g_list_append (list_prevent, "“");
	list_prevent = g_list_append (list_prevent, "<"); 
	list_prevent = g_list_append (list_prevent, ">"); 
	list_prevent = g_list_append (list_prevent, "|");
	list_prevent = g_list_append (list_prevent, "^"); 	
	modest_validating_entry_set_unallowed_characters (
	 	MODEST_VALIDATING_ENTRY (self->entry_account_title), list_prevent);
	g_list_free (list_prevent);
	list_prevent = NULL;
	modest_validating_entry_set_func(MODEST_VALIDATING_ENTRY(self->entry_account_title),
																	 on_entry_invalid_character, self);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_account_title), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (self->entry_account_title), 
					      on_entry_max, self);
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static GtkWidget*
create_page_user_details (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The name widgets: */
	self->entry_user_name = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_user_name), HILDON_GTK_INPUT_MODE_FULL);
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_user_name), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (self->entry_user_name), 
					      on_entry_max, self);
	GtkWidget *caption = create_caption_new_with_asterisk (self, sizegroup, 
							      _("mcen_li_emailsetup_name"), self->entry_user_name, NULL, HILDON_CAPTION_OPTIONAL);
	g_signal_connect(G_OBJECT(self->entry_user_name), "changed", 
									 G_CALLBACK(on_easysetup_changed), self);
	gtk_widget_show (self->entry_user_name);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the name, 
	 * as required by our UI specification: */
	GList *list_prevent = NULL;
	list_prevent = g_list_append (list_prevent, "<");
	list_prevent = g_list_append (list_prevent, ">");
	modest_validating_entry_set_unallowed_characters (
	 	MODEST_VALIDATING_ENTRY (self->entry_user_name), list_prevent);
	modest_validating_entry_set_func(MODEST_VALIDATING_ENTRY(self->entry_user_name),
																	 on_entry_invalid_character, self);
	g_list_free (list_prevent);
	
	/* The username widgets: */	
	self->entry_user_username = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_user_username), HILDON_GTK_INPUT_MODE_FULL);
	caption = create_caption_new_with_asterisk (self, sizegroup, _("mail_fi_username"), 
						   self->entry_user_username, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_user_username);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	g_signal_connect(G_OBJECT(self->entry_user_username), "changed", 
									 G_CALLBACK(on_easysetup_changed), self);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	modest_validating_entry_set_unallowed_characters_whitespace (
	 	MODEST_VALIDATING_ENTRY (self->entry_user_username));
	modest_validating_entry_set_func(MODEST_VALIDATING_ENTRY(self->entry_user_username),
																	 on_entry_invalid_character, self);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_user_username), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (self->entry_user_username), 
					      on_entry_max, self);
	
	/* The password widgets: */	
	self->entry_user_password = gtk_entry_new ();
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_user_password), 
					 HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);
	gtk_entry_set_visibility (GTK_ENTRY (self->entry_user_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (self->entry_user_password), '*'); */
	caption = create_caption_new_with_asterisk (self, sizegroup, 
						   _("mail_fi_password"), self->entry_user_password, NULL, HILDON_CAPTION_OPTIONAL);
	g_signal_connect(G_OBJECT(self->entry_user_password), "changed", 
									 G_CALLBACK(on_easysetup_changed), self);
	gtk_widget_show (self->entry_user_password);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* The email address widgets: */	
	self->entry_user_email = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_user_email), HILDON_GTK_INPUT_MODE_FULL);
	caption = create_caption_new_with_asterisk (self, sizegroup, 
						   _("mcen_li_emailsetup_email_address"), self->entry_user_email, NULL, HILDON_CAPTION_MANDATORY);
	gtk_entry_set_text (GTK_ENTRY (self->entry_user_email), EXAMPLE_EMAIL_ADDRESS); /* Default text. */
	gtk_widget_show (self->entry_user_email);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	g_signal_connect(G_OBJECT(self->entry_user_email), "changed", 
									 G_CALLBACK(on_easysetup_changed), self);
	gtk_widget_show (caption);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_user_email), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (self->entry_user_email), 
					      on_entry_max, self);
	
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static GtkWidget* create_page_complete_easysetup (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	
	GtkWidget *label = gtk_label_new(_("mcen_ia_emailsetup_setup_complete"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_max_width_chars (GTK_LABEL (label), 40);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	label = gtk_label_new (_("mcen_ia_easysetup_complete"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_max_width_chars (GTK_LABEL (label), 40);
	
	/* The documentation for gtk_label_set_line_wrap() says that we must 
	 * call gtk_widget_set_size_request() with a hard-coded width, 
	 * though I wonder why gtk_label_set_max_width_chars() isn't enough. */
	gtk_widget_set_size_request (label, 400, -1);
	
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	gtk_widget_show (GTK_WIDGET (box));
	return GTK_WIDGET (box);
}

/** Change the caption title for the incoming server, 
 * as specified in the UI spec:
 */
static void update_incoming_server_title (ModestEasysetupWizardDialog *self)
{
	const ModestTransportStoreProtocol protocol = easysetup_servertype_combo_box_get_active_servertype (
		EASYSETUP_SERVERTYPE_COMBO_BOX (self->combo_incoming_servertype));
	const gchar* type = 
		(protocol == MODEST_PROTOCOL_STORE_POP ? 
		 _("mail_fi_emailtype_pop3") : 
		 _("mail_fi_emailtype_imap") );
			
		
	/* Note that this produces a compiler warning, 
	 * because the compiler does not know that the translated string will have a %s in it.
	 * I do not see a way to avoid the warning while still using these Logical IDs. murrayc. */
	gchar* incomingserver_title = g_strdup_printf(_("mcen_li_emailsetup_servertype"), type);
	g_object_set (G_OBJECT (self->caption_incoming), "label", incomingserver_title, NULL);
	g_free(incomingserver_title);
}

/** Change the caption title for the incoming server, 
 * as specified in the UI spec:
 */
static void update_incoming_server_security_choices (ModestEasysetupWizardDialog *self)
{
	const ModestTransportStoreProtocol protocol = easysetup_servertype_combo_box_get_active_servertype (
		EASYSETUP_SERVERTYPE_COMBO_BOX (self->combo_incoming_servertype));
	
	/* Fill the combo with appropriately titled choices for POP or IMAP. */
	/* The choices are the same, but the titles are different, as in the UI spec. */
	modest_serversecurity_combo_box_fill (
		MODEST_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security), protocol);
}

static void on_combo_servertype_changed(GtkComboBox *combobox, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE(self);
	
	priv->dirty = TRUE;
	
	update_incoming_server_title (self);
	update_incoming_server_security_choices (self);

	set_default_custom_servernames (self);
}

static void on_entry_incoming_servername_changed(GtkEntry *entry, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	priv->dirty = TRUE;
	priv->server_changes |= MODEST_EASYSETUP_WIZARD_DIALOG_INCOMING_CHANGED;
}

static GtkWidget* create_page_custom_incoming (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	/* Show note that account type cannot be changed in future: */
	GtkWidget *label = gtk_label_new (_("mcen_ia_emailsetup_account_type"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_max_width_chars (GTK_LABEL (label), 40);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The incoming server widgets: */
	if (!self->combo_incoming_servertype)
		self->combo_incoming_servertype = GTK_WIDGET (easysetup_servertype_combo_box_new ());
	easysetup_servertype_combo_box_set_active_servertype (
		EASYSETUP_SERVERTYPE_COMBO_BOX (self->combo_incoming_servertype), MODEST_PROTOCOL_STORE_POP);
	GtkWidget *caption = create_caption_new_with_asterisk (self, sizegroup, 
							      _("mcen_li_emailsetup_type"), self->combo_incoming_servertype, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->combo_incoming_servertype);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	if(!self->entry_incomingserver)
	{
		self->entry_incomingserver = gtk_entry_new ();
		g_signal_connect(G_OBJECT(self->entry_incomingserver), "changed",
										 G_CALLBACK(on_easysetup_changed), self);
	}
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_incomingserver), HILDON_GTK_INPUT_MODE_FULL);
	set_default_custom_servernames (self);

	if (self->caption_incoming)
		gtk_widget_destroy (self->caption_incoming);
	   
	/* The caption title will be updated in update_incoming_server_title().
	 * so this default text will never be seen: */
	/* (Note: Changing the title seems pointless. murrayc) */
	self->caption_incoming = create_caption_new_with_asterisk (self, sizegroup, 
								  "Incoming Server", self->entry_incomingserver, NULL, HILDON_CAPTION_MANDATORY);
	update_incoming_server_title (self);
	gtk_widget_show (self->entry_incomingserver);
	gtk_box_pack_start (GTK_BOX (box), self->caption_incoming, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (self->caption_incoming);
	
	/* Change the caption title when the servertype changes, 
	 * as in the UI spec: */
	g_signal_connect (G_OBJECT (self->combo_incoming_servertype), "changed",
			  G_CALLBACK (on_combo_servertype_changed), self);

	/* Remember when the servername was changed manually: */
	g_signal_connect (G_OBJECT (self->entry_incomingserver), "changed",
	                  G_CALLBACK (on_entry_incoming_servername_changed), self);

	/* The secure connection widgets: */	
	if (!self->combo_incoming_security)
		self->combo_incoming_security = GTK_WIDGET (modest_serversecurity_combo_box_new ());
	update_incoming_server_security_choices (self);
	modest_serversecurity_combo_box_set_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security), MODEST_PROTOCOL_CONNECTION_NORMAL);
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_connection"), 
				      self->combo_incoming_security, NULL, HILDON_CAPTION_OPTIONAL);
	g_signal_connect (G_OBJECT (self->combo_incoming_security), "changed",
	                  G_CALLBACK (on_easysetup_changed), self);
	gtk_widget_show (self->combo_incoming_security);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	if(!self->checkbox_incoming_auth)
	{
		self->checkbox_incoming_auth = gtk_check_button_new ();
		g_signal_connect (G_OBJECT (self->checkbox_incoming_auth), "toggled",
	                  G_CALLBACK (on_easysetup_changed), self);
	}
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
				      self->checkbox_incoming_auth, NULL, HILDON_CAPTION_OPTIONAL);
	
	gtk_widget_show (self->checkbox_incoming_auth);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static void
on_toggle_button_changed (GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget *widget = GTK_WIDGET (user_data);
	
	/* Enable the widget only if the toggle button is active: */
	const gboolean enable = gtk_toggle_button_get_active (togglebutton);
	gtk_widget_set_sensitive (widget, enable);
}

/* Make the sensitivity of a widget depend on a toggle button.
 */
static void
enable_widget_for_togglebutton (GtkWidget *widget, GtkToggleButton* button)
{
	g_signal_connect (G_OBJECT (button), "toggled",
			  G_CALLBACK (on_toggle_button_changed), widget);
	
	/* Set the starting sensitivity: */
	on_toggle_button_changed (button, widget);
}

static void
on_button_outgoing_smtp_servers (GtkButton *button, gpointer user_data)
{
	ModestEasysetupWizardDialog * self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	ModestEasysetupWizardDialogPrivate* priv = WIZARD_DIALOG_GET_PRIVATE(self);
	
	/* We set dirty here because setting it depending on the connection specific dialog
	seems overkill */
	priv->dirty = TRUE;
	
	/* Create the window, if necessary: */
	if (!(self->specific_window)) {
		self->specific_window = GTK_WIDGET (modest_connection_specific_smtp_window_new ());
		modest_connection_specific_smtp_window_fill_with_connections (
			MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (self->specific_window), self->account_manager, 
			NULL /* account_name, not known yet. */);
	}

	/* Show the window: */
	gtk_window_set_transient_for (GTK_WINDOW (self->specific_window), GTK_WINDOW (self));
	gtk_widget_show (self->specific_window);
}

static void on_entry_outgoing_servername_changed (GtkEntry *entry, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	priv->server_changes |= MODEST_EASYSETUP_WIZARD_DIALOG_OUTGOING_CHANGED;
}

static GtkWidget* create_page_custom_outgoing (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The outgoing server widgets: */
	if (!self->entry_outgoingserver)
	{
		self->entry_outgoingserver = gtk_entry_new ();
		g_signal_connect (G_OBJECT (self->entry_outgoingserver), "changed",
	                  G_CALLBACK (on_easysetup_changed), self);
	}
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_outgoingserver), HILDON_GTK_INPUT_MODE_FULL);
	GtkWidget *caption = create_caption_new_with_asterisk (self, sizegroup, 
							      _("mcen_li_emailsetup_smtp"), self->entry_outgoingserver, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_outgoingserver);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	set_default_custom_servernames (self);
	
	/* The secure connection widgets: */	
	if (!self->combo_outgoing_security)
	{
		self->combo_outgoing_security = GTK_WIDGET (modest_serversecurity_combo_box_new ());
		g_signal_connect (G_OBJECT (self->combo_outgoing_security), "changed",
	                  G_CALLBACK (on_easysetup_changed), self);
	}
	modest_serversecurity_combo_box_fill (
		MODEST_SERVERSECURITY_COMBO_BOX (self->combo_outgoing_security), MODEST_PROTOCOL_TRANSPORT_SMTP);
	modest_serversecurity_combo_box_set_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (self->combo_outgoing_security), MODEST_PROTOCOL_CONNECTION_NORMAL);
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_connection"), 
				      self->combo_outgoing_security, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_outgoing_security);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* The secure authentication widgets: */
	if (!self->combo_outgoing_auth)
	{
		self->combo_outgoing_auth = GTK_WIDGET (modest_secureauth_combo_box_new ());
				g_signal_connect (G_OBJECT (self->combo_outgoing_auth), "changed",
	                  G_CALLBACK (on_easysetup_changed), self);
	}
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
				      self->combo_outgoing_auth, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_outgoing_auth);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	GtkWidget *separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), separator, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (separator);
	
	/* connection-specific checkbox: */
	if (!self->checkbox_outgoing_smtp_specific) {
		self->checkbox_outgoing_smtp_specific = gtk_check_button_new ();
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->checkbox_outgoing_smtp_specific), 
					      FALSE);
		g_signal_connect (G_OBJECT (self->checkbox_outgoing_smtp_specific), "toggled",
	                  G_CALLBACK (on_easysetup_changed), self);

	}
	caption = hildon_caption_new (sizegroup, _("mcen_fi_advsetup_connection_smtp"), 
				      self->checkbox_outgoing_smtp_specific, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->checkbox_outgoing_smtp_specific);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Connection-specific SMTP-Severs Edit button: */
	if (!self->button_outgoing_smtp_servers)
		self->button_outgoing_smtp_servers = gtk_button_new_with_label (_("mcen_bd_edit"));
	caption = hildon_caption_new (sizegroup, _("mcen_fi_advsetup_optional_smtp"), 
				      self->button_outgoing_smtp_servers, NULL, HILDON_CAPTION_OPTIONAL);
	hildon_caption_set_child_expand (HILDON_CAPTION (caption), FALSE);
	gtk_widget_show (self->button_outgoing_smtp_servers);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Only enable the button when the checkbox is checked: */
	enable_widget_for_togglebutton (self->button_outgoing_smtp_servers, 
					GTK_TOGGLE_BUTTON (self->checkbox_outgoing_smtp_specific));
		
	g_signal_connect (G_OBJECT (self->button_outgoing_smtp_servers), "clicked",
			  G_CALLBACK (on_button_outgoing_smtp_servers), self);

	g_signal_connect (G_OBJECT (self->entry_outgoingserver), "changed",
	                  G_CALLBACK (on_entry_outgoing_servername_changed), self);
	
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static gboolean
show_advanced_edit(gpointer user_data)
{
	ModestEasysetupWizardDialog * self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	
	if (!(self->saved_account_name))
		return FALSE;
	
	/* Show the Account Settings window: */
	ModestAccountSettingsDialog *dialog = modest_account_settings_dialog_new ();
	modest_account_settings_dialog_set_account_name (dialog, self->saved_account_name);
	
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self));
	
	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	return FALSE; /* Do not call this timeout callback again. */
}

static void
on_button_edit_advanced_settings (GtkButton *button, gpointer user_data)
{
	ModestEasysetupWizardDialog * self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	
	/* Save the new account, so we can edit it with ModestAccountSettingsDialog, 
	 * without recoding it to use non-gconf information.
	 * This account will be deleted if Finish is never actually clicked. */

	gboolean saved = TRUE;
	if (!(self->saved_account_name)) {
		saved = create_account (self, FALSE);
	}
		
	if (!saved)
		return;
		
	if (!(self->saved_account_name))
		return;
	
	/* Show the Account Settings window: */
	show_advanced_edit(self);
}
static GtkWidget* create_page_complete_custom (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *label = gtk_label_new(_("mcen_ia_emailsetup_setup_complete"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	label = gtk_label_new (_("mcen_ia_customsetup_complete"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	if (!self->button_edit)
		self->button_edit = gtk_button_new_with_label (_("mcen_bd_edit"));
	GtkWidget *caption = hildon_caption_new (NULL, _("mcen_fi_advanced_settings"), 
						 self->button_edit, NULL, HILDON_CAPTION_OPTIONAL);
	hildon_caption_set_child_expand (HILDON_CAPTION (caption), FALSE);
	gtk_widget_show (self->button_edit);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	g_signal_connect (G_OBJECT (self->button_edit), "clicked", 
			  G_CALLBACK (on_button_edit_advanced_settings), self);
	
	gtk_widget_show (GTK_WIDGET (box));
	return GTK_WIDGET (box);
}


/*
 */
static void 
on_response (ModestWizardDialog *wizard_dialog,
	     gint response_id,
	     gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (wizard_dialog);
	if (response_id == GTK_RESPONSE_CANCEL) {
		/* Remove any temporarily-saved account that will not actually be needed: */
		if (self->saved_account_name) {
			modest_account_mgr_remove_account (self->account_manager,
							   self->saved_account_name, FALSE);
		}
	}

	invoke_enable_buttons_vfunc (self);
}

static void 
on_response_before (ModestWizardDialog *wizard_dialog,
                    gint response_id,
                    gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (wizard_dialog);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE(wizard_dialog);
	if (response_id == GTK_RESPONSE_CANCEL) {
		/* This is mostly copied from
		 * src/maemo/modest-account-settings-dialog.c */
		if (priv->dirty)
		{
			GtkDialog *dialog = GTK_DIALOG (hildon_note_new_confirmation (GTK_WINDOW (self), 
				_("imum_nc_wizard_confirm_lose_changes")));
			/* TODO: These button names will be ambiguous, and not specified in the UI specification. */

			const gint dialog_response = gtk_dialog_run (dialog);
			gtk_widget_destroy (GTK_WIDGET (dialog));

			if (dialog_response != GTK_RESPONSE_OK)
			{
				/* This is a nasty hack. murrayc. */
				/* Don't let the dialog close */
				g_signal_stop_emission_by_name (wizard_dialog, "response");
			}
		}
	}
}

static void
modest_easysetup_wizard_dialog_init (ModestEasysetupWizardDialog *self)
{
	gtk_container_set_border_width (GTK_CONTAINER (self), MODEST_MARGIN_HALF);
	
	/* Create the notebook to be used by the ModestWizardDialog base class:
	 * Each page of the notebook will be a page of the wizard: */
	GtkNotebook *notebook = GTK_NOTEBOOK (gtk_notebook_new());
	
	/* Set the notebook used by the ModestWizardDialog base class: */
	g_object_set (G_OBJECT(self), "wizard-notebook", notebook, NULL);
    
	/* Set the wizard title:
	 * The actual window title will be a combination of this and the page's tab label title. */
	g_object_set (G_OBJECT(self), "wizard-name", _("mcen_ti_emailsetup"), NULL);

	/* Read in the information about known service providers: */
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	
	const gchar* filepath = MODEST_PROVIDERS_DATA_PATH; /* Defined in config.h */
	priv->presets = modest_presets_new (filepath);
	if (!(priv->presets)) {
		g_warning ("Could not locate the official provider data keyfile from %s", filepath);
	}
	
	g_assert(priv->presets);

	/* The server fields did not have been manually changed yet */
	priv->server_changes = 0;

	/* Get the account manager object, 
	 * so we can check for existing accounts,
	 * and create new accounts: */
	self->account_manager = modest_runtime_get_account_mgr ();
	g_assert (self->account_manager);
	g_object_ref (self->account_manager);
	
	/* Create the common pages, 
	 */
	self->page_welcome = create_page_welcome (self);
	self->page_account_details = create_page_account_details (self);
	self->page_user_details = create_page_user_details (self);
	
	/* Add the common pages: */
	gtk_notebook_append_page (notebook, self->page_welcome, 
				  gtk_label_new (_("mcen_ti_emailsetup_welcome")));
	gtk_notebook_append_page (notebook, self->page_account_details, 
				  gtk_label_new (_("mcen_ti_accountdetails")));
	gtk_notebook_append_page (notebook, self->page_user_details, 
				  gtk_label_new (_("mcen_ti_emailsetup_userdetails")));
		
	/* Create and add the easysetup-specific pages,
	 * because we need _some_ final page to enable the Next and Finish buttons: */
	create_subsequent_easysetup_pages (self);

	/* Connect to the dialog's response signal so we can enable/disable buttons 
	 * for the newly-selected page, because the prev/next buttons cause response to be emitted.
	 * Note that we use g_signal_connect_after() instead of g_signal_connect()
	 * so that we can be enable/disable after ModestWizardDialog has done its own 
	 * enabling/disabling of buttons.
	 * 
	 * HOWEVER, this doesn't work because ModestWizardDialog's response signal handler 
	 * does g_signal_stop_emission_by_name(), stopping our signal handler from running.
	 * 
	 * It's not enough to connect to the notebook's switch-page signal, because 
	 * ModestWizardDialog's "response" signal handler enables the buttons itself, 
	 * _after_ switching the page (understandably).
	 * (Note that if we had, if we used g_signal_connect() instead of g_signal_connect_after()
	 * then gtk_notebook_get_current_page() would return an incorrect value.)
	 */
	g_signal_connect_after (G_OBJECT (self), "response",
				G_CALLBACK (on_response), self);

	/* This is to show a confirmation dialog when the user hits cancel */
	g_signal_connect (G_OBJECT (self), "response",
	                  G_CALLBACK (on_response_before), self);

	/* Reset dirty, because there was no user input until now */
	priv->dirty = FALSE;
	
	/* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
	modest_window_mgr_prevent_hibernation_while_window_is_shown (
		modest_runtime_get_window_mgr (), GTK_WINDOW (self)); 
}

ModestEasysetupWizardDialog*
modest_easysetup_wizard_dialog_new (void)
{
	return g_object_new (MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, NULL);
}

static void create_subsequent_customsetup_pages (ModestEasysetupWizardDialog *self)
{
	GtkNotebook *notebook = NULL;
	g_object_get (self, "wizard-notebook", &notebook, NULL);
	g_assert(notebook);
	
	/* Create the custom pages: */
	if(!(self->page_custom_incoming)) {
		self->page_custom_incoming = create_page_custom_incoming (self);
	}
		
	if(!(self->page_custom_outgoing)) {
		self->page_custom_outgoing = create_page_custom_outgoing (self);
	}
	
	if(!(self->page_complete_customsetup)) {
		self->page_complete_customsetup = create_page_complete_custom (self);
	}
	
	if (!gtk_widget_get_parent (GTK_WIDGET (self->page_custom_incoming)))
		gtk_notebook_append_page (notebook, self->page_custom_incoming,
					  gtk_label_new (_("mcen_ti_emailsetup_incomingdetails")));
	
	if (!gtk_widget_get_parent (GTK_WIDGET (self->page_custom_outgoing)))		
		gtk_notebook_append_page (notebook, self->page_custom_outgoing,
					  gtk_label_new (_("mcen_ti_emailsetup_outgoingdetails")));
		
	if (!gtk_widget_get_parent (GTK_WIDGET (self->page_complete_customsetup)))
		gtk_notebook_append_page (notebook, self->page_complete_customsetup,
					  gtk_label_new (_("mcen_ti_emailsetup_complete")));
			
	/* This is unnecessary with GTK+ 2.10: */
	modest_wizard_dialog_force_title_update (MODEST_WIZARD_DIALOG(self));
}
	
static void create_subsequent_easysetup_pages (ModestEasysetupWizardDialog *self)
{
	GtkNotebook *notebook = NULL;
	g_object_get (self, "wizard-notebook", &notebook, NULL);
	g_assert(notebook);
	
	/* Create the easysetup-specific pages: */
	if(!self->page_complete_easysetup)
		self->page_complete_easysetup = create_page_complete_easysetup (self);

	if (!gtk_widget_get_parent (GTK_WIDGET (self->page_complete_easysetup)))
		gtk_notebook_append_page (notebook, self->page_complete_easysetup, 
					  gtk_label_new (_("mcen_ti_emailsetup_complete")));
			
	/* This is unnecessary with GTK+ 2.10: */
	modest_wizard_dialog_force_title_update (MODEST_WIZARD_DIALOG(self));
}
/* After the user details page,
 * the following pages depend on whether "Other" was chosen 
 * in the provider combobox on the account page
 */
static void create_subsequent_pages (ModestEasysetupWizardDialog *self)
{
	if (easysetup_provider_combo_box_get_active_provider_id (
		    EASYSETUP_PROVIDER_COMBO_BOX (self->combo_account_serviceprovider)) == 0) {
		/* "Other..." was selected: */
		
		/* Make sure that the easysetup pages do not exist: */
		if(self->page_complete_easysetup) {
			gtk_widget_destroy (self->page_complete_easysetup);
			self->page_complete_easysetup = NULL;
		}
		
		create_subsequent_customsetup_pages (self);
	}	
	else {
		/* A specific provider was selected: */
		{
			/* Make sure that the custom pages do not exist:
			 * Because they will be used if they exist, when creating the account. */
			if(self->page_custom_incoming) {
				gtk_widget_destroy (self->page_custom_incoming);
				self->page_custom_incoming = NULL;
			}
			
			if(self->page_custom_outgoing) {
				gtk_widget_destroy (self->page_custom_outgoing);
				self->page_custom_outgoing = NULL;
			}
			
			if(self->page_complete_customsetup) {
				gtk_widget_destroy (self->page_complete_customsetup);
				self->page_complete_customsetup = NULL;
			}
		}
		
		/* Create the easysetup pages: */
		create_subsequent_easysetup_pages (self);
	}
}


static gchar*
util_get_default_servername_from_email_address (const gchar* email_address, ModestTransportStoreProtocol servertype)
{
	if (!email_address)
		return NULL;
	
	gchar* at = g_utf8_strchr (email_address, -1, '@');
	if (!at || (g_utf8_strlen (at, -1) < 2))
		return NULL;
		
	gchar* domain = g_utf8_next_char (at);
	if(!domain)
		return NULL;
		
	const gchar* hostname = NULL;
	if (servertype == MODEST_PROTOCOL_STORE_POP)
		hostname = "pop";
	else if (servertype == MODEST_PROTOCOL_STORE_IMAP)
		hostname = "imap";
	else if (servertype == MODEST_PROTOCOL_TRANSPORT_SMTP)
		hostname = "smtp";
	
	if (!hostname)
		return NULL;
		
	return g_strdup_printf ("%s.%s", hostname, domain);
}

static void set_default_custom_servernames (ModestEasysetupWizardDialog *account_wizard)
{
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE(account_wizard);

	if (!account_wizard->entry_incomingserver)
		return;
		
	/* Set a default domain for the server, based on the email address,
	 * if no server name was already specified.
	 */
	if (account_wizard->entry_user_email
	    && ((priv->server_changes & MODEST_EASYSETUP_WIZARD_DIALOG_INCOMING_CHANGED) == 0)) {
		const ModestTransportStoreProtocol protocol = easysetup_servertype_combo_box_get_active_servertype (
			EASYSETUP_SERVERTYPE_COMBO_BOX (account_wizard->combo_incoming_servertype));
		const gchar* email_address = gtk_entry_get_text (GTK_ENTRY(account_wizard->entry_user_email));
		
		gchar* servername = util_get_default_servername_from_email_address (email_address, protocol);

		/* Do not set the INCOMING_CHANGED flag because of this edit */
		g_signal_handlers_block_by_func (G_OBJECT (account_wizard->entry_incomingserver), G_CALLBACK (on_entry_incoming_servername_changed), account_wizard);
		gtk_entry_set_text (GTK_ENTRY (account_wizard->entry_incomingserver), servername);
		g_signal_handlers_unblock_by_func (G_OBJECT (account_wizard->entry_incomingserver), G_CALLBACK (on_entry_incoming_servername_changed), account_wizard);

		g_free (servername);
	}
	
	/* Set a default domain for the server, based on the email address,
	 * if no server name was already specified.
	 */
	if (!account_wizard->entry_outgoingserver)
		return;
		
	if (account_wizard->entry_user_email
	    && ((priv->server_changes & MODEST_EASYSETUP_WIZARD_DIALOG_OUTGOING_CHANGED) == 0)) {
		const gchar* email_address = gtk_entry_get_text (GTK_ENTRY(account_wizard->entry_user_email));
		
		gchar* servername = util_get_default_servername_from_email_address (email_address, MODEST_PROTOCOL_TRANSPORT_SMTP);

		/* Do not set the OUTGOING_CHANGED flag because of this edit */
		g_signal_handlers_block_by_func (G_OBJECT (account_wizard->entry_outgoingserver), G_CALLBACK (on_entry_outgoing_servername_changed), account_wizard);
		gtk_entry_set_text (GTK_ENTRY (account_wizard->entry_outgoingserver), servername);
		g_signal_handlers_unblock_by_func (G_OBJECT (account_wizard->entry_outgoingserver), G_CALLBACK (on_entry_outgoing_servername_changed), account_wizard);

		g_free (servername);
	}
}

static gboolean
on_before_next (ModestWizardDialog *dialog, GtkWidget *current_page, GtkWidget *next_page)
{
	ModestEasysetupWizardDialog *account_wizard = MODEST_EASYSETUP_WIZARD_DIALOG (dialog);
	
	/* Do extra validation that couldn't be done for every key press,
	 * either because it was too slow,
	 * or because it requires interaction:
	 */
	if (current_page == account_wizard->page_account_details) {	
		/* Check that the title is not already in use: */
		const gchar* account_title = gtk_entry_get_text (GTK_ENTRY (account_wizard->entry_account_title));
		if ((!account_title) || (strlen(account_title) == 0))
			return FALSE;
			
		/* Aavoid a clash with an existing display name: */
		const gboolean name_in_use = modest_account_mgr_account_with_display_name_exists (
			account_wizard->account_manager, account_title);

		if (name_in_use) {
			/* Warn the user via a dialog: */
			hildon_banner_show_information(NULL, NULL, _("mail_ib_account_name_already_existing"));
            
			return FALSE;
		}
	}
	else if (current_page == account_wizard->page_user_details) {
		/* Check that the email address is valud: */
		const gchar* email_address = gtk_entry_get_text (GTK_ENTRY (account_wizard->entry_user_email));
		if ((!email_address) || (strlen(email_address) == 0))
			return FALSE;
			
		if (!modest_text_utils_validate_email_address (email_address, NULL)) {
			/* Warn the user via a dialog: */
			hildon_banner_show_information (NULL, NULL, _("mcen_ib_invalid_email"));
                                             
			/* Return focus to the email address entry: */
			gtk_widget_grab_focus (account_wizard->entry_user_email);
			gtk_editable_select_region (GTK_EDITABLE (account_wizard->entry_user_email), 0, -1);

			return FALSE;
		}
		
		/* Make sure that the subsequent pages are appropriate for the provider choice. */
		create_subsequent_pages (account_wizard);
	}
	
	/* TODO: The UI Spec wants us to check that the servernames are valid, 
	 * but does not specify how.
	 */
	  
	if(next_page == account_wizard->page_custom_incoming) {
		set_default_custom_servernames (account_wizard);
	}
	else if (next_page == account_wizard->page_custom_outgoing) {
		set_default_custom_servernames (account_wizard);
    /* Check if the server supports secure authentication */
		const ModestConnectionProtocol security_incoming = 
			modest_serversecurity_combo_box_get_active_serversecurity (
																																 MODEST_SERVERSECURITY_COMBO_BOX (
																																																	account_wizard->combo_incoming_security));
		if (gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (account_wizard->checkbox_incoming_auth))
				&& !modest_protocol_info_is_secure(security_incoming))
		{
				GList* methods = check_for_supported_auth_methods(account_wizard);
				if (!methods)
				{
					g_list_free(methods);
					return FALSE;
				}
				g_list_free(methods);
		}
	}
	
	/* If this is the last page, and this is a click on Finish, 
	 * then attempt to create the dialog.
	 */
	if(!next_page) /* This is NULL when this is a click on Finish. */
	{
		if (account_wizard->saved_account_name) {
			/* Just enable the already-saved account (temporarily created when 
			 * editing advanced settings): */
			modest_account_mgr_set_enabled (account_wizard->account_manager, 
							account_wizard->saved_account_name, TRUE);
		} else {
			create_account (account_wizard, TRUE /* enabled */);
		}
	}
	
	
	return TRUE;
}

static gboolean entry_is_empty (GtkWidget *entry)
{
	if (!entry)
		return FALSE;
		
	const gchar* text = gtk_entry_get_text (GTK_ENTRY (entry));
	if ((!text) || (strlen(text) == 0))
		return TRUE;
	else
		return FALSE;
}

static void
on_enable_buttons (ModestWizardDialog *dialog, GtkWidget *current_page)
{
	ModestEasysetupWizardDialog *account_wizard = MODEST_EASYSETUP_WIZARD_DIALOG (dialog);
	
	gboolean enable_next = TRUE;
	if (current_page == account_wizard->page_welcome) {
		enable_next = TRUE;
	}
	else if (current_page == account_wizard->page_account_details) {
		/* The account details title is mandatory: */
		if (entry_is_empty(account_wizard->entry_account_title))
			enable_next = FALSE;
	}
	else if (current_page == account_wizard->page_user_details) {	
		/* The user details username is mandatory: */
		if (entry_is_empty(account_wizard->entry_user_username))
			enable_next = FALSE;
			
		/* The user details email address is mandatory: */
		if (enable_next && entry_is_empty (account_wizard->entry_user_email))
			enable_next = FALSE;
	}
	else if (current_page == account_wizard->page_custom_incoming) {
		/* The custom incoming server is mandatory: */
		if (entry_is_empty(account_wizard->entry_incomingserver))
			enable_next = FALSE;
	}
			
	/* Enable the buttons, 
	 * identifying them via their associated response codes: */
	                           
	/* Disable the Finish button until we are on the last page,
	 * because HildonWizardDialog enables this for all but the first page: */
	GtkNotebook *notebook = NULL;
	GtkDialog *dialog_base = GTK_DIALOG (dialog);
	g_object_get (dialog_base, "wizard-notebook", &notebook, NULL);
	
	gint current = gtk_notebook_get_current_page (notebook);
	gint last = gtk_notebook_get_n_pages (notebook) - 1;
	const gboolean is_last = (current == last);
    
	if(!is_last) {
		gtk_dialog_set_response_sensitive (dialog_base,
						   MODEST_WIZARD_DIALOG_FINISH,
						   FALSE);
	} else
	{
		/* Disable Next on the last page: */
		enable_next = FALSE;
	}
    	
	gtk_dialog_set_response_sensitive (dialog_base,
					   MODEST_WIZARD_DIALOG_NEXT,
					   enable_next);
}

static void
modest_easysetup_wizard_dialog_class_init (ModestEasysetupWizardDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (ModestEasysetupWizardDialogPrivate));


	object_class->get_property = modest_easysetup_wizard_dialog_get_property;
	object_class->set_property = modest_easysetup_wizard_dialog_set_property;
	object_class->dispose = modest_easysetup_wizard_dialog_dispose;
	object_class->finalize = modest_easysetup_wizard_dialog_finalize;
	
	/* Provide a vfunc implementation so we can decide 
	 * when to enable/disable the prev/next buttons.
	 */
	ModestWizardDialogClass *base_klass = (ModestWizardDialogClass*)(klass);
	base_klass->before_next = on_before_next;
	base_klass->enable_buttons = on_enable_buttons;
}
 
static void
show_error (GtkWidget *parent_widget, const gchar* text)
{
	hildon_banner_show_information(parent_widget, NULL, text);
	
#if 0
	GtkDialog *dialog = GTK_DIALOG (hildon_note_new_information (parent_window, text)); */
	/*
	  GtkDialog *dialog = GTK_DIALOG (gtk_message_dialog_new (parent_window,
	  (GtkDialogFlags)0,
	  GTK_MESSAGE_ERROR,
	  GTK_BUTTONS_OK,
	  text ));
	*/
		 
	gtk_dialog_run (dialog);
	gtk_widget_destroy (GTK_WIDGET (dialog));
#endif
}

/** Attempt to create the account from the information that the user has entered.
 * @result: TRUE if the account was successfully created.
 */
gboolean
create_account (ModestEasysetupWizardDialog *self, gboolean enabled)
{
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	
	const gchar* display_name = gtk_entry_get_text (GTK_ENTRY (self->entry_account_title));

	/* Some checks: */
	if (!display_name)
		return FALSE;
		
	/* We should have checked for this already, 
	 * and changed that name accordingly, 
	 * but let's check again just in case:
	 */
	if (modest_account_mgr_account_with_display_name_exists (self->account_manager, display_name)) 
		return FALSE;

	/* Increment the non-user visible name if necessary, 
	 * based on the display name: */
	gchar *account_name_start = g_strdup_printf ("%sID", display_name);
	gchar* account_name = modest_account_mgr_get_unused_account_name (self->account_manager,
									  account_name_start, FALSE /* not a server account */);
	g_free (account_name_start);
		
	/* username and password (for both incoming and outgoing): */
	const gchar* username = gtk_entry_get_text (GTK_ENTRY (self->entry_user_username));
	const gchar* password = gtk_entry_get_text (GTK_ENTRY (self->entry_user_password));
	/* Incoming server: */
	/* Note: We need something as default for the ModestTransportStoreProtocol* values, 
	 * or modest_account_mgr_add_server_account will fail. */
	gchar* servername_incoming = NULL;
	guint serverport_incoming = 0;
	ModestTransportStoreProtocol protocol_incoming = MODEST_PROTOCOL_STORE_POP;
	ModestConnectionProtocol protocol_security_incoming = MODEST_PROTOCOL_CONNECTION_NORMAL;
	ModestAuthProtocol protocol_authentication_incoming = MODEST_PROTOCOL_AUTH_NONE;

	/* Get details from the specified presets: */
	gchar* provider_id = easysetup_provider_combo_box_get_active_provider_id (
		EASYSETUP_PROVIDER_COMBO_BOX (self->combo_account_serviceprovider));
	if (provider_id) {
		/* Use presets: */
		servername_incoming = modest_presets_get_server (priv->presets, provider_id, 
								 TRUE /* incoming */);
		
		ModestPresetsServerType servertype_incoming = modest_presets_get_info_server_type (priv->presets,
												   provider_id, 
												   TRUE /* incoming */);
		ModestPresetsSecurity security_incoming = modest_presets_get_info_server_security (priv->presets,
												   provider_id, 
												   TRUE /* incoming */);

		g_warning ("security incoming: %x", security_incoming);
			
		/* We don't check for SMTP here as that is impossible for an incoming server. */
		if (servertype_incoming == MODEST_PRESETS_SERVER_TYPE_IMAP) {
			protocol_incoming = MODEST_PROTOCOL_STORE_IMAP;
		} else if (servertype_incoming == MODEST_PRESETS_SERVER_TYPE_POP) {
			protocol_incoming = MODEST_PROTOCOL_STORE_POP; 
		}
		serverport_incoming = get_serverport_incoming(servertype_incoming, security_incoming);
		
		if (security_incoming & MODEST_PRESETS_SECURITY_SECURE_INCOMING)
			protocol_security_incoming = MODEST_PROTOCOL_CONNECTION_SSL; /* TODO: Is this what we want? */
		
		if (security_incoming & MODEST_PRESETS_SECURITY_APOP)
			protocol_authentication_incoming = MODEST_PROTOCOL_AUTH_PASSWORD; /* TODO: Is this what we want? */
	}
	else {
		/* Use custom pages because no preset was specified: */
		servername_incoming = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->entry_incomingserver) ));
		
		protocol_incoming = easysetup_servertype_combo_box_get_active_servertype (
			EASYSETUP_SERVERTYPE_COMBO_BOX (self->combo_incoming_servertype));
		
		protocol_security_incoming = modest_serversecurity_combo_box_get_active_serversecurity (
			MODEST_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security));
		
		/* The UI spec says:
		 * If secure authentication is unchecked, allow sending username and password also as plain text.
		 * If secure authentication is checked, require one of the secure methods during connection: SSL, TLS, CRAM-MD5 etc. 
		 */
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->checkbox_incoming_auth)) &&
				!modest_protocol_info_is_secure(protocol_security_incoming))
		{
				GList* methods = check_for_supported_auth_methods(self);
				if (!methods)
					return FALSE;
				else
				  protocol_authentication_incoming = (ModestAuthProtocol) (GPOINTER_TO_INT(methods->data));
		}
		else
			protocol_authentication_incoming = MODEST_PROTOCOL_AUTH_PASSWORD;
	}
	
	/* First we add the 2 server accounts, and then we add the account that uses them.
	 * If we don't do it in this order then we will experience a crash. */
	 
	/* Add a (incoming) server account, to be used by the account: */
	gchar *store_name_start = g_strconcat (account_name, "_store", NULL);
	gchar *store_name = modest_account_mgr_get_unused_account_name (self->account_manager, 
									store_name_start, TRUE /* server account */);
	g_free (store_name_start);
	gboolean created = modest_account_mgr_add_server_account (self->account_manager,
								  store_name,
								  servername_incoming,
								  serverport_incoming,
								  username, password,
								  protocol_incoming,
								  protocol_security_incoming,
								  protocol_authentication_incoming);		
		
	g_free (servername_incoming);
	
	if (!created) {
		/* TODO: Provide a Logical ID for the text: */
		show_error (GTK_WIDGET (self), _("An error occurred while creating the incoming account."));
		return FALSE;	
	}
	
	/* Outgoing server: */
	gchar* servername_outgoing = NULL;
	ModestTransportStoreProtocol protocol_outgoing = MODEST_PROTOCOL_STORE_POP;
	ModestConnectionProtocol protocol_security_outgoing = MODEST_PROTOCOL_CONNECTION_NORMAL;
	ModestAuthProtocol protocol_authentication_outgoing = MODEST_PROTOCOL_AUTH_NONE;
	guint serverport_outgoing = 0;
	
	if (provider_id) {
		/* Use presets: */
		servername_outgoing = modest_presets_get_server (priv->presets, provider_id, 
								 FALSE /* incoming */);
			
		ModestPresetsServerType servertype_outgoing = modest_presets_get_info_server_type (priv->presets,
												   provider_id, 
												   FALSE /* incoming */);
		
		/* Note: We need something as default, or modest_account_mgr_add_server_account will fail. */
		protocol_outgoing = MODEST_PROTOCOL_TRANSPORT_SENDMAIL; 
		if (servertype_outgoing == MODEST_PRESETS_SERVER_TYPE_SMTP)
			protocol_outgoing = MODEST_PROTOCOL_TRANSPORT_SMTP;
		
		ModestPresetsSecurity security_outgoing = 
			modest_presets_get_info_server_security (priv->presets, provider_id, 
								 FALSE /* incoming */);

		/* TODO: There is no SMTP authentication enum for presets, 
		   so we should probably check what the server supports. */
		protocol_security_outgoing = MODEST_PROTOCOL_CONNECTION_NORMAL;
		if (security_outgoing & MODEST_PRESETS_SECURITY_SECURE_SMTP) {
			/* printf("DEBUG: %s: using secure SMTP\n", __FUNCTION__); */
			protocol_security_outgoing = MODEST_PROTOCOL_CONNECTION_SSL; /* TODO: Is this what we want? */
			serverport_outgoing = 465;
			protocol_authentication_outgoing = MODEST_PROTOCOL_AUTH_PASSWORD;
		} else {
			/* printf("DEBUG: %s: using non-secure SMTP\n", __FUNCTION__); */
			protocol_authentication_outgoing = MODEST_PROTOCOL_AUTH_NONE;
		}
	}
	else {
		/* Use custom pages because no preset was specified: */
		servername_outgoing = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->entry_outgoingserver) ));
		
		protocol_outgoing = MODEST_PROTOCOL_TRANSPORT_SMTP; /* It's always SMTP for outgoing. */

		protocol_security_outgoing = modest_serversecurity_combo_box_get_active_serversecurity (
			MODEST_SERVERSECURITY_COMBO_BOX (self->combo_outgoing_security));
		
		protocol_authentication_outgoing = modest_secureauth_combo_box_get_active_secureauth (
			MODEST_SECUREAUTH_COMBO_BOX (self->combo_outgoing_auth));
	}
	    
	/* Add a (outgoing) server account to be used by the account: */
	gchar *transport_name_start = g_strconcat (account_name, "_transport", NULL);
	gchar *transport_name = modest_account_mgr_get_unused_account_name (self->account_manager, 
									    transport_name_start, TRUE /* server account */);
	g_free (transport_name_start);
	created = modest_account_mgr_add_server_account (self->account_manager,
							 transport_name,
							 servername_outgoing,
							 serverport_outgoing,
							 username, password,
							 protocol_outgoing,
							 protocol_security_outgoing,
							 protocol_authentication_outgoing);
		
	g_free (servername_outgoing);
		
	if (!created) {
		/* TODO: Provide a Logical ID for the text: */
		show_error (GTK_WIDGET (self), _("An error occurred while creating the outgoing account."));
		return FALSE;	
	}
	
	
	/* Create the account, which will contain the two "server accounts": */
 	created = modest_account_mgr_add_account (self->account_manager, account_name, 
						  store_name, /* The name of our POP/IMAP server account. */
						  transport_name, /* The name of our SMTP server account. */
						  enabled);
	g_free (store_name);
	g_free (transport_name);
	
	if (!created) {
		/* TODO: Provide a Logical ID for the text: */
		show_error (GTK_WIDGET (self), _("An error occurred while creating the account."));
		return FALSE;	
	}

	/* Sanity check: */
	/* There must be at least one account now: */
	/* Note, when this fails is is caused by a Maemo gconf bug that has been 
	 * fixed in versions after 3.1. */
	if(!modest_account_mgr_has_accounts (self->account_manager, FALSE))
		g_warning ("modest_account_mgr_account_names() returned NULL after adding an account.");
		
	/* The user name and email address must be set additionally: */
	const gchar* user_name = gtk_entry_get_text (GTK_ENTRY (self->entry_user_name));
	modest_account_mgr_set_string (self->account_manager, account_name,
				       MODEST_ACCOUNT_FULLNAME, user_name, FALSE /* not server account */);

	const gchar* emailaddress = gtk_entry_get_text (GTK_ENTRY (self->entry_user_email));
	modest_account_mgr_set_string (self->account_manager, account_name,
				       MODEST_ACCOUNT_EMAIL, emailaddress, FALSE /* not server account */);

	/* Set the display name: */
	modest_account_mgr_set_string (self->account_manager, account_name,
				       MODEST_ACCOUNT_DISPLAY_NAME, display_name, FALSE /* not server account */);

	/* Set retrieve type */ 
	const gchar *retrieve = MODEST_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY;
	modest_account_mgr_set_string (self->account_manager, account_name,
		MODEST_ACCOUNT_RETRIEVE, retrieve, FALSE /* not server account */);

	/* Save the connection-specific SMTP server accounts. */
	gboolean result = TRUE;
	if (self->specific_window)
		result = modest_connection_specific_smtp_window_save_server_accounts (
			MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (self->specific_window), account_name);
			
	g_free (self->saved_account_name);
	self->saved_account_name = g_strdup (account_name);
	
	g_free (account_name);

	return result;
}

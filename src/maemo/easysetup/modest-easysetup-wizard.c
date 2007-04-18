/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
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
#include <hildon-widgets/hildon-caption.h>
#include "maemo/easysetup/modest-easysetup-country-combo-box.h"
#include "maemo/easysetup/modest-easysetup-provider-combo-box.h"
#include "maemo/easysetup/modest-easysetup-servertype-combo-box.h"
#include "maemo/easysetup/modest-easysetup-serversecurity-combo-box.h"
#include "maemo/easysetup/modest-easysetup-secureauth-combo-box.h"
#include "maemo/easysetup/modest-validating-entry.h"
#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-runtime.h" /* For modest_runtime_get_account_mgr(). */
#include "maemo/modest-connection-specific-smtp-window.h"
#include <gconf/gconf-client.h>
#include <string.h> /* For strlen(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define EXAMPLE_EMAIL_ADDRESS "first.last@provider.com"

G_DEFINE_TYPE (ModestEasysetupWizardDialog, modest_easysetup_wizard_dialog, MODEST_TYPE_WIZARD_DIALOG);

#define WIZARD_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialogPrivate))

typedef struct _ModestEasysetupWizardDialogPrivate ModestEasysetupWizardDialogPrivate;

struct _ModestEasysetupWizardDialogPrivate
{
	ModestPresets *presets;
};

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
	
	G_OBJECT_CLASS (modest_easysetup_wizard_dialog_parent_class)->finalize (object);
}

static void
show_error (GtkWindow *parent_window, const gchar* text);

static gboolean
create_account (ModestEasysetupWizardDialog *self);

static void
create_subsequent_easysetup_pages (ModestEasysetupWizardDialog *self);

static void
set_default_custom_servernames(ModestEasysetupWizardDialog *dialog);

static void on_combo_servertype_changed(GtkComboBox *combobox, gpointer user_data);

static gchar*
util_increment_name (const gchar* text);

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
static GtkWidget* create_caption_new_with_asterix(ModestEasysetupWizardDialog *self,
	GtkSizeGroup *group,
	const gchar *value,
	GtkWidget *control,
	GtkWidget *icon,
	HildonCaptionStatus flag)
{
  GtkWidget *caption = hildon_caption_new (group, value, control, icon, flag);
  
/* The translated strings seem to already contain the *,
 * but this code can be used if that is not true in future.
 */
#if 0
	/* Add a * character to indicate mandatory fields,
	 * as specified in our "Email UI Specification": */
	if (flag == HILDON_CAPTION_MANDATORY) {
		gchar* title = g_strdup_printf("%s*", value);
		caption = hildon_caption_new (group, title, control, icon, flag);	
		g_free(title);
	}	
	else
		caption = hildon_caption_new (group, value, control, icon, flag);
#endif

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
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	GtkWidget *label = gtk_label_new(_("mcen_ia_emailsetup_intro"));
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
	
	/* Fill the providers combo, based on the selected country: */
	gint mcc_id = easysetup_country_combo_box_get_active_country_id (
		EASYSETUP_COUNTRY_COMBO_BOX (self->combo_account_country));
	easysetup_provider_combo_box_fill (
		EASYSETUP_PROVIDER_COMBO_BOX (self->combo_account_serviceprovider), priv->presets, mcc_id);
}

static void
on_combo_account_serviceprovider (GtkComboBox *widget, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	g_assert(self);
	ModestEasysetupWizardDialogPrivate *priv = WIZARD_DIALOG_GET_PRIVATE (self);
	
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


static GtkWidget*
create_page_account_details (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	GtkWidget *label = gtk_label_new(_("mcen_ia_accountdetails"));
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 2);
	gtk_widget_show (label);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	/* The country widgets: */
	self->combo_account_country = GTK_WIDGET (easysetup_country_combo_box_new ());
	GtkWidget *caption = create_caption_new_with_asterix (self, sizegroup, _("mcen_fi_country"), 
		self->combo_account_country, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_account_country);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* connect to country combo's changed signal, so we can fill the provider combo: */
    g_signal_connect (G_OBJECT (self->combo_account_country), "changed",
            G_CALLBACK (on_combo_account_country), self);
            
	
	/* The service provider widgets: */	
	self->combo_account_serviceprovider = GTK_WIDGET (easysetup_provider_combo_box_new ());
	
	caption = create_caption_new_with_asterix (self, sizegroup, _("mcen_fi_serviceprovider"), 
		self->combo_account_serviceprovider, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_account_serviceprovider);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
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
	self->entry_account_title = GTK_WIDGET (easysetup_validating_entry_new ());
	
	/* Set a default account title, choosing one that does not already exist: */
	/* Note that this is irrelevant to the non-user visible name, which we will create later. */
	gchar* default_acount_name = g_strdup (_("mcen_ia_emailsetup_defaultname"));
	while (modest_account_mgr_account_with_display_name_exists (self->account_manager, 
		default_acount_name)) {
			
		gchar * default_account_name2 = util_increment_name (default_acount_name);
		g_free (default_acount_name);
		default_acount_name = default_account_name2;
	}
	
	gtk_entry_set_text( GTK_ENTRY (self->entry_account_title), default_acount_name);
	g_free (default_acount_name);
	default_acount_name = NULL;

	caption = create_caption_new_with_asterix (self, sizegroup, _("mcen_fi_account_title"), 
		self->entry_account_title, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_account_title);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
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
	easysetup_validating_entry_set_unallowed_characters (
	 	EASYSETUP_VALIDATING_ENTRY (self->entry_account_title), list_prevent);
	g_list_free (list_prevent);
	
	/* Set max length as in the UI spec:
	 * TODO: The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_account_title), 64);
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static GtkWidget*
create_page_user_details (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The name widgets: */
	self->entry_user_name = GTK_WIDGET (easysetup_validating_entry_new ());
	/* Set max length as in the UI spec:
	 * TODO: The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_user_name), 64);
	GtkWidget *caption = create_caption_new_with_asterix (self, sizegroup, 
		_("mcen_li_emailsetup_name"), self->entry_user_name, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_user_name);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the name, 
	 * as required by our UI specification: */
	GList *list_prevent = NULL;
	list_prevent = g_list_append (list_prevent, "<");
	list_prevent = g_list_append (list_prevent, ">");
	easysetup_validating_entry_set_unallowed_characters (
	 	EASYSETUP_VALIDATING_ENTRY (self->entry_user_name), list_prevent);
	g_list_free (list_prevent);
	
	/* The username widgets: */	
	self->entry_user_username = GTK_WIDGET (easysetup_validating_entry_new ());
	caption = create_caption_new_with_asterix (self, sizegroup, _("mail_fi_username"), 
		self->entry_user_username, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_user_username);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	easysetup_validating_entry_set_unallowed_characters_whitespace (
	 	EASYSETUP_VALIDATING_ENTRY (self->entry_user_username));
	
	/* Set max length as in the UI spec:
	 * TODO: The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_user_username), 64);
	
	/* The password widgets: */	
	self->entry_user_password = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (self->entry_user_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (self->entry_user_password), '*'); */
	caption = create_caption_new_with_asterix (self, sizegroup, 
		_("mail_fi_password"), self->entry_user_password, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_user_password);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* The email address widgets: */	
	self->entry_user_email = GTK_WIDGET (easysetup_validating_entry_new ());
	caption = create_caption_new_with_asterix (self, sizegroup, 
		_("mcen_li_emailsetup_email_address"), self->entry_user_email, NULL, HILDON_CAPTION_MANDATORY);
	gtk_entry_set_text (GTK_ENTRY (self->entry_user_email), EXAMPLE_EMAIL_ADDRESS); /* Default text. */
	gtk_widget_show (self->entry_user_email);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* Set max length as in the UI spec:
	 * TODO: The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_user_email), 64);
	
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static GtkWidget* create_page_complete_easysetup (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	GtkWidget *label = gtk_label_new(_("mcen_ia_emailsetup_setup_complete"));
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
	ModestProtocol protocol = easysetup_servertype_combo_box_get_active_servertype (
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
	ModestProtocol protocol = easysetup_servertype_combo_box_get_active_servertype (
		EASYSETUP_SERVERTYPE_COMBO_BOX (self->combo_incoming_servertype));
	
	/* Fill the combo with appropriately titled choices for POP or IMAP. */
	/* The choices are the same, but the titles are different, as in the UI spec. */
	easysetup_serversecurity_combo_box_fill (
		EASYSETUP_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security), protocol);
}

static void on_combo_servertype_changed(GtkComboBox *combobox, gpointer user_data)
{
	ModestEasysetupWizardDialog *self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	update_incoming_server_title (self);
	update_incoming_server_security_choices (self);
}
           
static GtkWidget* create_page_custom_incoming (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The incoming server widgets: */
	if (!self->combo_incoming_servertype)
		self->combo_incoming_servertype = GTK_WIDGET (easysetup_servertype_combo_box_new ());
	easysetup_servertype_combo_box_set_active_servertype (
		EASYSETUP_SERVERTYPE_COMBO_BOX (self->combo_incoming_servertype), MODEST_PROTOCOL_STORE_POP);
	GtkWidget *caption = create_caption_new_with_asterix (self, sizegroup, 
		_("mcen_li_emailsetup_type"), self->combo_incoming_servertype, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->combo_incoming_servertype);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	if(!self->entry_incomingserver)
		self->entry_incomingserver = gtk_entry_new ();
	set_default_custom_servernames (self);

	if (self->caption_incoming)
	  gtk_widget_destroy (self->caption_incoming);
	   
	/* The caption title will be updated in update_incoming_server_title().
	 * so this default text will never be seen: */
	/* (Note: Changing the title seems pointless. murrayc) */
	self->caption_incoming = create_caption_new_with_asterix (self, sizegroup, 
		"Incoming Server", self->entry_incomingserver, NULL, HILDON_CAPTION_MANDATORY);
	update_incoming_server_title (self);
	gtk_widget_show (self->entry_incomingserver);
	gtk_box_pack_start (GTK_BOX (box), self->caption_incoming, FALSE, FALSE, 2);
	gtk_widget_show (self->caption_incoming);
	
	/* Change the caption title when the servertype changes, 
	 * as in the UI spec: */
	 g_signal_connect (G_OBJECT (self->combo_incoming_servertype), "changed",
        	G_CALLBACK (on_combo_servertype_changed), self);
	
	/* The secure connection widgets: */	
	if (!self->combo_incoming_security)
		self->combo_incoming_security = GTK_WIDGET (easysetup_serversecurity_combo_box_new ());
	update_incoming_server_security_choices (self);
	easysetup_serversecurity_combo_box_set_active_serversecurity (
		EASYSETUP_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security), MODEST_PROTOCOL_SECURITY_NONE);
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_connection"), 
		self->combo_incoming_security, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_incoming_security);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	if(!self->checkbox_incoming_auth)
		self->checkbox_incoming_auth = 
			gtk_check_button_new_with_label (_("mcen_li_emailsetup_secure_authentication"));
	gtk_box_pack_start (GTK_BOX (box), self->checkbox_incoming_auth, FALSE, FALSE, 2);
	gtk_widget_show (self->checkbox_incoming_auth);
	
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
on_smtp_servers_window_hide (GtkWindow *window, gpointer user_data)
{
	/* Destroy the window when it is closed: */
	gtk_widget_destroy (GTK_WIDGET (window));
}

static void
on_button_outgoing_smtp_servers (GtkButton *button, gpointer user_data)
{
	ModestEasysetupWizardDialog * self = MODEST_EASYSETUP_WIZARD_DIALOG (user_data);
	
	/* Show the window: */
	/* TODO: Retrieve the chosen settings,
	 * so we can supply them when creating the connection somehow.
	 */
	GtkWidget *window = GTK_WIDGET (modest_connection_specific_smtp_window_new ());

	gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (self));
	g_signal_connect (G_OBJECT (window), "hide",
        	G_CALLBACK (on_smtp_servers_window_hide), self);
	gtk_widget_show (window);
}

static GtkWidget* create_page_custom_outgoing (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The outgoing server widgets: */
	if (!self->entry_outgoingserver)
		self->entry_outgoingserver = gtk_entry_new ();
	GtkWidget *caption = create_caption_new_with_asterix (self, sizegroup, 
		_("mcen_li_emailsetup_smtp"), self->entry_outgoingserver, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_outgoingserver);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	set_default_custom_servernames (self);
	
	/* The secure connection widgets: */	
	if (!self->combo_outgoing_security)
		self->combo_outgoing_security = GTK_WIDGET (easysetup_serversecurity_combo_box_new ());
	easysetup_serversecurity_combo_box_fill (
		EASYSETUP_SERVERSECURITY_COMBO_BOX (self->combo_outgoing_security), MODEST_PROTOCOL_TRANSPORT_SMTP);
	easysetup_serversecurity_combo_box_set_active_serversecurity (
		EASYSETUP_SERVERSECURITY_COMBO_BOX (self->combo_outgoing_security), MODEST_PROTOCOL_SECURITY_NONE);
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_connection"), 
		self->combo_outgoing_security, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_outgoing_security);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* The secure authentication widgets: */
	if (!self->combo_outgoing_auth)
		self->combo_outgoing_auth = GTK_WIDGET (easysetup_secureauth_combo_box_new ());
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
		self->combo_outgoing_auth, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_outgoing_auth);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* connection-specific checkbox: */
	if (!self->checkbox_outgoing_smtp_specific) {
		self->checkbox_outgoing_smtp_specific = gtk_check_button_new_with_label (_("mcen_fi_advsetup_connection_smtp"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->checkbox_outgoing_smtp_specific), 
			FALSE);
	}
	gtk_box_pack_start (GTK_BOX (box), self->checkbox_outgoing_smtp_specific, FALSE, FALSE, 2);
	gtk_widget_show (self->checkbox_outgoing_smtp_specific);
	
	/* Connection-specific SMTP-Severs Edit button: */
	if (!self->button_outgoing_smtp_servers)
		self->button_outgoing_smtp_servers = gtk_button_new_with_label (_("mcen_bd_emailsetup_edit"));
	caption = hildon_caption_new (sizegroup, _("mcen_fi_advsetup_optional_smtp"), 
		self->button_outgoing_smtp_servers, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->button_outgoing_smtp_servers);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* Only enable the button when the checkbox is checked: */
	enable_widget_for_togglebutton (self->button_outgoing_smtp_servers, 
		GTK_TOGGLE_BUTTON (self->checkbox_outgoing_smtp_specific));
		
	g_signal_connect (G_OBJECT (self->button_outgoing_smtp_servers), "clicked",
        	G_CALLBACK (on_button_outgoing_smtp_servers), self);
	
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static GtkWidget* create_page_complete_custom (ModestEasysetupWizardDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	GtkWidget *label = gtk_label_new(_("mcen_ia_emailsetup_setup_complete"));
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	if (!self->button_edit)
		self->button_edit = gtk_button_new_with_label (_("mcen_bd_emailsetup_edit"));
	GtkWidget *caption = hildon_caption_new (NULL, _("mcen_fi_advanced_settings"), 
		self->button_edit, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->button_edit);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
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
	invoke_enable_buttons_vfunc (self);
}

static void
modest_easysetup_wizard_dialog_init (ModestEasysetupWizardDialog *self)
{
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
util_get_default_servername_from_email_address (const gchar* email_address, ModestProtocol servertype)
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

/* Add a number to the end of the text, or increment a number that is already there.
 */
static gchar*
util_increment_name (const gchar* text)
{
	/* Get the end character,
	 * also doing a UTF-8 validation which is required for using g_utf8_prev_char().
	 */
	const gchar* end = NULL;
	if (!g_utf8_validate (text, -1, &end))
		return NULL;
  
  	if (!end)
  		return NULL;
  		
  	--end; /* Go to before the null-termination. */
  		
  	/* Look at each UTF-8 characer, starting at the end: */
  	const gchar* p = end;
  	const gchar* alpha_end = NULL;
  	while (p)
  	{	
  		/* Stop when we reach the first character that is not a numeric digit: */
  		const gunichar ch = g_utf8_get_char (p);
  		if (!g_unichar_isdigit (ch)) {
  			alpha_end = p;
  			break;
  		}
  		
  		p = g_utf8_prev_char (p);	
  	}
  	
  	if(!alpha_end) {
  		/* The text must consist completely of numeric digits. */
  		alpha_end = text;
  	}
  	else
  		++alpha_end;
  	
  	/* Intepret and increment the number, if any: */
  	gint num = atol (alpha_end);
  	++num;
  	
	/* Get the name part: */
  	gint name_len = alpha_end - text;
  	gchar *name_without_number = g_malloc(name_len + 1);
  	memcpy (name_without_number, text, name_len);
  	name_without_number[name_len] = 0;\
  	
    /* Concatenate the text part and the new number: */	
  	gchar *result = g_strdup_printf("%s%d", name_without_number, num);
  	g_free (name_without_number);
  	
  	return result; 	
}
	
static void set_default_custom_servernames (ModestEasysetupWizardDialog *account_wizard)
{
	if (!account_wizard->entry_incomingserver)
		return;
		
	/* Set a default domain for the server, based on the email address,
	 * if no server name was already specified.
	 */
	const gchar* incoming_existing = gtk_entry_get_text (GTK_ENTRY (account_wizard->entry_incomingserver));
	if ((!incoming_existing || (strlen(incoming_existing) == 0)) 
		&& account_wizard->entry_user_email) {
		const ModestProtocol protocol = easysetup_servertype_combo_box_get_active_servertype (
			EASYSETUP_SERVERTYPE_COMBO_BOX (account_wizard->combo_incoming_servertype));
		const gchar* email_address = gtk_entry_get_text (GTK_ENTRY(account_wizard->entry_user_email));
		
		gchar* servername = util_get_default_servername_from_email_address (email_address, protocol);
		gtk_entry_set_text (GTK_ENTRY (account_wizard->entry_incomingserver), servername);
		g_free (servername);
	}
	
	/* Set a default domain for the server, based on the email address,
	 * if no server name was already specified.
	 */
	const gchar* outgoing_existing = gtk_entry_get_text (GTK_ENTRY (account_wizard->entry_outgoingserver));
	if ((!outgoing_existing || (strlen(outgoing_existing) == 0)) 
		&& account_wizard->entry_user_email) {
		const gchar* email_address = gtk_entry_get_text (GTK_ENTRY(account_wizard->entry_user_email));
		
		gchar* servername = util_get_default_servername_from_email_address (email_address, MODEST_PROTOCOL_TRANSPORT_SMTP);
		gtk_entry_set_text (GTK_ENTRY (account_wizard->entry_outgoingserver), servername);
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
		const gchar* account_name = gtk_entry_get_text (GTK_ENTRY (account_wizard->entry_account_title));
		if ((!account_name) || (strlen(account_name) == 0))
			return FALSE;
			
		/* Aavoid a clash with an existing display name: */
		const gboolean name_in_use = modest_account_mgr_account_with_display_name_exists (
			account_wizard->account_manager, account_name);

		if (name_in_use) {
			/* Warn the user via a dialog: */
			show_error (GTK_WINDOW (account_wizard), _("mail_ib_account_name_already_existing"));
            
			return FALSE;
		}
	}
	else if (current_page == account_wizard->page_user_details) {
		/* Check that the email address is valud: */
		const gchar* email_address = gtk_entry_get_text (GTK_ENTRY (account_wizard->entry_user_email));
		if ((!email_address) || (strlen(email_address) == 0))
			return FALSE;
			
		if (!modest_text_utils_validate_email_address (email_address)) {
			/* Warn the user via a dialog: */
			show_error (GTK_WINDOW (account_wizard), _("mcen_ib_invalid_email"));
                                             
            /* Return focus to the email address entry: */
            gtk_widget_grab_focus (account_wizard->entry_user_email);
            
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
	}
	
	/* If this is the last page, and this is a click on Finish, 
	 * then attempt to create the dialog.
	 */
	if(!next_page) /* This is NULL when this is a click on Finish. */
	{
		create_account (account_wizard);
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
	 * identifying them via their associated response codes:
	 */
	GtkDialog *dialog_base = GTK_DIALOG (dialog);
    gtk_dialog_set_response_sensitive (dialog_base,
                                       MODEST_WIZARD_DIALOG_NEXT,
                                       enable_next);
                                       
    /* Disable the Finish button until we are on the last page,
     * because HildonWizardDialog enables this for all but the first page: */
    GtkNotebook *notebook = NULL;
	g_object_get (dialog_base, "wizard-notebook", &notebook, NULL);
	
    gint current = gtk_notebook_get_current_page (notebook);
    gint last = gtk_notebook_get_n_pages (notebook) - 1;
    gboolean is_last = (current == last);
    
    if(!is_last) {
    	gtk_dialog_set_response_sensitive (dialog_base,
                                       MODEST_WIZARD_DIALOG_FINISH,
                                       FALSE);
    }
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
show_error (GtkWindow *parent_window, const gchar* text)
{
	GtkDialog *dialog = GTK_DIALOG (gtk_message_dialog_new (parent_window,
		(GtkDialogFlags)0,
		 GTK_MESSAGE_ERROR,
		 GTK_BUTTONS_OK,
		 text ));
		 
		 gtk_dialog_run (dialog);
		 gtk_widget_destroy (GTK_WIDGET (dialog));
}

/** Attempt to create the account from the information that the user has entered.
 * @result: TRUE if the account was successfully created.
 */
gboolean
create_account (ModestEasysetupWizardDialog *self)
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
	gchar *account_name = g_strdup_printf ("%sID", display_name);
	while (modest_account_mgr_account_exists (self->account_manager, 
		account_name, FALSE /*  server_account */)) {
			
		gchar * account_name2 = util_increment_name (account_name);
		g_free (account_name);
		account_name = account_name2;
	}
		
	/* username and password (for both incoming and outgoing): */
	const gchar* username = gtk_entry_get_text (GTK_ENTRY (self->entry_user_username));
	const gchar* password = gtk_entry_get_text (GTK_ENTRY (self->entry_user_password));
	
	/* Incoming server: */
	/* Note: We need something as default for the ModestProtocol values, 
	 * or modest_account_mgr_add_server_account will fail. */
	gchar* servername_incoming = NULL;
	ModestProtocol protocol_incoming = MODEST_PROTOCOL_STORE_POP;
	ModestProtocol protocol_security_incoming = MODEST_PROTOCOL_SECURITY_NONE;
	ModestProtocol protocol_authentication_incoming = MODEST_PROTOCOL_AUTH_NONE;
	
	/* Get details from the specified presets: */
	gchar* provider_id = easysetup_provider_combo_box_get_active_provider_id (
		EASYSETUP_PROVIDER_COMBO_BOX (self->combo_account_serviceprovider));
	if(provider_id) {
		/* Use presets: */
		
		servername_incoming = modest_presets_get_server (priv->presets, provider_id, 
			TRUE /* incoming */);
		
		ModestPresetsServerType servertype_incoming = modest_presets_get_info_server_type (priv->presets, provider_id, 
			TRUE /* incoming */);
		
	
		/* We don't check for SMTP here as that is impossible for an incoming server. */
		if (servertype_incoming == MODEST_PRESETS_SERVER_TYPE_IMAP)
			protocol_incoming = MODEST_PROTOCOL_STORE_IMAP;
		else if (servertype_incoming == MODEST_PRESETS_SERVER_TYPE_POP)
			protocol_incoming = MODEST_PROTOCOL_STORE_POP;
				
		ModestPresetsSecurity security_incoming = modest_presets_get_info_server_security (priv->presets, provider_id, 
			TRUE /* incoming */);
			
		
		if (security_incoming & MODEST_PRESETS_SECURITY_SECURE_INCOMING)
			protocol_security_incoming = MODEST_PROTOCOL_SECURITY_SSL; /* TODO: Is this what we want? */
		
		if (security_incoming & MODEST_PRESETS_SECURITY_APOP)
			protocol_authentication_incoming = MODEST_PROTOCOL_AUTH_PASSWORD; /* TODO: Is this what we want? */
	}
	else {
		/* Use custom pages because no preset was specified: */
		servername_incoming = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->entry_incomingserver) ));
		
		protocol_incoming = easysetup_servertype_combo_box_get_active_servertype (
		EASYSETUP_SERVERTYPE_COMBO_BOX (self->combo_incoming_servertype));
		
		protocol_security_incoming = easysetup_serversecurity_combo_box_get_active_serversecurity (
		EASYSETUP_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security));
		
		protocol_authentication_incoming = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->checkbox_incoming_auth)) 
			? MODEST_PROTOCOL_AUTH_PASSWORD
			: MODEST_PROTOCOL_AUTH_NONE;
		
	}
	
	/* First we add the 2 server accounts, and then we add the account that uses them.
	 * If we don't do it in this order then we will experience a crash. */
	 
	/* Add a (incoming) server account, to be used by the account: */
	gchar *store_name = g_strconcat (account_name, "_store", NULL);
	gboolean created = modest_account_mgr_add_server_account (self->account_manager,
		store_name,
		servername_incoming,
		username, password,
		protocol_incoming,
		protocol_security_incoming,
		protocol_authentication_incoming);		
		
	g_free (servername_incoming);
	
	if (!created) {
		/* TODO: Provide a Logical ID for the text: */
		show_error (GTK_WINDOW (self), _("An error occurred while creating the incoming account."));
		return FALSE;	
	}
	
	/* Sanity check: */
	/* There must be at least one account now: */
	GSList *account_names = modest_account_mgr_account_names (self->account_manager);
	if(!account_names)
	{
		g_warning ("modest_account_mgr_account_names() returned NULL after adding an account.");
	}
	g_slist_free (account_names);
	
	
	/* Outgoing server: */
	gchar* servername_outgoing = NULL;
	ModestProtocol protocol_outgoing = MODEST_PROTOCOL_STORE_POP;
	ModestProtocol protocol_security_outgoing = MODEST_PROTOCOL_SECURITY_NONE;
	ModestProtocol protocol_authentication_outgoing = MODEST_PROTOCOL_AUTH_NONE;
	
	if(provider_id) {
		/* Use presets: */
		servername_outgoing = modest_presets_get_server (priv->presets, provider_id, 
			FALSE /* incoming */);
			
		ModestPresetsServerType servertype_outgoing = modest_presets_get_info_server_type (priv->presets, provider_id, 
			FALSE /* incoming */);
		
		/* Note: We need something as default, or modest_account_mgr_add_server_account will fail. */
		protocol_outgoing = MODEST_PROTOCOL_TRANSPORT_SENDMAIL; 
		if (servertype_outgoing == MODEST_PRESETS_SERVER_TYPE_SMTP)
			protocol_outgoing = MODEST_PROTOCOL_TRANSPORT_SMTP; /* TODO: Is this what we want? */
		
		ModestPresetsSecurity security_outgoing = 
			modest_presets_get_info_server_security (priv->presets, provider_id, 
				FALSE /* incoming */);
			
		protocol_security_outgoing = MODEST_PROTOCOL_SECURITY_NONE;
		if (security_outgoing & MODEST_PRESETS_SECURITY_SECURE_SMTP)
			protocol_security_outgoing = MODEST_PROTOCOL_SECURITY_SSL; /* TODO: Is this what we want? */
		
		protocol_authentication_outgoing = MODEST_PROTOCOL_AUTH_NONE;
		/* TODO: There is no SMTP authentication enum for presets. */
	}
	else {
		/* Use custom pages because no preset was specified: */
		servername_outgoing = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->entry_outgoingserver) ));
		
		protocol_outgoing = MODEST_PROTOCOL_TRANSPORT_SMTP; /* It's always SMTP for outgoing. */

		protocol_security_outgoing = easysetup_serversecurity_combo_box_get_active_serversecurity (
			EASYSETUP_SERVERSECURITY_COMBO_BOX (self->combo_outgoing_security));
		
		protocol_authentication_outgoing = easysetup_secureauth_combo_box_get_active_secureauth (
			EASYSETUP_SECUREAUTH_COMBO_BOX (self->combo_outgoing_auth));
		
		/* TODO: 
		gboolean specific = gtk_toggle_button_get_active (
			GTK_TOGGLE_BUTTON (self->checkbox_outgoing_smtp_specific));
		*/
		
	}
	    
	/* Add a (outgoing) server account to be used by the account: */
	gchar *transport_name = g_strconcat (account_name, "_transport", NULL); /* What is this good for? */
	created = modest_account_mgr_add_server_account (self->account_manager,
		transport_name,
		servername_outgoing,
		username, password,
		protocol_outgoing,
		protocol_security_outgoing,
		protocol_authentication_outgoing);
		
	g_free (servername_outgoing);
		
	if (!created) {
		/* TODO: Provide a Logical ID for the text: */
		show_error (GTK_WINDOW (self), _("An error occurred while creating the outgoing account."));
		return FALSE;	
	}
	
	
	/* Create the account, which will contain the two "server accounts": */
 	created = modest_account_mgr_add_account (self->account_manager, account_name, 
 		store_name, /* The name of our POP/IMAP server account. */
		transport_name /* The name of our SMTP server account. */);
	g_free (store_name);
	g_free (transport_name);
	
	if (!created) {
		/* TODO: Provide a Logical ID for the text: */
		show_error (GTK_WINDOW (self), _("An error occurred while creating the account."));
		return FALSE;	
	}
	
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

	g_free (account_name);

	return FALSE;
}



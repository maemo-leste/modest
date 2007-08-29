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


#include "modest-account-settings-dialog.h"
#include <glib/gi18n.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkstock.h>
#include "modest-hildon-includes.h"

#include "widgets/modest-serversecurity-combo-box.h"
#include "widgets/modest-secureauth-combo-box.h"
#include "widgets/modest-validating-entry.h"
#include "widgets/modest-retrieve-combo-box.h"
#include "widgets/modest-limit-retrieve-combo-box.h"
#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-account-mgr-helpers.h" /* For modest_account_mgr_get_account_data(). */
#include "modest-runtime.h" /* For modest_runtime_get_account_mgr(). */
#include "modest-protocol-info.h"
#include "maemo/modest-connection-specific-smtp-window.h"
#include "maemo/modest-signature-editor-dialog.h"
#include "maemo/modest-maemo-utils.h"
#include "widgets/modest-ui-constants.h"

#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <tny-status.h>

#include <gconf/gconf-client.h>
#include <string.h> /* For strlen(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define EXAMPLE_EMAIL_ADDRESS "first.last@provider.com"

#define PORT_MIN 1
#define PORT_MAX 65535

G_DEFINE_TYPE (ModestAccountSettingsDialog, modest_account_settings_dialog, GTK_TYPE_DIALOG);

#define ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, ModestAccountSettingsDialogPrivate))

typedef struct _ModestAccountSettingsDialogPrivate ModestAccountSettingsDialogPrivate;

struct _ModestAccountSettingsDialogPrivate
{
};

static void
enable_buttons (ModestAccountSettingsDialog *self);

static gboolean
save_configuration (ModestAccountSettingsDialog *dialog);

static void
modest_account_settings_dialog_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_account_settings_dialog_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_account_settings_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_account_settings_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (modest_account_settings_dialog_parent_class)->dispose (object);
}

static void
modest_account_settings_dialog_finalize (GObject *object)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (object);
	
	if (self->account_name)
		g_free (self->account_name);
		
	if (self->original_account_title)
		g_free (self->original_account_title);
		
	if (self->account_manager)
		g_object_unref (G_OBJECT (self->account_manager));
		
	if (self->specific_window)
		gtk_widget_destroy (self->specific_window);
		
	if (self->signature_dialog)
		gtk_widget_destroy (self->signature_dialog);
	
	G_OBJECT_CLASS (modest_account_settings_dialog_parent_class)->finalize (object);
}

static void
show_error (GtkWidget *parent_widget, const gchar* text);

static void
on_combo_incoming_security_changed (GtkComboBox *widget, gpointer user_data);

static void
on_combo_outgoing_security_changed (GtkComboBox *widget, gpointer user_data);

static void
on_modified_combobox_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	self->modified = TRUE;
}

static void
on_modified_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	self->modified = TRUE;
}

static void
on_modified_checkbox_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	self->modified = TRUE;
}

static void
on_modified_number_editor_changed (HildonNumberEditor *number_editor, gint new_value, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	self->modified = TRUE;
}

/* Set a modified boolean whenever the widget is changed, 
 * so we can check for it later.
 */
static void
connect_for_modified (ModestAccountSettingsDialog *self, GtkWidget *widget)
{
	if (HILDON_IS_NUMBER_EDITOR (widget)) {
		g_signal_connect (G_OBJECT (widget), "notify::value",
			G_CALLBACK (on_modified_number_editor_changed), self);
	}
	else if (GTK_IS_ENTRY (widget)) {
		g_signal_connect (G_OBJECT (widget), "changed",
			G_CALLBACK (on_modified_entry_changed), self);
	} else if (GTK_IS_COMBO_BOX (widget)) {
		g_signal_connect (G_OBJECT (widget), "changed",
        		G_CALLBACK (on_modified_combobox_changed), self);	
	} else if (GTK_IS_TOGGLE_BUTTON (widget)) {
		g_signal_connect (G_OBJECT (widget), "toggled",
			G_CALLBACK (on_modified_checkbox_toggled), self);
	}
}

static void
on_caption_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	g_assert(self);
	enable_buttons(self);
}

static void
on_caption_combobox_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	g_assert(self);
	enable_buttons(self);
}

/** This is a convenience function to create a caption containing a mandatory widget.
 * When the widget is edited, the enable_buttons() vfunc will be called.
 */
static GtkWidget* create_caption_new_with_asterisk(ModestAccountSettingsDialog *self,
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

static void
on_entry_invalid_account_title_character (ModestValidatingEntry *self, const gchar* character, gpointer user_data)
{
	gchar *tmp, *msg;
			
	tmp = g_strndup (account_title_forbidden_chars, ACCOUNT_TITLE_FORBIDDEN_CHARS_LENGTH);
	msg = g_strdup_printf (_CS("ckdg_ib_illegal_characters_entered"), tmp);

	show_error (GTK_WIDGET (self), msg);

	g_free (msg);
	g_free (tmp);
}

static void
on_entry_invalid_fullname_character (ModestValidatingEntry *self, const gchar* character, gpointer user_data)
{
	gchar *tmp, *msg;
			
	tmp = g_strndup (user_name_forbidden_chars, USER_NAME_FORBIDDEN_CHARS_LENGTH);
	msg = g_strdup_printf (_CS("ckdg_ib_illegal_characters_entered"), tmp);

	show_error (GTK_WIDGET (self), msg);

	g_free (msg);
	g_free (tmp);
}


static void
on_entry_max (ModestValidatingEntry *self, gpointer user_data)
{
	/* ModestAccountSettingsDialog *dialog = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data); */
	show_error (GTK_WIDGET (self), _CS("ckdg_ib_maximum_characters_reached"));
}

static GtkWidget*
create_page_account_details (ModestAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *label = gtk_label_new(_("mcen_ia_accountdetails"));
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (label);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
           
	/* The description widgets: */	
	self->entry_account_title = GTK_WIDGET (modest_validating_entry_new ());
	/* Do use auto-capitalization: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_account_title), 
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_AUTOCAP);
	GtkWidget *caption = create_caption_new_with_asterisk (self, sizegroup, _("mcen_fi_account_title"), 
		self->entry_account_title, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_account_title);
	connect_for_modified (self, self->entry_account_title);
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
	list_prevent = g_list_append (list_prevent, "\"");
	list_prevent = g_list_append (list_prevent, "<"); 
	list_prevent = g_list_append (list_prevent, ">"); 
	list_prevent = g_list_append (list_prevent, "|");
	list_prevent = g_list_append (list_prevent, "^"); 	
	modest_validating_entry_set_unallowed_characters (
	 	MODEST_VALIDATING_ENTRY (self->entry_account_title), list_prevent);
	g_list_free (list_prevent);
	modest_validating_entry_set_func(MODEST_VALIDATING_ENTRY(self->entry_account_title),
					 on_entry_invalid_account_title_character, self);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_account_title), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (self->entry_account_title), 
		on_entry_max, self);
	
	/* The retrieve combobox: */
	self->combo_retrieve = GTK_WIDGET (modest_retrieve_combo_box_new ());
	caption = create_caption_new_with_asterisk (self, sizegroup, _("mcen_fi_advsetup_retrievetype"), 
		self->combo_retrieve, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->combo_retrieve);
	connect_for_modified (self, self->combo_retrieve);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* The limit-retrieve combobox: */
	self->combo_limit_retrieve = GTK_WIDGET (modest_limit_retrieve_combo_box_new ());
	caption = create_caption_new_with_asterisk (self, sizegroup, _("mcen_fi_advsetup_limit_retrieve"), 
		self->combo_limit_retrieve, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->combo_limit_retrieve);
	connect_for_modified (self, self->combo_limit_retrieve);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);

	/* The leave-messages widgets: */
	if(!self->checkbox_leave_messages)
		self->checkbox_leave_messages = gtk_check_button_new ();
	if (!self->caption_leave_messages) {
		self->caption_leave_messages = create_caption_new_with_asterisk (self, sizegroup, _("mcen_fi_advsetup_leave_on_server"), 
			self->checkbox_leave_messages, NULL, HILDON_CAPTION_MANDATORY);
	}
			
	gtk_widget_show (self->checkbox_leave_messages);
	connect_for_modified (self, self->checkbox_leave_messages);
	gtk_box_pack_start (GTK_BOX (box), self->caption_leave_messages, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (self->caption_leave_messages);
	
	gtk_widget_show (GTK_WIDGET (box));
	
	return GTK_WIDGET (box);
}

static gchar*
get_entered_account_title (ModestAccountSettingsDialog *dialog)
{
	const gchar* account_title = 
		gtk_entry_get_text (GTK_ENTRY (dialog->entry_account_title));
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
on_button_signature (GtkButton *button, gpointer user_data)
{
	ModestAccountSettingsDialog * self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	
	/* Create the window, if necessary: */
	if (!(self->signature_dialog)) {
		self->signature_dialog = GTK_WIDGET (modest_signature_editor_dialog_new ());
	
		gboolean use_signature = FALSE;
		gchar *signature = modest_account_mgr_get_signature(self->account_manager, self->account_name, 
			&use_signature);
		gchar* account_title = get_entered_account_title (self);
		modest_signature_editor_dialog_set_settings (
			MODEST_SIGNATURE_EDITOR_DIALOG (self->signature_dialog), 
			use_signature, signature, account_title);
		g_free (account_title);
		account_title = NULL;
		g_free (signature);
		signature = NULL;
	}

	/* Show the window: */	
	gtk_window_set_transient_for (GTK_WINDOW (self->signature_dialog), GTK_WINDOW (self));
	gtk_window_set_modal (GTK_WINDOW (self->signature_dialog), TRUE);
    const gint response = gtk_dialog_run (GTK_DIALOG (self->signature_dialog));
    gtk_widget_hide (self->signature_dialog);
    if (response != GTK_RESPONSE_OK) {
    	/* Destroy the widget now, and its data: */
    	gtk_widget_destroy (self->signature_dialog);
    	self->signature_dialog = NULL;
    }
    else {
    	/* Mark modified, so we use the dialog's data later: */
    	self->modified = TRUE;	
    }
}

static GtkWidget*
create_page_user_details (ModestAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkAdjustment *focus_adjustment = NULL;
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	GtkWidget *scrollwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	 
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
	gtk_widget_show (self->entry_user_name);
	connect_for_modified (self, self->entry_user_name);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);


	/* Prevent the use of some characters in the name, 
	 * as required by our UI specification: */
	GList *list_prevent = NULL;
	list_prevent = g_list_append (list_prevent, "<");
	list_prevent = g_list_append (list_prevent, ">");
	modest_validating_entry_set_unallowed_characters (
	 	MODEST_VALIDATING_ENTRY (self->entry_user_name), list_prevent);
	g_list_free (list_prevent);
	modest_validating_entry_set_func(MODEST_VALIDATING_ENTRY(self->entry_user_name),
					 on_entry_invalid_fullname_character, self);
	
	/* The username widgets: */	
	self->entry_user_username = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_user_username), HILDON_GTK_INPUT_MODE_FULL);
	caption = create_caption_new_with_asterisk (self, sizegroup, _("mail_fi_username"), 
		self->entry_user_username, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_user_username);
	connect_for_modified (self, self->entry_user_username);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	modest_validating_entry_set_unallowed_characters_whitespace (
	 	MODEST_VALIDATING_ENTRY (self->entry_user_username));
	
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
	gtk_widget_show (self->entry_user_password);
	connect_for_modified (self, self->entry_user_password);
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
	connect_for_modified (self, self->entry_user_email);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_user_email), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (self->entry_user_email), 
		on_entry_max, self);
	
	/* Signature button: */
	if (!self->button_signature)
		self->button_signature = gtk_button_new_with_label (_("mcen_bd_edit"));
	caption = hildon_caption_new (sizegroup, _("mcen_fi_email_signature"), 
		self->button_signature, NULL, HILDON_CAPTION_OPTIONAL);
	hildon_caption_set_child_expand (HILDON_CAPTION (caption), FALSE);
	gtk_widget_show (self->button_signature);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
		
	g_signal_connect (G_OBJECT (self->button_signature), "clicked",
        	G_CALLBACK (on_button_signature), self);
        	
	gtk_widget_show (GTK_WIDGET (box));
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwin), box);
	gtk_widget_show (scrollwin);

	focus_adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrollwin));
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (box), focus_adjustment); 
	
	return GTK_WIDGET (scrollwin);
}

/** Change the caption title for the incoming server, 
 * as specified in the UI spec:
 */
static void update_incoming_server_title (ModestAccountSettingsDialog *self, ModestTransportStoreProtocol protocol)
{
	const gchar* type = 
		(protocol == MODEST_PROTOCOL_STORE_POP ? 
			_("mail_fi_emailtype_pop3") : 
			_("mail_fi_emailtype_imap") );
			
		
	/* Note that this produces a compiler warning, 
	 * because the compiler does not know that the translated string will have a %s in it.
	 * I do not see a way to avoid the warning while still using these Logical IDs. murrayc. */
	gchar* incomingserver_title = g_strdup_printf(_("mcen_li_emailsetup_servertype"), type);
	
	/* This is a mandatory field, so add a *. This is usually done by 
	 * create_caption_new_with_asterisk() but we can't use that here. */
	gchar *with_asterisk = g_strconcat (incomingserver_title, "*", NULL);
	g_free (incomingserver_title);
	
	g_object_set (G_OBJECT (self->caption_incoming), "label", with_asterisk, NULL);
	g_free(with_asterisk);
}

/** Change the caption title for the incoming server, 
 * as specified in the UI spec:
 */
static void update_incoming_server_security_choices (ModestAccountSettingsDialog *self, ModestTransportStoreProtocol protocol)
{
	/* Fill the combo with appropriately titled choices for POP or IMAP. */
	/* The choices are the same, but the titles are different, as in the UI spec. */
	modest_serversecurity_combo_box_fill (
		MODEST_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security), protocol);
}
           
static GtkWidget* create_page_incoming (ModestAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The incoming server widgets: */
	if(!self->entry_incomingserver)
		self->entry_incomingserver = gtk_entry_new ();
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_incomingserver), HILDON_GTK_INPUT_MODE_FULL);

	if (self->caption_incoming)
	  gtk_widget_destroy (self->caption_incoming);
	   
	/* The caption title will be updated in update_incoming_server_title().
	 * so this default text will never be seen: */
	/* (Note: Changing the title seems pointless. murrayc) */
	self->caption_incoming = create_caption_new_with_asterisk (self, sizegroup, 
		"Incoming Server", self->entry_incomingserver, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_incomingserver);
	connect_for_modified (self, self->entry_incomingserver);
	gtk_box_pack_start (GTK_BOX (box), self->caption_incoming, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (self->caption_incoming);
	
	/* The secure connection widgets: */
	/* This will be filled by update_incoming_server_security_choices(). */
	if (!self->combo_incoming_security)
		self->combo_incoming_security = GTK_WIDGET (modest_serversecurity_combo_box_new ());
	GtkWidget *caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_connection"), 
		self->combo_incoming_security, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_incoming_security);
	connect_for_modified (self, self->combo_incoming_security);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Show a default port number when the security method changes, as per the UI spec: */
	g_signal_connect (G_OBJECT (self->combo_incoming_security), "changed", (GCallback)on_combo_incoming_security_changed, self);
	
	
	/* The port widgets: */
	if (!self->entry_incoming_port)
		self->entry_incoming_port = GTK_WIDGET (hildon_number_editor_new (PORT_MIN, PORT_MAX));
	caption = hildon_caption_new (sizegroup, _("mcen_fi_emailsetup_port"), 
		self->entry_incoming_port, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_incoming_port);
	connect_for_modified (self, self->entry_incoming_port);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* The secure authentication widgets: */
	if(!self->checkbox_incoming_auth)
		self->checkbox_incoming_auth = gtk_check_button_new ();
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
		self->checkbox_incoming_auth, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->checkbox_incoming_auth);
	connect_for_modified (self, self->checkbox_incoming_auth);
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
	ModestAccountSettingsDialog * self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	
	/* Create the window if necessary: */
	if (!(self->specific_window)) {
		self->specific_window = GTK_WIDGET (modest_connection_specific_smtp_window_new ());
		modest_connection_specific_smtp_window_fill_with_connections (
			MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (self->specific_window), self->account_manager, 
			self->account_name);
	}

	/* Show the window: */	
	gtk_window_set_transient_for (GTK_WINDOW (self->specific_window), GTK_WINDOW (self));
	gtk_window_set_modal (GTK_WINDOW (self->specific_window), TRUE);
    gtk_widget_show (self->specific_window);
}

static void
on_combo_outgoing_auth_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	
	ModestAuthProtocol protocol_security = 
		modest_secureauth_combo_box_get_active_secureauth (
			MODEST_SECUREAUTH_COMBO_BOX (self->combo_outgoing_auth));
	const gboolean secureauth_used = protocol_security != MODEST_PROTOCOL_AUTH_NONE;
	
	gtk_widget_set_sensitive (self->caption_outgoing_username, secureauth_used);
	gtk_widget_set_sensitive (self->caption_outgoing_password, secureauth_used);
}

static void
on_combo_outgoing_security_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	
	const gint port_number = 
		modest_serversecurity_combo_box_get_active_serversecurity_port (
			MODEST_SERVERSECURITY_COMBO_BOX (self->combo_outgoing_security));

	if(port_number != 0) {
		hildon_number_editor_set_value (
			HILDON_NUMBER_EDITOR (self->entry_outgoing_port), port_number);
	}		
}

static void
on_combo_incoming_security_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	
	const gint port_number = 
		modest_serversecurity_combo_box_get_active_serversecurity_port (
			MODEST_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security));

	if(port_number != 0) {
		hildon_number_editor_set_value (
			HILDON_NUMBER_EDITOR (self->entry_incoming_port), port_number);
	}		
}


static GtkWidget* create_page_outgoing (ModestAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkAdjustment *focus_adjustment = NULL;
	
	/* Put it all in a scrolled window, so that all widgets can be 
	 * accessed even when the on-screen keyboard is visible: */
	GtkWidget *scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), 
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The outgoing server widgets: */
	if (!self->entry_outgoingserver)
		self->entry_outgoingserver = gtk_entry_new ();
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_outgoingserver), HILDON_GTK_INPUT_MODE_FULL);
	GtkWidget *caption = create_caption_new_with_asterisk (self, sizegroup, 
		_("mcen_li_emailsetup_smtp"), self->entry_outgoingserver, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_outgoingserver);
	connect_for_modified (self, self->entry_outgoingserver);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* The secure authentication widgets: */
	if (!self->combo_outgoing_auth)
		self->combo_outgoing_auth = GTK_WIDGET (modest_secureauth_combo_box_new ());
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
		self->combo_outgoing_auth, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_outgoing_auth);
	connect_for_modified (self, self->combo_outgoing_auth);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Dim the outgoing username and password when no secure authentication is used, as per the UI spec: */
	g_signal_connect (G_OBJECT (self->combo_outgoing_auth), "changed", (GCallback)on_combo_outgoing_auth_changed, self);
	
	/* The username widgets: */	
	self->entry_outgoing_username = GTK_WIDGET (modest_validating_entry_new ());
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_outgoing_username), HILDON_GTK_INPUT_MODE_FULL);
	self->caption_outgoing_username = create_caption_new_with_asterisk (self, sizegroup, _("mail_fi_username"), 
		self->entry_outgoing_username, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (self->entry_outgoing_username);
	connect_for_modified (self, self->entry_outgoing_username);
	gtk_box_pack_start (GTK_BOX (box), self->caption_outgoing_username, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (self->caption_outgoing_username);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	modest_validating_entry_set_unallowed_characters_whitespace (
	 	MODEST_VALIDATING_ENTRY (self->entry_outgoing_username));
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (self->entry_outgoing_username), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (self->entry_outgoing_username), 
		on_entry_max, self);
		
	/* The password widgets: */	
	self->entry_outgoing_password = gtk_entry_new ();
	/* Auto-capitalization is the default, so let's turn it off: */
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (self->entry_outgoing_password), 
		HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_INVISIBLE);
	gtk_entry_set_visibility (GTK_ENTRY (self->entry_outgoing_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (self->entry_outgoing_password), '*'); */
	self->caption_outgoing_password = create_caption_new_with_asterisk (self, sizegroup, 
		_("mail_fi_password"), self->entry_outgoing_password, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_outgoing_password);
	connect_for_modified (self, self->entry_outgoing_password);
	gtk_box_pack_start (GTK_BOX (box), self->caption_outgoing_password, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (self->caption_outgoing_password);
	
	/* The secure connection widgets: */
	/* This will be filled and set with modest_serversecurity_combo_box_fill() 
	 * and modest_serversecurity_combo_box_set_active_serversecurity().
	 */
	if (!self->combo_outgoing_security)
		
		self->combo_outgoing_security = GTK_WIDGET (modest_serversecurity_combo_box_new ());
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_connection"), 
		self->combo_outgoing_security, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->combo_outgoing_security);
	connect_for_modified (self, self->combo_outgoing_security);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	
	/* Show a default port number when the security method changes, as per the UI spec: */
	g_signal_connect (G_OBJECT (self->combo_outgoing_security), "changed", (GCallback)on_combo_outgoing_security_changed, self);
	
	/* The port widgets: */
	if (!self->entry_outgoing_port)
		self->entry_outgoing_port = GTK_WIDGET (hildon_number_editor_new (PORT_MIN, PORT_MAX));
	caption = hildon_caption_new (sizegroup, _("mcen_fi_emailsetup_port"), 
		self->entry_outgoing_port, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->entry_outgoing_port);
	connect_for_modified (self, self->entry_outgoing_port);
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
	}
	caption = hildon_caption_new (sizegroup, _("mcen_fi_advsetup_connection_smtp"), 
		self->checkbox_outgoing_smtp_specific, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (self->checkbox_outgoing_smtp_specific);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (caption);
	connect_for_modified (self, self->checkbox_outgoing_smtp_specific);
	
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
		
	gtk_widget_show (GTK_WIDGET (box));
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrollwin), box);
	gtk_widget_show(scrollwin);

	focus_adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrollwin));
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (box), focus_adjustment);
	
	return GTK_WIDGET (scrollwin);
}


/** TODO: This doesn't work because hildon_number_editor_get_value() does not work until 
 * focus has been lost:
 * See https://bugs.maemo.org/show_bug.cgi?id=1806.
 */
static gboolean
check_hildon_number_editor_and_warn_value_not_in_range (HildonNumberEditor *widget, gint min, gint max)
{
	g_return_val_if_fail (widget, FALSE);
	
	const gint port = hildon_number_editor_get_value (widget);
	/* printf ("DEBUG: %s, port=%d\n", __FUNCTION__, port); */
	if (port < PORT_MIN || 
		port > PORT_MAX) {
			
		/* Warn the user via a dialog: */
		/*show_error (GTK_WINDOW (self), _("mcen_ib_invalid_email"));*/
		gchar *message = g_strdup_printf (_CS("ckct_ib_set_a_value_within_range"), 
				       min, 
				       max);
		hildon_banner_show_information (GTK_WIDGET (widget), NULL, message);
		g_free (message);
		message = NULL;

		/* Return focus to the email address entry: */
		gtk_widget_grab_focus (GTK_WIDGET (widget));
		
		return FALSE;
	}
	
	return TRUE;
}

	
static gboolean
check_data (ModestAccountSettingsDialog *self)
{
	/* Check that the title is not already in use: */
	gchar* account_title = get_entered_account_title (self);
	if (!account_title)
		return FALSE; /* Should be prevented already anyway. */
		
	if (strcmp(account_title, self->original_account_title) != 0) {
		/* Check the changed title: */
		const gboolean name_in_use  = modest_account_mgr_account_with_display_name_exists (self->account_manager,
			account_title);
	
		if (name_in_use) {
			/* Warn the user via a dialog: */
			hildon_banner_show_information(NULL, NULL, _("mail_ib_account_name_already_existing"));
	        
	        g_free (account_title);
			return FALSE;
		}
	}
	
	g_free (account_title);
	account_title  = NULL;

	/* Check that the email address is valid: */
	const gchar* email_address = gtk_entry_get_text (GTK_ENTRY (self->entry_user_email));
	if ((!email_address) || (strlen(email_address) == 0)) {
		return FALSE;
	}
			
	if (!modest_text_utils_validate_email_address (email_address, NULL)) {
		/* Warn the user via a dialog: */
		/*show_error (GTK_WINDOW (self), _("mcen_ib_invalid_email"));*/
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_invalid_email"));
                                         
	        /* Return focus to the email address entry: */
        	gtk_widget_grab_focus (self->entry_user_email);
		gtk_editable_select_region (GTK_EDITABLE (self->entry_user_email), 0, -1);
		return FALSE;
	}

	/* make sure the domain name for the incoming server is valid */
	const gchar* hostname = gtk_entry_get_text (GTK_ENTRY (self->entry_incomingserver));
	if ((!hostname) || (strlen(hostname) == 0)) {
		return FALSE;
	}
	
	if (!modest_text_utils_validate_domain_name (hostname)) {
		/* Warn the user via a dialog: */
		/*show_error (GTK_WINDOW (self), _("mcen_ib_invalid_email"));*/
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_invalid_servername"));
                                         
		/* Return focus to the email address entry: */
        gtk_widget_grab_focus (self->entry_incomingserver);
		gtk_editable_select_region (GTK_EDITABLE (self->entry_incomingserver), 0, -1);
		return FALSE;
	}

	/* make sure the domain name for the outgoing server is valid */
	const gchar* hostname2 = gtk_entry_get_text (GTK_ENTRY (self->entry_outgoingserver));
	if ((!hostname2) || (strlen(hostname2) == 0)) {
		return FALSE;
	}
	
	if (!modest_text_utils_validate_domain_name (hostname2)) {
		/* Warn the user via a dialog: */
		/*show_error (GTK_WINDOW (self), _("mcen_ib_invalid_email"));*/
		hildon_banner_show_information (self->entry_outgoingserver, NULL, _("mcen_ib_invalid_servername"));

		/* Return focus to the email address entry: */
		gtk_widget_grab_focus (self->entry_outgoingserver);
		gtk_editable_select_region (GTK_EDITABLE (self->entry_outgoingserver), 0, -1);
		return FALSE;
	}
	
	/* Check that the port numbers are acceptable: */
	if (!check_hildon_number_editor_and_warn_value_not_in_range ( 
		HILDON_NUMBER_EDITOR (self->entry_incoming_port), PORT_MIN, PORT_MAX)) {
		return FALSE;
	}
		
	if (!check_hildon_number_editor_and_warn_value_not_in_range ( 
		HILDON_NUMBER_EDITOR (self->entry_outgoing_port), PORT_MIN, PORT_MAX)) {
		return FALSE;
	}

		
	/* Find a suitable authentication method when secure authentication is desired */

	const gint port_num = hildon_number_editor_get_value (
			HILDON_NUMBER_EDITOR (self->entry_incoming_port));
	const gchar* username = gtk_entry_get_text (GTK_ENTRY (self->entry_user_username));

	const ModestConnectionProtocol protocol_security_incoming = modest_serversecurity_combo_box_get_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (self->combo_incoming_security));

	/* If we use an encrypted protocol then there is no need to encrypt the password */
	/* I don't think this is a good assumption. It overrides the user's request. murrayc: */
	if (!modest_protocol_info_is_secure(protocol_security_incoming))
	{
		if (gtk_toggle_button_get_active (
				GTK_TOGGLE_BUTTON (self->checkbox_incoming_auth))) {
			GError *error = NULL;

			GList *list_auth_methods = 
				modest_maemo_utils_get_supported_secure_authentication_methods (self->incoming_protocol, 
					hostname, port_num, username, GTK_WINDOW (self), &error);
			if (list_auth_methods) {
				/* Use the first supported method.
				 * TODO: Should we prioritize them, to prefer a particular one? */
				GList* method;
				for (method = list_auth_methods; method != NULL; method = g_list_next(method))
				{
					ModestAuthProtocol proto = (ModestAuthProtocol)(GPOINTER_TO_INT(method->data));
					// Allow secure methods, e.g MD5 only
					if (modest_protocol_info_auth_is_secure(proto))
					{
						self->protocol_authentication_incoming = proto;
						break;
					}
				}
				g_list_free (list_auth_methods);
			}

			if (list_auth_methods == NULL || 
					!modest_protocol_info_auth_is_secure(self->protocol_authentication_incoming))
			{
		  		if(error == NULL || error->domain != modest_maemo_utils_get_supported_secure_authentication_error_quark() ||
						error->code != MODEST_MAEMO_UTILS_GET_SUPPORTED_SECURE_AUTHENTICATION_ERROR_CANCELED)
				{
					show_error (GTK_WIDGET (self), _("Could not discover supported secure authentication methods."));
				}

				if(error != NULL)
					g_error_free(error);
					
				/* This is a nasty hack. jschmid. */
				/* Don't let the dialog close */
				/*g_signal_stop_emission_by_name (dialog, "response");*/
				return FALSE;
			}
		}
	}
	
	return TRUE;
}
/*
 */
static void 
on_response (GtkDialog *wizard_dialog,
	gint response_id,
	gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (wizard_dialog);
	enable_buttons (self);
	
	gboolean prevent_response = FALSE;

	/* Warn about unsaved changes: */
	if (response_id == GTK_RESPONSE_CANCEL && self->modified) {
		GtkDialog *dialog = GTK_DIALOG (hildon_note_new_confirmation (GTK_WINDOW (self), 
			_("imum_nc_wizard_confirm_lose_changes")));
		/* TODO: These button names will be ambiguous, and not specified in the UI specification. */
		 
		 const gint dialog_response = gtk_dialog_run (dialog);
		 gtk_widget_destroy (GTK_WIDGET (dialog));
		 
		if (dialog_response != GTK_RESPONSE_OK)
			prevent_response = TRUE;
	}
	/* Check for invalid input: */
	else if (response_id != GTK_RESPONSE_CANCEL && !check_data (self)) {
		prevent_response = TRUE;
	}
		
	if (prevent_response) {
		/* This is a nasty hack. murrayc. */
		/* Don't let the dialog close */
		g_signal_stop_emission_by_name (wizard_dialog, "response");
		return;	
	}
		
	if (response_id == GTK_RESPONSE_OK) {
		/* Try to save the changes if modified (NB #59251): */
		if (self->modified)
		{
			const gboolean saved = save_configuration (self);
			if (saved) {
				/* Do not show the account-saved dialog if we are just saving this 
				 * temporarily, because from the user's point of view it will not 
				 * really be saved (saved + enabled) until later.
				 */
				const gboolean enabled = 
					modest_account_mgr_get_enabled (self->account_manager, self->account_name);
				if (enabled)
					show_error (NULL, _("mcen_ib_advsetup_settings_saved"));
			}
			else
				show_error (NULL, _("mail_ib_setting_failed"));
		}
	}
}

static void
modest_account_settings_dialog_init (ModestAccountSettingsDialog *self)
{
	/* Create the notebook to be used by the GtkDialog base class:
	 * Each page of the notebook will be a page of the wizard: */
	self->notebook = GTK_NOTEBOOK (gtk_notebook_new());

	/* Get the account manager object, 
	 * so we can check for existing accounts,
	 * and create new accounts: */
	self->account_manager = modest_runtime_get_account_mgr ();
	g_assert (self->account_manager);
	g_object_ref (self->account_manager);
	
	self->protocol_authentication_incoming = MODEST_PROTOCOL_AUTH_PASSWORD;

    /* Create the common pages, 
     */
	self->page_account_details = create_page_account_details (self);
	self->page_user_details = create_page_user_details (self);
	self->page_incoming = create_page_incoming (self);
	self->page_outgoing = create_page_outgoing (self);
	
	/* Add the notebook pages: */
	gtk_notebook_append_page (self->notebook, self->page_account_details, 
		gtk_label_new (_("mcen_ti_account_settings_account")));
	gtk_notebook_append_page (self->notebook, self->page_user_details, 
		gtk_label_new (_("mcen_ti_account_settings_userinfo")));
	gtk_notebook_append_page (self->notebook, self->page_incoming,
		gtk_label_new (_("mcen_ti_advsetup_retrieval")));
	gtk_notebook_append_page (self->notebook, self->page_outgoing,
		gtk_label_new (_("mcen_ti_advsetup_sending")));
		
	GtkDialog *dialog = GTK_DIALOG (self);
	gtk_container_add (GTK_CONTAINER (dialog->vbox), GTK_WIDGET (self->notebook));
	gtk_container_set_border_width (GTK_CONTAINER (dialog->vbox), MODEST_MARGIN_HALF);
	gtk_widget_show (GTK_WIDGET (self->notebook));
        
    /* Add the buttons: */
    gtk_dialog_add_button (GTK_DIALOG(self), _("mcen_bd_dialog_ok"), GTK_RESPONSE_OK);
    gtk_dialog_add_button (GTK_DIALOG(self), _("mcen_bd_dialog_cancel"), GTK_RESPONSE_CANCEL);
    
    /* Connect to the dialog's response signal: */
    /* We use connect-before 
     * so we can stop the signal emission, 
     * to stop the default signal handler from closing the dialog.
     */
    g_signal_connect (G_OBJECT (self), "response",
            G_CALLBACK (on_response), self); 
            
    self->modified = FALSE;
    
    /* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
    modest_window_mgr_prevent_hibernation_while_window_is_shown (
    	modest_runtime_get_window_mgr (), GTK_WINDOW (self)); 
}

ModestAccountSettingsDialog*
modest_account_settings_dialog_new (void)
{
	return g_object_new (MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, NULL);
}

/** Update the UI with the stored account details, so they can be edited.
 * @account_name: Name of the account, which should contain incoming and outgoing server accounts.
 */
void modest_account_settings_dialog_set_account_name (ModestAccountSettingsDialog *dialog, const gchar* account_name)
{
	if (!account_name)
		return;
		
	/* Save the account name so we can refer to it later: */
	if (dialog->account_name)
		g_free (dialog->account_name);
	dialog->account_name = g_strdup (account_name);
	
		
	/* Get the account data for this account name: */
	ModestAccountData *account_data = modest_account_mgr_get_account_data (dialog->account_manager, 
		account_name);
	if (!account_data) {
		g_printerr ("modest: failed to get account data for %s\n", account_name);
		return;
	}
	
	/* Save the account title so we can refer to it if the user changes it: */
	if (dialog->original_account_title)
		g_free (dialog->original_account_title);
	dialog->original_account_title = g_strdup (account_data->display_name);
	

	if (!(account_data->store_account)) {
		g_printerr ("modest: account has no stores: %s\n", account_name);
		return;
	}
		
	/* Show the account data in the widgets: */
	
	/* Note that we never show the non-display name in the UI.
	 * (Though the display name defaults to the non-display name at the start.) */
	gtk_entry_set_text( GTK_ENTRY (dialog->entry_account_title),
		account_data->display_name ? account_data->display_name : "");
		
	gtk_entry_set_text( GTK_ENTRY (dialog->entry_user_name), 
		account_data->fullname ? account_data->fullname : "");
	gtk_entry_set_text( GTK_ENTRY (dialog->entry_user_email), 
		account_data->email ? account_data->email : "");
		
	ModestServerAccountData *incoming_account = account_data->store_account;
		
	if (incoming_account)
		modest_retrieve_combo_box_fill (MODEST_RETRIEVE_COMBO_BOX (dialog->combo_retrieve), incoming_account->proto);
	gchar *retrieve = modest_account_mgr_get_string (dialog->account_manager, account_name,
		MODEST_ACCOUNT_RETRIEVE, FALSE /* not server account */);
	if (!retrieve) {
		/* Default to something, though no default is specified in the UI spec: */
		retrieve = g_strdup (MODEST_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY);
	}
	modest_retrieve_combo_box_set_active_retrieve_conf (MODEST_RETRIEVE_COMBO_BOX (dialog->combo_retrieve), retrieve);
	g_free (retrieve);
	
	const gint limit_retrieve = modest_account_mgr_get_int (dialog->account_manager, account_name,
		MODEST_ACCOUNT_LIMIT_RETRIEVE, FALSE /* not server account */);
	modest_limit_retrieve_combo_box_set_active_limit_retrieve (MODEST_LIMIT_RETRIEVE_COMBO_BOX (dialog->combo_limit_retrieve), limit_retrieve);
	
	
	const gboolean leave_on_server = modest_account_mgr_get_bool (dialog->account_manager, account_name,
		MODEST_ACCOUNT_LEAVE_ON_SERVER, FALSE /* not server account */);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->checkbox_leave_messages), leave_on_server);	
	
	/* Only show the leave-on-server checkbox for POP, 
	 * as per the UI spec: */
	if (incoming_account->proto != MODEST_PROTOCOL_STORE_POP) {
		gtk_widget_hide (dialog->caption_leave_messages);
	} else {
		gtk_widget_show (dialog->caption_leave_messages);
	}
	
	update_incoming_server_security_choices (dialog, incoming_account->proto);
	if (incoming_account) {
		/* Remember this for later: */
		dialog->incoming_protocol = incoming_account->proto;
		
		gtk_entry_set_text( GTK_ENTRY (dialog->entry_user_username),
			incoming_account->username ? incoming_account->username : "");
		gtk_entry_set_text( GTK_ENTRY (dialog->entry_user_password), 
			incoming_account->password ? incoming_account->password : "");
			
		gtk_entry_set_text( GTK_ENTRY (dialog->entry_incomingserver), 
			incoming_account->hostname ? incoming_account->hostname : "");
			
		/* The UI spec says:
		 * If secure authentication is unchecked, allow sending username and password also as plain text.
    	 * If secure authentication is checked, require one of the secure methods during connection: SSL, TLS, CRAM-MD5 etc. 
	  	 * TODO: Do we need to discover which of these (SSL, TLS, CRAM-MD5) is supported?
         */														 
		const ModestConnectionProtocol security = modest_server_account_get_security (
			dialog->account_manager, incoming_account->account_name);
		modest_serversecurity_combo_box_set_active_serversecurity (
			MODEST_SERVERSECURITY_COMBO_BOX (dialog->combo_incoming_security), security);
		
		/* Check if we have
		 - a secure protocol
		 OR
		 - use encrypted passwords
		*/
		const ModestAuthProtocol secure_auth = modest_server_account_get_secure_auth(
			dialog->account_manager, incoming_account->account_name);
		dialog->protocol_authentication_incoming = secure_auth;
		if (modest_protocol_info_is_secure(security) || 
				modest_protocol_info_auth_is_secure(secure_auth))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dialog->checkbox_incoming_auth), 
																	 TRUE);
		}
		else
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (dialog->checkbox_incoming_auth), 
																	 FALSE);
		};
					
		update_incoming_server_title (dialog, incoming_account->proto);
		
		const gint port_num = modest_account_mgr_get_int (dialog->account_manager, incoming_account->account_name,
			MODEST_ACCOUNT_PORT, TRUE /* server account */);
			
		if (port_num == 0) {
			/* Show the appropriate port number: */
			on_combo_incoming_security_changed (
				GTK_COMBO_BOX (dialog->combo_incoming_security), dialog);
		} else {
			/* Keep the user-entered port-number,
			 * or the already-appropriate automatic port number: */
			hildon_number_editor_set_value (
				HILDON_NUMBER_EDITOR (dialog->entry_incoming_port), port_num);
		}
	}
	
	ModestServerAccountData *outgoing_account = account_data->transport_account;
	if (outgoing_account) {
		/* Remember this for later: */
		dialog->outgoing_protocol = outgoing_account->proto;
		
		gtk_entry_set_text( GTK_ENTRY (dialog->entry_outgoingserver), 
			outgoing_account->hostname ? outgoing_account->hostname : "");
		
		gtk_entry_set_text( GTK_ENTRY (dialog->entry_outgoing_username), 
			outgoing_account->username ? outgoing_account->username : "");
		gtk_entry_set_text( GTK_ENTRY (dialog->entry_outgoing_password), 
			outgoing_account->password ? outgoing_account->password : "");
		
		/* Get the secure-auth setting: */
		const ModestAuthProtocol secure_auth = modest_server_account_get_secure_auth(
			dialog->account_manager, outgoing_account->account_name);
		modest_secureauth_combo_box_set_active_secureauth (
			MODEST_SECUREAUTH_COMBO_BOX (dialog->combo_outgoing_auth), secure_auth);
		on_combo_outgoing_auth_changed (GTK_COMBO_BOX (dialog->combo_outgoing_auth), dialog);
		
		modest_serversecurity_combo_box_fill (
			MODEST_SERVERSECURITY_COMBO_BOX (dialog->combo_outgoing_security), outgoing_account->proto);
		
		/* Get the security setting: */
		const ModestConnectionProtocol security = modest_server_account_get_security (
			dialog->account_manager, outgoing_account->account_name);
		modest_serversecurity_combo_box_set_active_serversecurity (
			MODEST_SERVERSECURITY_COMBO_BOX (dialog->combo_outgoing_security), security);
		
		const gint port_num = modest_account_mgr_get_int (dialog->account_manager, outgoing_account->account_name,
			MODEST_ACCOUNT_PORT, TRUE /* server account */);
		if (port_num == 0) {
			/* Show the appropriate port number: */
			on_combo_outgoing_security_changed (
				GTK_COMBO_BOX (dialog->combo_outgoing_security), dialog);
		}
		else {
			/* Keep the user-entered port-number,
			 * or the already-appropriate automatic port number: */
			hildon_number_editor_set_value (
				HILDON_NUMBER_EDITOR (dialog->entry_outgoing_port), port_num);
		}
		
		const gboolean has_specific = 
			modest_account_mgr_get_use_connection_specific_smtp (
				dialog->account_manager, 
				account_name);
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dialog->checkbox_outgoing_smtp_specific), 
			has_specific);
	}

	/* Set window title according to account: */
	/* TODO: Is this the correct way to find a human-readable name for
	 * the protocol used? */
	const gchar* proto_str = modest_protocol_info_get_transport_store_protocol_name (dialog->incoming_protocol);
	gchar *proto_name = g_utf8_strup(proto_str, -1);
	gchar *account_title = modest_account_mgr_get_display_name(dialog->account_manager, account_name);

	gchar *title = g_strdup_printf(_("mcen_ti_account_settings"), proto_name, account_title);
	g_free (proto_name);
	g_free (account_title);

	gtk_window_set_title (GTK_WINDOW (dialog), title);
	g_free (title);

	/* account_data->is_enabled,  */
	/*account_data->is_default,  */

	/* account_data->store_account->proto */

	modest_account_mgr_free_account_data (dialog->account_manager, account_data);
	
	/* Unset the modified flag so we can detect changes later: */
	dialog->modified = FALSE;
}

/** Show the User Info tab.
 */
void modest_account_settings_dialog_switch_to_user_info (ModestAccountSettingsDialog *dialog)
{
	const gint page_num = gtk_notebook_page_num (dialog->notebook, dialog->page_user_details);
	if (page_num == -1) {
		g_warning ("%s: notebook page not found.\n", __FUNCTION__);	
	}
		
	/* Ensure that the widget is visible so that gtk_notebook_set_current_page() works: */
	/* TODO: even this hack (recommened by the GTK+ documentation) doesn't seem to work. */
	
	gtk_widget_show (dialog->page_user_details);
	gtk_widget_show (GTK_WIDGET (dialog->notebook));
	gtk_widget_show (GTK_WIDGET (dialog));
	gtk_notebook_set_current_page (dialog->notebook, page_num);
}

static gboolean
save_configuration (ModestAccountSettingsDialog *dialog)
{
	g_assert (dialog->account_name);
	
	const gchar* account_name = dialog->account_name;
		
	/* Set the account data from the widgets: */
	const gchar* user_name = gtk_entry_get_text (GTK_ENTRY (dialog->entry_user_name));
	gboolean test = modest_account_mgr_set_string (dialog->account_manager, account_name,
		MODEST_ACCOUNT_FULLNAME, user_name, FALSE /* not server account */);
	if (!test)
		return FALSE;
		
	const gchar* emailaddress = gtk_entry_get_text (GTK_ENTRY (dialog->entry_user_email));
	test = modest_account_mgr_set_string (dialog->account_manager, account_name,
		MODEST_ACCOUNT_EMAIL, emailaddress, FALSE /* not server account */);
	if (!test)
		return FALSE;
		
	/* Signature: */
	if (dialog->signature_dialog) {
		gboolean use_signature = FALSE;
    	gchar *signature = modest_signature_editor_dialog_get_settings (
    		MODEST_SIGNATURE_EDITOR_DIALOG (dialog->signature_dialog),
    		&use_signature);
    	
    	modest_account_mgr_set_signature(dialog->account_manager, account_name, 
    		signature, use_signature);
    	g_free (signature);
    }
	
	gchar *retrieve = modest_retrieve_combo_box_get_active_retrieve_conf (
		MODEST_RETRIEVE_COMBO_BOX (dialog->combo_retrieve));
	modest_account_mgr_set_string (dialog->account_manager, account_name,
		MODEST_ACCOUNT_RETRIEVE, retrieve, FALSE /* not server account */);
	g_free (retrieve);
	
	const gint limit_retrieve = modest_limit_retrieve_combo_box_get_active_limit_retrieve (
		MODEST_LIMIT_RETRIEVE_COMBO_BOX (dialog->combo_limit_retrieve));
	modest_account_mgr_set_int (dialog->account_manager, account_name,
		MODEST_ACCOUNT_LIMIT_RETRIEVE, limit_retrieve, FALSE /* not server account */);
	
	const gboolean leave_on_server = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->checkbox_leave_messages));
	test = modest_account_mgr_set_bool (dialog->account_manager, account_name,
		MODEST_ACCOUNT_LEAVE_ON_SERVER, leave_on_server, FALSE /* not server account */);
	if (!test)
		return FALSE;
			
	/* Incoming: */
	gchar* incoming_account_name = modest_account_mgr_get_string (dialog->account_manager, account_name,
		MODEST_ACCOUNT_STORE_ACCOUNT, FALSE /* not server account */);
	g_assert (incoming_account_name);
	
	const gchar* hostname = gtk_entry_get_text (GTK_ENTRY (dialog->entry_incomingserver));
	test = modest_account_mgr_set_string (dialog->account_manager, incoming_account_name,
		MODEST_ACCOUNT_HOSTNAME, hostname, TRUE /* server account */);
	if (!test)
		return FALSE;
				
	const gchar* username = gtk_entry_get_text (GTK_ENTRY (dialog->entry_user_username));
	modest_server_account_set_username (dialog->account_manager, incoming_account_name, username);
	
	const gchar* password = gtk_entry_get_text (GTK_ENTRY (dialog->entry_user_password));
	modest_server_account_set_password (dialog->account_manager, incoming_account_name, password);
			
	/* port: */
	gint port_num = hildon_number_editor_get_value (
			HILDON_NUMBER_EDITOR (dialog->entry_incoming_port));
	modest_account_mgr_set_int (dialog->account_manager, incoming_account_name,
			MODEST_ACCOUNT_PORT, port_num, TRUE /* server account */);
			
	/* The UI spec says:
	 * If secure authentication is unchecked, allow sending username and password also as plain text.
	 * If secure authentication is checked, require one of the secure methods during connection: SSL, TLS, CRAM-MD5 etc. 
	 */
	
	const ModestConnectionProtocol protocol_security_incoming = modest_serversecurity_combo_box_get_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (dialog->combo_incoming_security));
	modest_server_account_set_security (dialog->account_manager, incoming_account_name, protocol_security_incoming);
	
	modest_server_account_set_secure_auth (dialog->account_manager, incoming_account_name, dialog->protocol_authentication_incoming);
	
		
	g_free (incoming_account_name);
	
	/* Outgoing: */
	gchar* outgoing_account_name = modest_account_mgr_get_string (dialog->account_manager, account_name,
		MODEST_ACCOUNT_TRANSPORT_ACCOUNT, FALSE /* not server account */);
	g_assert (outgoing_account_name);
	
	hostname = gtk_entry_get_text (GTK_ENTRY (dialog->entry_outgoingserver));
	test = modest_account_mgr_set_string (dialog->account_manager, outgoing_account_name,
		MODEST_ACCOUNT_HOSTNAME, hostname, TRUE /* server account */);
	if (!test)
		return FALSE;
		
	username = gtk_entry_get_text (GTK_ENTRY (dialog->entry_outgoing_username));
	modest_server_account_set_username (dialog->account_manager, outgoing_account_name,
		username);
		
	password = gtk_entry_get_text (GTK_ENTRY (dialog->entry_outgoing_password));
	modest_server_account_set_password (dialog->account_manager, outgoing_account_name,
		password);
	
	const ModestConnectionProtocol protocol_security_outgoing = modest_serversecurity_combo_box_get_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (dialog->combo_outgoing_security));
	modest_server_account_set_security (dialog->account_manager, outgoing_account_name, protocol_security_outgoing);
	
	const ModestAuthProtocol protocol_authentication_outgoing = modest_secureauth_combo_box_get_active_secureauth (
		MODEST_SECUREAUTH_COMBO_BOX (dialog->combo_outgoing_auth));
	modest_server_account_set_secure_auth (dialog->account_manager, outgoing_account_name, protocol_authentication_outgoing);	
	
	/* port: */
	port_num = hildon_number_editor_get_value (
			HILDON_NUMBER_EDITOR (dialog->entry_outgoing_port));
	modest_account_mgr_set_int (dialog->account_manager, outgoing_account_name,
			MODEST_ACCOUNT_PORT, port_num, TRUE /* server account */);
			
	g_free (outgoing_account_name);
	
	
	/* Set the changed account title last, to simplify the previous code: */
	gchar* account_title = get_entered_account_title (dialog);
	if (!account_title)
		return FALSE; /* Should be prevented already anyway. */
		
	if (strcmp(account_title, account_name) != 0) {
		/* Change the title: */
		const gboolean test = modest_account_mgr_set_string (dialog->account_manager, account_name,
		MODEST_ACCOUNT_DISPLAY_NAME, account_title, FALSE /* not server account */);
		if (!test) {
			g_free (account_title);
			return FALSE;
		}
	}
	
	g_free (account_title);
	account_title = NULL;
	
	/* Save connection-specific SMTP server accounts: */
        modest_account_mgr_set_use_connection_specific_smtp(dialog->account_manager, account_name,
               gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->checkbox_outgoing_smtp_specific)));
	if (dialog->specific_window) {
		return modest_connection_specific_smtp_window_save_server_accounts (
			MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (dialog->specific_window), account_name);
	}
	else
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
enable_buttons (ModestAccountSettingsDialog *self)
{
	gboolean enable_ok = TRUE;
	
	/* The account details title is mandatory: */
	if (entry_is_empty(self->entry_account_title))
			enable_ok = FALSE;

	/* The user details username is mandatory: */
	if (entry_is_empty(self->entry_user_username))
		enable_ok = FALSE;
		
	/* The user details email address is mandatory: */
	if (enable_ok && entry_is_empty (self->entry_user_email))
		enable_ok = FALSE;

	/* The custom incoming server is mandatory: */
	if (entry_is_empty(self->entry_incomingserver))
		enable_ok = FALSE;
			
	/* Enable the buttons, 
	 * identifying them via their associated response codes:
	 */
	GtkDialog *dialog_base = GTK_DIALOG (self);
    gtk_dialog_set_response_sensitive (dialog_base,
                                       GTK_RESPONSE_OK,
                                       enable_ok);
}

static void
modest_account_settings_dialog_class_init (ModestAccountSettingsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (ModestAccountSettingsDialogPrivate));


	object_class->get_property = modest_account_settings_dialog_get_property;
	object_class->set_property = modest_account_settings_dialog_set_property;
	object_class->dispose = modest_account_settings_dialog_dispose;
	object_class->finalize = modest_account_settings_dialog_finalize;
}
 
static void
show_error (GtkWidget *parent_widget, const gchar* text)
{
	hildon_banner_show_information(parent_widget, NULL, text);
	
#if 0
	GtkDialog *dialog = GTK_DIALOG (hildon_note_new_information (parent_widget, text);
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

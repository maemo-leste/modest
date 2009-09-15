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


#include "widgets/modest-account-settings-dialog.h"
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

#include "widgets/modest-serversecurity-combo-box.h"
#include "widgets/modest-secureauth-combo-box.h"
#include "widgets/modest-validating-entry.h"
#include "widgets/modest-retrieve-combo-box.h"
#include "widgets/modest-limit-retrieve-combo-box.h"
#include "modest-defs.h"
#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-account-mgr-helpers.h" /* For modest_account_mgr_get_account_data(). */
#include "modest-runtime.h" /* For modest_runtime_get_account_mgr(). */
#include "modest-protocol-registry.h"
#include <modest-utils.h>
#include <modest-platform.h>
#include "widgets/modest-ui-constants.h"
#include "widgets/modest-default-account-settings-dialog.h"
#include <tny-account.h>
#include <tny-status.h>
#include <tny-simple-list.h>

#include <gconf/gconf-client.h>
#include <string.h> /* For strlen(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define PORT_MIN 1
#define PORT_MAX 65535

static void modest_account_settings_dialog_init (gpointer g, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (ModestDefaultAccountSettingsDialog, 
                        modest_default_account_settings_dialog, 
                        GTK_TYPE_DIALOG,
                        0, 
                        G_IMPLEMENT_INTERFACE (MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, 
                                               modest_account_settings_dialog_init));

#define MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				      MODEST_TYPE_DEFAULT_ACCOUNT_SETTINGS_DIALOG, \
				      ModestDefaultAccountSettingsDialogPrivate))

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
	
	GtkNotebook *notebook;
	
	GtkWidget *page_account_details;
	GtkWidget *entry_account_title;
	GtkWidget *combo_retrieve;
	GtkWidget *combo_limit_retrieve;
	GtkWidget *caption_leave_messages;
	GtkWidget *checkbox_leave_messages;
	
	GtkWidget *page_user_details;
	GtkWidget *entry_user_name;
	GtkWidget *entry_user_username;
	GtkWidget *entry_user_password;
	GtkWidget *entry_user_email;
	GtkWidget *entry_incoming_port;
	GtkWidget *button_signature;
	
	GtkWidget *page_complete_easysetup;
	
	GtkWidget *page_incoming;
	GtkWidget *caption_incoming;
	GtkWidget *entry_incomingserver;
	GtkWidget *combo_incoming_security;
	GtkWidget *checkbox_incoming_auth;

	GtkWidget *page_outgoing;
	GtkWidget *entry_outgoingserver;
	GtkWidget *caption_outgoing_username;
	GtkWidget *entry_outgoing_username;
	GtkWidget *caption_outgoing_password;
	GtkWidget *entry_outgoing_password;
	GtkWidget *combo_outgoing_security;
	GtkWidget *combo_outgoing_auth;
	GtkWidget *entry_outgoing_port;
	GtkWidget *checkbox_outgoing_smtp_specific;
	GtkWidget *button_outgoing_smtp_servers;
	
	GtkWidget *signature_dialog;
};

static void enable_buttons (ModestAccountSettingsDialog *self);
static gboolean save_configuration (ModestAccountSettingsDialog *dialog);
static void modest_default_account_settings_dialog_load_settings (ModestAccountSettingsDialog *dialog, 
								  ModestAccountSettings *settings);

static const gchar *
null_means_empty (const gchar * str)
{
	return str ? str : "";
}


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
	if (G_OBJECT_CLASS (modest_default_account_settings_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (modest_default_account_settings_dialog_parent_class)->dispose (object);
}

static void
modest_account_settings_dialog_finalize (GObject *object)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (object);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	
	if (priv->account_name)
		g_free (priv->account_name);
		
	if (priv->original_account_title)
		g_free (priv->original_account_title);
		
	if (priv->account_manager)
		g_object_unref (G_OBJECT (priv->account_manager));
		
	if (priv->signature_dialog)
		gtk_widget_destroy (priv->signature_dialog);

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}
	
	G_OBJECT_CLASS (modest_default_account_settings_dialog_parent_class)->finalize (object);
}

static void
on_combo_incoming_security_changed (GtkComboBox *widget, gpointer user_data);

static void
on_combo_outgoing_security_changed (GtkComboBox *widget, gpointer user_data);

static void
on_modified_combobox_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	priv->modified = TRUE;
}

static void
on_modified_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	priv->modified = TRUE;
}

static void
on_modified_checkbox_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	priv->modified = TRUE;
}

static void
on_modified_spin_button_changed (GtkSpinButton *spin_button, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	gint value = gtk_spin_button_get_value_as_int (spin_button);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_OK, value > 0);
	priv->modified = TRUE;
}

/* Set a modified boolean whenever the widget is changed, 
 * so we can check for it later.
 */
static void
connect_for_modified (ModestDefaultAccountSettingsDialog *self, 
		      GtkWidget *widget)
{
	if (GTK_SPIN_BUTTON (widget)) {
		g_signal_connect (G_OBJECT (widget), "value_changed",
			G_CALLBACK (on_modified_spin_button_changed), self);
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
on_field_entry_changed (GtkEditable *editable, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	g_assert(self);
	enable_buttons(self);
}

static void
on_field_combobox_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	g_assert(self);
	enable_buttons(self);
}

/** This is a convenience function to create a field containing a mandatory widget.
 * When the widget is edited, the enable_buttons() vfunc will be called.
 */
static GtkWidget* 
create_field(ModestDefaultAccountSettingsDialog *self,
	     GtkSizeGroup *group,
	     const gchar *value,
	     GtkWidget *control,
	     GtkWidget *icon,
	     gboolean mandatory)
{
	GtkWidget *hbox;
	gchar *title;
	GtkWidget *label;

	hbox = gtk_hbox_new (FALSE, 12);
	if (mandatory)
		title = g_strdup_printf("%s*:", value);
	else
		title = g_strdup_printf ("%s:", value);
	label = gtk_label_new (title);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	g_free (title);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), control, TRUE, TRUE, 0);
	gtk_size_group_add_widget (group, label);
  
	/* Connect to the appropriate changed signal for the widget, 
	 * so we can ask for the prev/next buttons to be enabled/disabled appropriately:
	 */
	if (GTK_IS_ENTRY (control)) {
		g_signal_connect (G_OBJECT (control), "changed",
        	G_CALLBACK (on_field_entry_changed), self);
		
	}
	else if (GTK_IS_COMBO_BOX (control)) {
		g_signal_connect (G_OBJECT (control), "changed",
        	G_CALLBACK (on_field_combobox_changed), self);
	}
	gtk_widget_show_all (hbox);
	 
	return hbox;
}

static void
on_entry_invalid_account_title_character (ModestValidatingEntry *self, const gchar* character, gpointer user_data)
{
	gchar *tmp, *msg;
			
	tmp = g_strndup (account_title_forbidden_chars, ACCOUNT_TITLE_FORBIDDEN_CHARS_LENGTH);
	msg = g_strdup_printf (_CS("ckdg_ib_illegal_characters_entered"), tmp);

	modest_platform_information_banner (GTK_WIDGET (self), NULL, msg);

	g_free (msg);
	g_free (tmp);
}

static void
on_entry_invalid_fullname_character (ModestValidatingEntry *self, const gchar* character, gpointer user_data)
{
	gchar *tmp, *msg;
			
	tmp = g_strndup (user_name_forbidden_chars, USER_NAME_FORBIDDEN_CHARS_LENGTH);
	msg = g_strdup_printf (_CS("ckdg_ib_illegal_characters_entered"), tmp);

	modest_platform_information_banner(GTK_WIDGET (self), NULL, msg);

	g_free (msg);
	g_free (tmp);
}


static void
on_entry_max (ModestValidatingEntry *self, gpointer user_data)
{
	modest_platform_information_banner(GTK_WIDGET (self), NULL, 
					 _CS("ckdg_ib_maximum_characters_reached"));
}

static GtkWidget*
create_page_account_details (ModestDefaultAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *alignment;	
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* The description widgets: */	
	priv->entry_account_title = GTK_WIDGET (modest_validating_entry_new ());
	GtkWidget *field = create_field (self, sizegroup, _("mcen_fi_account_title"), 
		priv->entry_account_title, NULL, TRUE);
	gtk_widget_show (priv->entry_account_title);
	connect_for_modified (self, priv->entry_account_title);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
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
	
	/* The retrieve combobox: */
	priv->combo_retrieve = GTK_WIDGET (modest_retrieve_combo_box_new ());
	field = create_field (self, sizegroup, _("mcen_fi_advsetup_retrievetype"), 
			      priv->combo_retrieve, NULL, TRUE);
	gtk_widget_show (priv->combo_retrieve);
	connect_for_modified (self, priv->combo_retrieve);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* The limit-retrieve combobox: */
	priv->combo_limit_retrieve = GTK_WIDGET (modest_limit_retrieve_combo_box_new ());
	field = create_field (self, sizegroup, _("mcen_fi_advsetup_limit_retrieve"), 
			      priv->combo_limit_retrieve, NULL, TRUE);
	gtk_widget_show (priv->combo_limit_retrieve);
	connect_for_modified (self, priv->combo_limit_retrieve);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);

	/* The leave-messages widgets: */
	if(!priv->checkbox_leave_messages)
		priv->checkbox_leave_messages = gtk_check_button_new ();
	if (!priv->caption_leave_messages) {
		priv->caption_leave_messages = create_field (self, sizegroup, _("mcen_fi_advsetup_leave_on_server"), 
							     priv->checkbox_leave_messages, NULL, TRUE);
	}
			
	gtk_widget_show (priv->checkbox_leave_messages);
	connect_for_modified (self, priv->checkbox_leave_messages);
	gtk_box_pack_start (GTK_BOX (box), priv->caption_leave_messages, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->caption_leave_messages);
	
	gtk_widget_show (GTK_WIDGET (box));

	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 12, 12, 12);
	gtk_container_add (GTK_CONTAINER (alignment), box);
	gtk_widget_show (alignment);
	
	return GTK_WIDGET (alignment);
}

static gchar*
get_entered_account_title (ModestAccountSettingsDialog *dialog)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);

	const gchar* account_title = 
		gtk_entry_get_text (GTK_ENTRY (priv->entry_account_title));
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
/* 	ModestAccountSettingsDialog * self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data); */
	
/* 	/\* Create the window, if necessary: *\/ */
/* 	if (!(priv->signature_dialog)) { */
/* 		priv->signature_dialog = GTK_WIDGET (modest_signature_editor_dialog_new ()); */
	
/* 		gboolean use_signature = modest_account_settings_get_use_signature (priv->settings); */
/* 		const gchar *signature = modest_account_settings_get_signature(priv->settings); */
/* 		gchar* account_title = get_entered_account_title (self); */
/* 		modest_signature_editor_dialog_set_settings ( */
/* 			MODEST_SIGNATURE_EDITOR_DIALOG (priv->signature_dialog),  */
/* 			use_signature, signature, account_title); */

/* 		g_free (account_title); */
/* 		account_title = NULL; */
/* 		signature = NULL; */
/* 	} */

/* 	/\* Show the window: *\/	 */
/* 	gtk_window_set_transient_for (GTK_WINDOW (priv->signature_dialog), GTK_WINDOW (self)); */
/* 	gtk_window_set_modal (GTK_WINDOW (priv->signature_dialog), TRUE); */
/*     const gint response = gtk_dialog_run (GTK_DIALOG (priv->signature_dialog)); */
/*     gtk_widget_hide (priv->signature_dialog); */
/*     if (response != GTK_RESPONSE_OK) { */
/*     	/\* Destroy the widget now, and its data: *\/ */
/*     	gtk_widget_destroy (priv->signature_dialog); */
/*     	priv->signature_dialog = NULL; */
/*     } */
/*     else { */
/*     	/\* Mark modified, so we use the dialog's data later: *\/ */
/*     	priv->modified = TRUE;	 */
/*     } */
}

static GtkWidget*
create_page_user_details (ModestDefaultAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *alignment;	
	GtkSizeGroup* sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	 
	/* The name widgets: */
	priv->entry_user_name = GTK_WIDGET (modest_validating_entry_new ());

	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_name), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_user_name), 
		on_entry_max, self);
	GtkWidget *field = create_field (self, sizegroup, 
					 _("mcen_li_emailsetup_name"), priv->entry_user_name, NULL, FALSE);
	gtk_widget_show (priv->entry_user_name);
	connect_for_modified (self, priv->entry_user_name);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);


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
	field = create_field (self, sizegroup, _("mail_fi_username"), 
				priv->entry_user_username, NULL, TRUE);
	gtk_widget_show (priv->entry_user_username);
	connect_for_modified (self, priv->entry_user_username);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	modest_validating_entry_set_unallowed_characters_whitespace (
	 	MODEST_VALIDATING_ENTRY (priv->entry_user_username));
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_username), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_user_username), 
		on_entry_max, self);
	
	/* The password widgets: */	
	priv->entry_user_password = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (priv->entry_user_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (priv->entry_user_password), '*'); */
	field = create_field (self, sizegroup, 
			      _("mail_fi_password"), priv->entry_user_password, NULL, FALSE);
	gtk_widget_show (priv->entry_user_password);
	connect_for_modified (self, priv->entry_user_password);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* The email address widgets: */	
	priv->entry_user_email = GTK_WIDGET (modest_validating_entry_new ());
	field = create_field (self, sizegroup, 
			      _("mcen_li_emailsetup_email_address"), priv->entry_user_email, NULL, TRUE);
	gtk_entry_set_text (GTK_ENTRY (priv->entry_user_email), MODEST_EXAMPLE_EMAIL_ADDRESS); /* Default text. */
	gtk_widget_show (priv->entry_user_email);
	connect_for_modified (self, priv->entry_user_email);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_email), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_user_email), 
		on_entry_max, self);
	
	/* Signature button: */
	if (!priv->button_signature)
		priv->button_signature = gtk_button_new_with_label (_("mcen_bd_edit"));
	field = create_field (self, sizegroup, _("mcen_fi_email_signature"), 
		priv->button_signature, NULL, FALSE);
	gtk_widget_show (priv->button_signature);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
		
	g_signal_connect (G_OBJECT (priv->button_signature), "clicked",
        	G_CALLBACK (on_button_signature), self);
        	
	gtk_widget_show (GTK_WIDGET (box));

	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 12, 12, 12);
	gtk_container_add (GTK_CONTAINER (alignment), box);
	gtk_widget_show (alignment);
	
	return GTK_WIDGET (alignment);
}

/** Change the field title for the incoming server, 
 * as specified in the UI spec:
 */
static void 
update_incoming_server_title (ModestAccountSettingsDialog *self, 
			      ModestProtocolType protocol)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	const gchar* type = 
		(protocol == MODEST_PROTOCOLS_STORE_POP ? 
			_("mail_fi_emailtype_pop3") : 
			_("mail_fi_emailtype_imap") );
			
		
	/* Note that this produces a compiler warning, 
	 * because the compiler does not know that the translated string will have a %s in it.
	 * I do not see a way to avoid the warning while still using these Logical IDs. murrayc. */
	gchar* incomingserver_title = g_strdup_printf(_("mcen_li_emailsetup_servertype"), type);
	
	/* This is a mandatory field, so add a *. This is usually done by 
	 * create_field() but we can't use that here. */
	gchar *with_asterisk = g_strconcat (incomingserver_title, "*", NULL);
	g_free (incomingserver_title);
	
	g_object_set (G_OBJECT (priv->caption_incoming), "label", with_asterisk, NULL);
	g_free(with_asterisk);
}

/** Change the field title for the incoming server, 
 * as specified in the UI spec:
 */
static void 
update_incoming_server_security_choices (ModestAccountSettingsDialog *self, 
					 ModestProtocolType protocol)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Fill the combo with appropriately titled choices for POP or IMAP. */
	/* The choices are the same, but the titles are different, as in the UI spec. */
	modest_serversecurity_combo_box_fill (
		MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_incoming_security), protocol);
}
           
static GtkWidget* 
create_page_incoming (ModestDefaultAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *alignment;
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	 
	/* The incoming server widgets: */
	if(!priv->entry_incomingserver)
		priv->entry_incomingserver = gtk_entry_new ();

	if (priv->caption_incoming)
	  gtk_widget_destroy (priv->caption_incoming);
	   
	/* The field title will be updated in update_incoming_server_title().
	 * so this default text will never be seen: */
	/* (Note: Changing the title seems pointless. murrayc) */
	priv->caption_incoming = create_field (self, sizegroup, 
					     "Incoming Server", priv->entry_incomingserver, NULL, TRUE);
	gtk_widget_show (priv->entry_incomingserver);
	connect_for_modified (self, priv->entry_incomingserver);
	gtk_box_pack_start (GTK_BOX (box), priv->caption_incoming, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->caption_incoming);
	
	/* The secure connection widgets: */
	/* This will be filled by update_incoming_server_security_choices(). */
	if (!priv->combo_incoming_security)
		priv->combo_incoming_security = GTK_WIDGET (modest_serversecurity_combo_box_new ());
	GtkWidget *field = create_field (self, sizegroup, _("mcen_li_emailsetup_secure_connection"), 
					 priv->combo_incoming_security, NULL, FALSE);
	gtk_widget_show (priv->combo_incoming_security);
	connect_for_modified (self, priv->combo_incoming_security);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* Show a default port number when the security method changes, as per the UI spec: */
	g_signal_connect (G_OBJECT (priv->combo_incoming_security), "changed", (GCallback)on_combo_incoming_security_changed, self);
	
	
	/* The port widgets: */
	if (!priv->entry_incoming_port)
		priv->entry_incoming_port = GTK_WIDGET (gtk_spin_button_new_with_range ((gdouble) PORT_MIN, (gdouble) PORT_MAX, 1.0));
	field = create_field (self, sizegroup, _("mcen_fi_emailsetup_port"), 
		priv->entry_incoming_port, NULL, FALSE);
	gtk_widget_show (priv->entry_incoming_port);
	connect_for_modified (self, priv->entry_incoming_port);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* The secure authentication widgets: */
	if(!priv->checkbox_incoming_auth)
		priv->checkbox_incoming_auth = gtk_check_button_new ();
	field = create_field (self, sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
			      priv->checkbox_incoming_auth, NULL, FALSE);
	gtk_widget_show (priv->checkbox_incoming_auth);
	connect_for_modified (self, priv->checkbox_incoming_auth);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	gtk_widget_show (GTK_WIDGET (box));
	
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 12, 12, 12);
	gtk_container_add (GTK_CONTAINER (alignment), box);
	gtk_widget_show (alignment);
	
	return GTK_WIDGET (alignment);
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
/* 	ModestAccountSettingsDialog * self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data); */
	
/* 	/\* Create the window if necessary: *\/ */
/* 	if (!(priv->specific_window)) { */
/* 		priv->specific_window = GTK_WIDGET (modest_connection_specific_smtp_window_new ()); */
/* 		modest_connection_specific_smtp_window_fill_with_connections ( */
/* 			MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (priv->specific_window), priv->account_manager); */
/* 	} */

/* 	/\* Show the window: *\/	 */
/* 	gtk_window_set_transient_for (GTK_WINDOW (priv->specific_window), GTK_WINDOW (self)); */
/* 	gtk_window_set_modal (GTK_WINDOW (priv->specific_window), TRUE); */
/* 	gtk_widget_show (priv->specific_window); */
/* 	priv->modified = TRUE; */
}

static void
on_combo_outgoing_auth_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	
	ModestProtocolType protocol_security = 
		modest_secureauth_combo_box_get_active_secureauth (
			MODEST_SECUREAUTH_COMBO_BOX (priv->combo_outgoing_auth));
	const gboolean secureauth_used = protocol_security != MODEST_PROTOCOLS_AUTH_NONE;
	
	gtk_widget_set_sensitive (priv->caption_outgoing_username, secureauth_used);
	gtk_widget_set_sensitive (priv->caption_outgoing_password, secureauth_used);
}

static void
on_combo_outgoing_security_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	
	const gint port_number = 
		modest_serversecurity_combo_box_get_active_serversecurity_port (
			MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_outgoing_security));

	if(port_number != 0) {
		gtk_spin_button_set_value (
			GTK_SPIN_BUTTON (priv->entry_outgoing_port), (gdouble) port_number);
	}		
}

static void
on_combo_incoming_security_changed (GtkComboBox *widget, gpointer user_data)
{
	ModestAccountSettingsDialog *self = MODEST_ACCOUNT_SETTINGS_DIALOG (user_data);
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	const gint port_number = 
		modest_serversecurity_combo_box_get_active_serversecurity_port (
			MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_incoming_security));

	if(port_number != 0) {
		gtk_spin_button_set_value (
			GTK_SPIN_BUTTON (priv->entry_incoming_port), (gdouble) port_number);
	}		
}


static GtkWidget* 
create_page_outgoing (ModestDefaultAccountSettingsDialog *self)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);
	GtkWidget *alignment;
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);
	 
	/* The outgoing server widgets: */
	if (!priv->entry_outgoingserver)
		priv->entry_outgoingserver = gtk_entry_new ();
	GtkWidget *field = create_field (self, sizegroup, 
					 _("mcen_li_emailsetup_smtp"), priv->entry_outgoingserver, NULL, FALSE);
	gtk_widget_show (priv->entry_outgoingserver);
	connect_for_modified (self, priv->entry_outgoingserver);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* The secure authentication widgets: */
	if (!priv->combo_outgoing_auth)
		priv->combo_outgoing_auth = GTK_WIDGET (modest_secureauth_combo_box_new ());
	field = create_field (self, sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
		priv->combo_outgoing_auth, NULL, FALSE);
	gtk_widget_show (priv->combo_outgoing_auth);
	connect_for_modified (self, priv->combo_outgoing_auth);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* Dim the outgoing username and password when no secure authentication is used, as per the UI spec: */
	g_signal_connect (G_OBJECT (priv->combo_outgoing_auth), "changed", (GCallback)on_combo_outgoing_auth_changed, self);
	
	/* The username widgets: */	
	priv->entry_outgoing_username = GTK_WIDGET (modest_validating_entry_new ());
	priv->caption_outgoing_username = create_field (self, sizegroup, _("mail_fi_username"), 
						      priv->entry_outgoing_username, NULL, TRUE);
	gtk_widget_show (priv->entry_outgoing_username);
	connect_for_modified (self, priv->entry_outgoing_username);
	gtk_box_pack_start (GTK_BOX (box), priv->caption_outgoing_username, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->caption_outgoing_username);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	modest_validating_entry_set_unallowed_characters_whitespace (
	 	MODEST_VALIDATING_ENTRY (priv->entry_outgoing_username));
	
	/* Set max length as in the UI spec:
	 * The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_outgoing_username), 64);
	modest_validating_entry_set_max_func (MODEST_VALIDATING_ENTRY (priv->entry_outgoing_username), 
		on_entry_max, self);
		
	/* The password widgets: */	
	priv->entry_outgoing_password = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (priv->entry_outgoing_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (priv->entry_outgoing_password), '*'); */
	priv->caption_outgoing_password = create_field (self, sizegroup, 
						      _("mail_fi_password"), priv->entry_outgoing_password, NULL, FALSE);
	gtk_widget_show (priv->entry_outgoing_password);
	connect_for_modified (self, priv->entry_outgoing_password);
	gtk_box_pack_start (GTK_BOX (box), priv->caption_outgoing_password, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->caption_outgoing_password);
	
	/* The secure connection widgets: */
	/* This will be filled and set with modest_serversecurity_combo_box_fill() 
	 * and modest_serversecurity_combo_box_set_active_serversecurity().
	 */
	if (!priv->combo_outgoing_security)
		
		priv->combo_outgoing_security = GTK_WIDGET (modest_serversecurity_combo_box_new ());
	field = create_field (self, sizegroup, _("mcen_li_emailsetup_secure_connection"), 
			      priv->combo_outgoing_security, NULL, FALSE);
	gtk_widget_show (priv->combo_outgoing_security);
	connect_for_modified (self, priv->combo_outgoing_security);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* Show a default port number when the security method changes, as per the UI spec: */
	g_signal_connect (G_OBJECT (priv->combo_outgoing_security), "changed", (GCallback)on_combo_outgoing_security_changed, self);
	
	/* The port widgets: */
	if (!priv->entry_outgoing_port)
		priv->entry_outgoing_port = GTK_WIDGET (gtk_spin_button_new_with_range ((gdouble)PORT_MIN, (gdouble) PORT_MAX, 1.0));
	field = create_field (self, sizegroup, _("mcen_fi_emailsetup_port"), 
			      priv->entry_outgoing_port, NULL, FALSE);
	gtk_widget_show (priv->entry_outgoing_port);
	connect_for_modified (self, priv->entry_outgoing_port);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	GtkWidget *separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), separator, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (separator);
	
	/* connection-specific checkbox: */
	if (!priv->checkbox_outgoing_smtp_specific) {
		priv->checkbox_outgoing_smtp_specific = gtk_check_button_new ();
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->checkbox_outgoing_smtp_specific), 
			FALSE);
	}
	field = create_field (self, sizegroup, _("mcen_fi_advsetup_connection_smtp"), 
			      priv->checkbox_outgoing_smtp_specific, NULL, FALSE);
	gtk_widget_show (priv->checkbox_outgoing_smtp_specific);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	connect_for_modified (self, priv->checkbox_outgoing_smtp_specific);
	
	/* Connection-specific SMTP-Severs Edit button: */
	if (!priv->button_outgoing_smtp_servers)
		priv->button_outgoing_smtp_servers = gtk_button_new_with_label (_("mcen_bd_edit"));
	field = create_field (self, sizegroup, _("mcen_fi_advsetup_optional_smtp"), 
			      priv->button_outgoing_smtp_servers, NULL, FALSE);
	gtk_widget_show (priv->button_outgoing_smtp_servers);
	gtk_box_pack_start (GTK_BOX (box), field, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (field);
	
	/* Only enable the button when the checkbox is checked: */
	enable_widget_for_togglebutton (priv->button_outgoing_smtp_servers, 
		GTK_TOGGLE_BUTTON (priv->checkbox_outgoing_smtp_specific));
		
	g_signal_connect (G_OBJECT (priv->button_outgoing_smtp_servers), "clicked",
        	G_CALLBACK (on_button_outgoing_smtp_servers), self);
		
	gtk_widget_show (GTK_WIDGET (box));
	
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 12, 12, 12);
	gtk_container_add (GTK_CONTAINER (alignment), box);
	gtk_widget_show (alignment);
	
	return GTK_WIDGET (alignment);
}
	
static gboolean
check_data (ModestAccountSettingsDialog *self)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Check that the title is not already in use: */
	gchar* account_title = get_entered_account_title (self);
	if (!account_title)
		return FALSE; /* Should be prevented already anyway. */
		
	if (g_strcmp0 (account_title, priv->original_account_title) != 0) {
		/* Check the changed title: */
		const gboolean name_in_use  = modest_account_mgr_account_with_display_name_exists (priv->account_manager,
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
	const gchar* email_address = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_email));
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
	const gchar* hostname = gtk_entry_get_text (GTK_ENTRY (priv->entry_incomingserver));
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
	const gchar* hostname2 = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoingserver));
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
	
	/* Find a suitable authentication method when secure authentication is desired */

	const gint port_num = gtk_spin_button_get_value_as_int (
			GTK_SPIN_BUTTON (priv->entry_incoming_port));
	const gchar* username = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_username));

	/*
	const ModestProtocolType protocol_security_incoming = modest_serversecurity_combo_box_get_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_incoming_security));
	*/
	/* If we use an encrypted protocol then there is no need to encrypt the password */
	/* I don't think this is a good assumption. It overrides the user's request. murrayc: 
	 *  if (!modest_protocol_info_is_secure(protocol_security_incoming)) */
	if (TRUE)
	{
		if (gtk_toggle_button_get_active (
				GTK_TOGGLE_BUTTON (priv->checkbox_incoming_auth))) {
			GError *error = NULL;

			GList *list_auth_methods = 
				modest_utils_get_supported_secure_authentication_methods (priv->incoming_protocol, 
					hostname, port_num, username, GTK_WINDOW (self), &error);
			if (list_auth_methods) {
				/* Use the first supported method.
				 * TODO: Should we prioritize them, to prefer a particular one? */
				GList* method;
				for (method = list_auth_methods; method != NULL; method = g_list_next(method))
				{
					ModestProtocolType proto = (ModestProtocolType)(GPOINTER_TO_INT(method->data));
					// Allow secure methods, e.g MD5 only
					if (modest_protocol_registry_protocol_type_is_secure (modest_runtime_get_protocol_registry (), proto))
					{
						priv->protocol_authentication_incoming = proto;
						break;
					}
				}
				g_list_free (list_auth_methods);
			}

			if (list_auth_methods == NULL || 
			    !modest_protocol_registry_protocol_type_is_secure (modest_runtime_get_protocol_registry (),
									       priv->protocol_authentication_incoming)) {
		  		if(error == NULL || error->domain != modest_utils_get_supported_secure_authentication_error_quark() ||
						error->code != MODEST_UTILS_GET_SUPPORTED_SECURE_AUTHENTICATION_ERROR_CANCELED)
					modest_platform_information_banner(GTK_WIDGET (self), NULL, 
									 _("Could not discover supported secure authentication methods."));

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
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	enable_buttons (self);
	
	gboolean prevent_response = FALSE;

	/* Warn about unsaved changes: */
	if (response_id == GTK_RESPONSE_CANCEL && priv->modified) {
		GtkDialog *dialog = GTK_DIALOG (modest_platform_run_confirmation_dialog (GTK_WINDOW (self), 
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
		if (priv->modified)
		{
			const gboolean saved = save_configuration (self);
			if (saved) {
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
modest_account_settings_dialog_init (gpointer g, gpointer iface_data)
{
	ModestAccountSettingsDialogClass *iface = (ModestAccountSettingsDialogClass *) g;

	iface->load_settings = modest_default_account_settings_dialog_load_settings;
}

static void
modest_default_account_settings_dialog_init (ModestDefaultAccountSettingsDialog *self)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Create the notebook to be used by the GtkDialog base class:
	 * Each page of the notebook will be a page of the wizard: */
	priv->notebook = GTK_NOTEBOOK (gtk_notebook_new());
	priv->settings = modest_account_settings_new ();

	/* Get the account manager object, 
	 * so we can check for existing accounts,
	 * and create new accounts: */
	priv->account_manager = modest_runtime_get_account_mgr ();
	g_assert (priv->account_manager);
	g_object_ref (priv->account_manager);
	
	priv->protocol_authentication_incoming = MODEST_PROTOCOLS_AUTH_PASSWORD;

    /* Create the common pages, 
     */
	priv->page_account_details = create_page_account_details (self);
	priv->page_user_details = create_page_user_details (self);
	priv->page_incoming = create_page_incoming (self);
	priv->page_outgoing = create_page_outgoing (self);
	
	/* Add the notebook pages: */
	gtk_notebook_append_page (priv->notebook, priv->page_account_details, 
		gtk_label_new (_("mcen_ti_account_settings_account")));
	gtk_notebook_append_page (priv->notebook, priv->page_user_details, 
		gtk_label_new (_("mcen_ti_account_settings_userinfo")));
	gtk_notebook_append_page (priv->notebook, priv->page_incoming,
		gtk_label_new (_("mcen_ti_advsetup_retrieval")));
	gtk_notebook_append_page (priv->notebook, priv->page_outgoing,
		gtk_label_new (_("mcen_ti_advsetup_sending")));
		
	GtkDialog *dialog = GTK_DIALOG (self);
	gtk_container_add (GTK_CONTAINER (dialog->vbox), GTK_WIDGET (priv->notebook));
	gtk_container_set_border_width (GTK_CONTAINER (dialog->vbox), MODEST_MARGIN_HALF);
	gtk_widget_show (GTK_WIDGET (priv->notebook));
        
    /* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_SAVE, GTK_RESPONSE_OK);
    
    /* Connect to the dialog's response signal: */
    /* We use connect-before 
     * so we can stop the signal emission, 
     * to stop the default signal handler from closing the dialog.
     */
    g_signal_connect (G_OBJECT (self), "response",
            G_CALLBACK (on_response), self); 
            
    priv->modified = FALSE;

    /* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
    modest_window_mgr_prevent_hibernation_while_window_is_shown (
    	modest_runtime_get_window_mgr (), GTK_WINDOW (self)); 

    gtk_window_set_default_size (GTK_WINDOW (self), 600, 400);

/*     hildon_help_dialog_help_enable (GTK_DIALOG(self), "applications_email_accountsettings", */
/* 				    modest_maemo_utils_get_osso_context()); */
}

ModestAccountSettingsDialog*
modest_default_account_settings_dialog_new (void)
{
	return g_object_new (MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, NULL);
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
	const gchar *account_name;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS (settings));
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);

	incoming_account = modest_account_settings_get_store_settings (settings);
	outgoing_account = modest_account_settings_get_transport_settings (settings);

	account_name = modest_account_settings_get_account_name (settings);
		
	/* Save the account name so we can refer to it later: */
	if (priv->account_name)
		g_free (priv->account_name);
	priv->account_name = g_strdup (account_name);

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
	modest_retrieve_combo_box_fill (MODEST_RETRIEVE_COMBO_BOX (priv->combo_retrieve), 
					modest_server_account_settings_get_protocol (incoming_account));
	modest_retrieve_combo_box_set_active_retrieve_conf (MODEST_RETRIEVE_COMBO_BOX (priv->combo_retrieve), 
							    modest_account_settings_get_retrieve_type (settings));
	modest_limit_retrieve_combo_box_set_active_limit_retrieve (
		MODEST_LIMIT_RETRIEVE_COMBO_BOX (priv->combo_limit_retrieve), 
		modest_account_settings_get_retrieve_limit (settings));
	
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->checkbox_leave_messages), 
				      modest_account_settings_get_leave_messages_on_server (settings));
	
	/* Only show the leave-on-server checkbox for POP, 
	 * as per the UI spec: */
	if (modest_server_account_settings_get_protocol (incoming_account) != MODEST_PROTOCOLS_STORE_POP) {
		gtk_widget_hide (priv->caption_leave_messages);
	} else {
		gtk_widget_show (priv->caption_leave_messages);
	}
	
	update_incoming_server_security_choices (dialog, modest_server_account_settings_get_protocol (incoming_account));
	if (incoming_account) {
		const gchar *username;
		const gchar *password;
		const gchar *hostname;
		/* Remember this for later: */
		priv->incoming_protocol = modest_server_account_settings_get_protocol (incoming_account);;
		
		hostname = modest_server_account_settings_get_hostname (incoming_account);
		username = modest_server_account_settings_get_username (incoming_account);
		password = modest_server_account_settings_get_password (incoming_account);
		gtk_entry_set_text( GTK_ENTRY (priv->entry_user_username),
				    null_means_empty (username));
		gtk_entry_set_text( GTK_ENTRY (priv->entry_user_password), 
				    null_means_empty (password));
			
		gtk_entry_set_text( GTK_ENTRY (priv->entry_incomingserver), 
				    null_means_empty (hostname));
			
		/* The UI spec says:
		 * If secure authentication is unchecked, allow sending username and password also as plain text.
    	 * If secure authentication is checked, require one of the secure methods during connection: SSL, TLS, CRAM-MD5 etc. 
	  	 * TODO: Do we need to discover which of these (SSL, TLS, CRAM-MD5) is supported?
         */														 
		modest_serversecurity_combo_box_set_active_serversecurity (
			MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_incoming_security), 
			modest_server_account_settings_get_security_protocol (incoming_account));
		
		/* Check if we have
		 - a secure protocol
		 OR
		 - use encrypted passwords
		*/
		const ModestProtocolType secure_auth = modest_server_account_settings_get_auth_protocol (incoming_account);
		priv->protocol_authentication_incoming = (secure_auth != MODEST_PROTOCOLS_AUTH_NONE)?
			secure_auth:MODEST_PROTOCOLS_AUTH_PASSWORD;
		if (modest_protocol_registry_protocol_type_is_secure(modest_runtime_get_protocol_registry (),
								     secure_auth))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (priv->checkbox_incoming_auth), 
						     TRUE);
		}
		else
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (priv->checkbox_incoming_auth), 
						     FALSE);
		};
					
		update_incoming_server_title (dialog, priv->incoming_protocol);
		
		const gint port_num = modest_server_account_settings_get_port (incoming_account);
		if (port_num == 0) {
			/* Show the appropriate port number: */
			on_combo_incoming_security_changed (
				GTK_COMBO_BOX (priv->combo_incoming_security), dialog);
		} else {
			/* Keep the user-entered port-number,
			 * or the already-appropriate automatic port number: */
			gtk_spin_button_set_value (
				GTK_SPIN_BUTTON (priv->entry_incoming_port), (gdouble) port_num);
		}
		g_object_unref (incoming_account);
	}
	
	outgoing_account = modest_account_settings_get_transport_settings (settings);
	if (outgoing_account) {
		const gchar *hostname;
		const gchar *username;
		const gchar *password;

		/* Remember this for later: */
		priv->outgoing_protocol = 
			modest_server_account_settings_get_protocol (outgoing_account);

		hostname = modest_server_account_settings_get_hostname (outgoing_account);
		username = modest_server_account_settings_get_username (outgoing_account);
		password = modest_server_account_settings_get_password (outgoing_account);
		gtk_entry_set_text( GTK_ENTRY (priv->entry_outgoingserver), 
				    null_means_empty (hostname));
		
		gtk_entry_set_text( GTK_ENTRY (priv->entry_outgoing_username), 
				    null_means_empty (username));
		gtk_entry_set_text( GTK_ENTRY (priv->entry_outgoing_password), 
				    null_means_empty (password));
		
		/* Get the secure-auth setting: */
		const ModestProtocolType secure_auth = modest_server_account_settings_get_auth_protocol (outgoing_account);
		modest_secureauth_combo_box_set_active_secureauth (
			MODEST_SECUREAUTH_COMBO_BOX (priv->combo_outgoing_auth), secure_auth);
		on_combo_outgoing_auth_changed (GTK_COMBO_BOX (priv->combo_outgoing_auth), dialog);
		
		modest_serversecurity_combo_box_fill (
			MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_outgoing_security), 
			priv->outgoing_protocol);
		
		/* Get the security setting: */
		const ModestProtocolType security = 
			modest_server_account_settings_get_security_protocol (outgoing_account);
		modest_serversecurity_combo_box_set_active_serversecurity (
			MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_outgoing_security), security);
		
		const gint port_num = modest_server_account_settings_get_port (outgoing_account);
		if (port_num == 0) {
			/* Show the appropriate port number: */
			on_combo_outgoing_security_changed (
				GTK_COMBO_BOX (priv->combo_outgoing_security), dialog);
		}
		else {
			/* Keep the user-entered port-number,
			 * or the already-appropriate automatic port number: */
			gtk_spin_button_set_value (
				GTK_SPIN_BUTTON (priv->entry_outgoing_port), (gdouble) port_num);
		}
		
		const gboolean has_specific = 
			modest_account_settings_get_use_connection_specific_smtp (settings);
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (priv->checkbox_outgoing_smtp_specific), 
			has_specific);
		g_object_unref (outgoing_account);
	}

	/* Set window title according to account: */
	/* TODO: Is this the correct way to find a human-readable name for
	 * the protocol used? */
	ModestProtocol *proto = 
		modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (), 
							       priv->incoming_protocol);

	const gchar* proto_str = modest_protocol_get_display_name (proto);
	gchar *proto_name = g_utf8_strup(proto_str, -1);
	const gchar *account_title = modest_account_settings_get_display_name(settings);

	gchar *title = g_strdup_printf(_("mcen_ti_account_settings"), proto_name, account_title);
	g_free (proto_name);

	gtk_window_set_title (GTK_WINDOW (dialog), title);
	g_free (title);

	/* account_data->is_enabled,  */
	/*account_data->is_default,  */

	/* Unset the modified flag so we can detect changes later: */
	priv->modified = FALSE;
}

/** Show the User Info tab.
 */
void 
modest_account_settings_dialog_switch_to_user_info (ModestAccountSettingsDialog *dialog)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);

	const gint page_num = gtk_notebook_page_num (priv->notebook, priv->page_user_details);
	if (page_num == -1) {
		g_warning ("%s: notebook page not found.\n", __FUNCTION__);	
	}
		
	/* Ensure that the widget is visible so that gtk_notebook_set_current_page() works: */
	/* TODO: even this hack (recommened by the GTK+ documentation) doesn't seem to work. */
	
	gtk_widget_show (priv->page_user_details);
	gtk_widget_show (GTK_WIDGET (priv->notebook));
	gtk_widget_show (GTK_WIDGET (dialog));
	gtk_notebook_set_current_page (priv->notebook, page_num);
}

static gboolean
save_configuration (ModestAccountSettingsDialog *dialog)
{
	ModestServerAccountSettings *store_settings;
	ModestServerAccountSettings *transport_settings;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);
		
	/* Set the account data from the widgets: */
	const gchar* account_name = priv->account_name;
	const gchar* user_fullname = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_name));
	modest_account_settings_set_fullname (priv->settings, user_fullname);
	
	const gchar* emailaddress = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_email));
	modest_account_settings_set_email_address (priv->settings, emailaddress);
		
/* 	/\* Signature: *\/ */
/* 	if (priv->signature_dialog) { */
/* 		gboolean use_signature = FALSE; */
/* 		gchar *signature =  */
/* 			modest_signature_editor_dialog_get_settings (MODEST_SIGNATURE_EDITOR_DIALOG (priv->signature_dialog), */
/* 								     &use_signature); */
    	
/* 		modest_account_settings_set_use_signature (priv->settings, use_signature); */
/* 		modest_account_settings_set_signature (priv->settings, signature); */
/* 	} */
	
	ModestAccountRetrieveType retrieve_type = modest_retrieve_combo_box_get_active_retrieve_conf (
		MODEST_RETRIEVE_COMBO_BOX (priv->combo_retrieve));
	modest_account_settings_set_retrieve_type (priv->settings, retrieve_type);
	
	gint retrieve_limit = modest_limit_retrieve_combo_box_get_active_limit_retrieve (
		MODEST_LIMIT_RETRIEVE_COMBO_BOX (priv->combo_limit_retrieve));
	modest_account_settings_set_retrieve_limit (priv->settings, retrieve_limit);
	
	const gboolean leave_on_server = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->checkbox_leave_messages));
	modest_account_settings_set_leave_messages_on_server (priv->settings, leave_on_server); 

	store_settings = modest_account_settings_get_store_settings (priv->settings);
			
	const gchar* hostname = gtk_entry_get_text (GTK_ENTRY (priv->entry_incomingserver));
	modest_server_account_settings_set_hostname (store_settings, hostname);
				
	const gchar* username = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_username));
	modest_server_account_settings_set_username (store_settings, username);
	
	const gchar* password = gtk_entry_get_text (GTK_ENTRY (priv->entry_user_password));
	modest_server_account_settings_set_password (store_settings, password);
			
	/* port: */
	gint port_num = gtk_spin_button_get_value_as_int (
			GTK_SPIN_BUTTON (priv->entry_incoming_port));
	modest_server_account_settings_set_port (store_settings, port_num);
			
	/* The UI spec says:
	 * If secure authentication is unchecked, allow sending username and password also as plain text.
	 * If secure authentication is checked, require one of the secure 
	 * methods during connection: SSL, TLS, CRAM-MD5 etc. 
	 */
	
	const ModestProtocolType protocol_security_incoming = modest_serversecurity_combo_box_get_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_incoming_security));
	modest_server_account_settings_set_security_protocol (store_settings, 
							      protocol_security_incoming);	
	modest_server_account_settings_set_auth_protocol (store_settings, 
							  priv->protocol_authentication_incoming);

	g_object_unref (store_settings);
	
	/* Outgoing: */
	transport_settings = modest_account_settings_get_transport_settings (priv->settings);
	
	hostname = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoingserver));
	modest_server_account_settings_set_hostname (transport_settings, hostname);
		
	username = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoing_username));
	modest_server_account_settings_set_username (transport_settings, username);
		
	password = gtk_entry_get_text (GTK_ENTRY (priv->entry_outgoing_password));
	modest_server_account_settings_set_password (transport_settings, password);
	
	const ModestProtocolType protocol_security_outgoing = modest_serversecurity_combo_box_get_active_serversecurity (
		MODEST_SERVERSECURITY_COMBO_BOX (priv->combo_outgoing_security));
	modest_server_account_settings_set_security_protocol (transport_settings, 
							      protocol_security_outgoing);
	
	const ModestProtocolType protocol_authentication_outgoing = modest_secureauth_combo_box_get_active_secureauth (
		MODEST_SECUREAUTH_COMBO_BOX (priv->combo_outgoing_auth));
	modest_server_account_settings_set_auth_protocol (transport_settings, protocol_authentication_outgoing);	
	
	/* port: */
	port_num = gtk_spin_button_get_value_as_int (
			GTK_SPIN_BUTTON (priv->entry_outgoing_port));
	modest_server_account_settings_set_port (transport_settings, port_num);
	g_object_unref (transport_settings);
	
	
	/* Set the changed account title last, to simplify the previous code: */
	gchar* account_title = get_entered_account_title (dialog);
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
		 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->checkbox_outgoing_smtp_specific)));

	/* this configuration is not persistent, we should not save */
	if (account_name != NULL)
		modest_account_mgr_save_account_settings (priv->account_manager, priv->settings);

/* 	if (priv->specific_window) { */
/* 		return modest_connection_specific_smtp_window_save_server_accounts ( */
/* 			MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (priv->specific_window)); */
/* 	} else { */
		return TRUE;
/* 	} */

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
	ModestProtocolType outgoing_auth_protocol;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);;
	
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

	/* The custom incoming server is mandatory: */
	if (enable_ok && entry_is_empty(priv->entry_outgoingserver))
		enable_ok = FALSE;

	/* Outgoing username is mandatory if outgoing auth is secure */
	if (priv->combo_outgoing_auth) {
		outgoing_auth_protocol = modest_secureauth_combo_box_get_active_secureauth (
			MODEST_SECUREAUTH_COMBO_BOX (priv->combo_outgoing_auth));
		if (enable_ok && 
		    outgoing_auth_protocol != MODEST_PROTOCOLS_AUTH_NONE &&
		    entry_is_empty (priv->entry_outgoing_username))
			enable_ok = FALSE;
	}
			
	/* Enable the buttons, 
	 * identifying them via their associated response codes:
	 */
	GtkDialog *dialog_base = GTK_DIALOG (self);
	gtk_dialog_set_response_sensitive (dialog_base,
					   GTK_RESPONSE_OK,
					   enable_ok);
}

void
modest_account_settings_dialog_check_allow_changes (ModestAccountSettingsDialog *self)
{
	ModestServerAccountSettings *incoming_settings;
	const gchar *server_account_name;
	gboolean username_known;
	ModestDefaultAccountSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (self);;

	if (!G_IS_OBJECT (priv->settings))
		return;

	incoming_settings = modest_account_settings_get_store_settings (priv->settings);
	server_account_name = modest_server_account_settings_get_account_name (incoming_settings);

	username_known = modest_account_mgr_get_server_account_username_has_succeeded (priv->account_manager, 
										       server_account_name);

	/* Enable or disable widgets */
	gtk_widget_set_sensitive (priv->entry_user_username, !username_known);
	gtk_widget_set_sensitive (priv->entry_incomingserver, !username_known);
	gtk_widget_set_sensitive (priv->entry_incoming_port, !username_known);
	gtk_widget_set_sensitive (priv->combo_incoming_security, !username_known);
}

void
modest_account_settings_dialog_save_password (ModestAccountSettingsDialog *dialog)
{
	ModestDefaultAccountSettingsDialogPrivate *priv;

	g_return_if_fail (MODEST_IS_ACCOUNT_SETTINGS_DIALOG (dialog));

	priv = MODEST_DEFAULT_ACCOUNT_SETTINGS_DIALOG_GET_PRIVATE (dialog);
	priv->modified = TRUE;
}

static void
modest_default_account_settings_dialog_class_init (ModestDefaultAccountSettingsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (ModestDefaultAccountSettingsDialogPrivate));

	object_class->get_property = modest_account_settings_dialog_get_property;
	object_class->set_property = modest_account_settings_dialog_set_property;
	object_class->dispose = modest_account_settings_dialog_dispose;
	object_class->finalize = modest_account_settings_dialog_finalize;
}

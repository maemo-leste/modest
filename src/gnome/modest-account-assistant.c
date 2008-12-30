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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <camel/camel-url.h>
#include <widgets/modest-combo-box.h>
#include "modest-account-assistant.h"
#include "modest-store-widget.h"
#include "modest-transport-widget.h"
#include "modest-text-utils.h"
#include "modest-runtime.h"
#include "modest-utils.h"
#include "modest-protocol-registry.h"
#include "modest-platform.h"
#include "gnome/modest-gnome-utils.h"

#include <string.h>

/* 'private'/'protected' functions */
static void       modest_account_assistant_class_init    (ModestAccountAssistantClass *klass);
static void       modest_account_assistant_init          (ModestAccountAssistant *obj);
static void       modest_account_assistant_finalize      (GObject *obj);
static gboolean   on_before_next (ModestWizardDialog *dialog, GtkWidget *current_page, GtkWidget *next_page);
static void       on_response (ModestWizardDialog *wizard_dialog,
			       gint response_id,
			       gpointer user_data);
static void       on_response_before (ModestWizardDialog *wizard_dialog,
				      gint response_id,
				      gpointer user_data);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountAssistantPrivate ModestAccountAssistantPrivate;
struct _ModestAccountAssistantPrivate {

	ModestAccountMgr *account_mgr;
	ModestAccountSettings *settings;
	gboolean   dirty;

	GtkWidget *notebook;

	GtkWidget *account_name;
	GtkWidget *fullname;
	GtkWidget *email;
       
	GtkWidget *username;
	GtkWidget *password;
	GtkWidget *store_server_widget;
	GtkWidget *store_protocol_combo;
	GtkWidget *store_security_combo;
	GtkWidget *store_secure_auth;
	GtkWidget *transport_server_widget;
	GtkWidget *transport_security_combo;
	GtkWidget *transport_secure_auth_combo;
	
	GtkWidget *transport_widget;

	GtkWidget *transport_holder;

	ModestPairList *receiving_transport_store_protos;
	ModestPairList *sending_transport_store_protos;
	ModestPairList *security_protos;
	ModestPairList *transport_security_protos;
	ModestPairList *transport_auth_protos;
};

#define MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                      MODEST_TYPE_ACCOUNT_ASSISTANT, \
                                                      ModestAccountAssistantPrivate))
/* globals */
static GtkAssistantClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

static void save_to_settings (ModestAccountAssistant *self);

GType
modest_account_assistant_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountAssistantClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_account_assistant_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountAssistant),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_account_assistant_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WIZARD_DIALOG,
		                                  "ModestAccountAssistant",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_assistant_class_init (ModestAccountAssistantClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_assistant_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountAssistantPrivate));

	ModestWizardDialogClass *base_klass = (ModestWizardDialogClass*)(klass);
	base_klass->before_next = on_before_next;
}

static gboolean
on_delete_event (GtkWidget *widget,
		 GdkEvent *event,
		 ModestAccountAssistant *assistant)
{
	gtk_dialog_response (GTK_DIALOG (assistant), GTK_RESPONSE_CANCEL);
	return TRUE;
}

static void
on_assistant_changed(GtkWidget* widget, ModestAccountAssistant* assistant)
{
	ModestAccountAssistantPrivate* priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(assistant);
	g_return_if_fail (priv != NULL);
	priv->dirty = TRUE;
}

static void
on_incoming_security_changed(GtkWidget* widget, ModestAccountAssistant* assistant)
{
	ModestAccountAssistantPrivate* priv;
	ModestProtocolType protocol_id;
	ModestProtocol *protocol_security_incoming;
	const gchar *name;
	ModestProtocolRegistry *registry;
	gboolean is_secure;

	g_return_if_fail (MODEST_IS_ACCOUNT_ASSISTANT (assistant));

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(assistant);
	registry = modest_runtime_get_protocol_registry ();
	name = (const gchar *) modest_combo_box_get_active_id  (MODEST_COMBO_BOX (priv->store_security_combo));
	protocol_security_incoming = modest_protocol_registry_get_protocol_by_name (registry, 
										    MODEST_PROTOCOL_REGISTRY_SECURE_PROTOCOLS,
										    name);
	protocol_id = modest_protocol_get_type_id (protocol_security_incoming);
	is_secure = modest_protocol_registry_protocol_type_is_secure (registry, protocol_id);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->store_secure_auth), is_secure);
	gtk_widget_set_sensitive (priv->store_secure_auth, !is_secure);
	
	on_assistant_changed (widget, assistant);
}

static void
invoke_enable_buttons_vfunc (ModestAccountAssistant *assistant)
{
	ModestWizardDialogClass *klass = MODEST_WIZARD_DIALOG_GET_CLASS (assistant);
	
	/* Call the vfunc, which may be overridden by derived classes: */
	if (klass->enable_buttons) {
		GtkNotebook *notebook = NULL;
		g_object_get (assistant, "wizard-notebook", &notebook, NULL);
		
		const gint current_page_num = gtk_notebook_get_current_page (notebook);
		if (current_page_num == -1)
			return;
			
		GtkWidget* current_page_widget = gtk_notebook_get_nth_page (notebook, current_page_num);
		(*(klass->enable_buttons))(MODEST_WIZARD_DIALOG (assistant), current_page_widget);
	}
}

static void
on_entry_changed (GtkEditable *editable, gpointer userdata)
{
	on_assistant_changed (NULL, MODEST_ACCOUNT_ASSISTANT (userdata));
	invoke_enable_buttons_vfunc(MODEST_ACCOUNT_ASSISTANT (userdata));
}

static void
on_combo_changed (GtkComboBox *combo, gpointer userdata)
{
	on_assistant_changed (NULL, MODEST_ACCOUNT_ASSISTANT (userdata));
	invoke_enable_buttons_vfunc(MODEST_ACCOUNT_ASSISTANT (userdata));
}

static void
add_intro_page (ModestAccountAssistant *assistant)
{
	GtkWidget *page, *label;
	ModestAccountAssistantPrivate *priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE (assistant);
	
	page = gtk_vbox_new (FALSE, 12);
	
	label = gtk_label_new (_("mcen_ia_emailsetup_intro"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), 12, 12);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 0);
	gtk_widget_set_size_request (label, 400, -1);
	gtk_widget_show_all (page);
	
	gtk_notebook_append_page (GTK_NOTEBOOK(priv->notebook), page, NULL);
		
	gtk_notebook_set_tab_label_text (GTK_NOTEBOOK(priv->notebook), page,
				      _("mcen_ti_emailsetup_welcome"));
	/* gtk_notebook_set_page_type (GTK_NOTEBOOK(assistant), page, */
	/* 			     GTK_ASSISTANT_PAGE_INTRO); */
	/* gtk_notebook_set_page_complete (GTK_ASSISTANT(assistant), */
	/* 				 page, TRUE); */
}


static void
set_current_page_complete (ModestAccountAssistant *self, gboolean complete)
{
	GtkWidget *page;
	gint pageno;
	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE (self);

	pageno = gtk_notebook_get_current_page (GTK_NOTEBOOK(priv->notebook));

	if (pageno != -1) {
		page   = gtk_notebook_get_nth_page (GTK_NOTEBOOK(priv->notebook), pageno);
		/* gtk_assistant_set_page_complete (GTK_NOTEBOOK(priv->notebook), page, complete); */
	}
}


static GtkWidget *
field_name_label (const gchar *text)
{
	GtkWidget *label;
	gchar *fixed_text;

	fixed_text = g_strconcat (text, ":", NULL);
	label = gtk_label_new (fixed_text);
	g_free (fixed_text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	return label;
}

static void
add_identity_page (ModestAccountAssistant *self)
{
	GtkWidget *page, *label, *table, *frame;
	GtkWidget *alignment;
	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	priv->account_name = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->account_name), 40);
	g_signal_connect (G_OBJECT (priv->account_name), "changed", G_CALLBACK (on_entry_changed), self);
	priv->fullname = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->fullname), 40);
	g_signal_connect (G_OBJECT (priv->fullname), "changed", G_CALLBACK (on_entry_changed), self);
	priv->email    = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->email), 40);
	g_signal_connect (G_OBJECT (priv->email), "changed", G_CALLBACK (on_entry_changed), self);
	priv->username = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->username), 40);
	g_signal_connect (G_OBJECT (priv->username), "changed", G_CALLBACK (on_entry_changed), self);
	priv->password = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->password), 40);
	gtk_entry_set_visibility (GTK_ENTRY (priv->password), FALSE);
	
	page = gtk_vbox_new (FALSE, 24);

	label = gtk_label_new (
		_("Please enter below the name for the account you're creating."));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 0, 12, 12);
	gtk_container_add (GTK_CONTAINER (alignment), label);
	gtk_box_pack_start (GTK_BOX(page), alignment, FALSE, FALSE, 0);
	
	table = gtk_table_new (1,2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 1);
	gtk_table_attach_defaults (GTK_TABLE(table),field_name_label (_("Account name")),
				   0,1,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->account_name,
				   1,2,0,1);
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 0, 12, 0);
	gtk_box_pack_start (GTK_BOX(page), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new (NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Public information </b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	table = gtk_table_new (2,2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_attach_defaults (GTK_TABLE(table),field_name_label (_("Full name")),
				   0,1,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),field_name_label (_("Email address")),
				   0,1,1,2);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->fullname,
				   1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->email,
				   1,2,1,2);
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (frame), alignment);
	gtk_box_pack_start (GTK_BOX(page), frame, FALSE, FALSE, 0);


	frame = gtk_frame_new (NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Server account </b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	table = gtk_table_new (2,2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_attach_defaults (GTK_TABLE(table),field_name_label (_("User name")),
				   0,1,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),field_name_label (_("Password")),
				   0,1,1,2);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->username,
				   1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->password,
				   1,2,1,2);
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (frame), alignment);
	gtk_box_pack_start (GTK_BOX(page), frame, FALSE, FALSE, 0);
	
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_container_add (GTK_CONTAINER (alignment), page);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 12, 12, 12);
	gtk_widget_show_all (alignment);
	gtk_notebook_append_page (GTK_NOTEBOOK(priv->notebook), alignment, NULL);
	
	gtk_notebook_set_tab_label_text (GTK_NOTEBOOK(priv->notebook), alignment,
				      _("Identity"));
	/* gtk_assistant_set_page_type (GTK_ASSISTANT(self), alignment, */
	/* 			     GTK_ASSISTANT_PAGE_CONTENT); */
	/* gtk_assistant_set_page_complete (GTK_ASSISTANT(self), */
	/* 				 alignment, FALSE); */
}	


static void
receiving_page_update_completeness (GtkEditable *editable,
				    ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	const gchar *txt;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	txt = gtk_entry_get_text (GTK_ENTRY (priv->store_server_widget));
	if (!txt || strlen(txt) == 0) {
		set_current_page_complete (self, FALSE);
		return;
	}
	set_current_page_complete (self, TRUE);
}

static void
add_receiving_page (ModestAccountAssistant *self)
{
	GtkWidget *page, *vbox;
	GtkWidget *table, *frame;
	GtkWidget *alignment;
	ModestAccountAssistantPrivate *priv;
	GtkWidget *label;
	const gchar *tag = MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	page = gtk_alignment_new (0.5, 0.0, 1.0, 0.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (page), 12, 12, 12, 12);
	vbox = gtk_vbox_new (FALSE, 24);
	gtk_container_add (GTK_CONTAINER (page), vbox);

	/* Warning label on top */
	label = gtk_label_new (_("Setting details for the incoming mail server."));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX(vbox),
			    label,
			    FALSE, FALSE, 0);

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	priv->receiving_transport_store_protos = modest_gnome_utils_get_protocols_pair_list (tag);
	priv->store_protocol_combo = modest_combo_box_new (priv->receiving_transport_store_protos, g_str_equal);
	priv->store_server_widget = gtk_entry_new ();

	/* Setup incoming server frame */
	frame = gtk_frame_new (NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Incoming server</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_attach (GTK_TABLE (table), field_name_label (_("Account type")),
			  0, 1, 0, 1,
			  GTK_FILL, 0, 0, 0);
	alignment = gtk_alignment_new (0.0, 0.5, 1.0, 1.0);
	gtk_container_add (GTK_CONTAINER (alignment), priv->store_protocol_combo);
	gtk_table_attach (GTK_TABLE (table), alignment,
			  1, 2, 0, 1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
	gtk_table_attach (GTK_TABLE (table), field_name_label (_("Incoming server")),
			  0, 1, 1, 2,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->store_server_widget,
			  1, 2, 1, 2,
			  GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (frame), alignment);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

	/* Setup security information widgets */
	tag = MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS;
	priv->security_protos = modest_gnome_utils_get_protocols_pair_list (tag);
	priv->store_security_combo = modest_combo_box_new (priv->security_protos, g_str_equal);
	priv->store_secure_auth = gtk_check_button_new ();

	/* Setup security frame */
	frame = gtk_frame_new (NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Security options</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_attach (GTK_TABLE (table), field_name_label (_("Secure connection")),
			  0, 1, 0, 1,
			  GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->store_security_combo,
			  1, 2, 0, 1,
			  GTK_FILL | GTK_EXPAND, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (table), field_name_label (_("Use secure authentication")),
			  0, 1, 1, 2,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), priv->store_secure_auth,
				   1, 2, 1, 2);
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (frame), alignment);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, FALSE, 0);
	
	/* Setup assistant page */
	gtk_notebook_append_page (GTK_NOTEBOOK(priv->notebook), page, NULL);
		
	gtk_notebook_set_tab_label_text (GTK_NOTEBOOK(priv->notebook), page,
					 _("Incoming details"));
	gtk_widget_show_all (page);
}




static void
on_sending_combo_box_changed (GtkComboBox *combo, ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	gchar *chosen;
	ModestProtocol *proto;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	chosen = gtk_combo_box_get_active_text (GTK_COMBO_BOX(combo));

	if (priv->transport_widget)
		gtk_container_remove (GTK_CONTAINER(priv->transport_holder),
				      priv->transport_widget);

	proto = modest_protocol_registry_get_protocol_by_name (modest_runtime_get_protocol_registry (),
							       MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
							       chosen);
	priv->transport_widget = modest_transport_widget_new (modest_protocol_get_type_id (proto));

	gtk_container_add (GTK_CONTAINER(priv->transport_holder),
			   priv->transport_widget);

	gtk_widget_show_all (priv->transport_holder);
	on_assistant_changed (NULL, MODEST_ACCOUNT_ASSISTANT (self));
}



static void
add_sending_page (ModestAccountAssistant *self)
{
	GtkWidget *page, *vbox;
	GtkWidget *table, *frame;
	GtkWidget *alignment;
	ModestAccountAssistantPrivate *priv;
	GtkWidget *label;
	const gchar *tag = MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	page = gtk_alignment_new (0.5, 0.0, 1.0, 0.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (page), 12, 12, 12, 12);
	vbox = gtk_vbox_new (FALSE, 24);
	gtk_container_add (GTK_CONTAINER (page), vbox);

	/* Warning label on top */
	label = gtk_label_new (_("Settings for the outgoing mail server"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX(vbox),
			    label,
			    FALSE, FALSE, 0);

	priv->transport_server_widget = gtk_entry_new ();
	/* Setup incoming server frame */
	frame = gtk_frame_new (NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Outgoing server</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_attach (GTK_TABLE (table), field_name_label (_("Outgoing server (SMTP)")),
			  0, 1, 0, 1,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->transport_server_widget,
			  1, 2, 0, 1,
			  GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (frame), alignment);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

	/* Setup security information widgets */
	priv->transport_security_protos = modest_gnome_utils_get_protocols_pair_list (tag);
	priv->transport_security_combo = modest_combo_box_new (priv->security_protos, g_str_equal);
	tag = MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS;
	priv->transport_auth_protos = modest_gnome_utils_get_protocols_pair_list (tag);
	priv->transport_secure_auth_combo = GTK_WIDGET (modest_combo_box_new (priv->transport_auth_protos, g_str_equal));

	/* Setup security frame */
	frame = gtk_frame_new (NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Security options</b>"));
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_attach (GTK_TABLE (table), field_name_label (_("Secure connection")),
			  0, 1, 0, 1,
			  GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (table), priv->transport_security_combo,
			  1, 2, 0, 1,
			  GTK_FILL | GTK_EXPAND, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (table), field_name_label (_("Secure authentication")),
			  0, 1, 1, 2,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), priv->transport_secure_auth_combo,
				   1, 2, 1, 2);
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
	gtk_container_add (GTK_CONTAINER (frame), alignment);
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, FALSE, 0);
	
	/* Setup assistant page */
	gtk_notebook_append_page (GTK_NOTEBOOK(priv->notebook), page, NULL);
		
	gtk_notebook_set_tab_label_text (GTK_NOTEBOOK(priv->notebook), page,
					 _("Outgoing details"));
	gtk_widget_show_all (page);

}

static void
modest_account_assistant_init (ModestAccountAssistant *obj)
{	
	ModestAccountAssistantPrivate *priv;	
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(obj);	

	priv->account_mgr	= NULL;

	priv->store_server_widget	= NULL;
	priv->transport_widget  = NULL;
	priv->settings = modest_account_settings_new ();
	priv->dirty = FALSE;

	priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);

	g_object_set (obj, "wizard-notebook", priv->notebook, NULL);
	g_object_set (obj, "wizard-name", _("Account wizard"), NULL);

	add_intro_page (obj);
	add_identity_page (obj); 
	add_receiving_page (obj); 
	add_sending_page (obj);

	gtk_notebook_set_current_page (GTK_NOTEBOOK(priv->notebook), 0);
	gtk_window_set_resizable (GTK_WINDOW(obj), TRUE); 	
	gtk_window_set_default_size (GTK_WINDOW(obj), 400, 400);
	
	gtk_window_set_modal (GTK_WINDOW(obj), TRUE);

	g_signal_connect_after (G_OBJECT (obj), "response",
				G_CALLBACK (on_response), obj);

	/* This is to show a confirmation dialog when the user hits cancel */
	g_signal_connect (G_OBJECT (obj), "response",
	                  G_CALLBACK (on_response_before), obj);

	g_signal_connect (G_OBJECT (obj), "delete-event",
			  G_CALLBACK (on_delete_event), obj);
}

static void
modest_account_assistant_finalize (GObject *obj)
{
	ModestAccountAssistantPrivate *priv;
		
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(obj);
	
	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	if (priv->settings) {
		g_object_unref (G_OBJECT (priv->settings));
		priv->settings = NULL;
	}
	
	/* These had to stay alive for as long as the comboboxes that used them: */
	modest_pair_list_free (priv->receiving_transport_store_protos);
	modest_pair_list_free (priv->sending_transport_store_protos);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static const gchar*
get_account_name (ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	return gtk_entry_get_text (GTK_ENTRY(priv->account_name));
}

static const gchar*
get_fullname (ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	return gtk_entry_get_text (GTK_ENTRY(priv->fullname));
}



static const gchar*
get_email (ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	return gtk_entry_get_text (GTK_ENTRY(priv->email));
}



/*
 * FIXME: hmmmm this a Camel internal thing, should move this
 * somewhere else
 */
static gchar*
get_account_uri (ModestProtocolType proto, const gchar* path)
{
	CamelURL *url;
	gchar *uri;
	
	if (proto == modest_protocol_registry_get_mbox_type_id ()) {
		url = camel_url_new ("mbox:", NULL);
	} else {
		if (proto == modest_protocol_registry_get_maildir_type_id ())
			url = camel_url_new ("maildir:", NULL);
		else
			g_return_val_if_reached (NULL);
	}

	camel_url_set_path (url, path);
	uri = camel_url_to_string (url, 0);
	camel_url_free (url);

	return uri;	
}

static gchar*
get_new_server_account_name (ModestAccountMgr* acc_mgr, 
			     ModestProtocolType proto_id,
			     const gchar *username, 
			     const gchar *servername)
{
	gchar *name;
	const gchar *proto_name;
	gint  i = 0;
	ModestProtocolRegistry *registry;
	ModestProtocol *proto;

	registry = modest_runtime_get_protocol_registry ();
	proto = modest_protocol_registry_get_protocol_by_type (registry, proto_id);
	proto_name = modest_protocol_get_name (proto);

	while (TRUE) {
		name = g_strdup_printf ("%s:%d", proto_name, i++);

		if (modest_account_mgr_account_exists (acc_mgr, name, TRUE))
			g_free (name);
		else
			break;
	}
	return name;
}



GtkWidget*
modest_account_assistant_new (ModestAccountMgr *account_mgr)
{
	GObject *obj;
	ModestAccountAssistant *self;
	ModestAccountAssistantPrivate *priv;

	g_return_val_if_fail (account_mgr, NULL);
	
	obj  = g_object_new(MODEST_TYPE_ACCOUNT_ASSISTANT, NULL);
	self = MODEST_ACCOUNT_ASSISTANT(obj);

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	g_object_ref (account_mgr);
	priv->account_mgr = account_mgr;

	return GTK_WIDGET(self);
}

static gchar*
get_entered_account_title (ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	const gchar* account_title;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	account_title = gtk_entry_get_text (GTK_ENTRY (priv->account_name));

	if (!account_title || (strlen (account_title) == 0)) {
		return NULL;
	} else {
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

static gboolean
on_before_next (ModestWizardDialog *dialog, GtkWidget *current_page, GtkWidget *next_page)
{
	ModestAccountAssistant *self = MODEST_ACCOUNT_ASSISTANT (dialog);
	ModestAccountAssistantPrivate *priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE (self);

	/* Do extra validation that couldn't be done for every key press,
	 * either because it was too slow,
	 * or because it requires interaction:
	 */
	if(!next_page) /* This is NULL when this is a click on Finish. */
	{
		save_to_settings (self);
		modest_account_mgr_add_account_from_settings (modest_runtime_get_account_mgr (), priv->settings);
	}
	
	
	return TRUE;
}

static gint 
get_serverport_incoming(ModestProtocolType protocol,
			ModestProtocolType security)
{
	int serverport_incoming = 0;

	/* We don't check for SMTP here as that is impossible for an incoming server. */
	if ((security == modest_protocol_registry_get_none_connection_type_id ()) ||
	    (security == modest_protocol_registry_get_tls_connection_type_id ()) ||
	    (security == modest_protocol_registry_get_tlsop_connection_type_id ())) {

		if (protocol == MODEST_PROTOCOLS_STORE_IMAP) {
			serverport_incoming = 143;
		} else if (protocol == MODEST_PROTOCOLS_STORE_POP) {
			serverport_incoming = 110;
		}
	} else if (security == modest_protocol_registry_get_ssl_connection_type_id ()) {
		if (protocol == MODEST_PROTOCOLS_STORE_IMAP) {
			serverport_incoming = 993;
		} else if  (protocol == MODEST_PROTOCOLS_STORE_POP) {
			serverport_incoming = 995;
		}
	}

	return serverport_incoming;
}

static GList* 
check_for_supported_auth_methods (ModestAccountAssistant* self)
{
	GError *error = NULL;
	ModestProtocolType protocol;
	const gchar* hostname;
	const gchar* username;
	gchar *store_protocol_name, *store_security_name;
	ModestProtocolType security_protocol;
	int port_num; 
	GList *list_auth_methods;
	ModestAccountAssistantPrivate *priv;
	ModestProtocolRegistry *registry;
	ModestProtocol *proto;
	
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE (self);
	hostname = gtk_entry_get_text(GTK_ENTRY(priv->store_server_widget));
	username = gtk_entry_get_text(GTK_ENTRY(priv->username));
	store_protocol_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->store_protocol_combo));
	registry = modest_runtime_get_protocol_registry ();
	proto = modest_protocol_registry_get_protocol_by_name (registry,
							       MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS,
							       store_protocol_name);
	protocol = modest_protocol_get_type_id (proto);

	g_free (store_protocol_name);
	store_security_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->store_security_combo));

	proto = modest_protocol_registry_get_protocol_by_name (registry,
							       MODEST_PROTOCOL_REGISTRY_CONNECTION_PROTOCOLS,
							       store_security_name);
	security_protocol = modest_protocol_get_type_id (proto);
	g_free (store_security_name);
	port_num = get_serverport_incoming(protocol, security_protocol); 
	list_auth_methods = modest_utils_get_supported_secure_authentication_methods (protocol, hostname, port_num, 
										      username, GTK_WINDOW (self), &error);

	if (list_auth_methods) {
		/* TODO: Select the correct method */
		GList* list = NULL;
		GList* method;
		for (method = list_auth_methods; method != NULL; method = g_list_next(method)) {
			ModestProtocolType auth = (ModestProtocolType) (GPOINTER_TO_INT(method->data));
			if (modest_protocol_registry_protocol_type_is_secure (registry, auth)) {
				list = g_list_append(list, GINT_TO_POINTER(auth));
			}
		}

		g_list_free(list_auth_methods);

		if (list)
			return list;
	}

	if(error != NULL)
		g_error_free(error);

	return NULL;
}

static ModestProtocolType check_first_supported_auth_method(ModestAccountAssistant* self)
{
	ModestProtocolType result = MODEST_PROTOCOLS_AUTH_PASSWORD;

	GList* methods = check_for_supported_auth_methods(self);
	if (methods)
	{
		/* Use the first one: */
		result = (ModestProtocolType) (GPOINTER_TO_INT(methods->data));
		g_list_free(methods);
	}

	return result;
}

/**
 * save_to_settings:
 * @self: a #ModestEasysetupWizardDialog
 *
 * takes information from all the wizard and stores it in settings
 */
static void
save_to_settings (ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE (self);
	gchar* display_name;
	const gchar *username, *password;
	gchar *store_hostname, *transport_hostname;
	guint store_port, transport_port;
	ModestProtocolType store_protocol, transport_protocol;
	ModestProtocolType store_security, transport_security;
	ModestProtocolType store_auth_protocol, transport_auth_protocol;
	ModestServerAccountSettings *store_settings, *transport_settings;
	const gchar *fullname, *email_address;

	/* username and password (for both incoming and outgoing): */
	username = gtk_entry_get_text (GTK_ENTRY (priv->username));
	password = gtk_entry_get_text (GTK_ENTRY (priv->password));

	/* Incoming server: */
	/* Note: We need something as default for the ModestTransportStoreProtocol* values, 
	 * or modest_account_mgr_add_server_account will fail. */
	store_port = 0;
	store_protocol = MODEST_PROTOCOLS_STORE_POP;
	store_security = MODEST_PROTOCOLS_CONNECTION_NONE;
	store_auth_protocol = MODEST_PROTOCOLS_AUTH_NONE;

	/* Use custom pages because no preset was specified: */
	store_hostname = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->store_server_widget) ));		
	store_protocol = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->store_protocol_combo));
	store_security = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->store_security_combo));

	/* The UI spec says: 
	 * If secure authentication is unchecked, allow sending username and password also as plain text.
	 * If secure authentication is checked, require one of the secure methods during 
	 * connection: SSL, TLS, CRAM-MD5 etc. */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->store_secure_auth)) &&
	    !modest_protocol_registry_protocol_type_is_secure(modest_runtime_get_protocol_registry (),
							      store_security)) {
		store_auth_protocol = check_first_supported_auth_method (self);
	} else {
		store_auth_protocol = MODEST_PROTOCOLS_AUTH_PASSWORD;
	}

	/* now we store the store account settings */
	store_settings = modest_account_settings_get_store_settings (priv->settings);
	modest_server_account_settings_set_hostname (store_settings, store_hostname);
	modest_server_account_settings_set_username (store_settings, username);
	modest_server_account_settings_set_password (store_settings, password);
	modest_server_account_settings_set_protocol (store_settings, store_protocol);
	modest_server_account_settings_set_security_protocol (store_settings, store_security);
	modest_server_account_settings_set_auth_protocol (store_settings, store_auth_protocol);
	if (store_port != 0)
		modest_server_account_settings_set_port (store_settings, store_port);

	g_object_unref (store_settings);
	g_free (store_hostname);
	
	/* Outgoing server: */
	transport_hostname = NULL;
	transport_protocol = MODEST_PROTOCOLS_STORE_POP;
	transport_security = MODEST_PROTOCOLS_CONNECTION_NONE;
	transport_auth_protocol = MODEST_PROTOCOLS_AUTH_NONE;
	transport_port = 0;
	
	transport_hostname = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->transport_server_widget) ));
	transport_protocol = MODEST_PROTOCOLS_TRANSPORT_SMTP; /* It's always SMTP for outgoing. */
	transport_security = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->transport_security_combo));
	transport_auth_protocol = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->transport_secure_auth_combo));
	    
	/* now we transport the transport account settings */
	transport_settings = modest_account_settings_get_transport_settings (priv->settings);
	modest_server_account_settings_set_hostname (transport_settings, transport_hostname);
	modest_server_account_settings_set_username (transport_settings, username);
	modest_server_account_settings_set_password (transport_settings, password);
	modest_server_account_settings_set_protocol (transport_settings, transport_protocol);
	modest_server_account_settings_set_security_protocol (transport_settings, transport_security);
	modest_server_account_settings_set_auth_protocol (transport_settings, transport_auth_protocol);
	if (transport_port != 0)
		modest_server_account_settings_set_port (transport_settings, transport_port);

	g_object_unref (transport_settings);
	g_free (transport_hostname);
	
	fullname = gtk_entry_get_text (GTK_ENTRY (priv->fullname));
	email_address = gtk_entry_get_text (GTK_ENTRY (priv->email));
	modest_account_settings_set_fullname (priv->settings, fullname);
	modest_account_settings_set_email_address (priv->settings, email_address);
	/* we don't set retrieve type to preserve advanced settings if any. By default account settings
	   are set to headers only */
	
	display_name = get_entered_account_title (self);
	modest_account_settings_set_display_name (priv->settings, display_name);
	g_free (display_name);

}

static void 
on_response (ModestWizardDialog *wizard_dialog,
	     gint response_id,
	     gpointer user_data)
{
	ModestAccountAssistant *self = MODEST_ACCOUNT_ASSISTANT (wizard_dialog);

	invoke_enable_buttons_vfunc (self);
}

static void 
on_response_before (ModestWizardDialog *wizard_dialog,
                    gint response_id,
                    gpointer user_data)
{
	ModestAccountAssistant *self = MODEST_ACCOUNT_ASSISTANT (wizard_dialog);
	ModestAccountAssistantPrivate *priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(wizard_dialog);

	if (response_id == GTK_RESPONSE_CANCEL) {
		/* This is mostly copied from
		 * src/maemo/modest-account-settings-dialog.c */
		if (priv->dirty) {
			gint dialog_response = modest_platform_run_confirmation_dialog (GTK_WINDOW (self), 
											_("imum_nc_wizard_confirm_lose_changes"));

			if (dialog_response != GTK_RESPONSE_OK) {
				/* Don't let the dialog close */
				g_signal_stop_emission_by_name (wizard_dialog, "response");
			}
		}
	}
}


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
#include <modest-protocol-info.h>

#include <string.h>

/* 'private'/'protected' functions */
static void       modest_account_assistant_class_init    (ModestAccountAssistantClass *klass);
static void       modest_account_assistant_init          (ModestAccountAssistant *obj);
static void       modest_account_assistant_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountAssistantPrivate ModestAccountAssistantPrivate;
struct _ModestAccountAssistantPrivate {

	ModestAccountMgr *account_mgr;

	GtkWidget *account_name;
	GtkWidget *fullname;
	GtkWidget *email;
       
	GtkWidget *username;
	GtkWidget *password;
	GtkWidget *store_server_widget;
	GtkWidget *store_protocol_combo;
	GtkWidget *store_security_combo;
	GtkWidget *store_secure_auth;
	
	GtkWidget *transport_widget;

	GtkWidget *transport_holder;

	ModestPairList *receiving_transport_store_protos;
	ModestPairList *sending_transport_store_protos;
	ModestPairList *security_protos;
};

#define MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                      MODEST_TYPE_ACCOUNT_ASSISTANT, \
                                                      ModestAccountAssistantPrivate))
/* globals */
static GtkAssistantClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

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
		my_type = g_type_register_static (GTK_TYPE_ASSISTANT,
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

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}



static void
add_intro_page (ModestAccountAssistant *assistant)
{
	GtkWidget *page, *label;
	
	page = gtk_vbox_new (FALSE, 12);
	
	label = gtk_label_new (_("mcen_ia_emailsetup_intro"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), 12, 12);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 0);
	gtk_widget_set_size_request (label, 400, -1);
	gtk_widget_show_all (page);
	
	gtk_assistant_append_page (GTK_ASSISTANT(assistant), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(assistant), page,
				      _("mcen_ti_emailsetup_welcome"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(assistant), page,
				     GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(assistant),
					 page, TRUE);
}


static void
set_current_page_complete (ModestAccountAssistant *self, gboolean complete)
{
	GtkWidget *page;
	gint pageno;

	pageno = gtk_assistant_get_current_page (GTK_ASSISTANT(self));

	if (pageno != -1) {
		page   = gtk_assistant_get_nth_page (GTK_ASSISTANT(self), pageno);
		gtk_assistant_set_page_complete (GTK_ASSISTANT(self), page, complete);
	}
}


static void
identity_page_update_completeness (GtkEditable *editable,
				   ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	const gchar *txt;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	txt = gtk_entry_get_text (GTK_ENTRY(priv->fullname));
	if (!txt || strlen(txt) == 0) {
		set_current_page_complete (self, FALSE);
		return;
	}

	/* FIXME: regexp check for email address */
	txt = gtk_entry_get_text (GTK_ENTRY(priv->email));
	if (!modest_text_utils_validate_email_address (txt, NULL)) {
		set_current_page_complete (self, FALSE);
		return;
	}

	txt = gtk_entry_get_text (GTK_ENTRY(priv->username));
	if (!txt || txt[0] == '\0') {
		set_current_page_complete (self, FALSE);
		return;
	}
	set_current_page_complete (self, TRUE);
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
	gtk_entry_set_max_length (GTK_ENTRY (priv->fullname), 40);
	priv->fullname = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (priv->fullname), 40);
	priv->email    = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->email), 40);
	priv->username = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->username), 40);
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

	g_signal_connect (G_OBJECT(priv->fullname), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	g_signal_connect (G_OBJECT(priv->username), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	g_signal_connect (G_OBJECT(priv->password), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	g_signal_connect (G_OBJECT(priv->email), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	g_signal_connect (G_OBJECT(priv->account_name), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	
	
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_container_add (GTK_CONTAINER (alignment), page);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 12, 12, 12);
	gtk_widget_show_all (alignment);
	gtk_assistant_append_page (GTK_ASSISTANT(self), alignment);
	
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), alignment,
				      _("Identity"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), alignment,
				     GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 alignment, FALSE);
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

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	page = gtk_alignment_new (0.5, 0.0, 1.0, 0.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (page), 12, 12, 12, 12);
	vbox = gtk_vbox_new (FALSE, 24);
	gtk_container_add (GTK_CONTAINER (page), vbox);

	/* Warning label on top */
	label = gtk_label_new (_("TODO: Note unable to..."));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX(vbox),
			    label,
			    FALSE, FALSE, 0);

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	priv->receiving_transport_store_protos = 
		modest_protocol_info_get_supported_store_protocol_pair_list ();
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
	priv->security_protos = 
		modest_protocol_info_get_connection_protocol_pair_list ();
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
	gtk_assistant_append_page (GTK_ASSISTANT(self), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Receiving mail"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, FALSE);
	gtk_widget_show_all (page);
}




static void
on_sending_combo_box_changed (GtkComboBox *combo, ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	gchar *chosen;
	
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	chosen = gtk_combo_box_get_active_text (GTK_COMBO_BOX(combo));

	if (priv->transport_widget)
		gtk_container_remove (GTK_CONTAINER(priv->transport_holder),
				      priv->transport_widget);
	priv->transport_widget =
		modest_transport_widget_new (modest_protocol_info_get_transport_store_protocol(chosen));

	gtk_container_add (GTK_CONTAINER(priv->transport_holder),
			   priv->transport_widget);

	gtk_widget_show_all (priv->transport_holder);
}



static void
add_sending_page (ModestAccountAssistant *self)
{
	GtkWidget *page, *box, *combo;
	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	page = gtk_vbox_new (FALSE, 6);
	
	gtk_box_pack_start (GTK_BOX(page),
			    gtk_label_new (
				    _("Please select among the following options")),
			    FALSE, FALSE, 0);
	box = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(box),
			    gtk_label_new(_("Server type")),
			    FALSE,FALSE,0);
	
	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	priv->sending_transport_store_protos = modest_protocol_info_get_transport_store_protocol_pair_list ();
	combo = modest_combo_box_new (priv->sending_transport_store_protos, g_str_equal);

	g_signal_connect (G_OBJECT(combo), "changed",
			  G_CALLBACK(on_sending_combo_box_changed), self);

	gtk_box_pack_start (GTK_BOX(box), combo, FALSE,FALSE,0);
	gtk_box_pack_start (GTK_BOX(page), box, FALSE,FALSE, 0);

	gtk_box_pack_start (GTK_BOX(page), gtk_hseparator_new(), FALSE, FALSE, 0);

	priv->transport_holder = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(page), priv->transport_holder,
			    FALSE, FALSE, 0);

	/* Force the selection */
	on_sending_combo_box_changed (GTK_COMBO_BOX (combo), self);

	gtk_assistant_append_page (GTK_ASSISTANT(self), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Sending mail"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, TRUE);
	gtk_widget_show_all (page);
}



static void
add_final_page (ModestAccountAssistant *self)
{
	GtkWidget *page, *box;
	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	page = gtk_vbox_new (FALSE, 6);
	
	gtk_box_pack_start (GTK_BOX(page),
			    gtk_label_new (
				    _("We're almost done. Press 'Apply' to store this new account")),
			    FALSE, FALSE, 6);
	box = gtk_hbox_new (FALSE, 6);
	priv->account_name =
		gtk_entry_new_with_max_length (40);
	gtk_entry_set_text (GTK_ENTRY(priv->account_name),
			    gtk_entry_get_text(GTK_ENTRY(priv->email)));
	gtk_box_pack_start (GTK_BOX(box),gtk_label_new(_("Account name:")),
			    FALSE,FALSE,6);
	gtk_box_pack_start (GTK_BOX(box),priv->account_name , FALSE,FALSE,6);
	
	gtk_box_pack_start (GTK_BOX(page), box, FALSE, FALSE, 6);
	
	gtk_assistant_append_page (GTK_ASSISTANT(self), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Account Management"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_CONFIRM);

	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, TRUE);
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
	
	/* These had to stay alive for as long as the comboboxes that used them: */
	modest_pair_list_free (priv->receiving_transport_store_protos);
	modest_pair_list_free (priv->sending_transport_store_protos);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
on_cancel (ModestAccountAssistant *self, gpointer user_data)
{
	GtkWidget *label;
	GtkWidget *dialog;
	int response;
	
	label = gtk_label_new (_("Are you sure you want to cancel\n"
				 "setting up a new account?"));
	
	dialog = gtk_dialog_new_with_buttons (_("Cancel"),
					      GTK_WINDOW(self),
					      GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_YES, GTK_RESPONSE_ACCEPT,
					      GTK_STOCK_NO,  GTK_RESPONSE_CANCEL,
					      NULL);
	
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox),
			    label, FALSE, FALSE, 6);

	gtk_widget_show_all ((GTK_DIALOG(dialog)->vbox));
	
	gtk_window_set_resizable (GTK_WINDOW(dialog), FALSE);
	
	response = gtk_dialog_run (GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);

	switch (response) {
	case GTK_RESPONSE_ACCEPT:
		/* close the assistant */
		gtk_widget_hide (GTK_WIDGET(self));
		break;
	case GTK_RESPONSE_CANCEL:
		/* don't do anything */
		break;
	default: g_assert_not_reached ();

	};			     
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


static void
on_close (ModestAccountAssistant *self, gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET (self));
}


/*
 * FIXME: hmmmm this a Camel internal thing, should move this
 * somewhere else
 */
static gchar*
get_account_uri (ModestTransportStoreProtocol proto, const gchar* path)
{
	CamelURL *url;
	gchar *uri;
	
	switch (proto) {
	case MODEST_PROTOCOL_STORE_MBOX:
		url = camel_url_new ("mbox:", NULL); break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
		url = camel_url_new ("maildir:", NULL); break;
	default:
		g_return_val_if_reached (NULL);
	}
	camel_url_set_path (url, path);
	uri = camel_url_to_string (url, 0);
	camel_url_free (url);

	return uri;	
}

static gchar*
get_new_server_account_name (ModestAccountMgr* acc_mgr, ModestTransportStoreProtocol proto,
			     const gchar* username, const gchar *servername)
{
	gchar *name;
	gint  i = 0;
	
	while (TRUE) {
		name = g_strdup_printf ("%s:%d",
					modest_protocol_info_get_transport_store_protocol_name(proto), i++);
		if (modest_account_mgr_account_exists (acc_mgr, name, TRUE))
			g_free (name);
		else
			break;
	}
	return name;
}


static void
on_apply (ModestAccountAssistant *self, gpointer user_data)
{
/* 	ModestAccountAssistantPrivate *priv; */
/* 	ModestTransportStoreProtocol proto = MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN; */
/* 	ModestAuthProtocol security = MODEST_PROTOCOL_CONNECTION_NORMAL; */
/* 	ModestConnectionProtocol auth = MODEST_PROTOCOL_AUTH_NONE; */
/* 	gchar *store_name, *transport_name; */
/* 	const gchar *account_name, *username, *servername, *path; */
/* 	ModestTransportWidget *transport; */

/* 	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self); */

/* 	/\* create server account -> store *\/ */
/* 	store = MODEST_STORE_WIDGET(priv->store_widget); */
/* 	proto    = modest_store_widget_get_proto (store); */
/* 	username = modest_store_widget_get_username (store); */
/* 	servername = modest_store_widget_get_servername (store); */
/* 	path       = modest_store_widget_get_path (store); */
/* 	security = modest_store_widget_get_security (store); */
/* 	auth = modest_store_widget_get_auth (store); */
/* 	store_name = get_new_server_account_name (priv->account_mgr, proto,username, servername); */

/* 	if (proto == MODEST_PROTOCOL_STORE_MAILDIR || */
/* 	    proto == MODEST_PROTOCOL_STORE_MBOX) { */
/* 		gchar *uri = get_account_uri (proto, path); */
/* 		modest_account_mgr_add_server_account_uri (priv->account_mgr, store_name, proto, uri); */
/* 		g_free (uri); */
/* 	} else */
/* 		modest_account_mgr_add_server_account (priv->account_mgr, */
/* 						       store_name,  */
/* 						       servername, */
/* 						       0, /\* FIXME: does this mean default?*\/ */
/* 						       username, */
/* 						       NULL,  */
/* 						       proto,  */
/* 						       security,  */
/* 						       auth); */
		
/* 	/\* create server account -> transport *\/ */
/* 	transport = MODEST_TRANSPORT_WIDGET(priv->transport_widget); */
/* 	proto = modest_transport_widget_get_proto (transport); */
/* 	username   = NULL; */
/* 	servername = NULL; */
/* 	if (proto == MODEST_PROTOCOL_TRANSPORT_SMTP) { */
/* 		servername = modest_transport_widget_get_servername (transport); */
/* 		if (modest_transport_widget_get_requires_auth (transport)) */
/* 			username = modest_transport_widget_get_username (transport); */
/* 	} */
	
/* 	transport_name = get_new_server_account_name (priv->account_mgr, proto,username, servername); */
/* 	modest_account_mgr_add_server_account (priv->account_mgr, */
/* 					       transport_name,	servername, */
/* 					       0, /\* FIXME: does this mean default?*\/ */
/* 					       username, NULL, */
/* 					       proto, security, auth); */

/* 	/\* create account *\/ */
/* 	account_name = get_account_name (self); */
/* 	modest_account_mgr_add_account (priv->account_mgr, */
/* 					account_name, */
/* 					account_name, */
/* 					get_fullname (self), */
/* 					get_email (self), */
/* 					MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY, */
/* 					store_name, */
/* 					transport_name, TRUE); */

/* 	/\* Frees *\/	 */
/* 	g_free (store_name); */
/* 	g_free (transport_name); */
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

	add_intro_page (self);
	add_identity_page (self); 
	add_receiving_page (self); 
	add_sending_page (self);
	add_final_page (self);

	gtk_assistant_set_current_page (GTK_ASSISTANT(self), 0);
	gtk_window_set_title (GTK_WINDOW(self),
			      _("Modest Account Wizard"));
	gtk_window_set_resizable (GTK_WINDOW(self), TRUE); 	
	gtk_window_set_default_size (GTK_WINDOW(self), 400, 400);
	
	gtk_window_set_modal (GTK_WINDOW(self), TRUE);

	g_signal_connect (G_OBJECT(self), "apply",
			  G_CALLBACK(on_apply), NULL);
	g_signal_connect (G_OBJECT(self), "cancel",
			  G_CALLBACK(on_cancel), NULL);
	g_signal_connect (G_OBJECT(self), "close",
			  G_CALLBACK(on_close), NULL);

	return GTK_WIDGET(self);
}

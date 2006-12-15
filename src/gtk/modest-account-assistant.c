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
#include "modest-account-assistant.h"
#include "modest-store-widget.h"
#include "modest-transport-widget.h"

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

	ModestWidgetFactory *factory;
	ModestAccountMgr *account_mgr;

	GtkWidget *account_name;
	GtkWidget *fullname;
	GtkWidget *email;
	
	GtkWidget *store_widget;
	GtkWidget *transport_widget;

	GtkWidget *transport_holder;
	GtkWidget *store_holder;	
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
	
	page = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new (
		_("Welcome to the account assistant\n\n"
		  "It will help to set up a new e-mail account\n"));
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 6);
	gtk_widget_show_all (page);
	
	gtk_assistant_append_page (GTK_ASSISTANT(assistant), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(assistant), page,
				      _("Modest Account Assistant"));
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
	page   = gtk_assistant_get_nth_page (GTK_ASSISTANT(self), pageno);

	gtk_assistant_set_page_complete (GTK_ASSISTANT(self), page, complete);
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
	if (!txt || strlen(txt) == 0) {
		set_current_page_complete (self, FALSE);
		return;
	}
	set_current_page_complete (self, TRUE);
}


static void
add_identity_page (ModestAccountAssistant *self)
{
	GtkWidget *page, *label, *table;
	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	priv->fullname = gtk_entry_new_with_max_length (40);
	priv->email    = gtk_entry_new_with_max_length (40);
	
	page = gtk_vbox_new (FALSE, 6);

	label = gtk_label_new (
		_("Please enter your name and your e-mail address below.\n\n"));
	gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 6);
	
	table = gtk_table_new (2,2, FALSE);
	gtk_table_attach_defaults (GTK_TABLE(table),gtk_label_new (_("Full name")),
				   0,1,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),gtk_label_new (_("E-mail address")),
				   0,1,1,2);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->fullname,
				   1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(table),priv->email,
				   1,2,1,2);

	g_signal_connect (G_OBJECT(priv->fullname), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	g_signal_connect (G_OBJECT(priv->email), "changed",
			  G_CALLBACK(identity_page_update_completeness),
			  self);
	
	gtk_box_pack_start (GTK_BOX(page), table, FALSE, FALSE, 6);
	gtk_widget_show_all (page);
	
	gtk_assistant_append_page (GTK_ASSISTANT(self), page);
	
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Identity"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, FALSE);
}	


static void
on_receiving_combo_box_changed (GtkComboBox *combo, ModestAccountAssistant *self)
{
	ModestAccountAssistantPrivate *priv;
	gchar *chosen;
	
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	chosen = gtk_combo_box_get_active_text (GTK_COMBO_BOX(combo));

	if (priv->store_widget)
		gtk_container_remove (GTK_CONTAINER(priv->store_holder),
				      priv->store_widget);
	
	priv->store_widget = modest_store_widget_new (priv->factory, chosen);

	gtk_container_add (GTK_CONTAINER(priv->store_holder),
			   priv->store_widget);
	
	gtk_widget_show_all (priv->store_holder);
	
}	

static void
add_receiving_page (ModestAccountAssistant *self)
{
	GtkWidget *page, *box, *combo;

	ModestAccountAssistantPrivate *priv;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);	
	page = gtk_vbox_new (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(page),
			    gtk_label_new (
				    _("Please select among the following options")),
			    FALSE, FALSE, 6);
	box = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(box),
			    gtk_label_new(_("Server type")),
			    FALSE,FALSE,6);

	combo = modest_widget_factory_get_combo_box (priv->factory,
						     MODEST_COMBO_BOX_TYPE_STORE_PROTOS);
	g_signal_connect (G_OBJECT(combo), "changed",
			  G_CALLBACK(on_receiving_combo_box_changed), self);

	gtk_box_pack_start (GTK_BOX(box), combo, FALSE,FALSE,6);
	gtk_box_pack_start (GTK_BOX(page), box, FALSE,FALSE, 6);
	
	gtk_box_pack_start (GTK_BOX(page), gtk_hseparator_new(), FALSE, FALSE, 0);

	priv->store_holder = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(page), priv->store_holder,
			    TRUE, TRUE, 0);
	
	gtk_assistant_append_page (GTK_ASSISTANT(self), page);
		
	gtk_assistant_set_page_title (GTK_ASSISTANT(self), page,
				      _("Receiving mail"));
	gtk_assistant_set_page_type (GTK_ASSISTANT(self), page,
				     GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete (GTK_ASSISTANT(self),
					 page, TRUE);
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
	
	priv->transport_widget = modest_transport_widget_new (priv->factory,
							      chosen);

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

	combo = modest_widget_factory_get_combo_box (priv->factory,
						     MODEST_COMBO_BOX_TYPE_TRANSPORT_PROTOS);
	g_signal_connect (G_OBJECT(combo), "changed",
			  G_CALLBACK(on_sending_combo_box_changed), self);

	gtk_box_pack_start (GTK_BOX(box), combo, FALSE,FALSE,0);
	gtk_box_pack_start (GTK_BOX(page), box, FALSE,FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX(page), gtk_hseparator_new(), FALSE, FALSE, 0);

	priv->transport_holder = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(page), priv->transport_holder,
			    FALSE, FALSE, 0);
	
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
	priv->factory           = NULL;
	priv->account_mgr	= NULL;

	priv->store_widget	= NULL;
	priv->transport_widget  = NULL;
}

static void
modest_account_assistant_finalize (GObject *obj)
{
	ModestAccountAssistantPrivate *priv;
		
	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(obj);

	if (priv->factory) {
		g_object_unref (G_OBJECT(priv->factory));
		priv->factory = NULL;
	}
	
	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}



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

static void
on_apply (ModestAccountAssistant *self, gpointer user_data)
{
	ModestAccountAssistantPrivate *priv;
	gchar *store_name;
	const gchar *account_name;
	ModestStoreWidget *store;

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);

	/* create account */

	/* create server account -> store */
	store = MODEST_STORE_WIDGET(priv->store_widget);
	store_name = g_strdup_printf ("%s:%s@%s",
				      modest_store_widget_get_proto (store),
				      modest_store_widget_get_username (store),
				      modest_store_widget_get_servername (store));
	
	modest_account_mgr_add_server_account (priv->account_mgr,
						store_name,
						modest_store_widget_get_servername (store),
						modest_store_widget_get_username (store),
						NULL,
						modest_store_widget_get_proto (store));

	/* create server account -> transport */
	account_name = get_account_name (self);
	modest_account_mgr_add_account (priv->account_mgr,
					account_name,
					store_name,
					NULL, NULL);
	modest_account_mgr_set_string (priv->account_mgr,
				       account_name,
				       MODEST_ACCOUNT_FULLNAME,
				       get_fullname(self), FALSE, NULL);
	modest_account_mgr_set_string (priv->account_mgr,
				       account_name,
				       MODEST_ACCOUNT_EMAIL,
				       get_email(self), FALSE, NULL);
	
	g_free (store_name);
}



GtkWidget*
modest_account_assistant_new (ModestAccountMgr *account_mgr, ModestWidgetFactory *factory)
{
	GObject *obj;
	ModestAccountAssistant *self;
	ModestAccountAssistantPrivate *priv;
		
	g_return_val_if_fail (factory, NULL);
	g_return_val_if_fail (account_mgr, NULL);
	
	obj  = g_object_new(MODEST_TYPE_ACCOUNT_ASSISTANT, NULL);
	self = MODEST_ACCOUNT_ASSISTANT(obj);

	priv = MODEST_ACCOUNT_ASSISTANT_GET_PRIVATE(self);
	
	g_object_ref (factory);
	priv->factory = factory;
	
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

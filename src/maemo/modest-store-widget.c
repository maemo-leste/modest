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
#include <widgets/modest-combo-box.h>
#include "modest-store-widget.h"
#include <string.h>

/* 'private'/'protected' functions */
static void modest_store_widget_class_init (ModestStoreWidgetClass *klass);
static void modest_store_widget_init       (ModestStoreWidget *obj);
static void modest_store_widget_finalize   (GObject *obj);
/* list my signals  */
enum {
	DATA_CHANGED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestStoreWidgetPrivate ModestStoreWidgetPrivate;
struct _ModestStoreWidgetPrivate {
	
	GtkWidget *servername;
	GtkWidget *username;
	
	ModestPairList *security_protos;
	GtkWidget *security;
	
	ModestPairList *transport_store_protos;
	
	GtkWidget *auth;
	GtkWidget *chooser;
	GtkWidget *remember_pwd;

	ModestTransportStoreProtocol proto;
};
#define MODEST_STORE_WIDGET_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_STORE_WIDGET, \
                                                 ModestStoreWidgetPrivate))
/* globals */
static GtkContainerClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

GType
modest_store_widget_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestStoreWidgetClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_store_widget_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestStoreWidget),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_store_widget_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_VBOX,
		                                  "ModestStoreWidget",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_store_widget_class_init (ModestStoreWidgetClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_store_widget_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestStoreWidgetPrivate));

	/* signal definitions go here, e.g.: */
	signals[DATA_CHANGED_SIGNAL] =
		g_signal_new ("data_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestStoreWidgetClass, data_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
modest_store_widget_init (ModestStoreWidget *obj)
{
 	ModestStoreWidgetPrivate *priv;
	
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(obj); 
	priv->proto = MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN;
}



static GtkWidget *
maildir_configuration (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;
	GtkWidget *label, *box, *hbox;
	
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);
	box = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(label),
			      _("<b>Maildir configuration</b>"));	
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 6);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(label),
			      _("Please select the path to your Maildir below"));	
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 6);
	
	label = gtk_label_new (_("Path:"));

	priv->chooser = 
		gtk_file_chooser_button_new (_("(none)"),
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), priv->chooser, FALSE, FALSE, 6);

	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 6);	

	return box;
}


static GtkWidget*
mbox_configuration (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;
	GtkWidget *label, *box, *hbox;
	
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);
	box = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(label),
			      _("<b>Mbox configuration</b>"));	
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 6);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(label),
			      _("Please select your mbox file below"));	
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 6);
	
	label = gtk_label_new (_("mbox:"));

	priv->chooser =
		gtk_file_chooser_button_new (_("(none)"),
					     GTK_FILE_CHOOSER_ACTION_OPEN);
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), priv->chooser, FALSE, FALSE, 6);
	
	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 6);	

	return box;
}

static void
on_entry_changed (GtkEntry *entry, gpointer user_data)
{
	g_signal_emit (MODEST_STORE_WIDGET (user_data), signals[DATA_CHANGED_SIGNAL], 0);
}

static GtkWidget*
imap_pop_configuration (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;
	GtkWidget *label, *box, *hbox;
	GtkWidget *combo;
	
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);
	box = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(label),_("<b>Server configuration</b>"));
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 6);
	
	hbox	= gtk_hbox_new (FALSE, 6);
	label   = gtk_label_new (_("Username:"));
 	if (!priv->username)
		priv->username = gtk_entry_new_with_max_length (40);
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(hbox), priv->username,FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 0);	

	hbox	= gtk_hbox_new (FALSE, 6);
	label   = gtk_label_new (_("Server:"));
	if (!priv->servername)
		priv->servername = gtk_entry_new_with_max_length (40);
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(hbox), priv->servername,FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 0);	

	label = gtk_label_new(NULL);
	gtk_label_set_markup (GTK_LABEL(label),_("<b>Security</b>"));
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 0);

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	priv->security_protos = modest_protocol_info_get_connection_protocol_pair_list ();
	priv->security = modest_combo_box_new (priv->security_protos, g_str_equal);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new(NULL);
	gtk_label_set_text (GTK_LABEL(label),_("Connection type:"));
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(hbox),  priv->security, FALSE, FALSE,0);
	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new(NULL);

	gtk_label_set_text (GTK_LABEL(label),_("Authentication:"));
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 6);
	
	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	priv->transport_store_protos = modest_protocol_info_get_transport_store_protocol_pair_list ();
	combo =  modest_combo_box_new (priv->transport_store_protos, g_str_equal);

	gtk_box_pack_start (GTK_BOX(hbox), combo, FALSE, FALSE, 0);
	priv->remember_pwd =
		gtk_check_button_new_with_label (_("Remember password"));
	gtk_box_pack_start (GTK_BOX(hbox),priv->remember_pwd,
			    FALSE, FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 0);

	/* Handle entry modifications */
	g_signal_connect (priv->username, "changed", G_CALLBACK (on_entry_changed), self);
	g_signal_connect (priv->servername, "changed", G_CALLBACK (on_entry_changed), self);

	return box;
}


static void
modest_store_widget_finalize (GObject *obj)
{
	ModestStoreWidgetPrivate *priv = MODEST_STORE_WIDGET_GET_PRIVATE(obj);
	
	/* These had to stay alive for as long as the comboboxes that used them: */
	modest_pair_list_free (priv->security_protos);
	modest_pair_list_free (priv->transport_store_protos);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



GtkWidget*
modest_store_widget_new (ModestTransportStoreProtocol proto)
{
	GObject *obj;
	GtkWidget *w;
	ModestStoreWidget *self;
	ModestStoreWidgetPrivate *priv;
	
	g_return_val_if_fail (proto, NULL);

	obj = g_object_new(MODEST_TYPE_STORE_WIDGET, NULL);
	self = MODEST_STORE_WIDGET(obj);
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);

	priv->proto = proto;
	
	if (proto == MODEST_PROTOCOL_STORE_POP || proto == MODEST_PROTOCOL_STORE_IMAP)
		w = imap_pop_configuration (self);
	else if (proto == MODEST_PROTOCOL_STORE_MAILDIR) 
		w = maildir_configuration (self);
	else if (proto == MODEST_PROTOCOL_STORE_MBOX)
		w = mbox_configuration (self);
	else
		w = gtk_label_new ("");
	
	gtk_widget_show_all (w);
	gtk_box_pack_start (GTK_BOX(self), w, FALSE, FALSE, 2);

	return GTK_WIDGET(self);
}

gboolean
modest_store_widget_get_remember_password (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;

	g_return_val_if_fail (self, FALSE);
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);

	if (GTK_IS_TOGGLE_BUTTON(priv->remember_pwd)) 
		return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->remember_pwd));
	else
		return FALSE;
}

const gchar*
modest_store_widget_get_username (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);

	if (GTK_IS_ENTRY(priv->username)) 
		return gtk_entry_get_text (GTK_ENTRY(priv->username));
	else
		return NULL;
}

const gchar*
modest_store_widget_get_servername (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);
	
	if (GTK_IS_ENTRY(priv->servername)) 
		return gtk_entry_get_text (GTK_ENTRY(priv->servername));
	else
		return NULL;
}


ModestTransportStoreProtocol
modest_store_widget_get_proto (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;

	g_return_val_if_fail (self, MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN);
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);

	return priv->proto;
}


gchar *
modest_store_widget_get_path (ModestStoreWidget *self)
{
	ModestStoreWidgetPrivate *priv;
	
	g_return_val_if_fail (self, NULL);
	priv = MODEST_STORE_WIDGET_GET_PRIVATE(self);

	if (GTK_IS_FILE_CHOOSER(priv->chooser))
		return gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(priv->chooser));
	else
		return NULL;
}

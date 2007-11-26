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
#include <modest-protocol-info.h>
#include "modest-transport-widget.h"
#include <string.h>



/* 'private'/'protected' functions */
static void modest_transport_widget_class_init (ModestTransportWidgetClass *klass);
static void modest_transport_widget_init       (ModestTransportWidget *obj);
static void modest_transport_widget_finalize   (GObject *obj);
/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTransportWidgetPrivate ModestTransportWidgetPrivate;
struct _ModestTransportWidgetPrivate {
	ModestTransportStoreProtocol proto;
	GtkWidget *servername;
	GtkWidget *username;
	GtkWidget *auth;
	GtkWidget *remember_pwd;
	
	ModestPairList *transport_store_protos;
	ModestPairList *auth_protos;
};
#define MODEST_TRANSPORT_WIDGET_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                     MODEST_TYPE_TRANSPORT_WIDGET, \
                                                     ModestTransportWidgetPrivate))
/* globals */
static GtkContainerClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_transport_widget_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTransportWidgetClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_transport_widget_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTransportWidget),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_transport_widget_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_VBOX,
		                                  "ModestTransportWidget",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_transport_widget_class_init (ModestTransportWidgetClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_transport_widget_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTransportWidgetPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_transport_widget_init (ModestTransportWidget *obj)
{
	ModestTransportWidgetPrivate *priv;
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(obj); 
	
	priv->proto = MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN;
}

static void
modest_transport_widget_finalize (GObject *obj)
{
	ModestTransportWidgetPrivate *priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(obj);
	
	/* These had to stay alive for as long as the comboboxes that used them: */
	modest_pair_list_free (priv->transport_store_protos);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
on_button_toggled (GtkToggleButton *button, gpointer user_data)
{
	ModestTransportWidgetPrivate *priv;

	priv = (ModestTransportWidgetPrivate *) user_data;

	if (gtk_toggle_button_get_active (button))
		gtk_widget_set_sensitive (gtk_widget_get_parent (priv->username), TRUE);
	else
		gtk_widget_set_sensitive (gtk_widget_get_parent (priv->username), FALSE);
}


static GtkWidget*
smtp_configuration (ModestTransportWidget *self)
{
	ModestTransportWidgetPrivate *priv;
	GtkWidget *label, *box, *hbox, *combo;
	
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(self);
	box = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(label),
			      _("<b>SMTP configuration</b>"));	
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 6);

	priv->servername = gtk_entry_new_with_max_length (40);
	priv->username = gtk_entry_new_with_max_length (40);

	/* Servername */	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), gtk_label_new (_("Servername: ")),
			    FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), priv->servername, TRUE, TRUE, 6);
	gtk_box_pack_start (GTK_BOX(box), hbox, TRUE, TRUE, 6);

	/* Auth */
	priv->auth = gtk_check_button_new_with_label (_("Requires authentication"));
	gtk_box_pack_start (GTK_BOX(box), priv->auth, TRUE, FALSE, 6);

	g_signal_connect (priv->auth, "toggled", G_CALLBACK (on_button_toggled), priv);
	
	/* Username */
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), gtk_label_new (_("Username: ")),
			    FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(hbox), priv->username, TRUE, TRUE, 6);
	gtk_widget_set_sensitive (hbox, FALSE);
	gtk_box_pack_start (GTK_BOX(box), hbox, TRUE, TRUE, 6);

	/* Security */
	label = gtk_label_new(NULL);
	gtk_label_set_markup (GTK_LABEL(label),_("<b>Security</b>"));
	gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new(NULL);
	gtk_label_set_text (GTK_LABEL(label),_("Connection type:"));
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	priv->transport_store_protos = modest_protocol_info_get_transport_store_protocol_pair_list ();
	combo  = modest_combo_box_new (priv->transport_store_protos, g_str_equal);
	
	gtk_box_pack_start (GTK_BOX(hbox), combo, FALSE, FALSE,0);
	gtk_box_pack_start (GTK_BOX(box), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 6);
	label = gtk_label_new(NULL);

	gtk_label_set_text (GTK_LABEL(label),_("Authentication:"));
	gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 6);

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	priv->auth_protos = modest_protocol_info_get_auth_protocol_pair_list ();
	combo  = modest_combo_box_new (priv->auth_protos, g_str_equal);
	
	gtk_box_pack_start (GTK_BOX(hbox), combo, FALSE, FALSE, 0);
	priv->remember_pwd =
		gtk_check_button_new_with_label (_("Remember password"));
	gtk_box_pack_start (GTK_BOX(hbox),priv->remember_pwd,
			    FALSE, FALSE, 0);	
	return box;
}


GtkWidget*
modest_transport_widget_new (ModestTransportStoreProtocol proto)
{
	GObject *obj;
	GtkWidget *w;
	ModestTransportWidget *self;
	ModestTransportWidgetPrivate *priv;
	
	g_return_val_if_fail (proto, NULL);

	obj = g_object_new(MODEST_TYPE_TRANSPORT_WIDGET, NULL);
	self = MODEST_TRANSPORT_WIDGET(obj);
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(self);

	priv->proto = proto;
	
	if (proto == MODEST_PROTOCOL_TRANSPORT_SMTP) 
		w = smtp_configuration (self);
	else
		w = gtk_label_new ("");
	
	gtk_widget_show_all (w);
	gtk_box_pack_start (GTK_BOX(self), w, FALSE, FALSE, 2);

	return GTK_WIDGET(self);
}


gboolean
modest_transport_widget_get_remember_password (ModestTransportWidget *self)
{
	ModestTransportWidgetPrivate *priv;

	g_return_val_if_fail (self, FALSE);
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(self);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->remember_pwd));
}

gboolean
modest_transport_widget_get_requires_auth (ModestTransportWidget *self)
{
	ModestTransportWidgetPrivate *priv;

	g_return_val_if_fail (self, FALSE);
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(self);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->auth));
}

const gchar*
modest_transport_widget_get_username (ModestTransportWidget *self)
{
	ModestTransportWidgetPrivate *priv;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(self);
	
	return gtk_entry_get_text (GTK_ENTRY(priv->username));
}

const gchar*
modest_transport_widget_get_servername (ModestTransportWidget *self)
{
	ModestTransportWidgetPrivate *priv;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(self);
	
	return gtk_entry_get_text (GTK_ENTRY(priv->servername));
}


ModestTransportStoreProtocol
modest_transport_widget_get_proto (ModestTransportWidget *self)
{
	ModestTransportWidgetPrivate *priv;

	g_return_val_if_fail (self, MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN);
	priv = MODEST_TRANSPORT_WIDGET_GET_PRIVATE(self);

	return priv->proto;
}


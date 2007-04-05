/* connection-specific-smtp-window.c */

#include "modest-connection-specific-smtp-edit-window.h"
#include <hildon-widgets/hildon-caption.h>
#include "maemo/easysetup/modest-easysetup-country-combo-box.h"
#include "maemo/easysetup/modest-easysetup-provider-combo-box.h"
#include "maemo/easysetup/modest-easysetup-servertype-combo-box.h"
#include "maemo/easysetup/modest-easysetup-serversecurity-combo-box.h"
#include "maemo/easysetup/modest-easysetup-secureauth-combo-box.h"
#include "maemo/easysetup/modest-validating-entry.h"
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkstock.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (ModestConnectionSpecificSmtpEditWindow, modest_connection_specific_smtp_edit_window, GTK_TYPE_WINDOW);

#define CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW, ModestConnectionSpecificSmtpEditWindowPrivate))

typedef struct _ModestConnectionSpecificSmtpEditWindowPrivate ModestConnectionSpecificSmtpEditWindowPrivate;

struct _ModestConnectionSpecificSmtpEditWindowPrivate
{
	GtkWidget *entry_outgoingserver;
	GtkWidget *combo_outgoing_auth;
	GtkWidget *entry_user_username;
	GtkWidget *entry_user_password;
	GtkWidget *combo_outgoing_security;
	GtkWidget *entry_port;
	
	GtkWidget *button_ok;
	GtkWidget *button_cancel;
};

static void
modest_connection_specific_smtp_edit_window_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_connection_specific_smtp_edit_window_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_connection_specific_smtp_edit_window_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_connection_specific_smtp_edit_window_parent_class)->dispose)
		G_OBJECT_CLASS (modest_connection_specific_smtp_edit_window_parent_class)->dispose (object);
}

static void
modest_connection_specific_smtp_edit_window_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_connection_specific_smtp_edit_window_parent_class)->finalize (object);
}

static void
modest_connection_specific_smtp_edit_window_class_init (ModestConnectionSpecificSmtpEditWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestConnectionSpecificSmtpEditWindowPrivate));

	object_class->get_property = modest_connection_specific_smtp_edit_window_get_property;
	object_class->set_property = modest_connection_specific_smtp_edit_window_set_property;
	object_class->dispose = modest_connection_specific_smtp_edit_window_dispose;
	object_class->finalize = modest_connection_specific_smtp_edit_window_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0,
	MODEL_COL_SERVER_NAME = 1,
	MODEL_COL_ID = 2
};

static void
on_button_edit (GtkButton *button, gpointer user_data)
{
	
}

static void
on_button_cancel (GtkButton *button, gpointer user_data)
{
	ModestConnectionSpecificSmtpEditWindow *self = MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (user_data);

	/* Hide the window.
	 * The code that showed it will respond to the hide signal. */	
	gtk_widget_hide (GTK_WIDGET (self));
}

static void
modest_connection_specific_smtp_edit_window_init (ModestConnectionSpecificSmtpEditWindow *self)
{
	ModestConnectionSpecificSmtpEditWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW_GET_PRIVATE (self);
	
	GtkWidget *box = gtk_vbox_new (FALSE, 2);
	
	/* Create a size group to be used by all captions.
	 * Note that HildonCaption does not create a default size group if we do not specify one.
	 * We use GTK_SIZE_GROUP_HORIZONTAL, so that the widths are the same. */
	GtkSizeGroup *sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	 
	/* The outgoing server widgets: */
	if (!priv->entry_outgoingserver)
		priv->entry_outgoingserver = gtk_entry_new ();
	GtkWidget *caption = hildon_caption_new (sizegroup, 
		_("mcen_li_emailsetup_smtp"), priv->entry_outgoingserver, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (priv->entry_outgoingserver);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* The secure authentication widgets: */
	if (!priv->combo_outgoing_auth)
		priv->combo_outgoing_auth = GTK_WIDGET (easysetup_secureauth_combo_box_new ());
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_authentication"), 
		priv->combo_outgoing_auth, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (priv->combo_outgoing_auth);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* The username widgets: */	
	priv->entry_user_username = GTK_WIDGET (easysetup_validating_entry_new ());
	caption = hildon_caption_new (sizegroup, _("mail_fi_username"), 
		priv->entry_user_username, NULL, HILDON_CAPTION_MANDATORY);
	gtk_widget_show (priv->entry_user_username);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* Prevent the use of some characters in the username, 
	 * as required by our UI specification: */
	easysetup_validating_entry_set_unallowed_characters_whitespace (
	 	EASYSETUP_VALIDATING_ENTRY (priv->entry_user_username));
	
	/* Set max length as in the UI spec:
	 * TODO: The UI spec seems to want us to show a dialog if we hit the maximum. */
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_user_username), 64);
	
	/* The password widgets: */	
	priv->entry_user_password = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (priv->entry_user_password), FALSE);
	/* gtk_entry_set_invisible_char (GTK_ENTRY (priv->entry_user_password), '*'); */
	caption = hildon_caption_new (sizegroup, 
		_("mail_fi_password"), priv->entry_user_password, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (priv->entry_user_password);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* The secure connection widgets: */	
	if (!priv->combo_outgoing_security)
		priv->combo_outgoing_security = GTK_WIDGET (easysetup_serversecurity_combo_box_new ());
	easysetup_serversecurity_combo_box_fill (
		EASYSETUP_SERVERSECURITY_COMBO_BOX (priv->combo_outgoing_security), MODEST_PROTOCOL_TRANSPORT_SMTP);
	easysetup_serversecurity_combo_box_set_active_serversecurity (
		EASYSETUP_SERVERSECURITY_COMBO_BOX (priv->combo_outgoing_security), MODEST_PROTOCOL_SECURITY_NONE);
	caption = hildon_caption_new (sizegroup, _("mcen_li_emailsetup_secure_connection"), 
		priv->combo_outgoing_security, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (priv->combo_outgoing_security);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* The port number widgets: */
	if (!priv->entry_port)
		priv->entry_port = gtk_entry_new ();
	caption = hildon_caption_new (sizegroup, 
		_("mcen_li_emailsetup_smtp"), priv->entry_port, NULL, HILDON_CAPTION_OPTIONAL);
	gtk_widget_show (priv->entry_port);
	gtk_box_pack_start (GTK_BOX (box), caption, FALSE, FALSE, 2);
	gtk_widget_show (caption);
	
	/* Add the buttons: */
	GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (box), hbox, TRUE, TRUE, 2);
	gtk_widget_show (hbox);
	
	GtkWidget *button_edit = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_box_pack_start (GTK_BOX (hbox), button_edit, TRUE, FALSE, 2);
	gtk_widget_show (button_edit);
	g_signal_connect (G_OBJECT (button_edit), "clicked",
        	G_CALLBACK (on_button_edit), self);
	
	GtkWidget *button_cancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_box_pack_start (GTK_BOX (hbox), button_cancel, TRUE, FALSE, 2);
	gtk_widget_show (button_cancel);
	g_signal_connect (G_OBJECT (button_edit), "clicked",
        	G_CALLBACK (on_button_cancel), self);
	
	gtk_widget_show (box);
}

ModestConnectionSpecificSmtpEditWindow*
modest_connection_specific_smtp_edit_window_new (void)
{
	return g_object_new (MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW, NULL);
}


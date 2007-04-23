/* connection-specific-smtp-window.c */

#include "modest-signature-editor-dialog.h"
#include "maemo/modest-maemo-ui-constants.h"
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-number-editor.h>
#include "widgets/modest-serversecurity-combo-box.h"
#include "widgets/modest-secureauth-combo-box.h"
#include "widgets/modest-validating-entry.h"
#include <modest-account-mgr-helpers.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktextview.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkstock.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (ModestSignatureEditorDialog, modest_signature_editor_dialog, GTK_TYPE_DIALOG);

#define SIGNATURE_EDITOR_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_SIGNATURE_EDITOR_DIALOG, ModestSignatureEditorDialogPrivate))

typedef struct _ModestSignatureEditorDialogPrivate ModestSignatureEditorDialogPrivate;

struct _ModestSignatureEditorDialogPrivate
{
	GtkWidget *checkbox_use;
	GtkWidget *label;
	GtkWidget *textview;
};

static void
modest_signature_editor_dialog_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_signature_editor_dialog_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_signature_editor_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_signature_editor_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (modest_signature_editor_dialog_parent_class)->dispose (object);
}

static void
modest_signature_editor_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_signature_editor_dialog_parent_class)->finalize (object);
}

static void
modest_signature_editor_dialog_class_init (ModestSignatureEditorDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestSignatureEditorDialogPrivate));

	object_class->get_property = modest_signature_editor_dialog_get_property;
	object_class->set_property = modest_signature_editor_dialog_set_property;
	object_class->dispose = modest_signature_editor_dialog_dispose;
	object_class->finalize = modest_signature_editor_dialog_finalize;
}

static void
modest_signature_editor_dialog_init (ModestSignatureEditorDialog *self)
{
	ModestSignatureEditorDialogPrivate *priv = 
		SIGNATURE_EDITOR_DIALOG_GET_PRIVATE (self);
	
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_email_signatures_edit_title"));
		
	GtkWidget *box = GTK_DIALOG(self)->vbox; /* gtk_vbox_new (FALSE, MODEST_MARGIN_HALF); */
	
	priv->checkbox_use = gtk_check_button_new_with_label (
		_("mcen_fi_email_signatures_use_signature"));
	gtk_box_pack_start (GTK_BOX (box), priv->checkbox_use, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->checkbox_use);
	
	priv->label = gtk_label_new (""); /* Set in modest_signature_editor_dialog_set_settings(). */
	gtk_box_pack_start (GTK_BOX (box), priv->label, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->label);
	
	priv->textview = gtk_text_view_new ();
	gtk_box_pack_start (GTK_BOX (box), priv->textview, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->textview);
	
	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	
	
	gtk_widget_show (box);
}

ModestSignatureEditorDialog*
modest_signature_editor_dialog_new (void)
{
	return g_object_new (MODEST_TYPE_SIGNATURE_EDITOR_DIALOG, NULL);
}

void
modest_signature_editor_dialog_set_settings (
	ModestSignatureEditorDialog *window, gboolean use_signature, const gchar* signature, 
	const gchar* account_title)
{
	ModestSignatureEditorDialogPrivate *priv = 
		SIGNATURE_EDITOR_DIALOG_GET_PRIVATE (window);

	/* This causes a warning because of the %s in the translation, but not in the original string: */
	gchar* label_text = g_strdup_printf (_("mcen_ia_email_signatures_edit_dlg_label"), 
		account_title);
	gtk_label_set_text (GTK_LABEL (priv->label), label_text);
	g_free (label_text);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->checkbox_use), use_signature);
	
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	if (signature)
		gtk_text_buffer_set_text (buffer, signature, -1);
	else
		gtk_text_buffer_set_text (buffer, "", -1);
}

/*
 * The result must be freed with g_free(). */
gchar*
modest_signature_editor_dialog_get_settings (
	ModestSignatureEditorDialog *window, gboolean* use_signature)
{
	ModestSignatureEditorDialogPrivate *priv = 
		SIGNATURE_EDITOR_DIALOG_GET_PRIVATE (window);
		
	g_assert(use_signature);
	
	*use_signature = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->checkbox_use));
			
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	return gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
}

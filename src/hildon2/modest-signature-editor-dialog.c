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

#include "modest-signature-editor-dialog.h"
#include "widgets/modest-ui-constants.h"
#include "modest-hildon-includes.h"
#include "widgets/modest-validating-entry.h"
#include "modest-runtime.h"
#include <modest-account-mgr-helpers.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktextview.h>
#include <gtk/gtklabel.h>
#include <hildon/hildon-pannable-area.h>
#include <gtk/gtkstock.h>
#include <glib/gi18n.h>
#include <modest-maemo-utils.h>
#include "modest-text-utils.h"

G_DEFINE_TYPE (ModestSignatureEditorDialog, modest_signature_editor_dialog, GTK_TYPE_DIALOG);

#define SIGNATURE_EDITOR_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_SIGNATURE_EDITOR_DIALOG, ModestSignatureEditorDialogPrivate))

typedef struct _ModestSignatureEditorDialogPrivate ModestSignatureEditorDialogPrivate;

struct _ModestSignatureEditorDialogPrivate
{
	GtkWidget *checkbox_use;
	GtkWidget *label;
	GtkWidget *pannable;
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
enable_widgets (ModestSignatureEditorDialog *self)
{
	ModestSignatureEditorDialogPrivate *priv = 
		SIGNATURE_EDITOR_DIALOG_GET_PRIVATE (self);
		
	const gboolean enable = hildon_check_button_get_active (HILDON_CHECK_BUTTON (priv->checkbox_use));
	gtk_widget_set_sensitive (priv->label, enable);
	gtk_widget_set_sensitive (priv->pannable, enable);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->textview), enable);
}

static void
on_toggle_button_changed (GtkToggleButton *togglebutton, gpointer user_data)
{
	ModestSignatureEditorDialog *self = MODEST_SIGNATURE_EDITOR_DIALOG (user_data);
	enable_widgets (self);
}

static void
modest_signature_editor_dialog_init (ModestSignatureEditorDialog *self)
{
	ModestSignatureEditorDialogPrivate *priv = 
		SIGNATURE_EDITOR_DIALOG_GET_PRIVATE (self);
	
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_email_signatures_edit_title"));
		
	GtkWidget *box = GTK_DIALOG(self)->vbox; /* gtk_vbox_new (FALSE, MODEST_MARGIN_HALF); */
	gtk_container_set_border_width (GTK_CONTAINER (box), MODEST_MARGIN_HALF);

	priv->checkbox_use = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (priv->checkbox_use), 
			      _("mcen_fi_email_signatures_use_signature"));
	gtk_button_set_alignment (GTK_BUTTON (priv->checkbox_use), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (box), priv->checkbox_use, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->checkbox_use);
	
	g_signal_connect (G_OBJECT (priv->checkbox_use), "toggled",
			  G_CALLBACK (on_toggle_button_changed), self);		
	
	priv->label = gtk_label_new (""); /* Set in modest_signature_editor_dialog_set_settings(). */
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (box), priv->label, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->label);
	
	priv->pannable = hildon_pannable_area_new ();
	gtk_box_pack_start (GTK_BOX (box), priv->pannable, TRUE, TRUE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->pannable);
		
	priv->textview = gtk_text_view_new ();
	gtk_container_add (GTK_CONTAINER (priv->pannable), priv->textview);
	gtk_widget_show (priv->textview);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	gtk_text_buffer_set_text (buffer, _("mcen_va_default_signature_tablet"), -1); /* Default, as per the UI spec. */
	
	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), _HL("wdgt_bd_save"), GTK_RESPONSE_OK);
	
	gtk_widget_show (box);
	gtk_widget_set_size_request (GTK_WIDGET (self), -1, 320);
	
	/* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
	modest_window_mgr_prevent_hibernation_while_window_is_shown (
		modest_runtime_get_window_mgr (), GTK_WINDOW (self)); 
	
	hildon_help_dialog_help_enable (GTK_DIALOG(self), "applications_email_signatureeditor",
					modest_maemo_utils_get_osso_context());
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
	gtk_label_set_ellipsize (GTK_LABEL (priv->label),  PANGO_ELLIPSIZE_END);
	g_free (label_text);
	
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (priv->checkbox_use), use_signature);
	
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	if (signature && signature[0] != '\0')
		gtk_text_buffer_set_text (buffer, signature, -1);
	else
		gtk_text_buffer_set_text (buffer, _("mcen_va_default_signature_tablet"), -1); /* Default, as per the UI spec. */
		
	enable_widgets (window);
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
	
	*use_signature = hildon_check_button_get_active (HILDON_CHECK_BUTTON (priv->checkbox_use));
			
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->textview));
	
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	return gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
}

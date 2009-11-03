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
#include <modest-scrollable.h>
#include <modest-toolkit-factory.h>
#include <gtk/gtkstock.h>
#include <glib/gi18n.h>
#include <modest-maemo-utils.h>
#include "modest-text-utils.h"
#include <hildon/hildon-text-view.h>

G_DEFINE_TYPE (ModestSignatureEditorDialog, modest_signature_editor_dialog, GTK_TYPE_DIALOG);

#define SIGNATURE_EDITOR_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_SIGNATURE_EDITOR_DIALOG, ModestSignatureEditorDialogPrivate))

typedef struct _ModestSignatureEditorDialogPrivate ModestSignatureEditorDialogPrivate;

struct _ModestSignatureEditorDialogPrivate
{
	GtkWidget *checkbox_use;
	GtkWidget *label;
	GtkWidget *scrollable;
	GtkWidget *textview;

	guint correct_scroll_idle;
};

static void text_buffer_end_user_action (GtkTextBuffer *buffer,
					 ModestSignatureEditorDialog *userdata);

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
	ModestSignatureEditorDialogPrivate *priv;

	priv = SIGNATURE_EDITOR_DIALOG_GET_PRIVATE (object);

	if (priv->correct_scroll_idle > 0) {
		g_source_remove (priv->correct_scroll_idle);
		priv->correct_scroll_idle = 0;
	}
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
		
	const gboolean enable = modest_togglable_get_active (priv->checkbox_use);
	gtk_widget_set_sensitive (priv->label, enable);
	gtk_widget_set_sensitive (priv->textview, enable);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->textview), enable);
}

static void
on_toggle_button_changed (GtkWidget *togglebutton, gpointer user_data)
{
	ModestSignatureEditorDialog *self = MODEST_SIGNATURE_EDITOR_DIALOG (user_data);
	enable_widgets (self);
}

static void
modest_signature_editor_dialog_init (ModestSignatureEditorDialog *self)
{
	ModestSignatureEditorDialogPrivate *priv = 
		SIGNATURE_EDITOR_DIALOG_GET_PRIVATE (self);
	GtkWidget *top_box, *align;
	
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_email_signatures_edit_title"));
		
	GtkWidget *box = GTK_DIALOG(self)->vbox; /* gtk_vbox_new (FALSE, MODEST_MARGIN_HALF); */
	top_box = gtk_vbox_new (FALSE, 0);

	priv->checkbox_use = modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
									 _("mcen_fi_email_signatures_use_signature"));
	gtk_box_pack_start (GTK_BOX (top_box), priv->checkbox_use, FALSE, FALSE, 0);
	gtk_widget_show (priv->checkbox_use);
	
	g_signal_connect (G_OBJECT (priv->checkbox_use), "toggled",
			  G_CALLBACK (on_toggle_button_changed), self);		
	
	priv->label = gtk_label_new (""); /* Set in modest_signature_editor_dialog_set_settings(). */
	gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.0);
	gtk_misc_set_padding (GTK_MISC (priv->label), MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE);
	gtk_box_pack_start (GTK_BOX (top_box), priv->label, FALSE, FALSE, 0);
	gtk_widget_show (priv->label);
	
	priv->textview = hildon_text_view_new ();
	gtk_widget_show (priv->textview);
	GtkTextBuffer *buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->textview));
	gtk_text_buffer_set_text (buffer, _("mcen_va_default_signature_tablet"), -1); /* Default, as per the UI spec. */
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->textview), GTK_WRAP_WORD_CHAR);
	gtk_box_pack_start (GTK_BOX (top_box), priv->textview, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (buffer), "end-user-action",
			  G_CALLBACK (text_buffer_end_user_action), self);
	
	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), _HL("wdgt_bd_save"), GTK_RESPONSE_OK);

	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DOUBLE, 0);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), top_box);
	
	gtk_widget_show (top_box);

	priv->scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());
	modest_scrollable_add_with_viewport (MODEST_SCROLLABLE (priv->scrollable), align);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), priv->scrollable);
	gtk_widget_show (priv->scrollable);		

	gtk_widget_show (box);
	gtk_widget_set_size_request (GTK_WIDGET (self), -1, MODEST_DIALOG_WINDOW_MAX_HEIGHT);
	
	/* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
	modest_window_mgr_prevent_hibernation_while_window_is_shown (
		modest_runtime_get_window_mgr (), GTK_WINDOW (self)); 

	priv->correct_scroll_idle = 0;
	
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
	
	modest_togglable_set_active (priv->checkbox_use, use_signature);
	
	GtkTextBuffer *buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->textview));
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
	
	*use_signature = modest_togglable_get_active (priv->checkbox_use);
			
	GtkTextBuffer *buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->textview));
	
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	return gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
}

static gboolean 
correct_scroll_idle_func (gpointer userdata)
{
	ModestSignatureEditorDialog *self = (ModestSignatureEditorDialog *) userdata;
	ModestSignatureEditorDialogPrivate *priv;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GdkRectangle rectangle;
	gint offset_min, offset_max;
	GtkTextMark *insert;
	GtkAdjustment *vadj;

	/* It could happen that the window was already closed */
	if (!GTK_WIDGET_VISIBLE (self))
		return FALSE;

	priv = SIGNATURE_EDITOR_DIALOG_GET_PRIVATE(self);
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->textview));

	insert = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &iter, insert);

	gtk_text_view_get_iter_location (GTK_TEXT_VIEW (priv->textview), &iter, &rectangle);
	offset_min = priv->textview->allocation.y + rectangle.y;
	offset_max = offset_min + rectangle.height;

	vadj = modest_scrollable_get_vadjustment (MODEST_SCROLLABLE (priv->scrollable));
	offset_min = MAX (offset_min - 48, 0);
	offset_max = MIN (offset_max + 48, vadj->upper);

	if ((offset_min < vadj->value) || (offset_max > vadj->value + vadj->page_size)) {
		/* We check if the current center of the visible area is already matching the center
		   of the selection */
		gint offset_center;
		gint center_top, center_bottom;

		offset_center = (offset_min + offset_max) / 2;
		center_top = vadj->value + vadj->page_size / 3;
		center_bottom = vadj->value + vadj->page_size * 2 / 3;

		if ((offset_center < center_top) ||
		    (offset_center > center_bottom))
			modest_scrollable_scroll_to (MODEST_SCROLLABLE (priv->scrollable), -1, offset_center);
	}

	priv->correct_scroll_idle = 0;
	return FALSE;
}

static void correct_scroll (ModestSignatureEditorDialog *self)
{
	ModestSignatureEditorDialogPrivate *priv;

	priv = SIGNATURE_EDITOR_DIALOG_GET_PRIVATE(self);

	if (!gtk_widget_is_focus (priv->textview))
		return;

	if (priv->correct_scroll_idle > 0) {
		return;
	}

	priv->correct_scroll_idle = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
						     (GSourceFunc) correct_scroll_idle_func,
						     g_object_ref (self),
						     g_object_unref);
}

static void
text_buffer_end_user_action (GtkTextBuffer *buffer,
			     ModestSignatureEditorDialog *userdata)
{

	correct_scroll (userdata);
}


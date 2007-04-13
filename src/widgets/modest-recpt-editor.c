/* Copyright (c) 2007, Nokia Corporation
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

#include <config.h>

#include <glib/gi18n-lib.h>

#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkbutton.h>

#include <modest-text-utils.h>
#include <modest-recpt-editor.h>

static GObjectClass *parent_class = NULL;

/* /\* signals *\/ */
/* enum { */
/* 	LAST_SIGNAL */
/* }; */

typedef struct _ModestRecptEditorPrivate ModestRecptEditorPrivate;

struct _ModestRecptEditorPrivate
{
	GtkWidget *text_view;
	GtkWidget *abook_button;
	GtkWidget *scrolled_window;
	gchar *recipients;
};

#define MODEST_RECPT_EDITOR_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_RECPT_EDITOR, ModestRecptEditorPrivate))

/* static guint signals[LAST_SIGNAL] = {0}; */

/* static functions: GObject */
static void modest_recpt_editor_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_recpt_editor_finalize (GObject *object);
static void modest_recpt_editor_class_init (ModestRecptEditorClass *klass);

/**
 * modest_recpt_editor_new:
 *
 * Return value: a new #ModestRecptEditor instance implemented for Gtk+
 **/
GtkWidget*
modest_recpt_editor_new (void)
{
	ModestRecptEditor *self = g_object_new (MODEST_TYPE_RECPT_EDITOR, 
						"homogeneous", FALSE,
						"spacing", 1,
						NULL);

	return GTK_WIDGET (self);
}

void
modest_recpt_editor_set_recipients (ModestRecptEditor *recpt_editor, const gchar *recipients)
{
	ModestRecptEditorPrivate *priv;
	GtkTextBuffer *buffer = NULL;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));

	gtk_text_buffer_set_text (buffer, recipients, -1);
	if (GTK_WIDGET_REALIZED (recpt_editor))
		gtk_widget_queue_resize (GTK_WIDGET (recpt_editor));

}

const gchar *
modest_recpt_editor_get_recipients (ModestRecptEditor *recpt_editor)
{
	ModestRecptEditorPrivate *priv;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;

	g_return_val_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor), NULL);
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	if (priv->recipients != NULL) {
		g_free (priv->recipients);
		priv->recipients = NULL;
	}

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	priv->recipients = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	return priv->recipients;

}

static void
modest_recpt_editor_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestRecptEditorPrivate *priv;
	GtkWidget *abook_icon;

	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (instance);

	priv->abook_button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->abook_button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click (GTK_BUTTON (priv->abook_button), FALSE);
	gtk_button_set_alignment (GTK_BUTTON (priv->abook_button), 1.0, 1.0);
	abook_icon = gtk_image_new_from_icon_name ("qgn_list_gene_contacts", GTK_ICON_SIZE_BUTTON);
	gtk_container_add (GTK_CONTAINER (priv->abook_button), abook_icon);

	priv->text_view = gtk_text_view_new ();
	priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->scrolled_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (priv->scrolled_window), priv->text_view);

	gtk_box_pack_start (GTK_BOX (instance), priv->scrolled_window, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (instance), priv->abook_button, FALSE, FALSE, 0);

	gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (priv->text_view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (priv->text_view), TRUE);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->text_view), TRUE);
	gtk_text_view_set_justification (GTK_TEXT_VIEW (priv->text_view), GTK_JUSTIFY_LEFT);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->text_view), GTK_WRAP_WORD_CHAR);

	return;
}

void
modest_recpt_editor_set_field_size_group (ModestRecptEditor *recpt_editor, GtkSizeGroup *size_group)
{
	ModestRecptEditorPrivate *priv;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	g_return_if_fail (GTK_IS_SIZE_GROUP (size_group));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	gtk_size_group_add_widget (size_group, priv->scrolled_window);
}

static void
modest_recpt_editor_finalize (GObject *object)
{
	(*parent_class->finalize) (object);

	return;
}

static void 
modest_recpt_editor_class_init (ModestRecptEditorClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = modest_recpt_editor_finalize;

	g_type_class_add_private (object_class, sizeof (ModestRecptEditorPrivate));

	return;
}

GType 
modest_recpt_editor_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestRecptEditorClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_recpt_editor_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestRecptEditor),
		  0,      /* n_preallocs */
		  modest_recpt_editor_instance_init    /* instance_init */
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
			"ModestRecptEditor",
			&info, 0);

	}

	return type;
}

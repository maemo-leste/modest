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

//#include <glib/gi18n-lib.h>

#include <string.h>
#include <gtk/gtk.h>

#include <tny-list.h>
#include <tny-simple-list.h>

#include <modest-platform.h>
#include <modest-runtime.h>
#include <modest-attachments-view.h>

static GObjectClass *parent_class = NULL;

/* signals */
enum {
	ACTIVATE_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestAttachmentsViewPriv ModestAttachmentsViewPriv;

struct _ModestAttachmentsViewPriv
{
	TnyMsg *msg;
};

#define MODEST_ATTACHMENTS_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_ATTACHMENTS_VIEW, ModestAttachmentsViewPriv))

static guint signals[LAST_SIGNAL] = {0};


/**
 * modest_attachments_view_new:
 * @msg: a #TnyMsg
 *
 * Constructor for attachments view widget.
 *
 * Return value: a new #ModestAttachmentsView instance implemented for Gtk+
 **/
GtkWidget*
modest_attachments_view_new (TnyMsg *msg)
{
	ModestAttachmentsView *self = g_object_new (MODEST_TYPE_ATTACHMENTS_VIEW, NULL);

	modest_attachments_view_set_message (self, msg);

	return GTK_WIDGET (self);
}

void
modest_attachments_view_set_message (ModestAttachmentsView *attachments_view, TnyMsg *msg)
{
	ModestAttachmentsViewPriv *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (attachments_view);
	TnyList *parts;
	TnyIterator *iter;
	gint index = 0;
	GtkTextBuffer *buffer = NULL;
	gboolean has_first = FALSE;
	GtkTextIter text_iter;
	gint icon_height;

	if (priv->msg) 
		g_object_unref (priv->msg);
	if (msg)
		g_object_ref (G_OBJECT(msg));
	
	priv->msg = msg;
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (attachments_view));
	gtk_text_buffer_set_text (buffer, "", -1);
	gtk_text_buffer_get_end_iter (buffer, &text_iter);

	if (priv->msg == NULL) {
		gtk_widget_hide (GTK_WIDGET (attachments_view));
		return;
	}

	parts = TNY_LIST (tny_simple_list_new ());
	tny_mime_part_get_parts (TNY_MIME_PART (priv->msg), parts);
	iter = tny_list_create_iterator (parts);

	gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON, NULL, &icon_height);

	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part;

		++index;
		part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_attachment (part)) {
			const gchar *filename = tny_mime_part_get_filename (part);
			gchar *file_icon_name = 
				modest_platform_get_file_icon_name (filename, tny_mime_part_get_content_type(part) , NULL);
			GdkPixbuf *pixbuf = NULL;
			GtkTextTag *tag = NULL;

			if (has_first) {
				gtk_text_buffer_insert (buffer, &text_iter, ", ", -1);
				gtk_text_buffer_get_end_iter (buffer, &text_iter);
			}
			
			tag = gtk_text_buffer_create_tag (buffer, NULL,
							  "underline", PANGO_UNDERLINE_SINGLE,
							  "foreground", "blue",
							  NULL);
			
			g_object_set_data (G_OBJECT (tag), "attachment-index", GINT_TO_POINTER (index));
			g_object_set_data (G_OBJECT (tag), "attachment-set", GINT_TO_POINTER (TRUE));
			
			if (file_icon_name) {
				pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), file_icon_name, 
								   icon_height, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
				if (pixbuf) {
					GtkTextTag *pixbuf_tag;
					GtkTextIter iter2;
					pixbuf_tag = gtk_text_buffer_create_tag (buffer, NULL, NULL);
					g_object_set_data (G_OBJECT (pixbuf_tag), "attachment-index", GINT_TO_POINTER (index));
					g_object_set_data (G_OBJECT (pixbuf_tag), "attachment-set", GINT_TO_POINTER (TRUE));
					gtk_text_buffer_insert_pixbuf (buffer, &text_iter, pixbuf);
					iter2 = text_iter;
					gtk_text_iter_backward_char (&iter2);
					gtk_text_buffer_apply_tag (buffer, pixbuf_tag, &iter2, &text_iter);
					gtk_text_buffer_get_end_iter (buffer, &text_iter);
				}
			}
			gtk_text_buffer_insert_with_tags (buffer, &text_iter, filename, -1, tag, NULL);
			gtk_text_buffer_get_end_iter (buffer, &text_iter);
			if (file_icon_name)
				g_free (file_icon_name);
			has_first = TRUE;
		}
		g_object_unref (part);
		tny_iterator_next (iter);
	}

	if (has_first)
		gtk_widget_show (GTK_WIDGET (attachments_view));
	else
		gtk_widget_hide (GTK_WIDGET (attachments_view));

}

static gboolean
button_release_event (GtkWidget *widget,
		      GdkEventButton *event,
		      gpointer user_data)
{
	gint buffer_x, buffer_y;
	GtkTextIter iter;
	GSList *tags = NULL;
	GSList *node = NULL;
	
	if ((event->type != GDK_BUTTON_RELEASE) 
	    || (event->button != 1))
		return FALSE;
	
	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget), GTK_TEXT_WINDOW_WIDGET,
					       event->x, event->y, &buffer_x, &buffer_y);
	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget), &iter, buffer_x, buffer_y);

	tags = gtk_text_iter_get_tags (&iter);

	for (node = tags; node != NULL; node = g_slist_next (node)) {
		GtkTextTag *tag = node->data;
		gboolean is_attachment = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "attachment-set"));

		if (is_attachment) {
			gint attachment_index = 0;

			attachment_index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "attachment-index"));
			g_signal_emit (G_OBJECT (widget), signals[ACTIVATE_SIGNAL], 0,
				       attachment_index);
			break;
		}
		
	}
	return FALSE;
}

static void
modest_attachments_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestAttachmentsViewPriv *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (instance);
	GtkTextBuffer *buffer = NULL;

	gtk_text_view_set_editable (GTK_TEXT_VIEW (instance), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (instance), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (instance), 0);
	gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (instance), 0);
	gtk_text_view_set_justification (GTK_TEXT_VIEW (instance), GTK_JUSTIFY_LEFT);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (instance), 0);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (instance), 0);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (instance), FALSE);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (instance));

	g_signal_connect (G_OBJECT (instance), "button-release-event", G_CALLBACK (button_release_event), NULL);

	priv->msg = NULL;

	return;
}

static void
modest_attachments_view_finalize (GObject *object)
{
	ModestAttachmentsViewPriv *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (object);

	if (priv->msg) {
		g_object_unref (priv->msg);
		priv->msg = NULL;
	}

	(*parent_class->finalize) (object);

	return;
}

static void 
modest_attachments_view_class_init (ModestAttachmentsViewClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = modest_attachments_view_finalize;

	klass->activate = NULL;

	g_type_class_add_private (object_class, sizeof (ModestAttachmentsViewPriv));

 	signals[ACTIVATE_SIGNAL] =
 		g_signal_new ("activate",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET(ModestAttachmentsViewClass, activate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);
	
	return;
}

GType 
modest_attachments_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestAttachmentsViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_attachments_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestAttachmentsView),
		  0,      /* n_preallocs */
		  modest_attachments_view_instance_init    /* instance_init */
		};

		type = g_type_register_static (GTK_TYPE_TEXT_VIEW,
			"ModestAttachmentsView",
			&info, 0);

	}

	return type;
}

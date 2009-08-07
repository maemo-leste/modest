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

#include <gtk/gtk.h>

#include <modest-scroll-text.h>

#define MODEST_SCROLL_TEXT_DEFAULT_LINE_LIMIT 2

static GObjectClass *parent_class = NULL;

typedef struct _ModestScrollTextPriv ModestScrollTextPriv;

struct _ModestScrollTextPriv
{
	GtkWidget *text_view;
	gint line_height;
	guint line_limit;
};

#define MODEST_SCROLL_TEXT_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_SCROLL_TEXT, ModestScrollTextPriv))


/**
 * modest_scroll_text_new:
 *
 * Return value: a new #ModestScrollText instance implemented for Gtk+
 **/
GtkWidget*
modest_scroll_text_new (GtkTextView *text_view, guint line_limit)
{
	ModestScrollText *self = g_object_new (MODEST_TYPE_SCROLL_TEXT, NULL);
	modest_scroll_text_set_line_limit (self, line_limit);
	modest_scroll_text_set_text_view (self, text_view);

	return GTK_WIDGET (self);
}

static void
size_request (GtkWidget *widget,
	      GtkRequisition *requisition,
	      gpointer user_data)
{
	ModestScrollTextPriv *priv = MODEST_SCROLL_TEXT_GET_PRIVATE (widget);
	const GtkWidget *text_view = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;
	guint line;
	guint line_limit;
	GdkRectangle iter_rectangle;
	GtkAdjustment *adj = NULL;
	GtkTextMark *insert_mark;
	GtkTextIter insert_iter;

	text_view = modest_scroll_text_get_text_view (MODEST_SCROLL_TEXT (widget));
	line_limit = priv->line_limit;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

	insert_mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, insert_mark);

	/* get the first line and the height of the line */
	gtk_text_buffer_get_start_iter (buffer, &iter);
	gtk_text_view_get_iter_location (GTK_TEXT_VIEW (text_view), &iter, &iter_rectangle);

	/* Count lines in text view */
	for (line = 0; line < line_limit; line++) {
		if (!gtk_text_view_forward_display_line_end (GTK_TEXT_VIEW (text_view), &iter))
			break;
		else 
			gtk_text_view_forward_display_line (GTK_TEXT_VIEW (text_view), &iter);
	}

	/* Change the adjustment properties for one line per step behavior */
	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (widget));
	if (adj != NULL) {
		g_object_set (G_OBJECT (adj), "page-increment", (gdouble) iter_rectangle.height + 1, "step-increment", (gdouble) iter_rectangle.height + 1, NULL);
		gtk_adjustment_changed (adj);
	}

	/* Set the requisition height to the get the limit of lines or less */
	if (line > 0) {
		requisition->height = iter_rectangle.height * MIN (line + 1, line_limit);
	} else {
		requisition->height = iter_rectangle.height;
	}

	if (gtk_scrolled_window_get_shadow_type (GTK_SCROLLED_WINDOW (widget)) != GTK_SHADOW_NONE) {
		requisition->height += GTK_WIDGET (widget)->style->ythickness * 2;
	}
		
	priv->line_height = iter_rectangle.height;

	/* Put again the cursor in the first character. Also scroll to first line */
	gtk_text_buffer_place_cursor (buffer, &insert_iter);
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (text_view), insert_mark);

}

static void
size_allocate (GtkWidget *widget,
	       GtkAllocation *allocation,
	       gpointer user_data)
{
	GtkAdjustment *adj = NULL;
	ModestScrollTextPriv *priv = MODEST_SCROLL_TEXT_GET_PRIVATE (widget);

	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (widget));
	if (adj != NULL) {		
		g_object_set (G_OBJECT (adj), "page-increment", (gdouble) priv->line_height, "step-increment", (gdouble) priv->line_height, NULL);
	}
	gtk_adjustment_changed (adj);
}

void 
modest_scroll_text_set_line_limit (ModestScrollText *scroll_text, guint line_limit)
{
	ModestScrollTextPriv *priv = MODEST_SCROLL_TEXT_GET_PRIVATE (scroll_text);

	if (line_limit == priv->line_limit)
		return;

	priv->line_limit = line_limit;
	if (GTK_WIDGET_REALIZED (scroll_text)) {
		gtk_widget_queue_resize (GTK_WIDGET (scroll_text));
	}
}

const GtkWidget *
modest_scroll_text_get_text_view (ModestScrollText *scroll_text)
{
	ModestScrollTextPriv *priv = MODEST_SCROLL_TEXT_GET_PRIVATE (scroll_text);

	if (priv->text_view == NULL)
		modest_scroll_text_set_text_view (scroll_text, NULL);

	return priv->text_view;
}

void
modest_scroll_text_set_text_view (ModestScrollText *scroll_text,
				  GtkTextView *text_view)
{
	ModestScrollTextPriv *priv = MODEST_SCROLL_TEXT_GET_PRIVATE (scroll_text);
	GtkStyle *style;

	g_return_if_fail (MODEST_IS_SCROLL_TEXT (scroll_text));
	if (text_view == NULL) {
		text_view = GTK_TEXT_VIEW(gtk_text_view_new ());
		gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (text_view), FALSE);
		gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
	}

	if (priv->text_view == GTK_WIDGET(text_view))
		return;

	if (priv->text_view != NULL) {
		gtk_container_remove (GTK_CONTAINER(scroll_text), priv->text_view);
		priv->text_view = NULL;
	}

	priv->text_view = GTK_WIDGET(text_view);

	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->text_view), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (priv->text_view), 0);
	gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (priv->text_view), 0);
	gtk_text_view_set_justification (GTK_TEXT_VIEW (priv->text_view), GTK_JUSTIFY_LEFT);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (priv->text_view), 0);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (priv->text_view), 0);

	style = gtk_rc_get_style (GTK_WIDGET (scroll_text));
	gtk_widget_modify_base (priv->text_view, GTK_STATE_NORMAL, & (style->bg[GTK_STATE_NORMAL]));

	gtk_container_add (GTK_CONTAINER (scroll_text), priv->text_view);

	if (GTK_WIDGET_REALIZED (scroll_text)) {
		gtk_widget_queue_resize (GTK_WIDGET (scroll_text));
	}
}

static void
modest_scroll_text_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestScrollTextPriv *priv = MODEST_SCROLL_TEXT_GET_PRIVATE (instance);

	gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (instance), NULL);
	gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (instance), NULL);

	priv->line_limit = MODEST_SCROLL_TEXT_DEFAULT_LINE_LIMIT;

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (instance), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	g_signal_connect (G_OBJECT (instance), "size-request", G_CALLBACK (size_request), NULL);
	g_signal_connect (G_OBJECT (instance), "size-allocate", G_CALLBACK (size_allocate), NULL);

	return;
}

static void
modest_scroll_text_finalize (GObject *object)
{
	(*parent_class->finalize) (object);

	return;
}

static void 
modest_scroll_text_class_init (ModestScrollTextClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = modest_scroll_text_finalize;

	g_type_class_add_private (object_class, sizeof (ModestScrollTextPriv));

	return;
}

GType 
modest_scroll_text_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestScrollTextClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_scroll_text_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestScrollText),
		  0,      /* n_preallocs */
		  modest_scroll_text_instance_init    /* instance_init */
		};

		type = g_type_register_static (GTK_TYPE_SCROLLED_WINDOW,
			"ModestScrollText",
			&info, 0);

	}

	return type;
}

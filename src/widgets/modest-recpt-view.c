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

#include <modest-text-utils.h>
#include <modest-recpt-view.h>

#define RECPT_VIEW_CLICK_AREA_THRESHOLD 32

static GObjectClass *parent_class = NULL;

/* signals */
enum {
	ACTIVATE_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestRecptViewPriv ModestRecptViewPriv;

struct _ModestRecptViewPriv
{
	gboolean button_pressed;
	gdouble pressed_x, pressed_y;
};

#define MODEST_RECPT_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_RECPT_VIEW, ModestRecptViewPriv))

static guint signals[LAST_SIGNAL] = {0};

/* static functions: GObject */
static void modest_recpt_view_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_recpt_view_finalize (GObject *object);
static void modest_recpt_view_class_init (ModestRecptViewClass *klass);
/* static functions: GtkWidget */
static gint button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gint button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);

/**
 * modest_recpt_view_new:
 *
 * Return value: a new #ModestRecptView instance implemented for Gtk+
 **/
GtkWidget*
modest_recpt_view_new (void)
{
	ModestRecptView *self = g_object_new (MODEST_TYPE_RECPT_VIEW, NULL);

	return GTK_WIDGET (self);
}

void
modest_recpt_view_set_recipients (ModestRecptView *recpt_view, const gchar *recipients)
{
	const GtkWidget *text_view = NULL;
	GtkTextBuffer *buffer = NULL;
	gchar *std_recipients;

	text_view = modest_scroll_text_get_text_view (MODEST_SCROLL_TEXT (recpt_view));
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

	if (recipients == NULL) {
		std_recipients = NULL;
	} else {
		std_recipients = modest_text_utils_address_with_standard_length (recipients);
	}

	gtk_text_buffer_set_text (buffer, std_recipients, -1);
	g_free (std_recipients);
	if (GTK_WIDGET_REALIZED (recpt_view))
		gtk_widget_queue_resize (GTK_WIDGET (recpt_view));

}

static gint
button_press_event (GtkWidget *widget,
		    GdkEventButton *event,
		    gpointer user_data)
{
	ModestRecptViewPriv *priv = MODEST_RECPT_VIEW_GET_PRIVATE (MODEST_RECPT_VIEW (user_data));

	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		priv->button_pressed = TRUE;
		priv->pressed_x = event->x;
		priv->pressed_y = event->y;
	}
	return TRUE;
}

static gint
button_release_event (GtkWidget *widget,
		      GdkEventButton *event,
		      gpointer user_data)
{
	ModestRecptViewPriv *priv = MODEST_RECPT_VIEW_GET_PRIVATE (MODEST_RECPT_VIEW (user_data));
	const GtkWidget *text_view = NULL;

	text_view = modest_scroll_text_get_text_view (MODEST_SCROLL_TEXT (user_data));

	if (event->type != GDK_BUTTON_RELEASE)
		return TRUE;

	if ((priv->button_pressed) &&
	    (event->type == GDK_BUTTON_RELEASE) &&
	    ((event->x >= priv->pressed_x - RECPT_VIEW_CLICK_AREA_THRESHOLD)&&
	     (event->x <= priv->pressed_x + RECPT_VIEW_CLICK_AREA_THRESHOLD)) &&
	    ((event->y >= priv->pressed_y - RECPT_VIEW_CLICK_AREA_THRESHOLD)&&
	     (event->y <= priv->pressed_y + RECPT_VIEW_CLICK_AREA_THRESHOLD))) {
		priv->button_pressed = FALSE;
		if (event->button == 1) {
			gint buffer_x, buffer_y;
			int index;
			GtkTextIter iter;
			gtk_widget_grab_focus (GTK_WIDGET (text_view));
			gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), GTK_TEXT_WINDOW_WIDGET,
							       event->x, event->y, &buffer_x, &buffer_y);
			gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (text_view), &iter, buffer_x, buffer_y);
			index = gtk_text_iter_get_offset (&iter);
			
			if (!gtk_text_iter_is_end (&iter)) {
				guint selection_start, selection_end;
				gboolean selected = FALSE;
				GtkTextIter start_iter, end_iter;
				GtkTextBuffer *buffer;

				buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
				if (gtk_text_buffer_get_selection_bounds (buffer, &start_iter, &end_iter) &&
				    gtk_text_iter_in_range (&iter, &start_iter, &end_iter)) {
					selected = TRUE;
				} else {
					gchar *text = NULL;
					GtkTextIter start_iter, end_iter;

					gtk_text_buffer_get_start_iter (buffer, &start_iter);
					gtk_text_buffer_get_end_iter (buffer, &end_iter);
					text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);

					/* text will not be NULL, but source code checkers should be satisfied */
					if (text) {
						modest_text_utils_address_range_at_position (text,
											     index,
											     &selection_start, &selection_end);
						/* TODO: now gtk label tries to select more than the label as usual,
						 *  and we force it to recover the selected region for the defined area.
						 *  It should be fixed (maybe preventing gtklabel to manage selections
						 *  in parallel with us
						 */
						gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, selection_start);
						gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, selection_end);
						gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);
						
						g_free (text);
					}		      
				}
				
				if (selected) {
					gchar *selection;

					gtk_text_buffer_get_selection_bounds (buffer, &start_iter, &end_iter);
					selection = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
					g_signal_emit (G_OBJECT (user_data), signals[ACTIVATE_SIGNAL], 0, selection);
					g_free (selection);
				}

			}
			return TRUE;
		}
	}
	priv->button_pressed = FALSE;
	return TRUE;
}

static void
modest_recpt_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	const GtkTextView *text_view = NULL;

	text_view = GTK_TEXT_VIEW(modest_scroll_text_get_text_view (MODEST_SCROLL_TEXT (instance)));

	g_signal_connect (G_OBJECT (text_view), "button-press-event", G_CALLBACK (button_press_event), instance);
	g_signal_connect_after (G_OBJECT (text_view), "button-release-event", G_CALLBACK (button_release_event), instance);

	return;
}

static void
modest_recpt_view_finalize (GObject *object)
{
	(*parent_class->finalize) (object);

	return;
}

static void 
modest_recpt_view_class_init (ModestRecptViewClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = modest_recpt_view_finalize;

	klass->activate = NULL;

	g_type_class_add_private (object_class, sizeof (ModestRecptViewPriv));

 	signals[ACTIVATE_SIGNAL] =
 		g_signal_new ("activate",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET(ModestRecptViewClass, activate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	
	return;
}

GType 
modest_recpt_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestRecptViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_recpt_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestRecptView),
		  0,      /* n_preallocs */
		  modest_recpt_view_instance_init    /* instance_init */
		};

		type = g_type_register_static (MODEST_TYPE_SCROLL_TEXT,
			"ModestRecptView",
			&info, 0);

	}

	return type;
}

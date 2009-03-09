/*
 * Copyright (C) 2009 Nokia Corporation, all rights reserved.
 *
 */

#include                                        "modest-wp-text-view.h"
#include <math.h>

#define MODEST_WP_TEXT_VIEW_DRAG_THRESHOLD 16.0

G_DEFINE_TYPE                                   (ModestWpTextView, modest_wp_text_view, WP_TYPE_TEXT_VIEW);

#define                                         MODEST_WP_TEXT_VIEW_GET_PRIVATE(obj) \
                                                (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                MODEST_TYPE_WP_TEXT_VIEW, ModestWpTextViewPrivate));

typedef struct                                  _ModestWpTextViewPrivate ModestWpTextViewPrivate;

struct                                          _ModestWpTextViewPrivate
{
    gdouble x;                                                      /* tap x position */
    gdouble y;                                                      /* tap y position */
};

GtkWidget *
modest_wp_text_view_new                            (void)
{
    GtkWidget *entry = g_object_new (MODEST_TYPE_WP_TEXT_VIEW, NULL);

    return entry;
}

static gint
modest_wp_text_view_button_press_event (GtkWidget        *widget,
					GdkEventButton   *event)
{
    ModestWpTextViewPrivate *priv = MODEST_WP_TEXT_VIEW_GET_PRIVATE (widget);

    if (GTK_TEXT_VIEW (widget)->editable &&
        hildon_gtk_im_context_filter_event (GTK_TEXT_VIEW (widget)->im_context, (GdkEvent*)event)) {
        GTK_TEXT_VIEW (widget)->need_im_reset = TRUE;
        return TRUE;
    }

    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        priv->x = event->x;
        priv->y = event->y;

        return TRUE;
    }

    return FALSE;
}

static gint
modest_wp_text_view_button_release_event (GtkWidget        *widget,
					  GdkEventButton   *event)
{
    GtkTextView *text_view = GTK_TEXT_VIEW (widget);
    ModestWpTextViewPrivate *priv = MODEST_WP_TEXT_VIEW_GET_PRIVATE (widget);
    GtkTextIter iter;
    gint x, y;

    if (text_view->editable &&
        hildon_gtk_im_context_filter_event (text_view->im_context, (GdkEvent*)event)) {
        text_view->need_im_reset = TRUE;
        return TRUE;
    }

    if (event->button == 1 && event->type == GDK_BUTTON_RELEASE) {
        if (fabs (priv->x - event->x) < MODEST_WP_TEXT_VIEW_DRAG_THRESHOLD &&
            fabs (priv->y - event->y) < MODEST_WP_TEXT_VIEW_DRAG_THRESHOLD) {
            GtkTextWindowType window_type;
	    GtkTextBuffer *buffer;

            window_type = gtk_text_view_get_window_type (text_view, event->window);
            gtk_text_view_window_to_buffer_coords (text_view,
                                                   window_type,
                                                   event->x, event->y,
                                                   &x, &y);
            gtk_text_view_get_iter_at_location (text_view, &iter, x, y);
	    buffer = gtk_text_view_get_buffer (text_view);
            if (gtk_text_buffer_get_char_count (buffer))
                gtk_text_buffer_place_cursor (buffer, &iter);

            gtk_widget_grab_focus (GTK_WIDGET (text_view));

            return TRUE;
        }
    }
    return FALSE;
}

static void
modest_wp_text_view_finalize                       (GObject *object)
{
    if (G_OBJECT_CLASS (modest_wp_text_view_parent_class)->finalize)
        G_OBJECT_CLASS (modest_wp_text_view_parent_class)->finalize (object);
}

static void
modest_wp_text_view_class_init                     (ModestWpTextViewClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *)klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

    gobject_class->finalize = modest_wp_text_view_finalize;
    widget_class->motion_notify_event = NULL;
    widget_class->button_press_event = modest_wp_text_view_button_press_event;
    widget_class->button_release_event = modest_wp_text_view_button_release_event;

    g_type_class_add_private (klass, sizeof (ModestWpTextViewPrivate));
}

static void
modest_wp_text_view_init                           (ModestWpTextView *self)
{
}

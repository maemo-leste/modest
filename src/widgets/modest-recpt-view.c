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

#include <string.h>
#include <gtk/gtk.h>

#include "modest-recpt-view.h"

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

static void
address_bounds_at_position (const gchar *recipients_list, gint position, gint *start, gint *end)
{
	gchar *current = NULL;
	gint range_start = 0;
	gint range_end = 0;
	gint index;
	gboolean is_quoted = FALSE;

	index = 0;
	for (current = (gchar *) recipients_list; *current != '\0'; current = g_utf8_find_next_char (current, NULL)) {
		gunichar c = g_utf8_get_char (current);

		if ((c == ',') && (!is_quoted)) {
			if (index < position) {
				range_start = index + 1;
			} else {
				break;
			}
		} else if (c == '\"') {
			is_quoted = !is_quoted;
		} else if ((c == ' ') &&(range_start == index)) {
			range_start ++;
		}
		index ++;
		range_end = index;
	}

	if (start)
		*start = range_start;
	if (end)
		*end = range_end;
}

static gboolean
button_press_event (GtkWidget *widget,
		    GdkEventButton *event,
		    gpointer user_data)
{
	ModestRecptViewPriv *priv = MODEST_RECPT_VIEW_GET_PRIVATE (MODEST_RECPT_VIEW (widget));

	if (!gtk_label_get_selectable (GTK_LABEL (widget)))
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS) {
		priv->button_pressed = TRUE;
		priv->pressed_x = event->x;
		priv->pressed_y = event->y;
	}
	return FALSE;
}

static gboolean
button_release_event (GtkWidget *widget,
		     GdkEventButton *event,
		     gpointer user_data)
{
	ModestRecptViewPriv *priv = MODEST_RECPT_VIEW_GET_PRIVATE (MODEST_RECPT_VIEW (widget));

	if (!gtk_label_get_selectable (GTK_LABEL (widget)))
		return TRUE;

	if (event->type != GDK_BUTTON_RELEASE)
		return TRUE;

	if ((priv->button_pressed) &&
	    (event->type == GDK_BUTTON_RELEASE) && 
	    (priv->pressed_x == event->x) &&
	    (priv->pressed_y == event->y)) {
		priv->button_pressed = FALSE;
		if (event->button == 1) {
			PangoLayout *layout = NULL;
			int index;
			layout = gtk_label_get_layout (GTK_LABEL (widget));
			if (pango_layout_xy_to_index (layout, event->x*PANGO_SCALE, event->y*PANGO_SCALE, &index, NULL)) {
				int selection_start, selection_end;
				gboolean selected = FALSE;
				if (gtk_label_get_selection_bounds (GTK_LABEL (widget),
								    &selection_start,
								    &selection_end) &&
				    (index >= selection_start)&&(index < selection_end)) {
					selected = TRUE;
				}

				address_bounds_at_position (gtk_label_get_text (GTK_LABEL (widget)),
							    index,
							    &selection_start, &selection_end);
				/* TODO: now gtk label tries to select more than the label as usual,
				 *  and we force it to recover the selected region for the defined area.
				 *  It should be fixed (maybe preventing gtklabel to manage selections
				 *  in parallel with us
				 */
				gtk_label_select_region (GTK_LABEL (widget), 
							 selection_start,
							 selection_end);
				
				if (selected)
					g_signal_emit (G_OBJECT (widget), signals[ACTIVATE_SIGNAL], 0);

				return TRUE;
			}
		}
	}
	priv->button_pressed = FALSE;
	return TRUE;
}

static void
modest_recpt_view_instance_init (GTypeInstance *instance, gpointer g_class)
{

	gtk_label_set_justify (GTK_LABEL (instance), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (instance), TRUE);
	gtk_label_set_selectable (GTK_LABEL (instance), TRUE);

	g_signal_connect (G_OBJECT (instance), "button-press-event", G_CALLBACK(button_press_event), NULL);
	g_signal_connect (G_OBJECT (instance), "button-release-event", G_CALLBACK(button_release_event), NULL);

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
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	
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

		type = g_type_register_static (GTK_TYPE_LABEL,
			"ModestRecptView",
			&info, 0);

	}

	return type;
}

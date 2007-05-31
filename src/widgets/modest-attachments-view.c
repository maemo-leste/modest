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
#include <gdk/gdkkeysyms.h>
#include <tny-list.h>
#include <tny-simple-list.h>

#include <modest-platform.h>
#include <modest-runtime.h>
#include <modest-attachment-view.h>
#include <modest-attachments-view.h>

static GObjectClass *parent_class = NULL;

/* signals */
enum {
	ACTIVATE_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestAttachmentsViewPrivate ModestAttachmentsViewPrivate;

struct _ModestAttachmentsViewPrivate
{
	TnyMsg *msg;
	GtkWidget *box;
	GList *selected;
	GtkWidget *rubber_start;
};

#define MODEST_ATTACHMENTS_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_ATTACHMENTS_VIEW, ModestAttachmentsViewPrivate))

static gboolean button_press_event (GtkWidget *widget, GdkEventButton *event, ModestAttachmentsView *atts_view);
static gboolean motion_notify_event (GtkWidget *widget, GdkEventMotion *event, ModestAttachmentsView *atts_view);
static gboolean button_release_event (GtkWidget *widget, GdkEventButton *event, ModestAttachmentsView *atts_view);
static gboolean key_press_event (GtkWidget *widget, GdkEventKey *event, ModestAttachmentsView *atts_view);
static GtkWidget *get_att_view_at_coords (ModestAttachmentsView *atts_view,
					  gdouble x, gdouble y);
static void unselect_all (ModestAttachmentsView *atts_view);
static void set_selected (ModestAttachmentsView *atts_view, ModestAttachmentView *att_view);
static void select_range (ModestAttachmentsView *atts_view, ModestAttachmentView *att1, ModestAttachmentView *att2);
static void clipboard_get (GtkClipboard *clipboard, GtkSelectionData *selection_data,
			   guint info, gpointer userdata);
static void clipboard_clear (GtkClipboard *clipboard, gpointer userdata);
static void own_clipboard (ModestAttachmentsView *atts_view);

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
	ModestAttachmentsView *self = g_object_new (MODEST_TYPE_ATTACHMENTS_VIEW, 
						    "resize-mode", GTK_RESIZE_PARENT,
						    NULL);

	modest_attachments_view_set_message (self, msg);

	return GTK_WIDGET (self);
}

void
modest_attachments_view_set_message (ModestAttachmentsView *attachments_view, TnyMsg *msg)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (attachments_view);
	TnyList *parts;
	TnyIterator *iter;
	
	if (msg == priv->msg) return;

	if (priv->msg) 
		g_object_unref (priv->msg);
	if (msg)
		g_object_ref (G_OBJECT(msg));
	
	priv->msg = msg;

	g_list_free (priv->selected);
	priv->selected = NULL;

	gtk_container_foreach (GTK_CONTAINER (priv->box), (GtkCallback) gtk_widget_destroy, NULL);
	
	if (priv->msg == NULL) {
		return;
	}

	parts = TNY_LIST (tny_simple_list_new ());
	tny_mime_part_get_parts (TNY_MIME_PART (priv->msg), parts);
	iter = tny_list_create_iterator (parts);

	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part;

		part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_attachment (part) || TNY_IS_MSG (part)) {
			modest_attachments_view_add_attachment (attachments_view, part);
		}
		g_object_unref (part);
		tny_iterator_next (iter);
	}

	gtk_widget_queue_draw (GTK_WIDGET (attachments_view));

}

void
modest_attachments_view_add_attachment (ModestAttachmentsView *attachments_view, TnyMimePart *part)
{
	GtkWidget *att_view = NULL;
	ModestAttachmentsViewPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (attachments_view));
	g_return_if_fail (TNY_IS_MIME_PART (part));

	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (attachments_view);

	att_view = modest_attachment_view_new (part);
	gtk_box_pack_end (GTK_BOX (priv->box), att_view, FALSE, FALSE, 0);
	gtk_widget_show_all (att_view);
}

void
modest_attachments_view_remove_attachment (ModestAttachmentsView *atts_view, TnyMimePart *mime_part)
{
	ModestAttachmentsViewPrivate *priv = NULL;
	GList *box_children = NULL, *node = NULL;
	ModestAttachmentView *found_att_view = NULL;

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (atts_view));
	g_return_if_fail (TNY_IS_MIME_PART (mime_part));

	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	box_children = gtk_container_get_children (GTK_CONTAINER (priv->box));

	for (node = box_children; node != NULL; node = g_list_next (node)) {
		ModestAttachmentView *att_view = (ModestAttachmentView *) node->data;
		TnyMimePart *cur_mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));

		if (mime_part == cur_mime_part)
			found_att_view = att_view;

		g_object_unref (cur_mime_part);

		if (found_att_view != NULL)
			break;
	}

	if (found_att_view) {
		gtk_widget_destroy (GTK_WIDGET (found_att_view));
	}

}

void
modest_attachments_view_remove_attachment_by_id (ModestAttachmentsView *atts_view, const gchar *att_id)
{
	ModestAttachmentsViewPrivate *priv = NULL;
	GList *box_children = NULL, *node = NULL;

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (atts_view));
	g_return_if_fail (att_id != NULL);

	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	box_children = gtk_container_get_children (GTK_CONTAINER (priv->box));

	for (node = box_children; node != NULL; node = g_list_next (node)) {
		ModestAttachmentView *att_view = (ModestAttachmentView *) node->data;
		TnyMimePart *cur_mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
		const gchar *mime_part_id = NULL;

		mime_part_id = tny_mime_part_get_content_id (cur_mime_part);
		if ((mime_part_id != NULL) && (strcmp (mime_part_id, att_id) == 0))
			gtk_widget_destroy (GTK_WIDGET (att_view));

		g_object_unref (cur_mime_part);
	}

}

static void
modest_attachments_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (instance);

	priv->msg = NULL;
	priv->box = gtk_vbox_new (FALSE, 0);
	priv->rubber_start = NULL;
	priv->selected = NULL;

	gtk_container_add (GTK_CONTAINER (instance), priv->box);
	gtk_event_box_set_above_child (GTK_EVENT_BOX (instance), TRUE);

	g_signal_connect (G_OBJECT (instance), "button-press-event", G_CALLBACK (button_press_event), instance);
	g_signal_connect (G_OBJECT (instance), "button-release-event", G_CALLBACK (button_release_event), instance);
	g_signal_connect (G_OBJECT (instance), "motion-notify-event", G_CALLBACK (motion_notify_event), instance);
	g_signal_connect (G_OBJECT (instance), "key-press-event", G_CALLBACK (key_press_event), instance);

	GTK_WIDGET_SET_FLAGS (instance, GTK_CAN_FOCUS);

	return;
}

static void
modest_attachments_view_finalize (GObject *object)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (object);

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

	g_type_class_add_private (object_class, sizeof (ModestAttachmentsViewPrivate));

 	signals[ACTIVATE_SIGNAL] =
 		g_signal_new ("activate",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET(ModestAttachmentsViewClass, activate),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, G_TYPE_OBJECT);
	
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

		type = g_type_register_static (GTK_TYPE_EVENT_BOX,
			"ModestAttachmentsView",
			&info, 0);

	}

	return type;
}

/* buttons signal events */
static gboolean
button_press_event (GtkWidget *widget, 
		    GdkEventButton *event,
		    ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	if (!GTK_WIDGET_HAS_FOCUS (widget))
		gtk_widget_grab_focus (widget);

	if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
		GtkWidget *att_view = get_att_view_at_coords (MODEST_ATTACHMENTS_VIEW (widget), 
							      event->x, event->y);

		if (att_view != NULL) {
			if (GTK_WIDGET_STATE (att_view) == GTK_STATE_SELECTED && (g_list_length (priv->selected) < 2)) {
				TnyMimePart *mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
				if (TNY_IS_MIME_PART (mime_part)) {
					g_signal_emit (G_OBJECT (widget), signals[ACTIVATE_SIGNAL], 0, mime_part);
					g_object_unref (mime_part);
				}
			} else {
				set_selected (MODEST_ATTACHMENTS_VIEW (widget), MODEST_ATTACHMENT_VIEW (att_view));
				priv->rubber_start = att_view;
				gtk_grab_add (widget);
			}
		}
	}
	return TRUE;

}

static gboolean
button_release_event (GtkWidget *widget,
		      GdkEventButton *event,
		      ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	if (widget == gtk_grab_get_current ()) {
		GtkWidget *att_view = get_att_view_at_coords (MODEST_ATTACHMENTS_VIEW (widget), 
							      event->x, event->y);

		if (att_view != NULL) {
			unselect_all (MODEST_ATTACHMENTS_VIEW (widget));
			select_range (MODEST_ATTACHMENTS_VIEW (widget), 
				      MODEST_ATTACHMENT_VIEW (priv->rubber_start), 
				      MODEST_ATTACHMENT_VIEW (att_view));
		}
		priv->rubber_start = NULL;
		
		gtk_grab_remove (widget);
	}
	return TRUE;
}

static gboolean
motion_notify_event (GtkWidget *widget,
		     GdkEventMotion *event,
		     ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	if (gtk_grab_get_current () == widget) {
		GtkWidget *att_view = get_att_view_at_coords (MODEST_ATTACHMENTS_VIEW (widget), 
							      event->x, event->y);

		if (att_view != NULL) {
			unselect_all (MODEST_ATTACHMENTS_VIEW (widget));
			select_range (MODEST_ATTACHMENTS_VIEW (widget), 
				      MODEST_ATTACHMENT_VIEW (priv->rubber_start), 
				      MODEST_ATTACHMENT_VIEW (att_view));
		}
	}
	return TRUE;
}

static gboolean
key_press_event (GtkWidget *widget,
		 GdkEventKey *event,
		 ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	/* If grabbed (for example rubber banding), escape leaves the rubberbanding mode */
	if (gtk_grab_get_current () == widget) {
		if (event->keyval == GDK_Escape) {
			set_selected (MODEST_ATTACHMENTS_VIEW (widget),
				      MODEST_ATTACHMENT_VIEW (priv->rubber_start));
			priv->rubber_start = NULL;
			gtk_grab_remove (widget);
			return TRUE;
		} 
		return FALSE;
	}

	if (event->keyval == GDK_Up) {
		ModestAttachmentView *current_sel = NULL;
		gboolean move_out = FALSE;
		GList * box_children, *new_sel;

		box_children = gtk_container_get_children (GTK_CONTAINER (priv->box));
		if (box_children == NULL)
			move_out = TRUE;
		else if ((priv->selected != NULL)&&(priv->selected->data != box_children->data))
			current_sel = (ModestAttachmentView *) priv->selected->data;
		else
			move_out = TRUE;

		if (move_out) {
			GtkWidget *toplevel = NULL;
			/* move cursor outside */
			toplevel = gtk_widget_get_toplevel (widget);
			if (GTK_WIDGET_TOPLEVEL (toplevel) && GTK_IS_WINDOW (toplevel))
				g_signal_emit_by_name (toplevel, "move-focus", GTK_DIR_UP);
			unselect_all (atts_view);
		} else {
			new_sel = g_list_find (box_children, (gpointer) current_sel);
			new_sel = g_list_previous (new_sel);
			set_selected (MODEST_ATTACHMENTS_VIEW (atts_view), MODEST_ATTACHMENT_VIEW (new_sel->data));
		}
		g_list_free (box_children);
		return TRUE;
	}

	if (event->keyval == GDK_Down) {
		ModestAttachmentView *current_sel = NULL;
		gboolean move_out = FALSE;
		GList * box_children, *new_sel, *last_child = NULL;

		box_children = gtk_container_get_children (GTK_CONTAINER (priv->box));

		if (box_children == NULL) {
			move_out = TRUE;
		} else {
			last_child = g_list_last (box_children);
			if (priv->selected != NULL) {
				GList *last_selected = g_list_last (priv->selected);
				if (last_selected->data != last_child->data)
					current_sel = (ModestAttachmentView *) last_selected->data;
				else
					move_out = TRUE;
			} else {
				move_out = TRUE;
			}
		}

		if (move_out) {
			GtkWidget *toplevel = NULL;
			/* move cursor outside */
			toplevel = gtk_widget_get_toplevel (widget);
			if (GTK_WIDGET_TOPLEVEL (toplevel) && GTK_IS_WINDOW (toplevel))
				g_signal_emit_by_name (toplevel, "move-focus", GTK_DIR_DOWN);
			unselect_all (atts_view);
		} else {
			new_sel = g_list_find (box_children, (gpointer) current_sel);
			new_sel = g_list_next (new_sel);
			set_selected (MODEST_ATTACHMENTS_VIEW (atts_view), MODEST_ATTACHMENT_VIEW (new_sel->data));
		}
		g_list_free (box_children);
		return TRUE;
	}

	/* Activates selected item */
	if (g_list_length (priv->selected) == 1) {
		ModestAttachmentView *att_view = (ModestAttachmentView *) priv->selected->data;
		if ((event->keyval == GDK_Return)) {
			TnyMimePart *mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
			if (TNY_IS_MIME_PART (mime_part)) {
				g_signal_emit (G_OBJECT (widget), signals[ACTIVATE_SIGNAL], 0, mime_part);
				g_object_unref (mime_part);
			}
			return TRUE;
		}
	}

	return FALSE;
}


static GtkWidget *
get_att_view_at_coords (ModestAttachmentsView *atts_view,
			gdouble x, gdouble y)
{
	ModestAttachmentsViewPrivate *priv = NULL;
	GList *att_view_list, *node;
	GtkWidget *result = NULL;

	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	att_view_list = gtk_container_get_children (GTK_CONTAINER (priv->box));
	
	for (node = att_view_list; node != NULL; node = g_list_next (node)) {
		GtkWidget *att_view = (GtkWidget *) node->data;
		gint pos_x, pos_y, w, h, int_x, int_y;

		pos_x = att_view->allocation.x;
		pos_y = att_view->allocation.y;
		w = att_view->allocation.width;
		h = att_view->allocation.height;

		int_x = (gint) x;
		int_y = (gint) y;

		if ((x >= pos_x) && (x <= (pos_x + w)) && (y >= pos_y) && (y <= (pos_y + h))) {
			result = att_view;
			break;
		}
	}

	g_list_free (att_view_list);
	return result;
}

static void
unselect_all (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = NULL;
	GList *att_view_list, *node;

	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	att_view_list = gtk_container_get_children (GTK_CONTAINER (priv->box));
	
	for (node = att_view_list; node != NULL; node = g_list_next (node)) {
		GtkWidget *att_view = (GtkWidget *) node->data;

		if (GTK_WIDGET_STATE (att_view) == GTK_STATE_SELECTED)
			gtk_widget_set_state (att_view, GTK_STATE_NORMAL);
	}

	g_list_free (priv->selected);
	priv->selected = NULL;

	g_list_free (att_view_list);
}

static void 
set_selected (ModestAttachmentsView *atts_view, ModestAttachmentView *att_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	unselect_all (atts_view);
	gtk_widget_set_state (GTK_WIDGET (att_view), GTK_STATE_SELECTED);
	g_list_free (priv->selected);
	priv->selected = NULL;
	priv->selected = g_list_append (priv->selected, att_view);
	
	own_clipboard (atts_view);
}

static void 
select_range (ModestAttachmentsView *atts_view, ModestAttachmentView *att1, ModestAttachmentView *att2)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	GList *children = NULL;
	GList *node = NULL;
	gboolean selecting = FALSE;

	unselect_all (atts_view);

	if (att1 == att2) {
		set_selected (atts_view, att1);
		return;
	}

	children = gtk_container_get_children (GTK_CONTAINER (priv->box));
	g_list_free (priv->selected);
	priv->selected = NULL;


	for (node = children; node != NULL; node = g_list_next (node)) {
		if ((node->data == att1) || (node->data == att2)) {
			gtk_widget_set_state (GTK_WIDGET (node->data), GTK_STATE_SELECTED);
			priv->selected = g_list_append (priv->selected, node->data);
			selecting = !selecting;
		} else if (selecting) {
			gtk_widget_set_state (GTK_WIDGET (node->data), GTK_STATE_SELECTED);
			priv->selected = g_list_append (priv->selected, node->data);
		}
			
	}
	g_list_free (children);
	
	own_clipboard (atts_view);
}

static void clipboard_get (GtkClipboard *clipboard, GtkSelectionData *selection_data,
			   guint info, gpointer userdata)
{
	ModestAttachmentsView *atts_view = (ModestAttachmentsView *) userdata;
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	if ((priv->selected != NULL)&&(priv->selected->next == NULL)) {
		TnyMimePart *mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (priv->selected->data));
		if (info != MODEST_ATTACHMENTS_VIEW_CLIPBOARD_TYPE_INDEX) {
			if (TNY_IS_MSG (mime_part)) {
				TnyHeader *header = tny_msg_get_header (TNY_MSG (mime_part));
				if (TNY_IS_HEADER (header)) {
					gtk_selection_data_set_text (selection_data, tny_header_get_subject (header), -1);
					g_object_unref (header);
				}
			} else {
				gtk_selection_data_set_text (selection_data, tny_mime_part_get_filename (mime_part), -1);
			}
		} else {
			/* MODEST_ATTACHMENT requested. As the content id is not filled in all the case, we'll
			 * use an internal index. This index is simply the index of the attachment in the vbox */
			GList *box_children = NULL;
			gint index;
			box_children = gtk_container_get_children (GTK_CONTAINER (priv->box));
			index = g_list_index (box_children, priv->selected);
			if (index >= 0) {
				gchar *index_str = g_strdup_printf("%d", index);
				gtk_selection_data_set_text (selection_data, index_str, -1);
				g_free (index_str);
			}
		}
	}
}

static void clipboard_clear (GtkClipboard *clipboard, gpointer userdata)
{
	ModestAttachmentsView *atts_view = (ModestAttachmentsView *) userdata;

	unselect_all (atts_view);
}

GList *
modest_attachments_view_get_selection (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv;
	GList *selection, *node;

	g_return_val_if_fail (MODEST_IS_ATTACHMENTS_VIEW (atts_view), NULL);
	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	selection = NULL;
	for (node = priv->selected; node != NULL; node = g_list_next (node)) {
		ModestAttachmentView *att_view = (ModestAttachmentView *) node->data;
		TnyMimePart *part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
		selection = g_list_append (selection, part);
	}
	
	return selection;
}

void
modest_attachments_view_select_all (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	GList *children = NULL;
	GList *node = NULL;

	unselect_all (atts_view);

	children = gtk_container_get_children (GTK_CONTAINER (priv->box));
	g_list_free (priv->selected);
	priv->selected = NULL;


	for (node = children; node != NULL; node = g_list_next (node)) {
		gtk_widget_set_state (GTK_WIDGET (node->data), GTK_STATE_SELECTED);
		priv->selected = g_list_append (priv->selected, node->data);
	}
	g_list_free (children);

	own_clipboard (atts_view);
}

static void
own_clipboard (ModestAttachmentsView *atts_view)
{
	GtkTargetEntry targets[] = {
		{"TEXT", 0, 0},
		{"UTF8_STRING", 0, 1},
		{"COMPOUND_TEXT", 0, 2},
		{"STRING", 0, 3},
		{MODEST_ATTACHMENTS_VIEW_CLIPBOARD_TYPE, 0, MODEST_ATTACHMENTS_VIEW_CLIPBOARD_TYPE_INDEX},
	};

	gtk_clipboard_set_with_owner (gtk_widget_get_clipboard (GTK_WIDGET (atts_view), GDK_SELECTION_PRIMARY),
				      targets, G_N_ELEMENTS (targets),
				      clipboard_get, clipboard_clear, G_OBJECT(atts_view));
			      
}

/* Copyright (c) 2007, 2009, Nokia Corporation
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

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <tny-list.h>
#include <tny-simple-list.h>

#include <modest-platform.h>
#include <modest-runtime.h>
#include <modest-attachment-view.h>
#include <modest-attachments-view.h>
#include <modest-tny-mime-part.h>
#include <modest-tny-msg.h>
#include <modest-ui-constants.h>

static GObjectClass *parent_class = NULL;

/* signals */
enum {
	ACTIVATE_SIGNAL,
	DELETE_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestAttachmentsViewPrivate ModestAttachmentsViewPrivate;

struct _ModestAttachmentsViewPrivate
{
	TnyMsg *msg;
	GtkWidget *box;
	GList *selected;
	GtkWidget *rubber_start;
	GtkWidget *press_att_view;
	GtkWidget *previous_selection;
	ModestAttachmentsViewStyle style;
};

#define MODEST_ATTACHMENTS_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_ATTACHMENTS_VIEW, ModestAttachmentsViewPrivate))

static gboolean button_press_event (GtkWidget *widget, GdkEventButton *event, ModestAttachmentsView *atts_view);
static gboolean motion_notify_event (GtkWidget *widget, GdkEventMotion *event, ModestAttachmentsView *atts_view);
static gboolean button_release_event (GtkWidget *widget, GdkEventButton *event, ModestAttachmentsView *atts_view);
static gboolean key_press_event (GtkWidget *widget, GdkEventKey *event, ModestAttachmentsView *atts_view);
static gboolean focus_out_event (GtkWidget *widget, GdkEventFocus *event, ModestAttachmentsView *atts_view);
static gboolean focus (GtkWidget *widget, GtkDirectionType direction, ModestAttachmentsView *atts_view);
static GtkWidget *get_att_view_at_coords (ModestAttachmentsView *atts_view,
					  gdouble x, gdouble y);
static void unselect_all (ModestAttachmentsView *atts_view);
static void set_selected (ModestAttachmentsView *atts_view, ModestAttachmentView *att_view);
static void select_range (ModestAttachmentsView *atts_view, ModestAttachmentView *att1, ModestAttachmentView *att2);
static void clipboard_get (GtkClipboard *clipboard, GtkSelectionData *selection_data,
			   guint info, gpointer userdata);
static void own_clipboard (ModestAttachmentsView *atts_view);
static void on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata);
static void update_style (ModestAttachmentsView *self);

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

static void
add_digest_attachments (ModestAttachmentsView *attachments_view, TnyMimePart *part)
{
	TnyList *parts;
	TnyIterator *iter;

	parts = TNY_LIST (tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART (part), parts);

	for (iter  = tny_list_create_iterator(parts); 
	     !tny_iterator_is_done (iter);
	     tny_iterator_next (iter)) {
		TnyMimePart *cur_part = TNY_MIME_PART (tny_iterator_get_current (iter));
		modest_attachments_view_add_attachment (attachments_view, cur_part, TRUE, 0);
		g_object_unref (cur_part);
	}
	g_object_unref (iter);
	g_object_unref (parts);

}


void
modest_attachments_view_set_message (ModestAttachmentsView *attachments_view, TnyMsg *msg)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (attachments_view);
	TnyList *parts;
	TnyIterator *iter;
	gchar *msg_content_type = NULL;
	TnyMimePart *part_to_check;
	gboolean body_found;
	gboolean is_alternate;
	
	if (msg == priv->msg) return;

	if (priv->msg)
		g_object_unref (priv->msg);
	if (msg)
		g_object_ref (G_OBJECT(msg));
	
	priv->msg = msg;

	g_list_free (priv->selected);
	priv->selected = NULL;

	gtk_container_foreach (GTK_CONTAINER (priv->box), (GtkCallback) gtk_widget_destroy, NULL);

	if (priv->msg == NULL)
		return;

	part_to_check = modest_tny_msg_get_attachments_parent (TNY_MSG (msg));
	msg_content_type = modest_tny_mime_part_get_content_type (TNY_MIME_PART (part_to_check));
	is_alternate = (msg_content_type != NULL) && !strcasecmp (msg_content_type, "multipart/alternative");

	/* If the top mime part is a multipart/related, we don't show the attachments, as they're
	 * embedded images in body */
	if ((msg_content_type != NULL) && !strcasecmp (msg_content_type, "multipart/related")) {
		gchar *header_content_type;
		gboolean application_multipart = FALSE;

		g_free (msg_content_type);

		header_content_type = modest_tny_mime_part_get_headers_content_type (TNY_MIME_PART (part_to_check));

		if ((header_content_type != NULL) && 
		    !strstr (header_content_type, "application/")) {
			application_multipart = TRUE;
		}
		g_free (header_content_type);

		if (application_multipart) {
			gtk_widget_queue_draw (GTK_WIDGET (attachments_view));
			g_object_unref (part_to_check);
			return;
		}
	} else {
		gboolean direct_attach;

		direct_attach = (!g_str_has_prefix (msg_content_type, "message/rfc822") && 
				 !g_str_has_prefix (msg_content_type, "multipart") && 
				 !g_str_has_prefix (msg_content_type, "text/"));

		g_free (msg_content_type);

		if (direct_attach) {
			modest_attachments_view_add_attachment (attachments_view, TNY_MIME_PART (part_to_check), TRUE, 0);
			gtk_widget_queue_draw (GTK_WIDGET (attachments_view));
			g_object_unref (part_to_check);
			return;
		}
	}

	parts = TNY_LIST (tny_simple_list_new ());
	tny_mime_part_get_parts (TNY_MIME_PART (part_to_check), parts);
	iter = tny_list_create_iterator (parts);

	body_found = FALSE;
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part;
		gchar *content_type;

		part = TNY_MIME_PART (tny_iterator_get_current (iter));

		if (part && (modest_tny_mime_part_is_attachment_for_modest (part))) {
			modest_attachments_view_add_attachment (attachments_view, part, TRUE, 0);

		} else if (part && !is_alternate) {
			content_type = g_ascii_strdown (tny_mime_part_get_content_type (part), -1);
			g_strstrip (content_type);

			if (g_str_has_prefix (content_type, "multipart/digest")) {
				add_digest_attachments (attachments_view, part);
			} else if (body_found && g_str_has_prefix (content_type, "text/")) {
				   modest_attachments_view_add_attachment (attachments_view, part, TRUE, 0);
			} else if (g_str_has_prefix (content_type, "multipart/") || 
				   g_str_has_prefix (content_type, "text/")) {
				body_found = TRUE;
			}
		}


		if (part)
			g_object_unref (part);

		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (parts);
	g_object_unref (part_to_check);

	gtk_widget_queue_draw (GTK_WIDGET (attachments_view));

}

void
modest_attachments_view_add_attachment (ModestAttachmentsView *attachments_view, TnyMimePart *part,
					gboolean detect_size, guint64 size)
{
	GtkWidget *att_view = NULL;
	ModestAttachmentsViewPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (attachments_view));
	g_return_if_fail (TNY_IS_MIME_PART (part));

	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (attachments_view);

	att_view = modest_attachment_view_new (part, detect_size);
	if (!detect_size)
		modest_attachment_view_set_size (MODEST_ATTACHMENT_VIEW (att_view), size);
	gtk_box_pack_start (GTK_BOX (priv->box), att_view, FALSE, FALSE, 0);
	gtk_widget_show_all (att_view);
	gtk_widget_queue_resize (GTK_WIDGET (attachments_view));
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
		GList *node = NULL;
		GtkWidget *next_widget = NULL;
		GList *box_children = NULL;

		box_children = gtk_container_get_children (GTK_CONTAINER (priv->box));
		node = g_list_find (box_children, found_att_view);
		if (node && node->next)
			next_widget = node->next->data;

		g_list_free (box_children);
		gtk_widget_destroy (GTK_WIDGET (found_att_view));

		node = g_list_find (priv->selected, found_att_view);
		if (node) {
			priv->selected = g_list_delete_link (priv->selected, node);
			if ((priv->selected == NULL) && (next_widget != NULL))
				set_selected (MODEST_ATTACHMENTS_VIEW (atts_view), 
					      MODEST_ATTACHMENT_VIEW (next_widget));
		}
		own_clipboard (atts_view);

	}

	gtk_widget_queue_resize (GTK_WIDGET (atts_view));
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
		if ((mime_part_id != NULL) && (strcmp (mime_part_id, att_id) == 0)) {
			gtk_widget_destroy (GTK_WIDGET (att_view));
			priv->selected = g_list_remove (priv->selected, att_view);
		}

		g_object_unref (cur_mime_part);
	}

	own_clipboard (atts_view);

	gtk_widget_queue_resize (GTK_WIDGET (atts_view));
}

static void
modest_attachments_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (instance);

	priv->msg = NULL;
	priv->box = gtk_vbox_new (FALSE, 0);
	priv->rubber_start = NULL;
	priv->press_att_view = NULL;
	priv->selected = NULL;
	priv->style = MODEST_ATTACHMENTS_VIEW_STYLE_SELECTABLE;

	gtk_container_add (GTK_CONTAINER (instance), priv->box);
	gtk_event_box_set_above_child (GTK_EVENT_BOX (instance), TRUE);

	g_signal_connect (G_OBJECT (instance), "button-press-event", G_CALLBACK (button_press_event), instance);
	g_signal_connect (G_OBJECT (instance), "button-release-event", G_CALLBACK (button_release_event), instance);
	g_signal_connect (G_OBJECT (instance), "motion-notify-event", G_CALLBACK (motion_notify_event), instance);
	g_signal_connect (G_OBJECT (instance), "key-press-event", G_CALLBACK (key_press_event), instance);
	g_signal_connect (G_OBJECT (instance), "focus-out-event", G_CALLBACK (focus_out_event), instance);
	g_signal_connect (G_OBJECT (instance), "focus", G_CALLBACK (focus), instance);

	GTK_WIDGET_SET_FLAGS (instance, GTK_CAN_FOCUS);

	g_signal_connect (G_OBJECT (instance), "notify::style", G_CALLBACK (on_notify_style), (gpointer) instance);

	update_style (MODEST_ATTACHMENTS_VIEW (instance));

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
	
 	signals[DELETE_SIGNAL] =
 		g_signal_new ("delete",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET(ModestAttachmentsViewClass, delete),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

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
	if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_SELECTABLE && 
	    !GTK_WIDGET_HAS_FOCUS (widget))
		gtk_widget_grab_focus (widget);

	if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
		GtkWidget *att_view = NULL;

		att_view = get_att_view_at_coords (MODEST_ATTACHMENTS_VIEW (widget), 
						   (gint) event->x_root, (gint) event->y_root);

		if (att_view != NULL) {
			if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_NO_FOCUS) {
				unselect_all (MODEST_ATTACHMENTS_VIEW (widget));
			} else if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_LINKS) {
				priv->press_att_view = att_view;
				set_selected (MODEST_ATTACHMENTS_VIEW (widget), MODEST_ATTACHMENT_VIEW (att_view));
				gtk_grab_add (widget);
			} else {
				if (g_list_length (priv->selected) == 1) {
					priv->previous_selection = GTK_WIDGET (priv->selected->data);
				} else {
					priv->previous_selection = NULL;
				}
				TnyMimePart *mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));

				/* Do not select purged attachments */
				if (TNY_IS_MIME_PART (mime_part) && !tny_mime_part_is_purged (mime_part)) {
					set_selected (MODEST_ATTACHMENTS_VIEW (widget), MODEST_ATTACHMENT_VIEW (att_view));
					priv->rubber_start = att_view;
					gtk_grab_add (widget);
				}
				g_object_unref (mime_part);
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
		GtkWidget *att_view = NULL;

		att_view = get_att_view_at_coords (MODEST_ATTACHMENTS_VIEW (widget), 
						   (gint) event->x_root, (gint) event->y_root);

		if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_LINKS) {
			unselect_all (MODEST_ATTACHMENTS_VIEW (widget));
			if (att_view == priv->press_att_view) {
				TnyMimePart *mime_part;
				mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
				g_signal_emit (G_OBJECT (widget), signals[ACTIVATE_SIGNAL], 0, mime_part);
				g_object_unref (mime_part);
			}
			priv->press_att_view = NULL;
		} else {

			if (priv->style != MODEST_ATTACHMENTS_VIEW_STYLE_NO_FOCUS &&
			    priv->rubber_start == att_view  && 
			    priv->previous_selection == att_view) {
				TnyMimePart *mime_part;
				mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
				g_signal_emit (G_OBJECT (widget), signals[ACTIVATE_SIGNAL], 0, mime_part);
				g_object_unref (mime_part);
			} else if (att_view != NULL) {
				unselect_all (MODEST_ATTACHMENTS_VIEW (widget));
				select_range (MODEST_ATTACHMENTS_VIEW (widget), 
					      MODEST_ATTACHMENT_VIEW (priv->rubber_start), 
					      MODEST_ATTACHMENT_VIEW (att_view));
			}
			priv->rubber_start = NULL;
		}
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
		GtkWidget *att_view = NULL;

		att_view = get_att_view_at_coords (MODEST_ATTACHMENTS_VIEW (widget), 
						   (gint) event->x_root, (gint) event->y_root);
		if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_LINKS) {
			if (att_view == priv->press_att_view) {
				if (priv->selected == NULL)
				set_selected (MODEST_ATTACHMENTS_VIEW (widget), MODEST_ATTACHMENT_VIEW (att_view));
			} else {
				if (priv->selected) {
					unselect_all (MODEST_ATTACHMENTS_VIEW (widget));
				}
			}
		} else {

			if (att_view != NULL) {
				unselect_all (MODEST_ATTACHMENTS_VIEW (widget));
				select_range (MODEST_ATTACHMENTS_VIEW (widget), 
					      MODEST_ATTACHMENT_VIEW (priv->rubber_start), 
					      MODEST_ATTACHMENT_VIEW (att_view));
			}
		}
		gdk_event_request_motions (event);
	}
	return TRUE;
}

static GList*
find_prev_or_next_not_purged (GList *list, gboolean prev, gboolean include_this)
{
	GList *tmp = NULL;
	gboolean is_valid;

	if (!include_this) {
		if (prev) {
			tmp = g_list_previous (list);
		} else {
			tmp = g_list_next (list);
		}
	} else {
		tmp = list;
	}

	if (!tmp)
		return NULL;

	do {
		ModestAttachmentView *att_view = (ModestAttachmentView *) tmp->data;
		TnyMimePart *mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
		
		/* Do not select purged attachments */
		if (TNY_IS_MIME_PART (mime_part) && !tny_mime_part_is_purged (mime_part)) {
			is_valid = TRUE;
		} else {
			if (prev)
				tmp = g_list_previous (tmp);
			else
				tmp = g_list_next (tmp);
			is_valid = FALSE;
		}
		g_object_unref (mime_part);
	} while (!is_valid && tmp);

	return tmp;
}


static gboolean
key_press_event (GtkWidget *widget,
		 GdkEventKey *event,
		 ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_NO_FOCUS) {
		unselect_all (atts_view);
		return FALSE;
	}

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
		GList * box_children, *new_sel, *first_child;

		box_children = gtk_container_get_children (GTK_CONTAINER (priv->box));
		if (box_children == NULL) {
			move_out = TRUE;
		} else { 
			first_child = box_children;
			first_child = find_prev_or_next_not_purged (box_children, FALSE, TRUE);
			if (priv->selected != NULL && first_child != NULL) {
				if (priv->selected->data != first_child->data)
					current_sel = (ModestAttachmentView *) priv->selected->data;
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
				g_signal_emit_by_name (toplevel, "move-focus", GTK_DIR_UP);
			unselect_all (atts_view);
		} else {
			new_sel = g_list_find (box_children, (gpointer) current_sel);
			new_sel = find_prev_or_next_not_purged (new_sel, TRUE, FALSE);
			/* We assume that we detected properly that
			   there is a not purge attachment so we don't
			   need to check NULL */
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
			last_child = find_prev_or_next_not_purged (last_child, TRUE, TRUE);
			if (priv->selected != NULL && last_child != NULL) {
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
			new_sel = find_prev_or_next_not_purged (new_sel, FALSE, FALSE);
			set_selected (MODEST_ATTACHMENTS_VIEW (atts_view), MODEST_ATTACHMENT_VIEW (new_sel->data));
		}
		g_list_free (box_children);
		return TRUE;
	}

	if (event->keyval == GDK_BackSpace) {
		g_signal_emit (G_OBJECT (widget), signals[DELETE_SIGNAL], 0);
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
		gint widget_x, widget_y;

		gdk_window_get_origin (att_view->window, &widget_x, &widget_y);

		pos_x = widget_x;
		pos_y = widget_y;
		w = att_view->allocation.width;
		h = att_view->allocation.height;

		int_x = (gint) x - GTK_WIDGET (atts_view)->allocation.x;
		int_y = (gint) y - GTK_WIDGET (atts_view)->allocation.y;

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
	TnyMimePart *part;

	unselect_all (atts_view);
	part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
	
	g_list_free (priv->selected);
	priv->selected = NULL;
	if (TNY_IS_MIME_PART (part) && !tny_mime_part_is_purged (part)) {
		gtk_widget_set_state (GTK_WIDGET (att_view), GTK_STATE_SELECTED);
		priv->selected = g_list_append (priv->selected, att_view);
	}
	if (part)
		g_object_unref (part);
	
	own_clipboard (atts_view);
}

static void 
select_range (ModestAttachmentsView *atts_view, ModestAttachmentView *att1, ModestAttachmentView *att2)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	GList *children = NULL;
	GList *node = NULL;
	gboolean selecting = FALSE;
	TnyMimePart *part;

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
			part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (node->data));
			if (!tny_mime_part_is_purged (part)) {
				gtk_widget_set_state (GTK_WIDGET (node->data), GTK_STATE_SELECTED);
				priv->selected = g_list_append (priv->selected, node->data);
			}
			g_object_unref (part);
			selecting = !selecting;
		} else if (selecting) {
			part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (node->data));
			if (!tny_mime_part_is_purged (part)) {
				gtk_widget_set_state (GTK_WIDGET (node->data), GTK_STATE_SELECTED);
				priv->selected = g_list_append (priv->selected, node->data);
			}
			g_object_unref (part);
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
		if (info == MODEST_ATTACHMENTS_VIEW_CLIPBOARD_TYPE_INDEX) {
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

TnyList *
modest_attachments_view_get_selection (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv;
	TnyList *selection;
	GList *node;

	g_return_val_if_fail (MODEST_IS_ATTACHMENTS_VIEW (atts_view), NULL);
	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	selection = tny_simple_list_new ();
	for (node = priv->selected; node != NULL; node = g_list_next (node)) {
		ModestAttachmentView *att_view = (ModestAttachmentView *) node->data;
		TnyMimePart *part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
		tny_list_append (selection, (GObject *) part);
		g_object_unref (part);
	}
	
	return selection;
}

TnyList *
modest_attachments_view_get_attachments (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv;
	TnyList *att_list;
	GList *children, *node= NULL;

	g_return_val_if_fail (MODEST_IS_ATTACHMENTS_VIEW (atts_view), NULL);
	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	att_list = TNY_LIST (tny_simple_list_new ());

	children = gtk_container_get_children (GTK_CONTAINER (priv->box));
	for (node = children; node != NULL; node = g_list_next (node)) {
		GtkWidget *att_view = GTK_WIDGET (node->data);
		TnyMimePart *mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));
		tny_list_append (att_list, (GObject *) mime_part);
		g_object_unref (mime_part);
	}
	g_list_free (children);
	return att_list;

}

void
modest_attachments_view_select_all (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	GList *children = NULL;
	GList *node = NULL;

	unselect_all (atts_view);

	if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_LINKS)
		return;

	children = gtk_container_get_children (GTK_CONTAINER (priv->box));
	g_list_free (priv->selected);
	priv->selected = NULL;

	for (node = children; node != NULL; node = g_list_next (node)) {
		ModestAttachmentView *att_view = (ModestAttachmentView *) node->data;
		TnyMimePart *mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));

		/* Do not select purged attachments */
		if (TNY_IS_MIME_PART (mime_part) && !tny_mime_part_is_purged (mime_part)) {
			gtk_widget_set_state (GTK_WIDGET (node->data), GTK_STATE_SELECTED);
			priv->selected = g_list_append (priv->selected, node->data);
		}
		g_object_unref (mime_part);
	}
	g_list_free (children);

	own_clipboard (atts_view);
}

gboolean
modest_attachments_view_has_attachments (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv;
	GList *children;
	gboolean result;

	g_return_val_if_fail (MODEST_IS_ATTACHMENTS_VIEW (atts_view), FALSE);
	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	children = gtk_container_get_children (GTK_CONTAINER (priv->box));
	result = (children != NULL);
	g_list_free (children);

	return result;
}

void
modest_attachments_view_get_sizes (ModestAttachmentsView *attachments_view,
				   gint *attachments_count,
				   guint64 *attachments_size)
{
	ModestAttachmentsViewPrivate *priv;
	GList *children, *node;

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (attachments_view));
	g_return_if_fail (attachments_count != NULL && attachments_size != NULL);

	*attachments_count = 0;
	*attachments_size = 0;

	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (attachments_view);

	children = gtk_container_get_children (GTK_CONTAINER (priv->box));
	for (node = children; node != NULL; node = g_list_next (node)) {
		GtkWidget *att_view = (GtkWidget *) node->data;
		TnyMimePart *part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (att_view));

		if (!tny_mime_part_is_purged (part)) {
			guint64 size;
			(*attachments_count) ++;
			size = modest_attachment_view_get_size (MODEST_ATTACHMENT_VIEW (att_view));
			if (size == 0) {
				/* we do a random estimation of the size of an attachment */
				size = 32768;
			}
			*attachments_size += size;
		}
		g_object_unref (part);
	}
	g_list_free (children);
}

static void
dummy_clear_func (GtkClipboard *clipboard,
		  gpointer user_data_or_owner)
{
	/* Do nothing */
}

static void
own_clipboard (ModestAttachmentsView *atts_view)
{
	GtkTargetEntry targets[] = {
		{MODEST_ATTACHMENTS_VIEW_CLIPBOARD_TYPE, 0, MODEST_ATTACHMENTS_VIEW_CLIPBOARD_TYPE_INDEX},
	};

	gtk_clipboard_set_with_owner (gtk_widget_get_clipboard (GTK_WIDGET (atts_view), GDK_SELECTION_PRIMARY),
				      targets, G_N_ELEMENTS (targets),
				      clipboard_get, dummy_clear_func, G_OBJECT(atts_view));
}

static gboolean 
focus_out_event (GtkWidget *widget, GdkEventFocus *event, ModestAttachmentsView *atts_view)
{
	if (!gtk_widget_is_focus (widget))
		unselect_all (atts_view);

	return FALSE;
}

static gboolean 
focus (GtkWidget *widget, GtkDirectionType direction, ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);
	GList *children = NULL;
	GtkWidget *toplevel = NULL;

	toplevel = gtk_widget_get_toplevel (widget);
	if (!gtk_window_has_toplevel_focus (GTK_WINDOW (toplevel)))
		return FALSE;

	if (priv->style != MODEST_ATTACHMENTS_VIEW_STYLE_NO_FOCUS) {
		children = gtk_container_get_children (GTK_CONTAINER (priv->box));
		if (children != NULL) {
			set_selected (atts_view, MODEST_ATTACHMENT_VIEW (children->data));
		}
		g_list_free (children);
	}

	return FALSE;
}

void 
modest_attachments_view_set_style (ModestAttachmentsView *self,
				   ModestAttachmentsViewStyle style)
{
	ModestAttachmentsViewPrivate *priv;

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (self));
	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (self);

	if (priv->style != style) {
		priv->style = style;
		gtk_widget_queue_draw (GTK_WIDGET (self));
		if (priv->style == MODEST_ATTACHMENTS_VIEW_STYLE_SELECTABLE) {
			GTK_WIDGET_SET_FLAGS (self, GTK_CAN_FOCUS);
		} else {
			GTK_WIDGET_UNSET_FLAGS (self, GTK_CAN_FOCUS);
		}

	}
}

guint
modest_attachments_view_get_num_attachments (ModestAttachmentsView *atts_view)
{
	ModestAttachmentsViewPrivate *priv;
	GList *children;
	gint result;

	g_return_val_if_fail (MODEST_IS_ATTACHMENTS_VIEW (atts_view), 0);
	priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (atts_view);

	children = gtk_container_get_children (GTK_CONTAINER (priv->box));
	result = g_list_length (children);
	g_list_free (children);

	return result;
}

static void 
on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata)
{
	if (strcmp ("style", spec->name) == 0) {
		update_style (MODEST_ATTACHMENTS_VIEW (obj));
		gtk_widget_queue_draw (GTK_WIDGET (obj));
	} 
}

/* This method updates the color (and other style settings) of widgets using secondary text color,
 * tracking the gtk style */
static void
update_style (ModestAttachmentsView *self)
{
#ifdef MODEST_COMPACT_HEADER_BG
	GdkColor bg_color;
	GtkStyle *style;
	GdkColor *current_bg;

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (self));

	gdk_color_parse (MODEST_COMPACT_HEADER_BG, &bg_color);
	style = gtk_widget_get_style (GTK_WIDGET (self));
	current_bg = &(style->bg[GTK_STATE_NORMAL]);
	if (current_bg->red != bg_color.red || current_bg->blue != bg_color.blue || current_bg->green != bg_color.green)
		gtk_widget_modify_bg (GTK_WIDGET (self), GTK_STATE_NORMAL, &bg_color);
#endif
}


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
#include <modest-attachment-view.h>
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

static void
activate_attachment (ModestAttachmentView *attachment_view,
		     gpointer userdata)
{
	TnyMimePart *mime_part;
      
	mime_part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (attachment_view));
	g_signal_emit (G_OBJECT (userdata), signals[ACTIVATE_SIGNAL], 0, mime_part);
	g_object_unref (mime_part);
}

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
						    "homogeneous", FALSE,
						    "spacing", 0,
						    "resize-mode", GTK_RESIZE_PARENT,
						    NULL);

	modest_attachments_view_set_message (self, msg);

	return GTK_WIDGET (self);
}

void
modest_attachments_view_set_message (ModestAttachmentsView *attachments_view, TnyMsg *msg)
{
	ModestAttachmentsViewPriv *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (attachments_view);
	TnyList *parts;
	TnyIterator *iter;

	if (priv->msg) 
		g_object_unref (priv->msg);
	if (msg)
		g_object_ref (G_OBJECT(msg));
	
	priv->msg = msg;

	gtk_container_foreach (GTK_CONTAINER (attachments_view), (GtkCallback) gtk_widget_destroy, NULL);
	
	if (priv->msg == NULL) {
		return;
	}

	parts = TNY_LIST (tny_simple_list_new ());
	tny_mime_part_get_parts (TNY_MIME_PART (priv->msg), parts);
	iter = tny_list_create_iterator (parts);

	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part;

		part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_attachment (part)) {
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

	g_return_if_fail (MODEST_IS_ATTACHMENTS_VIEW (attachments_view));
	g_return_if_fail (TNY_IS_MIME_PART (part));
	g_return_if_fail (tny_mime_part_is_attachment (part));

	att_view = modest_attachment_view_new (part);
	gtk_box_pack_end (GTK_BOX (attachments_view), att_view, FALSE, FALSE, 0);
	gtk_widget_show_all (att_view);
	g_signal_connect (G_OBJECT (att_view), "activate", G_CALLBACK (activate_attachment), (gpointer) attachments_view);
}

static void
modest_attachments_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestAttachmentsViewPriv *priv = MODEST_ATTACHMENTS_VIEW_GET_PRIVATE (instance);

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

		type = g_type_register_static (GTK_TYPE_VBOX,
			"ModestAttachmentsView",
			&info, 0);

	}

	return type;
}

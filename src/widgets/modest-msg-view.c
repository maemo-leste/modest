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

#include <tny-gtk-text-buffer-stream.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtkhtml/gtkhtml.h>
#include <gtkhtml/gtkhtml-stream.h>
#include <tny-list.h>
#include <tny-simple-list.h>

#include <modest-tny-msg.h>
#include <modest-text-utils.h>
#include "modest-msg-view.h"
#include "modest-tny-stream-gtkhtml.h"
#include <modest-mail-header-view.h>
#include <modest-attachments-view.h>
#include <modest-marshal.h>


/* 'private'/'protected' functions */
static void     modest_msg_view_class_init   (ModestMsgViewClass *klass);
static void     modest_msg_view_init         (ModestMsgView *obj);
static void     modest_msg_view_finalize     (GObject *obj);

/* headers signals */
static void on_recpt_activated (ModestMailHeaderView *header_view, const gchar *address, ModestMsgView *msg_view);
static void on_attachment_activated (ModestAttachmentsView * att_view, gint index, gpointer);

/* GtkHtml signals */
static gboolean on_link_clicked (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view);
static gboolean on_url_requested (GtkWidget *widget, const gchar *uri, GtkHTMLStream *stream,
				  ModestMsgView *msg_view);
static gboolean on_link_hover (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view);

/* size allocation handlers */
static void size_request (GtkWidget *widget, GtkRequisition *req, gpointer userdata);
static void size_allocate (GtkWidget *widget, GtkAllocation *alloc, gpointer userdata);
static void html_adjustment_changed (GtkAdjustment *adj, ModestMsgView * view);

/* list my signals */
enum {
	LINK_CLICKED_SIGNAL,
	LINK_HOVER_SIGNAL,
	ATTACHMENT_CLICKED_SIGNAL,
	RECPT_ACTIVATED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestMsgViewPrivate ModestMsgViewPrivate;
struct _ModestMsgViewPrivate {
	GtkWidget   *table;
	GtkWidget   *gtkhtml;
	GtkWidget   *mail_header_view;
	GtkWidget   *attachments_view;

	TnyMsg      *msg;

	GtkWidget   *headers_box;
	GtkWidget   *html_scroll;

	guint full_width, full_height, html_height;

	gulong  sig1, sig2, sig3;
};
#define MODEST_MSG_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_MSG_VIEW, \
                                                 ModestMsgViewPrivate))
/* globals */
static GtkContainerClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

GType
modest_msg_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMsgViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_msg_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMsgView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_msg_view_init,
			NULL
		};
 		my_type = g_type_register_static (GTK_TYPE_VIEWPORT,
		                                  "ModestMsgView",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_msg_view_class_init (ModestMsgViewClass *klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_view_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewPrivate));

 	signals[LINK_CLICKED_SIGNAL] =
 		g_signal_new ("link_clicked",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, link_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	
	signals[ATTACHMENT_CLICKED_SIGNAL] =
 		g_signal_new ("attachment_clicked",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, attachment_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_INT);
	
	signals[LINK_HOVER_SIGNAL] =
		g_signal_new ("link_hover",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, link_hover),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[RECPT_ACTIVATED_SIGNAL] =
		g_signal_new ("recpt_activated",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, recpt_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
}

/* THIS IS A HACK: we modify the size requisition and allocation system to negociate the
 * size of the GtkHtml so that it gets the correct height, and reports it to this widget
 * to propagate the new allocation. It should make it work when it's included in a scrolled
 * window with a viewport */

static void
size_request (GtkWidget *widget,
	      GtkRequisition *req,
	      gpointer userdata)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (widget);
	GtkRequisition req_headers;

	g_message ("SIZE REQUEST START w %d h %d", req->width, req->height);

	/* tries to allocate as much as possible of the current allocation for headers box */

	req_headers.height = priv->full_height;
	req_headers.width = req->width;
	req->height = priv->full_height;

	g_message ("SIZE REQUEST HEADER START w %d h %d", req_headers.width, req_headers.height);
	gtk_widget_size_request (priv->headers_box, &req_headers);
	g_message ("SIZE REQUEST HEADER END w %d h %d", req_headers.width, req_headers.height);
	g_message ("SIZE REQUEST END w %d h %d", req->width, req->height);
}

static void
size_allocate (GtkWidget *widget,
	       GtkAllocation *alloc,
	       gpointer userdata)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (widget);
	GtkAllocation headers_alloc;
	GtkAllocation html_alloc;
	GtkAdjustment *hadj, *vadj;

	g_message ("SIZE_ALLOCATE START w %d h %d", alloc->width, alloc->height);

	priv->full_width = alloc->width;
	priv->full_height = alloc->height;

	hadj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (widget));
	vadj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (widget));

	/* allocates all the visible with for the header. The height is
	   taken from the last requisition of the widget, supposing it's
	   been calculated depending on this width */
	headers_alloc.x = alloc->x;
	headers_alloc.y = alloc->y;
	headers_alloc.width = alloc->width;
	headers_alloc.height = priv->headers_box->requisition.height;
	if (priv->html_height != priv->gtkhtml->requisition.height)
		gtk_widget_size_allocate (priv->headers_box, &headers_alloc);

	/* allocates the gtk html space trying to negociate that it takes
	 * the available space, and as much height as it needs. To do this,
	 * it takes the internal adjustment upper value (see html_adjustment_changed)
	 */
	html_alloc.x = alloc->x;
	html_alloc.y = alloc->y + headers_alloc.height;
	html_alloc.width = alloc->width;
 	html_alloc.height = MAX(alloc->height, priv->html_height);
	gtk_widget_size_allocate (priv->gtkhtml, &html_alloc);

	/* Corrects the allocation of the full widget to include the final
	 * gtkhtml height */
	priv->full_height = headers_alloc.height + priv->html_height;
	alloc->height = priv->full_height;

	g_message ("SIZE_ALLOCATE END w %d h %d", alloc->width, alloc->height);

}


static void
html_adjustment_changed (GtkAdjustment *adj,
			 ModestMsgView * view)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (view);

	g_message ("ADJUSTMENT CHANGED START upper %f html_height %d", adj->upper, priv->html_height);
	
	/* correct the html height calculation depending on the range exposed
	 * by the html vertical adjustment
	 */
	if (((gint) adj->upper) != priv->gtkhtml->allocation.height) {
		priv->html_height = (gint) adj->upper;
	}
	gtk_widget_queue_resize (GTK_WIDGET(view));
}


static void
modest_msg_view_init (ModestMsgView *obj)
{
 	ModestMsgViewPrivate *priv;
	
	priv = MODEST_MSG_VIEW_GET_PRIVATE(obj);


	priv->full_width = 0;
	priv->full_height = 0;
	priv->html_height = G_MAXINT;

	priv->table = gtk_table_new (2, 2, FALSE);

	gtk_table_set_row_spacings (GTK_TABLE (priv->table), 0);
	gtk_table_set_col_spacings (GTK_TABLE (priv->table), 0);

	priv->html_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->html_scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

	priv->msg                     = NULL;

	priv->gtkhtml                 = gtk_html_new();
	gtk_html_set_editable        (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_allow_selection     (GTK_HTML(priv->gtkhtml), TRUE);
	gtk_html_set_caret_mode      (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_set_blocking        (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_set_images_blocking (GTK_HTML(priv->gtkhtml), FALSE);

	priv->mail_header_view        = GTK_WIDGET(modest_mail_header_view_new ());
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);

	priv->attachments_view        = GTK_WIDGET(modest_attachments_view_new (NULL));
	gtk_widget_set_no_show_all (priv->attachments_view, TRUE);

	priv->sig1 = g_signal_connect (G_OBJECT(priv->gtkhtml), "link_clicked",
				       G_CALLBACK(on_link_clicked), obj);
	priv->sig2 = g_signal_connect (G_OBJECT(priv->gtkhtml), "url_requested",
				       G_CALLBACK(on_url_requested), obj);
	priv->sig3 = g_signal_connect (G_OBJECT(priv->gtkhtml), "on_url",
				       G_CALLBACK(on_link_hover), obj);

	g_signal_connect (G_OBJECT (priv->mail_header_view), "recpt-activated", 
			  G_CALLBACK (on_recpt_activated), obj);

	g_signal_connect (G_OBJECT (priv->attachments_view), "activate",
			  G_CALLBACK (on_attachment_activated), obj);
}
	

static void
modest_msg_view_finalize (GObject *obj)
{	
	ModestMsgViewPrivate *priv;
	priv = MODEST_MSG_VIEW_GET_PRIVATE (obj);

	if (priv->msg) {
		g_object_unref (G_OBJECT(priv->msg));
		priv->msg = NULL;
	}
	
	/* we cannot disconnect sigs, because priv->gtkhtml is
	 * already dead */
	
	priv->gtkhtml = NULL;
	priv->attachments_view = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}

GtkWidget*
modest_msg_view_new (TnyMsg *msg)
{
	GObject *obj;
	ModestMsgView* self;
	ModestMsgViewPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_MSG_VIEW, NULL));
	self = MODEST_MSG_VIEW(obj);
	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	priv->headers_box = gtk_vbox_new (0, FALSE);

	if (priv->mail_header_view)
		gtk_box_pack_start (GTK_BOX(priv->headers_box), priv->mail_header_view, FALSE, FALSE, 0);
	
	if (priv->attachments_view)
		gtk_box_pack_start (GTK_BOX(priv->headers_box), priv->attachments_view, FALSE, FALSE, 0);

	gtk_table_attach (GTK_TABLE (priv->table), priv->headers_box, 0, 1, 0, 1, GTK_EXPAND, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (priv->table), gtk_label_new (""), 1, 2, 0, 1, 0, GTK_FILL|GTK_SHRINK, 0, 0);

	if (priv->gtkhtml) {
		gtk_container_add (GTK_CONTAINER (priv->html_scroll), priv->gtkhtml);
		gtk_table_attach (GTK_TABLE(priv->table), priv->html_scroll, 0, 2, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);
	}

	gtk_container_add (GTK_CONTAINER (self), priv->table);

	modest_msg_view_set_message (self, msg);

	g_signal_connect (G_OBJECT (self), "size-request", G_CALLBACK (size_request), NULL);
	g_signal_connect (G_OBJECT (self), "size-allocate", G_CALLBACK (size_allocate), NULL);
	g_signal_connect (G_OBJECT (gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(priv->html_scroll))), 
			  "changed", G_CALLBACK (html_adjustment_changed), self);
	
	return GTK_WIDGET(self);
}

static void
on_recpt_activated (ModestMailHeaderView *header_view, 
		    const gchar *address,
		    ModestMsgView * view)
{
	g_signal_emit (G_OBJECT (view), signals[RECPT_ACTIVATED_SIGNAL], 0, address);
}

static void
on_attachment_activated (ModestAttachmentsView * att_view, gint index, gpointer msg_view)
{

	
	if (index == 0) {
		/* index is 1-based, so 0 indicates an error */
		g_printerr ("modest: invalid attachment index: %d\n", index);
		return;
	}

	g_signal_emit (G_OBJECT(msg_view), signals[ATTACHMENT_CLICKED_SIGNAL],
		       0, index);
}

static gboolean
on_link_clicked (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view)
{
	g_return_val_if_fail (msg_view, FALSE);
	
	g_signal_emit (G_OBJECT(msg_view), signals[LINK_CLICKED_SIGNAL],
		       0, uri);

	return FALSE;
}


static gboolean
on_link_hover (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view)
{
	g_signal_emit (G_OBJECT(msg_view), signals[LINK_HOVER_SIGNAL],
		       0, uri);

	return FALSE;
}



static TnyMimePart *
find_cid_image (TnyMsg *msg, const gchar *cid)
{
	TnyMimePart *part = NULL;
	TnyList *parts;
	TnyIterator *iter;
	
	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (cid, NULL);
	
	parts  = TNY_LIST (tny_simple_list_new());

	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts); 
	iter   = tny_list_create_iterator (parts);
	
	while (!tny_iterator_is_done(iter)) {
		const gchar *part_cid;
		part = TNY_MIME_PART(tny_iterator_get_current(iter));
		part_cid = tny_mime_part_get_content_id (part);

		if (part_cid && strcmp (cid, part_cid) == 0)
			break;

		g_object_unref (G_OBJECT(part));
	
		part = NULL;
		tny_iterator_next (iter);
	}
	
	g_object_unref (G_OBJECT(iter));	
	g_object_unref (G_OBJECT(parts));
	
	return part;
}


static gboolean
on_url_requested (GtkWidget *widget, const gchar *uri,
		  GtkHTMLStream *stream, ModestMsgView *msg_view)
{
	ModestMsgViewPrivate *priv;
	priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	
	if (g_str_has_prefix (uri, "cid:")) {
		/* +4 ==> skip "cid:" */
		TnyMimePart *part = find_cid_image (priv->msg, uri + 4);
		if (!part) {
			g_printerr ("modest: '%s' not found\n", uri + 4);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_ERROR);
		} else {
			TnyStream *tny_stream =
				TNY_STREAM(modest_tny_stream_gtkhtml_new(stream));
			tny_mime_part_decode_to_stream ((TnyMimePart*)part,
								  tny_stream);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_OK);
	
			g_object_unref (G_OBJECT(tny_stream));
			g_object_unref (G_OBJECT(part));
		}
	}

	return TRUE;
}

static gboolean
set_html_message (ModestMsgView *self, TnyMimePart *tny_body, TnyMsg *msg)
{
	GtkHTMLStream *gtkhtml_stream;
	TnyStream *tny_stream;	
	ModestMsgViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (tny_body, FALSE);
	
	priv = MODEST_MSG_VIEW_GET_PRIVATE(self);

	gtkhtml_stream = gtk_html_begin(GTK_HTML(priv->gtkhtml));

	tny_stream     = TNY_STREAM(modest_tny_stream_gtkhtml_new (gtkhtml_stream));
	tny_stream_reset (tny_stream);

	tny_mime_part_decode_to_stream ((TnyMimePart*)tny_body, tny_stream);
	g_object_unref (G_OBJECT(tny_stream));
	
	gtk_html_stream_destroy (gtkhtml_stream);
	
	return TRUE;
}


/* FIXME: this is a hack --> we use the tny_text_buffer_stream to
 * get the message text, then write to gtkhtml 'by hand' */
static gboolean
set_text_message (ModestMsgView *self, TnyMimePart *tny_body, TnyMsg *msg)
{
	GtkTextBuffer *buf;
	GtkTextIter begin, end;
	TnyStream* txt_stream, *tny_stream;
	GtkHTMLStream *gtkhtml_stream;
	gchar *txt;
	ModestMsgViewPrivate *priv;
		
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (tny_body, FALSE);

	priv           = MODEST_MSG_VIEW_GET_PRIVATE(self);
	
	buf            = gtk_text_buffer_new (NULL);
	txt_stream     = TNY_STREAM(tny_gtk_text_buffer_stream_new (buf));
		
	tny_stream_reset (txt_stream);

	gtkhtml_stream = gtk_html_begin(GTK_HTML(priv->gtkhtml)); 
	tny_stream =  TNY_STREAM(modest_tny_stream_gtkhtml_new (gtkhtml_stream));
	
	// FIXME: tinymail
	tny_mime_part_decode_to_stream ((TnyMimePart*)tny_body, txt_stream);
	tny_stream_reset (txt_stream);		
	
	gtk_text_buffer_get_bounds (buf, &begin, &end);
	txt = gtk_text_buffer_get_text (buf, &begin, &end, FALSE);
	if (txt) {
		gchar *html = modest_text_utils_convert_to_html (txt);
		tny_stream_write (tny_stream, html, strlen(html));
		tny_stream_reset (tny_stream);
		g_free (txt);
		g_free (html);
	}
	
	g_object_unref (G_OBJECT(tny_stream));
	g_object_unref (G_OBJECT(txt_stream));
	g_object_unref (G_OBJECT(buf));
	
	gtk_html_stream_destroy (gtkhtml_stream);
	
	return TRUE;
}


static gboolean
set_empty_message (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	priv           = MODEST_MSG_VIEW_GET_PRIVATE(self);

	gtk_html_load_from_string (GTK_HTML(priv->gtkhtml),
				   "", 1);
	
	return TRUE;
}


void
modest_msg_view_set_message (ModestMsgView *self, TnyMsg *msg)
{
	TnyMimePart *body;
	ModestMsgViewPrivate *priv;
	TnyHeader *header;
	
	g_return_if_fail (self);
	
	priv = MODEST_MSG_VIEW_GET_PRIVATE(self);
	gtk_widget_set_no_show_all (priv->mail_header_view, FALSE);

	if (msg != priv->msg) {
		if (priv->msg)
			g_object_unref (G_OBJECT(priv->msg));
		if (msg)
			g_object_ref   (G_OBJECT(msg));
		priv->msg = msg;
	}
	
	if (!msg) {
		tny_header_view_clear (TNY_HEADER_VIEW (priv->mail_header_view));
		gtk_widget_hide_all (priv->mail_header_view);
		gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);
		set_empty_message (self);
		return;
	}

	header = tny_msg_get_header (msg);
	tny_header_view_set_header (TNY_HEADER_VIEW (priv->mail_header_view), header);
	g_object_unref (header);

	modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW(priv->attachments_view),
					     msg);
	
	body = modest_tny_msg_find_body_part (msg,TRUE);
	if (body) {
		if (tny_mime_part_content_type_is (body, "text/html"))
			set_html_message (self, body, msg);
		else
			set_text_message (self, body, msg);
	} else 
		set_empty_message (self);

	gtk_widget_queue_resize (GTK_WIDGET (self));
	
	gtk_widget_show (priv->gtkhtml);
	gtk_widget_show_all (priv->mail_header_view);
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);

}


TnyMsg*
modest_msg_view_get_message (ModestMsgView *self)
{
	g_return_val_if_fail (self, NULL);
	
	return MODEST_MSG_VIEW_GET_PRIVATE(self)->msg;
}


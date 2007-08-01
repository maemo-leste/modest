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
#include <gtkhtml/gtkhtml-search.h>
#include <gtkhtml/gtkhtml-embedded.h>
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
static void     modest_msg_view_destroy     (GtkObject *obj);
static void     set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void     get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

/* headers signals */
static void on_recpt_activated (ModestMailHeaderView *header_view, const gchar *address, ModestMsgView *msg_view);
static void on_attachment_activated (ModestAttachmentsView * att_view, TnyMimePart *mime_part, gpointer userdata);

/* GtkHtml signals */
static gboolean on_link_clicked (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view);
static gboolean on_url_requested (GtkWidget *widget, const gchar *uri, GtkHTMLStream *stream,
				  ModestMsgView *msg_view);
static gboolean on_link_hover (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view);

#ifdef MAEMO_CHANGES
static void     on_tap_and_hold (GtkWidget *widget, gpointer userdata); 
#endif /*MAEMO_CHANGES*/


/* size allocation and drawing handlers */
static void get_view_allocation (ModestMsgView *msg_view, GtkAllocation *allocation);
static void size_request (GtkWidget *widget, GtkRequisition *req);
static void size_allocate (GtkWidget *widget, GtkAllocation *alloc);
static void realize (GtkWidget *widget);
static void unrealize (GtkWidget *widget);
static gint expose (GtkWidget *widget, GdkEventExpose *event);
static void reclamp_adjustment (GtkAdjustment *adj, gboolean *value_changed);
static void set_hadjustment_values (ModestMsgView *msg_view, gboolean *value_changed);
static void set_scroll_adjustments (ModestMsgView *msg_view, GtkAdjustment *hadj, GtkAdjustment *vadj);
static void adjustment_value_changed (GtkAdjustment *adj, gpointer data);
static void html_adjustment_changed (GtkAdjustment *adj, gpointer data);
static void disconnect_vadjustment (ModestMsgView *obj);
static void disconnect_hadjustment (ModestMsgView *obj);
static gboolean idle_readjust_scroll (ModestMsgView *obj);

/* GtkContainer methods */
static void forall (GtkContainer *container, gboolean include_internals,
		    GtkCallback callback, gpointer userdata);
static void container_remove (GtkContainer *container, GtkWidget *widget);

/* list my signals */
enum {
	LINK_CLICKED_SIGNAL,
	LINK_HOVER_SIGNAL,
	ATTACHMENT_CLICKED_SIGNAL,
	RECPT_ACTIVATED_SIGNAL,
	LINK_CONTEXTUAL_SIGNAL,
	LAST_SIGNAL
};

/* list properties */
enum {
	PROP_0,
	PROP_HADJUSTMENT,
	PROP_VADJUSTMENT,
	PROP_SHADOW_TYPE
};

typedef struct _ModestMsgViewPrivate ModestMsgViewPrivate;
struct _ModestMsgViewPrivate {
	GtkWidget   *gtkhtml;
	GtkWidget   *mail_header_view;
	GtkWidget   *attachments_view;

	TnyMsg      *msg;

	/* embedded elements */
	GtkWidget   *headers_box;
	GtkWidget   *html_scroll;
	GtkWidget   *attachments_box;

	/* internal adjustments for set_scroll_adjustments */
	GtkAdjustment *hadj;
	GtkAdjustment *vadj;
	GtkShadowType shadow_type;

	/* gdk windows for drawing */
	GdkWindow *view_window;
	GdkWindow *headers_window;
	GdkWindow *html_window;

	/* zoom */
	gdouble current_zoom;

	/* link click management */
	gchar *last_url;

	TnyHeaderFlags priority_flags;

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
 		my_type = g_type_register_static (GTK_TYPE_CONTAINER,
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
	GtkObjectClass *gtkobject_class;
	GtkContainerClass *container_class;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass *) klass;
	gtkobject_class = (GtkObjectClass *) klass;
	container_class = (GtkContainerClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_view_finalize;
	gobject_class->set_property = set_property;
	gobject_class->get_property = get_property;
	gtkobject_class->destroy = modest_msg_view_destroy;

	widget_class->realize = realize;
	widget_class->unrealize = unrealize;
	widget_class->expose_event = expose;
	widget_class->size_request = size_request;
	widget_class->size_allocate = size_allocate;

	container_class->forall = forall;
	container_class->remove = container_remove;

	klass->set_scroll_adjustments = set_scroll_adjustments;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewPrivate));

	g_object_class_install_property (gobject_class,
					 PROP_HADJUSTMENT,
					 g_param_spec_object ("hadjustment", 
							      _("Horizontal adjustment"),
							      _("GtkAdjustment with information of the horizontal visible position"),
							      GTK_TYPE_ADJUSTMENT,
							      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (gobject_class,
					 PROP_VADJUSTMENT,
					 g_param_spec_object ("vadjustment", 
							      _("Vertical adjustment"),
							      _("GtkAdjustment with information of the vertical visible position"),
							      GTK_TYPE_ADJUSTMENT,
							      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (gobject_class,
					 PROP_SHADOW_TYPE,
					 g_param_spec_enum ("shadow_type", 
							    _("Shadow type"),
							    _("Kind of shadow that's shown around the view"),
							    GTK_TYPE_SHADOW_TYPE,
							    GTK_SHADOW_IN,
							    G_PARAM_READABLE | G_PARAM_WRITABLE ));

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
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, G_TYPE_OBJECT);
	
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

	signals[LINK_CONTEXTUAL_SIGNAL] =
		g_signal_new ("link_contextual",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, link_contextual),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ModestMsgViewClass, set_scroll_adjustments),
			      NULL, NULL,
			      modest_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2,
			      GTK_TYPE_ADJUSTMENT,
			      GTK_TYPE_ADJUSTMENT);
}

static void
set_property (GObject *object, 
	      guint prop_id, 
	      const GValue *value, 
	      GParamSpec *pspec)
{
	ModestMsgView *msg_view = MODEST_MSG_VIEW (object);

	switch (prop_id) {
	case PROP_HADJUSTMENT:
		modest_msg_view_set_hadjustment (msg_view, g_value_get_object (value));
		break;
	case PROP_VADJUSTMENT:
		modest_msg_view_set_vadjustment (msg_view, g_value_get_object (value));
		break;
	case PROP_SHADOW_TYPE:
		modest_msg_view_set_shadow_type (msg_view, g_value_get_enum (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
get_property (GObject *object, 
	      guint prop_id, 
	      GValue *value, 
	      GParamSpec *pspec)
{
	ModestMsgView *msg_view = MODEST_MSG_VIEW (object);
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	switch (prop_id) {
	case PROP_HADJUSTMENT:
		g_value_set_object (value, priv->hadj);
		break;
	case PROP_VADJUSTMENT:
		g_value_set_object (value, priv->vadj);
		break;
	case PROP_SHADOW_TYPE:
		g_value_set_enum (value, priv->shadow_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
disconnect_hadjustment (ModestMsgView *msg_view)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	if (priv->hadj) {
		g_signal_handlers_disconnect_by_func(priv->hadj, adjustment_value_changed, msg_view);
		g_object_unref (priv->hadj);
		priv->hadj = NULL;
	}
}

static void
disconnect_vadjustment (ModestMsgView *msg_view)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	if (priv->vadj) {
		g_signal_handlers_disconnect_by_func(priv->vadj, adjustment_value_changed, msg_view);
		g_object_unref (priv->vadj);
		priv->vadj = NULL;
	}
}

static void 
get_view_allocation (ModestMsgView *msg_view, GtkAllocation *allocation)
{
	/* This method gets the allocation of the widget in parent widget. It's the
	   real position and size of the widget */
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	
	allocation->x = 0;
	allocation->y = 0;

	if (priv->shadow_type != GTK_SHADOW_NONE) {
		allocation->x = GTK_WIDGET (msg_view)->style->xthickness;
		allocation->y = GTK_WIDGET (msg_view)->style->ythickness;
	}

	allocation->width = MAX (1, (GTK_WIDGET (msg_view)->allocation.width) - allocation->x * 2);
	allocation->height = MAX (1, (GTK_WIDGET (msg_view)->allocation.height) - allocation->y * 2);

}

static void 
reclamp_adjustment (GtkAdjustment *adj, 
		    gboolean *value_changed)
{
	gdouble value = adj->value;

	/* Correct value to be inside the expected values of a scroll */

	value = CLAMP (value, 0, adj->upper - adj->page_size);

	if (value != adj->value) {
		adj->value = value;
		if (value_changed)
			*value_changed = TRUE;
	} else if (value_changed) {
		*value_changed = FALSE;
	}
}

static void 
set_hadjustment_values (ModestMsgView *msg_view,
			gboolean *value_changed)
{
	GtkAllocation view_allocation;
	GtkAdjustment *hadj = modest_msg_view_get_hadjustment (msg_view);

	get_view_allocation (msg_view, &view_allocation);
	hadj->page_size = view_allocation.width;
	hadj->step_increment = view_allocation.width * 0.1;
	hadj->page_increment = view_allocation.width * 0.9;

	hadj->lower = 0;
	hadj->upper = view_allocation.width;

	reclamp_adjustment (hadj, value_changed);

}

static void 
set_vadjustment_values (ModestMsgView *msg_view,
			gboolean *value_changed)
{
	GtkAllocation view_allocation;
	GtkAdjustment *vadj = modest_msg_view_get_vadjustment (msg_view);
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	gint full_height = 0;
	GtkAdjustment *html_vadj;

	get_view_allocation (msg_view, &view_allocation);
	vadj->page_size = view_allocation.height;
	vadj->step_increment = view_allocation.height * 0.1;
	vadj->page_increment = view_allocation.height * 0.9;

	vadj->lower = 0;

	if (priv->headers_box && GTK_WIDGET_VISIBLE(priv->headers_box)) {
		GtkRequisition child_requisition;

		gtk_widget_get_child_requisition (priv->headers_box, &child_requisition);
		full_height = child_requisition.height;
	} else {
		full_height = 0;
	}
	
	/* Get the real height of the embedded html */
	if (priv->html_scroll && GTK_WIDGET_VISIBLE(priv->html_scroll)) {
		html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
		full_height += html_vadj->upper;
	}

	vadj->upper = MAX (view_allocation.height, full_height);

	reclamp_adjustment (vadj, value_changed);

}

static void
set_scroll_adjustments (ModestMsgView *msg_view,
			GtkAdjustment *hadj,
			GtkAdjustment *vadj)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	modest_msg_view_set_hadjustment (msg_view, hadj);
	modest_msg_view_set_vadjustment (msg_view, vadj);

	gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->gtkhtml), vadj);
}

static void
realize (GtkWidget *widget)
{
	ModestMsgView *msg_view = MODEST_MSG_VIEW (widget);
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	GtkAdjustment *hadj = modest_msg_view_get_hadjustment (msg_view);
	GtkAdjustment *vadj = modest_msg_view_get_vadjustment (msg_view);
	GdkWindowAttr attributes;
	gint event_mask;
	gint attributes_mask;
	GtkAllocation view_allocation;

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	/* The structure of the GdkWindow's is:
	 *    * widget->window: the shown gdkwindow embedding all the stuff inside
	 *    * view_window: a backing store gdkwindow containing the headers and contents
	 *      being scrolled. This window should have all the visible and non visible
	 *      widgets inside.
	 *    * headers_window: gdk window for headers_box.
	 *    * html_window: gdk window for html_scroll (the scrolled window containing the
	 *      gtkhtml showing the contents of the mail).
	 */

	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);

	event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
	attributes.event_mask = event_mask | GDK_BUTTON_PRESS_MASK;
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, msg_view);

	get_view_allocation (msg_view, &view_allocation);

	attributes.x = view_allocation.x;
	attributes.y = view_allocation.y;
	attributes.width = view_allocation.width;
	attributes.height = view_allocation.height;
	attributes.event_mask = 0;
	priv->view_window = gdk_window_new (widget->window, &attributes, attributes_mask);
	gdk_window_set_user_data (priv->view_window, msg_view);
	gdk_window_set_back_pixmap (priv->view_window, NULL, FALSE);

	attributes.x = -hadj->value;
	attributes.y = -vadj->value;
	attributes.width = hadj->upper;
	if (priv->headers_box)
		attributes.height = GTK_WIDGET (priv->headers_box)->allocation.height;
	else
		attributes.height = 1;
	attributes.event_mask = event_mask;

	priv->headers_window = gdk_window_new (priv->view_window, &attributes, attributes_mask);
	gdk_window_set_user_data (priv->headers_window, msg_view);

	if (priv->headers_box)
		gtk_widget_set_parent_window (priv->headers_box, priv->headers_window);

	attributes.x = -hadj->value;
	if (priv->headers_box)
		attributes.y = GTK_WIDGET (priv->headers_box)->allocation.height - vadj->value;
	else 
		attributes.y = -vadj->value;
	attributes.width = hadj->upper;
	if (priv->headers_box)
		attributes.height = vadj->upper - GTK_WIDGET (priv->headers_box)->allocation.height;
	else
		attributes.height = vadj->upper;
	attributes.event_mask = event_mask;

	priv->html_window = gdk_window_new (priv->view_window, &attributes, attributes_mask);
	gdk_window_set_user_data (priv->html_window, msg_view);

	if (priv->html_scroll)
		gtk_widget_set_parent_window (priv->html_scroll, priv->html_window);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
	gtk_style_set_background (widget->style, priv->headers_window, GTK_STATE_NORMAL);
	gtk_style_set_background (widget->style, priv->html_window, GTK_STATE_NORMAL);

	gtk_paint_flat_box(widget->style, priv->headers_window, GTK_STATE_NORMAL,
			   GTK_SHADOW_NONE, 
			   NULL, widget, "msgviewheaders",
			   0,0,-1,-1);
	gtk_paint_flat_box(widget->style, priv->html_window, GTK_STATE_NORMAL,
			   GTK_SHADOW_NONE, 
			   NULL, widget, "msgviewcontents",
			   0,0,-1,-1);

	gdk_window_show (priv->view_window);
	gdk_window_show (priv->headers_window);
	gdk_window_show (priv->html_window);

}

static void
unrealize (GtkWidget *widget)
{
	ModestMsgView *msg_view = MODEST_MSG_VIEW (widget);
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	gdk_window_set_user_data (priv->view_window, NULL);
	gdk_window_destroy (priv->view_window);
	priv->view_window = NULL;

	gdk_window_set_user_data (priv->headers_window, NULL);
	gdk_window_destroy (priv->headers_window);
	priv->headers_window = NULL;

	gdk_window_set_user_data (priv->html_window, NULL);
	gdk_window_destroy (priv->html_window);
	priv->html_window = NULL;

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		( * GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);

}

static gint
expose (GtkWidget *widget, 
	GdkEventExpose *event)
{
	ModestMsgView *msg_view;
	ModestMsgViewPrivate *priv;

	if (GTK_WIDGET_DRAWABLE (widget)) {
		msg_view = MODEST_MSG_VIEW (widget);
		priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
		if (event->window == widget->window) {
			gtk_paint_shadow (widget->style, widget->window,
					  GTK_STATE_NORMAL, priv->shadow_type,
					  &event->area, widget, "msgview",
					  0,0,-1,-1);
		} else if (event->window == priv->headers_window) {
			gtk_paint_flat_box(widget->style, priv->headers_window, GTK_STATE_NORMAL,
					   GTK_SHADOW_NONE, 
					   &event->area, widget, "msgviewheaders",
					   0,0,-1,-1);
		} else if (event->window == priv->html_window) {
			gtk_paint_flat_box(widget->style, priv->html_window, GTK_STATE_NORMAL,
					   GTK_SHADOW_NONE, 
					   &event->area, widget, "msgviewcontents",
					   0,0,-1,-1);
		}
		if (priv->headers_box)
			gtk_container_propagate_expose (GTK_CONTAINER (msg_view), priv->headers_box, event);
		if (priv->html_scroll)
			gtk_container_propagate_expose (GTK_CONTAINER (msg_view), priv->html_scroll, event);
		(* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);
	}

	return FALSE;
}

static void 
forall (GtkContainer *container, 
	gboolean include_internals,
	GtkCallback callback,
	gpointer userdata)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (container);
	g_return_if_fail (callback != NULL);

	if (priv->headers_box)
		(*callback) (priv->headers_box, userdata);
	if (priv->html_scroll)
		(*callback) (priv->html_scroll, userdata);
}

static void
container_remove (GtkContainer *container,
		  GtkWidget *widget)
{
	gboolean was_visible = FALSE;
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (container);
	was_visible = GTK_WIDGET_VISIBLE (widget);
	if (widget == priv->headers_box) {
		gtk_widget_unparent (priv->headers_box);
		priv->headers_box = NULL;
	} else if (widget == priv->html_scroll) {
		gtk_widget_unparent (priv->html_scroll);
		priv->html_scroll = NULL;
	} else {
		return;
	}
	if (was_visible)
		gtk_widget_queue_resize (GTK_WIDGET(container));

}

static void
size_request (GtkWidget *widget,
	      GtkRequisition *req)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (widget);
	GtkRequisition child_req;

	req->width = 0;
	req->height = 0;

	gtk_widget_size_request (priv->headers_box, &child_req);
	req->width = child_req.width;
	req->height += child_req.height;
	gtk_widget_size_request (priv->html_scroll, &child_req);
	req->width = MAX (child_req.width, req->width);
	req->height += child_req.height;

}

static void
size_allocate (GtkWidget *widget,
	       GtkAllocation *allocation)
{
	ModestMsgView *msg_view = MODEST_MSG_VIEW (widget);
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	gboolean hadj_value_changed, vadj_value_changed;
	GtkAllocation headers_allocation, html_allocation, view_allocation;
	GtkAdjustment *html_vadj;

	if (GTK_WIDGET_MAPPED (widget) &&
	    priv->shadow_type != GTK_SHADOW_NONE && 
	    (allocation->width != widget->allocation.width ||
	     allocation->height != widget->allocation.height))
		gdk_window_invalidate_rect (widget->window, NULL, FALSE);

	widget->allocation = *allocation;
	set_hadjustment_values (msg_view, &hadj_value_changed);
	set_vadjustment_values (msg_view, &vadj_value_changed);

	get_view_allocation (msg_view, &view_allocation);

	headers_allocation.x = 0;
	headers_allocation.y = 0;
	headers_allocation.width = view_allocation.width;
	if (priv->headers_box)
		headers_allocation.height = GTK_WIDGET (priv->headers_box)->requisition.height;
	else
		headers_allocation.height = 0;

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));

	html_allocation.x = 0;
	html_allocation.y = headers_allocation.height;
	html_allocation.width = view_allocation.width;
	html_allocation.height = MAX ((gint) html_vadj->upper, (gint)(priv->vadj->upper - headers_allocation.height));

	if (GTK_WIDGET_REALIZED (widget)) {
		gdk_window_move_resize (widget->window,
					allocation->x,
					allocation->y,
					allocation->width,
					allocation->height);

		gdk_window_move_resize (priv->view_window,
					view_allocation.x,
					view_allocation.y,
					view_allocation.width,
					view_allocation.height);
		gdk_window_move_resize (priv->headers_window,
					0,
					(gint) (- priv->vadj->value),
					allocation->width,
					headers_allocation.height);
		gdk_window_move_resize (priv->html_window,
					(gint) (- priv->hadj->value),
					(gint) (html_allocation.y - priv->vadj->value),
					(gint) priv->hadj->upper,
					html_allocation.height);
	}

	if (priv->headers_box && GTK_WIDGET_VISIBLE (priv->headers_box)) {
		gtk_widget_size_allocate (priv->headers_box, &headers_allocation);
	}
	if (priv->html_scroll && GTK_WIDGET_VISIBLE (priv->html_scroll)) {
		html_allocation.x = 0;
		html_allocation.y = 0;
		html_allocation.width = (gint) priv->hadj->upper;
		html_allocation.height = (gint) priv->vadj->upper - headers_allocation.height;
		gtk_widget_size_allocate (priv->html_scroll, &html_allocation);
	}
	gtk_adjustment_changed (priv->hadj);
	gtk_adjustment_changed (priv->vadj);

	if (hadj_value_changed)
		gtk_adjustment_value_changed (priv->hadj);
	if (vadj_value_changed)
		gtk_adjustment_value_changed (priv->vadj);

}

static void 
adjustment_value_changed (GtkAdjustment *adj, gpointer data)
{
	ModestMsgView *msg_view = NULL;
	ModestMsgViewPrivate *priv = NULL;

	g_return_if_fail (GTK_IS_ADJUSTMENT (adj));
	g_return_if_fail (MODEST_IS_MSG_VIEW (data));

	msg_view = MODEST_MSG_VIEW (data);
	priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	if (GTK_WIDGET_REALIZED (msg_view)) {
		GtkAdjustment *hadj = modest_msg_view_get_hadjustment (msg_view);
		GtkAdjustment *vadj = modest_msg_view_get_vadjustment (msg_view);
		gint headers_offset = 0;

		gtk_widget_queue_resize (priv->html_scroll);

		if (priv->headers_box && GTK_WIDGET_VISIBLE (priv->headers_box)) {
			gint old_x, old_y;
			gint new_x, new_y;
			gdk_window_get_position (priv->headers_window, &old_x, &old_y);
			new_x = 0;
			new_y = -vadj->value;

			headers_offset = GTK_WIDGET(priv->headers_box)->allocation.height;

			if (new_x != old_x || new_y != old_y) {
				gdk_window_move (priv->headers_window, new_x, new_y);
				gdk_window_process_updates (priv->headers_window, TRUE);
			}
		}
		
		if (priv->html_scroll && GTK_WIDGET_VISIBLE (priv->html_scroll)) {
			gint old_x, old_y;
			gint new_x, new_y;
			gdk_window_get_position (priv->html_window, &old_x, &old_y);
			new_x = -hadj->value;
			new_y = headers_offset - vadj->value;

			if (new_x != old_x || new_y != old_y) {
				gdk_window_move (priv->html_window, new_x, new_y);
				gdk_window_process_updates (priv->html_window, TRUE);
			}
		}
		
	}
}

static void
html_adjustment_changed (GtkAdjustment *adj,
			 gpointer userdata)
{
	ModestMsgView *msg_view = MODEST_MSG_VIEW (userdata);
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	GtkAdjustment *html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(priv->html_scroll));
	gboolean vadj_changed;
	gint new_height;

	g_signal_stop_emission_by_name (G_OBJECT (adj), "changed");

	priv->html_scroll->requisition.height = html_vadj->upper;
	if (html_vadj->upper == priv->html_scroll->allocation.height)
		return;
	priv->html_scroll->allocation.height = html_vadj->upper;

	set_vadjustment_values (msg_view, &vadj_changed);

	new_height = MAX ((gint) html_vadj->upper, (gint) (priv->vadj->upper - priv->headers_box->allocation.height));
	
	gtk_adjustment_changed (priv->vadj);
	if (GTK_WIDGET_DRAWABLE (priv->html_scroll)) {
		gdk_window_resize (priv->html_window, (gint) priv->hadj->upper, (gint) new_height);
		gdk_window_process_updates (priv->view_window, TRUE);
		gtk_container_resize_children (GTK_CONTAINER (msg_view));
	}
	
}

static void
modest_msg_view_init (ModestMsgView *obj)
{
 	ModestMsgViewPrivate *priv;
	GtkAdjustment *html_vadj;

	GTK_WIDGET_UNSET_FLAGS (obj, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (obj), TRUE);
	gtk_container_set_reallocate_redraws (GTK_CONTAINER (obj), TRUE);
	gtk_container_set_resize_mode (GTK_CONTAINER (obj), GTK_RESIZE_QUEUE);
	
	priv = MODEST_MSG_VIEW_GET_PRIVATE(obj);

	priv->current_zoom = 1.0;
	priv->priority_flags = 0;

	priv->hadj = NULL;
	priv->vadj = NULL;
	priv->shadow_type = GTK_SHADOW_IN;
	priv->view_window = NULL;
	priv->headers_window = NULL;
	priv->html_window = NULL;

	gtk_widget_push_composite_child ();
	priv->html_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_composite_name (priv->html_scroll, "contents");
	gtk_widget_pop_composite_child ();
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->html_scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

	priv->msg                     = NULL;

	priv->gtkhtml                 = gtk_html_new();
	gtk_html_set_editable        (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_allow_selection     (GTK_HTML(priv->gtkhtml), TRUE);
	gtk_html_set_caret_mode      (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_set_blocking        (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_set_images_blocking (GTK_HTML(priv->gtkhtml), FALSE);

	priv->mail_header_view        = GTK_WIDGET(modest_mail_header_view_new (TRUE));
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);

	priv->attachments_view        = GTK_WIDGET(modest_attachments_view_new (NULL));

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

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(priv->html_scroll));

	g_signal_connect (G_OBJECT (html_vadj), "changed",
			  G_CALLBACK (html_adjustment_changed), obj);

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
	
	disconnect_vadjustment (MODEST_MSG_VIEW(obj));
	disconnect_hadjustment (MODEST_MSG_VIEW(obj));

	priv->gtkhtml = NULL;
	priv->attachments_view = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}

static void
modest_msg_view_destroy (GtkObject *obj)
{	
	disconnect_vadjustment (MODEST_MSG_VIEW(obj));
	disconnect_hadjustment (MODEST_MSG_VIEW(obj));

	GTK_OBJECT_CLASS(parent_class)->destroy (obj);		
}

GtkAdjustment *
modest_msg_view_get_vadjustment (ModestMsgView *msg_view)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	if (!priv->vadj)
		modest_msg_view_set_vadjustment (msg_view, NULL);

	return priv->vadj;
	
}

GtkAdjustment *
modest_msg_view_get_hadjustment (ModestMsgView *msg_view)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	if (!priv->hadj)
		modest_msg_view_set_hadjustment (msg_view, NULL);

	return priv->hadj;
	
}

void
modest_msg_view_set_hadjustment (ModestMsgView *msg_view, GtkAdjustment *hadj)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	gboolean value_changed;

	if (hadj && hadj == priv->hadj)
		return;

	if (!hadj)
		hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,0.0,0.0,0.0,0.0,0.0));
	disconnect_hadjustment (msg_view);
	g_object_ref (G_OBJECT (hadj));
	gtk_object_sink (GTK_OBJECT (hadj));
	priv->hadj = hadj;
	set_hadjustment_values (msg_view, &value_changed);

	g_signal_connect (hadj, "value_changed", G_CALLBACK (adjustment_value_changed),
			  msg_view);

	gtk_adjustment_changed (hadj);
	if (value_changed)
		gtk_adjustment_value_changed (hadj);
	else
		adjustment_value_changed (hadj, msg_view);

	g_object_notify (G_OBJECT (msg_view), "hadjustment");
}

void
modest_msg_view_set_vadjustment (ModestMsgView *msg_view, GtkAdjustment *vadj)
{
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	gboolean value_changed;

	if (vadj && vadj == priv->vadj)
		return;

	if (!vadj)
		vadj = (GtkAdjustment *) gtk_adjustment_new (0.0,0.0,0.0,0.0,0.0,0.0);
	disconnect_vadjustment (msg_view);
	g_object_ref (G_OBJECT (vadj));
	gtk_object_sink (GTK_OBJECT (vadj));
	priv->vadj = vadj;
	set_vadjustment_values (msg_view, &value_changed);

	g_signal_connect (vadj, "value_changed", G_CALLBACK (adjustment_value_changed),
			  msg_view);

	gtk_adjustment_changed (vadj);
	if (value_changed)
		gtk_adjustment_value_changed (vadj);
	else
		adjustment_value_changed (vadj, msg_view);

	g_object_notify (G_OBJECT (msg_view), "vadjustment");
}

/** 
 * modest_msg_view_set_shadow_type:
 * @msg_view: a #ModestMsgView.
 * @shadow_type: new shadow type.
 *
 * Sets a shadow type of the message view.
 **/ 
void
modest_msg_view_set_shadow_type (ModestMsgView *msg_view,
				 GtkShadowType shadow_type)
{
	ModestMsgViewPrivate *priv;
	g_return_if_fail (MODEST_IS_MSG_VIEW (msg_view));

	priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	
	if (priv->shadow_type != shadow_type) {
		priv->shadow_type = shadow_type;
		
		if (GTK_WIDGET_VISIBLE (msg_view)) {
			gtk_widget_size_allocate (GTK_WIDGET (msg_view), &(GTK_WIDGET (msg_view)->allocation));
			gtk_widget_queue_draw (GTK_WIDGET (msg_view));
		}
		g_object_notify (G_OBJECT (msg_view), "shadow-type");
	}
}

/**
 * modest_msg_view_get_shadow_type:
 * @msg_view: a #ModestMsgView
 *
 * Gets the current shadow type of the #ModestMsgView.
 *
 * Return value: the shadow type 
 **/
GtkShadowType
modest_msg_view_get_shadow_type (ModestMsgView *msg_view)
{
	ModestMsgViewPrivate *priv;
	g_return_val_if_fail (MODEST_IS_MSG_VIEW (msg_view), GTK_SHADOW_NONE);
	priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	
	return priv->shadow_type;
}

GtkWidget*
modest_msg_view_new (TnyMsg *msg)
{
	GObject *obj;
	ModestMsgView* self;
	ModestMsgViewPrivate *priv;
	GtkWidget *separator;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_MSG_VIEW, NULL));
	self = MODEST_MSG_VIEW(obj);
	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	gtk_widget_push_composite_child ();
	priv->headers_box = gtk_vbox_new (0, FALSE);
	gtk_widget_set_composite_name (priv->headers_box, "headers");
	gtk_widget_pop_composite_child ();

	if (priv->mail_header_view)
		gtk_box_pack_start (GTK_BOX(priv->headers_box), priv->mail_header_view, FALSE, FALSE, 0);
	
	if (priv->attachments_view) {
		priv->attachments_box = (GtkWidget *) modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view),
												 _("Attachments:"), priv->attachments_view,
												 FALSE, FALSE);
		gtk_widget_hide_all (priv->attachments_box);
/* 		gtk_widget_set_no_show_all (priv->attachments_box, TRUE); */
	}

	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX(priv->headers_box), separator, FALSE, FALSE, 0);

	gtk_widget_set_parent (priv->headers_box, GTK_WIDGET (self));

	if (priv->gtkhtml) {
		gtk_container_add (GTK_CONTAINER (priv->html_scroll), priv->gtkhtml);
		gtk_widget_set_parent (priv->html_scroll, GTK_WIDGET(self));
#ifdef MAEMO_CHANGES
		gtk_widget_tap_and_hold_setup (GTK_WIDGET (priv->gtkhtml), NULL, NULL, 0);
		g_signal_connect (G_OBJECT (priv->gtkhtml), "tap-and-hold", G_CALLBACK (on_tap_and_hold), obj);
#endif
	}

	modest_msg_view_set_message (self, msg);

	return GTK_WIDGET(self);
}

#ifdef MAEMO_CHANGES
static void
on_tap_and_hold (GtkWidget *widget,
		 gpointer data)
{
	ModestMsgView *msg_view = (ModestMsgView *) data;
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	g_signal_emit (G_OBJECT (msg_view), signals[LINK_CONTEXTUAL_SIGNAL],
		       0, priv->last_url);
}
#endif

static void
on_recpt_activated (ModestMailHeaderView *header_view, 
		    const gchar *address,
		    ModestMsgView * view)
{
	g_signal_emit (G_OBJECT (view), signals[RECPT_ACTIVATED_SIGNAL], 0, address);
}

static void
on_attachment_activated (ModestAttachmentsView * att_view, TnyMimePart *mime_part, gpointer msg_view)
{

	g_signal_emit (G_OBJECT(msg_view), signals[ATTACHMENT_CLICKED_SIGNAL],
		       0, mime_part);
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
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);

	g_free (priv->last_url);
	priv->last_url = g_strdup (uri);

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

		if (part)
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
	GtkAdjustment *html_vadj;
	
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
		modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), NULL);
		gtk_widget_hide_all (priv->mail_header_view);
		gtk_widget_hide_all (priv->attachments_box);
		gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);
		set_empty_message (self);
		gtk_widget_queue_resize (GTK_WIDGET(self));
		gtk_widget_queue_draw (GTK_WIDGET(self));
		return;
	}

	header = tny_msg_get_header (msg);
	tny_header_view_set_header (TNY_HEADER_VIEW (priv->mail_header_view), header);
	g_object_unref (header);

	modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW(priv->attachments_view),
					     msg);
	
	body = modest_tny_msg_find_body_part (msg, TRUE);
	if (body) {
		if (tny_mime_part_content_type_is (body, "text/html"))
			set_html_message (self, body, msg);
		else
			set_text_message (self, body, msg);

		if(modest_attachments_view_has_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view))) {
			gtk_widget_show_all (priv->attachments_box);
		} else {
			gtk_widget_hide_all (priv->attachments_box);
		}

/* 		g_print ("---\nfilename %s\ncontent_location %s\ncontent_id%s\ncontent_type%s\n", */
/* 			 tny_mime_part_get_filename (body), */
/* 			 tny_mime_part_get_content_location (body), */
/* 			 tny_mime_part_get_content_id (body), */
/* 			 tny_mime_part_get_content_type (body)); */
			
	} else 
		set_empty_message (self);

	gtk_widget_show (priv->gtkhtml);
	gtk_widget_set_no_show_all (priv->attachments_box, TRUE);
	gtk_widget_show_all (priv->mail_header_view);
	gtk_widget_set_no_show_all (priv->attachments_box, FALSE);
/* 	gtk_widget_show_all (priv->attachments_box); */
/* 	gtk_widget_show_all (priv->attachments_box); */
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);
	gtk_widget_queue_resize (GTK_WIDGET(self));
	gtk_widget_queue_draw (GTK_WIDGET(self));

	if (priv->hadj != NULL)
		priv->hadj->value = 0.0;
	if (priv->vadj != NULL)
		priv->vadj->value = 0.0;

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));

	g_signal_emit_by_name (G_OBJECT (html_vadj), "changed");

	/* This is a hack to force reallocation of scroll after drawing all the stuff. This
	 * makes the html view get the proper and expected size and prevent being able to scroll
	 * the buffer when it shouldn't be scrollable */
	g_object_ref (self);
	g_timeout_add (250, (GSourceFunc) idle_readjust_scroll, self);
}


TnyMsg*
modest_msg_view_get_message (ModestMsgView *self)
{
	TnyMsg *msg;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW (self), NULL);

	msg = MODEST_MSG_VIEW_GET_PRIVATE(self)->msg;

	if (msg)
		g_object_ref (msg);
	
	return msg;
}

static gboolean
has_contents_receiver (gpointer engine, const gchar *data,
		       size_t len, gboolean *has_contents)
{
	if (len > 1 || ((len == 1)&&(data[0]!='\n'))) {
		*has_contents = TRUE;
		return FALSE;
	}
	return TRUE;
}

gboolean 
modest_msg_view_get_message_is_empty (ModestMsgView *self)
{
	/* TODO: Find some gtkhtml API to check whether there is any (visible, non markup)
	 * text in the message:
	 */
	ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (self);
	gboolean has_contents = FALSE;

	gtk_html_export (GTK_HTML (priv->gtkhtml), "text/plain", 
			 (GtkHTMLSaveReceiverFn) has_contents_receiver, &has_contents);
	
	return !has_contents;
}


gboolean 
modest_msg_view_search (ModestMsgView *self, const gchar *search)
{
	ModestMsgViewPrivate *priv;
	gboolean result;
	GtkAdjustment *vadj, *tmp_vadj;
	gdouble y_offset;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW (self), FALSE);

	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);
	vadj = gtk_layout_get_vadjustment (GTK_LAYOUT (priv->gtkhtml));
	g_object_ref (vadj);
	tmp_vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, vadj->lower, vadj->upper, vadj->step_increment, 32.0, 32.0));
	gtk_layout_set_vadjustment (GTK_LAYOUT (priv->gtkhtml), tmp_vadj);
	result = gtk_html_engine_search (GTK_HTML (priv->gtkhtml),
					 search,
					 FALSE,   /* case sensitive */
					 TRUE,    /* forward */
					 FALSE);  /* regexp */

// wait for the updated gtkhtml (w27) to enable this
#if 0
	if (result) {
		gint x, y, w, h;
		gdouble offset_top, offset_bottom;
		GtkAdjustment *adj;
		gtk_html_get_selection_area (GTK_HTML (priv->gtkhtml), &x, &y, &w, &h);
		offset_top = (gdouble) (priv->headers_box->requisition.height + y);
		offset_bottom = (gdouble) (priv->headers_box->requisition.height + y + h);
		adj = GTK_ADJUSTMENT (priv->vadj);
		if (offset_top < adj->value)
			gtk_adjustment_set_value (adj, offset_top + adj->page_increment - adj->page_size);
		else if (offset_bottom > adj->value + adj->page_increment)
			gtk_adjustment_set_value (adj, offset_bottom - adj->page_increment);
	}
#endif 

	y_offset = tmp_vadj->value;
	gtk_layout_set_vadjustment (GTK_LAYOUT (priv->gtkhtml), vadj);
	g_object_unref (vadj);

	return result;
}

gboolean
modest_msg_view_search_next (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;
	gboolean result;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW (self), FALSE);

	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);
	result = gtk_html_engine_search_next (GTK_HTML (priv->gtkhtml));

// fixme wait for new gtkhtml
#if 0
	if (result) {
		gint x, y, w, h;
		gdouble offset_top, offset_bottom;
		GtkAdjustment *adj;
		gtk_html_get_selection_area (GTK_HTML (priv->gtkhtml), &x, &y, &w, &h);
		g_message ("SELECTION AREA x%d y%d w%d h%d", x, y, w, h);
		offset_top = (gdouble) (priv->headers_box->requisition.height + y);
		offset_bottom = (gdouble) (priv->headers_box->requisition.height + y + h);
		adj = GTK_ADJUSTMENT (priv->vadj);
		if (offset_top < adj->value)
			gtk_adjustment_set_value (adj, offset_top + adj->page_increment - adj->page_size);
		else if (offset_bottom > adj->value + adj->page_increment)
			gtk_adjustment_set_value (adj, offset_bottom - adj->page_increment);
	}
#endif
	return result;
}

void
modest_msg_view_set_zoom (ModestMsgView *self, gdouble zoom)
{
	ModestMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_VIEW (self));

	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);
	priv->current_zoom = zoom;
	gtk_html_set_magnification (GTK_HTML(priv->gtkhtml), zoom);

	gtk_widget_queue_resize (priv->gtkhtml);
}

gdouble
modest_msg_view_get_zoom (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW (self), 1.0);

	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	return priv->current_zoom;
}

TnyHeaderFlags
modest_msg_view_get_priority (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW (self), 0);

	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	return priv->priority_flags;
}

void
modest_msg_view_set_priority (ModestMsgView *self, TnyHeaderFlags flags)
{
	ModestMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_VIEW (self));

	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	priv->priority_flags = flags & (TNY_HEADER_FLAG_HIGH_PRIORITY);

	modest_mail_header_view_set_priority (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), flags);
}

GList *
modest_msg_view_get_selected_attachments (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW (self), NULL);
	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	return modest_attachments_view_get_selection (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	
}

GList *
modest_msg_view_get_attachments (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW (self), NULL);
	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	return modest_attachments_view_get_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	
}

void
modest_msg_view_grab_focus (ModestMsgView *view)
{
	ModestMsgViewPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_MSG_VIEW (view));
	priv = MODEST_MSG_VIEW_GET_PRIVATE (view);

	gtk_widget_grab_focus (priv->gtkhtml);
}

void
modest_msg_view_remove_attachment (ModestMsgView *view, TnyMimePart *attachment)
{
	TnyMsg *msg;
	ModestMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_VIEW (view));
	g_return_if_fail (TNY_IS_MIME_PART (attachment));
	priv = MODEST_MSG_VIEW_GET_PRIVATE (view);

	msg = modest_msg_view_get_message (view);
	modest_attachments_view_remove_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
						   attachment);
	
}

gboolean
idle_readjust_scroll (ModestMsgView *view)
{
	if (GTK_WIDGET_DRAWABLE (view)) {
		ModestMsgViewPrivate *priv = MODEST_MSG_VIEW_GET_PRIVATE (view);
		GtkAdjustment *html_vadj;
		html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
		html_vadj->page_size = html_vadj->upper;
		gtk_adjustment_changed (html_vadj);
		gtk_widget_queue_resize (GTK_WIDGET (view));
		gtk_widget_queue_draw (GTK_WIDGET (view));
	}
	g_object_unref (G_OBJECT (view));
	return FALSE;
}

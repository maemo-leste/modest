/* Copyright (c) 2006, 2007, Nokia Corporation
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
#include <tny-gtk-text-buffer-stream.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <tny-list.h>
#include <tny-simple-list.h>
#include <tny-vfs-stream.h>

#include <modest-tny-msg.h>
#include <modest-text-utils.h>
#include <widgets/modest-msg-view.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <widgets/modest-compact-mail-header-view.h>
#include <hildon/hildon-gtk.h>
#else
#include <widgets/modest-expander-mail-header-view.h>
#endif
#include <widgets/modest-attachments-view.h>
#include <modest-marshal.h>
#include <widgets/modest-gtkhtml-mime-part-view.h>
#include <widgets/modest-gtkhtml-msg-view.h>
#include <widgets/modest-isearch-view.h>
#include <widgets/modest-ui-constants.h>
#include <modest-icon-names.h>

/* FIXNE: we should have no maemo-deps in widgets/ */
#ifndef MODEST_TOOLKIT_GTK
#include "maemo/modest-hildon-includes.h"
#endif /*!MODEST_TOOLKIT_GTK*/


/* 'private'/'protected' functions */
static void     modest_gtkhtml_msg_view_class_init   (ModestGtkhtmlMsgViewClass *klass);
static void     tny_header_view_init (gpointer g, gpointer iface_data);
static void     tny_msg_view_init (gpointer g, gpointer iface_data);
static void     tny_mime_part_view_init (gpointer g, gpointer iface_data);
static void     modest_mime_part_view_init (gpointer g, gpointer iface_data);
static void     modest_zoomable_init (gpointer g, gpointer iface_data);
static void     modest_isearch_view_init (gpointer g, gpointer iface_data);
static void     modest_msg_view_init (gpointer g, gpointer iface_data);
static void     modest_gtkhtml_msg_view_init         (ModestGtkhtmlMsgView *obj);
static void     modest_gtkhtml_msg_view_finalize     (GObject *obj);
static void     modest_gtkhtml_msg_view_destroy     (GtkObject *obj);
static void     set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void     get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

/* headers signals */
static void on_recpt_activated (ModestMailHeaderView *header_view, const gchar *address, ModestGtkhtmlMsgView *msg_view);
static void on_show_details (ModestMailHeaderView *header_view, ModestGtkhtmlMsgView *msg_view);
static void on_attachment_activated (ModestAttachmentsView * att_view, TnyMimePart *mime_part, gpointer userdata);

/* body view signals */
static gboolean on_activate_link (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMsgView *msg_view);
static gboolean on_fetch_url (GtkWidget *widget, const gchar *uri, TnyStream *stream,
			      ModestGtkhtmlMsgView *msg_view);
static gboolean on_link_hover (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMsgView *msg_view);
static void on_limit_error (GtkWidget *widget, ModestGtkhtmlMsgView *msg_view);

#ifdef MAEMO_CHANGES
static void     on_tap_and_hold (GtkWidget *widget, gpointer userdata); 
static gboolean on_tap_and_hold_query (GtkWidget *widget, GdkEvent *event, gpointer userdata); 
#endif /*MAEMO_CHANGES*/


/* size allocation and drawing handlers */
static void get_view_allocation (ModestGtkhtmlMsgView *msg_view, GtkAllocation *allocation);
static void size_request (GtkWidget *widget, GtkRequisition *req);
static void size_allocate (GtkWidget *widget, GtkAllocation *alloc);
static void realize (GtkWidget *widget);
static void unrealize (GtkWidget *widget);
static gint expose (GtkWidget *widget, GdkEventExpose *event);
static void reclamp_adjustment (GtkAdjustment *adj, gboolean *value_changed);
static void set_hadjustment_values (ModestGtkhtmlMsgView *msg_view, gboolean *value_changed);
static void set_scroll_adjustments (ModestGtkhtmlMsgView *msg_view, GtkAdjustment *hadj, GtkAdjustment *vadj);
static void adjustment_value_changed (GtkAdjustment *adj, gpointer data);
static void html_adjustment_changed (GtkAdjustment *adj, gpointer data);
static void disconnect_vadjustment (ModestGtkhtmlMsgView *obj);
static void disconnect_hadjustment (ModestGtkhtmlMsgView *obj);
static gboolean idle_readjust_scroll (ModestGtkhtmlMsgView *obj);

/* vertical panning implementation */
#ifdef MAEMO_CHANGES
static gboolean motion_notify_event (GtkWidget *widget,
				     GdkEventMotion *event,
				     gpointer userdata);
#endif

static gboolean 
button_press_event (GtkWidget *widget,
		    GdkEventButton *event,
		    gpointer userdata);
static gboolean 
button_release_event (GtkWidget *widget,
		      GdkEventButton *event,
		      gpointer userdata);

/* GtkContainer methods */
static void forall (GtkContainer *container, gboolean include_internals,
		    GtkCallback callback, gpointer userdata);
static void container_remove (GtkContainer *container, GtkWidget *widget);

/* TnyMimePartView implementation */
static void modest_msg_view_mp_clear (TnyMimePartView *self);
static void modest_msg_view_mp_set_part (TnyMimePartView *self, TnyMimePart *part);
static void modest_msg_view_mp_set_part_default (TnyMimePartView *self, TnyMimePart *part);
static TnyMimePart* modest_msg_view_mp_get_part (TnyMimePartView *self);
static TnyMimePart* modest_msg_view_mp_get_part_default (TnyMimePartView *self);
/* ModestMimePartView implementation */
static gboolean modest_msg_view_mp_is_empty (ModestMimePartView *self);
static gboolean modest_msg_view_mp_is_empty_default (ModestMimePartView *self);
/* TnyHeaderView implementation */
static void modest_msg_view_set_header (TnyHeaderView *self, TnyHeader *header);
static void modest_msg_view_set_header_default (TnyHeaderView *self, TnyHeader *header);
static void modest_msg_view_clear_header (TnyHeaderView *self);
static void modest_msg_view_clear_header_default (TnyHeaderView *self);
/* TnyMsgView implementation */
static TnyMsg *modest_msg_view_get_msg (TnyMsgView *self);
static TnyMsg *modest_msg_view_get_msg_default (TnyMsgView *self);
static void modest_msg_view_set_msg (TnyMsgView *self, TnyMsg *msg);
static void modest_msg_view_set_msg_default (TnyMsgView *self, TnyMsg *msg);
static void modest_msg_view_clear (TnyMsgView *self);
static void modest_msg_view_clear_default (TnyMsgView *self);
static void modest_msg_view_set_unavailable (TnyMsgView *self);
static void modest_msg_view_set_unavailable_default (TnyMsgView *self);
static TnyMimePartView *modest_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part);
static TnyMimePartView *modest_msg_view_create_mime_part_view_for_default (TnyMsgView *self, TnyMimePart *part);
static TnyMsgView *modest_msg_view_create_new_inline_viewer (TnyMsgView *self);
static TnyMsgView *modest_msg_view_create_new_inline_viewer_default (TnyMsgView *self);
/* ModestZoomable implementation */
static gdouble modest_msg_view_get_zoom (ModestZoomable *self);
static void modest_msg_view_set_zoom (ModestZoomable *self, gdouble value);
static gboolean modest_msg_view_zoom_minus (ModestZoomable *self);
static gboolean modest_msg_view_zoom_plus (ModestZoomable *self);
static gdouble modest_msg_view_get_zoom_default (ModestZoomable *self);
static void modest_msg_view_set_zoom_default (ModestZoomable *self, gdouble value);
static gboolean modest_msg_view_zoom_minus_default (ModestZoomable *self);
static gboolean modest_msg_view_zoom_plus_default (ModestZoomable *self);
/* ModestISearchView implementation */
static gboolean modest_msg_view_search (ModestISearchView *self, const gchar *string);
static gboolean modest_msg_view_search_default (ModestISearchView *self, const gchar *string);
static gboolean modest_msg_view_search_next (ModestISearchView *self);
static gboolean modest_msg_view_search_next_default (ModestISearchView *self);
/* ModestMsgView implementation */
static void modest_gtkhtml_msg_view_set_msg_with_other_body (ModestMsgView *self, TnyMsg *msg, TnyMimePart *other_body);
static GtkAdjustment *modest_gtkhtml_msg_view_get_vadjustment (ModestMsgView *self);
static GtkAdjustment *modest_gtkhtml_msg_view_get_hadjustment (ModestMsgView *self);
static void modest_gtkhtml_msg_view_set_vadjustment (ModestMsgView *self, GtkAdjustment *vadj);
static void modest_gtkhtml_msg_view_set_hadjustment (ModestMsgView *self, GtkAdjustment *hadj);
static void modest_gtkhtml_msg_view_set_shadow_type (ModestMsgView *self, GtkShadowType type);
static GtkShadowType modest_gtkhtml_msg_view_get_shadow_type (ModestMsgView *self);
static TnyHeaderFlags modest_gtkhtml_msg_view_get_priority (ModestMsgView *self);
static void modest_gtkhtml_msg_view_set_priority (ModestMsgView *self, TnyHeaderFlags flags);
static TnyList *modest_gtkhtml_msg_view_get_selected_attachments (ModestMsgView *self);
static TnyList *modest_gtkhtml_msg_view_get_attachments (ModestMsgView *self);
static void modest_gtkhtml_msg_view_grab_focus (ModestMsgView *self);
static void modest_gtkhtml_msg_view_remove_attachment (ModestMsgView *view, TnyMimePart *attachment);
static void modest_gtkhtml_msg_view_request_fetch_images (ModestMsgView *view);
static void modest_gtkhtml_msg_view_set_branding (ModestMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static gboolean modest_gtkhtml_msg_view_has_blocked_external_images (ModestMsgView *view);
static void modest_gtkhtml_msg_view_set_msg_with_other_body_default (ModestMsgView *view, TnyMsg *msg, TnyMimePart *part);
static GtkAdjustment *modest_gtkhtml_msg_view_get_vadjustment_default (ModestMsgView *self);
static GtkAdjustment *modest_gtkhtml_msg_view_get_hadjustment_default (ModestMsgView *self);
static void modest_gtkhtml_msg_view_set_vadjustment_default (ModestMsgView *self, GtkAdjustment *vadj);
static void modest_gtkhtml_msg_view_set_hadjustment_default (ModestMsgView *self, GtkAdjustment *hadj);
static void modest_gtkhtml_msg_view_set_shadow_type_default (ModestMsgView *self, GtkShadowType type);
static GtkShadowType modest_gtkhtml_msg_view_get_shadow_type_default (ModestMsgView *self);
static TnyHeaderFlags modest_gtkhtml_msg_view_get_priority_default (ModestMsgView *self);
static void modest_gtkhtml_msg_view_set_priority_default (ModestMsgView *self, TnyHeaderFlags flags);
static TnyList *modest_gtkhtml_msg_view_get_selected_attachments_default (ModestMsgView *self);
static TnyList *modest_gtkhtml_msg_view_get_attachments_default (ModestMsgView *self);
static void modest_gtkhtml_msg_view_grab_focus_default (ModestMsgView *self);
static void modest_gtkhtml_msg_view_remove_attachment_default (ModestMsgView *view, TnyMimePart *attachment);
static gboolean modest_gtkhtml_msg_view_has_blocked_external_images_default (ModestMsgView *view);
static void modest_gtkhtml_msg_view_request_fetch_images_default (ModestMsgView *view);
static void modest_gtkhtml_msg_view_set_branding_default (ModestMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);

/* internal api */
static void     set_header     (ModestGtkhtmlMsgView *self, TnyHeader *header);
static TnyMsg   *get_message   (ModestGtkhtmlMsgView *self);
static void     set_message    (ModestGtkhtmlMsgView *self, TnyMsg *tny_msg, TnyMimePart *other_body);
static gboolean is_empty       (ModestGtkhtmlMsgView *self); 
static void     set_zoom       (ModestGtkhtmlMsgView *self, gdouble zoom);
static gdouble  get_zoom       (ModestGtkhtmlMsgView *self);
static gboolean search         (ModestGtkhtmlMsgView *self, const gchar *search);
static gboolean search_next    (ModestGtkhtmlMsgView *self);
static GtkAdjustment *get_vadjustment (ModestGtkhtmlMsgView *self);
static GtkAdjustment *get_hadjustment (ModestGtkhtmlMsgView *self);
static void set_vadjustment (ModestGtkhtmlMsgView *self, GtkAdjustment *vadj);
static void set_hadjustment (ModestGtkhtmlMsgView *self, GtkAdjustment *hadj);
static void set_shadow_type (ModestGtkhtmlMsgView *self, GtkShadowType type);
static GtkShadowType get_shadow_type (ModestGtkhtmlMsgView *self);
static TnyHeaderFlags get_priority (ModestGtkhtmlMsgView *self);
static void set_priority (ModestGtkhtmlMsgView *self, TnyHeaderFlags flags);
static TnyList *get_selected_attachments (ModestGtkhtmlMsgView *self);
static TnyList *get_attachments (ModestGtkhtmlMsgView *self);
static void grab_focus (ModestGtkhtmlMsgView *self);
static void remove_attachment (ModestGtkhtmlMsgView *view, TnyMimePart *attachment);
static void request_fetch_images (ModestGtkhtmlMsgView *view);
static void set_branding (ModestGtkhtmlMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static gboolean has_blocked_external_images (ModestGtkhtmlMsgView *view);

/* list properties */
enum {
	PROP_0,
	PROP_HADJUSTMENT,
	PROP_VADJUSTMENT,
	PROP_SHADOW_TYPE
};

typedef struct _ModestGtkhtmlMsgViewPrivate ModestGtkhtmlMsgViewPrivate;
struct _ModestGtkhtmlMsgViewPrivate {
	GtkWidget   *body_view;
	GtkWidget   *mail_header_view;
	GtkWidget   *attachments_view;

	TnyMsg      *msg;

	/* embedded elements */
	GtkWidget   *headers_box;
	GtkWidget   *html_scroll;
	GtkWidget   *attachments_box;

#ifdef MODEST_TOOLKIT_HILDON2
	GtkWidget   *priority_box;
	GtkWidget   *priority_icon;
#endif

	/* internal adjustments for set_scroll_adjustments */
	GtkAdjustment *hadj;
	GtkAdjustment *vadj;
	GtkShadowType shadow_type;

	/* gdk windows for drawing */
	GdkWindow *view_window;
	GdkWindow *headers_window;
	GdkWindow *html_window;

	/* id handler for dragged scroll */
	guint idle_motion_id;

	/* idle changes count */
	gint idle_changes_count;
	guint idle_readjust_scroll_id;
	guint idle_resize_children_id;

	/* zoom */
	gdouble current_zoom;

	/* link click management */
	gchar *last_url;
};

#define MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										 MODEST_TYPE_GTKHTML_MSG_VIEW, \
										 ModestGtkhtmlMsgViewPrivate))

/* globals */
static GtkContainerClass *parent_class = NULL;

GType
modest_gtkhtml_msg_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestGtkhtmlMsgViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_gtkhtml_msg_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestGtkhtmlMsgView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_gtkhtml_msg_view_init,
			NULL
		};
		static const GInterfaceInfo tny_msg_view_info = 
		{
		  (GInterfaceInitFunc) tny_msg_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo tny_header_view_info = 
		{
		  (GInterfaceInitFunc) tny_header_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo modest_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) modest_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo modest_zoomable_info = 
		{
		  (GInterfaceInitFunc) modest_zoomable_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo modest_isearch_view_info = 
		{
		  (GInterfaceInitFunc) modest_isearch_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo modest_msg_view_info = 
		{
		  (GInterfaceInitFunc) modest_msg_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

 		my_type = g_type_register_static (GTK_TYPE_CONTAINER,
		                                  "ModestGtkhtmlMsgView",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_HEADER_VIEW, 
			&tny_header_view_info);

		g_type_add_interface_static (my_type, TNY_TYPE_MIME_PART_VIEW, 
			&tny_mime_part_view_info);

		g_type_add_interface_static (my_type, MODEST_TYPE_MIME_PART_VIEW, 
			&modest_mime_part_view_info);

		g_type_add_interface_static (my_type, TNY_TYPE_MSG_VIEW, 
			&tny_msg_view_info);

		g_type_add_interface_static (my_type, MODEST_TYPE_ZOOMABLE, 
			&modest_zoomable_info);

		g_type_add_interface_static (my_type, MODEST_TYPE_ISEARCH_VIEW, 
			&modest_isearch_view_info);

		g_type_add_interface_static (my_type, MODEST_TYPE_MSG_VIEW, 
			&modest_msg_view_info);
	}
	return my_type;
}

static void
modest_gtkhtml_msg_view_class_init (ModestGtkhtmlMsgViewClass *klass)
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
	gobject_class->finalize = modest_gtkhtml_msg_view_finalize;
	gobject_class->set_property = set_property;
	gobject_class->get_property = get_property;
	gtkobject_class->destroy = modest_gtkhtml_msg_view_destroy;

	widget_class->realize = realize;
	widget_class->unrealize = unrealize;
	widget_class->expose_event = expose;
	widget_class->size_request = size_request;
	widget_class->size_allocate = size_allocate;

	container_class->forall = forall;
	container_class->remove = container_remove;

	klass->set_header_func = modest_msg_view_set_header_default;
	klass->clear_header_func = modest_msg_view_clear_header_default;
	klass->set_scroll_adjustments = set_scroll_adjustments;
	klass->get_part_func = modest_msg_view_mp_get_part_default;
	klass->set_part_func = modest_msg_view_mp_set_part_default;
	klass->is_empty_func = modest_msg_view_mp_is_empty_default;
	klass->get_msg_func = modest_msg_view_get_msg_default;
	klass->set_msg_func = modest_msg_view_set_msg_default;
	klass->set_unavailable_func = modest_msg_view_set_unavailable_default;
	klass->clear_func = modest_msg_view_clear_default;
	klass->create_mime_part_view_for_func = modest_msg_view_create_mime_part_view_for_default;
	klass->create_new_inline_viewer_func = modest_msg_view_create_new_inline_viewer_default;
	klass->get_zoom_func = modest_msg_view_get_zoom_default;
	klass->set_zoom_func = modest_msg_view_set_zoom_default;
	klass->zoom_minus_func = modest_msg_view_zoom_minus_default;
	klass->zoom_plus_func = modest_msg_view_zoom_plus_default;
	klass->search_func = modest_msg_view_search_default;
	klass->search_next_func = modest_msg_view_search_next_default;
	klass->set_msg_with_other_body_func = modest_gtkhtml_msg_view_set_msg_with_other_body_default;
	klass->get_vadjustment_func = modest_gtkhtml_msg_view_get_vadjustment_default;
	klass->get_hadjustment_func = modest_gtkhtml_msg_view_get_hadjustment_default;
	klass->set_vadjustment_func = modest_gtkhtml_msg_view_set_vadjustment_default;
	klass->set_hadjustment_func = modest_gtkhtml_msg_view_set_hadjustment_default;
	klass->get_shadow_type_func = modest_gtkhtml_msg_view_get_shadow_type_default;
	klass->set_shadow_type_func = modest_gtkhtml_msg_view_set_shadow_type_default;
	klass->get_priority_func = modest_gtkhtml_msg_view_get_priority_default;
	klass->set_priority_func = modest_gtkhtml_msg_view_set_priority_default;
	klass->get_selected_attachments_func = modest_gtkhtml_msg_view_get_selected_attachments_default;
	klass->get_attachments_func = modest_gtkhtml_msg_view_get_attachments_default;
	klass->grab_focus_func = modest_gtkhtml_msg_view_grab_focus_default;
	klass->remove_attachment_func = modest_gtkhtml_msg_view_remove_attachment_default;
	klass->request_fetch_images_func = modest_gtkhtml_msg_view_request_fetch_images_default;
	klass->set_branding_func = modest_gtkhtml_msg_view_set_branding_default;
	klass->has_blocked_external_images_func = modest_gtkhtml_msg_view_has_blocked_external_images_default;

	g_type_class_add_private (gobject_class, sizeof(ModestGtkhtmlMsgViewPrivate));

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

	widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ModestGtkhtmlMsgViewClass, set_scroll_adjustments),
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
	ModestGtkhtmlMsgView *self = MODEST_GTKHTML_MSG_VIEW (object);

	switch (prop_id) {
	case PROP_HADJUSTMENT:
		set_hadjustment (self, g_value_get_object (value));
		break;
	case PROP_VADJUSTMENT:
		set_vadjustment (self, g_value_get_object (value));
		break;
	case PROP_SHADOW_TYPE:
		set_shadow_type (self, g_value_get_enum (value));
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
	ModestGtkhtmlMsgView *self = MODEST_GTKHTML_MSG_VIEW (object);
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

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
disconnect_hadjustment (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	if (priv->hadj) {
		g_signal_handlers_disconnect_by_func(priv->hadj, adjustment_value_changed, self);
		g_object_unref (priv->hadj);
		priv->hadj = NULL;
	}
}

static void
disconnect_vadjustment (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	if (priv->vadj) {
		g_signal_handlers_disconnect_by_func(priv->vadj, adjustment_value_changed, self);
		g_object_unref (priv->vadj);
		priv->vadj = NULL;
	}
}

static void 
get_view_allocation (ModestGtkhtmlMsgView *self, GtkAllocation *allocation)
{
	/* This method gets the allocation of the widget in parent widget. It's the
	   real position and size of the widget */
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	
	allocation->x = 0;
	allocation->y = 0;

	if (priv->shadow_type != GTK_SHADOW_NONE) {
		allocation->x = GTK_WIDGET (self)->style->xthickness;
		allocation->y = GTK_WIDGET (self)->style->ythickness;
	}

	allocation->width = MAX (1, (GTK_WIDGET (self)->allocation.width) - allocation->x * 2);
	allocation->height = MAX (1, (GTK_WIDGET (self)->allocation.height) - allocation->y * 2);

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
set_hadjustment_values (ModestGtkhtmlMsgView *self,
			gboolean *value_changed)
{
	GtkAllocation view_allocation;
	GtkAdjustment *hadj = get_hadjustment (self);
	gint full_width = 0;
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	get_view_allocation (self, &view_allocation);
	hadj->page_size = view_allocation.width;
	hadj->step_increment = view_allocation.width * 0.1;
	hadj->page_increment = view_allocation.width * 0.9;

	hadj->lower = 0;
	hadj->upper = view_allocation.width;

	/* Get the real width of the embedded html */
	if (priv->html_scroll && GTK_WIDGET_VISIBLE(priv->html_scroll)) {
		GtkAdjustment *html_hadj;
		html_hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
		full_width += html_hadj->upper;
	}

	hadj->upper = MAX (view_allocation.width, full_width);

	reclamp_adjustment (hadj, value_changed);

}

static void
set_vadjustment_values (ModestGtkhtmlMsgView *self,
			gboolean *value_changed)
{
	GtkAllocation view_allocation;
	GtkAdjustment *vadj = get_vadjustment (self);
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	gint full_height = 0;
	GtkAdjustment *html_vadj;

	get_view_allocation (self, &view_allocation);
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
set_scroll_adjustments (ModestGtkhtmlMsgView *self,
			GtkAdjustment *hadj,
			GtkAdjustment *vadj)
{
	set_hadjustment (self, hadj);
	set_vadjustment (self, vadj);

#ifndef MODEST_TOOLKIT_HILDON2
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->body_view), vadj);
#endif
}

static void
realize (GtkWidget *widget)
{
	ModestGtkhtmlMsgView *self = MODEST_GTKHTML_MSG_VIEW (widget);
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	GtkAdjustment *hadj = get_hadjustment (self);
	GtkAdjustment *vadj = get_vadjustment (self);
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
	 *      body view showing the contents of the mail).
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
	gdk_window_set_user_data (widget->window, self);

	get_view_allocation (self, &view_allocation);

	attributes.x = view_allocation.x;
	attributes.y = view_allocation.y;
	attributes.width = view_allocation.width;
	attributes.height = view_allocation.height;
	attributes.event_mask = 0;
	priv->view_window = gdk_window_new (widget->window, &attributes, attributes_mask);
	gdk_window_set_user_data (priv->view_window, self);
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
	gdk_window_set_user_data (priv->headers_window, self);

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
	gdk_window_set_user_data (priv->html_window, self);

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
	ModestGtkhtmlMsgView *self = MODEST_GTKHTML_MSG_VIEW (widget);
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

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
	ModestGtkhtmlMsgView *self;
	ModestGtkhtmlMsgViewPrivate *priv;

	if (GTK_WIDGET_DRAWABLE (widget)) {
		self = MODEST_GTKHTML_MSG_VIEW (widget);
		priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
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
			gtk_container_propagate_expose (GTK_CONTAINER (self), priv->headers_box, event);
		if (priv->html_scroll)
			gtk_container_propagate_expose (GTK_CONTAINER (self), priv->html_scroll, event);
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
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (container);
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
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (container);
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
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (widget);
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
	ModestGtkhtmlMsgView *self = MODEST_GTKHTML_MSG_VIEW (widget);
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	gboolean hadj_value_changed, vadj_value_changed;
	GtkAllocation headers_allocation, html_allocation, view_allocation;
	GtkAdjustment *html_vadj;

	if (GTK_WIDGET_MAPPED (widget) &&
	    priv->shadow_type != GTK_SHADOW_NONE && 
	    (allocation->width != widget->allocation.width ||
	     allocation->height != widget->allocation.height))
		gdk_window_invalidate_rect (widget->window, NULL, FALSE);

	if (priv->idle_readjust_scroll_id == 0 && priv->idle_changes_count < 5 && widget->allocation.width != allocation->width) {
		g_object_ref (self);
		priv->idle_readjust_scroll_id = g_idle_add ((GSourceFunc) idle_readjust_scroll, self);
		priv->idle_changes_count ++;
	}

	widget->allocation = *allocation;
	set_hadjustment_values (self, &hadj_value_changed);
	set_vadjustment_values (self, &vadj_value_changed);

	get_view_allocation (self, &view_allocation);

	headers_allocation.x = 0;
	headers_allocation.y = 0;
	headers_allocation.width = view_allocation.width;
	if (priv->headers_box)
		headers_allocation.height = GTK_WIDGET (priv->headers_box)->requisition.height;
	else
		headers_allocation.height = 0;

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));

	html_allocation.x = 0;
	html_allocation.y = MAX (0, headers_allocation.height - priv->vadj->value);
	html_allocation.width = view_allocation.width;
	if (html_vadj->upper < allocation->height) {
		html_allocation.height = MAX (html_vadj->upper, allocation->height - headers_allocation.height);
	} else {
		html_allocation.height = allocation->height;
	}
	/* html_allocation.height = MAX ((gint) html_vadj->upper, (gint)(priv->vadj->upper - headers_allocation.height)); */

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
					(gint) html_allocation.y,
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
		gtk_widget_size_allocate (priv->html_scroll, &html_allocation);
		if (html_vadj->page_size != html_allocation.height) {
			html_vadj->page_size = html_allocation.height;
			gtk_adjustment_changed (html_vadj);
		}
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
	ModestGtkhtmlMsgView *self = NULL;
	ModestGtkhtmlMsgViewPrivate *priv = NULL;

	g_return_if_fail (GTK_IS_ADJUSTMENT (adj));
	g_return_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (data));

	self = MODEST_GTKHTML_MSG_VIEW (data);
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	if (GTK_WIDGET_REALIZED (self)) {
		GtkAdjustment *hadj = get_hadjustment (self);
		GtkAdjustment *vadj = get_vadjustment (self);
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
			gint new_internal_vvalue;
			gdk_window_get_position (priv->html_window, &old_x, &old_y);
			new_x = -hadj->value;

			new_internal_vvalue = MAX (0, vadj->value - headers_offset);
			gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll)),
						  new_internal_vvalue);

			new_y = MAX (0, headers_offset - vadj->value);

			if (new_x != old_x || new_y != old_y) {
				gdk_window_move (priv->html_window, new_x, new_y);
				gdk_window_process_updates (priv->html_window, TRUE);
			}
		}
		
	}
}

static gboolean
resize_children_idle (GtkContainer *cont)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (cont);
	if (GTK_WIDGET_DRAWABLE (cont)) {
		gtk_container_resize_children (cont);
	}
	priv->idle_resize_children_id = 0;

	return FALSE;
}

static void
html_adjustment_changed (GtkAdjustment *adj,
			 gpointer userdata)
{
	ModestGtkhtmlMsgView *self = MODEST_GTKHTML_MSG_VIEW (userdata);
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	GtkAdjustment *html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(priv->html_scroll));
	gboolean vadj_changed;
	gint new_height;

	g_signal_stop_emission_by_name (G_OBJECT (adj), "changed");

	priv->html_scroll->requisition.height = html_vadj->upper;
	if ((priv->vadj != NULL && (html_vadj->upper == (priv->vadj->upper - GTK_WIDGET (priv->headers_box)->allocation.height)))
	    && html_vadj->page_size == priv->html_scroll->allocation.height)
		return;

	set_vadjustment_values (self, &vadj_changed);

	if (html_vadj->upper < GTK_WIDGET (self)->allocation.height) {
		new_height = MAX (html_vadj->upper, 
					GTK_WIDGET (self)->allocation.height - GTK_WIDGET(priv->headers_box)->allocation.height);
	} else {
		new_height = GTK_WIDGET (self)->allocation.height;
	}
	
	gtk_adjustment_changed (priv->vadj);
	if (GTK_WIDGET_DRAWABLE (priv->html_scroll)) {
		gdk_window_resize (priv->html_window, (gint) priv->hadj->upper, (gint) new_height);
		gdk_window_process_updates (priv->view_window, TRUE);
		if (priv->idle_resize_children_id == 0)
			priv->idle_resize_children_id = gdk_threads_add_idle ((GSourceFunc) resize_children_idle, self);
	}
	
}

gboolean
idle_readjust_scroll (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	/* We're out the main lock */
	gdk_threads_enter ();

	if (GTK_WIDGET_DRAWABLE (self)) {
		GtkAdjustment *html_vadj;
		GtkAdjustment *html_hadj;

		html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
		html_vadj->upper = 0;
		html_vadj->page_size = 0;
		g_signal_emit_by_name (G_OBJECT (html_vadj), "changed");

		html_hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
		html_hadj->upper = 0;
		html_hadj->page_size = 0;
		g_signal_emit_by_name (G_OBJECT (html_hadj), "changed");

		gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
		gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);

		gtk_widget_queue_resize (GTK_WIDGET (priv->body_view));
		gtk_widget_queue_draw (GTK_WIDGET (priv->body_view));

		/* Just another hack for making readjust really work. This forces an update
		 * of the scroll, and then, make the scroll really update properly the
		 * the size and not corrupt scrollable area */
		gtk_adjustment_set_value (priv->vadj, 1.0);
		gtk_adjustment_set_value (priv->vadj, 0.0);

	}
	priv->idle_readjust_scroll_id = 0;
	g_object_unref (G_OBJECT (self));


	gdk_threads_leave ();

	return FALSE;
}

static void
modest_gtkhtml_msg_view_init (ModestGtkhtmlMsgView *obj)
{
 	ModestGtkhtmlMsgViewPrivate *priv;
	GtkAdjustment *html_vadj;

	GTK_WIDGET_UNSET_FLAGS (obj, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (obj), TRUE);
	gtk_container_set_reallocate_redraws (GTK_CONTAINER (obj), TRUE);
	gtk_container_set_resize_mode (GTK_CONTAINER (obj), GTK_RESIZE_QUEUE);
	
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE(obj);

	priv->idle_changes_count = 0;
	priv->idle_readjust_scroll_id = 0;
	priv->idle_resize_children_id = 0;
	priv->current_zoom = 1.0;

	priv->hadj = NULL;
	priv->vadj = NULL;
	priv->shadow_type = GTK_SHADOW_IN;
	priv->view_window = NULL;
	priv->headers_window = NULL;
	priv->html_window = NULL;

	priv->idle_motion_id = 0;

	gtk_widget_push_composite_child ();
	priv->html_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_composite_name (priv->html_scroll, "contents");
	gtk_widget_pop_composite_child ();
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->html_scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

	priv->msg                     = NULL;

	priv->body_view                 = GTK_WIDGET (g_object_new (MODEST_TYPE_GTKHTML_MIME_PART_VIEW, NULL));
#ifdef MODEST_TOOLKIT_HILDON2
	priv->mail_header_view        = GTK_WIDGET(modest_compact_mail_header_view_new ());
#else
	priv->mail_header_view        = GTK_WIDGET(modest_expander_mail_header_view_new (TRUE));
#endif
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);
	priv->attachments_view        = GTK_WIDGET(modest_attachments_view_new (NULL));
#ifdef MODEST_TOOLKIT_HILDON2
	modest_attachments_view_set_style (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
					   MODEST_ATTACHMENTS_VIEW_STYLE_LINKS);
#else
	modest_attachments_view_set_style (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
					   MODEST_ATTACHMENTS_VIEW_STYLE_SELECTABLE);
#endif

	g_signal_connect (G_OBJECT(priv->body_view), "activate_link",
				       G_CALLBACK(on_activate_link), obj);
	g_signal_connect (G_OBJECT(priv->body_view), "fetch_url",
				       G_CALLBACK(on_fetch_url), obj);
	g_signal_connect (G_OBJECT(priv->body_view), "link_hover",
				       G_CALLBACK(on_link_hover), obj);
	g_signal_connect (G_OBJECT(priv->body_view), "limit_error",
			  G_CALLBACK(on_limit_error), obj);
#ifdef MAEMO_CHANGES
	g_signal_connect (G_OBJECT(priv->body_view), "motion-notify-event",
			  G_CALLBACK (motion_notify_event), obj);
#endif
	g_signal_connect (G_OBJECT (priv->body_view), "button-press-event",
			  G_CALLBACK (button_press_event), obj);
	g_signal_connect (G_OBJECT (priv->body_view), "button-release-event",
			  G_CALLBACK (button_release_event), obj);

	g_signal_connect (G_OBJECT (priv->mail_header_view), "recpt-activated", 
			  G_CALLBACK (on_recpt_activated), obj);
	g_signal_connect (G_OBJECT (priv->mail_header_view), "show-details", 
			  G_CALLBACK (on_show_details), obj);

	g_signal_connect (G_OBJECT (priv->attachments_view), "activate",
			  G_CALLBACK (on_attachment_activated), obj);

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(priv->html_scroll));

	g_signal_connect (G_OBJECT (html_vadj), "changed",
			  G_CALLBACK (html_adjustment_changed), obj);

	gtk_widget_push_composite_child ();
	priv->headers_box = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	gtk_widget_set_composite_name (priv->headers_box, "headers");
	gtk_widget_pop_composite_child ();

	if (priv->mail_header_view)
		gtk_box_pack_start (GTK_BOX(priv->headers_box), priv->mail_header_view, FALSE, FALSE, 0);

#ifdef MODEST_TOOLKIT_HILDON2
	priv->priority_icon = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->priority_icon), 0.0, 0.5);
	if (priv->priority_icon) {
		priv->priority_box = (GtkWidget *)
			modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view),
								   _("mcen_me_editor_message_priority"),
								   priv->priority_icon,
								   FALSE, FALSE);
								   
		gtk_widget_hide_all (priv->priority_box);
	}
#endif
	if (priv->attachments_view) {
#ifndef MODEST_TOOLKIT_HILDON2
		gchar *att_label = g_strconcat (_("mcen_me_viewer_attachments"), ":", NULL);
#else
		gchar *att_label = g_strconcat (_("mail_va_attachment"), ":", NULL);
#endif

		priv->attachments_box = (GtkWidget *)
			modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view),
								   att_label,
								   priv->attachments_view,
								   FALSE, FALSE);
		gtk_widget_hide_all (priv->attachments_box);
		g_free (att_label);
	}

#ifndef MODEST_TOOLKIT_HILDON2
	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX(priv->headers_box), separator, FALSE, FALSE, 0);
#endif

	gtk_widget_set_parent (priv->headers_box, GTK_WIDGET (obj));

	if (priv->body_view) {
		gtk_container_add (GTK_CONTAINER (priv->html_scroll), priv->body_view);
		gtk_widget_set_parent (priv->html_scroll, GTK_WIDGET(obj));
#ifdef MAEMO_CHANGES
		gtk_widget_tap_and_hold_setup (GTK_WIDGET (priv->body_view), NULL, NULL, 0);
		g_signal_connect (G_OBJECT (priv->body_view), "tap-and-hold", G_CALLBACK (on_tap_and_hold), obj);
		g_signal_connect (G_OBJECT (priv->body_view), "tap-and-hold-query", G_CALLBACK (on_tap_and_hold_query), obj);
#endif
	}
	
}
	

static void
modest_gtkhtml_msg_view_finalize (GObject *obj)
{	
	ModestGtkhtmlMsgViewPrivate *priv;
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (obj);

	if (priv->msg) {
		g_object_unref (G_OBJECT(priv->msg));
		priv->msg = NULL;
	}

	if (priv->idle_resize_children_id > 0) {
		g_source_remove (priv->idle_resize_children_id);
		priv->idle_resize_children_id = 0;
	}

	if (priv->idle_readjust_scroll_id > 0) {
		g_source_remove (priv->idle_readjust_scroll_id);
		priv->idle_readjust_scroll_id = 0;
	}

	if (priv->idle_motion_id > 0) {
		g_source_remove (priv->idle_motion_id);
		priv->idle_motion_id = 0;
	}
	
	/* we cannot disconnect sigs, because priv->body_view is
	 * already dead */
	
	disconnect_vadjustment (MODEST_GTKHTML_MSG_VIEW(obj));
	disconnect_hadjustment (MODEST_GTKHTML_MSG_VIEW(obj));

	priv->body_view = NULL;
	priv->attachments_view = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}

static void
modest_gtkhtml_msg_view_destroy (GtkObject *obj)
{	
	disconnect_vadjustment (MODEST_GTKHTML_MSG_VIEW(obj));
	disconnect_hadjustment (MODEST_GTKHTML_MSG_VIEW(obj));

	GTK_OBJECT_CLASS(parent_class)->destroy (obj);		
}

/* INTERNAL METHODS */

#ifdef MAEMO_CHANGES
static gboolean 
motion_notify_event (GtkWidget *widget,
		     GdkEventMotion *event,
		     gpointer userdata)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (userdata);

	/* Use panning information in gtkhtml widget to support also vertical panning */

	if (GTK_HTML (widget)->panning) {
		gint y, dy;
		gdouble value;

		gdk_window_get_pointer (GTK_LAYOUT (widget)->bin_window, NULL, &y, NULL);
		dy = y - GTK_HTML (widget)->lasty;
		value = priv->vadj->value - (gdouble) dy;

		if (value < priv->vadj->lower)
			value = priv->vadj->lower;
		else if (value > priv->vadj->upper - priv->vadj->page_size)
			value = priv->vadj->upper - priv->vadj->page_size;
		gtk_adjustment_set_value (priv->vadj, value);
		
	} 
	return FALSE;
}
#endif

static gboolean
idle_motion (gpointer userdata)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (userdata);
	if (GTK_HTML (priv->body_view)->in_selection_drag) {
		gdouble offset;
		GtkAdjustment *adj;
		gint gdk_y;
		gdk_window_get_pointer (gtk_widget_get_parent_window (priv->body_view), NULL, &gdk_y, NULL);
		offset= (gdouble) (priv->headers_box->requisition.height + gdk_y);
		adj = GTK_ADJUSTMENT (priv->vadj);
		if (offset < adj->value + adj->step_increment) {
			gtk_adjustment_set_value (adj, MAX (offset + adj->page_increment - adj->page_size, 0.0));
		} else if (offset > adj->value + adj->page_increment) {
			gtk_adjustment_set_value (adj, MIN (offset - adj->page_increment, adj->upper - adj->page_size));
		}
		gtk_widget_queue_resize (userdata);
	}
	return TRUE;
}

static gboolean 
button_press_event (GtkWidget *widget,
		    GdkEventButton *event,
		    gpointer userdata)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (userdata);

	if (priv->idle_motion_id == 0) {
		priv->idle_motion_id = g_timeout_add (200, idle_motion, userdata);
	}
	return FALSE;
}

static gboolean 
button_release_event (GtkWidget *widget,
		      GdkEventButton *event,
		      gpointer userdata)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (userdata);

	if (priv->idle_motion_id > 0) {
		gint gdk_y;
		g_source_remove (priv->idle_motion_id);
		
		priv->idle_motion_id = 0;;
		gdk_window_get_pointer (gtk_widget_get_parent_window (priv->body_view), NULL, &gdk_y, NULL);
		event->y = (gdouble) gdk_y;
	}
	return FALSE;
}

static GtkAdjustment *
get_vadjustment (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	if (!priv->vadj)
		set_vadjustment (self, NULL);

	return priv->vadj;
	
}

static GtkAdjustment *
get_hadjustment (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	if (!priv->hadj)
		set_hadjustment (self, NULL);

	return priv->hadj;
	
}

static void
set_hadjustment (ModestGtkhtmlMsgView *self, GtkAdjustment *hadj)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	gboolean value_changed;
	
	if (hadj && hadj == priv->hadj)
		return;

	if (!hadj)
		hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,0.0,0.0,0.0,0.0,0.0));
	disconnect_hadjustment (self);
	g_object_ref (G_OBJECT (hadj));
	gtk_object_sink (GTK_OBJECT (hadj));
	priv->hadj = hadj;
	set_hadjustment_values (self, &value_changed);

	g_signal_connect (hadj, "value_changed", G_CALLBACK (adjustment_value_changed),
			  self);

	gtk_adjustment_changed (hadj);
	if (value_changed)
		gtk_adjustment_value_changed (hadj);
	else
		adjustment_value_changed (hadj, self);

	g_object_notify (G_OBJECT (self), "hadjustment");
}

static void
set_vadjustment (ModestGtkhtmlMsgView *self, GtkAdjustment *vadj)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	gboolean value_changed;

	if (vadj && vadj == priv->vadj)
		return;

	if (!vadj)
		vadj = (GtkAdjustment *) gtk_adjustment_new (0.0,0.0,0.0,0.0,0.0,0.0);
	disconnect_vadjustment (self);
	g_object_ref (G_OBJECT (vadj));
	gtk_object_sink (GTK_OBJECT (vadj));
	priv->vadj = vadj;
	set_vadjustment_values (self, &value_changed);

	g_signal_connect (vadj, "value_changed", G_CALLBACK (adjustment_value_changed),
			  self);

	gtk_adjustment_changed (vadj);
	if (value_changed)
		gtk_adjustment_value_changed (vadj);
	else
		adjustment_value_changed (vadj, self);

	g_object_notify (G_OBJECT (self), "vadjustment");
}

static void
set_shadow_type (ModestGtkhtmlMsgView *self,
		 GtkShadowType shadow_type)
{
	ModestGtkhtmlMsgViewPrivate *priv;
	g_return_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self));

	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	
	if (priv->shadow_type != shadow_type) {
		priv->shadow_type = shadow_type;
		
		if (GTK_WIDGET_VISIBLE (self)) {
			gtk_widget_size_allocate (GTK_WIDGET (self), &(GTK_WIDGET (self)->allocation));
			gtk_widget_queue_draw (GTK_WIDGET (self));
		}
		g_object_notify (G_OBJECT (self), "shadow-type");
	}
}

static GtkShadowType
get_shadow_type (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv;
	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), GTK_SHADOW_NONE);
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	
	return priv->shadow_type;
}

GtkWidget*
modest_msg_view_new (TnyMsg *msg)
{
	GObject *obj;
	ModestGtkhtmlMsgView* self;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_GTKHTML_MSG_VIEW, NULL));
	self = MODEST_GTKHTML_MSG_VIEW(obj);
	tny_msg_view_set_msg (TNY_MSG_VIEW (self), msg);

	return GTK_WIDGET(self);
}

#ifdef MAEMO_CHANGES
static void
on_tap_and_hold (GtkWidget *widget,
		 gpointer data)
{
	ModestGtkhtmlMsgView *self = (ModestGtkhtmlMsgView *) data;
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	g_signal_emit_by_name (G_OBJECT (self), "link-contextual", priv->last_url);
}

static gboolean
on_tap_and_hold_query (GtkWidget *widget,
		       GdkEvent *event,
		       gpointer data)
{
	ModestGtkhtmlMsgView *self = (ModestGtkhtmlMsgView *) data;
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	/* Don't show the tap and hold animation if no url below */
	return (priv->last_url == NULL);
}
#endif

static void
on_recpt_activated (ModestMailHeaderView *header_view, 
		    const gchar *address,
		    ModestGtkhtmlMsgView *self)
{
	g_signal_emit_by_name (G_OBJECT (self), "recpt-activated", address);
}

static void
on_show_details (ModestMailHeaderView *header_view, 
		 ModestGtkhtmlMsgView *self)
{
	g_signal_emit_by_name (G_OBJECT (self), "show-details");
}

static void
on_attachment_activated (ModestAttachmentsView * att_view, TnyMimePart *mime_part, gpointer self)
{

	g_signal_emit_by_name (G_OBJECT(self), "attachment_clicked", mime_part);
}

static void
request_fetch_images (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	TnyMimePart *part;

	/* The message could have not been downloaded yet */
	if (priv->msg) {
		modest_mime_part_view_set_view_images (MODEST_MIME_PART_VIEW (priv->body_view), TRUE);
		part = tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (priv->body_view));
		if (part) {
			tny_mime_part_view_set_part (TNY_MIME_PART_VIEW (priv->body_view), part);
			g_object_unref (part);
		}
		tny_msg_set_allow_external_images (TNY_MSG (priv->msg), TRUE);
	}
}

static void
set_branding (ModestGtkhtmlMsgView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	modest_mail_header_view_set_branding (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), brand_name, brand_icon);
}

static gboolean
has_blocked_external_images (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	return (modest_mime_part_view_has_external_images (MODEST_MIME_PART_VIEW (priv->body_view)) &&
		!modest_mime_part_view_get_view_images (MODEST_MIME_PART_VIEW (priv->body_view)));
}

static gboolean
on_activate_link (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMsgView *self)
{
	gboolean result;
	g_return_val_if_fail (self, FALSE);

	g_signal_emit_by_name (G_OBJECT(self), "activate-link", uri, &result);

	return result;
}


static gboolean
on_link_hover (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	gboolean result;

	g_free (priv->last_url);
	priv->last_url = g_strdup (uri);

	g_signal_emit_by_name (G_OBJECT(self), "link-hover", uri, &result);

	return result;
}

static void 
on_limit_error (GtkWidget *widget, ModestGtkhtmlMsgView *msg_view)
{
	g_signal_emit_by_name (G_OBJECT (msg_view), "limit-error");
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

		/* if there is no content id, try the content location;
		 * this is what Outlook seems to use when it converts
		 * it's internal richtext to html
		 */
		if (!part_cid)
			part_cid = tny_mime_part_get_content_location (part);
		
		if (part_cid && strcmp (cid, part_cid) == 0)
			break;

		if (tny_mime_part_content_type_is (part, "multipart/related")) {
			TnyList *related_parts = TNY_LIST (tny_simple_list_new ());
			TnyIterator *related_iter = NULL;
			TnyMimePart *related_part = NULL;

			tny_mime_part_get_parts (part, related_parts);
			related_iter = tny_list_create_iterator (related_parts);

			while (!tny_iterator_is_done (related_iter)) {
				related_part = TNY_MIME_PART (tny_iterator_get_current (related_iter));
				part_cid = tny_mime_part_get_content_id (related_part);
				if (part_cid && strcmp (cid, part_cid) == 0) {
					break;
				}
				g_object_unref (related_part);
				related_part = NULL;
				tny_iterator_next (related_iter);
			}

			g_object_unref (related_iter);
			g_object_unref (related_parts);
			if (related_part != NULL) {
				g_object_unref (part);
				part = related_part;
				break;
			}
		}

		g_object_unref (G_OBJECT(part));
	
		part = NULL;
		tny_iterator_next (iter);
	}
	
	g_object_unref (G_OBJECT(iter));	
	g_object_unref (G_OBJECT(parts));
	
	return part;
}


static gboolean
on_fetch_url (GtkWidget *widget, const gchar *uri,
	      TnyStream *stream, ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv;
	const gchar* my_uri;
	TnyMimePart *part = NULL;
	


	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	/*
	 * we search for either something starting with cid:, or something
	 * with no prefix at all; this latter case occurs when sending mails
	 * with MS Outlook in rich-text mode, and 'attach-as-object
	 */
	if (g_str_has_prefix (uri, "cid:"))  
		my_uri = uri + 4;  /* +4 ==> skip "cid:" */
	else
		my_uri = uri;
	
	/* now try to find the embedded image */
	part = find_cid_image (priv->msg, my_uri);

	if (!part) {
		if (g_str_has_prefix (uri, "http:")) {
			if (modest_mime_part_view_get_view_images (MODEST_MIME_PART_VIEW (priv->body_view))) {
				gboolean result = FALSE;
				g_signal_emit_by_name (self, "fetch-image", uri, stream, &result);
				return result;
			} else {
				/* we return immediately to get a "image not found" icon */
				tny_stream_close (stream);
				return TRUE;
			}
		} else {
			return FALSE;
		}
	}

	tny_mime_part_decode_to_stream ((TnyMimePart*)part, stream, NULL);
	tny_stream_close (stream);
	g_object_unref (G_OBJECT(part));
	return TRUE;
}

static void
set_message (ModestGtkhtmlMsgView *self, TnyMsg *msg, TnyMimePart *other_body)
{
	TnyMimePart *body;
	ModestGtkhtmlMsgViewPrivate *priv;
	TnyHeader *header;
	GtkAdjustment *html_vadj, *html_hadj;

	g_return_if_fail (self);

	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE(self);
	modest_mail_header_view_set_loading (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), FALSE);
	gtk_widget_set_no_show_all (priv->mail_header_view, FALSE);
	modest_mime_part_view_set_view_images (MODEST_MIME_PART_VIEW (priv->body_view), FALSE);

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
#ifdef MODEST_TOOKIT_HILDON2
		gtk_widget_hide_all (priv->priority_box);
#endif
		gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);
		tny_mime_part_view_clear (TNY_MIME_PART_VIEW (priv->body_view));

		html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
		html_vadj->upper = 0;
		html_vadj->page_size = 0;
		g_signal_emit_by_name (G_OBJECT (html_vadj), "changed");

		html_hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
		html_hadj->upper = 0;
		html_hadj->page_size = 0;
		g_signal_emit_by_name (G_OBJECT (html_hadj), "changed");

		gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
		gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);

		priv->idle_changes_count = 0;
		gtk_widget_queue_resize (GTK_WIDGET (priv->body_view));

		gtk_widget_queue_resize (GTK_WIDGET(self));
		gtk_widget_queue_draw (GTK_WIDGET(self));
		return;
	}

	header = tny_msg_get_header (msg);
	tny_header_view_set_header (TNY_HEADER_VIEW (priv->mail_header_view), header);
	g_object_unref (header);

	modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
					     other_body?NULL:msg);

	modest_mime_part_view_set_view_images (MODEST_MIME_PART_VIEW (priv->body_view), tny_msg_get_allow_external_images (msg));

	if (other_body) {
		body = other_body;
		g_object_ref (body);
	} else {
		body = modest_tny_msg_find_body_part (msg, TRUE);
	}
	if (body) {
		ModestAttachmentsView *widget;

		tny_mime_part_view_set_part (TNY_MIME_PART_VIEW (priv->body_view), body);
		g_object_unref (body);
		widget = MODEST_ATTACHMENTS_VIEW (priv->attachments_view);

		if (modest_attachments_view_has_attachments (widget)) {
			GtkLabel *label;
			GList *children = NULL;
			gchar *text = NULL;
			/* Ugly but... */
			children = gtk_container_get_children (GTK_CONTAINER (priv->attachments_box));
			label = GTK_LABEL (children->data);
			gtk_widget_show_all (priv->attachments_box);
#ifdef MODEST_TOOLKIT_HILDON2
			if (modest_attachments_view_get_num_attachments (widget) > 1) {
				text = _("mail_va_attachments");
			} else {
				text = _("mail_va_attachment");
			}
#else
				text = _("mail_va_attachment");
#endif
			gtk_label_set_text (label, text);
		} else {
			gtk_widget_hide_all (priv->attachments_box);
		}

	} else {
		tny_mime_part_view_clear (TNY_MIME_PART_VIEW (priv->body_view));
	}

	/* Refresh priority */
	set_priority (self, tny_header_get_flags (header));

	gtk_widget_show (priv->body_view);
#ifdef MODEST_TOOLKIT_HILDON2
	gtk_widget_set_no_show_all (priv->priority_box, TRUE);
#endif
	gtk_widget_set_no_show_all (priv->attachments_box, TRUE);
	gtk_widget_show_all (priv->mail_header_view);
	gtk_widget_set_no_show_all (priv->attachments_box, FALSE);
#ifdef MODEST_TOOLKIT_HILDON2
	gtk_widget_set_no_show_all (priv->priority_box, FALSE);
#endif
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
	html_vadj->upper = 0;
	html_vadj->page_size = 0;
	g_signal_emit_by_name (G_OBJECT (html_vadj), "changed");

	html_hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
	html_hadj->upper = 0;
	html_hadj->page_size = 0;
	g_signal_emit_by_name (G_OBJECT (html_hadj), "changed");

	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);

	priv->idle_changes_count = 0;
	gtk_widget_queue_resize (GTK_WIDGET (priv->body_view));

	gtk_widget_queue_resize (GTK_WIDGET(self));
	gtk_widget_queue_draw (GTK_WIDGET(self));

	if (priv->hadj != NULL)
		priv->hadj->value = 0.0;
	if (priv->vadj != NULL)
		priv->vadj->value = 0.0;

	g_signal_emit_by_name (G_OBJECT (html_vadj), "changed");

	/* This is a hack to force reallocation of scroll after drawing all the stuff. This
	 * makes the html view get the proper and expected size and prevent being able to scroll
	 * the buffer when it shouldn't be scrollable */
	if (priv->idle_readjust_scroll_id == 0) {
		g_object_ref (self);
		priv->idle_readjust_scroll_id = g_idle_add ((GSourceFunc) idle_readjust_scroll, self);
	}
}

static void
set_header (ModestGtkhtmlMsgView *self, TnyHeader *header)
{
	ModestGtkhtmlMsgViewPrivate *priv;
	GtkAdjustment *html_vadj, *html_hadj;
	
	g_return_if_fail (self);

	if (header == NULL) {
		set_message (self, NULL, NULL);
		return;
	}
	
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE(self);
	modest_mail_header_view_set_loading (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), TRUE);
	gtk_widget_set_no_show_all (priv->mail_header_view, FALSE);
	modest_mime_part_view_set_view_images (MODEST_MIME_PART_VIEW (priv->body_view), FALSE);

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
	html_vadj->upper = 0;
	html_vadj->page_size = 0;
	g_signal_emit_by_name (G_OBJECT (html_vadj), "changed");

	html_hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
	html_hadj->upper = 0;
	html_hadj->page_size = 0;
	g_signal_emit_by_name (G_OBJECT (html_hadj), "changed");

	priv->idle_changes_count = 0;
	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);
	gtk_widget_queue_resize (GTK_WIDGET (priv->body_view));

	if (priv->msg) {
		g_object_unref (G_OBJECT(priv->msg));
	}
	priv->msg = NULL;
	
	tny_header_view_set_header (TNY_HEADER_VIEW (priv->mail_header_view), header);
	modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), NULL);
	gtk_widget_show_all (priv->mail_header_view);
	gtk_widget_hide_all (priv->attachments_box);
#ifdef MODEST_TOOLKIT_HILDON2
	gtk_widget_hide_all (priv->priority_box);
#endif
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);
	tny_mime_part_view_clear (TNY_MIME_PART_VIEW (priv->body_view));
	priv->idle_changes_count = 0;
	gtk_widget_queue_resize (GTK_WIDGET(self));
	gtk_widget_queue_draw (GTK_WIDGET(self));
}


static TnyMsg*
get_message (ModestGtkhtmlMsgView *self)
{
	TnyMsg *msg;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), NULL);

	msg = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE(self)->msg;

	if (msg)
		g_object_ref (msg);
	
	return msg;
}

static gboolean 
is_empty (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	return modest_mime_part_view_is_empty (MODEST_MIME_PART_VIEW (priv->body_view));
}

static void
set_zoom (ModestGtkhtmlMsgView *self, gdouble zoom)
{
	ModestGtkhtmlMsgViewPrivate *priv;
	GtkAdjustment *html_vadj, *html_hadj;

	g_return_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self));
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	modest_zoomable_set_zoom (MODEST_ZOOMABLE(priv->body_view), zoom);

	html_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
	html_vadj->upper = 0;
	html_vadj->page_size = 0;
	g_signal_emit_by_name (G_OBJECT (html_vadj), "changed");

	html_hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (priv->html_scroll));
	html_hadj->upper = 0;
	html_hadj->page_size = 0;
	g_signal_emit_by_name (G_OBJECT (html_hadj), "changed");

	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);

	priv->idle_changes_count = 0;
	gtk_widget_queue_resize (priv->body_view);
}

static gdouble
get_zoom (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), 1.0);
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	return modest_zoomable_get_zoom (MODEST_ZOOMABLE (priv->body_view));
}

static TnyHeaderFlags
get_priority (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), 0);

	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	return modest_mail_header_view_get_priority (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view));
}

static void
set_priority (ModestGtkhtmlMsgView *self, TnyHeaderFlags flags)
{
	ModestGtkhtmlMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self));
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	modest_mail_header_view_set_priority (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), flags);

#ifdef MODEST_TOOLKIT_HILDON2
	gboolean show_priority = FALSE;
	TnyHeaderFlags priority_flags;

	priority_flags = flags & TNY_HEADER_FLAG_PRIORITY_MASK;
	if (priority_flags == TNY_HEADER_FLAG_HIGH_PRIORITY) {
		show_priority = TRUE;
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon), MODEST_HEADER_ICON_HIGH, GTK_ICON_SIZE_MENU);
	} else if (priority_flags == TNY_HEADER_FLAG_LOW_PRIORITY) {
		show_priority = TRUE;
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon), MODEST_HEADER_ICON_LOW, GTK_ICON_SIZE_MENU);
	}

	if (show_priority && MODEST_IS_COMPACT_MAIL_HEADER_VIEW (priv->mail_header_view)) {
		gtk_widget_show_all  (priv->priority_box);
	} else {
		gtk_widget_hide_all (priv->priority_box);
	}
#endif

}

/* INCREMENTAL SEARCH IMPLEMENTATION */

static gboolean 
search (ModestGtkhtmlMsgView *self, const gchar *search)
{
	ModestGtkhtmlMsgViewPrivate *priv;
	gboolean result;
	GtkAdjustment *vadj, *tmp_vadj;
	gdouble y_offset;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), FALSE);

	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	vadj = gtk_layout_get_vadjustment (GTK_LAYOUT (priv->body_view));
	g_object_ref (vadj);
	tmp_vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, vadj->lower, vadj->upper, vadj->step_increment, 32.0, 32.0));
	gtk_layout_set_vadjustment (GTK_LAYOUT (priv->body_view), tmp_vadj);
	result = modest_isearch_view_search (MODEST_ISEARCH_VIEW (priv->body_view),
					     search);

	if (result) {
		gint x, y, w, h;
		gdouble offset_top, offset_bottom;
		GtkAdjustment *adj;
		if (modest_isearch_view_get_selection_area (MODEST_ISEARCH_VIEW (priv->body_view), &x, &y, &w, &h)) {
			offset_top = (gdouble) (priv->headers_box->requisition.height + y);
			offset_bottom = (gdouble) (priv->headers_box->requisition.height + y + h);
			adj = GTK_ADJUSTMENT (priv->vadj);
			if (offset_top < adj->value)
				gtk_adjustment_set_value (adj, offset_top + adj->page_increment - adj->page_size);
			else if (offset_bottom > adj->value + adj->page_increment)
				gtk_adjustment_set_value (adj, offset_bottom - adj->page_increment);
		}
	}

	y_offset = tmp_vadj->value;
	gtk_layout_set_vadjustment (GTK_LAYOUT (priv->body_view), vadj);
	g_object_unref (vadj);

	return result;
}

static gboolean
search_next (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv;
	gboolean result;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), FALSE);

	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);
	result = modest_isearch_view_search_next (MODEST_ISEARCH_VIEW (priv->body_view));

	if (result) {
		gint x, y, w, h;
		gdouble offset_top, offset_bottom;
		GtkAdjustment *adj;

		if (modest_isearch_view_get_selection_area (MODEST_ISEARCH_VIEW (priv->body_view), &x, &y, &w, &h)) {
			offset_top = (gdouble) (priv->headers_box->requisition.height + y);
			offset_bottom = (gdouble) (priv->headers_box->requisition.height + y + h);
			adj = GTK_ADJUSTMENT (priv->vadj);
			if (offset_top < adj->value)
				gtk_adjustment_set_value (adj, offset_top + adj->page_increment - adj->page_size);
			else if (offset_bottom > adj->value + adj->page_increment)
				gtk_adjustment_set_value (adj, offset_bottom - adj->page_increment);
		}
	}
	return result;
}

static TnyList *
get_selected_attachments (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), NULL);
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	return modest_attachments_view_get_selection (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	
}

static TnyList *
get_attachments (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self), NULL);
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	return modest_attachments_view_get_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	
}

static void
grab_focus (ModestGtkhtmlMsgView *self)
{
	ModestGtkhtmlMsgViewPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self));
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	gtk_widget_grab_focus (priv->body_view);
}

static void
remove_attachment (ModestGtkhtmlMsgView *self, TnyMimePart *attachment)
{
	ModestGtkhtmlMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_GTKHTML_MSG_VIEW (self));
	g_return_if_fail (TNY_IS_MIME_PART (attachment));
	priv = MODEST_GTKHTML_MSG_VIEW_GET_PRIVATE (self);

	modest_attachments_view_remove_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
						   attachment);
	
}

/* TNY HEADER VIEW IMPLEMENTATION */

static void
tny_header_view_init (gpointer g, gpointer iface_data)
{
	TnyHeaderViewIface *klass = (TnyHeaderViewIface *)g;

	klass->set_header = modest_msg_view_set_header;
	klass->clear = modest_msg_view_clear_header;

	return;
}

static void
modest_msg_view_set_header (TnyHeaderView *self, TnyHeader *header)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_header_func (self, header);
}


static void
modest_msg_view_set_header_default (TnyHeaderView *self, TnyHeader *header)
{
	set_header (MODEST_GTKHTML_MSG_VIEW (self), header);
}

static void
modest_msg_view_clear_header (TnyHeaderView *self)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->clear_header_func (self);
}


static void
modest_msg_view_clear_header_default (TnyHeaderView *self)
{
	set_message (MODEST_GTKHTML_MSG_VIEW (self), NULL, NULL);
}

/* TNY MSG IMPLEMENTATION */

static void
tny_msg_view_init (gpointer g, gpointer iface_data)
{
	TnyMsgViewIface *klass = (TnyMsgViewIface *)g;

	klass->get_msg = modest_msg_view_get_msg;
	klass->set_msg = modest_msg_view_set_msg;
	klass->set_unavailable = modest_msg_view_set_unavailable;
	klass->clear = modest_msg_view_clear;
	klass->create_mime_part_view_for = modest_msg_view_create_mime_part_view_for;
	klass->create_new_inline_viewer = modest_msg_view_create_new_inline_viewer;

	return;
}

static TnyMsg *
modest_msg_view_get_msg (TnyMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_msg_func (self);
}

static TnyMsg *
modest_msg_view_get_msg_default (TnyMsgView *self)
{
	return TNY_MSG (tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (self)));
}

static void
modest_msg_view_set_msg (TnyMsgView *self, TnyMsg *msg)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_msg_func (self, msg);
}

static void 
modest_msg_view_set_msg_default (TnyMsgView *self, TnyMsg *msg)
{

	tny_mime_part_view_set_part (TNY_MIME_PART_VIEW (self), TNY_MIME_PART (msg));

	return;
}

static void
modest_msg_view_set_unavailable (TnyMsgView *self)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_unavailable_func (self);
}

static void
modest_msg_view_set_unavailable_default (TnyMsgView *self)
{
	tny_msg_view_clear (self);

	return;
}

static void
modest_msg_view_clear (TnyMsgView *self)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->clear_func (self);
}

static void
modest_msg_view_clear_default (TnyMsgView *self)
{
	set_message (MODEST_GTKHTML_MSG_VIEW (self), NULL, NULL);
}

static TnyMimePartView*
modest_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->create_mime_part_view_for_func (self, part);
}

static TnyMimePartView*
modest_msg_view_create_mime_part_view_for_default (TnyMsgView *self, TnyMimePart *part)
{
	g_warning ("modest_msg_view_create_mime_part_view_for_default is not implemented");
	return NULL;
}

static TnyMsgView*
modest_msg_view_create_new_inline_viewer (TnyMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->create_new_inline_viewer_func (self);
}

static TnyMsgView*
modest_msg_view_create_new_inline_viewer_default (TnyMsgView *self)
{
	g_warning ("modest_msg_view_create_new_inline_viewer_default is not implemented");

	return NULL;
}

/* TNY MIME PART IMPLEMENTATION */

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part = modest_msg_view_mp_get_part;
	klass->set_part = modest_msg_view_mp_set_part;
	klass->clear = modest_msg_view_mp_clear;

	return;
}

static TnyMimePart* 
modest_msg_view_mp_get_part (TnyMimePartView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_part_func (self);
}


static TnyMimePart* 
modest_msg_view_mp_get_part_default (TnyMimePartView *self)
{
	return TNY_MIME_PART (get_message (MODEST_GTKHTML_MSG_VIEW (self)));
}

static void
modest_msg_view_mp_set_part (TnyMimePartView *self,
			     TnyMimePart *part)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_part_func (self, part);
}

static void
modest_msg_view_mp_set_part_default (TnyMimePartView *self,
				     TnyMimePart *part)
{
	g_return_if_fail ((part == NULL) || TNY_IS_MSG (part));

	set_message (MODEST_GTKHTML_MSG_VIEW (self), TNY_MSG (part), NULL);
}

static void
modest_msg_view_mp_clear (TnyMimePartView *self)
{
	tny_msg_view_clear (TNY_MSG_VIEW (self));
}

/* MODEST MIME PART VIEW IMPLEMENTATION */

static void
modest_mime_part_view_init (gpointer g, gpointer iface_data)
{
	ModestMimePartViewIface *klass = (ModestMimePartViewIface *)g;

	klass->is_empty_func = modest_msg_view_mp_is_empty;

	return;
}

static gboolean
modest_msg_view_mp_is_empty (ModestMimePartView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->is_empty_func (self);
}

static gboolean
modest_msg_view_mp_is_empty_default (ModestMimePartView *self)
{
	return is_empty (MODEST_GTKHTML_MSG_VIEW (self));
}

/* MODEST ZOOMABLE IMPLEMENTATION */
static void
modest_zoomable_init (gpointer g, gpointer iface_data)
{
	ModestZoomableIface *klass = (ModestZoomableIface *)g;

	klass->get_zoom_func = modest_msg_view_get_zoom;
	klass->set_zoom_func = modest_msg_view_set_zoom;
	klass->zoom_minus_func = modest_msg_view_zoom_minus;
	klass->zoom_plus_func = modest_msg_view_zoom_plus;

	return;
}

static gdouble
modest_msg_view_get_zoom (ModestZoomable *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_zoom_func (self);
}

static gdouble
modest_msg_view_get_zoom_default (ModestZoomable *self)
{
	return get_zoom (MODEST_GTKHTML_MSG_VIEW (self));
}

static void
modest_msg_view_set_zoom (ModestZoomable *self, gdouble value)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_zoom_func (self, value);
}

static void
modest_msg_view_set_zoom_default (ModestZoomable *self, gdouble value)
{
	set_zoom (MODEST_GTKHTML_MSG_VIEW (self), value);
}

static gboolean
modest_msg_view_zoom_minus (ModestZoomable *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->zoom_minus_func (self);
}

static gboolean
modest_msg_view_zoom_minus_default (ModestZoomable *self)
{
	/* operation not supported in ModestMsgView */
	return FALSE;
}

static gboolean
modest_msg_view_zoom_plus (ModestZoomable *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->zoom_plus_func (self);
}

static gboolean
modest_msg_view_zoom_plus_default (ModestZoomable *self)
{
	/* operation not supported in ModestMsgView */
	return FALSE;
}

/* MODEST ISEARCH VIEW IMPLEMENTATION */
static void
modest_isearch_view_init (gpointer g, gpointer iface_data)
{
	ModestISearchViewIface *klass = (ModestISearchViewIface *)g;

	klass->search_func = modest_msg_view_search;
	klass->search_next_func = modest_msg_view_search_next;

	return;
}

static gboolean
modest_msg_view_search (ModestISearchView *self, const gchar *string)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->search_func (self, string);
}

static gboolean
modest_msg_view_search_default (ModestISearchView *self, const gchar *string)
{
	return search (MODEST_GTKHTML_MSG_VIEW (self), string);
}

static gboolean
modest_msg_view_search_next (ModestISearchView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->search_next_func (self);
}

static gboolean
modest_msg_view_search_next_default (ModestISearchView *self)
{
	return search_next (MODEST_GTKHTML_MSG_VIEW (self));
}

/* MODEST MSG VIEW IMPLEMENTATION */
static void
modest_msg_view_init (gpointer g, gpointer iface_data)
{
	ModestMsgViewIface *klass = (ModestMsgViewIface *)g;

	klass->set_msg_with_other_body_func = modest_gtkhtml_msg_view_set_msg_with_other_body;
	klass->get_vadjustment_func = modest_gtkhtml_msg_view_get_vadjustment;
	klass->get_hadjustment_func = modest_gtkhtml_msg_view_get_hadjustment;
	klass->set_vadjustment_func = modest_gtkhtml_msg_view_set_vadjustment;
	klass->set_hadjustment_func = modest_gtkhtml_msg_view_set_hadjustment;
	klass->set_shadow_type_func = modest_gtkhtml_msg_view_set_shadow_type;
	klass->get_shadow_type_func = modest_gtkhtml_msg_view_get_shadow_type;
	klass->get_priority_func = modest_gtkhtml_msg_view_get_priority;
	klass->set_priority_func = modest_gtkhtml_msg_view_set_priority;
	klass->get_selected_attachments_func = modest_gtkhtml_msg_view_get_selected_attachments;
	klass->get_attachments_func = modest_gtkhtml_msg_view_get_attachments;
	klass->grab_focus_func = modest_gtkhtml_msg_view_grab_focus;
	klass->remove_attachment_func = modest_gtkhtml_msg_view_remove_attachment;
	klass->request_fetch_images_func = modest_gtkhtml_msg_view_request_fetch_images;
	klass->set_branding_func = modest_gtkhtml_msg_view_set_branding;
	klass->has_blocked_external_images_func = modest_gtkhtml_msg_view_has_blocked_external_images;

	return;
}

static void
modest_gtkhtml_msg_view_set_msg_with_other_body (ModestMsgView *self, TnyMsg *msg, TnyMimePart *other_body)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_msg_with_other_body_func (self, msg, other_body);
}

static void
modest_gtkhtml_msg_view_set_msg_with_other_body_default (ModestMsgView *self, TnyMsg *msg, TnyMimePart *other_body)
{
	set_message (MODEST_GTKHTML_MSG_VIEW (self), msg, other_body);
}

static GtkAdjustment*
modest_gtkhtml_msg_view_get_vadjustment (ModestMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_vadjustment_func (self);
}

static GtkAdjustment*
modest_gtkhtml_msg_view_get_vadjustment_default (ModestMsgView *self)
{
	return get_vadjustment (MODEST_GTKHTML_MSG_VIEW (self));
}

static GtkAdjustment*
modest_gtkhtml_msg_view_get_hadjustment (ModestMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_hadjustment_func (self);
}

static GtkAdjustment*
modest_gtkhtml_msg_view_get_hadjustment_default (ModestMsgView *self)
{
	return get_hadjustment (MODEST_GTKHTML_MSG_VIEW (self));
}

static void
modest_gtkhtml_msg_view_set_vadjustment (ModestMsgView *self, GtkAdjustment *adj)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_vadjustment_func (self, adj);
}

static void
modest_gtkhtml_msg_view_set_vadjustment_default (ModestMsgView *self, GtkAdjustment *adj)
{
	set_vadjustment (MODEST_GTKHTML_MSG_VIEW (self), adj);
}

static void
modest_gtkhtml_msg_view_set_hadjustment (ModestMsgView *self, GtkAdjustment *adj)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_hadjustment_func (self, adj);
}

static void
modest_gtkhtml_msg_view_set_hadjustment_default (ModestMsgView *self, GtkAdjustment *adj)
{
	set_hadjustment (MODEST_GTKHTML_MSG_VIEW (self), adj);
}

static void
modest_gtkhtml_msg_view_set_shadow_type (ModestMsgView *self, GtkShadowType type)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_shadow_type_func (self, type);
}

static void
modest_gtkhtml_msg_view_set_shadow_type_default (ModestMsgView *self, GtkShadowType type)
{
	set_shadow_type (MODEST_GTKHTML_MSG_VIEW (self), type);
}

static GtkShadowType
modest_gtkhtml_msg_view_get_shadow_type (ModestMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_shadow_type_func (self);
}

static GtkShadowType
modest_gtkhtml_msg_view_get_shadow_type_default (ModestMsgView *self)
{
	return get_shadow_type (MODEST_GTKHTML_MSG_VIEW (self));
}

static void
modest_gtkhtml_msg_view_set_priority (ModestMsgView *self, TnyHeaderFlags flags)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_priority_func (self, flags);
}

static void
modest_gtkhtml_msg_view_set_priority_default (ModestMsgView *self, TnyHeaderFlags flags)
{
	set_priority (MODEST_GTKHTML_MSG_VIEW (self), flags);
}

static TnyHeaderFlags
modest_gtkhtml_msg_view_get_priority (ModestMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_priority_func (self);
}

static TnyHeaderFlags
modest_gtkhtml_msg_view_get_priority_default (ModestMsgView *self)
{
	return get_priority (MODEST_GTKHTML_MSG_VIEW (self));
}

static TnyList*
modest_gtkhtml_msg_view_get_selected_attachments (ModestMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_selected_attachments_func (self);
}

static TnyList*
modest_gtkhtml_msg_view_get_selected_attachments_default (ModestMsgView *self)
{
	return get_selected_attachments (MODEST_GTKHTML_MSG_VIEW (self));
}

static TnyList*
modest_gtkhtml_msg_view_get_attachments (ModestMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->get_attachments_func (self);
}

static TnyList*
modest_gtkhtml_msg_view_get_attachments_default (ModestMsgView *self)
{
	return get_attachments (MODEST_GTKHTML_MSG_VIEW (self));
}

static void
modest_gtkhtml_msg_view_grab_focus (ModestMsgView *self)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->grab_focus_func (self);
}

static void
modest_gtkhtml_msg_view_grab_focus_default (ModestMsgView *self)
{
	grab_focus (MODEST_GTKHTML_MSG_VIEW (self));
}

static void
modest_gtkhtml_msg_view_remove_attachment (ModestMsgView *self, TnyMimePart *attachment)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->remove_attachment_func (self, attachment);
}

static void
modest_gtkhtml_msg_view_remove_attachment_default (ModestMsgView *self, TnyMimePart *attachment)
{
	remove_attachment (MODEST_GTKHTML_MSG_VIEW (self), attachment);
}

static void
modest_gtkhtml_msg_view_request_fetch_images (ModestMsgView *self)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->request_fetch_images_func (self);
}

static void
modest_gtkhtml_msg_view_request_fetch_images_default (ModestMsgView *self)
{
	request_fetch_images (MODEST_GTKHTML_MSG_VIEW (self));
}

static void
modest_gtkhtml_msg_view_set_branding (ModestMsgView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->set_branding_func (self, brand_name, brand_icon);
}

static void
modest_gtkhtml_msg_view_set_branding_default (ModestMsgView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	set_branding (MODEST_GTKHTML_MSG_VIEW (self), brand_name, brand_icon);
}

static gboolean
modest_gtkhtml_msg_view_has_blocked_external_images (ModestMsgView *self)
{
	return MODEST_GTKHTML_MSG_VIEW_GET_CLASS (self)->has_blocked_external_images_func (self);
}

static gboolean
modest_gtkhtml_msg_view_has_blocked_external_images_default (ModestMsgView *self)
{
	return has_blocked_external_images (MODEST_GTKHTML_MSG_VIEW (self));
}

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
#include <widgets/modest-compact-mail-header-view.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-gtk.h>
#endif
#include <widgets/modest-attachments-view.h>
#include <modest-marshal.h>
#include <widgets/modest-webkit-mime-part-view.h>
#include <widgets/modest-webkit-msg-view.h>
#include <widgets/modest-isearch-view.h>
#include <widgets/modest-ui-constants.h>
#include <modest-icon-names.h>
#include <gtk/gtk.h>

/* 'private'/'protected' functions */
static void     modest_webkit_msg_view_class_init   (ModestWebkitMsgViewClass *klass);
static void     tny_header_view_init (gpointer g, gpointer iface_data);
static void     tny_msg_view_init (gpointer g, gpointer iface_data);
static void     tny_mime_part_view_init (gpointer g, gpointer iface_data);
static void     modest_mime_part_view_init (gpointer g, gpointer iface_data);
static void     modest_zoomable_init (gpointer g, gpointer iface_data);
static void     modest_isearch_view_init (gpointer g, gpointer iface_data);
static void     modest_msg_view_init (gpointer g, gpointer iface_data);
static void     modest_webkit_msg_view_init         (ModestWebkitMsgView *obj);
static void     modest_webkit_msg_view_finalize     (GObject *obj);
static void     modest_webkit_msg_view_destroy     (GtkObject *obj);

/* headers signals */
static void on_recpt_activated (ModestMailHeaderView *header_view, const gchar *address, ModestWebkitMsgView *msg_view);
static void on_show_details (ModestMailHeaderView *header_view, ModestWebkitMsgView *msg_view);
static void on_attachment_activated (ModestAttachmentsView * att_view, TnyMimePart *mime_part, gpointer userdata);

/* body view signals */
static gboolean on_activate_link (GtkWidget *widget, const gchar *uri, ModestWebkitMsgView *msg_view);
static gboolean on_fetch_url (GtkWidget *widget, const gchar *uri, TnyStream *stream,
			      ModestWebkitMsgView *msg_view);
static gboolean on_link_hover (GtkWidget *widget, const gchar *uri, ModestWebkitMsgView *msg_view);
static void on_limit_error (GtkWidget *widget, ModestWebkitMsgView *msg_view);

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
static void modest_webkit_msg_view_set_msg_with_other_body (ModestMsgView *self, TnyMsg *msg, TnyMimePart *other_body);
static GtkAdjustment *modest_webkit_msg_view_get_vadjustment (ModestMsgView *self);
static GtkAdjustment *modest_webkit_msg_view_get_hadjustment (ModestMsgView *self);
static void modest_webkit_msg_view_set_vadjustment (ModestMsgView *self, GtkAdjustment *vadj);
static void modest_webkit_msg_view_set_hadjustment (ModestMsgView *self, GtkAdjustment *hadj);
static void modest_webkit_msg_view_set_shadow_type (ModestMsgView *self, GtkShadowType type);
static GtkShadowType modest_webkit_msg_view_get_shadow_type (ModestMsgView *self);
static TnyHeaderFlags modest_webkit_msg_view_get_priority (ModestMsgView *self);
static void modest_webkit_msg_view_set_priority (ModestMsgView *self, TnyHeaderFlags flags);
static TnyList *modest_webkit_msg_view_get_selected_attachments (ModestMsgView *self);
static TnyList *modest_webkit_msg_view_get_attachments (ModestMsgView *self);
static void modest_webkit_msg_view_grab_focus (ModestMsgView *self);
static void modest_webkit_msg_view_remove_attachment (ModestMsgView *view, TnyMimePart *attachment);
static void modest_webkit_msg_view_request_fetch_images (ModestMsgView *view);
static void modest_webkit_msg_view_set_branding (ModestMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static gboolean modest_webkit_msg_view_has_blocked_external_images (ModestMsgView *view);
static void modest_webkit_msg_view_set_msg_with_other_body_default (ModestMsgView *view, TnyMsg *msg, TnyMimePart *part);
static GtkAdjustment *modest_webkit_msg_view_get_vadjustment_default (ModestMsgView *self);
static GtkAdjustment *modest_webkit_msg_view_get_hadjustment_default (ModestMsgView *self);
static void modest_webkit_msg_view_set_vadjustment_default (ModestMsgView *self, GtkAdjustment *vadj);
static void modest_webkit_msg_view_set_hadjustment_default (ModestMsgView *self, GtkAdjustment *hadj);
static void modest_webkit_msg_view_set_shadow_type_default (ModestMsgView *self, GtkShadowType type);
static GtkShadowType modest_webkit_msg_view_get_shadow_type_default (ModestMsgView *self);
static TnyHeaderFlags modest_webkit_msg_view_get_priority_default (ModestMsgView *self);
static void modest_webkit_msg_view_set_priority_default (ModestMsgView *self, TnyHeaderFlags flags);
static TnyList *modest_webkit_msg_view_get_selected_attachments_default (ModestMsgView *self);
static TnyList *modest_webkit_msg_view_get_attachments_default (ModestMsgView *self);
static void modest_webkit_msg_view_grab_focus_default (ModestMsgView *self);
static void modest_webkit_msg_view_remove_attachment_default (ModestMsgView *view, TnyMimePart *attachment);
static gboolean modest_webkit_msg_view_has_blocked_external_images_default (ModestMsgView *view);
static void modest_webkit_msg_view_request_fetch_images_default (ModestMsgView *view);
static void modest_webkit_msg_view_set_branding_default (ModestMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);

/* internal api */
static void     set_header     (ModestWebkitMsgView *self, TnyHeader *header);
static TnyMsg   *get_message   (ModestWebkitMsgView *self);
static void     set_message    (ModestWebkitMsgView *self, TnyMsg *tny_msg, TnyMimePart *other_body);
static gboolean is_empty       (ModestWebkitMsgView *self); 
static void     set_zoom       (ModestWebkitMsgView *self, gdouble zoom);
static gdouble  get_zoom       (ModestWebkitMsgView *self);
static gboolean search         (ModestWebkitMsgView *self, const gchar *search);
static gboolean search_next    (ModestWebkitMsgView *self);
static GtkAdjustment *get_vadjustment (ModestWebkitMsgView *self);
static GtkAdjustment *get_hadjustment (ModestWebkitMsgView *self);
static void set_vadjustment (ModestWebkitMsgView *self, GtkAdjustment *vadj);
static void set_hadjustment (ModestWebkitMsgView *self, GtkAdjustment *hadj);
static void set_shadow_type (ModestWebkitMsgView *self, GtkShadowType type);
static GtkShadowType get_shadow_type (ModestWebkitMsgView *self);
static TnyHeaderFlags get_priority (ModestWebkitMsgView *self);
static void set_priority (ModestWebkitMsgView *self, TnyHeaderFlags flags);
static TnyList *get_selected_attachments (ModestWebkitMsgView *self);
static TnyList *get_attachments (ModestWebkitMsgView *self);
static void grab_focus (ModestWebkitMsgView *self);
static void remove_attachment (ModestWebkitMsgView *view, TnyMimePart *attachment);
static void request_fetch_images (ModestWebkitMsgView *view);
static void set_branding (ModestWebkitMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static gboolean has_blocked_external_images (ModestWebkitMsgView *view);

typedef struct _ModestWebkitMsgViewPrivate ModestWebkitMsgViewPrivate;
struct _ModestWebkitMsgViewPrivate {
	GtkWidget   *body_view;
	GtkWidget   *mail_header_view;
	GtkWidget   *attachments_view;

	TnyMsg      *msg;

	/* embedded elements */
	GtkWidget   *headers_box;
	GtkWidget   *attachments_box;

	GtkWidget   *priority_box;
	GtkWidget   *priority_icon;

	/* zoom */
	gdouble current_zoom;

	/* link click management */
	gchar *last_url;
};

#define MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										 MODEST_TYPE_WEBKIT_MSG_VIEW, \
										 ModestWebkitMsgViewPrivate))

/* globals */
static GtkContainerClass *parent_class = NULL;

GType
modest_webkit_msg_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestWebkitMsgViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_webkit_msg_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestWebkitMsgView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_webkit_msg_view_init,
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

 		my_type = g_type_register_static (GTK_TYPE_VIEWPORT,
		                                  "ModestWebkitMsgView",
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
modest_webkit_msg_view_class_init (ModestWebkitMsgViewClass *klass)
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
	gobject_class->finalize = modest_webkit_msg_view_finalize;
	gtkobject_class->destroy = modest_webkit_msg_view_destroy;

	klass->set_header_func = modest_msg_view_set_header_default;
	klass->clear_header_func = modest_msg_view_clear_header_default;
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
	klass->set_msg_with_other_body_func = modest_webkit_msg_view_set_msg_with_other_body_default;
	klass->get_vadjustment_func = modest_webkit_msg_view_get_vadjustment_default;
	klass->get_hadjustment_func = modest_webkit_msg_view_get_hadjustment_default;
	klass->set_vadjustment_func = modest_webkit_msg_view_set_vadjustment_default;
	klass->set_hadjustment_func = modest_webkit_msg_view_set_hadjustment_default;
	klass->get_shadow_type_func = modest_webkit_msg_view_get_shadow_type_default;
	klass->set_shadow_type_func = modest_webkit_msg_view_set_shadow_type_default;
	klass->get_priority_func = modest_webkit_msg_view_get_priority_default;
	klass->set_priority_func = modest_webkit_msg_view_set_priority_default;
	klass->get_selected_attachments_func = modest_webkit_msg_view_get_selected_attachments_default;
	klass->get_attachments_func = modest_webkit_msg_view_get_attachments_default;
	klass->grab_focus_func = modest_webkit_msg_view_grab_focus_default;
	klass->remove_attachment_func = modest_webkit_msg_view_remove_attachment_default;
	klass->request_fetch_images_func = modest_webkit_msg_view_request_fetch_images_default;
	klass->set_branding_func = modest_webkit_msg_view_set_branding_default;
	klass->has_blocked_external_images_func = modest_webkit_msg_view_has_blocked_external_images_default;

	g_type_class_add_private (gobject_class, sizeof(ModestWebkitMsgViewPrivate));

}

static void
modest_webkit_msg_view_init (ModestWebkitMsgView *obj)
{
 	ModestWebkitMsgViewPrivate *priv;
	GtkWidget *vbox;

	GTK_WIDGET_UNSET_FLAGS (obj, GTK_NO_WINDOW);
	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (obj), TRUE);
	gtk_container_set_reallocate_redraws (GTK_CONTAINER (obj), TRUE);
	gtk_container_set_resize_mode (GTK_CONTAINER (obj), GTK_RESIZE_QUEUE);
	
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE(obj);

	vbox = gtk_vbox_new (FALSE, 0);

	priv->current_zoom = 1.0;

	priv->msg                     = NULL;

	priv->body_view                 = GTK_WIDGET (g_object_new (MODEST_TYPE_WEBKIT_MIME_PART_VIEW, NULL));
	priv->mail_header_view        = GTK_WIDGET (modest_compact_mail_header_view_new ());
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

	g_signal_connect (G_OBJECT (priv->mail_header_view), "recpt-activated", 
			  G_CALLBACK (on_recpt_activated), obj);
	g_signal_connect (G_OBJECT (priv->mail_header_view), "show-details", 
			  G_CALLBACK (on_show_details), obj);

	g_signal_connect (G_OBJECT (priv->attachments_view), "activate",
			  G_CALLBACK (on_attachment_activated), obj);

	priv->headers_box = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (vbox), priv->headers_box, FALSE, FALSE, 0);

	if (priv->mail_header_view)
		gtk_box_pack_start (GTK_BOX(priv->headers_box), priv->mail_header_view, FALSE, FALSE, 0);

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
	if (priv->attachments_view) {
		gchar *att_label = g_strconcat (_("mail_va_attachment"), ":", NULL);

		priv->attachments_box = (GtkWidget *)
			modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view),
								   att_label,
								   priv->attachments_view,
								   FALSE, FALSE);
		gtk_widget_hide_all (priv->attachments_box);
		g_free (att_label);
	}


	if (priv->body_view) {
		gtk_box_pack_start (GTK_BOX (vbox), priv->body_view, TRUE, TRUE, 0);
	}

	gtk_container_add (GTK_CONTAINER (obj), vbox);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (vbox), gtk_viewport_get_vadjustment (GTK_VIEWPORT (obj)));
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->body_view), gtk_viewport_get_vadjustment (GTK_VIEWPORT (obj)));
	
}
	

static void
modest_webkit_msg_view_finalize (GObject *obj)
{	
	ModestWebkitMsgViewPrivate *priv;
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (obj);

	if (priv->msg) {
		g_object_unref (G_OBJECT(priv->msg));
		priv->msg = NULL;
	}

	priv->body_view = NULL;
	priv->attachments_view = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}

static void
modest_webkit_msg_view_destroy (GtkObject *obj)
{	

	GTK_OBJECT_CLASS(parent_class)->destroy (obj);		
}

static GtkAdjustment *
get_vadjustment (ModestWebkitMsgView *self)
{
	return gtk_viewport_get_vadjustment (GTK_VIEWPORT (self));
}

static GtkAdjustment *
get_hadjustment (ModestWebkitMsgView *self)
{
	return gtk_viewport_get_hadjustment (GTK_VIEWPORT (self));
}

static void
set_hadjustment (ModestWebkitMsgView *self, GtkAdjustment *hadj)
{
	gtk_viewport_set_hadjustment (GTK_VIEWPORT (self), hadj);
}

static void
set_vadjustment (ModestWebkitMsgView *self, GtkAdjustment *vadj)
{
	gtk_viewport_set_vadjustment (GTK_VIEWPORT (self), vadj);
}

static void
set_shadow_type (ModestWebkitMsgView *self,
		 GtkShadowType shadow_type)
{
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (self), shadow_type);
}

static GtkShadowType
get_shadow_type (ModestWebkitMsgView *self)
{
	return gtk_viewport_get_shadow_type (GTK_VIEWPORT (self));
}

/* INTERNAL METHODS */
GtkWidget*
modest_msg_view_new (TnyMsg *msg)
{
	GObject *obj;
	ModestWebkitMsgView* self;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_WEBKIT_MSG_VIEW, NULL));
	self = MODEST_WEBKIT_MSG_VIEW(obj);
	tny_msg_view_set_msg (TNY_MSG_VIEW (self), msg);

	return GTK_WIDGET(self);
}


static void
on_recpt_activated (ModestMailHeaderView *header_view, 
		    const gchar *address,
		    ModestWebkitMsgView *self)
{
	g_signal_emit_by_name (G_OBJECT (self), "recpt-activated", address);
}

static void
on_show_details (ModestMailHeaderView *header_view, 
		 ModestWebkitMsgView *self)
{
	g_signal_emit_by_name (G_OBJECT (self), "show-details");
}

static void
on_attachment_activated (ModestAttachmentsView * att_view, TnyMimePart *mime_part, gpointer self)
{

	g_signal_emit_by_name (G_OBJECT(self), "attachment_clicked", mime_part);
}

static void
request_fetch_images (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);
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
set_branding (ModestWebkitMsgView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	ModestWebkitMsgViewPrivate *priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	modest_mail_header_view_set_branding (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), brand_name, brand_icon);
}

static gboolean
has_blocked_external_images (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	return (modest_mime_part_view_has_external_images (MODEST_MIME_PART_VIEW (priv->body_view)) &&
		!modest_mime_part_view_get_view_images (MODEST_MIME_PART_VIEW (priv->body_view)));
}

static gboolean
on_activate_link (GtkWidget *widget, const gchar *uri, ModestWebkitMsgView *self)
{
	gboolean result;
	g_return_val_if_fail (self, FALSE);

	g_signal_emit_by_name (G_OBJECT(self), "activate-link", uri, &result);

	return result;
}


static gboolean
on_link_hover (GtkWidget *widget, const gchar *uri, ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);
	gboolean result;

	g_free (priv->last_url);
	priv->last_url = g_strdup (uri);

	g_signal_emit_by_name (G_OBJECT(self), "link-hover", uri, &result);

	return result;
}

static void 
on_limit_error (GtkWidget *widget, ModestWebkitMsgView *msg_view)
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
	      TnyStream *stream, ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv;
	const gchar* my_uri;
	TnyMimePart *part = NULL;
	


	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

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
set_message (ModestWebkitMsgView *self, TnyMsg *msg, TnyMimePart *other_body)
{
	TnyMimePart *body;
	ModestWebkitMsgViewPrivate *priv;
	TnyHeader *header;

	g_return_if_fail (self);

	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE(self);
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

		gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
		gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);

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
			if (modest_attachments_view_get_num_attachments (widget) > 1) {
				text = _("mail_va_attachments");
			} else {
				text = _("mail_va_attachment");
			}
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
	gtk_widget_set_no_show_all (priv->priority_box, TRUE);
	gtk_widget_set_no_show_all (priv->attachments_box, TRUE);
	gtk_widget_show_all (priv->mail_header_view);
	gtk_widget_set_no_show_all (priv->attachments_box, FALSE);
	gtk_widget_set_no_show_all (priv->priority_box, FALSE);
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);

	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);

	gtk_widget_queue_resize (GTK_WIDGET (priv->body_view));

	gtk_widget_queue_resize (GTK_WIDGET(self));
	gtk_widget_queue_draw (GTK_WIDGET(self));

}

static void
set_header (ModestWebkitMsgView *self, TnyHeader *header)
{
	ModestWebkitMsgViewPrivate *priv;
	
	g_return_if_fail (self);

	if (header == NULL) {
		set_message (self, NULL, NULL);
		return;
	}
	
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE(self);
	modest_mail_header_view_set_loading (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), TRUE);
	gtk_widget_set_no_show_all (priv->mail_header_view, FALSE);
	modest_mime_part_view_set_view_images (MODEST_MIME_PART_VIEW (priv->body_view), FALSE);

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
	gtk_widget_hide_all (priv->priority_box);
	gtk_widget_set_no_show_all (priv->mail_header_view, TRUE);
	tny_mime_part_view_clear (TNY_MIME_PART_VIEW (priv->body_view));
	gtk_widget_queue_resize (GTK_WIDGET(self));
	gtk_widget_queue_draw (GTK_WIDGET(self));
}


static TnyMsg*
get_message (ModestWebkitMsgView *self)
{
	TnyMsg *msg;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self), NULL);

	msg = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE(self)->msg;

	if (msg)
		g_object_ref (msg);
	
	return msg;
}

static gboolean 
is_empty (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	return modest_mime_part_view_is_empty (MODEST_MIME_PART_VIEW (priv->body_view));
}

static void
set_zoom (ModestWebkitMsgView *self, gdouble zoom)
{
	ModestWebkitMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self));
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	modest_zoomable_set_zoom (MODEST_ZOOMABLE(priv->body_view), zoom);

	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), 1, 1);
	gtk_widget_set_size_request (GTK_WIDGET (priv->body_view), -1, -1);

	gtk_widget_queue_resize (priv->body_view);
}

static gdouble
get_zoom (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self), 1.0);
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	return modest_zoomable_get_zoom (MODEST_ZOOMABLE (priv->body_view));
}

static TnyHeaderFlags
get_priority (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self), 0);

	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	return modest_mail_header_view_get_priority (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view));
}

static void
set_priority (ModestWebkitMsgView *self, TnyHeaderFlags flags)
{
	ModestWebkitMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self));
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	modest_mail_header_view_set_priority (MODEST_MAIL_HEADER_VIEW (priv->mail_header_view), flags);

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

}

/* INCREMENTAL SEARCH IMPLEMENTATION */

static gboolean 
search (ModestWebkitMsgView *self, const gchar *search)
{
	ModestWebkitMsgViewPrivate *priv;
	gboolean result;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self), FALSE);

	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);
	result = modest_isearch_view_search (MODEST_ISEARCH_VIEW (priv->body_view),
					     search);

	return result;
}

static gboolean
search_next (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv;
	gboolean result;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self), FALSE);

	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);
	result = modest_isearch_view_search_next (MODEST_ISEARCH_VIEW (priv->body_view));

	return result;
}

static TnyList *
get_selected_attachments (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self), NULL);
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	return modest_attachments_view_get_selection (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	
}

static TnyList *
get_attachments (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self), NULL);
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	return modest_attachments_view_get_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	
}

static void
grab_focus (ModestWebkitMsgView *self)
{
	ModestWebkitMsgViewPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self));
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

	gtk_widget_grab_focus (priv->body_view);
}

static void
remove_attachment (ModestWebkitMsgView *self, TnyMimePart *attachment)
{
	ModestWebkitMsgViewPrivate *priv;

	g_return_if_fail (MODEST_IS_WEBKIT_MSG_VIEW (self));
	g_return_if_fail (TNY_IS_MIME_PART (attachment));
	priv = MODEST_WEBKIT_MSG_VIEW_GET_PRIVATE (self);

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
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_header_func (self, header);
}


static void
modest_msg_view_set_header_default (TnyHeaderView *self, TnyHeader *header)
{
	set_header (MODEST_WEBKIT_MSG_VIEW (self), header);
}

static void
modest_msg_view_clear_header (TnyHeaderView *self)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->clear_header_func (self);
}


static void
modest_msg_view_clear_header_default (TnyHeaderView *self)
{
	set_message (MODEST_WEBKIT_MSG_VIEW (self), NULL, NULL);
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
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_msg_func (self);
}

static TnyMsg *
modest_msg_view_get_msg_default (TnyMsgView *self)
{
	return TNY_MSG (tny_mime_part_view_get_part (TNY_MIME_PART_VIEW (self)));
}

static void
modest_msg_view_set_msg (TnyMsgView *self, TnyMsg *msg)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_msg_func (self, msg);
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
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_unavailable_func (self);
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
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->clear_func (self);
}

static void
modest_msg_view_clear_default (TnyMsgView *self)
{
	set_message (MODEST_WEBKIT_MSG_VIEW (self), NULL, NULL);
}

static TnyMimePartView*
modest_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->create_mime_part_view_for_func (self, part);
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
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->create_new_inline_viewer_func (self);
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
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_part_func (self);
}


static TnyMimePart* 
modest_msg_view_mp_get_part_default (TnyMimePartView *self)
{
	return TNY_MIME_PART (get_message (MODEST_WEBKIT_MSG_VIEW (self)));
}

static void
modest_msg_view_mp_set_part (TnyMimePartView *self,
			     TnyMimePart *part)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_part_func (self, part);
}

static void
modest_msg_view_mp_set_part_default (TnyMimePartView *self,
				     TnyMimePart *part)
{
	g_return_if_fail ((part == NULL) || TNY_IS_MSG (part));

	set_message (MODEST_WEBKIT_MSG_VIEW (self), TNY_MSG (part), NULL);
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
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->is_empty_func (self);
}

static gboolean
modest_msg_view_mp_is_empty_default (ModestMimePartView *self)
{
	return is_empty (MODEST_WEBKIT_MSG_VIEW (self));
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
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_zoom_func (self);
}

static gdouble
modest_msg_view_get_zoom_default (ModestZoomable *self)
{
	return get_zoom (MODEST_WEBKIT_MSG_VIEW (self));
}

static void
modest_msg_view_set_zoom (ModestZoomable *self, gdouble value)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_zoom_func (self, value);
}

static void
modest_msg_view_set_zoom_default (ModestZoomable *self, gdouble value)
{
	set_zoom (MODEST_WEBKIT_MSG_VIEW (self), value);
}

static gboolean
modest_msg_view_zoom_minus (ModestZoomable *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->zoom_minus_func (self);
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
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->zoom_plus_func (self);
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
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->search_func (self, string);
}

static gboolean
modest_msg_view_search_default (ModestISearchView *self, const gchar *string)
{
	return search (MODEST_WEBKIT_MSG_VIEW (self), string);
}

static gboolean
modest_msg_view_search_next (ModestISearchView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->search_next_func (self);
}

static gboolean
modest_msg_view_search_next_default (ModestISearchView *self)
{
	return search_next (MODEST_WEBKIT_MSG_VIEW (self));
}

/* MODEST MSG VIEW IMPLEMENTATION */
static void
modest_msg_view_init (gpointer g, gpointer iface_data)
{
	ModestMsgViewIface *klass = (ModestMsgViewIface *)g;

	klass->set_msg_with_other_body_func = modest_webkit_msg_view_set_msg_with_other_body;
	klass->get_vadjustment_func = modest_webkit_msg_view_get_vadjustment;
	klass->get_hadjustment_func = modest_webkit_msg_view_get_hadjustment;
	klass->set_vadjustment_func = modest_webkit_msg_view_set_vadjustment;
	klass->set_hadjustment_func = modest_webkit_msg_view_set_hadjustment;
	klass->set_shadow_type_func = modest_webkit_msg_view_set_shadow_type;
	klass->get_shadow_type_func = modest_webkit_msg_view_get_shadow_type;
	klass->get_priority_func = modest_webkit_msg_view_get_priority;
	klass->set_priority_func = modest_webkit_msg_view_set_priority;
	klass->get_selected_attachments_func = modest_webkit_msg_view_get_selected_attachments;
	klass->get_attachments_func = modest_webkit_msg_view_get_attachments;
	klass->grab_focus_func = modest_webkit_msg_view_grab_focus;
	klass->remove_attachment_func = modest_webkit_msg_view_remove_attachment;
	klass->request_fetch_images_func = modest_webkit_msg_view_request_fetch_images;
	klass->set_branding_func = modest_webkit_msg_view_set_branding;
	klass->has_blocked_external_images_func = modest_webkit_msg_view_has_blocked_external_images;

	return;
}

static void
modest_webkit_msg_view_set_msg_with_other_body (ModestMsgView *self, TnyMsg *msg, TnyMimePart *other_body)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_msg_with_other_body_func (self, msg, other_body);
}

static void
modest_webkit_msg_view_set_msg_with_other_body_default (ModestMsgView *self, TnyMsg *msg, TnyMimePart *other_body)
{
	set_message (MODEST_WEBKIT_MSG_VIEW (self), msg, other_body);
}

static GtkAdjustment*
modest_webkit_msg_view_get_vadjustment (ModestMsgView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_vadjustment_func (self);
}

static GtkAdjustment*
modest_webkit_msg_view_get_vadjustment_default (ModestMsgView *self)
{
	return get_vadjustment (MODEST_WEBKIT_MSG_VIEW (self));
}

static GtkAdjustment*
modest_webkit_msg_view_get_hadjustment (ModestMsgView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_hadjustment_func (self);
}

static GtkAdjustment*
modest_webkit_msg_view_get_hadjustment_default (ModestMsgView *self)
{
	return get_hadjustment (MODEST_WEBKIT_MSG_VIEW (self));
}

static void
modest_webkit_msg_view_set_vadjustment (ModestMsgView *self, GtkAdjustment *adj)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_vadjustment_func (self, adj);
}

static void
modest_webkit_msg_view_set_vadjustment_default (ModestMsgView *self, GtkAdjustment *adj)
{
	set_vadjustment (MODEST_WEBKIT_MSG_VIEW (self), adj);
}

static void
modest_webkit_msg_view_set_hadjustment (ModestMsgView *self, GtkAdjustment *adj)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_hadjustment_func (self, adj);
}

static void
modest_webkit_msg_view_set_hadjustment_default (ModestMsgView *self, GtkAdjustment *adj)
{
	set_hadjustment (MODEST_WEBKIT_MSG_VIEW (self), adj);
}

static void
modest_webkit_msg_view_set_shadow_type (ModestMsgView *self, GtkShadowType type)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_shadow_type_func (self, type);
}

static void
modest_webkit_msg_view_set_shadow_type_default (ModestMsgView *self, GtkShadowType type)
{
	set_shadow_type (MODEST_WEBKIT_MSG_VIEW (self), type);
}

static GtkShadowType
modest_webkit_msg_view_get_shadow_type (ModestMsgView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_shadow_type_func (self);
}

static GtkShadowType
modest_webkit_msg_view_get_shadow_type_default (ModestMsgView *self)
{
	return get_shadow_type (MODEST_WEBKIT_MSG_VIEW (self));
}

static void
modest_webkit_msg_view_set_priority (ModestMsgView *self, TnyHeaderFlags flags)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_priority_func (self, flags);
}

static void
modest_webkit_msg_view_set_priority_default (ModestMsgView *self, TnyHeaderFlags flags)
{
	set_priority (MODEST_WEBKIT_MSG_VIEW (self), flags);
}

static TnyHeaderFlags
modest_webkit_msg_view_get_priority (ModestMsgView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_priority_func (self);
}

static TnyHeaderFlags
modest_webkit_msg_view_get_priority_default (ModestMsgView *self)
{
	return get_priority (MODEST_WEBKIT_MSG_VIEW (self));
}

static TnyList*
modest_webkit_msg_view_get_selected_attachments (ModestMsgView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_selected_attachments_func (self);
}

static TnyList*
modest_webkit_msg_view_get_selected_attachments_default (ModestMsgView *self)
{
	return get_selected_attachments (MODEST_WEBKIT_MSG_VIEW (self));
}

static TnyList*
modest_webkit_msg_view_get_attachments (ModestMsgView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->get_attachments_func (self);
}

static TnyList*
modest_webkit_msg_view_get_attachments_default (ModestMsgView *self)
{
	return get_attachments (MODEST_WEBKIT_MSG_VIEW (self));
}

static void
modest_webkit_msg_view_grab_focus (ModestMsgView *self)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->grab_focus_func (self);
}

static void
modest_webkit_msg_view_grab_focus_default (ModestMsgView *self)
{
	grab_focus (MODEST_WEBKIT_MSG_VIEW (self));
}

static void
modest_webkit_msg_view_remove_attachment (ModestMsgView *self, TnyMimePart *attachment)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->remove_attachment_func (self, attachment);
}

static void
modest_webkit_msg_view_remove_attachment_default (ModestMsgView *self, TnyMimePart *attachment)
{
	remove_attachment (MODEST_WEBKIT_MSG_VIEW (self), attachment);
}

static void
modest_webkit_msg_view_request_fetch_images (ModestMsgView *self)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->request_fetch_images_func (self);
}

static void
modest_webkit_msg_view_request_fetch_images_default (ModestMsgView *self)
{
	request_fetch_images (MODEST_WEBKIT_MSG_VIEW (self));
}

static void
modest_webkit_msg_view_set_branding (ModestMsgView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->set_branding_func (self, brand_name, brand_icon);
}

static void
modest_webkit_msg_view_set_branding_default (ModestMsgView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	set_branding (MODEST_WEBKIT_MSG_VIEW (self), brand_name, brand_icon);
}

static gboolean
modest_webkit_msg_view_has_blocked_external_images (ModestMsgView *self)
{
	return MODEST_WEBKIT_MSG_VIEW_GET_CLASS (self)->has_blocked_external_images_func (self);
}

static gboolean
modest_webkit_msg_view_has_blocked_external_images_default (ModestMsgView *self)
{
	return has_blocked_external_images (MODEST_WEBKIT_MSG_VIEW (self));
}

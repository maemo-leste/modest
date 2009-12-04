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
#include <widgets/modest-webkit-mime-part-view.h>
#include <string.h>
#include <webkit/webkit.h>
#include <tny-stream.h>
#include <tny-mime-part-view.h>
#include "modest-tny-mime-part.h"
#include <modest-stream-text-to-html.h>
#include <modest-text-utils.h>
#include <modest-conf.h>
#include <modest-runtime.h>
#include <widgets/modest-mime-part-view.h>
#include <widgets/modest-zoomable.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gdk/gdkkeysyms.h>
#include <modest-ui-constants.h>
#include <modest-tny-stream-webkit.h>

/* gobject structure methods */
static void    modest_webkit_mime_part_view_class_init (ModestWebkitMimePartViewClass *klass);
static void    tny_mime_part_view_init                  (gpointer g, gpointer iface_data);
static void    modest_mime_part_view_init               (gpointer g, gpointer iface_data);
static void    modest_zoomable_init                     (gpointer g, gpointer iface_data);
static void    modest_isearch_view_init                 (gpointer g, gpointer iface_data);
static void    modest_webkit_mime_part_view_init       (ModestWebkitMimePartView *self);
static void    modest_webkit_mime_part_view_finalize   (GObject *self);
static void    modest_webkit_mime_part_view_dispose    (GObject *self);

/* Webkit signal handlers */
static void on_resource_request_starting (WebKitWebView *webview,
					  WebKitWebFrame *frame,
					  WebKitWebResource *resource,
					  WebKitNetworkRequest *request,
					  WebKitNetworkResponse *response,
					  gpointer userdata);
static gboolean on_new_window_policy_decision_requested (WebKitWebView             *web_view,
							 WebKitWebFrame            *frame,
							 WebKitNetworkRequest      *request,
							 WebKitWebNavigationAction *navigation_action,
							 WebKitWebPolicyDecision   *policy_decision,
							 gpointer                   user_data);
static gboolean on_navigation_policy_decision_requested (WebKitWebView             *web_view,
							 WebKitWebFrame            *frame,
							 WebKitNetworkRequest      *request,
							 WebKitWebNavigationAction *navigation_action,
							 WebKitWebPolicyDecision   *policy_decision,
							 gpointer                   user_data);
static WebKitNavigationResponse on_navigation_requested                      (WebKitWebView        *web_view,
									      WebKitWebFrame       *frame,
									      WebKitNetworkRequest *request,
									      gpointer              user_data);
static void      on_notify_style  (GObject *obj, GParamSpec *spec, gpointer userdata);
static gboolean  update_style     (ModestWebkitMimePartView *self);
/* TnyMimePartView implementation */
static void modest_webkit_mime_part_view_clear (TnyMimePartView *self);
static void modest_webkit_mime_part_view_clear_default (TnyMimePartView *self);
static void modest_webkit_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part);
static void modest_webkit_mime_part_view_set_part_default (TnyMimePartView *self, TnyMimePart *part);
static TnyMimePart* modest_webkit_mime_part_view_get_part (TnyMimePartView *self);
static TnyMimePart* modest_webkit_mime_part_view_get_part_default (TnyMimePartView *self);
/* ModestMimePartView implementation */
static gboolean modest_webkit_mime_part_view_is_empty (ModestMimePartView *self);
static gboolean modest_webkit_mime_part_view_is_empty_default (ModestMimePartView *self);
static gboolean modest_webkit_mime_part_view_get_view_images (ModestMimePartView *self);
static gboolean modest_webkit_mime_part_view_get_view_images_default (ModestMimePartView *self);
static void     modest_webkit_mime_part_view_set_view_images (ModestMimePartView *self, gboolean view_images);
static void     modest_webkit_mime_part_view_set_view_images_default (ModestMimePartView *self, gboolean view_images);
static gboolean modest_webkit_mime_part_view_has_external_images (ModestMimePartView *self);
static gboolean modest_webkit_mime_part_view_has_external_images_default (ModestMimePartView *self);
/* ModestZoomable implementation */
static gdouble modest_webkit_mime_part_view_get_zoom (ModestZoomable *self);
static void modest_webkit_mime_part_view_set_zoom (ModestZoomable *self, gdouble value);
static gboolean modest_webkit_mime_part_view_zoom_minus (ModestZoomable *self);
static gboolean modest_webkit_mime_part_view_zoom_plus (ModestZoomable *self);
static gdouble modest_webkit_mime_part_view_get_zoom_default (ModestZoomable *self);
static void modest_webkit_mime_part_view_set_zoom_default (ModestZoomable *self, gdouble value);
static gboolean modest_webkit_mime_part_view_zoom_minus_default (ModestZoomable *self);
static gboolean modest_webkit_mime_part_view_zoom_plus_default (ModestZoomable *self);
/* ModestISearchView implementation */
static gboolean modest_webkit_mime_part_view_search                    (ModestISearchView *self, const gchar *string);
static gboolean modest_webkit_mime_part_view_search_next               (ModestISearchView *self);
static gboolean modest_webkit_mime_part_view_get_selection_area        (ModestISearchView *self, gint *x, gint *y, 
									 gint *width, gint *height);
static gboolean modest_webkit_mime_part_view_search_default            (ModestISearchView *self, const gchar *string);
static gboolean modest_webkit_mime_part_view_search_next_default       (ModestISearchView *self);
static gboolean modest_webkit_mime_part_view_get_selection_area_default (ModestISearchView *self, gint *x, gint *y, 
									  gint *width, gint *height);


/* internal api */
static TnyMimePart  *get_part   (ModestWebkitMimePartView *self);
static void          set_html_part   (ModestWebkitMimePartView *self, TnyMimePart *part, const gchar *encoding);
static void          set_text_part   (ModestWebkitMimePartView *self, TnyMimePart *part);
static void          set_empty_part  (ModestWebkitMimePartView *self);
static void          set_part   (ModestWebkitMimePartView *self, TnyMimePart *part);
static gboolean      is_empty   (ModestWebkitMimePartView *self);
static gboolean      get_view_images   (ModestWebkitMimePartView *self);
static void          set_view_images   (ModestWebkitMimePartView *self, gboolean view_images);
static gboolean      has_external_images   (ModestWebkitMimePartView *self);
static void          set_zoom   (ModestWebkitMimePartView *self, gdouble zoom);
static gdouble       get_zoom   (ModestWebkitMimePartView *self);
static gboolean      search             (ModestWebkitMimePartView *self, const gchar *string);
static gboolean      search_next        (ModestWebkitMimePartView *self);
static gboolean      get_selection_area (ModestWebkitMimePartView *self, gint *x, gint *y,
					 gint *width, gint *height);

typedef struct _ModestWebkitMimePartViewPrivate ModestWebkitMimePartViewPrivate;
struct _ModestWebkitMimePartViewPrivate {
	TnyMimePart *part;
	gdouble current_zoom;
	gboolean view_images;
	gboolean has_external_images;
	GSList *sighandlers;
};

#define MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										       MODEST_TYPE_WEBKIT_MIME_PART_VIEW, \
										       ModestWebkitMimePartViewPrivate))

enum {
	STOP_STREAMS_SIGNAL,
	LIMIT_ERROR_SIGNAL,
	LAST_SIGNAL
};

static WebKitWebViewClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

GtkWidget *
modest_webkit_mime_part_view_new ()
{
	return g_object_new (MODEST_TYPE_WEBKIT_MIME_PART_VIEW, NULL);
}

/* GOBJECT IMPLEMENTATION */
GType
modest_webkit_mime_part_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestWebkitMimePartViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_webkit_mime_part_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestWebkitMimePartView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_webkit_mime_part_view_init,
			NULL
		};

		static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
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

 		my_type = g_type_register_static (WEBKIT_TYPE_WEB_VIEW,
		                                  "ModestWebkitMimePartView",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_MIME_PART_VIEW, 
			&tny_mime_part_view_info);

		g_type_add_interface_static (my_type, MODEST_TYPE_MIME_PART_VIEW, 
			&modest_mime_part_view_info);

		g_type_add_interface_static (my_type, MODEST_TYPE_ZOOMABLE, 
			&modest_zoomable_info);
		g_type_add_interface_static (my_type, MODEST_TYPE_ISEARCH_VIEW, 
			&modest_isearch_view_info);
	}
	return my_type;
}

static void
modest_webkit_mime_part_view_class_init (ModestWebkitMimePartViewClass *klass)
{
	GObjectClass *gobject_class;
	GtkBindingSet *binding_set;

	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->dispose = modest_webkit_mime_part_view_dispose;
	gobject_class->finalize = modest_webkit_mime_part_view_finalize;

	klass->get_part_func = modest_webkit_mime_part_view_get_part_default;
	klass->set_part_func = modest_webkit_mime_part_view_set_part_default;
	klass->clear_func = modest_webkit_mime_part_view_clear_default;
	klass->is_empty_func = modest_webkit_mime_part_view_is_empty_default;
	klass->get_view_images_func = modest_webkit_mime_part_view_get_view_images_default;
	klass->set_view_images_func = modest_webkit_mime_part_view_set_view_images_default;
	klass->has_external_images_func = modest_webkit_mime_part_view_has_external_images_default;
	klass->get_zoom_func = modest_webkit_mime_part_view_get_zoom_default;
	klass->set_zoom_func = modest_webkit_mime_part_view_set_zoom_default;
	klass->zoom_minus_func = modest_webkit_mime_part_view_zoom_minus_default;
	klass->zoom_plus_func = modest_webkit_mime_part_view_zoom_plus_default;
	klass->search_func = modest_webkit_mime_part_view_search_default;
	klass->search_next_func = modest_webkit_mime_part_view_search_next_default;
	klass->get_selection_area_func = modest_webkit_mime_part_view_get_selection_area_default;

	binding_set = gtk_binding_set_by_class (klass);
	gtk_binding_entry_skip (binding_set, GDK_Down, 0);
	gtk_binding_entry_skip (binding_set, GDK_Up, 0);
	gtk_binding_entry_skip (binding_set, GDK_KP_Up, 0);
	gtk_binding_entry_skip (binding_set, GDK_KP_Down, 0);
	gtk_binding_entry_skip (binding_set, GDK_Page_Down, 0);
	gtk_binding_entry_skip (binding_set, GDK_Page_Up, 0);
	gtk_binding_entry_skip (binding_set, GDK_KP_Page_Up, 0);
	gtk_binding_entry_skip (binding_set, GDK_KP_Page_Down, 0);
	gtk_binding_entry_skip (binding_set, GDK_Home, 0);
	gtk_binding_entry_skip (binding_set, GDK_End, 0);
	gtk_binding_entry_skip (binding_set, GDK_KP_Home, 0);
	gtk_binding_entry_skip (binding_set, GDK_KP_End, 0);

	g_type_class_add_private (gobject_class, sizeof(ModestWebkitMimePartViewPrivate));

	signals[STOP_STREAMS_SIGNAL] = 
		g_signal_new ("stop-streams",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestWebkitMimePartViewClass,stop_streams),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[LIMIT_ERROR_SIGNAL] = 
		g_signal_new ("limit-error",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestWebkitMimePartViewClass,limit_error),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

}

static void
modest_webkit_mime_part_view_init (ModestWebkitMimePartView *self)
{
	ModestWebkitMimePartViewPrivate *priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (self);
	GdkColor base;
	GdkColor text;
	WebKitWebSettings *settings;

	gdk_color_parse ("#fff", &base);
	gdk_color_parse ("#000", &text);
	gtk_widget_modify_base (GTK_WIDGET (self), GTK_STATE_NORMAL, &base);
	gtk_widget_modify_text (GTK_WIDGET (self), GTK_STATE_NORMAL, &text);

	priv->sighandlers = NULL;

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self), "notify::style",
						       G_CALLBACK (on_notify_style), (gpointer) self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (self), "resource-request-starting",
						       G_CALLBACK (on_resource_request_starting), (gpointer) self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (self), "navigation-policy-decision-requested",
						       G_CALLBACK (on_navigation_policy_decision_requested), (gpointer) self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (self), "new-window-policy-decision-requested",
						       G_CALLBACK (on_new_window_policy_decision_requested), (gpointer) self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (self), "navigation-requested",
						       G_CALLBACK (on_navigation_requested), (gpointer) self);

	priv->part = NULL;
	priv->current_zoom = 1.0;
	priv->view_images = FALSE;
	priv->has_external_images = FALSE;

	settings = webkit_web_settings_new ();
	g_object_set (G_OBJECT (settings),
		      "auto-load-images", FALSE,
		      "enable-html5-database", FALSE,
		      "enable-html5-local-storage", FALSE, 
		      "enable-offline-web-application-cache", FALSE,
		      "enable-plugins", FALSE,
		      "enable-private-browsing", TRUE,
		      "enable-scripts", FALSE,
		      NULL);
	webkit_web_view_set_settings (WEBKIT_WEB_VIEW (self), settings);
	g_object_unref (settings);
	g_object_set (G_OBJECT (self), 
		      "editable", FALSE,
		      NULL);
}

static void
modest_webkit_mime_part_view_finalize (GObject *obj)
{
	ModestWebkitMimePartViewPrivate *priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (obj);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
modest_webkit_mime_part_view_dispose (GObject *obj)
{
	ModestWebkitMimePartViewPrivate *priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (obj);

	g_signal_emit (G_OBJECT (obj), signals[STOP_STREAMS_SIGNAL], 0);

	if (priv->part) {
		g_object_unref (priv->part);
		priv->part = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

/* WEBKIT SIGNALS HANDLERS */

static WebKitNavigationResponse
on_navigation_requested                      (WebKitWebView        *web_view,
					      WebKitWebFrame       *frame,
					      WebKitNetworkRequest *request,
					      gpointer              user_data)
{
	const gchar *uri;

	uri = webkit_network_request_get_uri (request);
	if (g_strcmp0 (uri, "about:blank") == 0) {
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
	} else if (g_str_has_prefix (uri, "cid:") == 0) {
		return WEBKIT_NAVIGATION_RESPONSE_DOWNLOAD;
	} else {
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
	}
}

static void
on_resource_request_starting (WebKitWebView *webview,
			      WebKitWebFrame *frame,
			      WebKitWebResource *resource,
			      WebKitNetworkRequest *request,
			      WebKitNetworkResponse *response,
			      gpointer userdata)
{
	ModestWebkitMimePartView *self = (ModestWebkitMimePartView *) userdata;
	g_return_if_fail (MODEST_IS_WEBKIT_MIME_PART_VIEW (self));

	if (g_str_has_prefix (webkit_network_request_get_uri (request), "http:")) {
		ModestWebkitMimePartViewPrivate *priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (self);

		if (!priv->view_images)
			priv->has_external_images = TRUE;
	}

	webkit_network_request_set_uri (request, "about:blank");
}

static gboolean
on_navigation_policy_decision_requested (WebKitWebView             *web_view,
					 WebKitWebFrame            *frame,
					 WebKitNetworkRequest      *request,
					 WebKitWebNavigationAction *navigation_action,
					 WebKitWebPolicyDecision   *policy_decision,
					 gpointer                   user_data)
{
	WebKitWebNavigationReason reason;
	reason = webkit_web_navigation_action_get_reason (navigation_action);
	if (reason == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
		const gchar *uri;
		gboolean result;

		webkit_web_policy_decision_ignore (policy_decision);
		uri = webkit_network_request_get_uri (WEBKIT_NETWORK_REQUEST (request));
		g_signal_emit_by_name (G_OBJECT (user_data), "activate-link", uri, &result);

		return TRUE;
	} else if (reason != WEBKIT_WEB_NAVIGATION_REASON_OTHER) {
		webkit_web_policy_decision_ignore (policy_decision);
		return TRUE;
	}
	return FALSE;
}

static gboolean
on_new_window_policy_decision_requested (WebKitWebView             *web_view,
					 WebKitWebFrame            *frame,
					 WebKitNetworkRequest      *request,
					 WebKitWebNavigationAction *navigation_action,
					 WebKitWebPolicyDecision   *policy_decision,
					 gpointer                   user_data)
{
	WebKitWebNavigationReason reason;
	reason = webkit_web_navigation_action_get_reason (navigation_action);
	if (reason == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
		const gchar *uri;
		gboolean result;

		webkit_web_policy_decision_ignore (policy_decision);
		uri = webkit_network_request_get_uri (WEBKIT_NETWORK_REQUEST (request));
		g_signal_emit_by_name (G_OBJECT (user_data), "activate-link", uri, &result);

		return TRUE;
	} else if (reason != WEBKIT_WEB_NAVIGATION_REASON_OTHER) {
		webkit_web_policy_decision_ignore (policy_decision);
		return TRUE;
	}
	return FALSE;
}

static void 
on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata)
{
	if (strcmp ("style", spec->name) == 0) {
		g_idle_add_full (G_PRIORITY_DEFAULT, (GSourceFunc) update_style, 
				 g_object_ref (obj), g_object_unref);
		gtk_widget_queue_draw (GTK_WIDGET (obj));
	}
}

gboolean
same_color (GdkColor *a, GdkColor *b)
{
	return ((a->red == b->red) && 
		(a->green == b->green) && 
		(a->blue == b->blue));
}

static gboolean
update_style (ModestWebkitMimePartView *self)
{
	GdkColor base;
	GdkColor text;
	GtkRcStyle *rc_style;

	gdk_threads_enter ();

	if (GTK_WIDGET_VISIBLE (self)) {
		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (self));

		gdk_color_parse ("#fff", &base);
		gdk_color_parse ("#000", &text);

		if (!same_color (&(rc_style->base[GTK_STATE_NORMAL]), &base) &&
		    !same_color (&(rc_style->text[GTK_STATE_NORMAL]), &text)) {

			rc_style->base[GTK_STATE_NORMAL] = base;
			rc_style->text[GTK_STATE_NORMAL] = text;
			gtk_widget_modify_style (GTK_WIDGET (self), rc_style);
		}
	}

	gdk_threads_leave ();

	return FALSE;
}


/* INTERNAL API */
static void
decode_to_stream_cb (TnyMimePart *self,
		     gboolean cancelled,
		     TnyStream *stream,
		     GError *err,
		     gpointer user_data)
{
	ModestWebkitMimePartView *view = (ModestWebkitMimePartView *) user_data;

	if (MODEST_IS_STREAM_TEXT_TO_HTML (stream)) {
		if (tny_stream_write (stream, "\n", 1) == -1) {
			g_warning ("failed to write CR in %s", __FUNCTION__);
		}
		if (modest_stream_text_to_html_limit_reached (MODEST_STREAM_TEXT_TO_HTML (stream))) {
			g_signal_emit (G_OBJECT (view), signals[LIMIT_ERROR_SIGNAL], 0);
		}
		tny_stream_reset (stream);
	} else {
		if (modest_tny_stream_webkit_limit_reached (MODEST_TNY_STREAM_WEBKIT (stream))) {
			g_signal_emit (G_OBJECT (view), signals[LIMIT_ERROR_SIGNAL], 0);
		}
	}
	tny_stream_close (stream);
}

static void
set_html_part (ModestWebkitMimePartView *self, TnyMimePart *part, const gchar *encoding)
{
	TnyStream *tny_stream;

	g_return_if_fail (self);
	g_return_if_fail (part);

	g_signal_emit (G_OBJECT (self), signals[STOP_STREAMS_SIGNAL], 0);

	tny_stream     = TNY_STREAM(modest_tny_stream_webkit_new (WEBKIT_WEB_VIEW (self), "text/html", encoding));
	modest_tny_stream_webkit_set_max_size (MODEST_TNY_STREAM_WEBKIT (tny_stream), 128*1024);
	tny_stream_reset (tny_stream);

	tny_mime_part_decode_to_stream_async (TNY_MIME_PART (part),
					      tny_stream, decode_to_stream_cb,
					      NULL, self);
	g_object_unref (tny_stream);
}

static void
set_text_part (ModestWebkitMimePartView *self, TnyMimePart *part)
{
	TnyStream* text_to_html_stream, *tny_stream;

	g_return_if_fail (self);
	g_return_if_fail (part);

	g_signal_emit (G_OBJECT (self), signals[STOP_STREAMS_SIGNAL], 0);

	tny_stream =  TNY_STREAM(modest_tny_stream_webkit_new (WEBKIT_WEB_VIEW (self), "text/html", "utf-8"));
	modest_tny_stream_webkit_set_max_size (MODEST_TNY_STREAM_WEBKIT (tny_stream), 128*1024);
	text_to_html_stream = TNY_STREAM (modest_stream_text_to_html_new (tny_stream));
	modest_stream_text_to_html_set_linkify_limit (MODEST_STREAM_TEXT_TO_HTML (text_to_html_stream),
						      64*1024);
	modest_stream_text_to_html_set_full_limit (MODEST_STREAM_TEXT_TO_HTML (text_to_html_stream),
						   128*1024);
	modest_stream_text_to_html_set_line_limit (MODEST_STREAM_TEXT_TO_HTML (text_to_html_stream),
						   1024);

	tny_mime_part_decode_to_stream_async (TNY_MIME_PART (part),
					      text_to_html_stream, decode_to_stream_cb,
					      NULL, self);

	g_object_unref (G_OBJECT(text_to_html_stream));
	g_object_unref (G_OBJECT(tny_stream));
}

static void
set_empty_part (ModestWebkitMimePartView *self)
{
	g_return_if_fail (self);

	g_signal_emit (G_OBJECT (self), signals[STOP_STREAMS_SIGNAL], 0);
	webkit_web_view_load_string (WEBKIT_WEB_VIEW (self), "", "text/plain", "utf-8", NULL);
}

static void
set_part (ModestWebkitMimePartView *self, TnyMimePart *part)
{
	ModestWebkitMimePartViewPrivate *priv;
	gchar *header_content_type, *header_content_type_lower;
	const gchar *tmp;
	gchar *charset = NULL;

	g_return_if_fail (self);
	
	priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE(self);
	priv->has_external_images = FALSE;

	if (part != priv->part) {
		if (priv->part)
			g_object_unref (G_OBJECT(priv->part));
		if (part)
			g_object_ref   (G_OBJECT(part));
		priv->part = part;
	}
	
	if (!part) {
		set_empty_part (self);
		return;
	}

	header_content_type = modest_tny_mime_part_get_header_value (part, "Content-Type");
	if (header_content_type) {
		header_content_type = g_strstrip (header_content_type);
		header_content_type_lower = g_ascii_strdown (header_content_type, -1);
	} else {
		header_content_type_lower = NULL;
	}

	if (header_content_type_lower) {
		tmp = strstr (header_content_type_lower, "charset=");
		if (tmp) {
			const gchar *tmp2;
			tmp = tmp + strlen ("charset=");
			
			tmp2 = strstr (tmp, ";");
			if (tmp2) {
				charset = g_strndup (tmp, tmp2-tmp);
			} else {
				charset = g_strdup (tmp);
			}
		}
	}

	if (tny_mime_part_content_type_is (part, "text/html")) {
		set_html_part (self, part, charset);
	} else {
		if (tny_mime_part_content_type_is (part, "message/rfc822")) {
			if (header_content_type) {
				if (g_str_has_prefix (header_content_type_lower, "text/html"))
					set_html_part (self, part, charset);
				else 
					set_text_part (self, part);

			} else {
				set_text_part (self, part);
			}
		} else {
			set_text_part (self, part);
		}
	}
	g_free (header_content_type_lower);
	g_free (header_content_type);

}

static TnyMimePart*
get_part (ModestWebkitMimePartView *self)
{
	TnyMimePart *part;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MIME_PART_VIEW (self), NULL);

	part = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE(self)->part;

	if (part)
		g_object_ref (part);
	
	return part;
}

static gboolean      
is_empty   (ModestWebkitMimePartView *self)
{
	return FALSE;
}

static gboolean      
get_view_images   (ModestWebkitMimePartView *self)
{
	ModestWebkitMimePartViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MIME_PART_VIEW (self), FALSE);

	priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (self);
	return priv->view_images;
}

static void
set_view_images   (ModestWebkitMimePartView *self, gboolean view_images)
{
	ModestWebkitMimePartViewPrivate *priv;

	g_return_if_fail (MODEST_IS_WEBKIT_MIME_PART_VIEW (self));

	priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (self);
	priv->view_images = view_images;
}

static gboolean      
has_external_images   (ModestWebkitMimePartView *self)
{
	ModestWebkitMimePartViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MIME_PART_VIEW (self), FALSE);

	priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (self);
	return priv->has_external_images;
}

static void
set_zoom (ModestWebkitMimePartView *self, gdouble zoom)
{
	ModestWebkitMimePartViewPrivate *priv;

	g_return_if_fail (MODEST_IS_WEBKIT_MIME_PART_VIEW (self));

	priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (self);
	priv->current_zoom = zoom;

	gtk_widget_queue_resize (GTK_WIDGET (self));
}

static gdouble
get_zoom (ModestWebkitMimePartView *self)
{
	ModestWebkitMimePartViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_WEBKIT_MIME_PART_VIEW (self), 1.0);

	priv = MODEST_WEBKIT_MIME_PART_VIEW_GET_PRIVATE (self);

	return priv->current_zoom;
}

static gboolean
search (ModestWebkitMimePartView *self, 
	const gchar *string)
{
	return FALSE;
}

static gboolean
search_next (ModestWebkitMimePartView *self)
{
	return FALSE;
}

static gboolean
get_selection_area (ModestWebkitMimePartView *self, 
		    gint *x, gint *y,
		    gint *width, gint *height)
{
	return FALSE;
}


/* TNY MIME PART IMPLEMENTATION */

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part = modest_webkit_mime_part_view_get_part;
	klass->set_part = modest_webkit_mime_part_view_set_part;
	klass->clear = modest_webkit_mime_part_view_clear;

	return;
}

static TnyMimePart* 
modest_webkit_mime_part_view_get_part (TnyMimePartView *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->get_part_func (self);
}


static TnyMimePart* 
modest_webkit_mime_part_view_get_part_default (TnyMimePartView *self)
{
	return TNY_MIME_PART (get_part (MODEST_WEBKIT_MIME_PART_VIEW (self)));
}

static void
modest_webkit_mime_part_view_set_part (TnyMimePartView *self,
					TnyMimePart *part)
{
	MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->set_part_func (self, part);
}

static void
modest_webkit_mime_part_view_set_part_default (TnyMimePartView *self,
						TnyMimePart *part)
{
	g_return_if_fail ((part == NULL) || TNY_IS_MIME_PART (part));

	set_part (MODEST_WEBKIT_MIME_PART_VIEW (self), part);
}

static void
modest_webkit_mime_part_view_clear (TnyMimePartView *self)
{
	MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->clear_func (self);
}

static void
modest_webkit_mime_part_view_clear_default (TnyMimePartView *self)
{
	set_part (MODEST_WEBKIT_MIME_PART_VIEW (self), NULL);
}

/* MODEST MIME PART VIEW IMPLEMENTATION */

static void
modest_mime_part_view_init (gpointer g, gpointer iface_data)
{
	ModestMimePartViewIface *klass = (ModestMimePartViewIface *)g;

	klass->is_empty_func = modest_webkit_mime_part_view_is_empty;
	klass->get_view_images_func = modest_webkit_mime_part_view_get_view_images;
	klass->set_view_images_func = modest_webkit_mime_part_view_set_view_images;
	klass->has_external_images_func = modest_webkit_mime_part_view_has_external_images;

	return;
}

static gboolean
modest_webkit_mime_part_view_is_empty (ModestMimePartView *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->is_empty_func (self);
}

static gboolean
modest_webkit_mime_part_view_get_view_images (ModestMimePartView *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->get_view_images_func (self);
}

static void
modest_webkit_mime_part_view_set_view_images (ModestMimePartView *self, gboolean view_images)
{
	MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->set_view_images_func (self, view_images);
}

static gboolean
modest_webkit_mime_part_view_has_external_images (ModestMimePartView *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->has_external_images_func (self);
}

static gboolean
modest_webkit_mime_part_view_is_empty_default (ModestMimePartView *self)
{
	return is_empty (MODEST_WEBKIT_MIME_PART_VIEW (self));
}

static gboolean
modest_webkit_mime_part_view_get_view_images_default (ModestMimePartView *self)
{
	return get_view_images (MODEST_WEBKIT_MIME_PART_VIEW (self));
}

static void
modest_webkit_mime_part_view_set_view_images_default (ModestMimePartView *self, gboolean view_images)
{
	set_view_images (MODEST_WEBKIT_MIME_PART_VIEW (self), view_images);
}

static gboolean
modest_webkit_mime_part_view_has_external_images_default (ModestMimePartView *self)
{
	return has_external_images (MODEST_WEBKIT_MIME_PART_VIEW (self));
}


/* MODEST ZOOMABLE IMPLEMENTATION */
static void
modest_zoomable_init (gpointer g, gpointer iface_data)
{
	ModestZoomableIface *klass = (ModestZoomableIface *)g;
	
	klass->get_zoom_func = modest_webkit_mime_part_view_get_zoom;
	klass->set_zoom_func = modest_webkit_mime_part_view_set_zoom;
	klass->zoom_minus_func = modest_webkit_mime_part_view_zoom_minus;
	klass->zoom_plus_func = modest_webkit_mime_part_view_zoom_plus;

	return;
}

static gdouble
modest_webkit_mime_part_view_get_zoom (ModestZoomable *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->get_zoom_func (self);
}

static gdouble
modest_webkit_mime_part_view_get_zoom_default (ModestZoomable *self)
{
	return get_zoom (MODEST_WEBKIT_MIME_PART_VIEW (self));
}

static void
modest_webkit_mime_part_view_set_zoom (ModestZoomable *self, gdouble value)
{
	MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->set_zoom_func (self, value);
}

static void
modest_webkit_mime_part_view_set_zoom_default (ModestZoomable *self, gdouble value)
{
	set_zoom (MODEST_WEBKIT_MIME_PART_VIEW (self), value);
}

static gboolean
modest_webkit_mime_part_view_zoom_minus (ModestZoomable *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->zoom_minus_func (self);
}

static gboolean
modest_webkit_mime_part_view_zoom_minus_default (ModestZoomable *self)
{
	/* operation not supported in ModestWebkitMimePartView */
	return FALSE;
}

static gboolean
modest_webkit_mime_part_view_zoom_plus (ModestZoomable *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->zoom_plus_func (self);
}

static gboolean
modest_webkit_mime_part_view_zoom_plus_default (ModestZoomable *self)
{
	/* operation not supported in ModestWebkitMimePartView */
	return FALSE;
}

/* ISEARCH VIEW IMPLEMENTATION */
static void
modest_isearch_view_init (gpointer g, gpointer iface_data)
{
	ModestISearchViewIface *klass = (ModestISearchViewIface *)g;
	
	klass->search_func = modest_webkit_mime_part_view_search;
	klass->search_next_func = modest_webkit_mime_part_view_search_next;
	klass->get_selection_area_func = modest_webkit_mime_part_view_get_selection_area;

	return;
}

static gboolean 
modest_webkit_mime_part_view_search (ModestISearchView *self, const gchar *string)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->search_func (self, string);
}

static gboolean 
modest_webkit_mime_part_view_search_default (ModestISearchView *self, const gchar *string)
{
	return search (MODEST_WEBKIT_MIME_PART_VIEW (self), string);
}

static gboolean 
modest_webkit_mime_part_view_search_next(ModestISearchView *self)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->search_next_func (self);
}

static gboolean 
modest_webkit_mime_part_view_search_next_default (ModestISearchView *self)
{
	return search_next (MODEST_WEBKIT_MIME_PART_VIEW (self));
}

static gboolean 
modest_webkit_mime_part_view_get_selection_area (ModestISearchView *self, gint *x, gint *y, 
						  gint *width, gint *height)
{
	return MODEST_WEBKIT_MIME_PART_VIEW_GET_CLASS (self)->get_selection_area_func (self, x, y, width, height);
}

static gboolean 
modest_webkit_mime_part_view_get_selection_area_default (ModestISearchView *self, gint *x, gint *y, 
							  gint *width, gint *height)
{
	return get_selection_area (MODEST_WEBKIT_MIME_PART_VIEW (self), x, y, width, height);
}

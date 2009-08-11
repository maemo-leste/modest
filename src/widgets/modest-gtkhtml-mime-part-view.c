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
#include <widgets/modest-gtkhtml-mime-part-view.h>
#include <string.h>
#include <gtkhtml/gtkhtml-stream.h>
#include <gtkhtml/gtkhtml-search.h>
#include <tny-stream.h>
#include <tny-mime-part-view.h>
#include "modest-tny-mime-part.h"
#include <modest-stream-text-to-html.h>
#include <modest-text-utils.h>
#include <modest-conf.h>
#include <modest-runtime.h>
#include <widgets/modest-mime-part-view.h>
#include <widgets/modest-zoomable.h>
#include <widgets/modest-tny-stream-gtkhtml.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gdk/gdkkeysyms.h>
#include <modest-ui-constants.h>

/* gobject structure methods */
static void    modest_gtkhtml_mime_part_view_class_init (ModestGtkhtmlMimePartViewClass *klass);
static void    tny_mime_part_view_init                  (gpointer g, gpointer iface_data);
static void    modest_mime_part_view_init               (gpointer g, gpointer iface_data);
static void    modest_zoomable_init                     (gpointer g, gpointer iface_data);
static void    modest_isearch_view_init                 (gpointer g, gpointer iface_data);
static void    modest_gtkhtml_mime_part_view_init       (ModestGtkhtmlMimePartView *self);
static void    modest_gtkhtml_mime_part_view_finalize   (GObject *self);
static void    modest_gtkhtml_mime_part_view_dispose    (GObject *self);

/* GtkHTML signal handlers */
static gboolean  on_link_clicked  (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMimePartView *self);
static gboolean  on_url           (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMimePartView *self);
static gboolean  on_url_requested (GtkWidget *widget, const gchar *uri, GtkHTMLStream *stream,
				   ModestGtkhtmlMimePartView *self);
static void      on_notify_style  (GObject *obj, GParamSpec *spec, gpointer userdata);
static gboolean  update_style     (ModestGtkhtmlMimePartView *self);
/* TnyMimePartView implementation */
static void modest_gtkhtml_mime_part_view_clear (TnyMimePartView *self);
static void modest_gtkhtml_mime_part_view_clear_default (TnyMimePartView *self);
static void modest_gtkhtml_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part);
static void modest_gtkhtml_mime_part_view_set_part_default (TnyMimePartView *self, TnyMimePart *part);
static TnyMimePart* modest_gtkhtml_mime_part_view_get_part (TnyMimePartView *self);
static TnyMimePart* modest_gtkhtml_mime_part_view_get_part_default (TnyMimePartView *self);
/* ModestMimePartView implementation */
static gboolean modest_gtkhtml_mime_part_view_is_empty (ModestMimePartView *self);
static gboolean modest_gtkhtml_mime_part_view_is_empty_default (ModestMimePartView *self);
static gboolean modest_gtkhtml_mime_part_view_get_view_images (ModestMimePartView *self);
static gboolean modest_gtkhtml_mime_part_view_get_view_images_default (ModestMimePartView *self);
static void     modest_gtkhtml_mime_part_view_set_view_images (ModestMimePartView *self, gboolean view_images);
static void     modest_gtkhtml_mime_part_view_set_view_images_default (ModestMimePartView *self, gboolean view_images);
static gboolean modest_gtkhtml_mime_part_view_has_external_images (ModestMimePartView *self);
static gboolean modest_gtkhtml_mime_part_view_has_external_images_default (ModestMimePartView *self);
/* ModestZoomable implementation */
static gdouble modest_gtkhtml_mime_part_view_get_zoom (ModestZoomable *self);
static void modest_gtkhtml_mime_part_view_set_zoom (ModestZoomable *self, gdouble value);
static gboolean modest_gtkhtml_mime_part_view_zoom_minus (ModestZoomable *self);
static gboolean modest_gtkhtml_mime_part_view_zoom_plus (ModestZoomable *self);
static gdouble modest_gtkhtml_mime_part_view_get_zoom_default (ModestZoomable *self);
static void modest_gtkhtml_mime_part_view_set_zoom_default (ModestZoomable *self, gdouble value);
static gboolean modest_gtkhtml_mime_part_view_zoom_minus_default (ModestZoomable *self);
static gboolean modest_gtkhtml_mime_part_view_zoom_plus_default (ModestZoomable *self);
/* ModestISearchView implementation */
static gboolean modest_gtkhtml_mime_part_view_search                    (ModestISearchView *self, const gchar *string);
static gboolean modest_gtkhtml_mime_part_view_search_next               (ModestISearchView *self);
static gboolean modest_gtkhtml_mime_part_view_get_selection_area        (ModestISearchView *self, gint *x, gint *y, 
									 gint *width, gint *height);
static gboolean modest_gtkhtml_mime_part_view_search_default            (ModestISearchView *self, const gchar *string);
static gboolean modest_gtkhtml_mime_part_view_search_next_default       (ModestISearchView *self);
static gboolean modest_gtkhtml_mime_part_view_get_selection_area_default (ModestISearchView *self, gint *x, gint *y, 
									  gint *width, gint *height);


/* internal api */
static TnyMimePart  *get_part   (ModestGtkhtmlMimePartView *self);
static void          set_html_part   (ModestGtkhtmlMimePartView *self, TnyMimePart *part, const gchar *encoding);
static void          set_text_part   (ModestGtkhtmlMimePartView *self, TnyMimePart *part);
static void          set_empty_part  (ModestGtkhtmlMimePartView *self);
static void          set_part   (ModestGtkhtmlMimePartView *self, TnyMimePart *part);
static gboolean      is_empty   (ModestGtkhtmlMimePartView *self);
static gboolean      get_view_images   (ModestGtkhtmlMimePartView *self);
static void          set_view_images   (ModestGtkhtmlMimePartView *self, gboolean view_images);
static gboolean      has_external_images   (ModestGtkhtmlMimePartView *self);
static void          set_zoom   (ModestGtkhtmlMimePartView *self, gdouble zoom);
static gdouble       get_zoom   (ModestGtkhtmlMimePartView *self);
static gboolean      has_contents_receiver (gpointer engine, const gchar *data,
					    size_t len, gboolean *has_contents);
static gboolean      search             (ModestGtkhtmlMimePartView *self, const gchar *string);
static gboolean      search_next        (ModestGtkhtmlMimePartView *self);
static gboolean      get_selection_area (ModestGtkhtmlMimePartView *self, gint *x, gint *y,
					 gint *width, gint *height);

typedef struct _ModestGtkhtmlMimePartViewPrivate ModestGtkhtmlMimePartViewPrivate;
struct _ModestGtkhtmlMimePartViewPrivate {
	TnyMimePart *part;
	gdouble current_zoom;
	gboolean view_images;
	gboolean has_external_images;
	GSList *sighandlers;
};

#define MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										       MODEST_TYPE_GTKHTML_MIME_PART_VIEW, \
										       ModestGtkhtmlMimePartViewPrivate))

enum {
	STOP_STREAMS_SIGNAL,
	LIMIT_ERROR_SIGNAL,
	LAST_SIGNAL
};

static GtkHTMLClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

GtkWidget *
modest_gtkhtml_mime_part_view_new ()
{
	return g_object_new (MODEST_TYPE_GTKHTML_MIME_PART_VIEW, NULL);
}

/* GOBJECT IMPLEMENTATION */
GType
modest_gtkhtml_mime_part_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestGtkhtmlMimePartViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_gtkhtml_mime_part_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestGtkhtmlMimePartView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_gtkhtml_mime_part_view_init,
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

 		my_type = g_type_register_static (GTK_TYPE_HTML,
		                                  "ModestGtkhtmlMimePartView",
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
modest_gtkhtml_mime_part_view_class_init (ModestGtkhtmlMimePartViewClass *klass)
{
	GObjectClass *gobject_class;
	GtkBindingSet *binding_set;

	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->dispose = modest_gtkhtml_mime_part_view_dispose;
	gobject_class->finalize = modest_gtkhtml_mime_part_view_finalize;

	klass->get_part_func = modest_gtkhtml_mime_part_view_get_part_default;
	klass->set_part_func = modest_gtkhtml_mime_part_view_set_part_default;
	klass->clear_func = modest_gtkhtml_mime_part_view_clear_default;
	klass->is_empty_func = modest_gtkhtml_mime_part_view_is_empty_default;
	klass->get_view_images_func = modest_gtkhtml_mime_part_view_get_view_images_default;
	klass->set_view_images_func = modest_gtkhtml_mime_part_view_set_view_images_default;
	klass->has_external_images_func = modest_gtkhtml_mime_part_view_has_external_images_default;
	klass->get_zoom_func = modest_gtkhtml_mime_part_view_get_zoom_default;
	klass->set_zoom_func = modest_gtkhtml_mime_part_view_set_zoom_default;
	klass->zoom_minus_func = modest_gtkhtml_mime_part_view_zoom_minus_default;
	klass->zoom_plus_func = modest_gtkhtml_mime_part_view_zoom_plus_default;
	klass->search_func = modest_gtkhtml_mime_part_view_search_default;
	klass->search_next_func = modest_gtkhtml_mime_part_view_search_next_default;
	klass->get_selection_area_func = modest_gtkhtml_mime_part_view_get_selection_area_default;

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

	g_type_class_add_private (gobject_class, sizeof(ModestGtkhtmlMimePartViewPrivate));

	signals[STOP_STREAMS_SIGNAL] = 
		g_signal_new ("stop-streams",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestGtkhtmlMimePartViewClass,stop_streams),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[LIMIT_ERROR_SIGNAL] = 
		g_signal_new ("limit-error",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestGtkhtmlMimePartViewClass,limit_error),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

}

static void
modest_gtkhtml_mime_part_view_init (ModestGtkhtmlMimePartView *self)
{
	ModestGtkhtmlMimePartViewPrivate *priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (self);
	GdkColor base;
	GdkColor text;

	gtk_html_set_editable        (GTK_HTML(self), FALSE);
	gtk_html_allow_selection     (GTK_HTML(self), TRUE);
	gtk_html_set_caret_mode      (GTK_HTML(self), FALSE);
	gtk_html_set_blocking        (GTK_HTML(self), TRUE);
	gtk_html_set_images_blocking (GTK_HTML(self), TRUE);
	/* We don't need this for Hildon2 as this widget will be most
	   likely inside pannable area */
#ifndef MODEST_TOOLKIT_HILDON2
        gtk_html_set_auto_panning    (GTK_HTML (self), TRUE);
#endif

#ifdef MODEST_TOOLKIT_HILDON2
#ifdef HAVE_GTK_HTML_SET_MAX_IMAGE_SIZE
	/* We set a maximum width of a bit less than the width of the screen, and a
	   maximum height of 2 times the full size of the window. Should be enough */
	gtk_html_set_max_image_size (GTK_HTML (self), 720, 880);
#endif
#ifdef HAVE_GTK_HTML_SET_ALLOW_DND
	gtk_html_set_allow_dnd       (GTK_HTML(self), FALSE);
#endif
#endif

#ifdef HAVE_GTK_HTML_SET_DEFAULT_ENGINE
	/* Enable Content type handling */
	gtk_html_set_default_engine (GTK_HTML (self), TRUE);
#endif

	gdk_color_parse ("#fff", &base);
	gdk_color_parse ("#000", &text);
	gtk_widget_modify_base (GTK_WIDGET (self), GTK_STATE_NORMAL, &base);
	gtk_widget_modify_text (GTK_WIDGET (self), GTK_STATE_NORMAL, &text);

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self), "link_clicked",
						       G_CALLBACK(on_link_clicked), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self), "url_requested",
						       G_CALLBACK(on_url_requested), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self), "on_url",
						       G_CALLBACK(on_url), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self), "notify::style",
						       G_CALLBACK (on_notify_style), (gpointer) self);

	priv->part = NULL;
	priv->current_zoom = 1.0;
	priv->view_images = FALSE;
	priv->has_external_images = FALSE;
}

static void
modest_gtkhtml_mime_part_view_finalize (GObject *obj)
{
	ModestGtkhtmlMimePartViewPrivate *priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (obj);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
modest_gtkhtml_mime_part_view_dispose (GObject *obj)
{
	ModestGtkhtmlMimePartViewPrivate *priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (obj);

	g_signal_emit (G_OBJECT (obj), signals[STOP_STREAMS_SIGNAL], 0);

	if (priv->part) {
		g_object_unref (priv->part);
		priv->part = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

/* GTKHTML SIGNALS HANDLERS */

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
update_style (ModestGtkhtmlMimePartView *self)
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



static gboolean
on_link_clicked (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMimePartView *self)
{
	gboolean result;
	g_return_val_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self), FALSE);

	g_signal_emit_by_name (G_OBJECT (self), "activate-link", uri, &result);
	return result;
}

static gboolean
on_url (GtkWidget *widget, const gchar *uri, ModestGtkhtmlMimePartView *self)
{
	gboolean result;
	g_return_val_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self), FALSE);

	g_signal_emit_by_name (G_OBJECT (self), "link-hover", uri, &result);
	return result;
}

static gboolean
on_url_requested (GtkWidget *widget, const gchar *uri, GtkHTMLStream *stream, 
		  ModestGtkhtmlMimePartView *self)
{
	gboolean result;
	TnyStream *tny_stream;
	g_return_val_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self), FALSE);

	if (g_str_has_prefix (uri, "http:")) {
		ModestGtkhtmlMimePartViewPrivate *priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (self);

		if (!priv->view_images)
			priv->has_external_images = TRUE;
	}

	tny_stream = TNY_STREAM (modest_tny_stream_gtkhtml_new (stream, GTK_HTML (widget)));
	g_signal_emit_by_name (MODEST_MIME_PART_VIEW (self), "fetch-url", uri, tny_stream, &result);
	g_object_unref (tny_stream);
	return result;
}

/* INTERNAL API */
static void
decode_to_stream_cb (TnyMimePart *self,
		     gboolean cancelled,
		     TnyStream *stream,
		     GError *err,
		     gpointer user_data)
{
	ModestGtkhtmlMimePartView *view = (ModestGtkhtmlMimePartView *) user_data;

	if (MODEST_IS_STREAM_TEXT_TO_HTML (stream)) {
		if (tny_stream_write (stream, "\n", 1) == -1) {
			g_warning ("failed to write CR in %s", __FUNCTION__);
		}
		if (modest_stream_text_to_html_limit_reached (MODEST_STREAM_TEXT_TO_HTML (stream))) {
			g_signal_emit (G_OBJECT (view), signals[LIMIT_ERROR_SIGNAL], 0);
		}
		tny_stream_reset (stream);
	} else {
		if (modest_tny_stream_gtkhtml_limit_reached (MODEST_TNY_STREAM_GTKHTML (stream))) {
			g_signal_emit (G_OBJECT (view), signals[LIMIT_ERROR_SIGNAL], 0);
		}
	}
	tny_stream_close (stream);
}

static void
set_html_part (ModestGtkhtmlMimePartView *self, TnyMimePart *part, const gchar *encoding)
{
	GtkHTMLStream *gtkhtml_stream;
	TnyStream *tny_stream;
	gchar *content_type;

	g_return_if_fail (self);
	g_return_if_fail (part);

	g_signal_emit (G_OBJECT (self), signals[STOP_STREAMS_SIGNAL], 0);

	content_type = g_strdup_printf ("text/html; charset=%s", encoding);
	gtkhtml_stream = gtk_html_begin_full(GTK_HTML(self), NULL, content_type, 0);
	g_free (content_type);

	tny_stream     = TNY_STREAM(modest_tny_stream_gtkhtml_new (gtkhtml_stream, GTK_HTML (self)));
	modest_tny_stream_gtkhtml_set_max_size (MODEST_TNY_STREAM_GTKHTML (tny_stream), 128*1024);
	tny_stream_reset (tny_stream);

	tny_mime_part_decode_to_stream_async (TNY_MIME_PART (part),
					      tny_stream, decode_to_stream_cb,
					      NULL, self);
	g_object_unref (tny_stream);
}

static void
set_text_part (ModestGtkhtmlMimePartView *self, TnyMimePart *part)
{
	TnyStream* text_to_html_stream, *tny_stream;
	GtkHTMLStream *gtkhtml_stream;

	g_return_if_fail (self);
	g_return_if_fail (part);

	g_signal_emit (G_OBJECT (self), signals[STOP_STREAMS_SIGNAL], 0);

	gtkhtml_stream = gtk_html_begin(GTK_HTML(self));
	tny_stream =  TNY_STREAM(modest_tny_stream_gtkhtml_new (gtkhtml_stream, GTK_HTML (self)));
	modest_tny_stream_gtkhtml_set_max_size (MODEST_TNY_STREAM_GTKHTML (tny_stream), 128*1024);
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
set_empty_part (ModestGtkhtmlMimePartView *self)
{
	g_return_if_fail (self);

	g_signal_emit (G_OBJECT (self), signals[STOP_STREAMS_SIGNAL], 0);
	gtk_html_load_from_string (GTK_HTML(self), "", 1);
}

static void
set_part (ModestGtkhtmlMimePartView *self, TnyMimePart *part)
{
	ModestGtkhtmlMimePartViewPrivate *priv;
	gchar *header_content_type, *header_content_type_lower;
	const gchar *tmp;
	gchar *charset = NULL;

	g_return_if_fail (self);
	
	priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE(self);
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
get_part (ModestGtkhtmlMimePartView *self)
{
	TnyMimePart *part;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self), NULL);

	part = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE(self)->part;

	if (part)
		g_object_ref (part);
	
	return part;
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

static gboolean      
is_empty   (ModestGtkhtmlMimePartView *self)
{
	/* TODO: Find some gtkhtml API to check whether there is any (visible, non markup)
	 * text in the message:
	 */
	gboolean has_contents = FALSE;

	gtk_html_export (GTK_HTML (self), "text/plain", 
			 (GtkHTMLSaveReceiverFn) has_contents_receiver, &has_contents);
	
	return !has_contents;
}

static gboolean      
get_view_images   (ModestGtkhtmlMimePartView *self)
{
	ModestGtkhtmlMimePartViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self), FALSE);

	priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (self);
	return priv->view_images;
}

static void
set_view_images   (ModestGtkhtmlMimePartView *self, gboolean view_images)
{
	ModestGtkhtmlMimePartViewPrivate *priv;

	g_return_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self));

	priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (self);
	priv->view_images = view_images;
}

static gboolean      
has_external_images   (ModestGtkhtmlMimePartView *self)
{
	ModestGtkhtmlMimePartViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self), FALSE);

	priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (self);
	return priv->has_external_images;
}

static void
set_zoom (ModestGtkhtmlMimePartView *self, gdouble zoom)
{
	ModestGtkhtmlMimePartViewPrivate *priv;

	g_return_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self));

	priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (self);
	priv->current_zoom = zoom;
	gtk_html_set_magnification (GTK_HTML(self), zoom);

	gtk_widget_queue_resize (GTK_WIDGET (self));
}

static gdouble
get_zoom (ModestGtkhtmlMimePartView *self)
{
	ModestGtkhtmlMimePartViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GTKHTML_MIME_PART_VIEW (self), 1.0);

	priv = MODEST_GTKHTML_MIME_PART_VIEW_GET_PRIVATE (self);

	return priv->current_zoom;
}

static gboolean
search (ModestGtkhtmlMimePartView *self, 
	const gchar *string)
{
	return gtk_html_engine_search (GTK_HTML (self),
				       string,
				       FALSE,   /* case sensitive */
				       TRUE,    /* forward */
				       FALSE);  /* regexp */
}

static gboolean
search_next (ModestGtkhtmlMimePartView *self)
{
	return gtk_html_engine_search_next (GTK_HTML (self));
}

static gboolean
get_selection_area (ModestGtkhtmlMimePartView *self, 
		    gint *x, gint *y,
		    gint *width, gint *height)
{
#ifdef HAVE_GTK_HTML_GET_SELECTION_AREA
	gtk_html_get_selection_area (GTK_HTML (self), x, y, width, height);
	return TRUE;
#else
	return FALSE;
#endif
}


/* TNY MIME PART IMPLEMENTATION */

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part = modest_gtkhtml_mime_part_view_get_part;
	klass->set_part = modest_gtkhtml_mime_part_view_set_part;
	klass->clear = modest_gtkhtml_mime_part_view_clear;

	return;
}

static TnyMimePart* 
modest_gtkhtml_mime_part_view_get_part (TnyMimePartView *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->get_part_func (self);
}


static TnyMimePart* 
modest_gtkhtml_mime_part_view_get_part_default (TnyMimePartView *self)
{
	return TNY_MIME_PART (get_part (MODEST_GTKHTML_MIME_PART_VIEW (self)));
}

static void
modest_gtkhtml_mime_part_view_set_part (TnyMimePartView *self,
					TnyMimePart *part)
{
	MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->set_part_func (self, part);
}

static void
modest_gtkhtml_mime_part_view_set_part_default (TnyMimePartView *self,
						TnyMimePart *part)
{
	g_return_if_fail ((part == NULL) || TNY_IS_MIME_PART (part));

	set_part (MODEST_GTKHTML_MIME_PART_VIEW (self), part);
}

static void
modest_gtkhtml_mime_part_view_clear (TnyMimePartView *self)
{
	MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->clear_func (self);
}

static void
modest_gtkhtml_mime_part_view_clear_default (TnyMimePartView *self)
{
	set_part (MODEST_GTKHTML_MIME_PART_VIEW (self), NULL);
}

/* MODEST MIME PART VIEW IMPLEMENTATION */

static void
modest_mime_part_view_init (gpointer g, gpointer iface_data)
{
	ModestMimePartViewIface *klass = (ModestMimePartViewIface *)g;

	klass->is_empty_func = modest_gtkhtml_mime_part_view_is_empty;
	klass->get_view_images_func = modest_gtkhtml_mime_part_view_get_view_images;
	klass->set_view_images_func = modest_gtkhtml_mime_part_view_set_view_images;
	klass->has_external_images_func = modest_gtkhtml_mime_part_view_has_external_images;

	return;
}

static gboolean
modest_gtkhtml_mime_part_view_is_empty (ModestMimePartView *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->is_empty_func (self);
}

static gboolean
modest_gtkhtml_mime_part_view_get_view_images (ModestMimePartView *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->get_view_images_func (self);
}

static void
modest_gtkhtml_mime_part_view_set_view_images (ModestMimePartView *self, gboolean view_images)
{
	MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->set_view_images_func (self, view_images);
}

static gboolean
modest_gtkhtml_mime_part_view_has_external_images (ModestMimePartView *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->has_external_images_func (self);
}

static gboolean
modest_gtkhtml_mime_part_view_is_empty_default (ModestMimePartView *self)
{
	return is_empty (MODEST_GTKHTML_MIME_PART_VIEW (self));
}

static gboolean
modest_gtkhtml_mime_part_view_get_view_images_default (ModestMimePartView *self)
{
	return get_view_images (MODEST_GTKHTML_MIME_PART_VIEW (self));
}

static void
modest_gtkhtml_mime_part_view_set_view_images_default (ModestMimePartView *self, gboolean view_images)
{
	set_view_images (MODEST_GTKHTML_MIME_PART_VIEW (self), view_images);
}

static gboolean
modest_gtkhtml_mime_part_view_has_external_images_default (ModestMimePartView *self)
{
	return has_external_images (MODEST_GTKHTML_MIME_PART_VIEW (self));
}


/* MODEST ZOOMABLE IMPLEMENTATION */
static void
modest_zoomable_init (gpointer g, gpointer iface_data)
{
	ModestZoomableIface *klass = (ModestZoomableIface *)g;
	
	klass->get_zoom_func = modest_gtkhtml_mime_part_view_get_zoom;
	klass->set_zoom_func = modest_gtkhtml_mime_part_view_set_zoom;
	klass->zoom_minus_func = modest_gtkhtml_mime_part_view_zoom_minus;
	klass->zoom_plus_func = modest_gtkhtml_mime_part_view_zoom_plus;

	return;
}

static gdouble
modest_gtkhtml_mime_part_view_get_zoom (ModestZoomable *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->get_zoom_func (self);
}

static gdouble
modest_gtkhtml_mime_part_view_get_zoom_default (ModestZoomable *self)
{
	return get_zoom (MODEST_GTKHTML_MIME_PART_VIEW (self));
}

static void
modest_gtkhtml_mime_part_view_set_zoom (ModestZoomable *self, gdouble value)
{
	MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->set_zoom_func (self, value);
}

static void
modest_gtkhtml_mime_part_view_set_zoom_default (ModestZoomable *self, gdouble value)
{
	set_zoom (MODEST_GTKHTML_MIME_PART_VIEW (self), value);
}

static gboolean
modest_gtkhtml_mime_part_view_zoom_minus (ModestZoomable *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->zoom_minus_func (self);
}

static gboolean
modest_gtkhtml_mime_part_view_zoom_minus_default (ModestZoomable *self)
{
	/* operation not supported in ModestGtkhtmlMimePartView */
	return FALSE;
}

static gboolean
modest_gtkhtml_mime_part_view_zoom_plus (ModestZoomable *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->zoom_plus_func (self);
}

static gboolean
modest_gtkhtml_mime_part_view_zoom_plus_default (ModestZoomable *self)
{
	/* operation not supported in ModestGtkhtmlMimePartView */
	return FALSE;
}

/* ISEARCH VIEW IMPLEMENTATION */
static void
modest_isearch_view_init (gpointer g, gpointer iface_data)
{
	ModestISearchViewIface *klass = (ModestISearchViewIface *)g;
	
	klass->search_func = modest_gtkhtml_mime_part_view_search;
	klass->search_next_func = modest_gtkhtml_mime_part_view_search_next;
	klass->get_selection_area_func = modest_gtkhtml_mime_part_view_get_selection_area;

	return;
}

static gboolean 
modest_gtkhtml_mime_part_view_search (ModestISearchView *self, const gchar *string)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->search_func (self, string);
}

static gboolean 
modest_gtkhtml_mime_part_view_search_default (ModestISearchView *self, const gchar *string)
{
	return search (MODEST_GTKHTML_MIME_PART_VIEW (self), string);
}

static gboolean 
modest_gtkhtml_mime_part_view_search_next(ModestISearchView *self)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->search_next_func (self);
}

static gboolean 
modest_gtkhtml_mime_part_view_search_next_default (ModestISearchView *self)
{
	return search_next (MODEST_GTKHTML_MIME_PART_VIEW (self));
}

static gboolean 
modest_gtkhtml_mime_part_view_get_selection_area (ModestISearchView *self, gint *x, gint *y, 
						  gint *width, gint *height)
{
	return MODEST_GTKHTML_MIME_PART_VIEW_GET_CLASS (self)->get_selection_area_func (self, x, y, width, height);
}

static gboolean 
modest_gtkhtml_mime_part_view_get_selection_area_default (ModestISearchView *self, gint *x, gint *y, 
							  gint *width, gint *height)
{
	return get_selection_area (MODEST_GTKHTML_MIME_PART_VIEW (self), x, y, width, height);
}

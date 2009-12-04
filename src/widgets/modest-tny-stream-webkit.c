/* Copyright (c) 2009, Nokia Corporation
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


/* modest-tny-stream-webkit.c */

#include "modest-tny-stream-webkit.h"
#include "modest-webkit-mime-part-view.h"
#include <tny-stream.h>

/* 'private'/'protected' functions */
static void  modest_tny_stream_webkit_class_init   (ModestTnyStreamWebkitClass *klass);
static void  modest_tny_stream_webkit_init         (ModestTnyStreamWebkit *obj);
static void  modest_tny_stream_webkit_finalize     (GObject *obj);

static void  modest_tny_stream_gtkhml_iface_init (gpointer g_iface, gpointer iface_data);

static void  stop_streams (ModestWebkitMimePartView *view, gpointer userdata);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyStreamWebkitPrivate ModestTnyStreamWebkitPrivate;
struct _ModestTnyStreamWebkitPrivate {
	GString *buffer;
	GtkWidget *webview;

	gchar *mime_type;
	gchar *encoding;

	guint stop_streams_id;

	gssize max_size;
	gssize current_size;
};
#define MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                       MODEST_TYPE_TNY_STREAM_WEBKIT, \
                                                       ModestTnyStreamWebkitPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_stream_webkit_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyStreamWebkitClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_stream_webkit_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyStreamWebkit),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_stream_webkit_init,
			NULL
		};

		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) modest_tny_stream_gtkhml_iface_init,
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
                };

		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestTnyStreamWebkit",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_STREAM,
					     &iface_info);

	}
	return my_type;
}

static void
modest_tny_stream_webkit_class_init (ModestTnyStreamWebkitClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_stream_webkit_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyStreamWebkitPrivate));
}

static void
modest_tny_stream_webkit_init (ModestTnyStreamWebkit *obj)
{
	ModestTnyStreamWebkitPrivate *priv;
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE(obj);

	priv->buffer  = NULL;
	priv->webview = NULL;
	priv->stop_streams_id = 0;

	priv->max_size = 0;
	priv->current_size = 0;

	priv->mime_type = NULL;
	priv->encoding = NULL;
}

static void
modest_tny_stream_webkit_finalize (GObject *obj)
{
	ModestTnyStreamWebkitPrivate *priv;

	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE(obj);
	if (priv->buffer)
		g_string_free (priv->buffer, TRUE);

	g_free (priv->mime_type);
	g_free (priv->encoding);

	if (priv->stop_streams_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->webview), priv->stop_streams_id);
		priv->stop_streams_id = 0;
	}

	if (priv->webview) {
		g_object_unref (priv->webview);
		priv->webview = NULL;
	}
}

GObject*
modest_tny_stream_webkit_new (WebKitWebView *webview, const gchar *mime_type, const gchar *encoding)
{
	GObject *obj;
	ModestTnyStreamWebkitPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_STREAM_WEBKIT, NULL));
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE(obj);

	g_return_val_if_fail (webview, NULL);

	priv->buffer = g_string_new ("");
	
	priv->webview = g_object_ref (webview);
	priv->mime_type = g_strdup (mime_type);
	priv->encoding = g_strdup (encoding);

	priv->stop_streams_id = g_signal_connect (G_OBJECT (webview), "stop-streams",
						  G_CALLBACK (stop_streams), obj);
	priv->current_size = 0;

	return obj;
}


/* the rest are interface functions */


static ssize_t
webkit_read (TnyStream *self, char *buffer, size_t n)
{
	return -1; /* we cannot read */
}


static ssize_t
webkit_write (TnyStream *self, const char *buffer, size_t n)
{
	ModestTnyStreamWebkitPrivate *priv;
	
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE(self);

	if (!priv->buffer) {
		g_print ("modest: cannot write to closed stream\n");
		return 0;
	}

	if (n == 0 || !buffer)
		return 0;

	if (!priv->webview || !GTK_WIDGET_VISIBLE (priv->webview))
		return -1;

	if (priv->max_size > 0) {

		/* We only use the maximum size for write method, and even we
		 * ignore and fake as we would do a successfull read */
		if (priv->current_size >= priv->max_size)
			return n;

		if (priv->current_size + n > priv->max_size)
			n = priv->max_size - priv->current_size;
	}

	if (!g_main_context_is_owner (NULL))
		gdk_threads_enter ();

	priv->buffer = g_string_append_len (priv->buffer, buffer, n);

	if (!g_main_context_is_owner (NULL))
		gdk_threads_leave ();
	priv->current_size += n;

	return n; /* hmmm */
}

	
static gint
webkit_flush (TnyStream *self)
{
	return 0;
}
	

static gint
webkit_close (TnyStream *self)
{
	ModestTnyStreamWebkitPrivate *priv;
	g_return_val_if_fail (self, 0);
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE(self);
	
	if (priv->stop_streams_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->webview), priv->stop_streams_id);
		priv->stop_streams_id = 0;
	}
	if (priv->webview) {
		webkit_web_view_load_string (WEBKIT_WEB_VIEW (priv->webview), priv->buffer->str, priv->mime_type, priv->encoding, NULL);

		g_object_unref (priv->webview);
		priv->webview = NULL;
	}
	if (priv->buffer)
		g_string_free (priv->buffer, TRUE);
	priv->buffer = NULL;

	return 0;
}


static gboolean
webkit_is_eos (TnyStream *self)
{
	return TRUE;
}


	
static gint
webkit_reset (TnyStream *self)
{
	return 0;
}

	
static ssize_t
webkit_write_to_stream (TnyStream *self, TnyStream *output)
{
	return 0;
}

static void
stop_streams (ModestWebkitMimePartView *view, gpointer userdata)
{
	ModestTnyStreamWebkit *self = (ModestTnyStreamWebkit *) userdata;
	ModestTnyStreamWebkitPrivate *priv;
	
	g_return_if_fail (self);
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE(self);

	if (priv->webview && priv->stop_streams_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->webview), priv->stop_streams_id);
		priv->stop_streams_id = 0;
	}
	
	if (priv->webview) {
		g_object_unref (priv->webview);
		priv->webview = NULL;
	}
}

void 
modest_tny_stream_webkit_set_max_size (ModestTnyStreamWebkit *stream, 
					gssize max_size)
{
	ModestTnyStreamWebkitPrivate *priv;

	g_return_if_fail (MODEST_IS_TNY_STREAM_WEBKIT (stream));
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE (stream);

	priv->max_size = max_size;
}

gssize 
modest_tny_stream_webkit_get_max_size (ModestTnyStreamWebkit *stream)
{
	ModestTnyStreamWebkitPrivate *priv;

	g_return_val_if_fail (MODEST_IS_TNY_STREAM_WEBKIT (stream), 0);
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE (stream);

	return priv->max_size;
}

gboolean
modest_tny_stream_webkit_limit_reached (ModestTnyStreamWebkit *self)
{
	ModestTnyStreamWebkitPrivate *priv;

	g_return_val_if_fail (MODEST_IS_TNY_STREAM_WEBKIT (self), 0);
	priv = MODEST_TNY_STREAM_WEBKIT_GET_PRIVATE (self);

	return (priv->max_size > 0) && (priv->current_size >= priv->max_size);
}

static void
modest_tny_stream_gtkhml_iface_init (gpointer g_iface, gpointer iface_data)
{
        TnyStreamIface *klass;
	
	g_return_if_fail (g_iface);

	klass = (TnyStreamIface*) g_iface;
	
        klass->read            = webkit_read;
        klass->write           = webkit_write;
        klass->flush           = webkit_flush;
        klass->close           = webkit_close;
	klass->is_eos          = webkit_is_eos;
	klass->reset           = webkit_reset;
	klass->write_to_stream = webkit_write_to_stream;
}

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


/* modest-stream-text-to-html.c */

#include "modest-stream-text-to-html.h"
#include <tny-stream.h>
#include <string.h>
#include <modest-text-utils.h>

#define HTML_PREFIX "<html><head>" \
	"<meta http-equiv=\"content-type\" content=\"text/html; charset=utf8\">" \
	"</head>" \
	"<body>"
#define HTML_SUFFIX "</body></html>"


/* 'private'/'protected' functions */
static void  modest_stream_text_to_html_class_init   (ModestStreamTextToHtmlClass *klass);
static void  modest_stream_text_to_html_init         (ModestStreamTextToHtml *obj);
static void  modest_stream_text_to_html_finalize     (GObject *obj);

static void  modest_stream_text_to_html_iface_init   (gpointer g_iface, gpointer iface_data);
static gboolean write_line (TnyStream *self, const gchar *str, gboolean convert_to_html);


typedef struct _ModestStreamTextToHtmlPrivate ModestStreamTextToHtmlPrivate;
struct _ModestStreamTextToHtmlPrivate {
	TnyStream *out_stream;
	GString *line_buffer;
	gboolean written_prefix;
	gsize linkify_limit;
	gsize full_limit;
	gsize line_limit;
	gsize total_output;
	gsize total_lines_output;
};
#define MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                       MODEST_TYPE_STREAM_TEXT_TO_HTML, \
                                                       ModestStreamTextToHtmlPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_stream_text_to_html_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestStreamTextToHtmlClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_stream_text_to_html_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestStreamTextToHtml),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_stream_text_to_html_init,
			NULL
		};

		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) modest_stream_text_to_html_iface_init,
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
                };

		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestStreamTextToHtml",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_STREAM,
					     &iface_info);

	}
	return my_type;
}

static void
modest_stream_text_to_html_class_init (ModestStreamTextToHtmlClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_stream_text_to_html_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestStreamTextToHtmlPrivate));
}

static void
modest_stream_text_to_html_init (ModestStreamTextToHtml *obj)
{
	ModestStreamTextToHtmlPrivate *priv;
	priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE(obj);

	priv->out_stream  = NULL;
	priv->written_prefix = FALSE;
	priv->line_buffer = NULL;
	priv->linkify_limit = 0;
	priv->full_limit = 0;
	priv->total_output = 0;
	priv->total_lines_output = 0;
	modest_text_utils_hyperlinkify_begin ();
}

static void
modest_stream_text_to_html_finalize (GObject *obj)
{
	ModestStreamTextToHtmlPrivate *priv;

	priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE(obj);
	if (priv->out_stream)
		g_object_unref (priv->out_stream);
	priv->out_stream = NULL;
	if (priv->line_buffer != NULL) {
		g_string_free (priv->line_buffer, TRUE);
	}
	modest_text_utils_hyperlinkify_end ();
}

GObject*
modest_stream_text_to_html_new (TnyStream *out_stream)
{
	GObject *obj;
	ModestStreamTextToHtmlPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_STREAM_TEXT_TO_HTML, NULL));
	priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE(obj);

	g_return_val_if_fail (out_stream, NULL);
	
	priv->out_stream = g_object_ref (out_stream);

	return obj;
}

void        
modest_stream_text_to_html_set_line_limit (ModestStreamTextToHtml *self, gssize limit)
{
	ModestStreamTextToHtmlPrivate *priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE (self);
	priv->line_limit = limit;
}

void        
modest_stream_text_to_html_set_linkify_limit (ModestStreamTextToHtml *self, gssize limit)
{
	ModestStreamTextToHtmlPrivate *priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE (self);
	priv->linkify_limit = limit;
}

void        
modest_stream_text_to_html_set_full_limit (ModestStreamTextToHtml *self, gssize limit)
{
	ModestStreamTextToHtmlPrivate *priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE (self);
	priv->full_limit = limit;
}

gboolean
modest_stream_text_to_html_limit_reached (ModestStreamTextToHtml *self)
{
	ModestStreamTextToHtmlPrivate *priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE (self);

	return (priv->full_limit > 0 && priv->total_output > priv->full_limit) ||
		(priv->line_limit > 0 && priv->total_lines_output > priv->line_limit);
	
}

/* the rest are interface functions */


static ssize_t
text_to_html_read (TnyStream *self, char *buffer, size_t n)
{
	return -1; /* we cannot read */
}

static gboolean 
write_line (TnyStream *self, const gchar *str, gboolean convert_to_html)
{
	ModestStreamTextToHtmlPrivate *priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE (self);
	gchar *html_buffer;
	gchar *offset;
	gssize pending_bytes;
	gboolean hyperlinkify = TRUE;

	/* we only leave for full limit if we're converting to html, so that we
	   preserve the prefix and suffix */
	if (convert_to_html && (priv->full_limit > 0) &&(priv->total_output > priv->full_limit))
		return TRUE;
	if (convert_to_html && (priv->line_limit > 0) && (priv->total_lines_output > priv->line_limit))
		return TRUE;
	if ((priv->linkify_limit > 0) && (priv->total_output > priv->linkify_limit))
		hyperlinkify = FALSE;
	if (convert_to_html) {
		html_buffer = modest_text_utils_convert_to_html_body (str, -1, hyperlinkify);
	} else {
		html_buffer = (gchar *) str;
	}

	pending_bytes = strlen (html_buffer);
	priv->total_output += pending_bytes;
	priv->total_lines_output ++;
	offset = html_buffer;

	while (pending_bytes > 0) {
		gssize written_bytes = 0;
		written_bytes = tny_stream_write (priv->out_stream, offset, pending_bytes);
		if (written_bytes < 0) {
			if (convert_to_html)
				g_free (html_buffer);
			return FALSE;
		}
		offset += written_bytes;
		pending_bytes -= written_bytes;
	}
	if (convert_to_html)
		g_free (html_buffer);

	return TRUE;
}

static ssize_t
text_to_html_write (TnyStream *self, const char *buffer, size_t n)
{
	
	ModestStreamTextToHtmlPrivate *priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE (self);
	gssize total = n;

	modest_text_utils_hyperlinkify_begin ();
	if ((!priv->written_prefix) && (n > 0)) {
		if (!write_line (self, HTML_PREFIX, FALSE)) {
			modest_text_utils_hyperlinkify_end ();
			return -1;
		}
		priv->written_prefix = TRUE;
	}

	while (n > 0) {
		gchar c = *buffer;

		if ((priv->full_limit > 0) && (priv->total_output > priv->full_limit))
			return n;
		if ((priv->line_limit > 0) && (priv->total_lines_output > priv->line_limit))
			return n;

		if (priv->line_buffer == NULL)
			priv->line_buffer = g_string_new (NULL);

		priv->line_buffer = g_string_append_c (priv->line_buffer, c);
		if (c == '\n') {
			if (tny_stream_flush (self) == -1) {
				modest_text_utils_hyperlinkify_end ();
				return -1;
			}
		}
		buffer ++;
		n--;
	}
	modest_text_utils_hyperlinkify_end ();
	return total;
}

	
static gint
text_to_html_flush (TnyStream *self)
{
	ModestStreamTextToHtmlPrivate *priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE (self);

	if (priv->line_buffer != NULL) {
		if (!write_line (self, priv->line_buffer->str, TRUE))
			return -1;
		g_string_free (priv->line_buffer, TRUE);
		priv->line_buffer = NULL;
	}
	return 0;
}
	

static gint
text_to_html_close (TnyStream *self)
{
	ModestStreamTextToHtmlPrivate *priv;
	g_return_val_if_fail (self, 0);
	priv = MODEST_STREAM_TEXT_TO_HTML_GET_PRIVATE(self);

	tny_stream_flush (self);
	if (!write_line (self, HTML_SUFFIX, FALSE))
		return -1;
	
	tny_stream_close (priv->out_stream);
	
	priv->out_stream = NULL;

	return 0;
}


static gboolean
text_to_html_is_eos (TnyStream *self)
{
	return TRUE;
}


	
static gint
text_to_html_reset (TnyStream *self)
{
	return 0;
}

	
static ssize_t
text_to_html_write_to_stream (TnyStream *self, TnyStream *output)
{
	return 0;
}


static void
modest_stream_text_to_html_iface_init (gpointer g_iface, gpointer iface_data)
{
        TnyStreamIface *klass;
	
	g_return_if_fail (g_iface);

	klass = (TnyStreamIface*) g_iface;
	
        klass->read            = text_to_html_read;
        klass->write           = text_to_html_write;
        klass->flush           = text_to_html_flush;
        klass->close           = text_to_html_close;
	klass->is_eos          = text_to_html_is_eos;
	klass->reset           = text_to_html_reset;
	klass->write_to_stream = text_to_html_write_to_stream;
}

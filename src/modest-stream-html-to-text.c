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


/* modest-stream-text-to-html.c */

#include "modest-stream-html-to-text.h"
#include <tny-stream.h>
#include <string.h>
#include <modest-text-utils.h>
#include <gtkhtml/gtkhtml-stream.h>


/* 'private'/'protected' functions */
static void  modest_stream_html_to_text_class_init   (ModestStreamHtmlToTextClass *klass);
static void  modest_stream_html_to_text_init         (ModestStreamHtmlToText *obj);
static void  modest_stream_html_to_text_finalize     (GObject *obj);
static void  modest_stream_html_to_text_iface_init   (gpointer g_iface, gpointer iface_data);

typedef struct _ModestStreamHtmlToTextPrivate ModestStreamHtmlToTextPrivate;
struct _ModestStreamHtmlToTextPrivate {
	GString *buffer;
	gint position;
	GtkHTML *html;
};
#define MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                       MODEST_TYPE_STREAM_HTML_TO_TEXT, \
                                                       ModestStreamHtmlToTextPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_stream_html_to_text_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestStreamHtmlToTextClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_stream_html_to_text_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestStreamHtmlToText),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_stream_html_to_text_init,
			NULL
		};

		static const GInterfaceInfo iface_info = {
			(GInterfaceInitFunc) modest_stream_html_to_text_iface_init,
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
                };

		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestStreamHtmlToText",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_STREAM,
					     &iface_info);

	}
	return my_type;
}

static void
modest_stream_html_to_text_class_init (ModestStreamHtmlToTextClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_stream_html_to_text_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestStreamHtmlToTextPrivate));
}

static void
modest_stream_html_to_text_init (ModestStreamHtmlToText *obj)
{
	ModestStreamHtmlToTextPrivate *priv;
	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(obj);

	priv->position = 0;
	priv->buffer = NULL;
	priv->html = NULL;
}

static void
modest_stream_html_to_text_finalize (GObject *obj)
{
	ModestStreamHtmlToTextPrivate *priv;

	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(obj);

	if (priv->buffer)
		g_string_free (priv->buffer, TRUE);
}

static gboolean
export_to_txt_cb (const HTMLEngine * engine,
		  const char *data,
		  unsigned int len,
		  void *user_data)
{
	ModestStreamHtmlToTextPrivate *priv;

	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(user_data);

	if (!priv->buffer)
		priv->buffer = g_string_new (data);
	else
		g_string_append (priv->buffer, data);

	return TRUE;
}

static gboolean
parse_input_stream (ModestStreamHtmlToText *self,
		    TnyStream *in_stream)
{
	GString *buffer;
	GtkHTMLStream *stream = NULL;
	ModestStreamHtmlToTextPrivate *priv;
	const guint BUFF_SIZE = 4096;
	gchar buff[BUFF_SIZE];

	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(self);

	buffer = g_string_new (NULL);
	while (!tny_stream_is_eos (in_stream)) {
		gint read;
		read = tny_stream_read (in_stream, buff, BUFF_SIZE);
		buffer = g_string_append_len (buffer, buff, read);
	}
	tny_stream_reset (in_stream);

	priv->html = g_object_new (GTK_TYPE_HTML, "visible", FALSE, NULL);
	gtk_html_set_default_engine (priv->html, TRUE);
	stream = gtk_html_begin_full(priv->html, NULL, "text/html", 0);
	gtk_html_write(priv->html, stream, buffer->str, buffer->len);
	gtk_html_end(priv->html, stream, 0);

	return gtk_html_export (priv->html, "text/plain",
				(GtkHTMLSaveReceiverFn) export_to_txt_cb, self);
}

TnyStream *
modest_stream_html_to_text_new (TnyStream *in_stream)
{
	GObject *obj;

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_STREAM_HTML_TO_TEXT, NULL));

	if (!parse_input_stream ((ModestStreamHtmlToText *) obj, in_stream)) {
		g_warning ("%s: error parsing the input stream", __FUNCTION__);
		g_object_unref (obj);
		obj = NULL;
	}

	return (TnyStream *) obj;
}

/* the rest are interface functions */
static ssize_t
html_to_text_read (TnyStream *self, char *buffer, size_t n)
{
	ModestStreamHtmlToTextPrivate *priv;
	gint i = 0;

	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE (self);

	if (priv->buffer) {
		for (i = 0; (i < n) && ((priv->position + i) < priv->buffer->len) ; i++)
			buffer[i] = priv->buffer->str[priv->position + i];
	}

	priv->position += i;

	return i;
}

static ssize_t
html_to_text_write (TnyStream *self, const char *buffer, size_t n)
{
	return -1;  /* we cannot write */
}

static gint
html_to_text_flush (TnyStream *self)
{
	/* ModestStreamHtmlToTextPrivate *priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE (self); */

	return 0;
}


static gint
html_to_text_close (TnyStream *self)
{
	ModestStreamHtmlToTextPrivate *priv;

	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(self);

	tny_stream_flush (self);

	return 0;
}


static gboolean
html_to_text_is_eos (TnyStream *self)
{
	ModestStreamHtmlToTextPrivate *priv;

	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(self);

	/* This could happen if the body is empty */
	if (!priv->buffer)
		return TRUE;
	else
		return (priv->position >= (priv->buffer->len - 1));
}



static gint
html_to_text_reset (TnyStream *self)
{
	ModestStreamHtmlToTextPrivate *priv;

	priv = MODEST_STREAM_HTML_TO_TEXT_GET_PRIVATE(self);
	priv->position = 0;

	return priv->position;
}


static ssize_t
html_to_text_write_to_stream (TnyStream *self, TnyStream *output)
{
	return 0;
}


static void
modest_stream_html_to_text_iface_init (gpointer g_iface, gpointer iface_data)
{
        TnyStreamIface *klass;

	g_return_if_fail (g_iface);

	klass = (TnyStreamIface*) g_iface;

        klass->read            = html_to_text_read;
        klass->write           = html_to_text_write;
        klass->flush           = html_to_text_flush;
        klass->close           = html_to_text_close;
	klass->is_eos          = html_to_text_is_eos;
	klass->reset           = html_to_text_reset;
	klass->write_to_stream = html_to_text_write_to_stream;
}

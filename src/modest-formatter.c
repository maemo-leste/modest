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

#include <glib/gi18n.h>
#include <string.h>
#include <tny-header.h>
#include <tny-gtk-text-buffer-stream.h>
#include <tny-camel-stream.h>
#include <camel/camel-stream-mem.h>
#include "modest-formatter.h"
#include "modest-text-utils.h"
#include "modest-tny-platform-factory.h"

typedef struct _ModestFormatterPrivate ModestFormatterPrivate;
struct _ModestFormatterPrivate {
	gchar *content_type;
};
#define MODEST_FORMATTER_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                          MODEST_TYPE_FORMATTER, \
                                          ModestFormatterPrivate))

static GObjectClass *parent_class = NULL;

typedef gchar* FormatterFunc (ModestFormatter *self, const gchar *text, TnyHeader *header);

static TnyMsg *modest_formatter_do (ModestFormatter *self, TnyMimePart *body,  TnyHeader *header, 
				    FormatterFunc func);

static gchar*  modest_formatter_wrapper_cite   (ModestFormatter *self, const gchar *text, TnyHeader *header);
static gchar*  modest_formatter_wrapper_quote  (ModestFormatter *self, const gchar *text, TnyHeader *header);
static gchar*  modest_formatter_wrapper_inline (ModestFormatter *self, const gchar *text, TnyHeader *header);

static gchar *
extract_text (ModestFormatter *self, TnyMimePart *body)
{
	TnyStream *stream;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	gchar *text, *converted_text;
	ModestFormatterPrivate *priv;

	buf = gtk_text_buffer_new (NULL);
	stream = TNY_STREAM (tny_gtk_text_buffer_stream_new (buf));
	tny_stream_reset (stream);
	tny_mime_part_decode_to_stream (body, stream);
	tny_stream_reset (stream);

	g_object_unref (G_OBJECT(stream));
	
	gtk_text_buffer_get_bounds (buf, &start, &end);
	text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	g_object_unref (G_OBJECT(buf));

	/* Convert to desired content type if needed */
	priv = MODEST_FORMATTER_GET_PRIVATE (self);

	if (strcmp (tny_mime_part_get_content_type (body), priv->content_type) == 0) {
		if (!strcmp (priv->content_type, "text/html"))
			converted_text = modest_text_utils_convert_to_html  (text);
		else
			converted_text = g_strdup (text);

		g_free (text);
		text = converted_text;
	}
	return text;
}

static void
construct_from_text (TnyMimePart *part,
		     const gchar *text,
		     const gchar *content_type)
{
	TnyStream *text_body_stream;

	/* Create the stream */
	text_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(text, strlen(text))));

	/* Construct MIME part */
	tny_stream_reset (text_body_stream);
	tny_mime_part_construct_from_stream (part, text_body_stream, content_type);
	tny_stream_reset (text_body_stream);

	/* Clean */
	g_object_unref (G_OBJECT (text_body_stream));
}

static TnyMsg *
modest_formatter_do (ModestFormatter *self, TnyMimePart *body, TnyHeader *header, FormatterFunc func)
{
	TnyMsg *new_msg = NULL;
	gchar *body_text = NULL, *txt = NULL;
	ModestFormatterPrivate *priv;
	TnyPlatformFactory *fact;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (header, NULL);
	g_return_val_if_fail (func, NULL);

	/* Build new part */
	fact = modest_tny_platform_factory_get_instance ();
	new_msg = tny_platform_factory_new_msg (fact);

	if (body)
		body_text = extract_text (self, body);
	else
		body_text = g_strdup ("");

	txt = (gchar *) func (self, (const gchar*) body_text, header);
	priv = MODEST_FORMATTER_GET_PRIVATE (self);
	construct_from_text (TNY_MIME_PART (new_msg), (const gchar*) txt, priv->content_type);

	/* Clean */
	g_free (body_text);
	g_free (txt);

	return new_msg;
}

TnyMsg *
modest_formatter_cite (ModestFormatter *self, TnyMimePart *body, TnyHeader *header)
{
	return modest_formatter_do (self, body, header, modest_formatter_wrapper_cite);
}

TnyMsg *
modest_formatter_quote (ModestFormatter *self, TnyMimePart *body, TnyHeader *header)
{
	return modest_formatter_do (self, body, header, modest_formatter_wrapper_quote);
}

TnyMsg *
modest_formatter_inline (ModestFormatter *self, TnyMimePart *body, TnyHeader *header)
{
	return modest_formatter_do (self, body, header, modest_formatter_wrapper_inline);
}

TnyMsg *
modest_formatter_attach (ModestFormatter *self, TnyMimePart *body, TnyHeader *header)
{
	TnyMsg *new_msg = NULL;
	gchar *attach_text = NULL;
	const gchar *subject;
	TnyMimePart *body_part = NULL, *attach_part = NULL;
	ModestFormatterPrivate *priv;
	TnyPlatformFactory *fact;

	fact = modest_tny_platform_factory_get_instance ();
	/* Build new part */
	new_msg     = tny_platform_factory_new_msg (fact);
	body_part   = tny_platform_factory_new_mime_part (fact);
	attach_part = tny_platform_factory_new_mime_part (fact);

	/* Create the two parts */
	priv = MODEST_FORMATTER_GET_PRIVATE (self);
	attach_text = extract_text (self, body);
	construct_from_text (body_part, "", priv->content_type);
	construct_from_text (attach_part, (const gchar*) attach_text, priv->content_type);
	subject = tny_header_get_subject (header);
	tny_mime_part_set_filename (attach_part, subject ? subject : _("No subject"));

	/* Add parts */
	tny_mime_part_add_part (TNY_MIME_PART (new_msg), body_part);
	tny_mime_part_add_part (TNY_MIME_PART (new_msg), attach_part);

	/* Clean */
	g_free (attach_text);

	return new_msg;
}

ModestFormatter*
modest_formatter_new (const gchar *content_type)
{
	ModestFormatter *formatter;
	ModestFormatterPrivate *priv;

	formatter = g_object_new (MODEST_TYPE_FORMATTER, NULL);
	priv = MODEST_FORMATTER_GET_PRIVATE (formatter);
	priv->content_type = g_strdup (content_type);

	return formatter;
}

static void
modest_formatter_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestFormatter *self = (ModestFormatter *)instance;
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	priv->content_type = NULL;
}

static void
modest_formatter_finalize (GObject *object)
{
	ModestFormatter *self = (ModestFormatter *)object;
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	if (priv->content_type)
		g_free (priv->content_type);

	(*parent_class->finalize) (object);
}

static void 
modest_formatter_class_init (ModestFormatterClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = modest_formatter_finalize;

	g_type_class_add_private (object_class, sizeof (ModestFormatterPrivate));
}

GType 
modest_formatter_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestFormatterClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_formatter_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestFormatter),
		  0,      /* n_preallocs */
		  modest_formatter_instance_init    /* instance_init */
		};
	        
		type = g_type_register_static (G_TYPE_OBJECT,
			"ModestFormatter",
			&info, 0);
	}

	return type;
}

/****************/
static gchar *
modest_formatter_wrapper_cite (ModestFormatter *self, const gchar *text, TnyHeader *header) 
{
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	return modest_text_utils_cite (text, 
				       priv->content_type, 
				       tny_header_get_from (header), 
				       tny_header_get_date_sent (header));
}

static gchar *
modest_formatter_wrapper_inline (ModestFormatter *self, const gchar *text, TnyHeader *header) 
{
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	return modest_text_utils_inline (text, 
					 priv->content_type, 
					 tny_header_get_from (header), 
					 tny_header_get_date_sent (header),
					 tny_header_get_to (header),
					 tny_header_get_subject (header));
}

static gchar *
modest_formatter_wrapper_quote (ModestFormatter *self, const gchar *text, TnyHeader *header) 
{
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	/* TODO: get 80 from the configuration */
	return modest_text_utils_quote (text, 
					priv->content_type, 
					tny_header_get_from (header), 
					tny_header_get_date_sent (header),
					80);
}

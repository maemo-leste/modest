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
#include <tny-simple-list.h>
#include <tny-gtk-text-buffer-stream.h>
#include <tny-camel-mem-stream.h>
#include <tny-camel-html-to-text-stream.h>
#include "modest-formatter.h"
#include "modest-text-utils.h"
#include "modest-tny-platform-factory.h"
#include <modest-runtime.h>

#define LINE_WRAP 78
#define MAX_BODY_LINES 1024
#define MAX_BODY_LENGTH 1024*128

typedef struct _ModestFormatterPrivate ModestFormatterPrivate;
struct _ModestFormatterPrivate {
	gchar *content_type;
	gchar *signature;
};
#define MODEST_FORMATTER_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                          MODEST_TYPE_FORMATTER, \
                                          ModestFormatterPrivate))

static GObjectClass *parent_class = NULL;

typedef gchar* FormatterFunc (ModestFormatter *self, const gchar *text, TnyHeader *header, GList *attachments);

static TnyMsg *modest_formatter_do (ModestFormatter *self, TnyMimePart *body,  TnyHeader *header, 
				    FormatterFunc func, GList *attachments);

static gchar*  modest_formatter_wrapper_cite   (ModestFormatter *self, const gchar *text,
						TnyHeader *header, GList *attachments);
static gchar*  modest_formatter_wrapper_quote  (ModestFormatter *self, const gchar *text,
						TnyHeader *header, GList *attachments);
static gchar*  modest_formatter_wrapper_inline (ModestFormatter *self, const gchar *text,
						TnyHeader *header, GList *attachments);

static TnyMimePart *find_body_parent (TnyMimePart *part);

static gchar *
extract_text (ModestFormatter *self, TnyMimePart *body)
{
	TnyStream *mp_stream;
	TnyStream *stream;
	TnyStream *input_stream;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	gchar *text;
	ModestFormatterPrivate *priv;
	gint total, total_lines, line_chars;

	buf = gtk_text_buffer_new (NULL);
	stream = TNY_STREAM (tny_gtk_text_buffer_stream_new (buf));
	tny_stream_reset (stream);
	mp_stream = tny_mime_part_get_decoded_stream (body);

	if (g_strcmp0 (tny_mime_part_get_content_type (body), "text/html") == 0) {
		input_stream = tny_camel_html_to_text_stream_new (mp_stream);
	} else {
		input_stream = g_object_ref (mp_stream);
	}

	total = 0;
	total_lines = 0;
	line_chars = 0;

	while (!tny_stream_is_eos (input_stream)) {
		gchar buffer [128];
		gchar *offset;
		gint n_read;
		gint next_read;

		next_read = MIN (128, MAX_BODY_LENGTH - total);
		if (next_read == 0)
			break;
		n_read = tny_stream_read (input_stream, buffer, next_read);

		if (G_UNLIKELY (n_read < 0))
			break;

		offset = buffer;
		while (offset < buffer + n_read) {
			
			if (*offset == '\n') {
				total_lines ++;
				line_chars = 0;
			} else {
				line_chars ++;
				if (line_chars >= LINE_WRAP) {
					total_lines ++;
					line_chars = 0;
				}
			}
			if (total_lines >= MAX_BODY_LINES)
				break;
			offset++;
		}

		

		if (offset - buffer > 0) {
			gint n_write;
			n_write = tny_stream_write (stream, buffer, offset - buffer);
			total += n_write;
		} else if (n_read == -1) {
			break;
		}

		if (total_lines >= MAX_BODY_LINES)
			break;
	}

	tny_stream_reset (stream);

	g_object_unref (G_OBJECT(stream));
	g_object_unref (G_OBJECT (mp_stream));
	g_object_unref (G_OBJECT (input_stream));
	
	gtk_text_buffer_get_bounds (buf, &start, &end);
	text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	g_object_unref (G_OBJECT(buf));

	/* Convert to desired content type if needed */
	priv = MODEST_FORMATTER_GET_PRIVATE (self);

	return text;
}

static void
construct_from_text (TnyMimePart *part,
		     const gchar *text,
		     const gchar *content_type)
{
	TnyStream *text_body_stream;

	/* Create the stream */
	text_body_stream = TNY_STREAM (tny_camel_mem_stream_new_with_buffer
					(text, strlen(text)));

	/* Construct MIME part */
	tny_stream_reset (text_body_stream);
	tny_mime_part_construct (part, text_body_stream, content_type, "7bit");
	tny_stream_reset (text_body_stream);

	/* Clean */
	g_object_unref (G_OBJECT (text_body_stream));
}

static TnyMsg *
modest_formatter_do (ModestFormatter *self, TnyMimePart *body, TnyHeader *header, FormatterFunc func,
		     GList *attachments)
{
	TnyMsg *new_msg = NULL;
	gchar *body_text = NULL, *txt = NULL;
	ModestFormatterPrivate *priv;
	TnyMimePart *body_part = NULL;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (header, NULL);
	g_return_val_if_fail (func, NULL);

	/* Build new part */
	new_msg = modest_formatter_create_message (self, TRUE, attachments != NULL, FALSE);
	body_part = modest_formatter_create_body_part (self, new_msg);

	if (body)
		body_text = extract_text (self, body);
	else
		body_text = g_strdup ("");

	txt = (gchar *) func (self, (const gchar*) body_text, header, attachments);
	priv = MODEST_FORMATTER_GET_PRIVATE (self);
	construct_from_text (TNY_MIME_PART (body_part), (const gchar*) txt, priv->content_type);
	g_object_unref (body_part);
	
	/* Clean */
	g_free (body_text);
	g_free (txt);

	return new_msg;
}

TnyMsg *
modest_formatter_cite (ModestFormatter *self, TnyMimePart *body, TnyHeader *header)
{
	return modest_formatter_do (self, body, header, modest_formatter_wrapper_cite, NULL);
}

TnyMsg *
modest_formatter_quote (ModestFormatter *self, TnyMimePart *body, TnyHeader *header, GList *attachments)
{
	return modest_formatter_do (self, body, header, modest_formatter_wrapper_quote, attachments);
}

TnyMsg *
modest_formatter_inline (ModestFormatter *self, TnyMimePart *body, TnyHeader *header, GList *attachments)
{
	return modest_formatter_do (self, body, header, modest_formatter_wrapper_inline, attachments);
}

TnyMsg *
modest_formatter_attach (ModestFormatter *self, TnyMsg *msg, TnyHeader *header)
{
	TnyMsg *new_msg = NULL;
	TnyMimePart *body_part = NULL;
	ModestFormatterPrivate *priv;
	gchar *txt;

	/* Build new part */
	new_msg     = modest_formatter_create_message (self, TRUE, TRUE, FALSE);
	body_part = modest_formatter_create_body_part (self, new_msg);

	/* Create the two parts */
	priv = MODEST_FORMATTER_GET_PRIVATE (self);
	txt = modest_text_utils_cite ("", priv->content_type, priv->signature,
				      NULL, tny_header_get_date_sent (header));
	construct_from_text (body_part, txt, priv->content_type);
	g_free (txt);
	g_object_unref (body_part);

	if (msg) {
		/* Add parts */
		tny_mime_part_add_part (TNY_MIME_PART (new_msg), TNY_MIME_PART (msg));
	}

	return new_msg;
}

ModestFormatter*
modest_formatter_new (const gchar *content_type, const gchar *signature)
{
	ModestFormatter *formatter;
	ModestFormatterPrivate *priv;

	formatter = g_object_new (MODEST_TYPE_FORMATTER, NULL);
	priv = MODEST_FORMATTER_GET_PRIVATE (formatter);
	priv->content_type = g_strdup (content_type);
	priv->signature = g_strdup (signature);

	return formatter;
}

static void
modest_formatter_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestFormatter *self = (ModestFormatter *)instance;
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	priv->content_type = NULL;
	priv->signature = NULL;
}

static void
modest_formatter_finalize (GObject *object)
{
	ModestFormatter *self = (ModestFormatter *)object;
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	if (priv->content_type)
		g_free (priv->content_type);

	if (priv->signature)
		g_free (priv->signature);

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
modest_formatter_wrapper_cite (ModestFormatter *self, const gchar *text, TnyHeader *header,
			       GList *attachments) 
{
	gchar *result, *from;
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);
	
	from = tny_header_dup_from (header);
	result = modest_text_utils_cite (text, 
					 priv->content_type, 
					 priv->signature,
					 from, 
					 tny_header_get_date_sent (header));
	g_free (from);
	return result;
}

static gchar *
modest_formatter_wrapper_inline (ModestFormatter *self, const gchar *text, TnyHeader *header,
				 GList *attachments) 
{
	gchar *result, *from, *to, *subject;
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);

	from = tny_header_dup_from (header);
	to = tny_header_dup_to (header);
	subject = tny_header_dup_subject (header);
	result =  modest_text_utils_inline (text, 
					    priv->content_type, 
					    priv->signature,
					    from,
					    tny_header_get_date_sent (header),
					    to,
					    subject);
	g_free (subject);
	g_free (to);
	g_free (from);
	return result;
}

static gchar *
modest_formatter_wrapper_quote (ModestFormatter *self, const gchar *text, TnyHeader *header,
				GList *attachments) 
{
	ModestFormatterPrivate *priv = MODEST_FORMATTER_GET_PRIVATE (self);
	GList *filenames = NULL;
	GList *node = NULL;
	gchar *result = NULL;
	gchar *from;

	/* First we need a GList of attachments filenames */
	for (node = attachments; node != NULL; node = g_list_next (node)) {
		TnyMimePart *part = (TnyMimePart *) node->data;
		gchar *filename = NULL;
		if (TNY_IS_MSG (part)) {
			TnyHeader *header = tny_msg_get_header (TNY_MSG (part));
			filename = tny_header_dup_subject (header);
			if ((filename == NULL)||(filename[0] == '\0')) {
				g_free (filename);
				filename = g_strdup (_("mail_va_no_subject"));
			}
			g_object_unref (header);
		} else {
			filename = g_strdup (tny_mime_part_get_filename (part));
			if ((filename == NULL)||(filename[0] == '\0')) {
				g_free (filename);
				filename = g_strdup ("");
			}
		}
		filenames = g_list_prepend (filenames, filename);
	}

	/* TODO: get 80 from the configuration */
	from = tny_header_dup_from (header);
	result = modest_text_utils_quote (text, 
					  priv->content_type, 
					  priv->signature,
					  from,
					  tny_header_get_date_sent (header),
					  filenames,
					  80);
	g_free (from);

	g_list_foreach (filenames, (GFunc) g_free, NULL);
	g_list_free (filenames);
	return result;
}

TnyMsg * 
modest_formatter_create_message (ModestFormatter *self, gboolean single_body, 
				 gboolean has_attachments, gboolean has_images)
{
	TnyMsg *result = NULL;
	TnyPlatformFactory *fact = NULL;
	TnyMimePart *body_mime_part = NULL;
	TnyMimePart *related_mime_part = NULL;

	fact    = modest_runtime_get_platform_factory ();
	result = tny_platform_factory_new_msg (fact);
	if (has_attachments) {
		tny_mime_part_set_content_type (TNY_MIME_PART (result), "multipart/mixed");
		if (has_images) {
			related_mime_part = tny_platform_factory_new_mime_part (fact);
			tny_mime_part_set_content_type (related_mime_part, "multipart/related");
			tny_mime_part_add_part (TNY_MIME_PART (result), related_mime_part);
		} else {
			related_mime_part = g_object_ref (result);
		}
			
		if (!single_body) {
			body_mime_part = tny_platform_factory_new_mime_part (fact);
			tny_mime_part_set_content_type (body_mime_part, "multipart/alternative");
			tny_mime_part_add_part (TNY_MIME_PART (related_mime_part), body_mime_part);
			g_object_unref (body_mime_part);
		}

		g_object_unref (related_mime_part);
	} else if (has_images) {
		tny_mime_part_set_content_type (TNY_MIME_PART (result), "multipart/related");

		if (!single_body) {
			body_mime_part = tny_platform_factory_new_mime_part (fact);
			tny_mime_part_set_content_type (body_mime_part, "multipart/alternative");
			tny_mime_part_add_part (TNY_MIME_PART (result), body_mime_part);
			g_object_unref (body_mime_part);
		}

	} else if (!single_body) {
		tny_mime_part_set_content_type (TNY_MIME_PART (result), "multipart/alternative");
	}

	return result;
}

TnyMimePart *
find_body_parent (TnyMimePart *part)
{
	const gchar *msg_content_type = NULL;
	msg_content_type = tny_mime_part_get_content_type (part);

	if ((msg_content_type != NULL) &&
	    (!g_strcasecmp (msg_content_type, "multipart/alternative")))
		return g_object_ref (part);
	else if ((msg_content_type != NULL) &&
		 (g_str_has_prefix (msg_content_type, "multipart/"))) {
		TnyIterator *iter = NULL;
		TnyMimePart *alternative_part = NULL;
		TnyMimePart *related_part = NULL;
		TnyList *parts = TNY_LIST (tny_simple_list_new ());
		tny_mime_part_get_parts (TNY_MIME_PART (part), parts);
		iter = tny_list_create_iterator (parts);

		while (!tny_iterator_is_done (iter)) {
			TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
			if (part && !g_strcasecmp(tny_mime_part_get_content_type (part), "multipart/alternative")) {
				alternative_part = part;
				break;
			} else if (part && !g_strcasecmp (tny_mime_part_get_content_type (part), "multipart/related")) {
				related_part = part;
				break;
			}

			if (part)
				g_object_unref (part);

			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (parts);
		if (related_part) {
			TnyMimePart *result;
			result = find_body_parent (related_part);
			g_object_unref (related_part);
			return result;
		} else if (alternative_part)
			return alternative_part;
		else 
			return g_object_ref (part);
	} else
		return NULL;
}

TnyMimePart * 
modest_formatter_create_body_part (ModestFormatter *self, TnyMsg *msg)
{
	TnyMimePart *result = NULL;
	TnyPlatformFactory *fact = NULL;
	TnyMimePart *parent = NULL;

	parent = find_body_parent (TNY_MIME_PART (msg));
	fact = modest_runtime_get_platform_factory ();
	if (parent != NULL) {
		result = tny_platform_factory_new_mime_part (fact);
		tny_mime_part_add_part (TNY_MIME_PART (parent), result);
		g_object_unref (parent);
	} else {
		result = g_object_ref (msg);
	}

	return result;

}

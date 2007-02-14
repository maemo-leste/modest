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

#include <string.h>
#include <gtkhtml/gtkhtml.h>
#include <tny-gtk-text-buffer-stream.h>
#include <tny-simple-list.h>
#include <tny-folder.h>
#include <modest-tny-platform-factory.h>
#include <tny-camel-stream.h>
#include <camel/camel-stream-mem.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

#include <modest-tny-msg.h>
#include "modest-text-utils.h"

static TnyMimePart * add_body_part (TnyMsg *msg, const gchar *body,
				    const gchar *content_type, gboolean has_attachments);
static void add_attachments (TnyMsg *msg, GList *attachments_list);
static char * get_content_type(const gchar *s);
static gboolean is_ascii(const gchar *s);

TnyMsg*
modest_tny_msg_new (const gchar* mailto, const gchar* from, const gchar *cc,
		    const gchar *bcc, const gchar* subject, const gchar *body,
		    GSList *attachments)
{
	TnyPlatformFactory *fact;
	TnyMsg *new_msg;
	TnyHeader *header;
	gchar *content_type;
	
	/* Create new msg */
	fact    = modest_runtime_get_platform_factory ();
	new_msg = tny_platform_factory_new_msg (fact);
	header  = tny_platform_factory_new_header (fact);
	
	/* WARNING: set the header before assign values to it */
	tny_msg_set_header (new_msg, header);
	tny_header_set_from (TNY_HEADER (header), from);
	tny_header_set_replyto (TNY_HEADER (header), from);
	tny_header_set_to (TNY_HEADER (header), mailto);
	tny_header_set_cc (TNY_HEADER (header), cc);
	tny_header_set_bcc (TNY_HEADER (header), bcc);
	tny_header_set_subject (TNY_HEADER (header), subject);

	content_type = get_content_type(body);
	
	
	/* Add the body of the new mail */	
	add_body_part (new_msg, body, content_type, (attachments ? TRUE: FALSE));
		       
	/* Add attachments */
	add_attachments (new_msg, (GList*) attachments);

	return new_msg;
}


/* FIXME: this func copy from modest-mail-operation: refactor */
static TnyMimePart *
add_body_part (TnyMsg *msg, 
	       const gchar *body,
	       const gchar *content_type,
	       gboolean has_attachments)
{
	TnyMimePart *text_body_part = NULL;
	TnyStream *text_body_stream;

	/* Create the stream */
	text_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(body, strlen(body))));

	/* Create body part if needed */
	if (has_attachments)
		text_body_part = tny_platform_factory_new_mime_part
			(modest_runtime_get_platform_factory ());
	else
		text_body_part = TNY_MIME_PART(msg);

	/* Construct MIME part */
	tny_stream_reset (text_body_stream);
	tny_mime_part_construct_from_stream (text_body_part,
					     text_body_stream,
					     content_type);
	tny_stream_reset (text_body_stream);

	/* Add part if needed */
	if (has_attachments) {
		tny_mime_part_add_part (TNY_MIME_PART (msg), text_body_part);
		g_object_unref (G_OBJECT(text_body_part));
	}

	/* Clean */
	g_object_unref (text_body_stream);

	return text_body_part;
}

static void
add_attachments (TnyMsg *msg, GList *attachments_list)
{
	GList *pos;
	TnyMimePart *attachment_part, *old_attachment;
	const gchar *attachment_content_type;
	const gchar *attachment_filename;
	TnyStream *attachment_stream;

	for (pos = (GList *)attachments_list; pos; pos = pos->next) {

		old_attachment = pos->data;
		attachment_filename = tny_mime_part_get_filename (old_attachment);
		attachment_stream = tny_mime_part_get_stream (old_attachment);
		attachment_part = tny_platform_factory_new_mime_part (
			modest_runtime_get_platform_factory());
		
		attachment_content_type = tny_mime_part_get_content_type (old_attachment);
				 
		tny_mime_part_construct_from_stream (attachment_part,
						     attachment_stream,
						     attachment_content_type);
		tny_stream_reset (attachment_stream);
		
		tny_mime_part_set_filename (attachment_part, attachment_filename);
		
		tny_mime_part_add_part (TNY_MIME_PART (msg), attachment_part);
/* 		g_object_unref (attachment_part); */
	}
}


gchar * 
modest_tny_msg_get_body (TnyMsg *msg, gboolean want_html)
{
	TnyStream *stream;
	TnyMimePart *body;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	gchar *to_quote;

	body = modest_tny_msg_find_body_part(msg, want_html);
	if (!body)
		return NULL;

	buf = gtk_text_buffer_new (NULL);
	stream = TNY_STREAM (tny_gtk_text_buffer_stream_new (buf));
	tny_stream_reset (stream);
	tny_mime_part_decode_to_stream (body, stream);
	tny_stream_reset (stream);

	g_object_unref (G_OBJECT(stream));
	g_object_unref (G_OBJECT(body));
	
	gtk_text_buffer_get_bounds (buf, &start, &end);
	to_quote = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	g_object_unref (buf);

	return to_quote;
}


TnyMimePart*
modest_tny_msg_find_body_part_from_mime_part (TnyMimePart *msg, gboolean want_html)
{
	const gchar *mime_type = want_html ? "text/html" : "text/plain";
	TnyMimePart *part = NULL;
	TnyList *parts;
	TnyIterator *iter;

	if (!msg)
		return NULL;

	parts = TNY_LIST (tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);

	iter  = tny_list_create_iterator(parts);

	/* no parts? assume it's single-part message */
	if (tny_iterator_is_done(iter)) 
		return TNY_MIME_PART (g_object_ref(G_OBJECT(msg)));
	else {
		gchar *content_type = NULL;
		do {
			part = TNY_MIME_PART(tny_iterator_get_current (iter));

			/* we need to strdown the content type, because
			 * tny_mime_part_has_content_type does not do it...
			 */
			content_type = g_ascii_strdown
				(tny_mime_part_get_content_type (part), -1);
							
			if (g_str_has_prefix (content_type, mime_type) &&
			    !tny_mime_part_is_attachment (part))
				break;
			
			if (g_str_has_prefix(content_type, "multipart")) {
				part = modest_tny_msg_find_body_part_from_mime_part (part, want_html);
				if (part)
					break;
			}
			if (part)
				g_object_unref (G_OBJECT(part));

			part = NULL;
			
			g_free (content_type);
			content_type = NULL;

			tny_iterator_next (iter);
			
		} while (!tny_iterator_is_done(iter));
		g_free (content_type);
	}
	
	g_object_unref (G_OBJECT(iter));
	g_object_unref (G_OBJECT(parts));

	/* if were trying to find an HTML part and couldn't find it,
	 * try to find a text/plain part instead
	 */
	if (!part && want_html) 
		return modest_tny_msg_find_body_part_from_mime_part (msg, FALSE);
	else
		return part; /* this maybe NULL, this is not an error; some message just don't have a body
			      * part */
}


TnyMimePart*
modest_tny_msg_find_body_part (TnyMsg *msg, gboolean want_html)
{
	return modest_tny_msg_find_body_part_from_mime_part (TNY_MIME_PART(msg),
							     want_html);
}



static gboolean
is_ascii(const gchar *s)
{
	while (s[0]) {
		if (s[0] & 128 || s[0] < 32)
			return FALSE;
		s++;
	}
	return TRUE;
}

static char *
get_content_type(const gchar *s)
{
	GString *type;
	
	type = g_string_new("text/plain");
	if (!is_ascii(s)) {
		if (g_utf8_validate(s, -1, NULL)) {
			g_string_append(type, "; charset=\"utf-8\"");
		} else {
			/* it should be impossible to reach this, but better safe than sorry */
			g_warning("invalid utf8 in message");
			g_string_append(type, "; charset=\"latin1\"");
		}
	}
	return g_string_free(type, FALSE);
}

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

#include "modest-tny-msg-actions.h"
#include "modest-text-utils.h"

static const gchar *
get_body_text (TnyMsg *msg, gboolean want_html)
{
	TnyStream *stream;
	TnyMimePart *body;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	const gchar *to_quote;

	body = modest_tny_msg_actions_find_body_part(msg, want_html);
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

static TnyMimePart*
modest_tny_msg_actions_find_body_part_from_mime_part (TnyMimePart *msg, gboolean want_html)
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
		do {
			const gchar *ct;
			gchar *content_type;
			part = TNY_MIME_PART(tny_iterator_get_current (iter));

			/* we need to strdown the content type, because
			 * tny_mime_part_has_content_type does not do it...
			 */
			ct = tny_mime_part_get_content_type (part);
			content_type = g_ascii_strdown (ct, strlen(ct));
						
			if (g_str_has_prefix (content_type, mime_type) &&
			    !tny_mime_part_is_attachment (part)) {
				g_free (content_type);
				break;
			}
			
			if (g_str_has_prefix(content_type, "multipart")) {
				part = modest_tny_msg_actions_find_body_part_from_mime_part (part,
											     want_html);
				g_free (content_type);
				if (part)
					break;
			}

			g_free (content_type);
			part = NULL;
			tny_iterator_next (iter);

		} while (!tny_iterator_is_done(iter));
	}
	
	/* did we find a matching part? */
	if (part)
		g_object_ref (G_OBJECT(part));

	g_object_unref (G_OBJECT(iter));
	g_object_unref (G_OBJECT(parts));

	/* if were trying to find an HTML part and couldn't find it,
	 * try to find a text/plain part instead
	 */
	if (!part && want_html) 
		return modest_tny_msg_actions_find_body_part_from_mime_part (msg, FALSE);

	if (!part)
		g_printerr ("modest: cannot find body part\n");
	
	return part ? part : NULL;
}


TnyMimePart*
modest_tny_msg_actions_find_body_part (TnyMsg *msg, gboolean want_html)
{
	return modest_tny_msg_actions_find_body_part_from_mime_part (TNY_MIME_PART(msg),
								     want_html);
}


TnyMimePart *
modest_tny_msg_actions_find_nth_part (TnyMsg *msg, gint index)
{
	TnyMimePart *part;
	TnyList *parts;
	TnyIterator *iter;

	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (index > 0, NULL);
		
	parts = TNY_LIST(tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART(msg), parts);
	iter  = tny_list_create_iterator (parts);

	part = NULL;
	
	if (!tny_iterator_is_done(iter)) {
		tny_iterator_nth (iter, index);
		part = TNY_MIME_PART(tny_iterator_get_current (iter));
	}

	g_object_unref (G_OBJECT(iter));
	g_object_unref (G_OBJECT(parts));

	return part;
}

gchar * 
modest_tny_msg_actions_find_body (TnyMsg *msg, gboolean want_html)
{
	const gchar *body;

	body = get_body_text (msg, want_html);

	if (body)
		return g_strdup (body);
	else 
		return NULL;
}

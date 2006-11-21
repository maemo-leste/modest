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


#include <gtk/gtk.h>
#include <gtkhtml/gtkhtml.h>
#include <tny-gtk-text-buffer-stream.h>
#include <tny-simple-list.h>
#include <tny-folder.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

#include "modest-tny-msg-actions.h"
#include "modest-text-utils.h"

static void modest_tny_msg_actions_xfer (TnyHeader *header, TnyFolder *folder, 
					 gboolean delete_original);


static const gchar *
get_body_text (TnyMsg *msg, gboolean want_html)
{
	TnyStream *stream;
	TnyMimePart *body;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	const gchar *to_quote;
	gchar *quoted;

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

gchar*
modest_tny_msg_actions_quote (TnyMsg * self, const gchar * from,
			      time_t sent_date, gint limit,
			      const gchar * to_quote)
{
	gchar *quoted_msg = NULL;
	const gchar *body;

	/* 2 cases: */

	/* a) quote text from selection */
	if (to_quote != NULL) 
		return modest_text_utils_quote (to_quote, from, sent_date,
						limit);
	
	/* b) try to find a text/plain part in the msg and quote it */
	body = get_body_text (self, FALSE);
	if (body)
		quoted_msg = modest_text_utils_quote (body, from, sent_date, limit);
	
	return quoted_msg;
}



TnyMimePart *
modest_tny_msg_actions_find_body_part (TnyMsg *msg, gboolean want_html)
{
	const gchar *mime_type = want_html ? "text/html" : "text/plain";
	TnyMimePart *part;
	TnyList *parts;
	TnyIterator *iter;

	if (!msg)
		return NULL;

	parts = TNY_LIST (tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);

	iter  = tny_list_create_iterator(parts);

	while (!tny_iterator_is_done(iter)) {

		part = TNY_MIME_PART(tny_iterator_get_current (iter));
		
		if (tny_mime_part_content_type_is (part, mime_type) &&
		    !tny_mime_part_is_attachment (part)) {
			break;
		}
		part = NULL;
		tny_iterator_next (iter);
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
		return modest_tny_msg_actions_find_body_part (msg, FALSE);

	return part ? part : NULL;
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


static void
modest_tny_msg_actions_xfer (TnyHeader *header, TnyFolder *folder, 
			     gboolean delete_original)
{
	TnyFolder *src_folder;
	TnyList *headers;

	src_folder = tny_header_get_folder (header);
	headers = tny_simple_list_new ();

	/* Move */
	tny_list_prepend (headers, G_OBJECT (header));
	tny_folder_transfer_msgs (src_folder, headers, folder, delete_original);

	/* Free */
	g_object_unref (headers);
	g_object_unref (folder);
}

void
modest_tny_msg_actions_copy (TnyHeader *header, TnyFolder *folder)
{
	g_return_if_fail (TNY_IS_HEADER (header));
	g_return_if_fail (TNY_IS_FOLDER (folder));

	modest_tny_msg_actions_xfer (header, folder, FALSE);
}

void
modest_tny_msg_actions_move (TnyHeader *header, TnyFolder *folder)
{
	g_return_if_fail (TNY_IS_HEADER (header));
	g_return_if_fail (TNY_IS_FOLDER (folder));

	modest_tny_msg_actions_xfer (header, folder, TRUE);
}

void
modest_tny_msg_actions_remove (TnyHeader *header)
{
	TnyFolder *folder;

	g_return_if_fail (TNY_IS_HEADER (header));

	folder = tny_header_get_folder (header);

	/* Remove */
	tny_folder_remove_msg (folder, header);
	tny_folder_expunge (folder);

	/* Free */
	g_object_unref (folder);
}

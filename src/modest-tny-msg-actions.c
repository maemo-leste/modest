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
#include <tny-text-buffer-stream.h>
#include <tny-mime-part-iface.h>
#include <tny-msg-iface.h>
#include <tny-list-iface.h>
#include <tny-list.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

#include "modest-tny-msg-actions.h"
#include "modest-text-utils.h"

static gchar *
quote_msg (TnyMsgIface* src, const gchar * from, time_t sent_date, gint limit)
{
	TnyStreamIface *stream;
	TnyMimePartIface *body;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	const gchar *to_quote;
	gchar *quoted;

	body = modest_tny_msg_actions_find_body_part(src, FALSE);
	if (!body)
		return NULL;

	buf = gtk_text_buffer_new (NULL);
	stream = TNY_STREAM_IFACE (tny_text_buffer_stream_new (buf));
	tny_stream_iface_reset (stream);
	tny_mime_part_iface_decode_to_stream (body, stream);
	tny_stream_iface_reset (stream);

	g_object_unref (G_OBJECT(stream));
	g_object_unref (G_OBJECT(body));
	
	gtk_text_buffer_get_bounds (buf, &start, &end);
	to_quote = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	quoted = modest_text_utils_quote (to_quote, from, sent_date, limit);
	g_object_unref (buf);

	return quoted;
}


gchar*
modest_tny_msg_actions_quote (TnyMsgIface * self, const gchar * from,
			      time_t sent_date, gint limit,
			      const gchar * to_quote)
{
	/* 2 cases: */

	/* a) quote text from selection */
	if (to_quote != NULL) 
		return modest_text_utils_quote (to_quote, from, sent_date,
						limit);
	
	/* b) try to find a text/plain part in the msg and quote it */
	return quote_msg (self, from, sent_date, limit);
}



TnyMimePartIface *
modest_tny_msg_actions_find_body_part (TnyMsgIface *msg, gboolean want_html)
{
	const gchar *mime_type = want_html ? "text/html" : "text/plain";
	TnyMimePartIface *part;
	TnyListIface *parts;
	TnyIteratorIface *iter;

	if (!msg)
		return NULL;

	parts = TNY_LIST_IFACE(tny_list_new());
	tny_msg_iface_get_parts ((TnyMsgIface*)msg, parts);

	iter  = tny_list_iface_create_iterator(parts);

	while (!tny_iterator_iface_is_done(iter)) {

		part = TNY_MIME_PART_IFACE(tny_iterator_iface_current (iter));
		
		if (tny_mime_part_iface_content_type_is (part, mime_type) &&
		    !tny_mime_part_iface_is_attachment (part)) {
			break;
		}
		part = NULL;
		tny_iterator_iface_next (iter);
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



TnyMimePartIface *
modest_tny_msg_actions_find_nth_part (TnyMsgIface *msg, gint index)
{
	TnyMimePartIface *part;
	TnyListIface *parts;
	TnyIteratorIface *iter;

	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (index > 0, NULL);
		
	parts = TNY_LIST_IFACE(tny_list_new());
	tny_msg_iface_get_parts ((TnyMsgIface*)msg, parts);
	iter  = tny_list_iface_create_iterator ((TnyListIface*)parts);
	if (!tny_iterator_iface_has_first(iter))
		return NULL;
	
	tny_iterator_iface_nth (iter, index);
	part = TNY_MIME_PART_IFACE(tny_iterator_iface_current (iter));

	g_object_unref (G_OBJECT(iter));
	g_object_unref (G_OBJECT(parts));

	return part;
}

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

/* TODO: put in auto* */
#include <tny-text-buffer-stream.h>
#include <tny-msg-mime-part-iface.h>
#include <tny-msg-iface.h>
#include <tny-list-iface.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

#include "modest-tny-msg-actions.h"
#include "modest-text-utils.h"


/* private */
static gchar *quote_msg (const TnyMsgIface * src, const gchar * from,
			 time_t sent_date, gint limit);

static gchar *
quote_msg (const TnyMsgIface * src, const gchar * from, time_t sent_date, gint limit)
{
	TnyStreamIface *stream;
	TnyMsgMimePartIface *body;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	const gchar *to_quote;
	gchar *quoted;

	/* the cast makes me uneasy... */
	body = modest_tny_msg_actions_find_body_part(src, FALSE);
	if (!body)
		return NULL;

	buf = gtk_text_buffer_new (NULL);
	stream = TNY_STREAM_IFACE (tny_text_buffer_stream_new (buf));
	tny_stream_iface_reset (stream);
	tny_msg_mime_part_iface_decode_to_stream (body, stream);
	tny_stream_iface_reset (stream);
	g_object_unref (stream);
	
	gtk_text_buffer_get_bounds (buf, &start, &end);
	to_quote = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	quoted = modest_text_utils_quote (to_quote, from, sent_date, limit);
	g_object_unref (buf);

	return quoted;
}


gchar*
modest_tny_msg_actions_quote (const TnyMsgIface * self, const gchar * from,
			      time_t sent_date, gint limit,
			      const gchar * to_quote)
{
	gchar *quoted;

	/* 2 cases: */

	/* a) quote text from selection */
	if (to_quote != NULL) 
		return modest_text_utils_quote (to_quote, from, sent_date,
						limit);
	
	/* b) try to find a text/plain part in the msg and quote it */
	return quote_msg (self, from, sent_date, limit);
}



TnyMsgMimePartIface *
modest_tny_msg_actions_find_body_part (const TnyMsgIface *msg, gboolean want_html)
{
	const gchar *mime_type = want_html ? "text/html" : "text/plain";
	TnyMsgMimePartIface *part;
	const TnyListIface *parts;
	TnyIteratorIface *iter;
	gboolean found;

	if (!msg)
		return NULL;

	found = FALSE;
	parts =  tny_msg_iface_get_parts ((TnyMsgIface *)msg);
	iter  = tny_list_iface_create_iterator ((TnyListIface*)parts);

	while (!tny_iterator_iface_is_done(iter)) {
		part = TNY_MSG_MIME_PART_IFACE(tny_iterator_iface_current (iter));
		
		if (tny_msg_mime_part_iface_content_type_is (part, mime_type) &&
		    !tny_msg_mime_part_iface_is_attachment (part)) {
			found = TRUE;
			break;
		}	
		tny_iterator_iface_next (iter);
	}

	g_object_unref (G_OBJECT(iter));

	/* if were trying to find an HTML part and could find it,
	 * try to find a text/plain part instead
	 */
	if (!found && want_html) 
		return modest_tny_msg_actions_find_body_part (msg, FALSE);

	return found ? part : NULL;
}




TnyMsgMimePartIface *
modest_tny_msg_actions_find_nth_part (const TnyMsgIface *msg, gint index)
{
	TnyMsgMimePartIface *part;
	const TnyListIface *parts;
	TnyIteratorIface *iter;

	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (index > 0, NULL);
		
	parts =  tny_msg_iface_get_parts ((TnyMsgIface *)msg);
	iter  = tny_list_iface_create_iterator ((TnyListIface*)parts);

	if (!tny_iterator_iface_has_first(iter))
		return NULL;

	part = tny_iterator_iface_nth (iter, index);

	g_object_unref (G_OBJECT(iter));

	return part;
}

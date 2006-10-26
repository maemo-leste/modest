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


#include <tny-msg.h>
#include <tny-mime-part.h>
#include <tny-stream.h>
#include <tny-header.h>
#include <tny-account.h>	
#include <tny-account-store.h>
#include <tny-transport-account.h>
#include <tny-stream-camel.h>
#include <tny-fs-stream.h>
#include <tny-camel-msg.h>
#include <tny-camel-header.h>
#include <tny-camel-stream.h>
#include <camel/camel-stream-mem.h>
#include <string.h>

#include "modest-tny-transport-actions.h"
#include "modest-tny-attachment.h"

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

gboolean
modest_tny_transport_actions_send_message (TnyTransportAccount *transport_account,
					   const gchar *from,
					   const gchar *to,
					   const gchar *cc,
					   const gchar *bcc,
					   const gchar *subject,
					   const gchar *body,
					   const GList *attachments_list)
{
	TnyMsg *new_msg;
	TnyMimePart *attachment_part, *text_body_part;
	TnyHeader *headers;
	TnyStream *text_body_stream, *attachment_stream;
	ModestTnyAttachment *attachment;
	GList *pos;
	gchar *content_type;
	const gchar *attachment_content_type;
	const gchar *attachment_filename;
	
	new_msg          = tny_camel_msg_new ();
	headers          = tny_camel_header_new ();
	text_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(body, strlen(body))));
	
	tny_header_set_from (TNY_HEADER (headers), from);
	tny_header_set_to (TNY_HEADER (headers), to);
	tny_header_set_cc (TNY_HEADER (headers), cc);
	tny_header_set_bcc (TNY_HEADER (headers), bcc);
	tny_header_set_subject (TNY_HEADER (headers), subject);
	tny_msg_set_header (new_msg, headers);

	content_type = get_content_type(body);
	
	if (attachments_list == NULL) {
		tny_stream_reset (text_body_stream);
		tny_mime_part_construct_from_stream (TNY_MIME_PART(new_msg),
						     text_body_stream, content_type);
		tny_stream_reset (text_body_stream);
	} else {
		text_body_part = 
			TNY_MIME_PART (tny_camel_mime_part_new(camel_mime_part_new()));
		tny_stream_reset (text_body_stream);
		tny_mime_part_construct_from_stream (text_body_part,
						     text_body_stream,
						     content_type);
		tny_stream_reset (text_body_stream);
		tny_msg_add_part(new_msg, text_body_part);
		//g_object_unref (G_OBJECT(text_body_part));
	}
	
/* 	for (    pos = (GList *)attachments_list; */
/* 	         pos; */
/* 	         pos = pos->next    ) { */
/* 		attachment = pos->data; */
/* 		attachment_filename = modest_tny_attachment_get_name(attachment); */
/* 		attachment_stream = modest_tny_attachment_get_stream(attachment); */
/* 		attachment_part = TNY_MIME_PART_IFACE (tny_camel_mime_part_new ( */
/* 							       camel_mime_part_new())); */
		
/* 		attachment_content_type = modest_tny_attachment_get_mime_type(attachment); */
				 
/* 		tny_mime_part_construct_from_stream (attachment_part, */
/* 						     attachment_stream, */
/* 						     attachment_content_type); */
/* 		tny_stream_reset (attachment_stream); */
		
/* 		tny_mime_part_set_filename(attachment_part, attachment_filename); */
		
/* 		tny_msg_add_part (new_msg, attachment_part); */
/* 		//g_object_unref(G_OBJECT(attachment_part)); */
/* 		//close(file); */
/* 	} */
	
	tny_transport_account_send (transport_account, new_msg);

	g_object_unref (G_OBJECT(text_body_stream));
	g_object_unref (G_OBJECT(headers));
	g_object_unref (G_OBJECT(new_msg));
	g_free(content_type);

	return TRUE;	
}

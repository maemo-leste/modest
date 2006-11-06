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

#include "modest-mail-operation.h"
/* include other impl specific header files */
#include <string.h>
#include <tny-mime-part.h>

/* 'private'/'protected' functions */
static void modest_mail_operation_class_init (ModestMailOperationClass *klass);
static void modest_mail_operation_init       (ModestMailOperation *obj);
static void modest_mail_operation_finalize   (GObject *obj);
/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMailOperationPrivate ModestMailOperationPrivate;
struct _ModestMailOperationPrivate {
};
#define MODEST_MAIL_OPERATION_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_MAIL_OPERATION, \
                                                   ModestMailOperationPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_mail_operation_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMailOperationClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_mail_operation_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMailOperation),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_mail_operation_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestMailOperation",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_mail_operation_class_init (ModestMailOperationClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_mail_operation_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMailOperationPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_mail_operation_init (ModestMailOperation *obj)
{
/* 	initialize this object, eg.: */
/* 	priv->frobnicate_mode = FALSE; */
}

static void
modest_mail_operation_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestMailOperation*
modest_mail_operation_new (void)
{
	return MODEST_MAIL_OPERATION(g_object_new(MODEST_TYPE_MAIL_OPERATION, NULL));
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

ModestMailOperation*
modest_mail_operation_send_mail (TnyTransportAccount *transport_account,
				 const gchar *from,
				 const gchar *to,
				 const gchar *cc,
				 const gchar *bcc,
				 const gchar *subject,
				 const gchar *body,
				 const GList *attachments_list)
{
	ModestMailOperation *mail_operation;
	TnyMsg *new_msg;
	TnyHeader *headers;
	TnyStream *text_body_stream, *attachment_stream;
	ModestTnyAttachment *attachment;
	GList *pos;
	gchar *content_type;
	const gchar *attachment_content_type;
	const gchar *attachment_filename;

	/* TODO: better error handling management. Do it inside the
	   ModestMailOperation, for example set operation state to
	   failed or something like this and fill a GError with a
	   short description */
	if (to == NULL || body == NULL)
		return NULL;

	mail_operation   = modest_mail_operation_new ();
	new_msg          = TNY_MSG (tny_camel_msg_new ());
	headers          = TNY_HEADER (tny_camel_header_new ());
	text_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(body, strlen(body))));

	/* IMPORTANT: set the header before assign values to it */
	tny_msg_set_header (new_msg, headers);
	tny_header_set_from (TNY_HEADER (headers), from);
	tny_header_set_to (TNY_HEADER (headers), to);
	tny_header_set_cc (TNY_HEADER (headers), cc);
	tny_header_set_bcc (TNY_HEADER (headers), bcc);
	tny_header_set_subject (TNY_HEADER (headers), subject);

	content_type = get_content_type(body);
	
	if (attachments_list == NULL) {
		tny_stream_reset (text_body_stream);
		tny_mime_part_construct_from_stream (TNY_MIME_PART(new_msg),
						     text_body_stream, content_type);
		tny_stream_reset (text_body_stream);
	} else {
		TnyMimePart *text_body_part;

		text_body_part = 
			TNY_MIME_PART (tny_camel_mime_part_new (camel_mime_part_new()));
		tny_stream_reset (text_body_stream);
		tny_mime_part_construct_from_stream (text_body_part,
						     text_body_stream,
						     content_type);
		tny_stream_reset (text_body_stream);
		tny_msg_add_part (new_msg, text_body_part);
		g_object_unref (G_OBJECT(text_body_part));
	}

	for (pos = (GList *)attachments_list;
	     pos;
	     pos = pos->next) {
		TnyMimePart *attachment_part;

		attachment = pos->data;
		attachment_filename = modest_tny_attachment_get_name (attachment);
		attachment_stream = modest_tny_attachment_get_stream (attachment);
		attachment_part = TNY_MIME_PART (tny_camel_mime_part_new (camel_mime_part_new()));
		
		attachment_content_type = modest_tny_attachment_get_mime_type (attachment);
				 
		tny_mime_part_construct_from_stream (attachment_part,
						     attachment_stream,
						     attachment_content_type);
		tny_stream_reset (attachment_stream);
		
		tny_mime_part_set_filename (attachment_part, attachment_filename);
		
		tny_msg_add_part (new_msg, attachment_part);
		g_object_unref(G_OBJECT(attachment_part));
	}
	
	tny_transport_account_send (transport_account, new_msg);

	g_object_unref (G_OBJECT(text_body_stream));
	g_object_unref (G_OBJECT(headers));
	g_object_unref (G_OBJECT(new_msg));
	g_free(content_type);

	return mail_operation;
}

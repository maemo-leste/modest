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
#include <modest-runtime.h>
#include "modest-formatter.h"
#include <tny-camel-stream.h>
#include <tny-camel-mime-part.h>
#include <camel/camel-stream-buffer.h>
#include <camel/camel-stream-mem.h>
#include <glib/gprintf.h>
#include <modest-tny-folder.h>
#include "modest-tny-mime-part.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

#include <modest-tny-msg.h>
#include "modest-text-utils.h"

static TnyMimePart * add_body_part (TnyMsg *msg, const gchar *body,
				    const gchar *content_type);
static TnyMimePart * add_html_body_part (TnyMsg *msg, const gchar *body);
static void add_attachments (TnyMimePart *part, GList *attachments_list, gboolean add_inline);
static void add_images (TnyMsg *msg, GList *attachments_list);
static char * get_content_type(const gchar *s);
static gboolean is_ascii(const gchar *s);


TnyMsg*
modest_tny_msg_new (const gchar* mailto, const gchar* from, const gchar *cc,
		    const gchar *bcc, const gchar* subject, const gchar *body,
		    GList *attachments)
{
	TnyMsg *new_msg;
	TnyHeader *header;
	gchar *content_type;
	
	/* Create new msg */
	new_msg = modest_formatter_create_message (NULL, TRUE, (attachments != NULL), FALSE);
	header  = tny_msg_get_header (new_msg);
	
	if ((from != NULL) && (strlen(from) > 0)) {
		tny_header_set_from (TNY_HEADER (header), from);
		tny_header_set_replyto (TNY_HEADER (header), from);
	}
	if ((mailto != NULL) && (strlen(mailto) > 0)) 
		tny_header_set_to (TNY_HEADER (header), mailto);
	if ((cc != NULL) && (strlen(cc) > 0)) 
		tny_header_set_cc (TNY_HEADER (header), cc);
	if ((bcc != NULL) && (strlen(bcc) > 0)) 
		tny_header_set_bcc (TNY_HEADER (header), bcc);
	
	if ((subject != NULL) && (strlen(subject) > 0)) 
		tny_header_set_subject (TNY_HEADER (header), subject);

	content_type = get_content_type(body);

	/* set modest as the X-Mailer
	 * we could this in the platform factory, but then the header
	 * would show up before all the others.
	 */
	tny_mime_part_set_header_pair (TNY_MIME_PART (new_msg), "X-Mailer", "Modest "
				       VERSION);
	
	/* Add the body of the new mail */
	/* This is needed even if body is NULL or empty. */
	add_body_part (new_msg, body, content_type);
	g_free (content_type);
		       
	/* Add attachments */
	if (attachments)
		add_attachments (TNY_MIME_PART (new_msg), attachments, FALSE);
	if (header)
		g_object_unref(header);

	return new_msg;
}

TnyMsg*
modest_tny_msg_new_html_plain (const gchar* mailto, const gchar* from, const gchar *cc,
			       const gchar *bcc, const gchar* subject, 
			       const gchar *html_body, const gchar *plain_body,
			       GList *attachments, GList *images)
{
	TnyMsg *new_msg;
	TnyHeader *header;
	gchar *content_type;
	
	/* Create new msg */
	new_msg = modest_formatter_create_message (NULL, FALSE, (attachments != NULL), (images != NULL));
	header  = tny_msg_get_header (new_msg);
	
	if ((from != NULL) && (strlen(from) > 0)) {
		tny_header_set_from (TNY_HEADER (header), from);
		tny_header_set_replyto (TNY_HEADER (header), from);
	}
	if ((mailto != NULL) && (strlen(mailto) > 0)) 
		tny_header_set_to (TNY_HEADER (header), mailto);
	if ((cc != NULL) && (strlen(cc) > 0)) 
		tny_header_set_cc (TNY_HEADER (header), cc);
	if ((bcc != NULL) && (strlen(bcc) > 0)) 
		tny_header_set_bcc (TNY_HEADER (header), bcc);
	
	if ((subject != NULL) && (strlen(subject) > 0)) 
		tny_header_set_subject (TNY_HEADER (header), subject);

	content_type = get_content_type(plain_body);
	
	/* set modest as the X-Mailer
	 * we could this in the platform factory, but then the header
	 * would show up before all the others.
	 */
	tny_mime_part_set_header_pair (TNY_MIME_PART (new_msg), "X-Mailer", "Modest "
				       VERSION);

	/* Add the body of the new mail */	
	add_body_part (new_msg, plain_body, content_type);
	add_html_body_part (new_msg, html_body);
	g_free (content_type);
		       
	/* Add attachments */
	add_attachments (TNY_MIME_PART (new_msg), attachments, FALSE);
	add_images (new_msg, images);
	if (header)
		g_object_unref(header);

	return new_msg;
}


/* FIXME: this func copy from modest-mail-operation: refactor */
static TnyMimePart *
add_body_part (TnyMsg *msg, 
	       const gchar *body,
	       const gchar *content_type)
{
	TnyMimePart *text_body_part = NULL;
	TnyStream *text_body_stream;

	/* Create the stream */
	text_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(body, (body ? strlen(body) : 0))));

	text_body_part = modest_formatter_create_body_part (NULL, msg);

	/* Construct MIME part */
	tny_stream_reset (text_body_stream);
	tny_mime_part_construct (text_body_part,
				 text_body_stream,
				 content_type, "7bit");
	tny_stream_reset (text_body_stream);

	g_object_unref (G_OBJECT(text_body_part));

	/* Clean */
	g_object_unref (text_body_stream);

	return text_body_part;
}

static TnyMimePart *
add_html_body_part (TnyMsg *msg, 
		    const gchar *body)
{
	TnyMimePart *html_body_part = NULL;
	TnyStream *html_body_stream;

	/* Create the stream */
	html_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(body, strlen(body))));

	/* Create body part if needed */
	html_body_part = modest_formatter_create_body_part (NULL, msg);

	/* Construct MIME part */
	tny_stream_reset (html_body_stream);
	tny_mime_part_construct (html_body_part,
			         html_body_stream,
			         "text/html; charset=utf-8", 
			         "7bit"); /* Sometimes it might be needed 
					     to make this one a 8bit! */
	tny_stream_reset (html_body_stream);

	g_object_unref (G_OBJECT(html_body_part));

	/* Clean */
	g_object_unref (html_body_stream);

	return html_body_part;
}

static TnyMimePart *
copy_mime_part (TnyMimePart *part)
{
	TnyMimePart *result = NULL;
	const gchar *attachment_content_type;
	const gchar *attachment_filename;
	const gchar *attachment_cid;
	TnyList *parts;
	TnyIterator *iterator;
	TnyStream *attachment_stream;
	const gchar *enc;
	
	if (TNY_IS_MSG (part)) {
		g_object_ref (part);
		return part;
	}

	result = tny_platform_factory_new_mime_part (
		modest_runtime_get_platform_factory());

	attachment_content_type = tny_mime_part_get_content_type (part);

	/* get mime part headers */
	attachment_filename = tny_mime_part_get_filename (part);
	attachment_cid = tny_mime_part_get_content_id (part);
	
	/* fill the stream */
 	attachment_stream = tny_mime_part_get_stream (part);
	enc = tny_mime_part_get_transfer_encoding (part);
	tny_stream_reset (attachment_stream);
	tny_mime_part_construct (result,
			         attachment_stream,
				 attachment_content_type, 
				 enc);
	tny_stream_reset (attachment_stream);
	
	/* set other mime part fields */
	tny_mime_part_set_filename (result, attachment_filename);
	tny_mime_part_set_content_id (result, attachment_cid);

	/* copy subparts */
	parts = tny_simple_list_new ();
	tny_mime_part_get_parts (part, parts);
	iterator = tny_list_create_iterator (parts);
	while (!tny_iterator_is_done (iterator)) {
		TnyMimePart *subpart = TNY_MIME_PART (tny_iterator_get_current (iterator));
		if (subpart) {
			const gchar *subpart_cid;
			TnyMimePart *subpart_copy = copy_mime_part (subpart);
			subpart_cid = tny_mime_part_get_content_id (subpart);
			tny_mime_part_add_part (result, subpart_copy);
			if (subpart_cid)
				tny_mime_part_set_content_id (result, subpart_cid);
			g_object_unref (subpart);
			g_object_unref (subpart_copy);
		}

		tny_iterator_next (iterator);
	}
	g_object_unref (iterator);
	g_object_unref (parts);
	g_object_unref (attachment_stream);

	return result;
}

static void
add_attachments (TnyMimePart *part, GList *attachments_list, gboolean add_inline)
{
	GList *pos;
	TnyMimePart *attachment_part, *old_attachment;

	for (pos = (GList *)attachments_list; pos; pos = pos->next) {

		old_attachment = pos->data;
		if (!tny_mime_part_is_purged (old_attachment)) {
			const gchar *old_cid;
			old_cid = tny_mime_part_get_content_id (old_attachment);
			attachment_part = copy_mime_part (old_attachment);
			tny_mime_part_set_header_pair (attachment_part, "Content-Disposition", 
						       add_inline?"inline":"attachment");
			tny_mime_part_set_transfer_encoding (TNY_MIME_PART (attachment_part), "base64");
			tny_mime_part_add_part (TNY_MIME_PART (part), attachment_part);
			if (old_cid)
				tny_mime_part_set_content_id (attachment_part, old_cid);
			g_object_unref (attachment_part);
		}
	}
}

static void
add_images (TnyMsg *msg, GList *images_list)
{
	TnyMimePart *related_part = NULL;
	const gchar *content_type;

	content_type = tny_mime_part_get_content_type (TNY_MIME_PART (msg));

	if ((content_type != NULL) && !strcasecmp (content_type, "multipart/related")) {
		related_part = g_object_ref (msg);
	} else if ((content_type != NULL) && !strcasecmp (content_type, "multipart/mixed")) {
		TnyList *parts = TNY_LIST (tny_simple_list_new ());
		TnyIterator *iter = NULL;
		tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);
		iter = tny_list_create_iterator (parts);

		while (!tny_iterator_is_done (iter)) {
			TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
			if (part && !g_strcasecmp (tny_mime_part_get_content_type (part), "multipart/related")) {
				related_part = part;
				break;
			} else {
				g_object_unref (part);
			}
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (parts);
	}

	if (related_part != NULL) {
		/* TODO: attach images in their proper place */
		add_attachments (related_part, images_list, TRUE);
		g_object_unref (related_part);
	}
}


gchar * 
modest_tny_msg_get_body (TnyMsg *msg, gboolean want_html, gboolean *is_html)
{
	TnyStream *stream;
	TnyMimePart *body;
	GtkTextBuffer *buf;
	GtkTextIter start, end;
	gchar *to_quote;
	gboolean result_was_html = TRUE;

	g_return_val_if_fail (msg && TNY_IS_MSG(msg), NULL);
	
	body = modest_tny_msg_find_body_part(msg, want_html);
	if (!body)
		return NULL;

	buf = gtk_text_buffer_new (NULL);
	stream = TNY_STREAM (tny_gtk_text_buffer_stream_new (buf));
	tny_stream_reset (stream);
	tny_mime_part_decode_to_stream (body, stream, NULL);
	tny_stream_reset (stream);
	
	gtk_text_buffer_get_bounds (buf, &start, &end);
	to_quote = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	if (tny_mime_part_content_type_is (body, "text/plain")) {
		gchar *to_quote_converted = modest_text_utils_convert_to_html (to_quote);
		g_free (to_quote);
		to_quote = to_quote_converted;
		result_was_html = FALSE;
	}

	g_object_unref (buf);
	g_object_unref (G_OBJECT(stream));
	g_object_unref (G_OBJECT(body));

	if (is_html != NULL)
		*is_html = result_was_html;

	return to_quote;
}


TnyMimePart*
modest_tny_msg_find_body_part_from_mime_part (TnyMimePart *msg, gboolean want_html)
{
	const gchar *desired_mime_type = want_html ? "text/html" : "text/plain";
	TnyMimePart *part = NULL;
	TnyList *parts = NULL;
	TnyIterator *iter = NULL;
	
	if (!msg)
		return NULL;

	parts = TNY_LIST (tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);

	iter  = tny_list_create_iterator(parts);

	/* no parts? assume it's single-part message */
	if (tny_iterator_is_done(iter)) {
		g_object_unref (G_OBJECT(iter));
		return TNY_MIME_PART (g_object_ref(G_OBJECT(msg)));
	} else {
		do {
			gchar *tmp, *content_type = NULL;
			gboolean has_content_disp_name = FALSE;

			part = TNY_MIME_PART(tny_iterator_get_current (iter));

			if (!part) {
				g_warning ("%s: not a valid mime part", __FUNCTION__);
				continue;
			}

			/* it's a message --> ignore */
			if (part && TNY_IS_MSG (part)) {
				g_object_unref (part);
				tny_iterator_next (iter);
				continue;
			}			

			/* we need to strdown the content type, because
			 * tny_mime_part_has_content_type does not do it...
			 */
			content_type = g_ascii_strdown (tny_mime_part_get_content_type (part), -1);
			
			/* mime-parts with a content-disposition header (either 'inline' or 'attachment')
			 * and a 'name=' thingy cannot be body parts
                         */
				
			tmp = modest_tny_mime_part_get_header_value (part, "Content-Disposition");
			if (tmp) {
				gchar *content_disp = g_ascii_strdown(tmp, -1);
				g_free (tmp);
				has_content_disp_name = g_strstr_len (content_disp, strlen(content_disp), "name=") != NULL;
				g_free (content_disp);
			}
			
			if (g_str_has_prefix (content_type, desired_mime_type) && !has_content_disp_name) {
				/* we found the desired mime-type! */
				g_free (content_type);
				break;

			} else 	if (g_str_has_prefix(content_type, "multipart")) {

				/* multipart? recurse! */
				g_object_unref (part);
				g_free (content_type);
				part = modest_tny_msg_find_body_part_from_mime_part (part, want_html);
				if (part)
					break;
			} else
				g_free (content_type);
			
			if (part) {
				g_object_unref (G_OBJECT(part));
				part = NULL;
			}
			
			tny_iterator_next (iter);
			
		} while (!tny_iterator_is_done(iter));
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
	g_return_val_if_fail (msg && TNY_IS_MSG(msg), NULL);
	
	return modest_tny_msg_find_body_part_from_mime_part (TNY_MIME_PART(msg),
							     want_html);
}


#define MODEST_TNY_MSG_PARENT_UID "parent-uid"

static TnyMsg *
create_reply_forward_mail (TnyMsg *msg, TnyHeader *header, const gchar *from,
			   const gchar *signature, gboolean is_reply,
			   guint type /*ignored*/, GList *attachments)
{
	TnyMsg *new_msg;
	TnyHeader *new_header;
	gchar *new_subject;
	TnyMimePart *body = NULL;
	ModestFormatter *formatter;
	gchar *subject_prefix;
	gboolean no_text_part;
	
	if (header)
		g_object_ref (header);
	else
		header = tny_msg_get_header (msg);

	/* Get body from original msg. Always look for the text/plain
	   part of the message to create the reply/forwarded mail */
	if (msg != NULL)
		body   = modest_tny_msg_find_body_part (msg, FALSE);
	
	if (modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_PREFER_FORMATTED_TEXT,
				  NULL))
		formatter = modest_formatter_new ("text/html", signature);
	else
		formatter = modest_formatter_new ("text/plain", signature);
	

	/* if we don't have a text-part */
	no_text_part = (!body) || (strcmp (tny_mime_part_get_content_type (body), "text/html")==0);
	
	/* when we're reply, include the text part if we have it, or nothing otherwise. */
	if (is_reply)
		new_msg = modest_formatter_quote  (formatter, no_text_part ? NULL: body, header,
						    attachments);
	else {
		/* for attachements; inline if there is a text part, and include the
		 * full old mail if there was none */
		if (no_text_part) 
			new_msg = modest_formatter_attach (formatter, msg, header);
		else 
			new_msg = modest_formatter_inline  (formatter, body, header,
							    attachments);
	}
	
	g_object_unref (G_OBJECT(formatter));
	if (body)
		g_object_unref (G_OBJECT(body));
	
	/* Fill the header */
	new_header = tny_msg_get_header (new_msg);	
	tny_header_set_from (new_header, from);
	tny_header_set_replyto (new_header, from);

	/* Change the subject */
	if (is_reply)
		subject_prefix = g_strconcat (_("mail_va_re"), ":", NULL);
	else
		subject_prefix = g_strconcat (_("mail_va_fw"), ":", NULL);
	new_subject =
		(gchar *) modest_text_utils_derived_subject (tny_header_get_subject(header),
							     subject_prefix);
	g_free (subject_prefix);
	tny_header_set_subject (new_header, (const gchar *) new_subject);
	g_free (new_subject);
	
	/* get the parent uid, and set it as a gobject property on the new msg */
	if (new_msg) {
		gchar* parent_uid = modest_tny_folder_get_header_unique_id (header);
		g_object_set_data_full (G_OBJECT(new_msg), MODEST_TNY_MSG_PARENT_UID,
					parent_uid, g_free);
	}

	/* set modest as the X-Mailer
	 * we could this in the platform factory, but then the header
	 * would show up before all the others.
	 */
	tny_mime_part_set_header_pair (TNY_MIME_PART (msg), "X-Mailer", "Modest "
				       VERSION);

	/* Clean */
	g_object_unref (G_OBJECT (new_header));
	g_object_unref (G_OBJECT (header));
	/* ugly to unref it here instead of in the calling func */

	return new_msg;
}

const gchar*
modest_tny_msg_get_parent_uid (TnyMsg *msg)
{
	g_return_val_if_fail (msg && TNY_IS_MSG(msg), NULL);
	
	return g_object_get_data (G_OBJECT(msg), MODEST_TNY_MSG_PARENT_UID);
}



static void
add_if_attachment (gpointer data, gpointer user_data)
{
	TnyMimePart *part;
	GList **attachments_list;

	part = TNY_MIME_PART (data);
	attachments_list = ((GList **) user_data);

	if ((tny_mime_part_is_attachment (part))||(TNY_IS_MSG (part))) {
		*attachments_list = g_list_prepend (*attachments_list, part);
		g_object_ref (part);
	}
}

TnyMsg* 
modest_tny_msg_create_forward_msg (TnyMsg *msg, 
				   const gchar *from,
				   const gchar *signature,
				   ModestTnyMsgForwardType forward_type)
{
	TnyMsg *new_msg;
	TnyList *parts = NULL;
	GList *attachments_list = NULL;

	g_return_val_if_fail (msg && TNY_IS_MSG(msg), NULL);
	
	/* Add attachments */
	parts = TNY_LIST (tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);
	tny_list_foreach (parts, add_if_attachment, &attachments_list);

	new_msg = create_reply_forward_mail (msg, NULL, from, signature, FALSE, forward_type,
					     attachments_list);
	add_attachments (TNY_MIME_PART (new_msg), attachments_list, FALSE);

	/* Clean */
	if (attachments_list) {
		g_list_foreach (attachments_list, (GFunc) g_object_unref, NULL);
		g_list_free (attachments_list);
	}
	g_object_unref (G_OBJECT (parts));

	return new_msg;
}



static gint
count_addresses (const gchar* addresses)
{
	gint count = 1;

	if (!addresses)
		return 0;
	
	while (*addresses) {
		if (*addresses == ',' || *addresses == ';')
			++count;
		++addresses;
	}
	
	return count;
}


/* get the new To:, based on the old header,
 * result is newly allocated or NULL in case of error
 * */
static gchar*
get_new_to (TnyMsg *msg, TnyHeader *header, const gchar* from,
	    ModestTnyMsgReplyMode reply_mode)
{
	const gchar* old_reply_to;
	const gchar* old_from;
	gchar* new_to;
	
	/* according to RFC2369 (http://www.faqs.org/rfcs/rfc2369.html), we
	 * can identify Mailing-List posts by the List-Help header.
	 * for mailing lists, both the Reply-To: and From: should be included
	 * in the new To:; for now, we're ignoring List-Post
	 */
	gchar* list_help = modest_tny_mime_part_get_header_value (TNY_MIME_PART(msg), 
								  "List-Help");
	gboolean is_mailing_list = (list_help != NULL);
	g_free (list_help);


	/* reply to sender, use ReplyTo or From */
	//old_reply_to = tny_header_get_replyto (header);
	old_reply_to = modest_tny_mime_part_get_header_value (TNY_MIME_PART(msg), 
							      "Reply-To"); 
	old_from     = tny_header_get_from (header);
	
	if (!old_from &&  !old_reply_to) {
		g_warning ("%s: failed to get either Reply-To: or From: from header",
			   __FUNCTION__);
		return NULL;
	}
	
	/* for mailing lists, use both Reply-To and From if we did a
	 * 'Reply All:'
	 * */
	if (is_mailing_list && reply_mode == MODEST_TNY_MSG_REPLY_MODE_ALL &&
	    old_reply_to && old_from && strcmp (old_from, old_reply_to) != 0)
		new_to = g_strjoin (",", old_reply_to, old_from, NULL);
	else
		/* otherwise use either Reply-To: (preferred) or From: */
		new_to = g_strdup (old_reply_to ? old_reply_to : old_from);

	/* in case of ReplyAll, we need to add the Recipients in the old To: */
	if (reply_mode == MODEST_TNY_MSG_REPLY_MODE_ALL) {
		const gchar *old_to = tny_header_get_to (header);
		if (!old_to) 
			g_warning ("%s: no To: address found in source mail",
				   __FUNCTION__);
		else {
			/* append the old To: */
			gchar *tmp = g_strjoin (",", new_to, old_to, NULL);
			g_free (new_to);
			new_to = tmp;
		}

		/* remove duplicate entries */
		gchar *tmp = modest_text_utils_remove_duplicate_addresses (new_to);
		g_free (new_to);
		new_to = tmp;
		
		/* now, strip me (the new From:) from the new_to, but only if
		 * there are >1 addresses there */
		if (count_addresses (new_to) > 1) {
			gchar *tmp = modest_text_utils_remove_address (new_to, from);
			g_free (new_to);
			new_to = tmp;
		}
	}

	return new_to;
}


/* get the new Cc:, based on the old header,
 * result is newly allocated or NULL in case of error */
static gchar*
get_new_cc (TnyHeader *header, const gchar* from)
{
	const gchar *old_cc;

	old_cc = tny_header_get_cc (header);
	if (!old_cc)
		return NULL;

	/* remove me (the new From:) from the Cc: list */
	return modest_text_utils_remove_address (old_cc, from);
}


TnyMsg* 
modest_tny_msg_create_reply_msg (TnyMsg *msg,
				 TnyHeader *header,
				 const gchar *from,
				 const gchar *signature,
				 ModestTnyMsgReplyType reply_type,
				 ModestTnyMsgReplyMode reply_mode)
{
	TnyMsg *new_msg = NULL;
	TnyHeader *new_header;
	gchar *new_to = NULL;
	TnyList *parts = NULL;
	GList *attachments_list = NULL;

	g_return_val_if_fail (msg && TNY_IS_MSG(msg), NULL);
	
	/* Add attachments */
	if (msg != NULL) {
		parts = TNY_LIST (tny_simple_list_new());
		tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);
		tny_list_foreach (parts, add_if_attachment, &attachments_list);
	}

	new_msg = create_reply_forward_mail (msg, header, from, signature, TRUE, reply_type,
					     attachments_list);
	if (attachments_list) {
		g_list_foreach (attachments_list, (GFunc) g_object_unref, NULL);
		g_list_free (attachments_list);
	}
	if (parts)
		g_object_unref (G_OBJECT (parts));

	/* Fill the header */
	if (header)
		g_object_ref (header);
	else
		header = tny_msg_get_header (msg);

	
	new_header = tny_msg_get_header(new_msg);
	new_to = get_new_to (msg, header, from, reply_mode);
	if (!new_to)
		g_warning ("%s: failed to get new To:", __FUNCTION__);
	else {
		tny_header_set_to (new_header, new_to);
		g_free (new_to);
	}	
	
	if (reply_mode == MODEST_TNY_MSG_REPLY_MODE_ALL) {
		gchar *new_cc = get_new_cc (header, from);
		if (new_cc) { 
			tny_header_set_cc (new_header, new_cc);
			g_free (new_cc);
		}
	}
	
	/* Clean */
	g_object_unref (G_OBJECT (new_header));
	g_object_unref (G_OBJECT (header));

	return new_msg;
}


static gboolean
is_ascii(const gchar *s)
{
	if (!s)
		return TRUE;
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

guint64
modest_tny_msg_estimate_size (const gchar *plain_body, const gchar *html_body,
			      guint64 parts_count,
			      guint64 parts_size)
{
	guint64 result;

	/* estimation of headers size */
	result = 1024;

	/* We add a 20% of size due to the increase in 7bit encoding */
	if (plain_body) {
		result += strlen (plain_body) * 120 / 100;
	}
	if (html_body) {
		result += strlen (html_body) * 120 / 100;
	}

	/* 256 bytes per additional part because of their headers */
	result += parts_count * 256;

	/* 150% of increase per encoding */
	result += parts_size * 3 / 2;

	return result;
}

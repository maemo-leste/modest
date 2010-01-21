/* Copyright (c) 2006, 2007, 2008 Nokia Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

#include "modest-tny-mime-part.h"
#include <tny-simple-list.h>
#include <tny-msg.h>
#include <string.h> /* for strlen */
#include <modest-tny-msg.h>
#include "modest-text-utils.h"


gchar*
modest_tny_mime_part_get_header_value (TnyMimePart *part, const gchar *header)
{
	TnyList *pairs;
	TnyIterator *iter;
	gchar *val;
	
	g_return_val_if_fail (part && TNY_IS_MIME_PART(part), NULL);
	g_return_val_if_fail (header, NULL);

	pairs = tny_simple_list_new ();
	
	tny_mime_part_get_header_pairs (part, pairs);
	iter = tny_list_create_iterator (pairs);

	val = NULL;
	while (!tny_iterator_is_done(iter) && !val) {

		TnyPair *pair = (TnyPair*)tny_iterator_get_current(iter);
		if (strcasecmp (header, tny_pair_get_name(pair)) == 0)
			val = g_strdup (tny_pair_get_value(pair));
		g_object_unref (pair);		

		tny_iterator_next (iter);
	}

	g_object_unref (pairs);
	g_object_unref (iter);

	return val;
}




/* we consider more things attachments than tinymail does...
 */
gboolean
modest_tny_mime_part_is_attachment_for_modest (TnyMimePart *part)
{
	gchar *tmp, *content_type;
	gboolean has_content_disp_name = FALSE;

	g_return_val_if_fail (part && TNY_IS_MIME_PART(part), FALSE);

	/* purged attachments were attachments in the past, so they're
	 * still attachments */
	if (tny_mime_part_is_purged (part))
		return TRUE; 
	
	/* if tinymail thinks it's an attachment, it is. One exception: if it's
	 * a multipart and it's not a message/rfc822 it cannot be an attahcment */
	if (tny_mime_part_is_attachment (part)) {
		if (!TNY_IS_MSG (part)) {
			const gchar *content_type;
			gchar *down_content_type;
			gboolean is_attachment;

			content_type = tny_mime_part_get_content_type (part);
			down_content_type = g_ascii_strdown (content_type, -1);

			is_attachment = !g_str_has_prefix (down_content_type, "multipart/");
			g_free (down_content_type);
			return is_attachment;
		} else {
			return TRUE;
		}
	}

	/* if the mime part is a message itself (ie. embedded), it's an attachment */
	if (TNY_IS_MSG (part))
		return TRUE;
	
	tmp = modest_tny_mime_part_get_header_value (part, "Content-Disposition");
	if (tmp) {
		/* If the Content-Disposition header contains a "name"
		 * parameter, treat the mime part as an attachment */
		gchar *content_disp = g_ascii_strdown(tmp, -1);
		gint len = strlen (content_disp);
		const gchar *substr = g_strstr_len (content_disp, len, "name");
		if (substr != NULL) {
			gint substrlen = len - (substr - content_disp);
			/* The parameter can appear in muliple
			 * ways. See RFC 2231 for details */
			has_content_disp_name =
				g_strstr_len (substr, substrlen, "name=") != NULL ||
				g_strstr_len (substr, substrlen, "name*=") != NULL ||
				g_strstr_len (substr, substrlen, "name*0=") != NULL ||
				g_strstr_len (substr, substrlen, "name*0*=") != NULL;
		}
		g_free (tmp);
		g_free (content_disp);
	}
		
	/* if it doesn't have a content deposition with a name= attribute, it's not an attachment */
	if (!has_content_disp_name)
		return FALSE;
	
	/* ok, it must be content-disposition "inline" then, because "attachment"
	 * is already handle above "...is_attachment". modest consider these "inline" things
         * attachments as well, unless they are embedded images for html mail 
	 */
	content_type = g_ascii_strdown (tny_mime_part_get_content_type (part), -1);
	if (!g_str_has_prefix (content_type, "image/")) {
		g_free (content_type);
		return TRUE; /* it's not an image, so it must be an attachment */
	}
	g_free (content_type);


	/* now, if it's an inline-image, and it has a content-id or location, we
	 * we guess it's an inline image, and not an attachment */
	if (tny_mime_part_get_content_id (part) || tny_mime_part_get_content_location(part))
		return FALSE;
		
	/* in other cases... */
	return TRUE;
}

gboolean
modest_tny_mime_part_is_msg (TnyMimePart *part)
{
	const gchar *content_type;
	gchar *down_content_type;

	if (!TNY_IS_MSG (part))
		return FALSE;

	content_type = tny_mime_part_get_content_type (part);
	down_content_type = g_ascii_strdown (content_type, -1);
	if ((g_str_has_prefix (down_content_type, "message/rfc822") ||
	     g_str_has_prefix (down_content_type, "multipart/") ||
	     g_str_has_prefix (down_content_type, "text/plain") ||
	     g_str_has_prefix (down_content_type, "text/html"))) {
		g_free (down_content_type);
		return TRUE;
	} else {
		g_free (down_content_type);
		return FALSE;
	}
}

void 
modest_tny_mime_part_to_string (TnyMimePart *part, gint indent)
{
	return;
	gint i;
	GString *indent_prefix;
	TnyList *list, *pairs_list;
	TnyIterator *iter, *pairs_iter;
	
	indent_prefix = g_string_new ("");
	for (i = 0; i < indent; i++) {
		indent_prefix = g_string_append_c (indent_prefix, ' ');
	}

	if (TNY_IS_MSG (part)) {
		TnyHeader *header;

		header = tny_msg_get_header (TNY_MSG (part));
		g_print ("(%s(MSG))\n", indent_prefix->str);
		g_object_unref (header);
	}

	list = tny_simple_list_new ();
	tny_mime_part_get_parts (part, list);
	pairs_list = tny_simple_list_new ();
	tny_mime_part_get_header_pairs (part, pairs_list);
	g_print ("%s(content=%s parts=%d location=%s)\n", indent_prefix->str, 
		 tny_mime_part_get_content_type (part),
		 tny_list_get_length (list),
		 tny_mime_part_get_content_location (part));
	for (pairs_iter = tny_list_create_iterator (pairs_list);
	     !tny_iterator_is_done (pairs_iter);
	     tny_iterator_next (pairs_iter)) {
		TnyPair *pair = TNY_PAIR (tny_iterator_get_current (pairs_iter));
		g_print ("%s(%s:%s)\n", indent_prefix->str, tny_pair_get_name (pair), tny_pair_get_value (pair));
		g_object_unref (pair);
	}
	for (iter = tny_list_create_iterator (list);
	     !tny_iterator_is_done (iter);
	     tny_iterator_next (iter)) {
		TnyMimePart *child;
		child = (TnyMimePart *) tny_iterator_get_current (iter);
		modest_tny_mime_part_to_string (child, indent + 3);
		g_object_unref (child);
	}
	g_object_unref (iter);
	g_object_unref (list);

	g_string_free (indent_prefix, TRUE);
}

gchar *
modest_tny_mime_part_get_headers_content_type (TnyMimePart *part)
{
	gchar *header_content_type;
	gchar *suffix;

	g_return_val_if_fail (TNY_IS_MIME_PART (part), NULL);

	header_content_type = modest_tny_mime_part_get_header_value (part, "Content-Type");

	/* See RFC2045 sec 5.2 */
	if (!header_content_type)
		return g_strdup ("text/plain; charset=us-ascii");

	header_content_type = g_strstrip (header_content_type);

	/* remove the ; suffix */
	suffix = index (header_content_type, ';');
	if (suffix)
		suffix[0] = '\0';

	return g_ascii_strdown (header_content_type, -1);
}

gchar *
modest_tny_mime_part_get_content_type (TnyMimePart *part)
{
	const gchar *content_type;
	gchar *retval = NULL;

	g_return_val_if_fail (TNY_IS_MIME_PART (part), NULL);
	content_type = tny_mime_part_get_content_type (part);

	if (g_str_has_prefix (content_type, "message/rfc822")) {
		retval = modest_tny_mime_part_get_headers_content_type (part);
	} 

	if (retval == NULL) {
		retval = g_ascii_strdown (content_type, -1);
	}

	return retval;
}

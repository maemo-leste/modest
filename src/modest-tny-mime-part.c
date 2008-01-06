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
	
	/* if tinymail thinks it's an attachment, it definitely is */
	if (tny_mime_part_is_attachment (part))
		return TRUE; 

	/* if the mime part is a message itself (ie. embedded), it's an attachment */
	if (TNY_IS_MSG (part))
		return TRUE;
	
	tmp = modest_tny_mime_part_get_header_value (part, "Content-Disposition");
	if (tmp) {
		gchar *content_disp = g_ascii_strdown(tmp, -1);
		g_free (tmp);
		has_content_disp_name = g_strstr_len (content_disp, strlen(content_disp), "name=") != NULL;
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



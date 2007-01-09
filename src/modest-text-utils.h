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


/* modest-text-utils.h */

#ifndef __MODEST_TEXT_UTILS_H__
#define __MODEST_TEXT_UTILS_H__

#include <time.h>
#include <glib.h>

/**
 * modest_text_utils_derived_subject:
 * @subject: a string which contains the original subject
 * @prefix: the prefix for the new subject (such as 'Re:' or 'Fwd:'),
 *           must not be NULL
 *
 * create a 'derived' subject line for eg. replies and forwards 
 * 
 * Returns: a newly allocated string containing the resulting subject
 * subject == NULL, then @prefix " " will be returned
 */
gchar* modest_text_utils_derived_subject (const gchar *subject, 
					  const gchar* prefix);


/**
 * modest_text_utils_quote:
 * @text: a non-NULL string which contains the message to quote
 * @from: a non-NULL  sender of the original message
 * @content_type: the non-NULL content type for the quoting, e.g. "text/html"
 * @sent_date: sent date/time of the original message
 * @limit: specifies the maximum characters per line in the quoted text
 * 
 * quote an existing message
 * 
 * Returns: a newly allocated string containing the quoted message
 */
gchar* modest_text_utils_quote (const gchar *text, 
				const gchar *content_type,
			        const gchar *from,
			        const time_t sent_date, 
				int limit);


/**
 * modest_text_utils_cited_text:
 * @from: sender of the message
 * @sent_date: the sent date of the original message
 * @text: the text of the original message
 *
 * cite the text in a message
 * 
 * Returns: a newly allocated string containing the cited text
 */
gchar* modest_text_utils_cite (const gchar *text,
			       const gchar *content_type,
			       const gchar *from,
			       time_t sent_date);

/**
 * modest_text_utils_inlined_text
 * @from: the non-NULL sender of the original message
 * @sent_date: sent date/time of the original message
 * @to: 
 * @subject: 
 * @text: 
 *
 * creates a new string with the "Original message" text prepended to
 * the text passed as argument and some data of the header
 * 
 * Returns: a newly allocated string containing the quoted message
 */
gchar*   modest_text_utils_inline (const gchar *text,
				   const gchar *content_type,
				   const gchar *from,
				   time_t sent_date,
				   const gchar *to,
				   const gchar *subject);

/**
 * modest_text_utils_remove_address
 * @address_list: none-NULL string with a comma-separated list of email addresses
 * @address: an specific e-mail address 
 *
 * remove a specific address from a list of email addresses; if @address
 * is NULL, returns an unchanged @address_list
 * 
 * Returns: a newly allocated string containing the new list, or NULL
 * in case of error or the original @address_list was NULL
 */
gchar*   modest_text_utils_remove_address (const gchar *address_list, 
					   const gchar *address);

/**
 * modest_text_utils_convert_to_html:
 * @txt: a string which contains the message to quote
 *
 * convert plain text (utf8) into html
 * 
 * Returns: a newly allocated string containing the html
 */
gchar*  modest_text_utils_convert_to_html (const gchar *txt);


/**
 * modest_text_utils_strftime:
 * @s:
 * @max:
 * @fmt:
 * @tm
 *
 * this is just an alias for strftime(3), so we can use that without
 * getting warning from gcc
 * 
 * Returns: a formatted string of max length @max in @s
 */
size_t modest_text_utils_strftime(char *s, size_t max, const char  *fmt, const  struct tm *tm);



/**
 * modest_text_utils_get_display_addres:
 * @address: original address (UTF8 string)
 *
 * make a 'display address' from an address:
 * "Foo Bar <foo@bar.cx> (Bla)" --> "Foo Bar"
 * ie. removes "<...>" and "(...)" parts
 * the change is in-place; removes leading/trailing whitespace
 * 
 * Returns: the new address. The string is *not* newly allocated.
 * NULL in case of error
 */
gchar* modest_text_utils_get_display_address (gchar *address);


/**
 * modest_text_utils_get_subject_prefix_len:
 * @subject: original subject (UTF8 string)
 *
 * determine the length of the "Re:/RE:/Fwd:" prefix in an e-mail address
 * 
 * Returns: the length of the  prefix, or 0 if there is none
 */
gint modest_text_utils_get_subject_prefix_len (const gchar *subject);


/**
 * modest_text_utils_utf8_strcmp:
 * @s1: the first string
 * @s2: the second string
 * @insensitive: should the comparison be case-insensitive?
 *
 * a strcmp that is NULL-safe, can deal with UTF8 and case-insensitive comparison 
 *
 * Returns: an integer less than, equal to, or greater than zero if s1 is found,
 * respectively, to be less than, to match, or be greater than s2.
 */
gint modest_text_utils_utf8_strcmp (const gchar* s1, const gchar *s2, gboolean insensitive);



/**
 * modest_text_utils_get_display_date:
 * @date: the date to display
 *
 * get a string representation for a date.
 * 
 * 
 * Returns: the new display date. The string is *not* newly allocated,
 * but is a static buffer (for optimization reasons). Therefore, when
 * you call this function, you will loose the data.
 */
const gchar* modest_text_utils_get_display_date (time_t date);

#endif /* __MODEST_TEXT_UTILS_H__ */

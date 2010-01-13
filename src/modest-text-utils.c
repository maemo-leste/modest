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



#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /*_GNU_SOURCE*/
#include <string.h> /* for strcasestr */


#include <glib.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <regex.h>
#include <modest-tny-platform-factory.h>
#include <modest-text-utils.h>
#include <modest-runtime.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

/* defines */
#define FORWARD_STRING _("mcen_ia_editor_original_message")
#define FROM_STRING _("mail_va_from")
#define SENT_STRING _("mcen_fi_message_properties_sent")
#define TO_STRING _("mail_va_to")
#define	SUBJECT_STRING _("mail_va_subject")
#define EMPTY_STRING ""

/*
 * do the hyperlinkification only for texts < 50 Kb,
 * as it's quite slow. Without this, e.g. mail with
 * an uuencoded part (which is not recognized as attachment,
 * will hang modest
 */
#define HYPERLINKIFY_MAX_LENGTH (1024*50)

/*
 * we need these regexps to find URLs in plain text e-mails
 */
typedef struct _url_match_pattern_t url_match_pattern_t;
struct _url_match_pattern_t {
	gchar   *regex;
	regex_t *preg;
	gchar   *prefix;
};

typedef struct _url_match_t url_match_t;
struct _url_match_t {
	guint offset;
	guint len;
	const gchar* prefix;
};


/*
 * we mark the ampersand with \007 when converting text->html
 * because after text->html we do hyperlink detecting, which
 * could be screwed up by the ampersand.
 * ie. 1<3 ==> 1\007lt;3
 */
#define MARK_AMP '\007'
#define MARK_AMP_STR "\007"

/* mark &amp; separately, because they are parts of urls.
 * ie. a&b => a\006amp;b, but a>b => a\007gt;b
 *
 * we need to handle '&' separately, because it can be part of URIs
 * (as in href="http://foo.bar?a=1&b=1"), so inside those URIs
 * we need to re-replace \006amp; with '&' again, while outside uri's
 * it will be '&amp;'
 * 
 * yes, it's messy, but a consequence of doing text->html first, then hyperlinkify
 */
#define MARK_AMP_URI '\006'
#define MARK_AMP_URI_STR "\006"


/* note: match MARK_AMP_URI_STR as well, because after txt->html, a '&' will look like $(MARK_AMP_URI_STR)"amp;" */
#define MAIL_VIEWER_URL_MATCH_PATTERNS  {				\
	{ "(feed:|)(file|rtsp|http|ftp|https|mms|mmsh|webcal|feed|rtsp|rdp|lastfm|sip)://[-a-z0-9_$.+!*(),;:@%=\?/~#&" MARK_AMP_URI_STR \
			"]+[-a-z0-9_$%&" MARK_AMP_URI_STR "=?/~#]",	\
	  NULL, NULL },\
	{ "www\\.[-a-z0-9_$.+!*(),;:@%=?/~#" MARK_AMP_URI_STR "]+[-a-z0-9_$%" MARK_AMP_URI_STR "=?/~#]",\
			NULL, "http://" },				\
	{ "ftp\\.[-a-z0-9_$.+!*(),;:@%=?/~#" MARK_AMP_URI_STR "]+[-a-z0-9_$%" MARK_AMP_URI_STR "=?/~#]",\
	  NULL, "ftp://" },\
	{ "(jabberto|voipto|sipto|sip|chatto|skype|xmpp):[-_a-z@0-9.+]+", \
	   NULL, NULL},						    \
	{ "mailto:[-_a-z0-9.\\+]+@[-_a-z0-9.]+",		    \
	  NULL, NULL},\
	{ "[-_a-z0-9.\\+]+@[-_a-z0-9.]+",\
	  NULL, "mailto:"}\
	}

const gchar account_title_forbidden_chars[] = {
	'\\', '/', ':', '*', '?', '\'', '<', '>', '|', '^'
};
const gchar folder_name_forbidden_chars[] = {
	'<', '>', ':', '\'', '/', '\\', '|', '?', '*', '^', '%', '$', '#', '&'
};
const gchar user_name_forbidden_chars[] = {
	'<', '>'
};
const guint ACCOUNT_TITLE_FORBIDDEN_CHARS_LENGTH = G_N_ELEMENTS (account_title_forbidden_chars);
const guint FOLDER_NAME_FORBIDDEN_CHARS_LENGTH = G_N_ELEMENTS (folder_name_forbidden_chars);
const guint USER_NAME_FORBIDDEN_CHARS_LENGTH = G_N_ELEMENTS (user_name_forbidden_chars);

/* private */
static gchar*   cite                    (const time_t sent_date, const gchar *from);
static void     hyperlinkify_plain_text (GString *txt, gint offset);
static gint     cmp_offsets_reverse     (const url_match_t *match1, const url_match_t *match2);
static GSList*  get_url_matches         (GString *txt, gint offset);

static GString* get_next_line           (const char *b, const gsize blen, const gchar * iter);
static int      get_indent_level        (const char *l);
static void     unquote_line            (GString * l, const gchar *quote_symbol);
static void     append_quoted           (GString * buf, const gchar *quote_symbol,
					 const int indent, const GString * str, 
					 const int cutpoint);
static int      get_breakpoint_utf8     (const gchar * s, const gint indent, const gint limit);
static int      get_breakpoint_ascii    (const gchar * s, const gint indent, const gint limit);
static int      get_breakpoint          (const gchar * s, const gint indent, const gint limit);

static gchar*   modest_text_utils_quote_plain_text (const gchar *text, 
						    const gchar *cite, 
						    const gchar *signature,
						    GList *attachments, 
						    int limit);

static gchar*   modest_text_utils_quote_html       (const gchar *text, 
						    const gchar *cite,
						    const gchar *signature,
						    GList *attachments,
						    int limit);
static gchar*   get_email_from_address (const gchar *address);
static void     remove_extra_spaces (gchar *string);



/* ******************************************************************* */
/* ************************* PUBLIC FUNCTIONS ************************ */
/* ******************************************************************* */

gchar *
modest_text_utils_quote (const gchar *text, 
			 const gchar *content_type,
			 const gchar *signature,
			 const gchar *from,
			 const time_t sent_date, 
			 GList *attachments,
			 int limit)
{
	gchar *retval, *cited;

	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);

	cited = cite (sent_date, from);
	
	if (content_type && strcmp (content_type, "text/html") == 0)
		/* TODO: extract the <body> of the HTML and pass it to
		   the function */
		retval = modest_text_utils_quote_html (text, cited, signature, attachments, limit);
	else
		retval = modest_text_utils_quote_plain_text (text, cited, signature, attachments, limit);
	
	g_free (cited);
	
	return retval;
}


gchar *
modest_text_utils_cite (const gchar *text,
			const gchar *content_type,
			const gchar *signature,
			const gchar *from,
			time_t sent_date)
{
	gchar *retval;
	gchar *tmp_sig;
	
	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);
	
	if (!signature) {
		tmp_sig = g_strdup (text);
	} else {
		tmp_sig = g_strconcat (text, "\n", MODEST_TEXT_UTILS_SIGNATURE_MARKER, "\n", signature, NULL);
	}

	if (strcmp (content_type, "text/html") == 0) {
		retval = modest_text_utils_convert_to_html_body (tmp_sig, -1, TRUE);
		g_free (tmp_sig);
	} else {
		retval = tmp_sig;
	}

	return retval;
}

static gchar *
forward_cite (const gchar *from,
	      const gchar *sent,
	      const gchar *to,
	      const gchar *subject)
{
	g_return_val_if_fail (sent, NULL);
	
	return g_strdup_printf ("%s\n%s %s\n%s %s\n%s %s\n%s %s\n", 
				FORWARD_STRING, 
				FROM_STRING, (from)?from:"",
				SENT_STRING, sent,
				TO_STRING, (to)?to:"",
				SUBJECT_STRING, (subject)?subject:"");
}

gchar * 
modest_text_utils_inline (const gchar *text,
			  const gchar *content_type,
			  const gchar *signature,
			  const gchar *from,
			  time_t sent_date,
			  const gchar *to,
			  const gchar *subject)
{
	gchar sent_str[101];
	gchar *cited;
	gchar *retval;
	
	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);
	
	modest_text_utils_strftime (sent_str, 100, "%c", sent_date);

	cited = forward_cite (from, sent_str, to, subject);
	
	if (content_type && strcmp (content_type, "text/html") == 0)
		retval = modest_text_utils_quote_html (text, cited, signature, NULL, 80);
	else
		retval = modest_text_utils_quote_plain_text (text, cited, signature, NULL, 80);
	
	g_free (cited);
	return retval;
}

/* just to prevent warnings:
 * warning: `%x' yields only last 2 digits of year in some locales
 */
gsize
modest_text_utils_strftime(char *s, gsize max, const char *fmt, time_t timet)
{
        struct tm tm;

	/* To prevent possible problems in strftime that could leave
	   garbage in the s variable */
	if (s)
		s[0] = '\0';
	else
		return 0;

	/* does not work on old maemo glib: 
	 *   g_date_set_time_t (&date, timet);
	 */
	localtime_r (&timet, &tm);
	return strftime(s, max, fmt, &tm);
}

gchar *
modest_text_utils_derived_subject (const gchar *subject, gboolean is_reply)
{
	gchar *tmp, *subject_dup, *retval, *prefix;
	const gchar *untranslated_prefix;
	gint prefix_len, untranslated_prefix_len;
	gboolean translated_found = FALSE;
	gboolean first_time;

	if (!subject || subject[0] == '\0')
		subject = _("mail_va_no_subject");

	subject_dup = g_strdup (subject);
	tmp = g_strchug (subject_dup);

	prefix = (is_reply) ? _("mail_va_re") : _("mail_va_fw");
	prefix = g_strconcat (prefix, ":", NULL);
	prefix_len = g_utf8_strlen (prefix, -1);

	untranslated_prefix =  (is_reply) ? "Re:" : "Fw:";
	untranslated_prefix_len = 3;

	/* We do not want things like "Re: Re: Re:" or "Fw: Fw:" so
	   delete the previous ones */
	first_time = TRUE;
	do {
		if (g_str_has_prefix (tmp, prefix)) {
			tmp += prefix_len;
			tmp = g_strchug (tmp);
			/* Do not consider translated prefixes in the
			   middle of a Re:Re:..Re: like sequence */
			if (G_UNLIKELY (first_time))
				translated_found = TRUE;
		} else if (g_str_has_prefix (tmp, untranslated_prefix)) {
			tmp += untranslated_prefix_len;
			tmp = g_strchug (tmp);
		} else {
			gchar *prefix_down, *tmp_down;

			/* We need this to properly check the cases of
			   some clients adding FW: instead of Fw: for
			   example */
			prefix_down = g_utf8_strdown (prefix, -1);
			tmp_down = g_utf8_strdown (tmp, -1);
			if (g_str_has_prefix (tmp_down, prefix_down)) {
				tmp += prefix_len;
				tmp = g_strchug (tmp);
				g_free (prefix_down);
				g_free (tmp_down);
			} else {
				g_free (prefix_down);
				g_free (tmp_down);
				break;
			}
		}
		first_time = FALSE;
	} while (tmp);

	if (!g_strcmp0 (subject, tmp)) {
		/* normal case */
		retval = g_strdup_printf ("%s %s", untranslated_prefix, tmp);
	} else {
		if (translated_found) {
			/* Found a translated prefix, i.e, "VS:" in Finish */
			retval = g_strdup_printf ("%s %s", prefix, tmp);
		} else {
			retval = g_strdup_printf ("%s %s", untranslated_prefix, tmp);
		}
	}
	g_free (subject_dup);
	g_free (prefix);

	return retval;
}


/* Performs a case-insensitive strstr for ASCII strings */
static const gchar *
ascii_stristr(const gchar *haystack, const gchar *needle)
{
	int needle_len;
	int haystack_len;
	const gchar *pos;
	const gchar *max_pos;

	if (haystack == NULL || needle == NULL) {
		return haystack;  /* as in strstr */
	}

	needle_len = strlen(needle);

	if (needle_len == 0) {
		return haystack;  /* as in strstr */
	}

	haystack_len = strlen(haystack);
	max_pos = haystack + haystack_len - needle_len;

	for (pos = haystack; pos <= max_pos; pos++) {
		if (g_ascii_strncasecmp (pos, needle, needle_len) == 0) {
			return pos;
		}
	}

	return NULL;
}


gchar*
modest_text_utils_remove_address (const gchar *address_list, const gchar *address)
{
	gchar *dup, *token, *ptr = NULL, *result;
	GString *filtered_emails;
	gchar *email_address;

	g_return_val_if_fail (address_list, NULL);

	if (!address)
		return g_strdup (address_list);

	email_address = get_email_from_address (address);

	/* search for substring */
	if (!ascii_stristr ((const char *) address_list, (const char *) email_address)) {
		g_free (email_address);
		return g_strdup (address_list);
	}

	dup = g_strdup (address_list);
	filtered_emails = g_string_new (NULL);

	token = strtok_r (dup, ",", &ptr);

	while (token != NULL) {
		/* Add to list if not found */
		if (!ascii_stristr ((const char *) token, (const char *) email_address)) {
			if (filtered_emails->len == 0)
				g_string_append_printf (filtered_emails, "%s", g_strstrip (token));
			else
				g_string_append_printf (filtered_emails, ",%s", g_strstrip (token));
		}
		token = strtok_r (NULL, ",", &ptr);
	}
	result = filtered_emails->str;

	/* Clean */
	g_free (email_address);
	g_free (dup);
	g_string_free (filtered_emails, FALSE);

	return result;
}


gchar*
modest_text_utils_remove_duplicate_addresses (const gchar *address_list)
{
	GSList *addresses, *cursor;
	GHashTable *table;
	gchar *new_list = NULL;
	
	g_return_val_if_fail (address_list, NULL);

	table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	addresses = modest_text_utils_split_addresses_list (address_list);

	cursor = addresses;
	while (cursor) {
		const gchar* address = (const gchar*)cursor->data;

		/* We need only the email to just compare it and not
		   the full address which would make "a <a@a.com>"
		   different from "a@a.com" */
		const gchar *email = get_email_from_address (address);

		/* ignore the address if already seen */
		if (g_hash_table_lookup (table, email) == 0) {
			gchar *tmp;

			/* Include the full address and not only the
			   email in the returned list */
			if (!new_list) {
				tmp = g_strdup (address);
			} else {
				tmp = g_strjoin (",", new_list, address, NULL);
				g_free (new_list);
			}
			new_list = tmp;
			
			g_hash_table_insert (table, (gchar*)email, GINT_TO_POINTER(1));
		}
		cursor = g_slist_next (cursor);
	}

	g_hash_table_unref (table);
	g_slist_foreach (addresses, (GFunc)g_free, NULL);
	g_slist_free (addresses);

	if (new_list == NULL)
		new_list = g_strdup ("");

	return new_list;
}


static void
modest_text_utils_convert_buffer_to_html_start (GString *html, const gchar *data, gssize n)
{
	guint		i;
	gboolean	space_seen = FALSE;
	guint           break_dist = 0; /* distance since last break point */

	if (n == -1)
		n = strlen (data);

	/* replace with special html chars where needed*/
	for (i = 0; i != n; ++i)  {
		guchar kar = data[i];
		
		if (space_seen && kar != ' ') {
			g_string_append (html, "&#32;");
			space_seen = FALSE;
		}
		
		/* we artificially insert a breakpoint (newline)
		 * after 256, to make sure our lines are not so long
		 * they will DOS the regexping later
		 * Also, check that kar is ASCII to make sure that we
		 * don't break a UTF8 char in two
		 */
		if (++break_dist >= 256 && kar < 127) {
			g_string_append_c (html, '\n');
			break_dist = 0;
		}
		
		switch (kar) {
		case 0:
		case MARK_AMP:
		case MARK_AMP_URI:	
			/* this is a temp place holder for '&'; we can only
				* set the real '&' after hyperlink translation, otherwise
				* we might screw that up */
			break; /* ignore embedded \0s and MARK_AMP */	
		case '<'  : g_string_append (html, MARK_AMP_STR "lt;");   break;
		case '>'  : g_string_append (html, MARK_AMP_STR "gt;");   break;
		case '&'  : g_string_append (html, MARK_AMP_URI_STR "amp;");  break; /* special case */
		case '"'  : g_string_append (html, MARK_AMP_STR "quot;");  break;

		/* don't convert &apos; --> wpeditor will try to re-convert it... */	
		//case '\'' : g_string_append (html, "&apos;"); break;
		case '\n' : g_string_append (html, "<br>\n");break_dist= 0; break;
		case '\t' : g_string_append (html, MARK_AMP_STR "nbsp;" MARK_AMP_STR "nbsp;" MARK_AMP_STR "nbsp; ");
			break_dist=0; break; /* note the space at the end*/
		case ' ':
			break_dist = 0;
			if (space_seen) { /* second space in a row */
				g_string_append (html, "&nbsp; ");
			} else
				space_seen = TRUE;
			break;
		default:
			g_string_append_c (html, kar);
		}
	}
}


static void
modest_text_utils_convert_buffer_to_html_finish (GString *html)
{
	int i;
	/* replace all our MARK_AMPs with real ones */
	for (i = 0; i != html->len; ++i)
		if ((html->str)[i] == MARK_AMP || (html->str)[i] == MARK_AMP_URI)
			(html->str)[i] = '&';
}


gchar*
modest_text_utils_convert_to_html (const gchar *data)
{
	GString		*html;	    
	gsize           len;

	g_return_val_if_fail (data, NULL);
	
	if (!data)
		return NULL;

	len = strlen (data);
	html = g_string_sized_new (1.5 * len);	/* just a  guess... */

	g_string_append_printf (html,
				"<html><head>"
				"<meta http-equiv=\"content-type\" content=\"text/html; charset=utf8\">"
				"</head>"
				"<body>");

	modest_text_utils_convert_buffer_to_html_start (html, data, -1);
	
	g_string_append (html, "</body></html>");

	if (len <= HYPERLINKIFY_MAX_LENGTH)
		hyperlinkify_plain_text (html, 0);

	modest_text_utils_convert_buffer_to_html_finish (html);
	
	return g_string_free (html, FALSE);
}

gchar *
modest_text_utils_convert_to_html_body (const gchar *data, gssize n, gboolean hyperlinkify)
{
	GString		*html;	    

	g_return_val_if_fail (data, NULL);

	if (!data)
		return NULL;

	if (n == -1) 
		n = strlen (data);
	html = g_string_sized_new (1.5 * n);	/* just a  guess... */

	modest_text_utils_convert_buffer_to_html_start (html, data, n);

	if (hyperlinkify && (n < HYPERLINKIFY_MAX_LENGTH))
		hyperlinkify_plain_text (html, 0);

	modest_text_utils_convert_buffer_to_html_finish (html);
	
	return g_string_free (html, FALSE);
}

void
modest_text_utils_get_addresses_indexes (const gchar *addresses, GSList **start_indexes, GSList **end_indexes)
{
	GString *str;
	gchar *start, *cur;

	if (!addresses)
		return;

	if (strlen (addresses) == 0)
		return;

	str = g_string_new ("");
	start = (gchar*) addresses;
	cur = (gchar*) addresses;

	for (cur = start; *cur != '\0'; cur = g_utf8_next_char (cur)) {
		if (*cur == ',' || *cur == ';') {
			gint *start_index, *end_index;
			gchar *next_char = g_utf8_next_char (cur);

			if (!g_utf8_strchr (start, (cur - start + 1), g_utf8_get_char ("@")) &&
			    next_char && *next_char != '\n' && *next_char != '\0')
				continue;

			start_index = g_new0 (gint, 1);
			end_index = g_new0 (gint, 1);
			*start_index = g_utf8_pointer_to_offset (addresses, start);
			*end_index = g_utf8_pointer_to_offset (addresses, cur);;
			*start_indexes = g_slist_prepend (*start_indexes, start_index);
			*end_indexes = g_slist_prepend (*end_indexes, end_index);
			start = g_utf8_next_char (cur);
		}
	}

	if (start != cur) {
		gint *start_index, *end_index;
		start_index = g_new0 (gint, 1);
		end_index = g_new0 (gint, 1);
		*start_index = g_utf8_pointer_to_offset (addresses, start);
		*end_index = g_utf8_pointer_to_offset (addresses, cur);;
		*start_indexes = g_slist_prepend (*start_indexes, start_index);
		*end_indexes = g_slist_prepend (*end_indexes, end_index);
	}

	if (*start_indexes)
		*start_indexes = g_slist_reverse (*start_indexes);
	if (*end_indexes)
		*end_indexes = g_slist_reverse (*end_indexes);
}


GSList *
modest_text_utils_split_addresses_list (const gchar *addresses)
{
	GSList *head;
	const gchar *my_addrs = addresses;
	const gchar *end;
	gchar *addr;
	gboolean after_at = FALSE;

	g_return_val_if_fail (addresses, NULL);

	/* skip any space, ',', ';' '\n' at the start */
	while (my_addrs && (my_addrs[0] == ' ' || my_addrs[0] == ',' ||
			    my_addrs[0] == ';' || my_addrs[0] == '\n'))
	       ++my_addrs;

	/* are we at the end of addresses list? */
	if (!my_addrs[0])
		return NULL;

	/* nope, we are at the start of some address
	 * now, let's find the end of the address */
	end = my_addrs + 1;
	while (end[0] && end[0] != ';' && !(after_at && end[0] == ',')) {
		if (end[0] == '\"') {
			while (end[0] && end[0] != '\"')
				++end;
		}
		if (end[0] == '@') {
			after_at = TRUE;
		}
		if ((end[0] && end[0] == '>')&&(end[1] && end[1] == ',')) {
			++end;
			break;
		}
		++end;
	}

	/* we got the address; copy it and remove trailing whitespace */
	addr = g_strndup (my_addrs, end - my_addrs);
	g_strchomp (addr);

	remove_extra_spaces (addr);

	head = g_slist_append (NULL, addr);
	head->next = modest_text_utils_split_addresses_list (end); /* recurse */

	return head;
}

gchar *
modest_text_utils_join_addresses (const gchar *from,
				  const gchar *to,
				  const gchar *cc,
				  const gchar *bcc)
{
	GString *buffer;
	gboolean add_separator = FALSE;

	buffer = g_string_new ("");

	if (from && strlen (from)) {
		buffer = g_string_append (buffer, from);
		add_separator = TRUE;
	}
	if (to && strlen (to)) {
		if (add_separator)
			buffer = g_string_append (buffer, "; ");
		else
			add_separator = TRUE;

		buffer = g_string_append (buffer, to);
	}
	if (cc && strlen (cc)) {
		if (add_separator)
			buffer = g_string_append (buffer, "; ");
		else
			add_separator = TRUE;

		buffer = g_string_append (buffer, cc);
	}
	if (bcc && strlen (bcc)) {
		if (add_separator)
			buffer = g_string_append (buffer, "; ");
		else
			add_separator = TRUE;

		buffer = g_string_append (buffer, bcc);
	}

	return g_string_free (buffer, FALSE);
}

void
modest_text_utils_address_range_at_position (const gchar *recipients_list,
					     guint position,
					     guint *start,
					     guint *end)
{
	gchar *current = NULL;
	gint range_start = 0;
	gint range_end = 0;
	gint index;
	gboolean is_quoted = FALSE;

	g_return_if_fail (recipients_list);
	g_return_if_fail (position < g_utf8_strlen(recipients_list, -1));
		
	index = 0;
	for (current = (gchar *) recipients_list; *current != '\0';
	     current = g_utf8_find_next_char (current, NULL)) {
		gunichar c = g_utf8_get_char (current);

		if ((c == ',') && (!is_quoted)) {
			if (index < position) {
				range_start = index + 1;
			} else {
				break;
			}
		} else if (c == '\"') {
			is_quoted = !is_quoted;
		} else if ((c == ' ') &&(range_start == index)) {
			range_start ++;
		}
		index ++;
		range_end = index;
	}

	if (start)
		*start = range_start;
	if (end)
		*end = range_end;
}

gchar *
modest_text_utils_address_with_standard_length (const gchar *recipients_list)
{
	gchar ** splitted;
	gchar ** current;
	GString *buffer = g_string_new ("");

	splitted = g_strsplit (recipients_list, "\n", 0);
	current = splitted;
	while (*current) {
		gchar *line;
		if (current != splitted)
			buffer = g_string_append_c (buffer, '\n');
		line = g_strndup (*splitted, 1000);
		buffer = g_string_append (buffer, line);
		g_free (line);
		current++;
	}

	g_strfreev (splitted);

	return g_string_free (buffer, FALSE);
}


/* ******************************************************************* */
/* ************************* UTILIY FUNCTIONS ************************ */
/* ******************************************************************* */

static GString *
get_next_line (const gchar * b, const gsize blen, const gchar * iter)
{
	GString *gs;
	const gchar *i0;
	
	if (iter > b + blen)
		return g_string_new("");
	
	i0 = iter;
	while (iter[0]) {
		if (iter[0] == '\n')
			break;
		iter++;
	}
	gs = g_string_new_len (i0, iter - i0);
	return gs;
}
static int
get_indent_level (const char *l)
{
	int indent = 0;

	while (l[0]) {
		if (l[0] == '>') {
			indent++;
			if (l[1] == ' ') {
				l++;
			}
		} else {
			break;
		}
		l++;

	}

	/*      if we hit the signature marker "-- ", we return -(indent + 1). This
	 *      stops reformatting.
	 */
	if (strcmp (l, MODEST_TEXT_UTILS_SIGNATURE_MARKER) == 0) {
		return -1 - indent;
	} else {
		return indent;
	}
}

static void
unquote_line (GString * l, const gchar *quote_symbol)
{
	gchar *p;
	gint quote_len;

	p = l->str;
	quote_len = strlen (quote_symbol);
	while (p[0]) {
		if (g_str_has_prefix (p, quote_symbol)) {
			if (p[quote_len] == ' ') {
				p += quote_len;
			}
		} else {
			break;
		}
		p++;
	}
	g_string_erase (l, 0, p - l->str);
}

static void
append_quoted (GString * buf, const gchar *quote_symbol,
	       int indent, const GString * str,
	       const int cutpoint)
{
	int i;
	gchar *quote_concat;

	indent = indent < 0 ? abs (indent) - 1 : indent;
	quote_concat = g_strconcat (quote_symbol, " ", NULL);
	for (i = 0; i <= indent; i++) {
		g_string_append (buf, quote_concat);
	}
	g_free (quote_concat);
	if (cutpoint > 0) {
		g_string_append_len (buf, str->str, cutpoint);
	} else {
		g_string_append (buf, str->str);
	}
	g_string_append (buf, "\n");
}

static int
get_breakpoint_utf8 (const gchar * s, gint indent, const gint limit)
{
	gint index = 0;
	const gchar *pos, *last;
	gunichar *uni;

	if (2*indent >= limit)
		return strlen (s);

	indent = indent < 0 ? abs (indent) - 1 : indent;

	last = NULL;
	pos = s;
	uni = g_utf8_to_ucs4_fast (s, -1, NULL);
	while (pos[0]) {
		if ((index + 2 * indent > limit) && last) {
			g_free (uni);
			return last - s;
		}
		if (g_unichar_isspace (uni[index])) {
			last = pos;
		}
		pos = g_utf8_next_char (pos);
		index++;
	}
	g_free (uni);
	return strlen (s);
}

static int
get_breakpoint_ascii (const gchar * s, const gint indent, const gint limit)
{
	gint i, last;

	last = strlen (s);
	if (last + 2 * indent < limit)
		return last;

	for (i = strlen (s); i > 0; i--) {
		if (s[i] == ' ') {
			if (i + 2 * indent <= limit) {
				return i;
			} else {
				last = i;
			}
		}
	}
	return last;
}

static int
get_breakpoint (const gchar * s, const gint indent, const gint limit)
{

	if (g_utf8_validate (s, -1, NULL)) {
		return get_breakpoint_utf8 (s, indent, limit);
	} else {		/* assume ASCII */
		//g_warning("invalid UTF-8 in msg");
		return get_breakpoint_ascii (s, indent, limit);
	}
}

static gchar *
cite (const time_t sent_date, const gchar *from)
{
	return g_strdup (_("mcen_ia_editor_original_message"));
}

static gchar *
quoted_attachments (GList *attachments)
{
	GList *node = NULL;
	GString *result = g_string_new ("");
	for (node = attachments; node != NULL; node = g_list_next (node)) {
		gchar *filename = (gchar *) node->data;
		g_string_append_printf ( result, "%s %s\n", _("mcen_ia_editor_attach_filename"), filename);
	}

	return g_string_free (result, FALSE);

}

static GString *
modest_text_utils_quote_body (GString *output, const gchar *text,
			      const gchar *quote_symbol,
			      int limit)
{

	const gchar *iter;
	gsize len;
	gint indent, breakpoint, rem_indent = 0;
	GString *l, *remaining;
	gchar *forced_wrap_append;

	iter = text;
	len = strlen(text);
	remaining = g_string_new ("");
	forced_wrap_append = NULL;
	do {

		if (forced_wrap_append) {
			gint next_line_indent;
			gint l_len_with_indent;

			g_string_erase (remaining, 0, -1);
			next_line_indent = get_indent_level (iter);
			l = get_next_line (text, len, iter);
			l_len_with_indent = l->len;
			unquote_line (l, quote_symbol);
			if ((l->str && l->str[0] == '\0') || (next_line_indent != indent)) {
				g_string_free (l, TRUE);
				l = g_string_new (forced_wrap_append);
			} else {
				gunichar first_in_l;
				iter = iter + l_len_with_indent + 1;
				first_in_l = g_utf8_get_char_validated (l->str, l->len);
				if (!g_unichar_isspace (first_in_l)) 
					l = g_string_prepend (l, " ");
				l = g_string_prepend (l, forced_wrap_append);
			}
			g_free (forced_wrap_append);
			forced_wrap_append = NULL;
		} else {
			l = get_next_line (text, len, iter);
			iter = iter + l->len + 1;
			indent = get_indent_level (l->str);
			unquote_line (l, quote_symbol);
		}

		if (remaining->len) {
			if (l->len && indent == rem_indent) {
				g_string_prepend (l, " ");
				g_string_prepend (l, remaining->str);
			} else {
				do {
					gunichar remaining_first;
					breakpoint =
						get_breakpoint (remaining->str,
								rem_indent,
								limit);
					if (breakpoint < remaining->len) {
					        g_free (forced_wrap_append);
						forced_wrap_append = g_strdup (remaining->str + breakpoint);
					} else {
						if (!forced_wrap_append)
							append_quoted (output, quote_symbol, rem_indent,
								       remaining, breakpoint);
					}
					g_string_erase (remaining, 0,
							breakpoint);
					remaining_first = g_utf8_get_char_validated (remaining->str, remaining->len);
					if (remaining_first != ((gunichar) -1)) {
						if (g_unichar_isspace (remaining_first)) {
							g_string_erase (remaining, 0, g_utf8_next_char (remaining->str) - remaining->str);
						}
					}
				} while (remaining->len);
			}
		}
		g_string_free (remaining, TRUE);
		breakpoint = get_breakpoint (l->str, indent, limit);
		remaining = g_string_new (l->str + breakpoint);
		if (remaining->str[0] == ' ') {
			g_string_erase (remaining, 0, 1);
		}
		rem_indent = indent;
		if (remaining->len > 0) {
			g_free (forced_wrap_append);
			forced_wrap_append = g_strdup (remaining->str);
		}
		append_quoted (output, quote_symbol, indent, l, breakpoint);
		g_string_free (l, TRUE);
	} while ((iter < text + len) || (remaining->str[0]) || forced_wrap_append);

	return output;
}

static gchar *
modest_text_utils_quote_plain_text (const gchar *text, 
				    const gchar *cite, 
				    const gchar *signature,
				    GList *attachments,
				    int limit)
{
	GString *q;
	gchar *attachments_string = NULL;

	q = g_string_new ("");

	if (signature != NULL) {
		g_string_append_printf (q, "\n%s\n", MODEST_TEXT_UTILS_SIGNATURE_MARKER);
		q = g_string_append (q, signature);
	}

	q = g_string_append (q, "\n");
	q = g_string_append (q, cite);
	q = g_string_append_c (q, '\n');

	q = modest_text_utils_quote_body (q, text, ">", limit);

	attachments_string = quoted_attachments (attachments);
	q = g_string_append (q, attachments_string);
	g_free (attachments_string);

	return g_string_free (q, FALSE);
}

static void
quote_html_add_to_gstring (GString *string,
			   const gchar *text)
{
	if (text && strcmp (text, "")) {
		gchar *html_text = modest_text_utils_convert_to_html_body (text, -1, TRUE);
		g_string_append_printf (string, "%s<br/>", html_text);
		g_free (html_text);
	}
}

static gchar*
modest_text_utils_quote_html (const gchar *text, 
			      const gchar *cite, 
			      const gchar *signature,
			      GList *attachments,
			      int limit)
{
	GString *result_string;

	result_string = 
		g_string_new ( \
			      "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" \
			      "<html>\n"				\
			      "<body>\n<br/>\n");

	if (text || cite || signature) {
		GString *quoted_text;
		g_string_append (result_string, "<pre>\n");
		if (signature) {
			quote_html_add_to_gstring (result_string, MODEST_TEXT_UTILS_SIGNATURE_MARKER);
			quote_html_add_to_gstring (result_string, signature);
		}
		quote_html_add_to_gstring (result_string, cite);
		quoted_text = g_string_new ("");
		quoted_text = modest_text_utils_quote_body (quoted_text, (text) ? text : "", ">", limit);
		quote_html_add_to_gstring (result_string, quoted_text->str);
		g_string_free (quoted_text, TRUE);
		if (attachments) {
			gchar *attachments_string = quoted_attachments (attachments);
			quote_html_add_to_gstring (result_string, attachments_string);
			g_free (attachments_string);
		}
		g_string_append (result_string, "</pre>");
	}
	g_string_append (result_string, "</body>");
	g_string_append (result_string, "</html>");

	return g_string_free (result_string, FALSE);
}

static gint 
cmp_offsets_reverse (const url_match_t *match1, const url_match_t *match2)
{
	return match2->offset - match1->offset;
}

static gint url_matches_block = 0;
static url_match_pattern_t patterns[] = MAIL_VIEWER_URL_MATCH_PATTERNS;
static GMutex *url_patterns_mutex = NULL;


static gboolean
compile_patterns ()
{
	guint i;
	const size_t pattern_num = sizeof(patterns)/sizeof(url_match_pattern_t);
	for (i = 0; i != pattern_num; ++i) {
		patterns[i].preg = g_slice_new0 (regex_t);
		
		/* this should not happen */
		if (regcomp (patterns[i].preg, patterns[i].regex,
			     REG_ICASE|REG_EXTENDED|REG_NEWLINE) != 0) {
			g_warning ("%s: error in regexp:\n%s\n", __FUNCTION__, patterns[i].regex);
			return FALSE;
		}
	}
	return TRUE;
}

static void 
free_patterns ()
{
	guint i;
	const size_t pattern_num = sizeof(patterns)/sizeof(url_match_pattern_t);
	for (i = 0; i != pattern_num; ++i) {
		regfree (patterns[i].preg);
		g_slice_free  (regex_t, patterns[i].preg);
	} /* don't free patterns itself -- it's static */
}

void
modest_text_utils_hyperlinkify_begin (void)
{

	if (url_patterns_mutex == NULL) {
		url_patterns_mutex = g_mutex_new ();
	}
	g_mutex_lock (url_patterns_mutex);
	if (url_matches_block == 0)
		compile_patterns ();
	url_matches_block ++;
	g_mutex_unlock (url_patterns_mutex);
}

void
modest_text_utils_hyperlinkify_end (void)
{
	g_mutex_lock (url_patterns_mutex);
	url_matches_block--;
	if (url_matches_block <= 0)
		free_patterns ();
	g_mutex_unlock (url_patterns_mutex);
}


static GSList*
get_url_matches (GString *txt, gint offset)
{
	regmatch_t rm;
        guint rv, i, tmp_offset = 0;
        GSList *match_list = NULL;

	const size_t pattern_num = sizeof(patterns)/sizeof(url_match_pattern_t);

	/* initalize the regexps */
	modest_text_utils_hyperlinkify_begin ();

        /* find all the matches */
	for (i = 0; i != pattern_num; ++i) {
		tmp_offset     = offset;	
		while (1) {
			url_match_t *match;
			gboolean is_submatch;
			GSList *cursor;
			
			if ((rv = regexec (patterns[i].preg, txt->str + tmp_offset, 1, &rm, 0)) != 0) {
				g_return_val_if_fail (rv == REG_NOMATCH, NULL); /* this should not happen */
				break; /* try next regexp */ 
			}
			if (rm.rm_so == -1)
				break;
			
			is_submatch = FALSE;
			/* check  old matches to see if this has already been matched */
			cursor = match_list;
			while (cursor && !is_submatch) {
				const url_match_t *old_match =
					(const url_match_t *) cursor->data;
				guint new_offset = tmp_offset + rm.rm_so;
				is_submatch = (new_offset >  old_match->offset &&
					       new_offset <  old_match->offset + old_match->len);
				cursor = g_slist_next (cursor);
			}

			if (!is_submatch) {
				/* make a list of our matches (<offset, len, prefix> tupels)*/
				match = g_slice_new (url_match_t);
				match->offset = tmp_offset + rm.rm_so;
				match->len    = rm.rm_eo - rm.rm_so;
				match->prefix = patterns[i].prefix;
				match_list = g_slist_prepend (match_list, match);
			}		
			tmp_offset += rm.rm_eo;
		}
	}

	modest_text_utils_hyperlinkify_end ();
	
	/* now sort the list, so the matches are in reverse order of occurence.
	 * that way, we can do the replacements starting from the end, so we don't need
	 * to recalculate the offsets
	 */
	match_list = g_slist_sort (match_list,
				   (GCompareFunc)cmp_offsets_reverse); 
	return match_list;	
}



/* replace all occurences of needle in haystack with repl*/
static gchar*
replace_string (const gchar *haystack, const gchar *needle, gchar repl)
{
	gchar *str, *cursor;

	if (!haystack || !needle || strlen(needle) == 0)
		return haystack ? g_strdup(haystack) : NULL;
	
	str = g_strdup (haystack);

	for (cursor = str; cursor && *cursor; ++cursor) {
		if (g_str_has_prefix (cursor, needle)) {
			cursor[0] = repl;
			memmove (cursor + 1,
				 cursor + strlen (needle),
				 strlen (cursor + strlen (needle)) + 1);
		}
	}
	
	return str;
}

static void
hyperlinkify_plain_text (GString *txt, gint offset)
{
	GSList *cursor;
	GSList *match_list = get_url_matches (txt, offset);

	/* we will work backwards, so the offsets stay valid */
	for (cursor = match_list; cursor; cursor = cursor->next) {

		url_match_t *match = (url_match_t*) cursor->data;
		gchar *url  = g_strndup (txt->str + match->offset, match->len);
		gchar *repl = NULL; /* replacement  */

		/* the string still contains $(MARK_AMP_URI_STR)"amp;" for each
		 * '&' in the original, because of the text->html conversion.
		 * in the href-URL (and only there), we must convert that back to
		 * '&'
		 */
		gchar *href_url = replace_string (url, MARK_AMP_URI_STR "amp;", '&');
		
		/* the prefix is NULL: use the one that is already there */
		repl = g_strdup_printf ("<a href=\"%s%s\">%s</a>",
					match->prefix ? match->prefix : EMPTY_STRING, 
					href_url, url);

		/* replace the old thing with our hyperlink
		 * replacement thing */
		g_string_erase  (txt, match->offset, match->len);
		g_string_insert (txt, match->offset, repl);
		
		g_free (url);
		g_free (repl);
		g_free (href_url);

		g_slice_free (url_match_t, match);	
	}
	
	g_slist_free (match_list);
}

void
modest_text_utils_hyperlinkify (GString *string_buffer)
{
	gchar *after_body;
	gint offset = 0;

	after_body = strstr (string_buffer->str, "<body>");
	if (after_body != NULL)
		offset = after_body - string_buffer->str;
	hyperlinkify_plain_text (string_buffer, offset);
}


/* for optimization reasons, we change the string in-place */
void
modest_text_utils_get_display_address (gchar *address)
{
	int i;

	g_return_if_fail (address);
	
	if (!address)
		return;
	
	/* should not be needed, and otherwise, we probably won't screw up the address
	 * more than it already is :) 
	 * g_return_val_if_fail (g_utf8_validate (address, -1, NULL), NULL);
	 * */
	
	/* remove leading whitespace */
	if (address[0] == ' ')
		g_strchug (address);
		
	for (i = 0; address[i]; ++i) {
		if (address[i] == '<') {
			if (G_UNLIKELY(i == 0)) {
				break; /* there's nothing else, leave it */
			}else {
				address[i] = '\0'; /* terminate the string here */
				break;
			}
		}
	}

	g_strchomp (address);
}


gchar *
modest_text_utils_get_display_addresses (const gchar *recipients)
{
	gchar *addresses;
	GSList *recipient_list;

	addresses = NULL;
	recipient_list = modest_text_utils_split_addresses_list (recipients);
	if (recipient_list) {
		GString *add_string = g_string_sized_new (strlen (recipients));
		GSList *iter = recipient_list;
		gboolean first = TRUE;

		while (iter) {
			/* Strings are changed in place */
			modest_text_utils_get_display_address ((gchar *) iter->data);
			if (G_UNLIKELY (first)) {
				g_string_append_printf (add_string, "%s", (gchar *) iter->data);
				first = FALSE;
			} else {
				g_string_append_printf (add_string, ", %s", (gchar *) iter->data);
			}
			iter = g_slist_next (iter);
		}
		g_slist_foreach (recipient_list, (GFunc) g_free, NULL);
		g_slist_free (recipient_list);
		addresses = g_string_free (add_string, FALSE);
	}

	return addresses;
}


gchar *
modest_text_utils_get_email_address (const gchar *full_address)
{
	const gchar *left, *right;

	g_return_val_if_fail (full_address, NULL);
	
	if (!full_address)
		return NULL;
	
	g_return_val_if_fail (g_utf8_validate (full_address, -1, NULL), NULL);
	
	left = g_strrstr_len (full_address, strlen(full_address), "<");
	if (left == NULL)
		return g_strdup (full_address);

	right = g_strstr_len (left, strlen(left), ">");
	if (right == NULL)
		return g_strdup (full_address);

	return g_strndup (left + 1, right - left - 1);
}

gint 
modest_text_utils_get_subject_prefix_len (const gchar *sub)
{
	gint prefix_len = 0;	

	g_return_val_if_fail (sub, 0);

	if (!sub)
		return 0;
	
	/* optimization: "Re", "RE", "re","Fwd", "FWD", "fwd","FW","Fw", "fw" */
	if (sub[0] != 'R' && sub[0] != 'F' && sub[0] != 'r' && sub[0] != 'f')
		return 0;
	else if (sub[0] && sub[1] != 'e' && sub[1] != 'E' && sub[1] != 'w' && sub[1] != 'W')
		return 0;

	prefix_len = 2;
	if (sub[2] == 'd')
		++prefix_len;

	/* skip over a [...] block */
	if (sub[prefix_len] == '[') {
		int c = prefix_len + 1;
		while (sub[c] && sub[c] != ']')
			++c;
		if (!sub[c])
			return 0; /* no end to the ']' found */
		else
			prefix_len = c + 1;
	}

	/* did we find the ':' ? */
	if (sub[prefix_len] == ':') {
		++prefix_len;
		if (sub[prefix_len] == ' ')
			++prefix_len;
		prefix_len += modest_text_utils_get_subject_prefix_len (sub + prefix_len);
/* 		g_warning ("['%s','%s']", sub, (char*) sub + prefix_len); */
		return prefix_len;
	} else
		return 0;
}


gint
modest_text_utils_utf8_strcmp (const gchar* s1, const gchar *s2, gboolean insensitive)
{

/* work even when s1 and/or s2 == NULL */
	if (G_UNLIKELY(s1 == s2))
		return 0;
	if (G_UNLIKELY(!s1))
		return -1;
	if (G_UNLIKELY(!s2))
		return 1;
	
	/* if it's not case sensitive */
	if (!insensitive) {

		/* optimization: shortcut if first char is ascii */ 
		if (((s1[0] & 0x80)== 0) && ((s2[0] & 0x80) == 0) &&
		    (s1[0] != s2[0])) 
			return s1[0] - s2[0];
		
		return g_utf8_collate (s1, s2);

	} else {
		gint result;
		gchar *n1, *n2;

		/* optimization: shortcut if first char is ascii */ 
		if (((s1[0] & 0x80) == 0) && ((s2[0] & 0x80) == 0) &&
		    (tolower(s1[0]) != tolower (s2[0]))) 
			return tolower(s1[0]) - tolower(s2[0]);
		
		n1 = g_utf8_strdown (s1, -1);
		n2 = g_utf8_strdown (s2, -1);
		
		result = g_utf8_collate (n1, n2);
		
		g_free (n1);
		g_free (n2);
	
		return result;
	}
}


const gchar*
modest_text_utils_get_display_date (time_t date)
{
#define DATE_BUF_SIZE 64 
	static gchar date_buf[DATE_BUF_SIZE];
	
	/* calculate the # of days since epoch for 
 	 * for today and for the date provided 
 	 * based on idea from pvanhoof */
	int day      = time(NULL) / (24 * 60 * 60);
	int date_day = date       / (24 * 60 * 60);

	/* if it's today, show the time, if it's not today, show the date instead */

	/* TODO: take into account the system config for 24/12h */
#ifdef MODEST_TOOLKIT_HILDON2
	if (day == date_day) /* is the date today? */
		modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, _HL("wdgt_va_24h_time"), date);
	else 
		modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, _HL("wdgt_va_date"), date); 
#else
	if (day == date_day) /* is the date today? */
		modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, "%X", date);
	else 
		modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, "%x", date); 
#endif

	return date_buf; /* this is a static buffer, don't free! */
}



gboolean
modest_text_utils_validate_folder_name (const gchar *folder_name)
{
	/* based on http://msdn2.microsoft.com/en-us/library/aa365247.aspx,
	 * with some extras */
	
	guint len;
	gint i;
	const gchar **cursor = NULL;
	const gchar *forbidden_names[] = { /* windows does not like these */
		"CON", "PRN", "AUX", "NUL", ".", "..", "cur", "tmp", "new", 
		NULL /* cur, tmp, new are reserved for Maildir */
	};
	
	/* cannot be NULL */
	if (!folder_name) 
		return FALSE;

	/* cannot be empty */
	len = strlen(folder_name);
	if (len == 0)
		return FALSE;
	
	/* cannot start with a dot, vfat does not seem to like that */
	if (folder_name[0] == '.')
		return FALSE;

	/* cannot start or end with a space */
	if (g_ascii_isspace(folder_name[0]) || g_ascii_isspace(folder_name[len - 1]))
		return FALSE; 

	/* cannot contain a forbidden char */	
	for (i = 0; i < len; i++)
		if (modest_text_utils_is_forbidden_char (folder_name[i], FOLDER_NAME_FORBIDDEN_CHARS))
			return FALSE;

	/* Cannot contain Windows port numbers. I'd like to use GRegex
	   but it's still not available in Maemo. sergio */
	if (!g_ascii_strncasecmp (folder_name, "LPT", 3) ||
	    !g_ascii_strncasecmp (folder_name, "COM", 3)) {
		glong val;
		gchar *endptr;

		/* We skip the first 3 characters for the
		   comparison */
		val = strtol(folder_name+3, &endptr, 10);

		/* If the conversion to long succeeded then the string
		   is not valid for us */
		if (*endptr == '\0')
			return FALSE;
		else
			return TRUE;
	}
	
	/* cannot contain a forbidden word */
	if (len <= 4) {
		for (cursor = forbidden_names; cursor && *cursor; ++cursor) {
			if (g_ascii_strcasecmp (folder_name, *cursor) == 0)
				return FALSE;
		}
	}

	return TRUE; /* it's valid! */
}



gboolean
modest_text_utils_validate_domain_name (const gchar *domain)
{
	gboolean valid = FALSE;
	regex_t rx;
	const gchar* domain_regex = "^([a-z0-9-]*[a-z0-9]\\.)+[a-z0-9-]*[a-z0-9]$";

	g_return_val_if_fail (domain, FALSE);
	
	if (!domain)
		return FALSE;
	
	memset (&rx, 0, sizeof(regex_t)); /* coverity wants this... */
		
	/* domain name: all alphanum or '-' or '.',
	 * but beginning/ending in alphanum */	
	if (regcomp (&rx, domain_regex, REG_ICASE|REG_EXTENDED|REG_NOSUB)) {
		g_warning ("BUG: error in regexp");
		return FALSE;
	}
	
	valid = (regexec (&rx, domain, 1, NULL, 0) == 0);
	regfree (&rx);
		
	return valid;
}



gboolean
modest_text_utils_validate_email_address (const gchar *email_address,
					  const gchar **invalid_char_position)
{
	int count = 0;
	const gchar *c = NULL, *domain = NULL;
	static gchar *rfc822_specials = "()<>@,;:\\\"[]&";
	
	if (invalid_char_position)
		*invalid_char_position = NULL;
	
	g_return_val_if_fail (email_address, FALSE);
	
	/* check that the email adress contains exactly one @ */
	if (!strstr(email_address, "@") || 
			(strstr(email_address, "@") != g_strrstr(email_address, "@"))) 
		return FALSE;
	
	/* first we validate the name portion (name@domain) */
	for (c = email_address;  *c;  c++) {
		if (*c == '\"' && 
		    (c == email_address || 
		     *(c - 1) == '.' || 
		     *(c - 1) == '\"')) {
			while (*++c) {
				if (*c == '\"') 
					break;
				if (*c == '\\' && (*++c == ' ')) 
					continue;
				if (*c <= ' ' || *c >= 127) 
					return FALSE;
			}
			if (!*c++) 
				return FALSE;
			if (*c == '@') 
				break;
			if (*c != '.') 
				return FALSE;
			continue;
		}
		if (*c == '@') 
			break;
		if (*c <= ' ' || *c >= 127) 
			return FALSE;
		if (strchr(rfc822_specials, *c)) {
			if (invalid_char_position)
				*invalid_char_position = c;
			return FALSE;
		}
	}
	if (c == email_address || *(c - 1) == '.') 
		return FALSE;

	/* next we validate the domain portion (name@domain) */
	if (!*(domain = ++c)) 
		return FALSE;
	do {
		if (*c == '.') {
			if (c == domain || *(c - 1) == '.' || *(c + 1) == '\0') 
				return FALSE;
			count++;
		}
		if (*c <= ' ' || *c >= 127) 
			return FALSE;
		if (strchr(rfc822_specials, *c)) {
			if (invalid_char_position)
				*invalid_char_position = c;
			return FALSE;
		}
	} while (*++c);

	return (count >= 1) ? TRUE : FALSE;
}

gboolean 
modest_text_utils_validate_recipient (const gchar *recipient, const gchar **invalid_char_position)
{
	gchar *stripped, *current;
	gchar *right_part;
	gboolean has_error = FALSE;

	if (invalid_char_position)
		*invalid_char_position = NULL;
	
	g_return_val_if_fail (recipient, FALSE);
	
	if (modest_text_utils_validate_email_address (recipient, invalid_char_position))
		return TRUE;

	stripped = g_strdup (recipient);
	stripped = g_strstrip (stripped);
	current = stripped;

	if (*current == '\0') {
		g_free (stripped);
		return FALSE;
	}

	/* quoted string */
	if (*current == '\"') {
		gchar *last_quote = NULL;
		current = g_utf8_next_char (current);
		has_error = TRUE;
		for (; *current != '\0'; current = g_utf8_next_char (current)) {
			if (*current == '\\') {
				/* TODO: This causes a warning, which breaks the build, 
				 * because a gchar cannot be < 0.
				 * murrayc. 
				if (current[1] <0) {
					has_error = TRUE;
					break;
				}
				*/
			} else if (*current == '\"') {
				has_error = FALSE;
				current = g_utf8_next_char (current);
				last_quote = current;
			}
		}
		if (last_quote)
			current = g_utf8_next_char (last_quote);
	} else {
		has_error = TRUE;
		for (current = stripped ; *current != '\0'; current = g_utf8_next_char (current)) {
			if (*current == '<') {
				has_error = FALSE;
				break;
			}
		}
	}
		
	if (has_error) {
		g_free (stripped);
		return FALSE;
	}

	right_part = g_strdup (current);
	g_free (stripped);
	right_part = g_strstrip (right_part);

	if (g_str_has_suffix (right_part, ",") || g_str_has_suffix (right_part, ";"))
               right_part [(strlen (right_part) - 1)] = '\0';

	if (g_str_has_prefix (right_part, "<") &&
	    g_str_has_suffix (right_part, ">")) {
		gchar *address;
		gboolean valid;

		address = g_strndup (right_part+1, strlen (right_part) - 2);
		g_free (right_part);
		valid = modest_text_utils_validate_email_address (address, invalid_char_position);
		g_free (address);
		return valid;
	} else {
		g_free (right_part);
		return FALSE;
	}
}


gchar *
modest_text_utils_get_display_size (guint64 size)
{
	const guint KB=1024;
	const guint MB=1024 * KB;
	const guint GB=1024 * MB;

	if (size == 0)
		return g_strdup_printf (_FM("sfil_li_size_kb"), (int) 0);
	if (0 <= size && size < KB)
		return g_strdup_printf (_FM("sfil_li_size_1kb_99kb"), (int) 1);
	else if (KB <= size && size < 100 * KB)
		return g_strdup_printf (_FM("sfil_li_size_1kb_99kb"), (int) size / KB);
	else if (100*KB <= size && size < MB)
		return g_strdup_printf (_FM("sfil_li_size_100kb_1mb"), (int) size / KB);
	else if (MB <= size && size < 10*MB)
		return g_strdup_printf (_FM("sfil_li_size_1mb_10mb"), (float) size / MB);
	else if (10*MB <= size && size < GB)
		return g_strdup_printf (_FM("sfil_li_size_10mb_1gb"), (float) size / MB);
	else
		return g_strdup_printf (_FM("sfil_li_size_1gb_or_greater"), (float) size / GB);
}

static gchar *
get_email_from_address (const gchar * address)
{
	gchar *left_limit, *right_limit;

	left_limit = strstr (address, "<");
	right_limit = g_strrstr (address, ">");

	if ((left_limit == NULL)||(right_limit == NULL)|| (left_limit > right_limit))
		return g_strdup (address);
	else
		return g_strndup (left_limit + 1, (right_limit - left_limit) - 1);
}

gchar *
modest_text_utils_get_color_string (GdkColor *color)
{
	g_return_val_if_fail (color, NULL);

	return g_strdup_printf ("#%x%x%x%x%x%x%x%x%x%x%x%x",
				(color->red >> 12)   & 0xf, (color->red >> 8)   & 0xf,
				(color->red >>  4)   & 0xf, (color->red)        & 0xf,
				(color->green >> 12) & 0xf, (color->green >> 8) & 0xf,
				(color->green >>  4) & 0xf, (color->green)      & 0xf,
				(color->blue >> 12)  & 0xf, (color->blue >> 8)  & 0xf,
				(color->blue >>  4)  & 0xf, (color->blue)       & 0xf);
}

gchar *
modest_text_utils_text_buffer_get_text (GtkTextBuffer *buffer)
{
	GtkTextIter start, end;
	gchar *slice, *current;
	GString *result = g_string_new ("");

	g_return_val_if_fail (buffer && GTK_IS_TEXT_BUFFER (buffer), NULL);
	
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	slice = gtk_text_buffer_get_slice (buffer, &start, &end, FALSE);
	current = slice;

	while (current && current != '\0') {
		if (g_utf8_get_char (current) == 0xFFFC) {
			result = g_string_append_c (result, ' ');
			current = g_utf8_next_char (current);
		} else {
			gchar *next = g_utf8_strchr (current, -1, 0xFFFC);
			if (next == NULL) {
				result = g_string_append (result, current);
			} else {
				result = g_string_append_len (result, current, next - current);
			}
			current = next;
		}
	}
	g_free (slice);

	return g_string_free (result, FALSE);
	
}

gboolean
modest_text_utils_is_forbidden_char (const gchar character,
				     ModestTextUtilsForbiddenCharType type)
{
	gint i, len;
	const gchar *forbidden_chars = NULL;
	
	/* We need to get the length in the switch because the
	   compiler needs to know the size at compile time */
	switch (type) {
	case ACCOUNT_TITLE_FORBIDDEN_CHARS:
		forbidden_chars = account_title_forbidden_chars;
		len = G_N_ELEMENTS (account_title_forbidden_chars);
		break;
	case FOLDER_NAME_FORBIDDEN_CHARS:
		forbidden_chars = folder_name_forbidden_chars;
		len = G_N_ELEMENTS (folder_name_forbidden_chars);
		break;
	case USER_NAME_FORBIDDEN_NAMES:
		forbidden_chars = user_name_forbidden_chars;
		len = G_N_ELEMENTS (user_name_forbidden_chars);
		break;
	default:
		g_return_val_if_reached (TRUE);
	}

	for (i = 0; i < len ; i++)
		if (forbidden_chars[i] == character)
			return TRUE;

	return FALSE; /* it's valid! */
}

gchar *      
modest_text_utils_label_get_selection (GtkLabel *label)
{
	gint start, end;
	gchar *selection;

	if (gtk_label_get_selection_bounds (GTK_LABEL (label), &start, &end)) {
		const gchar *start_offset;
		const gchar *end_offset;
		start_offset = gtk_label_get_text (GTK_LABEL (label));
		start_offset = g_utf8_offset_to_pointer (start_offset, start);
		end_offset = gtk_label_get_text (GTK_LABEL (label));
		end_offset = g_utf8_offset_to_pointer (end_offset, end);
		selection = g_strndup (start_offset, end_offset - start_offset);
		return selection;
	} else {
		return g_strdup ("");
	}
}

static gboolean
_forward_search_image_char (gunichar ch,
			    gpointer userdata)
{
	return (ch == 0xFFFC);
}

gboolean
modest_text_utils_buffer_selection_is_valid (GtkTextBuffer *buffer)
{
	gboolean result;
	GtkTextIter start, end;

	g_return_val_if_fail (GTK_IS_TEXT_BUFFER (buffer), FALSE);

	result = gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (buffer));

	/* check there are no images in selection */
	if (result) {
		gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
		if (gtk_text_iter_get_char (&start)== 0xFFFC)
			result = FALSE;
		else {
			gtk_text_iter_backward_char (&end);
			if (gtk_text_iter_forward_find_char (&start, _forward_search_image_char,
							     NULL, &end))
				result = FALSE;
		}
				    
	}

	return result;
}

static void
remove_quotes (gchar **quotes)
{
	if (g_str_has_prefix (*quotes, "\"") && g_str_has_suffix (*quotes, "\"")) {
		gchar *result;
		result = g_strndup ((*quotes)+1, strlen (*quotes) - 2);
		g_free (*quotes);
		*quotes = result;
	}
}

static void
remove_extra_spaces (gchar *string)
{
	gchar *start;

	start = string;
	while (start && start[0] != '\0') {
		if ((start[0] == ' ') && (start[1] == ' ')) {
			g_strchug (start+1);
		}
		start++;
	}
}

gchar *
modest_text_utils_escape_mnemonics (const gchar *text)
{
	const gchar *p;
	GString *result = NULL;

	if (text == NULL)
		return NULL;

	result = g_string_new ("");
	for (p = text; *p != '\0'; p++) {
		if (*p == '_')
			result = g_string_append (result, "__");
		else
			result = g_string_append_c (result, *p);
	}
	
	return g_string_free (result, FALSE);
}

gchar *
modest_text_utils_simplify_recipients (const gchar *recipients)
{
	GSList *addresses, *node;
	GString *result;
	gboolean is_first = TRUE;

	if (recipients == NULL)
		return g_strdup ("");

	addresses = modest_text_utils_split_addresses_list (recipients);
	result = g_string_new ("");

	for (node = addresses; node != NULL; node = g_slist_next (node)) {
		const gchar *address = (const gchar *) node->data;
		gchar *left_limit, *right_limit;

		left_limit = strstr (address, "<");
		right_limit = g_strrstr (address, ">");

		if (is_first)
			is_first = FALSE;
		else
			result = g_string_append (result, ", ");

		if ((left_limit == NULL)||(right_limit == NULL)|| (left_limit > right_limit)) {
			result = g_string_append (result, address);
		} else {
			gchar *name_side;
			gchar *email_side;
			name_side = g_strndup (address, left_limit - address);
			name_side = g_strstrip (name_side);
			remove_quotes (&name_side);
			email_side = get_email_from_address (address);
			if (name_side && email_side && !strcmp (name_side, email_side)) {
				result = g_string_append (result, email_side);
			} else {
				result = g_string_append (result, address);
			}
			g_free (name_side);
			g_free (email_side);
		}

	}
	g_slist_foreach (addresses, (GFunc)g_free, NULL);
	g_slist_free (addresses);

	return g_string_free (result, FALSE);

}

GSList *
modest_text_utils_remove_duplicate_addresses_list (GSList *address_list)
{
	GSList *new_list, *iter;
	GHashTable *table;

	g_return_val_if_fail (address_list, NULL);

	table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	new_list = address_list;
	iter = address_list;
	while (iter) {
		const gchar* address = (const gchar*)iter->data;

		/* We need only the email to just compare it and not
		   the full address which would make "a <a@a.com>"
		   different from "a@a.com" */
		const gchar *email = get_email_from_address (address);

		/* ignore the address if already seen */
		if (g_hash_table_lookup (table, email) == 0) {
			g_hash_table_insert (table, (gchar*)email, GINT_TO_POINTER(1));
			iter = g_slist_next (iter);
		} else {
			GSList *tmp = g_slist_next (iter);
			new_list = g_slist_delete_link (new_list, iter);
			iter = tmp;
		}
	}

	g_hash_table_unref (table);

	return new_list;
}

gchar *
modest_text_utils_get_secure_header (const gchar *value,
				     const gchar *header)
{
	const gint max_len = 16384;
	gchar *new_value = NULL;
	gchar *needle = g_strrstr (value, header);

	if (needle && value != needle)
		new_value = g_strdup (needle + strlen (header));

	if (!new_value)
		new_value = g_strdup (value);

	/* Do a max length check to prevent DoS attacks caused by huge
	   malformed headers */
	if (g_utf8_validate (new_value, -1, NULL)) {
		if (g_utf8_strlen (new_value, -1) > max_len) {
			gchar *tmp = g_malloc0 (max_len * 4);
			g_utf8_strncpy (tmp, (const gchar *) new_value, max_len);
			g_free (new_value);
			new_value = tmp;
		}
	} else {
		if (strlen (new_value) > max_len) {
			gchar *tmp = g_malloc0 (max_len);
			strncpy (new_value, tmp, max_len);
			g_free (new_value);
			new_value = tmp;
		}
	}

	return new_value;
}

static gboolean
is_quoted (const char *start, const gchar *end)
{
	gchar *c;

	c = (gchar *) start;
	while (*c == ' ')
		c = g_utf8_next_char (c);

	if (*c == '\0' || *c != '\"')
		return FALSE;

	c = (gchar *) end;
	while (*c == ' ' && c != start)
		c = g_utf8_prev_char (c);

	if (c == start || *c != '\"')
		return FALSE;

	return TRUE;
}


static void
quote_name_part (GString **str, gchar **cur, gchar **start)
{
	gchar *blank;
	gint str_len = *cur - *start;

	while (**start == ' ') {
		*start = g_utf8_next_char (*start);
		str_len--;
	}

	blank = g_utf8_strrchr (*start, str_len, g_utf8_get_char (" "));
	if (blank && (blank != *start)) {
		if (is_quoted (*start, blank - 1)) {
			*str = g_string_append_len (*str, *start, str_len);
			*str = g_string_append (*str, ";");
			*start = g_utf8_next_char (*cur);
		} else {
			*str = g_string_append_c (*str, '"');
			*str = g_string_append_len (*str, *start, (blank - *start));
			*str = g_string_append_c (*str, '"');
			*str = g_string_append_len (*str, blank, (*cur - blank));
			*str = g_string_append (*str, ";");
			*start = g_utf8_next_char (*cur);
		}
	} else {
		*str = g_string_append_len (*str, *start, str_len);
		*str = g_string_append (*str, ";");
		*start = g_utf8_next_char (*cur);
	}
}

gchar *
modest_text_utils_quote_names (const gchar *recipients)
{
	GString *str;
	gchar *start, *cur;

	str = g_string_new ("");
	start = (gchar*) recipients;
	cur = (gchar*) recipients;

	for (cur = start; *cur != '\0'; cur = g_utf8_next_char (cur)) {
		if (*cur == ',' || *cur == ';') {
			if (!g_utf8_strchr (start, (cur - start + 1), g_utf8_get_char ("@")))
				continue;
			quote_name_part (&str, &cur, &start);
		}
	}

	quote_name_part (&str, &cur, &start);

	return g_string_free (str, FALSE);
}

/* Returns TRUE if there is no recipients in the text buffer. Note
   that strings like " ; , " contain only separators and thus no
   recipients */
gboolean
modest_text_utils_no_recipient (GtkTextBuffer *buffer)
{
	gboolean retval = TRUE;
	gchar *text, *tmp;
	GtkTextIter start, end;

	if (gtk_text_buffer_get_char_count (buffer) == 0)
		return TRUE;

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	text = g_strstrip (gtk_text_buffer_get_text (buffer, &start, &end, FALSE));
	if (!g_strcmp0 (text, ""))
		return TRUE;

	tmp = text;
	while (tmp && *tmp != '\0') {
		if ((*tmp != ',') && (*tmp != ';') &&
		    (*tmp != '\r') && (*tmp != '\n') &&
		    (*tmp != ' ')) {
			retval = FALSE;
			break;
		} else {
			tmp++;
		}
	}
	g_free (text);

	return retval;
}

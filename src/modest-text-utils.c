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

#define MAIL_VIEWER_URL_MATCH_PATTERNS  {				\
	{ "(file|rtsp|http|ftp|https)://[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]+[-A-Za-z0-9_$%&=?/~#]",\
	  NULL, NULL },\
	{ "www\\.[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]*[^]}\\),?!;:\"]?)?",\
			NULL, "http://" },				\
	{ "ftp\\.[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]*[^]}\\),?!;:\"]?)?",\
	  NULL, "ftp://" },\
	{ "(voipto|callto|chatto|jabberto|xmpp):[-_a-z@0-9.\\+]+", \
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
	'<', '>', ':', '\'', '/', '\\', '|', '?', '*', '^', '%', '$'
};
const gchar user_name_forbidden_chars[] = {
	'<', '>'
};
const guint ACCOUNT_TITLE_FORBIDDEN_CHARS_LENGTH = G_N_ELEMENTS (account_title_forbidden_chars);
const guint FOLDER_NAME_FORBIDDEN_CHARS_LENGTH = G_N_ELEMENTS (folder_name_forbidden_chars);
const guint USER_NAME_FORBIDDEN_CHARS_LENGTH = G_N_ELEMENTS (user_name_forbidden_chars);

/* private */
static gchar*   cite                    (const time_t sent_date, const gchar *from);
static void     hyperlinkify_plain_text (GString *txt);
static gint     cmp_offsets_reverse     (const url_match_t *match1, const url_match_t *match2);
static GSList*  get_url_matches         (GString *txt);

static GString* get_next_line           (const char *b, const gsize blen, const gchar * iter);
static int      get_indent_level        (const char *l);
static void     unquote_line            (GString * l);
static void     append_quoted           (GString * buf, const int indent, const GString * str, 
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
	
	if (!signature)
		retval = g_strdup ("");
	else if (strcmp(content_type, "text/html") == 0) {
		tmp_sig = g_strconcat ("\n", signature, NULL);
		retval = modest_text_utils_convert_to_html_body(tmp_sig, -1, TRUE);
		g_free (tmp_sig);
	} else {
		retval = g_strconcat (text, "\n", signature, NULL);
	}

	return retval;
}

static gchar *
forward_cite (const gchar *from,
	      const gchar *sent,
	      const gchar *to,
	      const gchar *subject)
{
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

	/* does not work on old maemo glib: 
	 *   g_date_set_time_t (&date, timet);
	 */
	localtime_r (&timet, &tm);
	return strftime(s, max, fmt, &tm);
}

gchar *
modest_text_utils_derived_subject (const gchar *subject, const gchar *prefix)
{
	gchar *tmp;

	g_return_val_if_fail (prefix, NULL);

	if (!subject || subject[0] == '\0')
		subject = _("mail_va_no_subject");

	tmp = g_strchug (g_strdup (subject));

	if (!strncmp (tmp, prefix, strlen (prefix))) {
		return tmp;
	} else {
		g_free (tmp);
		return g_strdup_printf ("%s %s", prefix, subject);
	}
}

gchar*
modest_text_utils_remove_address (const gchar *address_list, const gchar *address)
{
	gchar *dup, *token, *ptr, *result;
	GString *filtered_emails;
	gchar *email_address;

	g_return_val_if_fail (address_list, NULL);

	if (!address)
		return g_strdup (address_list);

	email_address = get_email_from_address (address);
	
	/* search for substring */
	if (!strstr ((const char *) address_list, (const char *) email_address)) {
		g_free (email_address);
		return g_strdup (address_list);
	}

	dup = g_strdup (address_list);
	filtered_emails = g_string_new (NULL);
	
	token = strtok_r (dup, ",", &ptr);

	while (token != NULL) {
		/* Add to list if not found */
		if (!strstr ((const char *) token, (const char *) email_address)) {
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

static void
modest_text_utils_convert_buffer_to_html (GString *html, const gchar *data, gssize n)
{
	guint		 i;
	gboolean	space_seen = FALSE;
	guint           break_dist = 0; /* distance since last break point */

	if (n == -1)
		n = strlen (data);

	/* replace with special html chars where needed*/
	for (i = 0; i != n; ++i)  {
		char kar = data[i];
		
		if (space_seen && kar != ' ') {
			g_string_append_c (html, ' ');
			space_seen = FALSE;
		}
		
		/* we artificially insert a breakpoint (newline)
		 * after 256, to make sure our lines are not so long
		 * they will DOS the regexping later
		 */
		if (++break_dist == 256) {
			g_string_append_c (html, '\n');
			break_dist = 0;
		}
		
		switch (kar) {
		case 0:  break; /* ignore embedded \0s */	
		case '<'  : g_string_append (html, "&lt;");   break;
		case '>'  : g_string_append (html, "&gt;");   break;
		case '&'  : g_string_append (html, "&amp;");  break;
		case '"'  : g_string_append (html, "&quot;");  break;

		/* don't convert &apos; --> wpeditor will try to re-convert it... */	
		//case '\'' : g_string_append (html, "&apos;"); break;
		case '\n' : g_string_append (html, "<br>\n");              break_dist= 0; break;
		case '\t' : g_string_append (html, "&nbsp;&nbsp;&nbsp; "); break_dist=0; break; /* note the space at the end*/
		case ' ':
			break_dist = 0;
			if (space_seen) { /* second space in a row */
				g_string_append (html, "&nbsp; ");
				space_seen = FALSE;
			} else
				space_seen = TRUE;
			break;
		default:
			g_string_append_c (html, kar);
		}
	}
}

gchar*
modest_text_utils_convert_to_html (const gchar *data)
{
	GString		*html;	    
	gsize           len;
	
	if (!data)
		return NULL;

	len = strlen (data);
	html = g_string_sized_new (1.5 * len);	/* just a  guess... */

	g_string_append_printf (html,
				"<html><head>"
				"<meta http-equiv=\"content-type\" content=\"text/html; charset=utf8\">"
				"</head>"
				"<body>");

	modest_text_utils_convert_buffer_to_html (html, data, -1);
	
	g_string_append (html, "</body></html>");

	if (len <= HYPERLINKIFY_MAX_LENGTH)
		hyperlinkify_plain_text (html);

	return g_string_free (html, FALSE);
}

gchar *
modest_text_utils_convert_to_html_body (const gchar *data, gssize n, gboolean hyperlinkify)
{
	GString		*html;	    
	
	if (!data)
		return NULL;

	if (n == -1) 
		n = strlen (data);
	html = g_string_sized_new (1.5 * n);	/* just a  guess... */

	modest_text_utils_convert_buffer_to_html (html, data, n);

	if (hyperlinkify && (n < HYPERLINKIFY_MAX_LENGTH))
		hyperlinkify_plain_text (html);

	return g_string_free (html, FALSE);
}

void
modest_text_utils_get_addresses_indexes (const gchar *addresses, GSList **start_indexes, GSList **end_indexes)
{
	gchar *current, *start, *last_blank;
	gint start_offset = 0, current_offset = 0;

	g_return_if_fail (start_indexes != NULL);
	g_return_if_fail (end_indexes != NULL);

	start = (gchar *) addresses;
	current = start;
	last_blank = start;

	while (*current != '\0') {
		if ((start == current)&&((*current == ' ')||(*current == ',')||(*current == ';'))) {
			start = g_utf8_next_char (start);
			start_offset++;
			last_blank = current;
		} else if ((*current == ',')||(*current == ';')) {
			gint *start_index, *end_index;
			start_index = g_new0(gint, 1);
			end_index = g_new0(gint, 1);
			*start_index = start_offset;
			*end_index = current_offset;
			*start_indexes = g_slist_prepend (*start_indexes, start_index);
			*end_indexes = g_slist_prepend (*end_indexes, end_index);
			start = g_utf8_next_char (current);
			start_offset = current_offset + 1;
			last_blank = start;
		} else if (*current == '"') {
			current = g_utf8_next_char (current);
			current_offset ++;
			while ((*current != '"')&&(*current != '\0')) {
				current = g_utf8_next_char (current);
				current_offset ++;
			}
		}
				
		current = g_utf8_next_char (current);
		current_offset ++;
	}

	if (start != current) {
			gint *start_index, *end_index;
			start_index = g_new0(gint, 1);
			end_index = g_new0(gint, 1);
			*start_index = start_offset;
			*end_index = current_offset;
			*start_indexes = g_slist_prepend (*start_indexes, start_index);
			*end_indexes = g_slist_prepend (*end_indexes, end_index);
	}
	
	*start_indexes = g_slist_reverse (*start_indexes);
	*end_indexes = g_slist_reverse (*end_indexes);

	return;
}

GSList *
modest_text_utils_split_addresses_list (const gchar *addresses)
{
	gchar *current, *start, *last_blank;
	GSList *result = NULL;

	start = (gchar *) addresses;
	current = start;
	last_blank = start;

	while (*current != '\0') {
		if ((start == current)&&((*current == ' ')||(*current == ',')||(*current == ';'))) {
			start = g_utf8_next_char (start);
			last_blank = current;
		} else if ((*current == ',')||(*current == ';')) {
			gchar *new_address = NULL;
			new_address = g_strndup (start, current - last_blank);
			result = g_slist_prepend (result, new_address);
			start = g_utf8_next_char (current);
			last_blank = start;
		} else if (*current == '\"') {
			if (current == start) {
				current = g_utf8_next_char (current);
				start = g_utf8_next_char (start);
			}
			while ((*current != '\"')&&(*current != '\0'))
				current = g_utf8_next_char (current);
		}
				
		current = g_utf8_next_char (current);
	}

	if (start != current) {
		gchar *new_address = NULL;
		new_address = g_strndup (start, current - last_blank);
		result = g_slist_prepend (result, new_address);
	}

	result = g_slist_reverse (result);
	return result;

}

void
modest_text_utils_address_range_at_position (const gchar *recipients_list,
					     gint position,
					     gint *start,
					     gint *end)
{
	gchar *current = NULL;
	gint range_start = 0;
	gint range_end = 0;
	gint index;
	gboolean is_quoted = FALSE;

	index = 0;
	for (current = (gchar *) recipients_list; *current != '\0'; current = g_utf8_find_next_char (current, NULL)) {
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
	if (strcmp (l, "-- ") == 0) {
		return -1 - indent;
	} else {
		return indent;
	}
}

static void
unquote_line (GString * l)
{
	gchar *p;

	p = l->str;
	while (p[0]) {
		if (p[0] == '>') {
			if (p[1] == ' ') {
				p++;
			}
		} else {
			break;
		}
		p++;
	}
	g_string_erase (l, 0, p - l->str);
}

static void
append_quoted (GString * buf, int indent, const GString * str,
	       const int cutpoint)
{
	int i;

	indent = indent < 0 ? abs (indent) - 1 : indent;
	for (i = 0; i <= indent; i++) {
		g_string_append (buf, "> ");
	}
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

static gchar *
modest_text_utils_quote_plain_text (const gchar *text, 
				    const gchar *cite, 
				    const gchar *signature,
				    GList *attachments,
				    int limit)
{
	const gchar *iter;
	gint indent, breakpoint, rem_indent = 0;
	GString *q, *l, *remaining;
	gsize len;
	gchar *attachments_string = NULL;

	/* remaining will store the rest of the line if we have to break it */
	q = g_string_new ("\n");
	q = g_string_append (q, cite);
	q = g_string_append_c (q, '\n');
	remaining = g_string_new ("");

	iter = text;
	len = strlen(text);
	do {
		l = get_next_line (text, len, iter);
		iter = iter + l->len + 1;
		indent = get_indent_level (l->str);
		unquote_line (l);

		if (remaining->len) {
			if (l->len && indent == rem_indent) {
				g_string_prepend (l, " ");
				g_string_prepend (l, remaining->str);
			} else {
				do {
					breakpoint =
						get_breakpoint (remaining->str,
								rem_indent,
								limit);
					append_quoted (q, rem_indent,
						       remaining, breakpoint);
					g_string_erase (remaining, 0,
							breakpoint);
					if (remaining->str[0] == ' ') {
						g_string_erase (remaining, 0,
								1);
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
		append_quoted (q, indent, l, breakpoint);
		g_string_free (l, TRUE);
	} while ((iter < text + len) || (remaining->str[0]));

	attachments_string = quoted_attachments (attachments);
	q = g_string_append (q, attachments_string);
	g_free (attachments_string);

	if (signature != NULL) {
		q = g_string_append_c (q, '\n');
		q = g_string_append (q, signature);
	}

	return g_string_free (q, FALSE);
}

static gchar*
modest_text_utils_quote_html (const gchar *text, 
			      const gchar *cite, 
			      const gchar *signature,
			      GList *attachments,
			      int limit)
{
	gchar *result = NULL;
	gchar *signature_result = NULL;
	const gchar *format = \
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" \
		"<html>\n" \
		"<body>\n" \
		"<br/>%s<br/>" \
		"<pre>%s<br/>%s<br/>%s</pre>\n" \
		"</body>\n" \
		"</html>\n";
	gchar *attachments_string = NULL;
	gchar *q_attachments_string = NULL;
	gchar *q_cite = NULL;
	gchar *html_text = NULL;

	if (signature == NULL)
		signature_result = g_strdup ("");
	else
		signature_result = modest_text_utils_convert_to_html_body (signature, -1, TRUE);

	attachments_string = quoted_attachments (attachments);
	q_attachments_string = modest_text_utils_convert_to_html_body (attachments_string, -1, TRUE);
	q_cite = modest_text_utils_convert_to_html_body (cite, -1, TRUE);
	html_text = modest_text_utils_convert_to_html_body (text, -1, TRUE);
	result = g_strdup_printf (format, signature_result, q_cite, html_text, q_attachments_string);
	g_free (q_cite);
	g_free (html_text);
	g_free (attachments_string);
	g_free (q_attachments_string);
	g_free (signature_result);
	
	return result;
}

static gint 
cmp_offsets_reverse (const url_match_t *match1, const url_match_t *match2)
{
	return match2->offset - match1->offset;
}

static gboolean url_matches_block = 0;
static url_match_pattern_t patterns[] = MAIL_VIEWER_URL_MATCH_PATTERNS;


static gboolean
compile_patterns ()
{
	guint i;
	const size_t pattern_num = sizeof(patterns)/sizeof(url_match_pattern_t);
	for (i = 0; i != pattern_num; ++i) {
		patterns[i].preg = g_slice_new0 (regex_t);
		
		/* this should not happen */
		g_return_val_if_fail (regcomp (patterns[i].preg, patterns[i].regex,
					       REG_ICASE|REG_EXTENDED|REG_NEWLINE) == 0, FALSE);
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
	if (url_matches_block == 0)
		compile_patterns ();
	url_matches_block ++;
}

void
modest_text_utils_hyperlinkify_end (void)
{
	url_matches_block--;
	if (url_matches_block <= 0)
		free_patterns ();
}


static GSList*
get_url_matches (GString *txt)
{
	regmatch_t rm;
        guint rv, i, offset = 0;
        GSList *match_list = NULL;

	const size_t pattern_num = sizeof(patterns)/sizeof(url_match_pattern_t);

	/* initalize the regexps */
	modest_text_utils_hyperlinkify_begin ();

        /* find all the matches */
	for (i = 0; i != pattern_num; ++i) {
		offset     = 0;	
		while (1) {
			url_match_t *match;
			gboolean is_submatch;
			GSList *cursor;
			
			if ((rv = regexec (patterns[i].preg, txt->str + offset, 1, &rm, 0)) != 0) {
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
				guint new_offset = offset + rm.rm_so;
				is_submatch = (new_offset >  old_match->offset &&
					       new_offset <  old_match->offset + old_match->len);
				cursor = g_slist_next (cursor);
			}

			if (!is_submatch) {
				/* make a list of our matches (<offset, len, prefix> tupels)*/
				match = g_slice_new (url_match_t);
				match->offset = offset + rm.rm_so;
				match->len    = rm.rm_eo - rm.rm_so;
				match->prefix = patterns[i].prefix;
				match_list = g_slist_prepend (match_list, match);
			}
				
			offset += rm.rm_eo;
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



static void
hyperlinkify_plain_text (GString *txt)
{
	GSList *cursor;
	GSList *match_list = get_url_matches (txt);

	/* we will work backwards, so the offsets stay valid */
	for (cursor = match_list; cursor; cursor = cursor->next) {

		url_match_t *match = (url_match_t*) cursor->data;
		gchar *url  = g_strndup (txt->str + match->offset, match->len);
		gchar *repl = NULL; /* replacement  */

		/* the prefix is NULL: use the one that is already there */
		repl = g_strdup_printf ("<a href=\"%s%s\">%s</a>",
					match->prefix ? match->prefix : EMPTY_STRING, 
					url, url);

		/* replace the old thing with our hyperlink
		 * replacement thing */
		g_string_erase  (txt, match->offset, match->len);
		g_string_insert (txt, match->offset, repl);
		
		g_free (url);
		g_free (repl);

		g_slice_free (url_match_t, match);	
	}
	
	g_slist_free (match_list);
}


/* for optimization reasons, we change the string in-place */
void
modest_text_utils_get_display_address (gchar *address)
{
	int i;
	
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
			if (G_UNLIKELY(i == 0))
				return; /* there's nothing else, leave it */
			else {
				address[i] = '\0'; /* terminate the string here */
				return;
			}
		}
	}
}





gchar *
modest_text_utils_get_email_address (const gchar *full_address)
{
	const gchar *left, *right;
	
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
	gint i;
	static const gchar* prefix[] = {
		"Re:", "RE:", "RV:", "re:"
		"Fwd:", "FWD:", "FW:", "fwd:", "Fw:", "fw:", NULL
	};
		
	if (!sub || (sub[0] != 'R' && sub[0] != 'F' && sub[0] != 'r' && sub[0] != 'f')) /* optimization */
		return 0;

	i = 0;
	
	while (prefix[i]) {
		if (g_str_has_prefix(sub, prefix[i])) {
			int prefix_len = strlen(prefix[i]); 
			if (sub[prefix_len] == ' ')
				++prefix_len; /* ignore space after prefix as well */
			return prefix_len; 
		}
		++i;
	}
	return 0;
}


gint
modest_text_utils_utf8_strcmp (const gchar* s1, const gchar *s2, gboolean insensitive)
{
	gint result = 0;
	gchar *n1, *n2;

	/* work even when s1 and/or s2 == NULL */
	if (G_UNLIKELY(s1 == s2))
		return 0;

	/* if it's not case sensitive */
	if (!insensitive)
		return strcmp (s1 ? s1 : "", s2 ? s2 : "");
	
	n1 = g_utf8_collate_key (s1 ? s1 : "", -1);
	n2 = g_utf8_collate_key (s2 ? s2 : "", -1);
	
	result = strcmp (n1, n2);

	g_free (n1);
	g_free (n2);
	
	return result;
}


const gchar*
modest_text_utils_get_display_date (time_t date)
{
	time_t now;
#define DATE_BUF_SIZE 64 
	static const guint ONE_DAY = 24 * 60 * 60; /* seconds in one day */
	static gchar date_buf[DATE_BUF_SIZE];

	gchar today_buf [DATE_BUF_SIZE];  

	modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, "%x", date); 

	now = time (NULL);

	/* we check if the date is within the last 24h, if not, we don't
	 * have to do the extra, expensive strftime, which was very visible
	 * in the profiles.
	 */
	if (abs(now - date) < ONE_DAY) {
		
		/* it's within the last 24 hours, but double check */
		/* use the localized dates */
		modest_text_utils_strftime (today_buf,  DATE_BUF_SIZE, "%x", now); 

		/* if it's today, use the time instead */
		if (strcmp (date_buf, today_buf) == 0)
			modest_text_utils_strftime (date_buf, DATE_BUF_SIZE, "%X", date);
	}
	
	return date_buf;
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
		"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6",
		"COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
		".", "..", NULL
	};
	
	/* cannot be NULL */
	if (!folder_name) 
		return FALSE;

	/* cannot be empty */
	len = strlen(folder_name);
	if (len == 0)
		return FALSE;
	
	/* cannot start or end with a space */
	if (g_ascii_isspace(folder_name[0]) || g_ascii_isspace(folder_name[len - 1]))
		return FALSE; 

	/* cannot contain a forbidden char */	
	for (i = 0; i < len; i++)
		if (modest_text_utils_is_forbidden_char (folder_name[i], FOLDER_NAME_FORBIDDEN_CHARS))
			return FALSE;
	
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
	const gchar* domain_regex = "^[a-z0-9]([.]?[a-z0-9-])*[a-z0-9]$";

	memset (&rx, 0, sizeof(regex_t)); /* coverity wants this... */
	
	if (!domain)
		return FALSE;
	
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
modest_text_utils_validate_email_address (const gchar *email_address, const gchar **invalid_char_position)
{
	int count = 0;
	const gchar *c = NULL, *domain = NULL;
	static gchar *rfc822_specials = "()<>@,;:\\\"[]&";

	if (invalid_char_position != NULL)
		*invalid_char_position = NULL;
	
	/* check that the email adress contains exactly one @ */
	if (!strstr(email_address, "@") || 
			(strstr(email_address, "@") != g_strrstr(email_address, "@")))
	{
		return FALSE;
	}
	
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
				break;
			}
		}
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
		return g_strdup_printf(_FM("sfil_li_size_kb"), 0);
	if (0 < size && size < KB)
		return g_strdup_printf (_FM("sfil_li_size_kb"), 1);
	else if (KB <= size && size < 100 * KB)
		return g_strdup_printf (_FM("sfil_li_size_1kb_99kb"), size / KB);
	else if (100*KB <= size && size < MB)
		return g_strdup_printf (_FM("sfil_li_size_100kb_1mb"), (float) size / MB);
	else if (MB <= size && size < 10*MB)
		return g_strdup_printf (_FM("sfil_li_size_1mb_10mb"), (float) size / MB);
	else if (10*MB <= size && size < GB)
		return g_strdup_printf (_FM("sfil_li_size_10mb_1gb"), size / MB);
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

	g_return_val_if_fail (GTK_IS_TEXT_BUFFER (buffer), NULL);

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

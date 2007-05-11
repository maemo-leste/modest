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


#include <glib.h>
#include <string.h>
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
#define FORWARD_STRING _("-----Forwarded Message-----")
#define FROM_STRING _("From:")
#define SENT_STRING _("Sent:")
#define TO_STRING _("To:")
#define	SUBJECT_STRING _("Subject:")
#define EMPTY_STRING ""

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
	  NULL, "http://" },\
	{ "ftp\\.[-a-z0-9.]+[-a-z0-9](:[0-9]*)?(/[-A-Za-z0-9_$.+!*(),;:@%&=?/~#]*[^]}\\),?!;:\"]?)?",\
	  NULL, "ftp://" },\
	{ "(voipto|callto|chatto|jabberto|xmpp):[-_a-z@0-9.\\+]+", \
	   NULL, NULL},						    \
	{ "mailto:[-_a-z0-9.\\+]+@[-_a-z0-9.]+",		    \
	  NULL, NULL},\
	{ "[-_a-z0-9.\\+]+@[-_a-z0-9.]+",\
	  NULL, "mailto:"}\
	}

/* private */
static gchar*   cite                    (const time_t sent_date, const gchar *from);
static void     hyperlinkify_plain_text (GString *txt);
static gint     cmp_offsets_reverse     (const url_match_t *match1, const url_match_t *match2);
static void     chk_partial_match       (const url_match_t *match, guint* offset);
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
						    int limit);

static gchar*   modest_text_utils_quote_html       (const gchar *text, 
						    const gchar *cite, 
						    int limit);


/* ******************************************************************* */
/* ************************* PUBLIC FUNCTIONS ************************ */
/* ******************************************************************* */

gchar *
modest_text_utils_quote (const gchar *text, 
			 const gchar *content_type,
			 const gchar *signature,
			 const gchar *from,
			 const time_t sent_date, 
			 int limit)
{
	gchar *retval, *cited;

	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);

	cited = cite (sent_date, from);
	
	if (content_type && strcmp (content_type, "text/html") == 0)
		/* TODO: extract the <body> of the HTML and pass it to
		   the function */
		retval = modest_text_utils_quote_html (text, cited, limit);
	else
		retval = modest_text_utils_quote_plain_text (text, cited, limit);
	
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
	gchar *tmp, *retval;
	gchar *tmp_sig;

	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);

	if (!signature)
		tmp_sig = g_strdup ("");
	else if (!strcmp(content_type, "text/html")) {
		tmp_sig = modest_text_utils_convert_to_html_body(signature);
	} else {
		tmp_sig = g_strdup (signature);
	}

	tmp = cite (sent_date, from);
	retval = g_strdup_printf ("%s%s%s\n", tmp_sig, tmp, text);
	g_free (tmp_sig);
	g_free (tmp);

	return retval;
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
	gchar *formatted_signature;
	const gchar *plain_format = "%s%s\n%s %s\n%s %s\n%s %s\n%s %s\n\n%s";
	const gchar *html_format = \
		"%s%s<br>\n<table width=\"100%\" border=\"0\" cellspacing=\"2\" cellpadding=\"2\">\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<br><br>%s";
	const gchar *format;

	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);
	g_return_val_if_fail (text, NULL);
	
	modest_text_utils_strftime (sent_str, 100, "%c", sent_date);

	if (!strcmp (content_type, "text/html"))
		/* TODO: extract the <body> of the HTML and pass it to
		   the function */
		format = html_format;
	else
		format = plain_format;

	if (signature != NULL) {
		if (!strcmp (content_type, "text/html")) {
			formatted_signature = g_strconcat (signature, "<br/>", NULL);
		} else {
			formatted_signature = g_strconcat (signature, "\n", NULL);
		}
	} else {
		formatted_signature = "";
	}

	return g_strdup_printf (format, formatted_signature, 
				FORWARD_STRING,
				FROM_STRING, (from) ? from : EMPTY_STRING,
				SENT_STRING, sent_str,
				TO_STRING, (to) ? to : EMPTY_STRING,
				SUBJECT_STRING, (subject) ? subject : EMPTY_STRING,
				text);
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
	
	if (!subject)
		return g_strdup (prefix);

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

	g_return_val_if_fail (address_list, NULL);

	if (!address)
		return g_strdup (address_list);
	
	/* search for substring */
	if (!strstr ((const char *) address_list, (const char *) address))
		return g_strdup (address_list);

	dup = g_strdup (address_list);
	filtered_emails = g_string_new (NULL);
	
	token = strtok_r (dup, ",", &ptr);

	while (token != NULL) {
		/* Add to list if not found */
		if (!strstr ((const char *) token, (const char *) address)) {
			if (filtered_emails->len == 0)
				g_string_append_printf (filtered_emails, "%s", g_strstrip (token));
			else
				g_string_append_printf (filtered_emails, ",%s", g_strstrip (token));
		}
		token = strtok_r (NULL, ",", &ptr);
	}
	result = filtered_emails->str;

	/* Clean */
	g_free (dup);
	g_string_free (filtered_emails, FALSE);

	return result;
}

static void
modest_text_utils_convert_buffer_to_html (GString *html, const gchar *data)
{
	guint		 i;
	gboolean	space_seen = FALSE;
	gsize           len;

	len = strlen (data);

	/* replace with special html chars where needed*/
	for (i = 0; i != len; ++i)  {
		char kar = data[i];
		
		if (space_seen && kar != ' ') {
			g_string_append_c (html, ' ');
			space_seen = FALSE;
		}
		
		switch (kar) {
		case 0:  break; /* ignore embedded \0s */	
		case '<'  : g_string_append (html, "&lt;");   break;
		case '>'  : g_string_append (html, "&gt;");   break;
		case '&'  : g_string_append (html, "&amp;");  break;
		case '"'  : g_string_append (html, "&quot;");  break;
		case '\'' : g_string_append (html, "&apos;"); break;
		case '\n' : g_string_append (html, "<br>\n");  break;
		case '\t' : g_string_append (html, "&nbsp;&nbsp;&nbsp; "); break; /* note the space at the end*/
		case ' ':
			if (space_seen) { /* second space in a row */
				g_string_append (html, "&nbsp ");
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

	modest_text_utils_convert_buffer_to_html (html, data);
	
	g_string_append (html, "</body></html>");
	hyperlinkify_plain_text (html);

	return g_string_free (html, FALSE);
}

gchar *
modest_text_utils_convert_to_html_body (const gchar *data)
{
	GString		*html;	    
	gsize           len;
	
	if (!data)
		return NULL;

	len = strlen (data);
	html = g_string_sized_new (1.5 * len);	/* just a  guess... */

	modest_text_utils_convert_buffer_to_html (html, data);

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
	gchar sent_str[101];

	/* format sent_date */
	modest_text_utils_strftime (sent_str, 100, "%c", sent_date);
	return g_strdup_printf (N_("On %s, %s wrote:\n"), 
				sent_str, 
				(from) ? from : EMPTY_STRING);
}


static gchar *
modest_text_utils_quote_plain_text (const gchar *text, 
				    const gchar *cite, 
				    int limit)
{
	const gchar *iter;
	gint indent, breakpoint, rem_indent = 0;
	GString *q, *l, *remaining;
	gsize len;

	/* remaining will store the rest of the line if we have to break it */
	q = g_string_new (cite);
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

	return g_string_free (q, FALSE);
}

static gchar*
modest_text_utils_quote_html (const gchar *text, 
			      const gchar *cite, 
			      int limit)
{
	const gchar *format = \
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" \
		"<html>\n" \
		"<body>\n" \
		"%s" \
		"<blockquote type=\"cite\">\n%s\n</blockquote>\n" \
		"</body>\n" \
		"</html>\n";

	return g_strdup_printf (format, cite, text);
}

static gint 
cmp_offsets_reverse (const url_match_t *match1, const url_match_t *match2)
{
	return match2->offset - match1->offset;
}



/*
 * check if the match is inside an existing match... */
static void
chk_partial_match (const url_match_t *match, guint* offset)
{
	if (*offset >= match->offset && *offset < match->offset + match->len)
		*offset = -1;
}

static GSList*
get_url_matches (GString *txt)
{
	regmatch_t rm;
        guint rv, i, offset = 0;
        GSList *match_list = NULL;

	static url_match_pattern_t patterns[] = MAIL_VIEWER_URL_MATCH_PATTERNS;
	const size_t pattern_num = sizeof(patterns)/sizeof(url_match_pattern_t);

	/* initalize the regexps */
	for (i = 0; i != pattern_num; ++i) {
		patterns[i].preg = g_slice_new0 (regex_t);

		/* this should not happen */
		g_return_val_if_fail (regcomp (patterns[i].preg, patterns[i].regex,
					       REG_ICASE|REG_EXTENDED|REG_NEWLINE) == 0, NULL);
	}
        /* find all the matches */
	for (i = 0; i != pattern_num; ++i) {
		offset     = 0;	
		while (1) {
			int test_offset;
			if ((rv = regexec (patterns[i].preg, txt->str + offset, 1, &rm, 0)) != 0) {
				g_return_val_if_fail (rv == REG_NOMATCH, NULL); /* this should not happen */
				break; /* try next regexp */ 
			}
			if (rm.rm_so == -1)
				break;

			/* FIXME: optimize this */
			/* to avoid partial matches on something that was already found... */
			/* check_partial_match will put -1 in the data ptr if that is the case */
			test_offset = offset + rm.rm_so;
			g_slist_foreach (match_list, (GFunc)chk_partial_match, &test_offset);
			
			/* make a list of our matches (<offset, len, prefix> tupels)*/
			if (test_offset != -1) {
				url_match_t *match = g_slice_new (url_match_t);
				match->offset = offset + rm.rm_so;
				match->len    = rm.rm_eo - rm.rm_so;
				match->prefix = patterns[i].prefix;
				match_list = g_slist_prepend (match_list, match);
			}
			offset += rm.rm_eo;
		}
	}

	for (i = 0; i != pattern_num; ++i) {
		regfree (patterns[i].preg);
		g_slice_free  (regex_t, patterns[i].preg);
	} /* don't free patterns itself -- it's static */
	
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



gchar*
modest_text_utils_get_display_address (gchar *address)
{
	gchar *cursor;
	
	if (!address)
		return NULL;
	
	g_return_val_if_fail (g_utf8_validate (address, -1, NULL), NULL);
	
	g_strchug (address); /* remove leading whitespace */

	/*  <email@address> from display name */
	cursor = g_strstr_len (address, strlen(address), "<");
	if (cursor == address) /* there's nothing else? leave it */
		return address;
	if (cursor) 
		cursor[0]='\0';

	/* remove (bla bla) from display name */
	cursor = g_strstr_len (address, strlen(address), "(");
	if (cursor == address) /* there's nothing else? leave it */
		return address;
	if (cursor) 
		cursor[0]='\0';

	g_strchomp (address); /* remove trailing whitespace */

	return address;
}



gint 
modest_text_utils_get_subject_prefix_len (const gchar *sub)
{
	gint i;
	static const gchar* prefix[] = {
		"Re:", "RE:", "Fwd:", "FWD:", "FW:", NULL
	};
		
	if (!sub || (sub[0] != 'R' && sub[0] != 'F')) /* optimization */
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


gchar*
modest_text_utils_get_display_date (time_t date)
{
	time_t now;
	const guint BUF_SIZE = 64; 
	gchar date_buf[BUF_SIZE];  
	gchar now_buf [BUF_SIZE];  
	
	now = time (NULL);

	modest_text_utils_strftime (date_buf, BUF_SIZE, "%d/%m/%Y", date);
	modest_text_utils_strftime (now_buf,  BUF_SIZE, "%d/%m/%Y",  now); /* today */
/* 	modest_text_utils_strftime (date_buf, BUF_SIZE, "%x", date); */
/* 	modest_text_utils_strftime (now_buf,  BUF_SIZE, "%x",  now); /\* today *\/ */
	
	/* if this is today, get the time instead of the date */
	if (strcmp (date_buf, now_buf) == 0)
		modest_text_utils_strftime (date_buf, BUF_SIZE, "%H:%M %P", date);
	
	return g_strdup(date_buf);
}

gboolean 
modest_text_utils_validate_email_address (const gchar *email_address)
{
	int count = 0;
	const gchar *c = NULL, *domain = NULL;
	static gchar *rfc822_specials = "()<>@,;:\\\"[]";

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
		if (strchr(rfc822_specials, *c)) 
			return FALSE;
	}
	if (c == email_address || *(c - 1) == '.') 
		return FALSE;

	/* next we validate the domain portion (name@domain) */
	if (!*(domain = ++c)) 
		return FALSE;
	do {
		if (*c == '.') {
			if (c == domain || *(c - 1) == '.') 
				return FALSE;
			count++;
		}
		if (*c <= ' ' || *c >= 127) 
			return FALSE;
		if (strchr(rfc822_specials, *c)) 
			return FALSE;
	} while (*++c);

	return (count >= 1) ? TRUE : FALSE;
}

gboolean 
modest_text_utils_validate_recipient (const gchar *recipient)
{
	gchar *stripped, *current;
	gchar *right_part;
	gboolean has_error = FALSE;

	if (modest_text_utils_validate_email_address (recipient))
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
		valid = modest_text_utils_validate_email_address (address);
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
		return g_strdup_printf (_FM("sfil_li_size_100kb_1mb"), size / MB);
	else if (MB <= size && size < 10*MB)
		return g_strdup_printf (_FM("sfil_li_size_1mb_10mb"), size / MB);
	else if (10*MB <= size && size < GB)
		return g_strdup_printf (_FM("sfil_li_size_10mb_1gb"), size / MB);
	else
		return g_strdup_printf (_FM("sfil_li_size_1gb_or_greater"), size / GB);	
}

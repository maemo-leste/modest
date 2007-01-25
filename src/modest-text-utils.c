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
#include "modest-text-utils.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

/* defines */
#define FORWARD_STRING _("-----Forwarded Message-----")
#define FROM_STRING _("From:")
#define SENT_STRING _("Sent:")
#define TO_STRING _("To:")
#define	SUBJECT_STRING _("Subject:")

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
			 const gchar *from,
			 const time_t sent_date, 
			 int limit)
{
	gchar *retval, *cited;

	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);
	g_return_val_if_fail (from, NULL);

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
			const gchar *from,
			time_t sent_date)
{
	gchar *tmp, *retval;

	tmp = cite (sent_date, from);
	retval = g_strdup_printf ("%s%s\n", tmp, text);
	g_free (tmp);

	return retval;
}

gchar * 
modest_text_utils_inline (const gchar *text,
			  const gchar *content_type,
			  const gchar *from,
			  time_t sent_date,
			  const gchar *to,
			  const gchar *subject)
{
	gchar sent_str[101];
	const gchar *plain_format = "%s\n%s %s\n%s %s\n%s %s\n%s %s\n\n%s";
	const gchar *html_format = \
		"%s<br>\n<table width=\"100%\" border=\"0\" cellspacing=\"2\" cellpadding=\"2\">\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<tr><td>%s</td><td>%s</td></tr>\n" \
		"<br><br>%s";
	const gchar *format;

	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (content_type, NULL);
	g_return_val_if_fail (from, NULL);
	g_return_val_if_fail (text, NULL);
	g_return_val_if_fail (to, NULL);
	g_return_val_if_fail (subject, NULL);
	
	modest_text_utils_strftime (sent_str, 100, "%c", localtime (&sent_date));

	if (!strcmp (content_type, "text/html"))
		/* TODO: extract the <body> of the HTML and pass it to
		   the function */
		format = html_format;
	else
		format = plain_format;

	return g_strdup_printf (format, 
				FORWARD_STRING,
				FROM_STRING, from,
				SENT_STRING, sent_str,
				TO_STRING, to,
				SUBJECT_STRING, subject,
				text);
}

/* just to prevent warnings:
 * warning: `%x' yields only last 2 digits of year in some locales
 */
size_t
modest_text_utils_strftime(char *s, size_t max, const char  *fmt, const  struct tm *tm)
{
	return strftime(s, max, fmt, tm);
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

gchar*
modest_text_utils_convert_to_html (const gchar *data)
{
	guint		 i;
	gboolean	 first_space = TRUE;
	GString		*html;	    
	gsize           len;

	if (!data)
		return NULL;

	len = strlen (data);
	html = g_string_sized_new (len + 100);	/* just a  guess... */
	
	g_string_append_printf (html,
				"<html>"
				"<head>"
				"<meta http-equiv=\"content-type\""
				" content=\"text/html; charset=utf8\">"
				"</head>"
				"<body><tt>");
	
	/* replace with special html chars where needed*/
	for (i = 0; i != len; ++i)  {
		char	kar = data[i]; 
		switch (kar) {
			
		case 0:  break; /* ignore embedded \0s */	
		case '<' : g_string_append   (html, "&lt;"); break;
		case '>' : g_string_append   (html, "&gt;"); break;
		case '&' : g_string_append   (html, "&quot;"); break;
		case '\n': g_string_append   (html, "<br>\n"); break;
		default:
			if (kar == ' ') {
				g_string_append (html, first_space ? " " : "&nbsp;");
				first_space = FALSE;
			} else	if (kar == '\t')
				g_string_append (html, "&nbsp; &nbsp;&nbsp;");
			else {
				int charnum = 0;
				first_space = TRUE;
				/* optimization trick: accumulate 'normal' chars, then copy */
				do {
					kar = data [++charnum + i];
					
				} while ((i + charnum < len) &&
					 (kar > '>' || (kar != '<' && kar != '>'
							&& kar != '&' && kar !=  ' '
							&& kar != '\n' && kar != '\t')));
				g_string_append_len (html, &data[i], charnum);
				i += (charnum  - 1);
			}
		}
	}
	
	g_string_append (html, "</tt></body></html>");
	hyperlinkify_plain_text (html);

	return g_string_free (html, FALSE);
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
	modest_text_utils_strftime (sent_str, 100, "%c", localtime (&sent_date));
	return g_strdup_printf (N_("On %s, %s wrote:\n"), sent_str, from);
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
						get_breakpoint (remaining->	str,
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
		patterns[i].preg = g_new0 (regex_t,1);
		g_assert(regcomp (patterns[i].preg, patterns[i].regex,
				  REG_ICASE|REG_EXTENDED|REG_NEWLINE) == 0);
	}
        /* find all the matches */
	for (i = 0; i != pattern_num; ++i) {
		offset     = 0;	
		while (1) {
			int test_offset;
			if ((rv = regexec (patterns[i].preg, txt->str + offset, 1, &rm, 0)) != 0) {
				g_assert (rv == REG_NOMATCH); /* this should not happen */
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
				url_match_t *match = g_new (url_match_t,1);
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
		g_free  (patterns[i].preg);
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
					match->prefix ? match->prefix : "", url, url);

		/* replace the old thing with our hyperlink
		 * replacement thing */
		g_string_erase  (txt, match->offset, match->len);
		g_string_insert (txt, match->offset, repl);
		
		g_free (url);
		g_free (repl);

		g_free (cursor->data);	
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

static GHashTable*
get_display_date_cache (void)
{
	TnyPlatformFactory *fakt;
	ModestCacheMgr     *cache_mgr;

	fakt = modest_tny_platform_factory_get_instance ();
	
	cache_mgr =  modest_tny_platform_factory_get_cache_mgr_instance
		(MODEST_TNY_PLATFORM_FACTORY(fakt));
	
	return modest_cache_mgr_get_cache (cache_mgr,
					   MODEST_CACHE_MGR_CACHE_TYPE_DATE_STRING);
}



const gchar*
modest_text_utils_get_display_date (time_t date)
{
	static GHashTable *date_cache = NULL;

	struct tm date_tm, now_tm; 
	time_t now;

	const guint BUF_SIZE = 64; 
	gchar date_buf[BUF_SIZE];  
	gchar now_buf [BUF_SIZE];  
	gchar* cached_val;
	
	if (G_UNLIKELY(!date_cache))
		date_cache = get_display_date_cache ();
	
	cached_val = g_hash_table_lookup (date_cache, &date);
	if (cached_val)
		return cached_val;
						    
	now = time (NULL);
	
	localtime_r(&now, &now_tm);
	localtime_r(&date, &date_tm);

	/* get today's date */
	modest_text_utils_strftime (date_buf, BUF_SIZE, "%x", &date_tm);
	modest_text_utils_strftime (now_buf,  BUF_SIZE, "%x",  &now_tm);
	/* today */

	/* if this is today, get the time instead of the date */
	if (strcmp (date_buf, now_buf) == 0)
		strftime (date_buf, BUF_SIZE, _("%X"), &date_tm); 

	cached_val = g_strdup(date_buf);
	g_hash_table_insert (date_cache, (gpointer)&date, (gpointer)cached_val);
	
	return cached_val;
}

gboolean 
modest_text_utils_validate_email_address (const gchar *email_address)
{
	int count = 0;
	const gchar *c, *domain;
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




gchar *
modest_text_utils_get_display_size (guint size)
{
	const guint KB=1024;
	const guint MB=1024 * KB;
	const guint GB=1024 * MB;
	const guint TB=1024 * GB;

	if (size < KB)
		return g_strdup_printf (_("%0.1f Kb"), (double)size / KB);
	else if (size < MB)
		return g_strdup_printf (_("%d Kb"), size / KB);
	else if (size < GB)
		return g_strdup_printf (_("%d Mb"), size / MB);
	else if (size < TB)
		return g_strdup_printf (_("%d Gb"), size/ GB);
	else
		return g_strdup_printf (_("Very big"));
}

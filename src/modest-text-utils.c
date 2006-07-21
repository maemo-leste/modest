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


/* modest-ui.c */

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "modest-text-utils.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H */

/* private */
static GString *get_next_line (const char *b, const gsize blen, const gchar * iter);
static int get_indent_level (const char *l);
static void unquote_line (GString * l);
static void append_quoted (GString * buf, const int indent,
			   const GString * str, const int cutpoint);
static int get_breakpoint_utf8 (const gchar * s, const gint indent,
				const gint limit);
static int get_breakpoint_ascii (const gchar * s, const gint indent,
				 const gint limit);
static int get_breakpoint (const gchar * s, const gint indent,
			   const gint limit);

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



/* just to prevent warnings:
 * warning: `%x' yields only last 2 digits of year in some locales
 */
static size_t
my_strftime(char *s, size_t max, const char  *fmt,  const
	    struct tm *tm) {
	return strftime(s, max, fmt, tm);
}


gchar *
modest_text_utils_quote (const gchar * to_quote, const gchar * from,
			 const time_t sent_date, const int limit)
{
	const gchar *iter;
	gint indent, breakpoint, rem_indent = 0;
	gchar sent_str[101];
	GString *q, *l, *remaining;
	gsize len;

	/* format sent_date */
	my_strftime (sent_str, 100, "%c", localtime (&sent_date));
	q = g_string_new ("");
	g_string_printf (q, "On %s, %s wrote:\n", sent_str, from);

	/* remaining will store the rest of the line if we have to break it */
	remaining = g_string_new ("");

	iter = to_quote;
	len = strlen(to_quote);
	do {
		l = get_next_line (to_quote, len, iter);
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
	} while ((iter < to_quote + len) || (remaining->str[0]));

	return g_string_free (q, FALSE);
}

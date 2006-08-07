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

#include <tny-text-buffer-stream.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtkhtml/gtkhtml.h>
#include <gtkhtml/gtkhtml-stream.h>
#include <tny-list-iface.h>
#include <tny-list.h>

#include <modest-tny-msg-actions.h>
#include "modest-msg-view.h"
#include "modest-tny-stream-gtkhtml.h"


/* 'private'/'protected' functions */
static void     modest_msg_view_class_init   (ModestMsgViewClass *klass);
static void     modest_msg_view_init         (ModestMsgView *obj);
static void     modest_msg_view_finalize     (GObject *obj);


static GSList*  get_url_matches (GString *txt);
static gboolean on_link_clicked (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view);
static gboolean on_url_requested (GtkWidget *widget, const gchar *uri, GtkHTMLStream *stream,
				  ModestMsgView *msg_view);
static gboolean on_link_hover (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view);

/*
 * we need these regexps to find URLs in plain text e-mails
 */
typedef struct _UrlMatchPattern UrlMatchPattern;
struct _UrlMatchPattern {
	gchar   *regex;
	regex_t *preg;
	gchar   *prefix;
};

#define ATT_PREFIX "att:"

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


/* list my signals */
enum {
	LINK_CLICKED_SIGNAL,
	LINK_HOVER_SIGNAL,
	ATTACHMENT_CLICKED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestMsgViewPrivate ModestMsgViewPrivate;
struct _ModestMsgViewPrivate {

	GtkWidget   *gtkhtml;
	TnyMsgIface *msg;

	gulong  sig1, sig2, sig3;
};
#define MODEST_MSG_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_MSG_VIEW, \
                                                 ModestMsgViewPrivate))
/* globals */
static GtkContainerClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

GType
modest_msg_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMsgViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_msg_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMsgView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_msg_view_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_SCROLLED_WINDOW,
		                                  "ModestMsgView",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_msg_view_class_init (ModestMsgViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_view_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewPrivate));

		
 	signals[LINK_CLICKED_SIGNAL] =
 		g_signal_new ("link_clicked",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, link_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	
	signals[ATTACHMENT_CLICKED_SIGNAL] =
 		g_signal_new ("attachment_clicked",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, attachment_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_INT);
	
	signals[LINK_HOVER_SIGNAL] =
		g_signal_new ("link_hover",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestMsgViewClass, link_hover),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
modest_msg_view_init (ModestMsgView *obj)
{
 	ModestMsgViewPrivate *priv;
	
	priv = MODEST_MSG_VIEW_GET_PRIVATE(obj);

	priv->msg                     = NULL;
	priv->gtkhtml                 = gtk_html_new();
	
	gtk_html_set_editable        (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_allow_selection     (GTK_HTML(priv->gtkhtml), TRUE);
	gtk_html_set_caret_mode      (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_set_blocking        (GTK_HTML(priv->gtkhtml), FALSE);
	gtk_html_set_images_blocking (GTK_HTML(priv->gtkhtml), FALSE);

	priv->sig1 = g_signal_connect (G_OBJECT(priv->gtkhtml), "link_clicked",
				       G_CALLBACK(on_link_clicked), obj);
	priv->sig2 = g_signal_connect (G_OBJECT(priv->gtkhtml), "url_requested",
				       G_CALLBACK(on_url_requested), obj);
	priv->sig3 = g_signal_connect (G_OBJECT(priv->gtkhtml), "on_url",
				       G_CALLBACK(on_link_hover), obj);
}
	

static void
modest_msg_view_finalize (GObject *obj)
{	
	ModestMsgViewPrivate *priv;
	priv = MODEST_MSG_VIEW_GET_PRIVATE (obj);

	if (priv->msg) {
		g_object_unref (G_OBJECT(priv->msg));
		priv->msg = NULL;
	}
	
	/* we cannot disconnect sigs, because priv->gtkhtml is
	 * already dead */
	
	priv->gtkhtml = NULL;
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}


GtkWidget*
modest_msg_view_new (TnyMsgIface *msg)
{
	GObject *obj;
	ModestMsgView* self;
	ModestMsgViewPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_MSG_VIEW, NULL));
	self = MODEST_MSG_VIEW(obj);
	priv = MODEST_MSG_VIEW_GET_PRIVATE (self);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	if (priv->gtkhtml) 
		gtk_container_add (GTK_CONTAINER(obj), priv->gtkhtml);
	
	if (msg)
		modest_msg_view_set_message (self, msg);
	
	return GTK_WIDGET(self);
}


static gboolean
on_link_clicked (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view)
{
	int index;

	g_return_val_if_fail (msg_view, FALSE);
	
	/* is it an attachment? */
	if (g_str_has_prefix(uri, ATT_PREFIX)) {

		index = atoi (uri + strlen(ATT_PREFIX));
		
		if (index == 0) {
			/* index is 1-based, so 0 indicates an error */
			g_printerr ("modest: invalid attachment id: %s\n", uri);
			return FALSE;
		}

		g_signal_emit (G_OBJECT(msg_view), signals[ATTACHMENT_CLICKED_SIGNAL],
			       0, index);
		return FALSE;
	}

	g_signal_emit (G_OBJECT(msg_view), signals[LINK_CLICKED_SIGNAL],
		       0, uri);

	return FALSE;
}



static gboolean
on_link_hover (GtkWidget *widget, const gchar *uri, ModestMsgView *msg_view)
{
	if (uri && g_str_has_prefix (uri, ATT_PREFIX))
		return FALSE;

	g_signal_emit (G_OBJECT(msg_view), signals[LINK_HOVER_SIGNAL],
		       0, uri);

	return FALSE;
}



static TnyMimePartIface *
find_cid_image (TnyMsgIface *msg, const gchar *cid)
{
	TnyMimePartIface *part = NULL;
	TnyListIface *parts;
	TnyIteratorIface *iter;
	
	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (cid, NULL);
	
	parts  = TNY_LIST_IFACE (tny_list_new());

	tny_msg_iface_get_parts (msg, parts); 
	iter   = tny_list_iface_create_iterator (parts);
	
	while (!tny_iterator_iface_is_done(iter)) {
		const gchar *part_cid;
		part = TNY_MIME_PART_IFACE(tny_iterator_iface_current(iter));
		part_cid = tny_mime_part_iface_get_content_id (part);

		if (part_cid && strcmp (cid, part_cid) == 0)
			break;

		part = NULL;
		tny_iterator_iface_next (iter);
	}
	
	if (part)
		g_object_ref (G_OBJECT(part));

	g_object_unref (G_OBJECT(iter));	
	g_object_unref (G_OBJECT(parts));
	
	return part;
}


static gboolean
on_url_requested (GtkWidget *widget, const gchar *uri,
		  GtkHTMLStream *stream,
		  ModestMsgView *msg_view)
{
	ModestMsgViewPrivate *priv;
	priv = MODEST_MSG_VIEW_GET_PRIVATE (msg_view);
	
	if (g_str_has_prefix (uri, "cid:")) {
		/* +4 ==> skip "cid:" */
		TnyMimePartIface *part = find_cid_image (priv->msg, uri + 4);
		if (!part) {
			g_printerr ("modest: '%s' not found\n", uri + 4);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_ERROR);
		} else {
			TnyStreamIface *tny_stream =
				TNY_STREAM_IFACE(modest_tny_stream_gtkhtml_new(stream));
			tny_mime_part_iface_decode_to_stream ((TnyMimePartIface*)part,
								  tny_stream);
			gtk_html_stream_close (stream, GTK_HTML_STREAM_OK);
	
			g_object_unref (G_OBJECT(tny_stream));
			g_object_unref (G_OBJECT(part));
		}
	}

	return TRUE;
}


typedef struct  {
	guint offset;
	guint len;
	const gchar* prefix;
} url_match_t;



/* render the attachments as hyperlinks in html */
static gchar*
attachments_as_html (ModestMsgView *self, TnyMsgIface *msg)
{
	ModestMsgViewPrivate *priv;
	GString *appendix;
	TnyListIface *parts;
	TnyIteratorIface *iter;
	gchar *html;
	int index = 0;
	
	if (!msg)
		return NULL;

	priv  = MODEST_MSG_VIEW_GET_PRIVATE (self);

	parts = TNY_LIST_IFACE(tny_list_new());
	tny_msg_iface_get_parts (msg, parts);
	iter  = tny_list_iface_create_iterator (parts);
	
	appendix= g_string_new ("");
	
	while (!tny_iterator_iface_is_done(iter)) {
		TnyMimePartIface *part;

		++index; /* attachment numbers are 1-based */
		
		part = TNY_MIME_PART_IFACE(tny_iterator_iface_current (iter));

		if (tny_mime_part_iface_is_attachment (part)) {

			const gchar *filename = tny_mime_part_iface_get_filename(part);
			if (!filename)
				filename = _("attachment");

			g_string_append_printf (appendix, "<a href=\"%s%d\">%s</a> \n",
						ATT_PREFIX, index, filename);			 
		}
		tny_iterator_iface_next (iter);
	}
	g_object_unref (G_OBJECT(iter));
	
	if (appendix->len == 0) 
		return g_string_free (appendix, TRUE);

	html = g_strdup_printf ("<strong>%s:</strong> %s\n<hr>",
				_("Attachments"), appendix->str);			 
	g_string_free (appendix, TRUE);
	
	return html;
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



static gchar *
convert_to_html (const gchar *data)
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

	static UrlMatchPattern patterns[] = MAIL_VIEWER_URL_MATCH_PATTERNS;
	const size_t pattern_num = sizeof(patterns)/sizeof(UrlMatchPattern);

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



static gboolean
set_html_message (ModestMsgView *self, TnyMimePartIface *tny_body, TnyMsgIface *msg)
{
	gchar *html_attachments;
	TnyStreamIface *gtkhtml_stream;	
	ModestMsgViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (tny_body, FALSE);
	
	priv = MODEST_MSG_VIEW_GET_PRIVATE(self);

	gtkhtml_stream =
		TNY_STREAM_IFACE(modest_tny_stream_gtkhtml_new
				 (gtk_html_begin(GTK_HTML(priv->gtkhtml))));
	
	tny_stream_iface_reset (gtkhtml_stream);
	
	html_attachments = attachments_as_html(self, msg);
	if (html_attachments) {
		tny_stream_iface_write (gtkhtml_stream, html_attachments,
					strlen(html_attachments));
		tny_stream_iface_reset (gtkhtml_stream);
		g_free (html_attachments);
	}

	// FIXME: tinymail
	tny_mime_part_iface_decode_to_stream ((TnyMimePartIface*)tny_body,
						  gtkhtml_stream);

	g_object_unref (G_OBJECT(gtkhtml_stream));
	
	return TRUE;
}


/* this is a hack --> we use the tny_text_buffer_stream to
 * get the message text, then write to gtkhtml 'by hand' */
static gboolean
set_text_message (ModestMsgView *self, TnyMimePartIface *tny_body, TnyMsgIface *msg)
{
	GtkTextBuffer *buf;
	GtkTextIter begin, end;
	TnyStreamIface* txt_stream, *gtkhtml_stream;
	gchar *txt, *html_attachments;
	ModestMsgViewPrivate *priv;
		
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (tny_body, FALSE);

	priv           = MODEST_MSG_VIEW_GET_PRIVATE(self);
	
	buf            = gtk_text_buffer_new (NULL);
	txt_stream     = TNY_STREAM_IFACE(tny_text_buffer_stream_new (buf));
		
	tny_stream_iface_reset (txt_stream);
	
	gtkhtml_stream =
		TNY_STREAM_IFACE(modest_tny_stream_gtkhtml_new
				 (gtk_html_begin(GTK_HTML(priv->gtkhtml))));

	html_attachments = attachments_as_html(self, msg);
	if (html_attachments) {
		tny_stream_iface_write (gtkhtml_stream, html_attachments,
					strlen(html_attachments));
		tny_stream_iface_reset (gtkhtml_stream);
		g_free (html_attachments);
	}

	// FIXME: tinymail
	tny_mime_part_iface_decode_to_stream ((TnyMimePartIface*)tny_body,
						  txt_stream);
	tny_stream_iface_reset (txt_stream);		
	
	gtk_text_buffer_get_bounds (buf, &begin, &end);
	txt = gtk_text_buffer_get_text (buf, &begin, &end, FALSE);
	if (txt) {
		gchar *html = convert_to_html (txt);
		tny_stream_iface_write (gtkhtml_stream, html, strlen(html));
		tny_stream_iface_reset (gtkhtml_stream);
		g_free (txt);
		g_free (html);
	}
	
	g_object_unref (G_OBJECT(gtkhtml_stream));
	g_object_unref (G_OBJECT(txt_stream));
	g_object_unref (G_OBJECT(buf));

	return TRUE;
}


static gboolean
set_empty_message (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	priv           = MODEST_MSG_VIEW_GET_PRIVATE(self);

	gtk_html_load_from_string (GTK_HTML(priv->gtkhtml),
				   "", 1);
	
	return TRUE;
}


gchar *
modest_msg_view_get_selected_text (ModestMsgView *self)
{
	ModestMsgViewPrivate *priv;
	gchar *sel;
	GtkWidget *html;
	int len;
	GtkClipboard *clip;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_MSG_VIEW_GET_PRIVATE(self);
	html = priv->gtkhtml;
	
	/* I'm sure there is a better way to check for selected text */
	sel = gtk_html_get_selection_html(GTK_HTML(html), &len);
	if (!sel)
		return NULL;
	
	g_free(sel);
	
	clip = gtk_widget_get_clipboard(html, GDK_SELECTION_PRIMARY);
	return gtk_clipboard_wait_for_text(clip);
}


void
modest_msg_view_set_message (ModestMsgView *self, TnyMsgIface *msg)
{
	TnyMimePartIface *body;
	ModestMsgViewPrivate *priv;

	g_return_if_fail (self);
	
	priv = MODEST_MSG_VIEW_GET_PRIVATE(self);


	if (msg != priv->msg) {
		if (priv->msg)
			g_object_unref (G_OBJECT(priv->msg));
		if (msg)
			g_object_ref   (G_OBJECT(msg));
		priv->msg = msg;
	}
	
	if (!msg) {
		set_empty_message (self);
		return;
	}
		
	body = modest_tny_msg_actions_find_body_part (msg, TRUE);
	if (body) {
		if (tny_mime_part_iface_content_type_is (body, "text/html"))
			set_html_message (self, body, msg);
		else
			set_text_message (self, body, msg);
		return;
	} else 
		set_empty_message (self);
}

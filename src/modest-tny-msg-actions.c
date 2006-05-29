/* modest-ui.c */

/* insert (c)/licensing information) */

#include <gtk/gtk.h>
#include <gtkhtml/gtkhtml.h>

/* TODO: put in auto* */
#include <tny-text-buffer-stream.h>
#include <tny-msg-mime-part-iface.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include "modest-text-utils.h"


/* private */
static gchar *quote_msg(const TnyMsgIface *src, const gchar *from, time_t sent_date, gint limit, gboolean textorhtml);
static GtkTextBuffer *htmltotext(TnyMsgMimePartIface * body);


static GtkTextBuffer *
htmltotext(TnyMsgMimePartIface * body)
{
	GtkTextBuffer *buf;
#ifdef ACTIVATE_HACKS
	GtkWidget *html, *win;
	TnyStreamIface *stream;	
	GtkClipboard *clip;
	gchar *text;
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	html = gtk_html_new();
	gtk_container_add(GTK_CONTAINER(win), html);
	
	gtk_html_set_editable        (GTK_HTML(html), FALSE);
	gtk_html_allow_selection     (GTK_HTML(html), TRUE);
	gtk_html_set_caret_mode      (GTK_HTML(html), FALSE);
	gtk_html_set_blocking        (GTK_HTML(html), FALSE);
	gtk_html_set_images_blocking (GTK_HTML(html), FALSE);
	
	stream = TNY_STREAM_IFACE(modest_tny_stream_gtkhtml_new(
	        gtk_html_begin(GTK_HTML(html))));
	
	tny_stream_iface_reset (stream);
	tny_msg_mime_part_iface_decode_to_stream (body, stream);
	tny_stream_iface_reset (stream);
	g_object_unref (G_OBJECT(stream));
	
	gtk_widget_show_all(win);
	gtk_html_select_all(GTK_HTML(html));
	clip = gtk_widget_get_clipboard(html, GDK_SELECTION_PRIMARY);
	//clip = gtk_widget_get_clipboard(html, GDK_SELECTION_CLIPBOARD);
	text = gtk_clipboard_wait_for_text(clip);
	
	buf = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(buf, text, -1);
	g_free(text);
	/* destroy win & html */
#else
	buf = gtk_text_buffer_new(NULL);
#endif
	return buf;
}

gchar *
modest_tny_msg_actions_quote(const TnyMsgIface *self, const gchar *from, time_t sent_date, gint limit, GtkTextBuffer *to_quote)
{
	gchar *quoted;

	/* 3 cases: */
	
	/* a) quote text from selection */
	if (to_quote != NULL) {
		return modest_text_utils_quote(to_quote, from, sent_date, limit);
	}
		
	/* b) try to find a text/plain part in the msg and quote it */
	quoted = quote_msg(self, from, sent_date, limit, FALSE);
	if (quoted != NULL)
		return quoted;
	
	/* c) if that fails, try text/html */
	return quote_msg(self, from, sent_date, limit, TRUE);
}
	
	
static gchar *
quote_msg(const TnyMsgIface *src, const gchar *from, time_t sent_date, gint limit, gboolean textorhtml)
{

	GList *parts; /* LEAK? */
	TnyStreamIface* stream;
	TnyTextBufferStream *dest;
	TnyMsgMimePartIface *body = NULL;
	TnyMsgMimePartIface *part;
	GtkTextBuffer *buf;
	gchar *quoted;
	
	/* is the warning in this line due to a bug in tinymail? */
	parts  = (GList*) tny_msg_iface_get_parts (src);

	while (parts) {
		part = TNY_MSG_MIME_PART_IFACE(parts->data);
		if (tny_msg_mime_part_iface_content_type_is (part, textorhtml ? "text/html" : "text/plain")) {
			body = part;
			break;
		}
		parts = parts->next;
	}
	
	if (!body) {
		return NULL;
	}
	
	if (textorhtml == TRUE) {
		buf = htmltotext(body);
	} else {
		buf    = gtk_text_buffer_new (NULL);
		stream = TNY_STREAM_IFACE(tny_text_buffer_stream_new (buf));
		tny_stream_iface_reset (stream);
		tny_msg_mime_part_iface_decode_to_stream (body, stream);
		tny_stream_iface_reset (stream);
	}
	
	quoted = modest_text_utils_quote (buf, from, sent_date, limit);
		
	g_object_unref(stream);
	g_object_unref(buf);
	return quoted;
}

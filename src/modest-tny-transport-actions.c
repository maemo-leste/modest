/* modest-tny-transport-actions.c */

/* insert (c)/licensing information) */

#include <tny-msg.h>
#include <tny-msg-iface.h>			
#include <tny-msg-mime-part.h>
#include <tny-msg-mime-part-iface.h>		
#include <tny-stream-iface.h>
#include <tny-msg-header.h>
#include <tny-msg-header-iface.h>
#include <tny-account-iface.h>	
#include <tny-account-store-iface.h>
#include <tny-transport-account-iface.h>	
#include <tny-transport-account.h>
#include <tny-stream-camel.h>
#include <tny-fs-stream.h>
#include <string.h>
#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>

#include "modest-tny-transport-actions.h"
#include "modest-tny-attachment.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                              modest_tny_transport_actions_class_init   (ModestTnyTransportActionsClass *klass);
static void                              modest_tny_transport_actions_init         (ModestTnyTransportActions *obj);
static void                              modest_tny_transport_actions_finalize     (GObject *obj);
static gboolean                          is_ascii                                  (const gchar *s);
static char *                            get_content_type                          (const gchar *s);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyTransportActionsPrivate ModestTnyTransportActionsPrivate;
struct _ModestTnyTransportActionsPrivate {
	/* my private members go here, eg. */
	/* gboolean frobnicate_mode; */
};
#define MODEST_TNY_TRANSPORT_ACTIONS_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                          MODEST_TYPE_TNY_TRANSPORT_ACTIONS, \
                                                          ModestTnyTransportActionsPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_transport_actions_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyTransportActionsClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_transport_actions_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyTransportActions),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_transport_actions_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestTnyTransportActions",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_tny_transport_actions_class_init (ModestTnyTransportActionsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_transport_actions_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyTransportActionsPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_tny_transport_actions_init (ModestTnyTransportActions *obj)
{
/* uncomment the following if you init any of the private data */
/* 	ModestTnyTransportActionsPrivate *priv = MODEST_TNY_TRANSPORT_ACTIONS_GET_PRIVATE(obj); */

/* 	initialize this object, eg.: */
/* 	priv->frobnicate_mode = FALSE; */
}

static void
modest_tny_transport_actions_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
}

GObject*
modest_tny_transport_actions_new (void)
{
	return G_OBJECT(g_object_new(MODEST_TYPE_TNY_TRANSPORT_ACTIONS, NULL));
}

static gboolean
is_ascii(const gchar *s)
{
	while (s[0]) {
		if (s[0] & 128 || s[0] < 32)
			return FALSE;
		s++;
	}
	return TRUE;
}

static char *
get_content_type(const gchar *s)
{
	GString *type;
	
	type = g_string_new("text/plain");
	if (!is_ascii(s)) {
		if (g_utf8_validate(s, -1, NULL)) {
			g_string_append(type, "; charset=\"utf-8\"");
		} else {
			/* it should be impossible to reach this, but better safe than sorry */
			g_warning("invalid utf8 in message");
			g_string_append(type, "; charset=\"latin1\"");
		}
	}
	return g_string_free(type, FALSE);
}

gboolean
modest_tny_transport_actions_send_message (ModestTnyTransportActions *self,
					   TnyTransportAccountIface *transport_account,
					   const gchar *from,
					   const gchar *to,
					   const gchar *cc,
					   const gchar *bcc,
					   const gchar *subject,
					   const gchar *body,
					   const GList *attachments_list)
{
	TnyMsgIface *new_msg;
	TnyMsgMimePartIface *attachment_part, *text_body_part;
	TnyMsgHeaderIface *headers;
	TnyStreamIface *text_body_stream, *attachment_stream;
	ModestTnyAttachment *attachment;
	GList *pos;
	gchar *content_type;
	const gchar *attachment_content_type;
	gchar *attachment_filename;
	
	new_msg          = TNY_MSG_IFACE(tny_msg_new ());
	headers          = TNY_MSG_HEADER_IFACE(tny_msg_header_new ());
	text_body_stream = TNY_STREAM_IFACE (tny_stream_camel_new
	                                     (camel_stream_mem_new_with_buffer
	                                      (body, strlen(body))));
	
	tny_msg_header_iface_set_from (TNY_MSG_HEADER_IFACE (headers), from);
	tny_msg_header_iface_set_to (TNY_MSG_HEADER_IFACE (headers), to);
	tny_msg_header_iface_set_cc (TNY_MSG_HEADER_IFACE (headers), cc);
	tny_msg_header_iface_set_bcc (TNY_MSG_HEADER_IFACE (headers), bcc);
	tny_msg_header_iface_set_subject (TNY_MSG_HEADER_IFACE (headers), subject);
	tny_msg_iface_set_header (new_msg, headers);

	
	content_type = get_content_type(body);
	
	if (attachments_list == NULL) {
		tny_stream_iface_reset (text_body_stream);
		tny_msg_mime_part_iface_construct_from_stream (TNY_MSG_MIME_PART_IFACE(new_msg),
		                                               text_body_stream, content_type);
		tny_stream_iface_reset (text_body_stream);
	} else {
		text_body_part = TNY_MSG_MIME_PART_IFACE (tny_msg_mime_part_new(
		                                          camel_mime_part_new()));
		tny_stream_iface_reset (text_body_stream);
		tny_msg_mime_part_iface_construct_from_stream (text_body_part,
		                                               text_body_stream,
		                                               content_type);
		tny_stream_iface_reset (text_body_stream);
		tny_msg_iface_add_part(new_msg, text_body_part);
		//g_object_unref (G_OBJECT(text_body_part));
	}
	
	for (    pos = (GList *)attachments_list;
		     pos;
	         pos = pos->next    ) {
		attachment = pos->data;
		attachment_filename = g_path_get_basename(
		                                          modest_tny_attachment_get_filename(attachment));
		attachment_stream = modest_tny_attachment_get_stream(attachment);
		attachment_part = TNY_MSG_MIME_PART_IFACE (tny_msg_mime_part_new (
		                                                camel_mime_part_new()));
		
		attachment_content_type = modest_tny_attachment_get_mime_type(attachment);
		tny_msg_mime_part_iface_construct_from_stream (attachment_part,
		                                               attachment_stream,
		                                               attachment_content_type);
		tny_stream_iface_reset (attachment_stream);
		
		tny_msg_mime_part_iface_set_filename(attachment_part, attachment_filename);
		
		tny_msg_iface_add_part (new_msg, attachment_part);
		g_object_unref(G_OBJECT(attachment_stream));
		//g_object_unref(G_OBJECT(attachment_part));
		g_free(attachment_filename);
		//close(file);
	}
	
	tny_transport_account_iface_send (transport_account, new_msg);

	g_object_unref (G_OBJECT(text_body_stream));
	g_object_unref (G_OBJECT(headers));
	g_object_unref (G_OBJECT(new_msg));
	g_free(content_type);

	return TRUE;	
}

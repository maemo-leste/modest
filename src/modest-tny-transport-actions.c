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
#include <string.h>
#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>



#include "modest-tny-transport-actions.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                              modest_tny_transport_actions_class_init   (ModestTnyTransportActionsClass *klass);
static void                              modest_tny_transport_actions_init         (ModestTnyTransportActions *obj);
static void                              modest_tny_transport_actions_finalize     (GObject *obj);

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



gboolean
modest_tny_transport_actions_send_message (ModestTnyTransportActions *self,
					   TnyTransportAccountIface *transport_account,
					   const gchar *from,
					   const gchar *to,
					   const gchar *cc,
					   const gchar *bcc,
					   const gchar *subject,
					   const gchar *body)
{
	TnyMsgIface *new_msg;
	TnyMsgMimePartIface *body_part;
	TnyMsgHeaderIface *headers;
	TnyStreamIface *body_stream;

	new_msg     = TNY_MSG_IFACE(tny_msg_new ());
	headers     = TNY_MSG_HEADER_IFACE(tny_msg_header_new ());
	body_stream = TNY_STREAM_IFACE (tny_stream_camel_new
					(camel_stream_mem_new_with_buffer
					 (body, strlen(body))));
	body_part = TNY_MSG_MIME_PART_IFACE (tny_msg_mime_part_new
					     (camel_mime_part_new()));

	tny_msg_header_iface_set_from (TNY_MSG_HEADER_IFACE (headers), from);
	tny_msg_header_iface_set_to (TNY_MSG_HEADER_IFACE (headers), to);
	tny_msg_header_iface_set_cc (TNY_MSG_HEADER_IFACE (headers), cc);
	tny_msg_header_iface_set_bcc (TNY_MSG_HEADER_IFACE (headers), bcc);
	tny_msg_header_iface_set_subject (TNY_MSG_HEADER_IFACE (headers), subject);

	tny_msg_iface_set_header (new_msg, headers);
	tny_msg_mime_part_iface_construct_from_stream (body_part, body_stream,
						       "text/plain");
	tny_msg_mime_part_iface_set_content_type  (body_part,"text/plain");	
	
	tny_msg_mime_part_iface_set_content_type (
		TNY_MSG_MIME_PART_IFACE(new_msg), "text/plain");
	tny_stream_iface_reset (body_stream);
	
	tny_msg_mime_part_iface_construct_from_stream (TNY_MSG_MIME_PART_IFACE(new_msg),
						       body_stream, "text/plain");
	
	tny_transport_account_iface_send (transport_account, new_msg);

	g_object_unref (G_OBJECT(body_stream));
	g_object_unref (G_OBJECT(body_part));
	g_object_unref (G_OBJECT(headers));
	g_object_unref (G_OBJECT(new_msg));

	return TRUE;	
}




/* modest-tny-msg-view.c */

/* insert (c)/licensing information) */

#include "modest-tny-msg-view.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                     modest_tny_msg_view_class_init   (ModestTnyMsgViewClass *klass);
static void                     modest_tny_msg_view_init         (ModestTnyMsgView *obj);
static void                     modest_tny_msg_view_finalize     (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyMsgViewPrivate ModestTnyMsgViewPrivate;
struct _ModestTnyMsgViewPrivate {
	GtkWidget *text_view;
};
#define MODEST_TNY_MSG_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_TNY_MSG_VIEW, \
                                                 ModestTnyMsgViewPrivate))
/* globals */
static GtkContainerClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_msg_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyMsgViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_msg_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyMsgView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_msg_view_init,
		};
		my_type = g_type_register_static (GTK_TYPE_SCROLLED_WINDOW,
		                                  "ModestTnyMsgView",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_tny_msg_view_class_init (ModestTnyMsgViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_msg_view_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyMsgViewPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_tny_msg_view_init (ModestTnyMsgView *obj)
{
 	ModestTnyMsgViewPrivate *priv;

	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(obj);

	priv->text_view = NULL;
}

static void
modest_tny_msg_view_finalize (GObject *obj)
{
	/* no need to unref the text_view */	
}

GtkWidget*
modest_tny_msg_view_new (TnyMsgIface *msg)
{
	GObject *obj;
	ModestTnyMsgView* self;
	ModestTnyMsgViewPrivate *priv;

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_MSG_VIEW, NULL));
	self = MODEST_TNY_MSG_VIEW(obj);
	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(self);

	gtk_scrolled_window_set_policy(self, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	
	priv->text_view = gtk_text_view_new ();
	gtk_text_view_set_editable       (GTK_TEXT_VIEW(priv->text_view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW(priv->text_view), FALSE);

	gtk_container_add (GTK_CONTAINER(self), priv->text_view);
	
	if (msg)
		modest_tny_msg_view_set_message (self, msg);

	return GTK_WIDGET(self);
}



void
modest_tny_msg_view_set_message (ModestTnyMsgView *self, TnyMsgIface *msg)
{
	ModestTnyMsgViewPrivate *priv;
	GtkTextBuffer *buf;
	GList *parts;
	TnyStreamIface *stream;
	
	g_return_if_fail (self);

	priv = MODEST_TNY_MSG_VIEW_GET_PRIVATE(self);
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(priv->text_view));

	/* clear the message view */
	gtk_text_buffer_set_text (buf, "", 0);

	/* if msg is NULL, do nothing else */
	if (!msg) {
		return;
	}
	
	/* otherwise... find the body part */
	stream = TNY_STREAM_IFACE(tny_text_buffer_stream_new(buf));
	parts  = (GList*) tny_msg_iface_get_parts (msg);
	while (parts) {
		TnyMsgMimePartIface *part =
			TNY_MSG_MIME_PART_IFACE(parts->data);
		
		if (tny_msg_mime_part_iface_content_type_is (part, "text/plain")) {
			tny_stream_iface_reset (stream);
			tny_msg_mime_part_iface_write_to_stream (part, stream);
			tny_stream_iface_reset (stream);
			break;
		}

		parts = parts->next;
	}
}

/* modest-tny-attachment.c */

/* insert (c)/licensing information) */

#include "modest-tny-attachment.h"
#include "modest-tny-msg-actions.h"

#include <tny-stream-camel.h>
#include <tny-fs-stream.h>
#include <camel/camel.h>


/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                       modest_tny_attachment_class_init    (ModestTnyAttachmentClass *klass);
static void                       modest_tny_attachment_init          (ModestTnyAttachment *obj);
static void                       modest_tny_attachment_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyAttachmentPrivate ModestTnyAttachmentPrivate;
struct _ModestTnyAttachmentPrivate {
	gchar *name;
	gchar *filename;
	gchar *mime_type;
	gchar *disposition;
	gchar *content_id;
	TnyStreamIface *stream;
};
#define MODEST_TNY_ATTACHMENT_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_TNY_ATTACHMENT, \
                                                   ModestTnyAttachmentPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_attachment_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyAttachmentClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_attachment_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyAttachment),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_attachment_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestTnyAttachment",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_tny_attachment_class_init (ModestTnyAttachmentClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_attachment_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyAttachmentPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_tny_attachment_init (ModestTnyAttachment *obj)
{
	ModestTnyAttachmentPrivate *priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(obj);

	priv->name = NULL;
	priv->filename = NULL;
	priv->mime_type = NULL;
	priv->disposition = NULL;
	priv->content_id = NULL;
	priv->stream = NULL;
}

static void
modest_tny_attachment_finalize (GObject *obj)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(MODEST_TNY_ATTACHMENT(obj));
	g_free(priv->name);
	g_free(priv->mime_type);
	g_free(priv->disposition);
	g_free(priv->content_id);
	if (priv->stream)
		g_object_unref(G_OBJECT(priv->stream));
}

ModestTnyAttachment *
modest_tny_attachment_new (void)
{
	return MODEST_TNY_ATTACHMENT(G_OBJECT(g_object_new(MODEST_TYPE_TNY_ATTACHMENT, NULL)));
}


void
modest_tny_attachment_set_name (ModestTnyAttachment *self, const gchar * thing)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	g_free(priv->name);
	priv->name = g_strdup(thing);
}

const gchar *
modest_tny_attachment_get_name (ModestTnyAttachment *self)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	if (!priv->name)
		if (priv->filename)
			priv->name = g_path_get_basename(priv->filename);
	return priv->name;
}


void
modest_tny_attachment_set_filename (ModestTnyAttachment *self, const gchar * thing)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	g_free(priv->filename);
	priv->filename = g_strdup(thing);
}

const gchar *
modest_tny_attachment_get_filename (ModestTnyAttachment *self)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	return priv->filename;
}


void
modest_tny_attachment_set_mime_type (ModestTnyAttachment *self, const gchar * thing)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	g_free(priv->mime_type);
	priv->mime_type = g_strdup(thing);
}

const gchar *
modest_tny_attachment_get_mime_type (ModestTnyAttachment *self)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	return priv->mime_type;
}



void
modest_tny_attachment_guess_mime_type (ModestTnyAttachment *self)
{
	ModestTnyAttachmentPrivate *priv;
	gchar *suffixes[] = {".jpg", ".gif", ".mp3", NULL};
	gchar *types[]    = {"image/jpeg", "image/gif", "audio/mpeg", NULL};
	gint pos;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	if (!priv->filename)
		return;
	
	for (pos = 0 ; suffixes[pos] ; pos++) {
		if (g_str_has_suffix(priv->filename, suffixes[pos]))
			break;
	}
	
	g_free(priv->mime_type);
	if (suffixes[pos])
		priv->mime_type = g_strdup(types[pos]);
	else
		priv->mime_type = NULL;
}

static TnyStreamIface *
make_stream_from_file(const gchar * filename)
{
	gint file;
	
	file = open(filename, O_RDONLY);
	if (file < 0)
		return NULL;

	return TNY_STREAM_IFACE(tny_stream_camel_new(camel_stream_fs_new_with_fd(file)));
}

void
modest_tny_attachment_set_stream(ModestTnyAttachment *self, TnyStreamIface *thing)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	if (priv->stream)
		g_object_unref(G_OBJECT(priv->stream));
	priv->stream = thing;
}

TnyStreamIface *
modest_tny_attachment_get_stream (ModestTnyAttachment *self)
{
	ModestTnyAttachmentPrivate *priv;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	if (!priv->stream)
		if (priv->filename)
			priv->stream = make_stream_from_file(priv->filename);
	return priv->stream;
}


void
modest_tny_attachment_free_list(GList *list)
{
	/* this is pretty generic for a GList of GObjects */
	GList *pos;
	
	for (pos = list; pos; pos = pos->next)
		if (pos->data)
			g_object_unref(pos->data);
	g_list_free(list);
	return;
}


ModestTnyAttachment *
modest_tny_attachment_new_from_mime_part(TnyMsgMimePartIface *part)
{
	TnyStreamIface *mem_stream;
	ModestTnyAttachment *self;
	
	mem_stream = TNY_STREAM_IFACE(tny_stream_camel_new(camel_stream_mem_new()));
	self = modest_tny_attachment_new();
	tny_msg_mime_part_iface_decode_to_stream(part, mem_stream);
	tny_stream_iface_reset(mem_stream);
	modest_tny_attachment_set_stream(self, mem_stream);
	modest_tny_attachment_set_mime_type(self,
	                                    tny_msg_mime_part_iface_get_content_type(part));
	modest_tny_attachment_set_name(self,
	                                    tny_msg_mime_part_iface_get_filename(part));
	return self;
}

ModestTnyAttachment *
modest_tny_attachment_new_from_message(const TnyMsgIface *msg)
{
	TnyStreamIface *mem_stream, *msg_stream;
	ModestTnyAttachment *self;
	gint res;
	
	mem_stream = TNY_STREAM_IFACE(tny_stream_camel_new(camel_stream_mem_new()));
	msg_stream = tny_msg_mime_part_iface_get_stream(TNY_MSG_MIME_PART_IFACE(msg));
	printf("ping\n");
	tny_stream_iface_reset(msg_stream);
	res = tny_stream_iface_write_to_stream(msg_stream, mem_stream);
	//tny_msg_mime_part_iface_write_to_stream(TNY_MSG_MIME_PART_IFACE(msg), mem_stream);
	printf("pong, %d\n", res);
	tny_stream_iface_reset(msg_stream);
	tny_stream_iface_reset(mem_stream);
	self = modest_tny_attachment_new();
	modest_tny_attachment_set_stream(self, mem_stream);
	modest_tny_attachment_set_mime_type(self, "message/rfc822");
	modest_tny_attachment_set_name(self, "original message");
	return self;
}

GList *
modest_tny_attachment_new_list_from_msg(const TnyMsgIface *msg, gboolean with_body)
{
	GList *list = NULL;
	const GList *attachments = NULL;
	TnyMsgMimePartIface *part;
	ModestTnyAttachment *att;
	
#if 0	
	if (with_body) {
		/* TODO: make plain over html configurable */
		part = modest_tny_msg_actions_find_body_part ((TnyMsgIface *)msg, "text/plain");
		if (!part) 
			part = modest_tny_msg_actions_find_body_part ((TnyMsgIface *)msg, "text/html");
		if (part) {
			att = modest_tny_attachment_new_from_mime_part(part);
			/* TODO: i18n */
			modest_tny_attachment_set_name(att, "original message");
			list = g_list_append(list, att);
		}
	}
#endif
	if (with_body) {
		list = g_list_append(list, modest_tny_attachment_new_from_message(msg));
	} else {
		attachments = tny_msg_iface_get_parts((TnyMsgIface *)msg);
	}
	while (attachments) {
		part = attachments->data;
		if (tny_msg_mime_part_iface_is_attachment(part)) {
			att = modest_tny_attachment_new_from_mime_part(part);
			list = g_list_append(list, att);
		}
		attachments = attachments->next;
	}
	return list;
}

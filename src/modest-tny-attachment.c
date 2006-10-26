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


/* modest-tny-attachment.c */

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
			NULL
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
	gchar *suffixes[] = {".jpg", ".gif", ".png", ".mp3", ".ogg", /* default: */ "", NULL};
	gchar *types[]    = {"image/jpeg", "image/gif", "image/png", "audio/mpeg", "application/ogg", "application/octet-stream", NULL};
	gchar *low_fn;
	gint pos;
	
	priv = MODEST_TNY_ATTACHMENT_GET_PRIVATE(self);
	if (!priv->filename)
		return;
	
	low_fn = g_utf8_strdown(priv->filename, -1);
	for (pos = 0 ; suffixes[pos] ; pos++) {
		if (g_str_has_suffix(low_fn, suffixes[pos]))
			break;
	}
	
	g_free(low_fn);
	g_free(priv->mime_type);
	if (suffixes[pos])
		priv->mime_type = g_strdup(types[pos]);
	else
		priv->mime_type = NULL;
}

static TnyStreamIface *
make_stream_from_file(const gchar * filename)
{
	return NULL;
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
modest_tny_attachment_new_from_mime_part(TnyMimePartIface *part)
{
	return NULL;
}

ModestTnyAttachment *
modest_tny_attachment_new_from_message(const TnyMsgIface *msg)
{
	return NULL;
}

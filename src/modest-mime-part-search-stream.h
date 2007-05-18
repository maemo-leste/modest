#ifndef MODEST_MIME_PART_SEARCH_STREAM_H
#define MODEST_MIME_PART_SEARCH_STREAM_H

#include <glib.h>
#include <glib-object.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <tny-stream.h>

G_BEGIN_DECLS

#define MODEST_TYPE_MIME_PART_SEARCH_STREAM             (modest_mime_part_search_stream_get_type ())
#define MODEST_MIME_PART_SEARCH_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_MIME_PART_SEARCH_STREAM, ModestMimePartSearchStream))
#define MODEST_MIME_PART_SEARCH_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MODEST_TYPE_MIME_PART_SEARCH_STREAM, ModestMimePartSearchStreamClass))
#define MODEST_IS_MIME_PART_SEARCH_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_MIME_PART_SEARCH_STREAM))
#define MODEST_IS_MIME_PART_SEARCH_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MODEST_TYPE_MIME_PART_SEARCH_STREAM))
#define MODEST_MIME_PART_SEARCH_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MODEST_TYPE_MIME_PART_SEARCH_STREAM, ModestMimePartSearchStreamClass))

typedef struct _ModestMimePartSearchStream ModestMimePartSearchStream;
typedef struct _ModestMimePartSearchStreamClass ModestMimePartSearchStreamClass;

struct _ModestMimePartSearchStream
{
	GObject parent;
	gboolean found;
	gchar *search_for;
};

struct _ModestMimePartSearchStreamClass 
{
	GObjectClass parent;
};

GType  modest_mime_part_search_stream_get_type (void);
TnyStream* modest_mime_part_search_stream_new (const char *search_for);

G_END_DECLS

#endif


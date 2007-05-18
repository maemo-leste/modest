#define _GNU_SOURCE

#include <glib.h>
#include <string.h>

#include "modest-mime-part-search-stream.h"

static GObjectClass *parent_class = NULL;

static gssize
modest_mime_part_search_stream_read (TnyStream *self, char *buffer, gsize n)
{
	ModestMimePartSearchStream *me = (ModestMimePartSearchStream *) self;

	if (strcasestr (buffer, me->search_for) != NULL) {
		me->found = TRUE;
		return -1;
	}

	return (gssize) n;
}

static gssize
modest_mime_part_search_stream_write (TnyStream *self, const char *buffer, gsize n)
{
	ModestMimePartSearchStream *me = (ModestMimePartSearchStream *) self;

	if (strcasestr (buffer, me->search_for) != NULL) {
		me->found = TRUE;
		return -1;
	}

	return (gssize) n;
}

static gint
modest_mime_part_search_stream_flush (TnyStream *self)
{
	return 0;
}

static gint
modest_mime_part_search_stream_close (TnyStream *self)
{
	return 0;
}

static gboolean
modest_mime_part_search_stream_is_eos (TnyStream *self)
{
	return TRUE;
}

static gint
modest_mime_part_search_stream_reset (TnyStream *self)
{
	return 0;
}

static gssize
modest_mime_part_search_stream_write_to_stream (TnyStream *self, TnyStream *output)
{
	char tmp_buf[4096];
	ssize_t total = 0;
	ssize_t nb_read;
	ssize_t nb_written;

	g_assert (TNY_IS_STREAM (output));

	while (G_LIKELY (!tny_stream_is_eos (self))) 
	{
		nb_read = tny_stream_read (self, tmp_buf, sizeof (tmp_buf));
		if (G_UNLIKELY (nb_read < 0))
			return -1;
		else if (G_LIKELY (nb_read > 0)) {
			nb_written = 0;
	
			while (G_LIKELY (nb_written < nb_read))
			{
				ssize_t len = tny_stream_write (output, tmp_buf + nb_written,
								  nb_read - nb_written);
				if (G_UNLIKELY (len < 0))
					return -1;
				nb_written += len;
			}
			total += nb_written;
		}
	}
	return total;
}


TnyStream* 
modest_mime_part_search_stream_new (const char *search_for)
{
	ModestMimePartSearchStream *me = g_object_new (MODEST_TYPE_MIME_PART_SEARCH_STREAM, NULL);

	me->search_for = g_strdup (search_for);

	return TNY_STREAM (me);
}

static void
modest_mime_part_search_stream_finalize (GObject *object)
{
	ModestMimePartSearchStream *me = g_object_new (MODEST_TYPE_MIME_PART_SEARCH_STREAM, NULL);

	g_free (me->search_for);

	parent_class->finalize (object);
}

static void
modest_mime_part_search_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestMimePartSearchStream *me = (ModestMimePartSearchStream *) instance;

	me->found = FALSE;
}

static void
tny_stream_init (TnyStreamIface *klass)
{
	klass->read_func = modest_mime_part_search_stream_read;
	klass->write_func = modest_mime_part_search_stream_write;
	klass->flush_func = modest_mime_part_search_stream_flush;
	klass->close_func = modest_mime_part_search_stream_close;
	klass->is_eos_func = modest_mime_part_search_stream_is_eos;
	klass->reset_func = modest_mime_part_search_stream_reset;
	klass->write_to_stream_func = modest_mime_part_search_stream_write_to_stream;
}

static void
modest_mime_part_search_stream_class_init (ModestMimePartSearchStreamClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = modest_mime_part_search_stream_finalize;
}

GType
modest_mime_part_search_stream_get_type (void)
{
	static GType type = 0;
	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
			sizeof (ModestMimePartSearchStreamClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) modest_mime_part_search_stream_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (ModestMimePartSearchStream),
			0,      /* n_preallocs */
			modest_mime_part_search_stream_instance_init,    /* instance_init */
			NULL
		};


		static const GInterfaceInfo tny_stream_info = 
		{
			(GInterfaceInitFunc) tny_stream_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
			"ModestMimePartSearchStream",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_STREAM,
			&tny_stream_info);

	}
	return type;
}

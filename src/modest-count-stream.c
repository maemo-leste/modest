/* Copyright (c) 2007, Nokia Corporation
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

/* modest-count-stream.h */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-stream.h>
#include "modest-count-stream.h"

typedef struct _ModestCountStreamPrivate ModestCountStreamPrivate;
struct _ModestCountStreamPrivate {
	gsize count;
};
#define MODEST_COUNT_STREAM_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                       MODEST_TYPE_COUNT_STREAM, \
                                                       ModestCountStreamPrivate))

static GObjectClass *parent_class = NULL;

static gssize
modest_count_stream_read (TnyStream *self, char *buffer, gsize n)
{
        return 0;
}

static gssize
modest_count_stream_write (TnyStream *self, const char *buffer, gsize n)
{
	ModestCountStreamPrivate *priv;
	priv = MODEST_COUNT_STREAM_GET_PRIVATE(self);

	priv->count += n;
	return (gssize)n;
}

static gint
modest_count_stream_flush (TnyStream *self)
{
        return 0;
}

static gint
modest_count_stream_close (TnyStream *self)
{
        return 0;
}

static gboolean
modest_count_stream_is_eos (TnyStream *self)
{
        return TRUE;
}

static gint
modest_count_stream_reset (TnyStream *self)
{
        return 0;
}

static gssize
modest_count_stream_write_to_stream (TnyStream *self, TnyStream *output)
{
        return 0;
}

/**
 * modest_count_stream_get_count
 * @self: the ModestCountStream
 * 
 * returns the number of bytes that have been written through this stream
 * 
 * Returns: number of bytes that have passed through this stream
 */
gsize
modest_count_stream_get_count (ModestCountStream *self)
{
	ModestCountStreamPrivate *priv;
	priv = MODEST_COUNT_STREAM_GET_PRIVATE(self);

	return priv->count;
}

/**
 * modest_count_stream_reset_count
 * @self: the ModestCountStream
 * 
 * resets the internal counter
 * 
 * Returns:
 */
void
modest_count_stream_reset_count (ModestCountStream *self)
{
	ModestCountStreamPrivate *priv;
	priv = MODEST_COUNT_STREAM_GET_PRIVATE(self);

	priv->count = 0;
}

TnyStream*
modest_count_stream_new ()
{
	return TNY_STREAM (g_object_new (MODEST_TYPE_COUNT_STREAM, NULL)); 
}

static void
modest_count_stream_finalize (GObject *object)
{
        parent_class->finalize (object);
}
static void
modest_count_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestCountStreamPrivate *priv;
	priv = MODEST_COUNT_STREAM_GET_PRIVATE(instance);

	priv->count = 0;
}

static void
tny_stream_init (gpointer g_iface, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *) g_iface;

        klass->read = modest_count_stream_read;
        klass->write = modest_count_stream_write;
        klass->flush = modest_count_stream_flush;
        klass->close = modest_count_stream_close;
        klass->is_eos = modest_count_stream_is_eos;
        klass->reset = modest_count_stream_reset;
        klass->write_to_stream = modest_count_stream_write_to_stream;
}

static void
modest_count_stream_class_init (ModestCountStreamClass *klass)
{
        GObjectClass *object_class;

        parent_class = g_type_class_peek_parent (klass);
        object_class = (GObjectClass*) klass;
        object_class->finalize = modest_count_stream_finalize;
        
    	g_type_class_add_private (object_class, sizeof(ModestCountStream));
}
GType
modest_count_stream_get_type (void)
{
        static GType type = 0;
        if (G_UNLIKELY(type == 0))
        {
                static const GTypeInfo info =
                {
                        sizeof (ModestCountStreamClass),
                        NULL,   /* base_init */
                        NULL,   /* base_finalize */
                        (GClassInitFunc) modest_count_stream_class_init,   /* class_init */
                        NULL,   /* class_finalize */
                        NULL,   /* class_data */
                        sizeof (ModestCountStream),
                        0,      /* n_preallocs */
                        modest_count_stream_instance_init,    /* instance_init */
                        NULL
                };


                static const GInterfaceInfo tny_stream_info =
                {
                        (GInterfaceInitFunc) tny_stream_init, /* interface_init */
                        NULL,         /* interface_finalize */
                        NULL          /* interface_data */
                };

                type = g_type_register_static (G_TYPE_OBJECT,
					       "ModestCountStream",
					       &info, 0);

                g_type_add_interface_static (type, TNY_TYPE_STREAM,
                        &tny_stream_info);

        }
        return type;
}

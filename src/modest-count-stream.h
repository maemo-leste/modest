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


/* modest-count-streaml.h */

#ifndef __MODEST_COUNT_STREAM_H__
#define __MODEST_COUNT_STREAM_H__

#include <glib-object.h>
#include <tny-stream.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_COUNT_STREAM             (modest_count_stream_get_type())
#define MODEST_COUNT_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_COUNT_STREAM,ModestCountStream))
#define MODEST_COUNT_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_COUNT_STREAM,ModestCountStreamClass))
#define MODEST_IS_COUNT_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_COUNT_STREAM))
#define MODEST_IS_COUNT_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_COUNT_STREAM))
#define MODEST_COUNT_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_COUNT_STREAM,ModestCountStreamClass))

typedef struct _ModestCountStream      ModestCountStream;
typedef struct _ModestCountStreamClass ModestCountStreamClass;

struct _ModestCountStream {
	GObject parent;
};

struct _ModestCountStreamClass {
	GObjectClass parent_class;
};

GType       modest_count_stream_get_type    (void) G_GNUC_CONST;


/**
 * modest_count_stream_new:
 * 
 * creates a new #ModestStream
 * 
 * Returns: a new #ModestStream
 **/
TnyStream*    modest_count_stream_new         ();

/**
 * modest_count_stream_get_count
 * @self: the ModestCountStream
 * 
 * returns the number of bytes that have been written through this stream
 * 
 * Returns: number of bytes that have passed through this stream
 */
gsize		modest_count_stream_get_count (ModestCountStream *self);

/**
 * modest_count_stream_reset_count
 * @self: the ModestCountStream
 * 
 * resets the internal counter
 * 
 * Returns:
 */
void		modest_count_stream_reset_count (ModestCountStream *self);


G_END_DECLS

#endif /* __MODEST_COUNT_STREAM_H__ */

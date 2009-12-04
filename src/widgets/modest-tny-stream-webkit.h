/* Copyright (c) 2009, Nokia Corporation
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


/* modest-tny-stream-webkit.h */

#ifndef __MODEST_TNY_STREAM_WEBKIT_H__
#define __MODEST_TNY_STREAM_WEBKIT_H__

#include <glib-object.h>
#include <webkit/webkit.h>
#include <tny-stream.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_STREAM_WEBKIT             (modest_tny_stream_webkit_get_type())
#define MODEST_TNY_STREAM_WEBKIT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_STREAM_WEBKIT,ModestTnyStreamWebkit))
#define MODEST_TNY_STREAM_WEBKIT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_STREAM_WEBKIT,ModestTnyStreamWebkitClass))
#define MODEST_IS_TNY_STREAM_WEBKIT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_STREAM_WEBKIT))
#define MODEST_IS_TNY_STREAM_WEBKIT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_STREAM_WEBKIT))
#define MODEST_TNY_STREAM_WEBKIT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_STREAM_WEBKIT,ModestTnyStreamWebkitClass))

typedef struct _ModestTnyStreamWebkit      ModestTnyStreamWebkit;
typedef struct _ModestTnyStreamWebkitClass ModestTnyStreamWebkitClass;

struct _ModestTnyStreamWebkit {
	GObject parent;
};

struct _ModestTnyStreamWebkitClass {
	GObjectClass parent_class;
};

GType       modest_tny_stream_webkit_get_type    (void) G_GNUC_CONST;


/**
 * modest_tny_stream_webkit_new:
 * @stream: a #WebkitStream
 * 
 * creates a new #ModestTnyStreamWebkit
 * 
 * Returns: a new #ModestTnyStreamWebkit
 **/
GObject*    modest_tny_stream_webkit_new         (WebKitWebView *webview,
						  const gchar *mime_type, const gchar *encoding);

/**
 * modest_tny_stream_webkit_set_max_size:
 * @stream: a #ModestTnyStreamWebkit
 * @max_size: a #gssize
 *
 * set @max_size as the maximum size @stream will process
 */
void modest_tny_stream_webkit_set_max_size (ModestTnyStreamWebkit *stream, gssize max_size);

/**
 * modest_tny_stream_webkit_get_max_size:
 * @stream: a #ModestTnyStreamWebkit
 *
 * obtain the maximum size @stream will process
 *
 * Returns: a #gssize (or 0 if unlimited)
 */
gssize modest_tny_stream_webkit_get_max_size (ModestTnyStreamWebkit *stream);

/**
 * modest_tny_stream_webkit_limit_reached:
 * @self: a #ModestTnyStreamWebkit
 *
 * tells if on processing the stream, the max size limit has been hit.
 *
 * Returns: %TRUE if limit is reached, %FALSE otherwise.
 */
gboolean    modest_tny_stream_webkit_limit_reached (ModestTnyStreamWebkit *self);

G_END_DECLS

#endif /* __MODEST_TNY_STREAM_WEBKIT_H__ */


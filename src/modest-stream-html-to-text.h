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


/* modest-stream-text-to-html.h */

#ifndef __MODEST_STREAM_HTML_TO_TEXT_H__
#define __MODEST_STREAM_HTML_TO_TEXT_H__

#include <glib-object.h>
#include <gtkhtml/gtkhtml.h>
#include <tny-stream.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_STREAM_HTML_TO_TEXT             (modest_stream_html_to_text_get_type())
#define MODEST_STREAM_HTML_TO_TEXT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_STREAM_HTML_TO_TEXT,ModestStreamHtmlToText))
#define MODEST_STREAM_HTML_TO_TEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_STREAM_HTML_TO_TEXT,ModestStreamHtmlToTextClass))
#define MODEST_IS_STREAM_HTML_TO_TEXT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_STREAM_HTML_TO_TEXT))
#define MODEST_IS_STREAM_HTML_TO_TEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_STREAM_HTML_TO_TEXT))
#define MODEST_STREAM_HTML_TO_TEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_STREAM_HTML_TO_TEXT,ModestStreamHtmlToTextClass))

typedef struct _ModestStreamHtmlToText      ModestStreamHtmlToText;
typedef struct _ModestStreamHtmlToTextClass ModestStreamHtmlToTextClass;

struct _ModestStreamHtmlToText {
	GObject parent;
};

struct _ModestStreamHtmlToTextClass {
	GObjectClass parent_class;
};

GType       modest_stream_html_to_text_get_type    (void) G_GNUC_CONST;


/**
 * modest_stream_html_to_text_new:
 * @stream: a #GtkHTMLStream
 *
 * creates a new #ModestStreamHtmlToText
 *
 * Returns: a new #ModestStreamHtmlToText
 **/
TnyStream*    modest_stream_html_to_text_new         (TnyStream *out_stream);


G_END_DECLS

#endif /* __MODEST_STREAM_HTML_TO_TEXT_H__ */


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


/* modest-tny-stream-gtkhtml.h */

#ifndef __MODEST_TNY_STREAM_GTKHTML_H__
#define __MODEST_TNY_STREAM_GTKHTML_H__

#include <glib-object.h>
#include <gtkhtml/gtkhtml.h>
#include <tny-stream-iface.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_STREAM_GTKHTML             (modest_tny_stream_gtkhtml_get_type())
#define MODEST_TNY_STREAM_GTKHTML(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_STREAM_GTKHTML,ModestTnyStreamGtkhtml))
#define MODEST_TNY_STREAM_GTKHTML_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_STREAM_GTKHTML,ModestTnyStreamGtkhtmlClass))
#define MODEST_IS_TNY_STREAM_GTKHTML(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_STREAM_GTKHTML))
#define MODEST_IS_TNY_STREAM_GTKHTML_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_STREAM_GTKHTML))
#define MODEST_TNY_STREAM_GTKHTML_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_STREAM_GTKHTML,ModestTnyStreamGtkhtmlClass))

typedef struct _ModestTnyStreamGtkhtml      ModestTnyStreamGtkhtml;
typedef struct _ModestTnyStreamGtkhtmlClass ModestTnyStreamGtkhtmlClass;

struct _ModestTnyStreamGtkhtml {
	GObject parent;
};

struct _ModestTnyStreamGtkhtmlClass {
	GObjectClass parent_class;
};

GType       modest_tny_stream_gtkhtml_get_type    (void) G_GNUC_CONST;

GObject*    modest_tny_stream_gtkhtml_new         (GtkHTMLStream* stream);


G_END_DECLS

#endif /* __MODEST_TNY_STREAM_GTKHTML_H__ */


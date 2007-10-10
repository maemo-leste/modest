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

#ifndef MODEST_MIME_PART_VIEW_H
#define MODEST_MIME_PART_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <tny-stream.h>

G_BEGIN_DECLS

#define MODEST_TYPE_MIME_PART_VIEW            (modest_mime_part_view_get_type ())
#define MODEST_MIME_PART_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_MIME_PART_VIEW, ModestMimePartView))
#define MODEST_IS_MIME_PART_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_MIME_PART_VIEW))
#define MODEST_MIME_PART_VIEW_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MODEST_TYPE_MIME_PART_VIEW, ModestMimePartViewIface))

typedef struct _ModestMimePartView ModestMimePartView;
typedef struct _ModestMimePartViewIface ModestMimePartViewIface;

struct _ModestMimePartViewIface
{
	GTypeInterface parent;

	/* signals */
	gboolean (*activate_link) (ModestMimePartView *self, const gchar *uri);
	gboolean (*link_hover)    (ModestMimePartView *self, const gchar *uri);
	gboolean (*fetch_url)     (ModestMimePartView *self, const gchar *uri, TnyStream *stream);
	
	/* virtuals */
	gboolean (*is_empty_func) (ModestMimePartView *self);
};

GType modest_mime_part_view_get_type (void);

gboolean modest_mime_part_view_is_empty (ModestMimePartView *self);

G_END_DECLS

#endif

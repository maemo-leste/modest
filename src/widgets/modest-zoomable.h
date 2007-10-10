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

#ifndef MODEST_ZOOMABLE_H
#define MODEST_ZOOMABLE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MODEST_TYPE_ZOOMABLE            (modest_zoomable_get_type ())
#define MODEST_ZOOMABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_ZOOMABLE, ModestZoomable))
#define MODEST_IS_ZOOMABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_ZOOMABLE))
#define MODEST_ZOOMABLE_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MODEST_TYPE_ZOOMABLE, ModestZoomableIface))

typedef struct _ModestZoomable ModestZoomable;
typedef struct _ModestZoomableIface ModestZoomableIface;

struct _ModestZoomableIface
{
	GTypeInterface parent;

	void (*set_zoom_func) (ModestZoomable *self, gdouble zoom);
	gdouble (*get_zoom_func) (ModestZoomable *self);
	gboolean (*zoom_plus_func) (ModestZoomable *self);
	gboolean (*zoom_minus_func) (ModestZoomable *self);
};

GType modest_zoomable_get_type (void);

void            modest_zoomable_set_zoom    (ModestZoomable *zoomable,
					     gdouble value);
gdouble         modest_zoomable_get_zoom    (ModestZoomable *zoomable);
gboolean        modest_zoomable_zoom_plus   (ModestZoomable *zoomable);
gboolean        modest_zoomable_zoom_minus  (ModestZoomable *zoomable);


G_END_DECLS

#endif

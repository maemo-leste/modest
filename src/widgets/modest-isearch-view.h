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

#ifndef MODEST_ISEARCH_VIEW_H
#define MODEST_ISEARCH_VIEW_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MODEST_TYPE_ISEARCH_VIEW            (modest_isearch_view_get_type ())
#define MODEST_ISEARCH_VIEW(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_ISEARCH_VIEW, ModestISearchView))
#define MODEST_IS_ISEARCH_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_ISEARCH_VIEW))
#define MODEST_ISEARCH_VIEW_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MODEST_TYPE_ISEARCH_VIEW, ModestISearchViewIface))

typedef struct _ModestISearchView ModestISearchView;
typedef struct _ModestISearchViewIface ModestISearchViewIface;

struct _ModestISearchViewIface
{
	GTypeInterface parent;

	/* virtuals */
	gboolean (*search_func)             (ModestISearchView *self, const gchar *string);
	gboolean (*search_next_func)        (ModestISearchView *self);
	gboolean (*get_selection_area_func) (ModestISearchView *self, gint *x, gint *y, gint *width, gint *height);
};

GType modest_isearch_view_get_type (void);

gboolean modest_isearch_view_search              (ModestISearchView *self, const gchar *string);
gboolean modest_isearch_view_search_next         (ModestISearchView *self);
gboolean modest_isearch_view_get_selection_area  (ModestISearchView *self, gint *x, gint *y, gint *width, gint *height);

G_END_DECLS

#endif

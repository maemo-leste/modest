/* Copyright (c) 2006, 2007, Nokia Corporation
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

#ifndef __MODEST_MOZEMBED_MIME_PART_VIEW_H__
#define __MODEST_MOZEMBED_MIME_PART_VIEW_H__

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <tny-mime-part-view.h>
#include <widgets/modest-mime-part-view.h>
#include <widgets/modest-zoomable.h>
#include <widgets/modest-isearch-view.h>
#include <tny-moz-embed-html-mime-part-view.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MOZEMBED_MIME_PART_VIEW             (modest_mozembed_mime_part_view_get_type())
#define MODEST_MOZEMBED_MIME_PART_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MOZEMBED_MIME_PART_VIEW,ModestMozembedMimePartView))
#define MODEST_MOZEMBED_MIME_PART_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MOZEMBED_MIME_PART_VIEW,ModestMozembedMimePartViewClass))
#define MODEST_IS_MOZEMBED_MIME_PART_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MOZEMBED_MIME_PART_VIEW))
#define MODEST_IS_MOZEMBED_MIME_PART_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MOZEMBED_MIME_PART_VIEW))
#define MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MOZEMBED_MIME_PART_VIEW,ModestMozembedMimePartViewClass))

typedef struct _ModestMozembedMimePartView      ModestMozembedMimePartView;
typedef struct _ModestMozembedMimePartViewClass ModestMozembedMimePartViewClass;

struct _ModestMozembedMimePartView {
	TnyMozEmbedHtmlMimePartView parent;
};

struct _ModestMozembedMimePartViewClass {
	TnyMozEmbedHtmlMimePartViewClass parent_class;

	/* ModestMimePartView interface methods */
	gboolean (*is_empty_func) (ModestMimePartView *self);
	/* ModestZoomable interface methods */
	gdouble (*get_zoom_func) (ModestZoomable *self);
	void (*set_zoom_func) (ModestZoomable *self, gdouble value);
	gboolean (*zoom_minus_func) (ModestZoomable *self);
	gboolean (*zoom_plus_func) (ModestZoomable *self);
	/* ModestISearchView interface methods */
	gboolean (*search_func)             (ModestISearchView *self, const gchar *string);
	gboolean (*search_next_func)        (ModestISearchView *self);
	gboolean (*get_selection_area_func) (ModestISearchView *self, gint *x, gint *y, gint *width, gint *height);
};


/**
 *
 * modest_mozembed_mime_part_view_get_type
 *
 * get the GType for the this class
 *
 * Returns: the GType for this class
 */
GType        modest_mozembed_mime_part_view_get_type    (void) G_GNUC_CONST;

/**
 * modest_mozembed_mime_part_view_new:
 *
 * create a new #ModestMozembedMimePartView instance, implementing
 * interfaces #TnyMimePartView, #ModestMimePartView, #ModestZoomable
 * and #ModestISearchView.
 *
 * Returns: a #ModestMozembedMimePartView
 */
GtkWidget   *modest_mozembed_mime_part_view_new (void);

G_END_DECLS

#endif /* __MODEST_GTK_HTML_MIME_PART_VIEW_H__ */

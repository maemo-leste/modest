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

#include <widgets/modest-mozembed-mime-part-view.h>
#include <string.h>
#include <tny-stream.h>
#include <tny-mime-part-view.h>
#include <tny-gtk-text-buffer-stream.h>
#include <tny-moz-embed-html-mime-part-view.h>
#include <modest-text-utils.h>
#include <widgets/modest-mime-part-view.h>
#include <widgets/modest-zoomable.h>

/* gobject structure methods */
static void    modest_mozembed_mime_part_view_class_init (ModestMozembedMimePartViewClass *klass);
static void    modest_mime_part_view_init               (gpointer g, gpointer iface_data);
static void    modest_zoomable_init                     (gpointer g, gpointer iface_data);
static void    modest_isearch_view_init                 (gpointer g, gpointer iface_data);
static void    modest_mozembed_mime_part_view_init       (ModestMozembedMimePartView *self);
static void    modest_mozembed_mime_part_view_finalize   (GObject *self);

/* ModestMimePartView implementation */
static gboolean modest_mozembed_mime_part_view_is_empty (ModestMimePartView *self);
static gboolean modest_mozembed_mime_part_view_is_empty_default (ModestMimePartView *self);
/* ModestZoomable implementation */
static gdouble modest_mozembed_mime_part_view_get_zoom (ModestZoomable *self);
static void modest_mozembed_mime_part_view_set_zoom (ModestZoomable *self, gdouble value);
static gboolean modest_mozembed_mime_part_view_zoom_minus (ModestZoomable *self);
static gboolean modest_mozembed_mime_part_view_zoom_plus (ModestZoomable *self);
static gdouble modest_mozembed_mime_part_view_get_zoom_default (ModestZoomable *self);
static void modest_mozembed_mime_part_view_set_zoom_default (ModestZoomable *self, gdouble value);
static gboolean modest_mozembed_mime_part_view_zoom_minus_default (ModestZoomable *self);
static gboolean modest_mozembed_mime_part_view_zoom_plus_default (ModestZoomable *self);
/* ModestISearchView implementation */
static gboolean modest_mozembed_mime_part_view_search                    (ModestISearchView *self, const gchar *string);
static gboolean modest_mozembed_mime_part_view_search_next               (ModestISearchView *self);
static gboolean modest_mozembed_mime_part_view_get_selection_area        (ModestISearchView *self, gint *x, gint *y, 
									 gint *width, gint *height);
static gboolean modest_mozembed_mime_part_view_search_default            (ModestISearchView *self, const gchar *string);
static gboolean modest_mozembed_mime_part_view_search_next_default       (ModestISearchView *self);
static gboolean modest_mozembed_mime_part_view_get_selection_area_default (ModestISearchView *self, gint *x, gint *y, 
									  gint *width, gint *height);


/* internal api */
static gboolean      is_empty   (ModestMozembedMimePartView *self);
static void          set_zoom   (ModestMozembedMimePartView *self, gdouble zoom);
static gdouble       get_zoom   (ModestMozembedMimePartView *self);
static gboolean      search             (ModestMozembedMimePartView *self, const gchar *string);
static gboolean      search_next        (ModestMozembedMimePartView *self);
static gboolean      get_selection_area (ModestMozembedMimePartView *self, gint *x, gint *y,
					 gint *width, gint *height);

typedef struct _ModestMozembedMimePartViewPrivate ModestMozembedMimePartViewPrivate;
struct _ModestMozembedMimePartViewPrivate {
	gdouble current_zoom;
};

#define MODEST_MOZEMBED_MIME_PART_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										       MODEST_TYPE_MOZEMBED_MIME_PART_VIEW, \
										       ModestMozembedMimePartViewPrivate))

static GtkMozEmbedClass *parent_class = NULL;

GtkWidget *
modest_mozembed_mime_part_view_new ()
{
	return g_object_new (MODEST_TYPE_MOZEMBED_MIME_PART_VIEW, NULL);
}

/* GOBJECT IMPLEMENTATION */
GType
modest_mozembed_mime_part_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMozembedMimePartViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_mozembed_mime_part_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMozembedMimePartView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_mozembed_mime_part_view_init,
			NULL
		};

		static const GInterfaceInfo modest_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) modest_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo modest_zoomable_info = 
		{
		  (GInterfaceInitFunc) modest_zoomable_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo modest_isearch_view_info = 
		{
		  (GInterfaceInitFunc) modest_isearch_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

 		my_type = g_type_register_static (TNY_TYPE_MOZ_EMBED_HTML_MIME_PART_VIEW,
		                                  "ModestMozembedMimePartView",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, MODEST_TYPE_MIME_PART_VIEW, 
			&modest_mime_part_view_info);

		g_type_add_interface_static (my_type, MODEST_TYPE_ZOOMABLE, 
			&modest_zoomable_info);
		g_type_add_interface_static (my_type, MODEST_TYPE_ISEARCH_VIEW, 
			&modest_isearch_view_info);
	}
	return my_type;
}

static void
modest_mozembed_mime_part_view_class_init (ModestMozembedMimePartViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_mozembed_mime_part_view_finalize;
	klass->is_empty_func = modest_mozembed_mime_part_view_is_empty_default;
	klass->get_zoom_func = modest_mozembed_mime_part_view_get_zoom_default;
	klass->set_zoom_func = modest_mozembed_mime_part_view_set_zoom_default;
	klass->zoom_minus_func = modest_mozembed_mime_part_view_zoom_minus_default;
	klass->zoom_plus_func = modest_mozembed_mime_part_view_zoom_plus_default;
	klass->search_func = modest_mozembed_mime_part_view_search_default;
	klass->search_next_func = modest_mozembed_mime_part_view_search_next_default;
	klass->get_selection_area_func = modest_mozembed_mime_part_view_get_selection_area_default;
	
	g_type_class_add_private (gobject_class, sizeof(ModestMozembedMimePartViewPrivate));

}

static void    
modest_mozembed_mime_part_view_init (ModestMozembedMimePartView *self)
{
	ModestMozembedMimePartViewPrivate *priv = MODEST_MOZEMBED_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->current_zoom = 1.0;

}

static void
modest_mozembed_mime_part_view_finalize (GObject *obj)
{
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* INTERNAL API */

static gboolean      
is_empty   (ModestMozembedMimePartView *self)
{
	/* TODO: not implemented yet */
	return FALSE;
}

static void
set_zoom (ModestMozembedMimePartView *self, gdouble zoom)
{
	ModestMozembedMimePartViewPrivate *priv;

	g_return_if_fail (MODEST_IS_MOZEMBED_MIME_PART_VIEW (self));

	priv = MODEST_MOZEMBED_MIME_PART_VIEW_GET_PRIVATE (self);
	priv->current_zoom = zoom;
	gtk_moz_embed_set_zoom_level (GTK_MOZ_EMBED(self), (gint) (zoom*100), NULL);

	gtk_widget_queue_resize (GTK_WIDGET (self));
}

static gdouble
get_zoom (ModestMozembedMimePartView *self)
{
	ModestMozembedMimePartViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MOZEMBED_MIME_PART_VIEW (self), 1.0);

	priv = MODEST_MOZEMBED_MIME_PART_VIEW_GET_PRIVATE (self);

	return priv->current_zoom;
}

static gboolean
search (ModestMozembedMimePartView *self, 
	const gchar *string)
{
	/* TODO: Implement incremental search */
	return FALSE;
}

static gboolean
search_next (ModestMozembedMimePartView *self)
{
	/* TODO: Implement incremental search */
	return FALSE;
}

static gboolean
get_selection_area (ModestMozembedMimePartView *self, 
		    gint *x, gint *y,
		    gint *width, gint *height)
{
	/* TODO: Implement incremental search */
	return FALSE;

}


/* MODEST MIME PART VIEW IMPLEMENTATION */

static void
modest_mime_part_view_init (gpointer g, gpointer iface_data)
{
	ModestMimePartViewIface *klass = (ModestMimePartViewIface *)g;

	klass->is_empty_func = modest_mozembed_mime_part_view_is_empty;

	return;
}

static gboolean
modest_mozembed_mime_part_view_is_empty (ModestMimePartView *self)
{
	return MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->is_empty_func (self);
}

static gboolean
modest_mozembed_mime_part_view_is_empty_default (ModestMimePartView *self)
{
	return is_empty (MODEST_MOZEMBED_MIME_PART_VIEW (self));
}


/* MODEST ZOOMABLE IMPLEMENTATION */
static void
modest_zoomable_init (gpointer g, gpointer iface_data)
{
	ModestZoomableIface *klass = (ModestZoomableIface *)g;
	
	klass->get_zoom_func = modest_mozembed_mime_part_view_get_zoom;
	klass->set_zoom_func = modest_mozembed_mime_part_view_set_zoom;
	klass->zoom_minus_func = modest_mozembed_mime_part_view_zoom_minus;
	klass->zoom_plus_func = modest_mozembed_mime_part_view_zoom_plus;

	return;
}

static gdouble
modest_mozembed_mime_part_view_get_zoom (ModestZoomable *self)
{
	return MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->get_zoom_func (self);
}

static gdouble
modest_mozembed_mime_part_view_get_zoom_default (ModestZoomable *self)
{
	return get_zoom (MODEST_MOZEMBED_MIME_PART_VIEW (self));
}

static void
modest_mozembed_mime_part_view_set_zoom (ModestZoomable *self, gdouble value)
{
	MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->set_zoom_func (self, value);
}

static void
modest_mozembed_mime_part_view_set_zoom_default (ModestZoomable *self, gdouble value)
{
	set_zoom (MODEST_MOZEMBED_MIME_PART_VIEW (self), value);
}

static gboolean
modest_mozembed_mime_part_view_zoom_minus (ModestZoomable *self)
{
	return MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->zoom_minus_func (self);
}

static gboolean
modest_mozembed_mime_part_view_zoom_minus_default (ModestZoomable *self)
{
	/* operation not supported in ModestMozembedMimePartView */
	return FALSE;
}

static gboolean
modest_mozembed_mime_part_view_zoom_plus (ModestZoomable *self)
{
	return MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->zoom_plus_func (self);
}

static gboolean
modest_mozembed_mime_part_view_zoom_plus_default (ModestZoomable *self)
{
	/* operation not supported in ModestMozembedMimePartView */
	return FALSE;
}

/* ISEARCH VIEW IMPLEMENTATION */
static void
modest_isearch_view_init (gpointer g, gpointer iface_data)
{
	ModestISearchViewIface *klass = (ModestISearchViewIface *)g;
	
	klass->search_func = modest_mozembed_mime_part_view_search;
	klass->search_next_func = modest_mozembed_mime_part_view_search_next;
	klass->get_selection_area_func = modest_mozembed_mime_part_view_get_selection_area;

	return;
}

static gboolean 
modest_mozembed_mime_part_view_search (ModestISearchView *self, const gchar *string)
{
	return MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->search_func (self, string);
}

static gboolean 
modest_mozembed_mime_part_view_search_default (ModestISearchView *self, const gchar *string)
{
	return search (MODEST_MOZEMBED_MIME_PART_VIEW (self), string);
}

static gboolean 
modest_mozembed_mime_part_view_search_next(ModestISearchView *self)
{
	return MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->search_next_func (self);
}

static gboolean 
modest_mozembed_mime_part_view_search_next_default (ModestISearchView *self)
{
	return search_next (MODEST_MOZEMBED_MIME_PART_VIEW (self));
}

static gboolean 
modest_mozembed_mime_part_view_get_selection_area (ModestISearchView *self, gint *x, gint *y, 
						  gint *width, gint *height)
{
	return MODEST_MOZEMBED_MIME_PART_VIEW_GET_CLASS (self)->get_selection_area_func (self, x, y, width, height);
}

static gboolean 
modest_mozembed_mime_part_view_get_selection_area_default (ModestISearchView *self, gint *x, gint *y, 
							  gint *width, gint *height)
{
	return get_selection_area (MODEST_MOZEMBED_MIME_PART_VIEW (self), x, y, width, height);
}

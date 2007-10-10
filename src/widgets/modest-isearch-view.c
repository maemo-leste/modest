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

#include <config.h>

#include <modest-isearch-view.h>

/**
 * modest_isearch_view_search:
 * @self: a #ModestISearchView
 * @string: a string
 *
 * Begins a new search in @self trying to match @string.
 * 
 * Returns: %TRUE if @string was matches, %FALSE otherwise
 */
gboolean
modest_isearch_view_search (ModestISearchView *self, const gchar *string)
{
	return MODEST_ISEARCH_VIEW_GET_IFACE (self)->search_func (self, string);
}

/**
 * modest_isearch_view_search_next:
 * @self: a #ModestISearchView
 *
 * Continues the last search.
 * 
 * Returns: %TRUE if last search matches again, %FALSE otherwise
 */
gboolean
modest_isearch_view_search_next (ModestISearchView *self)
{
	return MODEST_ISEARCH_VIEW_GET_IFACE (self)->search_next_func (self);
}

/**
 * modest_isearch_view_get_selection_area:
 * @self: a #ModestISearchView
 * @x: a #gint pointer (out)
 * @y: a #gint pointer (out)
 * @width: a #gint pointer (out)
 * @height: a #gint pointer (out)
 *
 * Obtains the area of the last search match.
 * 
 * Returns: %TRUE if there's any search matched, %FALSE otherwise
 */
gboolean
modest_isearch_view_get_selection_area (ModestISearchView *self, 
					gint *x, gint *y, 
					gint *width, gint *height)
{
	return MODEST_ISEARCH_VIEW_GET_IFACE (self)->get_selection_area_func (self, x, y, width, height);
}

static void
modest_isearch_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* init signals here */
		initialized = TRUE;
	}
}

GType
modest_isearch_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0)) 
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestISearchViewIface),
		  modest_isearch_view_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL,   /* instance_init */
		  NULL
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
			"ModestISearchView", &info, 0);

	}

	return type;
}

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

#include <config.h>

#include <widgets/modest-sort-criterium-view.h>

gint
modest_sort_criterium_view_add_sort_key (ModestSortCriteriumView *self, const gchar *sort_key)
{
	return MODEST_SORT_CRITERIUM_VIEW_GET_IFACE (self)->add_sort_key_func (self, sort_key);
}

void
modest_sort_criterium_view_set_sort_key (ModestSortCriteriumView *self, gint key)
{
	MODEST_SORT_CRITERIUM_VIEW_GET_IFACE (self)->set_sort_key_func (self, key);
}

void
modest_sort_criterium_view_set_sort_order (ModestSortCriteriumView *self, GtkSortType sort_type)
{
	MODEST_SORT_CRITERIUM_VIEW_GET_IFACE (self)->set_sort_order_func (self, sort_type);
}

gint
modest_sort_criterium_view_get_sort_key (ModestSortCriteriumView *self)
{
	return MODEST_SORT_CRITERIUM_VIEW_GET_IFACE (self)->get_sort_key_func (self);
}

GtkSortType
modest_sort_criterium_view_get_sort_order (ModestSortCriteriumView *self)
{
	return MODEST_SORT_CRITERIUM_VIEW_GET_IFACE (self)->get_sort_order_func (self);
}


static void
modest_sort_criterium_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		
		initialized = TRUE;
	}
}

GType
modest_sort_criterium_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestSortCriteriumViewIface),
			modest_sort_criterium_view_base_init,   /* base init */
			NULL,		/* base finalize */
			NULL,           /* class_init */
			NULL,		/* class finalize */
			NULL,		/* class data */
			0,
			0,		/* n_preallocs */
			NULL,           /* instance_init */
			NULL
		};

 		my_type = g_type_register_static (G_TYPE_INTERFACE,
		                                  "ModestSortCriteriumView",
		                                  &my_info, 0);


	}
	return my_type;
}

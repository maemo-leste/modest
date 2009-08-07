/* Copyright (c) 2008, Nokia Corporation
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

#ifndef __MODEST_SORT_CRITERIUM_VIEW_H__
#define __MODEST_SORT_CRITERIUM_VIEW_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_SORT_CRITERIUM_VIEW             (modest_sort_criterium_view_get_type())
#define MODEST_SORT_CRITERIUM_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_SORT_CRITERIUM_VIEW,ModestSortCriteriumView))
#define MODEST_IS_SORT_CRITERIUM_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_SORT_CRITERIUM_VIEW))
#define MODEST_SORT_CRITERIUM_VIEW_GET_IFACE(obj)   (G_TYPE_INSTANCE_GET_INTERFACE((obj),MODEST_TYPE_SORT_CRITERIUM_VIEW,ModestSortCriteriumViewIface))

typedef struct _ModestSortCriteriumView      ModestSortCriteriumView;
typedef struct _ModestSortCriteriumViewIface ModestSortCriteriumViewIface;

struct _ModestSortCriteriumViewIface {
	GTypeInterface parent;

	gint (*add_sort_key_func) (ModestSortCriteriumView *self, const gchar *sort_key);
	void (*set_sort_key_func) (ModestSortCriteriumView *self, gint key);
	void (*set_sort_order_func) (ModestSortCriteriumView *self, GtkSortType sort_type);
	gint (*get_sort_key_func) (ModestSortCriteriumView *self);
	GtkSortType (*get_sort_order_func) (ModestSortCriteriumView *self);
};


/**
 *
 * modest_sort_criterium_view_get_type
 *
 * get the GType for the this interface
 *
 * Returns: the GType for this interface
 */
GType        modest_sort_criterium_view_get_type    (void) G_GNUC_CONST;


gint modest_sort_criterium_view_add_sort_key (ModestSortCriteriumView *self, const gchar *sort_key);
void modest_sort_criterium_view_set_sort_key (ModestSortCriteriumView *self, gint key);
void modest_sort_criterium_view_set_sort_order (ModestSortCriteriumView *self, GtkSortType sort_type);
gint modest_sort_criterium_view_get_sort_key (ModestSortCriteriumView *self);
GtkSortType modest_sort_criterium_view_get_sort_order (ModestSortCriteriumView *self);

G_END_DECLS

#endif /* __MODEST_SORT_CRITERIUM_VIEW_H__ */

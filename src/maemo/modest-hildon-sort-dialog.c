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

#include "modest-hildon-sort-dialog.h"
#include "widgets/modest-sort-criterium-view.h"


static gint    modest_hildon_sort_dialog_add_sort_key        (ModestSortCriteriumView *self,
								      const gchar *sort_key);
static void    modest_hildon_sort_dialog_set_sort_key (ModestSortCriteriumView *self, gint key);
static void    modest_hildon_sort_dialog_set_sort_order (ModestSortCriteriumView *self, GtkSortType sort_type);
static gint    modest_hildon_sort_dialog_get_sort_key (ModestSortCriteriumView *self);
static GtkSortType modest_hildon_sort_dialog_get_sort_order (ModestSortCriteriumView *self);
static void modest_sort_criterium_view_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (ModestHildonSortDialog, 
			modest_hildon_sort_dialog, 
			HILDON_TYPE_SORT_DIALOG,
			0,
			G_IMPLEMENT_INTERFACE (MODEST_TYPE_SORT_CRITERIUM_VIEW, modest_sort_criterium_view_init));

static void
modest_hildon_sort_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_hildon_sort_dialog_parent_class)->finalize (object);
}

static void
modest_hildon_sort_dialog_class_init (ModestHildonSortDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = modest_hildon_sort_dialog_finalize;

}

static void
modest_hildon_sort_dialog_init (ModestHildonSortDialog *self)
{
}

static gint
modest_hildon_sort_dialog_add_sort_key        (ModestSortCriteriumView *self,
					       const gchar *sort_key)
{
	g_return_val_if_fail (MODEST_IS_HILDON_SORT_DIALOG (self), 0);

	return hildon_sort_dialog_add_sort_key (HILDON_SORT_DIALOG (self), sort_key);
}

static void
modest_hildon_sort_dialog_set_sort_key (ModestSortCriteriumView *self, gint key)
{
	g_return_if_fail (MODEST_IS_HILDON_SORT_DIALOG (self));

	hildon_sort_dialog_set_sort_key (HILDON_SORT_DIALOG (self), key);
}

static void
modest_hildon_sort_dialog_set_sort_order (ModestSortCriteriumView *self, GtkSortType sort_type)
{
	g_return_if_fail (MODEST_IS_HILDON_SORT_DIALOG (self));

	hildon_sort_dialog_set_sort_order (HILDON_SORT_DIALOG (self), sort_type);
}

static gint
modest_hildon_sort_dialog_get_sort_key (ModestSortCriteriumView *self)
{
	g_return_val_if_fail (MODEST_IS_HILDON_SORT_DIALOG (self), 0);

	return hildon_sort_dialog_get_sort_key (HILDON_SORT_DIALOG (self));
}

static GtkSortType 
modest_hildon_sort_dialog_get_sort_order (ModestSortCriteriumView *self)
{
	g_return_val_if_fail (MODEST_IS_HILDON_SORT_DIALOG (self), GTK_SORT_ASCENDING);

	return hildon_sort_dialog_get_sort_order (HILDON_SORT_DIALOG (self));
}

static void
modest_sort_criterium_view_init (gpointer g_iface,
				 gpointer iface_data)
{
	ModestSortCriteriumViewIface *iface = (ModestSortCriteriumViewIface *) g_iface;

	iface->add_sort_key_func = modest_hildon_sort_dialog_add_sort_key;
	iface->get_sort_key_func = modest_hildon_sort_dialog_get_sort_key;
	iface->set_sort_key_func = modest_hildon_sort_dialog_set_sort_key;
	iface->get_sort_order_func = modest_hildon_sort_dialog_get_sort_order;
	iface->set_sort_order_func = modest_hildon_sort_dialog_set_sort_order;
}

GtkWidget*
modest_hildon_sort_dialog_new (GtkWindow *parent)
{
	GtkWidget *result = g_object_new (MODEST_TYPE_HILDON_SORT_DIALOG, NULL);


	if (parent)
		gtk_window_set_transient_for(GTK_WINDOW(result), parent);

	return result;
}


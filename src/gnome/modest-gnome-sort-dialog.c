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

#include <glib/gi18n.h>
#include "modest-gnome-sort-dialog.h"
#include "widgets/modest-sort-criterium-view.h"
#include <gtk/gtk.h>

static gint    modest_gnome_sort_dialog_add_sort_key        (ModestSortCriteriumView *self,
								      const gchar *sort_key);
static void    modest_gnome_sort_dialog_set_sort_key (ModestSortCriteriumView *self, gint key);
static void    modest_gnome_sort_dialog_set_sort_order (ModestSortCriteriumView *self, GtkSortType sort_type);
static gint    modest_gnome_sort_dialog_get_sort_key (ModestSortCriteriumView *self);
static GtkSortType modest_gnome_sort_dialog_get_sort_order (ModestSortCriteriumView *self);
static void modest_sort_criterium_view_init (gpointer g_iface, gpointer iface_data);

typedef struct _ModestGnomeSortDialogPrivate ModestGnomeSortDialogPrivate;
struct _ModestGnomeSortDialogPrivate {
	GtkWidget *sort_field_combo;
	GtkWidget *sort_type_combo;
	gint sort_field_next;
};

#define MODEST_GNOME_SORT_DIALOG_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										  MODEST_TYPE_GNOME_SORT_DIALOG, \
										 ModestGnomeSortDialogPrivate))


G_DEFINE_TYPE_EXTENDED (ModestGnomeSortDialog, 
			modest_gnome_sort_dialog, 
			GTK_TYPE_DIALOG,
			0,
			G_IMPLEMENT_INTERFACE (MODEST_TYPE_SORT_CRITERIUM_VIEW, modest_sort_criterium_view_init));

static void
modest_gnome_sort_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_gnome_sort_dialog_parent_class)->finalize (object);
}

static void
modest_gnome_sort_dialog_class_init (ModestGnomeSortDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = modest_gnome_sort_dialog_finalize;

	g_type_class_add_private (object_class, sizeof(ModestGnomeSortDialogPrivate));

}

static void
modest_gnome_sort_dialog_init (ModestGnomeSortDialog *self)
{
	GtkWidget *vbox;
	GtkStockItem item;
	ModestGnomeSortDialogPrivate *priv = MODEST_GNOME_SORT_DIALOG_GET_PRIVATE (self);

	gtk_dialog_add_buttons (GTK_DIALOG (self),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	vbox = gtk_vbox_new (FALSE, 3);
	priv->sort_field_combo = gtk_combo_box_new_text ();
	priv->sort_field_next = 0;
	priv->sort_type_combo = gtk_combo_box_new_text ();
	gtk_stock_lookup (GTK_STOCK_SORT_ASCENDING, &item);
	gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->sort_type_combo), GTK_SORT_ASCENDING, _(item.label));
	gtk_stock_lookup (GTK_STOCK_SORT_DESCENDING, &item);
	gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->sort_type_combo), GTK_SORT_DESCENDING, _(item.label));

	gtk_box_pack_start (GTK_BOX (vbox), priv->sort_field_combo, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->sort_type_combo, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (self)->vbox), vbox, FALSE, FALSE, 0);
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_me_inbox_sort"));

	gtk_widget_show_all (vbox);
}

static gint
modest_gnome_sort_dialog_add_sort_key        (ModestSortCriteriumView *self,
					       const gchar *sort_key)
{
	ModestGnomeSortDialogPrivate *priv;
	gint retval;

	g_return_val_if_fail (MODEST_IS_GNOME_SORT_DIALOG (self), 0);
	priv = MODEST_GNOME_SORT_DIALOG_GET_PRIVATE (self);

	retval = priv->sort_field_next;
	priv->sort_field_next++;

	gtk_combo_box_insert_text (GTK_COMBO_BOX (priv->sort_field_combo), retval, sort_key);

	return retval;
}

static void
modest_gnome_sort_dialog_set_sort_key (ModestSortCriteriumView *self, gint key)
{
	ModestGnomeSortDialogPrivate *priv;

	g_return_if_fail (MODEST_IS_GNOME_SORT_DIALOG (self));
	priv = MODEST_GNOME_SORT_DIALOG_GET_PRIVATE (self);

	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->sort_field_combo), key);
}

static void
modest_gnome_sort_dialog_set_sort_order (ModestSortCriteriumView *self, GtkSortType sort_type)
{
	ModestGnomeSortDialogPrivate *priv;

	g_return_if_fail (MODEST_IS_GNOME_SORT_DIALOG (self));
	priv = MODEST_GNOME_SORT_DIALOG_GET_PRIVATE (self);

	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->sort_type_combo), sort_type);
}

static gint
modest_gnome_sort_dialog_get_sort_key (ModestSortCriteriumView *self)
{
	ModestGnomeSortDialogPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GNOME_SORT_DIALOG (self), 0);
	priv = MODEST_GNOME_SORT_DIALOG_GET_PRIVATE (self);

	return gtk_combo_box_get_active (GTK_COMBO_BOX (priv->sort_field_combo));
}

static GtkSortType 
modest_gnome_sort_dialog_get_sort_order (ModestSortCriteriumView *self)
{
	ModestGnomeSortDialogPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GNOME_SORT_DIALOG (self), GTK_SORT_ASCENDING);
	priv = MODEST_GNOME_SORT_DIALOG_GET_PRIVATE (self);

	return gtk_combo_box_get_active (GTK_COMBO_BOX (priv->sort_type_combo));
}

static void
modest_sort_criterium_view_init (gpointer g_iface,
				 gpointer iface_data)
{
	ModestSortCriteriumViewIface *iface = (ModestSortCriteriumViewIface *) g_iface;

	iface->add_sort_key_func = modest_gnome_sort_dialog_add_sort_key;
	iface->get_sort_key_func = modest_gnome_sort_dialog_get_sort_key;
	iface->set_sort_key_func = modest_gnome_sort_dialog_set_sort_key;
	iface->get_sort_order_func = modest_gnome_sort_dialog_get_sort_order;
	iface->set_sort_order_func = modest_gnome_sort_dialog_set_sort_order;
}

GtkWidget*
modest_gnome_sort_dialog_new (GtkWindow *parent)
{
	GtkWidget *result = g_object_new (MODEST_TYPE_GNOME_SORT_DIALOG, NULL);


	if (parent)
		gtk_window_set_transient_for(GTK_WINDOW(result), parent);

	return result;
}


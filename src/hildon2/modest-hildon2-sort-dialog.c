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

#include "modest-defs.h"
#include "modest-text-utils.h"
#include "modest-hildon2-sort-dialog.h"
#include "widgets/modest-sort-criterium-view.h"


static gint    modest_hildon2_sort_dialog_add_sort_key  (ModestSortCriteriumView *self, const gchar *sort_key);
static void    modest_hildon2_sort_dialog_set_sort_key (ModestSortCriteriumView *self, gint key);
static void    modest_hildon2_sort_dialog_set_sort_order (ModestSortCriteriumView *self, GtkSortType sort_type);
static gint    modest_hildon2_sort_dialog_get_sort_key (ModestSortCriteriumView *self);
static GtkSortType modest_hildon2_sort_dialog_get_sort_order (ModestSortCriteriumView *self);
static void modest_sort_criterium_view_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_EXTENDED (ModestHildon2SortDialog, 
			modest_hildon2_sort_dialog, 
			HILDON_TYPE_PICKER_DIALOG,
			0,
			G_IMPLEMENT_INTERFACE (MODEST_TYPE_SORT_CRITERIUM_VIEW, modest_sort_criterium_view_init));

typedef struct _ModestHildon2SortDialogPrivate ModestHildon2SortDialogPrivate;
struct _ModestHildon2SortDialogPrivate {
	gint id;
	GtkSortType sort_type;
};

#define MODEST_HILDON2_SORT_DIALOG_GET_PRIVATE(o)   (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_HILDON2_SORT_DIALOG, \
                                                    ModestHildon2SortDialogPrivate))

enum {
	MODEST_HILDON2_SORT_DIALOG_RECIPIENT_AZ,
	MODEST_HILDON2_SORT_DIALOG_RECIPIENT_ZA,
	MODEST_HILDON2_SORT_DIALOG_DATE_NEWEST,
	MODEST_HILDON2_SORT_DIALOG_DATE_OLDEST,
	MODEST_HILDON2_SORT_DIALOG_SUBJECT_AZ,
	MODEST_HILDON2_SORT_DIALOG_SUBJECT_ZA,
	MODEST_HILDON2_SORT_DIALOG_ATTACHMENT,
	MODEST_HILDON2_SORT_DIALOG_SIZE_LARGEST,
	MODEST_HILDON2_SORT_DIALOG_SIZE_SMALLEST,
	MODEST_HILDON2_SORT_DIALOG_PRIORITY,
	MODEST_HILDON2_SORT_DIALOG_NUM_SORT_CRITERIUM
};

enum {
	NAME_COLUMN,
	ID_COLUMN,
	NUM_COLUMS
};

static void
modest_hildon2_sort_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_hildon2_sort_dialog_parent_class)->finalize (object);
}

static void
modest_hildon2_sort_dialog_class_init (ModestHildon2SortDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = modest_hildon2_sort_dialog_finalize;

	g_type_class_add_private (object_class, sizeof(ModestHildon2SortDialogClass));
}

static void
modest_hildon2_sort_dialog_init (ModestHildon2SortDialog *self)
{
	ModestHildon2SortDialogPrivate *priv;

	priv = MODEST_HILDON2_SORT_DIALOG_GET_PRIVATE(self);
	priv->id = 0;
	priv->sort_type = GTK_SORT_ASCENDING;
}

static gint
modest_hildon2_sort_dialog_add_sort_key (ModestSortCriteriumView *self, const gchar *sort_key)
{
	ModestHildon2SortDialogPrivate *priv;

	priv = MODEST_HILDON2_SORT_DIALOG_GET_PRIVATE(self);

	/* We return a different ID each time, it does not really
	   matter which one we return, we just want them to be
	   different */
	return priv->id++;
}

static void
modest_hildon2_sort_dialog_set_sort_key (ModestSortCriteriumView *self, gint key)
{
	HildonTouchSelector *selector;
	ModestHildon2SortDialogPrivate *priv;
	gint active;

	g_return_if_fail (MODEST_IS_HILDON2_SORT_DIALOG (self));

	priv = MODEST_HILDON2_SORT_DIALOG_GET_PRIVATE(self);
	selector = hildon_picker_dialog_get_selector (HILDON_PICKER_DIALOG (self));

	/* Map to our selector */
	switch (key) {
	case 0:
		if (priv->sort_type == GTK_SORT_ASCENDING)
			active = MODEST_HILDON2_SORT_DIALOG_RECIPIENT_AZ;
		else
			active = MODEST_HILDON2_SORT_DIALOG_RECIPIENT_ZA;
		break;
	case 1:
		if (priv->sort_type == GTK_SORT_ASCENDING)
			active = MODEST_HILDON2_SORT_DIALOG_DATE_OLDEST;
		else
			active = MODEST_HILDON2_SORT_DIALOG_DATE_NEWEST;
		break;
	case 2:
		if (priv->sort_type == GTK_SORT_ASCENDING)
			active = MODEST_HILDON2_SORT_DIALOG_SUBJECT_AZ;
		else
			active = MODEST_HILDON2_SORT_DIALOG_SUBJECT_ZA;
		break;
	case 3:
		active = MODEST_HILDON2_SORT_DIALOG_ATTACHMENT;
		break;
	case 4:
		if (priv->sort_type == GTK_SORT_ASCENDING)
			active = MODEST_HILDON2_SORT_DIALOG_SIZE_SMALLEST;
		else
			active = MODEST_HILDON2_SORT_DIALOG_SIZE_LARGEST;
		break;
	case 5:
		active = MODEST_HILDON2_SORT_DIALOG_PRIORITY;
		break;
	default:
		g_warning ("%s, no valid key found... falling back to default (0)", __FUNCTION__);
		active = 0;
	}
	hildon_touch_selector_set_active (selector, 0, active);
}

static void
modest_hildon2_sort_dialog_set_sort_order (ModestSortCriteriumView *self, 
					  GtkSortType sort_type)
{
	HildonTouchSelector *selector;
	gint active, new;
	ModestHildon2SortDialogPrivate *priv;

	g_return_if_fail (MODEST_IS_HILDON2_SORT_DIALOG (self));

	priv = MODEST_HILDON2_SORT_DIALOG_GET_PRIVATE(self);
	selector = hildon_picker_dialog_get_selector (HILDON_PICKER_DIALOG (self));
	active = hildon_touch_selector_get_active (selector, 0);

	/* Map to our selector, we need to check the current selected
	   key because this way we allow to select the order first and
	   then the key */
	switch (active) {
	case MODEST_HILDON2_SORT_DIALOG_RECIPIENT_AZ:
	case MODEST_HILDON2_SORT_DIALOG_RECIPIENT_ZA:
		if (sort_type == GTK_SORT_ASCENDING)
			new = MODEST_HILDON2_SORT_DIALOG_RECIPIENT_ZA;
		else
			new = MODEST_HILDON2_SORT_DIALOG_RECIPIENT_AZ;
		break;
	case MODEST_HILDON2_SORT_DIALOG_DATE_NEWEST:
	case MODEST_HILDON2_SORT_DIALOG_DATE_OLDEST:
		if (sort_type == GTK_SORT_ASCENDING)
			new = MODEST_HILDON2_SORT_DIALOG_DATE_NEWEST;
		else
			new = MODEST_HILDON2_SORT_DIALOG_DATE_OLDEST;
		break;
	case MODEST_HILDON2_SORT_DIALOG_SUBJECT_AZ:
	case MODEST_HILDON2_SORT_DIALOG_SUBJECT_ZA:
		if (sort_type == GTK_SORT_ASCENDING)
			new = MODEST_HILDON2_SORT_DIALOG_SUBJECT_ZA;
		else
			new = MODEST_HILDON2_SORT_DIALOG_SUBJECT_AZ;
		break;
	case MODEST_HILDON2_SORT_DIALOG_ATTACHMENT:
		new = MODEST_HILDON2_SORT_DIALOG_ATTACHMENT;
		break;
	case MODEST_HILDON2_SORT_DIALOG_SIZE_LARGEST:
	case MODEST_HILDON2_SORT_DIALOG_SIZE_SMALLEST:
		if (sort_type == GTK_SORT_ASCENDING)
			new = MODEST_HILDON2_SORT_DIALOG_SIZE_SMALLEST;
		else
			new = MODEST_HILDON2_SORT_DIALOG_SIZE_LARGEST;
		break;
	case MODEST_HILDON2_SORT_DIALOG_PRIORITY:
		new = MODEST_HILDON2_SORT_DIALOG_PRIORITY;
		break;
	default:
		g_warning ("%s, no valid key found... falling back to current (%d)", __FUNCTION__, active);
		new = active;
	}

	/* Activate the proper index */
	hildon_touch_selector_set_active (selector, 0, new);
	priv->sort_type = sort_type;
}

static gint
modest_hildon2_sort_dialog_get_sort_key (ModestSortCriteriumView *self)
{
	HildonTouchSelector *selector;
	gint active;

	g_return_val_if_fail (MODEST_IS_HILDON2_SORT_DIALOG (self), 0);

	selector = hildon_picker_dialog_get_selector (HILDON_PICKER_DIALOG (self));
	active = hildon_touch_selector_get_active (selector, 0);

	switch (active) {
	case MODEST_HILDON2_SORT_DIALOG_RECIPIENT_AZ:
	case MODEST_HILDON2_SORT_DIALOG_RECIPIENT_ZA:
		return 0;
	case MODEST_HILDON2_SORT_DIALOG_DATE_NEWEST:
	case MODEST_HILDON2_SORT_DIALOG_DATE_OLDEST:
		return 1;
	case MODEST_HILDON2_SORT_DIALOG_SUBJECT_AZ:
	case MODEST_HILDON2_SORT_DIALOG_SUBJECT_ZA:
		return 2;
	case MODEST_HILDON2_SORT_DIALOG_ATTACHMENT:
		return 3;
	case MODEST_HILDON2_SORT_DIALOG_SIZE_LARGEST:
	case MODEST_HILDON2_SORT_DIALOG_SIZE_SMALLEST:
		return 4;
	case MODEST_HILDON2_SORT_DIALOG_PRIORITY:
		return 5;
	default:
		/* Default is order by ascending date */
		g_return_val_if_reached (1);
		return 1;
	}
}

static GtkSortType 
modest_hildon2_sort_dialog_get_sort_order (ModestSortCriteriumView *self)
{
	HildonTouchSelector *selector;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint id;

	g_return_val_if_fail (MODEST_IS_HILDON2_SORT_DIALOG (self), GTK_SORT_ASCENDING);

	selector = hildon_picker_dialog_get_selector (HILDON_PICKER_DIALOG (self));
	model = hildon_touch_selector_get_model (selector, 0);

	if (!hildon_touch_selector_get_selected (selector, 0, &iter)) {
		g_return_val_if_fail (gtk_tree_model_get_iter_first (model, &iter), GTK_SORT_ASCENDING);
	}

	gtk_tree_model_get (model, &iter, ID_COLUMN, &id, -1);

	switch (id) {
	case MODEST_HILDON2_SORT_DIALOG_RECIPIENT_AZ:
	case MODEST_HILDON2_SORT_DIALOG_SUBJECT_AZ:
	case MODEST_HILDON2_SORT_DIALOG_DATE_OLDEST:
	case MODEST_HILDON2_SORT_DIALOG_SIZE_SMALLEST:
		return GTK_SORT_ASCENDING;
	case MODEST_HILDON2_SORT_DIALOG_RECIPIENT_ZA:
	case MODEST_HILDON2_SORT_DIALOG_DATE_NEWEST:
	case MODEST_HILDON2_SORT_DIALOG_SUBJECT_ZA:
	case MODEST_HILDON2_SORT_DIALOG_SIZE_LARGEST:	
	case MODEST_HILDON2_SORT_DIALOG_ATTACHMENT:
	case MODEST_HILDON2_SORT_DIALOG_PRIORITY:
		return GTK_SORT_DESCENDING;
	default:
		/* Default is order by ascending date */
		g_return_val_if_reached (GTK_SORT_ASCENDING);
		return GTK_SORT_ASCENDING;
	}
}

static void
modest_sort_criterium_view_init (gpointer g_iface,
				 gpointer iface_data)
{
	ModestSortCriteriumViewIface *iface = (ModestSortCriteriumViewIface *) g_iface;

	iface->add_sort_key_func = modest_hildon2_sort_dialog_add_sort_key;
	iface->get_sort_key_func = modest_hildon2_sort_dialog_get_sort_key;
	iface->set_sort_key_func = modest_hildon2_sort_dialog_set_sort_key;
	iface->get_sort_order_func = modest_hildon2_sort_dialog_get_sort_order;
	iface->set_sort_order_func = modest_hildon2_sort_dialog_set_sort_order;
}

static void
modest_hildon2_sort_dialog_fill (ModestHildon2SortDialog *self)
{
	GtkListStore *names_list;
	GtkWidget *selector;
	GtkTreeIter iter;
	HildonTouchSelectorColumn *column;

	/* Fill in model */
	names_list = gtk_list_store_new (NUM_COLUMS, G_TYPE_STRING, G_TYPE_INT);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_sender_recipient_az"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_RECIPIENT_AZ, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_sender_recipient_za"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_RECIPIENT_ZA, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_date_newest"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_DATE_NEWEST, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_date_oldest"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_DATE_OLDEST, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_subject_az"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_SUBJECT_AZ, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_subject_za"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_SUBJECT_ZA, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_attachment"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_ATTACHMENT, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_size_largest"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_SIZE_LARGEST, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_size_smallest"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_SIZE_SMALLEST, -1);
	gtk_list_store_append (names_list, &iter);
	gtk_list_store_set (names_list, &iter, NAME_COLUMN, _("mcen_li_sort_priority"), 
			    ID_COLUMN, MODEST_HILDON2_SORT_DIALOG_PRIORITY, -1);

	/* Add columns */
	selector = hildon_touch_selector_new ();
	column = hildon_touch_selector_append_text_column (HILDON_TOUCH_SELECTOR (selector), 
							   GTK_TREE_MODEL (names_list), TRUE);
	g_object_set (G_OBJECT (column), "text-column", NAME_COLUMN, NULL);

	/* Set the selector */
	hildon_picker_dialog_set_selector (HILDON_PICKER_DIALOG (self), HILDON_TOUCH_SELECTOR (selector));
}

GtkWidget*
modest_hildon2_sort_dialog_new (GtkWindow *parent)
{
	GtkWidget *result = g_object_new (MODEST_TYPE_HILDON2_SORT_DIALOG, NULL);

	modest_hildon2_sort_dialog_fill (MODEST_HILDON2_SORT_DIALOG (result));

	if (parent)
		gtk_window_set_transient_for(GTK_WINDOW(result), parent);

	/* Set title */
	gtk_window_set_title (GTK_WINDOW (result), _HL("ckdg_ti_sort"));

	return result;
}

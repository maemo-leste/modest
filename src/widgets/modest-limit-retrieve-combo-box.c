/*
 * Copyright (C) 2007 Nokia Corporation, all rights reserved.
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

#include "modest-limit-retrieve-combo-box.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

G_DEFINE_TYPE (ModestLimitRetrieveComboBox, modest_limit_retrieve_combo_box, GTK_TYPE_COMBO_BOX);

#define LIMIT_RETRIEVE_COMBO_BOX_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX, ModestLimitRetrieveComboBoxPrivate))

typedef struct _ModestLimitRetrieveComboBoxPrivate ModestLimitRetrieveComboBoxPrivate;

struct _ModestLimitRetrieveComboBoxPrivate
{
	GtkTreeModel *model;
};

static void
modest_limit_retrieve_combo_box_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_limit_retrieve_combo_box_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_limit_retrieve_combo_box_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_limit_retrieve_combo_box_parent_class)->dispose)
		G_OBJECT_CLASS (modest_limit_retrieve_combo_box_parent_class)->dispose (object);
}

static void
modest_limit_retrieve_combo_box_finalize (GObject *object)
{
	ModestLimitRetrieveComboBoxPrivate *priv = LIMIT_RETRIEVE_COMBO_BOX_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (modest_limit_retrieve_combo_box_parent_class)->finalize (object);
}

static void
modest_limit_retrieve_combo_box_class_init (ModestLimitRetrieveComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestLimitRetrieveComboBoxPrivate));

	object_class->get_property = modest_limit_retrieve_combo_box_get_property;
	object_class->set_property = modest_limit_retrieve_combo_box_set_property;
	object_class->dispose = modest_limit_retrieve_combo_box_dispose;
	object_class->finalize = modest_limit_retrieve_combo_box_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* a string */
	MODEL_COL_NUM = 1 /* an int */
};

static void modest_limit_retrieve_combo_box_fill (ModestLimitRetrieveComboBox *combobox);

static void
modest_limit_retrieve_combo_box_init (ModestLimitRetrieveComboBox *self)
{
	ModestLimitRetrieveComboBoxPrivate *priv = LIMIT_RETRIEVE_COMBO_BOX_GET_PRIVATE (self);

	/* Create a tree model for the combo box,
	 * with a string for the name, and an ID for the limit_retrieve.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));

	/* Setup the combo box: */
	GtkComboBox *combobox = GTK_COMBO_BOX (self);
	gtk_combo_box_set_model (combobox, priv->model);

	/* LimitRetrieve column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer, 
	"text", MODEL_COL_NAME, NULL);
	
	modest_limit_retrieve_combo_box_fill (self);
}

ModestLimitRetrieveComboBox*
modest_limit_retrieve_combo_box_new (void)
{
	return g_object_new (MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX, NULL);
}

/* Fill the combo box with appropriate choices.
 * #combobox: The combo box.
 * @protocol: IMAP or POP.
 */
static void modest_limit_retrieve_combo_box_fill (ModestLimitRetrieveComboBox *combobox)
{	
	ModestLimitRetrieveComboBoxPrivate *priv = LIMIT_RETRIEVE_COMBO_BOX_GET_PRIVATE (combobox);
	
	/* Remove any existing rows: */
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);
	
	GtkTreeIter iter;
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 0, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_nolimit"), -1);
	
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 200, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_200"), -1);
	
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 100, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_100"), -1);

	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 50, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_50"), -1);

	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, MODEL_COL_NUM, 20, MODEL_COL_NAME, _("mcen_fi_advsetup_retrieve_20"), -1);
}

/**
 * Returns the selected limit_retrieve, 
 * or 0 if no limit_retrieve was selected.
 */
gint
modest_limit_retrieve_combo_box_get_active_limit_retrieve (ModestLimitRetrieveComboBox *combobox)
{
	GtkTreeIter active;
	const gboolean found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &active);
	if (found) {
		ModestLimitRetrieveComboBoxPrivate *priv = LIMIT_RETRIEVE_COMBO_BOX_GET_PRIVATE (combobox);

		gint limit_retrieve = 0;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_NUM, &limit_retrieve, -1);
		return limit_retrieve;	
	}

	return 0; /* Failed. */
}

/* This allows us to pass more than one piece of data to the signal handler,
 * and get a result: */
typedef struct 
{
		ModestLimitRetrieveComboBox* self;
		gint num;
		gboolean found;
} ForEachData;

static gboolean
on_model_foreach_select_id(GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	ForEachData *state = (ForEachData*)(user_data);
	
	gboolean result = FALSE;
	
	/* Select the item if it has the matching name: */
	gint num = 0;
	gtk_tree_model_get (model, iter, MODEL_COL_NUM, &num, -1); 
	if(num == state->num) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (state->self), iter);
		
		state->found = TRUE;
		result = TRUE; /* Stop walking the tree. */
	}
	
	return result; /* Whether we keep walking the tree. */
}

/**
 * Selects the specified limit_retrieve, 
 * or FALSE if no limit_retrieve was selected.
 */
gboolean
modest_limit_retrieve_combo_box_set_active_limit_retrieve (ModestLimitRetrieveComboBox *combobox, gint limit_retrieve)
{
	ModestLimitRetrieveComboBoxPrivate *priv = LIMIT_RETRIEVE_COMBO_BOX_GET_PRIVATE (combobox);
	
	/* Create a state instance so we can send two items of data to the signal handler: */
	ForEachData *state = g_new0 (ForEachData, 1);
	state->self = combobox;
	state->num = limit_retrieve;
	state->found = FALSE;
	
	/* Look at each item, and select the one with the correct ID: */
	gtk_tree_model_foreach (priv->model, &on_model_foreach_select_id, state);

	const gboolean result = state->found;
	
	/* Free the state instance: */
	g_free(state);
	
	return result;
}


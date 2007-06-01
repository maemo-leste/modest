/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#include "modest-retrieve-combo-box.h"
#include "modest-defs.h" /* For the conf names. */
#include <gtk/gtkliststore.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <string.h> /* For memcpy() */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

G_DEFINE_TYPE (ModestRetrieveComboBox, modest_retrieve_combo_box, GTK_TYPE_COMBO_BOX);

#define RETRIEVE_COMBO_BOX_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_RETRIEVE_COMBO_BOX, ModestRetrieveComboBoxPrivate))

typedef struct _ModestRetrieveComboBoxPrivate ModestRetrieveComboBoxPrivate;

struct _ModestRetrieveComboBoxPrivate
{
	GtkTreeModel *model;
};

static void
modest_retrieve_combo_box_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_retrieve_combo_box_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_retrieve_combo_box_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_retrieve_combo_box_parent_class)->dispose)
		G_OBJECT_CLASS (modest_retrieve_combo_box_parent_class)->dispose (object);
}

static void
modest_retrieve_combo_box_finalize (GObject *object)
{
	ModestRetrieveComboBoxPrivate *priv = RETRIEVE_COMBO_BOX_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));

	G_OBJECT_CLASS (modest_retrieve_combo_box_parent_class)->finalize (object);
}

static void
modest_retrieve_combo_box_class_init (ModestRetrieveComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestRetrieveComboBoxPrivate));

	object_class->get_property = modest_retrieve_combo_box_get_property;
	object_class->set_property = modest_retrieve_combo_box_set_property;
	object_class->dispose = modest_retrieve_combo_box_dispose;
	object_class->finalize = modest_retrieve_combo_box_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* a string */
	MODEL_COL_CONF_NAME = 1 /* a string */
};

void modest_retrieve_combo_box_fill (ModestRetrieveComboBox *combobox, ModestTransportStoreProtocol protocol);

static void
modest_retrieve_combo_box_init (ModestRetrieveComboBox *self)
{
	ModestRetrieveComboBoxPrivate *priv = RETRIEVE_COMBO_BOX_GET_PRIVATE (self);

	/* Create a tree model for the combo box,
	 * with a string for the name, and an ID for the retrieve.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING));

	/* Setup the combo box: */
	GtkComboBox *combobox = GTK_COMBO_BOX (self);
	gtk_combo_box_set_model (combobox, priv->model);

	/* Retrieve column:
	 * The ID model column in not shown in the view. */
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combobox), renderer, 
	"text", MODEL_COL_NAME, NULL);
	
	/* The application must call modest_retrieve_combo_box_fill(), specifying POP or IMAP. */
}



ModestRetrieveComboBox*
modest_retrieve_combo_box_new (void)
{
	return g_object_new (MODEST_TYPE_RETRIEVE_COMBO_BOX, NULL);
}

/* Fill the combo box with appropriate choices.
 * #combobox: The combo box.
 * @protocol: IMAP or POP.
 */
void modest_retrieve_combo_box_fill (ModestRetrieveComboBox *combobox, ModestTransportStoreProtocol protocol)
{	
	ModestRetrieveComboBoxPrivate *priv = RETRIEVE_COMBO_BOX_GET_PRIVATE (combobox);
	
	/* Remove any existing rows: */
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	gtk_list_store_clear (liststore);
	
	GtkTreeIter iter;
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, 
		MODEL_COL_CONF_NAME, MODEST_ACCOUNT_RETRIEVE_VALUE_HEADERS_ONLY, 
		MODEL_COL_NAME, _("mcen_fi_advsetup_retrievetype_headers"), -1);
	
	/* Only IMAP should have this option, according to the UI spec: */
	if (protocol == MODEST_PROTOCOL_STORE_IMAP) {
		gtk_list_store_append (liststore, &iter);
		gtk_list_store_set (liststore, &iter, 
			MODEL_COL_CONF_NAME, MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES, 
			MODEL_COL_NAME, _("mcen_fi_advsetup_retrievetype_messages"), -1);
	}
	
	
	gtk_list_store_append (liststore, &iter);
	gtk_list_store_set (liststore, &iter, 
		MODEL_COL_CONF_NAME, MODEST_ACCOUNT_RETRIEVE_VALUE_MESSAGES_AND_ATTACHMENTS, 
		MODEL_COL_NAME, _("mcen_fi_advsetup_retrievetype_messages_attachments"), -1);
}

/**
 * Returns the selected retrieve.
 * or NULL if no retrieve was selected. The result must be freed with g_free().
 */
gchar*
modest_retrieve_combo_box_get_active_retrieve_conf (ModestRetrieveComboBox *combobox)
{
	GtkTreeIter active;
	const gboolean found = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combobox), &active);
	if (found) {
		ModestRetrieveComboBoxPrivate *priv = RETRIEVE_COMBO_BOX_GET_PRIVATE (combobox);

		gchar *retrieve = NULL;
		gtk_tree_model_get (priv->model, &active, MODEL_COL_CONF_NAME, &retrieve, -1);
		return retrieve;	
	}

	return NULL; /* Failed. */
}

/* This allows us to pass more than one piece of data to the signal handler,
 * and get a result: */
typedef struct 
{
		ModestRetrieveComboBox* self;
		const gchar* conf_name;
		gboolean found;
} ForEachData;

static gboolean
on_model_foreach_select_id(GtkTreeModel *model, 
	GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	ForEachData *state = (ForEachData*)(user_data);
	
	gboolean result = FALSE;
	
	/* Select the item if it has the matching name: */
	gchar * conf_name = 0;
	gtk_tree_model_get (model, iter, MODEL_COL_CONF_NAME, &conf_name, -1); 
	if(conf_name && state->conf_name && (strcmp(conf_name, state->conf_name) == 0)) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (state->self), iter);
		
		state->found = TRUE;
		result = TRUE; /* Stop walking the tree. */
	}
	g_free (conf_name);
	
	return result; /* Whether we keep walking the tree. */
}

/**
 * Selects the specified retrieve, 
 * or FALSE if no retrieve was selected.
 */
gboolean
modest_retrieve_combo_box_set_active_retrieve_conf (ModestRetrieveComboBox *combobox, const gchar* retrieve)
{
	ModestRetrieveComboBoxPrivate *priv = RETRIEVE_COMBO_BOX_GET_PRIVATE (combobox);
	
	/* Create a state instance so we can send two items of data to the signal handler: */
	ForEachData *state = g_new0 (ForEachData, 1);
	state->self = combobox;
	state->conf_name = retrieve;
	state->found = FALSE;
	
	/* Look at each item, and select the one with the correct ID: */
	gtk_tree_model_foreach (priv->model, &on_model_foreach_select_id, state);

	const gboolean result = state->found;
	
	/* Free the state instance: */
	g_free(state);
	
	return result;
}


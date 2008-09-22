/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _MODEST_LIMIT_RETRIEVE_COMBO_BOX
#define _MODEST_LIMIT_RETRIEVE_COMBO_BOX

#include <gtk/gtkcombobox.h>

G_BEGIN_DECLS

#define MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX modest_limit_retrieve_combo_box_get_type()

#define MODEST_LIMIT_RETRIEVE_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX, ModestLimitRetrieveComboBox))

#define MODEST_LIMIT_RETRIEVE_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX, ModestLimitRetrieveComboBoxClass))

#define MODEST_IS_LIMIT_RETRIEVE_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX))

#define MODEST_IS_LIMIT_RETRIEVE_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX))

#define MODEST_LIMIT_RETRIEVE_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_LIMIT_RETRIEVE_COMBO_BOX, ModestLimitRetrieveComboBoxClass))

typedef struct {
	GtkComboBox parent;
} ModestLimitRetrieveComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} ModestLimitRetrieveComboBoxClass;

GType modest_limit_retrieve_combo_box_get_type (void);

ModestLimitRetrieveComboBox* modest_limit_retrieve_combo_box_new (void);

gint modest_limit_retrieve_combo_box_get_active_limit_retrieve (ModestLimitRetrieveComboBox *combobox);

gboolean modest_limit_retrieve_combo_box_set_active_limit_retrieve (ModestLimitRetrieveComboBox *combobox, gint limit_retrieve);


G_END_DECLS

#endif /* _MODEST_LIMIT_RETRIEVE_COMBO_BOX */

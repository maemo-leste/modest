/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _MODEST_RETRIEVE_COMBO_BOX
#define _MODEST_RETRIEVE_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include "modest-protocol-info.h"
#include <modest-account-settings.h>

G_BEGIN_DECLS

#define MODEST_TYPE_RETRIEVE_COMBO_BOX modest_retrieve_combo_box_get_type()

#define MODEST_RETRIEVE_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_RETRIEVE_COMBO_BOX, ModestRetrieveComboBox))

#define MODEST_RETRIEVE_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_RETRIEVE_COMBO_BOX, ModestRetrieveComboBoxClass))

#define MODEST_IS_RETRIEVE_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_RETRIEVE_COMBO_BOX))

#define MODEST_IS_RETRIEVE_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_RETRIEVE_COMBO_BOX))

#define MODEST_RETRIEVE_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_RETRIEVE_COMBO_BOX, ModestRetrieveComboBoxClass))

typedef struct {
	GtkComboBox parent;
} ModestRetrieveComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} ModestRetrieveComboBoxClass;

GType modest_retrieve_combo_box_get_type (void);

ModestRetrieveComboBox* modest_retrieve_combo_box_new (void);

void modest_retrieve_combo_box_fill (ModestRetrieveComboBox *combobox, ModestTransportStoreProtocol protocol);

ModestAccountRetrieveType modest_retrieve_combo_box_get_active_retrieve_conf (ModestRetrieveComboBox *combobox);

gboolean modest_retrieve_combo_box_set_active_retrieve_conf (ModestRetrieveComboBox *combobox, 
							     ModestAccountRetrieveType retrieve_type);


G_END_DECLS

#endif /* _MODEST_RETRIEVE_COMBO_BOX */

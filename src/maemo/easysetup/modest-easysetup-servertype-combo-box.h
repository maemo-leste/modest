/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _EASYSETUP_SERVERTYPE_COMBO_BOX
#define _EASYSETUP_SERVERTYPE_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include "modest-protocol-info.h"

G_BEGIN_DECLS

#define EASYSETUP_TYPE_SERVERTYPE_COMBO_BOX easysetup_servertype_combo_box_get_type()

#define EASYSETUP_SERVERTYPE_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	EASYSETUP_TYPE_SERVERTYPE_COMBO_BOX, EasysetupServertypeComboBox))

#define EASYSETUP_SERVERTYPE_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	EASYSETUP_TYPE_SERVERTYPE_COMBO_BOX, EasysetupServertypeComboBoxClass))

#define EASYSETUP_IS_SERVERTYPE_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	EASYSETUP_TYPE_SERVERTYPE_COMBO_BOX))

#define EASYSETUP_IS_SERVERTYPE_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	EASYSETUP_TYPE_SERVERTYPE_COMBO_BOX))

#define EASYSETUP_SERVERTYPE_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	EASYSETUP_TYPE_SERVERTYPE_COMBO_BOX, EasysetupServertypeComboBoxClass))

typedef struct {
	GtkComboBox parent;
} EasysetupServertypeComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} EasysetupServertypeComboBoxClass;

GType easysetup_servertype_combo_box_get_type (void);

EasysetupServertypeComboBox* easysetup_servertype_combo_box_new (void);

ModestTransportStoreProtocol easysetup_servertype_combo_box_get_active_servertype (EasysetupServertypeComboBox *combobox);

gboolean easysetup_servertype_combo_box_set_active_servertype (EasysetupServertypeComboBox *combobox, ModestTransportStoreProtocol servertype);


G_END_DECLS

#endif /* _EASYSETUP_PROVIDER_COMBO_BOX */

/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _EASYSETUP_SERVERSECURITY_COMBO_BOX
#define _EASYSETUP_SERVERSECURITY_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include "modest-protocol-info.h"

G_BEGIN_DECLS

#define EASYSETUP_TYPE_SERVERSECURITY_COMBO_BOX easysetup_serversecurity_combo_box_get_type()

#define EASYSETUP_SERVERSECURITY_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	EASYSETUP_TYPE_SERVERSECURITY_COMBO_BOX, EasysetupServersecurityComboBox))

#define EASYSETUP_SERVERSECURITY_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	EASYSETUP_TYPE_SERVERSECURITY_COMBO_BOX, EasysetupServersecurityComboBoxClass))

#define EASYSETUP_IS_SERVERSECURITY_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	EASYSETUP_TYPE_SERVERSECURITY_COMBO_BOX))

#define EASYSETUP_IS_SERVERSECURITY_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	EASYSETUP_TYPE_SERVERSECURITY_COMBO_BOX))

#define EASYSETUP_SERVERSECURITY_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	EASYSETUP_TYPE_SERVERSECURITY_COMBO_BOX, EasysetupServersecurityComboBoxClass))

typedef struct {
	GtkComboBox parent;
} EasysetupServersecurityComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} EasysetupServersecurityComboBoxClass;

GType easysetup_serversecurity_combo_box_get_type (void);

EasysetupServersecurityComboBox* easysetup_serversecurity_combo_box_new (void);

void easysetup_serversecurity_combo_box_fill (EasysetupServersecurityComboBox *combobox, ModestProtocol protocol);

ModestProtocol easysetup_serversecurity_combo_box_get_active_serversecurity (EasysetupServersecurityComboBox *combobox);

gboolean easysetup_serversecurity_combo_box_set_active_serversecurity (EasysetupServersecurityComboBox *combobox, ModestProtocol serversecurity);


G_END_DECLS

#endif /* _EASYSETUP_PROVIDER_COMBO_BOX */

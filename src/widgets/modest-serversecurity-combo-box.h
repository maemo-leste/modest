/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _MODEST_SERVERSECURITY_COMBO_BOX
#define _MODEST_SERVERSECURITY_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include "modest-protocol-registry.h"

G_BEGIN_DECLS

#define MODEST_TYPE_SERVERSECURITY_COMBO_BOX modest_serversecurity_combo_box_get_type()

#define MODEST_SERVERSECURITY_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_SERVERSECURITY_COMBO_BOX, ModestServersecurityComboBox))

#define MODEST_SERVERSECURITY_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_SERVERSECURITY_COMBO_BOX, ModestServersecurityComboBoxClass))

#define EASYSETUP_IS_SERVERSECURITY_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_SERVERSECURITY_COMBO_BOX))

#define EASYSETUP_IS_SERVERSECURITY_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_SERVERSECURITY_COMBO_BOX))

#define MODEST_SERVERSECURITY_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_SERVERSECURITY_COMBO_BOX, ModestServersecurityComboBoxClass))

typedef struct {
	GtkComboBox parent;
} ModestServersecurityComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} ModestServersecurityComboBoxClass;

GType modest_serversecurity_combo_box_get_type (void);

ModestServersecurityComboBox* modest_serversecurity_combo_box_new (void);

void modest_serversecurity_combo_box_fill (ModestServersecurityComboBox *combobox, ModestProtocolType protocol);

ModestProtocolType modest_serversecurity_combo_box_get_active_serversecurity (ModestServersecurityComboBox *combobox);

gboolean modest_serversecurity_combo_box_set_active_serversecurity (ModestServersecurityComboBox *combobox,
								    ModestProtocolType serversecurity);

gint modest_serversecurity_combo_box_get_active_serversecurity_port (ModestServersecurityComboBox *combobox);


G_END_DECLS

#endif /* _EASYSETUP_PROVIDER_COMBO_BOX */

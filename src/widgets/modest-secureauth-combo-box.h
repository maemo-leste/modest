/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _MODEST_SECUREAUTH_COMBO_BOX
#define _MODEST_SECUREAUTH_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include "modest-protocol-info.h"

G_BEGIN_DECLS

#define MODEST_TYPE_SECUREAUTH_COMBO_BOX modest_secureauth_combo_box_get_type()

#define MODEST_SECUREAUTH_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_SECUREAUTH_COMBO_BOX, ModestSecureauthComboBox))

#define MODEST_SECUREAUTH_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_SECUREAUTH_COMBO_BOX, ModestSecureauthComboBoxClass))

#define EASYSETUP_IS_SECUREAUTH_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_SECUREAUTH_COMBO_BOX))

#define EASYSETUP_IS_SECUREAUTH_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_SECUREAUTH_COMBO_BOX))

#define MODEST_SECUREAUTH_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_SECUREAUTH_COMBO_BOX, ModestSecureauthComboBoxClass))

typedef struct {
	GtkComboBox parent;
} ModestSecureauthComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} ModestSecureauthComboBoxClass;

GType modest_secureauth_combo_box_get_type (void);

ModestSecureauthComboBox* modest_secureauth_combo_box_new (void);

ModestProtocol modest_secureauth_combo_box_get_active_secureauth (ModestSecureauthComboBox *combobox);

gboolean modest_secureauth_combo_box_set_active_secureauth (ModestSecureauthComboBox *combobox, ModestProtocol secureauth);


G_END_DECLS

#endif /* _EASYSETUP_PROVIDER_COMBO_BOX */

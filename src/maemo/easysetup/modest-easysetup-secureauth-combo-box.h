/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _EASYSETUP_SECUREAUTH_COMBO_BOX
#define _EASYSETUP_SECUREAUTH_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include "modest-account-mgr/modest-protocol-info.h"

G_BEGIN_DECLS

#define EASYSETUP_TYPE_SECUREAUTH_COMBO_BOX easysetup_secureauth_combo_box_get_type()

#define EASYSETUP_SECUREAUTH_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	EASYSETUP_TYPE_SECUREAUTH_COMBO_BOX, EasysetupSecureauthComboBox))

#define EASYSETUP_SECUREAUTH_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	EASYSETUP_TYPE_SECUREAUTH_COMBO_BOX, EasysetupSecureauthComboBoxClass))

#define EASYSETUP_IS_SECUREAUTH_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	EASYSETUP_TYPE_SECUREAUTH_COMBO_BOX))

#define EASYSETUP_IS_SECUREAUTH_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	EASYSETUP_TYPE_SECUREAUTH_COMBO_BOX))

#define EASYSETUP_SECUREAUTH_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	EASYSETUP_TYPE_SECUREAUTH_COMBO_BOX, EasysetupSecureauthComboBoxClass))

typedef struct {
	GtkComboBox parent;
} EasysetupSecureauthComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} EasysetupSecureauthComboBoxClass;

GType easysetup_secureauth_combo_box_get_type (void);

EasysetupSecureauthComboBox* easysetup_secureauth_combo_box_new (void);

ModestProtocol easysetup_secureauth_combo_box_get_active_secureauth (EasysetupSecureauthComboBox *combobox);

gboolean easysetup_secureauth_combo_box_set_active_secureauth (EasysetupSecureauthComboBox *combobox, ModestProtocol secureauth);


G_END_DECLS

#endif /* _EASYSETUP_PROVIDER_COMBO_BOX */

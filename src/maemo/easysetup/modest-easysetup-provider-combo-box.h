/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _EASYSETUP_PROVIDER_COMBO_BOX
#define _EASYSETUP_PROVIDER_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include "modest-presets.h"

G_BEGIN_DECLS

#define EASYSETUP_TYPE_PROVIDER_COMBO_BOX easysetup_provider_combo_box_get_type()

#define EASYSETUP_PROVIDER_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	EASYSETUP_TYPE_PROVIDER_COMBO_BOX, EasysetupProviderComboBox))

#define EASYSETUP_PROVIDER_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	EASYSETUP_TYPE_PROVIDER_COMBO_BOX, EasysetupProviderComboBoxClass))

#define EASYSETUP_IS_PROVIDER_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	EASYSETUP_TYPE_PROVIDER_COMBO_BOX))

#define EASYSETUP_IS_PROVIDER_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	EASYSETUP_TYPE_PROVIDER_COMBO_BOX))

#define EASYSETUP_PROVIDER_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	EASYSETUP_TYPE_PROVIDER_COMBO_BOX, EasysetupProviderComboBoxClass))

typedef struct {
	GtkComboBox parent;
} EasysetupProviderComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} EasysetupProviderComboBoxClass;

GType easysetup_provider_combo_box_get_type (void);

EasysetupProviderComboBox* easysetup_provider_combo_box_new (void);

void easysetup_provider_combo_box_fill (EasysetupProviderComboBox *combobox, ModestPresets *presets, gint country_id);

gchar* easysetup_provider_combo_box_get_active_provider_id (EasysetupProviderComboBox *combobox);

G_END_DECLS

#endif /* _EASYSETUP_PROVIDER_COMBO_BOX */

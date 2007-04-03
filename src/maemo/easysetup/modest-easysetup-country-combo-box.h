/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _EASYSETUP_COUNTRY_COMBO_BOX
#define _EASYSETUP_COUNTRY_COMBO_BOX

#include <gtk/gtkcombobox.h>

G_BEGIN_DECLS

#define EASYSETUP_TYPE_COUNTRY_COMBO_BOX easysetup_country_combo_box_get_type()

#define EASYSETUP_COUNTRY_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	EASYSETUP_TYPE_COUNTRY_COMBO_BOX, EasysetupCountryComboBox))

#define EASYSETUP_COUNTRY_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	EASYSETUP_TYPE_COUNTRY_COMBO_BOX, EasysetupCountryComboBoxClass))

#define EASYSETUP_IS_COUNTRY_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	EASYSETUP_TYPE_COUNTRY_COMBO_BOX))

#define EASYSETUP_IS_COUNTRY_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	EASYSETUP_TYPE_COUNTRY_COMBO_BOX))

#define EASYSETUP_COUNTRY_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	EASYSETUP_TYPE_COUNTRY_COMBO_BOX, EasysetupCountryComboBoxClass))

typedef struct {
	GtkComboBox parent;
} EasysetupCountryComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} EasysetupCountryComboBoxClass;

GType easysetup_country_combo_box_get_type (void);

EasysetupCountryComboBox* easysetup_country_combo_box_new (void);

guint easysetup_country_combo_box_get_active_country_id (EasysetupCountryComboBox *self);
gboolean easysetup_country_combo_box_set_active_country_id (EasysetupCountryComboBox *self, guint mcc_id);

G_END_DECLS

#endif /* _EASYSETUP_COUNTRY_COMBO_BOX */

/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

GSList* easysetup_country_combo_box_get_active_country_ids (EasysetupCountryComboBox *self);
gboolean easysetup_country_combo_box_set_active_country_id (EasysetupCountryComboBox *self, guint mcc_id);

G_END_DECLS

#endif /* _EASYSETUP_COUNTRY_COMBO_BOX */

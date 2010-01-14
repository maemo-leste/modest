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

#ifndef _MODEST_PROVIDER_COMBO_BOX
#define _MODEST_PROVIDER_COMBO_BOX

#include <gtk/gtkcombobox.h>
#include <modest-presets.h>

G_BEGIN_DECLS

#define MODEST_TYPE_PROVIDER_COMBO_BOX modest_provider_combo_box_get_type()

#define MODEST_PROVIDER_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_PROVIDER_COMBO_BOX, ModestProviderComboBox))

#define MODEST_PROVIDER_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_PROVIDER_COMBO_BOX, ModestProviderComboBoxClass))

#define MODEST_IS_PROVIDER_COMBO_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_PROVIDER_COMBO_BOX))

#define MODEST_IS_PROVIDER_COMBO_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_PROVIDER_COMBO_BOX))

#define MODEST_PROVIDER_COMBO_BOX_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_PROVIDER_COMBO_BOX, ModestProviderComboBoxClass))

/** The type of the ID
 *
 * this means the value of returned by get_active_provider_id will be
 * different depending on the value returned by get_active_id_type
 *
 * If the selected option is a provider then the ID will be the provider ID
 * If the selected option is "Other..." the the ID will be 0
 * If the selected option is a singleton provider protocol then the ID will be protocol name
 **/
typedef enum {
	MODEST_PROVIDER_COMBO_BOX_ID_PROVIDER,
	MODEST_PROVIDER_COMBO_BOX_ID_OTHER,
	MODEST_PROVIDER_COMBO_BOX_ID_PLUGIN_PROTOCOL
} ModestProviderComboBoxIdType;

typedef struct {
	GtkComboBox parent;
} ModestProviderComboBox;

typedef struct {
	GtkComboBoxClass parent_class;
} ModestProviderComboBoxClass;

GType modest_provider_combo_box_get_type (void);

ModestProviderComboBox* modest_provider_combo_box_new (void);

void modest_provider_combo_box_fill (ModestProviderComboBox *combobox, ModestPresets *presets,
				     gint mcc);

void modest_provider_combo_box_refresh (ModestProviderComboBox *combobox);

gchar* modest_provider_combo_box_get_active_provider_id (ModestProviderComboBox *combobox);

gchar* modest_provider_combo_box_get_active_provider_label (ModestProviderComboBox *combobox);

ModestProviderComboBoxIdType modest_provider_combo_box_get_active_id_type (ModestProviderComboBox *combobox);

void  modest_provider_combo_box_set_others_provider (ModestProviderComboBox *self);


G_END_DECLS

#endif /* _MODEST_PROVIDER_COMBO_BOX */

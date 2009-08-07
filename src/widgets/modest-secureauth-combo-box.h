/*
 * Copyright (C) 2007 Nokia Corporation, all rights reserved.
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

#ifndef _MODEST_SECUREAUTH_COMBO_BOX
#define _MODEST_SECUREAUTH_COMBO_BOX

#include <gtk/gtk.h>
#include "modest-protocol-registry.h"

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

ModestProtocolType modest_secureauth_combo_box_get_active_secureauth (ModestSecureauthComboBox *combobox);

gboolean modest_secureauth_combo_box_set_active_secureauth (ModestSecureauthComboBox *combobox,
							    ModestProtocolType secureauth);


G_END_DECLS

#endif /* _EASYSETUP_PROVIDER_COMBO_BOX */

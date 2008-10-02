/* Copyright (c) 2007, 2008, Nokia Corporation
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

#ifndef _MODEST_RETRIEVE_PICKER
#define _MODEST_RETRIEVE_PICKER

#include <hildon/hildon-picker-button.h>
#include "modest-protocol-registry.h"
#include <modest-account-settings.h>

G_BEGIN_DECLS

#define MODEST_TYPE_RETRIEVE_PICKER modest_retrieve_picker_get_type()

#define MODEST_RETRIEVE_PICKER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_RETRIEVE_PICKER, ModestRetrievePicker))

#define MODEST_RETRIEVE_PICKER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_RETRIEVE_PICKER, ModestRetrievePickerClass))

#define MODEST_IS_RETRIEVE_PICKER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_RETRIEVE_PICKER))

#define MODEST_IS_RETRIEVE_PICKER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_RETRIEVE_PICKER))

#define MODEST_RETRIEVE_PICKER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_RETRIEVE_PICKER, ModestRetrievePickerClass))

typedef struct {
	HildonPickerButton parent;
} ModestRetrievePicker;

typedef struct {
	HildonPickerButtonClass parent_class;
} ModestRetrievePickerClass;

GType modest_retrieve_picker_get_type (void);

ModestRetrievePicker* modest_retrieve_picker_new (void);

void modest_retrieve_picker_fill (ModestRetrievePicker *picker, ModestProtocolType protocol);

ModestAccountRetrieveType modest_retrieve_picker_get_active_retrieve_conf (ModestRetrievePicker *picker);

gboolean modest_retrieve_picker_set_active_retrieve_conf (ModestRetrievePicker *picker, 
							  ModestAccountRetrieveType retrieve_type);


G_END_DECLS

#endif /* _MODEST_RETRIEVE_PICKER */

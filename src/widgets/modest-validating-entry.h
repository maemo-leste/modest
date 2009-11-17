/* Copyright (c) 2007, Nokia Corporation
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

#ifndef _MODEST_VALIDATING_ENTRY
#define _MODEST_VALIDATING_ENTRY

#ifndef MODEST_TOOLKIT_HILDON2
#define USE_GTK_ENTRY
#endif

#ifndef USE_GTK_ENTRY
#include <hildon/hildon-entry.h>
#else
#include <gtk/gtk.h>
#endif

G_BEGIN_DECLS

#define MODEST_TYPE_VALIDATING_ENTRY modest_validating_entry_get_type()

#define MODEST_VALIDATING_ENTRY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_VALIDATING_ENTRY, ModestValidatingEntry))

#define MODEST_VALIDATING_ENTRY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_VALIDATING_ENTRY, ModestValidatingEntryClass))

#define EASYSETUP_IS_VALIDATING_ENTRY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_VALIDATING_ENTRY))

#define EASYSETUP_IS_VALIDATING_ENTRY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_VALIDATING_ENTRY))

#define MODEST_VALIDATING_ENTRY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_VALIDATING_ENTRY, ModestValidatingEntryClass))

typedef struct {
#ifndef USE_GTK_ENTRY
	HildonEntry parent;
#else
	GtkEntry parent;
#endif
} ModestValidatingEntry;

typedef struct {
#ifndef USE_GTK_ENTRY
	HildonEntryClass parent_class;
#else
	GtkEntryClass parent_class;
#endif
} ModestValidatingEntryClass;

GType modest_validating_entry_get_type (void);

ModestValidatingEntry* modest_validating_entry_new (void);

void modest_validating_entry_set_unallowed_characters (ModestValidatingEntry *self, GList *list);
void modest_validating_entry_set_unallowed_characters_whitespace (ModestValidatingEntry *self);

typedef void (* EasySetupValidatingEntryFunc) (ModestValidatingEntry *self, const gchar* character, gpointer user_data);
void modest_validating_entry_set_func (ModestValidatingEntry *self, EasySetupValidatingEntryFunc func, gpointer user_data);

typedef void (* EasySetupValidatingEntryMaxFunc) (ModestValidatingEntry *self, gpointer user_data);
void modest_validating_entry_set_max_func (ModestValidatingEntry *self, EasySetupValidatingEntryMaxFunc func, gpointer user_data);

G_END_DECLS

#endif /* _MODEST_VALIDATING_ENTRY */

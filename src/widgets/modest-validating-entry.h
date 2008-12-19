/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _MODEST_VALIDATING_ENTRY
#define _MODEST_VALIDATING_ENTRY

#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-entry.h>
#else
#include <gtk/gtkentry.h>
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
#ifdef MODEST_TOOLKIT_HILDON2
	HildonEntry parent;
#else
	GtkEntry parent;
#endif
} ModestValidatingEntry;

typedef struct {
#ifdef MODEST_TOOLKIT_HILDON2
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

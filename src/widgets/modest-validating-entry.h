/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _EASYSETUP_VALIDATING_ENTRY
#define _EASYSETUP_VALIDATING_ENTRY

#include <gtk/gtkentry.h>

G_BEGIN_DECLS

#define EASYSETUP_TYPE_VALIDATING_ENTRY easysetup_validating_entry_get_type()

#define EASYSETUP_VALIDATING_ENTRY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	EASYSETUP_TYPE_VALIDATING_ENTRY, EasysetupValidatingEntry))

#define EASYSETUP_VALIDATING_ENTRY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	EASYSETUP_TYPE_VALIDATING_ENTRY, EasysetupValidatingEntryClass))

#define EASYSETUP_IS_VALIDATING_ENTRY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	EASYSETUP_TYPE_VALIDATING_ENTRY))

#define EASYSETUP_IS_VALIDATING_ENTRY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	EASYSETUP_TYPE_VALIDATING_ENTRY))

#define EASYSETUP_VALIDATING_ENTRY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	EASYSETUP_TYPE_VALIDATING_ENTRY, EasysetupValidatingEntryClass))

typedef struct {
	GtkEntry parent;
} EasysetupValidatingEntry;

typedef struct {
	GtkEntryClass parent_class;
} EasysetupValidatingEntryClass;

GType easysetup_validating_entry_get_type (void);

EasysetupValidatingEntry* easysetup_validating_entry_new (void);

void easysetup_validating_entry_set_unallowed_characters (EasysetupValidatingEntry *self, GList *list);
void easysetup_validating_entry_set_unallowed_characters_whitespace (EasysetupValidatingEntry *self);

typedef void (* EasySetupValidatingEntryMaxFunc) (EasysetupValidatingEntry *self, gpointer user_data);
void easysetup_validating_entry_set_max_func (EasysetupValidatingEntry *self, EasySetupValidatingEntryMaxFunc func, gpointer user_data);

G_END_DECLS

#endif /* _EASYSETUP_VALIDATING_ENTRY */

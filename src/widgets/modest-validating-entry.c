/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#include "modest-validating-entry.h"
#include <gtk/gtksignal.h> /* For the gtk_signal_stop_emit_by_name() convenience function. */
#include <string.h> /* For strlen(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

G_DEFINE_TYPE (EasysetupValidatingEntry, easysetup_validating_entry, GTK_TYPE_ENTRY);

#define VALIDATING_ENTRY_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), EASYSETUP_TYPE_VALIDATING_ENTRY, EasysetupValidatingEntryPrivate))

typedef struct _EasysetupValidatingEntryPrivate EasysetupValidatingEntryPrivate;

struct _EasysetupValidatingEntryPrivate
{
	/* A list of gunichar, rather than char*,
	 * because gunichar is easier to deal with internally,
	 * but gchar* is easier to supply from the external interface.
	 */
	GList *list_prevent;
	
	gboolean prevent_whitespace;
};

static void
easysetup_validating_entry_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
easysetup_validating_entry_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
easysetup_validating_entry_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (easysetup_validating_entry_parent_class)->dispose)
		G_OBJECT_CLASS (easysetup_validating_entry_parent_class)->dispose (object);
}

static void
easysetup_validating_entry_finalize (GObject *object)
{
	EasysetupValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (object);
	
	/* Free the list and its items: */
	if (priv->list_prevent) {
		g_list_foreach (priv->list_prevent, (GFunc)&g_free, NULL);
		g_list_free (priv->list_prevent);
	}
	
	G_OBJECT_CLASS (easysetup_validating_entry_parent_class)->finalize (object);
}

static void
easysetup_validating_entry_class_init (EasysetupValidatingEntryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (EasysetupValidatingEntryPrivate));

	object_class->get_property = easysetup_validating_entry_get_property;
	object_class->set_property = easysetup_validating_entry_set_property;
	object_class->dispose = easysetup_validating_entry_dispose;
	object_class->finalize = easysetup_validating_entry_finalize;
}

static gint
on_list_compare(gconstpointer a, gconstpointer b)
{
	gunichar* unichar_a = (gunichar*)(a);
	gunichar* unichar_b = (gunichar*)(b);
	if(*unichar_a == *unichar_b)
		return 0;
	else
		return -1; /* Really, we should return > and <, but we don't use this for sorting. */
}
                                             
static void 
on_insert_text(GtkEditable *editable,
	gchar *new_text, gint new_text_length, 
	gint *position,
    gpointer user_data)
{
	EasysetupValidatingEntry *self = EASYSETUP_VALIDATING_ENTRY (user_data);
	EasysetupValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	
	if(!new_text_length)
		return;
		
	/* Note: new_text_length is documented as the number of bytes, not characters. */
	if(!g_utf8_validate (new_text, new_text_length, NULL))
		return;
	
	/* Look at each UTF-8 character in the text (it could be several via a drop or a paste),
	 * and check them */
	gboolean allow = TRUE;
	gchar *iter = new_text; /* new_text seems to be NULL-terminated, though that is not documented. */
	while (iter)
	{
		if(priv->list_prevent) {
			/* If the character is in our prevent list, 
			 * then do not allow this text to be entered.
			 * 
			 * This prevents entry of all text, without removing the unwanted characters.
			 * It is debatable whether that is the best thing to do.
			 */
			gunichar one_char = g_utf8_get_char (iter);
			GList *found = g_list_find_custom(priv->list_prevent, &one_char, &on_list_compare);
			if(found) {
				allow = FALSE;
				break;
			}	
		}
		
		if(priv->prevent_whitespace) {
			/* Check for whitespace characters: */
			gunichar one_char = g_utf8_get_char (iter);
			if (g_unichar_isspace (one_char)) {
				allow = FALSE;
				break;
			}
		}

		/* Crashes. Don't know why: iter = g_utf8_next_char (iter); 
		 * Maybe it doesn't check for null-termination. */	
		iter = g_utf8_find_next_char (iter, new_text + new_text_length);
	}
	
	if(!allow) {
		/* The signal documentation says 
		 * "by connecting to this signal and then stopping the signal with 
		 * gtk_signal_emit_stop(), it is possible to modify the inserted text, 
		 * or prevent it from being inserted entirely."
		 */
		 gtk_signal_emit_stop_by_name (GTK_OBJECT (self), "insert-text");
	}
} 
                                            
static void
easysetup_validating_entry_init (EasysetupValidatingEntry *self)
{
	/* Connect to the GtkEditable::insert-text signal 
	 * so we can filter out some characters:
	 * We connect _before_ so we can stop the default signal handler from running.
	 */
	g_signal_connect (G_OBJECT (self), "insert-text", (GCallback)&on_insert_text, self);
}

EasysetupValidatingEntry*
easysetup_validating_entry_new (void)
{
	return g_object_new (EASYSETUP_TYPE_VALIDATING_ENTRY, NULL);
}

/** Specify characters that may not be entered into this GtkEntry.
 *  
 * list: A list of gchar* strings. Each one identifies a UTF-8 character.
 */
void easysetup_validating_entry_set_unallowed_characters (EasysetupValidatingEntry *self, GList *list)
{
	EasysetupValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	    
	/* Free the list and its items: */	
	if (priv->list_prevent) {
		g_list_foreach (priv->list_prevent, (GFunc)&g_free, NULL);
		g_list_free (priv->list_prevent);
	}
     
    /* Do a deep copy of the list, converting gchar* to gunichar: */
    priv->list_prevent = NULL;
    GList *iter = NULL;               
    for (iter = list; iter != NULL; iter = iter->next) {
    	gunichar *one_char = g_new0 (gunichar, 1);
    	if(iter->data)
    		*one_char = g_utf8_get_char ((gchar*)iter->data);
    	else
    		*one_char = 0;
    		
    	priv->list_prevent = g_list_append (priv->list_prevent, one_char);	
    }
}

/** Specify that no whitespace characters may be entered into this GtkEntry.
 *  
 */
void easysetup_validating_entry_set_unallowed_characters_whitespace (EasysetupValidatingEntry *self)
{
	EasysetupValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	priv->prevent_whitespace = TRUE;
}

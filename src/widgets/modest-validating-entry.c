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

#include "modest-validating-entry.h"
#include <modest-ui-constants.h>
#include <string.h> /* For strlen(). */

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_GTK_ENTRY
G_DEFINE_TYPE (ModestValidatingEntry, modest_validating_entry, GTK_TYPE_ENTRY);
#else
G_DEFINE_TYPE (ModestValidatingEntry, modest_validating_entry, HILDON_TYPE_ENTRY);
#endif

#define VALIDATING_ENTRY_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_VALIDATING_ENTRY, ModestValidatingEntryPrivate))

typedef struct _ModestValidatingEntryPrivate ModestValidatingEntryPrivate;

struct _ModestValidatingEntryPrivate
{
	/* A list of gunichar, rather than char*,
	 * because gunichar is easier to deal with internally,
	 * but gchar* is easier to supply from the external interface.
	 */
	GList *list_prevent;
	
	gboolean prevent_whitespace;
	
	EasySetupValidatingEntryFunc func;
	gpointer func_user_data;

	EasySetupValidatingEntryMaxFunc max_func;
	gpointer max_func_user_data;
};

static void
modest_validating_entry_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_validating_entry_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_validating_entry_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_validating_entry_parent_class)->dispose)
		G_OBJECT_CLASS (modest_validating_entry_parent_class)->dispose (object);
}

static void
modest_validating_entry_finalize (GObject *object)
{
	ModestValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (object);
	
	/* Free the list and its items: */
	if (priv->list_prevent) {
		g_list_foreach (priv->list_prevent, (GFunc)&g_free, NULL);
		g_list_free (priv->list_prevent);
	}
	
	G_OBJECT_CLASS (modest_validating_entry_parent_class)->finalize (object);
}

static void
modest_validating_entry_class_init (ModestValidatingEntryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestValidatingEntryPrivate));

	object_class->get_property = modest_validating_entry_get_property;
	object_class->set_property = modest_validating_entry_set_property;
	object_class->dispose = modest_validating_entry_dispose;
	object_class->finalize = modest_validating_entry_finalize;
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
	ModestValidatingEntry *self = MODEST_VALIDATING_ENTRY (user_data);
	ModestValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	
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
				if (priv->func)
				{
					priv->func(self, iter, priv->func_user_data);
				}
				break;
			}	
		}
		
		if(priv->prevent_whitespace) {
			/* Check for whitespace characters: */
			gunichar one_char = g_utf8_get_char (iter);
			if (g_unichar_isspace (one_char)) {
				allow = FALSE;
				if (priv->func)
				{
					priv->func(self, NULL, priv->func_user_data);
				}
				break;
			}
		}

		/* Crashes. Don't know why: iter = g_utf8_next_char (iter); 
		 * Maybe it doesn't check for null-termination. */	
		iter = g_utf8_find_next_char (iter, new_text + new_text_length);
	}
	
	/* Prevent more than the max characters.
	 * The regular GtkEntry does this already, but we also want to call a specified callback,
	 * so that the application can show a warning dialog. */
	if(priv->max_func) {
	 	const gint max_num = gtk_entry_get_max_length (GTK_ENTRY (self));
	 	if (max_num > 0) {
	 		const gchar *existing_text = gtk_entry_get_text (GTK_ENTRY(self));
	 		const gint existing_length = existing_text ? g_utf8_strlen (existing_text, -1) : 0;
	 		const gint new_length_chars = g_utf8_strlen (new_text, new_text_length);
	 		
	 		if ((existing_length + new_length_chars) > max_num) {
	 			priv->max_func (self, priv->max_func_user_data);
	 			/* We shouldn't need to stop the signal because the underlying code will check too.
	 		 	* Well, that would maybe be a performance optimization, 
	 			 * but it's generally safer not to interfere too much. */	
	 		}
	 	}
	}
	
	if(!allow) {
		/* The signal documentation says: */
		/*   "by connecting to this signal and then stopping the signal with */
		/*   g_signal_stop_emission(), it is possible to modify the inserted text, */
		g_signal_stop_emission_by_name (self, "insert-text");
	}

} 
                                            
static void
modest_validating_entry_init (ModestValidatingEntry *self)
{
	/* Connect to the GtkEditable::insert-text signal 
	 * so we can filter out some characters:
	 * We connect _before_ so we can stop the default signal handler from running.
	 */
	g_signal_connect (G_OBJECT (self), "insert-text", (GCallback)&on_insert_text, self);
}

ModestValidatingEntry*
modest_validating_entry_new (void)
{
	ModestValidatingEntry *entry;
	
	entry = g_object_new (MODEST_TYPE_VALIDATING_ENTRY, NULL);

#ifdef MODEST_TOOLKIT_HILDON2
	hildon_gtk_widget_set_theme_size (GTK_WIDGET (entry), MODEST_EDITABLE_SIZE);
#endif

	return entry;
}

/** Specify characters that may not be entered into this GtkEntry.
 *  
 * list: A list of gchar* strings. Each one identifies a UTF-8 character.
 */
void modest_validating_entry_set_unallowed_characters (ModestValidatingEntry *self, GList *list)
{
	ModestValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	    
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
void modest_validating_entry_set_unallowed_characters_whitespace (ModestValidatingEntry *self)
{
	ModestValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	priv->prevent_whitespace = TRUE;
}

/** Set a callback to be called when the maximum number of characters have been entered.
 * This may be used to show an informative dialog.
 */
void modest_validating_entry_set_max_func (ModestValidatingEntry *self, EasySetupValidatingEntryMaxFunc func, gpointer user_data)
{
	ModestValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	priv->max_func = func;
	priv->max_func_user_data = user_data;
}

/** Set a callback to be called when a character was prevented so that a
 * note can be shown by the application to inform the user. For whitespaces,
 * character will be NULL
 */
void modest_validating_entry_set_func (ModestValidatingEntry *self, EasySetupValidatingEntryFunc func, gpointer user_data)
{
	ModestValidatingEntryPrivate *priv = VALIDATING_ENTRY_GET_PRIVATE (self);
	priv->func = func;
	priv->func_user_data = user_data;
}


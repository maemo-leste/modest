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


#ifndef __MODEST_EMAIL_CLIPBOARD_H__
#define __MODEST_EMAIL_CLIPBOARD_H__

#include <glib-object.h>
#include <modest-conf.h>
#include <modest-defs.h>
#include <tny-folder.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_EMAIL_CLIPBOARD             (modest_email_clipboard_get_type())
#define MODEST_EMAIL_CLIPBOARD(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_EMAIL_CLIPBOARD,ModestEmailClipboard))
#define MODEST_EMAIL_CLIPBOARD_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_EMAIL_CLIPBOARD,ModestEmailClipboardClass))
#define MODEST_IS_EMAIL_CLIPBOARD(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_EMAIL_CLIPBOARD))
#define MODEST_IS_EMAIL_CLIPBOARD_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_EMAIL_CLIPBOARD))
#define MODEST_EMAIL_CLIPBOARD_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_EMAIL_CLIPBOARD,ModestEmailClipboardClass))

typedef struct _ModestEmailClipboard      ModestEmailClipboard;
typedef struct _ModestEmailClipboardClass ModestEmailClipboardClass;

struct _ModestEmailClipboard {
	 GObject parent;
};

struct _ModestEmailClipboardClass {
	GObjectClass parent_class;

};

/**
 * modest_email_clipboard_get_type:
 * 
 * get the GType for #ModestEmailClipboard
 *  
 * Returns: the GType
 */
GType           modest_email_clipboard_get_type       (void) G_GNUC_CONST;


/**
 * modest_email_clipboard_new:
 *  
 * Returns: a new #ModestEmailClipboard, or NULL in case of error
 */
ModestEmailClipboard*        modest_email_clipboard_new (void);


/**
 * modest_email_clipboard_get_data:
 * @self: a #ModestEmailClipboard singlenton instance.   
 * @src_folder: a #TnyFolder instance which is the source of selection data. 
 * @data: a #TnyList of objects to manage.
 * @delete: determines whether data will be removed after copy them.
 *  
 * Gets data from clipboard to manage them with copy, cut and paste operations.
 * Currently imementation allows #TnyFolder or #TnyHeader objects.
 *
 * After getting data, clipboard will be cleared.
 */
void
modest_email_clipboard_get_data (ModestEmailClipboard *self,
				 TnyFolder **src_folder,
				 TnyList **data,
				 gboolean *delete);

/**
 * modest_email_clipboard_set_data:
 * @self: a #ModestEmailClipboard singlenton instance.   
 * @src_folder: a #TnyFolder instance which is the source of selection data. 
 * @data: a #TnyList of objects to manage.
 * @delete: determines whether data will be removed after copy them.
 *  
 * Sets data on clipboard to manage them wiht copy, cut and paste operations.
 * Currently imementation allows #TnyFolder or #TnyHeader objects.
 *
 */
void modest_email_clipboard_set_data (ModestEmailClipboard *self,
				      TnyFolder *src_folder, 
				      TnyList *data,
				      gboolean delete);


/**
 * modest_email_clipboard_clear:
 * @self: a #ModestEmailClipboard singlenton instance.   
 *  
 * Clear all data stored inside clipboard.
 */
void modest_email_clipboard_clear (ModestEmailClipboard *self);


/**
 * modest_email_clipboard_cleared:
 * @self: a #ModestEmailClipboard singlenton instance.   
 * 
 * Determines if clipboard is clreared, no seleciton data is stored.
 *  
 * returns TRUE, if clipboard is cleared, FALSE otherwise.
 */
gboolean modest_email_clipboard_cleared (ModestEmailClipboard *self);


/**
 * modest_email_clipboard_cleared:
 * @self: a #ModestEmailClipboard singlenton instance.   
 * @folder: a #TnyFolder instance to compare.
 * 
 * Determines if source folder stored on clipboard is the 
 * same as @folder, passed as argument. 
 *  
 * returns TRUE, if clipboard is cleared, FALSE otherwise.
 */
gboolean 
modest_email_clipboard_check_source_folder (ModestEmailClipboard *self,
					    const TnyFolder *folder);

/**
 * modest_email_clipboard_set_data:
 * @self: a #ModestEmailClipboard singlenton instance.   
 * @n_selected: the number of items copied and marked to delete.
 * 
 * Returns the string array of item identifiers stored on clipboard.
 * 
 * returns TRUE, if clipboard is cleared, FALSE otherwise.
 */
const gchar **modest_email_clipboard_get_hidding_ids (ModestEmailClipboard *self,
						      guint *n_selected);

G_END_DECLS

#endif /* __MODEST_EMAIL_CLIPBOARD_H__ */

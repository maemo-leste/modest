/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *	 contributors may be used to endorse or promote products derived from
 *	 this software without specific prior written permission.
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

#ifndef __MODEST_DETAILS_DIALOG
#define __MODEST_DETAILS_DIALOG

#include <glib.h>
#include <gtk/gtk.h>
#include <tny-header.h>


G_BEGIN_DECLS

#define MODEST_TYPE_DETAILS_DIALOG modest_details_dialog_get_type()

#define MODEST_DETAILS_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_DETAILS_DIALOG, ModestDetailsDialog))

#define MODEST_DETAILS_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_DETAILS_DIALOG, ModestDetailsDialogClass))

#define MODEST_IS_DETAILS_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_DETAILS_DIALOG))

#define MODEST_IS_DETAILS_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_DETAILS_DIALOG))

#define MODEST_DETAILS_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_DETAILS_DIALOG, ModestDetailsDialogClass))

typedef struct {
	GtkDialog parent;
	
} ModestDetailsDialog;

typedef struct {
	GtkDialogClass parent_class;

	/* virtual methods */
	void (*create_container_func) (ModestDetailsDialog *self);
	void (*add_data_func) (ModestDetailsDialog *self, const gchar *label, const gchar *value);
	void (*set_header_func) (ModestDetailsDialog *self, TnyHeader *header, gboolean get_size);
	void (*set_folder_func) (ModestDetailsDialog *self, TnyFolder *folder);
	void (*set_message_size_func) (ModestDetailsDialog *self, guint message_size);

} ModestDetailsDialogClass;

GType modest_details_dialog_get_type (void);

GtkWidget* modest_details_dialog_new_with_header  (GtkWindow *parent, 
						   TnyHeader *header,
						   gboolean get_size);

GtkWidget* modest_details_dialog_new_with_folder  (GtkWindow *parent, 
						   TnyFolder *folder);

void       modest_details_dialog_add_data         (ModestDetailsDialog *self, 
						   const gchar *label, 
						   const gchar *value);

void       modest_details_dialog_set_message_size (ModestDetailsDialog *self,
						   guint message_size);

G_END_DECLS

#endif /* __MODEST_DETAILS_DIALOG */

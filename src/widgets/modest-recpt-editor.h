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

#ifndef MODEST_RECPT_EDITOR_H
#define MODEST_RECPT_EDITOR_H
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODEST_TYPE_RECPT_EDITOR             (modest_recpt_editor_get_type ())
#define MODEST_RECPT_EDITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_RECPT_EDITOR, ModestRecptEditor))
#define MODEST_RECPT_EDITOR_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MODEST_TYPE_RECPT_EDITOR, ModestRecptEditorClass))
#define MODEST_IS_RECPT_EDITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_RECPT_EDITOR))
#define MODEST_IS_RECPT_EDITOR_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MODEST_TYPE_RECPT_EDITOR))
#define MODEST_RECPT_EDITOR_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MODEST_TYPE_RECPT_EDITOR, ModestRecptEditorClass))

typedef struct _ModestRecptEditor ModestRecptEditor;
typedef struct _ModestRecptEditorClass ModestRecptEditorClass;

struct _ModestRecptEditor
{
	GtkHBox parent;

};

struct _ModestRecptEditorClass
{
	GtkHBoxClass parent_class;

	/* signals */

	void (*open_addressbook) (ModestRecptEditor *editor);
};

GType modest_recpt_editor_get_type (void);

GtkWidget* modest_recpt_editor_new (void);

void modest_recpt_editor_set_recipients (ModestRecptEditor *recpt_editor, const gchar *recipients);
const gchar *modest_recpt_editor_get_recipients (ModestRecptEditor *repct_editor);
void modest_recpt_editor_add_recipients (ModestRecptEditor *recpt_editor, const gchar *recipients);
void modest_recpt_editor_add_resolved_recipient (ModestRecptEditor *recpt_editor, 
						 GSList *email_list, 
						 const gchar * recipient_id);
void modest_recpt_editor_replace_with_resolved_recipient (ModestRecptEditor *recpt_editor, 
							  GtkTextIter *start, GtkTextIter *end,
							  GSList *email_list, 
							  const gchar *recipient_id);

void modest_recpt_editor_replace_with_resolved_recipients (ModestRecptEditor *recpt_editor, 
							   GtkTextIter *start, GtkTextIter *end,
							   GSList *email_lists_list, 
							   GSList *recipient_ids_list);

void modest_recpt_editor_set_field_size_group (ModestRecptEditor *recpt_editor, GtkSizeGroup *size_group);
GtkTextBuffer *modest_recpt_editor_get_buffer (ModestRecptEditor *recpt_editor);
void modest_recpt_editor_grab_focus (ModestRecptEditor *recpt_editor);
gboolean modest_recpt_editor_has_focus (ModestRecptEditor *recpt_editor);
void modest_recpt_editor_set_show_abook_button (ModestRecptEditor *recpt_editor, gboolean show);
gboolean modest_recpt_editor_get_show_abook_button (ModestRecptEditor *recpt_editor, gboolean show);

G_END_DECLS

#endif

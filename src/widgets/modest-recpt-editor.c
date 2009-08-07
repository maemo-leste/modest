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

#include <config.h>

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include <modest-text-utils.h>
#include <modest-recpt-editor.h>
#include <modest-scroll-text.h>
#include <pango/pango-attributes.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <modest-ui-constants.h>

/* FIXNE: we should have no maemo-deps in widgets/ */
#ifndef MODEST_TOOLKIT_GTK
#include "modest-hildon-includes.h"
#endif /*!MODEST_TOOLKIT_GTK*/

#define RECPT_BUTTON_WIDTH_HILDON2 118


static GObjectClass *parent_class = NULL;

#define RECIPIENT_TAG_ID "recpt-id"

/* signals */
enum {
	OPEN_ADDRESSBOOK_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestRecptEditorPrivate ModestRecptEditorPrivate;

struct _ModestRecptEditorPrivate
{
	GtkWidget *text_view;
	GtkWidget *abook_button;
	GtkWidget *scrolled_window;
	gchar *recipients;
	gulong on_mark_set_handler;
	gboolean show_abook;
};

#define MODEST_RECPT_EDITOR_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_RECPT_EDITOR, ModestRecptEditorPrivate))

static guint signals[LAST_SIGNAL] = {0};

/* static functions: GObject */
static void modest_recpt_editor_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_recpt_editor_finalize (GObject *object);
static void modest_recpt_editor_class_init (ModestRecptEditorClass *klass);

/* widget events */
static void modest_recpt_editor_on_abook_clicked (GtkButton *button,
						  ModestRecptEditor *editor);
static void modest_recpt_editor_add_tags (ModestRecptEditor *editor,
					  const gchar * recipient_id);
static gboolean modest_recpt_editor_on_focus_in (GtkTextView *text_view,
					     GdkEventFocus *event,
					     ModestRecptEditor *editor);
static void modest_recpt_editor_on_mark_set (GtkTextBuffer *buffer,
					     GtkTextIter *iter,
					     GtkTextMark *mark,
					     ModestRecptEditor *editor);
static void modest_recpt_editor_on_insert_text (GtkTextBuffer *buffer,
						GtkTextIter *location,
						gchar *text,
						gint len,
						ModestRecptEditor *editor);
static void modest_recpt_editor_on_insert_text_after (GtkTextBuffer *buffer,
						      GtkTextIter *location,
						      gchar *text,
						      gint len,
						      ModestRecptEditor *editor);
static gboolean modest_recpt_editor_on_key_press_event (GtkTextView *text_view,
							  GdkEventKey *key,
							  ModestRecptEditor *editor);
static GtkTextTag *iter_has_recipient (GtkTextIter *iter);
static gunichar iter_previous_char (GtkTextIter *iter);
/* static gunichar iter_next_char (GtkTextIter *iter); */
static GtkTextTag *prev_iter_has_recipient (GtkTextIter *iter);
/* static GtkTextTag *next_iter_has_recipient (GtkTextIter *iter); */
static void select_tag_of_iter (GtkTextIter *iter, GtkTextTag *tag, gboolean grow, gboolean left_not_right);
static gboolean quote_opened (GtkTextIter *iter);
static gboolean is_valid_insert (const gchar *text, gint len);
static gchar *create_valid_text (const gchar *text, gint len);

/**
 * modest_recpt_editor_new:
 *
 * Return value: a new #ModestRecptEditor instance implemented for Gtk+
 **/
GtkWidget*
modest_recpt_editor_new (void)
{
	ModestRecptEditor *self = g_object_new (MODEST_TYPE_RECPT_EDITOR, 
						"homogeneous", FALSE,
						"spacing", MODEST_MARGIN_NONE,
						NULL);

	return GTK_WIDGET (self);
}

void
modest_recpt_editor_set_recipients (ModestRecptEditor *recpt_editor, const gchar *recipients)
{
	ModestRecptEditorPrivate *priv;
	GtkTextBuffer *buffer = NULL;
	gchar *valid_recipients = NULL;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

#ifdef MODEST_TOOLKIT_HILDON2
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif

	valid_recipients = create_valid_text (recipients, -1);
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);
	gtk_text_buffer_set_text (buffer, valid_recipients, -1);
	g_free (valid_recipients);
	if (GTK_WIDGET_REALIZED (recpt_editor))
		gtk_widget_queue_resize (GTK_WIDGET (recpt_editor));
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);

}

void
modest_recpt_editor_add_recipients (ModestRecptEditor *recpt_editor, const gchar *recipients)
{
	ModestRecptEditorPrivate *priv;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;
	gchar *string_to_add;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	if (recipients == NULL)
		return;

#ifdef MODEST_TOOLKIT_HILDON2
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif

	if (gtk_text_buffer_get_char_count (buffer) > 0) {
		string_to_add = g_strconcat (";\n", recipients, NULL);
	} else {
		string_to_add = g_strdup (recipients);
	}

	gtk_text_buffer_get_end_iter (buffer, &iter);

	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);

	gtk_text_buffer_insert (buffer, &iter, string_to_add, -1);

	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);

	if (GTK_WIDGET_REALIZED (recpt_editor))
		gtk_widget_queue_resize (GTK_WIDGET (recpt_editor));

	g_free (string_to_add);
}

void 
modest_recpt_editor_add_resolved_recipient (ModestRecptEditor *recpt_editor, GSList *email_list, const gchar * recipient_id)
{
	ModestRecptEditorPrivate *priv;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end, iter;
	GSList *node;
	gboolean is_first_recipient = TRUE;
      
	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

#ifdef MODEST_TOOLKIT_HILDON2
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif

	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	if (gtk_text_buffer_get_char_count (buffer) > 0) {
		gchar * buffer_contents;

		gtk_text_buffer_get_bounds (buffer, &start, &end);
		buffer_contents = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
		g_strstrip (buffer_contents);
		if (!g_str_has_suffix (buffer_contents, "\n")) {
			if (g_str_has_suffix (buffer_contents, ";")||(g_str_has_suffix (buffer_contents, ",")))
				gtk_text_buffer_insert (buffer, &end, "\n", -1);
			else
				gtk_text_buffer_insert (buffer, &end, ";\n", -1);
		}
		g_free (buffer_contents);
	}

	gtk_text_buffer_get_end_iter (buffer, &iter);

	for (node = email_list; node != NULL; node = g_slist_next (node)) {
		gchar *recipient = (gchar *) node->data;

		if ((recipient) && (strlen (recipient) != 0)) {

			if (!is_first_recipient)
				gtk_text_buffer_insert (buffer, &iter, "\n", -1);

			gtk_text_buffer_insert (buffer, &iter, recipient, -1);
			gtk_text_buffer_insert (buffer, &iter, ";", -1);
			is_first_recipient = FALSE;
		}
	}
	modest_recpt_editor_add_tags (recpt_editor, recipient_id);

	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);
}

void 
modest_recpt_editor_replace_with_resolved_recipient (ModestRecptEditor *recpt_editor, 
						     GtkTextIter *start, GtkTextIter *end,
						     GSList *email_list, const gchar * recipient_id)
{
	GSList *email_lists_list;
	GSList *recipient_ids_list;

	email_lists_list = g_slist_append (NULL, email_list);
	recipient_ids_list = g_slist_append (NULL, (gpointer) recipient_id);

	modest_recpt_editor_replace_with_resolved_recipients (recpt_editor, start, end,
							      email_lists_list, recipient_ids_list);

	g_slist_free (email_lists_list);
	g_slist_free (recipient_ids_list);

}

void 
modest_recpt_editor_replace_with_resolved_recipients (ModestRecptEditor *recpt_editor, 
						     GtkTextIter *start, GtkTextIter *end,
						     GSList *email_lists_list, GSList * recipient_ids_list)
{
	ModestRecptEditorPrivate *priv;
	GtkTextBuffer *buffer;
	GtkTextTag *tag;
	GSList *node;
	gboolean is_first_recipient = TRUE;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

#ifdef MODEST_TOOLKIT_HILDON2
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);

	gtk_text_buffer_delete (buffer, start, end);

	while (email_lists_list && recipient_ids_list) {
		gchar *recipient_id = (gchar *) recipient_ids_list->data;
		GSList *email_list = (GSList *) email_lists_list->data;

		tag = gtk_text_buffer_create_tag (buffer, NULL, 
						  "underline", PANGO_UNDERLINE_SINGLE,
						  "wrap-mode", GTK_WRAP_NONE,
						  "editable", TRUE, NULL);

		g_object_set_data (G_OBJECT (tag), "recipient-tag-id", GINT_TO_POINTER (RECIPIENT_TAG_ID));
		g_object_set_data_full (G_OBJECT (tag), "recipient-id", g_strdup (recipient_id), (GDestroyNotify) g_free);

		for (node = email_list; node != NULL; node = g_slist_next (node)) {
			gchar *recipient = (gchar *) node->data;

			if ((recipient) && (strlen (recipient) != 0)) {

				if (!is_first_recipient || !gtk_text_iter_is_start(start))
					gtk_text_buffer_insert (buffer, start, "\n", -1);

				gtk_text_buffer_insert_with_tags (buffer, start, recipient, -1, tag, NULL);

				if (node->next != NULL)
					gtk_text_buffer_insert (buffer, start, ";", -1);
				is_first_recipient = FALSE;
			}
		}

		email_lists_list = g_slist_next (email_lists_list);
		recipient_ids_list = g_slist_next (recipient_ids_list);

		/* Add a separator between lists of emails*/
		if (recipient_ids_list)
			gtk_text_buffer_insert (buffer, start, ";", -1);
	}
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text, recpt_editor);
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text_after, recpt_editor);

}


const gchar *
modest_recpt_editor_get_recipients (ModestRecptEditor *recpt_editor)
{
	ModestRecptEditorPrivate *priv;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;

	g_return_val_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor), NULL);
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	if (priv->recipients != NULL) {
		g_free (priv->recipients);
		priv->recipients = NULL;
	}

#ifdef MODEST_TOOKIT_HILDON2
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	priv->recipients = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	return priv->recipients;

}

static void
modest_recpt_editor_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestRecptEditorPrivate *priv;
	GtkWidget *abook_icon;
	GtkWidget *abook_align;
	GtkTextBuffer *buffer;

	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (instance);

	priv->show_abook = TRUE;
	priv->abook_button = gtk_button_new ();
	gtk_widget_set_no_show_all (GTK_WIDGET (priv->abook_button), TRUE);
	gtk_widget_show (priv->abook_button);
#ifdef MODEST_TOOLKIT_HILDON2
	gtk_widget_set_size_request (priv->abook_button, RECPT_BUTTON_WIDTH_HILDON2, -1);
#else
	gtk_button_set_relief (GTK_BUTTON (priv->abook_button), GTK_RELIEF_NONE);
#endif
	gtk_button_set_focus_on_click (GTK_BUTTON (priv->abook_button), FALSE);
	GTK_WIDGET_UNSET_FLAGS (priv->abook_button, GTK_CAN_FOCUS);
	gtk_button_set_alignment (GTK_BUTTON (priv->abook_button), 0.5, 0.5);
#ifdef MODEST_TOOLKIT_HILDON2
	abook_icon = gtk_image_new_from_icon_name ("general_contacts", HILDON_ICON_SIZE_FINGER);
#else
	abook_icon = gtk_image_new_from_icon_name ("qgn_list_addressbook", GTK_ICON_SIZE_BUTTON);
#endif
	gtk_container_add (GTK_CONTAINER (priv->abook_button), abook_icon);

#ifdef MODEST_TOOLKIT_HILDON2
	priv->text_view = hildon_text_view_new ();
#else
	priv->text_view = gtk_text_view_new ();
#endif
	/* Auto-capitalization is the default, so let's turn it off: */
#ifdef MAEMO_CHANGES
	hildon_gtk_text_view_set_input_mode (GTK_TEXT_VIEW (priv->text_view), 
		HILDON_GTK_INPUT_MODE_FULL);
#endif
	
	priv->recipients = NULL;

#ifdef MODEST_TOOLKIT_HILDON2
	priv->scrolled_window = NULL;
	gtk_box_pack_start (GTK_BOX (instance), priv->text_view, TRUE, TRUE, 0);
#else
	priv->scrolled_window = modest_scroll_text_new (GTK_TEXT_VIEW (priv->text_view), 1024);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window), GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->scrolled_window), GTK_SHADOW_IN);
/* 	gtk_container_add (GTK_CONTAINER (priv->scrolled_window), priv->text_view); */

	gtk_box_pack_start (GTK_BOX (instance), priv->scrolled_window, TRUE, TRUE, 0);
/* 	gtk_box_pack_start (GTK_BOX (instance), priv->text_view, TRUE, TRUE, 0); */
#endif
	abook_align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (abook_align), 0, 0, MODEST_MARGIN_DEFAULT, 0);
	gtk_container_add (GTK_CONTAINER (abook_align), priv->abook_button);
	gtk_box_pack_end (GTK_BOX (instance), abook_align, FALSE, FALSE, 0);

	gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (priv->text_view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (priv->text_view), TRUE);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->text_view), TRUE);

	gtk_text_view_set_justification (GTK_TEXT_VIEW (priv->text_view), GTK_JUSTIFY_LEFT);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->text_view), GTK_WRAP_CHAR);

	gtk_widget_set_size_request (priv->text_view, 75, -1);

#ifdef MODEST_TOOLKIT_HILDON2
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif
	g_signal_connect (G_OBJECT (priv->abook_button), "clicked", G_CALLBACK (modest_recpt_editor_on_abook_clicked), instance);
	g_signal_connect (G_OBJECT (priv->text_view), "key-press-event", G_CALLBACK (modest_recpt_editor_on_key_press_event), instance);
	g_signal_connect (G_OBJECT (priv->text_view), "focus-in-event", G_CALLBACK (modest_recpt_editor_on_focus_in), instance);
	g_signal_connect (G_OBJECT (buffer), "insert-text",
			  G_CALLBACK (modest_recpt_editor_on_insert_text),
			  instance);
	g_signal_connect_after (G_OBJECT (buffer), "insert-text",
				G_CALLBACK (modest_recpt_editor_on_insert_text_after),
				instance);

	priv->on_mark_set_handler = g_signal_connect (G_OBJECT (buffer), "mark-set", 
						      G_CALLBACK (modest_recpt_editor_on_mark_set), 
						      instance);

	return;
}

void
modest_recpt_editor_set_field_size_group (ModestRecptEditor *recpt_editor, GtkSizeGroup *size_group)
{
	ModestRecptEditorPrivate *priv;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	g_return_if_fail (GTK_IS_SIZE_GROUP (size_group));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

#ifdef MODEST_TOOLKIT_HILDON2
	gtk_size_group_add_widget (size_group, priv->text_view);
#else
	gtk_size_group_add_widget (size_group, priv->scrolled_window);
#endif
}

GtkTextBuffer *
modest_recpt_editor_get_buffer (ModestRecptEditor *recpt_editor)
{
	ModestRecptEditorPrivate *priv;

	g_return_val_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor), NULL);
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

#ifdef MODEST_TOOLKIT_HILDON2
	return hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	return gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif
}

static void
modest_recpt_editor_on_abook_clicked (GtkButton *button, ModestRecptEditor *editor)
{
	g_return_if_fail (MODEST_IS_RECPT_EDITOR (editor));

	g_signal_emit_by_name (G_OBJECT (editor), "open-addressbook");
}

static void
modest_recpt_editor_on_mark_set (GtkTextBuffer *buffer,
				 GtkTextIter *iter,
				 GtkTextMark *mark,
				 ModestRecptEditor *recpt_editor)
{
	ModestRecptEditorPrivate *priv;
	GtkTextIter insert_iter, selection_iter;
	GtkTextMark *selection_bound;
	GtkTextMark *insert;
	GtkTextTag *tag;
	gboolean selection_changed = FALSE;
	gboolean select_to_left;

	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	buffer = modest_recpt_editor_get_buffer (recpt_editor);
	selection_bound = gtk_text_buffer_get_selection_bound (buffer);
	insert = gtk_text_buffer_get_insert (buffer);

	if (mark != selection_bound && mark != insert)
		return;

	gtk_text_buffer_get_iter_at_mark (buffer, &insert_iter, insert);
	gtk_text_buffer_get_iter_at_mark (buffer, &selection_iter, selection_bound);

	select_to_left = gtk_text_iter_get_offset (&selection_iter) > gtk_text_iter_get_offset (&insert_iter);

	tag = iter_has_recipient (&insert_iter);
	if (tag) {
		if (select_to_left) {
			if (!gtk_text_iter_begins_tag (&insert_iter, tag)) {
				gtk_text_iter_backward_to_tag_toggle (&insert_iter, tag);
				selection_changed = TRUE;
			}
		} else {
			gtk_text_iter_forward_to_tag_toggle (&insert_iter, tag);
			selection_changed = TRUE;
		}
	}
		
	tag = iter_has_recipient (&selection_iter);
	if (tag != NULL) {
		if (select_to_left) {
			gtk_text_iter_forward_to_tag_toggle (&selection_iter, tag);
			selection_changed = TRUE;
		} else {
			if (!gtk_text_iter_begins_tag (&selection_iter, tag)) {
				gtk_text_iter_backward_to_tag_toggle (&selection_iter, tag);
				selection_changed = TRUE;
			}
		}
	}
	
	if (selection_changed) {
		/* We block this signal handler in order to prevent a
		   stack overflow caused by recursive calls to this
		   handler as the select_range call could issue a
		   "mark-set" signal */
		g_signal_handler_block (buffer, priv->on_mark_set_handler);
		gtk_text_buffer_select_range (buffer, &insert_iter, &selection_iter);
		g_signal_handler_unblock (buffer, priv->on_mark_set_handler);
	}
}

static gboolean 
modest_recpt_editor_on_focus_in (GtkTextView *text_view,
				 GdkEventFocus *event,
				 ModestRecptEditor *editor)
{
	ModestRecptEditorPrivate *priv = MODEST_RECPT_EDITOR_GET_PRIVATE (editor);
	gtk_text_view_place_cursor_onscreen (GTK_TEXT_VIEW (priv->text_view));

	return FALSE;
}

static gboolean
is_valid_insert (const gchar *text, gint len)
{
	gunichar c;
	gunichar next_c;
	gint i= 0;
	gboolean quoted = FALSE;
	const gchar *current, *next_current;
	if (text == NULL)
		return TRUE;
	current = text;

	while (((len == -1)||(i < len)) && (*current != '\0')) {
		c = g_utf8_get_char (current);
		next_current = g_utf8_next_char (current);
		if (next_current && *next_current != '\0')
			next_c = g_utf8_get_char (g_utf8_next_char (current));
		else
			next_c = 0;
		if (!quoted && ((c == g_utf8_get_char(",") || c == g_utf8_get_char (";")))) {
			if ((next_c != 0) && (next_c != g_utf8_get_char ("\n")))
				return FALSE;
			else {
			  current = g_utf8_next_char (next_current);
			  continue;
			}
		}
		if (c == 0x2022 || c == 0xfffc ||
		    c == g_utf8_get_char ("\n") ||
		    c == g_utf8_get_char ("\t"))
			return FALSE;
		if (c == g_utf8_get_char ("\""))
			quoted = !quoted;
		current = g_utf8_next_char (current);
		i = current - text;
	}
	return TRUE;
}

static gchar *
create_valid_text (const gchar *text, gint len)
{
	gunichar c;
	gunichar next_c;
	gint i= 0;
	GString *str;
	gboolean quoted = FALSE;
	const gchar *current, *next_current;

	if (text == NULL)
		return NULL;

	str = g_string_new ("");
	current = text;

	while (((len == -1)||(i < len)) && (*current != '\0')) {
		c = g_utf8_get_char (current);
		next_current = g_utf8_next_char (current);
		if (next_current && *next_current != '\0')
			next_c = g_utf8_get_char (g_utf8_next_char (current));
		else
			next_c = 0;
		if (c != 0x2022 && c != 0xfffc &&
		    c != g_utf8_get_char ("\n") &&
		    c != g_utf8_get_char ("\t"))
			str = g_string_append_unichar (str, c);
		if (!quoted && ((c == g_utf8_get_char(",") || c == g_utf8_get_char (";")))) {
			if ((next_c != 0) && (next_c != g_utf8_get_char ("\n"))) {
				gchar *last_separator = MAX (g_utf8_strrchr(str->str, -1, g_utf8_get_char (",")),
							     g_utf8_strrchr(str->str, -1, g_utf8_get_char (";")));
				if (last_separator) {
					gchar *last_at = g_utf8_strrchr (str->str, -1, g_utf8_get_char ("@"));
					if (last_at) {
						if (last_at < last_separator)
							str = g_string_append_c (str, '\n');
					}
				} else {
					if (g_utf8_strrchr (str->str, -1, g_utf8_get_char ("@")))
						str = g_string_append_c (str, '\n');
				}
			}
		}
		if (c == g_utf8_get_char ("\""))
			quoted = !quoted;
		current = g_utf8_next_char (current);
		i = current - text;
	}

	return g_string_free (str, FALSE);
}

/* Called after the default handler, and thus after the text was
   inserted. We use this to insert a break after a ',' or a ';'*/
static void
modest_recpt_editor_on_insert_text_after (GtkTextBuffer *buffer,
					  GtkTextIter *location,
					  gchar *text,
					  gint len,
					  ModestRecptEditor *editor)
{
	GtkTextIter prev;
	gunichar prev_char;
	ModestRecptEditorPrivate *priv = MODEST_RECPT_EDITOR_GET_PRIVATE (editor);

	prev = *location;
	/* We must go backwards twice as location points to the next
	   valid position to insert text */
	if (!gtk_text_iter_backward_chars (&prev, 2))
		return;

	prev_char = gtk_text_iter_get_char (&prev);
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text, editor);
	g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text_after, editor);
	if ((prev_char == ';'||prev_char == ',')&&(!quote_opened(&prev))) {
		GtkTextMark *insert;
		gtk_text_iter_forward_char (&prev);
		gtk_text_buffer_insert (buffer, &prev, "\n",-1);
		insert = gtk_text_buffer_get_insert (buffer);
		gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->text_view), &prev, 0.0,TRUE, 0.0, 1.0);
	}
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text, editor);
	g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text_after, editor);
}

/* Called before the default handler, we use it to validate the inputs */
static void
modest_recpt_editor_on_insert_text (GtkTextBuffer *buffer,
				    GtkTextIter *location,
				    gchar *text,
				    gint len,
				    ModestRecptEditor *editor)
{
	if (len > 1024)
		len = 1024;

	if (!is_valid_insert (text, len)) {
		gchar *new_text = create_valid_text (text, len);
		g_signal_stop_emission_by_name (G_OBJECT (buffer), "insert-text");
		g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text, editor);
		g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text_after, editor);
		gtk_text_buffer_insert (buffer, location, new_text, -1);
		g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text, editor);
		g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text_after, editor);
		g_free (new_text);
		return;
	}

	if (iter_has_recipient (location)) {
		gtk_text_buffer_get_end_iter (buffer, location);
		gtk_text_buffer_place_cursor (buffer, location);
	}
}

static GtkTextTag *
iter_has_recipient (GtkTextIter *iter)
{
	GSList *tags, *node;
	GtkTextTag *result = NULL;

	tags = gtk_text_iter_get_tags (iter);

	for (node = tags; node != NULL; node = g_slist_next (node)) {
		GtkTextTag *tag = GTK_TEXT_TAG (node->data);

		if (g_object_get_data (G_OBJECT (tag), "recipient-tag-id")) {
			result = tag;
			break;
		}
	}
	g_slist_free (tags);
	return result;
}

static GtkTextTag *
prev_iter_has_recipient (GtkTextIter *iter)
{
	GtkTextIter prev;

	prev = *iter;
	gtk_text_iter_backward_char (&prev);
	return iter_has_recipient (&prev);
}

/* static GtkTextTag * */
/* next_iter_has_recipient (GtkTextIter *iter) */
/* { */
/* 	GtkTextIter next; */

/* 	next = *iter; */
/* 	return iter_has_recipient (&next); */
/* } */

static gunichar 
iter_previous_char (GtkTextIter *iter)
{
	GtkTextIter prev;

	prev = *iter;
	gtk_text_iter_backward_char (&prev);
	return gtk_text_iter_get_char (&prev);
}

/* static gunichar  */
/* iter_next_char (GtkTextIter *iter) */
/* { */
/* 	GtkTextIter next; */

/* 	next = *iter; */
/* 	gtk_text_iter_forward_char (&next); */
/* 	return gtk_text_iter_get_char (&next); */
/* } */

static void
select_tag_of_iter (GtkTextIter *iter, GtkTextTag *tag, gboolean grow, gboolean left_not_right)
{
	GtkTextIter start, end;

	start = *iter;
	if (!gtk_text_iter_begins_tag (&start, tag)) {
		gtk_text_iter_backward_to_tag_toggle (&start, tag);
	} else {
		if (!left_not_right) {
			gtk_text_buffer_select_range (gtk_text_iter_get_buffer (iter), &start, &start);
			return;
		}
	}
	end = *iter;
	if (!gtk_text_iter_ends_tag (&end, tag)) {
		gtk_text_iter_forward_to_tag_toggle (&end, tag);
	} else {
		if (left_not_right) {
			gtk_text_buffer_select_range (gtk_text_iter_get_buffer (iter), &end, &end);
			return;
		}
	}
	if (grow) {
		if (left_not_right)
			gtk_text_buffer_select_range (gtk_text_iter_get_buffer (iter), &start, &end);
		else
			gtk_text_buffer_select_range (gtk_text_iter_get_buffer (iter), &end, &start);
	} else {
		if (left_not_right)
			gtk_text_buffer_select_range (gtk_text_iter_get_buffer (iter), &start, &start);
		else
			gtk_text_buffer_select_range (gtk_text_iter_get_buffer (iter), &end, &end);
	}
	*iter = left_not_right?start:end;
}

static gboolean 
quote_opened (GtkTextIter *iter)
{
	GtkTextIter start;
	GtkTextBuffer *buffer;
	gboolean opened = FALSE;

	buffer = gtk_text_iter_get_buffer (iter);
	gtk_text_buffer_get_start_iter (buffer, &start);

	while (!gtk_text_iter_equal (&start, iter)) {
		gunichar current_char = gtk_text_iter_get_char (&start);
		if (current_char == '"')
			opened = !opened;
		else if (current_char == '\\')
			gtk_text_iter_forward_char (&start);
		if (!gtk_text_iter_equal (&start, iter))
			gtk_text_iter_forward_char (&start);
			
	}
	return opened;

}


static gboolean
modest_recpt_editor_on_key_press_event (GtkTextView *text_view,
					  GdkEventKey *key,
					  ModestRecptEditor *editor)
{
	GtkTextMark *insert;
	GtkTextMark *selection;
	GtkTextBuffer * buffer;
	GtkTextIter location, selection_loc;
	GtkTextTag *tag;
	gboolean shift_pressed;
	gboolean select_to_left;
	gboolean has_selection;
     
#ifdef MODEST_TOOLKIT_HILDON2
	buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (text_view));
#else
	buffer = gtk_text_view_get_buffer (text_view);
#endif
	insert = gtk_text_buffer_get_insert (buffer);
	selection = gtk_text_buffer_get_selection_bound (buffer);

	/* cases to cover:
	 *    * cursor is on resolved recipient:
	 *        - right should go to first character after the recipient (usually ; or ,)
	 *        - left should fo to the first character before the recipient
	 *        - return should run check names on the recipient.
	 *    * cursor is just after a recipient:
	 *        - right should go to the next character. If it's a recipient, should select
	 *          it
	 *        - left should go to the previous character. If it's a recipient, should go
	 *          to the first character of the recipient, and select it.
	 *    * cursor is on arbitrary text:
	 *        - return should add a ; and go to the next line
	 *        - left or right standard ones.
	 *    * cursor is after a \n:
	 *        - left should go to the character before the \n (as if \n was not a character)
	 *    * cursor is before a \n:
	 *        - right should go to the character after the \n
	 */

	gtk_text_buffer_get_iter_at_mark (buffer, &location, insert);
	gtk_text_buffer_get_iter_at_mark (buffer, &selection_loc, selection);

	select_to_left = gtk_text_iter_get_offset (&selection_loc) > gtk_text_iter_get_offset (&location);
	has_selection = gtk_text_iter_get_offset (&selection_loc) != gtk_text_iter_get_offset (&location);
	shift_pressed = key->state & GDK_SHIFT_MASK;

	switch (key->keyval) {
	case GDK_Left:
	case GDK_KP_Left: 
	{
		gboolean cursor_ready = FALSE;
		GtkTextIter prev_location;

		prev_location = location;
		while (!cursor_ready) {
			if (iter_previous_char (&location) == '\n') {
				gtk_text_iter_backward_char (&location);
			} else {
				cursor_ready = TRUE;
			}
		}
		tag = iter_has_recipient (&location);
		if (has_selection && gtk_text_iter_ends_tag (&prev_location, tag)) {
			gtk_text_iter_backward_to_tag_toggle (&prev_location, tag);
			location = prev_location;
			cursor_ready = FALSE;
			while (!cursor_ready) {
				if (iter_previous_char (&location) == '\n') {
					gtk_text_iter_backward_char (&location);
				} else {
					cursor_ready = TRUE;
				}
			}
		}
		
		if ((tag != NULL)&& (gtk_text_iter_is_start (&location) || !(gtk_text_iter_begins_tag (&location, tag)))) {
			if (has_selection) {
				gtk_text_buffer_select_range (buffer, &location, &location);
			} else {
				select_tag_of_iter (&location, tag, select_to_left, TRUE);
			}
			gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view), insert, 0.0, FALSE, 0.0, 1.0);

			if (shift_pressed) {
				gtk_text_buffer_select_range (buffer, &location, &selection_loc);
			}
			return TRUE;
		}
		gtk_text_iter_backward_char (&location);
		tag = iter_has_recipient (&location);
		if (tag != NULL)
			select_tag_of_iter (&location, tag, select_to_left, TRUE);
		else {
			gtk_text_buffer_place_cursor (buffer, &location);
		}
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view), insert, 0.0, FALSE, 0.0, 1.0);

		if (shift_pressed) {
			gtk_text_buffer_select_range (buffer, &location, &selection_loc);
		}

		return TRUE;
	}
	break;
	case GDK_Right:
	case GDK_KP_Right:
	{
		gboolean cursor_moved = FALSE;

		tag = iter_has_recipient (&location);
		if ((tag != NULL)&&(!gtk_text_iter_ends_tag (&location, tag))) {
			gtk_text_iter_forward_to_tag_toggle (&location, tag);
			while (gtk_text_iter_get_char (&location) == '\n')
				gtk_text_iter_forward_char (&location);
			gtk_text_buffer_place_cursor (buffer, &location);
			gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view), insert, 0.0, FALSE, 0.0, 1.0);

			if (shift_pressed) {
				gtk_text_buffer_select_range (buffer, &location, &selection_loc);
			}
			return TRUE;
		}

		while (gtk_text_iter_get_char (&location) == '\n') {
			gtk_text_iter_forward_char (&location);
			cursor_moved = TRUE;
		}
		if (!cursor_moved)
			gtk_text_iter_forward_char (&location);
		while (gtk_text_iter_get_char (&location) == '\n') {
			gtk_text_iter_forward_char (&location);
			cursor_moved = TRUE;
		}

		tag = iter_has_recipient (&location);
		if (tag != NULL)
			select_tag_of_iter (&location, tag, !select_to_left, FALSE);
		else
			gtk_text_buffer_place_cursor (buffer, &location);
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view), insert, 0.0, FALSE, 0.0, 1.0);

		if (shift_pressed) {
			gtk_text_buffer_select_range (buffer, &location, &selection_loc);
		}
		return TRUE;
	}
	break;
	case GDK_Return:
	case GDK_KP_Enter:
	{
		gint insert_offset, selection_offset;
		insert_offset = gtk_text_iter_get_offset (&location);
		selection_offset = gtk_text_iter_get_offset (&selection_loc);
		g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text, editor);
		g_signal_handlers_block_by_func (buffer, modest_recpt_editor_on_insert_text_after, editor);
		if (selection_offset > insert_offset)
			location = selection_loc;
		tag = iter_has_recipient (&location);
		if (tag != NULL) {
			gtk_text_buffer_get_end_iter (buffer, &location);
			gtk_text_buffer_place_cursor (buffer, &location);
			if ((iter_previous_char (&location) != ';')&&(iter_previous_char (&location) != ','))
				gtk_text_buffer_insert_at_cursor (buffer, ";", -1);
			gtk_text_buffer_insert_at_cursor (buffer, "\n", -1);
		} else {
			gunichar prev_char = iter_previous_char (&location);
			if ((gtk_text_iter_is_start (&location))||(prev_char == '\n')
			    ||(prev_char == ';')||(prev_char == ',')) 
				g_signal_emit_by_name (G_OBJECT (editor), "open-addressbook");
			else {
				if ((prev_char != ';') && (prev_char != ','))
					gtk_text_buffer_insert_at_cursor (buffer, ";", -1);
				gtk_text_buffer_insert_at_cursor (buffer, "\n", -1);
			}
		}
		g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text, editor);
		g_signal_handlers_unblock_by_func (buffer, modest_recpt_editor_on_insert_text_after, editor);
		return TRUE;
	}
	break;
	case GDK_BackSpace:
	{
		#if GTK_CHECK_VERSION(2, 10, 0) /* gtk_text_buffer_get_has_selection is only available in GTK+ 2.10 */
		if (gtk_text_buffer_get_has_selection (buffer)) {
			gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
			return TRUE;
		}
		#else
		if (gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL)) {
			gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
			return TRUE;
		}
		#endif

		tag = prev_iter_has_recipient (&location);
		if (tag != NULL) {
			GtkTextIter iter_in_tag;
			iter_in_tag = location;
			gtk_text_iter_backward_char (&iter_in_tag);
			select_tag_of_iter (&iter_in_tag, tag, FALSE, TRUE);
			gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
			return TRUE;
		}
		return FALSE;
	}
	break;
	default:
		return FALSE;
	}
}

static void _discard_chars (GtkTextIter *start, GtkTextIter *end)
{
	while (!gtk_text_iter_equal (start, end)) {
		gunichar c = gtk_text_iter_get_char (start);

		if (c == '\n' || c == ';' || c == ',' || c == ' ') {
			if (!gtk_text_iter_forward_char (start))
				break;
		} else {
			break;
		}

	}
}

/* NOTE: before calling this function be sure that both
   modest_recpt_editor_on_insert_text and
   modest_recpt_editor_on_insert_text_after won't be triggered during
   the execution of the procedure. You'll have to block both signal
   handlers otherwise you'll get an infinite loop and most likely a
   SIGSEV caused by a stack overflow */
static void
modest_recpt_editor_add_tags (ModestRecptEditor *editor,
			      const gchar * recipient_id)
{

	ModestRecptEditorPrivate *priv = MODEST_RECPT_EDITOR_GET_PRIVATE (editor);
#ifdef MODEST_TOOLKIT_HILDON2
	GtkTextBuffer *buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW (priv->text_view));
#else
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
#endif
	GtkTextTag *tag;
	GtkTextIter start, end;
	gchar * buffer_contents;
	GtkTextIter start_match, end_match;

	/* This would move the cursor to the end of the buffer
	   containing new line character. */
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	buffer_contents = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	gtk_text_buffer_set_text(buffer,buffer_contents,strlen(buffer_contents));

	tag = gtk_text_buffer_create_tag (buffer, NULL,
					  "underline", PANGO_UNDERLINE_SINGLE,
					  "wrap-mode", GTK_WRAP_NONE,
					  "editable", TRUE, NULL);

	g_object_set_data (G_OBJECT (tag), "recipient-tag-id", GINT_TO_POINTER (RECIPIENT_TAG_ID));
	g_object_set_data_full (G_OBJECT (tag), "recipient-id", g_strdup (recipient_id), (GDestroyNotify) g_free);

	/* Formatting the buffer content by applying tag */
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	_discard_chars (&start, &end);
	while (gtk_text_iter_forward_search(&start, ";",
					    GTK_TEXT_SEARCH_TEXT_ONLY |
					    GTK_TEXT_SEARCH_VISIBLE_ONLY,
					    &start_match, &end_match, &end )) {
		int offset;

		gtk_text_buffer_apply_tag(buffer, tag, &start, &start_match);
		offset = gtk_text_iter_get_offset (&end_match);
		gtk_text_buffer_get_iter_at_offset(buffer, &start, offset);
		_discard_chars (&start, &end);
	}
	g_free (buffer_contents);
}

void
modest_recpt_editor_grab_focus (ModestRecptEditor *recpt_editor)
{
	ModestRecptEditorPrivate *priv;
	
	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	if (GTK_WIDGET_VISIBLE (recpt_editor)) {
		gtk_widget_grab_focus (priv->text_view);
	}
}

gboolean
modest_recpt_editor_has_focus (ModestRecptEditor *recpt_editor)
{
	ModestRecptEditorPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor), FALSE);
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	return GTK_WIDGET_VISIBLE (priv->text_view) && 
		gtk_widget_is_focus (priv->text_view);
}

void 
modest_recpt_editor_set_show_abook_button (ModestRecptEditor *recpt_editor, gboolean show)
{
	ModestRecptEditorPrivate *priv;
	
	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	priv->show_abook = show;

	if (show)
		gtk_widget_show (priv->abook_button);
	else
		gtk_widget_hide (priv->abook_button);
}

gboolean
modest_recpt_editor_get_show_abook_button (ModestRecptEditor *recpt_editor, gboolean show)
{
	ModestRecptEditorPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor), FALSE);
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (recpt_editor);

	return priv->show_abook;
	
}

static void
modest_recpt_editor_finalize (GObject *object)
{
	ModestRecptEditorPrivate *priv;
	priv = MODEST_RECPT_EDITOR_GET_PRIVATE (object);

	if (g_signal_handler_is_connected (object, priv->on_mark_set_handler))
		g_signal_handler_disconnect (object, priv->on_mark_set_handler);
	priv->on_mark_set_handler = 0;

	if (priv->recipients) {
		g_free (priv->recipients);
		priv->recipients = NULL;
	}

	(*parent_class->finalize) (object);

	return;
}

static void 
modest_recpt_editor_class_init (ModestRecptEditorClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = modest_recpt_editor_finalize;

	g_type_class_add_private (object_class, sizeof (ModestRecptEditorPrivate));

	signals[OPEN_ADDRESSBOOK_SIGNAL] = 
		g_signal_new ("open-addressbook",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ModestRecptEditorClass, open_addressbook),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	return;
}

GType 
modest_recpt_editor_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestRecptEditorClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_recpt_editor_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestRecptEditor),
		  0,      /* n_preallocs */
		  modest_recpt_editor_instance_init    /* instance_init */
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
			"ModestRecptEditor",
			&info, 0);

	}

	return type;
}

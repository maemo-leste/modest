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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <string.h>
#include <tny-account-store.h>
#include <tny-fs-stream.h>
#include <tny-vfs-stream.h>

#include <config.h>

#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>

#include <widgets/modest-msg-edit-window.h>
#include <widgets/modest-combo-box.h>
#include <widgets/modest-recpt-editor.h>
#include <widgets/modest-attachments-view.h>

#include <modest-runtime.h>

#include "modest-platform.h"
#include "modest-icon-names.h"
#include "modest-widget-memory.h"
#include "modest-window-priv.h"
#include "modest-mail-operation.h"
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg.h"
#include "modest-tny-folder.h"
#include "modest-address-book.h"
#include "modest-text-utils.h"
#include <tny-simple-list.h>
#include <wptextview.h>
#include <wptextbuffer.h>
#include "modest-scroll-area.h"

#include "modest-hildon-includes.h"
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <hildon-widgets/hildon-color-chooser.h>
#endif
#include "widgets/modest-msg-edit-window-ui.h"
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#else
#include <libgnomevfs/gnome-vfs-mime.h>
#endif
#include "modest-maemo-utils.h"


#define DEFAULT_FONT_SIZE 3
#define DEFAULT_FONT 2
#define DEFAULT_SIZE_BUTTON_FONT_FAMILY "Sans"
#define DEFAULT_SIZE_COMBOBOX_WIDTH 80
#define DEFAULT_MAIN_VBOX_SPACING 6
#define SUBJECT_MAX_LENGTH 1000
#define IMAGE_MAX_WIDTH 608
#define DEFAULT_FONT_SCALE 1.5

static void  modest_msg_edit_window_class_init   (ModestMsgEditWindowClass *klass);
static void  modest_msg_edit_window_init         (ModestMsgEditWindow *obj);
static void  modest_msg_edit_window_finalize     (GObject *obj);

static gboolean msg_body_focus (GtkWidget *focus, GdkEventFocus *event, gpointer userdata);
static void  recpt_field_changed (GtkTextBuffer *buffer, ModestMsgEditWindow *editor);
static void  send_insensitive_press (GtkWidget *widget, ModestMsgEditWindow *editor);
static void  style_insensitive_press (GtkWidget *widget, ModestMsgEditWindow *editor);
static void  setup_insensitive_handlers (ModestMsgEditWindow *editor);
static void  reset_modified (ModestMsgEditWindow *editor);

static void  text_buffer_refresh_attributes (WPTextBuffer *buffer, ModestMsgEditWindow *window);
static void  text_buffer_can_undo (GtkTextBuffer *buffer, gboolean can_undo, ModestMsgEditWindow *window);
static void  text_buffer_can_redo (GtkTextBuffer *buffer, gboolean can_redo, ModestMsgEditWindow *window);
static void  text_buffer_apply_tag (GtkTextBuffer *buffer, GtkTextTag *tag, 
				    GtkTextIter *start, GtkTextIter *end,
				    gpointer userdata);
static void  text_buffer_delete_images_by_id (GtkTextBuffer *buffer, const gchar * image_id);
static void  subject_field_changed (GtkEditable *editable, ModestMsgEditWindow *window);
static void  modest_msg_edit_window_color_button_change (ModestMsgEditWindow *window,
							 gpointer userdata);
static void  modest_msg_edit_window_size_change (GtkCheckMenuItem *menu_item,
						 gpointer userdata);
static void  modest_msg_edit_window_font_change (GtkCheckMenuItem *menu_item,
						 gpointer userdata);
static void  modest_msg_edit_window_setup_toolbar (ModestMsgEditWindow *window);
static gboolean modest_msg_edit_window_window_state_event (GtkWidget *widget, 
							   GdkEventWindowState *event, 
							   gpointer userdata);
static void modest_msg_edit_window_open_addressbook (ModestMsgEditWindow *window,
						     ModestRecptEditor *editor);
static void modest_msg_edit_window_add_attachment_clicked (GtkButton *button,
							   ModestMsgEditWindow *window);

/* ModestWindow methods implementation */
static void  modest_msg_edit_window_set_zoom (ModestWindow *window, gdouble zoom);
static gdouble modest_msg_edit_window_get_zoom (ModestWindow *window);
static gboolean modest_msg_edit_window_zoom_minus (ModestWindow *window);
static gboolean modest_msg_edit_window_zoom_plus (ModestWindow *window);
static void modest_msg_edit_window_show_toolbar   (ModestWindow *window,
						   gboolean show_toolbar);
static void modest_msg_edit_window_clipboard_owner_change (GtkClipboard *clipboard,
							   GdkEvent *event,
							   ModestMsgEditWindow *window);
static void update_window_title (ModestMsgEditWindow *window);
static void update_dimmed (ModestMsgEditWindow *window);
static void update_paste_dimming (ModestMsgEditWindow *window);
static void update_select_all_dimming (ModestMsgEditWindow *window);
static void update_zoom_dimming (ModestMsgEditWindow *window);

/* Find toolbar */
static void modest_msg_edit_window_find_toolbar_search (GtkWidget *widget,
							ModestMsgEditWindow *window);
static void modest_msg_edit_window_find_toolbar_close (GtkWidget *widget,
						       ModestMsgEditWindow *window);
static gboolean gtk_text_iter_forward_search_insensitive (const GtkTextIter *iter,
							  const gchar *str,
							  GtkTextIter *match_start,
							  GtkTextIter *match_end);
							  
static void DEBUG_BUFFER (WPTextBuffer *buffer)
{
#ifdef DEBUG
	GtkTextIter iter;

	g_message ("BEGIN BUFFER OF SIZE %d", gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (buffer)));
	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &iter);
	while (!gtk_text_iter_is_end (&iter)) {
		GString *output = g_string_new ("");
		GSList *toggled_tags;
		GSList *node;

		toggled_tags = gtk_text_iter_get_toggled_tags (&iter, FALSE);
		g_string_append_printf (output, "%d: CLOSED [ ", gtk_text_iter_get_offset (&iter));
		for (node = toggled_tags; node != NULL; node = g_slist_next (node)) {
			GtkTextTag *tag = (GtkTextTag *) node->data;
			const gchar *name;
			g_object_get (G_OBJECT (tag), "name", &name, NULL);
			output = g_string_append (output, name);
			g_string_append (output, " ");
		}
		output = g_string_append (output, "] OPENED [ ");
		toggled_tags = gtk_text_iter_get_toggled_tags (&iter, TRUE);
		for (node = toggled_tags; node != NULL; node = g_slist_next (node)) {
			GtkTextTag *tag = (GtkTextTag *) node->data;
			const gchar *name;
			g_object_get (G_OBJECT (tag), "name", &name, NULL);
			output = g_string_append (output, name);
			g_string_append (output, " ");
		}
		output = g_string_append (output, "]\n");
		g_message ("%s", output->str);
		g_string_free (output, TRUE);
		gtk_text_iter_forward_to_tag_toggle (&iter, NULL);
	}
	g_message ("END BUFFER");
#endif
}


/* static gboolean */
/* on_key_pressed (GtkWidget *self, */
/* 		GdkEventKey *event, */
/* 		gpointer user_data); */

static void edit_menu_activated (GtkAction *action,
				 gpointer userdata);
static void view_menu_activated (GtkAction *action,
				 gpointer userdata);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMsgEditWindowPrivate ModestMsgEditWindowPrivate;
struct _ModestMsgEditWindowPrivate {
	GtkWidget   *msg_body;
	GtkWidget   *header_box;
	
	ModestPairList *from_field_protos;
	GtkWidget   *from_field;
	
	GtkWidget   *to_field;
	GtkWidget   *cc_field;
	GtkWidget   *bcc_field;
	GtkWidget   *subject_field;
	GtkWidget   *attachments_view;
	GtkWidget   *priority_icon;
	GtkWidget   *add_attachment_button;

	GtkWidget   *cc_caption;
	GtkWidget   *bcc_caption;
	gboolean     update_caption_visibility;
	GtkWidget   *attachments_caption;

	GtkTextBuffer *text_buffer;

	GtkWidget   *font_size_toolitem;
	GtkWidget   *font_face_toolitem;
	GtkWidget   *font_color_button;
	GSList      *font_items_group;
	GtkWidget   *font_tool_button_label;
	GSList      *size_items_group;
	GtkWidget   *size_tool_button_label;
	
	GtkWidget   *find_toolbar;
	gchar       *last_search;

	GtkWidget   *scroll;
	GtkWidget   *scroll_area;

	gint last_cid;
	GList *attachments;

	TnyHeaderFlags priority_flags;

	gdouble zoom_level;
	
	gulong      clipboard_change_handler_id;

	TnyMsg      *draft_msg;
	gboolean    sent;
};

#define MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_MSG_EDIT_WINDOW, \
                                                    ModestMsgEditWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_msg_edit_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMsgEditWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_msg_edit_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMsgEditWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_msg_edit_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
		                                  "ModestMsgEditWindow",
		                                  &my_info, 0);

		wp_text_buffer_library_init ();
	}
	return my_type;
}

static void
save_state (ModestWindow *self)
{
	modest_widget_memory_save (modest_runtime_get_conf(),
				   G_OBJECT(self), MODEST_CONF_EDIT_WINDOW_KEY);
}


static void
restore_settings (ModestMsgEditWindow *self)
{
	modest_widget_memory_restore (modest_runtime_get_conf(),
				      G_OBJECT(self), MODEST_CONF_EDIT_WINDOW_KEY);
}


static void
modest_msg_edit_window_class_init (ModestMsgEditWindowClass *klass)
{
	GObjectClass *gobject_class;
	ModestWindowClass *modest_window_class;
	gobject_class = (GObjectClass*) klass;
	modest_window_class = (ModestWindowClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_edit_window_finalize;

	modest_window_class->set_zoom_func = modest_msg_edit_window_set_zoom;
	modest_window_class->get_zoom_func = modest_msg_edit_window_get_zoom;
	modest_window_class->zoom_plus_func = modest_msg_edit_window_zoom_plus;
	modest_window_class->zoom_minus_func = modest_msg_edit_window_zoom_minus;
	modest_window_class->show_toolbar_func = modest_msg_edit_window_show_toolbar;
	modest_window_class->save_state_func = save_state;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgEditWindowPrivate));


}

static void
modest_msg_edit_window_init (ModestMsgEditWindow *obj)
{
	ModestMsgEditWindowPrivate *priv;
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);

	priv->msg_body      = NULL;
	priv->from_field    = NULL;
	priv->to_field      = NULL;
	priv->cc_field      = NULL;
	priv->bcc_field     = NULL;
	priv->subject_field = NULL;
	priv->attachments   = NULL;
	priv->last_cid      = 0;
	priv->zoom_level    = 1.0;

	priv->cc_caption    = NULL;
	priv->bcc_caption    = NULL;
	priv->update_caption_visibility = FALSE;

	priv->priority_flags = 0;

	priv->find_toolbar = NULL;
	priv->last_search = NULL;

	priv->draft_msg = NULL;
	priv->clipboard_change_handler_id = 0;
	priv->sent = FALSE;
}


/* FIXME: this is a dup from the one in gtk/ */

/** 
 * @result: A ModestPairList, which must be freed with modest_pair_list_free().
 */
static ModestPairList*
get_transports (void)
{
	GSList *transports = NULL;
	
	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr();
 	GSList *accounts = modest_account_mgr_account_names (account_mgr, 
							     TRUE /* only enabled accounts. */); 
 						
 	GSList *cursor = accounts;
	while (cursor) {
		gchar *account_name = cursor->data;
		gchar *from_string  = NULL;
		if (account_name) {
			from_string = modest_account_mgr_get_from_string (account_mgr,
									  account_name);
		}
		
		if (from_string && account_name) {
			gchar *name = account_name;
			ModestPair *pair = modest_pair_new ((gpointer) name,
						(gpointer) from_string , TRUE);
			transports = g_slist_prepend (transports, pair);
		}
		
		cursor = cursor->next;
	}
	g_slist_free (accounts); /* only free the accounts, not the elements,
				  * because they are used in the pairlist */
	return transports;
}


static void
init_window (ModestMsgEditWindow *obj)
{
	GtkWidget *from_caption, *to_caption, *subject_caption;
	GtkWidget *main_vbox;
	ModestMsgEditWindowPrivate *priv;

	GtkSizeGroup *size_group;
	GtkWidget *frame;
	GtkWidget *subject_box;
	GtkWidget *attachment_icon;
	GtkWidget *window_box;
#if (GTK_MINOR_VERSION >= 10)
	GdkAtom deserialize_type;
#endif
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	priv->from_field_protos = get_transports ();

 	priv->from_field    = modest_combo_box_new (priv->from_field_protos, g_str_equal);

	priv->to_field      = modest_recpt_editor_new ();
	priv->cc_field      = modest_recpt_editor_new ();
	priv->bcc_field     = modest_recpt_editor_new ();
	subject_box = gtk_hbox_new (FALSE, 0);
	priv->priority_icon = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (subject_box), priv->priority_icon, FALSE, FALSE, 0);
	priv->subject_field = gtk_entry_new_with_max_length (SUBJECT_MAX_LENGTH);
	g_object_set (G_OBJECT (priv->subject_field), "truncate-multiline", TRUE, NULL);
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->subject_field), 
					 HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_AUTOCAP);
	gtk_box_pack_start (GTK_BOX (subject_box), priv->subject_field, TRUE, TRUE, 0);
	priv->add_attachment_button = gtk_button_new ();
	GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (priv->add_attachment_button), GTK_CAN_FOCUS);
	gtk_button_set_relief (GTK_BUTTON (priv->add_attachment_button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click (GTK_BUTTON (priv->add_attachment_button), FALSE);
	gtk_button_set_alignment (GTK_BUTTON (priv->add_attachment_button), 1.0, 1.0);
	attachment_icon = gtk_image_new_from_icon_name (MODEST_HEADER_ICON_ATTACH, GTK_ICON_SIZE_BUTTON);
	gtk_container_add (GTK_CONTAINER (priv->add_attachment_button), attachment_icon);
	gtk_box_pack_start (GTK_BOX (subject_box), priv->add_attachment_button, FALSE, FALSE, 0);
	priv->attachments_view = modest_attachments_view_new (NULL);
	
	priv->header_box = gtk_vbox_new (FALSE, 0);
	
	from_caption = hildon_caption_new (size_group, _("mail_va_from"), priv->from_field, NULL, 0);
	to_caption = hildon_caption_new (size_group, _("mail_va_to"), priv->to_field, NULL, 0);
	priv->cc_caption = hildon_caption_new (size_group, _("mail_va_cc"), priv->cc_field, NULL, 0);
	priv->bcc_caption = hildon_caption_new (size_group, _("mail_va_hotfix1"), priv->bcc_field, NULL, 0);
	subject_caption = hildon_caption_new (size_group, _("mail_va_subject"), subject_box, NULL, 0);
	priv->attachments_caption = hildon_caption_new (size_group, _("mail_va_attachment"), priv->attachments_view, NULL, 0);
	g_object_unref (size_group);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	modest_recpt_editor_set_field_size_group (MODEST_RECPT_EDITOR (priv->to_field), size_group);
	modest_recpt_editor_set_field_size_group (MODEST_RECPT_EDITOR (priv->cc_field), size_group);
	modest_recpt_editor_set_field_size_group (MODEST_RECPT_EDITOR (priv->bcc_field), size_group);
	gtk_size_group_add_widget (size_group, priv->subject_field);
	gtk_size_group_add_widget (size_group, priv->attachments_view);
	g_object_unref (size_group);

	gtk_box_pack_start (GTK_BOX (priv->header_box), from_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), to_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), priv->cc_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), priv->bcc_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), subject_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), priv->attachments_caption, FALSE, FALSE, 0);
	gtk_widget_set_no_show_all (priv->attachments_caption, TRUE);


	priv->msg_body = wp_text_view_new ();
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->msg_body), GTK_WRAP_WORD_CHAR);
	priv->text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));
	g_object_set (priv->text_buffer, "font_scale", DEFAULT_FONT_SCALE, NULL);
	wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
#if (GTK_MINOR_VERSION >= 10)
	gtk_text_buffer_register_serialize_tagset(GTK_TEXT_BUFFER(priv->text_buffer), "wp-text-buffer");
	deserialize_type = gtk_text_buffer_register_deserialize_tagset(GTK_TEXT_BUFFER(priv->text_buffer), 
								       "wp-text-buffer");
	gtk_text_buffer_deserialize_set_can_create_tags (GTK_TEXT_BUFFER (priv->text_buffer), 
							 deserialize_type, TRUE);
#endif
	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);

	priv->find_toolbar = hildon_find_toolbar_new (NULL);
	gtk_widget_set_no_show_all (priv->find_toolbar, TRUE);

/* 	g_signal_connect (G_OBJECT (obj), "key_pressed", G_CALLBACK (on_key_pressed), NULL) */

	g_signal_connect (G_OBJECT (priv->text_buffer), "refresh_attributes",
			  G_CALLBACK (text_buffer_refresh_attributes), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "can-undo",
			  G_CALLBACK (text_buffer_can_undo), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "can-redo",
			  G_CALLBACK (text_buffer_can_redo), obj);
	g_signal_connect (G_OBJECT (obj), "window-state-event",
			  G_CALLBACK (modest_msg_edit_window_window_state_event),
			  NULL);
	g_signal_connect_after (G_OBJECT (priv->text_buffer), "apply-tag",
				G_CALLBACK (text_buffer_apply_tag), obj);
	g_signal_connect_swapped (G_OBJECT (priv->to_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);
	g_signal_connect_swapped (G_OBJECT (priv->cc_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);
	g_signal_connect_swapped (G_OBJECT (priv->bcc_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);

	g_signal_connect (G_OBJECT (priv->add_attachment_button), "clicked",
			  G_CALLBACK (modest_msg_edit_window_add_attachment_clicked), obj);

	g_signal_connect (G_OBJECT (priv->msg_body), "focus-in-event",
			  G_CALLBACK (msg_body_focus), obj);
	g_signal_connect (G_OBJECT (priv->msg_body), "focus-out-event",
			  G_CALLBACK (msg_body_focus), obj);
	g_signal_connect (G_OBJECT (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->to_field))),
			  "changed", G_CALLBACK (recpt_field_changed), obj);
	g_signal_connect (G_OBJECT (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->cc_field))),
			  "changed", G_CALLBACK (recpt_field_changed), obj);
	g_signal_connect (G_OBJECT (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->bcc_field))),
			  "changed", G_CALLBACK (recpt_field_changed), obj);
	recpt_field_changed (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->to_field)), MODEST_MSG_EDIT_WINDOW (obj));
	g_signal_connect (G_OBJECT (priv->subject_field), "changed", G_CALLBACK (subject_field_changed), obj);

	g_signal_connect (G_OBJECT (priv->find_toolbar), "close", G_CALLBACK (modest_msg_edit_window_find_toolbar_close), obj);
	g_signal_connect (G_OBJECT (priv->find_toolbar), "search", G_CALLBACK (modest_msg_edit_window_find_toolbar_search), obj);

	priv->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->scroll), GTK_SHADOW_NONE);
	modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->scroll), TRUE);

	main_vbox = gtk_vbox_new  (FALSE, DEFAULT_MAIN_VBOX_SPACING);

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->header_box, FALSE, FALSE, 0);
	frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX(main_vbox), frame, TRUE, TRUE, 0);

	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (priv->scroll), main_vbox);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (main_vbox), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scroll)));
	gtk_widget_show_all (GTK_WIDGET(priv->scroll));
	
	window_box = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (window_box), priv->scroll, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER(obj), window_box);
	priv->scroll_area = modest_scroll_area_new (priv->scroll, priv->msg_body);
	gtk_container_add (GTK_CONTAINER (frame), priv->scroll_area);
	
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->scroll_area), 
					     gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scroll)));

	priv->clipboard_change_handler_id = g_signal_connect (G_OBJECT (gtk_clipboard_get (GDK_SELECTION_PRIMARY)), "owner-change",
							      G_CALLBACK (modest_msg_edit_window_clipboard_owner_change), obj);

}
	


static void
modest_msg_edit_window_finalize (GObject *obj)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (obj);

	if (priv->clipboard_change_handler_id > 0) {
		g_signal_handler_disconnect (gtk_clipboard_get (GDK_SELECTION_PRIMARY), priv->clipboard_change_handler_id);
		priv->clipboard_change_handler_id = 0;
	}
	
	if (priv->draft_msg != NULL) {
		TnyHeader *header = tny_msg_get_header (priv->draft_msg);
		if (TNY_IS_HEADER (header)) {
			ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();
			modest_window_mgr_unregister_header (mgr, header);
		}
		g_object_unref (priv->draft_msg);
		priv->draft_msg = NULL;
	}

	/* This had to stay alive for as long as the combobox that used it: */
	modest_pair_list_free (priv->from_field_protos);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static GtkWidget *
menubar_to_menu (GtkUIManager *ui_manager)
{
	GtkWidget *main_menu;
	GtkWidget *menubar;
	GList *iter;

	/* Create new main menu */
	main_menu = gtk_menu_new();

	/* Get the menubar from the UI manager */
	menubar = gtk_ui_manager_get_widget (ui_manager, "/MenuBar");

	iter = gtk_container_get_children (GTK_CONTAINER (menubar));
	while (iter) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent(menu, main_menu);

		iter = g_list_next (iter);
	}
	return main_menu;
}

static GdkPixbuf *
pixbuf_from_stream (TnyStream *stream, const gchar *mime_type)
{
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;

	loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, NULL);

	if (loader == NULL)
		return NULL;

	tny_stream_reset (TNY_STREAM (stream));
	while (!tny_stream_is_eos (TNY_STREAM (stream))) {
		unsigned char read_buffer[128];
		gint readed;
		readed = tny_stream_read (TNY_STREAM (stream), (char *) read_buffer, 128);
		if (!gdk_pixbuf_loader_write (loader, read_buffer, readed, NULL))
			break;
	}

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	g_object_ref (pixbuf);
	gdk_pixbuf_loader_close (loader, NULL);
	g_object_unref (loader);

	if (gdk_pixbuf_get_width (pixbuf) > IMAGE_MAX_WIDTH) {
		GdkPixbuf *new_pixbuf;
		gint new_height;
		new_height = (gdk_pixbuf_get_height (pixbuf) * IMAGE_MAX_WIDTH) /
			gdk_pixbuf_get_width (pixbuf);
		new_pixbuf = gdk_pixbuf_scale_simple (pixbuf, IMAGE_MAX_WIDTH, new_height, GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = new_pixbuf;
	}

	return pixbuf;
}

static void
replace_with_attachments (ModestMsgEditWindow *self, GList *attachments)
{
	ModestMsgEditWindowPrivate *priv;
	GList *node;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	for (node = attachments; node != NULL; node = g_list_next (node)) {
		TnyMimePart *part = (TnyMimePart *) node->data;
		const gchar *cid = tny_mime_part_get_content_id (part);
		const gchar *mime_type = tny_mime_part_get_content_type (part);
		if ((cid != NULL)&&(mime_type != NULL)) {
			TnyStream *stream = tny_mime_part_get_stream (part);
			GdkPixbuf *pixbuf = pixbuf_from_stream (stream, mime_type);
			g_object_unref (stream);

			if (pixbuf != NULL) {
				wp_text_buffer_replace_image (WP_TEXT_BUFFER (priv->text_buffer), cid, pixbuf);
				g_object_unref (pixbuf);
			}
		}
	}
}

static void
update_last_cid (ModestMsgEditWindow *self, GList *attachments)
{
	GList *node;
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	for (node = attachments; node != NULL; node = g_list_next (node)) {
		TnyMimePart *part = (TnyMimePart *) node->data;
		const gchar *cid = tny_mime_part_get_content_id (part);
		if (cid != NULL) {
			char *invalid = NULL;
			gint int_cid = strtol (cid, &invalid, 10);
			if ((invalid != NULL) && (*invalid == '\0') && (int_cid > priv->last_cid)) {
				priv->last_cid = int_cid;
			}
		}
		
	}
}

static void
set_msg (ModestMsgEditWindow *self, TnyMsg *msg)
{
	TnyHeader *header;
	const gchar *to, *cc, *bcc, *subject;
	gchar *body;
	ModestMsgEditWindowPrivate *priv;
	GtkTextIter iter;
	TnyHeaderFlags priority_flags;
	TnyFolder *msg_folder;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (TNY_IS_MSG (msg));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	header = tny_msg_get_header (msg);
	to      = tny_header_get_to (header);
	cc      = tny_header_get_cc (header);
	bcc     = tny_header_get_bcc (header);
	subject = tny_header_get_subject (header);
	priority_flags = tny_header_get_flags (header) & TNY_HEADER_FLAG_PRIORITY;

	if (to)
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->to_field),  to);
	if (cc) {
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->cc_field),  cc);
		gtk_widget_set_no_show_all (priv->cc_caption, FALSE);
		gtk_widget_show (priv->cc_caption);
	} else if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_CC, NULL)) {
		gtk_widget_set_no_show_all (priv->cc_caption, TRUE);
		gtk_widget_hide (priv->cc_caption);
	}
	if (bcc) {
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->bcc_field), bcc);
		gtk_widget_set_no_show_all (priv->bcc_caption, FALSE);
		gtk_widget_show (priv->bcc_caption);
	} else if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_BCC, NULL)) {
		gtk_widget_set_no_show_all (priv->bcc_caption, TRUE);
		gtk_widget_hide (priv->bcc_caption);
	} 
	if (subject)
		gtk_entry_set_text (GTK_ENTRY(priv->subject_field), subject);
	modest_msg_edit_window_set_priority_flags (MODEST_MSG_EDIT_WINDOW(self),
						   priority_flags);

	update_window_title (self);

	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
	body = modest_tny_msg_get_body (msg, TRUE);

	if ((body == NULL)||(body[0] == '\0')) {
		g_free (body);
		body = modest_text_utils_convert_to_html ("");
	}
	wp_text_buffer_load_document_begin (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
	wp_text_buffer_load_document_write (WP_TEXT_BUFFER (priv->text_buffer),
					    (gchar *) body,
					    strlen (body));
	wp_text_buffer_load_document_end (WP_TEXT_BUFFER (priv->text_buffer));
	g_free (body);

	/* Get the default format required from configuration */
	if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_PREFER_FORMATTED_TEXT, NULL)) {
		wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), FALSE);
	}

	/* Set the default focus depending on having already a To: field or not */
	if ((!to)||(*to == '\0')) {
		modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->to_field));
	} else {
		gtk_widget_grab_focus (priv->msg_body);
	}

	/* TODO: lower priority, select in the From: combo to the
	   value that comes from msg <- not sure, should it be
	   allowed? */
	
	/* Add attachments to the view */
	modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), msg);
	priv->attachments = modest_attachments_view_get_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	if (priv->attachments == NULL) {
		gtk_widget_hide (priv->attachments_caption);
	} else {
		gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
		gtk_widget_show_all (priv->attachments_caption);
		replace_with_attachments (self, priv->attachments);
	}
	update_last_cid (self, priv->attachments);

	DEBUG_BUFFER (WP_TEXT_BUFFER (priv->text_buffer));

	gtk_text_buffer_get_start_iter (priv->text_buffer, &iter);
	gtk_text_buffer_place_cursor (priv->text_buffer, &iter);

	reset_modified (self);

	update_dimmed (self);
	text_buffer_can_undo (priv->text_buffer, FALSE, self);
	text_buffer_can_redo (priv->text_buffer, FALSE, self);

	/* we should set a reference to the incoming message if it is a draft */
	msg_folder = tny_msg_get_folder (msg);
	if (msg_folder) {
		if (modest_tny_folder_is_local_folder (msg_folder) &&
		    modest_tny_folder_get_local_or_mmc_folder_type (msg_folder) == TNY_FOLDER_TYPE_DRAFTS)
			priv->draft_msg = g_object_ref(msg);
		g_object_unref (msg_folder);
	}
}

static void
menu_tool_button_clicked_popup (GtkMenuToolButton *item,
				gpointer data)
{
	GList *item_children, *node;
	GtkWidget *bin_child;

	bin_child = gtk_bin_get_child (GTK_BIN(item));

	item_children = gtk_container_get_children (GTK_CONTAINER (bin_child));
	
	for (node = item_children; node != NULL; node = g_list_next (node)) {
		if (GTK_IS_TOGGLE_BUTTON (node->data)) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (node->data), TRUE);
		}
	}
	g_list_free (item_children);
}

static void
menu_tool_button_dont_expand (GtkMenuToolButton *item)
{
	GtkWidget *box;
	GList *item_children, *node;

	box = gtk_bin_get_child (GTK_BIN (item));
	gtk_box_set_homogeneous (GTK_BOX (box), TRUE);
	item_children = gtk_container_get_children (GTK_CONTAINER (box));
	
	for (node = item_children; node != NULL; node = g_list_next (node)) {
		gtk_box_set_child_packing (GTK_BOX (box), GTK_WIDGET (node->data), TRUE, TRUE, 0, GTK_PACK_START);
		if (GTK_IS_TOGGLE_BUTTON (node->data))
			gtk_button_set_alignment (GTK_BUTTON (node->data), 0.0, 0.5);
		else if (GTK_IS_BUTTON (node->data))
			gtk_button_set_alignment (GTK_BUTTON (node->data), 1.0, 0.5);
	}
	g_list_free (item_children);
}


static void
modest_msg_edit_window_setup_toolbar (ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	GtkWidget *placeholder;
	GtkWidget *tool_item;
	gint insert_index;
	gchar size_text[5];
	gint size_index;
	gint font_index;
	GtkWidget *sizes_menu;
	GtkWidget *fonts_menu;
	GSList *radio_group = NULL;
	GSList *node = NULL;
	gchar *markup;

	/* Toolbar */
	parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	hildon_window_add_toolbar (HILDON_WINDOW (window), GTK_TOOLBAR (parent_priv->toolbar));

	/* should we hide the toolbar? */
	if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_TOOLBAR, NULL))
		gtk_widget_hide (parent_priv->toolbar);

	/* Font color placeholder */
	placeholder = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/FontColor");
	insert_index = gtk_toolbar_get_item_index(GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM(placeholder));

	/* font color */
	tool_item = GTK_WIDGET (gtk_tool_item_new ());
	priv->font_color_button = hildon_color_button_new ();
	GTK_WIDGET_UNSET_FLAGS (tool_item, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (priv->font_color_button, GTK_CAN_FOCUS);
	gtk_container_add (GTK_CONTAINER (tool_item), priv->font_color_button);
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	g_signal_connect_swapped (G_OBJECT (priv->font_color_button), "notify::color", G_CALLBACK (modest_msg_edit_window_color_button_change), window);

	/* Font size and face placeholder */
	placeholder = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/FontAttributes");
	insert_index = gtk_toolbar_get_item_index(GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM(placeholder));
	/* font_size */
	tool_item = GTK_WIDGET (gtk_menu_tool_button_new (NULL, NULL));
	priv->size_tool_button_label = gtk_label_new (NULL);
	snprintf(size_text, sizeof(size_text), "%d", wp_font_size[DEFAULT_FONT_SIZE]);
	markup = g_strconcat ("<span font_family='", DEFAULT_SIZE_BUTTON_FONT_FAMILY, "'>",
			      size_text,"</span>", NULL);
	gtk_label_set_markup (GTK_LABEL (priv->size_tool_button_label), markup);
	g_free (markup);
	gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (tool_item), priv->size_tool_button_label);
	sizes_menu = gtk_menu_new ();
	priv->size_items_group = NULL;
	radio_group = NULL;
	for (size_index = 0; size_index < WP_FONT_SIZE_COUNT; size_index++) {
		GtkWidget *size_menu_item;

		snprintf(size_text, sizeof(size_text), "%d", wp_font_size[size_index]);
		size_menu_item = gtk_radio_menu_item_new_with_label (radio_group, size_text);
		radio_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (size_menu_item));
		gtk_menu_shell_append (GTK_MENU_SHELL (sizes_menu), size_menu_item);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (size_menu_item), (wp_font_size[size_index] == 12));
		gtk_widget_show (size_menu_item);

		priv->size_items_group = g_slist_prepend (priv->size_items_group, size_menu_item);
			
	}

	for (node = radio_group; node != NULL; node = g_slist_next (node)) {
		GtkWidget *item = (GtkWidget *) node->data;
		g_signal_connect (G_OBJECT (item), "toggled", G_CALLBACK (modest_msg_edit_window_size_change),
				  window);
	}

	priv->size_items_group = g_slist_reverse (priv->size_items_group);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (tool_item), sizes_menu);
	g_signal_connect (G_OBJECT (tool_item), "clicked", G_CALLBACK (menu_tool_button_clicked_popup), NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	menu_tool_button_dont_expand (GTK_MENU_TOOL_BUTTON (tool_item));
	priv->font_size_toolitem = tool_item;

	/* font face */
	tool_item = GTK_WIDGET (gtk_menu_tool_button_new (NULL, NULL));
	priv->font_tool_button_label = gtk_label_new (NULL);
	markup = g_strconcat ("<span font_family='", wp_get_font_name(DEFAULT_FONT), "'>Tt</span>", NULL);
	gtk_label_set_markup (GTK_LABEL (priv->font_tool_button_label), markup);
	g_free(markup);
	gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (tool_item), priv->font_tool_button_label);
	fonts_menu = gtk_menu_new ();
	priv->font_items_group = NULL;
	radio_group = NULL;
	for (font_index = 0; font_index < wp_get_font_count (); font_index++) {
		GtkWidget *font_menu_item;
		GtkWidget *child_label;

		font_menu_item = gtk_radio_menu_item_new_with_label (radio_group, "");
		child_label = gtk_bin_get_child (GTK_BIN (font_menu_item));
		markup = g_strconcat ("<span font_family='", wp_get_font_name (font_index),"'>", 
				      wp_get_font_name (font_index), "</span>", NULL);
		gtk_label_set_markup (GTK_LABEL (child_label), markup);
		g_free (markup);
		
		radio_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (font_menu_item));
		gtk_menu_shell_append (GTK_MENU_SHELL (fonts_menu), font_menu_item);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (font_menu_item), (font_index == DEFAULT_FONT));
		gtk_widget_show (font_menu_item);

		priv->font_items_group = g_slist_prepend (priv->font_items_group, font_menu_item);
			
	}
	for (node = radio_group; node != NULL; node = g_slist_next (node)) {
		GtkWidget *item = (GtkWidget *) node->data;
		g_signal_connect (G_OBJECT (item), "toggled", G_CALLBACK (modest_msg_edit_window_font_change),
				  window);
	}
	priv->font_items_group = g_slist_reverse (priv->font_items_group);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (tool_item), fonts_menu);
	g_signal_connect (G_OBJECT (tool_item), "clicked", G_CALLBACK (menu_tool_button_clicked_popup), NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	menu_tool_button_dont_expand (GTK_MENU_TOOL_BUTTON (tool_item));
	priv->font_face_toolitem = tool_item;

	/* Set expand and homogeneous for remaining items */
	tool_item = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSend");
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	tool_item = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	tool_item = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);


}



ModestWindow*
modest_msg_edit_window_new (TnyMsg *msg, const gchar *account_name)
{
	GObject *obj;
	ModestWindowPrivate *parent_priv;
	ModestMsgEditWindowPrivate *priv;
	GtkActionGroup *action_group;
	GError *error = NULL;
	GdkPixbuf *window_icon = NULL;
	GtkAction *action;
	ModestConf *conf;
	gboolean prefer_formatted;
	gint file_format;
	ModestPair *account_pair = NULL;

	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (account_name, NULL);
	
	obj = g_object_new(MODEST_TYPE_MSG_EDIT_WINDOW, NULL);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (obj);

	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMsgEditWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_msg_edit_action_entries,
				      G_N_ELEMENTS (modest_msg_edit_action_entries),
				      obj);
	gtk_action_group_add_toggle_actions (action_group,
					     modest_msg_edit_toggle_action_entries,
					     G_N_ELEMENTS (modest_msg_edit_toggle_action_entries),
					     obj);
	gtk_action_group_add_radio_actions (action_group,
					    modest_msg_edit_alignment_radio_action_entries,
					    G_N_ELEMENTS (modest_msg_edit_alignment_radio_action_entries),
					    GTK_JUSTIFY_LEFT,
					    G_CALLBACK (modest_ui_actions_on_change_justify),
					    obj);
	gtk_action_group_add_radio_actions (action_group,
					    modest_msg_edit_zoom_action_entries,
					    G_N_ELEMENTS (modest_msg_edit_zoom_action_entries),
					    100,
					    G_CALLBACK (modest_ui_actions_on_change_zoom),
					    obj);
	gtk_action_group_add_radio_actions (action_group,
					    modest_msg_edit_priority_action_entries,
					    G_N_ELEMENTS (modest_msg_edit_priority_action_entries),
					    0,
					    G_CALLBACK (modest_ui_actions_msg_edit_on_change_priority),
					    obj);
	gtk_action_group_add_radio_actions (action_group,
					    modest_msg_edit_file_format_action_entries,
					    G_N_ELEMENTS (modest_msg_edit_file_format_action_entries),
					    modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_PREFER_FORMATTED_TEXT, NULL),
					    G_CALLBACK (modest_ui_actions_msg_edit_on_change_file_format),
					    obj);
	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 0);
	g_object_unref (action_group);

	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager, MODEST_UIDIR "modest-msg-edit-window-ui.xml",
					 &error);
	if (error != NULL) {
		g_warning ("Could not merge modest-msg-edit-window-ui.xml: %s", error->message);
		g_clear_error (&error);
	}

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));

	/* Menubar */
	parent_priv->menubar = menubar_to_menu (parent_priv->ui_manager);
	hildon_window_set_menu (HILDON_WINDOW (obj), GTK_MENU (parent_priv->menubar));

	/* Init window */
	init_window (MODEST_MSG_EDIT_WINDOW(obj));

	restore_settings (MODEST_MSG_EDIT_WINDOW(obj));
		
	modest_window_set_active_account (MODEST_WINDOW(obj), account_name);

	modest_msg_edit_window_setup_toolbar (MODEST_MSG_EDIT_WINDOW (obj));
	hildon_window_add_toolbar (HILDON_WINDOW (obj), GTK_TOOLBAR (priv->find_toolbar));

	setup_insensitive_handlers (MODEST_MSG_EDIT_WINDOW (obj));

	account_pair = modest_pair_list_find_by_first_as_string (priv->from_field_protos, account_name);
	if (account_pair != NULL)
		modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->from_field), account_pair->first);

	set_msg (MODEST_MSG_EDIT_WINDOW (obj), msg);

	text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (obj));

	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_MSG_EDIT_ICON);
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (obj), window_icon);
		g_object_unref (window_icon);
	}

	/* Dim at start clipboard actions */
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/CutMenu");
	gtk_action_set_sensitive (action, FALSE);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/CopyMenu");
	gtk_action_set_sensitive (action, FALSE);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/AttachmentsMenu/RemoveAttachmentsMenu");
	gtk_action_set_sensitive (action, FALSE);

	/* Update select all */
	update_select_all_dimming (MODEST_MSG_EDIT_WINDOW (obj));
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu");
	g_signal_connect (G_OBJECT (action), "activate", G_CALLBACK (edit_menu_activated), obj);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu");
	g_signal_connect (G_OBJECT (action), "activate", G_CALLBACK (view_menu_activated), obj);

	/* set initial state of cc and bcc */
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewCcFieldMenu");
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action),
					       modest_conf_get_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_CC, NULL));
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewBccFieldMenu");
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action),
					       modest_conf_get_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_BCC, NULL));

	/* Setup the file format */
	conf = modest_runtime_get_conf ();
	prefer_formatted = modest_conf_get_bool (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, &error);
	if (error) {
		g_clear_error (&error);
		file_format = MODEST_FILE_FORMAT_FORMATTED_TEXT;
	} else
		file_format = (prefer_formatted) ? 
			MODEST_FILE_FORMAT_FORMATTED_TEXT : 
			MODEST_FILE_FORMAT_PLAIN_TEXT;
	modest_msg_edit_window_set_file_format (MODEST_MSG_EDIT_WINDOW (obj), file_format);

	update_paste_dimming (MODEST_MSG_EDIT_WINDOW (obj));
	priv->update_caption_visibility = TRUE;
	
	return (ModestWindow*) obj;
}

static gint
get_formatted_data_cb (const gchar *buffer, gpointer user_data)
{
	GString **string_buffer = (GString **) user_data;

	*string_buffer = g_string_append (*string_buffer, buffer);
   
	return 0;
}

/**
 * @result: A new string which should be freed with g_free().
 */
static gchar *
get_formatted_data (ModestMsgEditWindow *edit_window)
{
	ModestMsgEditWindowPrivate *priv;
	GString *string_buffer = g_string_new ("");
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (edit_window);

	wp_text_buffer_save_document (WP_TEXT_BUFFER(priv->text_buffer), get_formatted_data_cb, &string_buffer);

	return g_string_free (string_buffer, FALSE);
									
}

MsgData * 
modest_msg_edit_window_get_msg_data (ModestMsgEditWindow *edit_window)
{
	MsgData *data;
	const gchar *account_name;
	ModestMsgEditWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window), NULL);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (edit_window);
									
	account_name = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->from_field));
	g_return_val_if_fail (account_name, NULL);
	
	
	/* don't free these (except from) */
	data = g_slice_new0 (MsgData);
	data->from    =  modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
							     account_name);
	data->account_name = g_strdup (account_name);
	data->to      =  g_strdup (modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR (priv->to_field)));
	data->cc      =  g_strdup (modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR (priv->cc_field)));
	data->bcc     =  g_strdup (modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR (priv->bcc_field)));
	data->subject =  g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->subject_field)));
	if (priv->draft_msg) {
		data->draft_msg = g_object_ref (priv->draft_msg);
	} else {
		data->draft_msg = NULL;
	}

	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));
	GtkTextIter b, e;
	gtk_text_buffer_get_bounds (buf, &b, &e);
	data->plain_body = g_strdup (gtk_text_buffer_get_text (priv->text_buffer, &b, &e, FALSE)); /* returns a copy */

	if (wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer)))
		data->html_body = get_formatted_data (edit_window); /* returns a copy. */
	else
		data->html_body = NULL;

	/* deep-copy the data */
	GList *cursor = priv->attachments;
	data->attachments = NULL;
	while (cursor) {
		if (!(TNY_IS_MIME_PART(cursor->data))) {
			g_warning ("strange data in attachment list");
			cursor = g_list_next (cursor);
			continue;
		}
		data->attachments = g_list_append (data->attachments,
						   g_object_ref (cursor->data));
		cursor = g_list_next (cursor);
	}
	
	data->priority_flags = priv->priority_flags;

	return data;
}


static void
unref_gobject (GObject *obj, gpointer data)
{
	if (!G_IS_OBJECT(obj))
		return;
	g_object_unref (obj);
}

void 
modest_msg_edit_window_free_msg_data (ModestMsgEditWindow *edit_window,
						      MsgData *data)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window));

	if (!data)
		return;

	g_free (data->to);
	g_free (data->cc);
	g_free (data->bcc);
	g_free (data->subject);
	g_free (data->plain_body);
	g_free (data->html_body);
	g_free (data->account_name);
	
	if (data->draft_msg != NULL) {
		g_object_unref (data->draft_msg);
		data->draft_msg = NULL;
	}
	
	g_list_foreach (data->attachments, (GFunc)unref_gobject,  NULL);
	g_list_free (data->attachments);
	
	g_slice_free (MsgData, data);
}

ModestMsgEditFormat
modest_msg_edit_window_get_format (ModestMsgEditWindow *self)
{
	gboolean rich_text;
	ModestMsgEditWindowPrivate *priv = NULL;
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self), MODEST_MSG_EDIT_FORMAT_HTML);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	rich_text = wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer));
	if (rich_text)
		return MODEST_MSG_EDIT_FORMAT_HTML;
	else
		return MODEST_MSG_EDIT_FORMAT_TEXT;
}

void
modest_msg_edit_window_set_format (ModestMsgEditWindow *self,
				   ModestMsgEditFormat format)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	switch (format) {
	case MODEST_MSG_EDIT_FORMAT_HTML:
		wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
		break;
	case MODEST_MSG_EDIT_FORMAT_TEXT:
		wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), FALSE);
		break;
	default:
		g_return_if_reached ();
	}
}

ModestMsgEditFormatState *
modest_msg_edit_window_get_format_state (ModestMsgEditWindow *self)
{
	ModestMsgEditFormatState *format_state = NULL;
	ModestMsgEditWindowPrivate *priv;
	WPTextBufferFormat *buffer_format = g_new0 (WPTextBufferFormat, 1);

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self), NULL);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, TRUE);

	format_state = g_new0 (ModestMsgEditFormatState, 1);
	format_state->bold = buffer_format->bold&0x1;
	format_state->italics = buffer_format->italic&0x1;
	format_state->bullet = buffer_format->bullet&0x1;
	format_state->color = buffer_format->color;
	format_state->font_size = buffer_format->font_size;
	format_state->font_family = wp_get_font_name (buffer_format->font);
	format_state->justification = buffer_format->justification;
	g_free (buffer_format);

	return format_state;
 
}

void
modest_msg_edit_window_set_format_state (ModestMsgEditWindow *self,
					 const ModestMsgEditFormatState *format_state)
{
	ModestMsgEditWindowPrivate *priv;
	WPTextBufferFormat *buffer_format = g_new0 (WPTextBufferFormat, 1);
	WPTextBufferFormat *current_format = g_new0 (WPTextBufferFormat, 1);
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (format_state != NULL);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);
	gtk_widget_grab_focus (priv->msg_body);
	buffer_format->bold = (format_state->bold != FALSE);
	buffer_format->italic = (format_state->italics != FALSE);
	buffer_format->color = format_state->color;
	buffer_format->font_size = format_state->font_size;
	buffer_format->font = wp_get_font_index (format_state->font_family, 0);
	buffer_format->justification = format_state->justification;
	buffer_format->bullet = format_state->bullet;

	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), current_format, TRUE);

	buffer_format->cs.bold = ((buffer_format->bold&0x1) != (current_format->bold&0x1));
	buffer_format->cs.italic = ((buffer_format->italic&0x1) != (current_format->italic&0x1));
	buffer_format->cs.color = !gdk_color_equal(&(buffer_format->color), &(current_format->color));
	buffer_format->cs.font_size =  (buffer_format->font_size != current_format->font_size);
	buffer_format->cs.font = (buffer_format->font != current_format->font);
	buffer_format->cs.justification = (buffer_format->justification != current_format->justification);
	buffer_format->cs.bullet = (buffer_format->bullet != current_format->bullet);

	wp_text_buffer_freeze (WP_TEXT_BUFFER (priv->text_buffer));
	if (buffer_format->cs.bold) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_BOLD, (gpointer) (buffer_format->bold&0x1));
	}
	if (buffer_format->cs.italic) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_ITALIC, (gpointer) (buffer_format->italic&0x1));
	}
	if (buffer_format->cs.color) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FORECOLOR, (gpointer) (&(buffer_format->color)));
	}
	if (buffer_format->cs.font_size) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT_SIZE, (gpointer) (buffer_format->font_size));
	}
	if (buffer_format->cs.justification) {
		switch (buffer_format->justification) {
		case GTK_JUSTIFY_LEFT:
			wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_LEFT, (gpointer) TRUE);
			break;
		case GTK_JUSTIFY_CENTER:
			wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_CENTER, (gpointer) TRUE);
			break;
		case GTK_JUSTIFY_RIGHT:
			wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_RIGHT, (gpointer) TRUE);
			break;
		default:
			break;
		}
			
	}
	if (buffer_format->cs.font) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT, (gpointer) (buffer_format->font));
	}
	wp_text_buffer_thaw (WP_TEXT_BUFFER (priv->text_buffer));
	if (buffer_format->cs.bullet) {
	  wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_BULLET, (gpointer) ((buffer_format->bullet)?1:0));
	}
/* 	wp_text_buffer_set_format (WP_TEXT_BUFFER (priv->text_buffer), buffer_format); */

	g_free (current_format);

}

static void
text_buffer_refresh_attributes (WPTextBuffer *buffer, ModestMsgEditWindow *window)
{
	WPTextBufferFormat *buffer_format = g_new0 (WPTextBufferFormat, 1);
	GtkAction *action;
	ModestWindowPrivate *parent_priv;
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *new_size_menuitem;
	GtkWidget *new_font_menuitem;
	
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer))) {
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/FileFormatMenu/FileFormatFormattedTextMenu");
		if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
			modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), TRUE);
	} else {
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/FileFormatMenu/FileFormatPlainTextMenu");
		if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
			modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), TRUE);
	}

	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, FALSE);
	
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bold);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->italic);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/BulletedListMenu");
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bullet);

	g_signal_handlers_block_by_func (G_OBJECT (priv->font_color_button), 
					 G_CALLBACK (modest_msg_edit_window_color_button_change),
					 window);
	hildon_color_button_set_color (HILDON_COLOR_BUTTON (priv->font_color_button), & (buffer_format->color));
	g_signal_handlers_unblock_by_func (G_OBJECT (priv->font_color_button), 
					   G_CALLBACK (modest_msg_edit_window_color_button_change),
					   window);

	new_size_menuitem = GTK_WIDGET ((g_slist_nth (priv->size_items_group, 
						      buffer_format->font_size))->data);
	if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (new_size_menuitem))) {
		GtkWidget *label;
		gchar *markup;

		label = gtk_bin_get_child (GTK_BIN (new_size_menuitem));
		markup = g_strconcat ("<span font_family='Serif'>", gtk_label_get_text (GTK_LABEL (label)), "</span>", NULL);
		gtk_label_set_markup (GTK_LABEL (priv->size_tool_button_label), markup);
		g_free (markup);
		g_signal_handlers_block_by_func (G_OBJECT (new_size_menuitem),
						 G_CALLBACK (modest_msg_edit_window_size_change),
						 window);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (new_size_menuitem), TRUE);
		g_signal_handlers_unblock_by_func (G_OBJECT (new_size_menuitem),
						   G_CALLBACK (modest_msg_edit_window_size_change),
						   window);
	}

	new_font_menuitem = GTK_WIDGET ((g_slist_nth (priv->font_items_group, 
						      buffer_format->font))->data);
	if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (new_font_menuitem))) {
		GtkWidget *label;
		gchar *markup;

		label = gtk_bin_get_child (GTK_BIN (new_font_menuitem));
		markup = g_strconcat ("<span font_family='", gtk_label_get_text (GTK_LABEL (label)),"'>Tt</span>", NULL);
		gtk_label_set_markup (GTK_LABEL (priv->font_tool_button_label), markup);
		g_free (markup);
		g_signal_handlers_block_by_func (G_OBJECT (new_font_menuitem),
						 G_CALLBACK (modest_msg_edit_window_font_change),
						 window);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (new_font_menuitem), TRUE);
		g_signal_handlers_unblock_by_func (G_OBJECT (new_font_menuitem),
						   G_CALLBACK (modest_msg_edit_window_font_change),
						   window);
	}

	g_free (buffer_format);

}


void
modest_msg_edit_window_select_color (ModestMsgEditWindow *window)
{
	
	WPTextBufferFormat *buffer_format = g_new0 (WPTextBufferFormat, 1);
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;
	gint response;
	const GdkColor *new_color = NULL;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, FALSE);
	
#ifdef MODEST_HILDON_VERSION_0	
	dialog = hildon_color_selector_new (GTK_WINDOW (window));
	hildon_color_selector_set_color (HILDON_COLOR_SELECTOR (dialog), &(buffer_format->color));
#else
	dialog = hildon_color_chooser_new ();
	hildon_color_chooser_set_color (HILDON_COLOR_CHOOSER (dialog), &(buffer_format->color));
#endif /*MODEST_HILDON_VERSION_0*/		
	g_free (buffer_format);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK: {
#ifdef MODEST_HILDON_VERSION_0
		new_color = hildon_color_selector_get_color (HILDON_COLOR_SELECTOR (dialog));
#else
		GdkColor col;
		hildon_color_chooser_get_color (HILDON_COLOR_CHOOSER(dialog), &col);
		new_color = &col;
#endif /*MODEST_HILDON_VERSION_0*/
	}

	break;
	default:
		break;
	}
	gtk_widget_destroy (dialog);

	if (new_color != NULL)
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FORECOLOR, (gpointer) new_color);

}

void
modest_msg_edit_window_select_background_color (ModestMsgEditWindow *window)
{
	
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;
	gint response;
	GdkColor *old_color = NULL;
	const GdkColor *new_color = NULL;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	old_color = (GdkColor*)wp_text_buffer_get_background_color (WP_TEXT_BUFFER (priv->text_buffer));
	
#ifdef MODEST_HILDON_VERSION_0	
	dialog = hildon_color_selector_new (GTK_WINDOW (window));
	hildon_color_selector_set_color (HILDON_COLOR_SELECTOR (dialog),(GdkColor*)old_color);
#else
	dialog = hildon_color_chooser_new ();
	hildon_color_chooser_set_color (HILDON_COLOR_CHOOSER (dialog),(GdkColor*)old_color);
#endif /*MODEST_HILDON_VERSION_9*/		

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK: {
#ifdef MODEST_HILDON_VERSION_0
		new_color = hildon_color_selector_get_color (HILDON_COLOR_SELECTOR (dialog));
#else
		GdkColor col;
		hildon_color_chooser_get_color (HILDON_COLOR_CHOOSER(dialog), &col);
		new_color = &col;
#endif /*MODEST_HILDON_VERSION_0*/
          }
		break;
	default:
		break;
	}
	gtk_widget_destroy (dialog);

	if (new_color != NULL)
		wp_text_buffer_set_background_color (WP_TEXT_BUFFER (priv->text_buffer), new_color);

}

static TnyStream* create_stream_for_uri (const gchar* uri)
{
	if (!uri)
		return NULL;
		
	TnyStream *result = NULL;

	GnomeVFSHandle *handle = NULL;
	GnomeVFSResult test = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	if (test == GNOME_VFS_OK) {
		/* Create the tinymail stream: */
		/* Presumably tinymai will call gnome_vfs_close (handle) later. */
		result = TNY_STREAM (tny_vfs_stream_new (handle));
	}
	
	return result;
}

void
modest_msg_edit_window_insert_image (ModestMsgEditWindow *window)
{
	
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;
	gint response = 0;
	GSList *uris = NULL;
	GSList *uri_node = NULL;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_window_set_title (GTK_WINDOW (dialog), _("mcen_ia_select_inline_image_title"));
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

	modest_maemo_utils_setup_images_filechooser (GTK_FILE_CHOOSER (dialog));

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK:
		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
		break;
	default:
		break;
	}
	gtk_widget_destroy (dialog);

	for (uri_node = uris; uri_node != NULL; uri_node = g_slist_next (uri_node)) {
		const gchar *uri;
		GnomeVFSHandle *handle = NULL;
		GnomeVFSResult result;
		GtkTextIter position;
		GtkTextMark *insert_mark;

		uri = (const gchar *) uri_node->data;
		result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
		if (result == GNOME_VFS_OK) {
			GdkPixbuf *pixbuf;
			GnomeVFSFileInfo info;
			gchar *filename, *basename, *escaped_filename;
			TnyMimePart *mime_part;
			gchar *content_id;
			const gchar *mime_type = NULL;
			GnomeVFSURI *vfs_uri;

			vfs_uri = gnome_vfs_uri_new (uri);

			escaped_filename = g_path_get_basename (gnome_vfs_uri_get_path (vfs_uri));
			filename = gnome_vfs_unescape_string_for_display (escaped_filename);
			g_free (escaped_filename);
			gnome_vfs_uri_unref (vfs_uri);

			if (gnome_vfs_get_file_info (uri, &info, GNOME_VFS_FILE_INFO_GET_MIME_TYPE
						     | GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE) 
			    == GNOME_VFS_OK)
				mime_type = gnome_vfs_file_info_get_mime_type (&info);

			mime_part = tny_platform_factory_new_mime_part
				(modest_runtime_get_platform_factory ());
				
			TnyStream *stream = create_stream_for_uri (uri);
			tny_mime_part_construct_from_stream (mime_part, stream, mime_type);
			
			content_id = g_strdup_printf ("%d", priv->last_cid);
			tny_mime_part_set_content_id (mime_part, content_id);
			g_free (content_id);
			priv->last_cid++;
			
			basename = g_path_get_basename (filename);
			tny_mime_part_set_filename (mime_part, basename);
			g_free (basename);

			pixbuf = pixbuf_from_stream (stream, mime_type);
			
			if (pixbuf != NULL) {
				insert_mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (priv->text_buffer));
				gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (priv->text_buffer), &position, insert_mark);
				wp_text_buffer_insert_image (WP_TEXT_BUFFER (priv->text_buffer), &position, g_strdup (tny_mime_part_get_content_id (mime_part)), pixbuf);
			} 

			priv->attachments = g_list_prepend (priv->attachments, mime_part);
			modest_attachments_view_add_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
								mime_part);
			gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
			gtk_widget_show_all (priv->attachments_caption);
			gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
			g_free (filename);

		}
	}


}

void
modest_msg_edit_window_offer_attach_file (ModestMsgEditWindow *window)
{
	
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;
	gint response = 0;
	GSList *uris = NULL;
	GSList *uri_node;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_window_set_title (GTK_WINDOW (dialog), _("mcen_ti_select_attachment_title"));
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK:
		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
		break;
	default:
		break;
	}
	gtk_widget_destroy (dialog);

	for (uri_node = uris; uri_node != NULL; uri_node = g_slist_next (uri_node)) {
		const gchar *uri = (const gchar *) uri_node->data;
		modest_msg_edit_window_attach_file_one (window, uri);
	}
	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);
}

void
modest_msg_edit_window_attach_file_one (
		ModestMsgEditWindow *window,
		const gchar *uri)
{
	g_return_if_fail (window);
	g_return_if_fail (uri);
		
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	
	GnomeVFSHandle *handle = NULL;
	GnomeVFSResult result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	if (result == GNOME_VFS_OK) {
		TnyMimePart *mime_part;
		TnyStream *stream;
		const gchar *mime_type = NULL;
		gchar *basename;
		gchar *escaped_filename;
		gchar *filename;
		gchar *content_id;
		GnomeVFSFileInfo info;
		GnomeVFSURI *vfs_uri;

		vfs_uri = gnome_vfs_uri_new (uri);
		

		escaped_filename = g_path_get_basename (gnome_vfs_uri_get_path (vfs_uri));
		filename = gnome_vfs_unescape_string_for_display (escaped_filename);
		g_free (escaped_filename);
		gnome_vfs_uri_unref (vfs_uri);
		
		if (gnome_vfs_get_file_info (uri, 
					     &info, 
					     GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					     GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE)
		    == GNOME_VFS_OK)
			mime_type = gnome_vfs_file_info_get_mime_type (&info);
		mime_part = tny_platform_factory_new_mime_part
			(modest_runtime_get_platform_factory ());
		stream = TNY_STREAM (tny_vfs_stream_new (handle));
		
		tny_mime_part_construct_from_stream (mime_part, stream, mime_type);
		
		content_id = g_strdup_printf ("%d", priv->last_cid);
		tny_mime_part_set_content_id (mime_part, content_id);
		g_free (content_id);
		priv->last_cid++;
		
		basename = g_path_get_basename (filename);
		tny_mime_part_set_filename (mime_part, basename);
		g_free (basename);
		
		priv->attachments = g_list_prepend (priv->attachments, mime_part);
		modest_attachments_view_add_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
							mime_part);
		gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
		gtk_widget_show_all (priv->attachments_caption);
		gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
		g_free (filename);
	}
}

void
modest_msg_edit_window_remove_attachments (ModestMsgEditWindow *window,
					  GList *att_list)
{
	ModestMsgEditWindowPrivate *priv;
	gboolean clean_list = FALSE;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (att_list == NULL) {
		att_list = modest_attachments_view_get_selection (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
		clean_list = TRUE;
	}

	if (att_list == NULL) {
		hildon_banner_show_information (NULL, NULL, _("TODO: no attachments selected to remove"));
	} else {
		GtkWidget *confirmation_dialog = NULL;
		gboolean dialog_response;
		GList *node;
		gchar *message = NULL;
		const gchar *filename = NULL;

		if (att_list->next == NULL) {
			filename = tny_mime_part_get_filename (TNY_MIME_PART (att_list->data));
		} else {
			filename = "";
		}
		message = g_strdup_printf (ngettext("emev_nc_delete_attachment", "emev_nc_delete_attachments",
						    att_list->next == NULL), filename);
		confirmation_dialog = hildon_note_new_confirmation (GTK_WINDOW (window), message);
		g_free (message);
		dialog_response = (gtk_dialog_run (GTK_DIALOG (confirmation_dialog))==GTK_RESPONSE_OK);
		gtk_widget_destroy (confirmation_dialog);
		if (!dialog_response) {
			if (clean_list)
				g_list_free (att_list);
			return;
		}
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_removing_attachment"));

		for (node = att_list; node != NULL; node = g_list_next (node)) {
			TnyMimePart *mime_part = (TnyMimePart *) node->data;
			const gchar *att_id;
			priv->attachments = g_list_remove (priv->attachments, mime_part);

			modest_attachments_view_remove_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
								   mime_part);
			if (priv->attachments == NULL)
				gtk_widget_hide (priv->attachments_caption);
			att_id = tny_mime_part_get_content_id (mime_part);
			if (att_id != NULL)
				text_buffer_delete_images_by_id (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body)),
								 att_id);
			g_object_unref (mime_part);
			gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
		}
	}

	if (clean_list)
		g_list_free (att_list);
}

static void
modest_msg_edit_window_color_button_change (ModestMsgEditWindow *window,
					    gpointer userdata)
{
	ModestMsgEditWindowPrivate *priv;
	GdkColor *new_color;
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
#ifdef MODEST_HAVE_HILDON0_WIDGETS	
	new_color = hildon_color_button_get_color (HILDON_COLOR_BUTTON (priv->font_color_button));
#else 
	GdkColor col;
	hildon_color_button_get_color (HILDON_COLOR_BUTTON(priv->font_color_button), &col);
	new_color = &col;
#endif /*#ifdef MODEST_HAVE_HILDON0_WIDGETS*/

	wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FORECOLOR, (gpointer) new_color);
	
	gtk_window_set_focus (GTK_WINDOW (window), priv->msg_body);

}

static void
modest_msg_edit_window_size_change (GtkCheckMenuItem *menu_item,
				    gpointer userdata)
{
	ModestMsgEditWindowPrivate *priv;
	gint new_size_index;
	ModestMsgEditWindow *window;
	GtkWidget *label;
	
	window = MODEST_MSG_EDIT_WINDOW (userdata);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	gtk_widget_grab_focus (GTK_WIDGET (priv->msg_body));

	if (gtk_check_menu_item_get_active (menu_item)) {
		gchar *markup;
		WPTextBufferFormat format;

		memset (&format, 0, sizeof (format));
		wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), &format, FALSE);

		label = gtk_bin_get_child (GTK_BIN (menu_item));
		
		new_size_index = atoi (gtk_label_get_text (GTK_LABEL (label)));
		format.cs.font_size = TRUE;
		format.cs.text_position = TRUE;
		format.cs.font = TRUE;
		format.font_size = wp_get_font_size_index (new_size_index, DEFAULT_FONT_SIZE);
/* 		wp_text_buffer_set_format (WP_TEXT_BUFFER (priv->text_buffer), &format); */

		if (!wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT_SIZE,
						   (gpointer) wp_get_font_size_index (new_size_index, 12)))
			wp_text_view_reset_and_show_im (WP_TEXT_VIEW (priv->msg_body));
		
		text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (window));
		markup = g_strconcat ("<span font_family='", DEFAULT_SIZE_BUTTON_FONT_FAMILY, "'>", gtk_label_get_text (GTK_LABEL (label)), "</span>", NULL);
		gtk_label_set_markup (GTK_LABEL (priv->size_tool_button_label), markup);
		g_free (markup);
	}
}

static void
modest_msg_edit_window_font_change (GtkCheckMenuItem *menu_item,
				    gpointer userdata)
{
	ModestMsgEditWindowPrivate *priv;
	gint new_font_index;
	ModestMsgEditWindow *window;
	GtkWidget *label;
	
	window = MODEST_MSG_EDIT_WINDOW (userdata);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	gtk_widget_grab_focus (GTK_WIDGET (priv->msg_body));

	if (gtk_check_menu_item_get_active (menu_item)) {
		gchar *markup;

		label = gtk_bin_get_child (GTK_BIN (menu_item));
		
		new_font_index = wp_get_font_index (gtk_label_get_text (GTK_LABEL (label)), DEFAULT_FONT);

		if (!wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT, 
						   (gpointer) new_font_index))
			wp_text_view_reset_and_show_im (WP_TEXT_VIEW (priv->msg_body));
		
		text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (window));
		    markup = g_strconcat ("<span font_family='",gtk_label_get_text (GTK_LABEL (label)),"'>Tt</span>", NULL);
		gtk_label_set_markup (GTK_LABEL (priv->font_tool_button_label), markup);
		g_free (markup);
	}
}

static void
modest_msg_edit_window_set_zoom (ModestWindow *window,
				 gdouble zoom)
{
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkRadioAction *zoom_radio_action;
     
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	priv->zoom_level = zoom;
	wp_text_buffer_set_font_scaling_factor (WP_TEXT_BUFFER (priv->text_buffer), zoom*DEFAULT_FONT_SCALE);

	/* Zoom level menu options should be updated with the current zoom level */
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	zoom_radio_action = GTK_RADIO_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, 
									 "/MenuBar/ViewMenu/ZoomMenu/Zoom50Menu"));
#ifdef MODEST_HAVE_HILDON0_WIDGETS
	/* FIXME: Not availible before Gtk 2.10 */
#else
	gtk_radio_action_set_current_value (zoom_radio_action, (gint) (zoom*100.0+0.1));
#endif
}

static gdouble
modest_msg_edit_window_get_zoom (ModestWindow *window)
{
	ModestMsgEditWindowPrivate *priv;
     
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), 1.0);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	return priv->zoom_level;
}

static gboolean
zoom_allowed (ModestMsgEditWindow *window)
{
	GtkWidget *focus;

	focus = gtk_window_get_focus (GTK_WINDOW (window));
	return (focus != NULL && WP_IS_TEXT_VIEW (focus));
}

static gboolean
modest_msg_edit_window_zoom_plus (ModestWindow *window)
{
	ModestWindowPrivate *parent_priv;
	GtkRadioAction *zoom_radio_action;
	GSList *group, *node;

	/* First we check if the text view is focused. If not, zooming is not allowed */
	if (!zoom_allowed (MODEST_MSG_EDIT_WINDOW (window))) {
		hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_cannot_zoom_here"));
		return FALSE;
	}

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	zoom_radio_action = GTK_RADIO_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, 
									 "/MenuBar/ViewMenu/ZoomMenu/Zoom50Menu"));

	group = gtk_radio_action_get_group (zoom_radio_action);

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (group->data))) {
		hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_max_zoom_level_reached"));
		return FALSE;
	}

	for (node = group; node != NULL; node = g_slist_next (node)) {
		if ((node->next != NULL) && gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (node->next->data))) {
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (node->data), TRUE);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
modest_msg_edit_window_zoom_minus (ModestWindow *window)
{
	ModestWindowPrivate *parent_priv;
	GtkRadioAction *zoom_radio_action;
	GSList *group, *node;

	/* First we check if the text view is focused. If not, zooming is not allowed */
	if (!zoom_allowed (MODEST_MSG_EDIT_WINDOW (window))) {
		hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_cannot_zoom_here"));
		return FALSE;
	}

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	zoom_radio_action = GTK_RADIO_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, 
									 "/MenuBar/ViewMenu/ZoomMenu/Zoom50Menu"));

	group = gtk_radio_action_get_group (zoom_radio_action);

	for (node = group; node != NULL; node = g_slist_next (node)) {
		if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (node->data))) {
			if (node->next != NULL) {
				gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (node->next->data), TRUE);
				return TRUE;
			} else
				hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_min_zoom_level_reached"));
			break;
		}
	}
	return FALSE;
}

static gboolean
modest_msg_edit_window_window_state_event (GtkWidget *widget, GdkEventWindowState *event, gpointer userdata)
{
	if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
		ModestWindowPrivate *parent_priv;
		ModestWindowMgr *mgr;
		gboolean is_fullscreen;
		GtkAction *fs_toggle_action;
		gboolean active;

		mgr = modest_runtime_get_window_mgr ();
		is_fullscreen = (modest_window_mgr_get_fullscreen_mode (mgr))?1:0;

		parent_priv = MODEST_WINDOW_GET_PRIVATE (widget);
		
		fs_toggle_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewToggleFullscreenMenu");
		active = (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (fs_toggle_action)))?1:0;
		if (is_fullscreen != active)
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fs_toggle_action), is_fullscreen);
	}

	return FALSE;

}

void
modest_msg_edit_window_toggle_fullscreen (ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv;
	GtkAction *fs_toggle_action;
	gboolean active;

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);

	fs_toggle_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewToggleFullscreenMenu");
	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (fs_toggle_action));
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fs_toggle_action), !active);
}

void
modest_msg_edit_window_show_cc (ModestMsgEditWindow *window, 
				gboolean show)
{
	ModestMsgEditWindowPrivate *priv = NULL;
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	if (!priv->update_caption_visibility)
		return;

	gtk_widget_set_no_show_all (priv->cc_caption, TRUE);
	if (show)
		gtk_widget_show (priv->cc_caption);
	else
		gtk_widget_hide (priv->cc_caption);

	modest_conf_set_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_CC, show, NULL);
}

void
modest_msg_edit_window_show_bcc (ModestMsgEditWindow *window, 
				 gboolean show)
{
	ModestMsgEditWindowPrivate *priv = NULL;
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	if (!priv->update_caption_visibility)
		return;

	gtk_widget_set_no_show_all (priv->bcc_caption, TRUE);
	if (show)
		gtk_widget_show (priv->bcc_caption);
	else
		gtk_widget_hide (priv->bcc_caption);

	modest_conf_set_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_BCC, show, NULL);
}

static void
modest_msg_edit_window_open_addressbook (ModestMsgEditWindow *window,
					 ModestRecptEditor *editor)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail ((editor == NULL) || (MODEST_IS_RECPT_EDITOR (editor)));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (editor == NULL) {
		GtkWidget *view_focus;
		view_focus = gtk_window_get_focus (GTK_WINDOW (window));

		/* This code should be kept in sync with ModestRecptEditor. The
		   textview inside the recpt editor is the one that really gets the
		   focus. As it's inside a scrolled window, and this one inside the
		   hbox recpt editor inherits from, we'll need to go up in the 
		   hierarchy to know if the text view is part of the recpt editor
		   or if it's a different text entry */

		if (gtk_widget_get_parent (view_focus)) {
			GtkWidget *first_parent;

			first_parent = gtk_widget_get_parent (view_focus);
			if (gtk_widget_get_parent (first_parent) && 
			    MODEST_IS_RECPT_EDITOR (gtk_widget_get_parent (first_parent))) {
				editor = MODEST_RECPT_EDITOR (gtk_widget_get_parent (first_parent));
			}
		}

		if (editor == NULL)
			editor = MODEST_RECPT_EDITOR (priv->to_field);

	}

	modest_address_book_select_addresses (editor);

}

void
modest_msg_edit_window_select_contacts (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	modest_msg_edit_window_open_addressbook (window, NULL);
}

static void
modest_msg_edit_window_show_toolbar (ModestWindow *self,
				     gboolean show_toolbar)
{
	ModestWindowPrivate *parent_priv;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	/* FIXME: we can not just use the code of
	   modest_msg_edit_window_setup_toolbar because it has a
	   mixture of both initialization and creation code. */

	if (show_toolbar)
		gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
	else
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
}

void
modest_msg_edit_window_set_priority_flags (ModestMsgEditWindow *window,
					   TnyHeaderFlags priority_flags)
{
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	priority_flags = priority_flags & (TNY_HEADER_FLAG_PRIORITY);

	if (priv->priority_flags != priority_flags) {
		GtkAction *priority_action = NULL;

		priv->priority_flags = priority_flags;

		switch (priority_flags) {
		case TNY_HEADER_FLAG_HIGH_PRIORITY:
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon), "qgn_list_messaging_high", GTK_ICON_SIZE_MENU);
			gtk_widget_show (priv->priority_icon);
			priority_action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
								     "/MenuBar/ToolsMenu/MessagePriorityMenu/MessagePriorityHighMenu");
			break;
		case TNY_HEADER_FLAG_LOW_PRIORITY:
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon), "qgn_list_messaging_low", GTK_ICON_SIZE_MENU);
			gtk_widget_show (priv->priority_icon);
			priority_action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
								     "/MenuBar/ToolsMenu/MessagePriorityMenu/MessagePriorityLowMenu");
			break;
		default:
			gtk_widget_hide (priv->priority_icon);
			priority_action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
								     "/MenuBar/ToolsMenu/MessagePriorityMenu/MessagePriorityNormalMenu");
			break;
		}
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (priority_action), TRUE);
		gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
	}
}

void
modest_msg_edit_window_set_file_format (ModestMsgEditWindow *window,
					gint file_format)
{
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	gint current_format;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	current_format = wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer))
		? MODEST_FILE_FORMAT_FORMATTED_TEXT : MODEST_FILE_FORMAT_PLAIN_TEXT;

	if (current_format != file_format) {
		switch (file_format) {
		case MODEST_FILE_FORMAT_FORMATTED_TEXT:
			wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
			break;
		case MODEST_FILE_FORMAT_PLAIN_TEXT:
		{
			GtkWidget *dialog;
			gint response;
			dialog = hildon_note_new_confirmation (NULL, _("emev_nc_formatting_lost"));
			response = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			if (response == GTK_RESPONSE_OK) {
				wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), FALSE);
			} else {
				GtkToggleAction *action = GTK_TOGGLE_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/FileFormatMenu/FileFormatFormattedTextMenu"));
				modest_maemo_toggle_action_set_active_block_notify (action, TRUE);
			}
		}
			break;
		}
		update_dimmed (window);
	}
}

void
modest_msg_edit_window_select_font (ModestMsgEditWindow *window)
{
	GtkWidget *dialog;
	ModestMsgEditWindowPrivate *priv;
	WPTextBufferFormat oldfmt, fmt;
	gint old_position = 0;
	gint response = 0;
	gint position = 0;
	gint font_size;
	GdkColor *color = NULL;
	gboolean bold, bold_set, italic, italic_set;
	gboolean underline, underline_set;
	gboolean strikethrough, strikethrough_set;
	gboolean position_set;
	gboolean font_size_set, font_set, color_set;
	gchar *font_name;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	dialog = hildon_font_selection_dialog_new (GTK_WINDOW (window), NULL);

	/* First we get the currently selected font information */
	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), &oldfmt, TRUE);
	g_object_set (G_OBJECT (dialog), "font-scaling", priv->zoom_level, NULL);

	switch (oldfmt.text_position) {
	case TEXT_POSITION_NORMAL:
		old_position = 0;
		break;
	case TEXT_POSITION_SUPERSCRIPT:
		old_position = 1;
		break;
	default:
		old_position = -1;
		break;
	}

	g_object_set (G_OBJECT (dialog),
		      "bold", oldfmt.bold != FALSE,
		      "bold-set", !oldfmt.cs.bold,
		      "underline", oldfmt.underline != FALSE,
		      "underline-set", !oldfmt.cs.underline,
		      "italic", oldfmt.italic != FALSE,
		      "italic-set", !oldfmt.cs.italic,
		      "strikethrough", oldfmt.strikethrough != FALSE,
		      "strikethrough-set", !oldfmt.cs.strikethrough,
		      "color", &oldfmt.color,
		      "color-set", !oldfmt.cs.color,
		      "size", wp_font_size[oldfmt.font_size],
		      "size-set", !oldfmt.cs.font_size,
		      "position", old_position,
		      "position-set", !oldfmt.cs.text_position,
		      "family", wp_get_font_name (oldfmt.font),
		      "family-set", !oldfmt.cs.font,
		      NULL);

	gtk_widget_show_all (dialog);
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response == GTK_RESPONSE_OK) {

		g_object_get( dialog,
			      "bold", &bold,
			      "bold-set", &bold_set,
			      "underline", &underline,
			      "underline-set", &underline_set,
			      "italic", &italic,
			      "italic-set", &italic_set,
			      "strikethrough", &strikethrough,
			      "strikethrough-set", &strikethrough_set,
			      "color", &color,
			      "color-set", &color_set,
			      "size", &font_size,
			      "size-set", &font_size_set,
			      "family", &font_name,
			      "family-set", &font_set,
			      "position", &position,
			      "position-set", &position_set,
			      NULL );
		
	}	

	if (response == GTK_RESPONSE_OK) {
		memset(&fmt, 0, sizeof(fmt));
		if (bold_set) {
			fmt.bold = bold;
			fmt.cs.bold = TRUE;
		}
		if (italic_set) {
			fmt.italic = italic;
			fmt.cs.italic = TRUE;
		}
		if (underline_set) {
			fmt.underline = underline;
			fmt.cs.underline = TRUE;
		}
		if (strikethrough_set) {
			fmt.strikethrough = strikethrough;
			fmt.cs.strikethrough = TRUE;
		}
		if (position_set) {
			fmt.text_position =
				( position == 0 )
				? TEXT_POSITION_NORMAL
				: ( ( position == 1 )
				    ? TEXT_POSITION_SUPERSCRIPT
				    : TEXT_POSITION_SUBSCRIPT );
			fmt.cs.text_position = TRUE;
		}
		if (color_set) {
			fmt.color = *color;
			fmt.cs.color = TRUE;
		}
		if (font_set) {
			fmt.font = wp_get_font_index(font_name,
						     DEFAULT_FONT);
			fmt.cs.font = TRUE;
		}
		g_free(font_name);
		if (font_size_set) {
			fmt.font_size = wp_get_font_size_index(
				font_size, DEFAULT_FONT_SIZE);
			fmt.cs.font_size = TRUE;
		}
		wp_text_buffer_set_format(WP_TEXT_BUFFER(priv->text_buffer), &fmt);
	}
	gtk_widget_destroy (dialog);
	
	gtk_widget_grab_focus(GTK_WIDGET(priv->msg_body));
}

void
modest_msg_edit_window_undo (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	wp_text_buffer_undo (WP_TEXT_BUFFER (priv->text_buffer));

	update_dimmed (window);

}

void
modest_msg_edit_window_redo (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	wp_text_buffer_redo (WP_TEXT_BUFFER (priv->text_buffer));

	update_dimmed (window);

}

static void
update_dimmed (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	GtkAction *action;
	GtkWidget *widget;
	gboolean rich_text;
	gboolean editor_focused;

	rich_text = wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer));
	editor_focused = gtk_widget_is_focus (priv->msg_body);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/SelectFontMenu");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/BulletedListMenu");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentLeftMenu");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentCenterMenu");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentRightMenu");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/AttachmentsMenu/InsertImageMenu");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	gtk_action_set_sensitive (action, rich_text && editor_focused);
	widget = priv->font_color_button;
	gtk_widget_set_sensitive (widget, rich_text && editor_focused);
	widget = priv->font_size_toolitem;
	gtk_widget_set_sensitive (widget, rich_text && editor_focused);
	widget = priv->font_face_toolitem;
	gtk_widget_set_sensitive (widget, rich_text && editor_focused);
}

static void
setup_insensitive_handlers (ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	GtkWidget *widget;

	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSend");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (send_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = priv->font_color_button;
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = priv->font_size_toolitem;
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = priv->font_face_toolitem;
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);

}

static void  
text_buffer_can_undo (GtkTextBuffer *buffer, gboolean can_undo, ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	GtkAction *action;

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/UndoMenu");
	gtk_action_set_sensitive (action, can_undo);
}

static void  
text_buffer_can_redo (GtkTextBuffer *buffer, gboolean can_redo, ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	GtkAction *action;

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/RedoMenu");
	gtk_action_set_sensitive (action, can_redo);
}

static void
text_buffer_delete_images_by_id (GtkTextBuffer *buffer, const gchar * image_id)
{
	GtkTextIter iter;
	GtkTextIter match_start, match_end;

	if (image_id == NULL)
		return;

	gtk_text_buffer_get_start_iter (buffer, &iter);

	while (gtk_text_iter_forward_search (&iter, "\xef\xbf\xbc", 0, &match_start, &match_end, NULL)) {
		GSList *tags = gtk_text_iter_get_tags (&match_start);
		GSList *node;
		for (node = tags; node != NULL; node = g_slist_next (node)) {
			GtkTextTag *tag = (GtkTextTag *) node->data;
			if (g_object_get_data (G_OBJECT (tag), "image-set") != NULL) {
				gchar *cur_image_id = g_object_get_data (G_OBJECT (tag), "image-index");
				if ((cur_image_id != NULL) && (strcmp (image_id, cur_image_id)==0)) {
					gint offset;
					offset = gtk_text_iter_get_offset (&match_start);
					gtk_text_buffer_delete (buffer, &match_start, &match_end);
					gtk_text_buffer_get_iter_at_offset (buffer, &iter, offset);
				}
			}
		}
		gtk_text_iter_forward_char (&iter);
	}
}

static gboolean
msg_body_focus (GtkWidget *focus,
		GdkEventFocus *event,
		gpointer userdata)
{
	
	update_dimmed (MODEST_MSG_EDIT_WINDOW (userdata));
	return FALSE;
}

static void
recpt_field_changed (GtkTextBuffer *buffer,
		  ModestMsgEditWindow *editor)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (editor);
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (editor);
	GtkTextBuffer *to_buffer, *cc_buffer, *bcc_buffer;
	gboolean dim = FALSE;
	GtkAction *action;

	to_buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->to_field));
	cc_buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->cc_field));
	bcc_buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->bcc_field));
	
	dim = ((gtk_text_buffer_get_char_count (to_buffer) + 
		gtk_text_buffer_get_char_count (cc_buffer) +
		gtk_text_buffer_get_char_count (bcc_buffer)) == 0);
			
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarSend");
	gtk_action_set_sensitive (action, !dim);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EmailMenu/SendMenu");
	gtk_action_set_sensitive (action, !dim);
}

static void  
send_insensitive_press (GtkWidget *widget, ModestMsgEditWindow *editor)
{
	hildon_banner_show_information (NULL, NULL, _("mcen_ib_add_recipients_first"));
}

static void
style_insensitive_press (GtkWidget *widget, ModestMsgEditWindow *editor)
{
	gboolean rich_text, editor_focused;

	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (editor);
	rich_text = wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer));
	editor_focused = gtk_widget_is_focus (priv->msg_body);

	if (!rich_text)
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_item_unavailable_plaintext"));
	else if (!editor_focused)
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_move_cursor_to_message"));
}

static void
reset_modified (ModestMsgEditWindow *editor)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (editor);
	GtkTextBuffer *buffer;

	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->to_field));
	gtk_text_buffer_set_modified (buffer, FALSE);
	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->cc_field));
	gtk_text_buffer_set_modified (buffer, FALSE);
	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->bcc_field));
	gtk_text_buffer_set_modified (buffer, FALSE);
	gtk_text_buffer_set_modified (priv->text_buffer, FALSE);
}

gboolean
modest_msg_edit_window_is_modified (ModestMsgEditWindow *editor)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (editor);
	GtkTextBuffer *buffer;

	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->to_field));
	if (gtk_text_buffer_get_modified (buffer))
		return TRUE;
	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->cc_field));
	if (gtk_text_buffer_get_modified (buffer))
		return TRUE;
	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->bcc_field));
	if (gtk_text_buffer_get_modified (buffer))
		return TRUE;
	if (gtk_text_buffer_get_modified (priv->text_buffer))
		return TRUE;

	return FALSE;
}

gboolean
modest_msg_edit_window_check_names (ModestMsgEditWindow *window, gboolean add_to_addressbook)
{
	ModestMsgEditWindowPrivate *priv = NULL;
	
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), FALSE);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	/* check if there's no recipient added */
	if ((gtk_text_buffer_get_char_count (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->to_field))) == 0) &&
	    (gtk_text_buffer_get_char_count (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->cc_field))) == 0) &&
	    (gtk_text_buffer_get_char_count (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->bcc_field))) == 0)) {
		/* no recipient contents, then select contacts */
		modest_msg_edit_window_open_addressbook (window, NULL);
		return FALSE;
	}

	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->to_field),  add_to_addressbook))
		return FALSE;
	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->cc_field),  add_to_addressbook))
		return FALSE;
	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->bcc_field), add_to_addressbook))
		return FALSE;

	modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->to_field));

	return TRUE;

}

static void
modest_msg_edit_window_add_attachment_clicked (GtkButton *button,
					       ModestMsgEditWindow *window)
{
	modest_msg_edit_window_offer_attach_file (window);
}

static void
modest_msg_edit_window_clipboard_owner_change (GtkClipboard *clipboard,
					       GdkEvent *event,
					       ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv;
	ModestMsgEditWindowPrivate *priv;
	GtkAction *action;
	gboolean has_selection;
	GtkWidget *focused;
	GList *selected_attachments = NULL;
	gint n_att_selected = 0;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);

	if (!GTK_WIDGET_VISIBLE (window))
		return;
	has_selection = gtk_clipboard_wait_for_targets (clipboard, NULL, NULL);
	focused = gtk_window_get_focus (GTK_WINDOW (window));

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/CutMenu");
	gtk_action_set_sensitive (action, (has_selection) && (!MODEST_IS_ATTACHMENTS_VIEW (focused)));
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/CopyMenu");
	gtk_action_set_sensitive (action, (has_selection) && (!MODEST_IS_ATTACHMENTS_VIEW (focused)));

	selected_attachments = modest_attachments_view_get_selection (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	n_att_selected = g_list_length (selected_attachments);
	g_list_free (selected_attachments);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/AttachmentsMenu/RemoveAttachmentsMenu");
	gtk_action_set_sensitive (action, n_att_selected == 1);
	
	update_paste_dimming (window);
}

static void 
update_window_title (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = NULL;
	const gchar *subject;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	subject = gtk_entry_get_text (GTK_ENTRY (priv->subject_field));
	if (subject == NULL || subject[0] == '\0')
		subject = _("mail_va_new_email");

	gtk_window_set_title (GTK_WINDOW (window), subject);

}

static void  
subject_field_changed (GtkEditable *editable, 
		       ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	update_window_title (window);
	gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
}

gboolean
message_is_empty (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), FALSE);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	/** TODO: Add wpeditor API to tell us if there is any _visible_ text,
	 * so we can ignore markup.
	 */
	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));
	gint count = 0;
	if (buf)
		count = gtk_text_buffer_get_char_count (buf);
		
	return count == 0;
}
	
void
modest_msg_edit_window_toggle_find_toolbar (ModestMsgEditWindow *window,
					    gboolean show)
{
	ModestMsgEditWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	gtk_widget_set_no_show_all (priv->find_toolbar, FALSE);

	if (show) {
		gtk_widget_show_all (priv->find_toolbar);
		hildon_find_toolbar_highlight_entry (HILDON_FIND_TOOLBAR (priv->find_toolbar), TRUE);
	} else {
		gtk_widget_hide_all (priv->find_toolbar);
		gtk_widget_grab_focus (priv->msg_body);
	}
    
}

static gboolean 
gtk_text_iter_forward_search_insensitive (const GtkTextIter *iter,
					  const gchar *str,
					  GtkTextIter *match_start,
					  GtkTextIter *match_end)
{
	GtkTextIter end_iter;
	gchar *str_casefold;
	gint str_chars_n;
	gchar *range_text;
	gchar *range_casefold;
	gint offset;
	gint range_chars_n;
	gboolean result = FALSE;

	if (str == NULL)
		return TRUE;
	
	/* get end iter */
	end_iter = *iter;
	gtk_text_iter_forward_to_end (&end_iter);

	str_casefold = g_utf8_casefold (str, -1);
	str_chars_n = strlen (str);

	range_text = gtk_text_iter_get_visible_text (iter, &end_iter);
	range_casefold = g_utf8_casefold (range_text, -1);
	range_chars_n = strlen (range_casefold);

	if (range_chars_n < str_chars_n) {
		g_free (str_casefold);
		g_free (range_text);
		g_free (range_casefold);
		return FALSE;
	}

	for (offset = 0; offset <= range_chars_n - str_chars_n; offset++) {
		gchar *range_subtext = g_strndup (range_casefold + offset, str_chars_n);
		if (!g_utf8_collate (range_subtext, str_casefold)) {
			gchar *found_text = g_strndup (range_text + offset, str_chars_n);
			result = TRUE;
			gtk_text_iter_forward_search (iter, found_text, GTK_TEXT_SEARCH_VISIBLE_ONLY|GTK_TEXT_SEARCH_TEXT_ONLY,
						      match_start, match_end, NULL);
			g_free (found_text);
		}
		g_free (range_subtext);
		if (result)
			break;
	}
	g_free (str_casefold);
	g_free (range_text);
	g_free (range_casefold);

	return result;
}


static void 
modest_msg_edit_window_find_toolbar_search (GtkWidget *widget,
					    ModestMsgEditWindow *window)
{
	gchar *current_search = NULL;
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	gboolean result;
	GtkTextIter selection_start, selection_end;
	GtkTextIter match_start, match_end;
	gboolean continue_search = FALSE;

	if (message_is_empty (window)) {
		g_free (priv->last_search);
		priv->last_search = NULL;
		hildon_banner_show_information (GTK_WIDGET (window), NULL, _("mail_ib_nothing_to_find"));
		return;
	}

	g_object_get (G_OBJECT (widget), "prefix", &current_search, NULL);
	if ((current_search == NULL) || (strcmp (current_search, "") == 0)) {
		g_free (current_search);
		g_free (priv->last_search);
		priv->last_search = NULL;
		/* Information banner about empty search */
		hildon_banner_show_information (NULL, NULL, dgettext ("hildon-common-strings", "ecdg_ib_find_rep_enter_text"));
		return;
	}

	if ((priv->last_search != NULL)&&(!strcmp (current_search, priv->last_search))) {
		continue_search = TRUE;
	} else {
		g_free (priv->last_search);
		priv->last_search = g_strdup (current_search);
	}

	if (continue_search) {
		gtk_text_buffer_get_selection_bounds (priv->text_buffer, &selection_start, &selection_end);
		result = gtk_text_iter_forward_search_insensitive (&selection_end, current_search, 
								   &match_start, &match_end);
		if (!result)
			hildon_banner_show_information (NULL, NULL, dgettext ("hildon-libs", "ckct_ib_find_search_complete"));
	} else {
		GtkTextIter buffer_start;
		gtk_text_buffer_get_start_iter (priv->text_buffer, &buffer_start);
		result = gtk_text_iter_forward_search_insensitive (&buffer_start, current_search, 
								   &match_start, &match_end);
		if (!result)
			hildon_banner_show_information (NULL, NULL, dgettext ("hildon-libs", "ckct_ib_find_no_matches"));
	}

	/* Mark as selected the string found in search */
	if (result) {
		gtk_text_buffer_select_range (priv->text_buffer, &match_start, &match_end);
		gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->msg_body), &match_start, 0.0, TRUE, 0.0, 0.0);
	} else {
		g_free (priv->last_search);
		priv->last_search = NULL;
	}
	g_free (current_search);
}

static void
modest_msg_edit_window_find_toolbar_close (GtkWidget *widget,
					   ModestMsgEditWindow *window)
{
	GtkToggleAction *toggle;
	ModestWindowPrivate *parent_priv;
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);

	toggle = GTK_TOGGLE_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ToolsMenu/FindInMessageMenu"));
	gtk_toggle_action_set_active (toggle, FALSE);
}


static void 
update_paste_dimming (ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	GtkAction *action = NULL;
	GtkClipboard *clipboard = NULL;
	ModestEmailClipboard *e_clipboard;
	GtkWidget *focused;
	gboolean active;

	focused = gtk_window_get_focus (GTK_WINDOW (window));

	e_clipboard = modest_runtime_get_email_clipboard ();
	if (!modest_email_clipboard_cleared (e_clipboard)) {
		active = TRUE;
	} else {
		clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
		active = gtk_clipboard_wait_is_text_available (clipboard);
	}

	if (active) {
		if (MODEST_IS_ATTACHMENTS_VIEW (focused))
			active = FALSE;
	}

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/PasteMenu");
	gtk_action_set_sensitive (action, active);

}

static void 
update_select_all_dimming (ModestMsgEditWindow *window)
{
	GtkWidget *focused;
	gboolean dimmed = FALSE;
	GtkAction *action;
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);

	focused = gtk_window_get_focus (GTK_WINDOW (window));
	if (GTK_IS_ENTRY (focused)) {
		const gchar *current_text;
		current_text = gtk_entry_get_text (GTK_ENTRY (focused));
		dimmed = ((current_text == NULL) || (current_text[0] == '\0'));
	} else if (GTK_IS_TEXT_VIEW (focused)) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (focused));
		dimmed = (gtk_text_buffer_get_char_count (buffer) < 1);
	} else if (MODEST_IS_ATTACHMENTS_VIEW (focused)) {
		dimmed = FALSE;
	}
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/SelectAllMenu");
	gtk_action_set_sensitive (action, !dimmed);
}

static void 
update_zoom_dimming (ModestMsgEditWindow *window)
{
	GtkWidget *focused;
	gboolean dimmed = FALSE;
	GtkAction *action;
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);

	focused = gtk_window_get_focus (GTK_WINDOW (window));
	dimmed = ! WP_IS_TEXT_VIEW (focused);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ZoomMenu");
	gtk_action_set_sensitive (action, !dimmed);
}

static void
edit_menu_activated (GtkAction *action,
		     gpointer userdata)
{
	ModestMsgEditWindow *window = MODEST_MSG_EDIT_WINDOW (userdata);

	update_select_all_dimming (window);
	update_paste_dimming (window);
}
static void
view_menu_activated (GtkAction *action,
		     gpointer userdata)
{
	ModestMsgEditWindow *window = MODEST_MSG_EDIT_WINDOW (userdata);

	update_zoom_dimming (window);
}

gboolean 
modest_msg_edit_window_get_sent (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(window);
	return priv->sent;
}

void 
modest_msg_edit_window_set_sent (ModestMsgEditWindow *window, 
				 gboolean sent)
{
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(window);
	priv->sent = sent;
}


void            
modest_msg_edit_window_set_draft (ModestMsgEditWindow *window,
				  TnyMsg *draft)
{
	ModestMsgEditWindowPrivate *priv;
	TnyHeader *header = NULL;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	g_return_if_fail ((draft == NULL)||(TNY_IS_MSG (draft)));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();

	if (priv->draft_msg != NULL) {
		header = tny_msg_get_header (priv->draft_msg);
		if (TNY_IS_HEADER (header)) {
			modest_window_mgr_unregister_header (mgr, header);
		}
		g_object_unref (priv->draft_msg);
	}

	if (draft != NULL) {
		g_object_ref (draft);
		header = tny_msg_get_header (draft);
		if (TNY_IS_HEADER (header))
			modest_window_mgr_register_header (mgr, header);
	}

	priv->draft_msg = draft;
}

static void  
text_buffer_apply_tag (GtkTextBuffer *buffer, GtkTextTag *tag, 
		       GtkTextIter *start, GtkTextIter *end,
		       gpointer userdata)
{
	ModestMsgEditWindow *window = MODEST_MSG_EDIT_WINDOW (userdata);
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (userdata);
	gchar *tag_name;

	if (tag == NULL+13) return;
	g_object_get (G_OBJECT (tag), "name", &tag_name, NULL);
	if ((tag_name != NULL) && (g_str_has_prefix (tag_name, "image-tag-replace-"))) {
		replace_with_attachments (window, priv->attachments);
	}
}

void                    
modest_msg_edit_window_add_part (ModestMsgEditWindow *window,
				 TnyMimePart *part)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	g_return_if_fail (TNY_IS_MIME_PART (part));
	priv->attachments = g_list_prepend (priv->attachments, part);
	g_object_ref (part);
	modest_attachments_view_add_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), part);
	gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
	gtk_widget_show_all (priv->attachments_caption);
	gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
}

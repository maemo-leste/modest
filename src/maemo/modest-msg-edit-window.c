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
#include "modest-address-book.h"
#include "modest-text-utils.h"
#include <tny-simple-list.h>
#include <wptextview.h>
#include <wptextbuffer.h>
#include "modest-scroll-area.h"

#include "modest-hildon-includes.h"
#include "widgets/modest-msg-edit-window-ui.h"


#define DEFAULT_FONT_SIZE 3
#define DEFAULT_FONT 2
#define DEFAULT_SIZE_BUTTON_FONT_FAMILY "Sans"
#define DEFAULT_SIZE_COMBOBOX_WIDTH 80
#define DEFAULT_MAIN_VBOX_SPACING 6
#define SUBJECT_MAX_LENGTH 1000

static void  modest_msg_edit_window_class_init   (ModestMsgEditWindowClass *klass);
static void  modest_msg_edit_window_init         (ModestMsgEditWindow *obj);
static void  modest_msg_edit_window_finalize     (GObject *obj);

static gboolean msg_body_focus (GtkWidget *focus, GdkEventFocus *event, gpointer userdata);
static void  to_field_changed (GtkTextBuffer *buffer, ModestMsgEditWindow *editor);
static void  send_insensitive_press (GtkWidget *widget, ModestMsgEditWindow *editor);
static void  style_insensitive_press (GtkWidget *widget, ModestMsgEditWindow *editor);
static void  setup_insensitive_handlers (ModestMsgEditWindow *editor);
static void  reset_modified (ModestMsgEditWindow *editor);
static gboolean is_modified (ModestMsgEditWindow *editor);

static void  text_buffer_refresh_attributes (WPTextBuffer *buffer, ModestMsgEditWindow *window);
static void  text_buffer_mark_set (GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark, gpointer userdata);
static void  text_buffer_can_undo (GtkTextBuffer *buffer, gboolean can_undo, ModestMsgEditWindow *window);
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

/* ModestWindow methods implementation */
static void  modest_msg_edit_window_set_zoom (ModestWindow *window, gdouble zoom);
static gdouble modest_msg_edit_window_get_zoom (ModestWindow *window);
static gboolean modest_msg_edit_window_zoom_minus (ModestWindow *window);
static gboolean modest_msg_edit_window_zoom_plus (ModestWindow *window);
static void modest_msg_edit_window_show_toolbar   (ModestWindow *window,
						   gboolean show_toolbar);
static void update_dimmed (ModestMsgEditWindow *window);


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
	GtkWidget   *attachments_caption;

	GtkTextBuffer *text_buffer;

	GtkWidget   *font_size_toolitem;
	GtkWidget   *font_face_toolitem;
	GtkWidget   *font_color_button;
	GSList      *font_items_group;
	GtkWidget   *font_tool_button_label;
	GSList      *size_items_group;
	GtkWidget   *size_tool_button_label;

	GtkWidget   *scroll;

	gint last_cid;
	GList *attachments;

	TnyHeaderFlags priority_flags;

	gdouble zoom_level;

	TnyMsg      *draft_msg;
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

	g_type_class_add_private (gobject_class, sizeof(ModestMsgEditWindowPrivate));

	modest_window_class->save_state_func = save_state;
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

	priv->priority_flags = 0;

	priv->draft_msg = NULL;
}


/* FIXME: this is a dup from the one in gtk/ */
static ModestPairList*
get_transports (void)
{
	ModestAccountMgr *account_mgr;
	GSList *transports = NULL;
	GSList *cursor, *accounts;
	
	account_mgr = modest_runtime_get_account_mgr();
 	cursor = accounts = modest_account_mgr_account_names (account_mgr, 
 						TRUE /* only enabled accounts. */); 
	while (cursor) {
		gchar *account_name = (gchar*)cursor->data;
		gchar *from_string  = modest_account_mgr_get_from_string (account_mgr,
									  account_name);
		if (!from_string)  {
			/* something went wrong: ignore this one */
			g_free (account_name);
			cursor->data = NULL;
		} else {
			ModestPair *pair;
			pair = modest_pair_new ((gpointer) account_name,
						(gpointer) from_string , TRUE);
			transports = g_slist_prepend (transports, pair);
		} /* don't free account name; it's freed when the transports list is freed */
		cursor = cursor->next;
	}
	g_slist_free (accounts);
	return transports;
}


static void
text_buffer_mark_set (GtkTextBuffer *buffer, GtkTextIter *iter, GtkTextMark *mark, gpointer userdata)
{
	ModestMsgEditWindow *window;
	ModestMsgEditWindowPrivate *priv;
	GdkRectangle location;
	gint v_scroll_min_value = 0;
	gint v_scroll_max_value = 0;
	gint v_scroll_visible;
	GtkAdjustment *vadj;
	GtkTextMark *insert_mark;
	GtkTextIter insert_iter;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (userdata));
	g_return_if_fail (GTK_IS_TEXT_MARK (mark));
	window = MODEST_MSG_EDIT_WINDOW (userdata);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
		
	insert_mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (priv->text_buffer));
	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (priv->text_buffer), &insert_iter, insert_mark);
	gtk_text_view_get_iter_location (GTK_TEXT_VIEW (priv->msg_body), &insert_iter, &location);
	
	if (priv->header_box)
		v_scroll_min_value += priv->header_box->allocation.height + DEFAULT_MAIN_VBOX_SPACING;
	v_scroll_min_value += location.y;
	v_scroll_max_value = v_scroll_min_value + location.height;
	
	v_scroll_visible = GTK_WIDGET (window)->allocation.height;
	
	vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scroll));
	
	if (((gdouble) v_scroll_min_value) < vadj->value)
		gtk_adjustment_set_value (vadj, v_scroll_min_value);
	else if (((gdouble) v_scroll_max_value) > (vadj->value + vadj->page_size))
		gtk_adjustment_set_value (vadj, ((gdouble)v_scroll_max_value) - vadj->page_size);
}

static void
init_window (ModestMsgEditWindow *obj)
{
	GtkWidget *from_caption, *to_caption, *subject_caption;
	GtkWidget *main_vbox;
	ModestMsgEditWindowPrivate *priv;
	ModestPairList *protos;
	GtkSizeGroup *size_group;
	GtkWidget *frame;
	GtkWidget *scroll_area;
	GtkWidget *subject_box;
	GtkWidget *attachment_icon;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	protos = get_transports ();
 	priv->from_field    = modest_combo_box_new (protos, g_str_equal);
	modest_pair_list_free (protos);

	priv->to_field      = modest_recpt_editor_new ();
	priv->cc_field      = modest_recpt_editor_new ();
	priv->bcc_field     = modest_recpt_editor_new ();
	subject_box = gtk_hbox_new (FALSE, 0);
	priv->priority_icon = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (subject_box), priv->priority_icon, FALSE, FALSE, 0);
	priv->subject_field = gtk_entry_new_with_max_length (SUBJECT_MAX_LENGTH);
	g_object_set (G_OBJECT (priv->subject_field), "hildon-input-mode", HILDON_GTK_INPUT_MODE_FULL, NULL);
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
	g_object_set (priv->text_buffer, "font_scale", 1.0, NULL);
	wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
/* 	gtk_text_buffer_set_can_paste_rich_text (priv->text_buffer, TRUE); */
	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);

	g_signal_connect (G_OBJECT (priv->text_buffer), "refresh_attributes",
			  G_CALLBACK (text_buffer_refresh_attributes), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "mark-set",
			  G_CALLBACK (text_buffer_mark_set), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "can-undo",
			  G_CALLBACK (text_buffer_can_undo), obj);
	g_signal_connect (G_OBJECT (obj), "window-state-event",
			  G_CALLBACK (modest_msg_edit_window_window_state_event),
			  NULL);
	g_signal_connect_swapped (G_OBJECT (priv->to_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);
	g_signal_connect_swapped (G_OBJECT (priv->cc_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);
	g_signal_connect_swapped (G_OBJECT (priv->bcc_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);

	g_signal_connect (G_OBJECT (priv->msg_body), "focus-in-event",
			  G_CALLBACK (msg_body_focus), obj);
	g_signal_connect (G_OBJECT (priv->msg_body), "focus-out-event",
			  G_CALLBACK (msg_body_focus), obj);
	g_signal_connect (G_OBJECT (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->to_field))),
			  "changed", G_CALLBACK (to_field_changed), obj);
	to_field_changed (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->to_field)), MODEST_MSG_EDIT_WINDOW (obj));

	priv->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->scroll), GTK_SHADOW_NONE);
	
	main_vbox = gtk_vbox_new  (FALSE, DEFAULT_MAIN_VBOX_SPACING);

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->header_box, FALSE, FALSE, 0);
	frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX(main_vbox), frame, TRUE, TRUE, 0);

	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (priv->scroll), main_vbox);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (main_vbox), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scroll)));
	gtk_widget_show_all (GTK_WIDGET(priv->scroll));
	
	if (!modest_conf_get_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_CC, NULL))
		gtk_widget_hide (priv->cc_field);
	if (!modest_conf_get_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_BCC, NULL))
		gtk_widget_hide (priv->bcc_field);

	gtk_container_add (GTK_CONTAINER(obj), priv->scroll);
	scroll_area = modest_scroll_area_new (priv->scroll, priv->msg_body);
	gtk_container_add (GTK_CONTAINER (frame), scroll_area);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (scroll_area), 
					     gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (priv->scroll)));
}
	


static void
modest_msg_edit_window_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, ModestMsgEditWindow *self)
{
	GtkWidget *close_dialog;
	ModestMsgEditWindowPrivate *priv;
	gint response;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);
	modest_window_save_state (MODEST_WINDOW (self));
	if (is_modified (self)) {
		close_dialog = hildon_note_new_confirmation (GTK_WINDOW (self), _("mcen_nc_no_email_message_modified_save_changes"));
		response = gtk_dialog_run (GTK_DIALOG (close_dialog));
		gtk_widget_destroy (close_dialog);

		if (response != GTK_RESPONSE_CANCEL) {
			modest_ui_actions_on_save_to_drafts (NULL, self);
		}
	} 
/* 	/\* remove old message from drafts *\/ */
/* 	if (priv->draft_msg) { */
/* 		TnyHeader *header = tny_msg_get_header (priv->draft_msg); */
/* 		TnyAccount *account = modest_tny_account_store_get_tny_account_by_account (modest_runtime_get_account_store(), */
/* 											   account_name, */
/* 											   TNY_ACCOUNT_TYPE_STORE); */
/* 		TnyFolder *folder = modest_tny_account_get_special_folder (account, TNY_FOLDER_TYPE_DRAFTS); */
/* 		g_return_val_if_fail (TNY_IS_HEADER (header), FALSE); */
/* 		g_return_val_if_fail (TNY_IS_FOLDER (folder), FALSE); */
/* 		tny_folder_remove_msg (folder, header, NULL); */
/* 		g_object_unref (folder); */
/* 		g_object_unref (header); */
/* 		g_object_unref (priv->draft_msg); */
/* 		priv->draft_msg = NULL; */
/* 	} */
	gtk_widget_destroy (GTK_WIDGET (self));
	
	return TRUE;
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


static void
set_msg (ModestMsgEditWindow *self, TnyMsg *msg)
{
	TnyHeader *header;
	const gchar *to, *cc, *bcc, *subject;
	gchar *body;
	ModestMsgEditWindowPrivate *priv;
	GtkTextIter iter;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (TNY_IS_MSG (msg));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	header = tny_msg_get_header (msg);
	to      = tny_header_get_to (header);
	cc      = tny_header_get_cc (header);
	bcc     = tny_header_get_bcc (header);
	subject = tny_header_get_subject (header);

	if (to)
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->to_field),  to);
	if (cc)
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->cc_field),  cc);
	if (bcc)
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->bcc_field), bcc);
	if (subject)
		gtk_entry_set_text (GTK_ENTRY(priv->subject_field), subject);	

/* 	gtk_text_buffer_set_can_paste_rich_text (priv->text_buffer, TRUE); */
	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
	body = modest_tny_msg_get_body (msg, FALSE);

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
	if (priv->attachments == NULL)
		gtk_widget_hide_all (priv->attachments_caption);

	gtk_text_buffer_get_start_iter (priv->text_buffer, &iter);
	gtk_text_buffer_place_cursor (priv->text_buffer, &iter);

	reset_modified (self);

	update_dimmed (self);
	text_buffer_can_undo (priv->text_buffer, FALSE, self);

	priv->draft_msg = msg;
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
			
		g_signal_connect (G_OBJECT (size_menu_item), "toggled", G_CALLBACK (modest_msg_edit_window_size_change),
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
			
		g_signal_connect (G_OBJECT (font_menu_item), "toggled", G_CALLBACK (modest_msg_edit_window_font_change),
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

	g_return_val_if_fail (msg, NULL);
	
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
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager, MODEST_UIDIR "modest-msg-edit-window-ui.xml", &error);
	if (error != NULL) {
		g_warning ("Could not merge modest-msg-edit-window-ui.xml: %s", error->message);
		g_error_free (error);
		error = NULL;
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
		
	gtk_window_set_title (GTK_WINDOW(obj), "Modest");
	gtk_window_set_icon_from_file (GTK_WINDOW(obj), MODEST_APP_ICON, NULL);

	g_signal_connect (G_OBJECT(obj), "delete-event",
			  G_CALLBACK(on_delete_event), obj);

	modest_window_set_active_account (MODEST_WINDOW(obj), account_name);

	modest_msg_edit_window_setup_toolbar (MODEST_MSG_EDIT_WINDOW (obj));

	setup_insensitive_handlers (MODEST_MSG_EDIT_WINDOW (obj));

	set_msg (MODEST_MSG_EDIT_WINDOW (obj), msg);

	text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (obj));

	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_MSG_EDIT_ICON);
	gtk_window_set_icon (GTK_WINDOW (obj), window_icon);
	
	return (ModestWindow*)obj;
}

static gint
get_formatted_data_cb (const gchar *buffer, gpointer user_data)
{
	GString **string_buffer = (GString **) user_data;

	*string_buffer = g_string_append (*string_buffer, buffer);
   
	return 0;
}

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
	GtkTextBuffer *buf;
	GtkTextIter b, e;
	ModestMsgEditWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window), NULL);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (edit_window);
									
	account_name = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->from_field));
	g_return_val_if_fail (account_name, NULL);
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));	
	gtk_text_buffer_get_bounds (buf, &b, &e);
	
	/* don't free these (except from) */
	data = g_slice_new0 (MsgData);
	data->from    =  modest_account_mgr_get_from_string (modest_runtime_get_account_mgr(),
							     account_name);
	data->to      =  (gchar*) modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR(priv->to_field));
	data->cc      =  (gchar*) modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR(priv->cc_field));
	data->bcc     =  (gchar*) modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR(priv->bcc_field));
	data->subject =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->subject_field));	
	data->plain_body =  (gchar *) gtk_text_buffer_get_text (priv->text_buffer, &b, &e, FALSE);
	if (wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer)))
		data->html_body = get_formatted_data (edit_window);
	else
		data->html_body = NULL;
	data->attachments = priv->attachments;
	data->priority_flags = priv->priority_flags;

	return data;
}

void 
modest_msg_edit_window_free_msg_data (ModestMsgEditWindow *edit_window,
						      MsgData *data)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window));

	g_free (data->from);
	g_free (data->html_body);
	g_free (data->plain_body);
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
	buffer_format->cs.color = gdk_color_equal(&(buffer_format->color), &(current_format->color));
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
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_BOLD, (gpointer) (buffer_format->font_size));
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
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_BOLD, (gpointer) (buffer_format->font));
	}
	if (buffer_format->cs.bullet) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_BULLET, (gpointer) (buffer_format->bullet));
	}
/* 	wp_text_buffer_set_format (WP_TEXT_BUFFER (priv->text_buffer), buffer_format); */
	wp_text_buffer_thaw (WP_TEXT_BUFFER (priv->text_buffer));

	g_free (current_format);

}

static void
toggle_action_set_active_block_notify (GtkToggleAction *action,
				       gboolean value)
{
	GSList *proxies = NULL;

	for (proxies = gtk_action_get_proxies (GTK_ACTION (action));
	     proxies != NULL; proxies = g_slist_next (proxies)) {
		GtkWidget *widget = (GtkWidget *) proxies->data;
		gtk_action_block_activate_from (GTK_ACTION (action), widget);
	}

	gtk_toggle_action_set_active (action, value);

	for (proxies = gtk_action_get_proxies (GTK_ACTION (action));
	     proxies != NULL; proxies = g_slist_next (proxies)) {
		GtkWidget *widget = (GtkWidget *) proxies->data;
		gtk_action_unblock_activate_from (GTK_ACTION (action), widget);
	}
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
			toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), TRUE);
	} else {
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/FileFormatMenu/FileFormatPlainTextMenu");
		if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
			toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), TRUE);
	}

	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, FALSE);
	
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bold);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->italic);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/BulletedListMenu");
	toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bullet);

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

void
modest_msg_edit_window_insert_image (ModestMsgEditWindow *window)
{
	
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;
	gint response = 0;
	gchar *filename = NULL;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (window), GTK_FILE_CHOOSER_ACTION_OPEN);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK:
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		break;
	default:
		break;
	}
	gtk_widget_destroy (dialog);

	if (filename) {
		GdkPixbuf *pixbuf = NULL;
		GtkTextIter position;
		GtkTextMark *insert_mark;

		pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
		if (pixbuf) {
			gint image_file_id;
			GdkPixbufFormat *pixbuf_format;

			image_file_id = g_open (filename, O_RDONLY, 0);
			pixbuf_format = gdk_pixbuf_get_file_info (filename, NULL, NULL);
			if ((image_file_id != -1)&&(pixbuf_format != NULL)) {
				TnyMimePart *image_part;
				TnyStream *image_stream;
				gchar **mime_types;
				gchar *mime_type;
				gchar *basename;
				gchar *content_id;

				mime_types = gdk_pixbuf_format_get_mime_types (pixbuf_format);
				if ((mime_types != NULL) && (mime_types[0] != NULL)) {
					mime_type = mime_types[0];
				} else {
					mime_type = "image/unknown";
				}
				image_part = tny_platform_factory_new_mime_part
					(modest_runtime_get_platform_factory ());
				image_stream = TNY_STREAM (tny_fs_stream_new (image_file_id));

				tny_mime_part_construct_from_stream (image_part, image_stream, mime_type);
				g_strfreev (mime_types);

				content_id = g_strdup_printf ("%d", priv->last_cid);
				tny_mime_part_set_content_id (image_part, content_id);
				g_free (content_id);
				priv->last_cid++;

				basename = g_path_get_basename (filename);
				tny_mime_part_set_filename (image_part, basename);
				g_free (basename);
				
				insert_mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (priv->text_buffer));
				gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (priv->text_buffer), &position, insert_mark);
				wp_text_buffer_insert_image (WP_TEXT_BUFFER (priv->text_buffer), &position, g_strdup (tny_mime_part_get_content_id (image_part)), pixbuf);
				priv->attachments = g_list_prepend (priv->attachments, image_part);
				modest_attachments_view_add_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
									image_part);
				gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
				gtk_widget_show_all (priv->attachments_caption);
			} else if (image_file_id == -1) {
				close (image_file_id);
			}
		}
	}


}

static void
modest_msg_edit_window_color_button_change (ModestMsgEditWindow *window,
					    gpointer userdata)
{
	ModestMsgEditWindowPrivate *priv;
	GdkColor *new_color;
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
#ifdef MODEST_HILDON_VERSION_0	
	new_color = hildon_color_button_get_color (HILDON_COLOR_BUTTON (priv->font_color_button));
#else 
	GdkColor col;
	hildon_color_button_get_color (HILDON_COLOR_BUTTON(priv->font_color_button), &col);
	new_color = &col;
#endif /*MODEST_HILDON_VERSION_0*/

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

		label = gtk_bin_get_child (GTK_BIN (menu_item));
		
		new_size_index = atoi (gtk_label_get_text (GTK_LABEL (label)));

		if (!wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT_SIZE, 
						   (gpointer) wp_get_font_size_index (new_size_index, 12)))
			wp_text_view_reset_and_show_im (WP_TEXT_VIEW (priv->msg_body));
		
		text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (window));
		markup = g_strconcat ("<span font_family='Serif'>", gtk_label_get_text (GTK_LABEL (label)), "</span>", NULL);
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
     
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	priv->zoom_level = zoom;
	wp_text_buffer_set_font_scaling_factor (WP_TEXT_BUFFER (priv->text_buffer), zoom);
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
modest_msg_edit_window_zoom_plus (ModestWindow *window)
{
	ModestWindowPrivate *parent_priv;
	GtkRadioAction *zoom_radio_action;
	GSList *group, *node;

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	zoom_radio_action = GTK_RADIO_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, 
									 "/MenuBar/ViewMenu/ZoomMenu/Zoom50Menu"));

	group = gtk_radio_action_get_group (zoom_radio_action);

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (group->data))) {
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_max_zoom_level"));
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
				hildon_banner_show_information (NULL, NULL, _("mcen_ib_min_zoom_level"));
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
	if (show)
		gtk_widget_show (priv->cc_caption);
	else
		gtk_widget_hide (priv->cc_caption);
}

void
modest_msg_edit_window_show_bcc (ModestMsgEditWindow *window, 
				 gboolean show)
{
	ModestMsgEditWindowPrivate *priv = NULL;
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	if (show)
		gtk_widget_show (priv->bcc_caption);
	else
		gtk_widget_hide (priv->bcc_caption);
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
	
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));
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

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	priority_flags = priority_flags & (TNY_HEADER_FLAG_HIGH_PRIORITY);

	if (priv->priority_flags != priority_flags) {

		priv->priority_flags = priority_flags;

		switch (priority_flags) {
		case TNY_HEADER_FLAG_HIGH_PRIORITY:
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon), "qgn_list_messaging_high", GTK_ICON_SIZE_MENU);
			gtk_widget_show (priv->priority_icon);
			break;
		case TNY_HEADER_FLAG_LOW_PRIORITY:
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon), "qgn_list_messaging_low", GTK_ICON_SIZE_MENU);
			gtk_widget_show (priv->priority_icon);
			break;
		default:
			gtk_widget_hide (priv->priority_icon);
			break;
		}
	}
}

void
modest_msg_edit_window_set_file_format (ModestMsgEditWindow *window,
					gint file_format)
{
	ModestMsgEditWindowPrivate *priv;
	gint current_format;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

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
			if (response == GTK_RESPONSE_OK)
				wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), FALSE);
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
	
	dialog = hildon_font_selection_dialog_new (NULL, NULL);

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

	gtk_widget_destroy (dialog);
	
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
		gdk_color_free(color);
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
		gtk_widget_grab_focus(GTK_WIDGET(priv->msg_body));
		wp_text_buffer_set_format(WP_TEXT_BUFFER(priv->text_buffer), &fmt);
	}

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
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/EmailMenu/SendMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (send_insensitive_press), window);

	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/FormatMenu/SelectFontMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/FormatMenu/BulletedListMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentLeftMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentCenterMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentRightMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
	widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar/AttachmentsMenu/InsertImageMenu");
	g_signal_connect (G_OBJECT (widget), "insensitive-press", G_CALLBACK (style_insensitive_press), window);
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

static gboolean
msg_body_focus (GtkWidget *focus,
		GdkEventFocus *event,
		gpointer userdata)
{
	update_dimmed (MODEST_MSG_EDIT_WINDOW (userdata));
	return FALSE;
}

static void
to_field_changed (GtkTextBuffer *buffer,
		  ModestMsgEditWindow *editor)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (editor);
	GtkAction *action;

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarSend");
	gtk_action_set_sensitive (action, gtk_text_buffer_get_char_count (buffer) != 0);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EmailMenu/SendMenu");
	gtk_action_set_sensitive (action, gtk_text_buffer_get_char_count (buffer) != 0);
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

static gboolean
is_modified (ModestMsgEditWindow *editor)
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
modest_msg_edit_window_check_names (ModestMsgEditWindow *window)
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

	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->to_field)))
		return FALSE;
	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->cc_field)))
		return FALSE;
	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->bcc_field)))
		return FALSE;

	modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->to_field));

	return TRUE;

}

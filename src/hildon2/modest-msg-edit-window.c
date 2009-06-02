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
#include <tny-camel-mem-stream.h>
#include <modest-account-protocol.h>

#include <config.h>

#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>

#include <widgets/modest-msg-edit-window.h>
#include <modest-selector-picker.h>
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
#include "modest-tny-account.h"
#include "modest-address-book.h"
#include "modest-text-utils.h"
#include <tny-simple-list.h>
#include <modest-wp-text-view.h>
#include <wptextbuffer.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-touch-selector.h>
#include <hildon/hildon-picker-dialog.h>
#include "modest-msg-edit-window-ui-dimming.h"

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
#include <modest-utils.h>
#include "modest-maemo-utils.h"
#include <modest-ui-constants.h>


#define DEFAULT_FONT_SIZE 3
#define DEFAULT_FONT 2
#define DEFAULT_SIZE_BUTTON_FONT_FAMILY "Sans"
#define DEFAULT_MAIN_VBOX_SPACING 6
#define SUBJECT_MAX_LENGTH 1000
#define IMAGE_MAX_WIDTH 560
#define DEFAULT_FONT_SCALE 1.5
#define ATTACHMENT_BUTTON_WIDTH 118
#define MAX_FROM_VALUE 36
#define MAX_BODY_LENGTH 4096

static gboolean is_wp_text_buffer_started = FALSE;

static void  modest_msg_edit_window_class_init   (ModestMsgEditWindowClass *klass);
static void  modest_msg_edit_window_init         (ModestMsgEditWindow *obj);
static void  modest_msg_edit_window_finalize     (GObject *obj);

static gboolean msg_body_focus (GtkWidget *focus, GdkEventFocus *event, gpointer userdata);
static void  body_changed (GtkTextBuffer *buffer, ModestMsgEditWindow *editor);
static void  recpt_field_changed (GtkTextBuffer *buffer, ModestMsgEditWindow *editor);

static void  text_buffer_refresh_attributes (WPTextBuffer *buffer, ModestMsgEditWindow *window);
static void  text_buffer_can_undo (GtkTextBuffer *buffer, gboolean can_undo, ModestMsgEditWindow *window);
static void  text_buffer_can_redo (GtkTextBuffer *buffer, gboolean can_redo, ModestMsgEditWindow *window);
static void  text_buffer_apply_tag (GtkTextBuffer *buffer, GtkTextTag *tag, 
				    GtkTextIter *start, GtkTextIter *end,
				    gpointer userdata);
static void  text_buffer_delete_images_by_id (GtkTextBuffer *buffer, const gchar * image_id);
static void  body_insert_text (GtkTextBuffer *buffer, 
				      GtkTextIter *location,
				      gchar *text,
				      gint len,
				      ModestMsgEditWindow *window);
static void  subject_field_changed (GtkEditable *editable, ModestMsgEditWindow *window);
static void  subject_field_insert_text (GtkEditable *editable, 
					gchar *new_text,
					gint new_text_length,
					gint *position,
					ModestMsgEditWindow *window);
static void  modest_msg_edit_window_color_button_change (ModestMsgEditWindow *window,
							 gpointer userdata);
static void  modest_msg_edit_window_setup_toolbar (ModestMsgEditWindow *window);

static void modest_msg_edit_window_open_addressbook (ModestMsgEditWindow *window,
						     ModestRecptEditor *editor);
static void modest_msg_edit_window_add_attachment_clicked (GtkButton *button,
							   ModestMsgEditWindow *window);

/* ModestWindow methods implementation */
static void modest_msg_edit_window_disconnect_signals (ModestWindow *window);
static void modest_msg_edit_window_show_toolbar   (ModestWindow *window,
						   gboolean show_toolbar);
static void modest_msg_edit_window_clipboard_owner_change (GtkClipboard *clipboard,
							   GdkEvent *event,
							   ModestMsgEditWindow *window);
static void modest_msg_edit_window_clipboard_owner_handle_change_in_idle (ModestMsgEditWindow *window);
static void subject_field_move_cursor (GtkEntry *entry,
				       GtkMovementStep step,
				       gint a1,
				       gboolean a2,
				       gpointer userdata);
static void update_window_title (ModestMsgEditWindow *window);

/* Find toolbar */
static void modest_msg_edit_window_find_toolbar_search (GtkWidget *widget,
							ModestMsgEditWindow *window);
static void modest_msg_edit_window_find_toolbar_close (GtkWidget *widget,
						       ModestMsgEditWindow *window);
static gboolean gtk_text_iter_forward_search_insensitive (const GtkTextIter *iter,
							  const gchar *str,
							  GtkTextIter *match_start,
							  GtkTextIter *match_end);

static void remove_tags (WPTextBuffer *buffer);

static void on_account_removed (TnyAccountStore *account_store, 
				TnyAccount *account,
				gpointer user_data);

static void init_window (ModestMsgEditWindow *obj);

gboolean scroll_drag_timeout (gpointer userdata);
static void correct_scroll (ModestMsgEditWindow *w);
static void correct_scroll_without_drag_check (ModestMsgEditWindow *w, gboolean only_if_focused);
static void text_buffer_end_user_action (GtkTextBuffer *buffer,
					 ModestMsgEditWindow *userdata);
static void text_buffer_mark_set (GtkTextBuffer *buffer,
				  GtkTextIter *iter,
				  GtkTextMark *mark,
				  ModestMsgEditWindow *userdata);
static void on_message_settings (GtkAction *action,
				 ModestMsgEditWindow *window);
static void setup_menu (ModestMsgEditWindow *self);

static void from_field_changed (HildonPickerButton *button,
				ModestMsgEditWindow *self);
static void font_size_clicked (GtkToolButton *button,
			       ModestMsgEditWindow *window);
static void font_face_clicked (GtkToolButton *button,
			       ModestMsgEditWindow *window);
static void update_signature (ModestMsgEditWindow *self,
			      const gchar *old_account, 
			      const gchar *new_account);
static void update_branding (ModestMsgEditWindow *self,
			     const gchar *new_account);
static GtkWidget *_create_addressbook_box (GtkSizeGroup *title_size_group, GtkSizeGroup *value_size_group,
					   const gchar *label, GtkWidget *control);
static void max_chars_banner_unref (ModestMsgEditWindow *self, GObject *old_ref);
static void DEBUG_BUFFER (WPTextBuffer *buffer)
{
#ifdef DEBUG
	GtkTextIter iter;
	g_debug ("BEGIN BUFFER OF SIZE %d", gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (buffer)));

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
		g_debug ("%s", output->str);
		g_string_free (output, TRUE);
		gtk_text_iter_forward_to_tag_toggle (&iter, NULL);
	}
	g_debug ("END BUFFER");
#endif
}

static const GtkActionEntry hildon2_msg_edit_action_entries [] = {
	{ "MessageSettings", NULL, N_("mcen_me_message_settings"), NULL, NULL, G_CALLBACK (on_message_settings)},
};


/* static gboolean */
/* on_key_pressed (GtkWidget *self, */
/* 		GdkEventKey *event, */
/* 		gpointer user_data); */

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
	gchar       *last_from_account;
	gchar       *original_account_name;

	gchar       *references;
	gchar       *in_reply_to;

	gchar       *original_mailbox;
	
	GtkWidget   *to_field;
	GtkWidget   *cc_field;
	GtkWidget   *bcc_field;
	GtkWidget   *subject_field;
	GtkWidget   *attachments_view;
	GtkWidget   *priority_icon;
	GtkWidget   *subject_box;
	GtkWidget   *send_button;

	GtkWidget   *cc_caption;
	GtkWidget   *bcc_caption;
	gboolean     update_caption_visibility;
	GtkWidget   *attachments_caption;

	GtkTextBuffer *text_buffer;

	GtkWidget   *font_size_toolitem;
	GtkWidget   *font_face_toolitem;
	GtkWidget   *font_color_button;
	GtkWidget   *font_color_toolitem;
	GSList      *font_items_group;
	GtkTreeModel *faces_model;
	gint         current_face_index;
	GtkWidget   *font_tool_button_label;
	GtkTreeModel *sizes_model;
	gint         current_size_index;
	GtkWidget   *size_tool_button_label;

	GtkWidget   *find_toolbar;
	gchar       *last_search;

	GtkWidget   *font_dialog;

	GtkWidget   *pannable;
	guint        correct_scroll_idle;
	guint        scroll_drag_timeout_id;
	gdouble      last_upper;

	gint next_cid;
	TnyList *attachments;
	TnyList *images;
	guint64 images_size;
	gint images_count;

	TnyHeaderFlags priority_flags;

	gboolean    can_undo, can_redo;
	gulong      clipboard_change_handler_id;
	gulong      default_clipboard_change_handler_id;
	gulong      account_removed_handler_id;
	guint       clipboard_owner_idle;
	gchar       *clipboard_text;

	TnyMsg      *draft_msg;
	TnyMsg      *outbox_msg;
	gchar       *msg_uid;

	gboolean    sent;

	GtkWidget   *app_menu;
	GtkWidget   *cc_button;
	GtkWidget   *bcc_button;

	GtkWidget   *max_chars_banner;

	GtkWidget   *brand_icon;
	GtkWidget   *brand_label;
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
		my_type = g_type_register_static (MODEST_TYPE_HILDON2_WINDOW,
		                                  "ModestMsgEditWindow",
		                                  &my_info, 0);

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
	ModestConf *conf = NULL;

	conf = modest_runtime_get_conf ();

	/* Dim at start clipboard actions */
	modest_widget_memory_restore (conf, G_OBJECT(self), MODEST_CONF_EDIT_WINDOW_KEY);
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

	modest_window_class->show_toolbar_func = modest_msg_edit_window_show_toolbar;
	modest_window_class->save_state_func = save_state;
	modest_window_class->disconnect_signals_func = modest_msg_edit_window_disconnect_signals;

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
	priv->attachments   = TNY_LIST (tny_simple_list_new ());
	priv->images        = TNY_LIST (tny_simple_list_new ());
	priv->images_size   = 0;
	priv->images_count  = 0;
	priv->next_cid      = 0;

	priv->cc_caption    = NULL;
	priv->bcc_caption    = NULL;
	priv->update_caption_visibility = FALSE;

	priv->priority_flags = 0;

	priv->find_toolbar = NULL;
	priv->last_search = NULL;

	priv->draft_msg = NULL;
	priv->outbox_msg = NULL;
	priv->msg_uid = NULL;

	priv->can_undo = FALSE;
	priv->can_redo = FALSE;
	priv->clipboard_change_handler_id = 0;
	priv->default_clipboard_change_handler_id = 0;
	priv->account_removed_handler_id = 0;
	priv->clipboard_owner_idle = 0;
	priv->clipboard_text = NULL;
	priv->sent = FALSE;

	priv->scroll_drag_timeout_id = 0;
	priv->correct_scroll_idle = 0;
	priv->last_upper = 0.0;

	priv->font_dialog = NULL;
	priv->app_menu = NULL;

	priv->references = NULL;
	priv->in_reply_to = NULL;
	priv->max_chars_banner = NULL;

	if (!is_wp_text_buffer_started) {
		is_wp_text_buffer_started = TRUE;
		wp_text_buffer_library_init ();
	}

	init_window (obj);
	
	hildon_program_add_window (hildon_program_get_instance(),
				   HILDON_WINDOW(obj));
}

static gchar *
multimailbox_get_default_mailbox (const gchar *account_name)
{
	gchar *transport_account;
	gchar *result = NULL;

	transport_account = modest_account_mgr_get_server_account_name (modest_runtime_get_account_mgr (),
									account_name,
									TNY_ACCOUNT_TYPE_TRANSPORT);
	if (transport_account) {
		gchar *proto;
		ModestProtocolRegistry *registry;

		registry = modest_runtime_get_protocol_registry ();

		proto = modest_account_mgr_get_string (modest_runtime_get_account_mgr (), transport_account, 
						       MODEST_ACCOUNT_PROTO, TRUE);
		if (proto != NULL) {
			ModestProtocol *protocol = 
				modest_protocol_registry_get_protocol_by_name (registry,
									       MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
									       proto);
			if (MODEST_ACCOUNT_PROTOCOL (protocol)) {
				ModestPairList *pair_list;

				pair_list = modest_account_protocol_get_from_list (MODEST_ACCOUNT_PROTOCOL (protocol),
										   account_name);
				if (pair_list) {
					ModestPair *pair = (ModestPair *) pair_list->data;
					result = g_strdup ((const gchar *) pair->first);
					modest_pair_list_free (pair_list);
				}
			}
			
		}
	}

	return result;
}

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
		if (account_name) {

			gchar *transport_account;
			gboolean multi_mailbox = FALSE;
			ModestProtocol *protocol = NULL;

			if (modest_account_mgr_account_is_multimailbox (account_mgr, account_name, &protocol)) {

				transport_account = modest_account_mgr_get_server_account_name 
					(modest_runtime_get_account_mgr (),
					 account_name,
					 TNY_ACCOUNT_TYPE_TRANSPORT);
				if (protocol && MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
					ModestPairList *pair_list;
					pair_list = modest_account_protocol_get_from_list (MODEST_ACCOUNT_PROTOCOL (protocol),
											   account_name);
					if (pair_list) {
						transports = g_slist_concat (transports, pair_list);
						multi_mailbox = TRUE;
					}
				}
			}

			if (!multi_mailbox) {
				gchar *from_string  = NULL;

				from_string = modest_account_mgr_get_from_string (account_mgr,
										  account_name, NULL);
				if (from_string && account_name) {
					gchar *name = account_name;
					ModestPair *pair = modest_pair_new ((gpointer) name,
									    (gpointer) from_string , TRUE);
					transports = g_slist_prepend (transports, pair);
				}
			}
		}
		
		cursor = cursor->next;
	}
	g_slist_free (accounts); /* only free the accounts, not the elements,
				  * because they are used in the pairlist */
	return transports;
}

static void window_focus (GtkWindow *window,
			  GtkWidget *widget,
			  gpointer userdata)
{
	modest_window_check_dimming_rules_group (MODEST_WINDOW (userdata), MODEST_DIMMING_RULES_CLIPBOARD);
}

gboolean
scroll_drag_timeout (gpointer userdata)
{
	ModestMsgEditWindow *win = (ModestMsgEditWindow *) userdata;
	ModestMsgEditWindowPrivate *priv;

	/* It could happen that the window was already closed */
	if (!GTK_WIDGET_VISIBLE (win))
		return FALSE;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(win);

	correct_scroll_without_drag_check (win, TRUE);

	priv->scroll_drag_timeout_id = 0;

	return FALSE;
}

static gboolean 
correct_scroll_without_drag_check_idle (gpointer userdata)
{
	ModestMsgEditWindow *w = (ModestMsgEditWindow *) userdata;
	ModestMsgEditWindowPrivate *priv;
	GtkTextIter iter;
	GdkRectangle rectangle;
	gint offset_min, offset_max;
	GtkTextMark *insert;
	GtkAdjustment *vadj;

	/* It could happen that the window was already closed */
	if (!GTK_WIDGET_VISIBLE (w))
		return FALSE;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(w);

	insert = gtk_text_buffer_get_insert (priv->text_buffer);
	gtk_text_buffer_get_iter_at_mark (priv->text_buffer, &iter, insert);

	gtk_text_view_get_iter_location (GTK_TEXT_VIEW (priv->msg_body), &iter, &rectangle);
	offset_min = priv->msg_body->allocation.y + rectangle.y;
	offset_max = offset_min + rectangle.height;

	vadj = hildon_pannable_area_get_vadjustment (HILDON_PANNABLE_AREA (priv->pannable));
	offset_min = MAX (offset_min - 48, 0);
	offset_max = MIN (offset_max + 48, vadj->upper);

	gtk_adjustment_clamp_page (vadj, (gdouble) offset_min, (gdouble) offset_max);

	priv->correct_scroll_idle = 0;
	return FALSE;
}

static void
correct_scroll_without_drag_check (ModestMsgEditWindow *w, gboolean only_if_focused)
{
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(w);

	if (only_if_focused && !gtk_widget_is_focus (priv->msg_body))
		return;

	if (priv->correct_scroll_idle > 0) {
		return;
	}

	priv->correct_scroll_idle = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
						     (GSourceFunc) correct_scroll_without_drag_check_idle,
						     g_object_ref (w),
						     g_object_unref);
}

static void
correct_scroll (ModestMsgEditWindow *w)
{
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(w);
	if (gtk_grab_get_current () == priv->msg_body) {
		if (priv->scroll_drag_timeout_id == 0) {
			priv->scroll_drag_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
									   500,
									   (GSourceFunc) scroll_drag_timeout,
									   g_object_ref (w),
									   g_object_unref);
		}
		return;
	}

	correct_scroll_without_drag_check (w, TRUE);
}

static void
text_buffer_end_user_action (GtkTextBuffer *buffer,
			     ModestMsgEditWindow *userdata)
{

	correct_scroll (userdata);
}

static void
text_buffer_mark_set (GtkTextBuffer *buffer,
		      GtkTextIter *iter,
		      GtkTextMark *mark,
		      ModestMsgEditWindow *userdata)
{
	gtk_text_buffer_begin_user_action (buffer);
	gtk_text_buffer_end_user_action (buffer);
}

static void
cut_clipboard_check (GtkTextView *text_view,
		     gpointer userdata)
{
	GtkTextBuffer *buffer;
	
	buffer = gtk_text_view_get_buffer (text_view);
	if (!modest_text_utils_buffer_selection_is_valid (buffer)) {
		g_signal_stop_emission_by_name ((gpointer )text_view, "cut-clipboard");
	}
}

static void
copy_clipboard_check (GtkTextView *text_view,
		     gpointer userdata)
{
	GtkTextBuffer *buffer;
	
	buffer = gtk_text_view_get_buffer (text_view);
	if (!modest_text_utils_buffer_selection_is_valid (buffer)) {
		g_signal_stop_emission_by_name ((gpointer )text_view, "copy-clipboard");
	}
}

static void
attachment_deleted (ModestAttachmentsView *attachments_view,
		    gpointer user_data)
{
	modest_msg_edit_window_remove_attachments (MODEST_MSG_EDIT_WINDOW (user_data),
						   NULL);
}

static void
connect_signals (ModestMsgEditWindow *obj)
{
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);

	g_signal_connect (G_OBJECT (priv->text_buffer), "refresh_attributes",
			  G_CALLBACK (text_buffer_refresh_attributes), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "can-undo",
			  G_CALLBACK (text_buffer_can_undo), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "can-redo",
			  G_CALLBACK (text_buffer_can_redo), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "changed",
                          G_CALLBACK (body_changed), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "insert-text", 
			  G_CALLBACK (body_insert_text), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "modified-changed",
                          G_CALLBACK (body_changed), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "end-user-action",
			  G_CALLBACK (text_buffer_end_user_action), obj);
	g_signal_connect (G_OBJECT (priv->text_buffer), "mark-set",
			  G_CALLBACK (text_buffer_mark_set), obj);
	g_signal_connect_after (G_OBJECT (priv->text_buffer), "apply-tag",
				G_CALLBACK (text_buffer_apply_tag), obj);
	g_signal_connect_swapped (G_OBJECT (priv->to_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);
	g_signal_connect_swapped (G_OBJECT (priv->cc_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);
	g_signal_connect_swapped (G_OBJECT (priv->bcc_field), "open-addressbook", 
				  G_CALLBACK (modest_msg_edit_window_open_addressbook), obj);

	g_signal_connect (G_OBJECT (priv->send_button), "clicked",
			  G_CALLBACK (modest_ui_actions_on_send), obj);

	g_signal_connect (G_OBJECT (priv->from_field), "value-changed",
			  G_CALLBACK (from_field_changed), obj);

	g_signal_connect (G_OBJECT (priv->msg_body), "focus-in-event",
			  G_CALLBACK (msg_body_focus), obj);
	g_signal_connect (G_OBJECT (priv->msg_body), "focus-out-event",
			  G_CALLBACK (msg_body_focus), obj);
	g_signal_connect (G_OBJECT (obj), "set-focus", G_CALLBACK (window_focus), obj);
	g_signal_connect (G_OBJECT (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->to_field))),
			  "changed", G_CALLBACK (recpt_field_changed), obj);
	g_signal_connect (G_OBJECT (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->cc_field))),
			  "changed", G_CALLBACK (recpt_field_changed), obj);
	g_signal_connect (G_OBJECT (modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR (priv->bcc_field))),
			  "changed", G_CALLBACK (recpt_field_changed), obj);
	g_signal_connect (G_OBJECT (priv->subject_field), "changed", G_CALLBACK (subject_field_changed), obj);
	g_signal_connect_after (G_OBJECT (priv->subject_field), "move-cursor", G_CALLBACK (subject_field_move_cursor), obj);
	g_signal_connect (G_OBJECT (priv->subject_field), "insert-text", G_CALLBACK (subject_field_insert_text), obj);

	g_signal_connect (G_OBJECT (priv->find_toolbar), "search", G_CALLBACK (modest_msg_edit_window_find_toolbar_search), obj);
	g_signal_connect (G_OBJECT (priv->find_toolbar), "close", G_CALLBACK (modest_msg_edit_window_find_toolbar_close), obj);
 
	priv->clipboard_change_handler_id = 
		g_signal_connect (G_OBJECT (gtk_clipboard_get (GDK_SELECTION_PRIMARY)), "owner-change",
				  G_CALLBACK (modest_msg_edit_window_clipboard_owner_change), obj);
	priv->default_clipboard_change_handler_id = 
		g_signal_connect (G_OBJECT (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD)), "owner-change",
				  G_CALLBACK (modest_msg_edit_window_clipboard_owner_change), obj);

	g_signal_connect (G_OBJECT (priv->msg_body), "cut-clipboard", G_CALLBACK (cut_clipboard_check), NULL);
	g_signal_connect (G_OBJECT (priv->msg_body), "copy-clipboard", G_CALLBACK (copy_clipboard_check), NULL);
	g_signal_connect (G_OBJECT (priv->attachments_view), "delete", G_CALLBACK (attachment_deleted), obj);
}

static void
init_wp_text_view_style ()
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		gtk_rc_parse_string ("class \"WPTextView\" style \"fremantle-textview\"");
		initialized = TRUE;
	}
}	

static void
init_window (ModestMsgEditWindow *obj)
{
	GtkWidget *to_caption, *subject_caption;
	GtkWidget *main_vbox;
	ModestMsgEditWindowPrivate *priv;
	GtkActionGroup *action_group;
	ModestWindowPrivate *parent_priv;
	GdkPixbuf *window_icon = NULL;
	GError *error = NULL;

	GtkSizeGroup *title_size_group;
	GtkSizeGroup *value_size_group;
	GtkWidget *window_box;
	GtkWidget *window_align;
#if (GTK_MINOR_VERSION >= 10)
	GdkAtom deserialize_type;
#endif
	GtkWidget *from_send_hbox;
	GtkWidget *send_icon;
	GtkWidget *attachments_label;
	GtkWidget *branding_box;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (obj);

	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMsgEditWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_msg_edit_action_entries,
				      G_N_ELEMENTS (modest_msg_edit_action_entries),
				      obj);
	gtk_action_group_add_actions (action_group,
				      hildon2_msg_edit_action_entries,
				      G_N_ELEMENTS (hildon2_msg_edit_action_entries),
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

	parent_priv->menubar = NULL;

	title_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	value_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Note: This ModestPairList* must exist for as long as the picker
	 * that uses it, because the ModestSelectorPicker uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
 	priv->from_field    = modest_selector_picker_new (MODEST_EDITABLE_SIZE,
							  HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
							  NULL, g_str_equal);
	modest_selector_picker_set_value_max_chars (MODEST_SELECTOR_PICKER (priv->from_field), MAX_FROM_VALUE);
	modest_maemo_utils_set_hbutton_layout (title_size_group, NULL, 
					       _("mail_va_from"), priv->from_field);
	hildon_button_set_alignment (HILDON_BUTTON (priv->from_field), 0.0, 0.5, 1.0, 1.0);
	hildon_button_set_title_alignment (HILDON_BUTTON (priv->from_field), 0.0, 0.5);
	hildon_button_set_value_alignment (HILDON_BUTTON (priv->from_field), 1.0, 0.5);

	priv->to_field      = modest_recpt_editor_new ();
	priv->cc_field      = modest_recpt_editor_new ();
	priv->bcc_field     = modest_recpt_editor_new ();
	modest_recpt_editor_set_show_abook_button (MODEST_RECPT_EDITOR (priv->to_field), FALSE);
	modest_recpt_editor_set_show_abook_button (MODEST_RECPT_EDITOR (priv->cc_field), FALSE);
	modest_recpt_editor_set_show_abook_button (MODEST_RECPT_EDITOR (priv->bcc_field), FALSE);
	priv->subject_box = gtk_hbox_new (FALSE, MODEST_MARGIN_NONE);
	priv->priority_icon = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (priv->subject_box), priv->priority_icon, FALSE, FALSE, 0);
	priv->subject_field = hildon_entry_new (MODEST_EDITABLE_SIZE);
	gtk_entry_set_max_length (GTK_ENTRY (priv->subject_field) ,SUBJECT_MAX_LENGTH);
	g_object_set (G_OBJECT (priv->subject_field), "truncate-multiline", TRUE, NULL);
	hildon_gtk_entry_set_input_mode (GTK_ENTRY (priv->subject_field), 
					 HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_AUTOCAP);
	gtk_box_pack_start (GTK_BOX (priv->subject_box), priv->subject_field, TRUE, TRUE, 0);
	priv->attachments_view = modest_attachments_view_new (NULL);
	modest_attachments_view_set_style (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
					   MODEST_ATTACHMENTS_VIEW_STYLE_NO_FOCUS);
	
	priv->header_box = gtk_vbox_new (FALSE, 0);
	
	to_caption = _create_addressbook_box
		(title_size_group, value_size_group,
		 _("mail_va_to"), priv->to_field);
	priv->cc_caption = _create_addressbook_box
		(title_size_group, value_size_group,
		 _("mail_va_cc"), priv->cc_field);
	priv->bcc_caption = _create_addressbook_box
		(title_size_group, value_size_group,
		 _("mail_va_hotfix1"), priv->bcc_field);
	subject_caption = modest_maemo_utils_create_captioned (title_size_group, value_size_group,
							       _("mail_va_subject"), FALSE, priv->subject_box);
	priv->attachments_caption = modest_maemo_utils_create_captioned_with_size_type (NULL, NULL,
											_("mail_va_attachment"), 
											FALSE,
											priv->attachments_view,
											HILDON_SIZE_AUTO_WIDTH |
											HILDON_SIZE_AUTO_HEIGHT);
	attachments_label = modest_maemo_utils_captioned_get_label_widget (priv->attachments_caption);
	hildon_gtk_widget_set_theme_size (attachments_label, HILDON_SIZE_AUTO_HEIGHT);


	priv->send_button = hildon_gtk_button_new (HILDON_SIZE_FINGER_HEIGHT);
	send_icon = gtk_image_new_from_icon_name (MODEST_TOOLBAR_ICON_MAIL_SEND, HILDON_ICON_SIZE_FINGER);
	gtk_container_add (GTK_CONTAINER (priv->send_button), send_icon);
	gtk_widget_set_size_request (GTK_WIDGET (priv->send_button), 148, -1);

	g_object_unref (title_size_group);
	g_object_unref (value_size_group);

	priv->brand_icon = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->brand_icon), 0.5, 0.5);
	priv->brand_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->brand_label), 0.0, 0.5);
	gtk_widget_set_no_show_all (priv->brand_icon, TRUE);
	gtk_widget_set_no_show_all (priv->brand_label, TRUE);

	from_send_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (from_send_hbox), priv->from_field, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (from_send_hbox), priv->send_button, FALSE, FALSE, 0);

	branding_box = gtk_hbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (branding_box), priv->brand_icon, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (branding_box), priv->brand_label, TRUE, TRUE, 0);


	gtk_box_pack_start (GTK_BOX (priv->header_box), from_send_hbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), to_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), priv->cc_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), priv->bcc_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), subject_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), priv->attachments_caption, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->header_box), branding_box, FALSE, FALSE, 0);
	gtk_widget_set_no_show_all (priv->attachments_caption, TRUE);

	init_wp_text_view_style ();

	priv->msg_body = modest_wp_text_view_new ();
	

	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->msg_body), GTK_WRAP_WORD_CHAR);
	priv->text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));
	g_object_set (priv->text_buffer, "font_scale", DEFAULT_FONT_SCALE, NULL);
	wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
#if (GTK_MINOR_VERSION >= 10)
	gtk_text_buffer_register_serialize_tagset(GTK_TEXT_BUFFER(priv->text_buffer), NULL);
	deserialize_type = gtk_text_buffer_register_deserialize_tagset(GTK_TEXT_BUFFER(priv->text_buffer), 
								       NULL);
	gtk_text_buffer_deserialize_set_can_create_tags (GTK_TEXT_BUFFER (priv->text_buffer), 
							 deserialize_type, TRUE);
#endif
	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);

	priv->find_toolbar = hildon_find_toolbar_new (NULL);
	gtk_widget_set_no_show_all (priv->find_toolbar, TRUE);

/* 	g_signal_connect (G_OBJECT (obj), "key_pressed", G_CALLBACK (on_key_pressed), NULL) */

	priv->pannable = hildon_pannable_area_new ();

	g_object_set (G_OBJECT (priv->pannable), "hscrollbar-policy", GTK_POLICY_NEVER, NULL);
	
	main_vbox = gtk_vbox_new  (FALSE, DEFAULT_MAIN_VBOX_SPACING);
	window_align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (window_align), 0, 0, MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DEFAULT);

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->header_box, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->msg_body, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (window_align), main_vbox);

	hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA (priv->pannable), window_align);
	gtk_widget_show_all (GTK_WIDGET(priv->pannable));
	
	window_box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER(obj), window_box);

	gtk_box_pack_start (GTK_BOX (window_box), priv->pannable, TRUE, TRUE, 0);

	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_MSG_EDIT_ICON, MODEST_ICON_SIZE_BIG); 
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (obj), window_icon);
		g_object_unref (window_icon);
	}	
}
	
static void
modest_msg_edit_window_disconnect_signals (ModestWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (gtk_clipboard_get (GDK_SELECTION_PRIMARY) &&
	    g_signal_handler_is_connected (gtk_clipboard_get (GDK_SELECTION_PRIMARY), 
					   priv->clipboard_change_handler_id))
		g_signal_handler_disconnect (gtk_clipboard_get (GDK_SELECTION_PRIMARY), 
					     priv->clipboard_change_handler_id);
	if (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD) &&
	    g_signal_handler_is_connected (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), 
					   priv->default_clipboard_change_handler_id))
		g_signal_handler_disconnect (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), 
					     priv->default_clipboard_change_handler_id);

	if (priv->account_removed_handler_id && 
	    g_signal_handler_is_connected (modest_runtime_get_account_store (), 
					   priv->account_removed_handler_id))
		g_signal_handler_disconnect(modest_runtime_get_account_store (), 
					   priv->account_removed_handler_id);
}

static void
modest_msg_edit_window_finalize (GObject *obj)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (obj);

	if (priv->max_chars_banner) {
		g_object_weak_unref (G_OBJECT (priv->max_chars_banner), (GWeakNotify) max_chars_banner_unref, obj);
		priv->max_chars_banner = FALSE;
	}

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_msg_edit_window_disconnect_signals (MODEST_WINDOW (obj));

	if (priv->font_dialog != NULL) {
		gtk_dialog_response (GTK_DIALOG (priv->font_dialog), GTK_RESPONSE_NONE);
	}

	if (priv->clipboard_text != NULL) {
		g_free (priv->clipboard_text);
		priv->clipboard_text = NULL;
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
	if (priv->outbox_msg != NULL) {
		TnyHeader *header = tny_msg_get_header (priv->outbox_msg);
		if (TNY_IS_HEADER (header)) {
			ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();
			modest_window_mgr_unregister_header (mgr, header);
		}
		g_object_unref (priv->outbox_msg);
		priv->outbox_msg = NULL;
	}
	if (priv->correct_scroll_idle > 0) {
		g_source_remove (priv->correct_scroll_idle);
		priv->correct_scroll_idle = 0;
	}
	if (priv->scroll_drag_timeout_id > 0) {
		g_source_remove (priv->scroll_drag_timeout_id);
		priv->scroll_drag_timeout_id = 0;
	}
	if (priv->clipboard_owner_idle > 0) {
		g_source_remove (priv->clipboard_owner_idle);
		priv->clipboard_owner_idle = 0;
	}
	if (priv->original_account_name)
		g_free (priv->original_account_name);
	if (priv->original_mailbox)
		g_free (priv->original_mailbox);
	g_free (priv->msg_uid);
	g_free (priv->last_search);
	g_free (priv->references);
	g_free (priv->in_reply_to);
	g_object_unref (priv->faces_model);
	g_object_unref (priv->sizes_model);
	g_object_unref (priv->attachments);
	g_object_unref (priv->images);

	/* This had to stay alive for as long as the picker that used it: */
	modest_pair_list_free (priv->from_field_protos);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
pixbuf_size_prepared (GdkPixbufLoader *loader,
		      gint width,
		      gint height,
		      ModestMsgEditWindow *self)
{
	gint new_height, new_width;
	gboolean set_size;
	
	new_height = height;
	new_width = width;
	set_size = FALSE;

	if (width > IMAGE_MAX_WIDTH) {
		new_height = height * IMAGE_MAX_WIDTH / width;
		new_width = IMAGE_MAX_WIDTH;
	}

	gdk_pixbuf_loader_set_size (loader, new_width, new_height);
}

static GdkPixbuf *
pixbuf_from_stream (TnyStream *stream,
		    const gchar *mime_type,
		    guint64 *stream_size,
		    ModestMsgEditWindow *self)
{
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;
	guint64 size;
	GError *error = NULL;

	size = 0;

	loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, NULL);

	if (loader == NULL) {
		if (stream_size)
			*stream_size = 0;
		return NULL;
	}
	g_signal_connect (G_OBJECT (loader), "size-prepared", G_CALLBACK (pixbuf_size_prepared), self);

	hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), TRUE);

	tny_stream_reset (TNY_STREAM (stream));
	while (!tny_stream_is_eos (TNY_STREAM (stream))) {
		unsigned char read_buffer[128];
		gint readed;
		readed = tny_stream_read (TNY_STREAM (stream), (char *) read_buffer, 128);
		size += readed;
		if (!gdk_pixbuf_loader_write (loader, read_buffer, readed, &error)) {
			break;
		}
		/* Allow some UI responsiveness */
		while (gtk_events_pending ())
			gtk_main_iteration ();
	}
	hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), FALSE);

	gdk_pixbuf_loader_close (loader, &error);

	if (error)
		g_error_free (error);
	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (pixbuf) 
		g_object_ref (pixbuf);
	g_object_unref (loader);

	if (!pixbuf)
		return NULL;

	if (gdk_pixbuf_get_width (pixbuf) > IMAGE_MAX_WIDTH) {
		GdkPixbuf *new_pixbuf;
		gint new_height;
		new_height = (gdk_pixbuf_get_height (pixbuf) * IMAGE_MAX_WIDTH) /
			gdk_pixbuf_get_width (pixbuf);
		new_pixbuf = gdk_pixbuf_scale_simple (pixbuf, IMAGE_MAX_WIDTH, new_height, GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = new_pixbuf;
	}

	if (stream_size)
		*stream_size = size;

	return pixbuf;
}

static void
replace_with_images (ModestMsgEditWindow *self, TnyList *attachments)
{
	ModestMsgEditWindowPrivate *priv;
	TnyIterator *iter;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	g_object_ref (self);
	for (iter = tny_list_create_iterator (attachments);
	     !tny_iterator_is_done (iter);
	     tny_iterator_next (iter)) {
		TnyMimePart *part = (TnyMimePart *) tny_iterator_get_current (iter);
		const gchar *cid = tny_mime_part_get_content_id (part);
		const gchar *mime_type = tny_mime_part_get_content_type (part);
		if ((cid != NULL)&&(mime_type != NULL)) {
			guint64 stream_size;
			TnyStream *stream = tny_mime_part_get_decoded_stream (part);
			GdkPixbuf *pixbuf = pixbuf_from_stream (stream, mime_type, &stream_size, self);


			g_object_unref (stream);

			if (pixbuf != NULL) {
				priv->images_count ++;
				priv->images_size += stream_size;
				wp_text_buffer_replace_image (WP_TEXT_BUFFER (priv->text_buffer), cid, pixbuf);
				g_object_unref (pixbuf);
			}
		}
		g_object_unref (part);
	}
	g_object_unref (iter);
	g_object_unref (self);
}

static void
get_related_images (ModestMsgEditWindow *self, TnyMsg *msg)
{
	TnyMimePart *parent = NULL;
	const gchar *content_type = NULL;
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	content_type = tny_mime_part_get_content_type (TNY_MIME_PART (msg));

	if (content_type && !g_strcasecmp (content_type, "multipart/related")) {
		parent = g_object_ref (msg);
	} else if (content_type && !g_strcasecmp (content_type, "multipart/mixed")) {
		TnyList *parts = TNY_LIST (tny_simple_list_new ());
		TnyIterator *iter;

		tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);
		iter = tny_list_create_iterator (parts);
		while (!tny_iterator_is_done (iter)) {
			TnyMimePart *part;
			part = TNY_MIME_PART (tny_iterator_get_current (iter));
			content_type = tny_mime_part_get_content_type (part);
			if (content_type && !g_strcasecmp (content_type, "multipart/related")) {
				parent = part;
				break;
			} else {
				g_object_unref (part);
			}
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (parts);
	}

	if (parent != NULL) {
		TnyList *parts = TNY_LIST (tny_simple_list_new ());
		TnyIterator *iter;

		tny_mime_part_get_parts (TNY_MIME_PART (parent), parts);
		iter = tny_list_create_iterator (parts);
		while (!tny_iterator_is_done (iter)) {
			TnyMimePart *part;
			part = TNY_MIME_PART (tny_iterator_get_current (iter));
			content_type = tny_mime_part_get_content_type (part);
			if (content_type && g_str_has_prefix (content_type, "image/")) {
				tny_list_prepend (priv->images, (GObject *) part);
			} 
			g_object_unref (part);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (parts);
		g_object_unref (parent);
	}
}

static void
update_next_cid (ModestMsgEditWindow *self, TnyList *attachments)
{
	TnyIterator *iter;
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	for (iter = tny_list_create_iterator (attachments) ; 
	     !tny_iterator_is_done (iter);
	     tny_iterator_next (iter)) {
		TnyMimePart *part = (TnyMimePart *) tny_iterator_get_current (iter);
		const gchar *cid = tny_mime_part_get_content_id (part);
		if (cid != NULL) {
			char *invalid = NULL;
			gint int_cid = strtol (cid, &invalid, 10);
			if ((invalid != NULL) && (*invalid == '\0') && (int_cid >= priv->next_cid)) {
				priv->next_cid = int_cid + 1;
			}
		}
		g_object_unref (part);
	}
	g_object_unref (iter);
}

static void
set_msg (ModestMsgEditWindow *self, TnyMsg *msg, gboolean preserve_is_rich)
{
	TnyHeader *header;
	gchar *to, *cc, *bcc, *subject;
	gchar *body;
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkTextIter iter;
	TnyHeaderFlags priority_flags;
	TnyFolder *msg_folder;
	gboolean is_html = FALSE;
	gboolean field_view_set;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (TNY_IS_MSG (msg));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (self);

	header = tny_msg_get_header (msg);
	to      = tny_header_dup_to (header);
	cc      = tny_header_dup_cc (header);
	bcc     = tny_header_dup_bcc (header);
	subject = tny_header_dup_subject (header);

	modest_tny_msg_get_references (TNY_MSG (msg), NULL, &(priv->references), &(priv->in_reply_to));
	priority_flags = tny_header_get_priority (header);

	if (to)
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->to_field),  to);

	field_view_set = TRUE;
	if (cc) {
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->cc_field),  cc);
		gtk_widget_set_no_show_all (priv->cc_caption, FALSE);
		gtk_widget_show (priv->cc_caption);
	} else if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_CC, NULL)) {
		gtk_widget_set_no_show_all (priv->cc_caption, TRUE);
		gtk_widget_hide (priv->cc_caption);
		field_view_set = FALSE;
	}
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (priv->cc_button), field_view_set);

	field_view_set = TRUE;
	if (bcc) {
		modest_recpt_editor_set_recipients (MODEST_RECPT_EDITOR (priv->bcc_field), bcc);
		gtk_widget_set_no_show_all (priv->bcc_caption, FALSE);
		gtk_widget_show (priv->bcc_caption);
	} else if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_BCC, NULL)) {
		gtk_widget_set_no_show_all (priv->bcc_caption, TRUE);
		gtk_widget_hide (priv->bcc_caption);
		field_view_set = FALSE;
	}
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (priv->bcc_button), field_view_set);


	if (subject)
		gtk_entry_set_text (GTK_ENTRY(priv->subject_field), subject);
	modest_msg_edit_window_set_priority_flags (MODEST_MSG_EDIT_WINDOW(self),
						   priority_flags);

	update_window_title (self);

	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
	body = modest_tny_msg_get_body (msg, TRUE, &is_html);

	if ((body == NULL)||(body[0] == '\0')) {
		g_free (body);
		body = modest_text_utils_convert_to_html ("");
		is_html = FALSE;
	}
	wp_text_buffer_load_document_begin (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
	wp_text_buffer_load_document_write (WP_TEXT_BUFFER (priv->text_buffer),
					    (gchar *) body,
					    strlen (body));
	wp_text_buffer_load_document_end (WP_TEXT_BUFFER (priv->text_buffer));
	g_free (body);

	/* Add attachments to the view */
	modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), msg);
	priv->attachments = modest_attachments_view_get_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	if (tny_list_get_length (priv->attachments) == 0) {
		gtk_widget_hide (priv->attachments_caption);
	} else {
		gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
		gtk_widget_show_all (priv->attachments_caption);
	}
	get_related_images (self, msg);
	update_next_cid (self, priv->attachments);
	update_next_cid (self, priv->images);
	replace_with_images (self, priv->images);

	if (preserve_is_rich && !is_html) {
		wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), FALSE);
	/* Get the default format required from configuration */
	} else if (!preserve_is_rich && !modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_PREFER_FORMATTED_TEXT, NULL)) {
		wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), FALSE);
	}

	/* Set the default focus depending on having already a To: field or not */
	if ((!to)||(*to == '\0')) {
		modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->to_field));
	} else {
		gtk_widget_grab_focus (priv->msg_body);
	}

	DEBUG_BUFFER (WP_TEXT_BUFFER (priv->text_buffer));

	gtk_text_buffer_get_start_iter (priv->text_buffer, &iter);
	gtk_text_buffer_place_cursor (priv->text_buffer, &iter);

	modest_msg_edit_window_set_modified (self, FALSE);

	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (self));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
	text_buffer_can_undo (priv->text_buffer, FALSE, self);
	text_buffer_can_redo (priv->text_buffer, FALSE, self);

	if (priv->msg_uid) {
		g_free (priv->msg_uid);
		priv->msg_uid = NULL;
	}

	/* we should set a reference to the incoming message if it is a draft */
	msg_folder = tny_msg_get_folder (msg);
	if (msg_folder) {		
		if (modest_tny_folder_is_local_folder (msg_folder)) {
			TnyFolderType type = modest_tny_folder_get_local_or_mmc_folder_type (msg_folder);
			if (type == TNY_FOLDER_TYPE_INVALID)
				g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
			
			if (type == TNY_FOLDER_TYPE_DRAFTS) 
				priv->draft_msg = g_object_ref(msg);
			if (type == TNY_FOLDER_TYPE_OUTBOX)
				priv->outbox_msg = g_object_ref(msg);
			priv->msg_uid = modest_tny_folder_get_header_unique_id (header);
		}
		g_object_unref (msg_folder);
	}

	g_free (to);
	g_free (subject);
	g_free (cc);
	g_free (bcc);
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
	gchar *markup;
	GtkWidget *arrow;
	GtkWidget *hbox;

	/* Toolbar */
	parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (parent_priv->toolbar), FALSE);
	gtk_toolbar_set_icon_size (GTK_TOOLBAR (parent_priv->toolbar), HILDON_ICON_SIZE_FINGER);
	hildon_window_add_toolbar (HILDON_WINDOW (window), GTK_TOOLBAR (parent_priv->toolbar));

	/* Font color placeholder */
	placeholder = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/FontColor");
	insert_index = gtk_toolbar_get_item_index(GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM(placeholder));

	/* font color */
	priv->font_color_toolitem = GTK_WIDGET (gtk_tool_item_new ());
	priv->font_color_button = hildon_color_button_new ();
	gtk_widget_set_size_request (priv->font_color_button, -1, 48);
	GTK_WIDGET_UNSET_FLAGS (priv->font_color_toolitem, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (priv->font_color_button, GTK_CAN_FOCUS);
	gtk_container_add (GTK_CONTAINER (priv->font_color_toolitem), priv->font_color_button);
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->font_color_toolitem), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->font_color_toolitem), TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(parent_priv->toolbar), GTK_TOOL_ITEM (priv->font_color_toolitem), insert_index);
	g_signal_connect_swapped (G_OBJECT (priv->font_color_button), 
				  "notify::color", 
				  G_CALLBACK (modest_msg_edit_window_color_button_change), 
				  window);

	/* Font size and face placeholder */
	placeholder = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/FontAttributes");
	insert_index = gtk_toolbar_get_item_index(GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM(placeholder));
	/* font_size */
	tool_item = GTK_WIDGET (gtk_tool_button_new (NULL, NULL));
	priv->size_tool_button_label = gtk_label_new (NULL);
	snprintf(size_text, sizeof(size_text), "%d", wp_font_size[DEFAULT_FONT_SIZE]);
	markup = g_strconcat ("<span font_family='", DEFAULT_SIZE_BUTTON_FONT_FAMILY, "'>",
			      size_text, "</span>", NULL);
	gtk_label_set_markup (GTK_LABEL (priv->size_tool_button_label), markup);
	gtk_misc_set_alignment (GTK_MISC (priv->size_tool_button_label), 1.0, 0.5);
	g_free (markup);
	hildon_helper_set_logical_font (priv->size_tool_button_label, "LargeSystemFont");
	hbox = gtk_hbox_new (MODEST_MARGIN_DEFAULT, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), priv->size_tool_button_label, TRUE, TRUE, 0);
	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_misc_set_alignment (GTK_MISC (arrow), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (arrow, FALSE);
	gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (tool_item), hbox);
	sizes_menu = gtk_menu_new ();
	priv->sizes_model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	for (size_index = 0; size_index < WP_FONT_SIZE_COUNT; size_index++) {
		GtkTreeIter iter;

		snprintf(size_text, sizeof(size_text), "%d", wp_font_size[size_index]);
		gtk_list_store_append (GTK_LIST_STORE (priv->sizes_model), &iter);

		gtk_list_store_set (GTK_LIST_STORE (priv->sizes_model), &iter, 
				    0, size_text,
				    -1);

		if (wp_font_size[size_index] == 12)
			priv->current_size_index = size_index;
					
	}

	g_signal_connect (G_OBJECT (tool_item), "clicked", G_CALLBACK (font_size_clicked), window);
	gtk_toolbar_insert (GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	priv->font_size_toolitem = tool_item;

	/* font face */
	tool_item = GTK_WIDGET (gtk_tool_button_new (NULL, NULL));
	priv->font_tool_button_label = gtk_label_new (NULL);
	markup = g_strconcat ("<span font_family='", wp_get_font_name(DEFAULT_FONT), "'>Tt</span>", NULL);
	gtk_label_set_markup (GTK_LABEL (priv->font_tool_button_label), markup);
	gtk_misc_set_alignment (GTK_MISC (priv->font_tool_button_label), 1.0, 0.5);
	g_free(markup);
	hildon_helper_set_logical_font (priv->font_tool_button_label, "LargeSystemFont");
	hbox = gtk_hbox_new (MODEST_MARGIN_DEFAULT, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), priv->font_tool_button_label, TRUE, TRUE, 0);
	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_misc_set_alignment (GTK_MISC (arrow), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (arrow, FALSE);
	gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (tool_item), hbox);
	fonts_menu = gtk_menu_new ();
	priv->faces_model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	for (font_index = 0; font_index < wp_get_font_count (); font_index++) {
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE (priv->faces_model), &iter);

		gtk_list_store_set (GTK_LIST_STORE (priv->faces_model), &iter, 
				    0, wp_get_font_name (font_index),
				    -1);

		if (font_index == DEFAULT_FONT)
			priv->current_face_index = font_index;
	}
	g_signal_connect (G_OBJECT (tool_item), "clicked", G_CALLBACK (font_face_clicked), window);
	gtk_toolbar_insert (GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	priv->font_face_toolitem = tool_item;

	/* Set expand and homogeneous for remaining items */
	tool_item = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);
	tool_item = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), TRUE);

	/* Explicitelly show all the toolbar (a normal gtk_widget_show
	   will not show the tool items added to the placeholders) */
	gtk_widget_show_all (parent_priv->toolbar);

	/* Set the no show all *after* showing all items. We do not
	   want the toolbar to be shown with a show all because it
	   could go agains the gconf setting regarding showing or not
	   the toolbar of the editor window */
	gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);
}



ModestWindow*
modest_msg_edit_window_new (TnyMsg *msg, const gchar *account_name, const gchar *mailbox, gboolean preserve_is_rich)
{
	GObject *obj;
	ModestWindowPrivate *parent_priv;
	ModestMsgEditWindowPrivate *priv;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	ModestDimmingRulesGroup *clipboard_rules_group = NULL;
	ModestWindowMgr *mgr = NULL;

	g_return_val_if_fail (msg, NULL);
	g_return_val_if_fail (account_name, NULL);

	mgr = modest_runtime_get_window_mgr ();
	
	obj = G_OBJECT (modest_window_mgr_get_msg_edit_window (mgr));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (obj);

	/* Menubar. Update the state of some toggles */
	priv->from_field_protos = get_transports ();
	priv->original_mailbox = NULL;
 	modest_selector_picker_set_pair_list (MODEST_SELECTOR_PICKER (priv->from_field), priv->from_field_protos);
	modest_selector_picker_set_active_id (MODEST_SELECTOR_PICKER (priv->from_field), (gpointer) account_name);
	priv->last_from_account = modest_selector_picker_get_active_id (MODEST_SELECTOR_PICKER (priv->from_field));
	update_branding (MODEST_MSG_EDIT_WINDOW (obj), priv->last_from_account);
	if (mailbox && modest_pair_list_find_by_first_as_string (priv->from_field_protos, mailbox)) {
		modest_selector_picker_set_active_id (MODEST_SELECTOR_PICKER (priv->from_field), (gpointer) mailbox);
		priv->original_mailbox = g_strdup (mailbox);
	} else if (modest_account_mgr_account_is_multimailbox (modest_runtime_get_account_mgr (), account_name, NULL)) {
		/* We set the first mailbox as the active mailbox */
		priv->original_mailbox = multimailbox_get_default_mailbox (account_name);
		if (priv->original_mailbox != NULL)
			modest_selector_picker_set_active_id (MODEST_SELECTOR_PICKER (priv->from_field),
							      (gpointer) priv->original_mailbox);
		else
			modest_selector_picker_set_active_id (MODEST_SELECTOR_PICKER (priv->from_field),
							      (gpointer) account_name);
	} else {
		modest_selector_picker_set_active_id (MODEST_SELECTOR_PICKER (priv->from_field), (gpointer) account_name);
	}
	priv->last_from_account = modest_selector_picker_get_active_id (MODEST_SELECTOR_PICKER (priv->from_field));
	hildon_button_set_title (HILDON_BUTTON (priv->from_field),
				 _("mail_va_from"));
	hildon_button_set_value (HILDON_BUTTON (priv->from_field), 
				 hildon_touch_selector_get_current_text 
				 (HILDON_TOUCH_SELECTOR (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (priv->from_field)))));
	modest_msg_edit_window_setup_toolbar (MODEST_MSG_EDIT_WINDOW (obj));
	hildon_window_add_toolbar (HILDON_WINDOW (obj), GTK_TOOLBAR (priv->find_toolbar));

	/* Init window */
	connect_signals (MODEST_MSG_EDIT_WINDOW(obj));

	restore_settings (MODEST_MSG_EDIT_WINDOW(obj));
		
	modest_window_set_active_account (MODEST_WINDOW(obj), account_name);
	modest_window_set_active_mailbox (MODEST_WINDOW(obj), priv->original_mailbox);

	priv->original_account_name = (account_name) ? g_strdup (account_name) : NULL;

	toolbar_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_TOOLBAR, TRUE);
	clipboard_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_CLIPBOARD, FALSE);
	/* Add common dimming rules */
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_msg_edit_window_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_msg_edit_window_toolbar_dimming_entries),
					      MODEST_WINDOW (obj));
	modest_dimming_rules_group_add_widget_rule (toolbar_rules_group, priv->font_color_toolitem,
						    G_CALLBACK (modest_ui_dimming_rules_on_set_style),
						    MODEST_WINDOW (obj));
	modest_dimming_rules_group_add_widget_rule (toolbar_rules_group, priv->font_size_toolitem,
						    G_CALLBACK (modest_ui_dimming_rules_on_set_style),
						    MODEST_WINDOW (obj));
	modest_dimming_rules_group_add_widget_rule (toolbar_rules_group, priv->font_face_toolitem,
						    G_CALLBACK (modest_ui_dimming_rules_on_set_style),
						    MODEST_WINDOW (obj));
	modest_dimming_rules_group_add_widget_rule (toolbar_rules_group, priv->send_button,
						    G_CALLBACK (modest_ui_dimming_rules_on_send),
						    MODEST_WINDOW (obj));
	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, clipboard_rules_group);

	/* Setup app menu */
	setup_menu (MODEST_MSG_EDIT_WINDOW (obj));

        /* Checks the dimming rules */
	g_object_unref (toolbar_rules_group);
	g_object_unref (clipboard_rules_group);
	gtk_widget_hide (priv->priority_icon);
	gtk_widget_queue_resize (priv->subject_box);
	set_msg (MODEST_MSG_EDIT_WINDOW (obj), msg, preserve_is_rich);

	text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (obj));

	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (obj));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (obj));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (obj), MODEST_DIMMING_RULES_CLIPBOARD);
	priv->update_caption_visibility = TRUE;

	modest_msg_edit_window_set_modified (MODEST_MSG_EDIT_WINDOW (obj), FALSE);

	/* Track account-removed signal, this window should be closed
	   in the case we're creating a mail associated to the account
	   that is deleted */
	priv->account_removed_handler_id = 
		g_signal_connect (G_OBJECT (modest_runtime_get_account_store ()),
				  "account_removed",
				  G_CALLBACK(on_account_removed),
				  obj);

	modest_msg_edit_window_clipboard_owner_handle_change_in_idle (MODEST_MSG_EDIT_WINDOW (obj));

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

	modest_text_utils_hyperlinkify (string_buffer);

	gtk_text_buffer_set_modified (priv->text_buffer, TRUE);

	return g_string_free (string_buffer, FALSE);
									
}

MsgData * 
modest_msg_edit_window_get_msg_data (ModestMsgEditWindow *edit_window)
{
	MsgData *data;
	const gchar *account_name;
	ModestMsgEditWindowPrivate *priv;
	TnyIterator *att_iter;
	const gchar *picker_active_id;
	
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window), NULL);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (edit_window);
	
	picker_active_id = modest_selector_picker_get_active_id (MODEST_SELECTOR_PICKER (priv->from_field));
	g_return_val_if_fail (picker_active_id, NULL);
	account_name = modest_utils_get_account_name_from_recipient (picker_active_id, NULL);
	
	/* don't free these (except from) */
	data = g_slice_new0 (MsgData);
	data->from    =  g_strdup ((gchar *) modest_selector_picker_get_active_display_name (MODEST_SELECTOR_PICKER (priv->from_field)));
	data->account_name = g_strdup (account_name);
	data->to      =  g_strdup (modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR (priv->to_field)));
	data->cc      =  g_strdup (modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR (priv->cc_field)));
	data->bcc     =  g_strdup (modest_recpt_editor_get_recipients (MODEST_RECPT_EDITOR (priv->bcc_field)));
	data->subject =  g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->subject_field)));
	data->references = g_strdup (priv->references);
	data->in_reply_to = g_strdup (priv->in_reply_to);
	if (priv->draft_msg) {
		data->draft_msg = g_object_ref (priv->draft_msg);
	} else if (priv->outbox_msg) {
		data->draft_msg = g_object_ref (priv->outbox_msg);
	} else {
		data->draft_msg = NULL;
	}

	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));
	GtkTextIter b, e;
	gtk_text_buffer_get_bounds (buf, &b, &e);
	data->plain_body = modest_text_utils_text_buffer_get_text (priv->text_buffer); /* returns a copy */

	if (wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer)))
		data->html_body = get_formatted_data (edit_window); /* returns a copy. */
	else
		data->html_body = NULL;

	/* deep-copy the data */
	att_iter = tny_list_create_iterator (priv->attachments);
	data->attachments = NULL;
	while (!tny_iterator_is_done (att_iter)) {
		TnyMimePart *part = (TnyMimePart *) tny_iterator_get_current (att_iter);
		if (!(TNY_IS_MIME_PART(part))) {
			g_warning ("strange data in attachment list");
			g_object_unref (part);
			tny_iterator_next (att_iter);
			continue;
		}
		data->attachments = g_list_append (data->attachments,
						   part);
		tny_iterator_next (att_iter);
	}
	g_object_unref (att_iter);

	GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table (GTK_TEXT_BUFFER (priv->text_buffer));
	att_iter = tny_list_create_iterator (priv->images);
	data->images = NULL;
	while (!tny_iterator_is_done (att_iter)) {
		TnyMimePart *part = (TnyMimePart *) tny_iterator_get_current (att_iter);
		const gchar *cid;
		if (!(TNY_IS_MIME_PART(part))) {
			g_warning ("strange data in attachment list");
			g_object_unref (part);
			tny_iterator_next (att_iter);
			continue;
		}
		cid = tny_mime_part_get_content_id (part);
		if (cid) {			
			gchar *image_tag_id;
			GtkTextTag *image_tag;
			GtkTextIter iter;
			image_tag_id = g_strdup_printf ("image-tag-%s", cid);
			image_tag = gtk_text_tag_table_lookup (tag_table, image_tag_id);
			g_free (image_tag_id);
			
			gtk_text_buffer_get_start_iter (priv->text_buffer, &iter);
			if (image_tag && 
			    ((gtk_text_iter_has_tag (&iter, image_tag))||
			     (gtk_text_iter_forward_to_tag_toggle (&iter, image_tag))))
				data->images = g_list_append (data->images,
							      g_object_ref (part));
		}
		g_object_unref (part);
		tny_iterator_next (att_iter);
	}
	g_object_unref (att_iter);
	
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
	g_free (data->from);
	g_free (data->subject);
	g_free (data->plain_body);
	g_free (data->html_body);
	g_free (data->account_name);
	g_free (data->references);
	g_free (data->in_reply_to);
	
	if (data->draft_msg != NULL) {
		g_object_unref (data->draft_msg);
		data->draft_msg = NULL;
	}
	
	g_list_foreach (data->attachments, (GFunc)unref_gobject,  NULL);
	g_list_free (data->attachments);
	g_list_foreach (data->images, (GFunc)unref_gobject,  NULL);
	g_list_free (data->images);
	
	g_slice_free (MsgData, data);
}

void                    
modest_msg_edit_window_get_parts_size (ModestMsgEditWindow *window,
				       gint *parts_count,
				       guint64 *parts_size)
{
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	modest_attachments_view_get_sizes (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), parts_count, parts_size);

	/* TODO: add images */
	*parts_size += priv->images_size;
	*parts_count += priv->images_count;

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
	ModestWindowPrivate *parent_priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (self);

	switch (format) {
	case MODEST_MSG_EDIT_FORMAT_HTML:
		wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
		if (parent_priv->toolbar) gtk_widget_show (parent_priv->toolbar);
		break;
	case MODEST_MSG_EDIT_FORMAT_TEXT:
		wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), FALSE);
		if (parent_priv->toolbar) gtk_widget_hide (parent_priv->toolbar);
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
	WPTextBufferFormat *buffer_format;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self), NULL);

	buffer_format = g_new0 (WPTextBufferFormat, 1);
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
	WPTextBufferFormat *buffer_format;
	WPTextBufferFormat *current_format;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (format_state != NULL);

	buffer_format = g_new0 (WPTextBufferFormat, 1);
	current_format = g_new0 (WPTextBufferFormat, 1);

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
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_BOLD,
					      GINT_TO_POINTER (buffer_format->bold&0x1));
	}
	if (buffer_format->cs.italic) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_ITALIC,
					      GINT_TO_POINTER (buffer_format->italic&0x1));
	}
	if (buffer_format->cs.color) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FORECOLOR,
					      GINT_TO_POINTER (&(buffer_format->color)));
	}
	if (buffer_format->cs.font_size) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT_SIZE,
					      GINT_TO_POINTER (buffer_format->font_size));
	}
	if (buffer_format->cs.justification) {
		switch (buffer_format->justification) {
		case GTK_JUSTIFY_LEFT:
			wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_LEFT,
						      GINT_TO_POINTER(TRUE));
			break;
		case GTK_JUSTIFY_CENTER:
			wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_CENTER,
						      GINT_TO_POINTER(TRUE));
			break;
		case GTK_JUSTIFY_RIGHT:
			wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_RIGHT,
						      GINT_TO_POINTER(TRUE));
			break;
		default:
			break;
		}
			
	}
	if (buffer_format->cs.font) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT,
					      GINT_TO_POINTER (buffer_format->font));
	}
	wp_text_buffer_thaw (WP_TEXT_BUFFER (priv->text_buffer));
	if (buffer_format->cs.bullet) {
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_BULLET,
					      GINT_TO_POINTER ((buffer_format->bullet)?1:0));
	}
/* 	wp_text_buffer_set_format (WP_TEXT_BUFFER (priv->text_buffer), buffer_format); */
	
	text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), self);
	
	g_free (buffer_format);
	g_free (current_format);

	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (self));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
}

static void
text_buffer_refresh_attributes (WPTextBuffer *buffer, ModestMsgEditWindow *window)
{
	WPTextBufferFormat *buffer_format = g_new0 (WPTextBufferFormat, 1);
	GtkAction *action;
	ModestWindowPrivate *parent_priv;
	ModestMsgEditWindowPrivate *priv;
	
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer))) {
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/FileFormatFormattedTextMenu");
		if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
			modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), TRUE);
	} else {
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/FileFormatPlainTextMenu");
		if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
			modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), TRUE);
	}

	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, FALSE);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bold);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->italic);

/* 	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/BulletedListMenu"); */
/* 	modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bullet); */

	action = NULL;
	switch (buffer_format->justification)
	{
	case GTK_JUSTIFY_LEFT:
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentLeftMenu");
		break;
	case GTK_JUSTIFY_CENTER:
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentCenterMenu");
		break;
	case GTK_JUSTIFY_RIGHT:
		action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/AlignmentMenu/AlignmentRightMenu");
		break;
	default:
		break;
	}
	
	if (action != NULL)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
	
	g_signal_handlers_block_by_func (G_OBJECT (priv->font_color_button), 
					 G_CALLBACK (modest_msg_edit_window_color_button_change),
					 window);
	hildon_color_button_set_color (HILDON_COLOR_BUTTON (priv->font_color_button), & (buffer_format->color));
	g_signal_handlers_unblock_by_func (G_OBJECT (priv->font_color_button), 
					   G_CALLBACK (modest_msg_edit_window_color_button_change),
					   window);

	if (priv->current_size_index != buffer_format->font_size) {
		GtkTreeIter iter;
		GtkTreePath *path;

		path = gtk_tree_path_new_from_indices (buffer_format->font_size, -1);
		if (gtk_tree_model_get_iter (priv->sizes_model, &iter, path)) {
			gchar *size_text;
			gchar *markup;

			priv->current_size_index = buffer_format->font_size;

			gtk_tree_model_get (priv->sizes_model, &iter, 0, &size_text, -1);
			markup = g_strconcat ("<span font_family='Sans'>", 
					      size_text, "</span>", NULL);
			
			gtk_label_set_markup (GTK_LABEL (priv->size_tool_button_label), markup);
			g_free (markup);
			g_free (size_text);
		}
		gtk_tree_path_free (path);		
	}

	if (priv->current_face_index != buffer_format->font) {
		GtkTreeIter iter;
		GtkTreePath *path;

		path = gtk_tree_path_new_from_indices (buffer_format->font, -1);
		if (gtk_tree_model_get_iter (priv->faces_model, &iter, path)) {
			gchar *face_name;
			gchar *markup;

			priv->current_face_index = buffer_format->font;
			gtk_tree_model_get (priv->faces_model, &iter, 0, &face_name, -1);
			markup = g_strconcat ("<span font_family='", face_name, "'>Tt</span>", NULL);
			gtk_label_set_markup (GTK_LABEL (priv->font_tool_button_label), markup);
			g_free (face_name);
			g_free (markup);
		}

	}

	g_free (buffer_format);

}

#ifdef MODEST_HILDON_VERSION_0
void
modest_msg_edit_window_select_color (ModestMsgEditWindow *window)
{
	
	WPTextBufferFormat *buffer_format = g_new0 (WPTextBufferFormat, 1);
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;
	gint response;
	GdkColor *new_color = NULL;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, FALSE);
	
	dialog = hildon_color_selector_new (GTK_WINDOW (window));
	hildon_color_selector_set_color (HILDON_COLOR_SELECTOR (dialog), &(buffer_format->color));
	g_free (buffer_format);

	if (gtk_dialog_run (GTK_DIALOG (dialog) == GTK_RESPONSE_OK)) {
		new_color = hildon_color_selector_get_color (HILDON_COLOR_SELECTOR (dialog));
		if (new_color != NULL) {
			wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FORECOLOR,
						      (gpointer) new_color);
		}
	}
	gtk_widget_destroy (dialog);
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
	
	dialog = hildon_color_selector_new (GTK_WINDOW (window));
	hildon_color_selector_set_color (HILDON_COLOR_SELECTOR (dialog),(GdkColor*)old_color);

	if (gtk_dialog_run (GTK_DIALOG (dialog) == GTK_RESPONSE_OK)) {
		new_color = hildon_color_selector_get_color (HILDON_COLOR_SELECTOR (dialog));
		if (new_color != NULL)
			wp_text_buffer_set_background_color (WP_TEXT_BUFFER (priv->text_buffer), new_color);
	}
	gtk_widget_destroy (dialog);

}

#else 
void
modest_msg_edit_window_select_color (ModestMsgEditWindow *window)
{
	
	WPTextBufferFormat *buffer_format = g_new0 (WPTextBufferFormat, 1);
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, FALSE);
		
	dialog = hildon_color_chooser_new ();
	hildon_color_chooser_set_color (HILDON_COLOR_CHOOSER (dialog), &(buffer_format->color));
	g_free (buffer_format);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		GdkColor col;
		hildon_color_chooser_get_color (HILDON_COLOR_CHOOSER(dialog), &col);
		wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FORECOLOR,
					      (gpointer) &col);
	}
	gtk_widget_destroy (dialog);
}


void
modest_msg_edit_window_select_background_color (ModestMsgEditWindow *window)
{
	
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *dialog = NULL;
	GdkColor *old_color = NULL;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	old_color = (GdkColor*)wp_text_buffer_get_background_color (WP_TEXT_BUFFER (priv->text_buffer));
	
	dialog = hildon_color_chooser_new ();
	hildon_color_chooser_set_color (HILDON_COLOR_CHOOSER (dialog),(GdkColor*)old_color);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) { 
		GdkColor col;
		hildon_color_chooser_get_color (HILDON_COLOR_CHOOSER(dialog), &col);
		wp_text_buffer_set_background_color (WP_TEXT_BUFFER (priv->text_buffer), &col);
	}
	gtk_widget_destroy (dialog);
}

#endif /*!MODEST_HILDON_VERSION_0*/



static TnyStream*
create_stream_for_uri (const gchar* uri)
{
	if (!uri)
		return NULL;
		
	TnyStream *result = NULL;

	GnomeVFSHandle *handle = NULL;
	GnomeVFSResult test = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	if (test == GNOME_VFS_OK) {
		TnyStream *vfssstream = TNY_STREAM (tny_vfs_stream_new (handle));
		/* Streams over OBEX (Bluetooth) are not seekable but
		 * we expect them to be (we might need to read them
		 * several times). So if this is a Bluetooth URI just
		 * read the whole file into memory (this is not a fast
		 * protocol so we can assume that these files are not
		 * going to be very big) */
		if ((g_ascii_strncasecmp (uri, "obex://", 7) == 0)||
		    (g_ascii_strncasecmp (uri, "upnpav://", 9) == 0)) {
			TnyStream *memstream = tny_camel_mem_stream_new ();
			tny_stream_write_to_stream (vfssstream, memstream);
			g_object_unref (vfssstream);
			result = memstream;
		} else {
			result = vfssstream;
		}
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

	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), GTK_WINDOW (window));

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK:
	{
		gchar *current_folder;
		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
		current_folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dialog));
		if (current_folder && current_folder != '\0') {
			GError *err = NULL;
			modest_conf_set_string (modest_runtime_get_conf (), MODEST_CONF_LATEST_INSERT_IMAGE_PATH, 
						current_folder, &err);
			if (err != NULL) {
				g_debug ("Error storing latest used folder: %s", err->message);
				g_error_free (err);
			}
		}
		g_free (current_folder);
	}
	break;
	default:
		break;
	}
	gtk_widget_destroy (dialog);

	g_object_ref (window);
	/* The operation could take some time so allow the dialog to be closed */
	while (gtk_events_pending ())
		gtk_main_iteration ();

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
			GnomeVFSFileInfo *info;
			gchar *filename, *basename, *escaped_filename;
			TnyMimePart *mime_part;
			gchar *content_id;
			const gchar *mime_type = NULL;
			GnomeVFSURI *vfs_uri;
			guint64 stream_size;

			gnome_vfs_close (handle);
			vfs_uri = gnome_vfs_uri_new (uri);

			escaped_filename = g_path_get_basename (gnome_vfs_uri_get_path (vfs_uri));
			filename = gnome_vfs_unescape_string_for_display (escaped_filename);
			g_free (escaped_filename);
			gnome_vfs_uri_unref (vfs_uri);
			info = gnome_vfs_file_info_new ();

			if (gnome_vfs_get_file_info (uri, info, GNOME_VFS_FILE_INFO_GET_MIME_TYPE
						     | GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE) 
			    == GNOME_VFS_OK)
				mime_type = gnome_vfs_file_info_get_mime_type (info);

			mime_part = tny_platform_factory_new_mime_part
				(modest_runtime_get_platform_factory ());

			TnyStream *stream = create_stream_for_uri (uri);

			if (stream == NULL) {

				modest_platform_information_banner (NULL, NULL, 
								    _FM("sfil_ib_opening_not_allowed"));
				g_free (filename);
				g_object_unref (mime_part);
				gnome_vfs_file_info_unref (info);
				continue;
			}

			tny_mime_part_construct (mime_part, stream, mime_type, "base64");

			content_id = g_strdup_printf ("%d", priv->next_cid);
			tny_mime_part_set_content_id (mime_part, content_id);
			g_free (content_id);
			priv->next_cid++;

			basename = g_path_get_basename (filename);
			tny_mime_part_set_filename (mime_part, basename);
			g_free (basename);

			pixbuf = pixbuf_from_stream (stream, mime_type, &stream_size, window);

			if (pixbuf != NULL) {
				priv->images_size += stream_size;
				priv->images_count ++;
				insert_mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (priv->text_buffer));
				gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (priv->text_buffer), &position, insert_mark);
				wp_text_buffer_insert_image (WP_TEXT_BUFFER (priv->text_buffer), &position, 
							     tny_mime_part_get_content_id (mime_part), pixbuf);
				g_object_unref (pixbuf);

				tny_list_prepend (priv->images, (GObject *) mime_part);
				gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
			} else {
				modest_platform_information_banner (NULL, NULL,
								    _("mail_ib_file_operation_failed"));
			}

			g_free (filename);
			g_object_unref (mime_part);
			gnome_vfs_file_info_unref (info);

		}
	}
	g_object_unref (window);
}

static void
on_attach_file_response (GtkDialog *dialog,
			 gint       arg1,
			 gpointer   user_data)
{
	GSList *uris = NULL;
	GSList *uri_node;
	GnomeVFSFileSize total_size, allowed_size;
	ModestMsgEditWindow *window;
	ModestMsgEditWindowPrivate *priv;
	gint att_num;
	guint64 att_size;

	switch (arg1) {
	case GTK_RESPONSE_OK:
	{
		gchar *current_folder;

		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dialog));
		current_folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dialog));
		if (current_folder && current_folder != '\0') {
			GError *err = NULL;
			modest_conf_set_string (modest_runtime_get_conf (), MODEST_CONF_LATEST_ATTACH_FILE_PATH, 
						current_folder, &err);
			if (err != NULL) {
				g_debug ("Error storing latest used folder: %s", err->message);
				g_error_free (err);
			}
		}
		g_free (current_folder);
	}
	break;
	default:
		break;
	}

	window = MODEST_MSG_EDIT_WINDOW (user_data);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	/* allowed size is the maximum size - what's already there */
	modest_attachments_view_get_sizes (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
					   &att_num, &att_size);
	allowed_size = MODEST_MAX_ATTACHMENT_SIZE - att_size;

	total_size = 0;
	for (uri_node = uris; uri_node != NULL; uri_node = g_slist_next (uri_node)) {

		const gchar *uri = (const gchar *) uri_node->data;

		total_size += 
			modest_msg_edit_window_attach_file_one (window, uri, allowed_size);

		if (total_size > allowed_size) {
			g_debug ("%s: total size: %u", 
				   __FUNCTION__, (unsigned int)total_size);
			break;
		}
		allowed_size -= total_size;
	}
	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
modest_msg_edit_window_offer_attach_file (ModestMsgEditWindow *window)
{
	GtkWidget *dialog = NULL;
	ModestMsgEditWindowPrivate *priv;
	gchar *conf_folder;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW(window));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (modest_platform_check_memory_low (MODEST_WINDOW(window), TRUE))
		return;

	dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (window), 
						 GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_window_set_title (GTK_WINDOW (dialog), _("mcen_ti_select_attachment_title"));
	conf_folder = modest_conf_get_string (modest_runtime_get_conf (), MODEST_CONF_LATEST_ATTACH_FILE_PATH, NULL);
	if (conf_folder && conf_folder[0] != '\0') {
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog), conf_folder);
	} else {
		gchar *docs_folder;
		/* Set the default folder to images folder */
		docs_folder = g_build_filename (g_getenv (MODEST_MAEMO_UTILS_MYDOCS_ENV),
						".documents", NULL);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), docs_folder);
		g_free (docs_folder);
	}
	g_free (conf_folder);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), GTK_WINDOW (window));

	/* Connect to response & show */
	g_signal_connect (dialog, "response", 
			  G_CALLBACK (on_attach_file_response), window);
	gtk_widget_show (dialog);
}


GnomeVFSFileSize
modest_msg_edit_window_attach_file_one (ModestMsgEditWindow *window,
					const gchar *uri, 
					GnomeVFSFileSize allowed_size)

{
	GnomeVFSHandle *handle = NULL;
	ModestMsgEditWindowPrivate *priv;
	GnomeVFSResult result;
	GnomeVFSFileSize size = 0;
	g_return_val_if_fail (window, 0);
	g_return_val_if_fail (uri, 0);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	if (result == GNOME_VFS_OK) {
		TnyMimePart *mime_part;
		TnyStream *stream;
		const gchar *mime_type = NULL;
		gchar *basename;
		gchar *escaped_filename;
		gchar *filename;
		gchar *content_id;
		GnomeVFSFileInfo *info;
		GnomeVFSURI *vfs_uri;

		gnome_vfs_close (handle);
		vfs_uri = gnome_vfs_uri_new (uri);
		

		escaped_filename = g_path_get_basename (gnome_vfs_uri_get_path (vfs_uri));
		filename = gnome_vfs_unescape_string_for_display (escaped_filename);
		g_free (escaped_filename);
		gnome_vfs_uri_unref (vfs_uri);

		info = gnome_vfs_file_info_new ();
		
		if (gnome_vfs_get_file_info (uri, 
					     info, 
					     GNOME_VFS_FILE_INFO_GET_MIME_TYPE)
		    == GNOME_VFS_OK)
			mime_type = gnome_vfs_file_info_get_mime_type (info);
		mime_part = tny_platform_factory_new_mime_part
			(modest_runtime_get_platform_factory ());
		
		/* try to get the attachment's size; this may fail for weird
		 * file systems, like obex, upnp... */
		if (allowed_size != 0 &&
		    info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SIZE) {
			size = info->size;
			if (size > allowed_size) {
				modest_platform_information_banner (NULL, NULL, 
								    _FM("sfil_ib_opening_not_allowed"));
				g_free (filename);
				return 0;
			}
		} else
			g_debug ("%s: could not get attachment size", __FUNCTION__);
		
		stream = create_stream_for_uri (uri);
		
		if (stream == NULL) {

			modest_platform_information_banner (NULL, NULL, _FM("sfil_ib_opening_not_allowed"));

			g_object_unref (mime_part);
			g_free (filename);
			gnome_vfs_file_info_unref (info);
			return 0;
		}

		tny_mime_part_construct (mime_part, stream, mime_type, "base64");
		g_object_unref (stream);
		
		content_id = g_strdup_printf ("%d", priv->next_cid);
		tny_mime_part_set_content_id (mime_part, content_id);
		g_free (content_id);
		priv->next_cid++;
		
		basename = g_path_get_basename (filename);
		tny_mime_part_set_filename (mime_part, basename);
		g_free (basename);
		
		tny_list_prepend (priv->attachments, (GObject *) mime_part);
		modest_attachments_view_add_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
							mime_part,
							info->size == 0, info->size);
		gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
		gtk_widget_show_all (priv->attachments_caption);
		gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
		g_free (filename);
		g_object_unref (mime_part);
		gnome_vfs_file_info_unref (info);
	}

	return size;
}

void
modest_msg_edit_window_remove_attachments (ModestMsgEditWindow *window,
					   TnyList *att_list)
{
	ModestMsgEditWindowPrivate *priv;
	TnyIterator *iter;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (att_list == NULL) {
		att_list = modest_attachments_view_get_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
		if (!modest_maemo_utils_select_attachments (GTK_WINDOW (window), att_list, TRUE)) {
			g_object_unref (att_list);
			return;
		}
	} else {
		g_object_ref (att_list);
	}

	if (tny_list_get_length (att_list) == 0) {
		hildon_banner_show_information (NULL, NULL, _("TODO: no attachments selected to remove"));
	} else {
		gboolean dialog_response;
		gchar *message = NULL;
		gchar *filename = NULL;

		if (tny_list_get_length (att_list) == 1) {
			TnyMimePart *part;
			iter = tny_list_create_iterator (att_list);
			part = (TnyMimePart *) tny_iterator_get_current (iter);
			g_object_unref (iter);
			if (TNY_IS_MSG (part)) {
				TnyHeader *header = tny_msg_get_header (TNY_MSG (part));
				if (header) {
					filename = tny_header_dup_subject (header);
					g_object_unref (header);
				}
				if (filename == NULL) {
					filename = g_strdup (_("mail_va_no_subject"));
				}
			} else {
				filename = g_strdup (tny_mime_part_get_filename (TNY_MIME_PART (part)));
			}
			g_object_unref (part);
		} else {
			filename = g_strdup ("");
		}
		message = g_strdup_printf (ngettext("emev_nc_delete_attachment", 
						    "emev_nc_delete_attachments",
						    tny_list_get_length (att_list)), filename);
		g_free (filename);

		dialog_response = modest_platform_run_confirmation_dialog (GTK_WINDOW (window), 
									   message);
		g_free (message);

		if (dialog_response != GTK_RESPONSE_OK) {
			g_object_unref (att_list);
			return;
		}

		for (iter = tny_list_create_iterator (att_list);
		     !tny_iterator_is_done (iter);
		     tny_iterator_next (iter)) {
			TnyMimePart *mime_part = (TnyMimePart *) tny_iterator_get_current (iter);
			const gchar *att_id;
			tny_list_remove (priv->attachments, (GObject *) mime_part);

			modest_attachments_view_remove_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
								   mime_part);
			if (tny_list_get_length (priv->attachments) == 0)
				gtk_widget_hide (priv->attachments_caption);
			att_id = tny_mime_part_get_content_id (mime_part);
			if (att_id != NULL)
				text_buffer_delete_images_by_id (gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body)),
								 att_id);
			gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
			g_object_unref (mime_part);
		}
		g_object_unref (iter);
	}

	g_object_unref (att_list);

	/* if the last attachment has been removed, focus the Subject: field */
	if (!modest_attachments_view_has_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view)))
		gtk_widget_grab_focus (priv->subject_field);
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
font_size_clicked (GtkToolButton *button,
		   ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *selector, *dialog;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	selector = hildon_touch_selector_new ();
	hildon_touch_selector_append_text_column (HILDON_TOUCH_SELECTOR (selector), priv->sizes_model, TRUE);
	hildon_touch_selector_set_active (HILDON_TOUCH_SELECTOR (selector), 0, priv->current_size_index);

	dialog = hildon_picker_dialog_new (GTK_WINDOW (window));
	hildon_picker_dialog_set_selector (HILDON_PICKER_DIALOG (dialog), HILDON_TOUCH_SELECTOR (selector));
	gtk_window_set_title (GTK_WINDOW (dialog), _("mcen_ti_font_size"));

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		gint new_index;
		gchar *size_text;
		gchar *markup;
		WPTextBufferFormat format;

		new_index = hildon_touch_selector_get_active (HILDON_TOUCH_SELECTOR (selector), 0);

		memset (&format, 0, sizeof (format));
		wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), &format, FALSE);

		format.cs.font_size = TRUE;
		format.cs.text_position = TRUE;
		format.cs.font = TRUE;
		format.font_size = new_index;
/* 		wp_text_buffer_set_format (WP_TEXT_BUFFER (priv->text_buffer), &format); */

		if (!wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT_SIZE,
						   GINT_TO_POINTER (new_index)))
			wp_text_view_reset_and_show_im (WP_TEXT_VIEW (priv->msg_body));
		
		text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (window));
		size_text = hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector));
		markup = g_strconcat ("<span font_family='", DEFAULT_SIZE_BUTTON_FONT_FAMILY, "'>", 
				      size_text, "</span>", NULL);
		g_free (size_text);
		gtk_label_set_markup (GTK_LABEL (priv->size_tool_button_label), markup);
		g_free (markup);

	}
	gtk_widget_destroy (dialog);

	gtk_widget_grab_focus (GTK_WIDGET (priv->msg_body));

}

static void
font_face_clicked (GtkToolButton *button,
		   ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;
	GtkWidget *selector, *dialog;
	GtkCellRenderer *renderer;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	selector = hildon_touch_selector_new ();
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "alignment", PANGO_ALIGN_CENTER, "xalign", 0.5, NULL);
	hildon_touch_selector_append_column (HILDON_TOUCH_SELECTOR (selector), priv->faces_model, 
					     renderer, "family", 0, "text", 0, NULL);
	hildon_touch_selector_set_active (HILDON_TOUCH_SELECTOR (selector), 0, priv->current_face_index);

	dialog = hildon_picker_dialog_new (GTK_WINDOW (window));
	hildon_picker_dialog_set_selector (HILDON_PICKER_DIALOG (dialog), HILDON_TOUCH_SELECTOR (selector));
	gtk_window_set_title (GTK_WINDOW (dialog), _("mcen_ti_font_face"));

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		gint new_font_index;
		GtkTreePath *path;
		GtkTreeIter iter;

		new_font_index = hildon_touch_selector_get_active (HILDON_TOUCH_SELECTOR (selector), 0);
		path = gtk_tree_path_new_from_indices (new_font_index, -1);
		if (gtk_tree_model_get_iter (priv->faces_model, &iter, path)) {
			gchar *face_name;
			gchar *markup;

			gtk_tree_model_get (priv->faces_model, &iter, 0, &face_name, -1);

			if (!wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT, 
							   GINT_TO_POINTER(new_font_index)))
				wp_text_view_reset_and_show_im (WP_TEXT_VIEW (priv->msg_body));

			markup = g_strconcat ("<span font_family='", face_name, "'>Tt</span>", NULL);
			gtk_label_set_markup (GTK_LABEL (priv->font_tool_button_label), markup);

			text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (window));
			g_free (face_name);
			g_free (markup);
		}
		gtk_tree_path_free (path);

	}
	gtk_widget_destroy (dialog);

	gtk_widget_grab_focus (GTK_WIDGET (priv->msg_body));

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
	
	/* we check for low-mem; in that case, show a warning, and don't allow
	 * for the addressbook
	 */
	if (modest_platform_check_memory_low (MODEST_WINDOW(window), TRUE))
		return;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (editor == NULL) {
		GtkWidget *view_focus, *parent;
		view_focus = gtk_window_get_focus (GTK_WINDOW (window));

		/* This code should be kept in sync with ModestRecptEditor. The
		   textview inside the recpt editor is the one that really gets the
		   focus. As it's inside a scrolled window, and this one inside the
		   hbox recpt editor inherits from, we'll need to go up in the 
		   hierarchy to know if the text view is part of the recpt editor
		   or if it's a different text entry */
		parent = gtk_widget_get_parent (view_focus);
		if (parent && MODEST_IS_RECPT_EDITOR (parent))
			editor = MODEST_RECPT_EDITOR (parent);

		if (editor == NULL)
			editor = MODEST_RECPT_EDITOR (priv->to_field);
	}

	modest_address_book_select_addresses (editor, GTK_WINDOW (window));
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
	ModestMsgEditWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	/* We can not just use the code of
	   modest_msg_edit_window_setup_toolbar because it has a
	   mixture of both initialization and creation code. */
	if (show_toolbar) {
		gint current_format;
		current_format = wp_text_buffer_is_rich_text (WP_TEXT_BUFFER (priv->text_buffer))
			? MODEST_FILE_FORMAT_FORMATTED_TEXT : MODEST_FILE_FORMAT_PLAIN_TEXT;
		if (current_format == MODEST_FILE_FORMAT_PLAIN_TEXT) {
			gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
		} else {
			gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
		}
	} else {
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
	}
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

	if (priv->priority_flags != priority_flags) {
		GtkAction *priority_action = NULL;

		priv->priority_flags = priority_flags;

		switch (priority_flags) {
		case TNY_HEADER_FLAG_HIGH_PRIORITY:
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon),
						      MODEST_HEADER_ICON_HIGH, 
						      HILDON_ICON_SIZE_SMALL);
			gtk_widget_show (priv->priority_icon);
			priority_action = gtk_ui_manager_get_action (parent_priv->ui_manager,
								     "/MenuBar/ToolsMenu/MessagePriorityMenu/MessagePriorityHighMenu");
			break;
		case TNY_HEADER_FLAG_LOW_PRIORITY:
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->priority_icon),
						      MODEST_HEADER_ICON_LOW,
						      HILDON_ICON_SIZE_SMALL);
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
	gtk_widget_queue_resize (priv->subject_box);
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

	gtk_widget_set_no_show_all (GTK_WIDGET (parent_priv->toolbar), TRUE);

	if (parent_priv->toolbar) {
		if (file_format == MODEST_FILE_FORMAT_PLAIN_TEXT) {
			gtk_widget_hide (parent_priv->toolbar);
		} else {
			gtk_widget_show (parent_priv->toolbar);
		}
	}

	if (current_format != file_format) {
		switch (file_format) {
		case MODEST_FILE_FORMAT_FORMATTED_TEXT:
			wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
			remove_tags (WP_TEXT_BUFFER (priv->text_buffer));
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
				if (parent_priv->toolbar) gtk_widget_hide (parent_priv->toolbar);
			} else {
				GtkToggleAction *action = GTK_TOGGLE_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/FormatMenu/FileFormatFormattedTextMenu"));
				modest_utils_toggle_action_set_active_block_notify (action, TRUE);
			}
		}
			break;
		}
		modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
		modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
		text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), window);
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
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr(),
				     GTK_WINDOW(dialog), GTK_WINDOW (window));

	/* First we get the currently selected font information */
	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), &oldfmt, TRUE);

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

	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), GTK_WINDOW (window));
	gtk_widget_show_all (dialog);
	priv->font_dialog = dialog;
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	priv->font_dialog = NULL;
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
			fmt.font_size = oldfmt.font_size;
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
			fmt.cs.font_size = TRUE;
			fmt.font_size = wp_get_font_size_index(font_size, DEFAULT_FONT_SIZE);
		}
		wp_text_buffer_set_format(WP_TEXT_BUFFER(priv->text_buffer), &fmt);
		text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), window);
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

	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
}

void
modest_msg_edit_window_redo (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	
	wp_text_buffer_redo (WP_TEXT_BUFFER (priv->text_buffer));

	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));

}

static void  
text_buffer_can_undo (GtkTextBuffer *buffer, gboolean can_undo, ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	priv->can_undo = can_undo;
}

static void  
text_buffer_can_redo (GtkTextBuffer *buffer, gboolean can_redo, ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	priv->can_redo = can_redo;
}

gboolean            
modest_msg_edit_window_can_undo (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), FALSE);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	return priv->can_undo;
}

gboolean            
modest_msg_edit_window_can_redo (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), FALSE);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	return priv->can_redo;
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

static gboolean
msg_body_focus (GtkWidget *focus,
		GdkEventFocus *event,
		gpointer userdata)
{
	
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (userdata));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (userdata));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (userdata), MODEST_DIMMING_RULES_CLIPBOARD);
	return FALSE;
}

static void
recpt_field_changed (GtkTextBuffer *buffer,
		  ModestMsgEditWindow *editor)
{
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (editor));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (editor));
}

static void
body_changed (GtkTextBuffer *buffer, ModestMsgEditWindow *editor)
{
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (editor));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (editor));
}

void
modest_msg_edit_window_set_modified (ModestMsgEditWindow *editor,
				     gboolean modified)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (editor);
	GtkTextBuffer *buffer;

	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->to_field));
	gtk_text_buffer_set_modified (buffer, modified);
	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->cc_field));
	gtk_text_buffer_set_modified (buffer, modified);
	buffer = modest_recpt_editor_get_buffer (MODEST_RECPT_EDITOR(priv->bcc_field));
	gtk_text_buffer_set_modified (buffer, modified);
	gtk_text_buffer_set_modified (priv->text_buffer, modified);
}

gboolean
modest_msg_edit_window_is_modified (ModestMsgEditWindow *editor)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (editor);
	const char *account_name;
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

	account_name = modest_selector_picker_get_active_id (MODEST_SELECTOR_PICKER (priv->from_field));
	if (priv->original_mailbox) {
		if (!account_name || strcmp (account_name, priv->original_mailbox))
			return TRUE;
	} else if (!priv->original_account_name || strcmp(account_name, priv->original_account_name)) {
		return TRUE;
	}

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

	g_object_ref (window);
	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->to_field),  add_to_addressbook)) {
		modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->to_field));
		g_object_unref (window);
		return FALSE;
	}
	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->cc_field),  add_to_addressbook)) {
		modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->cc_field));
		g_object_unref (window);
		return FALSE;
	}
	if (!modest_address_book_check_names (MODEST_RECPT_EDITOR (priv->bcc_field), add_to_addressbook)) {
		modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->bcc_field));
		g_object_unref (window);
		return FALSE;
	}

	if (!modest_recpt_editor_has_focus (MODEST_RECPT_EDITOR (priv->cc_field)) &&
	    !modest_recpt_editor_has_focus (MODEST_RECPT_EDITOR (priv->bcc_field)))
		modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (priv->to_field));
	g_object_unref (window);

	return TRUE;

}

static void
modest_msg_edit_window_add_attachment_clicked (GtkButton *button,
					       ModestMsgEditWindow *window)
{
	modest_msg_edit_window_offer_attach_file (window);
}

const gchar *
modest_msg_edit_window_get_clipboard_text (ModestMsgEditWindow *win)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (win);

	return priv->clipboard_text;
}

static void
modest_msg_edit_window_clipboard_owner_change (GtkClipboard *clipboard,
					       GdkEvent *event,
					       ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	GtkClipboard *selection_clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	gchar *text = NULL;

	/* It could happen that the window was already closed */
	if (!GTK_WIDGET_VISIBLE (window))
		return;

	g_object_ref (window);
	text = gtk_clipboard_wait_for_text (selection_clipboard);

	if (priv->clipboard_text != NULL) {
		g_free (priv->clipboard_text);
	}
	priv->clipboard_text = text;

	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);

	g_object_unref (window);
}

static gboolean clipboard_owner_change_idle (gpointer userdata)
{
	ModestMsgEditWindow *window = (ModestMsgEditWindow *) userdata;
	ModestMsgEditWindowPrivate *priv;

	gdk_threads_enter ();
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), FALSE);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	priv->clipboard_owner_idle = 0;
	modest_msg_edit_window_clipboard_owner_change (NULL, NULL, window);
	gdk_threads_leave ();

	return FALSE;
}

static void
modest_msg_edit_window_clipboard_owner_handle_change_in_idle (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	if (priv->clipboard_owner_idle == 0) {
		priv->clipboard_owner_idle = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
							      clipboard_owner_change_idle, 
							      g_object_ref (window),
							      g_object_unref);
	}
}

static void 
subject_field_move_cursor (GtkEntry *entry,
			   GtkMovementStep step,
			   gint a1,
			   gboolean a2,
			   gpointer window)
{
	/* It could happen that the window was already closed */
	if (!GTK_WIDGET_VISIBLE (window))
		return;

	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);
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
body_insert_text (GtkTextBuffer *buffer, 
		  GtkTextIter *location,
		  gchar *text,
		  gint len,
		  ModestMsgEditWindow *window)
{
	GtkTextIter end_iter;
	gint offset;
	glong utf8_len;

	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (buffer), &end_iter);

	offset = gtk_text_iter_get_offset (&end_iter);
	utf8_len = g_utf8_strlen (text, len);

	if (offset + utf8_len > MAX_BODY_LENGTH) {
		g_signal_stop_emission_by_name (G_OBJECT (buffer), "insert-text");
		if (offset < MAX_BODY_LENGTH)
		{
			gchar *result;
			gchar *utf8_end;

			utf8_end = g_utf8_offset_to_pointer (text, MAX_BODY_LENGTH - offset);

			/* Prevent endless recursion */
			result = g_strndup (text, utf8_end - text);
			g_signal_handlers_block_by_func (G_OBJECT (buffer), G_CALLBACK (body_insert_text), window);
			g_signal_emit_by_name (G_OBJECT (buffer), "insert-text", location,
					       (gpointer) result, (gpointer) (utf8_end - text),
					       (gpointer) window);
			g_signal_handlers_unblock_by_func (G_OBJECT (buffer), G_CALLBACK (body_insert_text), window);
		}

	}
	if (offset + utf8_len > MAX_BODY_LENGTH) {
		if (priv->max_chars_banner == NULL) {
			priv->max_chars_banner = hildon_banner_show_information (GTK_WIDGET (window), NULL, 
										 _CS("ckdg_ib_maximum_characters_reached"));
			g_object_weak_ref (G_OBJECT (priv->max_chars_banner), (GWeakNotify) max_chars_banner_unref, window);
		}
	}
}

static void  
subject_field_changed (GtkEditable *editable, 
		       ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	update_window_title (window);
	gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
}
static void  
subject_field_insert_text (GtkEditable *editable, 
			   gchar *new_text,
			   gint new_text_length,
			   gint *position,
			   ModestMsgEditWindow *window)
{
	GString *result = g_string_new ("");
	gchar *current;
	gint result_len = 0;
	const gchar *entry_text = NULL;
	gint old_length;

	entry_text = gtk_entry_get_text (GTK_ENTRY (editable));
	old_length = g_utf8_strlen (entry_text, -1);

	for (current = new_text; current != NULL && *current != '\0'; current = g_utf8_next_char (current)) {
		gunichar c = g_utf8_get_char_validated (current, 8);
		/* Invalid unichar, stop */
		if (c == -1)
			break;
		/* a bullet */
		if (c == 0x2022)
			continue;
		result = g_string_append_unichar (result, c);
		result_len++;
	}

	if (MIN (result_len, 1000) != g_utf8_strlen (new_text, 1000)) {
		g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");
		if (result_len > 0)
		{
			/* Prevent endless recursion */
			g_signal_handlers_block_by_func(G_OBJECT(editable), G_CALLBACK(subject_field_insert_text), window);
			g_signal_emit_by_name (editable, "insert-text", 
					       (gpointer) result->str, (gpointer) result->len,
					       (gpointer) position, (gpointer) window);
		       g_signal_handlers_unblock_by_func(G_OBJECT(editable), G_CALLBACK(subject_field_insert_text), window);
		}
	}

	if (result_len + old_length > 1000) {
		hildon_banner_show_information (GTK_WIDGET (window), NULL, 
						_CS("ckdg_ib_maximum_characters_reached"));
	}
	g_string_free (result, TRUE);
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
			if (!gtk_text_iter_forward_search (iter, found_text, GTK_TEXT_SEARCH_VISIBLE_ONLY|GTK_TEXT_SEARCH_TEXT_ONLY,
							   match_start, match_end, NULL)) {
				g_debug ("Matched string with collate, but not matched in model");
			}
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
		hildon_banner_show_information (NULL, NULL, _CS("ecdg_ib_find_rep_enter_text"));
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
			hildon_banner_show_information (NULL, NULL, _HL("ckct_ib_find_search_complete"));
	} else {
		GtkTextIter buffer_start;
		gtk_text_buffer_get_start_iter (priv->text_buffer, &buffer_start);
		result = gtk_text_iter_forward_search_insensitive (&buffer_start, current_search, 
								   &match_start, &match_end);
		if (!result)
			hildon_banner_show_information (NULL, NULL, _HL("ckct_ib_find_no_matches"));
	}

	/* Mark as selected the string found in search */
	if (result) {
		gtk_text_buffer_select_range (priv->text_buffer, &match_start, &match_end);
		gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->msg_body), &match_start, 0.0, TRUE, 0.0, 0.0);
		correct_scroll_without_drag_check (MODEST_MSG_EDIT_WINDOW (window), FALSE);
	} else {
		g_free (priv->last_search);
		priv->last_search = NULL;
	}
	g_free (current_search);
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

static void
modest_msg_edit_window_find_toolbar_close (GtkWidget *widget,
                                          ModestMsgEditWindow *window)
{
	modest_msg_edit_window_toggle_find_toolbar (window, FALSE);
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

	if (priv->draft_msg != NULL) {
		g_object_unref (priv->draft_msg);
	}

	if (draft != NULL) {
		g_object_ref (draft);
		header = tny_msg_get_header (draft);
		if (priv->msg_uid) {
			g_free (priv->msg_uid);
			priv->msg_uid = NULL;
		}
		priv->msg_uid = modest_tny_folder_get_header_unique_id (header);
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

	if (tag == NULL) return;
	g_object_get (G_OBJECT (tag), "name", &tag_name, NULL);
	if ((tag_name != NULL) && (g_str_has_prefix (tag_name, "image-tag-replace-"))) {
		replace_with_images (window, priv->images);
	}
}

void                    
modest_msg_edit_window_add_part (ModestMsgEditWindow *window,
				 TnyMimePart *part)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	g_return_if_fail (TNY_IS_MIME_PART (part));
	tny_list_prepend (priv->attachments, (GObject *) part);
	modest_attachments_view_add_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), part, TRUE, 0);
	gtk_widget_set_no_show_all (priv->attachments_caption, FALSE);
	gtk_widget_show_all (priv->attachments_caption);
	gtk_text_buffer_set_modified (priv->text_buffer, TRUE);
}

const gchar*    
modest_msg_edit_window_get_message_uid (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), NULL);	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	return priv->msg_uid;
}

GtkWidget *
modest_msg_edit_window_get_child_widget (ModestMsgEditWindow *win,
					 ModestMsgEditWindowWidgetType widget_type)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (win), NULL);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (win);

	switch (widget_type) {
	case MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BODY:
		return priv->msg_body;
		break;
	case MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_TO:
		return priv->to_field;
		break;
	case MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_CC:
		return priv->cc_field;
		break;
	case MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BCC:
		return priv->bcc_field;
		break;
	case MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_SUBJECT:
		return priv->subject_field;
		break;
	case MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_ATTACHMENTS:
		return priv->attachments_view;
		break;
	default:
		return NULL;
	}
}

static void 
remove_tags (WPTextBuffer *buffer)
{
	GtkTextIter start, end;

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);
	gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (buffer), &end);

	gtk_text_buffer_remove_all_tags (GTK_TEXT_BUFFER (buffer), &start, &end);
}

static void
on_account_removed (TnyAccountStore *account_store, 
		    TnyAccount *account,
		    gpointer user_data)
{
	/* Do nothing if it's a store account, because we use the
	   transport to send the messages */
	if (tny_account_get_account_type(account) == TNY_ACCOUNT_TYPE_TRANSPORT) {
		const gchar *parent_acc = NULL;
		const gchar *our_acc = NULL;

		our_acc = modest_window_get_active_account (MODEST_WINDOW (user_data));
		parent_acc = modest_tny_account_get_parent_modest_account_name_for_server_account (account);
		/* Close this window if I'm showing a message of the removed account */
		if (strcmp (parent_acc, our_acc) == 0)
			modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (user_data));
	}
}

static void update_signature (ModestMsgEditWindow *self,
			      const gchar *old_account, 
			      const gchar *new_account)
{
	ModestMsgEditWindowPrivate *priv;
	gboolean has_old_signature, has_new_signature;
	GtkTextIter iter;
	GtkTextIter match_start, match_end;
	ModestAccountMgr *mgr;
	gchar *signature;
	gchar *full_signature;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	gtk_text_buffer_begin_user_action (priv->text_buffer);

	gtk_text_buffer_get_start_iter (priv->text_buffer, &iter);
	mgr = modest_runtime_get_account_mgr ();


	if (old_account) {
		signature = modest_account_mgr_get_signature_from_recipient (mgr, old_account, &has_old_signature);
		if (has_old_signature) {
			full_signature = g_strconcat ("\n--\n", signature, NULL);
			if (gtk_text_iter_forward_search (&iter, full_signature, 0, &match_start, &match_end, NULL)) {
				gtk_text_buffer_delete (priv->text_buffer, &match_start, &match_end);
				iter = match_start;
			} else if (gtk_text_iter_forward_search (&iter, _("mcen_ia_editor_original_message"), 0,
								 &match_start, &match_end, NULL)) {
				iter = match_start;
			}
			g_free (full_signature);
		}
		g_free (signature);
	}

	priv->last_from_account = modest_selector_picker_get_active_id (MODEST_SELECTOR_PICKER (priv->from_field));
	signature = modest_account_mgr_get_signature_from_recipient (mgr, new_account, &has_new_signature);
	if (has_new_signature) {
		full_signature = g_strconcat ("\n--\n", signature, NULL);
		gtk_text_buffer_insert (priv->text_buffer, &iter, full_signature, -1);
		g_free (full_signature);
	}
	g_free (signature);
	gtk_text_buffer_end_user_action (priv->text_buffer);
}

static void update_branding (ModestMsgEditWindow *self,
			     const gchar *new_account)
{
	ModestMsgEditWindowPrivate *priv;
	ModestAccountMgr *mgr;
	const GdkPixbuf *new_icon = NULL;
	gchar *new_label = NULL;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	mgr = modest_runtime_get_account_mgr ();

	modest_account_mgr_get_branding_from_recipient (mgr, new_account, &new_label, &new_icon, MODEST_ICON_SIZE_SMALL);
	if (new_icon) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (priv->brand_icon), (GdkPixbuf *) new_icon);
		gtk_widget_show (priv->brand_icon);
	} else {
		gtk_widget_hide (priv->brand_icon);
	}
	if (new_label) {
		gtk_label_set_text (GTK_LABEL (priv->brand_label), new_label);
		gtk_widget_show (priv->brand_label);
		g_free (new_label);
	} else {
		gtk_widget_hide (priv->brand_label);
	}
}

static void
from_field_changed (HildonPickerButton *button,
		    ModestMsgEditWindow *self)
{
	ModestMsgEditWindowPrivate *priv;
	gchar *old_account, *new_account;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	old_account = priv->last_from_account;
	priv->last_from_account = modest_selector_picker_get_active_id (MODEST_SELECTOR_PICKER (priv->from_field));
	new_account = priv->last_from_account;

	update_signature (self, old_account, new_account);
	update_branding (self, new_account);

}

typedef struct _MessageSettingsHelper {
	ModestMsgEditWindow *window;
	GSList *priority_group;
	GSList *format_group;
	GtkToggleButton *current_priority;
	GtkToggleButton *current_format;
} MessageSettingsHelper;

static void
on_priority_toggle (GtkToggleButton *button, 
		    MessageSettingsHelper *helper)
{
	GSList *node;
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (helper->window);
	if (gtk_toggle_button_get_active (button)) {

		for (node = helper->priority_group; node != NULL; node = g_slist_next (node)) {
			GtkToggleButton *node_button = (GtkToggleButton *) node->data;
			if ((node_button != button) &&
			    gtk_toggle_button_get_active (node_button)) {
				gtk_toggle_button_set_active (node_button, FALSE);
			}
		}
		helper->current_priority = button;
	} else {
		gboolean found = FALSE;
		/* If no one is active, activate it again */
		for (node = helper->priority_group; node != NULL; node = g_slist_next (node)) {
			GtkToggleButton *node_button = (GtkToggleButton *) node->data;
			if (gtk_toggle_button_get_active (node_button)) {
				found = TRUE;
				break;
			}
		}
		if (!found) {
			gtk_toggle_button_set_active (button, TRUE);
		}
	}
}

static void
on_format_toggle (GtkToggleButton *button,
		  MessageSettingsHelper *helper)
{
	GSList *node;
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (helper->window);
	if (gtk_toggle_button_get_active (button)) {

		for (node = helper->format_group; node != NULL; node = g_slist_next (node)) {
			GtkToggleButton *node_button = (GtkToggleButton *) node->data;
			if ((node_button != button) &&
			    gtk_toggle_button_get_active (node_button)) {
				gtk_toggle_button_set_active (node_button, FALSE);
			}
		}
		helper->current_format = button;
	} else {
		gboolean found = FALSE;
		/* If no one is active, activate it again */
		for (node = helper->format_group; node != NULL; node = g_slist_next (node)) {
			GtkToggleButton *node_button = (GtkToggleButton *) node->data;
			if (gtk_toggle_button_get_active (node_button)) {
				found = TRUE;
				break;
			}
		}
		if (!found) {
			gtk_toggle_button_set_active (button, TRUE);
		}
	}

}

static void
modest_msg_edit_window_show_msg_settings_dialog (ModestMsgEditWindow *window)
{
	GtkWidget *dialog;
	GtkWidget *align;
	GtkWidget *vbox;
	GtkWidget *priority_hbox;
	GtkWidget *high_toggle, *medium_toggle, *low_toggle;
	GtkWidget *captioned;
	GtkSizeGroup *title_sizegroup, *value_sizegroup;
	GtkWidget *format_hbox;
	GtkWidget *html_toggle, *text_toggle;
	ModestMsgEditWindowPrivate *priv;
	MessageSettingsHelper helper = {0,};

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	helper.window = window;
	helper.priority_group = NULL;
	helper.format_group = NULL;

	title_sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	value_sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	dialog = gtk_dialog_new_with_buttons (_("mcen_me_message_settings"), NULL,
					      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					      _HL("wdgt_bd_done"), GTK_RESPONSE_ACCEPT, NULL);
	vbox = gtk_vbox_new (FALSE, 0);
	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE, 0);
	gtk_container_add (GTK_CONTAINER (align), vbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), align);
	gtk_widget_show (align);
	gtk_widget_show (vbox);

	/* Priority toggles */
	priority_hbox = gtk_hbox_new (TRUE, 0);
	high_toggle = hildon_gtk_toggle_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (high_toggle), _("mcen_me_editor_priority_high"));
	helper.priority_group = g_slist_prepend (helper.priority_group, high_toggle);
	g_object_set_data (G_OBJECT (high_toggle), "priority", GINT_TO_POINTER (TNY_HEADER_FLAG_HIGH_PRIORITY));
	medium_toggle = hildon_gtk_toggle_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (medium_toggle), _("mcen_me_editor_priority_normal"));
	helper.priority_group = g_slist_prepend (helper.priority_group, medium_toggle);
	g_object_set_data (G_OBJECT (medium_toggle), "priority", GINT_TO_POINTER (TNY_HEADER_FLAG_NORMAL_PRIORITY));
	low_toggle = hildon_gtk_toggle_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (low_toggle), _("mcen_me_editor_priority_low"));
	helper.priority_group = g_slist_prepend (helper.priority_group, low_toggle);
	g_object_set_data (G_OBJECT (low_toggle), "priority", GINT_TO_POINTER (TNY_HEADER_FLAG_LOW_PRIORITY));
	gtk_box_pack_start (GTK_BOX (priority_hbox), low_toggle, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (priority_hbox), medium_toggle, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (priority_hbox), high_toggle, TRUE, TRUE, 0);
	gtk_widget_show_all (priority_hbox);
	captioned = modest_maemo_utils_create_captioned (title_sizegroup, value_sizegroup,
							 _("mcen_me_editor_message_priority"), FALSE, priority_hbox);
	gtk_widget_show (captioned);
	gtk_box_pack_start (GTK_BOX (vbox), captioned, FALSE, FALSE, 0);

	/* format toggles */
	format_hbox = gtk_hbox_new (TRUE, 0);
	html_toggle = hildon_gtk_toggle_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (html_toggle), _("mcen_me_editor_formatted_text"));
	helper.format_group = g_slist_prepend (helper.format_group, html_toggle);
	g_object_set_data (G_OBJECT (html_toggle), "format", GINT_TO_POINTER (MODEST_MSG_EDIT_FORMAT_HTML));
	g_object_set_data (G_OBJECT (html_toggle), "file-format", GINT_TO_POINTER (MODEST_FILE_FORMAT_FORMATTED_TEXT));
	text_toggle = hildon_gtk_toggle_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (text_toggle), _("mcen_me_editor_plain_text"));
	helper.format_group = g_slist_prepend (helper.format_group, text_toggle);
	g_object_set_data (G_OBJECT (text_toggle), "format", GINT_TO_POINTER (MODEST_MSG_EDIT_FORMAT_TEXT));
	g_object_set_data (G_OBJECT (text_toggle), "file-format", GINT_TO_POINTER (MODEST_FILE_FORMAT_PLAIN_TEXT));
	gtk_box_pack_start (GTK_BOX (format_hbox), html_toggle, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (format_hbox), text_toggle, TRUE, TRUE, 0);
	gtk_widget_show_all (format_hbox);
	gtk_widget_show (format_hbox);
	gtk_box_pack_start (GTK_BOX (vbox), format_hbox, FALSE, FALSE, 0);


	g_object_unref (title_sizegroup);
	g_object_unref (value_sizegroup);

	/* Set current values */
	switch (priv->priority_flags) {
	case TNY_HEADER_FLAG_HIGH_PRIORITY:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (high_toggle), TRUE);
		helper.current_priority = GTK_TOGGLE_BUTTON (high_toggle);
		break;
	case TNY_HEADER_FLAG_LOW_PRIORITY:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (low_toggle), TRUE);
		helper.current_priority = GTK_TOGGLE_BUTTON (low_toggle);
		break;
	default:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (medium_toggle), TRUE);
		helper.current_priority = GTK_TOGGLE_BUTTON (medium_toggle);
		break;
	}

	switch (modest_msg_edit_window_get_format (window)) {
	case MODEST_MSG_EDIT_FORMAT_TEXT:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (text_toggle), TRUE);
		helper.current_format = GTK_TOGGLE_BUTTON (text_toggle);
		break;
	case MODEST_MSG_EDIT_FORMAT_HTML:
	default:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (html_toggle), TRUE);
		helper.current_format = GTK_TOGGLE_BUTTON (html_toggle);
		break;
	}

	/* Signal connects */
	g_signal_connect (G_OBJECT (high_toggle), "toggled", G_CALLBACK (on_priority_toggle), &helper);
	g_signal_connect (G_OBJECT (medium_toggle), "toggled", G_CALLBACK (on_priority_toggle), &helper);
	g_signal_connect (G_OBJECT (low_toggle), "toggled", G_CALLBACK (on_priority_toggle), &helper);
	g_signal_connect (G_OBJECT (html_toggle), "toggled", G_CALLBACK (on_format_toggle), &helper);
	g_signal_connect (G_OBJECT (text_toggle), "toggled", G_CALLBACK (on_format_toggle), &helper);

	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (dialog), GTK_WINDOW (window));

	/* Save settings if the user clicked on done */
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		TnyHeaderFlags flags;
		ModestMsgEditFormat old_format, new_format;

		/* Set priority flags */
		flags = (TnyHeaderFlags) g_object_get_data (G_OBJECT (helper.current_priority), "priority");
		if (priv->priority_flags !=  flags)
			modest_msg_edit_window_set_priority_flags (window, flags);

		/* Set edit format */
		old_format = modest_msg_edit_window_get_format (window);
		new_format = (ModestMsgEditFormat) g_object_get_data (G_OBJECT (helper.current_format), "format");
		if (old_format != new_format) {
			gint file_format = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (helper.current_format), "file-format"));
			modest_msg_edit_window_set_file_format (window, file_format);
		}
	}

	gtk_widget_destroy (dialog);
	g_slist_free (helper.priority_group);
}

static void
on_message_settings (GtkAction *action,
		     ModestMsgEditWindow *window)
{
	modest_msg_edit_window_show_msg_settings_dialog (window);
}

static void
on_cc_button_toggled (HildonCheckButton *button,
		      ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	modest_msg_edit_window_show_cc (MODEST_MSG_EDIT_WINDOW (window),
					hildon_check_button_get_active (button));
}

static void
on_bcc_button_toggled (HildonCheckButton *button,
		      ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	modest_msg_edit_window_show_bcc (MODEST_MSG_EDIT_WINDOW (window),
					hildon_check_button_get_active (button));
}

static void 
setup_menu (ModestMsgEditWindow *self)
{
	ModestMsgEditWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW(self));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	/* Settings menu buttons */
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_editor_checknames"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_check_names),
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_undo"), "<Ctrl>z",
					   APP_MENU_CALLBACK (modest_ui_actions_on_undo),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_undo));

	priv->cc_button = hildon_check_button_new (0);
	gtk_button_set_label (GTK_BUTTON (priv->cc_button), _("mcen_me_editor_showcc"));
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (priv->cc_button),
					FALSE);
	modest_hildon2_window_add_button_to_menu (MODEST_HILDON2_WINDOW (self), GTK_BUTTON (priv->cc_button),
						  NULL);
	g_signal_connect (G_OBJECT (priv->cc_button), "toggled",
			  G_CALLBACK (on_cc_button_toggled), (gpointer) self);
	gtk_button_set_alignment (GTK_BUTTON (priv->cc_button), 0.5, 0.5);
	gtk_button_set_alignment (GTK_BUTTON (priv->cc_button), 0.5, 0.5);

	priv->bcc_button = hildon_check_button_new (0);
	gtk_button_set_label (GTK_BUTTON (priv->bcc_button), _("mcen_me_editor_showbcc"));
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (priv->bcc_button),
					FALSE);
	modest_hildon2_window_add_button_to_menu (MODEST_HILDON2_WINDOW (self), GTK_BUTTON (priv->bcc_button),
						  NULL);
	g_signal_connect (G_OBJECT (priv->bcc_button), "toggled",
			  G_CALLBACK (on_bcc_button_toggled), (gpointer) self);
	gtk_button_set_alignment (GTK_BUTTON (priv->bcc_button), 0.5, 0.5);
	gtk_button_set_alignment (GTK_BUTTON (priv->bcc_button), 0.5, 0.5);

	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_editor_attach_inlineimage"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_insert_image),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_set_style));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_editor_add_attachment"), NULL,
					   APP_MENU_CALLBACK (modest_msg_edit_window_add_attachment_clicked),
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_remove_attachments"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_remove_attachments),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_editor_remove_attachment));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_message_settings"), NULL,
					   APP_MENU_CALLBACK (on_message_settings),
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_viewer_find"), "<Ctrl>f",
					   APP_MENU_CALLBACK (modest_ui_actions_on_toggle_find_in_page),
					   NULL);
}

static void
emit_open_addressbook (GtkButton *button,
		       ModestRecptEditor *editor)
{
	g_signal_emit_by_name (G_OBJECT (editor), "open-addressbook");
}

static GtkWidget *
_create_addressbook_box (GtkSizeGroup *title_size_group, GtkSizeGroup *value_size_group,
			 const gchar *label, GtkWidget *control)
{
	GtkWidget *abook_button;
	GtkWidget *align;
	GtkWidget *box;
	GtkWidget *label_widget;

	box = gtk_hbox_new (FALSE, 0);

	align = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 0, MODEST_MARGIN_DEFAULT);

	abook_button = hildon_gtk_button_new (HILDON_SIZE_FINGER_HEIGHT);
	label_widget = gtk_label_new (label);
	gtk_misc_set_alignment (GTK_MISC (label_widget), 0.0, 0.5);
	gtk_container_add (GTK_CONTAINER (abook_button), label_widget);

	gtk_container_add (GTK_CONTAINER (align), abook_button);
	gtk_widget_set_size_request (label_widget, 148 - MODEST_MARGIN_DOUBLE, -1);
	gtk_box_pack_start (GTK_BOX (box), align, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
	if (title_size_group)
		gtk_size_group_add_widget (title_size_group, label_widget);
	if (value_size_group)
		gtk_size_group_add_widget (value_size_group, control);

	g_signal_connect (G_OBJECT (abook_button), "clicked",
			  G_CALLBACK (emit_open_addressbook), control);
  
	return box;  
}

static void 
max_chars_banner_unref (ModestMsgEditWindow *self, GObject *old_ref)
{
	ModestMsgEditWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW(self));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);
	priv->max_chars_banner = NULL;
}

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
#include <tny-account-store.h>
#include <tny-fs-stream.h>

#include <config.h>

#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>

#include <widgets/modest-msg-edit-window.h>
#include <widgets/modest-combo-box.h>

#include <modest-runtime.h>

#include <widgets/modest-msg-edit-window-ui.h>
#include "modest-icon-names.h"
#include "modest-widget-memory.h"
#include "modest-window-priv.h"
#include "modest-mail-operation.h"
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg.h"
#include <tny-simple-list.h>
#include <wptextview.h>
#include <wptextbuffer.h>
#include <hildon-widgets/hildon-color-selector.h>
#include <hildon-widgets/hildon-color-button.h>

#ifdef MODEST_HILDON_VERSION_0
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#else
#include <hildon/hildon-file-chooser-dialog.h>
#endif /*MODEST_HILDON_VERSION_0 */


#define DEFAULT_FONT_SIZE 3
#define DEFAULT_FONT 2
#define DEFAULT_SIZE_COMBOBOX_WIDTH 80

static void  modest_msg_edit_window_class_init   (ModestMsgEditWindowClass *klass);
static void  modest_msg_edit_window_init         (ModestMsgEditWindow *obj);
static void  modest_msg_edit_window_finalize     (GObject *obj);

static void  text_buffer_refresh_attributes (WPTextBuffer *buffer, ModestMsgEditWindow *window);
static void  modest_msg_edit_window_color_button_change (ModestMsgEditWindow *window,
							 gpointer userdata);
static void  modest_msg_edit_window_size_combobox_change (ModestMsgEditWindow *window,
							  gpointer userdata);
static void  modest_msg_edit_window_font_combobox_change (ModestMsgEditWindow *window,
							  gpointer userdata);
static void  modest_msg_edit_window_setup_toolbar (ModestMsgEditWindow *window);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMsgEditWindowPrivate ModestMsgEditWindowPrivate;
struct _ModestMsgEditWindowPrivate {
	GtkWidget   *msg_body;
	GtkWidget   *from_field;
	GtkWidget   *to_field;
	GtkWidget   *cc_field;
	GtkWidget   *bcc_field;
	GtkWidget   *subject_field;

	GtkTextBuffer *text_buffer;

	GtkWidget   *font_color_button;
	GtkWidget   *size_combobox;
	GtkWidget   *font_combobox;

	gint last_cid;
	GList *attachments;
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
modest_msg_edit_window_class_init (ModestMsgEditWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_edit_window_finalize;

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
	priv->attachments = NULL;
	priv->last_cid = 0;
}



static void
save_settings (ModestMsgEditWindow *self)
{
	modest_widget_memory_save (modest_runtime_get_conf(),
				   G_OBJECT(self), "modest-edit-msg-window");
}


static void
restore_settings (ModestMsgEditWindow *self)
{
	modest_widget_memory_restore (modest_runtime_get_conf(),
				      G_OBJECT(self), "modest-edit-msg-window");
}


/* FIXME: this is a dup from the one in gtk/ */
static ModestPairList*
get_transports (void)
{
	ModestAccountMgr *account_mgr;
	GSList *transports = NULL;
	GSList *cursor, *accounts;
	
	account_mgr = modest_runtime_get_account_mgr();
 	cursor = accounts = modest_account_mgr_account_names (account_mgr); 
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
init_window (ModestMsgEditWindow *obj)
{
	GtkWidget *to_button, *cc_button, *bcc_button; 
	GtkWidget *header_table;
	GtkWidget *main_vbox;
	GtkWidget *body_scroll;
	ModestMsgEditWindowPrivate *priv;
	ModestPairList *protos;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);

	to_button     = gtk_button_new_with_label (_("To..."));
	cc_button     = gtk_button_new_with_label (_("Cc..."));
	bcc_button    = gtk_button_new_with_label (_("Bcc..."));
		
	protos = get_transports ();
 	priv->from_field    = modest_combo_box_new (protos, g_str_equal);
	modest_pair_list_free (protos);

	priv->to_field      = gtk_entry_new_with_max_length (80);
	priv->cc_field      = gtk_entry_new_with_max_length (80);
	priv->bcc_field     = gtk_entry_new_with_max_length (80);
	priv->subject_field = gtk_entry_new_with_max_length (80);
	
	header_table = gtk_table_new (5,2, FALSE);
	
	gtk_table_attach (GTK_TABLE(header_table), gtk_label_new (_("From:")),
			  0,1,0,1, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), to_button,     0,1,1,2, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), cc_button,     0,1,2,3, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), bcc_button,    0,1,3,4, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), gtk_label_new (_("Subject:")),
			  0,1,4,5, GTK_SHRINK, 0, 0, 0);

	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->from_field,   1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->to_field,     1,2,1,2);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->cc_field,     1,2,2,3);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->bcc_field,    1,2,3,4);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->subject_field,1,2,4,5);

	priv->msg_body = wp_text_view_new ();
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (priv->msg_body), GTK_WRAP_WORD_CHAR);
	priv->text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));
	g_object_set (priv->text_buffer, "font_scale", 1.0, NULL);
	wp_text_buffer_enable_rich_text (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
/* 	gtk_text_buffer_set_can_paste_rich_text (priv->text_buffer, TRUE); */
	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
	g_signal_connect (G_OBJECT (priv->text_buffer), "refresh_attributes",
			  G_CALLBACK (text_buffer_refresh_attributes), obj);

	body_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (body_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (body_scroll), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (body_scroll), priv->msg_body);
	
	main_vbox = gtk_vbox_new  (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(main_vbox), header_table, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), body_scroll, TRUE, TRUE, 6);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	
	if (!modest_conf_get_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_CC, NULL))
		gtk_widget_hide (priv->cc_field);
	if (!modest_conf_get_bool(modest_runtime_get_conf(), MODEST_CONF_SHOW_BCC, NULL))
		gtk_widget_hide (priv->bcc_field);

	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
}
	


static void
modest_msg_edit_window_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, ModestMsgEditWindow *self)
{
	save_settings (self);
	return FALSE;
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
	const gchar *to, *cc, *bcc, *subject, *body;
	ModestMsgEditWindowPrivate *priv;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (TNY_IS_MSG (msg));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	header = tny_msg_get_header (msg);
	to      = tny_header_get_to (header);
	cc      = tny_header_get_cc (header);
	bcc     = tny_header_get_bcc (header);
	subject = tny_header_get_subject (header);

	if (to)
		gtk_entry_set_text (GTK_ENTRY(priv->to_field),  to);
	if (cc)
		gtk_entry_set_text (GTK_ENTRY(priv->cc_field),  cc);
	if (bcc)
		gtk_entry_set_text (GTK_ENTRY(priv->bcc_field), bcc);
	if (subject)
		gtk_entry_set_text (GTK_ENTRY(priv->subject_field), subject);	
	
/* 	gtk_text_buffer_set_can_paste_rich_text (priv->text_buffer, TRUE); */
	wp_text_buffer_reset_buffer (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
	body = modest_tny_msg_get_body (msg, FALSE);
	if ((body!=NULL) && (body[0] != '\0')) {
		wp_text_buffer_load_document_begin (WP_TEXT_BUFFER (priv->text_buffer), TRUE);
		wp_text_buffer_load_document_write (WP_TEXT_BUFFER (priv->text_buffer),
						    (gchar *) body,
						    -1);
		wp_text_buffer_load_document_end (WP_TEXT_BUFFER (priv->text_buffer));
	} else {
		WPTextBufferFormat fmt = {0};

		fmt.font_size = DEFAULT_FONT_SIZE;
		fmt.font = DEFAULT_FONT;
		fmt.rich_text = 1;
		fmt.text_position = TEXT_POSITION_NORMAL;
		fmt.justification = 0;
		fmt.cs.font_size = 1;
		fmt.cs.font = 1;
		fmt.cs.text_position = 1;
		fmt.cs.justification = 1;
		wp_text_buffer_set_format (WP_TEXT_BUFFER (priv->text_buffer), &fmt);
	}

	/* TODO: lower priority, select in the From: combo to the
	   value that comes from msg <- not sure, should it be
	   allowed? */
	
	/* TODO: set attachments */
}

static void
modest_msg_edit_window_setup_toolbar (ModestMsgEditWindow *window)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	GtkWidget *font_placeholder;
	GtkWidget *tool_item;
	gint insert_index;
	gint size_index;
	gint font_index;

	/* Toolbar */
	parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	hildon_window_add_toolbar (HILDON_WINDOW (window), GTK_TOOLBAR (parent_priv->toolbar));

	/* should we hide the toolbar? */
	if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_TOOLBAR, NULL))
		gtk_widget_hide (parent_priv->toolbar);

	/* Font management toolbar elements */
	font_placeholder = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/FontAttributes");
	insert_index = gtk_toolbar_get_item_index(GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM(font_placeholder));

	/* font color */
	tool_item = GTK_WIDGET (gtk_tool_item_new ());
	priv->font_color_button = hildon_color_button_new ();
	gtk_container_add (GTK_CONTAINER (tool_item), priv->font_color_button);
	gtk_toolbar_insert(GTK_TOOLBAR(parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	g_signal_connect_swapped (G_OBJECT (priv->font_color_button), "notify::color", G_CALLBACK (modest_msg_edit_window_color_button_change), window);

	/* font_size */
	priv->size_combobox = gtk_combo_box_new_text ();
	gtk_widget_set_size_request (priv->size_combobox, DEFAULT_SIZE_COMBOBOX_WIDTH, -1);
	for (size_index = 0; size_index < WP_FONT_SIZE_COUNT; size_index++) {
		gchar size_text[5];
		snprintf(size_text, sizeof(size_text), "%d", wp_font_size[size_index]);
		gtk_combo_box_append_text (GTK_COMBO_BOX (priv->size_combobox), size_text);
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->size_combobox), wp_get_font_size_index(12, 4));
	tool_item = GTK_WIDGET (gtk_tool_item_new ());
	gtk_container_add (GTK_CONTAINER (tool_item), priv->size_combobox);
	gtk_toolbar_insert(GTK_TOOLBAR(parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	g_signal_connect_swapped (G_OBJECT (priv->size_combobox), "changed", G_CALLBACK (modest_msg_edit_window_size_combobox_change), window);

	priv->font_combobox = gtk_combo_box_new_text ();
	for (font_index = 0; font_index < wp_get_font_count (); font_index++) {
		gtk_combo_box_append_text (GTK_COMBO_BOX (priv->font_combobox), wp_get_font_name (font_index));
	}
	tool_item = GTK_WIDGET (gtk_tool_item_new ());
	gtk_container_add (GTK_CONTAINER (tool_item), priv->font_combobox);
	gtk_toolbar_insert (GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM (tool_item), insert_index);
	g_signal_connect_swapped (G_OBJECT (priv->font_combobox), "changed", G_CALLBACK (modest_msg_edit_window_font_combobox_change), window);
}



ModestWindow*
modest_msg_edit_window_new (TnyMsg *msg, const gchar *account_name)
{
	GObject *obj;
	ModestWindowPrivate *parent_priv;
	ModestMsgEditWindowPrivate *priv;
	GtkActionGroup *action_group;
	GError *error = NULL;

	g_return_val_if_fail (msg, NULL);
	
	obj = g_object_new(MODEST_TYPE_MSG_EDIT_WINDOW, NULL);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (obj);

	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMsgEditWindowActions");

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

	set_msg (MODEST_MSG_EDIT_WINDOW (obj), msg);

	text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (obj));
	
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
	data->to      =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->to_field));
	data->cc      =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->cc_field));
	data->bcc     =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->bcc_field));
	data->subject =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->subject_field));	
	data->plain_body =  (gchar *) gtk_text_buffer_get_text (priv->text_buffer, &b, &e, FALSE);
	data->html_body  =  get_formatted_data (edit_window);
	data->attachments = priv->attachments;

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
	
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	wp_text_buffer_get_attributes (WP_TEXT_BUFFER (priv->text_buffer), buffer_format, FALSE);
	
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsBold");
	toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bold);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsItalics");
	toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->italic);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ActionsBulletedList");
	toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), buffer_format->bullet);

	g_signal_handlers_block_by_func (G_OBJECT (priv->font_color_button), 
					 G_CALLBACK (modest_msg_edit_window_color_button_change),
					 window);
	hildon_color_button_set_color (HILDON_COLOR_BUTTON (priv->font_color_button), & (buffer_format->color));
	g_signal_handlers_unblock_by_func (G_OBJECT (priv->font_color_button), 
					   G_CALLBACK (modest_msg_edit_window_color_button_change),
					   window);

	g_signal_handlers_block_by_func (G_OBJECT (priv->size_combobox), 
					 G_CALLBACK (modest_msg_edit_window_size_combobox_change),
					 window);
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->size_combobox), buffer_format->font_size);
	g_signal_handlers_unblock_by_func (G_OBJECT (priv->size_combobox), 
					   G_CALLBACK (modest_msg_edit_window_size_combobox_change),
					   window);

	g_signal_handlers_block_by_func (G_OBJECT (priv->font_combobox), 
					 G_CALLBACK (modest_msg_edit_window_font_combobox_change),
					 window);
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->font_combobox), buffer_format->font);
	g_signal_handlers_unblock_by_func (G_OBJECT (priv->font_combobox), 
					   G_CALLBACK (modest_msg_edit_window_font_combobox_change),
					   window);

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
	
	dialog = hildon_color_selector_new (GTK_WINDOW (window));
	hildon_color_selector_set_color (HILDON_COLOR_SELECTOR (dialog), & (buffer_format->color));
	g_free (buffer_format);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK:
		new_color = hildon_color_selector_get_color (HILDON_COLOR_SELECTOR (dialog));
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
	const GdkColor *old_color = NULL;
	const GdkColor *new_color = NULL;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	old_color = wp_text_buffer_get_background_color (WP_TEXT_BUFFER (priv->text_buffer));
	
	dialog = hildon_color_selector_new (GTK_WINDOW (window));
	hildon_color_selector_set_color (HILDON_COLOR_SELECTOR (dialog), (GdkColor *) old_color);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (response) {
	case GTK_RESPONSE_OK:
		new_color = hildon_color_selector_get_color (HILDON_COLOR_SELECTOR (dialog));
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
	new_color = hildon_color_button_get_color (HILDON_COLOR_BUTTON (priv->font_color_button));

	wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FORECOLOR, (gpointer) new_color);
	gtk_window_set_focus (GTK_WINDOW (window), priv->msg_body);

}

static void
modest_msg_edit_window_size_combobox_change (ModestMsgEditWindow *window,
					     gpointer userdata)
{
	ModestMsgEditWindowPrivate *priv;
	gint new_size_index;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	gtk_widget_grab_focus (GTK_WIDGET (priv->msg_body));

	new_size_index = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->size_combobox));

	if (!wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT_SIZE, (gpointer) new_size_index))
		wp_text_view_reset_and_show_im (WP_TEXT_VIEW (priv->msg_body));

	text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (window));;
}

static void
modest_msg_edit_window_font_combobox_change (ModestMsgEditWindow *window,
					     gpointer userdata)
{
	ModestMsgEditWindowPrivate *priv;
	gint new_font_index;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);
	gtk_widget_grab_focus (GTK_WIDGET (priv->msg_body));

	new_font_index = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->font_combobox));

	if (!wp_text_buffer_set_attribute (WP_TEXT_BUFFER (priv->text_buffer), WPT_FONT, (gpointer) new_font_index))
		wp_text_view_reset_and_show_im (WP_TEXT_VIEW (priv->msg_body));

	text_buffer_refresh_attributes (WP_TEXT_BUFFER (priv->text_buffer), MODEST_MSG_EDIT_WINDOW (window));
}

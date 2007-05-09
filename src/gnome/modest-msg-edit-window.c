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
#include <glib/gi18n.h>
#include <string.h>
#include <tny-account-store.h>
#include <tny-simple-list.h>
#include <modest-conf.h>
#include <modest-runtime.h>
#include <modest-tny-msg.h>

#include <widgets/modest-window-priv.h>
#include <widgets/modest-msg-edit-window.h>
#include <widgets/modest-msg-edit-window-ui.h>
#include <widgets/modest-combo-box.h>

#include <modest-widget-memory.h>
#include <modest-account-mgr-helpers.h>

static void  modest_msg_edit_window_class_init   (ModestMsgEditWindowClass *klass);
static void  modest_msg_edit_window_init         (ModestMsgEditWindow *obj);
static void  modest_msg_edit_window_finalize     (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMsgEditWindowPrivate ModestMsgEditWindowPrivate;
struct _ModestMsgEditWindowPrivate {

	GtkWidget   *toolbar;
	GtkWidget   *menubar;

	GtkWidget   *msg_body;
	GtkWidget   *from_field;
	GtkWidget   *to_field;
	GtkWidget   *cc_field;
	GtkWidget   *bcc_field;
	GtkWidget   *subject_field;
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
	}
	return my_type;
}


static void
save_state (ModestWindow *self)
{
	modest_widget_memory_save (modest_runtime_get_conf (),
				    G_OBJECT(self), "modest-edit-msg-window");
}


static void
restore_settings (ModestMsgEditWindow *self)
{
	modest_widget_memory_restore (modest_runtime_get_conf (),
				      G_OBJECT(self), "modest-edit-msg-window");
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

	g_type_class_add_private (gobject_class, sizeof(ModestMsgEditWindowPrivate));

	modest_window_class->save_state_func = save_state;
}

static void
modest_msg_edit_window_init (ModestMsgEditWindow *obj)
{
	ModestMsgEditWindowPrivate *priv;
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);

	priv->toolbar       = NULL;
	priv->menubar       = NULL;
	priv->msg_body      = NULL;
	priv->from_field    = NULL;
	priv->to_field      = NULL;
	priv->cc_field      = NULL;
	priv->bcc_field     = NULL;
	priv->subject_field = NULL;
}


static ModestPairList*
get_transports (void)
{
	ModestAccountMgr *account_mgr;
	GSList *transports = NULL;
	GSList *cursor, *accounts;
	
	account_mgr = modest_runtime_get_account_mgr();
	cursor = accounts = modest_account_mgr_account_names (account_mgr, TRUE);
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
on_from_combo_changed (ModestComboBox *combo, ModestWindow *win)
{
	modest_window_set_active_account (
		win, modest_combo_box_get_active_id(combo));
}



static void
init_window (ModestMsgEditWindow *obj, const gchar* account)
{
	GtkWidget *to_button, *cc_button, *bcc_button; 
	GtkWidget *header_table;
	GtkWidget *main_vbox;
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	ModestPairList *protos;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	to_button     = gtk_button_new_with_label (_("To..."));
	cc_button     = gtk_button_new_with_label (_("Cc..."));
	bcc_button    = gtk_button_new_with_label (_("Bcc..."));
	
	protos = get_transports ();
 	priv->from_field    = modest_combo_box_new (protos, g_str_equal);
	modest_pair_list_free (protos);
	if (account) {
		modest_combo_box_set_active_id (MODEST_COMBO_BOX(priv->from_field),
						(gpointer)account);
		modest_window_set_active_account (MODEST_WINDOW(obj), account);
	}
	/* auto-update the active account */
	g_signal_connect (G_OBJECT(priv->from_field), "changed", G_CALLBACK(on_from_combo_changed), obj);
	
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

	priv->msg_body = gtk_text_view_new ();
	
	main_vbox = gtk_vbox_new  (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), header_table, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->msg_body, TRUE, TRUE, 6);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
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
	modest_window_save_state (MODEST_WINDOW(self));
	return FALSE;
}


static void
set_msg (ModestMsgEditWindow *self, TnyMsg *msg)
{
	TnyHeader *header;
	GtkTextBuffer *buf;
	const gchar *to, *cc, *bcc, *subject;
	ModestMsgEditWindowPrivate *priv;
	gchar *body;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (TNY_IS_MSG (msg));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	header  = tny_msg_get_header (msg);
	to      = tny_header_get_to (header);
	cc      = tny_header_get_cc (header);
	bcc     = tny_header_get_bcc (header);
	subject = tny_header_get_subject (header);

	if (to)
		gtk_entry_set_text (GTK_ENTRY(priv->to_field), to);
	if (cc)
		gtk_entry_set_text (GTK_ENTRY(priv->cc_field), cc);
	if (bcc)
		gtk_entry_set_text (GTK_ENTRY(priv->bcc_field),  bcc);
	if (subject)
		gtk_entry_set_text (GTK_ENTRY(priv->subject_field), subject);

	
	buf  = gtk_text_view_get_buffer (GTK_TEXT_VIEW(priv->msg_body));
	body = modest_tny_msg_get_body (msg, FALSE);
	if (body) 
		gtk_text_buffer_set_text (buf, body, -1);
	g_free (body);
}


ModestWindow *
modest_msg_edit_window_new (TnyMsg *msg, const gchar *account)
{
	ModestMsgEditWindow *self;
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkActionGroup *action_group;
	GError *error = NULL;

	g_return_val_if_fail (msg, NULL);
	
	self = MODEST_MSG_EDIT_WINDOW(g_object_new(MODEST_TYPE_MSG_EDIT_WINDOW, NULL));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	
	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMsgEditWindowActions");

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_msg_edit_action_entries,
				      G_N_ELEMENTS (modest_msg_edit_action_entries),
				      self);
	gtk_action_group_add_toggle_actions (action_group,
					     modest_msg_edit_toggle_action_entries,
					     G_N_ELEMENTS (modest_msg_edit_toggle_action_entries),
					     self);
	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 0);
	g_object_unref (action_group);

	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager,
					 MODEST_UIDIR "modest-msg-edit-window-ui.xml",
					 &error);
	if (error) {
		g_printerr ("modest: could not merge modest-msg-edit-window-ui.xml: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	}
	/* ****** */

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (self), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));

	/* Toolbar / Menubar */
	priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	priv->menubar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar");

	gtk_toolbar_set_tooltips (GTK_TOOLBAR (priv->toolbar), TRUE);

	/* Init window */
	init_window (MODEST_MSG_EDIT_WINDOW(self), account);

	restore_settings (MODEST_MSG_EDIT_WINDOW(self));
	
	gtk_window_set_title (GTK_WINDOW(self), "Modest");
	gtk_window_set_icon_from_file (GTK_WINDOW(self), MODEST_APP_ICON, NULL);

	g_signal_connect (G_OBJECT(self), "delete-event",
			  G_CALLBACK(on_delete_event), self);
	
	set_msg (self, msg);
	
	return MODEST_WINDOW(self);
}


MsgData * 
modest_msg_edit_window_get_msg_data (ModestMsgEditWindow *edit_window)
{
	MsgData *data;
	GtkTextBuffer *buf;
	GtkTextIter b, e;
	const gchar *account_name;
	gchar *from_string = NULL;
	ModestMsgEditWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window), NULL);

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (edit_window);
	
	account_name = (gchar*)modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->from_field));
	if (account_name) 
		from_string = modest_account_mgr_get_from_string (
			modest_runtime_get_account_mgr(), account_name);
	if (!from_string) {
		g_printerr ("modest: cannot get from string\n");
		return NULL;
	}
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));
	gtk_text_buffer_get_bounds (buf, &b, &e);

	/* don't free these (except from) */
	data = g_slice_new0 (MsgData);
	data->from    =  from_string, /* will be freed when data is freed */
	data->to      =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->to_field));
	data->cc      =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->cc_field));
	data->bcc     =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->bcc_field));
	data->subject =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->subject_field));
	data->plain_body    =  gtk_text_buffer_get_text (buf, &b, &e, FALSE);
	/* No rich supported yet, then html body is NULL */
	data->html_body = NULL;

	return data;
}

void 
modest_msg_edit_window_free_msg_data (ModestMsgEditWindow *edit_window,
				      MsgData *data)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window));

	g_free (data->from);
	g_free (data->plain_body);
	g_free (data->html_body);
	g_slice_free (MsgData, data);
}

/* Rich formatting API functions */
ModestMsgEditFormat
modest_msg_edit_window_get_format (ModestMsgEditWindow *self)
{
	return MODEST_MSG_EDIT_FORMAT_TEXT;
}

void
modest_msg_edit_window_set_format (ModestMsgEditWindow *self,
				   ModestMsgEditFormat format)
{
	switch (format) {
	case MODEST_MSG_EDIT_FORMAT_TEXT:
		break;
	case MODEST_MSG_EDIT_FORMAT_HTML:
		g_message ("HTML format not supported in Gnome ModestMsgEditWindow");
		break;
	default:
		break;
	}
}

ModestMsgEditFormatState *
modest_msg_edit_window_get_format_state (ModestMsgEditWindow *self)
{
	ModestMsgEditFormatState *format_state;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self), NULL);

	format_state = g_new0 (ModestMsgEditFormatState, 1);

	return format_state;
}

void
modest_msg_edit_window_set_format_state (ModestMsgEditWindow *self, 
					 const ModestMsgEditFormatState *format_state)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (self));

	/* Ends silently as set_format_state should do nothing when edit window format
	   is not HTML */
	return;
}

void
modest_msg_edit_window_select_color (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	g_message ("Select color operation is not supported");
}

void
modest_msg_edit_window_select_file_format (ModestMsgEditWindow *window,
					   gint file_format)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	g_message ("Select file format operation is not supported");
}

void
modest_msg_edit_window_select_font (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	g_message ("Select font operation is not supported");
}

void
modest_msg_edit_window_select_background_color (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	g_message ("Select background color operation is not supported");
}

void
modest_msg_edit_window_insert_image (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	g_message ("Insert image operation is not supported");
}

void
modest_msg_edit_window_attach_file (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	g_message ("Attach file operation is not supported");
}

void
modest_msg_edit_window_remove_attachments (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_MSG_EDIT_WINDOW (window));

	g_message ("Remove attachments operation is not supported");
}

void
modest_msg_edit_window_show_cc (ModestMsgEditWindow *window, 
				gboolean show)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	g_message ("not implemented yet %s", __FUNCTION__);
}
void
modest_msg_edit_window_show_bcc (ModestMsgEditWindow *window, 
				gboolean show)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	g_message ("not implemented yet %s", __FUNCTION__);
}
void
modest_msg_edit_window_undo (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
      
	g_message ("not implemented yet %s", __FUNCTION__);
}
void
modest_msg_edit_window_toggle_fullscreen (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
      
	g_message ("not implemented yet %s", __FUNCTION__);
}
void
modest_msg_edit_window_set_priority_flags (ModestMsgEditWindow *window,
					   TnyHeaderFlags priority_flags)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
      
	g_message ("not implemented yet %s", __FUNCTION__);
}


void
modest_msg_edit_window_select_contacts (ModestMsgEditWindow *window)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	g_message ("not implemented yet %s", __FUNCTION__);
}

gboolean
modest_msg_edit_window_check_names (ModestMsgEditWindow *window)
{
	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), FALSE);

	g_message ("not implemented yet %s", __FUNCTION__);
	return TRUE;
}

void
modest_msg_edit_window_set_file_format (ModestMsgEditWindow *window,
					gint file_format)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));

	g_message ("not implemented yet %s", __FUNCTION__);
}

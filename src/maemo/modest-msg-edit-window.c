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
#include <tny-account-store.h>

#include <gtk/gtk.h>
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

	priv->msg_body = gtk_text_view_new ();
	
	main_vbox = gtk_vbox_new  (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(main_vbox), header_table, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->msg_body, TRUE, TRUE, 6);

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


#if 0
static void
set_msg (ModestMsgEditWindow *self, TnyMsg *msg)
{
	TnyHeader *header;
	GtkTextBuffer *buf;
	const gchar *to, *cc, *bcc, *subject;
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
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(priv->msg_body));
	gtk_text_buffer_set_text (buf,
				  (const gchar *) modest_tny_msg_get_body (msg, FALSE),
				  -1);

	/* TODO: lower priority, select in the From: combo to the
	   value that comes from msg <- not sure, should it be
	   allowed? */
	
	/* TODO: set attachments */
}
#endif

	

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


	/* Toolbar */
	parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	hildon_window_add_toolbar (HILDON_WINDOW (obj), GTK_TOOLBAR (parent_priv->toolbar));

	/* should we hide the toolbar? */
	if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_TOOLBAR, NULL))
		gtk_widget_hide (parent_priv->toolbar);

	
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
	return (ModestWindow*)obj;
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
	data->body    =  gtk_text_buffer_get_text (buf, &b, &e, FALSE);

	return data;
}

void 
modest_msg_edit_window_free_msg_data (ModestMsgEditWindow *edit_window,
						      MsgData *data)
{
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (edit_window));

	g_free (data->from);
	g_free (data->body);
	g_slice_free (MsgData, data);
}

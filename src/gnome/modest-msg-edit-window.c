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
#include <widgets/modest-attachments-view.h>

#include <modest-widget-memory.h>
#include <modest-account-mgr-helpers.h>
#include <modest-tny-folder.h>
#include <gtkhtml/gtkhtml.h>
#include "modest-msg-edit-window-ui-dimming.h"
#include <modest-platform.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs.h>
#include <tny-vfs-stream.h>

static void  modest_msg_edit_window_class_init   (ModestMsgEditWindowClass *klass);
static void  modest_msg_edit_window_init         (ModestMsgEditWindow *obj);
static void  modest_msg_edit_window_finalize     (GObject *obj);

static void  modest_msg_edit_window_add_attachment_clicked (GtkButton *button,
							    ModestMsgEditWindow *window);
static void update_next_cid (ModestMsgEditWindow *self, TnyList *attachments);

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
	
	ModestPairList *from_field_protos;
	GtkWidget   *from_field;
	
	GtkWidget   *to_field;
	GtkWidget   *cc_field;
	GtkWidget   *bcc_field;
	GtkWidget   *subject_field;

	GtkWidget   *attachments_view;
	TnyList     *attachments;
	guint       next_cid;

	TnyMsg      *draft_msg;
	TnyMsg      *outbox_msg;
	gchar       *msg_uid;

	gchar       *references;
	gchar       *in_reply_to;

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
	priv->attachments_view = NULL;
	priv->attachments = TNY_LIST (tny_simple_list_new ());
	priv->sent          = FALSE;
	priv->next_cid      = 0;
	priv->draft_msg = NULL;
	priv->outbox_msg = NULL;
	priv->msg_uid = NULL;
	priv->references = NULL;
	priv->in_reply_to = NULL;
}

/** 
 * @result: A ModestPairList, which must be freed with modest_pair_list_free().
 */
static ModestPairList*
get_transports (void)
{
	ModestAccountMgr *account_mgr;
	GSList *transports = NULL;
	GSList *cursor, *accounts;
	
	account_mgr = modest_runtime_get_account_mgr();
	cursor = accounts = modest_account_mgr_account_names (account_mgr, TRUE);
	while (cursor) {
		gchar *account_name = cursor->data ? g_strdup((gchar*)cursor->data) : NULL;
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
	GtkWidget *to_button, *cc_button, *bcc_button, *add_attachment_button; 
	GtkWidget *header_table;
	GtkWidget *main_vbox;
	GtkWidget *msg_vbox;
	GtkWidget *scrolled_window;
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	to_button     = gtk_button_new_with_label (_("To..."));
	cc_button     = gtk_button_new_with_label (_("Cc..."));
	bcc_button    = gtk_button_new_with_label (_("Bcc..."));
	add_attachment_button = gtk_button_new_with_label (_("Attach..."));
	
	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	priv->from_field_protos = get_transports ();
 	priv->from_field    = modest_combo_box_new (priv->from_field_protos, g_str_equal);
	
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
	priv->attachments_view = modest_attachments_view_new (NULL);
	
	header_table = gtk_table_new (6,2, FALSE);
	
	gtk_table_attach (GTK_TABLE(header_table), gtk_label_new (_("From:")),
			  0,1,0,1, GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), to_button,     0,1,1,2, GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), cc_button,     0,1,2,3, GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), bcc_button,    0,1,3,4, GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), gtk_label_new (_("Subject:")),
			  0,1,4,5, GTK_FILL, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), add_attachment_button, 0, 1, 5, 6, GTK_FILL, 0, 0, 0);

	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->from_field,   1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->to_field,     1,2,1,2);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->cc_field,     1,2,2,3);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->bcc_field,    1,2,3,4);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->subject_field,1,2,4,5);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->attachments_view,1,2,5,6);

	priv->msg_body = gtk_html_new ();
	gtk_html_load_empty (GTK_HTML (priv->msg_body));
	gtk_html_set_editable (GTK_HTML (priv->msg_body), TRUE);
	
	main_vbox = gtk_vbox_new  (FALSE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, FALSE, 0);
	
	msg_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), msg_vbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX(msg_vbox), header_table, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (scrolled_window), priv->msg_body); 

	gtk_box_pack_start (GTK_BOX(msg_vbox), scrolled_window, TRUE, TRUE, 0);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	gtk_container_add (GTK_CONTAINER(obj), main_vbox);

	g_signal_connect_object (add_attachment_button, "clicked", 
				 G_CALLBACK (modest_msg_edit_window_add_attachment_clicked), G_OBJECT (obj), 0);
}


static void
modest_msg_edit_window_finalize (GObject *obj)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(obj);
	
	/* These had to stay alive for as long as the comboboxes that used them: */
	modest_pair_list_free (priv->from_field_protos);
	g_object_unref (priv->attachments);
	priv->attachments = NULL;

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
	g_free (priv->msg_uid);
	g_free (priv->references);
	g_free (priv->in_reply_to);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
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
	TnyFolder *msg_folder;
	gchar *to, *cc, *bcc, *subject;
	ModestMsgEditWindowPrivate *priv;
	gchar *body;
	
	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (self));
	g_return_if_fail (TNY_IS_MSG (msg));

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (self);

	header  = tny_msg_get_header (msg);
	to      = tny_header_dup_to (header);
	cc      = tny_header_dup_cc (header);
	bcc     = tny_header_dup_bcc (header);
	subject = tny_header_dup_subject (header);

	if (to)
		gtk_entry_set_text (GTK_ENTRY(priv->to_field), to);
	if (cc)
		gtk_entry_set_text (GTK_ENTRY(priv->cc_field), cc);
	if (bcc)
		gtk_entry_set_text (GTK_ENTRY(priv->bcc_field),  bcc);
	if (subject)
		gtk_entry_set_text (GTK_ENTRY(priv->subject_field), subject);

	modest_tny_msg_get_references (TNY_MSG (msg), NULL, &(priv->references), &(priv->in_reply_to));

	modest_attachments_view_set_message (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), msg);
	priv->attachments = modest_attachments_view_get_attachments (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	update_next_cid (self, priv->attachments);

	body = modest_tny_msg_get_body (msg, FALSE, NULL);
	gtk_html_set_editable (GTK_HTML (priv->msg_body), FALSE);
	if (body) {
		gtk_html_load_from_string (GTK_HTML (priv->msg_body), body, -1);
	}
	gtk_html_set_editable (GTK_HTML (priv->msg_body), TRUE);
	g_free (body);

	modest_msg_edit_window_set_modified (self, FALSE);

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

	g_free (subject);
	g_free (to);
	g_free (cc);
	g_free (bcc);
}


ModestWindow *
modest_msg_edit_window_new (TnyMsg *msg, const gchar *account,
			    gboolean preserve_is_rich)
{
	ModestMsgEditWindow *self;
	ModestMsgEditWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkActionGroup *action_group;
	ModestDimmingRulesGroup *menu_rules_group = NULL;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	ModestDimmingRulesGroup *clipboard_rules_group = NULL;
	GError *error = NULL;

	g_return_val_if_fail (msg, NULL);
	
	self = MODEST_MSG_EDIT_WINDOW(g_object_new(MODEST_TYPE_MSG_EDIT_WINDOW, NULL));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	
	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMsgEditWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

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
	
	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new ();
	menu_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);
	toolbar_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_TOOLBAR, TRUE);
	clipboard_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_CLIPBOARD, FALSE);

	/* Add common dimming rules */
	modest_dimming_rules_group_add_rules (menu_rules_group, 
					      modest_msg_edit_window_menu_dimming_entries,
					      G_N_ELEMENTS (modest_msg_edit_window_menu_dimming_entries),
					      MODEST_WINDOW (self));
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_msg_edit_window_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_msg_edit_window_toolbar_dimming_entries),
					      MODEST_WINDOW (self));
/* 	modest_dimming_rules_group_add_widget_rule (toolbar_rules_group, priv->font_color_button, */
/* 						    G_CALLBACK (modest_ui_dimming_rules_on_set_style), */
/* 						    MODEST_WINDOW (self)); */
/* 	modest_dimming_rules_group_add_widget_rule (toolbar_rules_group, priv->font_size_toolitem, */
/* 						    G_CALLBACK (modest_ui_dimming_rules_on_set_style), */
/* 						    MODEST_WINDOW (self)); */
/* 	modest_dimming_rules_group_add_widget_rule (toolbar_rules_group, priv->font_face_toolitem, */
/* 						    G_CALLBACK (modest_ui_dimming_rules_on_set_style), */
/* 						    MODEST_WINDOW (self)); */
	modest_dimming_rules_group_add_rules (clipboard_rules_group, 
					      modest_msg_edit_window_clipboard_dimming_entries,
					      G_N_ELEMENTS (modest_msg_edit_window_clipboard_dimming_entries),
					      MODEST_WINDOW (self));
	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, menu_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, clipboard_rules_group);
        /* Checks the dimming rules */
	g_object_unref (menu_rules_group);
	g_object_unref (toolbar_rules_group);
	g_object_unref (clipboard_rules_group);

	set_msg (self, msg);
	
	return MODEST_WINDOW(self);
}

static gboolean
html_export_save_buffer (gpointer engine,
			 const gchar *data,
			 size_t len,
			 GString **buffer)
{
	*buffer = g_string_append (*buffer, data);
	return TRUE;
}


MsgData * 
modest_msg_edit_window_get_msg_data (ModestMsgEditWindow *edit_window)
{
	MsgData *data;
	
	const gchar *account_name;
	gchar *from_string = NULL;
	ModestMsgEditWindowPrivate *priv;
	GString *buffer;
	TnyIterator *att_iter;
	
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
	
	
	
	data = g_slice_new0 (MsgData);
	data->from    =  from_string; /* will be freed when data is freed */
	data->to      =  g_strdup ( gtk_entry_get_text (GTK_ENTRY(priv->to_field)));
	data->cc      =  g_strdup ( gtk_entry_get_text (GTK_ENTRY(priv->cc_field)));
	data->bcc     =  g_strdup ( gtk_entry_get_text (GTK_ENTRY(priv->bcc_field)));
	data->subject =  g_strdup ( gtk_entry_get_text (GTK_ENTRY(priv->subject_field)));
	data->references = g_strdup (priv->references);
	data->in_reply_to = g_strdup (priv->in_reply_to);

/* 	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body)); */
/* 	GtkTextIter b, e; */
/* 	gtk_text_buffer_get_bounds (buf, &b, &e); */
/* 	data->plain_body =  gtk_text_buffer_get_text (buf, &b, &e, FALSE); /\* Returns a copy. *\/ */

	buffer = g_string_new ("");
	gtk_html_export (GTK_HTML (priv->msg_body), "text/html", (GtkHTMLSaveReceiverFn) html_export_save_buffer, &buffer);
	data->html_body = g_string_free (buffer, FALSE);
	buffer = g_string_new ("");
	gtk_html_export (GTK_HTML (priv->msg_body), "text/plain", (GtkHTMLSaveReceiverFn) html_export_save_buffer, &buffer);
	data->plain_body = g_string_free (buffer, FALSE);

	/* deep-copy the attachment data */
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


	if (priv->draft_msg) {
		data->draft_msg = g_object_ref (priv->draft_msg);
	} else if (priv->outbox_msg) {
		data->draft_msg = g_object_ref (priv->outbox_msg);
	} else {
		data->draft_msg = NULL;
	}
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
	g_free (data->references);
	g_free (data->in_reply_to);

	if (data->draft_msg != NULL) {
		g_object_unref (data->draft_msg);
		data->draft_msg = NULL;
	}	
	
	g_list_foreach (data->attachments, (GFunc)unref_gobject,  NULL);
	g_list_free (data->attachments);

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
modest_msg_edit_window_remove_attachments (ModestMsgEditWindow *window,
					   TnyList *att_list)
{
	ModestMsgEditWindowPrivate *priv;
	TnyIterator *iter;

	g_return_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window));
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	if (att_list == NULL) {
		att_list = modest_attachments_view_get_selection (MODEST_ATTACHMENTS_VIEW (priv->attachments_view));
	} else {
		g_object_ref (att_list);
	}

	if (tny_list_get_length (att_list) == 0) {
		modest_platform_information_banner (NULL, NULL, _("TODO: no attachments selected to remove"));
	} else {
		GtkWidget *confirmation_dialog = NULL;
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
		message = g_strdup_printf (ngettext("emev_nc_delete_attachment", "emev_nc_delete_attachments",
						    (tny_list_get_length (att_list) == 1)), filename);
		g_free (filename);
		confirmation_dialog = gtk_message_dialog_new (GTK_WINDOW (window), 
							      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
							      GTK_MESSAGE_QUESTION,
							      GTK_BUTTONS_OK_CANCEL,
							      message);
		g_free (message);
		dialog_response = (gtk_dialog_run (GTK_DIALOG (confirmation_dialog))==GTK_RESPONSE_OK);
		gtk_widget_destroy (confirmation_dialog);
		if (!dialog_response) {
			g_object_unref (att_list);
			return;
		}
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_removing_attachment"));
		
		for (iter = tny_list_create_iterator (att_list);
		     !tny_iterator_is_done (iter);
		     tny_iterator_next (iter)) {
			TnyMimePart *mime_part = (TnyMimePart *) tny_iterator_get_current (iter);
			tny_list_remove (priv->attachments, (GObject *) mime_part);

			modest_attachments_view_remove_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view),
								   mime_part);
			g_object_unref (mime_part);
		}
		g_object_unref (iter);
	}

	g_object_unref (att_list);

}

static void
modest_msg_edit_window_add_attachment_clicked (GtkButton *button,
					       ModestMsgEditWindow *window)
{
	modest_msg_edit_window_offer_attach_file (window);
}

void 
modest_msg_edit_window_get_parts_size (ModestMsgEditWindow *window,
				       gint *parts_count,
				       guint64 *parts_size)
{
	ModestMsgEditWindowPrivate *priv;

	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	modest_attachments_view_get_sizes (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), parts_count, parts_size);

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
modest_msg_edit_window_check_names (ModestMsgEditWindow *window, gboolean add_to_addressbook)
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
	return NULL;
}

/* FUNCTIONS NOT IMPLEMENTED YET */

void            
modest_msg_edit_window_set_modified      (ModestMsgEditWindow *window,
					  gboolean modified)
{
	g_message (__FUNCTION__);
}

void            
modest_msg_edit_window_toggle_find_toolbar (ModestMsgEditWindow *window,
					    gboolean show)
{
	g_message ("NOT IMPLEMENTED %s", __FUNCTION__);
}

void                    
modest_msg_edit_window_add_part (ModestMsgEditWindow *window,
				 TnyMimePart *part)
{
	ModestMsgEditWindowPrivate *priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	g_return_if_fail (TNY_IS_MIME_PART (part));
	tny_list_prepend (priv->attachments, (GObject *) part);
	modest_attachments_view_add_attachment (MODEST_ATTACHMENTS_VIEW (priv->attachments_view), part, TRUE, 0);
}

void            
modest_msg_edit_window_redo               (ModestMsgEditWindow *window)
{
	g_message ("NOT IMPLEMENTED %s", __FUNCTION__);
}

void                    
modest_msg_edit_window_offer_attach_file           (ModestMsgEditWindow *window)
{
	GtkWidget *dialog = NULL;
	gint response = 0;
	GSList *uris = NULL;
	GSList *uri_node;
	
	dialog = gtk_file_chooser_dialog_new (_("mcen_ti_select_attachment_title"), 
					      GTK_WINDOW (window), 
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_OPEN,
					      GTK_RESPONSE_OK,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      NULL);

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
		modest_msg_edit_window_attach_file_one (window, uri, MODEST_MAX_ATTACHMENT_SIZE);
	}
	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);
}

GnomeVFSFileSize
modest_msg_edit_window_attach_file_one (ModestMsgEditWindow *window, 
					const gchar *uri,
					GnomeVFSFileSize allowed_size)
{
	GnomeVFSHandle *handle = NULL;
	ModestMsgEditWindowPrivate *priv;
	GnomeVFSResult result;

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
		stream = TNY_STREAM (tny_vfs_stream_new (handle));
		
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
		g_free (filename);
		g_object_unref (mime_part);
		gnome_vfs_file_info_unref (info);
	}
	/* TODO: return proper file size */
	return 0;
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
		if (GTK_WIDGET_REALIZED (window))
			modest_window_mgr_register_window (mgr, MODEST_WINDOW (window), NULL);
	}

	priv->draft_msg = draft;
}

gboolean        
modest_msg_edit_window_is_modified         (ModestMsgEditWindow *window)
{
	g_message ("NOT IMPLEMENTED %s", __FUNCTION__);
	return TRUE;
}

const gchar *
modest_msg_edit_window_get_clipboard_text (ModestMsgEditWindow *win)
{
	g_message ("NOT IMPLEMENTED %s", __FUNCTION__);
	return NULL;
}

gboolean            
modest_msg_edit_window_can_redo               (ModestMsgEditWindow *window)
{
	g_message ("NOT IMPLEMENTED %s", __FUNCTION__);
	return FALSE;
}
gboolean            
modest_msg_edit_window_can_undo               (ModestMsgEditWindow *window)
{
	g_message ("NOT IMPLEMENTED %s", __FUNCTION__);
	return FALSE;
}

const gchar*    
modest_msg_edit_window_get_message_uid (ModestMsgEditWindow *window)
{
	ModestMsgEditWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_EDIT_WINDOW (window), NULL);	
	priv = MODEST_MSG_EDIT_WINDOW_GET_PRIVATE (window);

	return priv->msg_uid;
}

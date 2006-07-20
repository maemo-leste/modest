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
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

/* TODO: put in auto* */
#include <tny-text-buffer-stream.h>
#include <tny-msg-folder.h>
#include <tny-list.h>


#include "../modest-ui.h"
#include "../modest-window-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-identity-mgr.h"

#include "../modest-tny-account-store.h"
#include "../modest-tny-folder-tree-view.h"
#include "../modest-tny-header-tree-view.h"
#include "../modest-tny-msg-view.h"
#include "../modest-tny-transport-actions.h"
#include "../modest-tny-store-actions.h"

#include "../modest-text-utils.h"
#include "../modest-tny-msg-actions.h"

#include "../modest-tny-attachment.h"

#include "../modest-editor-window.h"

#include "modest-ui-glade.h"
#include "modest-ui-wizard.h"

#include "modest-ui-message-editor.h"


static void on_attach_button_clicked (GtkWidget *widget, ModestEditorWindow *modest_editwin);

static void on_send_button_clicked (GtkWidget *widget, ModestEditorWindow *modest_editwin);


typedef struct {
	ModestUI *modest_ui;
	ModestEditorWindow *edit_win;
	GladeXML *glade_xml;
	GList *attachments;
} EditWinData;


static gboolean close_edit_confirm_dialog(ModestEditorWindow *edit_win)
{
	GtkWidget *mdialog;
	gint res;

	mdialog = gtk_message_dialog_new(GTK_WINDOW(edit_win),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			_("Message was modified.\nDiscard Changes?"));
	gtk_widget_show_all (mdialog);

	res=gtk_dialog_run(GTK_DIALOG(mdialog));
	gtk_widget_destroy (mdialog);
	if (res == GTK_RESPONSE_YES)
		return TRUE;
	else
		return FALSE;
}

static void
close_edit_window (GtkWidget *win, GdkEvent *event, gpointer data)
{
	ModestEditorWindow *edit_win;
	ModestUIPrivate *priv;
	EditWinData *win_data;

	edit_win = (ModestEditorWindow *)data;
	win_data = modest_editor_window_get_data(edit_win);
	priv = MODEST_UI_GET_PRIVATE(win_data->modest_ui);

	// g_message("window was %s modified", modest_editor_window_get_modified(edit_win) ? "" : "not");

	if (modest_editor_window_get_modified(edit_win)) {
		if (close_edit_confirm_dialog(edit_win)) {
			gtk_widget_hide (GTK_WIDGET(edit_win));
			modest_window_mgr_unregister(priv->modest_window_mgr, G_OBJECT(edit_win));
			gtk_widget_destroy(GTK_WIDGET(edit_win));
			g_message("closing window");
		} else {
			g_message("not closing window");
		}
	} else {
		gtk_widget_hide (GTK_WIDGET(edit_win));
		modest_window_mgr_unregister(priv->modest_window_mgr, G_OBJECT(edit_win));
		gtk_widget_destroy(GTK_WIDGET(edit_win));
		g_message("closing window");
	}
}


GtkContainer
*modest_ui_new_editor_window (ModestUI *modest_ui, gpointer *user_data)
{
	GtkWidget       *top_container;
	GladeXML		*glade_xml;
	EditWinData		*win_data;

	glade_xml = glade_xml_new(MODEST_GLADE, "new_mail_top_container", NULL);
	if (!glade_xml)
		return NULL;

	win_data = g_malloc(sizeof(EditWinData));
	win_data->modest_ui = modest_ui;
	win_data->glade_xml = glade_xml;
	win_data->attachments = NULL;

	*user_data = win_data;

	top_container = glade_xml_get_widget(glade_xml, "new_mail_top_container");
	if (!top_container) {
		g_object_unref(G_OBJECT(glade_xml));
		return NULL;
	}
	
	return GTK_CONTAINER(top_container);
}


gboolean
modest_ui_editor_window_set_to_header(const gchar *to, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;
	w = glade_xml_get_widget(glade_xml, "to_entry");
	gtk_entry_set_text(GTK_ENTRY(w), to);

	return TRUE;
}


gboolean
modest_ui_editor_window_set_cc_header(const gchar *cc, gpointer window_data)
{
	GladeXML *glade_xml;
	// GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;
/*
	w = glade_xml_get_widget(glade_xml, "cc_entry");
	gtk_entry_set_text(GTK_ENTRY(w), cc);
*/
	return TRUE;
}


gboolean
modest_ui_editor_window_set_bcc_header(const gchar *bcc, gpointer window_data)
{
	GladeXML *glade_xml;
	// GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;
/*
	w = glade_xml_get_widget(glade_xml, "bcc_entry");
	gtk_entry_set_text(GTK_ENTRY(w), bcc);
*/
	return TRUE;
}


gboolean
modest_ui_editor_window_set_subject_header(const gchar *subject, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;

	w = glade_xml_get_widget(glade_xml, "subject_entry");
	gtk_entry_set_text(GTK_ENTRY(w), subject);

	return TRUE;
}


gboolean
modest_ui_editor_window_set_body(const gchar *body, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *body_view;
	GtkTextBuffer *buf;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;

	body_view = glade_xml_get_widget(glade_xml, "body_view");
	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(body_view));

	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buf), body, -1);

	return TRUE;
}


gboolean
modest_ui_editor_window_update_attachments(gpointer window_data)
{
	GladeXML *glade_xml;
	
	glade_xml = ((EditWinData *) window_data)->glade_xml;

	//body_view = glade_xml_get_widget(glade_xml, "body_view");
	
	return TRUE;
}


static
void on_editor_entry_changed(GtkEditable *editable,
                                            gpointer     user_data)
{
	GtkWidget *edit_win;
	EditWinData *windata;

	edit_win = (GtkWidget *)user_data;
	windata = (EditWinData *)modest_editor_window_get_data(MODEST_EDITOR_WINDOW(edit_win));

	modest_editor_window_set_modified(MODEST_EDITOR_WINDOW(edit_win), TRUE);
}


static
void on_editor_buffer_changed (GtkTextBuffer *textbuffer,
                                            gpointer       user_data)
{
	GtkWidget *edit_win;
	
	edit_win = (GtkWidget *)user_data;
	modest_editor_window_set_modified(MODEST_EDITOR_WINDOW(edit_win), TRUE);
}


static void
new_editor_with_presets (ModestUI *modest_ui, const gchar *to_header,
							const gchar *cc_header, const gchar *bcc_header,
							const gchar *subject_header, const gchar *body,
                            const GList *attachments)
{
	GtkWidget *edit_win;
	GladeXML *glade_xml;
	GtkWidget *btn, *w;
	GtkTextBuffer *buf;
	EditWinData *windata;
	ModestUIPrivate *priv;
	gint height, width;

	g_return_if_fail (modest_ui);

	edit_win = GTK_WIDGET(modest_editor_window_new(modest_ui));
	windata = (EditWinData *)modest_editor_window_get_data(MODEST_EDITOR_WINDOW(edit_win));
	g_return_if_fail(windata);

	glade_xml = windata->glade_xml;
	btn = glade_xml_get_widget (glade_xml, "toolb_send");
	g_signal_connect (btn, "clicked", G_CALLBACK(on_send_button_clicked),
			  edit_win);
	btn = glade_xml_get_widget (glade_xml, "toolb_attach");
	g_signal_connect (btn, "clicked", G_CALLBACK(on_attach_button_clicked),
			  edit_win);

	w = glade_xml_get_widget (glade_xml, "to_entry");
	g_signal_connect(w, "changed", G_CALLBACK(on_editor_entry_changed), edit_win);
	w = glade_xml_get_widget (glade_xml, "subject_entry");
	g_signal_connect(w, "changed", G_CALLBACK(on_editor_entry_changed), edit_win);
	w = glade_xml_get_widget (glade_xml, "body_view");
	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
	g_signal_connect(buf, "changed", G_CALLBACK(on_editor_buffer_changed), edit_win);

	g_signal_connect (edit_win, "destroy-event", G_CALLBACK(close_edit_window),
			  edit_win);
	g_signal_connect (edit_win, "delete-event", G_CALLBACK(close_edit_window),
			  edit_win);

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	height = modest_conf_get_int (priv->modest_conf,
					  MODEST_EDIT_WINDOW_HEIGHT, NULL);
	width  = modest_conf_get_int (priv->modest_conf,
					  MODEST_EDIT_WINDOW_WIDTH, NULL);

	// g_message("new editor win@%dx%d", width, height);

	gtk_widget_set_usize (GTK_WIDGET(edit_win), width, height);
	if (strlen(subject_header) > 0)
		gtk_window_set_title (GTK_WINDOW(edit_win), subject_header);
	else
		gtk_window_set_title (GTK_WINDOW(edit_win), _("Untitled"));

	modest_window_mgr_register(priv->modest_window_mgr, G_OBJECT(edit_win), MODEST_EDIT_WINDOW, 0);

	modest_editor_window_set_to_header(MODEST_EDITOR_WINDOW(edit_win), to_header);
	modest_editor_window_set_cc_header(MODEST_EDITOR_WINDOW(edit_win), cc_header);
	modest_editor_window_set_bcc_header(MODEST_EDITOR_WINDOW(edit_win), bcc_header);
	modest_editor_window_set_subject_header(MODEST_EDITOR_WINDOW(edit_win), subject_header);
	modest_editor_window_set_body(MODEST_EDITOR_WINDOW(edit_win), body);
	modest_editor_window_set_attachments(MODEST_EDITOR_WINDOW(edit_win), attachments);

	modest_editor_window_set_modified(MODEST_EDITOR_WINDOW(edit_win), FALSE);

	gtk_widget_show(edit_win);
}


void
on_new_mail_clicked (GtkWidget *widget, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *) user_data;

	new_editor_with_presets(modest_ui, "", "", "", "", "", NULL);
}

void
ui_on_mailto_clicked (GtkWidget *widget, const gchar *uri, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *) user_data;
	
	new_editor_with_presets(modest_ui, uri, "", "", "", "", NULL);
}


void
quoted_send_msg (ModestUI *modest_ui, quoted_send_type qstype)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;

	TnyMsgHeaderIface *header;

	ModestTnyHeaderTreeView *header_view;
	ModestTnyMsgView *msg_view;
	ModestUIPrivate *priv;

	const TnyMsgIface *msg;
	const TnyMsgFolderIface *folder;
	GString *re_sub;
	const gchar *subject, *from;
	gchar *unquoted, *quoted;
	time_t sent_date;
	gint line_limit = 76;
	
	GList *attachments = NULL;

	g_return_if_fail (modest_ui);

	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	msg_view = MODEST_TNY_MSG_VIEW(priv->message_view);
	g_return_if_fail (msg_view);

	header_view = MODEST_TNY_HEADER_TREE_VIEW(priv->header_view);
	g_return_if_fail (header_view);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(header_view));
	g_return_if_fail (sel);

	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		g_warning("nothing to reply to");
		return;
	}

	gtk_tree_model_get (model, &iter,
			    TNY_MSG_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);

	if (!header) {
		g_warning("no header");
		return;
	}

	folder = tny_msg_header_iface_get_folder (TNY_MSG_HEADER_IFACE(header));
	if (!folder) {
		g_warning ("cannot find folder");
		return;
	}

	msg = tny_msg_folder_iface_get_message (TNY_MSG_FOLDER_IFACE(folder), header);
	if (!msg) {
		g_warning ("cannot find msg");
		return;
	}

	subject = tny_msg_header_iface_get_subject(header);
	re_sub = g_string_new(subject);
	/* FIXME: honor replyto, cc */
	from = tny_msg_header_iface_get_from(header);
	sent_date = tny_msg_header_iface_get_date_sent(header);

	unquoted = modest_tny_msg_view_get_selected_text(msg_view);
	quoted = modest_tny_msg_actions_quote(msg, from, sent_date, line_limit, unquoted);

	switch (qstype) {
		case QUOTED_SEND_REPLY:
			g_string_prepend(re_sub, _("Re: "));
			new_editor_with_presets(modest_ui, from, /* cc */ "", /* bcc */ "", re_sub->str, quoted, attachments);
			break;
		case QUOTED_SEND_FORWARD:
			attachments = modest_tny_attachment_new_list_from_msg(msg, FALSE);
			g_string_prepend(re_sub, _("Fwd: "));
			new_editor_with_presets(modest_ui, /* from */ "", /* cc */ "", /* bcc */ "", re_sub->str, quoted, attachments);
			break;
		case QUOTED_SEND_FORWARD_ATTACHED:
			attachments = modest_tny_attachment_new_list_from_msg(msg, TRUE);
			g_string_prepend(re_sub, _("Fwd: "));
			new_editor_with_presets(modest_ui, /* from */ "", /* cc */ "", /* bcc */ "", re_sub->str, "", attachments);
			break;
		default:
			break;
	}
	g_free(quoted);
	g_free(unquoted);
	g_string_free(re_sub, TRUE);
}


static void
on_attach_button_clicked (GtkWidget *widget, ModestEditorWindow *modest_editwin)
{
	/* open file selector */
	GtkWidget *dialog;
	ModestTnyAttachment *attachment;
	gchar *filename = NULL;
	
	dialog = gtk_file_chooser_dialog_new ("Open File",
										  GTK_WINDOW(modest_editwin),
										  GTK_FILE_CHOOSER_ACTION_OPEN,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
										  NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		printf ("file:%s\n", filename);
	}
	gtk_widget_destroy (dialog);
      
	/* check file */
	if (!filename)
		return;
	
	attachment = modest_tny_attachment_new();
	modest_tny_attachment_set_filename(attachment, filename);
	modest_tny_attachment_guess_mime_type(attachment);
	
	modest_editor_window_attach_file(modest_editwin, attachment);
	
	g_free (filename);
}


static void
on_send_button_clicked (GtkWidget *widget, ModestEditorWindow *modest_editwin)
{
	ModestTnyTransportActions *actions;
	ModestUI *modest_ui;
	ModestUIPrivate *priv;
	GtkWidget *to_entry, *subject_entry, *body_view;
	const gchar *to, *subject, *email_from;
	gchar *body;
	GtkTextIter start, end;
	GtkTextBuffer *buf;
	TnyTransportAccountIface *transport_account;
	ModestIdentityMgr *id_mgr;
	EditWinData *win_data;
	GList * attachments;

	TnyListIface *transport_accounts;
	TnyIteratorIface *iter;
	
	win_data = modest_editor_window_get_data(modest_editwin);
	modest_ui = win_data->modest_ui;

	g_return_if_fail (modest_ui);

	actions = MODEST_TNY_TRANSPORT_ACTIONS
		(modest_tny_transport_actions_new ());

	priv = MODEST_UI_GET_PRIVATE(modest_ui);


	to_entry      = glade_xml_get_widget (win_data->glade_xml, "to_entry");
	subject_entry = glade_xml_get_widget (win_data->glade_xml, "subject_entry");
	body_view     = glade_xml_get_widget (win_data->glade_xml, "body_view");

	to      = gtk_entry_get_text (GTK_ENTRY(to_entry));
	subject = gtk_entry_get_text (GTK_ENTRY(subject_entry));

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(body_view));
	gtk_text_buffer_get_bounds (buf, &start, &end);
	body    = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

	id_mgr = priv->modest_id_mgr;
	email_from = modest_identity_mgr_get_identity_string(id_mgr,
							     MODEST_IDENTITY_DEFAULT_IDENTITY,
							     MODEST_IDENTITY_EMAIL, NULL);
	attachments = modest_editor_window_get_attachments(modest_editwin);
	if (!email_from)
		email_from = "";
	
	g_message("sending \"%s\" %s ==> %s", subject, email_from, to);

	transport_accounts = TNY_LIST_IFACE(tny_list_new ());
	tny_account_store_iface_get_accounts (priv->account_store,
					      transport_accounts,
					      TNY_ACCOUNT_STORE_IFACE_TRANSPORT_ACCOUNTS);
	
	iter = tny_list_iface_create_iterator (transport_accounts);
		
	if (!transport_accounts || !tny_iterator_iface_has_first(iter)) {
		g_printerr ("modest: cannot send message: no transport account defined");
		return;
	} else { /* take the first one! */
		tny_iterator_iface_first (iter);
		transport_account = 
			TNY_TRANSPORT_ACCOUNT_IFACE(tny_iterator_iface_current(iter));
	}
	
	modest_tny_transport_actions_send_message (actions,
						   transport_account,
						   email_from,
						   to, "", "", subject,
						   body,
						   attachments);
	g_object_unref (G_OBJECT(iter));
	g_object_unref (G_OBJECT(transport_accounts));
	
	modest_editor_window_set_attachments(modest_editwin, NULL); /* This unrefs them, too. */
	g_free (body);
	g_object_unref (G_OBJECT(actions));

	gtk_widget_hide (GTK_WIDGET(modest_editwin));
	modest_window_mgr_unregister(priv->modest_window_mgr, G_OBJECT(modest_editwin));
	if (GTK_IS_WIDGET(modest_editwin)) {
		gtk_widget_destroy(GTK_WIDGET(modest_editwin));
	} else
		g_warning("editor window has vanished!");
}

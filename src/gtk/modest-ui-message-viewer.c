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

#include "../modest-ui.h"
#include "../modest-window-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-identity-mgr.h"

#include "../modest-tny-account-store.h"
#include "../widgets/modest-folder-view.h"
#include "../widgets/modest-header-view.h"
#include "../widgets/modest-msg-view.h"
#include "../modest-tny-transport-actions.h"
#include "../modest-tny-store-actions.h"

#include "../modest-text-utils.h"
#include "../modest-tny-msg-actions.h"

#include "../modest-viewer-window.h"

#include "modest-ui-glade.h"
#include "modest-ui-message-viewer.h"



typedef struct {
	ModestUI *modest_ui;
	ModestViewerWindow *viewer_win;
	GladeXML *glade_xml;
} ViewerWinData;


GtkContainer
*modest_ui_new_viewer_window (ModestUI *modest_ui, GtkWidget *msg_view, TnyMsgIface *msg, gpointer *user_data)
{
	GtkWidget	*top_container;
	GladeXML	*glade_xml;
	ViewerWinData	*win_data;
	GtkWidget	*paned;

	glade_xml = glade_xml_new(MODEST_GLADE, "viewer_top_container", NULL);
	if (!glade_xml)
		return NULL;

	win_data = g_malloc(sizeof(ViewerWinData));
	win_data->modest_ui = modest_ui;
	win_data->glade_xml = glade_xml;

	*user_data = win_data;

	top_container = glade_xml_get_widget(glade_xml, "viewer_top_container");
	if (!top_container) {
		g_object_unref(G_OBJECT(glade_xml));
		return NULL;
	}

	if (!GTK_IS_WIDGET(msg_view))
		return NULL;
	paned = glade_xml_get_widget(glade_xml, "vpaned3");
	gtk_paned_add2(GTK_PANED(paned), msg_view);
	gtk_widget_show_all(msg_view);

	return GTK_CONTAINER(top_container);
}


static void
close_viewer_window(GtkWidget *win, GdkEvent *event, gpointer data)
{
	ModestViewerWindow *viewer_win;
	ModestUIPrivate *priv;
	ViewerWinData *win_data;

	viewer_win = (ModestViewerWindow *)data;
	win_data = modest_viewer_window_get_data(viewer_win);
	priv = MODEST_UI_GET_PRIVATE(win_data->modest_ui);

	modest_window_mgr_unregister(priv->modest_window_mgr, G_OBJECT(viewer_win));
	gtk_widget_hide (GTK_WIDGET(viewer_win));
	gtk_widget_destroy(GTK_WIDGET(viewer_win));
}


static void
open_message_viewer_window(ModestUI *modest_ui)
{
	GtkWidget *viewer_win;
	ModestUIPrivate *priv;
	gint width, height;
	ViewerWinData *windata;
	GtkWidget *paned;
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkScrolledWindow *scroll;
	ModestHeaderView *header_view;
	TnyMsgHeaderIface *header;
	const TnyMsgFolderIface *folder;
	TnyMsgIface *msg;
	const gchar *subject, *to, *from;
	time_t sent_date;
	gchar date_str[101];
	GtkWidget *w;

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	/* FIXME: maybe use seperate viewer defaults? */
	height = modest_conf_get_int (priv->modest_conf, MODEST_EDIT_WINDOW_HEIGHT, NULL);
	width  = modest_conf_get_int (priv->modest_conf, MODEST_EDIT_WINDOW_WIDTH, NULL);

	paned = glade_xml_get_widget (priv->glade_xml,"mail_paned");
	g_return_if_fail (paned);

	scroll = GTK_SCROLLED_WINDOW(gtk_paned_get_child1 (GTK_PANED(paned)));
        g_return_if_fail (scroll);

	header_view = MODEST_HEADER_VIEW(gtk_bin_get_child (GTK_BIN(scroll)));
	g_return_if_fail (header_view);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(header_view));
	g_return_if_fail (sel);

	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		g_warning("nothing to display");
		return;
	}

	gtk_tree_model_get (model, &iter,
		TNY_MSG_HEADER_LIST_MODEL_INSTANCE_COLUMN, &header, -1);

	if (!header) {
		g_warning("no header");
		return;
	}

	folder = tny_msg_header_iface_get_folder (TNY_MSG_HEADER_IFACE(header));
	if (!folder) {
		g_warning ("cannot find folder");
		return;
	}

	msg = (TnyMsgIface *) tny_msg_folder_iface_get_message (TNY_MSG_FOLDER_IFACE(folder), header);
	if (!msg) {
		g_warning ("cannot find msg");
		return;
	}

	viewer_win = modest_viewer_window_new(modest_ui, msg);
	windata = (ViewerWinData *)modest_viewer_window_get_data(MODEST_VIEWER_WINDOW(viewer_win));
	g_return_if_fail(windata);

	subject = tny_msg_header_iface_get_subject(header);
	from = tny_msg_header_iface_get_from(header);
	to = tny_msg_header_iface_get_to(header);
	sent_date = tny_msg_header_iface_get_date_sent(header);
	strftime (date_str, 100, "%c", localtime (&sent_date));

	w = glade_xml_get_widget (windata->glade_xml, "from");
	gtk_label_set_text(GTK_LABEL(w), from);
	w = glade_xml_get_widget (windata->glade_xml, "to");
	gtk_label_set_text(GTK_LABEL(w), to);
	w = glade_xml_get_widget (windata->glade_xml, "subject");
	gtk_label_set_text(GTK_LABEL(w), subject);
	w = glade_xml_get_widget (windata->glade_xml, "date");
	gtk_label_set_text(GTK_LABEL(w), date_str);

	// g_message("new viewer win@%dx%d", width, height);
	gtk_widget_set_usize (GTK_WIDGET(viewer_win), width, height);
	gtk_widget_show(viewer_win);
	modest_window_mgr_register(priv->modest_window_mgr, G_OBJECT(viewer_win), MODEST_VIEW_WINDOW, 0);
	g_signal_connect (viewer_win, "destroy-event", G_CALLBACK(close_viewer_window), viewer_win);
	g_signal_connect (viewer_win, "delete-event", G_CALLBACK(close_viewer_window), viewer_win);
}


void
on_open_message_clicked (GtkWidget *widget, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *)user_data;

	open_message_viewer_window(modest_ui);
}

void
on_message_activated (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	ModestUI *modest_ui = (ModestUI *)user_data;

	open_message_viewer_window(modest_ui);
}

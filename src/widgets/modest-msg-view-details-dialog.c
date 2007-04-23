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

#include "modest-msg-view-details-dialog.h"

#include <glib/gi18n.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktable.h>
#include <gtk/gtkstock.h>
#include <gtk/gtklabel.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-header-view.h>
#include <tny-folder.h>
#include <modest-tny-folder.h>
#include <modest-text-utils.h>

static void modest_msg_view_details_dialog_add_row (ModestMsgViewDetailsDialog *self, const gchar *label, const gchar *value);
static void modest_msg_view_details_dialog_set_header (TnyHeaderView *self, TnyHeader *header);
static void modest_msg_view_details_dialog_clear (TnyHeaderView *self);
static void modest_msg_view_details_dialog_tny_header_view_init (gpointer g_iface, gpointer iface_data);
static TnyFolderType modest_msg_view_details_dialog_get_folder_type (ModestMsgViewDetailsDialog *window);

G_DEFINE_TYPE_WITH_CODE (ModestMsgViewDetailsDialog, modest_msg_view_details_dialog, GTK_TYPE_DIALOG, G_IMPLEMENT_INTERFACE ( TNY_TYPE_HEADER_VIEW, modest_msg_view_details_dialog_tny_header_view_init));

#define MODEST_MSG_VIEW_DETAILS_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_MSG_VIEW_DETAILS_DIALOG, ModestMsgViewDetailsDialogPrivate))


typedef struct _ModestMsgViewDetailsDialogPrivate ModestMsgViewDetailsDialogPrivate;

struct _ModestMsgViewDetailsDialogPrivate
{
	TnyHeader *header;
	GtkWidget *props_table;
};

static void
modest_msg_view_details_dialog_get_property (GObject *object, guint property_id,
					     GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_msg_view_details_dialog_set_property (GObject *object, guint property_id,
					     const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_msg_view_details_dialog_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_msg_view_details_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (modest_msg_view_details_dialog_parent_class)->dispose (object);
}

static void
modest_msg_view_details_dialog_finalize (GObject *object)
{
	ModestMsgViewDetailsDialogPrivate *priv = MODEST_MSG_VIEW_DETAILS_DIALOG_GET_PRIVATE (object);

	if (priv->header) {
		g_object_unref (priv->header);
		priv->header = NULL;
	}
	G_OBJECT_CLASS (modest_msg_view_details_dialog_parent_class)->finalize (object);
}

static void
modest_msg_view_details_dialog_class_init (ModestMsgViewDetailsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestMsgViewDetailsDialogPrivate));

	object_class->get_property = modest_msg_view_details_dialog_get_property;
	object_class->set_property = modest_msg_view_details_dialog_set_property;
	object_class->dispose = modest_msg_view_details_dialog_dispose;
	object_class->finalize = modest_msg_view_details_dialog_finalize;
}

static void
modest_msg_view_details_dialog_init (ModestMsgViewDetailsDialog *self)
{
	ModestMsgViewDetailsDialogPrivate *priv = MODEST_MSG_VIEW_DETAILS_DIALOG_GET_PRIVATE (self);
	GtkWidget *scrollbar;

	priv->header = NULL;
	scrollbar = gtk_scrolled_window_new (NULL, NULL);

	gtk_window_set_default_size (GTK_WINDOW (self), 400, 220);
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_message_properties"));

	priv->props_table = gtk_table_new (0, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (priv->props_table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (priv->props_table), 1);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollbar), priv->props_table);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollbar), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), scrollbar);

	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_dialog_add_button (GTK_DIALOG (self), _("mcen_bd_close"), GTK_RESPONSE_CLOSE);
}

static void
modest_msg_view_details_dialog_tny_header_view_init (gpointer g_iface, gpointer iface_data)
{
	TnyHeaderViewIface *iface = (TnyHeaderViewIface *) g_iface;

	iface->set_header_func = modest_msg_view_details_dialog_set_header;
	iface->clear_func = modest_msg_view_details_dialog_clear;
}

GtkWidget*
modest_msg_view_details_dialog_new (GtkWindow *parent, TnyHeader *header)
{
	GtkWidget *dialog;
	dialog = GTK_WIDGET (g_object_new (MODEST_TYPE_MSG_VIEW_DETAILS_DIALOG, "transient-for", parent, NULL));

	if (header != NULL)
		modest_msg_view_details_dialog_set_header (TNY_HEADER_VIEW (dialog), header);

	return dialog;
}

static void
modest_msg_view_details_dialog_add_row (ModestMsgViewDetailsDialog *self,
					const gchar *label,
					const gchar *value)
{
	ModestMsgViewDetailsDialogPrivate *priv;
	guint n_rows = 0;
	GtkWidget *label_w, *value_w;

	priv = MODEST_MSG_VIEW_DETAILS_DIALOG_GET_PRIVATE (self);

	g_object_get (G_OBJECT (priv->props_table), "n-rows", &n_rows,NULL);
	label_w = gtk_label_new (label);
	gtk_misc_set_alignment (GTK_MISC (label_w), 1.0, 0.0);
	gtk_label_set_justify (GTK_LABEL (label_w), GTK_JUSTIFY_RIGHT);
	value_w = gtk_label_new (value);
	gtk_label_set_line_wrap (GTK_LABEL (value_w), TRUE);
	gtk_misc_set_alignment (GTK_MISC (value_w), 0.0, 0.0);
	gtk_label_set_justify (GTK_LABEL (value_w), GTK_JUSTIFY_LEFT);

	gtk_table_attach (GTK_TABLE (priv->props_table), label_w, 0, 1, n_rows, n_rows + 1, GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);
	gtk_table_attach (GTK_TABLE (priv->props_table), value_w, 1, 2, n_rows, n_rows + 1, GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);

}

static TnyFolderType
modest_msg_view_details_dialog_get_folder_type (ModestMsgViewDetailsDialog *window)
{
	ModestMsgViewDetailsDialogPrivate *priv;
	TnyFolderType folder_type;

	priv = MODEST_MSG_VIEW_DETAILS_DIALOG_GET_PRIVATE (window);

	folder_type = TNY_FOLDER_TYPE_UNKNOWN;

	if (priv->header) {
		TnyFolder *folder;

		folder = tny_header_get_folder (priv->header);
		
		if (folder) {
			folder_type = tny_folder_get_folder_type (folder);
			
			if (folder_type == TNY_FOLDER_TYPE_NORMAL || folder_type == TNY_FOLDER_TYPE_UNKNOWN) {
				const gchar *fname = tny_folder_get_name (folder);
				folder_type = modest_tny_folder_guess_folder_type_from_name (fname);
			}

			g_object_unref (folder);
		}
	}

	return folder_type;
}


static void
modest_msg_view_details_dialog_set_header (TnyHeaderView *self, 
					   TnyHeader *header)
{
	ModestMsgViewDetailsDialogPrivate *priv;
	gchar *from, *subject, *to, *cc;
	time_t received, sent;
 	guint size;
	gchar *size_s;

	g_return_if_fail (MODEST_IS_MSG_VIEW_DETAILS_DIALOG (self));
	priv = MODEST_MSG_VIEW_DETAILS_DIALOG_GET_PRIVATE (self);

	modest_msg_view_details_dialog_clear (self);

	priv->header = header;

	if (header != NULL) {
		TnyFolderType folder_type;
		g_object_ref (header);

		folder_type = modest_msg_view_details_dialog_get_folder_type (MODEST_MSG_VIEW_DETAILS_DIALOG (self));

		from = g_strdup (tny_header_get_from (header));
		to = g_strdup (tny_header_get_to (header));
		subject = g_strdup (tny_header_get_subject (header));
		cc = g_strdup (tny_header_get_cc (header));
		received = tny_header_get_date_received (header);
		sent = tny_header_get_date_sent (header);
		size = tny_header_get_message_size (header);

		if (from == NULL)
			from = g_strdup ("");
		if (to == NULL)
			to = g_strdup ("");
		if (subject == NULL)
			subject = g_strdup ("");
		if (cc == NULL)
			cc = g_strdup ("");

		modest_msg_view_details_dialog_add_row (MODEST_MSG_VIEW_DETAILS_DIALOG (self), _("mcen_fi_message_properties_from"), from);
		modest_msg_view_details_dialog_add_row (MODEST_MSG_VIEW_DETAILS_DIALOG (self), _("mcen_fi_message_properties_subject"), subject);
		if ((folder_type != TNY_FOLDER_TYPE_SENT)&&
		    (folder_type != TNY_FOLDER_TYPE_DRAFTS)&&
		    (folder_type != TNY_FOLDER_TYPE_OUTBOX)) {
			gchar *received_s;

			received_s = modest_text_utils_get_display_date (received);
			if (received_s == NULL)
				received_s = g_strdup (received_s);
			modest_msg_view_details_dialog_add_row (MODEST_MSG_VIEW_DETAILS_DIALOG (self), _("mcen_fi_message_properties_received"), received_s);
			g_free (received_s);
		} 

		if ((folder_type != TNY_FOLDER_TYPE_DRAFTS)&&
		    (folder_type != TNY_FOLDER_TYPE_OUTBOX)) {
			gchar *sent_s;

			sent_s = modest_text_utils_get_display_date (sent);
			if (sent_s == NULL)
				sent_s = g_strdup (sent_s);
			modest_msg_view_details_dialog_add_row (MODEST_MSG_VIEW_DETAILS_DIALOG (self), _("mcen_fi_message_properties_sent"), sent_s);
			g_free (sent_s);
		} 

		modest_msg_view_details_dialog_add_row (MODEST_MSG_VIEW_DETAILS_DIALOG (self), _("mcen_fi_message_properties_to"), to);
		modest_msg_view_details_dialog_add_row (MODEST_MSG_VIEW_DETAILS_DIALOG (self), _("mcen_fi_message_properties_cc"), cc);

		if (size <= 0)
			size_s = g_strdup (_("mcen_va_message_properties_size_noinfo"));
		else
			size_s = modest_text_utils_get_display_size (size);
		modest_msg_view_details_dialog_add_row (MODEST_MSG_VIEW_DETAILS_DIALOG (self), _("mcen_fi_message_properties_size"), size_s);
		g_free (size_s);

		g_free (to);
		g_free (from);
		g_free (subject);
		g_free (cc);
	}

}

static void
modest_msg_view_details_dialog_clear (TnyHeaderView *self)
{
	ModestMsgViewDetailsDialogPrivate *priv;
	GList *children, *node;

	g_return_if_fail (MODEST_IS_MSG_VIEW_DETAILS_DIALOG (self));
	priv = MODEST_MSG_VIEW_DETAILS_DIALOG_GET_PRIVATE (self);

	if (priv->header != NULL) {
		g_object_unref (priv->header);
		priv->header = NULL;
	}

	children = gtk_container_get_children (GTK_CONTAINER (priv->props_table));
	for (node = children; node != NULL; node = g_list_next (node))
		gtk_widget_destroy (GTK_WIDGET (node->data));
	
}

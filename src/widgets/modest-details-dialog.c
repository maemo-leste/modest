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

#include "modest-details-dialog.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktable.h>
#include <gtk/gtkstock.h>
#include <gtk/gtklabel.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-header-view.h>
#include <tny-folder-store.h>
#include <modest-tny-folder.h>
#include <modest-tny-account.h>
#include <modest-text-utils.h>
#include <modest-datetime-formatter.h>
#include <string.h> /* for strlen */
#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-helper.h>
#endif

static void    modest_details_dialog_set_header_default          (ModestDetailsDialog *self,
								  TnyHeader *header,
								  gboolean get_size);

static void    modest_details_dialog_set_folder_default          (ModestDetailsDialog *self,
								  TnyFolder *foler);

static void    modest_details_dialog_set_message_size_default    (ModestDetailsDialog *self, 
								  guint message_size);

static void    modest_details_dialog_create_container_default    (ModestDetailsDialog *self);

static void    modest_details_dialog_add_data_default            (ModestDetailsDialog *self,
								  const gchar *label,
								  const gchar *value);


G_DEFINE_TYPE (ModestDetailsDialog, 
	       modest_details_dialog, 
	       GTK_TYPE_DIALOG);

#define MODEST_DETAILS_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_DETAILS_DIALOG, ModestDetailsDialogPrivate))


typedef struct _ModestDetailsDialogPrivate ModestDetailsDialogPrivate;

struct _ModestDetailsDialogPrivate
{
	GtkWidget *props_table;
};

static void
modest_details_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_details_dialog_parent_class)->finalize (object);
}

static void
modest_details_dialog_class_init (ModestDetailsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestDetailsDialogPrivate));
	object_class->finalize = modest_details_dialog_finalize;

	klass->create_container_func = modest_details_dialog_create_container_default;
	klass->add_data_func = modest_details_dialog_add_data_default;
	klass->set_header_func = modest_details_dialog_set_header_default;
	klass->set_message_size_func = modest_details_dialog_set_message_size_default;
	klass->set_folder_func = modest_details_dialog_set_folder_default;
}

static void
modest_details_dialog_init (ModestDetailsDialog *self)
{
}

GtkWidget*
modest_details_dialog_new_with_header (GtkWindow *parent, 
				       TnyHeader *header,
				       gboolean get_size)
{
	ModestDetailsDialog *dialog;

	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);
	g_return_val_if_fail (TNY_IS_HEADER (header), NULL);

	dialog = (ModestDetailsDialog *) (g_object_new (MODEST_TYPE_DETAILS_DIALOG, 
							"transient-for", parent, 
							NULL));

	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->create_container_func (dialog);
	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->set_header_func (dialog, header, get_size);

	/* Add close button */
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("mcen_bd_close"), GTK_RESPONSE_CLOSE);

	return GTK_WIDGET (dialog);
}

GtkWidget* 
modest_details_dialog_new_with_folder  (GtkWindow *parent, 
					TnyFolder *folder)
{
	ModestDetailsDialog *dialog;

	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), NULL);

	dialog = (ModestDetailsDialog *) (g_object_new (MODEST_TYPE_DETAILS_DIALOG, 
							"transient-for", parent, 
							NULL));

	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->create_container_func (dialog);
	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->set_folder_func (dialog, folder);

	/* Add close button */
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("mcen_bd_close"), GTK_RESPONSE_CLOSE);

	return GTK_WIDGET (dialog);
}

void
modest_details_dialog_add_data (ModestDetailsDialog *self,
				const gchar *label,
				const gchar *value)
{
	MODEST_DETAILS_DIALOG_GET_CLASS (self)->add_data_func (self, label, value);
}

void
modest_details_dialog_set_message_size (ModestDetailsDialog *self,
					guint size)
{
	MODEST_DETAILS_DIALOG_GET_CLASS (self)->set_message_size_func (self, size);
}

static void
modest_details_dialog_add_data_default (ModestDetailsDialog *self,
					const gchar *label,
					const gchar *value)
{
	ModestDetailsDialogPrivate *priv;
	guint n_rows = 0;
	GtkWidget *label_w, *value_w;
	gchar *secure_value;

	priv = MODEST_DETAILS_DIALOG_GET_PRIVATE (self);

	g_object_get (G_OBJECT (priv->props_table), "n-rows", &n_rows,NULL);

	/* Create label */
	label_w = gtk_label_new (label);
	gtk_misc_set_alignment (GTK_MISC (label_w), 0.0, 0.0);
	gtk_label_set_justify (GTK_LABEL (label_w), GTK_JUSTIFY_LEFT);

#ifdef MODEST_TOOLKIT_HILDON2
	hildon_helper_set_logical_color (label_w,
			GTK_RC_FG, GTK_STATE_NORMAL, "SecondaryTextColor");
#endif

	/* Create secure value */
	secure_value = modest_text_utils_get_secure_header (value, "");

	/* Create value */
	value_w = gtk_label_new (secure_value);
	gtk_label_set_line_wrap ((GtkLabel *) value_w, TRUE);
	gtk_label_set_line_wrap_mode ((GtkLabel *) value_w, PANGO_WRAP_WORD_CHAR);
	gtk_misc_set_alignment (GTK_MISC (value_w), 0.0, 0.0);
	gtk_label_set_justify ((GtkLabel *) value_w, GTK_JUSTIFY_LEFT);

	/* Attach label and value */
	gtk_table_attach (GTK_TABLE (priv->props_table), 
			  label_w, 0, 1, 
			  n_rows, n_rows + 1, 
			  GTK_SHRINK|GTK_FILL, 
			  GTK_SHRINK|GTK_FILL, 
			  0, 0);
	gtk_table_attach (GTK_TABLE (priv->props_table), 
			  value_w, 1, 2, 
			  n_rows, n_rows + 1, 
			  GTK_EXPAND|GTK_FILL, 
			  GTK_SHRINK|GTK_FILL, 
			  0, 0);

	g_free (secure_value);
}

static void 
replace_recipients (gchar **recipients)
{

	gchar *result;

	result = modest_text_utils_simplify_recipients (*recipients);

	g_free (*recipients);
	*recipients = result;
}

static void
modest_details_dialog_set_header_default (ModestDetailsDialog *self,
					  TnyHeader *header,
					  gboolean get_size)
{
	gchar *from = NULL, *subject = NULL, *to = NULL, *cc = NULL, *bcc = NULL;
	time_t received, sent;
 	guint size;
	gchar *size_s;
	TnyFolder *folder;
	TnyFolderType folder_type;
	ModestDatetimeFormatter *datetime_formatter;
	const gchar *date_time_str;

	datetime_formatter = modest_datetime_formatter_new ();

	/* Set window title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_message_properties"));

	folder = tny_header_get_folder (header);
	if (folder) {
		folder_type = modest_tny_folder_guess_folder_type (folder);
		g_object_unref (folder);
	} else {
		folder_type = TNY_FOLDER_TYPE_NORMAL;
	}

	g_return_if_fail (folder_type != TNY_FOLDER_TYPE_INVALID);

	/* Get header data */
	from = tny_header_dup_from (header);
	to = tny_header_dup_to (header);
	subject = tny_header_dup_subject (header);
	cc = tny_header_dup_cc (header);
	bcc = tny_header_dup_bcc (header);
	received = tny_header_get_date_received (header);
	sent = tny_header_get_date_sent (header);
	if (get_size) {
		size = tny_header_get_message_size (header);
	} else {
		size = -1;
	}

	replace_recipients (&from);
	replace_recipients (&to);
	replace_recipients (&cc);
	if (subject == NULL)
		subject = g_strdup ("");

	if (!strcmp (subject, "")) {
		g_free (subject);
		subject = g_strdup (_("mail_va_no_subject"));
	}

	/* Add from and subject for all folders */
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_from"), from);
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_subject"), subject);


	/* for inbox, user-created folders and archive: Received */
	if (received && (folder_type != TNY_FOLDER_TYPE_SENT) &&
	    (folder_type != TNY_FOLDER_TYPE_DRAFTS) &&
	    (folder_type != TNY_FOLDER_TYPE_OUTBOX)) {
		date_time_str = modest_datetime_formatter_display_long_datetime (datetime_formatter, 
									    received);

		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_received"),
						date_time_str);
	}

	/* for drafts (created) */
	if (folder_type == TNY_FOLDER_TYPE_DRAFTS) {
		date_time_str = modest_datetime_formatter_display_long_datetime (datetime_formatter, 
									    received);
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_created"),
						date_time_str);
	}

	/* for everyting except outbox, drafts: Sent */
	if (sent && (folder_type != TNY_FOLDER_TYPE_DRAFTS)&&
	    (folder_type != TNY_FOLDER_TYPE_OUTBOX)) {
		
		date_time_str = modest_datetime_formatter_display_long_datetime (datetime_formatter, 
									    sent);
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_sent"),
						date_time_str);
	}

	/* Set To and CC */
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_to"), to);

	/* only show cc when it's there */
	if (cc && strlen(cc) > 0)
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_cc"), cc);

	/* only show cc when it's there */
	if (bcc && strlen(bcc) > 0) {
		replace_recipients (&bcc);
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_bcc"), bcc);
	}

	/* Set size */
	if (get_size) {
		size_s = modest_text_utils_get_display_size (size);
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_size"), size_s);
		g_free (size_s);
	}

	/* Frees */
	g_object_unref (datetime_formatter);
	g_free (to);
	g_free (from);
	g_free (subject);
	g_free (cc);
	g_free (bcc);
}

static void
modest_details_dialog_set_message_size_default (ModestDetailsDialog *self,
						guint size)
{
	gchar *size_s;
	size_s = modest_text_utils_get_display_size (size);
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_size"), size_s);
	g_free (size_s);
	gtk_widget_show_all (GTK_WIDGET (self));
}

static void
modest_details_dialog_set_folder_default (ModestDetailsDialog *self,
					  TnyFolder *folder)
{
	gchar *count_s, *size_s, *name = NULL;
	guint size, count;

	g_return_if_fail (folder && TNY_IS_FOLDER (folder));
	g_return_if_fail (modest_tny_folder_guess_folder_type (folder)
			  != TNY_FOLDER_TYPE_INVALID);

	/* Set window title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_folder_properties"));

	/* Get data. We use our function because it's recursive */
	count = tny_folder_get_all_count (TNY_FOLDER (folder));
	size = tny_folder_get_local_size (TNY_FOLDER (folder));

	/* Format count and size */
	count_s = g_strdup_printf ("%d", count);
	size_s = modest_text_utils_get_display_size (size);

	/* Different names for the local folders */
	if (modest_tny_folder_is_local_folder (folder) ||
	    modest_tny_folder_is_memory_card_folder (folder)) {
		gint type = modest_tny_folder_get_local_or_mmc_folder_type (folder);
		if (type != TNY_FOLDER_TYPE_UNKNOWN)
			name = g_strdup(modest_local_folder_info_get_type_display_name (type));
	}

	if (!name) {
		if (tny_folder_get_folder_type (folder) == TNY_FOLDER_TYPE_INBOX)
			name = g_strdup (_("mcen_me_folder_inbox"));
		else
			name = g_strdup (tny_folder_get_name (folder));
	}

#ifdef MODEST_TOOLKIT_HILDON2
	modest_details_dialog_add_data (self, _("mcen_fi_folder_properties_foldername"), name);
#else
	gchar *tmp = g_strconcat (_("mcen_fi_folder_properties_foldername"), ":", NULL);
	modest_details_dialog_add_data (self, tmp, name);
	g_free (tmp);
#endif

#ifdef MODEST_TOOLKIT_HILDON2
	modest_details_dialog_add_data (self, _("mcen_fi_folder_properties_messages"), count_s);
#else
	gchar *tmp = g_strconcat (_("mcen_fi_folder_properties_messages"), ":", NULL);
	modest_details_dialog_add_data (self, tmp, count_s);
	g_free (tmp);
#endif

#ifdef MODEST_TOOLKIT_HILDON2
	modest_details_dialog_add_data (self, _("mcen_fi_folder_properties_size"), size_s);
#else
	gchar *tmp = g_strconcat (_("mcen_fi_folder_properties_size"), ":", NULL);
	modest_details_dialog_add_data (self, tmp, size_s);
	g_free (tmp);
#endif

	/* Frees */
	g_free (name);
	g_free (size_s);
	g_free (count_s);
}

static gboolean
on_key_press_event (GtkWindow *window, GdkEventKey *event, gpointer userdata)
{
	GtkWidget *focused;

	focused = gtk_window_get_focus (window);
	if (GTK_IS_SCROLLED_WINDOW (focused)) {
		GtkAdjustment *vadj;
		gboolean return_value;

		vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (focused));
		switch (event->keyval) {
		case GDK_Up:
		case GDK_KP_Up:
			if (vadj->value > 0.0) {
				g_signal_emit_by_name (G_OBJECT (focused), "scroll-child", GTK_SCROLL_STEP_UP, FALSE, 
						       &return_value);
				return TRUE;
			}
			break;
		case GDK_Down:
		case GDK_KP_Down:
			if (vadj->value < vadj->upper - vadj->page_size) {
				g_signal_emit_by_name (G_OBJECT (focused), "scroll-child", GTK_SCROLL_STEP_DOWN, FALSE, 
						       &return_value);
				return TRUE;
			}
			break;
		}
	}

	return FALSE;
}

static void
modest_details_dialog_create_container_default (ModestDetailsDialog *self)
{
	ModestDetailsDialogPrivate *priv;
	GtkWidget *scrollbar;

	priv = MODEST_DETAILS_DIALOG_GET_PRIVATE (self);
	scrollbar = gtk_scrolled_window_new (NULL, NULL);

	gtk_window_set_default_size (GTK_WINDOW (self), 400, 220);

	priv->props_table = gtk_table_new (0, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (priv->props_table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (priv->props_table), 1);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollbar), priv->props_table);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollbar), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->props_table), 
					     gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrollbar)));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), scrollbar);

	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);

	g_signal_connect (self, "key-press-event", G_CALLBACK (on_key_press_event), self);
}

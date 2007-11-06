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
#include <string.h> /* for strlen */

static void    modest_details_dialog_set_header_default          (ModestDetailsDialog *self,
								  TnyHeader *header);

static void    modest_details_dialog_set_folder_default          (ModestDetailsDialog *self,
								  TnyFolder *foler);

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
	klass->set_folder_func = modest_details_dialog_set_folder_default;
}

static void
modest_details_dialog_init (ModestDetailsDialog *self)
{
	MODEST_DETAILS_DIALOG_GET_CLASS (self)->create_container_func (self);
}

GtkWidget*
modest_details_dialog_new_with_header (GtkWindow *parent, 
				       TnyHeader *header)
{
	ModestDetailsDialog *dialog;

	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);
	g_return_val_if_fail (TNY_IS_HEADER (header), NULL);

	dialog = (ModestDetailsDialog *) (g_object_new (MODEST_TYPE_DETAILS_DIALOG, 
							"transient-for", parent, 
							NULL));

	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->set_header_func (dialog, header);

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

	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->set_folder_func (dialog, folder);

	return GTK_WIDGET (dialog);
}

void
modest_details_dialog_add_data (ModestDetailsDialog *self,
				const gchar *label,
				const gchar *value)
{
	MODEST_DETAILS_DIALOG_GET_CLASS (self)->add_data_func (self, label, value);
}

static void
modest_details_dialog_add_data_default (ModestDetailsDialog *self,
					const gchar *label,
					const gchar *value)
{
	ModestDetailsDialogPrivate *priv;
	guint n_rows = 0;
	GtkWidget *label_w, *value_w;

	priv = MODEST_DETAILS_DIALOG_GET_PRIVATE (self);

	g_object_get (G_OBJECT (priv->props_table), "n-rows", &n_rows,NULL);

	/* Create label */
	label_w = gtk_label_new (label);
	gtk_misc_set_alignment (GTK_MISC (label_w), 1.0, 0.0);
	gtk_label_set_justify (GTK_LABEL (label_w), GTK_JUSTIFY_RIGHT);

	/* Create value */
	value_w = gtk_label_new (value);
	gtk_label_set_line_wrap (GTK_LABEL (value_w), TRUE);
	gtk_misc_set_alignment (GTK_MISC (value_w), 0.0, 0.0);
	gtk_label_set_justify (GTK_LABEL (value_w), GTK_JUSTIFY_LEFT);

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
}


static void
modest_details_dialog_set_header_default (ModestDetailsDialog *self,
					  TnyHeader *header)
{
	gchar *from, *subject, *to, *cc;
	time_t received, sent;
 	guint size;
	gchar *size_s;
	TnyFolder *folder;
	TnyFolderType folder_type;
#define DATE_TIME_BUFFER_SIZE 128
	gchar date_time_buffer [DATE_TIME_BUFFER_SIZE];
	
	/* Set window title & Add close button */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_message_properties"));
	gtk_dialog_add_button (GTK_DIALOG (self), _("mcen_bd_close"), GTK_RESPONSE_CLOSE);

	folder = tny_header_get_folder (header);
	folder_type = modest_tny_folder_guess_folder_type (folder);
	g_object_unref (folder);

	g_return_if_fail (folder_type != TNY_FOLDER_TYPE_INVALID);
	
	/* Get header data */
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

	/* Add from and subject for all folders */
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_from"), from);
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_subject"), subject);


	/* for inbox, user-created folders and archive: Received */
	if (received && (folder_type != TNY_FOLDER_TYPE_SENT) &&
	    (folder_type != TNY_FOLDER_TYPE_DRAFTS) &&
	    (folder_type != TNY_FOLDER_TYPE_OUTBOX)) {
		
		modest_text_utils_strftime (date_time_buffer, DATE_TIME_BUFFER_SIZE, "%x %X",
					    received);
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_received"),
						date_time_buffer);
	}

	/* for outbox, drafts: Modified: (_created) */
	if ((folder_type == TNY_FOLDER_TYPE_DRAFTS) ||
	    (folder_type == TNY_FOLDER_TYPE_OUTBOX) ||
	    (folder_type == TNY_FOLDER_TYPE_SENT)) {
 		modest_text_utils_strftime (date_time_buffer, DATE_TIME_BUFFER_SIZE, "%x %X",
					    received);
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_created"),
						date_time_buffer);
	}

	/* for everyting except outbox, drafts: Sent */
	if (sent && (folder_type != TNY_FOLDER_TYPE_DRAFTS)&&
	    (folder_type != TNY_FOLDER_TYPE_OUTBOX)) {
		
		modest_text_utils_strftime (date_time_buffer, DATE_TIME_BUFFER_SIZE, "%x %X",
					    sent);
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_sent"),
						date_time_buffer);
	}
	
	/* Set To and CC */
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_to"), to);

	/* only show cc when it's there */
	if (cc && strlen(cc) > 0)
		modest_details_dialog_add_data (self, _("mcen_fi_message_properties_cc"), cc);

	/* Set size */
	if (size <= 0)
		size_s = g_strdup (_("mcen_va_message_properties_size_noinfo"));
	else
		size_s = modest_text_utils_get_display_size (size);
	modest_details_dialog_add_data (self, _("mcen_fi_message_properties_size"), size_s);
	g_free (size_s);

	/* Frees */
	g_free (to);
	g_free (from);
	g_free (subject);
	g_free (cc);
}

static void
modest_details_dialog_set_folder_default (ModestDetailsDialog *self,
					  TnyFolder *folder)
{
	gchar *count_s, *size_s, *name = NULL;
	gint size, count;


	g_return_if_fail (folder && TNY_IS_FOLDER (folder));
	g_return_if_fail (modest_tny_folder_guess_folder_type (folder)
			  != TNY_FOLDER_TYPE_INVALID);
	
	/* Set window title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_folder_properties"));
	gtk_dialog_add_button (GTK_DIALOG (self), _("mcen_bd_close"), GTK_RESPONSE_CLOSE);

	/* Get data. We use our function because it's recursive */
	count = modest_tny_folder_store_get_message_count (TNY_FOLDER_STORE (folder));
	size = modest_tny_folder_store_get_local_size (TNY_FOLDER_STORE (folder));

	/* Format count and size */
	count_s = g_strdup_printf ("%d", count);
	if (size <= 0)
		size_s = g_strdup (_("mcen_va_message_properties_size_noinfo"));
	else
		size_s = modest_text_utils_get_display_size (size);

	/* Different names for the local folders */
	if (modest_tny_folder_is_local_folder (folder)) {
		gint type = modest_tny_folder_get_local_or_mmc_folder_type (folder);
		if (type != TNY_FOLDER_TYPE_UNKNOWN)
			name = g_strdup(modest_local_folder_info_get_type_display_name (type));
	} 

	if (!name)	
		name = g_strdup (tny_folder_get_name (folder));

	modest_details_dialog_add_data (self, _("mcen_fi_folder_properties_foldername"), name);
	modest_details_dialog_add_data (self, _("mcen_fi_folder_properties_messages"), count_s);
	modest_details_dialog_add_data (self, _("mcen_fi_folder_properties_size"), size_s);

	/* Frees */
	g_free (name);
	g_free (size_s);
	g_free (count_s);
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
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), scrollbar);

	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
}

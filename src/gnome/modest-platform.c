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

#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomeui/gnome-icon-lookup.h>
#include <tny-gnome-device.h>

#include "modest-platform.h"
#include "modest-mail-operation-queue.h"
#include "modest-runtime.h"

gboolean
modest_platform_init (void)
{	
	return TRUE; /* nothing to do */
}


TnyDevice*
modest_platform_get_new_device (void)
{
	return TNY_DEVICE (tny_gnome_device_new ());
}


gchar*
modest_platform_get_file_icon_name (const gchar* name, const gchar* mime_type,
					  gchar **effective_mime_type)
{
	GString *mime_str = NULL;
	gchar *icon_name  = NULL;
	gchar *uri;
	const static gchar* octet_stream = "application/octet-stream";
	
	g_return_val_if_fail (name || mime_type, NULL);

	if (!mime_type || g_ascii_strcasecmp (mime_type, octet_stream)) 
		mime_str = g_string_new(gnome_vfs_mime_type_from_name_or_default
					(name, "application/octet-stream"));
	else {
		mime_str = g_string_new (mime_type);
		g_string_ascii_down (mime_str);
	}

	uri = g_strconcat ("file:///", name ? name : "dummy", NULL);
	icon_name  = gnome_icon_lookup (gtk_icon_theme_get_default(), NULL,
					uri, NULL, NULL, mime_str->str, 0, 0);
	g_free (uri);

	if (effective_mime_type)
		*effective_mime_type = g_string_free (mime_str, FALSE);
	else
		g_string_free (mime_str, TRUE);

	return icon_name;
}

gboolean 
modest_platform_activate_uri (const gchar *uri)
{
	modest_runtime_not_implemented (NULL);
	return FALSE;
}

gboolean 
modest_platform_show_uri_popup (const gchar *uri)
{
	modest_runtime_not_implemented (NULL);
	return FALSE;
}

GdkPixbuf*
modest_platform_get_icon (const gchar *name)
{
	GError *err = NULL;
	GdkPixbuf* pixbuf;

	g_return_val_if_fail (name, NULL);

	pixbuf = gdk_pixbuf_new_from_file (name, &err);

	if (!pixbuf) {
		g_printerr ("modest: error while loading icon '%s': %s\n",
			    name, err->message);
		g_error_free (err);
	}
	
	return pixbuf;
}


const gchar*
modest_platform_get_app_name (void)
{
	return ("Modest");
}

gint 
modest_platform_run_new_folder_dialog (GtkWindow *parent_window,
				       TnyFolderStore *parent_folder,
				       gchar *suggested_name,
				       gchar **folder_name)
{
	GtkWidget *dialog, *entry;
	gboolean finished = FALSE;
	gint result;
	TnyFolder *new_folder;
	ModestMailOperation *mail_op;

	/* Ask the user for the folder name */
	dialog = gtk_dialog_new_with_buttons (_("New Folder Name"),
					      parent_window,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    gtk_label_new (_("Please enter a name for the new folder")),
			    FALSE, FALSE, 0);
		
	entry = gtk_entry_new_with_max_length (40);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    entry,
			    TRUE, FALSE, 0);
	
	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));

	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT)
		*folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	gtk_widget_destroy (dialog);

	return result;
}


gint
modest_platform_run_confirmation_dialog (GtkWindow *parent_window,
					 const gchar *msg)
{
	/* TODO implement confirmation dialog */
	return GTK_RESPONSE_CANCEL;
}

void
modest_platform_run_information_dialog (GtkWindow *parent_window,
					ModestInformationDialogType type)
{
	switch (type) {
	case MODEST_INFORMATION_CREATE_FOLDER:
		break;
	case MODEST_INFORMATION_DELETE_FOLDER:
		break;
	};

	/* TODO: implement a information dialog */
}

gboolean modest_platform_connect_and_wait (GtkWindow *parent_window)
{
	/* TODO: Do something with network-manager? 
	   Otherwise, maybe it is safe to assume that we would already be online if we could be. */
	return TRUE;
}

gboolean modest_platform_set_update_interval (guint minutes)
{
	/* TODO. */
}


/* Copyright (c) 2007, Nokia Corporation
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

#include <glib.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <string.h> /* for strlen */
#include <modest-runtime.h>
#include <libgnomevfs/gnome-vfs.h>
#include <tny-fs-stream.h>
#include <tny-camel-account.h>
#include <tny-status.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>

#include <modest-defs.h>
#include "modest-utils.h"
#include "modest-platform.h"
#include <modest-local-folder-info.h>

GQuark
modest_utils_get_supported_secure_authentication_error_quark (void)
{
	return g_quark_from_static_string("modest-utils-get-supported-secure-authentication-error-quark");
}

gboolean 
modest_utils_folder_writable (const gchar *filename)
{
	g_return_val_if_fail (filename, FALSE);

	if (!filename)
		return FALSE;
	
	if (g_strncasecmp (filename, "obex", 4) != 0) {
		GnomeVFSFileInfo *folder_info;
		gchar *folder;
		folder = g_path_get_dirname (filename);
		folder_info = gnome_vfs_file_info_new ();
		gnome_vfs_get_file_info (folder, folder_info,
					 GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);
		g_free (folder);
		if (!((folder_info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE) ||
		      (folder_info->permissions & GNOME_VFS_PERM_USER_WRITE))) {
			return FALSE;
		}
		gnome_vfs_file_info_unref (folder_info);
	}
	return TRUE;
}

gboolean 
modest_utils_file_exists (const gchar *filename)
{
	GnomeVFSURI *uri = NULL;
	gboolean result = FALSE;

	g_return_val_if_fail (filename, FALSE);
	
	uri = gnome_vfs_uri_new (filename);
	if (uri) {
		result = gnome_vfs_uri_exists (uri);
		gnome_vfs_uri_unref (uri);
	}
	return result;
}

TnyFsStream *
modest_utils_create_temp_stream (const gchar *orig_name, const gchar *hash_base, gchar **path)
{
	gint fd;
	gchar *filepath = NULL;
	gchar *tmpdir;
	guint hash_number;

	/* hmmm... maybe we need a modest_text_utils_validate_file_name? */
	g_return_val_if_fail (orig_name && strlen(orig_name) != 0, NULL);

	if (strlen(orig_name) > 200) {
		g_warning ("%s: filename too long ('%s')",
			   __FUNCTION__, orig_name);
		return NULL;
	}
	
	if (g_strstr_len (orig_name, strlen(orig_name), "/") != NULL) {
		g_warning ("%s: filename contains '/' character(s) (%s)",
			   __FUNCTION__, orig_name);
		return NULL;
	}
		
	/* make a random subdir under /tmp or /var/tmp */
	if (hash_base != NULL) {
		hash_number = g_str_hash (hash_base);
	} else {
		hash_number = (guint) random ();
	}
	tmpdir = g_strdup_printf ("%s/%u", g_get_tmp_dir (), hash_number);
	if ((g_access (tmpdir, R_OK) == -1) && (g_mkdir (tmpdir, 0755) == -1)) {
		g_warning ("%s: failed to create dir '%s': %s",
			   __FUNCTION__, tmpdir, g_strerror(errno));
		g_free (tmpdir);
		return NULL;
	}

	filepath = g_strconcat (tmpdir, "/", orig_name, NULL);
	/* don't overwrite if it already exists, even if it is writable */
	if (modest_utils_file_exists (filepath)) {
		if (path!=NULL) {
			*path = filepath;
		} else {
			g_free (filepath);
		}
		g_free (tmpdir);
		return NULL;
	} else {
		/* try to write the file there */
		fd = g_open (filepath, O_CREAT|O_WRONLY|O_TRUNC, 0644);
		if (fd == -1) {
			g_warning ("%s: failed to create '%s': %s",
					__FUNCTION__, filepath, g_strerror(errno));			
			g_free (filepath);
			g_free (tmpdir);
			return NULL;
		}
	}

	g_free (tmpdir);

	if (path)
		*path = filepath;

	return TNY_FS_STREAM (tny_fs_stream_new (fd));
}

typedef struct 
{
	gboolean cancel;
	GList *result;
	GtkWidget* dialog;
	GtkWidget* progress;
	GError* error;
} ModestGetSupportedAuthInfo;

static void on_camel_account_get_supported_secure_authentication_status (
	GObject *self, TnyStatus *status, gpointer user_data)
{
	/*ModestGetSupportedAuthInfo* info = (ModestGetSupportedAuthInfo*) user_data;*/
}

static gboolean
on_idle_secure_auth_finished (gpointer user_data)
{
	ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
	/* Operation has finished, close the dialog. Control continues after
	 * gtk_dialog_run in modest_utils_get_supported_secure_authentication_methods() */

	/* This is a GDK lock because we are an idle callback and
	 * the code below is or does Gtk+ code */

	gdk_threads_enter(); /* CHECKED */
	gtk_dialog_response (GTK_DIALOG (info->dialog), GTK_RESPONSE_ACCEPT);
	gdk_threads_leave(); /* CHECKED */

	return FALSE;
}

static void
on_camel_account_get_supported_secure_authentication (TnyCamelAccount *self, gboolean cancelled,
	TnyList *auth_types, GError *err, gpointer user_data)
{
	g_return_if_fail (TNY_IS_CAMEL_ACCOUNT(self));
	g_return_if_fail (TNY_IS_LIST(auth_types));
	
	ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
	g_return_if_fail (info);
	

	/* Free everything if the actual action was canceled */
	if (info->cancel) {
		/* The operation was canceled and the ownership of the info given to us
		 * so that we could still check the cancel flag. */
		g_slice_free (ModestGetSupportedAuthInfo, info);
		info = NULL;
	}
	else
	{
		if (err) {
			if (info->error) {
				g_error_free (info->error);
				info->error = NULL;
			}
			
			info->error = g_error_copy (err);
		}

		if (!auth_types) {
			g_warning ("DEBUG: %s: auth_types is NULL.\n", __FUNCTION__);
		}
		else if (tny_list_get_length(auth_types) == 0) 
			g_warning ("DEBUG: %s: auth_types is an empty TnyList.\n", __FUNCTION__);
		else {
			ModestPairList* pairs = modest_protocol_info_get_auth_protocol_pair_list ();
  
			/* Get the enum value for the strings: */
			GList *result = NULL;
			TnyIterator* iter = tny_list_create_iterator(auth_types);
			while (!tny_iterator_is_done(iter)) {
				TnyPair *pair = TNY_PAIR(tny_iterator_get_current(iter));
				const gchar *auth_name = NULL;
				if (pair) {
					auth_name = tny_pair_get_name(pair);
					g_object_unref (pair);
					pair = NULL;
				}

				printf("DEBUG: %s: auth_name=%s\n", __FUNCTION__, auth_name);

				ModestAuthProtocol proto = modest_protocol_info_get_auth_protocol (auth_name);
				if(proto != MODEST_PROTOCOL_AUTH_NONE)
						result = g_list_prepend(result, GINT_TO_POINTER(proto));

				tny_iterator_next(iter);
			}
			g_object_unref (iter);

			modest_pair_list_free (pairs);
	
			info->result = result;
		}

		printf("DEBUG: finished\n");
				
		/* Close the dialog in a main thread */
		g_idle_add(on_idle_secure_auth_finished, info);
	}
}

static void
on_secure_auth_cancel(GtkWidget* dialog, int response, gpointer user_data)
{
	g_return_if_fail (GTK_IS_WIDGET(dialog));
	
	if(response == GTK_RESPONSE_REJECT || response == GTK_RESPONSE_DELETE_EVENT) {
		ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
		g_return_if_fail(info);
		/* This gives the ownership of the info to the worker thread. */
		info->result = NULL;
		info->cancel = TRUE;
	}
}

GList*
modest_utils_get_supported_secure_authentication_methods (ModestTransportStoreProtocol proto, 
	const gchar* hostname, gint port, const gchar* username, GtkWindow *parent_window, GError** error)
{
	g_return_val_if_fail (proto != MODEST_PROTOCOL_TRANSPORT_STORE_UNKNOWN, NULL);
	
	/* We need a connection to get the capabilities; */
	if (!modest_platform_connect_and_wait (GTK_WINDOW (parent_window), NULL))
		return NULL;
	 
	/*
	result = g_list_append (result, GINT_TO_POINTER (MODEST_PROTOCOL_AUTH_CRAMMD5));
	*/
	
	/* Create a TnyCamelAccount so we can use 
	 * tny_camel_account_get_supported_secure_authentication(): */
	TnyAccount * tny_account = NULL;
	switch (proto) {
	case MODEST_PROTOCOL_TRANSPORT_SENDMAIL:
	case MODEST_PROTOCOL_TRANSPORT_SMTP:
		tny_account = TNY_ACCOUNT(tny_camel_transport_account_new ()); break;
	case MODEST_PROTOCOL_STORE_POP:
		tny_account = TNY_ACCOUNT(tny_camel_pop_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_IMAP:
		tny_account = TNY_ACCOUNT(tny_camel_imap_store_account_new ()); break;
	case MODEST_PROTOCOL_STORE_MAILDIR:
	case MODEST_PROTOCOL_STORE_MBOX:
		tny_account = TNY_ACCOUNT(tny_camel_store_account_new()); break;
	default:
		tny_account = NULL;
	}

	
	if (!tny_account) {
		g_printerr ("%s could not create tny account.", __FUNCTION__);
		return NULL;
	}
	
	/* Set proto, so that the prepare_func() vfunc will work when we call 
	 * set_session(): */
	 /* TODO: Why isn't this done in account_new()? */
	tny_account_set_proto (tny_account,
			       modest_protocol_info_get_transport_store_protocol_name(proto));

	tny_account_set_hostname (tny_account, hostname);
	/* Required for POP, at least */
	tny_account_set_user (tny_account, username);
			       
	if(port > 0)
		tny_account_set_port (tny_account, port);
		
	/* Set the session for the account, so we can use it: */
	ModestTnyAccountStore *account_store = modest_runtime_get_account_store ();
	TnySessionCamel *session = 
		modest_tny_account_store_get_session (TNY_ACCOUNT_STORE (account_store));
	g_return_val_if_fail (session, NULL);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);
	
	
	/* Ask camel to ask the server, asynchronously: */
	ModestGetSupportedAuthInfo *info = g_slice_new (ModestGetSupportedAuthInfo);
	info->result = NULL;
	info->cancel = FALSE;
	info->error = NULL;
	info->progress = gtk_progress_bar_new();
	/* TODO: Need logical_ID for the title: */
	info->dialog = gtk_dialog_new_with_buttons(_("Authentication"),
	                                           parent_window, GTK_DIALOG_MODAL,
	                                           _("mcen_bd_dialog_cancel"),
	                                           GTK_RESPONSE_REJECT,
	                                           NULL);
	//gtk_window_set_default_size(GTK_WINDOW(info->dialog), 300, 100);
	
	g_signal_connect(G_OBJECT(info->dialog), "response", G_CALLBACK(on_secure_auth_cancel), info);
	
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox),
	                  gtk_label_new(_("emev_ni_checking_supported_auth_methods")));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox), info->progress);
	gtk_widget_show_all(info->dialog);
	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(info->progress));
	
	printf ("DEBUG: %s: STARTING.\n", __FUNCTION__);
	tny_camel_account_get_supported_secure_authentication (
		TNY_CAMEL_ACCOUNT (tny_account),
		on_camel_account_get_supported_secure_authentication,
		on_camel_account_get_supported_secure_authentication_status,
		info);

	gtk_dialog_run (GTK_DIALOG (info->dialog));
	
	gtk_widget_destroy(info->dialog);
			
	GList *result = info->result;
	if (!info->cancel)
	{
		if (info->error) {
			gchar * debug_url_string = tny_account_get_url_string  (tny_account);
			g_warning ("DEBUG: %s:\n  error: %s\n  account url: %s", __FUNCTION__, info->error->message, 
				debug_url_string);
			g_free (debug_url_string);
			
			g_propagate_error(error, info->error);
			info->error = NULL;
		}

		g_slice_free (ModestGetSupportedAuthInfo, info);
		info = NULL;
	}
	else
	{
		// Tell the caller that the operation was canceled so it can
		// make a difference
		g_set_error(error,
		            modest_utils_get_supported_secure_authentication_error_quark(),
		            MODEST_UTILS_GET_SUPPORTED_SECURE_AUTHENTICATION_ERROR_CANCELED,
			    "User has canceled query");
	}

	return result;
}

void 
modest_utils_show_dialog_and_forget (GtkWindow *parent_window, 
				     GtkDialog *dialog)
{
	g_return_if_fail (GTK_IS_WINDOW(parent_window));
	g_return_if_fail (GTK_IS_DIALOG(dialog));

	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent_window);
	
	/* Destroy the dialog when it is closed: */
	g_signal_connect_swapped (dialog, 
				  "response", 
				  G_CALLBACK (gtk_widget_destroy), 
				  dialog);

	gtk_widget_show (GTK_WIDGET (dialog));
}

void
modest_utils_toggle_action_set_active_block_notify (GtkToggleAction *action, gboolean value)
{
	GSList *proxies = NULL;

	g_return_if_fail (GTK_IS_TOGGLE_ACTION (action));

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


gint 
modest_list_index (TnyList *list, GObject *object)
{
	TnyIterator *iter;
	gint index = 0;

	g_return_val_if_fail (TNY_IS_LIST(list), -1);
	g_return_val_if_fail (G_IS_OBJECT(object), -1);
	
	iter = tny_list_create_iterator (list);
	while (!tny_iterator_is_done (iter)) {
		GObject *current = tny_iterator_get_current (iter);

		g_object_unref (current);
		if (current == object)
			break;

		tny_iterator_next (iter);
		index++;
	}

	if (tny_iterator_is_done (iter))
		index = -1;
	g_object_unref (iter);
	return index;
}

guint64 
modest_folder_available_space (const gchar *maildir_path)
{
	gchar *folder;
	gchar *uri_string;
	GnomeVFSURI *uri;
	GnomeVFSFileSize size;

	folder = modest_local_folder_info_get_maildir_path (maildir_path);
	uri_string = gnome_vfs_get_uri_from_local_path (folder);
	uri = gnome_vfs_uri_new (uri_string);
	g_free (folder);
	g_free (uri_string);

	if (uri) {
		if (gnome_vfs_get_volume_free_space (uri, &size) != GNOME_VFS_OK)
			size = -1;
		gnome_vfs_uri_unref (uri);
	} else {
		size = -1;
	}

	return (guint64) size;
}

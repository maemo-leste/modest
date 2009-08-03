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
#include <tny-camel-send-queue.h>
#include <tny-camel-transport-account.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <locale.h>
#include <modest-defs.h>
#include "modest-utils.h"
#include "modest-platform.h"
#include <modest-account-protocol.h>
#include "modest-account-mgr-helpers.h"
#include "modest-text-utils.h"
#include <modest-local-folder-info.h>
#include "widgets/modest-header-view.h"
#include "widgets/modest-main-window.h"
#include "modest-widget-memory.h"
#include "widgets/modest-sort-criterium-view.h"
#ifdef MODEST_TOOLKIT_HILDON2
#include "modest-header-window.h"
#endif
#include <langinfo.h>

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

	if (g_ascii_strncasecmp (filename, "obex", 4) != 0) {
		GnomeVFSFileInfo *folder_info = NULL;
		GnomeVFSResult result = GNOME_VFS_OK;
		GnomeVFSURI *uri = NULL;
		GnomeVFSURI *folder_uri = NULL;

		uri = gnome_vfs_uri_new (filename);
		folder_uri = gnome_vfs_uri_get_parent (uri);

		if (folder_uri != NULL) {
			folder_info = gnome_vfs_file_info_new ();
			result = gnome_vfs_get_file_info_uri (folder_uri, folder_info,
							      GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS);
			gnome_vfs_uri_unref (folder_uri);
		}
		gnome_vfs_uri_unref (uri);

		if (folder_uri == NULL)
			return FALSE;

		if ((result != GNOME_VFS_OK) ||
		    (!((folder_info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE) ||
		       (folder_info->permissions & GNOME_VFS_PERM_USER_WRITE)))) {

			gnome_vfs_file_info_unref (folder_info);
			return FALSE;
		}
		gnome_vfs_file_info_unref (folder_info);
	}
	return TRUE;
}

gboolean
modest_utils_file_exists (const gchar *filename)
{
	gboolean result = FALSE;
	gchar *escaped;

	g_return_val_if_fail (filename, FALSE);

	escaped = g_uri_escape_string (filename, NULL, FALSE);
	if (g_access (escaped, F_OK) == 0)
		result = TRUE;
	g_free (escaped);

	return result;
}

TnyFsStream *
modest_utils_create_temp_stream (const gchar *orig_name, const gchar *hash_base, gchar **path)
{
	gint fd;
	gchar *filepath = NULL;
	gchar *tmpdir, *tmp;
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

	tmp = g_uri_escape_string (orig_name, NULL, FALSE);
	filepath = g_build_filename (tmpdir, tmp, NULL);
	g_free (tmp);

	/* if file exists, first we try to remove it */
	if (g_access (filepath, F_OK) == 0)
		g_unlink (filepath);

	/* don't overwrite if it already exists, even if it is writable */
	if (g_access (filepath, F_OK) == 0) {
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
	else
		g_free (filepath);

	return TNY_FS_STREAM (tny_fs_stream_new (fd));
}

typedef struct 
{
	GList **result;
	GtkWidget* dialog;
	GtkWidget* progress;
} ModestGetSupportedAuthInfo;

static gboolean
on_idle_secure_auth_finished (gpointer user_data)
{
	/* Operation has finished, close the dialog. Control continues after
	 * gtk_dialog_run in modest_utils_get_supported_secure_authentication_methods() */
	gdk_threads_enter(); /* CHECKED */
	gtk_dialog_response (GTK_DIALOG (user_data), GTK_RESPONSE_ACCEPT);
	gdk_threads_leave(); /* CHECKED */

	return FALSE;
}

static void
on_camel_account_get_supported_secure_authentication (TnyCamelAccount *self,
						      gboolean cancelled,
						      TnyList *auth_types,
						      GError *err,
						      gpointer user_data)
{
	ModestPairList *pairs;
	GList *result;
	ModestProtocolRegistry *protocol_registry;
	ModestGetSupportedAuthInfo *info = (ModestGetSupportedAuthInfo*)user_data;
	TnyIterator* iter;

	g_return_if_fail (user_data);
	g_return_if_fail (TNY_IS_CAMEL_ACCOUNT(self));
	g_return_if_fail (TNY_IS_LIST(auth_types));

	info = (ModestGetSupportedAuthInfo *) user_data;

	/* Free everything if the actual action was canceled */
	if (cancelled) {
		g_debug ("%s: operation canceled\n", __FUNCTION__);
		goto close_dialog;
	}

	if (err) {
		g_debug ("%s: error getting the supported auth methods\n", __FUNCTION__);
		goto close_dialog;
	}

	if (!auth_types) {
		g_debug ("%s: auth_types is NULL.\n", __FUNCTION__);
		goto close_dialog;
	}

	if (tny_list_get_length(auth_types) == 0) {
		g_debug ("%s: auth_types is an empty TnyList.\n", __FUNCTION__);
		goto close_dialog;
	}

	protocol_registry = modest_runtime_get_protocol_registry ();
	pairs = modest_protocol_registry_get_pair_list_by_tag (protocol_registry, MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS);

	/* Get the enum value for the strings: */
	result = NULL;
	iter = tny_list_create_iterator(auth_types);
	while (!tny_iterator_is_done(iter)) {
		TnyPair *pair;
		const gchar *auth_name;
		ModestProtocolType protocol_type;

		pair = TNY_PAIR(tny_iterator_get_current(iter));
		auth_name = NULL;
		if (pair) {
			auth_name = tny_pair_get_name(pair);
			g_object_unref (pair);
			pair = NULL;
		}

		g_debug ("%s: auth_name=%s\n", __FUNCTION__, auth_name);

		protocol_type = modest_protocol_get_type_id (modest_protocol_registry_get_protocol_by_name (protocol_registry,
													    MODEST_PROTOCOL_REGISTRY_AUTH_PROTOCOLS,
													    auth_name));

		if (modest_protocol_registry_protocol_type_is_secure (protocol_registry, protocol_type))
			result = g_list_prepend(result, GINT_TO_POINTER(protocol_type));

		tny_iterator_next(iter);
	}
	g_object_unref (iter);

	modest_pair_list_free (pairs);
	*(info->result) = result;

 close_dialog:
	/* Close the dialog in a main thread */
	g_idle_add(on_idle_secure_auth_finished, info->dialog);

	/* Free the info */
	g_slice_free (ModestGetSupportedAuthInfo, info);
}

typedef struct {
	GtkWidget *progress;
	gboolean not_finished;
} KeepPulsing;

static gboolean
keep_pulsing (gpointer user_data)
{
	KeepPulsing *info = (KeepPulsing *) user_data;

	if (!info->not_finished) {
		g_slice_free (KeepPulsing, info);
		return FALSE;
	}

	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (info->progress));
	return TRUE;
}

GList*
modest_utils_get_supported_secure_authentication_methods (ModestProtocolType protocol_type,
							  const gchar* hostname,
							  gint port,
							  const gchar* username,
							  GtkWindow *parent_window,
							  GError** error)
{
	TnyAccount * tny_account = NULL;
	ModestProtocolRegistry *protocol_registry;
	GtkWidget *dialog;
	gint retval;
	ModestTnyAccountStore *account_store;
	TnySessionCamel *session = NULL;
	ModestProtocol *protocol = NULL;
	GList *result = NULL;
	GtkWidget *progress;

	g_return_val_if_fail (protocol_type != MODEST_PROTOCOL_REGISTRY_TYPE_INVALID, NULL);

	protocol_registry = modest_runtime_get_protocol_registry ();

	/* We need a connection to get the capabilities; */
	if (!modest_platform_connect_and_wait (GTK_WINDOW (parent_window), NULL))
		return NULL;

	/* Create a TnyCamelAccount so we can use 
	 * tny_camel_account_get_supported_secure_authentication(): */
	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry, protocol_type);
	tny_account = NULL;
	if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
		tny_account = modest_account_protocol_create_account (MODEST_ACCOUNT_PROTOCOL (protocol));
	}

	if (!tny_account) {
		g_printerr ("%s could not create tny account.", __FUNCTION__);
		return NULL;
	}

	/* Set proto, so that the prepare_func() vfunc will work when
	 * we call set_session(): */
	protocol = modest_protocol_registry_get_protocol_by_type (protocol_registry, protocol_type);
	tny_account_set_id (tny_account, "temp_account");
	tny_account_set_proto (tny_account, modest_protocol_get_name (protocol));
	tny_account_set_hostname (tny_account, hostname);
	tny_account_set_user (tny_account, username);

	if(port > 0)
		tny_account_set_port (tny_account, port);

	/* Set the session for the account, so we can use it: */
	account_store = modest_runtime_get_account_store ();
	session = modest_tny_account_store_get_session (TNY_ACCOUNT_STORE (account_store));
	g_return_val_if_fail (session, NULL);
	tny_camel_account_set_session (TNY_CAMEL_ACCOUNT(tny_account), session);

	dialog = gtk_dialog_new_with_buttons(" ",
					     parent_window,
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     _("mcen_bd_dialog_cancel"),
					     GTK_RESPONSE_REJECT,
					     NULL);

	/* Ask camel to ask the server, asynchronously: */
	ModestGetSupportedAuthInfo *info = g_slice_new (ModestGetSupportedAuthInfo);
	info->result = &result;
	info->dialog = dialog;

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox),
	                  gtk_label_new(_("emev_ni_checking_supported_auth_methods")));
	progress = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(info->dialog)->vbox), progress);
	gtk_widget_show_all(info->dialog);

	KeepPulsing *pi = g_slice_new (KeepPulsing);
	pi->progress = progress;
	pi->not_finished = TRUE;

	/* Starts the pulsing of the progressbar */
	g_timeout_add (500, keep_pulsing, pi);

	tny_camel_account_get_supported_secure_authentication (TNY_CAMEL_ACCOUNT (tny_account),
							       on_camel_account_get_supported_secure_authentication,
							       NULL,
							       info);

	retval = gtk_dialog_run (GTK_DIALOG (info->dialog));

	pi->not_finished = FALSE;
	/* pi is freed in the timeout itself to avoid a GCond here */

	gtk_widget_destroy(dialog);

	/* Frees */
	tny_account_cancel (tny_account);
	g_object_unref (tny_account);

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
modest_utils_get_available_space (const gchar *maildir_path)
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
			size = 0;
		gnome_vfs_uri_unref (uri);
	} else {
		size = 0;
	}

	return (guint64) size;
}
static void
on_destroy_dialog (GtkDialog *dialog)
{
	gtk_widget_destroy (GTK_WIDGET(dialog));
	if (gtk_events_pending ())
		gtk_main_iteration ();
}

static guint
checked_modest_sort_criterium_view_add_sort_key (ModestSortCriteriumView *view, const gchar* key, guint max)
{
	gint sort_key;
	
	g_return_val_if_fail (view && MODEST_IS_SORT_CRITERIUM_VIEW(view), 0);
	g_return_val_if_fail (key, 0);
	
	sort_key = modest_sort_criterium_view_add_sort_key (view, key);
	if (sort_key < 0 || sort_key >= max) {
		g_warning ("%s: out of range (%d) for %s", __FUNCTION__, sort_key, key);
		return 0;
	} else
		return (guint)sort_key;	
}

static void
launch_sort_headers_dialog (GtkWindow *parent_window,
			    GtkDialog *dialog)
{
	ModestHeaderView *header_view = NULL;
	GList *cols = NULL;
	GtkSortType sort_type;
	gint sort_key;
	gint default_key = 0;
	gint result;
	gboolean outgoing = FALSE;
	gint current_sort_colid = -1;
	GtkSortType current_sort_type;
	gint attachments_sort_id;
	gint priority_sort_id;
	GtkTreeSortable *sortable;
	
	/* Get header window */
	if (MODEST_IS_MAIN_WINDOW (parent_window)) {
		header_view = MODEST_HEADER_VIEW(modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(parent_window),
										      MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW));
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (MODEST_IS_HEADER_WINDOW (parent_window)) {
		header_view = MODEST_HEADER_VIEW (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (parent_window)));
#endif

	}
	if (!header_view)
		return;
	
	/* Add sorting keys */
	cols = modest_header_view_get_columns (header_view);
	if (cols == NULL) 
		return;
#define SORT_ID_NUM 6
	int sort_model_ids[SORT_ID_NUM];
	int sort_ids[SORT_ID_NUM];

	outgoing = (GPOINTER_TO_INT (g_object_get_data(G_OBJECT(cols->data), MODEST_HEADER_VIEW_COLUMN))==
		    MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT);

	sort_key = checked_modest_sort_criterium_view_add_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), _("mcen_li_sort_sender_recipient"),
								    SORT_ID_NUM);
	if (outgoing) {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT;
	} else {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN;
	}

	sort_key = checked_modest_sort_criterium_view_add_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), _("mcen_li_sort_date"),
							    SORT_ID_NUM);
	if (outgoing) {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_SENT_DATE;
	} else {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_RECEIVED_DATE;
	}
	default_key = sort_key;

	sort_key = checked_modest_sort_criterium_view_add_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), _("mcen_li_sort_subject"),
								    SORT_ID_NUM);
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN;
	if (outgoing)
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT;
	else
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN;

	sort_key = checked_modest_sort_criterium_view_add_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), _("mcen_li_sort_attachment"),
								    SORT_ID_NUM);
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN;
	sort_ids[sort_key] = TNY_HEADER_FLAG_ATTACHMENTS;
	attachments_sort_id = sort_key;

	sort_key = checked_modest_sort_criterium_view_add_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), _("mcen_li_sort_size"),
								    SORT_ID_NUM);
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN;
	sort_ids[sort_key] = 0;

	sort_key = checked_modest_sort_criterium_view_add_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), _("mcen_li_sort_priority"),
								    SORT_ID_NUM);
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN;
	sort_ids[sort_key] = TNY_HEADER_FLAG_PRIORITY_MASK;
	priority_sort_id = sort_key;
	
	sortable = GTK_TREE_SORTABLE (gtk_tree_view_get_model (GTK_TREE_VIEW (header_view)));
	/* Launch dialogs */
	if (!gtk_tree_sortable_get_sort_column_id (sortable,
						   &current_sort_colid, &current_sort_type)) {
		modest_sort_criterium_view_set_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), default_key);
		modest_sort_criterium_view_set_sort_order (MODEST_SORT_CRITERIUM_VIEW (dialog), GTK_SORT_DESCENDING);
	} else {
		modest_sort_criterium_view_set_sort_order (MODEST_SORT_CRITERIUM_VIEW (dialog), current_sort_type);
		if (current_sort_colid == TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN) {
			gpointer flags_sort_type_pointer;
			flags_sort_type_pointer = g_object_get_data (G_OBJECT (cols->data), MODEST_HEADER_VIEW_FLAG_SORT);
			if (GPOINTER_TO_INT (flags_sort_type_pointer) == TNY_HEADER_FLAG_PRIORITY_MASK)
				modest_sort_criterium_view_set_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), priority_sort_id);
			else
				modest_sort_criterium_view_set_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), attachments_sort_id);
		} else {
			gint current_sort_keyid = 0;
			while (current_sort_keyid < SORT_ID_NUM) {
				if (sort_model_ids[current_sort_keyid] == current_sort_colid)
					break;
				else 
					current_sort_keyid++;
			}
			modest_sort_criterium_view_set_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog), current_sort_keyid);
		}
	}

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	if (result == GTK_RESPONSE_OK) {
		sort_key = modest_sort_criterium_view_get_sort_key (MODEST_SORT_CRITERIUM_VIEW (dialog));
		if (sort_key < 0 || sort_key > SORT_ID_NUM -1) {
			g_warning ("%s: out of range (%d)", __FUNCTION__, sort_key);
			sort_key = 0;
		}

		sort_type = modest_sort_criterium_view_get_sort_order (MODEST_SORT_CRITERIUM_VIEW (dialog));
		if (sort_model_ids[sort_key] == TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN) {
			g_object_set_data (G_OBJECT(cols->data), MODEST_HEADER_VIEW_FLAG_SORT,
					   GINT_TO_POINTER (sort_ids[sort_key]));
			/* This is a hack to make it resort rows always when flag fields are
			 * selected. If we do not do this, changing sort field from priority to
			 * attachments does not work */
			modest_header_view_sort_by_column_id (header_view, 0, sort_type);
		} else {
			gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (cols->data), 
								 sort_model_ids[sort_key]);
		}

		modest_header_view_sort_by_column_id (header_view, sort_model_ids[sort_key], sort_type);
		gtk_tree_sortable_sort_column_changed (sortable);
	}

	modest_widget_memory_save (modest_runtime_get_conf (),
				   G_OBJECT (header_view), MODEST_CONF_HEADER_VIEW_KEY);
	
	/* free */
	g_list_free(cols);	
}

void
modest_utils_run_sort_dialog (GtkWindow *parent_window,
			      ModestSortDialogType type)
{
	GtkWidget *dialog = NULL;

	/* Build dialog */
	dialog = modest_platform_create_sort_dialog (parent_window);
	if (dialog == NULL)
		return;
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (dialog), parent_window);

	/* Fill sort keys */
	switch (type) {
	case MODEST_SORT_HEADERS:
		launch_sort_headers_dialog (parent_window, 
					    GTK_DIALOG(dialog));
		break;
	}
	
	/* Free */
	on_destroy_dialog (GTK_DIALOG(dialog));
}


gchar *
modest_images_cache_get_id (const gchar *account, const gchar *uri)
{
	GnomeVFSURI *vfs_uri;
	gchar *result;
 
	vfs_uri = gnome_vfs_uri_new (uri);
	if (vfs_uri == NULL)
		return NULL;
 
	result = g_strdup_printf ("%s__%x", account, gnome_vfs_uri_hash (vfs_uri));
	gnome_vfs_uri_unref (vfs_uri);
 
	return result;
}

gchar *
modest_utils_get_account_name_from_recipient (const gchar *from_header, gchar **mailbox)
{
	gchar *account_name = NULL;
	ModestAccountMgr *mgr = NULL;
	GSList *accounts = NULL, *node = NULL;

	if (mailbox)
		*mailbox = NULL;
	g_return_val_if_fail (from_header, NULL);

	mgr = modest_runtime_get_account_mgr ();
	accounts = modest_account_mgr_account_names (mgr, TRUE);
		
	for (node = accounts; node != NULL; node = g_slist_next (node)) {
		gchar *from;
		gchar *transport_account;

		if (!strcmp (from_header, node->data)) {
			account_name = g_strdup (node->data);
			break;
		}

		transport_account = modest_account_mgr_get_server_account_name (modest_runtime_get_account_mgr (),
										(const gchar *) node->data,
										TNY_ACCOUNT_TYPE_TRANSPORT);
		if (transport_account) {
			gchar *proto;
			proto = modest_account_mgr_get_string (mgr, transport_account, MODEST_ACCOUNT_PROTO, TRUE);

			if (proto != NULL) {
				ModestProtocol *protocol = 
					modest_protocol_registry_get_protocol_by_name (modest_runtime_get_protocol_registry (),
										       MODEST_PROTOCOL_REGISTRY_TRANSPORT_PROTOCOLS,
										       proto);
				if (protocol && MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
					ModestPairList *pair_list;
					ModestPair *pair;
					gchar *from_header_email =
						modest_text_utils_get_email_address (from_header);
					pair_list = modest_account_protocol_get_from_list (MODEST_ACCOUNT_PROTOCOL (protocol),
											   node->data);
					
					pair = modest_pair_list_find_by_first_as_string (pair_list, from_header_email);
					if (pair != NULL) {
						account_name = g_strdup (node->data);
						if (mailbox)
							*mailbox = g_strdup (from_header_email);
					}
					
					modest_pair_list_free (pair_list);
					
				}
				g_free (proto);
			}
			g_free (transport_account);
		}
		if (mailbox && *mailbox)
			break;

		from = 
			modest_account_mgr_get_from_string (mgr, node->data, NULL);
			
		if (from) {
			gchar *from_email = 
				modest_text_utils_get_email_address (from);
			gchar *from_header_email =
				modest_text_utils_get_email_address (from_header);
				
			if (from_email && from_header_email) {
				if (!modest_text_utils_utf8_strcmp (from_header_email, from_email, TRUE)) {
					account_name = g_strdup (node->data);
					g_free (from);
					g_free (from_email);
					break;
				}
			}
			g_free (from_email);
			g_free (from_header_email);
			g_free (from);
		}

			
	}
	g_slist_foreach (accounts, (GFunc) g_free, NULL);
	g_slist_free (accounts);

	return account_name;
}

void 
modest_utils_on_entry_invalid_character (ModestValidatingEntry *self, 
					 const gchar* character,
					 gpointer user_data)
{
	gchar *message = NULL;
	const gchar *show_char = NULL;

	if (character)
		show_char = character;
	else {
		show_char = "' '";
	}
	
	message = g_strdup_printf (_CS("ckdg_ib_illegal_characters_entered"), show_char);
	modest_platform_information_banner (GTK_WIDGET (self), NULL, message);
	g_free (message);
}

FILE*
modest_utils_open_mcc_mapping_file (gboolean from_lc_messages, gboolean *translated)
{
	FILE* result = NULL;
	const gchar* path;
	const gchar *env_list;
	gchar **parts, **node;

	if (from_lc_messages) {
		env_list = setlocale (LC_MESSAGES, NULL);
	} else {
		env_list = getenv ("LANG");
	}
	parts = g_strsplit (env_list, ":", 0);
	gchar *path1 = NULL;
	const gchar* path2 = MODEST_MCC_MAPPING;

	if (translated)
		*translated = TRUE;

	path = NULL;
	for (node = parts; path == NULL && node != NULL && *node != NULL && **node != '\0'; node++) {
		path1 = g_strdup_printf ("%s.%s", MODEST_OPERATOR_WIZARD_MCC_MAPPING, *node);
		if (access (path1, R_OK) == 0) {
			path = path1;
			break;
		} else {
			g_free (path1);
			path1 = NULL;
		}
	}
	g_strfreev (parts);

	if (path == NULL) {
		if (access (MODEST_OPERATOR_WIZARD_MCC_MAPPING, R_OK) == 0) {
			path = MODEST_OPERATOR_WIZARD_MCC_MAPPING;
			if (translated)
				*translated = FALSE;
		} else if (access (path2, R_OK) == 0) {
			path = path2;
		} else {
			g_warning ("%s: neither '%s' nor '%s' is a readable mapping file",
				   __FUNCTION__, path1, path2);
			goto end;
		}
	}

	result = fopen (path, "r");
	if (!result) {
		g_warning ("%s: error opening mapping file '%s': %s",
			   __FUNCTION__, path, strerror(errno));
		goto end;
	}
 end:
	g_free (path1);
	return result;
}

/* cluster mcc's, based on the list
 * http://en.wikipedia.org/wiki/Mobile_country_code
 */
static int
effective_mcc (gint mcc)
{
	switch (mcc) {
	case 405: return 404; /* india */
	case 441: return 440; /* japan */	
	case 235: return 234; /* united kingdom */
	case 311:
	case 312:
	case 313:
	case 314:
	case 315:
	case 316: return 310; /* united states */
	default:  return mcc;
	}
}

/* each line is of the form:
   xxx    logical_id

  NOTE: this function is NOT re-entrant, the out-param country
  are static strings that should NOT be freed. and will change when
  calling this function again

  also note, this function will return the "effective mcc", which
  is the normalized mcc for a country - ie. even if the there
  are multiple entries for the United States with various mccs,
  this function will always return 310, even if the real mcc parsed
  would be 314. see the 'effective_mcc' function above.
*/
static int
parse_mcc_mapping_line (const char* line,  char** country)
{
	char mcc[4];  /* the mcc code, always 3 bytes*/
	gchar *iter, *tab, *final;

	if (!line) {
		*country = NULL;
		return 0;
	}

	/* Go to the first tab (Country separator) */
	tab = g_utf8_strrchr (line, -1, '\t');
	if (!tab)
		return 0;

	*country = g_utf8_find_next_char (tab, NULL);

	/* Replace by end of string. We need to use strlen, because
	   g_utf8_strrchr expects bytes and not UTF8 characters  */
	final = g_utf8_strrchr (tab, strlen (tab) + 1, '\n');
	if (G_LIKELY (final))
		*final = '\0';
	else
		tab[strlen(tab) - 1] = '\0';

	/* Get MCC code */
	mcc[0] = g_utf8_get_char (line);
	iter = g_utf8_find_next_char (line, NULL);
	mcc[1] = g_utf8_get_char (iter);
	iter = g_utf8_find_next_char (iter, NULL);
	mcc[2] = g_utf8_get_char (iter);
	mcc[3] = '\0';

	return effective_mcc ((int) strtol ((const char*)mcc, NULL, 10));
}

#define MCC_FILE_MAX_LINE_LEN 128 /* max length of a line in MCC file */

/** Note that the mcc_mapping file is installed 
 * by the operator-wizard-settings package.
 */
GtkTreeModel *
modest_utils_create_country_model (void)
{
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gtk_list_store_new (2,  G_TYPE_STRING, G_TYPE_INT));

	return model;
}

void
modest_utils_fill_country_model (GtkTreeModel *model, gint *locale_mcc)
{
	gboolean translated;
	char line[MCC_FILE_MAX_LINE_LEN];
	guint previous_mcc = 0;
	gchar *territory;
	GHashTable *country_hash;
	FILE *file;

	/* First we need to know our current region */
	file = modest_utils_open_mcc_mapping_file (FALSE, &translated);
	if (!file) {
		g_warning ("Could not open mcc_mapping file");
		return;
	}

	/* Get the territory specified for the current locale */
	territory = nl_langinfo (_NL_ADDRESS_COUNTRY_NAME);

	while (fgets (line, MCC_FILE_MAX_LINE_LEN, file) != NULL) {
		int mcc;
		char *country = NULL;

		mcc = parse_mcc_mapping_line (line, &country);
		if (!country || mcc == 0) {
			g_warning ("%s: error parsing line: '%s'", __FUNCTION__, line);
			continue;
		}

		if (mcc == previous_mcc) {
			/* g_warning ("already seen: %s", line); */
			continue;
		}
		previous_mcc = mcc;

		if (!(*locale_mcc)) {
			if (translated) {
				if (!g_utf8_collate (country, territory))
					*locale_mcc = mcc;
			} else {
				gchar *translation = dgettext ("osso-countries", country);
				if (!g_utf8_collate (translation, territory))
					*locale_mcc = mcc;
			}
		}
	}
	fclose (file);

	/* Now we fill the model */
	file = modest_utils_open_mcc_mapping_file (TRUE, &translated);
	if (!file) {
		g_warning ("Could not open mcc_mapping file");
		return;
	}

	country_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	while (fgets (line, MCC_FILE_MAX_LINE_LEN, file) != NULL) {

		int mcc;
		char *country = NULL;
		GtkTreeIter iter;
		const gchar *name_translated;

		mcc = parse_mcc_mapping_line (line, &country);
		if (!country || mcc == 0) {
			g_warning ("%s: error parsing line: '%s'", __FUNCTION__, line);
			continue;
		}

		if (mcc == previous_mcc ||
		    g_hash_table_lookup (country_hash, country)) {
			g_debug ("already seen: '%s' %d", country, mcc);
			continue;
		}
		previous_mcc = mcc;

		g_hash_table_insert (country_hash, g_strdup (country), GINT_TO_POINTER (mcc));

		name_translated = dgettext ("osso-countries", country);

		/* Add the row to the model: */
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set(GTK_LIST_STORE (model), &iter, 
				   MODEST_UTILS_COUNTRY_MODEL_COLUMN_MCC, mcc, 
				   MODEST_UTILS_COUNTRY_MODEL_COLUMN_NAME, name_translated, 
				   -1);
	}


	g_hash_table_unref (country_hash);
	fclose (file);

	/* Fallback to Finland */
	if (!(*locale_mcc))
		*locale_mcc = 244;

	/* Sort the items: */
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), 
					      MODEST_UTILS_COUNTRY_MODEL_COLUMN_NAME, GTK_SORT_ASCENDING);
}

GList *
modest_utils_create_notification_list_from_header_list (TnyList *header_list)
{
	GList *new_headers_list;
	TnyIterator *iter;

	g_return_val_if_fail (TNY_IS_LIST (header_list), NULL);
	g_return_val_if_fail (tny_list_get_length (header_list) > 0, NULL);

	new_headers_list = NULL;
	iter = tny_list_create_iterator (header_list);
	while (!tny_iterator_is_done (iter)) {
		ModestMsgNotificationData *data;
		TnyHeader *header;
		TnyFolder *folder;

		header = (TnyHeader *) tny_iterator_get_current (iter);
		if (header) {
			folder = tny_header_get_folder (header);

			if (folder) {
				gchar *uri, *uid;

				uid = tny_header_dup_uid (header);
				uri = g_strdup_printf ("%s/%s",
						       tny_folder_get_url_string (folder),
						       uid);
				g_free (uid);

				/* Create data & add to list */
				data = g_slice_new0 (ModestMsgNotificationData);
				data->subject = tny_header_dup_subject (header);
				data->from = tny_header_dup_from (header);
				data->uri = uri;

				new_headers_list = g_list_append (new_headers_list, data);

				g_object_unref (folder);
			}
			g_object_unref (header);
		}
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	return new_headers_list;
}

static void
free_notification_data (gpointer data,
			gpointer user_data)
{
	ModestMsgNotificationData *notification_data  = (ModestMsgNotificationData *) data;

	g_free (notification_data->from);
	g_free (notification_data->subject);
	g_free (notification_data->uri);

	g_slice_free (ModestMsgNotificationData, notification_data);
}

void
modest_utils_free_notification_list (GList *notification_list)
{
	g_return_if_fail (g_list_length (notification_list) > 0);

	g_list_foreach (notification_list, free_notification_data, NULL);
	g_list_free (notification_list);
}

void
modest_utils_flush_send_queue (const gchar *account_id)
{
	TnyTransportAccount *account;

	/* Get the transport account */
	account = (TnyTransportAccount *)
		modest_tny_account_store_get_server_account (modest_runtime_get_account_store (),
							     account_id,
							     TNY_ACCOUNT_TYPE_TRANSPORT);
	if (account) {
		ModestMailOperation *wakeup_op;
		ModestTnySendQueue *send_queue = modest_runtime_get_send_queue (account, TRUE);

		/* Flush it! */
		wakeup_op = modest_mail_operation_new (NULL);
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
						 wakeup_op);
		modest_mail_operation_queue_wakeup (wakeup_op, send_queue);

		g_object_unref (account);
	}
}

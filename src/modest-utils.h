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


#ifndef __MODEST_UTILS_H__
#define __MODEST_UTILS_H__

#include <gtk/gtk.h>
#include <stdio.h> /* for FILE* */
#include <tny-fs-stream.h>
#include <modest-protocol.h>
#include "widgets/modest-validating-entry.h"

typedef enum {
	MODEST_UTILS_GET_SUPPORTED_SECURE_AUTHENTICATION_ERROR_CANCELED
} ModestUtilsGetSupportedSecureAuthenticationError;

typedef enum _ModestSortDialogType {
	MODEST_SORT_HEADERS,
} ModestSortDialogType;

typedef struct _ModestMsgNotificationData {
	gchar *subject;
	gchar *from;
	gchar *uri;
} ModestMsgNotificationData;

GQuark modest_utils_get_supported_secure_authentication_error_quark (void);


/**
 * modest_utils_folder_writable:
 * @filename: a string
 *
 * Checks if @filename is in a writable folder
 *
 * Returns: %TRUE if @filename is writable, %FALSE otherwise
 */
gboolean modest_utils_folder_writable (const gchar *filename);

/**
 * modest_utils_file_exists:
 * @filename: a string
 *
 * Checks if @filename exists
 *
 * Returns: %TRUE if @filename currently exists, %FALSE otherwise
 */
gboolean modest_utils_file_exists (const gchar *filename);

/**
 * modest_utils_create_temp_stream:
 * @orig_name: a string with the original name of the extension, or %NULL
 * @hash_base: if %NULL, subdir will be random. If not, it will be a hash
 * of this.
 * @path: a string with the created file path. 
 *
 * Creates a temporary fs stream, in a random subdir of /tmp or /var/tmp.
 *
 * Returns: a #TnyFsStream, or %NULL if operation failed.  Note that it is 
 * possible that the file already exists but it is not writable. In that case,
 * the function would return NULL and @path would contain its path.
 */
TnyFsStream *modest_utils_create_temp_stream (const gchar *orig_name, const gchar *hash_base, gchar **path);

/**
 * modest_utils_get_supported_secure_authentication_methods:
 * @proto: the protocol
 * @hostname: hostname of the mail server to check
 * @port: mail server port
 * @username: username of the account to check for
 * @parent_window: a GtkWindow that can be used a parent for progress indication
 *
 * Get a list of supported authentication methods of the server
 *  
 * Returns: GList* of the method names. This list needs to be freed using g_list_free.
 *
 */

GList* modest_utils_get_supported_secure_authentication_methods (ModestProtocolType proto, 
	const gchar* hostname, gint port, const gchar* username, GtkWindow *parent_window, GError** error);

/** modest_show_information_note_in_main_context_and_forget:
 * @parent_window: The window for which the note should be transient.
 * @message: The text to show.
 * 
 * This calls modest_maemo_show_information_note_and_forget() in an idle handler.
 * This should be used when you are not sure that you are in the main context, 
 * because you should try to use GTK+ UI code only in the main context.
 */
void modest_utils_show_information_note_in_main_context_and_forget (GtkWindow *parent_window, const gchar* message);

/** modest_show_dialog_and_forget:
 * @parent_window: The window for which the note should be transient.
 * @message: The dialog to show.
 * 
 * Show the dialog and destroy it when it is closed, without 
 * blocking. Use this when you don't want to use gtk_dialog_run(), which might lead 
 * to hangs.
 */
void modest_utils_show_dialog_and_forget (GtkWindow *parent_window, GtkDialog *dialog);

/**
 * modest_toggle_action_set_active_block_notify:
 * @action: a #GtkToggleAction
 * @value: a #gboolean
 *
 * updates the toggle action active status, but blocking the notification of the changes.
 */
void modest_utils_toggle_action_set_active_block_notify (GtkToggleAction *action, gboolean value);

/**
 * modest_utils_run_sort_dialog:
 * @parent_window: the modest window the dialog has been requested from
 * @type: a #ModestSortDialogType
 *
 * raises a sort dialog for this window
 */
void modest_utils_run_sort_dialog (GtkWindow *parent_window, ModestSortDialogType type);


/**
 * modest_list_index:
 * @list: a #TnyList
 * @object: a #GObject
 *
 * finds the index of @object in @list
 *
 * Returns: the index of @object, or -1 if @object is not in @list
 */
gint modest_list_index (TnyList *list, GObject *object);

/**
 * modest_utils_get_available_space:
 * @maildir_path: the path of the maildir folder, or %NULL to
 * get the space available in local folders
 *
 * Obtains the space available in the local folder.
 *
 * Returns: a #guint64
 */
guint64 modest_utils_get_available_space (const gchar *maildir_path);

/**
 * modest_images_cache_get_id:
 * @account: a #TnyAccount
 * @uri: an uri string
 *
 * obtains the hash corresponding to an image external resource to be
 * stored in image cache.
 *
 * Returns: a newly allocated string containing the hash key
 */
gchar *modest_images_cache_get_id (const gchar *account, const gchar *uri);


/**
 * modest_utils_get_account_name_from_recipient:
 * @from: the result of a call to tny_header_dup_from
 *
 * returns the account name that corresponds to the given from address
 *
 * Returns: a newly allocated string containing the account name or
 * %NULL in case of error
 */
gchar *modest_utils_get_account_name_from_recipient (const gchar *from, gchar **mailbox);

void modest_utils_on_entry_invalid_character (ModestValidatingEntry *self, 
					      const gchar* character,
					      gpointer user_data);

/**
 * modest_utils_open_mcc_mapping_file:
 * @translated: a #gboolean pointer
 *
 * open the mcc mapping file, or %NULL if it fails. It also
 * sets @translated to %TRUE if the file is translated
 *
 * Returns: file ptr or %NULL in case of error
 */
FILE* modest_utils_open_mcc_mapping_file (gboolean from_lc_messages, gboolean *translated);

typedef enum {
	MODEST_UTILS_COUNTRY_MODEL_COLUMN_NAME = 0,
	MODEST_UTILS_COUNTRY_MODEL_COLUMN_MCC = 1,
	MODEST_UTILS_COUNTRY_MODEL_N_COLUMNS
} ModestUtilsCountryModelColumns;

/**
 * modest_utils_create_country_model:
 * @locale_mcc: a #gboolean
 *
 * creates the countries tree model used in wizard from the mcc
 * files.
 *
 * Returns: an empty #GtkTreeModel with the columns enumerated in
 *  #ModestUtilsCountryModelColumns
 */
GtkTreeModel *modest_utils_create_country_model (void);

/**
 * modest_utils_fill_country_model:
 * @model: a #GtkTreeModel (obtained with modest_utils_create_country_model
 * @locale_mcc: a #gboolean
 *
 * fills the countries tree model used in wizard from the mcc
 * files.
 *
 */
void modest_utils_fill_country_model (GtkTreeModel *model, gint *locale_mcc);

/**
 * modest_utils_create_notification_list_from_header_list:
 * @header_list: a #TnyList of #TnyHeader instances
 *
 * This function transforms a list of #TnyHeader objects into a list
 * that will be used to issue new email notifications
 *
 * Returns: a #GList of #ModestMsgNotificationData
 **/
GList *modest_utils_create_notification_list_from_header_list (TnyList *header_list);

/**
 * modest_utils_free_notification_list:
 * @notification_list: a #GList of #ModestMsgNotificationData
 *
 * Frees a list of #ModestMsgNotificationData structures
 **/
void  modest_utils_free_notification_list (GList *notification_list);
#endif /*__MODEST_MAEMO_UTILS_H__*/

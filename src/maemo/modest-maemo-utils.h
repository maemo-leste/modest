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


#ifndef __MODEST_MAEMO_UTILS_H__
#define __MODEST_MAEMO_UTILS_H__

#include <gtk/gtk.h>
#include <stdio.h> /* for FILE* */
#include <modest-protocol-info.h>
#include <tny-fs-stream.h>
#include <libosso.h>


#define MODEST_MAEMO_UTILS_MYDOCS_FOLDER "MyDocs"
#define MODEST_MAEMO_UTILS_DEFAULT_IMAGE_FOLDER ".images"

typedef enum {
	MODEST_MAEMO_UTILS_GET_SUPPORTED_SECURE_AUTHENTICATION_ERROR_CANCELED
} ModestMaemoUtilsGetSupportedSecureAuthenticationError;

GQuark modest_maemo_utils_get_supported_secure_authentication_error_quark (void);

/**
 * modest_maemo_utils_menubar_to_menu:
 * @ui_manager: a ui manager, with the menubar at "/MenuBar" 
 * 
 * convert a menubar description (in a GtkUIManager) in to a menu
 * 
 * Returns: a new menu, or NULL in case of error
 */
GtkWidget*    modest_maemo_utils_menubar_to_menu (GtkUIManager *ui_manager);


/**
 * modest_maemo_utils_get_device_name
 *
 * get the name for this device. Note: this queries the bluetooth
 * name over DBUS, and may block. The result will be available in
 * MODEST_CONF_DEVICE_NAME in ModestConf; it will be updated when it
 * changes
 * 
 */
void modest_maemo_utils_get_device_name (void);

/**
 * modest_maemo_utils_folder_writable:
 * @filename: a string
 *
 * Checks if @filename is in a writable folder
 *
 * Returns: %TRUE if @filename is writable, %FALSE otherwise
 */
gboolean modest_maemo_utils_folder_writable (const gchar *filename);

/**
 * modest_maemo_utils_file_exists:
 * @filename: a string
 *
 * Checks if @filename exists
 *
 * Returns: %TRUE if @filename currently exists, %FALSE otherwise
 */
gboolean modest_maemo_utils_file_exists (const gchar *filename);

/**
 * modest_maemo_utils_create_temp_stream:
 * @orig_name: a string with the original name of the extension, or %NULL
 * @hash_base: if %NULL, subdir will be random. If not, it will be a hash
 * of this.
 * @path: a string with the created file path
 *
 * Creates a temporary fs stream, in a random subdir of /tmp or /var/tmp.
 *
 * Returns: a #TnyFsStream, or %NULL if operation failed.
 */
TnyFsStream *modest_maemo_utils_create_temp_stream (const gchar *orig_name, const gchar *hash_base, gchar **path);

/**
 * modest_maemo_utils_get_supported_secure_authentication_methods:
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

GList* modest_maemo_utils_get_supported_secure_authentication_methods (ModestTransportStoreProtocol proto, 
	const gchar* hostname, gint port, const gchar* username, GtkWindow *parent_window, GError** error);

/**
 * modest_maemo_utils_setup_images_filechooser:
 * @chooser: a #GtkFileChooser
 *
 * Configures the default folder, and mime filter of a filechooser
 * for images.
 */
void modest_maemo_utils_setup_images_filechooser (GtkFileChooser *chooser);


/**
 * modest_maemo_utils_get_osso_context:
 *
 * get the osso_context pointer for this application
 * 
 * Return: the osso context pointer 
 */
osso_context_t *modest_maemo_utils_get_osso_context (void);



/** modest_maemo_show_information_note_in_main_context_and_forget:
 * @parent_window: The window for which the note should be transient.
 * @message: The text to show.
 * 
 * This calls modest_maemo_show_information_note_and_forget() in an idle handler.
 * This should be used when you are not sure that you are in the main context, 
 * because you should try to use GTK+ UI code only in the main context.
 */
void modest_maemo_show_information_note_in_main_context_and_forget (GtkWindow *parent_window, const gchar* message);

/** modest_maemo_show_dialog_and_forget:
 * @parent_window: The window for which the note should be transient.
 * @message: The dialog to show.
 * 
 * Show the dialog and destroy it when it is closed, without 
 * blocking. Use this when you don't want to use gtk_dialog_run(), which might lead 
 * to hangs.
 */
void modest_maemo_show_dialog_and_forget (GtkWindow *parent_window, GtkDialog *dialog);


/**
 * modest_maemo_open_mcc_mapping_file:
 *
 * open the mcc mapping file, or NULL if it fails
 *
 * Returns: file ptr or NULL in case of error
 */
FILE* modest_maemo_open_mcc_mapping_file (void);

/**
 * modest_maemo_set_thumbable_scrollbar:
 * @win: a scrollable window
 * @thumbable: set it to thumbable (TRUE) or small (FALSE)
 *
 * changes the thumbability of scrollbars in a scrollable window
 */
void modest_maemo_set_thumbable_scrollbar (GtkScrolledWindow *win, gboolean thumbable);


/**
 * modest_maemo_toggle_action_set_active_block_notify:
 * @action: a #GtkToggleAction
 * @value: a #gboolean
 *
 * updates the toggle action active status, but blocking the notification of the changes.
 */
void modest_maemo_toggle_action_set_active_block_notify (GtkToggleAction *action, gboolean value);


/**
 * modest_maemo_get_osso_context:
 *
 * retrieve the osso context for this application
 * 
 * Returns: the current osso_context_t ptr  
 */
osso_context_t* modest_maemo_utils_get_osso_context (void);

/**
 * modest_maemo_set_osso_context:
 *
 * remember the osso-context for this application 
 * 
 * @osso_context: a valid osso_context_t pointer
 *  
 */
void modest_maemo_utils_set_osso_context (osso_context_t *osso_context);

#endif /*__MODEST_MAEMO_UTILS_H__*/

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

#ifndef __MODEST_PLATFORM_H__
#define __MODEST_PLATFORM_H__

#include <tny-device.h>
#include "widgets/modest-window.h"
#include "widgets/modest-folder-view.h"
#include "widgets/modest-sort-criterium-view.h"
#ifdef MODEST_PLATFORM_MAEMO
#include <libosso.h>
#endif

G_BEGIN_DECLS

typedef enum _ModestConfirmationDialogType {
	MODEST_CONFIRMATION_DELETE_FOLDER,
} ModestConfirmationDialogType;

typedef enum _ModestConnectedVia {
	MODEST_CONNECTED_VIA_WLAN_OR_WIMAX = 1,
	MODEST_CONNECTED_VIA_ANY = 2,
} ModestConnectedVia;

#define MODEST_GTK_RESPONSE_NEW_FOLDER 1

/**
 * modest_platform_platform_init:
 *
 * platform specific initialization function
 *
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean modest_platform_init (int argc, char *argv[]);


/**
 * modest_platform_platform_init:
 *
 * platform specific un-initialization function
 * 
 * Returns: TRUE if succeeded, FALSE otherwise
 */
gboolean modest_platform_uninit (void);


/**
 * modest_platform_get_new_device:
 *
 * platform specific initialization function
 * 
 * Returns: TRUE if succeeded, FALSE otherwise
 */
TnyDevice*  modest_platform_get_new_device (void);


/**
 * modest_platform_get_file_icon_name:
 * @name: the name of the file, or NULL
 * @mime_type: the mime-type, or NULL
 * @effective_mime_type: out-param which receives the 'effective mime-type', ie., the mime type
 * that will be used. May be NULL if you're not interested in this. Note: the returned string
 * is newly allocated, and should be g_free'd when done with it.
 *
 * this function gets the icon for the file, based on the file name and/or the mime type,
 * using the following strategy:
 * (1) if mime_type != NULL and mime_type != application/octet-stream, find the
 *     the icon name for this mime type
 * (2) otherwise, guess the icon type from the file name, and then goto (1)
 *
 * Returns: the icon name
 */
gchar*  modest_platform_get_file_icon_name (const gchar* name, const gchar* mime_type,
					    gchar **effective_mime_type);

/**
 * modest_platform_activate_uri:
 * @uri: the uri to activate
 *
 * This function activates an URI
 *
 * Returns: %TRUE if successful, %FALSE if not.
 **/
gboolean modest_platform_activate_uri (const gchar *uri);

/**
 * modest_platform_activate_file:
 * @path: the path to activate
 * @mime_type: the mime type of the path, or %NULL to guess
 *
 * This function activates a file
 *
 * Returns: %TRUE if successful, %FALSE if not.
 **/
gboolean modest_platform_activate_file (const gchar *path, const gchar *mime_type);

/**
 * modest_platform_show_uri_popup:
 * @uri: an URI with the string
 *
 * This function show the popup of actions for an URI
 *
 * Returns: %TRUE if successful, %FALSE if not.
 **/
gboolean modest_platform_show_uri_popup (const gchar *uri);

/**
 * modest_platform_get_icon:
 * @name: the name of the icon
 * @size: the icon size, use MODEST_ICON_SMALL or MODEST_ICON_BIG
 *
 * this function returns an icon, or NULL in case of error 
 */
GdkPixbuf* modest_platform_get_icon (const gchar *name, guint icon_size);


/**
 * modest_platform_get_app_name:
 *
 * this function returns the name of the application. Do not modify.
 */
const gchar* modest_platform_get_app_name (void);


/**
 * modest_platform_run_new_folder_dialog:
 * @parent_window: a #GtkWindow
 * @suggested_parent: the parent of the new folder
 * @suggested_name: the suggested name for the new folder
 * @folder_name: the folder name selected by the user for the new folder
 * @parent: the chosen #TnyFolderStore (should be unreffed)
 * 
 * runs a "new folder" confirmation dialog. The dialog will suggest a
 * folder name which depends of the platform if the #suggested_name
 * parametter is NULL. If the user input a valid folder name it's
 * returned in the #folder_name attribute.
 * 
 * Returns: the #GtkResponseType returned by the dialog
 **/
gint      modest_platform_run_new_folder_dialog        (GtkWindow *parent_window,
							TnyFolderStore *suggested_parent,
							gchar *suggested_name,
							gchar **folder_name,
							TnyFolderStore **parent);

/**
 * modest_platform_run_rename_folder_dialog:
 * @parent_window: a #GtkWindow
 * @parent: the parent of the folder
 * @suggested_name: current name of the folder
 * @folder_name: the new folder name selected by the user for the folder
 * 
 * runs a "rename folder" confirmation dialog. If the user input a valid folder name it's
 * returned in the #folder_name attribute.
 * 
 * Returns: the #GtkResponseType returned by the dialog
 **/
gint      modest_platform_run_rename_folder_dialog        (ModestWindow *parent_window,
							   TnyFolderStore *parent,
							   const gchar *current_name,
							   gchar **folder_name);

/**
 * modest_platform_run_confirmation_dialog:
 * @parent_window: the parent #GtkWindow of the dialog
 * @message: the message to show to the user
 * 
 * runs a confirmation dialog
 * 
 * Returns: GTK_RESPONSE_OK or GTK_RESPONSE_CANCEL
 **/
gint      modest_platform_run_confirmation_dialog      (GtkWindow *parent_window,
							const gchar *message);


/**
 * modest_platform_run_confirmation_dialog_with_buttons:
 * @parent_window: the parent #GtkWindow of the dialog
 * @message: the message to show to the user
 * @button_accept: the text to show in the label of the accept button
 * @button_cancel: the text to show in the label of the cancel button
 * 
 * runs a confirmation dialog with the given values for the buttons
 * 
 * Returns: GTK_RESPONSE_OK or GTK_RESPONSE_CANCEL
 **/
gint
modest_platform_run_confirmation_dialog_with_buttons (GtkWindow *parent_window,
						      const gchar *message,
						      const gchar *button_accept,
						      const gchar *button_cancel);

/**
 * modest_platform_run_information_dialog:
 * @parent_window: the parent #GtkWindow of the dialog
 * @message: the message to show
 * @block: whether or not the dialog should block the main loop or not while running
 * 
 * shows an information dialog
 **/
void      modest_platform_run_information_dialog       (GtkWindow *parent_window,
							const gchar *message,
							gboolean block);

/**
 * modest_platform_create_sort_dialog:
 * @parent_window: the parent #GtkWindow of the dialog
 * 
 * creates a proper sort dialog for the platform
 *
 * Returns: a #GtkDialog implementing #ModestSortCriteriumView interface
 **/
GtkWidget *modest_platform_create_sort_dialog       (GtkWindow *parent_window);
		
/*
 * modest_platform_connect_and_wait:
 * @parent_window: the parent #GtkWindow for any interactive or progress feedback UI.
 * @account: The account to be used.
 * @return value: Whether a connection was made.
 * 
 * Attempts to make a connection, possibly showing interactive UI to achieve this.
 * This will return TRUE immediately if a connection is already open.
 * Otherwise, this function blocks until the connection attempt has either succeded or failed.
 * This also sets the account to online, if it is a store account, in case it has been set to offline mode.
 */		
gboolean modest_platform_connect_and_wait (GtkWindow *parent_window, TnyAccount *account);

		
/*
 * modest_platform_connect_and_wait_if_network_account:
 * @parent_window: the parent #GtkWindow for any interactive or progress feedback UI.
 * @account: The account that might need a connection in subsequent operations.
 * @return value: Whether a connection was made. Also returns TRUE if no connection is necessary.
 * 
 * Like modest_platform_connect_and_wait(), but only attempts to make a connection if the 
 * account uses the network. For instance, this just returns TRUE for local maildir accounts. 
 */
gboolean modest_platform_connect_and_wait_if_network_account (GtkWindow *parent_window, TnyAccount *account);

/*
 * modest_platform_connect_and_wait_if_network_account:
 * @parent_window: the parent #GtkWindow for any interactive or progress feedback UI.
 * @folder_store: The folder store (folder or account) that might need a connection in subsequent operations.
 * @return value: Whether a connection was made. Also returns TRUE if no connection is necessary.
 * 
 * Like modest_platform_connect_and_wait(), but only attempts to make a connection if the 
 * folder store uses the network. For instance, this just returns TRUE for local maildir folders. 
 */
gboolean modest_platform_connect_and_wait_if_network_folderstore (GtkWindow *parent_window, TnyFolderStore *folder_store);

/**
 * modest_platform_set_update_interval:
 * @minutes: The number of minutes between updates, or 0 for no updates.
 * 
 * Set the number of minutes between automatic updates of email accounts.
 * The platform will cause the send/receive action to happen repeatedly.
 **/
gboolean modest_platform_set_update_interval (guint minutes);

/**
 * modest_platform_get_global_settings_dialog:
 * @void: 
 * 
 * returns the global settings dialog
 * 
 * Return value: a new #ModestGlobalSettingsDialog dialog
 **/
GtkWidget* modest_platform_get_global_settings_dialog (void);

/**
 * modest_platform_push_email_notification:
 *
 * Notify the user when new e-mail arrives by playing a sound, making
 * a light blink, etc.
 */
void modest_platform_push_email_notification(void);

/**
 * modest_platform_on_new_headers_received:
 * @header_list: a list of #ModestMsgNotificationData
 * @show_visual: adds a visual notification 
 *
 * Performs the required actions when new headers are
 * received. Tipically it's useful for showing new email notifications
 **/
void modest_platform_on_new_headers_received (GList *URI_list,
					      gboolean show_visual);

/**
 * modest_platform_show_help:
 * @parent_window: 
 * @help_id: the help topic id to be shown in the help dialog
 * 
 * shows the application help dialog
 **/
void modest_platform_show_help (GtkWindow *parent_window, 
				const gchar *help_id);

/**
 * modest_platform_show_search_messages:
 * @parent_window: window the dialog will be child of
 *
 * shows the search messages dialog
 **/
void modest_platform_show_search_messages (GtkWindow *parent_window);

/**
 * modest_platform_show_addressbook:
 * @parent_window: window the dialog will be child of
 *
 * shows the addressbook
 **/
void modest_platform_show_addressbook (GtkWindow *parent_window);


GtkWidget* modest_platform_create_folder_view (TnyFolderStoreQuery *query);

void modest_platform_information_banner (GtkWidget *widget,
					 const gchar *icon_name,
					 const gchar *text);

void modest_platform_system_banner (GtkWidget *widget,
				    const gchar *icon_name,
				    const gchar *text);

/* Timeout is in miliseconds */
void modest_platform_information_banner_with_timeout (GtkWidget *parent,
						      const gchar *icon_name,
						      const gchar *text,
						      gint timeout);

GtkWidget *
modest_platform_animation_banner (GtkWidget *parent,
				  const gchar *annimation_name,
				  const gchar *text);
				  
/* TODO: This isn't platform-dependent, so this isn't the best place for this. */
/* Return TRUE immediately if the account is already online,
 * otherwise check every second for NUMBER_OF_TRIES seconds and return TRUE as 
 * soon as the account is online, or FALSE if the account does 
 * not become online in the NUMBER_OF_TRIES seconds.
 * This is useful when the D-Bus method was run immediately after 
 * the application was started (when using D-Bus activation), 
 * because the account usually takes a short time to go online.
 * The return value is maybe not very useful.
 */
gboolean modest_platform_check_and_wait_for_account_is_online(TnyAccount *account);



/**
 * modest_platform_run_certificate_confirmation_dialog:
 * @server_name: name of the server we get this dialog for
 * @certificate: the text representation of the certificate
 *
 * show the unknown-certificate confirmation dialog
 *
 *  Returns: TRUE (Ok-pressed) or FALSE (cancel pressed)
 **/
gboolean modest_platform_run_certificate_confirmation_dialog (const gchar* server_name,
							      const gchar *certificate);


/**
 * modest_platform_run_alert_dialog:
 * @prompt: prompt for the dialog
 * @is_question: is it a question dialog? 
 *
 * show the alert dialog for TnyAlerts
 * if it's a aquest
 *
 *  Returns: TRUE (Ok-pressed) or FALSE (cancel pressed)
 **/
gboolean modest_platform_run_alert_dialog (const gchar* prompt, gboolean is_question);


/**
 * modest_platform_remove_new_mail_notifications:
 * @only_visuals: remove only the visual notifications (like LEDs)
 *
 * Removes all the active new mail notifications
 **/
void modest_platform_remove_new_mail_notifications (gboolean only_visuals);

/* ModestConnectedPerformer:
 * @canceled: whether or not the user canceled
 * @err: whether an error occured during connecting, or NULL of not
 * @parent_window: the parent window or NULL
 * @account: the account or NULL
 * @user_data: your own user data
 * 
 * This is the callback for the modest_platform_connect_and_perform* functions
 */
typedef void (*ModestConnectedPerformer) (gboolean canceled, 
					  GError *err,
					  GtkWindow *parent_window, 
					  TnyAccount *account, 
					  gpointer user_data);

typedef struct {
	TnyAccount *dst_account;
	ModestConnectedPerformer callback;
	gpointer data;
} DoubleConnectionInfo;

/*
 * modest_platform_connect_and_perform:
 * @force: force the device to connect if we're offline, if FALSE then it does not connect if required
 * @parent_window: the parent #GtkWindow for any interactive or progress feedback UI.
 * @account: The account to be used.
 * @callback: will be called when finished, can be NULL
 * @user_data: user data for @callback
 * 
 * Attempts to make a connection, possibly showing interactive UI to achieve this.
 * This will return immediately if a connection is already open, which results in an instant
 * call of @callback. While making a connection, @account, if not NULL, will go online too. If
 * @account is NULL, only a network connection is made using the platform's device.
 */		
void modest_platform_connect_and_perform (GtkWindow *parent_window, 
					  gboolean force,
					  TnyAccount *account, 
					  ModestConnectedPerformer callback, 
					  gpointer user_data);
 		
/*
 * modest_platform_connect_if_remote_and_perform:
 * @parent_window: the parent #GtkWindow for any interactive or progress feedback UI.
 * @folder_store: The folder store (folder or account) that might need a connection in subsequent operations.
 * @callback: will be called when finished, can be NULL
 * @user_data: user data for @callback
 * 
 * Like modest_platform_connect_and_perform(), but only attempts to make a connection if the 
 * folder store uses the network. For instance, this just returns for local maildir folders. It
 * will in that case synchronously and instantly perform the @callback
 */
void modest_platform_connect_if_remote_and_perform (GtkWindow *parent_window, 
						    gboolean force,
						    TnyFolderStore *folder_store,
						    ModestConnectedPerformer callback, 
						    gpointer user_data);

/*
 * modest_platform_double_connect_and_perform:
 * @parent_window: the parent #GtkWindow for any interactive or progress feedback UI.
 * @folder_store: The folder store (folder or account) that might need a connection in subsequent operations.
 * @callback: will be called when finished, can be NULL
 * @info: 
 * 
 */
void modest_platform_double_connect_and_perform (GtkWindow *parent_window, 
						 gboolean force,
						 TnyFolderStore *folder_store,
						 DoubleConnectionInfo *info);

/**
 * modest_platform_get_account_settings_wizard:
 * @settings: a #ModestAccountSettings
 *
 * creates a dialog for editing @settings
 *
 * Returns: the newly created dialog.
 */
GtkWidget *modest_platform_get_account_settings_wizard (void);

ModestConnectedVia modest_platform_get_current_connection (void);




/**
 * modest_platform_check_memory_low:
 * 
 * @win: a ModestWindow, or NULL
 * @visuals: whether or not show visual information
 *
 * see if memory is too low for big memory consuming operations
 * optionally show a warning dialog if @win was provided
 *
 * Returns: TRUE if we're in lowmem state, FALSE otherwise
 */
gboolean modest_platform_check_memory_low (ModestWindow *win,
					   gboolean visuals);


/**
 * modest_platform_run_folder_details_dialog:
 * @parent_window: the parent #GtkWindow for the new dialog
 * @folder: the #TnyFolder whose details will be shown
 *
 * Shows the folder details dialog
 **/
void     modest_platform_run_folder_details_dialog (GtkWindow *parent_window,
						    TnyFolder *folder);

/**
 * modest_platform_run_header_details_dialog:
 * @parent_window: the parent #GtkWindow for the new dialog
 * @header: the #TnyHeader whose details will be shown
 * @async_get_size: %TRUE if size is obtained asynchronously from @msg
 * @msg: a #TnyMsg
 *
 * Shows the header details dialog
 **/
void     modest_platform_run_header_details_dialog (GtkWindow *parent_window,
						    TnyHeader *header,
						    gboolean async_get_size,
						    TnyMsg *msg);

/**
 * modest_platform_on_runtime_initialized:
 *
 * This function will be used by platforms to connect objects between
 * themselves once all the singletons have been created. So this
 * function MUST be called *before* modest_init
 **/
void     modest_platform_on_runtime_initialized ();

#ifdef MODEST_PLATFORM_MAEMO
/**
 * modest_platform_get_osso_context:
 *
 * Obtains the osso context pointer for the application
 *
 * Returns: the osso context pointer
 */
osso_context_t *modest_platform_get_osso_context (void);
#endif



GtkWidget* modest_platform_create_move_to_dialog (GtkWindow *parent_window,
						  GtkWidget **folder_view);

TnyList* modest_platform_get_list_to_move (ModestWindow *window);

G_END_DECLS

#endif /* __MODEST_PLATFORM_UTILS_H__ */

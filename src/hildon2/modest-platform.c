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

#include <config.h>
#include <glib/gi18n.h>

#include <modest-platform.h>
#include <modest-defs.h>
#include <modest-runtime.h>
#include <modest-main-window.h>
#include <modest-header-view.h>
#include "modest-hildon2-global-settings-dialog.h"
#include "modest-widget-memory.h"
#include <modest-hildon-includes.h>
#include <modest-maemo-utils.h>
#include <modest-utils.h>
#include <dbus_api/modest-dbus-callbacks.h>
#include <libosso.h>
#include <tny-maemo-conic-device.h>
#include <tny-camel-folder.h>
#include <tny-simple-list.h>
#include <tny-merge-folder.h>
#include <tny-error.h>
#include <tny-folder.h>
#include <tny-account-store-view.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmain.h>
#include <modest-text-utils.h>
#include "modest-tny-folder.h"
#include "modest-tny-account.h"
#include <string.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <modest-account-settings-dialog.h>
#include <modest-easysetup-wizard-dialog.h>
#include "modest-hildon2-sort-dialog.h"
#include <hildon/hildon.h>
#include <osso-mem.h>
#include "hildon2/modest-hildon2-details-dialog.h"
#include "hildon2/modest-hildon2-window-mgr.h"
#ifdef MODEST_USE_PROFILE
#include <keys_nokia.h>
#include <libprofile.h>
#endif
#include <canberra.h>
#include <modest-datetime-formatter.h>
#include "modest-header-window.h"
#include <modest-folder-window.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-ui-constants.h>
#include <modest-selector-picker.h>
#include <modest-icon-names.h>
#include <modest-count-stream.h>
#include <math.h>

#ifdef MODEST_HAVE_MCE
#include <mce/dbus-names.h>
#endif /*MODEST_HAVE_MCE*/

#ifdef MODEST_HAVE_ABOOK
#include <libosso-abook/osso-abook.h>
#endif /*MODEST_HAVE_ABOOK*/

#ifdef MODEST_HAVE_LIBALARM
#include <alarmd/libalarm.h> /* For alarm_event_add(), etc. */
#endif /*MODEST_HAVE_LIBALARM*/


#define HILDON_OSSO_URI_ACTION "uri-action"
#define URI_ACTION_COPY "copy:"
#define MODEST_NOTIFICATION_CATEGORY "email-message"
#define MODEST_NEW_MAIL_LIGHTING_PATTERN "PatternCommunicationEmail"
#ifdef MODEST_USE_PROFILE
#define PROFILE_MAIL_TONE PROFILEKEY_EMAIL_ALERT_TONE
#define PROFILE_MAIL_VOLUME PROFILEKEY_EMAIL_ALERT_VOLUME
#else
#define MAIL_TONE "message-new-email"
#endif

#define COMMON_FOLDER_DIALOG_ENTRY "entry"
#define COMMON_FOLDER_DIALOG_ACCOUNT_PICKER "account-picker"
#define FOLDER_PICKER_CURRENT_FOLDER "current-folder"
#define FOLDER_PICKER_ORIGINAL_ACCOUNT "original-account"
#define MODEST_ALARMD_APPID PACKAGE_NAME

static ca_context *ca_con = NULL;
static gboolean ca_con_opened = FALSE;


static void modest_platform_play_email_tone (void);


static void	
on_modest_conf_update_interval_changed (ModestConf* self, 
					const gchar *key, 
					ModestConfEvent event,
					ModestConfNotificationId id, 
					gpointer user_data)
{
	g_return_if_fail (key);
	
	if (strcmp (key, MODEST_CONF_UPDATE_INTERVAL) == 0) {
		const guint update_interval_minutes = 
			modest_conf_get_int (self, MODEST_CONF_UPDATE_INTERVAL, NULL);
		modest_platform_set_update_interval (update_interval_minutes);
	}
}



static gboolean
check_required_files (void)
{
	FILE *mcc_file = modest_utils_open_mcc_mapping_file ();

	if (!mcc_file) {
		g_printerr ("modest: check for mcc file (for LANG) failed\n");
		return FALSE;
	} else {
		fclose (mcc_file);
	}

	if (access(MODEST_PROVIDER_DATA_FILE, R_OK) != 0 &&
	    access(MODEST_FALLBACK_PROVIDER_DATA_FILE, R_OK) != 0) {
		g_printerr ("modest: cannot find providers data\n");
		return FALSE;
	}

	return TRUE;
}


/* the gpointer here is the osso_context. */
gboolean
modest_platform_init (int argc, char *argv[])
{
	osso_context_t *osso_context;
	osso_hw_state_t hw_state = { 0 };
	DBusConnection *con;
	GSList *acc_names;

	if (!check_required_files ()) {
		g_printerr ("modest: missing required files\n");
		return FALSE;
	}

	osso_context = modest_maemo_utils_get_osso_context();

	if ((con = osso_get_dbus_connection (osso_context)) == NULL) {
		g_printerr ("modest: could not get dbus connection\n");
		return FALSE;
	}

	/* Add a D-Bus handler to be used when the main osso-rpc 
	 * D-Bus handler has not handled something.
	 * We use this for D-Bus methods that need to use more complex types 
	 * than osso-rpc supports. 
	 */
	if (!dbus_connection_add_filter (con,
					 modest_dbus_req_filter,
					 NULL,
					 NULL)) {

		g_printerr ("modest: Could not add D-Bus filter\n");
		return FALSE;
	}

	/* Register our simple D-Bus callbacks, via the osso API: */
	osso_return_t result = osso_rpc_set_cb_f(osso_context, 
                               MODEST_DBUS_SERVICE, 
                               MODEST_DBUS_OBJECT, 
                               MODEST_DBUS_IFACE,
                               modest_dbus_req_handler, NULL /* user_data */);
    	if (result != OSSO_OK) {
       		g_printerr ("modest: Error setting D-BUS callback (%d)\n", result);
		return FALSE;
   	}

	/* Register hardware event dbus callback: */
    	hw_state.shutdown_ind = TRUE;
	osso_hw_set_event_cb(osso_context, NULL, NULL, NULL);

	/* Make sure that the update interval is changed whenever its gconf key 
	 * is changed */
	/* CAUTION: we're not using here the
	   modest_conf_listen_to_namespace because we know that there
	   are other parts of Modest listening for this namespace, so
	   we'll receive the notifications anyway. We basically do not
	   use it because there is no easy way to do the
	   modest_conf_forget_namespace */
	ModestConf *conf = modest_runtime_get_conf ();
	g_signal_connect (G_OBJECT(conf),
			  "key_changed",
			  G_CALLBACK (on_modest_conf_update_interval_changed), 
			  NULL);

	/* only force the setting of the default interval, if there are actually
	 * any accounts */
	acc_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr(), TRUE);
	if (acc_names) {
		/* Get the initial update interval from gconf: */
		on_modest_conf_update_interval_changed(conf, MODEST_CONF_UPDATE_INTERVAL,
						       MODEST_CONF_EVENT_KEY_CHANGED, 0, NULL);
		modest_account_mgr_free_account_names (acc_names);
	}

	
#ifdef MODEST_HAVE_ABOOK
	/* initialize the addressbook */
	if (!osso_abook_init (&argc, &argv, osso_context)) {
		g_printerr ("modest: failed to initialized addressbook\n");
		return FALSE;
	}
#endif /*MODEST_HAVE_ABOOK*/

	return TRUE;
}

gboolean
modest_platform_uninit (void)
{
	osso_context_t *osso_context =
		modest_maemo_utils_get_osso_context ();
	if (osso_context)
		osso_deinitialize (osso_context);

	return TRUE;
}




TnyDevice*
modest_platform_get_new_device (void)
{
	return TNY_DEVICE (tny_maemo_conic_device_new ());
}

gchar*
modest_platform_get_file_icon_name (const gchar* name, const gchar* mime_type,
				    gchar **effective_mime_type)
{
	GString *mime_str = NULL;
	gchar *icon_name  = NULL;
	gchar **icons, **cursor;
	
	if (!mime_type || g_ascii_strcasecmp (mime_type, "application/octet-stream") == 0) 
		mime_str = g_string_new (gnome_vfs_get_mime_type_for_name (name));
	else {
		mime_str = g_string_new (mime_type);
		g_string_ascii_down (mime_str);
	}
	
	icons = hildon_mime_get_icon_names (mime_str->str, NULL);
	
	for (cursor = icons; cursor; ++cursor) {
		if (!g_ascii_strcasecmp (*cursor, "gnome-mime-message") ||
		    !g_ascii_strcasecmp (*cursor, "gnome-mime-message-rfc822")) {
			icon_name = g_strdup ("qgn_list_messagin");
			break;
		} else if (gtk_icon_theme_has_icon (gtk_icon_theme_get_default(), *cursor)) {
			icon_name = g_strdup (*cursor);
			break;
		}
	}
	g_strfreev (icons);

	if (effective_mime_type)
		*effective_mime_type = g_string_free (mime_str, FALSE);
	else
		g_string_free (mime_str, TRUE);
	
	return icon_name;
}


static gboolean
checked_hildon_uri_open (const gchar *uri, HildonURIAction *action)
{
	GError *err = NULL;
	gboolean result;

	g_return_val_if_fail (uri, FALSE);
	
	result = hildon_uri_open (uri, action, &err);
	if (!result) {
		g_printerr ("modest: hildon_uri_open ('%s', %p) failed: %s",
			    uri, action,  err && err->message ? err->message : "unknown error");
		if (err)
			g_error_free (err);
	}
	return result;
}



gboolean 
modest_platform_activate_uri (const gchar *uri)
{
	HildonURIAction *action;
	gboolean result = FALSE;
	GSList *actions, *iter = NULL;
	
	g_return_val_if_fail (uri, FALSE);
	if (!uri)
		return FALSE;

	/* don't try to activate file: uri's -- they might confuse the user,
	 * and/or might have security implications */
	if (!g_str_has_prefix (uri, "file:")) {
		
		actions = hildon_uri_get_actions_by_uri (uri, -1, NULL);
		
		for (iter = actions; iter; iter = g_slist_next (iter)) {
			action = (HildonURIAction*) iter->data;
			if (action && strcmp (hildon_uri_action_get_service (action),
					      "com.nokia.modest") == 0) {
				result = checked_hildon_uri_open (uri, action);
				break;
			}
		}
		
		/* if we could not open it with email, try something else */
		if (!result)
			result = checked_hildon_uri_open (uri, NULL);	
	} 
	
	if (!result) {
		ModestWindow *parent =
			modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(), FALSE);
		hildon_banner_show_information (parent ? GTK_WIDGET(parent): NULL, NULL,
						_("mcen_ib_unsupported_link"));
		g_debug ("%s: cannot open uri '%s'", __FUNCTION__,uri);
	} 
	
	return result;
}

gboolean 
modest_platform_activate_file (const gchar *path, const gchar *mime_type)
{
	gint result = 0;
	DBusConnection *con;
	gchar *uri_path = NULL;
	
	uri_path = gnome_vfs_get_uri_from_local_path (path);	
	con = osso_get_dbus_connection (modest_maemo_utils_get_osso_context());
	
	if (mime_type)
		result = hildon_mime_open_file_with_mime_type (con, uri_path, mime_type);
	if (result != 1)
		result = hildon_mime_open_file (con, uri_path);
	if (result != 1)
		modest_platform_run_information_dialog (NULL, _("mcen_ni_noregistered_viewer"), FALSE);
	
	return result != 1;
}

typedef struct  {
	GSList *actions;
	gchar  *uri;
} ModestPlatformPopupInfo;

static gboolean
delete_uri_popup (GtkWidget *menu,
		  GdkEvent *event,
		  gpointer userdata)
{
	ModestPlatformPopupInfo *popup_info = (ModestPlatformPopupInfo *) userdata;

	g_free (popup_info->uri);
	hildon_uri_free_actions (popup_info->actions);

	return FALSE;
}

static void
activate_uri_popup_item (GtkMenuItem *menu_item,
			 gpointer userdata)
{
	GSList *node;
	ModestPlatformPopupInfo *popup_info = (ModestPlatformPopupInfo *) userdata;
	const gchar* action_name;

	action_name = g_object_get_data (G_OBJECT(menu_item), HILDON_OSSO_URI_ACTION);
	if (!action_name) {
		g_printerr ("modest: no action name defined\n");
		return;
	}

	/* special handling for the copy menu item -- copy the uri to the clipboard */
	/* if it's a copy thingy, the uri will look like 'copy:http://slashdot.org' */
	if (g_str_has_prefix (action_name, URI_ACTION_COPY)) {
		GtkClipboard *clipboard = gtk_clipboard_get (GDK_NONE);
		const gchar *uri = (const gchar *) popup_info->uri;

		/* Special case: ignore "mailto:" prefixes */
		if (g_str_has_prefix (uri, "mailto:"))
			uri = popup_info->uri + strlen ("mailto:");

		gtk_clipboard_set_text (clipboard, uri, strlen (uri));
		modest_platform_information_banner (NULL, NULL, _CS("ecoc_ib_edwin_copied"));
		return; /* we're done */
	}

	/* now, the real uri-actions... */
	for (node = popup_info->actions; node != NULL; node = g_slist_next (node)) {
		HildonURIAction *action = (HildonURIAction *) node->data;
		if (strcmp (action_name, hildon_uri_action_get_name (action))==0) {
			if (!checked_hildon_uri_open (popup_info->uri, action)) {
				ModestWindow *parent =
					modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(), FALSE);
				hildon_banner_show_information (parent ? GTK_WIDGET(parent): NULL, NULL,
								_("mcen_ib_unsupported_link"));
			}
			break;
		}
	}
}

gboolean
modest_platform_show_uri_popup (const gchar *uri)
{
	GSList *actions_list, *node;
	GtkWidget *menu;
	ModestPlatformPopupInfo *popup_info;
	GtkWidget *menu_item;

	if (uri == NULL)
		return FALSE;

	/* Create menu */
	menu = gtk_menu_new ();
	popup_info = g_new0 (ModestPlatformPopupInfo, 1);
	popup_info->uri = g_strdup (uri);

	/* don't add actions for file: uri's -- they might confuse the user,
	 * and/or might have security implications
	 * we still allow to copy the url though
	 */
	if (g_str_has_prefix (uri, "file:"))
		goto add_copy_link;

	actions_list = hildon_uri_get_actions_by_uri (uri, -1, NULL);
	if (!actions_list)
		goto add_copy_link;

	popup_info->actions = actions_list;
	for (node = actions_list; node != NULL; node = g_slist_next (node)) {
		const gchar *action_name;
		const gchar *translation_domain;
		HildonURIAction *action = (HildonURIAction *) node->data;
		action_name = hildon_uri_action_get_name (action);
		translation_domain = hildon_uri_action_get_translation_domain (action);
		menu_item = gtk_menu_item_new_with_label (dgettext(translation_domain, action_name));
		hildon_gtk_widget_set_theme_size (menu_item, MODEST_EDITABLE_SIZE);
		g_object_set_data (G_OBJECT(menu_item), HILDON_OSSO_URI_ACTION, (gpointer)action_name);  /* hack */
		g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (activate_uri_popup_item),
				  popup_info);

		if (hildon_uri_is_default_action (action, NULL)) {
			gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
		} else {
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		}
		gtk_widget_show (menu_item);
	}

 add_copy_link:
	/* Add the "Copy link" menu option */
	menu_item = gtk_menu_item_new_with_label (_UR("uri_link_copy_link_location"));
	hildon_gtk_widget_set_theme_size (menu_item, MODEST_EDITABLE_SIZE);
	g_object_set_data (G_OBJECT(menu_item), HILDON_OSSO_URI_ACTION, (gpointer) URI_ACTION_COPY);
	g_signal_connect (G_OBJECT (menu_item), "activate", (GCallback) activate_uri_popup_item, popup_info);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	/* and what to do when the link is deleted */
	g_signal_connect (G_OBJECT (menu), "delete-event", G_CALLBACK (delete_uri_popup), popup_info);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time ());

	return TRUE;
}


GdkPixbuf*
modest_platform_get_icon (const gchar *name, guint icon_size)
{
	GError *err = NULL;
	GdkPixbuf* pixbuf = NULL;
	GtkIconTheme *current_theme = NULL;

	g_return_val_if_fail (name, NULL);

	/* strlen == 0 is not really an error; it just
	 * means the icon is not available
	 */
	if (!name || strlen(name) == 0)
		return NULL;

	current_theme = gtk_icon_theme_get_default ();
	pixbuf = gtk_icon_theme_load_icon (current_theme, name, icon_size,
					   GTK_ICON_LOOKUP_NO_SVG,
					   &err);
	if (!pixbuf) {
		g_warning ("Error loading theme icon '%s': %s\n",
			    name, err->message);
		g_error_free (err);
	} 
	return pixbuf;
}

const gchar*
modest_platform_get_app_name (void)
{
	return _("mcen_ap_name");
}

static void
entry_insert_text (GtkEditable *editable,
		   const gchar *text,
		   gint         length,
		   gint        *position,
		   gpointer     data)
{
	gchar *chars;
	gint chars_length;

	chars = gtk_editable_get_chars (editable, 0, -1);
	chars_length = g_utf8_strlen (chars, -1);
	g_free (chars);

	/* Show WID-INF036 */
	if (chars_length >= 20) {
		hildon_banner_show_information  (gtk_widget_get_parent (GTK_WIDGET (data)), NULL,
						 _CS("ckdg_ib_maximum_characters_reached"));
	} else {
		if (modest_text_utils_is_forbidden_char (*text, FOLDER_NAME_FORBIDDEN_CHARS)) {
			/* Show an error */
			gchar *tmp, *msg;

			tmp = g_strndup (folder_name_forbidden_chars,
					 FOLDER_NAME_FORBIDDEN_CHARS_LENGTH);
			msg = g_strdup_printf (_CS("ckdg_ib_illegal_characters_entered"), tmp);
			hildon_banner_show_information  (gtk_widget_get_parent (GTK_WIDGET (data)),
							 NULL, msg);
			g_free (msg);
			g_free (tmp);
		} else {
			if (length >= 20) {
				hildon_banner_show_information  (gtk_widget_get_parent (GTK_WIDGET (data)), NULL,
								 _CS("ckdg_ib_maximum_characters_reached"));
			}
			/* Write the text in the entry if it's valid */
			g_signal_handlers_block_by_func (editable,
							 (gpointer) entry_insert_text, data);
			gtk_editable_insert_text (editable, text, length, position);
			g_signal_handlers_unblock_by_func (editable,
							   (gpointer) entry_insert_text, data);
		}
	}
	/* Do not allow further processing */
	g_signal_stop_emission_by_name (editable, "insert_text");
}

static void
entry_changed (GtkEditable *editable,
	       gpointer     user_data)
{
	gchar *chars;
	GtkWidget *ok_button;
	GList *buttons;

	buttons = gtk_container_get_children (GTK_CONTAINER (GTK_DIALOG (user_data)->action_area));
	ok_button = GTK_WIDGET (buttons->data);

	chars = gtk_editable_get_chars (editable, 0, -1);
	g_return_if_fail (chars != NULL);


	if (g_utf8_strlen (chars,-1) >= 20) {
		hildon_banner_show_information  (gtk_widget_get_parent (GTK_WIDGET (user_data)), NULL,
						 _CS("ckdg_ib_maximum_characters_reached"));
	}
	gtk_widget_set_sensitive (ok_button, modest_text_utils_validate_folder_name(chars));

	/* Free */
	g_list_free (buttons);
	g_free (chars);
}



static void
on_response (GtkDialog *dialog,
	     gint response,
	     gpointer user_data)
{
	GtkWidget *entry, *picker;
	TnyFolderStore *parent;
	const gchar *new_name;
	gboolean exists;

	if (response != GTK_RESPONSE_ACCEPT)
		return;

	/* Get entry */
	entry = g_object_get_data (G_OBJECT (dialog), COMMON_FOLDER_DIALOG_ENTRY);
	picker = g_object_get_data (G_OBJECT (dialog), COMMON_FOLDER_DIALOG_ACCOUNT_PICKER);

	parent = TNY_FOLDER_STORE (user_data);
	new_name = gtk_entry_get_text (GTK_ENTRY (entry));
	exists = FALSE;

	if (picker != NULL)
		parent = g_object_get_data (G_OBJECT (picker), FOLDER_PICKER_CURRENT_FOLDER);

	/* Look for another folder with the same name */
	if (!TNY_IS_MERGE_FOLDER (parent) &&
	    modest_tny_folder_has_subfolder_with_name (parent, new_name, TRUE))
		exists = TRUE;

	if (!exists) {
		if (TNY_IS_ACCOUNT (parent) &&
		    modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (parent)) &&
		    modest_tny_local_folders_account_folder_name_in_use (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (parent),
									 new_name)) {
			exists = TRUE;
		}
	}

	if (exists) {
		/* Show an error */
		hildon_banner_show_information (gtk_widget_get_parent (GTK_WIDGET (dialog)), 
						NULL, _CS("ckdg_ib_folder_already_exists"));
		/* Select the text */
		gtk_entry_select_region (GTK_ENTRY (entry), 0, -1);
		gtk_widget_grab_focus (entry);
		/* Do not close the dialog */
		g_signal_stop_emission_by_name (dialog, "response");
	}
}

typedef struct _FolderChooserData {
	TnyFolderStore *store;
	GtkWidget *dialog;
} FolderChooserData;

static void
folder_chooser_activated (ModestFolderView *folder_view,
			  TnyFolderStore *folder,
			  FolderChooserData *userdata)
{
	userdata->store = folder;
	gtk_dialog_response (GTK_DIALOG (userdata->dialog), GTK_RESPONSE_OK);
}

static TnyFolderStore *
folder_chooser_dialog_run (ModestFolderView *original,
			   TnyFolderStore *current,
			   GtkButton *picker)
{
	GtkWidget *folder_view;
	FolderChooserData userdata = {NULL, NULL};
	GtkWidget *pannable;
	const gchar *visible_id = NULL;

	userdata.dialog = gtk_dialog_new ();
	pannable = hildon_pannable_area_new ();
	folder_view = modest_platform_create_folder_view (NULL);

	gtk_window_set_title (GTK_WINDOW (userdata.dialog), _FM("ckdg_ti_change_folder"));

	modest_folder_view_copy_model (MODEST_FOLDER_VIEW (original),
				       MODEST_FOLDER_VIEW (folder_view));

	if (TNY_IS_ACCOUNT (current)) {
		/* Local folders and MMC account are always shown
		   along with the currently visible server account */
		if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (current)) ||
		    modest_tny_account_is_memory_card_account (TNY_ACCOUNT (current)))
			visible_id = g_object_get_data ((GObject *) picker, FOLDER_PICKER_ORIGINAL_ACCOUNT);
		else
			visible_id = tny_account_get_id (TNY_ACCOUNT (current));
	} else if (TNY_IS_FOLDER (current)) {
		TnyAccount *account;
		account = modest_tny_folder_get_account ((TnyFolder *) current);
		if (account) {
			if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (account)) ||
			    modest_tny_account_is_memory_card_account (TNY_ACCOUNT (account))) {
				visible_id = g_object_get_data ((GObject *) picker, FOLDER_PICKER_ORIGINAL_ACCOUNT);
			} else {
				visible_id = tny_account_get_id (account);
			}
			g_object_unref (account);
		}
	} else {
		visible_id =
			modest_folder_view_get_account_id_of_visible_server_account (MODEST_FOLDER_VIEW(original));
	}

	modest_folder_view_set_account_id_of_visible_server_account (MODEST_FOLDER_VIEW(folder_view),
								     visible_id);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (userdata.dialog)->vbox), pannable);
	gtk_container_add (GTK_CONTAINER (pannable), folder_view);
	gtk_widget_set_size_request (pannable, -1, 320);

	gtk_widget_show (folder_view);
	gtk_widget_show (pannable);
	gtk_widget_show (userdata.dialog);
	g_signal_connect (G_OBJECT (folder_view), "folder-activated", 
			  G_CALLBACK (folder_chooser_activated), 
			  (gpointer) &userdata);

	gtk_dialog_run (GTK_DIALOG (userdata.dialog));
	gtk_widget_destroy (userdata.dialog);

	return userdata.store;
}

static gchar *
folder_store_get_display_name (TnyFolderStore *store)
{
	if (TNY_IS_ACCOUNT (store)) {
		if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (store)))
			return modest_conf_get_string (modest_runtime_get_conf(),
						       MODEST_CONF_DEVICE_NAME, NULL);
		else
			return g_strdup (tny_account_get_name (TNY_ACCOUNT (store)));
	} else {
		gchar *fname;
		TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;

		fname = g_strdup (tny_folder_get_name (TNY_FOLDER (store)));
		type = tny_folder_get_folder_type (TNY_FOLDER (store));
		if (modest_tny_folder_is_local_folder (TNY_FOLDER (store)) ||
		    modest_tny_folder_is_memory_card_folder (TNY_FOLDER (store))) {
			type = modest_tny_folder_get_local_or_mmc_folder_type (TNY_FOLDER (store));
			if (type != TNY_FOLDER_TYPE_UNKNOWN) {
				g_free (fname);
				fname = g_strdup (modest_local_folder_info_get_type_display_name (type));
			}
		} else {
			/* Sometimes an special folder is reported by the server as
			   NORMAL, like some versions of Dovecot */
			if (type == TNY_FOLDER_TYPE_NORMAL ||
			    type == TNY_FOLDER_TYPE_UNKNOWN) {
				type = modest_tny_folder_guess_folder_type (TNY_FOLDER (store));
			}
		}

		if (type == TNY_FOLDER_TYPE_INBOX) {
			g_free (fname);
			fname = g_strdup (_("mcen_me_folder_inbox"));
		}
		return fname;
	}
}

GtkWidget *
get_image_for_folder_store (TnyFolderStore *store,
			    gint size)
{
	GdkPixbuf *pixbuf;
	const gchar *icon_name = NULL;
	GtkWidget *image = NULL;

	if (TNY_IS_ACCOUNT (store)) {
		if (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (store)))
			icon_name = MODEST_FOLDER_ICON_LOCAL_FOLDERS;
		else if (modest_tny_account_is_memory_card_account (TNY_ACCOUNT (store)))
			icon_name = MODEST_FOLDER_ICON_MMC;
		else
			icon_name = MODEST_FOLDER_ICON_ACCOUNT;
	} else {
		TnyFolderType type = modest_tny_folder_guess_folder_type (TNY_FOLDER (store));
		if (modest_tny_folder_is_remote_folder (TNY_FOLDER (store))) {
			switch (type) {
			case TNY_FOLDER_TYPE_INBOX:
				icon_name = MODEST_FOLDER_ICON_INBOX;
				break;
			default:
				icon_name = MODEST_FOLDER_ICON_REMOTE_FOLDER;
			}
		} else if (modest_tny_folder_is_local_folder (TNY_FOLDER (store))) {
			switch (type) {
			case TNY_FOLDER_TYPE_OUTBOX:
				icon_name = MODEST_FOLDER_ICON_OUTBOX;
				break;
			case TNY_FOLDER_TYPE_DRAFTS:
				icon_name = MODEST_FOLDER_ICON_DRAFTS;
				break;
			case TNY_FOLDER_TYPE_SENT:
				icon_name = MODEST_FOLDER_ICON_SENT;
				break;
			default:
				icon_name = MODEST_FOLDER_ICON_NORMAL;
			}
		} else if (modest_tny_folder_is_memory_card_folder (TNY_FOLDER (store))) {
			icon_name = MODEST_FOLDER_ICON_MMC_FOLDER;
		}
	}

	/* Set icon */
	pixbuf = modest_platform_get_icon (icon_name, size);

	if (pixbuf) {
		image = gtk_image_new_from_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}

	return image;
}

static void
folder_picker_set_store (GtkButton *button, TnyFolderStore *store)
{
	gchar *name;

	if (store == NULL) {
		g_object_set_data (G_OBJECT (button), FOLDER_PICKER_CURRENT_FOLDER, NULL);
	} else {
		GtkWidget *image;

		g_object_set_data_full (G_OBJECT (button), FOLDER_PICKER_CURRENT_FOLDER,
					g_object_ref (store),
					(GDestroyNotify) g_object_unref);
		name = folder_store_get_display_name (store);
		hildon_button_set_value (HILDON_BUTTON (button), name);
		g_free (name);

		/* Select icon */
		image = get_image_for_folder_store (store, MODEST_ICON_SIZE_SMALL);
		if (image)
			hildon_button_set_image (HILDON_BUTTON (button), image);
	}
}

/* Always returns DUPs so you must free the returned value */
static gchar *
get_next_folder_name (const gchar *suggested_name, 
		      TnyFolderStore *suggested_folder)
{
	const gchar *default_name = _FM("ckdg_va_new_folder_name_stub");
	unsigned int i;
	gchar *real_suggested_name;

	if (suggested_name !=NULL) {
		return g_strdup (suggested_name);
	}

	for(i = 0; i < 100; ++ i) {
		gboolean exists = FALSE;

		if (i == 0)
			real_suggested_name = g_strdup (default_name);
		else
			real_suggested_name = g_strdup_printf ("%s(%d)",
							       _FM("ckdg_va_new_folder_name_stub"),
							       i);
		exists = modest_tny_folder_has_subfolder_with_name (suggested_folder,
								    real_suggested_name,
								    TRUE);

		if (!exists)
			break;

		g_free (real_suggested_name);
	}

	/* Didn't find a free number */
	if (i == 100)
		real_suggested_name = g_strdup (default_name);

	return real_suggested_name;
}

typedef struct {
	ModestFolderView *folder_view;
	GtkEntry *entry;
} FolderPickerHelper;

static void
folder_picker_clicked (GtkButton *button,
		       FolderPickerHelper *helper)
{
	TnyFolderStore *store, *current;

	current = g_object_get_data (G_OBJECT (button), FOLDER_PICKER_CURRENT_FOLDER);

	store = folder_chooser_dialog_run (helper->folder_view, current, button);
	if (store) {
		const gchar *current_name;
		gboolean exists = FALSE;

		folder_picker_set_store (GTK_BUTTON (button), store);

		/* Update the name of the folder */
		current_name = gtk_entry_get_text (helper->entry);

		if (TNY_IS_FOLDER_STORE (store))
			exists = modest_tny_folder_has_subfolder_with_name (store,
									    current_name,
									    TRUE);
		if (exists) {
			gchar *new_name = get_next_folder_name (NULL, store);
			gtk_entry_set_text (helper->entry, new_name);
			g_free (new_name);
		}
	}
}

static GtkWidget *
folder_picker_new (TnyFolderStore *suggested, FolderPickerHelper *helper)
{
	GtkWidget *button;
	const gchar *acc_id = NULL;

	button = hildon_button_new (MODEST_EDITABLE_SIZE,
				    HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	hildon_button_set_alignment (HILDON_BUTTON (button), 0.0, 0.5, 1.0, 1.0);

	if (suggested) {

		folder_picker_set_store (GTK_BUTTON (button), suggested);

		if (TNY_IS_ACCOUNT (suggested)) {
			if (!modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (suggested)) &&
			    !modest_tny_account_is_memory_card_account (TNY_ACCOUNT (suggested)))
				acc_id = tny_account_get_id ((TnyAccount *) suggested);
		} else {
			TnyAccount *account = modest_tny_folder_get_account ((TnyFolder *) suggested);
			if (account) {
				acc_id = tny_account_get_id ((TnyAccount *) account);
				g_object_unref (account);
			}
		}
	}

	if (!acc_id)
		acc_id = modest_folder_view_get_account_id_of_visible_server_account (MODEST_FOLDER_VIEW(helper->folder_view));

	g_object_set_data_full (G_OBJECT (button), FOLDER_PICKER_ORIGINAL_ACCOUNT,
				g_strdup (acc_id), (GDestroyNotify) g_free);


	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (folder_picker_clicked),
			  helper);

	return button;
}


static gint
modest_platform_run_folder_common_dialog (GtkWindow *parent_window,
					  TnyFolderStore *suggested_parent,
					  const gchar *dialog_title,
					  const gchar *label_text,
					  const gchar *suggested_name,
					  gboolean show_name,
					  gboolean show_parent,
					  gchar **folder_name,
					  TnyFolderStore **parent)
{
	GtkWidget *accept_btn = NULL;
	GtkWidget *dialog, *entry = NULL, *label_entry = NULL,  *label_location = NULL, *hbox;
	GtkWidget *account_picker = NULL;
	GList *buttons = NULL;
	gint result;
	GtkSizeGroup *sizegroup;
	ModestFolderView *folder_view;
	ModestWindow *folder_window;
	ModestHildon2WindowMgr *window_mgr;
	FolderPickerHelper *helper = NULL;
	GtkWidget *top_vbox, *top_align;

	window_mgr = (ModestHildon2WindowMgr *) modest_runtime_get_window_mgr ();
	folder_window = modest_hildon2_window_mgr_get_folder_window (window_mgr);
	g_return_val_if_fail (MODEST_IS_FOLDER_WINDOW (folder_window), GTK_RESPONSE_NONE);
	
	folder_view = modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (folder_window));
	
	top_vbox = gtk_vbox_new (FALSE, 0);
	top_align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (top_align), 0, 0, MODEST_MARGIN_DOUBLE, 0);
	
	/* Ask the user for the folder name */
	dialog = gtk_dialog_new_with_buttons (dialog_title,
					      parent_window,
					      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT,
					      _FM("ckdg_bd_new_folder_dialog_ok"),
					      GTK_RESPONSE_ACCEPT,
					      NULL);

	/* Add accept button (with unsensitive handler) */
	buttons = gtk_container_get_children (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area));
	accept_btn = GTK_WIDGET (buttons->data);

	sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	if (show_name) {
		label_entry = gtk_label_new (label_text);
		entry = hildon_entry_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
		gtk_entry_set_max_length (GTK_ENTRY (entry), 20);

		gtk_misc_set_alignment (GTK_MISC (label_entry), 0.0, 0.5);
		gtk_size_group_add_widget (sizegroup, label_entry);
		
		if (suggested_name)
		  gtk_entry_set_text (GTK_ENTRY (entry), suggested_name);
		else
			gtk_entry_set_text (GTK_ENTRY (entry), _FM("ckdg_va_new_folder_name_stub"));
		gtk_entry_set_width_chars (GTK_ENTRY (entry),
					   MAX (g_utf8_strlen (gtk_entry_get_text (GTK_ENTRY (entry)), -1),
						g_utf8_strlen (_FM("ckdg_va_new_folder_name_stub"), -1)));
		gtk_entry_select_region (GTK_ENTRY (entry), 0, -1);
	}
	
	if (show_parent) {
	  
		label_location = gtk_label_new (_FM("ckdg_fi_new_folder_location"));

		gtk_misc_set_alignment (GTK_MISC (label_location), 0.0, 0.5);
		gtk_size_group_add_widget (sizegroup, label_location);

		helper = g_slice_new0 (FolderPickerHelper);
		helper->folder_view = folder_view;
		helper->entry = (GtkEntry *) entry;

		account_picker = folder_picker_new (suggested_parent, helper);
	}

	g_object_unref (sizegroup);
	
	/* Connect to the response method to avoid closing the dialog
	   when an invalid name is selected*/
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (on_response),
			  suggested_parent);
	
	if (show_name) {
		/* Track entry changes */
		g_signal_connect (entry,
				  "insert-text",
				  G_CALLBACK (entry_insert_text),
				  dialog);
		g_signal_connect (entry,
				  "changed",
				  G_CALLBACK (entry_changed),
				  dialog);
	}
	
	
	/* Some locales like pt_BR need this to get the full window
	   title shown */
	gtk_widget_set_size_request (GTK_WIDGET (dialog), 300, -1);
	
	/* Create the hbox */
	if (show_name) {
		hbox = gtk_hbox_new (FALSE, 12);
		gtk_box_pack_start (GTK_BOX (hbox), label_entry, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
		
		/* Add hbox to dialog */
		gtk_box_pack_start (GTK_BOX (top_vbox), 
				    hbox, FALSE, FALSE, 0);
		g_object_set_data (G_OBJECT (dialog), COMMON_FOLDER_DIALOG_ENTRY, entry);
	}

	if (show_parent) {
		hbox = gtk_hbox_new (FALSE, 12);
		gtk_box_pack_start (GTK_BOX (hbox), label_location, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), account_picker, TRUE, TRUE, 0);

		/* Add hbox to dialog */
		gtk_box_pack_start (GTK_BOX (top_vbox), 
				    hbox, FALSE, FALSE, 0);
		g_object_set_data (G_OBJECT (dialog), COMMON_FOLDER_DIALOG_ACCOUNT_PICKER, account_picker);
	}
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), parent_window);

	gtk_container_add (GTK_CONTAINER (top_align), top_vbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), top_align, TRUE, TRUE, 0);

	gtk_widget_show_all (GTK_WIDGET(dialog));

	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		if (show_name)
			*folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
		if (show_parent) {
			*parent = g_object_get_data (G_OBJECT (account_picker), FOLDER_PICKER_CURRENT_FOLDER);
			if (*parent)
				g_object_ref (*parent);
		}
	}

	gtk_widget_destroy (dialog);

	if (helper)
		g_slice_free (FolderPickerHelper, helper);

	while (gtk_events_pending ())
		gtk_main_iteration ();

	return result;
}

gint
modest_platform_run_new_folder_dialog (GtkWindow *parent_window,
				       TnyFolderStore *suggested_folder,
				       gchar *suggested_name,
				       gchar **folder_name,
				       TnyFolderStore **parent_folder)
{
	gchar *real_suggested_name = NULL;
	gint result;
	ModestTnyAccountStore *acc_store;
	TnyAccount *account;
	gboolean do_free = FALSE;

	real_suggested_name = get_next_folder_name ((const gchar *) suggested_name,
						    suggested_folder);

	/* In hildon 2.2 we always suggest the archive folder as parent */
	if (!suggested_folder) {
		acc_store = modest_runtime_get_account_store ();
		account = modest_tny_account_store_get_mmc_folders_account (acc_store);
		if (account) {
			suggested_folder = (TnyFolderStore *)
				modest_tny_account_get_special_folder (account,
								       TNY_FOLDER_TYPE_ARCHIVE);
			g_object_unref (account);
			account = NULL;
		}
	}

	/* If there is not archive folder then fallback to local folders account */
	if (!suggested_folder) {
		do_free = TRUE;
		suggested_folder = (TnyFolderStore *)
			modest_tny_account_store_get_local_folders_account (acc_store);
	}

	result = modest_platform_run_folder_common_dialog (parent_window,
							   suggested_folder,
							   _HL("ckdg_ti_new_folder"),
							   _FM("ckdg_fi_new_folder_name"),
							   real_suggested_name,
							   TRUE,
							   TRUE,
							   folder_name,
							   parent_folder);

	if (do_free)
		g_object_unref (suggested_folder);

	g_free(real_suggested_name);

	return result;
}

gint
modest_platform_run_rename_folder_dialog (GtkWindow *parent_window,
                                          TnyFolderStore *parent_folder,
                                          const gchar *suggested_name,
                                          gchar **folder_name)
{
	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent_folder), GTK_RESPONSE_REJECT);

	return modest_platform_run_folder_common_dialog (parent_window, 
							 parent_folder,
							 _HL("ckdg_ti_rename_folder"),
							 _HL("ckdg_fi_rename_name"),
							 suggested_name,
							 TRUE,
							 FALSE,
							 folder_name,
							 NULL);
}



static void
on_destroy_dialog (GtkWidget *dialog)
{
	/* This could happen when the dialogs get programatically
	   hidden or destroyed (for example when closing the
	   application while a dialog is being shown) */
	if (!GTK_IS_WIDGET (dialog))
		return;

	gtk_widget_destroy (dialog);

	if (gtk_events_pending ())
		gtk_main_iteration ();
}

gint
modest_platform_run_confirmation_dialog (GtkWindow *parent_window,
					 const gchar *message)
{
	GtkWidget *dialog;
	gint response;
	
	dialog = hildon_note_new_confirmation (parent_window, message);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), parent_window);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	on_destroy_dialog (dialog);

	return response;
}

gint
modest_platform_run_confirmation_dialog_with_buttons (GtkWindow *parent_window,
						      const gchar *message,
						      const gchar *button_accept,
						      const gchar *button_cancel)
{
	GtkWidget *dialog;
	gint response;
	
	dialog = hildon_note_new_confirmation_add_buttons (parent_window, message,
							   button_accept, GTK_RESPONSE_ACCEPT,
							   button_cancel, GTK_RESPONSE_CANCEL,
							   NULL);

	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), parent_window);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	on_destroy_dialog (dialog);

	return response;
}
	
void
modest_platform_run_information_dialog (GtkWindow *parent_window,
					const gchar *message,
					gboolean block)
{
	GtkWidget *note;
	
	note = hildon_note_new_information (parent_window, message);
	if (block)
		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
					     GTK_WINDOW (note), parent_window);
	
	if (block) {
		gtk_dialog_run (GTK_DIALOG (note));
	
		on_destroy_dialog (note);
	} else {
		g_signal_connect_swapped (note,
					  "response", 
					  G_CALLBACK (on_destroy_dialog),
					  note);

		gtk_widget_show_all (note);
	}
}

typedef struct _ConnectAndWaitData {
	GMutex *mutex;
	GMainLoop *wait_loop;
	gboolean has_callback;
	gulong handler;
} ConnectAndWaitData;


static void
quit_wait_loop (TnyAccount *account,
		ConnectAndWaitData *data) 
{
	/* Set the has_callback to TRUE (means that the callback was
	   executed and wake up every code waiting for cond to be
	   TRUE */
	g_mutex_lock (data->mutex);
	data->has_callback = TRUE;
	if (data->wait_loop)
		g_main_loop_quit (data->wait_loop);
	g_mutex_unlock (data->mutex);
}

static void
on_connection_status_changed (TnyAccount *account, 
			      TnyConnectionStatus status,
			      gpointer user_data)
{
	TnyConnectionStatus conn_status;
	ConnectAndWaitData *data;
			
	/* Ignore if reconnecting or disconnected */
	conn_status = tny_account_get_connection_status (account);
	if (conn_status == TNY_CONNECTION_STATUS_RECONNECTING ||
	    conn_status == TNY_CONNECTION_STATUS_DISCONNECTED)
		return;

	/* Remove the handler */
	data = (ConnectAndWaitData *) user_data;
	g_signal_handler_disconnect (account, data->handler);

	/* Quit from wait loop */
	quit_wait_loop (account, (ConnectAndWaitData *) user_data);
}

static void
on_tny_camel_account_set_online_cb (TnyCamelAccount *account, 
				    gboolean canceled, 
				    GError *err, 
				    gpointer user_data)
{
	/* Quit from wait loop */
	quit_wait_loop (TNY_ACCOUNT (account), (ConnectAndWaitData *) user_data);
}

gboolean 
modest_platform_connect_and_wait (GtkWindow *parent_window, 
				  TnyAccount *account)
{
	ConnectAndWaitData *data = NULL;
	gboolean device_online;
	TnyDevice *device;
	TnyConnectionStatus conn_status;
	gboolean user_requested;
	
	device = modest_runtime_get_device();
	device_online = tny_device_is_online (device);

	/* Whether the connection is user requested or automatically
	   requested, for example via D-Bus */
	user_requested = (parent_window) ? TRUE : FALSE;

	/* If there is no account check only the device status */
	if (!account) {
		if (device_online)
			return TRUE;
		else
			return tny_maemo_conic_device_connect (TNY_MAEMO_CONIC_DEVICE (device), 
							       NULL, user_requested);
	}

	/* Return if the account is already connected */
	conn_status = tny_account_get_connection_status (account);
	if (device_online && conn_status == TNY_CONNECTION_STATUS_CONNECTED)
		return TRUE;

	/* Create the helper */
	data = g_slice_new0 (ConnectAndWaitData);
	data->mutex = g_mutex_new ();
	data->has_callback = FALSE;

	/* Connect the device */
	if (!device_online) {
		/* Track account connection status changes */
		data->handler = g_signal_connect (account, "connection-status-changed",
						  G_CALLBACK (on_connection_status_changed),
						  data);
		/* Try to connect the device */
		device_online = tny_maemo_conic_device_connect (TNY_MAEMO_CONIC_DEVICE (device), 
								NULL, user_requested);

		/* If the device connection failed then exit */
		if (!device_online && data->handler)
			goto frees;
	} else {
		/* Force a reconnection of the account */
		tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (account), TRUE, 
					      on_tny_camel_account_set_online_cb, data);
	}

	/* Wait until the callback is executed */
	g_mutex_lock (data->mutex);
	if (!data->has_callback) {
		data->wait_loop = g_main_loop_new (g_main_context_new (), FALSE);
		gdk_threads_leave ();
		g_mutex_unlock (data->mutex);
		g_main_loop_run (data->wait_loop);
		g_mutex_lock (data->mutex);
		gdk_threads_enter ();
	}
	g_mutex_unlock (data->mutex);

 frees:
	if (g_signal_handler_is_connected (account, data->handler))
		g_signal_handler_disconnect (account, data->handler);
	g_mutex_free (data->mutex);
	g_main_loop_unref (data->wait_loop);
	g_slice_free (ConnectAndWaitData, data);

	conn_status = tny_account_get_connection_status (account);
	return (conn_status == TNY_CONNECTION_STATUS_CONNECTED)	? TRUE: FALSE;
}

gboolean 
modest_platform_connect_and_wait_if_network_account (GtkWindow *parent_window, TnyAccount *account)
{
	if (tny_account_get_account_type (account) == TNY_ACCOUNT_TYPE_STORE) {
		if (!modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (account))) {
			/* This must be a maildir account, which does not require a connection: */
			return TRUE;
		}
	}

	return modest_platform_connect_and_wait (parent_window, account);
}

gboolean 
modest_platform_connect_and_wait_if_network_folderstore (GtkWindow *parent_window, TnyFolderStore *folder_store)
{
	if (!folder_store)
		return TRUE; /* Maybe it is something local. */
		
	gboolean result = TRUE;
	if (TNY_IS_FOLDER (folder_store)) {
		/* Get the folder's parent account: */
		TnyAccount *account = tny_folder_get_account(TNY_FOLDER (folder_store));
		if (account != NULL) {
			result = modest_platform_connect_and_wait_if_network_account (NULL, account);
			g_object_unref (account);
		}
	} else if (TNY_IS_ACCOUNT (folder_store)) {
		/* Use the folder store as an account: */
		result = modest_platform_connect_and_wait_if_network_account (NULL, TNY_ACCOUNT (folder_store));
	}

	return result;
}

GtkWidget *
modest_platform_create_sort_dialog       (GtkWindow *parent_window)
{
	GtkWidget *dialog;

	dialog = modest_hildon2_sort_dialog_new (parent_window);

	return dialog;
}


gboolean 
modest_platform_set_update_interval (guint minutes)
{
#ifdef MODEST_HAVE_LIBALARM

	cookie_t alarm_cookie, *alarm_cookies;
	ModestConf *conf = modest_runtime_get_conf ();
	if (!conf)
		return FALSE;

	if (minutes > 0) {
		GSList *acc_names = modest_account_mgr_account_names (modest_runtime_get_account_mgr (), TRUE);
		if (!acc_names) {
			minutes = 0;
		} else {
			modest_account_mgr_free_account_names (acc_names);
		}
	}

	/* cookie_t alarm_cookie = modest_conf_get_int (conf, MODEST_CONF_ALARM_ID, NULL); */

	/* Delete any existing alarm, because we will replace it: */
	alarm_cookies = alarmd_event_query (0,0, 0,0, MODEST_ALARMD_APPID);
	if (alarm_cookies) {
		/* alarmd_event_query returns a zero terminated array */
		for (; *alarm_cookies != 0; alarm_cookies++) {
			alarmd_event_del (*alarm_cookies);
		}
		modest_conf_set_int (conf, MODEST_CONF_ALARM_ID, 0, NULL);
	}

	/* 0 means no updates: */
	if (minutes == 0)
		return TRUE;

	/* Register alarm: */

	/* Set the interval in alarm_event_t structure: */
	alarm_event_t *event = alarm_event_create ();
	alarm_event_add_actions (event, 1);
	alarm_action_t *action = alarm_event_get_action (event, 0);
	alarm_event_set_alarm_appid (event, MODEST_ALARMD_APPID);
	event->alarm_time = minutes * 60; /* seconds */

	/* Set recurrence every few minutes: */
	event->recur_secs = minutes*60;
	event->recur_count = -1; /* Means infinite */

	/* Specify what should happen when the alarm happens:
	 * It should call this D-Bus method: */

	action->dbus_path = g_strdup(MODEST_DBUS_OBJECT);
	action->dbus_interface = g_strdup (MODEST_DBUS_IFACE);
	action->dbus_service = g_strdup (MODEST_DBUS_SERVICE);
	action->dbus_name = g_strdup (MODEST_DBUS_METHOD_SEND_RECEIVE);
	action->flags = ALARM_ACTION_WHEN_TRIGGERED | ALARM_ACTION_TYPE_DBUS | ALARM_ACTION_DBUS_USE_ACTIVATION;

	/* Use ALARM_EVENT_NO_DIALOG: Otherwise, a dialog will be shown if 
	 * exec_name or dbus_path is NULL, even though we have specified no dialog text.
	 * Also use ALARM_EVENT_ACTIVATION so that modest is started (without UI) to get emails 
	 * This is why we want to use the Alarm API instead of just g_timeout_add().
	 * (The old maemo email-client did this, though it isn't specified in the UI spec.)
	 * ALARM_EVENT_CONNECTED will prevent the alarm from being called in case that the device is offline
         */
	event->flags = ALARM_EVENT_CONNECTED | ALARM_EVENT_RUN_DELAYED;

	alarm_cookie = alarmd_event_add (event);

	/* now, free it */
	alarm_event_delete (event);

	/* Store the alarm ID in GConf, so we can remove it later:
	 * This is apparently valid between application instances. */
	modest_conf_set_int (conf, MODEST_CONF_ALARM_ID, alarm_cookie, NULL);

	if (!alarm_cookie) {
	    /* Error */
	    g_warning ("Error setting alarm event. \n");

	    return FALSE;
	}
#endif /* MODEST_HAVE_LIBALARM */
	return TRUE;
}

void
modest_platform_push_email_notification(void)
{
	gboolean screen_on, app_in_foreground;

	/* Get the window status */
	app_in_foreground = hildon_program_get_is_topmost (hildon_program_get_instance ());

	screen_on = modest_window_mgr_screen_is_on (modest_runtime_get_window_mgr ());

	/* If the screen is on and the app is in the
	   foreground we don't show anything */
	if (!(screen_on && app_in_foreground)) {

		modest_platform_play_email_tone ();

		/* Activate LED. This must be deactivated by
		   modest_platform_remove_new_mail_notifications */
#ifdef MODEST_HAVE_MCE
		osso_rpc_run_system (modest_maemo_utils_get_osso_context (),
				     MCE_SERVICE,
				     MCE_REQUEST_PATH,
				     MCE_REQUEST_IF,
				     MCE_ACTIVATE_LED_PATTERN,
				     NULL,
				     DBUS_TYPE_STRING, MODEST_NEW_MAIL_LIGHTING_PATTERN,
				     DBUS_TYPE_INVALID);
#endif
	}
}

void
modest_platform_on_new_headers_received (GList *URI_list,
					 gboolean show_visual)
{
	/* Check if the user wants to show notifications or not */
	if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_NOTIFICATIONS, NULL))
		return;

	if (g_list_length (URI_list) == 0)
		return;

#ifdef MODEST_HAVE_HILDON_NOTIFY
	/* For any other case issue a notification */
	HildonNotification *notification;
	ModestMsgNotificationData *data;
	gint notif_id;
	gchar *from;
	TnyAccountStore *acc_store;
	TnyAccount *account;
	gchar *acc_name;

	data = (ModestMsgNotificationData *) URI_list->data;

	/* String is changed in-place. There is no need to
	   actually dup the data->from string but we just do
	   it in order not to modify the original contents */
	from = g_strdup (data->from);
	modest_text_utils_get_display_address (from);

	/* Create notification */
	notification = hildon_notification_new (from,
						data->subject,
						"qgn_list_messagin",
						MODEST_NOTIFICATION_CATEGORY);
	g_free (from);

	/* Add DBus action */
	hildon_notification_add_dbus_action(notification,
					    "default",
					    "Cancel",
					    MODEST_DBUS_SERVICE,
					    MODEST_DBUS_OBJECT,
					    MODEST_DBUS_IFACE,
					    MODEST_DBUS_METHOD_OPEN_MESSAGE,
					    G_TYPE_STRING, data->uri,
					    -1);

	/* Set the led pattern */
	if (data->time)
		notify_notification_set_hint_int32 (NOTIFY_NOTIFICATION (notification),
						    "time", data->time);

	notify_notification_set_hint_int32 (NOTIFY_NOTIFICATION (notification),
					    "dialog-type", 4);
	notify_notification_set_hint_string(NOTIFY_NOTIFICATION (notification),
					    "led-pattern",
					    MODEST_NEW_MAIL_LIGHTING_PATTERN);

	/* Make the notification persistent */
	notify_notification_set_hint_byte (NOTIFY_NOTIFICATION (notification),
					   "persistent", TRUE);

	/* Set the number of new notifications */
	notify_notification_set_hint_int32 (NOTIFY_NOTIFICATION (notification),
					    "amount", g_list_length (URI_list));

	/* Set the account of the headers */
	acc_store = (TnyAccountStore *) modest_runtime_get_account_store ();
	account = tny_account_store_find_account (acc_store, data->uri);
	acc_name = NULL;
	if (account) {
		acc_name = g_strdup (modest_tny_account_get_parent_modest_account_name_for_server_account (account));
		notify_notification_set_hint_string(NOTIFY_NOTIFICATION (notification),
						    "email-account",
						    acc_name);
		g_object_unref (account);
	}

	if (notify_notification_show (NOTIFY_NOTIFICATION (notification), NULL)) {
		GSList *notifications_list = NULL;

		/* Get previous notifications ids */
		if (acc_name) {
			notifications_list = modest_account_mgr_get_list (modest_runtime_get_account_mgr (), acc_name,
									  MODEST_ACCOUNT_NOTIFICATION_IDS,
									  MODEST_CONF_VALUE_INT, FALSE);
		}

		/* Save id in the list */
		g_object_get(G_OBJECT (notification), "id", &notif_id, NULL);
		notifications_list = g_slist_prepend (notifications_list, GINT_TO_POINTER(notif_id));

		/* We don't listen for the "closed" signal, because we
		   don't care about if the notification was removed or
		   not to store the list in gconf */

		/* Save the ids */
		if (acc_name)
			modest_account_mgr_set_list (modest_runtime_get_account_mgr (), acc_name,
						     MODEST_ACCOUNT_NOTIFICATION_IDS,
						     notifications_list, MODEST_CONF_VALUE_INT, FALSE);
			
		g_slist_free (notifications_list);
	} else {
		g_warning ("Failed to send notification");
	}
	g_free (acc_name);

#endif /*MODEST_HAVE_HILDON_NOTIFY*/
}

void
modest_platform_remove_new_mail_notifications (gboolean only_visuals, const gchar *acc_name) 
{
	if (only_visuals) {
#ifdef MODEST_HAVE_MCE
		osso_rpc_run_system (modest_maemo_utils_get_osso_context (),
				     MCE_SERVICE,
				     MCE_REQUEST_PATH,
				     MCE_REQUEST_IF,
				     MCE_DEACTIVATE_LED_PATTERN,
				     NULL,
				     DBUS_TYPE_STRING, MODEST_NEW_MAIL_LIGHTING_PATTERN,
				     DBUS_TYPE_INVALID);
#endif
		return;
	}

#ifdef MODEST_HAVE_HILDON_NOTIFY
	GSList *notif_list = NULL;

	/* Get previous notifications ids */
	notif_list = modest_account_mgr_get_list (modest_runtime_get_account_mgr (), 
						  acc_name,
						  MODEST_ACCOUNT_NOTIFICATION_IDS, 
						  MODEST_CONF_VALUE_INT, FALSE);

        while (notif_list) {
		gint notif_id;
		NotifyNotification *notif;

		/* Nasty HACK to remove the notifications, set the id
		   of the existing ones and then close them */
		notif_id = GPOINTER_TO_INT(notif_list->data);
		notif = notify_notification_new("dummy", NULL, NULL, NULL);
		g_object_set(G_OBJECT(notif), "id", notif_id, NULL);

		/* Close the notification, note that some ids could be
		   already invalid, but we don't care because it does
		   not fail */
		notify_notification_close(notif, NULL);
		g_object_unref(notif);

		/* Delete the link, it's like going to the next */
		notif_list = g_slist_delete_link (notif_list, notif_list);
        }

	/* Save the ids */
	modest_account_mgr_set_list (modest_runtime_get_account_mgr (), acc_name,
				     MODEST_ACCOUNT_NOTIFICATION_IDS,
				     notif_list, MODEST_CONF_VALUE_INT, FALSE);

	g_slist_free (notif_list);

#endif /* MODEST_HAVE_HILDON_NOTIFY */
}



GtkWidget * 
modest_platform_get_global_settings_dialog ()
{
	return modest_hildon2_global_settings_dialog_new ();
}

void
modest_platform_show_help (GtkWindow *parent_window, 
			   const gchar *help_id)
{
	return;
}

void 
modest_platform_show_search_messages (GtkWindow *parent_window)
{
	osso_return_t result = OSSO_ERROR;
	
	result = osso_rpc_run_with_defaults (modest_maemo_utils_get_osso_context(),
					     "osso_global_search",
					     "search_email", NULL, DBUS_TYPE_INVALID);

	if (result != OSSO_OK) {
		g_warning ("%s: osso_rpc_run_with_defaults() failed.\n", __FUNCTION__);
	}
}

void 
modest_platform_show_addressbook (GtkWindow *parent_window)
{
	osso_return_t result = OSSO_ERROR;

	result = osso_rpc_run_with_defaults (modest_maemo_utils_get_osso_context(),
					     "osso_addressbook",
					     "top_application", NULL, DBUS_TYPE_INVALID);

	if (result != OSSO_OK) {
		g_warning ("%s: osso_rpc_run_with_defaults() failed.\n", __FUNCTION__);
	}
}

static GtkWidget *
modest_platform_create_folder_view_full (TnyFolderStoreQuery *query, gboolean do_refresh)
{
	GtkWidget *widget = modest_folder_view_new_full (query, do_refresh);

	/* Show one account by default */
	modest_folder_view_set_style (MODEST_FOLDER_VIEW (widget),
				      MODEST_FOLDER_VIEW_STYLE_SHOW_ONE);

	return widget;
}

GtkWidget *
modest_platform_create_folder_view (TnyFolderStoreQuery *query)
{
	return modest_platform_create_folder_view_full (query, TRUE);
}

void
banner_finish (gpointer data, GObject *object)
{
	ModestWindowMgr *mgr = (ModestWindowMgr *) data;
	modest_window_mgr_unregister_banner (mgr);
	g_object_unref (mgr);
}

void 
modest_platform_information_banner (GtkWidget *parent,
				    const gchar *icon_name,
				    const gchar *text)
{
	GtkWidget *banner_parent = NULL;
	ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();

	if (modest_window_mgr_get_num_windows (mgr) == 0)
		return;

	if (parent && GTK_IS_WINDOW (parent)) {
		/* If the window is the active one then show the
		   banner on top of this window */
		if (gtk_window_is_active (GTK_WINDOW (parent)))
			banner_parent = parent;
		/* If the window is not the topmost but it's visible
		   (it's minimized for example) then show the banner
		   with no parent */ 
		else if (GTK_WIDGET_VISIBLE (parent))
			banner_parent = NULL;
		/* If the window is hidden (like the main window when
		   running in the background) then do not show
		   anything */
		else 
			return;
	}

	modest_platform_system_banner (banner_parent, icon_name, text);
}

void 
modest_platform_system_banner (GtkWidget *parent,
			       const gchar *icon_name,
			       const gchar *text)
{
	GtkWidget *banner = NULL;
	ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();

	if (parent && GTK_IS_WINDOW (parent)) {
		if (!gtk_window_is_active (GTK_WINDOW (parent)))
			parent = NULL;
	}

	banner = hildon_banner_show_information (parent, icon_name, text);

	modest_window_mgr_register_banner (mgr);
	g_object_ref (mgr);
	g_object_weak_ref ((GObject *) banner, banner_finish, mgr);
}

void
modest_platform_information_banner_with_timeout (GtkWidget *parent,
						 const gchar *icon_name,
						 const gchar *text,
						 gint timeout)
{
	GtkWidget *banner;

	if (modest_window_mgr_get_num_windows (modest_runtime_get_window_mgr ()) == 0)
		return;

	banner = hildon_banner_show_information (parent, icon_name, text);
	hildon_banner_set_timeout(HILDON_BANNER(banner), timeout);
}

GtkWidget *
modest_platform_animation_banner (GtkWidget *parent,
				  const gchar *animation_name,
				  const gchar *text)
{
	GtkWidget *inf_note = NULL;

	g_return_val_if_fail (text != NULL, NULL);

	if (modest_window_mgr_get_num_windows (modest_runtime_get_window_mgr ()) == 0)
		return NULL;

	/* If the parent is not visible then do not show */
	if (parent && !GTK_WIDGET_VISIBLE (parent))
		return NULL;

	inf_note = hildon_banner_show_animation (parent, animation_name, text);

	return inf_note;
}

typedef struct
{
	GMainLoop* loop;
	TnyAccount *account;
	gboolean is_online;
	gint count_tries;
} CheckAccountIdleData;

#define NUMBER_OF_TRIES 10 /* Try approx every second, ten times. */

static gboolean 
on_timeout_check_account_is_online(CheckAccountIdleData* data)
{
	gboolean stop_trying = FALSE;
	g_return_val_if_fail (data && data->account, FALSE);

	if (data && data->account && 
		/* We want to wait until TNY_CONNECTION_STATUS_INIT has changed to something else,
		 * after which the account is likely to be usable, or never likely to be usable soon: */
		(tny_account_get_connection_status (data->account) != TNY_CONNECTION_STATUS_INIT) )
	{
		data->is_online = TRUE;

		stop_trying = TRUE;
	} else {
		/* Give up if we have tried too many times: */
		if (data->count_tries >= NUMBER_OF_TRIES) {
			stop_trying = TRUE;
		} else {
			/* Wait for another timeout: */
			++(data->count_tries);
		}
	}

	if (stop_trying) {
		/* Allow the function that requested this idle callback to continue: */
		if (data->loop)
			g_main_loop_quit (data->loop);

		if (data->account)
			g_object_unref (data->account);

		return FALSE; /* Don't call this again. */
	} else {
		return TRUE; /* Call this timeout callback again. */
	}
}

/* Return TRUE immediately if the account is already online,
 * otherwise check every second for NUMBER_OF_TRIES seconds and return TRUE as 
 * soon as the account is online, or FALSE if the account does 
 * not become online in the NUMBER_OF_TRIES seconds.
 * This is useful when the D-Bus method was run immediately after 
 * the application was started (when using D-Bus activation), 
 * because the account usually takes a short time to go online.
 * The return value is maybe not very useful.
 */
gboolean
modest_platform_check_and_wait_for_account_is_online(TnyAccount *account)
{
	gboolean is_online;

	g_return_val_if_fail (account, FALSE);

	if (!tny_device_is_online (modest_runtime_get_device())) {
		printf ("DEBUG: %s: device is offline.\n", __FUNCTION__);
		return FALSE;
	}

	/* The local_folders account never seems to leave TNY_CONNECTION_STATUS_INIT,
	 * so we avoid wait unnecessarily: */
	if (!modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (account)))
		return TRUE;

	/* The POP & IMAP store accounts seem to be TNY_CONNECTION_STATUS_DISCONNECTED, 
	 * and that seems to be an OK time to use them. Maybe it's just TNY_CONNECTION_STATUS_INIT that 
	 * we want to avoid. */
	if (tny_account_get_connection_status (account) != TNY_CONNECTION_STATUS_INIT)
		return TRUE;
		
	/* This blocks on the result: */
	CheckAccountIdleData *data = g_slice_new0 (CheckAccountIdleData);
	data->is_online = FALSE;
	data->account = account;
	g_object_ref (data->account);
	data->count_tries = 0;
		
	GMainContext *context = NULL; /* g_main_context_new (); */
	data->loop = g_main_loop_new (context, FALSE /* not running */);

	g_timeout_add (1000, (GSourceFunc)(on_timeout_check_account_is_online), data);

	/* This main loop will run until the idle handler has stopped it: */
	g_main_loop_run (data->loop);

	g_main_loop_unref (data->loop);
	/* g_main_context_unref (context); */

	is_online = data->is_online;
	g_slice_free (CheckAccountIdleData, data);
	
	return is_online;	
}



static void
on_cert_dialog_response (GtkDialog *dialog, gint response_id,  const gchar* cert)
{
	/* GTK_RESPONSE_HELP means we need to show the certificate */
	if (response_id == GTK_RESPONSE_APPLY) {
		GtkWidget *note;
		gchar *msg;
		
		/* Do not close the dialog */
		g_signal_stop_emission_by_name (dialog, "response");

		msg = g_strdup_printf (_("mcen_ni_view_unknown_certificate"), cert);	
		note = hildon_note_new_information (NULL, msg);
		gtk_dialog_run (GTK_DIALOG(note));
		gtk_widget_destroy (note);
	}
}


gboolean
modest_platform_run_certificate_confirmation_dialog (const gchar* server_name,
						     const gchar *certificate)
{
	GtkWidget *note;
	gint response;
	ModestWindow *win;
	HildonWindowStack *stack;

	stack = hildon_window_stack_get_default ();
	win = MODEST_WINDOW (hildon_window_stack_peek (stack));

	if (!win) {
		g_debug ("%s: don't show dialogs if there's no window shown; assuming 'Cancel'",
			 __FUNCTION__);
		return FALSE;
	}

	gchar *question = g_strdup_printf (_("mcen_nc_unknown_certificate"),
					   server_name);

	/* We use GTK_RESPONSE_APPLY because we want the button in the
	   middle of OK and CANCEL the same as the browser does for
	   example. With GTK_RESPONSE_HELP the view button is aligned
	   to the left while the other two to the right */
	note = hildon_note_new_confirmation_add_buttons  (
		(GtkWindow *) win,
		question,
		_HL("wdgt_bd_yes"),     GTK_RESPONSE_OK,
		_HL("wdgt_bd_view"),          GTK_RESPONSE_APPLY,   /* abusing this... */
		_HL("wdgt_bd_no"), GTK_RESPONSE_CANCEL,
		NULL, NULL);

	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     (GtkWindow *) note, (GtkWindow *) win);

	g_signal_connect (G_OBJECT(note), "response",
			  G_CALLBACK(on_cert_dialog_response),
			  (gpointer) certificate);

	response = gtk_dialog_run(GTK_DIALOG(note));

	on_destroy_dialog (note);
	g_free (question);

	return response == GTK_RESPONSE_OK;
}

gboolean
modest_platform_run_alert_dialog (const gchar* prompt,
				  gboolean is_question)
{
	ModestWindow *top_win;
	HildonWindowStack *stack;

	stack = hildon_window_stack_get_default ();
	top_win = MODEST_WINDOW (hildon_window_stack_peek (stack));

	if (!top_win) {
		g_debug ("%s: don't show dialogs if there's no window shown; assuming 'Cancel'",
			 __FUNCTION__);
		return FALSE;
	}

	gboolean retval = TRUE;
	if (is_question) {
		/* The Tinymail documentation says that we should show Yes and No buttons,
		 * when it is a question.
		 * Obviously, we need tinymail to use more specific error codes instead,
		 * so we know what buttons to show. */
		GtkWidget *dialog = GTK_WIDGET (hildon_note_new_confirmation (GTK_WINDOW (top_win), 
									      prompt));
		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
					     GTK_WINDOW (dialog), GTK_WINDOW (top_win));

		const int response = gtk_dialog_run (GTK_DIALOG (dialog));
		retval = (response == GTK_RESPONSE_YES) || (response == GTK_RESPONSE_OK);

		on_destroy_dialog (dialog);
	} else {
	 	/* Just show the error text and use the default response: */
	 	modest_platform_run_information_dialog (GTK_WINDOW (top_win), 
							prompt, FALSE);
	}
	return retval;
}

/***************/
typedef struct {
 	GtkWindow *parent_window;
 	ModestConnectedPerformer callback;
 	TnyAccount *account;
 	gpointer user_data;
 	gchar *iap;
 	TnyDevice *device;
} OnWentOnlineInfo;
 
static void 
on_went_online_info_free (OnWentOnlineInfo *info)
{
 	/* And if we cleanup, we DO cleanup  :-)  */
 	
 	if (info->device)
 		g_object_unref (info->device);
 	if (info->iap)
 		g_free (info->iap);
 	if (info->parent_window)
 		g_object_unref (info->parent_window);
 	if (info->account)
 		g_object_unref (info->account);
 	
 	g_slice_free (OnWentOnlineInfo, info);
 	
 	/* We're done ... */
 	
 	return;
}
 
static void
on_account_went_online (TnyCamelAccount *account, gboolean canceled, GError *err, gpointer user_data)
{
 	OnWentOnlineInfo *info = (OnWentOnlineInfo *) user_data;
 
 	/* Now it's really time to callback to the caller. If going online didn't succeed,
 	 * err will be set. We don't free it, Tinymail does that! If a cancel happened,
 	 * canceled will be set. Etcetera etcetera. */
 	
 	if (info->callback) {
 		info->callback (canceled, err, info->parent_window, info->account, info->user_data);
 	}
 	
 	/* This is our last call, we must cleanup here if we didn't yet do that */
 	on_went_online_info_free (info);
 	
 	return;
}
 
 
static void
on_conic_device_went_online (TnyMaemoConicDevice *device, const gchar* iap_id, gboolean canceled, GError *err, gpointer user_data)
{
 	OnWentOnlineInfo *info = (OnWentOnlineInfo *) user_data;
 	info->iap = g_strdup (iap_id);
 	
 	if (canceled || err || !info->account) {
 	
 		/* If there's a problem or if there's no account (then that's it for us, we callback
 		 * the caller's callback now. He'll have to handle err or canceled, of course.
 		 * We are not really online, as the account is not really online here ... */	
 		
 		/* We'll use the err and the canceled of this cb. TnyMaemoConicDevice delivered us
 		 * this info. We don't cleanup err, Tinymail does that! */
 		
 		if (info->callback) {
 			
 			/* info->account can be NULL here, this means that the user did not
 			 * provide a nice account instance. We'll assume that the user knows
 			 * what he's doing and is happy with just the device going online. 
 			 * 
 			 * We can't do magic, we don't know what account the user wants to
 			 * see going online. So just the device goes online, end of story */
 			
 			info->callback (canceled, err, info->parent_window, info->account, info->user_data);
 		}
 		
 	} else if (info->account) {
 		
 		/* If there's no problem and if we have an account, we'll put the account
 		 * online too. When done, the callback of bringing the account online
 		 * will callback the caller's callback. This is the most normal case. */
 
 		info->device = TNY_DEVICE (g_object_ref (device));
 		
 		tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (info->account), TRUE,
					      on_account_went_online, info);
 		
 		/* The on_account_went_online cb frees up the info, go look if you
 		 * don't believe me! (so we return here) */
 		
 		return;
 	}
 	
 	/* We cleanup if we are not bringing the account online too */
 	on_went_online_info_free (info);
 
 	return;	
}
 	
void 
modest_platform_connect_and_perform (GtkWindow *parent_window, 
				     gboolean force,
				     TnyAccount *account, 
				     ModestConnectedPerformer callback, 
				     gpointer user_data)
{
 	gboolean device_online;
 	TnyDevice *device;
 	TnyConnectionStatus conn_status;
 	OnWentOnlineInfo *info;
 	
 	device = modest_runtime_get_device();
 	device_online = tny_device_is_online (device);

 	/* If there is no account check only the device status */
 	if (!account) {
 		
 		if (device_online) {
 
 			/* We promise to instantly perform the callback, so ... */
 			if (callback) {
 				callback (FALSE, NULL, parent_window, account, user_data);
 			}
 			
 		} else {
 			
 			info = g_slice_new0 (OnWentOnlineInfo);
 			
 			info->iap = NULL;
 			info->device = NULL;
 			info->account = NULL;
 		
 			if (parent_window)
 				info->parent_window = (GtkWindow *) g_object_ref (parent_window);
 			else
 				info->parent_window = NULL;
 			info->user_data = user_data;
 			info->callback = callback;
 		
 			tny_maemo_conic_device_connect_async (TNY_MAEMO_CONIC_DEVICE (device), NULL,
							      force, on_conic_device_went_online, 
							      info);
 
 			/* We'll cleanup in on_conic_device_went_online */
 		}
 
 		/* The other code has no more reason to run. This is all that we can do for the
 		 * caller (he should have given us a nice and clean account instance!). We
 		 * can't do magic, we don't know what account he intends to bring online. So
 		 * we'll just bring the device online (and await his false bug report). */
 		
 		return;
 	}
 
 	
 	/* Return if the account is already connected */
 	
 	conn_status = tny_account_get_connection_status (account);
 	if (device_online && conn_status == TNY_CONNECTION_STATUS_CONNECTED) {
 
 		/* We promise to instantly perform the callback, so ... */
 		if (callback) {
 			callback (FALSE, NULL, parent_window, account, user_data);
 		}
 		
 		return;
 	}
 	
 	/* Else, we are in a state that requires that we go online before we
 	 * call the caller's callback. */
 	
 	info = g_slice_new0 (OnWentOnlineInfo);
 	
 	info->device = NULL;
 	info->iap = NULL;
 	info->account = TNY_ACCOUNT (g_object_ref (account));
 	
 	if (parent_window)
 		info->parent_window = (GtkWindow *) g_object_ref (parent_window);
 	else
 		info->parent_window = NULL;
 	
 	/* So we'll put the callback away for later ... */
 	
 	info->user_data = user_data;
 	info->callback = callback;
 	
 	if (!device_online) {
 
 		/* If also the device is offline, then we connect both the device 
 		 * and the account */
 		
 		tny_maemo_conic_device_connect_async (TNY_MAEMO_CONIC_DEVICE (device), NULL,
						      force, on_conic_device_went_online, 
						      info);
 		
 	} else {
 		
 		/* If the device is online, we'll just connect the account */
 		
 		tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (account), TRUE, 
					      on_account_went_online, info);
 	}
 
 	/* The info gets freed by on_account_went_online or on_conic_device_went_online
 	 * in both situations, go look if you don't believe me! */
 	
 	return;
}

void
modest_platform_connect_if_remote_and_perform (GtkWindow *parent_window, 
					       gboolean force,
					       TnyFolderStore *folder_store, 
					       ModestConnectedPerformer callback, 
					       gpointer user_data)
{
	TnyAccount *account = NULL;

	if (!folder_store ||
	    (TNY_IS_MERGE_FOLDER (folder_store) &&
	     (tny_folder_get_folder_type (TNY_FOLDER(folder_store)) == TNY_FOLDER_TYPE_OUTBOX))) {

 		/* We promise to instantly perform the callback, so ... */
 		if (callback) {
			GError *error = NULL;
			g_set_error (&error, TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_UNKNOWN,
				     "Unable to move or not found folder");
 			callback (FALSE, error, parent_window, NULL, user_data);
			g_error_free (error);
		}
 		return;

 	} else if (TNY_IS_FOLDER (folder_store)) {
 		/* Get the folder's parent account: */
 		account = tny_folder_get_account (TNY_FOLDER (folder_store));
 	} else if (TNY_IS_ACCOUNT (folder_store)) {
 		/* Use the folder store as an account: */
 		account = TNY_ACCOUNT (g_object_ref (folder_store));
 	}

	if (account && (tny_account_get_account_type (account) == TNY_ACCOUNT_TYPE_STORE)) {
 		if (!modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (account))) {
 			/* No need to connect a local account */
 			if (callback)
 				callback (FALSE, NULL, parent_window, account, user_data);

			goto clean;
 		}
 	}
  	modest_platform_connect_and_perform (parent_window, force, account, callback, user_data);

 clean:
	if (account)
		g_object_unref (account);
}

static void
src_account_connect_performer (gboolean canceled,
			       GError *err,
			       GtkWindow *parent_window,
			       TnyAccount *src_account,
			       gpointer user_data)
{
	DoubleConnectionInfo *info = (DoubleConnectionInfo *) user_data;

	if (canceled || err) {
		/* If there was any error call the user callback */
		info->callback (canceled, err, parent_window, src_account, info->data);
	} else {
		/* Connect the destination account */
		modest_platform_connect_if_remote_and_perform (parent_window, TRUE, 
							       TNY_FOLDER_STORE (info->dst_account),
							       info->callback, info->data);
	}

	/* Free the info object */
	g_object_unref (info->dst_account);
	g_slice_free (DoubleConnectionInfo, info);
}


void 
modest_platform_double_connect_and_perform (GtkWindow *parent_window, 
					    gboolean force,
					    TnyFolderStore *folder_store,
					    DoubleConnectionInfo *connect_info)
{
	modest_platform_connect_if_remote_and_perform(parent_window, 
						      force,
						      folder_store, 
						      src_account_connect_performer, 
						      connect_info);
}

GtkWidget *
modest_platform_get_account_settings_wizard (void)
{
	ModestEasysetupWizardDialog *dialog = modest_easysetup_wizard_dialog_new ();

	return GTK_WIDGET (dialog);
}

ModestConnectedVia
modest_platform_get_current_connection (void)
{
	TnyDevice *device = NULL;
	ModestConnectedVia retval = MODEST_CONNECTED_VIA_ANY;
	
	device = modest_runtime_get_device ();

	if (!tny_device_is_online (device))
		return MODEST_CONNECTED_VIA_ANY;

#ifdef MODEST_HAVE_CONIC
	/* Get iap id */
	const gchar *iap_id = tny_maemo_conic_device_get_current_iap_id (TNY_MAEMO_CONIC_DEVICE (device));
	if (iap_id) {
		ConIcIap *iap = tny_maemo_conic_device_get_iap (
			TNY_MAEMO_CONIC_DEVICE (device), iap_id);
		const gchar *bearer_type = con_ic_iap_get_bearer_type (iap);
		if (bearer_type) {
			if (!strcmp (bearer_type, CON_IC_BEARER_WLAN_INFRA) ||
			    !strcmp (bearer_type, CON_IC_BEARER_WLAN_ADHOC) ||
			    !strcmp (bearer_type, "WIMAX")) {
				retval = MODEST_CONNECTED_VIA_WLAN_OR_WIMAX;
			} else {
				retval = MODEST_CONNECTED_VIA_ANY;
			}
		}	
		g_object_unref (iap);
	}
#else
	retval = MODEST_CONNECTED_VIA_WLAN_OR_WIMAX; /* assume WLAN (fast) internet */  
#endif /* MODEST_HAVE_CONIC */
	return retval;
}



gboolean
modest_platform_check_memory_low (ModestWindow *win,
				  gboolean visuals)
{
	gboolean lowmem;
	
	/* are we in low memory state? */
	lowmem = osso_mem_in_lowmem_state () ? TRUE : FALSE;
	
	if (win && lowmem && visuals)
		modest_platform_run_information_dialog (
			GTK_WINDOW(win),
			_KR("memr_ib_operation_disabled"),
			TRUE);

	if (lowmem)
		g_debug ("%s: low memory reached. disallowing some operations",
			 __FUNCTION__);

	return lowmem;
}

void 
modest_platform_run_folder_details_dialog (GtkWindow *parent_window,
					   TnyFolder *folder)
{
	GtkWidget *dialog;
	
	/* Create dialog */
	dialog = modest_hildon2_details_dialog_new_with_folder (parent_window, folder);

	/* Run dialog */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), 
				     parent_window);
	gtk_widget_show_all (dialog);

	g_signal_connect_swapped (dialog, "response", 
				  G_CALLBACK (gtk_widget_destroy),
				  dialog);
}

typedef struct _HeaderDetailsGetSizeInfo {
	GtkWidget *dialog;
	TnyMimePart *part;
	guint total;
} HeaderDetailsGetSizeInfo;

static void 
header_details_dialog_destroy (gpointer userdata,
			       GObject *object)
{
	HeaderDetailsGetSizeInfo *info = (HeaderDetailsGetSizeInfo *) userdata;

	info->dialog = NULL;
}

static gboolean
idle_get_mime_part_size_cb (gpointer userdata)
{
	HeaderDetailsGetSizeInfo *info = (HeaderDetailsGetSizeInfo *) userdata;
	gdk_threads_enter ();

	if (info->dialog && GTK_WIDGET_VISIBLE (info->dialog)) {
		modest_details_dialog_set_message_size (MODEST_DETAILS_DIALOG (info->dialog),
							info->total);
	}

	if (info->dialog) {
		g_object_weak_unref (G_OBJECT (info->dialog), header_details_dialog_destroy, info);
		info->dialog = NULL;
	}
	g_object_unref (info->part);
	g_slice_free (HeaderDetailsGetSizeInfo, info);

	gdk_threads_leave ();

	return FALSE;
}

static gpointer
get_mime_part_size_thread (gpointer thr_user_data)
{
	HeaderDetailsGetSizeInfo *info = (HeaderDetailsGetSizeInfo *) thr_user_data;
	gssize result = 0;
	TnyStream *count_stream;

	count_stream = modest_count_stream_new ();
	result = tny_mime_part_decode_to_stream (info->part, count_stream, NULL);
	info->total = modest_count_stream_get_count(MODEST_COUNT_STREAM (count_stream));
	if (info->total == 0) {
		modest_count_stream_reset_count(MODEST_COUNT_STREAM (count_stream));
		result = tny_mime_part_write_to_stream (info->part, count_stream, NULL);
		info->total = modest_count_stream_get_count(MODEST_COUNT_STREAM (count_stream));
	}
	
	/* if there was an error, don't set the size (this is pretty uncommon) */
	if (result < 0) {
		g_warning ("%s: error while writing mime part to stream\n", __FUNCTION__);
	}
	g_idle_add (idle_get_mime_part_size_cb, info);

	return NULL;
}

void
modest_platform_run_header_details_dialog (GtkWindow *parent_window,
					   TnyHeader *header,
					   gboolean async_get_size,
					   TnyMsg *msg)
{
	GtkWidget *dialog;

	/* Create dialog */
	dialog = modest_hildon2_details_dialog_new_with_header (parent_window, header, !async_get_size);

	if (async_get_size && msg && TNY_IS_MSG (msg)) {
		HeaderDetailsGetSizeInfo *info;
		info = g_slice_new (HeaderDetailsGetSizeInfo);
		info->dialog = dialog;
		info->total = 0;
		info->part = TNY_MIME_PART (g_object_ref (msg));

		g_object_weak_ref (G_OBJECT (dialog), header_details_dialog_destroy, info);
		g_thread_create (get_mime_part_size_thread, info, FALSE, NULL);
	}

	/* Run dialog */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (dialog),
				     parent_window);
	gtk_widget_show_all (dialog);

	g_signal_connect_swapped (dialog, "response", 
				  G_CALLBACK (gtk_widget_destroy),
				  dialog);
}

osso_context_t *
modest_platform_get_osso_context (void)
{
	return modest_maemo_utils_get_osso_context ();
}

static gfloat
convert_volume_to_db (int linear_volume)
{
    gfloat linear_converted = linear_volume / 100.0;
    gfloat db_vol = 0.0;
    
    db_vol = 20 * log10 (linear_converted);
    if (isinf (db_vol) != 0)
        return -60.0;

    return db_vol;
}

static void
modest_platform_play_email_tone (void)
{
	gchar *mail_tone;
	gint mail_volume_int;
	int ret;
	ca_proplist *pl = NULL;
	gfloat db_volume;

#ifdef MODEST_USE_PROFILE
	gchar *active_profile;
	gchar *mail_volume;

	active_profile = profile_get_profile ();
	mail_tone = profile_get_value (active_profile, PROFILE_MAIL_TONE);
	mail_volume = profile_get_value (active_profile, PROFILE_MAIL_VOLUME);
	mail_volume_int = profile_parse_int (mail_volume);
	g_free (mail_volume);
	g_free (active_profile);
#else
	mail_tone = g_strdup (MAIL_TONE);
	mail_volume_int = 100;
#endif

	if (mail_tone && !strstr (mail_tone, "/")) {
		gchar *tmp;

		tmp = g_strconcat ("/usr/share/sounds", mail_tone, NULL);
		g_free (mail_tone);
		mail_tone = tmp;
	}

	if (mail_volume_int > 0) {

		if (ca_con == NULL) {
			if ((ret = ca_context_create (&ca_con)) != CA_SUCCESS) {
				g_warning("ca_context_create: %s\n", ca_strerror(ret));
				ca_con = NULL;
				return;
			}
			if ((ret = ca_context_set_driver (ca_con, "gstreamer")) != CA_SUCCESS) {
				g_warning ("ca_context_set_driver: %s\n", ca_strerror (ret));
				ca_con = NULL;
				return;
			}
		}

		if (!ca_con_opened) {
			if ((ret = ca_context_open(ca_con)) != CA_SUCCESS) {
				g_warning("ca_context_open: %s\n", ca_strerror(ret));
				return;
			} else {
				ca_con_opened = TRUE;
			}
		}

		ca_proplist_create(&pl);
		ca_proplist_sets(pl, CA_PROP_MEDIA_FILENAME, mail_tone);
		db_volume = convert_volume_to_db (mail_volume_int);
		ca_proplist_setf(pl, CA_PROP_CANBERRA_VOLUME, "%f", db_volume);

		ret = ca_context_play_full(ca_con, 0, pl, NULL, NULL);
		g_debug("ca_context_play_full (vol %f): %s\n", (gfloat) mail_volume_int, ca_strerror(ret));

		ca_proplist_destroy(pl);
	}

	g_free (mail_tone);
}

#define MOVE_TO_DIALOG_FOLDER_VIEW "folder-view"
#define MOVE_TO_DIALOG_BACK_BUTTON "back-button"
#define MOVE_TO_DIALOG_ACTION_BUTTON "action-button"
#define MOVE_TO_DIALOG_SHOWING_FOLDERS "showing-folders"
#define MOVE_TO_DIALOG_PANNABLE "pannable"
#define MOVE_TO_FOLDER_SEPARATOR "/"

static void
translate_path (gchar **path)
{
	gchar **parts;
	gchar **current;
	GString *output;
	gboolean add_separator;

	parts = g_strsplit (*path, MOVE_TO_FOLDER_SEPARATOR, 0);
	g_free (*path);

	current = parts;
	output = g_string_new ("");
	add_separator = FALSE;

	while (*current != NULL) {
		TnyFolderType folder_type;
		gchar *downcase;

		if (add_separator) {
			output = g_string_append (output, MOVE_TO_FOLDER_SEPARATOR);
		} else {
			add_separator = TRUE;
		}

		downcase = g_ascii_strdown (*current, -1);
		folder_type = modest_local_folder_info_get_type (downcase);
		if (strcmp (downcase, "inbox") == 0) {
			output = g_string_append (output, _("mcen_me_folder_inbox"));
		} else if (folder_type == TNY_FOLDER_TYPE_ARCHIVE ||
		    folder_type == TNY_FOLDER_TYPE_DRAFTS ||
		    folder_type == TNY_FOLDER_TYPE_SENT ||
		    folder_type == TNY_FOLDER_TYPE_OUTBOX) {
			output = g_string_append (output, modest_local_folder_info_get_type_display_name (folder_type));
		} else {
			output = g_string_append (output, *current);
		}
		g_free (downcase);

		current++;
	}

	g_strfreev (parts);
	*path = g_string_free (output, FALSE);
}

static void
move_to_dialog_set_selected_folder_store (GtkWidget *dialog, 
					  TnyFolderStore *folder_store)
{
	GtkWidget *action_button;
	GtkWidget *image = NULL;
	TnyAccount *account;
	gchar *account_name = NULL, *short_name = NULL;

        action_button = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_ACTION_BUTTON));

	/* Get account name */
	if (TNY_IS_FOLDER (folder_store))
		account = tny_folder_get_account (TNY_FOLDER (folder_store));
	else
		account = g_object_ref (folder_store);

	if (modest_tny_account_is_virtual_local_folders (account))
		account_name = modest_conf_get_string (modest_runtime_get_conf(),
						       MODEST_CONF_DEVICE_NAME, NULL);

	if (!account_name)
		account_name = g_strdup (tny_account_get_name (account));

	g_object_unref (account);

	/* Set title of button: account or folder name */
	if (TNY_IS_FOLDER (folder_store))
		short_name = folder_store_get_display_name (folder_store);
	else
		short_name = g_strdup (account_name);

	hildon_button_set_title (HILDON_BUTTON (action_button), short_name);

	/* Set value of button, folder full name */
	if (TNY_IS_CAMEL_FOLDER (folder_store)) {
		const gchar *camel_full_name;
		gchar *last_slash, *full_name;

		camel_full_name = tny_camel_folder_get_full_name (TNY_CAMEL_FOLDER (folder_store));
		last_slash = g_strrstr (camel_full_name, "/");
		if (last_slash) {
			gchar *prefix = g_strndup (camel_full_name, last_slash - camel_full_name + 1);
			full_name = g_strconcat (account_name, MOVE_TO_FOLDER_SEPARATOR, prefix, short_name, NULL);
			g_free (prefix);
		} else {
			full_name = g_strconcat (account_name, MOVE_TO_FOLDER_SEPARATOR,
						 short_name,
						 NULL);
		}
		translate_path (&full_name);
		hildon_button_set_value (HILDON_BUTTON (action_button), full_name);
		g_free (full_name);
	}
	g_free (account_name);
	g_free (short_name);

	/* Set image for the button */
	image = get_image_for_folder_store (folder_store, MODEST_ICON_SIZE_BIG);
	if (image)
		hildon_button_set_image (HILDON_BUTTON (action_button), image);
}

static void
move_to_dialog_show_accounts (GtkWidget *dialog)
{
	GtkWidget *back_button;
	GtkWidget *folder_view;
	GtkWidget *pannable;
	GtkWidget *action_button;

        back_button = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_BACK_BUTTON));
        action_button = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_ACTION_BUTTON));
        folder_view = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_FOLDER_VIEW));
        pannable = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_PANNABLE));

	gtk_widget_set_sensitive (back_button, FALSE);
	gtk_widget_set_sensitive (action_button, FALSE);

	/* Need to set this here, otherwise callbacks called because
	   of filtering won't perform correctly */
	g_object_set_data (G_OBJECT (dialog), MOVE_TO_DIALOG_SHOWING_FOLDERS, GINT_TO_POINTER (FALSE));

	/* Reset action button */
	hildon_button_set_title (HILDON_BUTTON (action_button), NULL);
	hildon_button_set_value (HILDON_BUTTON (action_button), NULL);
	hildon_button_set_image (HILDON_BUTTON (action_button), NULL);

	modest_folder_view_set_account_id_of_visible_server_account (MODEST_FOLDER_VIEW (folder_view), NULL);
	modest_folder_view_show_non_move_folders (MODEST_FOLDER_VIEW (folder_view), TRUE);
	modest_folder_view_set_style (MODEST_FOLDER_VIEW (folder_view), MODEST_FOLDER_VIEW_STYLE_SHOW_ALL);
	modest_folder_view_unset_filter (MODEST_FOLDER_VIEW (folder_view),
					 MODEST_FOLDER_VIEW_FILTER_HIDE_MCC_FOLDERS);
	modest_folder_view_unset_filter (MODEST_FOLDER_VIEW (folder_view),
				       MODEST_FOLDER_VIEW_FILTER_HIDE_LOCAL_FOLDERS);
	modest_folder_view_unset_filter (MODEST_FOLDER_VIEW (folder_view), 
					 MODEST_FOLDER_VIEW_FILTER_HIDE_ACCOUNTS);
	modest_folder_view_set_filter (MODEST_FOLDER_VIEW (folder_view), 
				       MODEST_FOLDER_VIEW_FILTER_HIDE_FOLDERS);
	hildon_pannable_area_jump_to (HILDON_PANNABLE_AREA (pannable), 0, 0);
}

static void
move_to_dialog_show_folders (GtkWidget *dialog, TnyFolderStore *folder_store)
{
	GtkWidget *back_button;
	GtkWidget *folder_view;
	TnyAccount *account;
	const gchar *account_id;
	GtkWidget *pannable;
	GtkWidget *action_button;

        back_button =
		GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_BACK_BUTTON));
        action_button =
		GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_ACTION_BUTTON));
        folder_view =
		GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_FOLDER_VIEW));
        pannable =
		GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_PANNABLE));

	gtk_widget_set_sensitive (back_button, TRUE);
	gtk_widget_set_sensitive (action_button, TRUE);

	/* Need to set this here, otherwise callbacks called because
	   of filtering won't perform correctly */
	g_object_set_data (G_OBJECT (dialog),
			   MOVE_TO_DIALOG_SHOWING_FOLDERS,
			   GINT_TO_POINTER (TRUE));

	account = TNY_ACCOUNT (folder_store);
	if (modest_tny_account_is_virtual_local_folders (account)) {
		account_id = tny_account_get_id (account);
		modest_folder_view_set_filter (MODEST_FOLDER_VIEW (folder_view),
					       MODEST_FOLDER_VIEW_FILTER_HIDE_MCC_FOLDERS);
	} else if (modest_tny_account_is_memory_card_account (account)) {
		account_id = tny_account_get_id (account);
		modest_folder_view_set_filter (MODEST_FOLDER_VIEW (folder_view),
					       MODEST_FOLDER_VIEW_FILTER_HIDE_LOCAL_FOLDERS);
	} else {
		account_id = tny_account_get_id (account);
		modest_folder_view_set_filter (MODEST_FOLDER_VIEW (folder_view),
					       MODEST_FOLDER_VIEW_FILTER_HIDE_LOCAL_FOLDERS);
		modest_folder_view_set_filter (MODEST_FOLDER_VIEW (folder_view),
					       MODEST_FOLDER_VIEW_FILTER_HIDE_MCC_FOLDERS);
	}

	move_to_dialog_set_selected_folder_store (dialog, folder_store);
	modest_folder_view_set_account_id_of_visible_server_account (MODEST_FOLDER_VIEW (folder_view),
								     account_id);

	modest_folder_view_show_non_move_folders (MODEST_FOLDER_VIEW (folder_view), FALSE);
	modest_folder_view_set_style (MODEST_FOLDER_VIEW (folder_view), MODEST_FOLDER_VIEW_STYLE_SHOW_ONE);
	modest_folder_view_set_filter (MODEST_FOLDER_VIEW (folder_view), MODEST_FOLDER_VIEW_FILTER_HIDE_ACCOUNTS);
	modest_folder_view_unset_filter (MODEST_FOLDER_VIEW (folder_view), MODEST_FOLDER_VIEW_FILTER_HIDE_FOLDERS);
	hildon_pannable_area_jump_to (HILDON_PANNABLE_AREA (pannable), 0, 0);
}

static void
on_move_to_dialog_back_clicked (GtkButton *button,
				gpointer userdata)
{
	GtkWidget *dialog = (GtkWidget *) userdata;

	/* Back to show accounts */
	move_to_dialog_show_accounts (dialog);
}

static void
on_move_to_dialog_row_activated (GtkTreeView       *tree_view,
                                    GtkTreePath       *path,
                                    GtkTreeViewColumn *column,
                                    gpointer           user_data)
{
	TnyFolderStore *selected = NULL;
	GtkWidget *dialog;
	GtkWidget *folder_view;
	gboolean showing_folders;

	dialog = (GtkWidget *) user_data;
	showing_folders = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), 
							      MOVE_TO_DIALOG_SHOWING_FOLDERS));

	folder_view = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), 
						     MOVE_TO_DIALOG_FOLDER_VIEW));

	selected = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));
	if (!selected)
		return;

	if (!showing_folders) {
		gboolean valid = TRUE;

		if (TNY_IS_ACCOUNT (selected) &&
		    modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (selected))) {
			ModestProtocolType protocol_type;

			protocol_type = modest_tny_account_get_protocol_type (TNY_ACCOUNT (selected));
			valid  = !modest_protocol_registry_protocol_type_has_tag 
				(modest_runtime_get_protocol_registry (),
				 protocol_type,
				 MODEST_PROTOCOL_REGISTRY_STORE_FORBID_INCOMING_XFERS);
		}
		if (valid)
			move_to_dialog_show_folders (dialog, selected);
	} else {
		move_to_dialog_set_selected_folder_store (dialog, selected);
	}
	g_object_unref (selected);
}

static void
on_move_to_dialog_selection_changed (GtkTreeSelection *selection,
				     gpointer          user_data)
{
	gboolean showing_folders;
	GtkWidget *dialog;

	dialog = (GtkWidget *) user_data;
	showing_folders = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_SHOWING_FOLDERS));
	if (showing_folders) {
		TnyFolderStore *selected;
		GtkWidget *folder_view;

		folder_view = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_FOLDER_VIEW));
		selected = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

		if (selected) {
			move_to_dialog_set_selected_folder_store (dialog, selected);
			g_object_unref (selected);
		}
	}
}

static void
on_move_to_dialog_action_clicked (GtkButton *selection,
				  gpointer   user_data)
{
	GtkWidget *dialog;
	gboolean showing_folders;

	dialog = (GtkWidget *) user_data;
	showing_folders = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_SHOWING_FOLDERS));
	if (showing_folders) {
		TnyFolderStore *selected;
		GtkWidget *folder_view;

		folder_view = GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), MOVE_TO_DIALOG_FOLDER_VIEW));
		selected = modest_folder_view_get_selected (MODEST_FOLDER_VIEW (folder_view));

		if (selected) {
			/* It's not possible to select root folders as
			   targets unless they're the local account or
			   the memory card account */
			if ((TNY_IS_FOLDER (selected) && !TNY_IS_MERGE_FOLDER (selected)) ||
			    (TNY_IS_ACCOUNT (selected) &&
			     (modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (selected)) ||
			      modest_tny_account_is_memory_card_account (TNY_ACCOUNT (selected)))))
				gtk_dialog_response  (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
			g_object_unref (selected);
		}
	}
}

static void
move_to_dialog_activity_changed (ModestFolderView *folder_view, gboolean activity, GtkDialog *dialog)
{
	hildon_gtk_window_set_progress_indicator (GTK_WINDOW (dialog), activity?1:0);
}

GtkWidget *
modest_platform_create_move_to_dialog (GtkWindow *parent_window,
				       GtkWidget **folder_view)
{
	GtkWidget *dialog, *folder_view_container;
	GtkWidget *align;
	GtkWidget *buttons_hbox;
	GtkWidget *back_button;
	GdkPixbuf *back_pixbuf;
	GtkWidget *top_vbox;
	GtkWidget *action_button;
	GtkTreeSelection *selection;

	/* Create dialog. We cannot use a touch selector because we
	   need to use here the folder view widget directly */
	dialog = gtk_dialog_new_with_buttons (_("mcen_ti_moveto_folders_title"),
					      GTK_WINDOW (parent_window),
					      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR |
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      _FM ("ckdg_bd_change_folder_new_folder"),
					      MODEST_GTK_RESPONSE_NEW_FOLDER,
	                                      NULL);

	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DOUBLE, MODEST_MARGIN_NONE);
	top_vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_HALF);

	/* Create folder view */
	*folder_view = modest_platform_create_folder_view_full (NULL, FALSE);
	g_signal_connect (G_OBJECT (*folder_view), "activity-changed", G_CALLBACK (move_to_dialog_activity_changed),
			  dialog);

	modest_folder_view_set_cell_style (MODEST_FOLDER_VIEW (*folder_view),
					   MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT);
	modest_folder_view_show_message_count (MODEST_FOLDER_VIEW (*folder_view),
					       FALSE);
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (*folder_view),
						  (TnyAccountStore *) modest_runtime_get_account_store ());

	buttons_hbox = gtk_hbox_new (FALSE, MODEST_MARGIN_HALF);
	back_button = gtk_button_new ();
	back_pixbuf = modest_platform_get_icon (_FM("filemanager_folder_up"), MODEST_ICON_SIZE_BIG);
	if (back_pixbuf) {
		gtk_button_set_image (GTK_BUTTON (back_button), gtk_image_new_from_pixbuf (back_pixbuf));
		g_object_unref (back_pixbuf);
	}

	action_button = hildon_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
					   HILDON_BUTTON_ARRANGEMENT_VERTICAL);
	gtk_button_set_alignment (GTK_BUTTON (action_button), 0.0, 0.5);

	gtk_box_pack_start (GTK_BOX (buttons_hbox), back_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (buttons_hbox), action_button, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (GTK_WIDGET (back_button), FALSE);
	gtk_widget_set_sensitive (GTK_WIDGET (action_button), FALSE);
	gtk_box_pack_start (GTK_BOX (top_vbox), buttons_hbox, FALSE, FALSE, 0);

	/* Create pannable and add it to the dialog */
	folder_view_container = hildon_pannable_area_new ();
	gtk_container_add (GTK_CONTAINER (folder_view_container), *folder_view);
	gtk_box_pack_start (GTK_BOX (top_vbox), folder_view_container, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (align), top_vbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), align, TRUE, TRUE, 0);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 300);

	gtk_widget_show (GTK_DIALOG (dialog)->vbox);
	gtk_widget_show (folder_view_container);
	gtk_widget_show (align);
	gtk_widget_show (top_vbox);
	gtk_widget_show (*folder_view);
	gtk_widget_show_all (back_button);
	gtk_widget_show (action_button);
	gtk_widget_show (buttons_hbox);
	gtk_widget_show (dialog);

	g_object_set_data (G_OBJECT (dialog), MOVE_TO_DIALOG_FOLDER_VIEW, *folder_view);
	g_object_set_data (G_OBJECT (dialog), MOVE_TO_DIALOG_BACK_BUTTON, back_button);
	g_object_set_data (G_OBJECT (dialog), MOVE_TO_DIALOG_ACTION_BUTTON, action_button);
	g_object_set_data (G_OBJECT (dialog), MOVE_TO_DIALOG_PANNABLE, folder_view_container);

        /* Simulate the behaviour of a HildonPickerDialog by emitting
	   a response when a folder is selected */
        g_signal_connect (*folder_view, "row-activated",
                          G_CALLBACK (on_move_to_dialog_row_activated),
                          dialog);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (*folder_view));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (on_move_to_dialog_selection_changed),
			  dialog);

	g_signal_connect (action_button, "clicked",
			  G_CALLBACK (on_move_to_dialog_action_clicked),
			  dialog);

	g_signal_connect (back_button, "clicked",
			  G_CALLBACK (on_move_to_dialog_back_clicked),
			  dialog);

	move_to_dialog_show_accounts (dialog);

	return dialog;
}

TnyList *
modest_platform_get_list_to_move (ModestWindow *window)
{
	TnyList *list = NULL;

	if (MODEST_IS_HEADER_WINDOW (window)) {
		ModestHeaderView *header_view;

		header_view = modest_header_window_get_header_view (MODEST_HEADER_WINDOW (window));
		list = modest_header_view_get_selected_headers (header_view);
	} else if (MODEST_IS_FOLDER_WINDOW (window)) {
		ModestFolderView *folder_view;
		TnyFolderStore *selected_folder;

		list = TNY_LIST (tny_simple_list_new ());
		folder_view = modest_folder_window_get_folder_view (MODEST_FOLDER_WINDOW (window));
		selected_folder = modest_folder_view_get_selected (folder_view);
		if (selected_folder) {
			tny_list_prepend (list, G_OBJECT (selected_folder));
			g_object_unref (selected_folder);
		}
		return list;
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		TnyHeader *header;

		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (window));
		if (header) {
			list = TNY_LIST (tny_simple_list_new ());
			tny_list_prepend (list, G_OBJECT (header));
			g_object_unref (header);
		}
	} else {
		g_return_val_if_reached (NULL);
	}

	return list;
}

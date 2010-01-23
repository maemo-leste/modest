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
#include <modest-defs.h>
#include <modest-platform.h>
#include <modest-runtime.h>
#include <modest-main-window.h>
#include <modest-header-view.h>
#include "maemo/modest-maemo-global-settings-dialog.h"
#include "modest-widget-memory.h"
#include <modest-hildon-includes.h>
#include <modest-utils.h>
#include <modest-maemo-utils.h>
#include <dbus_api/modest-dbus-callbacks.h>
#include <maemo/modest-osso-autosave-callbacks.h>
#include <libosso.h>
#include <tny-maemo-conic-device.h>
#include <tny-simple-list.h>
#include <tny-folder.h>
#include <tny-error.h>
#include <tny-merge-folder.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmain.h>
#include <modest-text-utils.h>
#include "modest-tny-folder.h"
#include "modest-tny-account.h"
#include <string.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <modest-account-settings-dialog.h>
#include <easysetup/modest-easysetup-wizard-dialog.h>
#include "modest-hildon-sort-dialog.h"
#include <hildon/hildon-sound.h>
#include <osso-mem.h>
#include "widgets/modest-details-dialog.h"

#ifdef MODEST_HAVE_MCE
#include <mce/dbus-names.h>
#endif /*MODEST_HAVE_MCE*/

#ifdef MODEST_HAVE_ABOOK
#include <libosso-abook/osso-abook.h>
#endif /*MODEST_HAVE_ABOOK*/

#ifdef MODEST_HAVE_LIBALARM
#include <alarmd/alarm_event.h> /* For alarm_event_add(), etc. */
#endif /*MODEST_HAVE_LIBALARM*/


#define HILDON_OSSO_URI_ACTION "uri-action"
#define URI_ACTION_COPY "copy:"
#define MODEST_NEW_MAIL_SOUND_FILE "/usr/share/sounds/ui-new_email.wav"
#define MODEST_NEW_MAIL_LIGHTING_PATTERN "PatternCommunicationEmail"

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
		g_printerr ("modest: check for mcc file failed\n");
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
	
	osso_context = 	osso_initialize(PACKAGE,PACKAGE_VERSION,
					FALSE, NULL);	
	if (!osso_context) {
		g_printerr ("modest: failed to acquire osso context\n");
		return FALSE;
	}
	modest_maemo_utils_set_osso_context (osso_context);

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

	/* Register osso auto-save callbacks: */
	result = osso_application_set_autosave_cb (osso_context, 
		modest_on_osso_application_autosave, NULL /* user_data */);
	if (result != OSSO_OK) {
		g_printerr ("modest: osso_application_set_autosave_cb() failed.\n");
		return FALSE;
	}
	

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
		action_name += strlen(URI_ACTION_COPY); /* jump past the prefix */

		if (g_str_has_prefix (action_name, "mailto:")) /* ignore mailto: prefixes */
			action_name += strlen ("mailto:");
		
		gtk_clipboard_set_text (clipboard, action_name, strlen (action_name));
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
	GSList *actions_list;

	if (uri == NULL)
		return FALSE;
	
	actions_list = hildon_uri_get_actions_by_uri (uri, -1, NULL);
	if (actions_list) {

		GtkWidget *menu = gtk_menu_new ();
		ModestPlatformPopupInfo *popup_info = g_new0 (ModestPlatformPopupInfo, 1);

		/* don't add actions for file: uri's -- they might confuse the user,
		 * and/or might have security implications
		 * we still allow to copy the url though
		 */
		if (!g_str_has_prefix (uri, "file:")) { 		
		
			GSList *node;			
			popup_info->actions = actions_list;
			popup_info->uri = g_strdup (uri);
			
			for (node = actions_list; node != NULL; node = g_slist_next (node)) {
				GtkWidget *menu_item;
				const gchar *action_name;
				const gchar *translation_domain;
				HildonURIAction *action = (HildonURIAction *) node->data;
				action_name = hildon_uri_action_get_name (action);
				translation_domain = hildon_uri_action_get_translation_domain (action);
				menu_item = gtk_menu_item_new_with_label (dgettext(translation_domain, action_name));
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
		}

		/* always add the copy item */
		GtkWidget* menu_item = gtk_menu_item_new_with_label (dgettext("osso-uri",
									      "uri_link_copy_link_location"));
		g_object_set_data_full (G_OBJECT(menu_item), HILDON_OSSO_URI_ACTION,
					g_strconcat (URI_ACTION_COPY, uri, NULL),
					g_free);
		g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (activate_uri_popup_item),NULL);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show (menu_item);

		
		/* and what to do when the link is deleted */
		g_signal_connect (G_OBJECT (menu), "delete-event", G_CALLBACK (delete_uri_popup), popup_info);
		gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time ());
						  
	} else {
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_unsupported_link"));
	}
	
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
	ok_button = GTK_WIDGET (buttons->next->data);
	
	chars = gtk_editable_get_chars (editable, 0, -1);
	g_return_if_fail (chars != NULL);

	
	if (g_utf8_strlen (chars,-1) >= 21)
		hildon_banner_show_information  (gtk_widget_get_parent (GTK_WIDGET (user_data)), NULL,
						 _CS("ckdg_ib_maximum_characters_reached"));
	else
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
	GList *child_vbox, *child_hbox;
	GtkWidget *hbox, *entry;
	TnyFolderStore *parent;
	const gchar *new_name;
	gboolean exists;

	if (response != GTK_RESPONSE_ACCEPT)
		return;
	
	/* Get entry */
	child_vbox = gtk_container_get_children (GTK_CONTAINER (dialog->vbox));
	hbox = child_vbox->data;
	child_hbox = gtk_container_get_children (GTK_CONTAINER (hbox));
	entry = child_hbox->next->data;
	
	parent = TNY_FOLDER_STORE (user_data);
	new_name = gtk_entry_get_text (GTK_ENTRY (entry));
	exists = FALSE;
	
	/* Look for another folder with the same name */
	if (modest_tny_folder_has_subfolder_with_name (parent, 
						       new_name,
						       TRUE)) {
		exists = TRUE;
	}
	
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



static gint
modest_platform_run_folder_name_dialog (GtkWindow *parent_window,
					TnyFolderStore *parent,
					const gchar *dialog_title,
					const gchar *label_text,
					const gchar *suggested_name,
					gchar **folder_name)
{
	GtkWidget *accept_btn = NULL; 
	GtkWidget *dialog, *entry, *label, *hbox;
	GList *buttons = NULL;
	gint result;

	/* Ask the user for the folder name */
	dialog = gtk_dialog_new_with_buttons (dialog_title,
					      parent_window,
					      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT,
					      _("mcen_bd_dialog_ok"),
					      GTK_RESPONSE_ACCEPT,
					      _("mcen_bd_dialog_cancel"),
					      GTK_RESPONSE_REJECT,
					      NULL);

	/* Add accept button (with unsensitive handler) */
	buttons = gtk_container_get_children (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area));
	accept_btn = GTK_WIDGET (buttons->next->data);
	/* Create label and entry */
	label = gtk_label_new (label_text);
	/* TODO: check that the suggested name does not exist */
	/* We set 21 as maximum because we want to show WID-INF036
	   when the user inputs more that 20 */
	entry = gtk_entry_new_with_max_length (21);
	if (suggested_name)
		gtk_entry_set_text (GTK_ENTRY (entry), suggested_name);
	else
		gtk_entry_set_text (GTK_ENTRY (entry), _("mcen_ia_default_folder_name"));
	gtk_entry_set_width_chars (GTK_ENTRY (entry),
				   MAX (g_utf8_strlen (gtk_entry_get_text (GTK_ENTRY (entry)), -1),
					g_utf8_strlen (_("mcen_ia_default_folder_name"), -1)));
	gtk_entry_select_region (GTK_ENTRY (entry), 0, -1);

	/* Connect to the response method to avoid closing the dialog
	   when an invalid name is selected*/
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (on_response),
			  parent);

	/* Track entry changes */
	g_signal_connect (entry,
			  "insert-text",
			  G_CALLBACK (entry_insert_text),
			  dialog);
	g_signal_connect (entry,
			  "changed",
			  G_CALLBACK (entry_changed),
			  dialog);


	/* Some locales like pt_BR need this to get the full window
	   title shown */
	gtk_widget_set_size_request (GTK_WIDGET (dialog), 300, -1);

	/* Create the hbox */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

	/* Add hbox to dialog */
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    hbox, FALSE, FALSE, 0);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), GTK_WINDOW (parent_window));
	gtk_widget_show_all (GTK_WIDGET(dialog));
		
	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT)
		*folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	gtk_widget_destroy (dialog);

	while (gtk_events_pending ())
		gtk_main_iteration ();

	return result;
}

gint
modest_platform_run_new_folder_dialog (GtkWindow *parent_window,
				       TnyFolderStore *suggested_parent,
				       gchar *suggested_name,
				       gchar **folder_name,
				       TnyFolderStore **parent_folder)
{
	gchar *real_suggested_name = NULL, *tmp = NULL;
	gint result;

	if(suggested_name == NULL)
	{
		const gchar *default_name = _("mcen_ia_default_folder_name");
		unsigned int i;
		gchar num_str[3];

		for(i = 0; i < 100; ++ i) {
			gboolean exists = FALSE;

			sprintf(num_str, "%.2u", i);

			if (i == 0)
				real_suggested_name = g_strdup (default_name);
			else
				real_suggested_name = g_strdup_printf (_("mcen_ia_default_folder_name_s"),
				                                       num_str);
			exists = modest_tny_folder_has_subfolder_with_name (suggested_parent,
									    real_suggested_name,
									    TRUE);

			if (!exists)
				break;

			g_free (real_suggested_name);
		}

		/* Didn't find a free number */
		if (i == 100)
			real_suggested_name = g_strdup (default_name);
	} else {
		real_suggested_name = suggested_name;
	}

	tmp = g_strconcat (_("mcen_fi_new_folder_name"), ":", NULL);
	result = modest_platform_run_folder_name_dialog (parent_window, 
							 suggested_parent,
	                                                 _("mcen_ti_new_folder"),
	                                                 tmp,
	                                                 real_suggested_name,
	                                                 folder_name);
	g_free (tmp);

	if (suggested_name == NULL)
		g_free(real_suggested_name);

	if (parent_folder != NULL) {
		*parent_folder = suggested_parent?g_object_ref (suggested_parent): NULL;
	}

	return result;
}

gint
modest_platform_run_rename_folder_dialog (GtkWindow *parent_window,
                                          TnyFolderStore *parent_folder,
                                          const gchar *suggested_name,
                                          gchar **folder_name)
{
	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent_folder), GTK_RESPONSE_REJECT);

	return modest_platform_run_folder_name_dialog (parent_window, 
						       parent_folder,
						       _HL("ckdg_ti_rename_folder"),
						       _HL("ckdg_fi_rename_name"),
						       suggested_name,
						       folder_name);
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
				     GTK_WINDOW (dialog), GTK_WINDOW (parent_window));

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
				     GTK_WINDOW (dialog), GTK_WINDOW (parent_window));

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
					     GTK_WINDOW (note), GTK_WINDOW (parent_window));
	
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
	if (data) {
		if (g_signal_handler_is_connected (account, data->handler))
			g_signal_handler_disconnect (account, data->handler);
		g_mutex_free (data->mutex);
		g_main_loop_unref (data->wait_loop);
		g_slice_free (ConnectAndWaitData, data);
	}

	conn_status = tny_account_get_connection_status (account);
	return (conn_status == TNY_CONNECTION_STATUS_CONNECTED)	? TRUE: FALSE;
}

gboolean 
modest_platform_connect_and_wait_if_network_account (GtkWindow *parent_window, TnyAccount *account)
{
	if (tny_account_get_account_type (account) == TNY_ACCOUNT_TYPE_STORE) {
		if (!TNY_IS_CAMEL_POP_STORE_ACCOUNT (account) &&
		    !TNY_IS_CAMEL_IMAP_STORE_ACCOUNT (account)) {
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

	dialog = modest_hildon_sort_dialog_new (parent_window);

	hildon_help_dialog_help_enable (GTK_DIALOG(dialog),
					"applications_email_sort",
					modest_maemo_utils_get_osso_context());

	return dialog;
}


gboolean 
modest_platform_set_update_interval (guint minutes)
{
#ifdef MODEST_HAVE_LIBALARM
	
	ModestConf *conf = modest_runtime_get_conf ();
	if (!conf)
		return FALSE;
		
	cookie_t alarm_cookie = modest_conf_get_int (conf, MODEST_CONF_ALARM_ID, NULL);

	/* Delete any existing alarm,
	 * because we will replace it: */
	if (alarm_cookie) {
		if (alarm_event_del(alarm_cookie) != 1)
			g_debug ("%s: alarm %d was not on the queue", __FUNCTION__, (int)alarm_cookie);
		alarm_cookie = 0;
		modest_conf_set_int (conf, MODEST_CONF_ALARM_ID, 0, NULL);
	}
	
	/* 0 means no updates: */
	if (minutes == 0)
		return TRUE;
	
     
	/* Register alarm: */
	
	/* Set the interval in alarm_event_t structure: */
	alarm_event_t *event = g_new0(alarm_event_t, 1);
	event->alarm_time = minutes * 60; /* seconds */
	
	/* Set recurrence every few minutes: */
	event->recurrence = minutes;
	event->recurrence_count = -1; /* Means infinite */

	/* Specify what should happen when the alarm happens:
	 * It should call this D-Bus method: */
	 
	event->dbus_path = g_strdup(MODEST_DBUS_OBJECT);
	event->dbus_interface = g_strdup (MODEST_DBUS_IFACE);
	event->dbus_service = g_strdup (MODEST_DBUS_SERVICE);
	event->dbus_name = g_strdup (MODEST_DBUS_METHOD_SEND_RECEIVE);

	/* Use ALARM_EVENT_NO_DIALOG: Otherwise, a dialog will be shown if 
	 * exec_name or dbus_path is NULL, even though we have specified no dialog text.
	 * Also use ALARM_EVENT_ACTIVATION so that modest is started (without UI) to get emails 
	 * This is why we want to use the Alarm API instead of just g_timeout_add().
	 * (The old maemo email-client did this, though it isn't specified in the UI spec.)
	 * ALARM_EVENT_CONNECTED will prevent the alarm from being called in case that the device is offline
         */
	event->flags = ALARM_EVENT_NO_DIALOG | ALARM_EVENT_ACTIVATION | ALARM_EVENT_CONNECTED;
	
	alarm_cookie = alarm_event_add (event);

	/* now, free it */
	alarm_event_free (event);
	
	/* Store the alarm ID in GConf, so we can remove it later:
	 * This is apparently valid between application instances. */
	modest_conf_set_int (conf, MODEST_CONF_ALARM_ID, alarm_cookie, NULL);
	
	if (!alarm_cookie) {
	    /* Error */
	    const alarm_error_t alarm_error = alarmd_get_error ();
	    g_debug ("Error setting alarm event. Error code: '%d'\n", alarm_error);
	    
	    /* Give people some clue: */
	    /* The alarm API should have a function for this: */
	    if (alarm_error == ALARMD_ERROR_DBUS) {
	    	g_debug ("  ALARMD_ERROR_DBUS: An error with D-Bus occurred, probably coudn't get a D-Bus connection.\n");
	    } else if (alarm_error == ALARMD_ERROR_CONNECTION) {
	    	g_debug ("  ALARMD_ERROR_CONNECTION: Could not contact alarmd via D-Bus.\n");
	    } else if (alarm_error == ALARMD_ERROR_INTERNAL) {
	    	g_debug ("  ALARMD_ERROR_INTERNAL: Some alarmd or libalarm internal error, possibly a version mismatch.\n");
	    } else if (alarm_error == ALARMD_ERROR_MEMORY) {
	    	g_debug ("  ALARMD_ERROR_MEMORY: A memory allocation failed.\n");
	    } else if (alarm_error == ALARMD_ERROR_ARGUMENT) {
	    	g_debug ("  ALARMD_ERROR_ARGUMENT: An argument given by caller was invalid.\n");
	    } else if (alarm_error == ALARMD_ERROR_NOT_RUNNING) {
	    	g_debug ("  ALARMD_ERROR_NOT_RUNNING: alarmd is not running.\n");
	    }
	    
	    return FALSE;
	}
#endif /* MODEST_HAVE_LIBALARM */	
	return TRUE;
}

void
modest_platform_push_email_notification(void)
{
	gboolean play_sound;
	ModestWindow *main_window;
	gboolean screen_on = TRUE, app_in_foreground;

	/* Check whether or not we should play a sound */
	play_sound = modest_conf_get_bool (modest_runtime_get_conf (),
					   MODEST_CONF_PLAY_SOUND_MSG_ARRIVE,
					   NULL);

	/* Get the screen status */
	main_window = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (), FALSE);
	if (main_window)
		screen_on = modest_main_window_screen_is_on (MODEST_MAIN_WINDOW (main_window));

	/* Get the window status */
	app_in_foreground = hildon_program_get_is_topmost (hildon_program_get_instance ());

	/* If the screen is on and the app is in the
	   foreground we don't show anything */
	if (!(screen_on && app_in_foreground)) {
		/* Play a sound */
		if (play_sound)
			hildon_play_system_sound (MODEST_NEW_MAIL_SOUND_FILE);

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
	if (g_list_length (URI_list) == 0)
		return;

	if (!show_visual) {
                modest_platform_push_email_notification ();
		/* We do a return here to avoid indentation with an else */
		return;
	}

#ifdef MODEST_HAVE_HILDON_NOTIFY
	gboolean play_sound;

	/* Check whether or not we should play a sound */
	play_sound = modest_conf_get_bool (modest_runtime_get_conf (),
					   MODEST_CONF_PLAY_SOUND_MSG_ARRIVE,
					   NULL);

	HildonNotification *notification;
	GList *iter;
	GSList *notifications_list = NULL;

	/* Get previous notifications ids */
	notifications_list = modest_conf_get_list (modest_runtime_get_conf (), 
						   MODEST_CONF_NOTIFICATION_IDS, 
						   MODEST_CONF_VALUE_INT, NULL);

	iter = URI_list;
	while (iter) {
		gchar *display_address = NULL;
		gboolean first_notification = TRUE;
		gint notif_id;
		ModestMsgNotificationData *data;

		data = (ModestMsgNotificationData *) iter->data;

		display_address = g_strdup (data->from);
		modest_text_utils_get_display_address (display_address); /* string is changed in-place */

		notification = hildon_notification_new (display_address,
							data->subject,
							"qgn_list_messagin",
							"email.arrive");
		g_free (display_address);

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

		/* Play sound if the user wants. Show the LED
		   pattern. Show and play just one */
		if (G_UNLIKELY (first_notification)) {
			first_notification = FALSE;
			if (play_sound)  {
				notify_notification_set_hint_string(NOTIFY_NOTIFICATION (notification),
								    "sound-file", MODEST_NEW_MAIL_SOUND_FILE);
			}

			/* Set the led pattern */
			notify_notification_set_hint_int32 (NOTIFY_NOTIFICATION (notification),
							    "dialog-type", 4);
			notify_notification_set_hint_string(NOTIFY_NOTIFICATION (notification),
							    "led-pattern",
							    MODEST_NEW_MAIL_LIGHTING_PATTERN);
		}

		/* Notify. We need to do this in an idle because this function
		   could be called from a thread */
		notify_notification_show (NOTIFY_NOTIFICATION (notification), NULL);

		/* Save id in the list */
		g_object_get(G_OBJECT(notification), "id", &notif_id, NULL);
		notifications_list = g_slist_prepend (notifications_list, GINT_TO_POINTER(notif_id));
		/* We don't listen for the "closed" signal, because we
		   don't care about if the notification was removed or
		   not to store the list in gconf */

		iter = g_list_next (iter);
	}

	/* Save the ids */
	modest_conf_set_list (modest_runtime_get_conf (), MODEST_CONF_NOTIFICATION_IDS, 
			      notifications_list, MODEST_CONF_VALUE_INT, NULL);

	g_slist_free (notifications_list);

#endif /*MODEST_HAVE_HILDON_NOTIFY*/
}

void
modest_platform_remove_new_mail_notifications (gboolean only_visuals) 
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
	notif_list = modest_conf_get_list (modest_runtime_get_conf (), 
					   MODEST_CONF_NOTIFICATION_IDS, 
					   MODEST_CONF_VALUE_INT, NULL);

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
	modest_conf_set_list (modest_runtime_get_conf (), MODEST_CONF_NOTIFICATION_IDS, 
			      notif_list, MODEST_CONF_VALUE_INT, NULL);

	g_slist_free (notif_list);

#endif /* MODEST_HAVE_HILDON_NOTIFY */
}



GtkWidget * 
modest_platform_get_global_settings_dialog ()
{
	return modest_maemo_global_settings_dialog_new ();
}

void
modest_platform_show_help (GtkWindow *parent_window, 
			   const gchar *help_id)
{
	osso_return_t result;
	g_return_if_fail (help_id);

	result = hildon_help_show (modest_maemo_utils_get_osso_context(),
				   help_id, HILDON_HELP_SHOW_DIALOG);
	
	if (result != OSSO_OK) {
		gchar *error_msg;
		error_msg = g_strdup_printf ("FIXME The help topic %s could not be found", help_id); 
		hildon_banner_show_information (GTK_WIDGET (parent_window),
						NULL,
						error_msg);
		g_free (error_msg);
	}
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

GtkWidget *
modest_platform_create_folder_view (TnyFolderStoreQuery *query)
{
	GtkWidget *widget = modest_folder_view_new (query);

	/* Show one account by default */
	modest_folder_view_set_style (MODEST_FOLDER_VIEW (widget),
				      MODEST_FOLDER_VIEW_STYLE_SHOW_ONE);

	/* Restore settings */
	modest_widget_memory_restore (modest_runtime_get_conf(), 
				      G_OBJECT (widget),
				      MODEST_CONF_FOLDER_VIEW_KEY);

	return widget;
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
	
	printf ("DEBUG: %s: tny_account_get_connection_status()==%d\n", __FUNCTION__,
		tny_account_get_connection_status (data->account));	
	
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
	
	printf ("DEBUG: %s: account id=%s\n", __FUNCTION__, tny_account_get_id (account));
	
	if (!tny_device_is_online (modest_runtime_get_device())) {
		printf ("DEBUG: %s: device is offline.\n", __FUNCTION__);
		return FALSE;
	}
	
	/* The local_folders account never seems to leave TNY_CONNECTION_STATUS_INIT,
	 * so we avoid wait unnecessarily: */
	if (!TNY_IS_CAMEL_POP_STORE_ACCOUNT (account) && 
		!TNY_IS_CAMEL_IMAP_STORE_ACCOUNT (account) ) {
		return TRUE;		
	}
		
	printf ("DEBUG: %s: tny_account_get_connection_status()==%d\n",
		__FUNCTION__, tny_account_get_connection_status (account));
	
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
		note = hildon_note_new_information (GTK_WINDOW(dialog), msg);
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
	ModestWindow *main_win;
	
	if (!modest_window_mgr_main_window_exists (modest_runtime_get_window_mgr())) {
		g_debug ("%s: don't show dialogs if there's no main window; assuming 'Cancel'",
			 __FUNCTION__);
		return FALSE;
	}

	/* don't create it */
	main_win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(), FALSE);
	g_return_val_if_fail (main_win, FALSE); /* should not happen */
	
	
	gchar *question = g_strdup_printf (_("mcen_nc_unknown_certificate"),
					   server_name);
	
	/* We use GTK_RESPONSE_APPLY because we want the button in the
	   middle of OK and CANCEL the same as the browser does for
	   example. With GTK_RESPONSE_HELP the view button is aligned
	   to the left while the other two to the right */
	note = hildon_note_new_confirmation_add_buttons  (
		GTK_WINDOW(main_win),
		question,
		_("mcen_bd_dialog_ok"),     GTK_RESPONSE_OK,
		_("mcen_bd_view"),          GTK_RESPONSE_APPLY,   /* abusing this... */
		_("mcen_bd_dialog_cancel"), GTK_RESPONSE_CANCEL,
		NULL, NULL);
	
	g_signal_connect (G_OBJECT(note), "response", 
			  G_CALLBACK(on_cert_dialog_response),
			  (gpointer) certificate);
	
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (note), (GtkWindow *) main_win);
	response = gtk_dialog_run(GTK_DIALOG(note));

	on_destroy_dialog (note);
	g_free (question);
	
	return response == GTK_RESPONSE_OK;
}

gboolean
modest_platform_run_alert_dialog (const gchar* prompt, 
				  gboolean is_question)
{	
	ModestWindow *main_win; 

	if (!modest_window_mgr_main_window_exists (modest_runtime_get_window_mgr())) {
		g_debug ("%s:\n'%s'\ndon't show dialogs if there's no main window;"
			   " assuming 'Cancel' for questions, 'Ok' otherwise", prompt, __FUNCTION__);
		return is_question ? FALSE : TRUE;
	}

	main_win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (), FALSE);
	g_return_val_if_fail (main_win, FALSE); /* should not happen */
	
	gboolean retval = TRUE;
	if (is_question) {
		/* The Tinymail documentation says that we should show Yes and No buttons, 
		 * when it is a question.
		 * Obviously, we need tinymail to use more specific error codes instead,
		 * so we know what buttons to show. */
		GtkWidget *dialog = GTK_WIDGET (hildon_note_new_confirmation (GTK_WINDOW (main_win), 
									      prompt));
		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
					     GTK_WINDOW (dialog), (GtkWindow *) main_win);
		
		const int response = gtk_dialog_run (GTK_DIALOG (dialog));
		retval = (response == GTK_RESPONSE_YES) || (response == GTK_RESPONSE_OK);
		
		on_destroy_dialog (dialog);		
	} else {
	 	/* Just show the error text and use the default response: */
	 	modest_platform_run_information_dialog (GTK_WINDOW (main_win), 
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
 
	if (tny_account_get_account_type (account) == TNY_ACCOUNT_TYPE_STORE) {
 		if (!TNY_IS_CAMEL_POP_STORE_ACCOUNT (account) &&
 		    !TNY_IS_CAMEL_IMAP_STORE_ACCOUNT (account)) {
 
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
	dialog = modest_details_dialog_new_with_folder (parent_window, folder);

	/* Run dialog */
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
				     GTK_WINDOW (dialog), 
				     parent_window);
	gtk_widget_show_all (dialog);

	g_signal_connect_swapped (dialog, "response", 
				  G_CALLBACK (gtk_widget_destroy),
				  dialog);
}

void
modest_platform_run_header_details_dialog (GtkWindow *parent_window,
					   TnyHeader *header,
					   gboolean async_get_size,
					   TnyMsg *msg)
{
	GtkWidget *dialog;
	
	/* Create dialog */
	dialog = modest_details_dialog_new_with_header (parent_window, header, TRUE);

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

GtkWidget* 
modest_platform_create_move_to_dialog (GtkWindow *parent_window,
				       GtkWidget **folder_view)
{
	GtkWidget *dialog, *folder_view_container;

	dialog = gtk_dialog_new_with_buttons (_("mcen_ti_moveto_folders_title"),
					      GTK_WINDOW (parent_window),
					      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR |
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      _("mcen_bd_dialog_ok"), GTK_RESPONSE_OK,
					      _("mcen_bd_new"), MODEST_GTK_RESPONSE_NEW_FOLDER,
					      _("mcen_bd_dialog_cancel"), GTK_RESPONSE_CANCEL,
	                                      NULL);

	/* Create folder view */
	*folder_view = modest_platform_create_folder_view (NULL);

	/* Create pannable and add it to the dialog */
	folder_view_container = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (folder_view_container),
					 GTK_POLICY_AUTOMATIC,
					 GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), folder_view_container);
	gtk_container_add (GTK_CONTAINER (folder_view_container), *folder_view);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 300);

	gtk_widget_show (GTK_DIALOG (dialog)->vbox);
	gtk_widget_show (folder_view_container);
	gtk_widget_show (*folder_view);

	return dialog;
}


TnyList *
modest_platform_get_list_to_move (ModestWindow *window)
{
	TnyList *list = NULL;

	/* If it's a main window then it could be that we're moving a
	   folder or a set of messages */
	if (MODEST_IS_MAIN_WINDOW (window)) {
		ModestHeaderView *header_view = NULL;
		ModestFolderView *folder_view = NULL;

		folder_view = (ModestFolderView *)
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW);
		header_view = (ModestHeaderView *)
			modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (window),
							     MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW);

		/* Get folder or messages to transfer */
		if (gtk_widget_is_focus (GTK_WIDGET (folder_view))) {
			TnyFolderStore *src_folder;

			src_folder = modest_folder_view_get_selected (folder_view);
			if (src_folder) {
				list = tny_simple_list_new ();
				tny_list_prepend (list, G_OBJECT (src_folder));
				g_object_unref (src_folder);
			}
		} else if (gtk_widget_is_focus (GTK_WIDGET(header_view))) {
			list = modest_header_view_get_selected_headers(header_view);
		}
	} else if (MODEST_IS_MSG_VIEW_WINDOW (window)) {
		TnyHeader *header = NULL;

		/* We simply return the currently viewed message */
		header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (window));
		if (header) {
			list = tny_simple_list_new ();
			tny_list_prepend (list, G_OBJECT (header));
			g_object_unref (header);
		}
	} else {
		g_return_val_if_reached (NULL);
	}

	return list;
}

DBusConnection*
modest_platform_get_dbus_connection (void)
{
	osso_context_t *osso_context;
	DBusConnection *con;

	osso_context = modest_maemo_utils_get_osso_context();

	con = osso_get_dbus_connection (osso_context);

	return con;
}

void
modest_platform_emit_folder_updated_signal (const gchar *account_id, const gchar *folder_id)
{
	DBusConnection *con;

	con = modest_platform_get_dbus_connection ();
	if (!con) return;

	modest_dbus_emit_folder_updated_signal (con, account_id, folder_id);
}


void
modest_platform_emit_account_created_signal (const gchar *account_id)
{
	DBusConnection *con;

	con = modest_platform_get_dbus_connection ();
	if (!con) return;

	modest_dbus_emit_account_created_signal (con, account_id);
}

void
modest_platform_emit_account_removed_signal (const gchar *account_id)
{
	DBusConnection *con;

	con = modest_platform_get_dbus_connection ();
	if (!con) return;

	modest_dbus_emit_account_removed_signal (con, account_id);
}

void
modest_platform_emit_msg_read_changed_signal (const gchar *msg_uid,
					      gboolean is_read)
{
	DBusConnection *con;

	con = modest_platform_get_dbus_connection ();
	if (!con) return;

	modest_dbus_emit_msg_read_changed_signal (con, msg_uid, is_read);
}

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
#include <modest-runtime.h>
#include <modest-main-window.h>
#include <modest-header-view.h>
#include "maemo/modest-maemo-global-settings-dialog.h"
#include "modest-widget-memory.h"
#include <modest-hildon-includes.h>
#include <modest-maemo-utils.h>
#include <dbus_api/modest-dbus-callbacks.h>
#include <libosso-abook/osso-abook.h>
#include <maemo/modest-osso-autosave-callbacks.h>
#include <libosso.h>
#include <alarmd/alarm_event.h> /* For alarm_event_add(), etc. */
#include <tny-maemo-conic-device.h>
#include <tny-simple-list.h>
#include <tny-folder.h>
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmain.h>
#include <modest-text-utils.h>
#include "modest-tny-folder.h"
#include <string.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>


#define HILDON_OSSO_URI_ACTION "uri-action"
#define URI_ACTION_COPY "copy:"

/* The maximun number of notifications that could be shown in the
   desktop. It's specified by the specs and limited by the screen
   size */
#define MAX_NOTIFICATIONS 6

static osso_context_t *osso_context = NULL;

static void	
on_modest_conf_update_interval_changed (ModestConf* self, 
					const gchar *key, 
					ModestConfEvent event,
					ModestConfNotificationId id, 
					gpointer user_data)
{
	if (strcmp (key, MODEST_CONF_UPDATE_INTERVAL) == 0) {
		const guint update_interval_minutes = 
			modest_conf_get_int (self, MODEST_CONF_UPDATE_INTERVAL, NULL);
		modest_platform_set_update_interval (update_interval_minutes);
	}
}



static gboolean
check_required_files (void)
{
	FILE *mcc_file = modest_maemo_open_mcc_mapping_file ();
	if (!mcc_file) {
		g_printerr ("modest: check for mcc file failed\n");
		return FALSE;
	} else 
		fclose (mcc_file);

	if (access (MODEST_PROVIDERS_DATA_PATH, R_OK) != 0) {
		g_printerr ("modest: cannot find providers data\n");
		return FALSE;
	}

	return TRUE;
}



gboolean
modest_platform_init (int argc, char *argv[])
{
	osso_hw_state_t hw_state = { 0 };
	DBusConnection *con;	

	if (!check_required_files ()) {
		g_printerr ("modest: missing required files\n");
		return FALSE;
	}

	
	osso_context =
		osso_initialize(PACKAGE,PACKAGE_VERSION,
				FALSE, NULL);	
	if (!osso_context) {
		g_printerr ("modest: failed to acquire osso context\n");
		return FALSE;
	}

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

	/* Add handler for Exit D-BUS messages.
	 * Not used because osso_application_set_exit_cb() is deprecated and obsolete:
	result = osso_application_set_exit_cb(osso_context,
                                          modest_dbus_exit_event_handler,
                                          (gpointer) NULL);
	if (result != OSSO_OK) {
		g_print("Error setting exit callback (%d)\n", result);
		return OSSO_ERROR;
	}
	*/

	/* Register hardware event dbus callback: */
    	hw_state.shutdown_ind = TRUE;
	osso_hw_set_event_cb(osso_context, NULL,/*&hw_state*/ modest_osso_cb_hw_state_handler, NULL);

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
			  
	/* Get the initial update interval from gconf: */
	on_modest_conf_update_interval_changed(conf, MODEST_CONF_UPDATE_INTERVAL,
					       MODEST_CONF_EVENT_KEY_CHANGED, 0, NULL);

	/* initialize the addressbook */
	if (!osso_abook_init (&argc, &argv, osso_context)) {
		g_printerr ("modest: failed to initialized addressbook\n");
		return FALSE;
	}
		
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
	
	if (!mime_type || !g_ascii_strcasecmp (mime_type, "application/octet-stream")) 
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


gboolean 
modest_platform_activate_uri (const gchar *uri)
{
	HildonURIAction *action;
	gboolean result = FALSE;
	GSList *actions, *iter = NULL;
	
	g_return_val_if_fail (uri, FALSE);
	if (!uri)
		return FALSE;
	
	actions = hildon_uri_get_actions_by_uri (uri, -1, NULL);
	
	for (iter = actions; iter; iter = g_slist_next (iter)) {
		action = (HildonURIAction*) iter->data;
		if (action && strcmp (hildon_uri_action_get_service (action),
				      "com.nokia.modest") == 0) {
			GError *err = NULL;
			result = hildon_uri_open (uri, action, &err);
			if (!result && err) {
				g_printerr ("modest: modest_platform_activate_uri : %s",
					    err->message ? err->message : "unknown error");
				g_error_free (err);
			}
			break;
		}
	}
	
	/* if we could not open it with email, try something else */
	if (!result)
	       	result = hildon_uri_open (uri, NULL, NULL);	
		
	if (!result)
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_unsupported_link"));
	
	return result;
}

gboolean 
modest_platform_activate_file (const gchar *path, const gchar *mime_type)
{
	gint result = 0;
	DBusConnection *con;
	gchar *uri_path = NULL;

	uri_path = g_strconcat ("file://", path, NULL);	
	con = osso_get_dbus_connection (osso_context);
	
	if (mime_type)
		result = hildon_mime_open_file_with_mime_type (con, uri_path, mime_type);
	if (result != 1)
		result = hildon_mime_open_file (con, uri_path);
	if (result != 1)
		modest_platform_run_information_dialog (NULL, _("mcen_ni_noregistered_viewer"));
	
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
		return; /* we're done */
	}
	
	/* now, the real uri-actions... */
	for (node = popup_info->actions; node != NULL; node = g_slist_next (node)) {
		HildonURIAction *action = (HildonURIAction *) node->data;
		if (strcmp (action_name, hildon_uri_action_get_name (action))==0) {
			hildon_uri_open (popup_info->uri, action, NULL);
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
	if (actions_list != NULL) {
		GSList *node;
		GtkWidget *menu = gtk_menu_new ();
		ModestPlatformPopupInfo *popup_info = g_new0 (ModestPlatformPopupInfo, 1);

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

		/* always add the copy item */
		GtkWidget* menu_item = gtk_menu_item_new_with_label (dgettext("osso-uri", "uri_link_copy_link_location"));
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
modest_platform_get_icon (const gchar *name)
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
	
#if 0 /* do we still need this? */
	if (g_str_has_suffix (name, ".png")) { /*FIXME: hack*/
		pixbuf = gdk_pixbuf_new_from_file (name, &err);
		if (!pixbuf) {
			g_printerr ("modest: error loading icon '%s': %s\n",
				    name, err->message);
			g_error_free (err);
			return NULL;
		}
		return pixbuf;
	}
#endif /* */
	current_theme = gtk_icon_theme_get_default ();
	pixbuf = gtk_icon_theme_load_icon (current_theme, name, 26,
					   GTK_ICON_LOOKUP_NO_SVG,
					   &err);
	if (!pixbuf) {
		g_printerr ("modest: error loading theme icon '%s': %s\n",
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
launch_sort_headers_dialog (GtkWindow *parent_window,
			    HildonSortDialog *dialog)
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
	}
	if (!header_view) return;

	/* Add sorting keys */
	cols = modest_header_view_get_columns (header_view);
	if (cols == NULL) return;
	int sort_model_ids[6];
	int sort_ids[6];


	outgoing = (GPOINTER_TO_INT (g_object_get_data(G_OBJECT(cols->data), MODEST_HEADER_VIEW_COLUMN))==
		    MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT);

	sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_sender_recipient"));
	if (outgoing) {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT;
	} else {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN;
	}

	sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_date"));
	if (outgoing) {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_SENT_DATE;
	} else {
		sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN;
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_RECEIVED_DATE;
	}
	default_key = sort_key;

	sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_subject"));
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN;
	if (outgoing)
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT;
	else
		sort_ids[sort_key] = MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN;

	sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_attachment"));
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN;
	sort_ids[sort_key] = TNY_HEADER_FLAG_ATTACHMENTS;
	attachments_sort_id = sort_key;

	sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_size"));
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN;
	sort_ids[sort_key] = 0;

	sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_priority"));
	sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN;
	sort_ids[sort_key] = TNY_HEADER_FLAG_PRIORITY;
	priority_sort_id = sort_key;

	sortable = GTK_TREE_SORTABLE (gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (header_view)))));
	/* Launch dialogs */
	if (!gtk_tree_sortable_get_sort_column_id (sortable,
						   &current_sort_colid, &current_sort_type)) {
		hildon_sort_dialog_set_sort_key (dialog, default_key);
		hildon_sort_dialog_set_sort_order (dialog, GTK_SORT_DESCENDING);
	} else {
		hildon_sort_dialog_set_sort_order (dialog, current_sort_type);
		if (current_sort_colid == TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN) {
			gpointer flags_sort_type_pointer;
			flags_sort_type_pointer = g_object_get_data (G_OBJECT (cols->data), MODEST_HEADER_VIEW_FLAG_SORT);
			if (GPOINTER_TO_INT (flags_sort_type_pointer) == TNY_HEADER_FLAG_PRIORITY)
				hildon_sort_dialog_set_sort_key (dialog, priority_sort_id);
			else
				hildon_sort_dialog_set_sort_key (dialog, attachments_sort_id);
		} else {
			gint current_sort_keyid = 0;
			while (current_sort_keyid < 6) {
				if (sort_model_ids[current_sort_keyid] == current_sort_colid)
					break;
				else 
					current_sort_keyid++;
			}
			hildon_sort_dialog_set_sort_key (dialog, current_sort_keyid);
		}
	}

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	if (result == GTK_RESPONSE_OK) {
		sort_key = hildon_sort_dialog_get_sort_key (dialog);
		sort_type = hildon_sort_dialog_get_sort_order (dialog);
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
	
/* 	while (gtk_events_pending ()) */
/* 		gtk_main_iteration (); */

	/* free */
	g_list_free(cols);	
}



static void
on_response (GtkDialog *dialog,
	     gint response,
	     gpointer user_data)
{
	GList *child_vbox, *child_hbox;
	GtkWidget *hbox, *entry;
	TnyFolderStore *parent;

	if (response != GTK_RESPONSE_ACCEPT)
		return;

	/* Get entry */
	child_vbox = gtk_container_get_children (GTK_CONTAINER (dialog->vbox));
	hbox = child_vbox->data;
	child_hbox = gtk_container_get_children (GTK_CONTAINER (hbox));
	entry = child_hbox->next->data;

	parent = TNY_FOLDER_STORE (user_data);

	/* Look for another folder with the same name */
	if (modest_tny_folder_has_subfolder_with_name (parent, 
						       gtk_entry_get_text (GTK_ENTRY (entry)))) {
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

	/* Create the hbox */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, FALSE, 0);

	/* Add hbox to dialog */
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    hbox, FALSE, FALSE, 0);
	
	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent_window);
	
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
				       TnyFolderStore *parent_folder,
				       gchar *suggested_name,
				       gchar **folder_name)
{
	gchar *real_suggested_name = NULL;
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

			exists = modest_tny_folder_has_subfolder_with_name (parent_folder,
									    real_suggested_name);

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

	result = modest_platform_run_folder_name_dialog (parent_window, 
							 parent_folder,
	                                                 _("mcen_ti_new_folder"),
	                                                 _("mcen_fi_new_folder_name"),
	                                                 real_suggested_name,
	                                                 folder_name);
	if (suggested_name == NULL)
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

	return modest_platform_run_folder_name_dialog (parent_window, 
						       parent_folder,
						       _HL("ckdg_ti_rename_folder"),
						       _HL("ckdg_fi_rename_name"),
						       suggested_name,
						       folder_name);
}



static void
on_destroy_dialog (GtkDialog *dialog)
{
	gtk_widget_destroy (GTK_WIDGET(dialog));
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
				     GTK_WINDOW (dialog));

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	on_destroy_dialog (GTK_DIALOG(dialog));
	
	while (gtk_events_pending ())
		gtk_main_iteration ();

	return response;
}
	
gint
modest_platform_run_yes_no_dialog (GtkWindow *parent_window,
				   const gchar *message)
{
	GtkWidget *dialog;
	gint response;
	
	dialog = hildon_note_new_confirmation_add_buttons (parent_window, message,
							   _("mcen_bd_yes"), GTK_RESPONSE_YES,
							   _("mcen_bd_no"), GTK_RESPONSE_NO,
							   NULL);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), GTK_WINDOW (dialog));
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	
	on_destroy_dialog (GTK_DIALOG(dialog));

	while (gtk_events_pending ())
		gtk_main_iteration ();

	return response;
}



void
modest_platform_run_information_dialog (GtkWindow *parent_window,
					const gchar *message)
{
	GtkWidget *note;
	
	note = hildon_note_new_information (parent_window, message);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (note));
	
	g_signal_connect_swapped (note,
				  "response", 
				  G_CALLBACK (on_destroy_dialog),
				  note);

	gtk_widget_show_all (note);
}



typedef struct _UtilIdleData {
	GMutex *mutex;
	GMainLoop *wait_loop;
	gboolean has_callback;
	gulong handler;
} UtilIdleData;

static void
on_connection_status_changed (TnyAccount *account, 
			      TnyConnectionStatus status,
			      gpointer user_data)
{
	UtilIdleData *data = (UtilIdleData *) user_data;
	TnyConnectionStatus conn_status;
			
	conn_status = tny_account_get_connection_status (account);
	if (conn_status == TNY_CONNECTION_STATUS_RECONNECTING ||
	    conn_status == TNY_CONNECTION_STATUS_DISCONNECTED)
		return;

	/* Remove the handler */
	g_signal_handler_disconnect (account, data->handler);

	/* Set the has_callback to TRUE (means that the callback was
	   executed and wake up every code waiting for cond to be
	   TRUE */
	g_mutex_lock (data->mutex);
	data->has_callback = TRUE;
	if (data->wait_loop)
		g_main_loop_quit (data->wait_loop);
	g_mutex_unlock (data->mutex);
}

gboolean 
modest_platform_connect_and_wait (GtkWindow *parent_window, 
				  TnyAccount *account)
{
	UtilIdleData *data = NULL;
	gboolean device_online;
	TnyDevice *device;
	TnyConnectionStatus conn_status;
	
	device = modest_runtime_get_device();
	device_online = tny_device_is_online (device);

	/* If there is no account check only the device status */
	if (!account) {
		if (device_online)
			return TRUE;
		else
			return tny_maemo_conic_device_connect (TNY_MAEMO_CONIC_DEVICE (device), NULL);
	}

	/* Return if the account is already connected */
	conn_status = tny_account_get_connection_status (account);
	if (device_online && conn_status == TNY_CONNECTION_STATUS_CONNECTED)
		return TRUE;

	/* Connect the device */
	if (!device_online) {
		data = g_slice_new0 (UtilIdleData);
		data->mutex = g_mutex_new ();
		data->has_callback = FALSE;

		/* Track account connection status changes */
		data->handler = g_signal_connect (account, "connection-status-changed",					    
						  G_CALLBACK (on_connection_status_changed),
						  data);
		/* Try to connect the device */
		device_online = tny_maemo_conic_device_connect (TNY_MAEMO_CONIC_DEVICE (device), NULL);

		/* If the device connection failed then exit */
		if (!device_online && data->handler)
			goto frees;
	}

	/* Wait until the callback is executed */
	g_mutex_lock (data->mutex);
	if (!data->has_callback) {
		data->wait_loop = g_main_loop_new (NULL, FALSE);
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
		g_slice_free (UtilIdleData, data);
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

gboolean 
modest_platform_is_network_folderstore (TnyFolderStore *folder_store)
{
        TnyAccount *account = NULL;
        gboolean result = TRUE;

        g_return_val_if_fail(TNY_IS_FOLDER_STORE(folder_store), FALSE);

        if (TNY_IS_FOLDER (folder_store)) {
                /* Get the folder's parent account: */
                account = tny_folder_get_account(TNY_FOLDER(folder_store));
        } else if (TNY_IS_ACCOUNT (folder_store)) {
                account = TNY_ACCOUNT(folder_store);
                g_object_ref(account);
        }

        if (account != NULL) {
                if (tny_account_get_account_type (account) == TNY_ACCOUNT_TYPE_STORE) {
                        if (!TNY_IS_CAMEL_POP_STORE_ACCOUNT (account) &&
                            !TNY_IS_CAMEL_IMAP_STORE_ACCOUNT (account)) {
                                /* This must be a maildir account, which does
                                 * not require a connection: */
                                result = FALSE;
                        }
                }
                g_object_unref (account);
        } else {
                result = FALSE;
        }

        return result;
}

void
modest_platform_run_sort_dialog (GtkWindow *parent_window,
				 ModestSortDialogType type)
{
	GtkWidget *dialog = NULL;

	/* Build dialog */
	dialog = hildon_sort_dialog_new (parent_window);
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (dialog));
	
	/* Fill sort keys */
	switch (type) {
	case MODEST_SORT_HEADERS:
		launch_sort_headers_dialog (parent_window, 
					    HILDON_SORT_DIALOG(dialog));
		break;
	}
	
	/* Free */
	on_destroy_dialog (GTK_DIALOG(dialog));
}


gboolean 
modest_platform_set_update_interval (guint minutes)
{
	ModestConf *conf = modest_runtime_get_conf ();
	if (!conf)
		return FALSE;
		
	cookie_t alarm_cookie = modest_conf_get_int (conf, MODEST_CONF_ALARM_ID, NULL);

	/* Delete any existing alarm,
	 * because we will replace it: */
	if (alarm_cookie) {
		/* TODO: What does the alarm_event_del() return value mean? */
		alarm_event_del(alarm_cookie);
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
         */
	event->flags = ALARM_EVENT_NO_DIALOG | ALARM_EVENT_ACTIVATION;
	
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
	
	return TRUE;
}

void 
modest_platform_on_new_headers_received (TnyList *header_list) 
{
#ifdef MODEST_HAVE_HILDON_NOTIFY
	HildonNotification *notification;
	TnyIterator *iter;
	GSList *notifications_list = NULL;

	/* Get previous notifications ids */
	notifications_list = modest_conf_get_list (modest_runtime_get_conf (), 
						   MODEST_CONF_NOTIFICATION_IDS, 
						   MODEST_CONF_VALUE_INT, NULL);

	iter = tny_list_create_iterator (header_list);
	while (!tny_iterator_is_done (iter)) {
		gchar *url = NULL, *display_address = NULL, *display_date = NULL, *summary = NULL;
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
		TnyFolder *folder = tny_header_get_folder (header);
		gboolean first_notification = TRUE;
		gint notif_id;
	
		display_date = modest_text_utils_get_display_date (tny_header_get_date_received (header));

		display_address = g_strdup(tny_header_get_from (header));
		modest_text_utils_get_display_address (display_address); /* string is changed in-place */
		
		summary = g_strdup_printf ("%s - %s", display_date, display_address);
		notification = hildon_notification_new (summary,
							tny_header_get_subject (header),
							"qgn_list_messagin",
							"email.arrive");
		
		/* Create the message URL */
		url = g_strdup_printf ("%s/%s", tny_folder_get_url_string (folder), 
				       tny_header_get_uid (header));

		hildon_notification_add_dbus_action(notification,
						    "default",
						    "Cancel",
						    MODEST_DBUS_SERVICE,
						    MODEST_DBUS_OBJECT,
						    MODEST_DBUS_IFACE,
						    MODEST_DBUS_METHOD_OPEN_MESSAGE,
						    G_TYPE_STRING, url,
						    -1);

		/* Play sound if the user wants. Show the LED
		   pattern. Show and play just one */
		if (G_UNLIKELY (first_notification)) {
			first_notification = FALSE;
			if (modest_conf_get_bool (modest_runtime_get_conf (),
						  MODEST_CONF_PLAY_SOUND_MSG_ARRIVE,
						  NULL))  {
				notify_notification_set_hint_string(NOTIFY_NOTIFICATION (notification),
								    "sound-file", "/usr/share/sounds/ui-new_email.wav");
			}

			/* Set the led pattern */
			notify_notification_set_hint_int32 (NOTIFY_NOTIFICATION (notification),
							    "dialog-type", 4);
			notify_notification_set_hint_string(NOTIFY_NOTIFICATION (notification),
							    "led-pattern",
							    "PatternCommunicationEmail");			
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
	
		/* Free & carry on */
		g_free (display_date);
		g_free (display_address);
		g_free (summary);
		g_free (url);
		g_object_unref (folder);
		g_object_unref (header);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	/* Save the ids */
	modest_conf_set_list (modest_runtime_get_conf (), MODEST_CONF_NOTIFICATION_IDS, 
			      notifications_list, MODEST_CONF_VALUE_INT, NULL);

	g_slist_free (notifications_list);
	
#endif /*MODEST_HAVE_HILDON_NOTIFY*/
}

void
modest_platform_remove_new_mail_notifications (void) 
{
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
	g_return_if_fail (osso_context);

	result = hildon_help_show (osso_context, help_id, HILDON_HELP_SHOW_DIALOG);

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
modest_platform_set_dialog_help (GtkDialog *parent_window, 
				 const gchar *help_id)
{
	gboolean result;
	g_return_if_fail (help_id);
	g_return_if_fail (osso_context);
	g_return_if_fail (GTK_IS_DIALOG (parent_window));

	result = hildon_help_dialog_help_enable (parent_window, help_id, osso_context);

	if (!result)
		g_warning ("Help topic %s not found", help_id);

}

void 
modest_platform_show_search_messages (GtkWindow *parent_window)
{
	osso_return_t result = OSSO_ERROR;
	
	result = osso_rpc_run_with_defaults (osso_context, "osso_global_search", "search_email", NULL, DBUS_TYPE_INVALID);

	if (result != OSSO_OK) {
		g_warning ("%s: osso_rpc_run_with_defaults() failed.\n", __FUNCTION__);
	}
}

void 
modest_platform_show_addressbook (GtkWindow *parent_window)
{
	osso_return_t result = OSSO_ERROR;
	
	result = osso_rpc_run_with_defaults (osso_context, "osso_addressbook", "top_application", NULL, DBUS_TYPE_INVALID);

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
modest_platform_information_banner (GtkWidget *parent,
				    const gchar *icon_name,
				    const gchar *text)
{
	hildon_banner_show_information (parent, icon_name, text);
}

GtkWidget *
modest_platform_animation_banner (GtkWidget *parent,
				  const gchar *animation_name,
				  const gchar *text)
{
	GtkWidget *inf_note = NULL;

	g_return_val_if_fail (text != NULL, NULL);

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
on_timeout_check_account_is_online(gpointer user_data)
{
	printf ("DEBUG: %s:\n", __FUNCTION__);
	CheckAccountIdleData *data = (CheckAccountIdleData*)user_data;
	
	if (!data) {
		g_warning ("%s: data is NULL.\n", __FUNCTION__);
	}
	
	if (!(data->account)) {
		g_warning ("%s: data->account is NULL.\n", __FUNCTION__);
	}
	
	if (data && data->account) {
		printf ("DEBUG: %s: tny_account_get_connection_status()==%d\n", __FUNCTION__, tny_account_get_connection_status (data->account));	
	}
	
	gboolean stop_trying = FALSE;
	if (data && data->account && 
		/* We want to wait until TNY_CONNECTION_STATUS_INIT has changed to something else,
		 * after which the account is likely to be usable, or never likely to be usable soon: */
		(tny_account_get_connection_status (data->account) != TNY_CONNECTION_STATUS_INIT) )
	{
		data->is_online = TRUE;
		
		stop_trying = TRUE;
	}
	else {
		/* Give up if we have tried too many times: */
		if (data->count_tries >= NUMBER_OF_TRIES)
		{
			stop_trying = TRUE;
		}
		else {
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

	g_timeout_add (1000, on_timeout_check_account_is_online, data);

	/* This main loop will run until the idle handler has stopped it: */
	g_main_loop_run (data->loop);

	g_main_loop_unref (data->loop);
	/* g_main_context_unref (context); */

	g_slice_free (CheckAccountIdleData, data);
	
	return data->is_online;	
}



static void
on_cert_dialog_response (GtkDialog *dialog, gint response_id,  const gchar* cert)
{
	/* GTK_RESPONSE_HELP means we need to show the certificate */
	if (response_id == GTK_RESPONSE_HELP) {
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
modest_platform_run_certificate_conformation_dialog (const gchar* server_name,
						     const gchar *certificate)
{
	GtkWidget *note;
	gint response;
	GtkWindow *main_win =
		(GtkWindow*)modest_window_mgr_get_main_window (modest_runtime_get_window_mgr());

	gchar *question = g_strdup_printf (_("mcen_nc_unknown_certificate"),
					   server_name);
	
	note = hildon_note_new_confirmation_add_buttons  (
		main_win,
		question,
		_("mcen_bd_dialog_ok"),     GTK_RESPONSE_OK,
		_("mcen_bd_view"),          GTK_RESPONSE_HELP,   /* abusing this... */
		_("mcen_bd_dialog_cancel"), GTK_RESPONSE_CANCEL,
		NULL, NULL);
	
	g_signal_connect (G_OBJECT(note), "response", 
			  G_CALLBACK(on_cert_dialog_response),
			  (gpointer) certificate);
	
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
				     GTK_WINDOW (note));
	response = gtk_dialog_run(GTK_DIALOG(note));

	on_destroy_dialog (GTK_DIALOG(note));
	g_free (question);
	
	return response;
}
	


gboolean
modest_platform_run_alert_dialog (const gchar* prompt, 
				  gboolean is_question)
{	
	ModestWindow *main_window = 
		modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ());
	
	gboolean retval = TRUE;
	if (is_question) {
		/* The Tinymail documentation says that we should show Yes and No buttons, 
		 * when it is a question.
		 * Obviously, we need tinymail to use more specific error codes instead,
		 * so we know what buttons to show. */
		GtkWidget *dialog = GTK_WIDGET (hildon_note_new_confirmation (GTK_WINDOW (main_window), 
									      prompt));
		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), 
					     GTK_WINDOW (dialog));
		
		const int response = gtk_dialog_run (GTK_DIALOG (dialog));
		retval = (response == GTK_RESPONSE_YES) || (response == GTK_RESPONSE_OK);
		
		on_destroy_dialog (GTK_DIALOG(dialog));		
	} else {
	 	/* Just show the error text and use the default response: */
	 	modest_platform_run_information_dialog (GTK_WINDOW (main_window), 
							prompt);
	}
	return retval;
}

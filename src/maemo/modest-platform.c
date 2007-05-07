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

#include <modest-hildon-includes.h>

#include <dbus_api/modest-dbus-callbacks.h>
#include <maemo/modest-osso-autosave-callbacks.h>
#include <libosso.h>
#include <alarmd/alarm_event.h> /* For alarm_event_add(), etc. */
#include <tny-maemo-conic-device.h>
#include <tny-folder.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmain.h>
#include <string.h>
	
gboolean
modest_platform_init (void)
{	
	osso_context_t *osso_context =
		osso_initialize(PACKAGE, PACKAGE_VERSION,
				TRUE, NULL);	
	if (!osso_context) {
		g_printerr ("modest: failed to acquire osso context\n");
		return FALSE;
	}

	/* Register our D-Bus callbacks, via the osso API: */
	osso_return_t result = osso_rpc_set_cb_f(osso_context, 
                               MODEST_DBUS_SERVICE, 
                               MODEST_DBUS_OBJECT, 
                               MODEST_DBUS_IFACE,
                               modest_dbus_req_handler, NULL /* user_data */);
    	if (result != OSSO_OK) {
       		g_print("Error setting D-BUS callback (%d)\n", result);
       		return OSSO_ERROR;
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
    osso_hw_set_event_cb(osso_context, NULL, modest_osso_cb_hw_state_handler, NULL);

	/* Register osso auto-save callbacks: */
	result = osso_application_set_autosave_cb (osso_context, 
		modest_on_osso_application_autosave, NULL /* user_data */);
	if (result != OSSO_OK) {
		g_warning ("osso_application_set_autosave_cb() failed.");	
	}
	
	
	/* TODO: Get the actual update interval from gconf, 
	 * when that preferences dialog has been implemented.
	 * And make sure that this is called again whenever that is changed. */
	const guint update_interval_minutes = 15;
	modest_platform_set_update_interval (update_interval_minutes);
	
	return TRUE;
}

TnyDevice*
modest_platform_get_new_device (void)
{
	return TNY_DEVICE (tny_maemo_conic_device_new ());
}


const gchar*
guess_mime_type_from_name (const gchar* name)
{
	int i;
	const gchar* ext;
	const static gchar* octet_stream= "application/octet-stream";
	const static gchar* mime_map[][2] = {
		{ "pdf",  "application/pdf"},
		{ "doc",  "application/msword"},
		{ "xls",  "application/excel"},
		{ "png",  "image/png" },
		{ "gif",  "image/gif" },
		{ "jpg",  "image/jpeg"},
		{ "jpeg", "image/jpeg"},
		{ "mp3",  "audio/mp3" }
	};

	if (!name)
		return octet_stream;
	
	ext = g_strrstr (name, ".");
	if (!ext)
		return octet_stream;
	
	for (i = 0; i != G_N_ELEMENTS(mime_map); ++i) {
		if (g_ascii_strcasecmp (mime_map[i][0], ext + 1)) /* +1: ignore '.'*/
			return mime_map[i][1];
	}
	return octet_stream;
}


gchar*
modest_platform_get_file_icon_name (const gchar* name, const gchar* mime_type,
					  gchar **effective_mime_type)
{
	GString *mime_str = NULL;
	gchar *icon_name  = NULL;
	gchar **icons, **cursor;
	
	
	g_return_val_if_fail (name || mime_type, NULL);

	if (!mime_type || g_ascii_strcasecmp (mime_type, "application/octet-stream")) 
		mime_str = g_string_new (guess_mime_type_from_name(name));
	else {
		mime_str = g_string_new (mime_type);
		g_string_ascii_down (mime_str);
	}
#ifdef MODEST_HILDON_VERSION_0
	icons = osso_mime_get_icon_names (mime_str->str, NULL);
#else
	icons = hildon_mime_get_icon_names (mime_str->str, NULL);
#endif /*MODEST_HILDON_VERSION_0*/
	for (cursor = icons; cursor; ++cursor) {
		if (gtk_icon_theme_has_icon (gtk_icon_theme_get_default(), *cursor)) {
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
	gboolean result;

#ifdef MODEST_HILDON_VERSION_0
	result = osso_uri_open (uri, NULL, NULL);
#else
	result = hildon_uri_open (uri, NULL, NULL);
#endif

	if (!result)
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_unsupported_link"));
	return result;
}

typedef struct  {
	GSList * actions;
	gchar *uri;
} ModestPlatformPopupInfo;

static gboolean
delete_uri_popup (GtkWidget *menu,
		  GdkEvent *event,
		  gpointer userdata)
{
	ModestPlatformPopupInfo *popup_info = (ModestPlatformPopupInfo *) userdata;

	g_free (popup_info->uri);
#ifdef MODEST_HILDON_VERSION_0
	osso_uri_free_actions (popup_info->actions);
#else
	hildon_uri_free_actions (popup_info->actions);
#endif
	return FALSE;
}

static void
activate_uri_popup_item (GtkMenuItem *menu_item,
			 gpointer userdata)
{
	GSList *node;
	ModestPlatformPopupInfo *popup_info = (ModestPlatformPopupInfo *) userdata;
	GtkWidget *label;

	label = gtk_bin_get_child (GTK_BIN (menu_item));

	for (node = popup_info->actions; node != NULL; node = g_slist_next (node)) {
#ifdef MODEST_HILDON_VERSION_0
		OssoURIAction *action = (OssoURIAction *) node->data;
		if (strcmp (gtk_label_get_text (GTK_LABEL(label)), osso_uri_action_get_name (action))==0) {
			osso_uri_open (popup_info->uri, action, NULL);
			break;
		}
#else
		HildonURIAction *action = (HildonURIAction *) node->data;
		if (strcmp (gtk_label_get_text (GTK_LABEL(label)), hildon_uri_action_get_name (action))==0) {
			hildon_uri_open (popup_info->uri, action, NULL);
			break;
		}
#endif
	}
}

gboolean 
modest_platform_show_uri_popup (const gchar *uri)
{
	gchar *scheme;
	GSList *actions_list;

	if (uri == NULL)
		return FALSE;
#ifdef MODEST_HILDON_VERSION_0
	scheme = osso_uri_get_scheme_from_uri (uri, NULL);
	actions_list = osso_uri_get_actions (scheme, NULL);
#else
	scheme = hildon_uri_get_scheme_from_uri (uri, NULL);
	actions_list = hildon_uri_get_actions (scheme, NULL);
#endif
	if (actions_list != NULL) {
		GSList *node;
		GtkWidget *menu = gtk_menu_new ();
		ModestPlatformPopupInfo *popup_info = g_new0 (ModestPlatformPopupInfo, 1);

		popup_info->actions = actions_list;
		popup_info->uri = g_strdup (uri);
	      
		for (node = actions_list; node != NULL; node = g_slist_next (node)) {
			GtkWidget *menu_item;

#ifdef MODEST_HILDON_VERSION_0
			OssoURIAction *action;

			action = (OssoURIAction *) node->data;
			menu_item = gtk_menu_item_new_with_label (osso_uri_action_get_name (action));
			g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (activate_uri_popup_item), popup_info);
			
			if (osso_uri_is_default_action (action, NULL)) {
				gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
			} else {
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
			}
#else
			HildonURIAction *action;

			action = (HildonURIAction *) node->data;
			menu_item = gtk_menu_item_new_with_label (hildon_uri_action_get_name (action));
			g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (activate_uri_popup_item), popup_info);
			
			if (hildon_uri_is_default_action (action, NULL)) {
				gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
			} else {
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
			}
#endif

			gtk_widget_show (menu_item);
		}
		g_signal_connect (G_OBJECT (menu), "delete-event", G_CALLBACK (delete_uri_popup), popup_info);
		gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time ());
						  
	} else {
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_unsupported_link"));
	}
		
	g_free (scheme);
	return TRUE;
}


GdkPixbuf*
modest_platform_get_icon (const gchar *name)
{
	GError *err = NULL;
	GdkPixbuf* pixbuf = NULL;
	GtkIconTheme *current_theme = NULL;

	g_return_val_if_fail (name, NULL);

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
	chars_length = strlen (chars);

	/* Show WID-INF036 */
	if (chars_length == 20) {
		hildon_banner_show_information  (gtk_widget_get_parent (GTK_WIDGET (data)), NULL,
						 _("mcen_ib_maxchar_reached"));
	} else {
		if (chars_length == 0) {
			/* A blank space is not valid as first character */
			if (strcmp (text, " ")) {
				GtkWidget *ok_button;
				GList *buttons;

				/* Show OK button */
				buttons = gtk_container_get_children (GTK_CONTAINER (GTK_DIALOG (data)->action_area));
				ok_button = GTK_WIDGET (buttons->next->data);
				gtk_widget_set_sensitive (ok_button, TRUE);
				g_list_free (buttons);
			}
		}

		/* Write the text in the entry */
		g_signal_handlers_block_by_func (editable,
						 (gpointer) entry_insert_text, data);
		gtk_editable_insert_text (editable, text, length, position);
		g_signal_handlers_unblock_by_func (editable,
						   (gpointer) entry_insert_text, data);
	}
	/* Do not allow further processing */
	g_signal_stop_emission_by_name (editable, "insert_text");
}

static void
entry_changed (GtkEditable *editable,
	       gpointer     user_data)
{
	gchar *chars;

	chars = gtk_editable_get_chars (editable, 0, -1);

	/* Dimm OK button */
	if (strlen (chars) == 0) {
		GtkWidget *ok_button;
		GList *buttons;

		buttons = gtk_container_get_children (GTK_CONTAINER (GTK_DIALOG (user_data)->action_area));
		ok_button = GTK_WIDGET (buttons->next->data);
		gtk_widget_set_sensitive (ok_button, FALSE);

		g_list_free (buttons);
	}
	g_free (chars);
}

static void
launch_sort_headers_dialog (GtkWindow *parent_window,
			    HildonSortDialog *dialog)
{
	ModestHeaderView *header_view = NULL;
	GList *cols = NULL;
	GList *tmp = NULL;
	GtkSortType sort_type;
	gint sort_key;
	gint result;
	
	/* Get header window */
	if (MODEST_IS_MAIN_WINDOW (parent_window)) {
		header_view = MODEST_HEADER_VIEW(modest_main_window_get_child_widget (MODEST_MAIN_WINDOW(parent_window),
										      MODEST_WIDGET_TYPE_HEADER_VIEW));
	}
	if (!header_view) return;

	/* Add sorting keys */
	cols = modest_header_view_get_columns (header_view);	
	if (cols == NULL) return;
	int num_cols = g_list_length(cols);
	int sort_ids[num_cols];
	int sort_model_ids[num_cols];
	GtkTreeViewColumn *sort_cols[num_cols];
	for (tmp=cols; tmp; tmp=tmp->next) {
		gint col_id = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(tmp->data), MODEST_HEADER_VIEW_COLUMN));
		switch (col_id) {
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_FLAG:
			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_attachment"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_HEADER_FLAG_ATTACHMENTS;
			sort_cols[sort_key] = tmp->data;

			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_priority"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_HEADER_FLAG_PRIORITY;
			sort_cols[sort_key] = tmp->data;
			break;
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT:
			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_sender_recipient"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN;
			sort_cols[sort_key] = tmp->data;

			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_subject"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN;
			sort_cols[sort_key] = tmp->data;
			break;
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN:
			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_sender_recipient"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN;
			sort_cols[sort_key] = tmp->data;

			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_subject"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN;
			sort_cols[sort_key] = tmp->data;
			break;
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_RECEIVED_DATE:
			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_date"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
			sort_cols[sort_key] = tmp->data;
			break;
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_SENT_DATE:
			sort_key = hildon_sort_dialog_add_sort_key (dialog, _("mcen_li_sort_date"));
			sort_ids[sort_key] = col_id;
			sort_model_ids[sort_key] = TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
			sort_cols[sort_key] = tmp->data;
			break;
		default:
			return;
		}
	}
	
	/* Launch dialogs */
	result = gtk_dialog_run (GTK_DIALOG (dialog));
	if (result == GTK_RESPONSE_OK) {
		sort_key = hildon_sort_dialog_get_sort_key (dialog);
		sort_type = hildon_sort_dialog_get_sort_order (dialog);
		if (sort_ids[sort_key] == MODEST_HEADER_VIEW_COLUMN_COMPACT_FLAG)
			g_object_set_data(G_OBJECT(sort_cols[sort_key]), 
					  MODEST_HEADER_VIEW_FLAG_SORT, 
					  GINT_TO_POINTER(sort_model_ids[sort_key]));
		
		else
			gtk_tree_view_column_set_sort_column_id (sort_cols[sort_key], sort_model_ids[sort_key]);
		
		modest_header_view_sort_by_column_id (header_view, sort_ids[sort_key], sort_type);
	}
	
	/* free */
	g_list_free(cols);	
}



gint
modest_platform_run_new_folder_dialog (GtkWindow *parent_window,
				       TnyFolderStore *parent_folder,
				       gchar *suggested_name,
				       gchar **folder_name)
{
	GtkWidget *dialog, *entry, *label, *hbox;
	gint result;

	/* Ask the user for the folder name */
	dialog = gtk_dialog_new_with_buttons (_("mcen_ti_new_folder"),
					      parent_window,
					      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      NULL);

	/* Create label and entry */
	label = gtk_label_new (_("mcen_fi_new_folder_name"));
	/* TODO: check that the suggested name does not exist */
	/* We set 21 as maximum because we want to show WID-INF036
	   when the user inputs more that 20 */
	entry = gtk_entry_new_with_max_length (21);
	if (suggested_name)
		gtk_entry_set_text (GTK_ENTRY (entry), suggested_name);
	else
		gtk_entry_set_text (GTK_ENTRY (entry), _("mcen_ia_default_folder_name"));
	gtk_entry_select_region (GTK_ENTRY (entry), 0, -1);

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
	
	result = gtk_dialog_run (GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT)
		*folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	gtk_widget_destroy (dialog);

	return result;
}

gint
modest_platform_run_confirmation_dialog (GtkWindow *parent_window,
					 const gchar *message)
{
	GtkWidget *dialog;
	gint response;

	dialog = hildon_note_new_confirmation (parent_window, message);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (GTK_WIDGET (dialog));

	return response;
}

void
modest_platform_run_information_dialog (GtkWindow *parent_window,
					ModestInformationDialogType type)
{
	GtkWidget *dialog;
	gchar *message = NULL;

	switch (type) {
	case MODEST_INFORMATION_CREATE_FOLDER:
		message = _("mail_in_ui_folder_create_error");
		break;
	case MODEST_INFORMATION_DELETE_FOLDER:
		message = _("mail_in_ui_folder_delete_error");
		break;
	};

	dialog = hildon_note_new_information (parent_window, message);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

gboolean modest_platform_connect_and_wait (GtkWindow *parent_window)
{
	TnyDevice *device = modest_runtime_get_device();
	
	if (tny_device_is_online (device))
		return TRUE;
		
	/* TODO: Block on the result: */
	gboolean request_sent = tny_maemo_conic_device_connect (TNY_MAEMO_CONIC_DEVICE (device), NULL);
	if (!request_sent)
		return FALSE;

	return TRUE;
}

void
modest_platform_run_sort_dialog (GtkWindow *parent_window,
				 ModestSortDialogType type)
{
	GtkWidget *dialog = NULL;

	/* Build dialog */
	dialog = hildon_sort_dialog_new (parent_window);
	gtk_window_set_modal (GTK_WINDOW(dialog), TRUE);
	
	/* Fill sort keys */
	switch (type) {
	case MODEST_SORT_HEADERS:
		launch_sort_headers_dialog (parent_window, 
					    HILDON_SORT_DIALOG(dialog));
		break;
	}
	
	/* Free */
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


gboolean modest_platform_set_update_interval (guint minutes)
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
	
	/* Get current time: */
	time_t time_now;
	time (&time_now);
	struct tm *st_time = localtime (&time_now);
	
	/* Add minutes to tm_min field: */
	st_time->tm_min += minutes;
	
	/* Set the time in alarm_event_t structure: */
	alarm_event_t event;
	memset (&event, 0, sizeof (alarm_event_t));
	event.alarm_time = mktime (st_time);

	/* Specify what should happen when the alarm happens:
	 * It should call this D-Bus method: */
	 
	/* Note: I am surpised that alarmd can't just use the modest.service file
	 * for this. murrayc. */
	event.dbus_path = g_strdup(PREFIX "/bin/modest");
	
	event.dbus_interface = g_strdup (MODEST_DBUS_IFACE);
	event.dbus_service = g_strdup (MODEST_DBUS_SERVICE);
	event.dbus_name = g_strdup (MODEST_DBUS_METHOD_SEND_RECEIVE);
	
	alarm_cookie = alarm_event_add (&event);
	
	/* Store the alarm ID in GConf, so we can remove it later:
	 * This is apparently valid between application instances. */
	modest_conf_set_int (conf, MODEST_CONF_ALARM_ID, alarm_cookie, NULL);
	
	if (!alarm_cookie) {
	    /* Error */
	    const alarm_error_t alarm_error = alarmd_get_error ();
	    printf ("Error setting alarm event. Error code: '%d'\n", alarm_error);
	    
	    /* Give people some clue: */
	    /* The alarm API should have a function for this: */
	    if (alarm_error == ALARMD_ERROR_DBUS) {
	    	printf ("  ALARMD_ERROR_DBUS: An error with D-Bus occurred, probably coudn't get a D-Bus connection.\n");
	    } else if (alarm_error == ALARMD_ERROR_CONNECTION) {
	    	printf ("  ALARMD_ERROR_CONNECTION: Could not contact alarmd via D-Bus.\n");
	    } else if (alarm_error == ALARMD_ERROR_INTERNAL) {
	    	printf ("  ALARMD_ERROR_INTERNAL: Some alarmd or libalarm internal error, possibly a version mismatch.\n");
	    } else if (alarm_error == ALARMD_ERROR_MEMORY) {
	    	printf ("  ALARMD_ERROR_MEMORY: A memory allocation failed.\n");
	    } else if (alarm_error == ALARMD_ERROR_ARGUMENT) {
	    	printf ("  ALARMD_ERROR_ARGUMENT: An argument given by caller was invalid.\n");
	    }
	    
	    return FALSE;
	}
	
	return TRUE;
}


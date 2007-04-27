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
#include <dbus_api/modest-dbus-callbacks.h>
#include <libosso.h>
#include <modest-hildon-includes.h>
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
	GdkPixbuf* pixbuf;
	GtkIconTheme *current_theme;

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


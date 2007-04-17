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
#include <libosso.h>

#ifdef MODEST_HILDON_VERSION_0
#include <osso-mime.h>
#include <osso-uri.h>
#else
#include <hildon-mime.h>
#include <hildon-uri.h>
#endif /*MODEST_HILDON_VERSION_0*/

#include <tny-maemo-conic-device.h>
#include <gtk/gtkicontheme.h>
#include <hildon-widgets/hildon-banner.h>
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

	current_theme = gtk_icon_theme_get_default ();
	pixbuf = gtk_icon_theme_load_icon (current_theme, name, 26,
					   GTK_ICON_LOOKUP_NO_SVG,
					   &err);
	if (!pixbuf) {
		g_printerr ("modest: error while loading icon '%s': %s\n",
			    name, err->message);
		g_error_free (err);
	}
	
	return pixbuf;
}

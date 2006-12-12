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

#include "modest-widget-memory.h"

#include <modest-tny-platform-factory.h>
#include <widgets/modest-header-view.h>
#include <widgets/modest-msg-view.h>
#include <widgets/modest-folder-view.h>

#include <string.h>

#define PARAM_X             "x"
#define PARAM_Y             "y"
#define PARAM_HEIGHT        "height"
#define PARAM_WIDTH         "width"
#define PARAM_POS           "pos"
#define PARAM_COLUMN_WIDTH  "column-width"

static gchar*
get_keyname (ModestConf *conf, const gchar *name, const gchar *param)
{
	gchar *esc_name, *keyname;
	esc_name = modest_conf_key_escape (conf, name);

	keyname = g_strdup_printf ("%s/%s/%s",
				   MODEST_CONF_WIDGET_NAMESPACE, 
				   esc_name, param);
	g_free (esc_name);
	return keyname;
}


static gchar*
get_keyname_with_type (ModestConf *conf, const gchar *name, guint type, const gchar *param)
{
	gchar *esc_name, *keyname;
	esc_name = modest_conf_key_escape (conf, name);

	keyname = g_strdup_printf ("%s/%s/%s_%d",
				   MODEST_CONF_WIDGET_NAMESPACE, 
				   esc_name, param, type);
	g_free (esc_name);
	return keyname;
}


static gboolean
save_settings_widget (ModestConf *conf, GtkWidget *widget, const gchar *name)
{
	gchar *key;

	key = get_keyname (conf, name, PARAM_HEIGHT);
	modest_conf_set_int (conf, key, GTK_WIDGET(widget)->allocation.height, NULL);
	g_free (key);
	
	key = get_keyname (conf, name, PARAM_WIDTH);
	modest_conf_set_int (conf, key, GTK_WIDGET(widget)->allocation.width, NULL);
	g_free (key);

	return TRUE;
}


static gboolean
restore_settings_widget (ModestConf *conf, GtkWidget *widget, const gchar *name)
{
	GtkRequisition req;
	gchar *key;

	key = get_keyname (conf, name, PARAM_HEIGHT);
	
	if (modest_conf_key_exists (conf, key, NULL))
		req.height = modest_conf_get_int (conf, key, NULL);

	g_free (key);
	
	key = get_keyname (conf, name, PARAM_WIDTH);
	if (modest_conf_key_exists (conf, key, NULL))
		req.width = modest_conf_get_int (conf, key, NULL);
	g_free (key);

	if (req.height && req.width) 
		gtk_widget_size_request (widget, &req);

	return TRUE;

}



static gboolean
save_settings_window (ModestConf *conf, GtkWindow *win, const gchar *name)
{
	gchar *key;
	int height, width;
	
	gtk_window_get_size (win, &width, &height);
	
	key = get_keyname (conf, name, PARAM_HEIGHT);
	modest_conf_set_int (conf, key, height, NULL);
	g_free (key);
	
	key = get_keyname (conf, name, PARAM_WIDTH);
	modest_conf_set_int (conf, key, width, NULL);
	g_free (key);
	
	return TRUE;
}


static gboolean
restore_settings_window (ModestConf *conf, GtkWindow *win, const gchar *name)
{
	gchar *key;
	int height = 0, width = 0;

	key = get_keyname (conf, name, PARAM_HEIGHT);
	
	if (modest_conf_key_exists (conf, key, NULL))
		height = modest_conf_get_int (conf, key, NULL);

	g_free (key);

	key = get_keyname (conf, name, PARAM_WIDTH);
	if (modest_conf_key_exists (conf, key, NULL))
		width = modest_conf_get_int (conf, key, NULL);

	g_free (key);

	if (height && width)
		gtk_window_set_default_size (win, width, height);

	return TRUE;
}


static gboolean
save_settings_paned (ModestConf *conf, GtkPaned *paned, const gchar *name)
{
	gchar *key;
	int pos;

	pos = gtk_paned_get_position (paned);
	
	key = get_keyname (conf, name, PARAM_POS);
	modest_conf_set_int (conf, key, pos, NULL);
	g_free (key);
	
	return TRUE;
}


static gboolean
restore_settings_paned (ModestConf *conf, GtkPaned *paned, const gchar *name)
{
 	gchar *key;
	int pos;
	
	key = get_keyname (conf, name, PARAM_POS);
	
	if (modest_conf_key_exists (conf, key, NULL)) {
		pos = modest_conf_get_int (conf, key, NULL);
		gtk_paned_set_position (paned, pos);
	}

	g_free (key);
	return TRUE;
}


static gboolean
save_settings_header_view (ModestConf *conf, ModestHeaderView *header_view,
			   const gchar *name)
{
	gchar *key;
	GString *str;
	GList *cols, *cursor;
	TnyFolder *folder;
	TnyFolderType type;

	folder = modest_header_view_get_folder (header_view);
	if (!folder) 
		return TRUE; /* no folder: no settings */ 
	
	type = modest_folder_view_guess_folder_type (folder);
	key = get_keyname_with_type (conf, name, type, PARAM_COLUMN_WIDTH);

	cursor = cols = modest_header_view_get_columns (header_view);
	str = g_string_new (NULL);
	
	while (cursor) {

		int col_id, width;
		GtkTreeViewColumn *col;
		
		col    = GTK_TREE_VIEW_COLUMN (cursor->data);
		col_id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT(col),
							    MODEST_HEADER_VIEW_COLUMN));
		width = gtk_tree_view_column_get_width (col);
		
		g_string_append_printf (str, "%d:%d ", col_id, width);  
		
		cursor = g_list_next (cursor);
	}

	modest_conf_set_string (conf, key, str->str, NULL);

	g_free (key);	
	g_string_free (str, TRUE);
	g_list_free (cols);
	
	return TRUE;
}


static gboolean
restore_settings_header_view (ModestConf *conf, ModestHeaderView *header_view,
			      const gchar *name)
{
	gchar *key;
	TnyFolder *folder;
	TnyFolderType type;

	folder = modest_header_view_get_folder (header_view);
	if (!folder) 
		return TRUE; /* no folder: no settings */ 
	
	type = modest_folder_view_guess_folder_type (folder);
	
	key = get_keyname_with_type (conf, name, type, PARAM_COLUMN_WIDTH);
	if (modest_conf_key_exists (conf, key, NULL)) {
		
		gchar *data, *cursor;
		guint col, width;
		GList *cols = NULL;
		GList *colwidths = NULL;
	
		cursor = data = modest_conf_get_string (conf, key, NULL);
		while (cursor && sscanf (cursor, "%u:%u ", &col, &width) == 2) {
			cols      = g_list_append (cols, GINT_TO_POINTER(col));
			colwidths = g_list_append (colwidths, GINT_TO_POINTER(width));
			cursor = strchr (cursor + 1, ' ');
		}
		g_free (data);	
		
		if (cols) {
			GList *viewcolumns, *colcursor, *widthcursor;
			modest_header_view_set_columns (header_view, cols);

			widthcursor = colwidths;
			colcursor = viewcolumns = gtk_tree_view_get_columns (GTK_TREE_VIEW(header_view));
			while (colcursor && widthcursor) {
				gtk_tree_view_column_set_fixed_width(GTK_TREE_VIEW_COLUMN(colcursor->data),
								     GPOINTER_TO_INT(widthcursor->data));
				colcursor = g_list_next (colcursor);
				widthcursor = g_list_next (widthcursor);
			}
			g_list_free (cols);
			g_list_free (colwidths);
			g_list_free (viewcolumns);
		}
	}

	g_free (key);
	return TRUE;
}



static gboolean
save_settings_folder_view (ModestConf *conf, ModestFolderView *folder_view,
			   const gchar *name)
{
	return TRUE; /* FIXME: implement this */
}

static gboolean
restore_settings_folder_view (ModestConf *conf, ModestFolderView *folder_view,
			      const gchar *name)
{
	return TRUE; /* FIXME: implement this */
}


static gboolean
save_settings_msg_view (ModestConf *conf, ModestMsgView *msg_view,
			   const gchar *name)
{
	return TRUE; /* FIXME: implement this */
}

static gboolean
restore_settings_msg_view (ModestConf *conf, ModestMsgView *msg_view,
			      const gchar *name)
{
	return TRUE; /* FIXME: implement this */
}



gboolean
modest_widget_memory_save (ModestConf *conf, GObject *widget, const gchar *name)
{
	g_return_val_if_fail (conf, FALSE);
	g_return_val_if_fail (widget, FALSE);
	g_return_val_if_fail (name, FALSE);
	
	if (GTK_IS_WINDOW(widget))
		return save_settings_window (conf, GTK_WINDOW(widget), name);
	else if (GTK_IS_PANED(widget))
		return save_settings_paned (conf, GTK_PANED(widget), name);
	else if (MODEST_IS_HEADER_VIEW(widget))
		return save_settings_header_view (conf, MODEST_HEADER_VIEW(widget), name);
	else if (MODEST_IS_FOLDER_VIEW(widget))
		return save_settings_folder_view (conf, MODEST_FOLDER_VIEW(widget), name);
	else if (MODEST_IS_MSG_VIEW(widget))
		return save_settings_msg_view (conf, MODEST_MSG_VIEW(widget), name);
	else if (GTK_IS_WIDGET(widget))
		return save_settings_widget (conf, GTK_WIDGET(widget), name);
	
	g_printerr ("modest: %p (%s) is not a known widget\n", widget, name);	
	return FALSE;
}



gboolean
modest_widget_memory_restore (ModestConf *conf, GObject *widget, const gchar *name)
{
	g_return_val_if_fail (conf, FALSE);
	g_return_val_if_fail (widget, FALSE);
	g_return_val_if_fail (name, FALSE);
	
	if (GTK_IS_WINDOW(widget))
		return restore_settings_window (conf, GTK_WINDOW(widget), name);
	else if (GTK_IS_PANED(widget))
		return restore_settings_paned (conf, GTK_PANED(widget), name);
	else if (MODEST_IS_HEADER_VIEW(widget))
		return restore_settings_header_view (conf, MODEST_HEADER_VIEW(widget), name);
 	else if (MODEST_IS_FOLDER_VIEW(widget))
		return restore_settings_folder_view (conf, MODEST_FOLDER_VIEW(widget), name);
	else if (MODEST_IS_MSG_VIEW(widget))
		return restore_settings_msg_view (conf, MODEST_MSG_VIEW(widget), name);
	else if (GTK_IS_WIDGET(widget))
		return restore_settings_widget (conf, GTK_WIDGET(widget), name);
	
	g_printerr ("modest: %p (%s) is not a known widget\n", widget, name);
	return FALSE;
}

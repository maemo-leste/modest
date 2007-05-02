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

#include <modest-widget-memory.h>
#include <modest-widget-memory-priv.h>
#include <modest-runtime.h>
#include <modest-account-mgr-helpers.h>
#include <modest-tny-platform-factory.h>
#include <modest-tny-folder.h>
#include <widgets/modest-header-view.h>
#include <widgets/modest-msg-view.h>
#include <widgets/modest-folder-view.h>
#include "widgets/modest-main-window.h"
#include <string.h>

gchar*
_modest_widget_memory_get_keyname (const gchar *name, const gchar *param)
{
	gchar *esc_name, *keyname;

	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (param, NULL);
	
	esc_name = modest_conf_key_escape (name);

	keyname = g_strdup_printf ("%s/%s/%s",
				   MODEST_CONF_WIDGET_NAMESPACE, 
				   esc_name, param);
	g_free (esc_name);
	return keyname;
}


gchar*
_modest_widget_memory_get_keyname_with_type (const gchar *name, guint type,
					     const gchar *param)
{
	gchar *esc_name, *keyname;
	
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (param, NULL);

	esc_name = modest_conf_key_escape (name);

	keyname = g_strdup_printf ("%s/%s/%s_%d",
				   MODEST_CONF_WIDGET_NAMESPACE, 
				   esc_name, param, type);
	g_free (esc_name);
	return keyname;
}


gchar*
_modest_widget_memory_get_keyname_with_double_type (const gchar *name,
						    guint type1, guint type2,
						    const gchar *param)
{
	gchar *esc_name, *keyname;
	
	g_return_val_if_fail (name, NULL);
	g_return_val_if_fail (param, NULL);

	esc_name = modest_conf_key_escape (name);

	keyname = g_strdup_printf ("%s/%s/%s_%d_%d",
				   MODEST_CONF_WIDGET_NAMESPACE, 
				   esc_name, param, type1, type2);
	g_free (esc_name);
	return keyname;
}



static gboolean
save_settings_widget (ModestConf *conf, GtkWidget *widget, const gchar *name)
{
	gchar *key;

	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_HEIGHT);
	modest_conf_set_int (conf, key, GTK_WIDGET(widget)->allocation.height, NULL);
	g_free (key);
	
	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_WIDTH);
	modest_conf_set_int (conf, key, GTK_WIDGET(widget)->allocation.width, NULL);
	g_free (key);

	return TRUE;
}


static gboolean
restore_settings_widget (ModestConf *conf, GtkWidget *widget, const gchar *name)
{
	GtkRequisition req;
	gchar *key;

	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_HEIGHT);
	
	if (modest_conf_key_exists (conf, key, NULL))
		req.height = modest_conf_get_int (conf, key, NULL);

	g_free (key);
	
	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_WIDTH);
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
	
	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_HEIGHT);
	modest_conf_set_int (conf, key, height, NULL);
	g_free (key);
	
	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_WIDTH);
	modest_conf_set_int (conf, key, width, NULL);
	g_free (key);

	/* Save also the main window style */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		ModestMainWindowStyle style = modest_main_window_get_style (MODEST_MAIN_WINDOW (win));

		key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_WINDOW_STYLE);
		modest_conf_set_int (conf, key, style, NULL);
		g_free (key);
	}
	
	return TRUE;
}


static gboolean
restore_settings_window (ModestConf *conf, GtkWindow *win, const gchar *name)
{
	gchar *key;
	int height = 0, width = 0;

	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_HEIGHT);
	
	if (modest_conf_key_exists (conf, key, NULL))
		height = modest_conf_get_int (conf, key, NULL);

	g_free (key);

	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_WIDTH);
	if (modest_conf_key_exists (conf, key, NULL))
		width = modest_conf_get_int (conf, key, NULL);

	g_free (key);

	/* Added this ugly ifdef, because in Maemo the
	   gtk_window_set_default_size() makes "drag-motion" signal
	   report bad coordinates, so drag-and-drop do not work
	   properly */
#ifdef MODEST_PLATFORM_GNOME
	if (height && width)
		gtk_window_set_default_size (win, width, height);
#endif

	/* Restore also the main window style */
	if (MODEST_IS_MAIN_WINDOW (win)) {
		ModestMainWindowStyle style;

		key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_WINDOW_STYLE);
		if (modest_conf_key_exists (conf, key, NULL)) {
			style = (ModestMainWindowStyle) modest_conf_get_int (conf, key, NULL);		
			modest_main_window_set_style (MODEST_MAIN_WINDOW (win), style);
			g_free (key);
		}
	}

	return TRUE;
}


static gboolean
save_settings_paned (ModestConf *conf, GtkPaned *paned, const gchar *name)
{
	gchar *key;
	int pos;

	pos = gtk_paned_get_position (paned);
	
	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_POS);
	modest_conf_set_int (conf, key, pos, NULL);
	g_free (key);
	
	return TRUE;
}


static gboolean
restore_settings_paned (ModestConf *conf, GtkPaned *paned, const gchar *name)
{
 	gchar *key;
	int pos;
	
	key = _modest_widget_memory_get_keyname (name, MODEST_WIDGET_MEMORY_PARAM_POS);
	
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
	ModestHeaderViewStyle style;
	gint sort_colid;
	GtkSortType sort_type;
	
	folder = modest_header_view_get_folder (header_view);
	if (!folder || modest_header_view_is_empty (header_view))
		return TRUE; /* no non-empty folder: no settings */
	
	type  = modest_tny_folder_guess_folder_type (folder);
	style = modest_header_view_get_style   (header_view);
	
	key = _modest_widget_memory_get_keyname_with_double_type (name, type, style,
								  MODEST_WIDGET_MEMORY_PARAM_COLUMN_WIDTH);

	cursor = cols = modest_header_view_get_columns (header_view);
	str = g_string_new (NULL);

	/* NOTE: the exact details of this format are important, as they
	 * are also used in modest-init.
	 */
	sort_colid = modest_header_view_get_sort_column_id (header_view, type); 
	sort_type = modest_header_view_get_sort_type (header_view, type); 
	while (cursor) {

		int col_id, width, sort;
		GtkTreeViewColumn *col;
		
		col    = GTK_TREE_VIEW_COLUMN (cursor->data);
		col_id = GPOINTER_TO_INT(g_object_get_data (G_OBJECT(col),
							    MODEST_HEADER_VIEW_COLUMN));
		width = gtk_tree_view_column_get_width (col);
		sort = 0;
		if (sort_colid == col_id)
			sort = (sort_type == GTK_SORT_ASCENDING) ? 1:-1;
			
		g_string_append_printf (str, "%d:%d:%d ", col_id, width, sort);
		cursor = g_list_next (cursor);
	}

	modest_conf_set_string (conf, key, str->str, NULL);

	g_free (key);	
	g_string_free (str, TRUE);
	g_list_free (cols);
	g_object_unref (G_OBJECT (folder));
	
	return TRUE;
}


static gboolean
restore_settings_header_view (ModestConf *conf, ModestHeaderView *header_view,
			      const gchar *name)
{
	guint col, width;
	gint sort;
	gchar *key;
	TnyFolder *folder;
	TnyFolderType type;
	ModestHeaderViewStyle style;

	folder = modest_header_view_get_folder (header_view);
	if (!folder || modest_header_view_is_empty (header_view))
		return TRUE; /* no non-empty folder: no settings */
	
	type = modest_tny_folder_guess_folder_type (folder);
	style = modest_header_view_get_style   (header_view);

	key = _modest_widget_memory_get_keyname_with_double_type (name, type, style,
								  MODEST_WIDGET_MEMORY_PARAM_COLUMN_WIDTH);
	if (modest_conf_key_exists (conf, key, NULL)) {
		
		gchar *data, *cursor;
		GList *cols = NULL;
		GList *colwidths = NULL;
		GList *colsortables = NULL;
		GtkTreeModel *sortable;

		cursor = data = modest_conf_get_string (conf, key, NULL);
		while (cursor && sscanf (cursor, "%d:%d:%d ", &col, &width, &sort) == 3) {
			cols      = g_list_append (cols, GINT_TO_POINTER(col));
			colwidths = g_list_append (colwidths, GINT_TO_POINTER(width));
			colsortables = g_list_append (colsortables, GINT_TO_POINTER(sort));
			cursor = strchr (cursor + 1, ' ');
		}
		g_free (data);	
		
		if (cols) {
			GList *viewcolumns, *colcursor, *widthcursor, *sortablecursor;
			modest_header_view_set_columns (header_view, cols, type);
			sortable = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));

			widthcursor = colwidths;
			sortablecursor = colsortables;
			colcursor = viewcolumns = gtk_tree_view_get_columns (GTK_TREE_VIEW(header_view));
			while (colcursor && widthcursor && sortablecursor) {
				int width = GPOINTER_TO_INT(widthcursor->data);
				int sort = GPOINTER_TO_INT(sortablecursor->data);
				if (width > 0)
					gtk_tree_view_column_set_max_width(GTK_TREE_VIEW_COLUMN(colcursor->data),
									   width);
				if (sort != 0) {
					int colid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(colcursor->data), MODEST_HEADER_VIEW_COLUMN));
					GtkSortType sort_type = (sort == 1) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING; 
					modest_header_view_set_sort_params (header_view, colid, sort_type, type);
					gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(sortable),
									      colid,
									      sort_type);
					gtk_tree_sortable_sort_column_changed (GTK_TREE_SORTABLE(sortable));
				}
				colcursor = g_list_next (colcursor);
				widthcursor = g_list_next (widthcursor);
				sortablecursor = g_list_next (sortablecursor);
			}

			g_list_free (cols);
			g_list_free (colwidths);
			g_list_free (colsortables);
			g_list_free (viewcolumns);
		}
	}

	g_free (key);
	g_object_unref (G_OBJECT (folder));

	return TRUE;
}



static gboolean
save_settings_folder_view (ModestConf *conf, ModestFolderView *folder_view,
			   const gchar *name)
{
	gchar *key;
	const gchar* account_id;

	/* Restore the visible account */
	key = _modest_widget_memory_get_keyname (name, "visible_server_account_id");

	account_id = modest_folder_view_get_account_id_of_visible_server_account (folder_view);
	if (account_id)
		modest_conf_set_string (conf, key, account_id, NULL);

	g_free (key);

	return TRUE;
}

static gboolean
restore_settings_folder_view (ModestConf *conf, 
			      ModestFolderView *folder_view,
			      const gchar *name)
{
	gchar *key, *account_id;

	/* Restore the visible account */
	key = _modest_widget_memory_get_keyname (name, "visible_server_account_id");

	if (modest_conf_key_exists (conf, key, NULL)) {
		account_id = modest_conf_get_string (conf, key, NULL);
		modest_folder_view_set_account_id_of_visible_server_account (folder_view, 
									     (const gchar *) account_id);
		g_free (account_id);
	} else {
		ModestAccountMgr *mgr;
		gchar *default_acc;

		/* If there is no visible account id in the
		   configuration then pick the default account as
		   visible account */
		mgr = modest_runtime_get_account_mgr ();
		default_acc = modest_account_mgr_get_default_account (mgr);
		if (default_acc) {
			ModestAccountData *acc_data;
			const gchar *server_acc_id;

			acc_data = modest_account_mgr_get_account_data (mgr, (const gchar*) default_acc);
			server_acc_id = (const gchar *) acc_data->store_account->account_name;

			modest_conf_set_string (conf, key, server_acc_id, NULL);
			modest_folder_view_set_account_id_of_visible_server_account (folder_view, server_acc_id);

			g_free (default_acc);
		}
	}

	g_free (key);

	return TRUE;
}


static gboolean
save_settings_msg_view (ModestConf *conf, 
			ModestMsgView *msg_view,
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

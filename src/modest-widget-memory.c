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

#define PARAM_X           "x"
#define PARAM_Y           "y"
#define PARAM_HEIGHT      "height"
#define PARAM_WIDTH       "width"
#define PARAM_POS         "pos"

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
modest_widget_memory_save_settings_treeview (ModestConf *conf, GtkTreeView *treeview,
					     const gchar *name)
{
	GList *cols;

	cols = gtk_tree_view_get_columns (treeview);
	/* FIXME: implement this */
	while (cols) {
		gint size = gtk_tree_view_column_get_width (GTK_TREE_VIEW_COLUMN(cols->data));
		cols = g_list_next (cols);
	}
	
	
	g_list_free (cols);
	
	return TRUE;
}



static gboolean
modest_widget_memory_restore_settings_treeview (ModestConf *conf, GtkTreeView *treeview,
						const gchar *name)
{
	/* FIXME */
	return TRUE;
}


gboolean
modest_widget_memory_save_settings (ModestConf *conf, GtkWidget *widget,
				    const gchar *name)
{
	g_return_val_if_fail (conf, FALSE);
	g_return_val_if_fail (widget, FALSE);
	g_return_val_if_fail (name, FALSE);

	if (GTK_IS_WINDOW(widget))
		return save_settings_window (conf, (GtkWindow*)widget, name);
	else if (GTK_IS_PANED(widget))
		return save_settings_paned (conf, (GtkPaned*)widget, name);
	else if (GTK_IS_WIDGET(widget))
		return save_settings_widget (conf, widget, name);

	return TRUE;
}



gboolean
modest_widget_memory_restore_settings (ModestConf *conf, GtkWidget *widget,
				       const gchar *name)
{
	g_return_val_if_fail (conf, FALSE);
	g_return_val_if_fail (widget, FALSE);
	g_return_val_if_fail (name, FALSE);
	
	if (GTK_IS_WINDOW(widget))
		return restore_settings_window (conf, (GtkWindow*)widget, name);
	else if (GTK_IS_PANED(widget))
		return restore_settings_paned (conf, (GtkPaned*)widget, name);
	else if (GTK_IS_WIDGET(widget))
		return restore_settings_widget (conf, widget, name);
	
	return TRUE;
}

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

#include <modest-header-view.h>
#include <modest-header-view-priv.h>
#include <modest-icon-names.h>
#include <modest-text-utils.h>
#include <modest-runtime.h>
#include <glib/gi18n.h>

static GdkPixbuf*
get_cached_icon (const gchar *name)
{
	GError *err = NULL;
	gpointer pixbuf;
	gpointer orig_key;
	static GHashTable *icon_cache = NULL;
	
	g_return_val_if_fail (name, NULL);

	if (G_UNLIKELY(!icon_cache))
		icon_cache = modest_cache_mgr_get_cache (modest_runtime_get_cache_mgr(),
							 MODEST_CACHE_MGR_CACHE_TYPE_PIXBUF);
	
	if (!icon_cache || !g_hash_table_lookup_extended (icon_cache, name, &orig_key,&pixbuf)) {
		pixbuf = (gpointer)gdk_pixbuf_new_from_file (name, &err);
		if (!pixbuf) {
			g_printerr ("modest: error while loading '%s': %s\n",
				    name, err->message);
			g_error_free (err);
		}
		/* if we cannot find it, we still insert (if we have a cache), so we get the error
		 * only once */
		if (icon_cache)
			g_hash_table_insert (icon_cache, g_strdup(name),(gpointer)pixbuf);
	}
	return GDK_PIXBUF(pixbuf);
}



/*
 * optimization
 */
static GdkPixbuf*
get_pixbuf_for_flag (TnyHeaderFlags flag)
{
	/* optimization */
	static GdkPixbuf *deleted_pixbuf       = NULL;
	static GdkPixbuf *seen_pixbuf          = NULL;
	static GdkPixbuf *unread_pixbuf        = NULL;
	static GdkPixbuf *attachments_pixbuf   = NULL;
	
	switch (flag) {
	case TNY_HEADER_FLAG_DELETED:
		if (G_UNLIKELY(!deleted_pixbuf))
			deleted_pixbuf = get_cached_icon (MODEST_HEADER_ICON_DELETED);
		return deleted_pixbuf;
	case TNY_HEADER_FLAG_SEEN:
		if (G_UNLIKELY(!seen_pixbuf))
			seen_pixbuf = get_cached_icon (MODEST_HEADER_ICON_READ);
		return seen_pixbuf;
	case TNY_HEADER_FLAG_ATTACHMENTS:
		if (G_UNLIKELY(!attachments_pixbuf))
			attachments_pixbuf = get_cached_icon (MODEST_HEADER_ICON_ATTACH);
		return attachments_pixbuf;
	default:
		if (G_UNLIKELY(!unread_pixbuf))
			unread_pixbuf = get_cached_icon (MODEST_HEADER_ICON_UNREAD);
		return unread_pixbuf;
	}
}


void
_modest_header_view_msgtype_cell_data (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
		   GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
	TnyHeaderFlags flags;
		
	gtk_tree_model_get (tree_model, iter, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);

	if (flags & TNY_HEADER_FLAG_DELETED)
		g_object_set (G_OBJECT (renderer), "pixbuf",
			      get_pixbuf_for_flag (TNY_HEADER_FLAG_DELETED), NULL);	      
	else if (flags & TNY_HEADER_FLAG_SEEN)
		g_object_set (G_OBJECT (renderer), "pixbuf",
			      get_pixbuf_for_flag (TNY_HEADER_FLAG_SEEN), NULL);	      
	else 
		g_object_set (G_OBJECT (renderer), "pixbuf",
			      get_pixbuf_for_flag (0), NULL); /* ughh, FIXME */		      
}

void
_modest_header_view_attach_cell_data (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
				      GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer user_data)
{
	TnyHeaderFlags flags;

	gtk_tree_model_get (tree_model, iter, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);

	if (flags & TNY_HEADER_FLAG_ATTACHMENTS)
		g_object_set (G_OBJECT (renderer), "pixbuf",
			      get_pixbuf_for_flag (TNY_HEADER_FLAG_ATTACHMENTS),
			      NULL);
}

void
_modest_header_view_header_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		  GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data)
{
	TnyHeaderFlags flags;
	
	gtk_tree_model_get (tree_model, iter, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);

	g_object_set (G_OBJECT(renderer),
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? 400: 800,
		      "strikethrough",  (flags & TNY_HEADER_FLAG_DELETED) ?  TRUE:FALSE,
		      NULL);	
}


void
_modest_header_view_date_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
				     GtkTreeModel *tree_model,  GtkTreeIter *iter,
				     gpointer user_data)
{
	TnyHeaderFlags flags;
	guint date, date_col;
	gchar *display_date;
	gboolean received = GPOINTER_TO_INT(user_data);

	if (received)
		date_col = TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN;
	else
		date_col = TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN;

	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    date_col, &date,
			    -1);

	display_date = modest_text_utils_get_display_date (date);
	g_object_set (G_OBJECT(renderer),
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? 400: 800,
		      "style",  (flags & TNY_HEADER_FLAG_DELETED) ?
		                 PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,
		      "text",    display_date,
		      NULL);
	g_free (display_date);
}


void
_modest_header_view_sender_receiver_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
			    GtkTreeModel *tree_model,  GtkTreeIter *iter,  gboolean is_sender)
{
	TnyHeaderFlags flags;
	gchar *address;
	gint sender_receiver_col;

	if (is_sender)
		sender_receiver_col = TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN;
	else
		sender_receiver_col = TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN;
		
	gtk_tree_model_get (tree_model, iter,
			    sender_receiver_col,  &address,
			    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    -1);
	
	g_object_set (G_OBJECT(renderer),
		      "text",
		      modest_text_utils_get_display_address (address),
		      "weight",
		      (flags & TNY_HEADER_FLAG_SEEN) ? 400 : 800,
		      "style",
		      (flags & TNY_HEADER_FLAG_DELETED)?PANGO_STYLE_ITALIC:PANGO_STYLE_NORMAL,
		      NULL);

	g_free (address);	
}
/*
 * this for both incoming and outgoing mail, depending on the the user_data
 * parameter. in the incoming case, show 'From' and 'Received date', in the
 * outgoing case, show 'To' and 'Sent date'
 */
void
_modest_header_view_compact_header_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
					       GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data)
{
	GObject *rendobj;
	TnyHeaderFlags flags;
	gchar *address, *subject, *header, *display_date;
	time_t date;
	gboolean is_incoming;

	is_incoming = GPOINTER_TO_INT(user_data); /* GPOINTER_TO_BOOLEAN is not available
						   * in older versions of glib...*/

	if (is_incoming)
		gtk_tree_model_get (tree_model, iter,
				    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
				    TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,  &address,
				    TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &subject,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN, &date,   
				    -1);
	else
		gtk_tree_model_get (tree_model, iter,
				    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
				    TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN,  &address,
				    TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &subject,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &date,   
				    -1);
	
	rendobj = G_OBJECT(renderer);
	display_date = modest_text_utils_get_display_date (date);
	header = g_strdup_printf ("%s %s\n%s",
				  modest_text_utils_get_display_address (address),
				  display_date, subject);
	g_free (address);
	g_free (subject);
	g_free (display_date);
	
	g_object_set (G_OBJECT(renderer),
		      "text",  header,
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? 400: 800,
		      "style",  (flags & TNY_HEADER_FLAG_DELETED) ?
		                 PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,
		      NULL);	
	g_free (header);
}


void
_modest_header_view_size_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
				     GtkTreeModel *tree_model,  GtkTreeIter *iter,
				     gpointer user_data)
{
        TnyHeaderFlags flags;
	guint size;
	gchar *size_str;
	
	gtk_tree_model_get (tree_model, iter,
			   TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			   TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN, &size,
			    -1);
	
	size_str = modest_text_utils_get_display_size (size);
	
	g_object_set (G_OBJECT(renderer),
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? 400: 800,
		      "style",  (flags & TNY_HEADER_FLAG_DELETED) ?
		      PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,
		      "text",    size_str,       
                      NULL);
       g_free (size_str);

 }

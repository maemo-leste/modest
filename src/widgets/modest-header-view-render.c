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

#include <tny-simple-list.h>
#include <modest-header-view.h>
#include <modest-header-view-priv.h>
#include <modest-icon-names.h>
#include <modest-text-utils.h>
#include <modest-tny-account-store.h>
#include <modest-tny-send-queue.h>
#include <modest-tny-folder.h>
#include <modest-tny-account.h>
#include <modest-runtime.h>
#include <glib/gi18n.h>
#include <modest-platform.h>
#include <string.h>

typedef enum {
	RENDER_CELL_STYLE_DEFAULT=0,
	RENDER_CELL_STYLE_GREY
} RenderCellStyle;

static const gchar *
get_status_string (ModestTnySendQueueStatus status)
{
	switch (status) {
	case MODEST_TNY_SEND_QUEUE_WAITING:
		return _("mcen_li_outbox_waiting");
		break;
	case MODEST_TNY_SEND_QUEUE_SENDING:
		return _("mcen_li_outbox_sending");
		break;
	case MODEST_TNY_SEND_QUEUE_SUSPENDED:
		return _("mcen_li_outbox_suspended");
		break;
	case MODEST_TNY_SEND_QUEUE_FAILED:
		return _("mcen_li_outbox_failed");
		break;
	default:
		return "";
		break;
	}
}

static GdkPixbuf*
get_pixbuf_for_flag (TnyHeaderFlags flag)
{
	/* optimization */
	static GdkPixbuf *deleted_pixbuf          = NULL;
	static GdkPixbuf *seen_pixbuf             = NULL;
	static GdkPixbuf *unread_pixbuf           = NULL;
	static GdkPixbuf *attachments_pixbuf      = NULL;
	static GdkPixbuf *high_pixbuf             = NULL;
	static GdkPixbuf *low_pixbuf             = NULL;
	
	switch (flag) {
	case TNY_HEADER_FLAG_DELETED:
		if (G_UNLIKELY(!deleted_pixbuf))
			deleted_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_DELETED,
								   MODEST_ICON_SIZE_SMALL);
		return deleted_pixbuf;
	case TNY_HEADER_FLAG_SEEN:
		if (G_UNLIKELY(!seen_pixbuf))
			seen_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_READ,
								MODEST_ICON_SIZE_SMALL);
		return seen_pixbuf;
	case TNY_HEADER_FLAG_ATTACHMENTS:
		if (G_UNLIKELY(!attachments_pixbuf))
			attachments_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_ATTACH,
								       MODEST_ICON_SIZE_SMALL);
		return attachments_pixbuf;
	case TNY_HEADER_FLAG_HIGH_PRIORITY:
		if (G_UNLIKELY(!high_pixbuf))
			high_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_HIGH,
								MODEST_ICON_SIZE_SMALL);
		return high_pixbuf;
	case TNY_HEADER_FLAG_LOW_PRIORITY:
		if (G_UNLIKELY(!low_pixbuf))
			low_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_LOW,
							       MODEST_ICON_SIZE_SMALL);
		return low_pixbuf;
	case TNY_HEADER_FLAG_NORMAL_PRIORITY:
		return NULL;
	default:
		if (G_UNLIKELY(!unread_pixbuf))
			unread_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_UNREAD,
								  MODEST_ICON_SIZE_SMALL);
		return unread_pixbuf;
	}
}

static void
set_common_flags (GtkCellRenderer *renderer, TnyHeaderFlags flags)
{
	g_object_set (G_OBJECT(renderer),
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? PANGO_WEIGHT_NORMAL: PANGO_WEIGHT_ULTRABOLD,
		      "strikethrough",  (flags & TNY_HEADER_FLAG_DELETED) ?  TRUE:FALSE,
		      NULL);	
}

static void
set_cell_text (GtkCellRenderer *renderer, 
	       const gchar *text, 
	       TnyHeaderFlags flags,
	       RenderCellStyle style)
{
	PangoWeight weight;
	gboolean strikethrough;

	weight =  (flags & TNY_HEADER_FLAG_SEEN) ? PANGO_WEIGHT_NORMAL: PANGO_WEIGHT_ULTRABOLD;
	strikethrough = (flags & TNY_HEADER_FLAG_DELETED) ?  TRUE:FALSE;
	g_object_freeze_notify (G_OBJECT (renderer));
	g_object_set (G_OBJECT (renderer), 
		      "text", text, 
		      "weight", (flags & TNY_HEADER_FLAG_SEEN) ? PANGO_WEIGHT_NORMAL : PANGO_WEIGHT_ULTRABOLD,
		      "strikethrough", (flags &TNY_HEADER_FLAG_DELETED) ? TRUE : FALSE,
		      NULL);
	switch (style) {
	case RENDER_CELL_STYLE_GREY:
		g_object_set (G_OBJECT (renderer), 
			      "foreground", "#666666",
			      "scale", PANGO_SCALE_SMALL,
			      NULL);
		break;
	case RENDER_CELL_STYLE_DEFAULT:
		break;
	}
	g_object_thaw_notify (G_OBJECT (renderer));
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
	set_common_flags (renderer, flags);
}


void
_modest_header_view_date_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
				     GtkTreeModel *tree_model,  GtkTreeIter *iter,
				     gpointer user_data)
{
	TnyHeaderFlags flags;
	guint date, date_col;
	gboolean received = GPOINTER_TO_INT(user_data);

	if (received)
		date_col = TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN;
	else
		date_col = TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN;

	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    date_col, &date,
			    -1);
	
	set_cell_text (renderer, modest_text_utils_get_display_date (date),
		       flags, RENDER_CELL_STYLE_DEFAULT);
}

void
_modest_header_view_sender_receiver_cell_data  (GtkTreeViewColumn *column,  
						GtkCellRenderer *renderer,
						GtkTreeModel *tree_model,  
						GtkTreeIter *iter,  
						gboolean is_sender)
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
	
	modest_text_utils_get_display_address (address); /* string is changed in-place */
	set_cell_text (renderer, address, flags, RENDER_CELL_STYLE_DEFAULT);
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
	/* Note that GtkTreeModel is a GtkTreeModelFilter. */
	
	/* printf ("DEBUG: %s: tree_model gtype=%s\n", __FUNCTION__, G_OBJECT_TYPE_NAME (tree_model)); */
	
	TnyHeaderFlags flags = 0;
	gchar *address = NULL;
	gchar *subject = NULL;
	time_t date;

#ifdef MAEMO_CHANGES
#ifdef HAVE_GTK_TREE_VIEW_COLUMN_GET_CELL_DATA_HINT
	GtkTreeCellDataHint hint;
#endif
#endif
	
	GtkCellRenderer *recipient_cell, *date_or_status_cell, *subject_cell,
		*attach_cell, *priority_cell,
		*recipient_box, *subject_box = NULL;
	TnyHeader *msg_header = NULL;
	TnyHeaderFlags prio = 0;


	g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (column));
	g_return_if_fail (GTK_IS_CELL_RENDERER (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));

#ifdef MAEMO_CHANGES
#ifdef HAVE_GTK_TREE_VIEW_COLUMN_GET_CELL_DATA_HINT
	hint = gtk_tree_view_column_get_cell_data_hint (GTK_TREE_VIEW_COLUMN (column));

	if (hint != GTK_TREE_CELL_DATA_HINT_ALL)
		return;
#endif
#endif
	
	recipient_box = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (renderer), "recpt-box-renderer"));
	subject_box = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (renderer), "subject-box-renderer"));
	priority_cell = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (subject_box), "priority-renderer"));
	subject_cell = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (subject_box), "subject-renderer"));
	attach_cell = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (recipient_box), "attach-renderer"));
	recipient_cell = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (recipient_box), "recipient-renderer"));
	date_or_status_cell = GTK_CELL_RENDERER (g_object_get_data (G_OBJECT (recipient_box), "date-renderer"));

	ModestHeaderViewCompactHeaderMode header_mode = GPOINTER_TO_INT (user_data); 

	if (header_mode == MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_IN)
		gtk_tree_model_get (tree_model, iter,
				    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
				    TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,  &address,
				    TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &subject,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN, &date,
				    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &msg_header,
				    -1);
	else
		gtk_tree_model_get (tree_model, iter,
				    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
				    TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN,  &address,
				    TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &subject,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &date,
				    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &msg_header,
				    -1);	
	/* flags */
	/* FIXME: we might gain something by doing all the g_object_set's at once */
	if (flags & TNY_HEADER_FLAG_ATTACHMENTS)
		g_object_set (G_OBJECT (attach_cell), "pixbuf",
			      get_pixbuf_for_flag (TNY_HEADER_FLAG_ATTACHMENTS),
			      NULL);
	else
		g_object_set (G_OBJECT (attach_cell), "pixbuf",
			      NULL, NULL);

	if (msg_header)
		prio = tny_header_get_priority (msg_header);
	g_object_set (G_OBJECT (priority_cell), "pixbuf",
		      get_pixbuf_for_flag (prio), 
		      NULL);

	set_cell_text (subject_cell, (subject && subject[0] != 0)?subject:_("mail_va_no_subject"), 
		       flags, RENDER_CELL_STYLE_DEFAULT);
	g_free (subject);

	/* FIXME: we hardcode the color to #666666; instead we should use SecondaryTextColour from the
	 * theme (gtkrc file) */
	modest_text_utils_get_display_address (address); /* changed in-place */
	set_cell_text (recipient_cell, address, flags, RENDER_CELL_STYLE_GREY);
	
	if (header_mode == MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_OUTBOX) {
		ModestTnySendQueueStatus status = MODEST_TNY_SEND_QUEUE_UNKNOWN;
		const gchar *status_str = "";
		if (msg_header != NULL) {
			status = modest_tny_all_send_queues_get_msg_status (msg_header);
			if (status == MODEST_TNY_SEND_QUEUE_SUSPENDED) {
				tny_header_set_flag (msg_header, TNY_HEADER_FLAG_SUSPENDED);
			}
		}
		
		status_str = get_status_string (status);
		set_cell_text (date_or_status_cell, status_str, flags, RENDER_CELL_STYLE_GREY);
	} else {		
		set_cell_text (date_or_status_cell, date ? modest_text_utils_get_display_date (date) : "",
			       flags, RENDER_CELL_STYLE_GREY);
	}
	
	if (msg_header != NULL)
		g_object_unref (msg_header);
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
	
	set_cell_text (renderer, size_str, flags, RENDER_CELL_STYLE_DEFAULT);

	g_free (size_str);
 }

void
_modest_header_view_status_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
				       GtkTreeModel *tree_model,  GtkTreeIter *iter,
				       gpointer user_data)
{
        TnyHeaderFlags flags;
	//guint status;
	gchar *status_str;
	
	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    -1);

       if (flags & TNY_HEADER_FLAG_SUSPENDED)
	       status_str = g_strdup(_("mcen_li_outbox_suspended"));
       else	       
	       status_str = g_strdup(_("mcen_li_outbox_waiting"));
       
	set_cell_text (renderer, status_str, flags, RENDER_CELL_STYLE_DEFAULT);

	g_free (status_str);
 }


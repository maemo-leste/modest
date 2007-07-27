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
#include <modest-tny-send-queue.h>
#include <modest-tny-folder.h>
#include <modest-tny-account.h>
#include <modest-runtime.h>
#include <glib/gi18n.h>
#include <modest-platform.h>
#include <string.h>


void 
fill_list_of_caches (gpointer key, gpointer value, gpointer userdata)
{
	GSList **send_queues = (GSList **) userdata;
	*send_queues = g_slist_prepend (*send_queues, value);
}

static ModestTnySendQueueStatus
get_status_of_uid (TnyHeader *header)
{
	ModestCacheMgr *cache_mgr;
	GHashTable     *send_queue_cache;
	GSList *send_queues = NULL, *node;
	/* get_msg_status returns suspended by default, so we want to detect changes */
	ModestTnySendQueueStatus status = MODEST_TNY_SEND_QUEUE_SUSPENDED;
	ModestTnySendQueueStatus queue_status = MODEST_TNY_SEND_QUEUE_SUSPENDED;
	gchar *msg_uid = NULL;
	
	msg_uid = modest_tny_send_queue_get_msg_id (header);
	cache_mgr = modest_runtime_get_cache_mgr ();
	send_queue_cache = modest_cache_mgr_get_cache (cache_mgr,
						       MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);
	
	g_hash_table_foreach (send_queue_cache, (GHFunc) fill_list_of_caches, &send_queues);
	
	for (node = send_queues; node != NULL; node = g_slist_next (node)) {
		ModestTnySendQueue *send_queue = MODEST_TNY_SEND_QUEUE (node->data);

		queue_status = modest_tny_send_queue_get_msg_status (send_queue, msg_uid);
		if (queue_status != MODEST_TNY_SEND_QUEUE_UNKNONW) {
			status = queue_status;
			break;
		}
	}

	g_free(msg_uid);
	g_slist_free (send_queues);
	return status;
}

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
			deleted_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_DELETED);
		return deleted_pixbuf;
	case TNY_HEADER_FLAG_SEEN:
		if (G_UNLIKELY(!seen_pixbuf))
			seen_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_READ);
		return seen_pixbuf;
	case TNY_HEADER_FLAG_ATTACHMENTS:
		if (G_UNLIKELY(!attachments_pixbuf))
			attachments_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_ATTACH);
		return attachments_pixbuf;
	case TNY_HEADER_FLAG_HIGH_PRIORITY:
		if (G_UNLIKELY(!high_pixbuf))
			high_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_HIGH);
		return high_pixbuf;
	case TNY_HEADER_FLAG_LOW_PRIORITY:
		if (G_UNLIKELY(!low_pixbuf))
			low_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_LOW);
		return low_pixbuf;
	default:
		if (G_UNLIKELY(!unread_pixbuf))
			unread_pixbuf = modest_platform_get_icon (MODEST_HEADER_ICON_UNREAD);
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
	gchar *display_date = NULL;
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
	g_object_set (G_OBJECT(renderer), "text", display_date, NULL);	

	set_common_flags (renderer, flags);
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
		      NULL);
	g_free (address);
	set_common_flags (renderer, flags);
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
	g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (column));
	g_return_if_fail (GTK_IS_CELL_RENDERER (renderer));
	g_return_if_fail (GTK_IS_TREE_MODEL (tree_model));
	
	/* Note that GtkTreeModel is a GtkTreeModelFilter. */
	
	/* printf ("DEBUG: %s: tree_model gtype=%s\n", __FUNCTION__, G_OBJECT_TYPE_NAME (tree_model)); */
	
	TnyHeaderFlags flags = 0;
	TnyHeaderFlags prior_flags = 0;
	gchar *address = NULL;
	gchar *subject = NULL;
	gchar *header = NULL;
	time_t date = 0;
	
	GtkCellRenderer *recipient_cell, *date_or_status_cell, *subject_cell,
		*attach_cell, *priority_cell,
		*recipient_box, *subject_box = NULL;
	TnyHeader *msg_header = NULL;
	gchar *display_date = NULL, *tmp_date = NULL;

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
	prior_flags = flags & TNY_HEADER_FLAG_PRIORITY;
	if (flags & TNY_HEADER_FLAG_ATTACHMENTS)
		g_object_set (G_OBJECT (attach_cell), "pixbuf",
			      get_pixbuf_for_flag (TNY_HEADER_FLAG_ATTACHMENTS),
			      NULL);
	else
		g_object_set (G_OBJECT (attach_cell), "pixbuf",
			      NULL, NULL);
	if (flags & TNY_HEADER_FLAG_PRIORITY)
		g_object_set (G_OBJECT (priority_cell), "pixbuf",
			      get_pixbuf_for_flag (prior_flags),
/* 			      get_pixbuf_for_flag (flags & TNY_HEADER_FLAG_PRIORITY), */
			      NULL);
	else
		g_object_set (G_OBJECT (priority_cell), "pixbuf",
			      NULL, NULL);
	header = g_markup_printf_escaped ("%s", (subject && strlen (subject)) ? subject : _("mail_va_no_subject"));
	g_free (subject);
	g_object_set (G_OBJECT (subject_cell), "markup", header, NULL);
	g_free (header);
	set_common_flags (subject_cell, flags);


	/* fixme: we hardcode the color to #666666; instead we should use SecundaryTextColour from the
	 * theme (gtkrc file) */
	
	header = g_markup_printf_escaped ("<span size='small' foreground='#666666'>%s</span>", modest_text_utils_get_display_address (address));
	g_free (address);
	g_object_set (G_OBJECT (recipient_cell),
		      "markup", header,
		      NULL);
	g_free (header);
	set_common_flags (recipient_cell, flags);

	if (header_mode == MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_OUTBOX) {
		ModestTnySendQueueStatus status = MODEST_TNY_SEND_QUEUE_WAITING;
		const gchar *status_str = "";
		if (msg_header != NULL) {
			status = get_status_of_uid (msg_header);
			if (status == MODEST_TNY_SEND_QUEUE_SUSPENDED) {
				tny_header_unset_flags (msg_header, TNY_HEADER_FLAG_PRIORITY);
				tny_header_set_flags (msg_header, TNY_HEADER_FLAG_SUSPENDED_PRIORITY);
			}
/* 			if (prior_flags == TNY_HEADER_FLAG_SUSPENDED_PRIORITY) */
/* 				status = MODEST_TNY_SEND_QUEUE_SUSPENDED; */
		}
		status_str = get_status_string (status);
		display_date = g_strdup_printf("<span size='small' foreground='#666666'>%s</span>", status_str);
		g_object_set (G_OBJECT (date_or_status_cell),
			      "markup", display_date,
			      NULL);
		g_free (display_date);
	} else {
		/* in some rare cases, mail might have no Date: field. it case,
		 * don't show the date, instead of bogus 1/1/1970
		 */
		if (date)
			tmp_date = modest_text_utils_get_display_date (date);
		else
			tmp_date = g_strdup ("");
		
		display_date = g_strdup_printf ("<span size='small' foreground='#666666'>%s</span>", tmp_date);
		g_object_set (G_OBJECT (date_or_status_cell),
			      "markup", display_date,
			      NULL);
		g_free (tmp_date);
		g_free (display_date);
	}
	if (msg_header != NULL)
		g_object_unref (msg_header);
	set_common_flags (date_or_status_cell, flags);
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
	
	g_object_set (G_OBJECT(renderer), "text", size_str, NULL);
	set_common_flags (renderer, flags);

	g_free (size_str);
 }

void
_modest_header_view_status_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
				       GtkTreeModel *tree_model,  GtkTreeIter *iter,
				       gpointer user_data)
{
        TnyHeaderFlags flags, prior_flags;
	//guint status;
	gchar *status_str;
	
	gtk_tree_model_get (tree_model, iter,
			    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    -1);
	
       prior_flags = flags & TNY_HEADER_FLAG_PRIORITY;
       if (prior_flags == TNY_HEADER_FLAG_SUSPENDED_PRIORITY)
	       status_str = g_strdup(_("mcen_li_outbox_suspended"));
       else	       
	       status_str = g_strdup(_("mcen_li_outbox_waiting"));
       
	g_object_set (G_OBJECT(renderer), "text", status_str, NULL);
	set_common_flags (renderer, flags);

	g_free (status_str);
 }


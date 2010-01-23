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


#include <modest-tny-send-queue.h>
#include <tny-simple-list.h>
#include <tny-iterator.h>
#include <tny-folder.h>
#include <tny-error.h>
#include <tny-camel-msg.h>
#include <tny-folder-change.h>
#include <tny-folder-observer.h>
#include <modest-tny-account.h>
#include <modest-runtime.h>
#include <modest-platform.h>
#include <widgets/modest-window-mgr.h>
#include <modest-marshal.h>
#include <modest-debug.h>
#include <string.h> /* strcmp */

/* 'private'/'protected' functions */
static void modest_tny_send_queue_class_init (ModestTnySendQueueClass *klass);
static void modest_tny_send_queue_finalize   (GObject *obj);
static void modest_tny_send_queue_instance_init (GTypeInstance *instance, gpointer g_class);

/* Signal handlers */ 
static void _on_msg_start_sending (TnySendQueue *self, 
				   TnyHeader *header, 
				   TnyMsg *msg, 
				   int done, 
				   int total, 
				   gpointer user_data);

static void _on_msg_has_been_sent (TnySendQueue *self, 
				   TnyHeader *header, 
				   TnyMsg *msg, 
				   int done, 
				   int total, 
				   gpointer user_data);

static void _on_msg_error_happened (TnySendQueue *self, 
				    TnyHeader *header, 
				    TnyMsg *msg, 
				    GError *err, 
				    gpointer user_data);

static void _on_queue_start        (TnySendQueue *self, 
				    gpointer user_data);

static void _on_queue_stop         (TnySendQueue *self,
				    gpointer data);

static void modest_tny_send_queue_add_async (TnySendQueue *self, 
					     TnyMsg *msg, 
					     TnySendQueueAddCallback callback, 
					     TnyStatusCallback status_callback, 
					     gpointer user_data);

static TnyFolder* modest_tny_send_queue_get_outbox  (TnySendQueue *self);
static TnyFolder* modest_tny_send_queue_get_sentbox (TnySendQueue *self);
static void modest_tny_send_queue_cancel (TnySendQueue *self,
					  TnySendQueueCancelAction cancel_action,
					  GError **err);

/* list my signals  */
enum {
	STATUS_CHANGED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _SendInfo SendInfo;
struct _SendInfo {
	gchar* msg_id;
	ModestTnySendQueueStatus status;
};

typedef struct _ModestTnySendQueuePrivate ModestTnySendQueuePrivate;
struct _ModestTnySendQueuePrivate {
	/* Queued infos */
	GQueue* queue;

	/* The info that is currently being sent */
	GList* current;

	/* Special folders */
	TnyFolder *outbox;
	TnyFolder *sentbox;

	/* last was send receive operation?*/
	gboolean requested_send_receive;
	gboolean sending;
	gboolean suspend;

	GSList *sighandlers;
};

#define MODEST_TNY_SEND_QUEUE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_TNY_SEND_QUEUE, \
                                                   ModestTnySendQueuePrivate))

/* globals */
static TnyCamelSendQueueClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

/*
 * this thread actually tries to send all the mails in the outbox and keeps
 * track of their state.
 */

static int
on_modest_tny_send_queue_compare_id (gconstpointer info, gconstpointer msg_id)
{
	g_return_val_if_fail (info && ((SendInfo*)info)->msg_id && msg_id, -1);
	
	return strcmp( ((SendInfo*)info)->msg_id, msg_id);
}

static void
modest_tny_send_queue_info_free (SendInfo *info)
{
	g_free(info->msg_id);
	g_slice_free(SendInfo, info);
}

static GList*
modest_tny_send_queue_lookup_info (ModestTnySendQueue *self, const gchar *msg_id)
{
	ModestTnySendQueuePrivate *priv;
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	return g_queue_find_custom (priv->queue, msg_id, on_modest_tny_send_queue_compare_id);
}


static void
queue_item_to_string (gpointer data, gchar **user_data)
{
	SendInfo *info = (SendInfo*)data;
	const gchar *status;
	gchar *tmp;
	
	if (!(user_data && *user_data))
		return;
	
	switch (info->status) {
	case MODEST_TNY_SEND_QUEUE_UNKNOWN: status = "UNKNOWN"; break;
	case MODEST_TNY_SEND_QUEUE_WAITING: status = "WAITING"; break;
	case MODEST_TNY_SEND_QUEUE_SUSPENDED: status = "SUSPENDED"; break;
	case MODEST_TNY_SEND_QUEUE_SENDING: status = "SENDING"; break;
	case MODEST_TNY_SEND_QUEUE_FAILED: status = "FAILED"; break;
	default: status= "UNEXPECTED"; break;
	}

	tmp = g_strdup_printf ("%s\"%s\" => [%s]\n",
			       *user_data, info->msg_id, status);
	g_free (*user_data);
	*user_data = tmp;
}

gchar*
modest_tny_send_queue_to_string (ModestTnySendQueue *self)
{
	gchar *str;
	ModestTnySendQueuePrivate *priv;

	g_return_val_if_fail (MODEST_IS_TNY_SEND_QUEUE(self), NULL);
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	str = g_strdup_printf ("items in the send queue: %d\n",
			       g_queue_get_length (priv->queue));
	
	g_queue_foreach (priv->queue, (GFunc)queue_item_to_string, &str);

	return str;
}

typedef struct {
	TnySendQueueAddCallback callback;
	gpointer user_data;
} AddAsyncHelper;

static void
_on_added_to_outbox (TnySendQueue *self, 
		     gboolean cancelled, 
		     TnyMsg *msg, 
		     GError *err,
		     gpointer user_data) 
{
	ModestTnySendQueuePrivate *priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE(self);
	TnyHeader *header = NULL;
	SendInfo *info = NULL;
	GList* existing = NULL;
	gchar* msg_id = NULL;
	AddAsyncHelper *helper;

	g_return_if_fail (TNY_IS_SEND_QUEUE(self));
	g_return_if_fail (TNY_IS_CAMEL_MSG(msg));

	header = tny_msg_get_header (msg);
	msg_id = modest_tny_send_queue_get_msg_id (header);
	if (!msg_id) {
		g_warning ("%s: No msg_id returned for header", __FUNCTION__);
		goto end;
	}

	/* Put newly added message in WAITING state */
	existing = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE(self), msg_id);
	if(existing != NULL) {
		info = existing->data;
		info->status = MODEST_TNY_SEND_QUEUE_WAITING;
	} else {
		info = g_slice_new (SendInfo);
		info->msg_id = msg_id;
		info->status = MODEST_TNY_SEND_QUEUE_WAITING;
		g_queue_push_tail (priv->queue, info);
	}

	g_signal_emit (self, signals[STATUS_CHANGED_SIGNAL], 0, info->msg_id, info->status);

 end:
	g_object_unref (G_OBJECT(header));

	/* Call the user callback */
	helper = (AddAsyncHelper *) user_data;
	if (helper->callback)
		helper->callback (self, cancelled, msg, err, helper->user_data);
	g_slice_free (AddAsyncHelper, helper);
}

static void
_add_message (ModestTnySendQueue *self, TnyHeader *header)
{
	ModestWindowMgr *mgr = NULL;
	ModestTnySendQueuePrivate *priv;
	SendInfo *info = NULL;
	GList* existing = NULL;
	gchar* msg_uid = NULL;
	ModestTnySendQueueStatus status = MODEST_TNY_SEND_QUEUE_UNKNOWN;
	gboolean editing = FALSE;

	g_return_if_fail (TNY_IS_SEND_QUEUE(self));
	g_return_if_fail (TNY_IS_HEADER(header));
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	/* Check whether the mail is already in the queue */
	msg_uid = modest_tny_send_queue_get_msg_id (header);
	status = modest_tny_send_queue_get_msg_status (self, msg_uid);
	switch (status) {
	case MODEST_TNY_SEND_QUEUE_UNKNOWN:
	case MODEST_TNY_SEND_QUEUE_SUSPENDED:
	case MODEST_TNY_SEND_QUEUE_FAILED:

		/* Check if it already exists on queue */
		existing = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE(self), msg_uid);
		if(existing != NULL)
			break;
		
		/* Check if its being edited */
		mgr = modest_runtime_get_window_mgr ();
		editing = modest_window_mgr_find_registered_header (mgr, header, NULL);
		if (editing)
			break;
		
		/* Add new meesage info */
		info = g_slice_new0 (SendInfo);
		info->msg_id = strdup(msg_uid);
		info->status = MODEST_TNY_SEND_QUEUE_WAITING;
		g_queue_push_tail (priv->queue, info);
		break;
	default:
		break;
	}

	/* Free */
	g_free(msg_uid);
}

static void 
modest_tny_send_queue_add_async (TnySendQueue *self, 
				 TnyMsg *msg, 
				 TnySendQueueAddCallback callback, 
				 TnyStatusCallback status_callback, 
				 gpointer user_data)
{
	AddAsyncHelper *helper = g_slice_new0 (AddAsyncHelper);
	helper->callback = callback;
	helper->user_data = user_data;

	/* Call the superclass passing our own callback */
	TNY_CAMEL_SEND_QUEUE_CLASS(parent_class)->add_async (self, msg, 
							     _on_added_to_outbox, 
							     status_callback, 
							     helper);
}


static TnyFolder*
modest_tny_send_queue_get_sentbox (TnySendQueue *self)
{
	ModestTnySendQueuePrivate *priv;

	g_return_val_if_fail (self, NULL);

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	return g_object_ref (priv->sentbox);
}


static TnyFolder*
modest_tny_send_queue_get_outbox (TnySendQueue *self)
{
	ModestTnySendQueuePrivate *priv;

	g_return_val_if_fail (self, NULL);

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	return g_object_ref (priv->outbox);
}

static void
modest_tny_send_queue_cancel (TnySendQueue *self,
			      TnySendQueueCancelAction cancel_action,
			      GError **err)
{
	ModestTnySendQueuePrivate *priv;

	g_return_if_fail (self);

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	/* Call the parent */
	TNY_CAMEL_SEND_QUEUE_CLASS(parent_class)->cancel (self, cancel_action, err);

	if (cancel_action == TNY_SEND_QUEUE_CANCEL_ACTION_SUSPEND && (err == NULL || *err == NULL))
		priv->suspend = TRUE;
}

GType
modest_tny_send_queue_get_type (void)
{
	static GType my_type = 0;

	if (my_type == 0) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnySendQueueClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_send_queue_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnySendQueue),
			0,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_send_queue_instance_init,
			NULL
		};
		
		my_type = g_type_register_static (TNY_TYPE_CAMEL_SEND_QUEUE,
						  "ModestTnySendQueue",
						  &my_info, 0);
	}
	return my_type;
}


static void
modest_tny_send_queue_class_init (ModestTnySendQueueClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	
	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_send_queue_finalize;

	TNY_CAMEL_SEND_QUEUE_CLASS(klass)->add_async   = modest_tny_send_queue_add_async;
	TNY_CAMEL_SEND_QUEUE_CLASS(klass)->get_outbox  = modest_tny_send_queue_get_outbox;
        TNY_CAMEL_SEND_QUEUE_CLASS(klass)->get_sentbox = modest_tny_send_queue_get_sentbox;
        TNY_CAMEL_SEND_QUEUE_CLASS(klass)->cancel      = modest_tny_send_queue_cancel;
	klass->status_changed   = NULL;

	signals[STATUS_CHANGED_SIGNAL] =
		g_signal_new ("status_changed",
		              G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestTnySendQueueClass, status_changed),
			      NULL, NULL,
			      modest_marshal_VOID__STRING_INT,
			      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT);

	g_type_class_add_private (gobject_class, sizeof(ModestTnySendQueuePrivate));
}

static void
modest_tny_send_queue_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestTnySendQueuePrivate *priv;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (instance);
	priv->queue = g_queue_new();
	priv->current = NULL;
	priv->outbox = NULL;
	priv->sentbox = NULL;
	priv->sending = FALSE;
	priv->suspend = FALSE;
	priv->sighandlers = NULL;
}

static void
modest_tny_send_queue_finalize (GObject *obj)
{
	ModestTnySendQueuePrivate *priv;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (obj);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;

	g_queue_foreach (priv->queue, (GFunc)modest_tny_send_queue_info_free, NULL);
	g_queue_free (priv->queue);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
	g_object_unref (priv->outbox);
	g_object_unref (priv->sentbox);
}

typedef struct {
	TnyCamelTransportAccount *account;
	ModestTnySendQueue *queue;
} GetHeadersInfo;

static void
new_queue_get_headers_async_cb (TnyFolder *folder, 
				gboolean cancelled, 
				TnyList *headers, 
				GError *err, 
				gpointer user_data)
{
	ModestTnySendQueue *self;
	TnyIterator *iter;
	GetHeadersInfo *info;
	ModestMailOperation *wakeup_op;

	info = (GetHeadersInfo *) user_data;
	self = MODEST_TNY_SEND_QUEUE (info->queue);

	/* In case of error set the transport account anyway */
	if (cancelled || err)
		goto set_transport;

	/* Add messages to our internal queue */
	iter = tny_list_create_iterator (headers);
	while (!tny_iterator_is_done (iter)) {
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
		_add_message (self, header);
		g_object_unref (header);	
		tny_iterator_next (iter);
	}

	/* Reenable suspended items */
	wakeup_op = modest_mail_operation_new (NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 wakeup_op);
	modest_mail_operation_queue_wakeup (wakeup_op, MODEST_TNY_SEND_QUEUE (self));

	/* Frees */
	g_object_unref (iter);
	g_object_unref (headers);

 set_transport:
	/* Do this at the end, because it'll call tny_send_queue_flush
	   which will call tny_send_queue_get_outbox and
	   tny_send_queue_get_sentbox */
	tny_camel_send_queue_set_transport_account (TNY_CAMEL_SEND_QUEUE(self),
						    info->account);

	/* Frees */
	g_object_unref (info->account); 
	g_object_unref (info->queue); 
	g_slice_free (GetHeadersInfo, info);
}

ModestTnySendQueue*
modest_tny_send_queue_new (TnyCamelTransportAccount *account)
{
	ModestTnySendQueue *self = NULL;
	ModestTnySendQueuePrivate *priv = NULL;
	TnyList *headers = NULL;
	GetHeadersInfo *info;

	g_return_val_if_fail (TNY_IS_CAMEL_TRANSPORT_ACCOUNT(account), NULL);

	self = MODEST_TNY_SEND_QUEUE(g_object_new(MODEST_TYPE_TNY_SEND_QUEUE, NULL));

	/* Set outbox and sentbox */
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	priv->outbox  = modest_tny_account_get_special_folder (TNY_ACCOUNT(account),
							       TNY_FOLDER_TYPE_OUTBOX);
	priv->sentbox = modest_tny_account_get_special_folder (TNY_ACCOUNT(account),
							       TNY_FOLDER_TYPE_SENT);

	/* NOTE that this could happen if there was not enough disk
	   space when the account was created */
	if (!priv->outbox || !priv->sentbox) {
		g_object_unref (self);
		return NULL;
	}

	/* Connect signals to control when a msg is being or has been sent */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self),
						       "msg-sending",
						       G_CALLBACK(_on_msg_start_sending),
						       NULL);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self), "msg-sent",
						       G_CALLBACK(_on_msg_has_been_sent),
						       NULL);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(self), "error-happened",
						       G_CALLBACK(_on_msg_error_happened),
						       NULL);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (self), "queue-start",
						       G_CALLBACK (_on_queue_start),
						       NULL);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (self), "queue-stop",
						       G_CALLBACK (_on_queue_stop),
						       NULL);
	priv->requested_send_receive = FALSE;

	headers = tny_simple_list_new ();
	info = g_slice_new0 (GetHeadersInfo);
	info->account = g_object_ref (account);
	info->queue = g_object_ref (self);
	tny_folder_get_headers_async (priv->outbox, headers, TRUE, 
				      new_queue_get_headers_async_cb, 
				      NULL, info);

	return self;
}

gboolean
modest_tny_send_queue_msg_is_being_sent (ModestTnySendQueue* self,
					 const gchar *msg_id)
{	
	ModestTnySendQueueStatus status;
	
	g_return_val_if_fail (msg_id != NULL, FALSE); 
	
	status = modest_tny_send_queue_get_msg_status (self, msg_id);
	return status == MODEST_TNY_SEND_QUEUE_SENDING;
}

gboolean
modest_tny_send_queue_sending_in_progress (ModestTnySendQueue* self)
{	
	ModestTnySendQueuePrivate *priv;

	g_return_val_if_fail (MODEST_IS_TNY_SEND_QUEUE(self), FALSE);
	
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	return priv->sending;
}

ModestTnySendQueueStatus
modest_tny_send_queue_get_msg_status (ModestTnySendQueue *self, const gchar *msg_id)
{
	GList *item;

	g_return_val_if_fail (MODEST_IS_TNY_SEND_QUEUE(self), MODEST_TNY_SEND_QUEUE_UNKNOWN);
	g_return_val_if_fail (msg_id, MODEST_TNY_SEND_QUEUE_UNKNOWN);

	item = modest_tny_send_queue_lookup_info (self, msg_id);
	if (!item)
		return MODEST_TNY_SEND_QUEUE_UNKNOWN;
	else
		return ((SendInfo*)item->data)->status;
}

gchar *
modest_tny_send_queue_get_msg_id (TnyHeader *header)
{
	gchar* msg_uid = NULL;
	gchar *subject;
	time_t date_received;
		
	g_return_val_if_fail (header && TNY_IS_HEADER(header), NULL);

	/* Get message uid */
	subject = tny_header_dup_subject (header);
	date_received = tny_header_get_date_received (header);

	msg_uid = g_strdup_printf ("%s %d", subject, (int) date_received);
	g_free (subject);

	return msg_uid;
}


static void
_on_msg_start_sending (TnySendQueue *self, TnyHeader *header,
		       TnyMsg *msg, int done, int total, gpointer user_data)
{
	ModestTnySendQueuePrivate *priv = NULL;
	GList *item = NULL;
	SendInfo *info = NULL;
	gchar *msg_id = NULL;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	/* Get message uid */
	msg_id = modest_tny_send_queue_get_msg_id (header);
	if (msg_id) 
		item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), msg_id);
	else
		g_warning ("%s: could not get msg-id for header", __FUNCTION__);
	
	if (item) {
		/* Set current status item */
		info = item->data;
		info->status = MODEST_TNY_SEND_QUEUE_SENDING;
		g_signal_emit (self, signals[STATUS_CHANGED_SIGNAL], 0, info->msg_id, info->status);
		priv->current = item;
	} else
		g_warning ("%s: could not find item with id '%s'", __FUNCTION__, msg_id);
	
	/* free */
	g_free (msg_id);
}

static void 
_on_msg_has_been_sent (TnySendQueue *self,
		       TnyHeader *header,
		       TnyMsg *msg, 
		       int done, 
		       int total,
		       gpointer user_data)
{
	ModestTnySendQueuePrivate *priv;
	gchar *msg_id = NULL;
	GList *item;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	/* Get message uid */
	msg_id = modest_tny_send_queue_get_msg_id (header);

	tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);

	modest_platform_emit_msg_read_changed_signal (msg_id, TRUE);

	tny_folder_sync_async (priv->sentbox, FALSE, NULL, NULL, NULL);

	/* Get status info */
	item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), msg_id);


	/* TODO: note that item=NULL must not happen, but I found that
	   tinymail is issuing the message-sent signal twice, because
	   tny_camel_send_queue_update is called twice for each
	   message sent. This must be fixed in tinymail. Sergio */
	if (item) {
		/* Remove status info */
		modest_tny_send_queue_info_free (item->data);
		g_queue_delete_link (priv->queue, item);
		priv->current = NULL;
		
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_message_sent"));
	}

	/* free */
	g_free(msg_id);
}

static void 
_on_msg_error_happened (TnySendQueue *self,
			TnyHeader *header,
			TnyMsg *msg,
			GError *err,
			gpointer user_data)
{
	ModestTnySendQueuePrivate *priv = NULL;
	
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	/* Note that header could be NULL. Tinymail notifies about
	   generic send queue errors with this signal as well, and
	   those notifications are not bound to any particular header
	   or message */
	if (header && TNY_IS_HEADER (header)) {
		SendInfo *info = NULL;
		GList *item = NULL;
		gchar* msg_uid = NULL;

		/* Get sending info (create new if it doesn not exist) */
		msg_uid = modest_tny_send_queue_get_msg_id (header);
		item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), 
							  msg_uid);

		/* TODO: this should not happen (but it does), so the
		   problem should be located in the way we generate
		   the message uids */
		if (!item) {
			g_warning ("%s: could not find item with id '%s'", __FUNCTION__, msg_uid);
			g_free(msg_uid);
			return;
		}

		info = item->data;

		/* Keep in queue so that we remember that the opertion has failed */
		/* and was not just cancelled */
		if (err->code == TNY_SYSTEM_ERROR_CANCEL) {
			info->status = MODEST_TNY_SEND_QUEUE_SUSPENDED;
		} else {
			info->status = MODEST_TNY_SEND_QUEUE_FAILED;
		}
		priv->current = NULL;

		/* Notify status has changed */
		g_signal_emit (self, signals[STATUS_CHANGED_SIGNAL], 0, info->msg_id, info->status);

		/* free */
		g_free(msg_uid);
	}
}

static void 
_on_queue_start (TnySendQueue *self,
		 gpointer data)
{
	ModestTnySendQueuePrivate *priv;
	ModestMailOperation *mail_op;

	mail_op = modest_mail_operation_new (NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);
	modest_mail_operation_run_queue (mail_op, MODEST_TNY_SEND_QUEUE (self));
	g_object_unref (mail_op);

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	priv->sending = TRUE;
}

static void
on_queue_stop_get_headers_async_cb (TnyFolder *folder,
				    gboolean cancelled,
				    TnyList *headers,
				    GError *err,
				    gpointer user_data)
{
	ModestTnySendQueue *self = (ModestTnySendQueue *) user_data;
	TnyIterator *iter;

	if (cancelled || err)
		goto end;

	/* Update the info about headers */
	iter = tny_list_create_iterator (headers);
	while (!tny_iterator_is_done (iter)) {
		TnyHeader *header;

		header = (TnyHeader *) tny_iterator_get_current (iter);
		if (header) {
			gchar *msg_id = NULL;
			GList *item = NULL;

			/* Get message uid */
			msg_id = modest_tny_send_queue_get_msg_id (header);
			if (msg_id)
				item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), msg_id);
			else
				g_warning ("%s: could not get msg-id for header", __FUNCTION__);

			if (item) {
				SendInfo *info;
				/* Set current status item */
				info = item->data;
				if (tny_header_get_flags (header) & TNY_HEADER_FLAG_SUSPENDED) {
					info->status = MODEST_TNY_SEND_QUEUE_SUSPENDED;
					g_signal_emit (self, signals[STATUS_CHANGED_SIGNAL], 0,
						       info->msg_id, info->status);
				}
			} else {
				g_warning ("%s: could not find item with id '%s'", __FUNCTION__, msg_id);
			}
			g_object_unref (header);
		}
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

 end:
	/* Unrefs */
	g_object_unref (headers);
	g_object_unref (self);
}

static void
_on_queue_stop (TnySendQueue *self,
		gpointer data)
{
	ModestTnySendQueuePrivate *priv;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	priv->sending = FALSE;

	if (priv->suspend) {
		TnyList *headers;
		priv->suspend = FALSE;

		/* Update the state of messages in the queue */
		headers = tny_simple_list_new ();
		tny_folder_get_headers_async (priv->outbox, headers, TRUE,
					      on_queue_stop_get_headers_async_cb,
					      NULL, g_object_ref (self));
	}
}

static void
fill_list_of_caches (gpointer key, gpointer value, gpointer userdata)
{
	GSList **send_queues = (GSList **) userdata;
	*send_queues = g_slist_prepend (*send_queues, value);
}

/* This function shouldn't be here. Move it to another place. Sergio */
ModestTnySendQueueStatus
modest_tny_all_send_queues_get_msg_status (TnyHeader *header)
{
	ModestCacheMgr *cache_mgr = NULL;
	GHashTable     *send_queue_cache = NULL;
	ModestTnyAccountStore *accounts_store = NULL;
	TnyList *accounts = NULL;
	TnyIterator *iter = NULL;
	TnyTransportAccount *account = NULL;
	GSList *send_queues = NULL, *node;
	/* get_msg_status returns suspended by default, so we want to detect changes */
	ModestTnySendQueueStatus status = MODEST_TNY_SEND_QUEUE_UNKNOWN;
	ModestTnySendQueueStatus queue_status = MODEST_TNY_SEND_QUEUE_UNKNOWN;
	gchar *msg_uid = NULL;
	ModestTnySendQueue *send_queue = NULL;

	g_return_val_if_fail (TNY_IS_HEADER(header), MODEST_TNY_SEND_QUEUE_UNKNOWN);

	msg_uid = modest_tny_send_queue_get_msg_id (header);
	cache_mgr = modest_runtime_get_cache_mgr ();
	send_queue_cache = modest_cache_mgr_get_cache (cache_mgr,
						       MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);

	g_hash_table_foreach (send_queue_cache, (GHFunc) fill_list_of_caches, &send_queues);
	if (send_queues == NULL) {
		accounts = tny_simple_list_new (); 
		accounts_store = modest_runtime_get_account_store ();
		tny_account_store_get_accounts (TNY_ACCOUNT_STORE(accounts_store), 
						accounts, 
						TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);

		iter = tny_list_create_iterator (accounts);
		while (!tny_iterator_is_done (iter)) {
			account = TNY_TRANSPORT_ACCOUNT(tny_iterator_get_current (iter));
			send_queue = modest_runtime_get_send_queue(TNY_TRANSPORT_ACCOUNT(account), TRUE);
			g_object_unref(account);
			if (TNY_IS_SEND_QUEUE (send_queue)) {
				queue_status = modest_tny_send_queue_get_msg_status (send_queue, msg_uid);
				if (queue_status != MODEST_TNY_SEND_QUEUE_UNKNOWN) {
					status = queue_status;
					break;
				}
			}
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (accounts);
	}
	else {
		for (node = send_queues; node != NULL; node = g_slist_next (node)) {
			send_queue = MODEST_TNY_SEND_QUEUE (node->data);

			queue_status = modest_tny_send_queue_get_msg_status (send_queue, msg_uid);
			if (queue_status != MODEST_TNY_SEND_QUEUE_UNKNOWN) {
				status = queue_status;
				break;
			}
		}
	}

	g_free(msg_uid);
	g_slist_free (send_queues);
	return status;
}

typedef struct _WakeupHelper {
	ModestTnySendQueue *self;
	ModestTnySendQueueWakeupFunc callback;
	gpointer userdata;
} WakeupHelper;

static void
wakeup_sync_cb (TnyFolder *self, gboolean cancelled, GError *err, gpointer userdata)
{
	WakeupHelper *helper = (WakeupHelper *) userdata;

	if (helper->callback) {
		helper->callback (helper->self, cancelled, err, helper->userdata);
	}
	g_object_unref (helper->self);
	g_slice_free (WakeupHelper, helper);
}

static void
wakeup_get_headers_async_cb (TnyFolder *folder, 
			     gboolean cancelled, 
			     TnyList *headers, 
			     GError *err, 
			     gpointer user_data)
{
	ModestTnySendQueue *self;
	ModestTnySendQueuePrivate *priv;
	TnyIterator *iter;
	WakeupHelper *helper = (WakeupHelper *) user_data;

	self = MODEST_TNY_SEND_QUEUE (helper->self);
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	if (cancelled || err) {
		g_debug ("Failed to wake up the headers of the send queue");
		g_object_unref (self);
		if (helper->callback) {
			helper->callback (helper->self, cancelled, err, helper->userdata);
		}
		g_object_unref (helper->self);
		g_slice_free (WakeupHelper, helper);
		return;
	}

	/* Wake up every single suspended header */
	iter = tny_list_create_iterator (headers);
	while (!tny_iterator_is_done (iter)) {
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));

		if (tny_header_get_flags (header) & TNY_HEADER_FLAG_SUSPENDED) {
			gchar *msg_id;
			GList *item;
			SendInfo *info;

			/* Unset the suspended flag */
			tny_header_unset_flag (header, TNY_HEADER_FLAG_SUSPENDED);

			/* Notify view */
			msg_id = modest_tny_send_queue_get_msg_id (header);			
			item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), msg_id);
			if (!item) {
				info = g_slice_new (SendInfo);
				info->msg_id = msg_id;
				g_queue_push_tail (priv->queue, info);
			} else {
				info = (SendInfo *) item->data;
				g_free (msg_id);
			}
			info->status = MODEST_TNY_SEND_QUEUE_WAITING;
			g_signal_emit (self, signals[STATUS_CHANGED_SIGNAL], 0, info->msg_id, info->status);		
		}

		/* Frees */
		g_object_unref (header);
		tny_iterator_next (iter);
	}

	/* Make changes persistent on disk */
	tny_folder_sync_async (priv->outbox, FALSE, wakeup_sync_cb, NULL, helper);

	/* Frees */
	g_object_unref (iter);
	g_object_unref (headers);
}

void   
modest_tny_send_queue_wakeup (ModestTnySendQueue *self, 
			      ModestTnySendQueueWakeupFunc callback,
			      gpointer userdata)
{
	ModestTnySendQueuePrivate *priv;
	TnyList *headers;
	WakeupHelper *helper;

	g_return_if_fail (MODEST_IS_TNY_SEND_QUEUE (self));
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	helper = g_slice_new (WakeupHelper);
	helper->self = g_object_ref (self);
	helper->callback = callback;
	helper->userdata = userdata;

	headers = tny_simple_list_new ();
	tny_folder_get_headers_async (priv->outbox, headers, TRUE, 
				      wakeup_get_headers_async_cb, 
				      NULL, helper);
}

gboolean 
modest_tny_send_queue_get_requested_send_receive (ModestTnySendQueue *self)
{
	ModestTnySendQueuePrivate *priv;

	g_return_val_if_fail (MODEST_IS_TNY_SEND_QUEUE (self), FALSE);
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	return priv->requested_send_receive;
}

void 
modest_tny_send_queue_set_requested_send_receive (ModestTnySendQueue *self, gboolean requested_send_receive)
{
	ModestTnySendQueuePrivate *priv;

	g_return_if_fail (MODEST_IS_TNY_SEND_QUEUE (self));
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	priv->requested_send_receive = requested_send_receive;
}

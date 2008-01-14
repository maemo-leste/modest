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

static TnyFolder* modest_tny_send_queue_get_outbox  (TnySendQueue *self);
static TnyFolder* modest_tny_send_queue_get_sentbox (TnySendQueue *self);

/* list my signals  */
enum {
	STATUS_CHANGED,
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
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

static void
modest_tny_send_queue_cancel (TnySendQueue *self, gboolean remove, GError **err)
{
	ModestTnySendQueuePrivate *priv;
	SendInfo *info;
	TnyIterator *iter = NULL;
	TnyFolder *outbox = NULL;
	TnyList *headers = tny_simple_list_new ();
	TnyHeader *header = NULL;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	if(priv->current != NULL)
	{
		info = priv->current->data;
		priv->current = NULL;

		/* Keep in list until retry, so that we know that this was suspended
		 * by the user and not by an error. */
		info->status = MODEST_TNY_SEND_QUEUE_SUSPENDED;

		g_signal_emit (self, signals[STATUS_CHANGED], 0, info->msg_id, info->status);
	}

	/* Set flags to supend sending operaiton (if removed, this is not necessary) */
	if (!remove) {		
		outbox = modest_tny_send_queue_get_outbox (TNY_SEND_QUEUE(self));
		if (!outbox) {
			g_warning ("%s: modest_tny_send_queue_get_outbox(..) returned NULL\n", __FUNCTION__);
			goto frees;
		}
		tny_folder_get_headers (outbox, headers, TRUE, err);
		if (err != NULL) goto frees;
		iter = tny_list_create_iterator (headers);
		while (!tny_iterator_is_done (iter)) {
			header = TNY_HEADER (tny_iterator_get_current (iter));
			if (header) {
				tny_header_set_flag (header, TNY_HEADER_FLAG_SUSPENDED);
				tny_iterator_next (iter);
				g_object_unref (header);
			}
		}
		
		g_queue_foreach (priv->queue, (GFunc)modest_tny_send_queue_info_free, NULL);
		g_queue_free (priv->queue);
		priv->queue = g_queue_new();
	}
		
	/* Dont call super class implementaiton, becasue camel removes messages from outbox */
	TNY_CAMEL_SEND_QUEUE_CLASS(parent_class)->cancel_func (self, remove, err); /* FIXME */

 frees:
	if (headers != NULL)
		g_object_unref (G_OBJECT (headers));
	if (outbox != NULL) 
		g_object_unref (G_OBJECT (outbox));
	if (iter != NULL) 
		g_object_unref (iter);
}

static void
_on_added_to_outbox (TnySendQueue *self, gboolean cancelled, TnyMsg *msg, GError *err,
		     gpointer user_data) 
{
	ModestTnySendQueuePrivate *priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE(self);
	TnyHeader *header = NULL;
	SendInfo *info = NULL;
	GList* existing = NULL;
	gchar* msg_id = NULL;

	g_return_if_fail (TNY_IS_SEND_QUEUE(self));
	g_return_if_fail (TNY_IS_CAMEL_MSG(msg));

	header = tny_msg_get_header (msg);
	msg_id = modest_tny_send_queue_get_msg_id (header);
/* 	msg_id = tny_header_get_message_id (header); */
	g_return_if_fail(msg_id != NULL);

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

	g_signal_emit (self, signals[STATUS_CHANGED], 0, info->msg_id, info->status);

	g_object_unref(G_OBJECT(header));
}

void
modest_tny_send_queue_add (ModestTnySendQueue *self, TnyMsg *msg, GError **err)
{
	g_return_if_fail (MODEST_IS_TNY_SEND_QUEUE(self));
	g_return_if_fail (TNY_IS_CAMEL_MSG(msg));

	tny_camel_send_queue_add_async (TNY_CAMEL_SEND_QUEUE(self), 
					msg, 
					_on_added_to_outbox, 
					NULL, 
					NULL);
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
		info = g_slice_new (SendInfo);
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

static TnyFolder*
modest_tny_send_queue_get_sentbox (TnySendQueue *self)
{
	TnyFolder *folder;
	TnyCamelTransportAccount *account;

	g_return_val_if_fail (self, NULL);

	account = tny_camel_send_queue_get_transport_account (TNY_CAMEL_SEND_QUEUE(self));
	if (!account) {
		g_printerr ("modest: no account for send queue\n");
		return NULL;
	}
	folder  = modest_tny_account_get_special_folder (TNY_ACCOUNT(account),
							 TNY_FOLDER_TYPE_SENT);
	g_object_unref (G_OBJECT(account));

	return folder;
}


static TnyFolder*
modest_tny_send_queue_get_outbox (TnySendQueue *self)
{
	TnyFolder *folder;
	TnyCamelTransportAccount *account;

	g_return_val_if_fail (self, NULL);

	account = tny_camel_send_queue_get_transport_account (TNY_CAMEL_SEND_QUEUE(self));
	if (!account) {
		g_printerr ("modest: no account for send queue\n");
		return NULL;
	}
	folder  = modest_tny_account_get_special_folder (TNY_ACCOUNT(account),
							 TNY_FOLDER_TYPE_OUTBOX);

	/* This vfunc's tinymail contract does not allow it to return NULL. */
	if (!folder) {
		g_warning("%s: Returning NULL.\n", __FUNCTION__);
	}

	g_object_unref (G_OBJECT(account));

	return folder;
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

	TNY_CAMEL_SEND_QUEUE_CLASS(klass)->get_outbox_func  = modest_tny_send_queue_get_outbox;
        TNY_CAMEL_SEND_QUEUE_CLASS(klass)->get_sentbox_func = modest_tny_send_queue_get_sentbox;
        TNY_CAMEL_SEND_QUEUE_CLASS(klass)->cancel_func      = modest_tny_send_queue_cancel;
	klass->status_changed   = NULL;

	signals[STATUS_CHANGED] =
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
}

static void
modest_tny_send_queue_finalize (GObject *obj)
{
	ModestTnySendQueuePrivate *priv;
		
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (obj);

	g_queue_foreach (priv->queue, (GFunc)modest_tny_send_queue_info_free, NULL);
	g_queue_free (priv->queue);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestTnySendQueue*
modest_tny_send_queue_new (TnyCamelTransportAccount *account)
{
	ModestTnySendQueue *self;
	
	g_return_val_if_fail (TNY_IS_CAMEL_TRANSPORT_ACCOUNT(account), NULL);
	
	self = MODEST_TNY_SEND_QUEUE(g_object_new(MODEST_TYPE_TNY_SEND_QUEUE, NULL));
	
	tny_camel_send_queue_set_transport_account (TNY_CAMEL_SEND_QUEUE(self),
						    account); 

	/* Connect signals to control when a msg is being or has been sent */
	g_signal_connect (G_OBJECT(self), "msg-sending",
			  G_CALLBACK(_on_msg_start_sending),
			  NULL);			  
	g_signal_connect (G_OBJECT(self), "msg-sent",
			  G_CALLBACK(_on_msg_has_been_sent), 
			  NULL);
	g_signal_connect (G_OBJECT(self), "error-happened",
	                  G_CALLBACK(_on_msg_error_happened),
			  NULL);
	return self;
}



void
modest_tny_send_queue_try_to_send (ModestTnySendQueue* self)
{
	TnyIterator *iter = NULL;
	TnyFolder *outbox = NULL;
	TnyList *headers = tny_simple_list_new ();
	TnyHeader *header = NULL;
	GError *err = NULL;

	g_return_if_fail (MODEST_IS_TNY_SEND_QUEUE(self));
	
	outbox = modest_tny_send_queue_get_outbox (TNY_SEND_QUEUE(self));
	if (!outbox)
		return;

	tny_folder_get_headers (outbox, headers, TRUE, &err);
	if (err != NULL) 
		goto frees;

	iter = tny_list_create_iterator (headers);
	while (!tny_iterator_is_done (iter)) {
		header = TNY_HEADER (tny_iterator_get_current (iter));
		if (header) {	
			_add_message (self, header); 
			g_object_unref (header);
		}

		tny_iterator_next (iter);
	}
	
	/* Flush send queue */
	tny_camel_send_queue_flush (TNY_CAMEL_SEND_QUEUE(self));

 frees:
	if (headers != NULL)
		g_object_unref (G_OBJECT (headers));
	if (outbox != NULL) 
		g_object_unref (G_OBJECT (outbox));
	if (iter != NULL) 
		g_object_unref (iter);
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
	
	return priv->current != NULL;
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
	const gchar *subject;
	time_t date_received;
		
	g_return_val_if_fail (header && TNY_IS_HEADER(header), NULL);

	/* Get message uid */
	subject = tny_header_get_subject (header);
	date_received = tny_header_get_date_received (header);

	msg_uid = g_strdup_printf ("%s %d", subject, (int) date_received);

	return msg_uid;
}


static void
_on_msg_start_sending (TnySendQueue *self,
		       TnyHeader *header,
		       TnyMsg *msg,
		       int done, 
		       int total,
		       gpointer user_data)
{
	ModestTnySendQueuePrivate *priv = NULL;
	GList *item = NULL;
	SendInfo *info = NULL;
	gchar *msg_id = NULL;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	/* Get message uid */
	msg_id = modest_tny_send_queue_get_msg_id (header);

	/* Get status info */
	item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), msg_id);
	if (!item) 
		g_warning  ("%s: item (%s) should not be NULL",
			    __FUNCTION__, msg_id ? msg_id : "<none>");
	else {
		info = item->data;
		info->status = MODEST_TNY_SEND_QUEUE_SENDING;
		
		/* Set current status item */
		g_signal_emit (self, signals[STATUS_CHANGED], 0, info->msg_id, info->status);
		priv->current = item;
	}

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

	/* Get status info */
	item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), msg_id);
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
	SendInfo *info = NULL;
	GList *item = NULL;
	gchar* msg_uid = NULL;
	
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	/* Get sending info (create new if it doesn not exist) */
	msg_uid = modest_tny_send_queue_get_msg_id (header);
	item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), 
						  msg_uid);	
	if (item == NULL) {
		info = g_slice_new (SendInfo);
		info->msg_id = (msg_uid != NULL)? strdup(msg_uid) : NULL;
		g_queue_push_tail (priv->queue, info);
	} else
		info = item->data;

	/* Keep in queue so that we remember that the opertion has failed */
	/* and was not just cancelled */
	info->status = MODEST_TNY_SEND_QUEUE_FAILED;
	priv->current = NULL;
	
	/* Notify status has changed */
	g_signal_emit (self, signals[STATUS_CHANGED], 0, info->msg_id, info->status);

	/* free */
	g_free(msg_uid);
}

static void 
fill_list_of_caches (gpointer key, gpointer value, gpointer userdata)
{
	GSList **send_queues = (GSList **) userdata;
	*send_queues = g_slist_prepend (*send_queues, value);
}

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
			send_queue = modest_runtime_get_send_queue(TNY_TRANSPORT_ACCOUNT(account));
			g_object_unref(account);

			queue_status = modest_tny_send_queue_get_msg_status (send_queue, msg_uid);
			if (queue_status != MODEST_TNY_SEND_QUEUE_UNKNOWN) {
				status = queue_status;
				break;
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

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
#include <modest-tny-account.h>
#include <modest-marshal.h>
#include <string.h> /* strcmp */

/* 'private'/'protected' functions */
static void modest_tny_send_queue_class_init (ModestTnySendQueueClass *klass);
static void modest_tny_send_queue_finalize   (GObject *obj);
static void modest_tny_send_queue_instance_init (GTypeInstance *instance, gpointer g_class);

/* Signal handlers */ 
/* static void _on_msg_start_sending (TnySendQueue *self, TnyMsg *msg, gpointer user_data); */
static void _on_msg_has_been_sent (TnySendQueue *self, TnyMsg *msg, gpointer user_data);
static void _on_msg_error_happened (TnySendQueue *self, TnyHeader *header, TnyMsg *msg, GError *err, gpointer user_data);

static TnyFolder*modest_tny_send_queue_get_outbox (TnySendQueue *self);
static TnyFolder*modest_tny_send_queue_get_sentbox (TnySendQueue *self);

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
on_modest_tny_send_queue_compare_id(gconstpointer info, gconstpointer msg_id)
{
	return strcmp( ((SendInfo*)info)->msg_id, msg_id);
}

static void
modest_tny_send_queue_info_free(SendInfo *info)
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
modest_tny_send_queue_cancel (TnySendQueue *self, gboolean remove, GError **err)
{
	ModestTnySendQueuePrivate *priv;
	SendInfo *info;

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
		TnyIterator *iter = NULL;
		TnyFolder *outbox = NULL;
		TnyList *headers = tny_simple_list_new ();
		outbox = modest_tny_send_queue_get_outbox (self);
		tny_folder_get_headers (outbox, headers, TRUE, err);
		if (err != NULL && *err != NULL) goto frees;
		iter = tny_list_create_iterator (headers);
		while (!tny_iterator_is_done (iter)) {
			TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
			tny_header_unset_flags (header, TNY_HEADER_FLAG_PRIORITY);
			tny_header_set_flags (header, TNY_HEADER_FLAG_SUSPENDED_PRIORITY);
			g_object_unref (header);
			tny_iterator_next (iter);
		}
	frees:
		g_object_unref (G_OBJECT (headers));
		g_object_unref (G_OBJECT (outbox));
	}
		
	/* Dont call super class implementaiton, becasue camel removes messages from outbox */
	TNY_CAMEL_SEND_QUEUE_CLASS(parent_class)->cancel_func (self, remove, err); /* FIXME */
}

static void
modest_tny_send_queue_add (TnySendQueue *self, TnyMsg *msg, GError **err)
{
	ModestTnySendQueuePrivate *priv;
	TnyHeader *header;
	SendInfo *info = NULL;
	GList* existing;
	const gchar* msg_id;

	g_return_if_fail (TNY_IS_SEND_QUEUE(self));
	g_return_if_fail (TNY_IS_CAMEL_MSG(msg));

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	header = tny_msg_get_header (msg);

	/* FIXME: do something smart here... */

	/* Note that this call actually sets the message id to something
	 * sensible. */	
	TNY_CAMEL_SEND_QUEUE_CLASS(parent_class)->add_func (self, msg, err); /* FIXME */

	/* Check whether the mail is already in the queue */
	msg_id = tny_header_get_message_id (header);
	existing = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE(self), msg_id);
	if(existing != NULL)
	{
		//g_assert(info->status == MODEST_TNY_SEND_QUEUE_SUSPENDED ||
		//        info->status == MODEST_TNY_SEND_QUEUE_FAILED);

		info = existing->data;
		info->status = MODEST_TNY_SEND_QUEUE_WAITING;
	}
	else
	{
		
		info = g_slice_new (SendInfo);

		info->msg_id = strdup(msg_id);
		info->status = MODEST_TNY_SEND_QUEUE_WAITING;
		g_queue_push_tail (priv->queue, info);
	}

	g_signal_emit (self, signals[STATUS_CHANGED], 0, info->msg_id, info->status);
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

	TNY_CAMEL_SEND_QUEUE_CLASS(klass)->add_func         = modest_tny_send_queue_add;
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
	/* TODO: this signal was implemented in tinymail camel send queue, but im
	   waiting for implement some unit tests nbefore commited changes */
/* 	if (FALSE) { */
/* 		g_signal_connect (G_OBJECT(self), "msg-sending", */
/* 				  G_CALLBACK(_on_msg_start_sending),  */
/* 				  NULL); */
/* 	} */
			  
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
	/* Flush send queue */
	tny_camel_send_queue_flush (TNY_CAMEL_SEND_QUEUE(self));
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
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	return priv->current != NULL;
}

ModestTnySendQueueStatus
modest_tny_send_queue_get_msg_status (ModestTnySendQueue *self, const gchar *msg_id)
{
  GList *item;
  item = modest_tny_send_queue_lookup_info (self, msg_id);
  if(item == NULL) return MODEST_TNY_SEND_QUEUE_SUSPENDED;
  return ((SendInfo*)item->data)->status;
}

/* static void  */
/* _on_msg_start_sending (TnySendQueue *self, */
/* 		       TnyMsg *msg, */
/* 		       gpointer user_data) */
/* { */
/* 	ModestTnySendQueuePrivate *priv; */
/* 	TnyHeader *header; */
/* 	GList *item; */
/* 	SendInfo *info; */

/* 	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self); */

/* 	header = tny_msg_get_header(msg); */
/* 	item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), tny_header_get_message_id (header)); */

/* 	if (item != NULL) */
/* 	{ */
/* 		info = item->data; */
/* 		info->status = MODEST_TNY_SEND_QUEUE_SENDING; */

/* 		g_signal_emit (self, signals[STATUS_CHANGED], 0, info->msg_id, info->status); */
/* 	} */

/* 	priv->current = item; */
/* } */

static void 
_on_msg_has_been_sent (TnySendQueue *self,
		       TnyMsg *msg, 
		       gpointer user_data)
{
	ModestTnySendQueuePrivate *priv;
	TnyHeader *header;
	GList *item;

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	header = tny_msg_get_header (msg);

	/* TODO: Use this version as soon as the msg-sending
	 *  notification works */
#if 0
	item = priv->current;
	g_assert(item != NULL);
#else
	item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self), tny_header_get_message_id(header));
	g_assert(item != NULL);
#endif

	modest_tny_send_queue_info_free (item->data);
	g_queue_delete_link (priv->queue, item);
	priv->current = NULL;
}

static void _on_msg_error_happened (TnySendQueue *self,
                                    TnyHeader *header,
				    TnyMsg *msg,
				    GError *err,
				    gpointer user_data)
{
/* 	ModestTnySendQueuePrivate *priv; */
/* 	SendInfo *info; */
/* 	GList *item; */
/* 	TnyHeader *msg_header; */

/* 	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self); */

	/* TODO: Use this version as soon as the msg-sending
	 *  notification works */
/* #if 0 */
/* 	item = priv->current; */
/* 	g_assert(item != NULL); */
/* 	info = priv->current->data; */
/* #else */
/* 	/\* TODO: Why do we get the msg and its header separately? The docs */
/* 	 * don't really tell. *\/ */
/* 	g_assert(header == tny_msg_get_header (msg)); // ???? */
/* 	msg_header = tny_msg_get_header (msg); */
/* 	item = modest_tny_send_queue_lookup_info (MODEST_TNY_SEND_QUEUE (self),  */
/* 						  tny_header_get_message_id (msg_header)); */
/* 	g_object_unref (msg_header); */
/* 	g_assert(item != NULL); */
/* 	info = item->data; */
/* #endif */

	/* Keep in queue so that we remember that the opertion has failed
	 * and was not just cancelled */
/* 	info->status = MODEST_TNY_SEND_QUEUE_FAILED; */
/* 	g_signal_emit (self, signals[STATUS_CHANGED], 0, info->msg_id, info->status); */
}

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

#include <glib.h>
#include <glib-object.h>
#include <tny-send-queue.h>
#include <tny-camel-send-queue.h>
#include <tny-msg.h>
#include <tny-camel-transport-account.h>

#ifndef __MODEST_TNY_SEND_QUEUE_H__
#define __MODEST_TNY_SEND_QUEUE_H__

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_SEND_QUEUE             (modest_tny_send_queue_get_type())
#define MODEST_TNY_SEND_QUEUE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_SEND_QUEUE,ModestTnySendQueue))
#define MODEST_TNY_SEND_QUEUE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_SEND_QUEUE,TnySendQueue))
#define MODEST_IS_TNY_SEND_QUEUE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_SEND_QUEUE))
#define MODEST_IS_TNY_SEND_QUEUE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_SEND_QUEUE))
#define MODEST_TNY_SEND_QUEUE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_SEND_QUEUE,ModestTnySendQueueClass))

typedef struct _ModestTnySendQueue      ModestTnySendQueue;
typedef struct _ModestTnySendQueueClass ModestTnySendQueueClass;

typedef enum {
	MODEST_TNY_SEND_QUEUE_UNKNOWN,
	MODEST_TNY_SEND_QUEUE_WAITING,
	MODEST_TNY_SEND_QUEUE_SUSPENDED,
	MODEST_TNY_SEND_QUEUE_SENDING,
	MODEST_TNY_SEND_QUEUE_FAILED
} ModestTnySendQueueStatus;

struct _ModestTnySendQueue {
	TnyCamelSendQueue  parent;
};

struct _ModestTnySendQueueClass {
	TnyCamelSendQueueClass parent_class;

	/* Signals */
	void (*status_changed)(ModestTnySendQueue *self, const gchar *msg_id, ModestTnySendQueueStatus status);
};

/**
 * modest_tny_send_queue_get_type:
 * 
 * get the #GType for #ModestTnySendQueue
 *  
 * Returns: the #GType
 */
GType        modest_tny_send_queue_get_type    (void) G_GNUC_CONST;


/**
 * modest_tny_send_queue_new:
 * @account: a valid camel transport account
 * 
 * create a new modest #ModestTnySendQueue object. 
 * 
 * Returns: a new #ModestTnySendQueue instance, or NULL in case
 * of any error
 */
ModestTnySendQueue*    modest_tny_send_queue_new        (TnyCamelTransportAccount *account);


/**
 * modest_tny_send_queue_sending_in_progress:
 * @self: a valid #ModestTnySendQueue instance
 *
 * Checks if sending operation is currently in progress on @self send queue.
 */
gboolean modest_tny_send_queue_sending_in_progress (ModestTnySendQueue* self);

/**
 * modest_tny_send_queue_msg_is_being_sent:
 * @self: a valid #ModestTnySendQueue instance
 * @msg_id: the message id ti check.
 *
 * Checks if message identifies with @msg_id is currently being sent.
 */
gboolean modest_tny_send_queue_msg_is_being_sent (ModestTnySendQueue* self, const gchar *msg_id);

/**
 * modest_tny_send_queue_get_msg_status:
 * @self: a valid #ModestTnySendQueue instance
 * @msg_id: The message id to check
 *
 * Returns the status of the message identified with @msg_id. The status tells
 * whether the message is currently being sent, is waiting for being sent or
 * sending the message failed.
 */
ModestTnySendQueueStatus
modest_tny_send_queue_get_msg_status (ModestTnySendQueue *self, const gchar *msg_id);

gchar *
modest_tny_send_queue_get_msg_id (TnyHeader *header);

/**
 * modest_tny_all_send_queues_get_msg_status:
 * @header: a #TnyHeader
 *
 * obtain status of message (searching for it in all queues)
 *
 * Returns: a #ModestTnySendQueueStatus
 */
ModestTnySendQueueStatus
modest_tny_all_send_queues_get_msg_status (TnyHeader *header);


/**
 * modest_tny_send_queue_to_string:
 * @self: a valid #ModestTnySendQueue instance
 *
 * get a string representation of a send queue (for debugging)
 *
 * Returns: a newly allocated string, or NULL in case of error
 */
gchar* modest_tny_send_queue_to_string (ModestTnySendQueue *self);

typedef void (*ModestTnySendQueueWakeupFunc) (ModestTnySendQueue *self, gboolean cancelled, GError *err, gpointer userdata);

/**
 * modest_tny_send_queue_wakeup:
 * @self: a valid #ModestTnySendQueue instance
 *
 * Wakes up all suspended messages in the send queue. This means that
 * the send queue will try to send them again. Note that you'd
 * probably need a tny_send_queue_flush to force it
 */
void   modest_tny_send_queue_wakeup (ModestTnySendQueue *self, 
				     ModestTnySendQueueWakeupFunc callback,
				     gpointer userdata);

/**
 * modest_tny_send_queue_get_requested_send_receive:
 * @self: a #ModestTnySendQueue
 *
 * gets if the last request to send queue was an interactive send
 * receive or not.
 *
 * Returns: %TRUE if last request was an interactive send receive,
 * %FALSE otherwise.
 */
gboolean modest_tny_send_queue_get_requested_send_receive (ModestTnySendQueue *self);

/**
 * modest_tny_send_queue_set_requested_send_receive:
 * @self: a #ModestTnySendQueue
 * @requested_send_receive: mode.
 *
 * this should be called on each call to process the queue, to distinguish if the
 * action was an interactive send receive.
 */
void modest_tny_send_queue_set_requested_send_receive (ModestTnySendQueue *self, gboolean requested_send_receive);


G_END_DECLS

#endif /* __MODEST_TNY_SEND_QUEUE_H__ */


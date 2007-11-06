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

/* modest-tny-platform-factory.h */

#ifndef __MODEST_MAIL_OPERATION_QUEUE_H__
#define __MODEST_MAIL_OPERATION_QUEUE_H__

#include <glib-object.h>
#include "modest-mail-operation.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAIL_OPERATION_QUEUE             (modest_mail_operation_queue_get_type())
#define MODEST_MAIL_OPERATION_QUEUE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAIL_OPERATION_QUEUE,ModestMailOperationQueue))
#define MODEST_MAIL_OPERATION_QUEUE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MAIL_OPERATION_QUEUE,GObject))
#define MODEST_IS_MAIL_OPERATION_QUEUE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAIL_OPERATION_QUEUE))
#define MODEST_IS_MAIL_OPERATION_QUEUE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MAIL_OPERATION_QUEUE))
#define MODEST_MAIL_OPERATION_QUEUE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MAIL_OPERATION_QUEUE,ModestMailOperationQueueClass))

typedef enum _ModestMailOperationQueueNotification {
	MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED,
	MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED
} ModestMailOperationQueueNotification;

typedef struct _ModestMailOperationQueue      ModestMailOperationQueue;
typedef struct _ModestMailOperationQueueClass ModestMailOperationQueueClass;

struct _ModestMailOperationQueue {
	 GObject parent;
};

struct _ModestMailOperationQueueClass {
	GObjectClass parent_class;

	/* Signals */
	void (*queue_changed) (ModestMailOperationQueue *self, 
			       ModestMailOperation *mail_op,
			       ModestMailOperationQueueNotification type);

	void (*queue_empty) (ModestMailOperationQueue *self);
};

/* member functions */
GType                   modest_mail_operation_queue_get_type      (void) G_GNUC_CONST;

ModestMailOperationQueue *    modest_mail_operation_queue_new  (void);

/**
 * modest_mail_operation_queue_add:
 * @op_queue: a #ModestMailOperationQueue
 * @mail_op: the #ModestMailOperation that will be added to the queue
 * 
 * Adds a mail operation at the end of the queue. It also adds a
 * reference to the mail operation so the caller could free it
 **/
void    modest_mail_operation_queue_add        (ModestMailOperationQueue *op_queue, 
						ModestMailOperation *mail_op);

/**
 * modest_mail_operation_queue_remove:
 * @op_queue: a #ModestMailOperationQueue
 * @mail_op: the #ModestMailOperation that will be removed from the queue
 * 
 * Removes a mail operation from the queue. This method does not free
 * the mail operation
 **/
void    modest_mail_operation_queue_remove     (ModestMailOperationQueue *op_queue, 
						ModestMailOperation *mail_op);
/**
 * modest_mail_operation_queue_num_elements:
 * @op_queue:  a #ModestMailOperationQueue
 * 
 * Gets the numeber of elements stored in #ModestMailOperationQueue.
 **/
guint 
modest_mail_operation_queue_num_elements (ModestMailOperationQueue *self);

/**
 * modest_mail_operation_queue_cancel:
 * @op_queue:  a #ModestMailOperationQueue
 * @mail_op:  the #ModestMailOperation that will be canceled
 * 
 * Cancels a #ModestMailOperation if it's not finished and removes it
 * from the queue
 **/
void    modest_mail_operation_queue_cancel     (ModestMailOperationQueue *op_queue, 
						ModestMailOperation *mail_op);
/**
 * modest_mail_operation_queue_cancel_all:
 * @op_queue:  a #ModestMailOperationQueue
 * 
 * Cancels all the unfinished #ModestMailOperation of the queue
 **/
void    modest_mail_operation_queue_cancel_all (ModestMailOperationQueue *op_queue);

G_END_DECLS

#endif /* __MODEST_MAIL_OPERATION_QUEUE_H__ */

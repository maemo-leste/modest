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

#include "config.h"
#include "modest-marshal.h"
#include "modest-mail-operation-queue.h"
#include "modest-runtime.h"

/* 'private'/'protected' functions */
static void modest_mail_operation_queue_class_init (ModestMailOperationQueueClass *klass);
static void modest_mail_operation_queue_init       (ModestMailOperationQueue *obj);
static void modest_mail_operation_queue_finalize   (GObject *obj);

static void
on_progress_changed (ModestMailOperation *mail_op,
                     ModestMailOperationState *state,
		     gpointer user_data);

/* list my signals  */
enum {
	QUEUE_CHANGED_SIGNAL,
	NUM_SIGNALS
};

typedef struct _ModestMailOperationQueuePrivate ModestMailOperationQueuePrivate;
struct _ModestMailOperationQueuePrivate {
	GQueue *op_queue;
	GMutex *queue_lock;
	guint   op_id;
};
#define MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                         MODEST_TYPE_MAIL_OPERATION_QUEUE, \
                                                         ModestMailOperationQueuePrivate))
/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[NUM_SIGNALS] = {0};

GType
modest_mail_operation_queue_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMailOperationQueueClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_mail_operation_queue_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMailOperationQueue),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_mail_operation_queue_init,
			NULL
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestMailOperationQueue",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_mail_operation_queue_class_init (ModestMailOperationQueueClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	parent_class  = g_type_class_peek_parent (klass);

	gobject_class->finalize    = modest_mail_operation_queue_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMailOperationQueuePrivate));

	/**
	 * ModestMailOperationQueue::queue-changed
	 * @self: the #ModestMailOperationQueue that emits the signal
	 * @mail_op: the #ModestMailOperation affected
	 * @type: the type of change in the queue
	 * @user_data: user data set when the signal handler was connected
	 *
	 * Emitted whenever the contents of the queue change
	 */
	signals[QUEUE_CHANGED_SIGNAL] =
		g_signal_new ("queue-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestMailOperationQueueClass, queue_changed),
			      NULL, NULL,
			      modest_marshal_VOID__POINTER_INT,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
}

static void
modest_mail_operation_queue_init (ModestMailOperationQueue *obj)
{
	ModestMailOperationQueuePrivate *priv;

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(obj);

	priv->op_queue   = g_queue_new ();
	priv->queue_lock = g_mutex_new ();
	priv->op_id = 0;
}

static void
on_finalize_foreach(gpointer op,
                    gpointer user_data)
{
	ModestMailOperationQueue *queue;
	ModestMailOperationQueuePrivate *priv;
	ModestMailOperation *mail_op;

	queue = MODEST_MAIL_OPERATION_QUEUE (user_data);
	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE (queue);
	mail_op = MODEST_MAIL_OPERATION (op);

	/* Simply remove from queue, but without emitting a
	 * QUEUE_CHANGED_SIGNAL because we are in finalize anyway and have
	 * the lock acquired. */
	g_signal_handlers_disconnect_by_func (mail_op, G_CALLBACK (on_progress_changed), user_data);

	modest_mail_operation_cancel (mail_op);
	g_queue_remove (priv->op_queue, mail_op);
	g_object_unref (G_OBJECT (mail_op));
}

static void
modest_mail_operation_queue_finalize (GObject *obj)
{
	ModestMailOperationQueuePrivate *priv;

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(obj);

	g_mutex_lock (priv->queue_lock);

	if (priv->op_queue) {
		/* Cancel all */
		if (!g_queue_is_empty (priv->op_queue)) {
			g_queue_foreach (priv->op_queue,
			                 (GFunc)on_finalize_foreach,
			                 MODEST_MAIL_OPERATION_QUEUE (obj));
		}

		g_queue_free (priv->op_queue);
	}

	g_mutex_unlock (priv->queue_lock);
	g_mutex_free (priv->queue_lock);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestMailOperationQueue *
modest_mail_operation_queue_new (void)
{
	ModestMailOperationQueue *self = g_object_new (MODEST_TYPE_MAIL_OPERATION_QUEUE, NULL);

	return MODEST_MAIL_OPERATION_QUEUE (self);
}

static void
on_progress_changed (ModestMailOperation *mail_op,
                     ModestMailOperationState *state,
		     gpointer user_data)
{
	ModestMailOperationQueue *queue;

	if(!state->finished)
		return;

	/* Remove operation from queue when finished */
	queue = MODEST_MAIL_OPERATION_QUEUE (user_data);
	modest_mail_operation_queue_remove (queue, mail_op);
}

void 
modest_mail_operation_queue_add (ModestMailOperationQueue *self, 
				 ModestMailOperation *mail_op)
{
	ModestMailOperationQueuePrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION_QUEUE (self));
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (mail_op));
	
	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(self);

	g_mutex_lock (priv->queue_lock);
	g_queue_push_tail (priv->op_queue, g_object_ref (mail_op));
	modest_mail_operation_set_id (mail_op, priv->op_id++);
	g_mutex_unlock (priv->queue_lock);

	/* Get notified when the operation ends to remove it from the queue */
	g_signal_connect (G_OBJECT (mail_op), "progress_changed",
	                  G_CALLBACK (on_progress_changed), self);

	/* Notify observers */
	g_signal_emit (self, signals[QUEUE_CHANGED_SIGNAL], 0,
		       mail_op, MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED);
}

void 
modest_mail_operation_queue_remove (ModestMailOperationQueue *self,
				    ModestMailOperation *mail_op)
{
	ModestMailOperationQueuePrivate *priv;
	ModestMailOperationStatus status;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION_QUEUE (self));
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (mail_op));

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(self);

	g_mutex_lock (priv->queue_lock);
	g_queue_remove (priv->op_queue, mail_op);
	g_mutex_unlock (priv->queue_lock);

	g_signal_handlers_disconnect_by_func (G_OBJECT (mail_op),
	                                      G_CALLBACK (on_progress_changed),
	                                      self);

	/* Notify observers */
	g_signal_emit (self, signals[QUEUE_CHANGED_SIGNAL], 0,
		       mail_op, MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED);

	/* Check errors */
	status = modest_mail_operation_get_status (mail_op);
	if (status != MODEST_MAIL_OPERATION_STATUS_SUCCESS) {
		/* This is a sanity check. Shouldn't be needed, but
		   prevent possible application crashes. It's useful
		   also for detecting mail operations with invalid
		   status and error handling */
		if (modest_mail_operation_get_error (mail_op) != NULL)
			modest_mail_operation_execute_error_handler (mail_op);
		else {
			if (status == MODEST_MAIL_OPERATION_STATUS_CANCELED) 
				g_warning ("%s: operation canceled \n", __FUNCTION__);
			else
				g_warning ("%s: possible error in a mail operation " \
					   "implementation. The status is not successful " \
					   "but the mail operation does not have any " \
					   "error set\n", __FUNCTION__);
		}
	}

	/* Free object */

	/* We do not own the last reference when this operation is deleted
	 * as response to a progress changed signal from the mail operation
	 * itself, in which case the glib signal system owns a reference
	 * until the signal emission is complete. armin. */
	/* modest_runtime_verify_object_last_ref (mail_op, ""); */
	g_object_unref (G_OBJECT (mail_op));
}

guint 
modest_mail_operation_queue_num_elements (ModestMailOperationQueue *self)
{
	ModestMailOperationQueuePrivate *priv;
	guint length = 0;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION_QUEUE (self), 0);
	
	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(self);

	g_mutex_lock (priv->queue_lock);
	length = g_queue_get_length (priv->op_queue);
	g_mutex_unlock (priv->queue_lock);

	return length;
}

void 
modest_mail_operation_queue_cancel (ModestMailOperationQueue *self, 
				    ModestMailOperation *mail_op)
{
	ModestMailOperationQueuePrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION_QUEUE (self));
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (mail_op));

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(self);

	/* This triggers a progess_changed signal in which we remove
	 * the operation from the queue. */
	modest_mail_operation_cancel (mail_op);
}

static void
on_cancel_all_foreach (gpointer op, gpointer list)
{
	*((GSList**)list) = g_slist_prepend (*((GSList**)list), MODEST_MAIL_OPERATION (op));
}

void 
modest_mail_operation_queue_cancel_all (ModestMailOperationQueue *self)
{
	ModestMailOperationQueuePrivate *priv;
	GSList* operations_to_cancel;
	GSList* cur;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION_QUEUE (self));

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(self);

	/* Remember which operations to cancel. This is the only thing that
	 * is done while holding the lock, so we do not need to call
	 * functions from other files while holding the lock, which could
	 * lead to a deadlock if such a call re-enters into this queue and
	 * tries to acquire another lock. */
	g_mutex_lock (priv->queue_lock);
	g_queue_foreach (priv->op_queue, (GFunc) on_cancel_all_foreach, &operations_to_cancel);
	g_mutex_unlock (priv->queue_lock);

	/* TODO: Reverse the list, to remove operations in order? */

	for(cur = operations_to_cancel; cur != NULL; cur = cur->next) {
		/* This triggers a progress_changed signal in which we remove
		 * the operation from the queue. */
		modest_mail_operation_cancel (MODEST_MAIL_OPERATION (cur->data));
	}

	g_slist_free(operations_to_cancel);
}

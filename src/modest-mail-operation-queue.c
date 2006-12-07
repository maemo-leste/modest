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
#include "modest-mail-operation-queue.h"

/* 'private'/'protected' functions */
static void modest_mail_operation_queue_class_init (ModestMailOperationQueueClass *klass);
static void modest_mail_operation_queue_init       (ModestMailOperationQueue *obj);
static void modest_mail_operation_queue_finalize   (GObject *obj);

static GObject *modest_mail_operation_queue_constructor (GType type, guint n_construct_params,
							 GObjectConstructParam *construct_params);

static void modest_mail_operation_queue_cancel_no_block_wrapper (ModestMailOperation *mail_op,
								 ModestMailOperationQueue *op_queue);

static void modest_mail_operation_queue_cancel_no_block         (ModestMailOperationQueue *op_queue,
								 ModestMailOperation *mail_op);

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMailOperationQueuePrivate ModestMailOperationQueuePrivate;
struct _ModestMailOperationQueuePrivate {
	GQueue *op_queue;
	GMutex *queue_lock;
};
#define MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                         MODEST_TYPE_MAIL_OPERATION_QUEUE, \
                                                         ModestMailOperationQueuePrivate))
/* globals */
static GObjectClass *parent_class = NULL;
static ModestMailOperationQueue *singleton = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

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
	gobject_class->constructor = modest_mail_operation_queue_constructor;

	g_type_class_add_private (gobject_class, sizeof(ModestMailOperationQueuePrivate));
}

static void
modest_mail_operation_queue_init (ModestMailOperationQueue *obj)
{
	ModestMailOperationQueuePrivate *priv;

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(obj);

	priv->op_queue   = g_queue_new ();
	priv->queue_lock = g_mutex_new ();
}

static GObject*
modest_mail_operation_queue_constructor (GType type, guint n_construct_params,
					 GObjectConstructParam *construct_params)
{
	GObject *object;

	if (!singleton)	{
		object = G_OBJECT_CLASS (parent_class)->constructor (type,
				n_construct_params, construct_params);

		singleton = MODEST_MAIL_OPERATION_QUEUE (object);
	} else {
		object = G_OBJECT (singleton);
		g_object_freeze_notify (G_OBJECT (singleton));
	}

	return object;
}

static void
modest_mail_operation_queue_finalize (GObject *obj)
{
	ModestMailOperationQueuePrivate *priv;

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(obj);

	g_mutex_lock (priv->queue_lock);

	if (priv->op_queue) {
		if (!g_queue_is_empty (priv->op_queue))
			g_queue_foreach (priv->op_queue, (GFunc) g_object_unref, NULL);
		g_queue_free (priv->op_queue);
	}

	g_mutex_unlock (priv->queue_lock);
	g_mutex_free (priv->queue_lock);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestMailOperationQueue *
modest_mail_operation_queue_get_instance (void)
{
	ModestMailOperationQueue *self = g_object_new (MODEST_TYPE_MAIL_OPERATION_QUEUE, NULL);

	return MODEST_MAIL_OPERATION_QUEUE (self);
}

void 
modest_mail_operation_queue_add (ModestMailOperationQueue *op_queue, 
				 ModestMailOperation *mail_op)
{
	ModestMailOperationQueuePrivate *priv;

	if (!MODEST_IS_MAIL_OPERATION (mail_op) ||
	    !MODEST_IS_MAIL_OPERATION_QUEUE (op_queue)) {
		g_warning ("%s: bad parametters", G_GNUC_FUNCTION);
		return;
	}

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(op_queue);

	g_mutex_lock (priv->queue_lock);
	g_queue_push_tail (priv->op_queue, g_object_ref (mail_op));
	g_mutex_unlock (priv->queue_lock);
}

void 
modest_mail_operation_queue_remove (ModestMailOperationQueue *op_queue, 
				    ModestMailOperation *mail_op)
{
	ModestMailOperationQueuePrivate *priv;

	if (!MODEST_IS_MAIL_OPERATION (mail_op) ||
	    !MODEST_IS_MAIL_OPERATION_QUEUE (op_queue)) {
		g_warning ("%s: invalid paramette", G_GNUC_FUNCTION);
		return;
	}

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(op_queue);

	g_mutex_lock (priv->queue_lock);
	g_queue_remove (priv->op_queue, mail_op);
	g_mutex_unlock (priv->queue_lock);

	g_object_unref (G_OBJECT (mail_op));
}


/* Utility function intended to be used with g_queue_foreach */
static void
modest_mail_operation_queue_cancel_no_block_wrapper (ModestMailOperation *mail_op,
						     ModestMailOperationQueue *op_queue)
{
	modest_mail_operation_queue_cancel_no_block (op_queue, mail_op);
}

static void 
modest_mail_operation_queue_cancel_no_block (ModestMailOperationQueue *op_queue,
					     ModestMailOperation *mail_op)
{
	/* TODO: place here the cancel code */
}

void 
modest_mail_operation_queue_cancel (ModestMailOperationQueue *op_queue, 
				    ModestMailOperation *mail_op)
{
	ModestMailOperationQueuePrivate *priv;
	GList *iter;

	if (!MODEST_IS_MAIL_OPERATION (mail_op) ||
	    !MODEST_IS_MAIL_OPERATION_QUEUE (op_queue)) {
		g_warning ("%s: invalid paramette", G_GNUC_FUNCTION);
		return;
	}

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(op_queue);

	g_mutex_lock (priv->queue_lock);
	modest_mail_operation_queue_cancel_no_block (op_queue, mail_op);
	g_mutex_unlock (priv->queue_lock);
}

void 
modest_mail_operation_queue_cancel_all (ModestMailOperationQueue *op_queue)
{
	ModestMailOperationQueuePrivate *priv;

	if (!MODEST_IS_MAIL_OPERATION_QUEUE (op_queue)) {
		g_warning ("%s: invalid paramette", G_GNUC_FUNCTION);
		return;
	}

	priv = MODEST_MAIL_OPERATION_QUEUE_GET_PRIVATE(op_queue);

	g_mutex_lock (priv->queue_lock);
	g_queue_foreach (priv->op_queue, 
			 (GFunc) modest_mail_operation_queue_cancel_no_block_wrapper, 
			 op_queue);
	g_mutex_unlock (priv->queue_lock);
}

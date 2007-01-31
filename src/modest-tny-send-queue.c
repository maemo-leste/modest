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

/* 'private'/'protected' functions */
static void modest_tny_send_queue_class_init (ModestTnySendQueueClass *klass);
static void modest_tny_send_queue_init       (gpointer g, gpointer iface_data);
static void modest_tny_send_queue_finalize   (GObject *obj);
static void modest_tny_send_queue_instance_init (GTypeInstance *instance, gpointer g_class);

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnySendQueuePrivate ModestTnySendQueuePrivate;
struct _ModestTnySendQueuePrivate {
	TnyTransportAccount *account;
	GThread *flush_outbox_thread;
	GMutex  *flush_lock;
	
};
#define MODEST_TNY_SEND_QUEUE_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_TNY_SEND_QUEUE, \
                                                   ModestTnySendQueuePrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */


/*
 * this thread actually tries to send all the mails in the outbox
 */
static void
flush_outbox_thread (TnySendQueue *self)
{
	TnyFolder *outbox, *sentbox;
	TnyMsg    *msg;
	TnyHeader *header;

	TnyList *headers;		
	TnyIterator *iter;

	ModestTnySendQueuePrivate *priv;

	priv    = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	outbox  = tny_send_queue_get_outbox (self);
	sentbox = tny_send_queue_get_sentbox (self);
	
	headers = tny_simple_list_new ();

	g_mutex_lock (priv->flush_lock);
	tny_folder_get_headers (outbox, headers, TRUE, NULL); /* FIXME: err */
	iter = tny_list_create_iterator (headers);

	while (!tny_iterator_is_done (iter)) {
		header = TNY_HEADER(tny_iterator_get_current(iter));
		msg = tny_folder_get_msg (outbox, header, NULL);	
		tny_transport_account_send (priv->account,
					    msg, NULL); /* FIXME: err */
		tny_folder_add_msg    (sentbox, msg, NULL); /* FIXME: err */
		tny_folder_remove_msg (outbox, header, NULL); /* FIXME: err */

		g_object_unref (G_OBJECT(header));
		g_object_unref (G_OBJECT(msg));

		tny_iterator_next (iter);
	}
	
	
	g_object_unref (G_OBJECT(headers));
	g_object_unref (G_OBJECT(iter));

	priv->flush_outbox_thread = NULL;
	g_mutex_unlock (priv->flush_lock);
	
	g_thread_exit (NULL);
}


static void
modest_tny_send_queue_cancel (TnySendQueue *self, gboolean remove)
{
	MODEST_TNY_SEND_QUEUE_GET_CLASS(self)->cancel_func (self, remove);
}

static void
modest_tny_send_queue_add (TnySendQueue *self, TnyMsg *msg)
{
	MODEST_TNY_SEND_QUEUE_GET_CLASS(self)->add_func (self, msg);
}


static TnyFolder*
modest_tny_send_queue_get_sentbox (TnySendQueue *self)
{
	return MODEST_TNY_SEND_QUEUE_GET_CLASS(self)->get_sentbox_func (self);
}


static TnyFolder*
modest_tny_send_queue_get_outbox (TnySendQueue *self)
{
	return MODEST_TNY_SEND_QUEUE_GET_CLASS(self)->get_outbox_func (self);
}




static void
modest_tny_send_queue_cancel_default (TnySendQueue *self, gboolean remove)
{
	/* FIXME */
}


static void
modest_tny_send_queue_add_default (TnySendQueue *self, TnyMsg *msg)
{
	ModestTnySendQueuePrivate *priv; 
	TnyFolder *outbox;
	
	g_return_if_fail (self);
	g_return_if_fail (TNY_IS_CAMEL_MSG(msg));
	
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	outbox = tny_send_queue_get_outbox (self);
	tny_folder_add_msg (outbox, msg, NULL);

	priv->flush_outbox_thread = g_thread_create (
		(GThreadFunc)flush_outbox_thread,
		self, FALSE, NULL);
}

static TnyFolder*
modest_tny_send_queue_get_sentbox_default (TnySendQueue *self)
{
	ModestTnySendQueuePrivate *priv; 

	g_return_val_if_fail (self, NULL);

	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);
	
	return  modest_tny_account_get_special_folder (TNY_ACCOUNT(priv->account),
						       TNY_FOLDER_TYPE_SENT);
}


static TnyFolder*
modest_tny_send_queue_get_outbox_default (TnySendQueue *self)
{
	ModestTnySendQueuePrivate *priv; 

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	return modest_tny_account_get_special_folder (TNY_ACCOUNT(priv->account),
						      TNY_FOLDER_TYPE_OUTBOX);
}


GType
modest_tny_send_queue_get_type (void)
{
	static GType my_type = 0;

	if (G_UNLIKELY (!camel_type_init_done)) {
		if (!g_thread_supported ()) 
			g_thread_init (NULL);
		camel_type_init ();
		camel_type_init_done = TRUE;
	}
	
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnySendQueueClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_send_queue_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnySendQueue),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_send_queue_instance_init,
			NULL
		};

		static const GInterfaceInfo tny_send_queue_info = {
			(GInterfaceInitFunc) modest_tny_send_queue_init,
			/* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
		my_type = g_type_register_static (G_TYPE_OBJECT,
						  "ModestTnySendQueue",
		                                  &my_info, 0);
		g_type_add_interface_static (my_type, TNY_TYPE_SEND_QUEUE,
					     &tny_send_queue_info);
	}
	
	return my_type;
}


static void
modest_tny_send_queue_init (gpointer g, gpointer iface_data)
{
	TnySendQueueIface *klass = (TnySendQueueIface*)g;
	
	klass->add_func         = modest_tny_send_queue_add;
	klass->get_outbox_func  = modest_tny_send_queue_get_outbox;
        klass->get_sentbox_func = modest_tny_send_queue_get_sentbox;
        klass->cancel_func      = modest_tny_send_queue_cancel;
}



static void
modest_tny_send_queue_class_init (ModestTnySendQueueClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	
	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_send_queue_finalize;

	klass->add_func         = modest_tny_send_queue_add_default;
	klass->get_outbox_func  = modest_tny_send_queue_get_outbox_default;
        klass->get_sentbox_func = modest_tny_send_queue_get_sentbox_default;
        klass->cancel_func      = modest_tny_send_queue_cancel_default;

	g_type_class_add_private (gobject_class, sizeof(ModestTnySendQueuePrivate));
}

static void
modest_tny_send_queue_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestTnySendQueue *self;  
        ModestTnySendQueuePrivate *priv; 

	self = (ModestTnySendQueue*)instance;
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	priv->flush_lock          = g_mutex_new ();
	priv->flush_outbox_thread = NULL;
	priv->account = NULL;
}

static void
modest_tny_send_queue_finalize (GObject *obj)
{
	ModestTnySendQueuePrivate *priv; 
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (obj);
	
	if (priv->account) {
		g_object_unref (priv->account);
		priv->account = NULL;
	}

	if (priv->flush_lock) {
		g_mutex_free (priv->flush_lock);
		priv->flush_lock = NULL;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestTnySendQueue*
modest_tny_send_queue_new (TnyTransportAccount *account)
{
	ModestTnySendQueue *self;
	ModestTnySendQueuePrivate *priv;

	g_return_val_if_fail (account, NULL);
	
	self = MODEST_TNY_SEND_QUEUE(g_object_new(MODEST_TYPE_TNY_SEND_QUEUE, NULL));
	priv = MODEST_TNY_SEND_QUEUE_GET_PRIVATE (self);

	priv->account = account;
	g_object_ref (G_OBJECT(priv->account));
	
	return self;
}


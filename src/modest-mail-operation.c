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

#include <string.h>
#include <stdarg.h>
#include <tny-mime-part.h>
#include <tny-store-account.h>
#include <tny-folder-store.h>
#include <tny-folder-store-query.h>
#include <tny-camel-stream.h>
#include <tny-camel-pop-store-account.h>
#include <tny-camel-pop-folder.h>
#include <tny-camel-imap-folder.h>
#include <tny-camel-mem-stream.h>
#include <tny-simple-list.h>
#include <tny-send-queue.h>
#include <tny-status.h>
#include <tny-error.h>
#include <tny-folder-observer.h>
#include <camel/camel-stream-mem.h>
#include <glib/gi18n.h>
#include <modest-defs.h>
#include "modest-platform.h"
#include "modest-account-mgr-helpers.h"
#include <modest-tny-account.h>
#include <modest-tny-send-queue.h>
#include <modest-runtime.h>
#include "modest-text-utils.h"
#include "modest-tny-msg.h"
#include "modest-tny-folder.h"
#include "modest-tny-account-store.h"
#include "modest-tny-platform-factory.h"
#include "modest-marshal.h"
#include "modest-error.h"
#include "modest-mail-operation.h"
#include <modest-count-stream.h>
#include <libgnomevfs/gnome-vfs.h>
#include "modest-utils.h"
#include "modest-debug.h"
#ifdef MODEST_USE_LIBTIME
#include <clockd/libtime.h>
#endif
#include "modest-account-protocol.h"
#include <camel/camel-stream-null.h>
#include <widgets/modest-msg-view-window.h>

#define KB 1024

/* 
 * Remove all these #ifdef stuff when the tinymail's idle calls become
 * locked
 */
#define TINYMAIL_IDLES_NOT_LOCKED_YET 1

/* 'private'/'protected' functions */
static void modest_mail_operation_class_init (ModestMailOperationClass *klass);
static void modest_mail_operation_init       (ModestMailOperation *obj);
static void modest_mail_operation_finalize   (GObject *obj);

static void     get_msg_async_cb (TnyFolder *folder, 
				  gboolean cancelled, 
				  TnyMsg *msg, 
				  GError *rr, 
				  gpointer user_data);

static void     get_msg_status_cb (GObject *obj,
				   TnyStatus *status,  
				   gpointer user_data);

static void     modest_mail_operation_notify_start (ModestMailOperation *self);
static void     modest_mail_operation_notify_end (ModestMailOperation *self);

static void     notify_progress_of_multiple_messages (ModestMailOperation *self,
						      TnyStatus *status,
						      gint *last_total_bytes,
						      gint *sum_total_bytes,
						      gint total_bytes,
						      gboolean increment_done);

static guint    compute_message_list_size (TnyList *headers, guint num_elements);

static int      compare_headers_by_date   (gconstpointer a,
					   gconstpointer b);

static void     sync_folder_finish_callback (TnyFolder *self, 
					     gboolean cancelled, 
					     GError *err, 
					     gpointer user_data);

static gboolean _check_memory_low         (ModestMailOperation *mail_op);


typedef struct {
	ModestTnySendQueue *queue;
	ModestMailOperation *self;
	guint error_handler;
	guint start_handler;
	guint stop_handler;
} RunQueueHelper;

static void run_queue_notify_and_destroy (RunQueueHelper *helper,
					  ModestMailOperationStatus status);

/* Helpers for the update account operation (send & receive)*/
typedef struct 
{
	ModestMailOperation *mail_op;
	gchar *account_name;
	UpdateAccountCallback callback;
	gpointer user_data;
	TnyList *folders;
	gint pending_calls;
	gboolean poke_all;
	TnyFolderObserver *inbox_observer;
	gboolean interactive;
	gboolean msg_readed;
} UpdateAccountInfo;

static void destroy_update_account_info         (UpdateAccountInfo *info);

static void update_account_send_mail            (UpdateAccountInfo *info);

static void update_account_get_msg_async_cb     (TnyFolder *folder, 
						 gboolean canceled, 
						 TnyMsg *msg, 
						 GError *err, 
						 gpointer user_data);

static void update_account_notify_user_and_free (UpdateAccountInfo *info, 
						 TnyList *new_headers);

enum _ModestMailOperationSignals 
{
	PROGRESS_CHANGED_SIGNAL,
	OPERATION_STARTED_SIGNAL,
	OPERATION_FINISHED_SIGNAL,
	NUM_SIGNALS
};

typedef struct _ModestMailOperationPrivate ModestMailOperationPrivate;
struct _ModestMailOperationPrivate {
	TnyAccount                *account;
	guint                      done;
	guint                      total;
	GObject                   *source;
	GError                    *error;
	ErrorCheckingUserCallback  error_checking;
	gpointer                   error_checking_user_data;
	ErrorCheckingUserDataDestroyer error_checking_user_data_destroyer;
	ModestMailOperationStatus  status;	
	ModestMailOperationTypeOperation op_type;
};

#define MODEST_MAIL_OPERATION_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_MAIL_OPERATION, \
                                                   ModestMailOperationPrivate))

#define CHECK_EXCEPTION(priv, new_status)  if (priv->error) {\
                                                   priv->status = new_status;\
                                               }


typedef struct {
	GetMsgAsyncUserCallback user_callback;
	TnyHeader *header;
	TnyIterator *more_msgs;
	gpointer user_data;
	ModestMailOperation *mail_op;
	GDestroyNotify destroy_notify;
	gint last_total_bytes;
	gint sum_total_bytes;
	gint total_bytes;
	TnyIterator *get_parts;
	TnyMsg *msg;
} GetMsgInfo;

typedef struct _RefreshAsyncHelper {	
	ModestMailOperation *mail_op;
	RefreshAsyncUserCallback user_callback;	
	gpointer user_data;
} RefreshAsyncHelper;

typedef struct _XFerMsgsAsyncHelper
{
	ModestMailOperation *mail_op;
	TnyList *headers;
	TnyIterator *more_msgs;
	TnyFolder *dest_folder;
	XferMsgsAsyncUserCallback user_callback;	
	gboolean delete;
	gpointer user_data;
	gint last_total_bytes;
	gint sum_total_bytes;
	gint total_bytes;
} XFerMsgsAsyncHelper;

typedef struct _XFerFolderAsyncHelper
{
	ModestMailOperation *mail_op;
	XferFolderAsyncUserCallback user_callback;	
	gpointer user_data;
} XFerFolderAsyncHelper;

typedef struct _SyncFolderHelper {
	ModestMailOperation *mail_op;
	SyncFolderCallback user_callback;
	gpointer user_data;
} SyncFolderHelper;

typedef void (*ModestMailOperationCreateMsgCallback) (ModestMailOperation *mail_op,
						      TnyMsg *msg,
						      gpointer userdata);

static void          modest_mail_operation_create_msg (ModestMailOperation *self,
						       const gchar *from, const gchar *to,
						       const gchar *cc, const gchar *bcc,
						       const gchar *subject, const gchar *plain_body,
						       const gchar *html_body, const GList *attachments_list,
						       const GList *images_list,
						       TnyHeaderFlags priority_flags,
						       const gchar *references, const gchar *in_reply_to,
						       ModestMailOperationCreateMsgCallback callback,
						       gpointer userdata);

static gboolean      idle_notify_queue (gpointer data);
typedef struct
{
	ModestMailOperation *mail_op;
	gchar *from;
	gchar *to;
	gchar *cc;
	gchar *bcc;
	gchar *subject;
	gchar *references;
	gchar *in_reply_to;
	gchar *plain_body;
	gchar *html_body;
	GList *attachments_list;
	GList *images_list;
	TnyHeaderFlags priority_flags;
	ModestMailOperationCreateMsgCallback callback;
	gpointer userdata;
} CreateMsgInfo;

typedef struct
{
	ModestMailOperation *mail_op;
	TnyMsg *msg;
	ModestMailOperationCreateMsgCallback callback;
	gpointer userdata;
} CreateMsgIdleInfo;

/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[NUM_SIGNALS] = {0};

GType
modest_mail_operation_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMailOperationClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_mail_operation_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMailOperation),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_mail_operation_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestMailOperation",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_mail_operation_class_init (ModestMailOperationClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_mail_operation_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMailOperationPrivate));

	/**
	 * ModestMailOperation::progress-changed
	 * @self: the #MailOperation that emits the signal
	 * @user_data: user data set when the signal handler was connected
	 *
	 * Emitted when the progress of a mail operation changes
	 */
 	signals[PROGRESS_CHANGED_SIGNAL] = 
		g_signal_new ("progress-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestMailOperationClass, progress_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	/**
	 * operation-started
	 *
	 * This signal is issued whenever a mail operation starts, and
	 * starts mean when the tinymail operation is issued. This
	 * means that it could happen that something wrong happens and
	 * the tinymail function is never called. In this situation a
	 * operation-finished will be issued without any
	 * operation-started
	 */
	signals[OPERATION_STARTED_SIGNAL] =
		g_signal_new ("operation-started",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestMailOperationClass, operation_started),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	/**
	 * operation-started
	 *
	 * This signal is issued whenever a mail operation
	 * finishes. Note that this signal could be issued without any
	 * previous "operation-started" signal, because this last one
	 * is only issued when the tinymail operation is successfully
	 * started
	 */
	signals[OPERATION_FINISHED_SIGNAL] =
		g_signal_new ("operation-finished",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestMailOperationClass, operation_finished),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
modest_mail_operation_init (ModestMailOperation *obj)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);

	priv->account        = NULL;
	priv->status         = MODEST_MAIL_OPERATION_STATUS_INVALID;
	priv->op_type        = MODEST_MAIL_OPERATION_TYPE_UNKNOWN;
	priv->error          = NULL;
	priv->done           = 0;
	priv->total          = 0;
	priv->source         = NULL;
	priv->error_checking = NULL;
	priv->error_checking_user_data = NULL;
}

static void
modest_mail_operation_finalize (GObject *obj)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);

	
	
	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}
	if (priv->source) {
		g_object_unref (priv->source);
		priv->source = NULL;
	}
	if (priv->account) {
		g_object_unref (priv->account);
		priv->account = NULL;
	}


	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestMailOperation*
modest_mail_operation_new (GObject *source)
{
	ModestMailOperation *obj;
	ModestMailOperationPrivate *priv;
		
	obj = MODEST_MAIL_OPERATION(g_object_new(MODEST_TYPE_MAIL_OPERATION, NULL));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);

	if (source != NULL)
		priv->source = g_object_ref(source);

	return obj;
}

ModestMailOperation*
modest_mail_operation_new_with_error_handling (GObject *source,
					       ErrorCheckingUserCallback error_handler,
					       gpointer user_data,
					       ErrorCheckingUserDataDestroyer error_handler_destroyer)
{
	ModestMailOperation *obj;
	ModestMailOperationPrivate *priv;
		
	obj = modest_mail_operation_new (source);
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);
	
	g_return_val_if_fail (error_handler != NULL, obj);
	priv->error_checking = error_handler;
	priv->error_checking_user_data = user_data;
	priv->error_checking_user_data_destroyer = error_handler_destroyer;

	return obj;
}

void
modest_mail_operation_execute_error_handler (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_if_fail (self && MODEST_IS_MAIL_OPERATION(self));
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	g_return_if_fail(priv->status != MODEST_MAIL_OPERATION_STATUS_SUCCESS);	    

	/* Call the user callback */
	if (priv->error_checking != NULL)
		priv->error_checking (self, priv->error_checking_user_data);
}


ModestMailOperationTypeOperation
modest_mail_operation_get_type_operation (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_MAIL_OPERATION(self),
			      MODEST_MAIL_OPERATION_TYPE_UNKNOWN);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	
	return priv->op_type;
}

gboolean 
modest_mail_operation_is_mine (ModestMailOperation *self, 
			       GObject *me)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_MAIL_OPERATION(self),
			      FALSE);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	if (priv->source == NULL) return FALSE;

	return priv->source == me;
}

GObject *
modest_mail_operation_get_source (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_MAIL_OPERATION(self),
			      NULL);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	if (!priv) {
		g_warning ("BUG: %s: priv == NULL", __FUNCTION__);
		return NULL;
	}
	
	return (priv->source) ? g_object_ref (priv->source) : NULL;
}

ModestMailOperationStatus
modest_mail_operation_get_status (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self, MODEST_MAIL_OPERATION_STATUS_INVALID);
	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self),
			      MODEST_MAIL_OPERATION_STATUS_INVALID);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	if (!priv) {
		g_warning ("BUG: %s: priv == NULL", __FUNCTION__);
		return MODEST_MAIL_OPERATION_STATUS_INVALID;
	}
	
	return priv->status;
}

const GError *
modest_mail_operation_get_error (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), NULL);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (!priv) {
		g_warning ("BUG: %s: priv == NULL", __FUNCTION__);
		return NULL;
	}

	return priv->error;
}

gboolean 
modest_mail_operation_cancel (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;
	gboolean canceled = FALSE;
	
	g_return_val_if_fail (self && MODEST_IS_MAIL_OPERATION (self), FALSE);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* Set new status */
	priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
	
	/* Cancel the mail operation */
	g_return_val_if_fail (priv->account, FALSE);
	tny_account_cancel (priv->account);

	if (priv->op_type == MODEST_MAIL_OPERATION_TYPE_SEND) {
		ModestTnySendQueue *queue;
		queue = modest_runtime_get_send_queue (TNY_TRANSPORT_ACCOUNT (priv->account),
						       TRUE);

		/* Cancel the sending of the following next messages */
		if (TNY_IS_SEND_QUEUE (queue))
			tny_send_queue_cancel (TNY_SEND_QUEUE (queue), TNY_SEND_QUEUE_CANCEL_ACTION_SUSPEND, NULL);
	}
	
	return canceled;
}

guint 
modest_mail_operation_get_task_done (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_MAIL_OPERATION(self),
			      0);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	return priv->done;
}

guint 
modest_mail_operation_get_task_total (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_MAIL_OPERATION(self),
			      0);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	return priv->total;
}

gboolean
modest_mail_operation_is_finished (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;
	gboolean retval = FALSE;

	g_return_val_if_fail (self && MODEST_IS_MAIL_OPERATION(self),
			      FALSE);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (priv->status == MODEST_MAIL_OPERATION_STATUS_SUCCESS   ||
	    priv->status == MODEST_MAIL_OPERATION_STATUS_FAILED    ||
	    priv->status == MODEST_MAIL_OPERATION_STATUS_CANCELED  ||
	    priv->status == MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS) {
		retval = TRUE;
	} else {
		retval = FALSE;
	}

	return retval;
}

/*
 * Creates an image of the current state of a mail operation, the
 * caller must free it
 */
static ModestMailOperationState *
modest_mail_operation_clone_state (ModestMailOperation *self)
{
	ModestMailOperationState *state;
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	g_return_val_if_fail (priv, NULL);

	if (!priv)
		return NULL;

	state = g_slice_new (ModestMailOperationState);

	state->status = priv->status;
	state->op_type = priv->op_type;
	state->done = priv->done;
	state->total = priv->total;
	state->finished = modest_mail_operation_is_finished (self);
	state->bytes_done = 0;
	state->bytes_total = 0;

	return state;
}

/* ******************************************************************* */
/* ************************** SEND   ACTIONS ************************* */
/* ******************************************************************* */

typedef struct 
{
	ModestMailOperation *mail_op;
	gboolean notify;
} SendNewMailHelper;

static void
send_mail_on_sync_async_cb (TnyFolder *folder, 
			    gboolean cancelled, 
			    GError *err, 
			    gpointer user_data)
{
	ModestMailOperationPrivate *priv;
	ModestMailOperation *self;
	SendNewMailHelper *helper;

	helper = (SendNewMailHelper *) user_data;
	self = helper->mail_op;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (cancelled)
		goto end;

	if (err) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_SEND_QUEUE_ADD_ERROR,
			     "Error adding a msg to the send queue\n");
		priv->status = MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS;
	} else {
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

 end:
	if (helper->notify)
		modest_mail_operation_notify_end (self);

	g_object_unref (helper->mail_op);
	g_slice_free (SendNewMailHelper, helper);
}

static void
run_queue_start (TnySendQueue *self,
		 gpointer user_data)
{
	RunQueueHelper *helper = (RunQueueHelper *) user_data;
	ModestMailOperation *mail_op;
			
	g_debug ("%s sending queue successfully started", __FUNCTION__);

	/* Wait for the message to be sent */
	mail_op = modest_mail_operation_new (NULL);
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
					 mail_op);
	modest_mail_operation_run_queue (mail_op, helper->queue);
	g_object_unref (mail_op);

	/* Free the helper and end operation */
	run_queue_notify_and_destroy (helper, MODEST_MAIL_OPERATION_STATUS_SUCCESS);
}

static void 
run_queue_error_happened (TnySendQueue *queue, 
			  TnyHeader *header, 
			  TnyMsg *msg, 
			  GError *error, 
			  gpointer user_data)
{
	RunQueueHelper *helper = (RunQueueHelper *) user_data;
	ModestMailOperationPrivate *priv;

	/* If we are here this means that the send queue could not
	   start to send emails. Shouldn't happen as this means that
	   we could not create the thread */
	g_debug ("%s sending queue failed to create the thread", __FUNCTION__);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (helper->self);
	priv->error = g_error_copy ((const GError *) error);

	if (error->code != TNY_SYSTEM_ERROR_UNKNOWN) {
		/* This code is here for safety reasons. It should
		   never be called, because that would mean that we
		   are not controlling some error case */
		g_warning ("%s Error %s should not happen", 
			   __FUNCTION__, error->message);
	}

	/* Free helper and end operation */
	run_queue_notify_and_destroy (helper, MODEST_MAIL_OPERATION_STATUS_FAILED);
}

static void
send_mail_on_added_to_outbox (TnySendQueue *send_queue, 
			      gboolean cancelled, 
			      TnyMsg *msg, 
			      GError *err,
			      gpointer user_data)
{
	ModestMailOperationPrivate *priv;
	ModestMailOperation *self;
	SendNewMailHelper *helper;

	helper = (SendNewMailHelper *) user_data;
	self = helper->mail_op;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (cancelled)
		goto end;

	if (err) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_SEND_QUEUE_ADD_ERROR,
			     "Error adding a msg to the send queue\n");
		priv->status = MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS;
	} else {
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

 end:
	if (helper->notify) {
		TnyTransportAccount *trans_account;
		ModestTnySendQueue *queue;

		trans_account = (TnyTransportAccount *) modest_mail_operation_get_account (self);
		if (trans_account) {
			queue = modest_runtime_get_send_queue (trans_account, TRUE);
			if (queue) {
				RunQueueHelper *helper;

				/* Create the helper */
				helper = g_slice_new0 (RunQueueHelper);
				helper->queue = g_object_ref (queue);
				helper->self = g_object_ref (self);

				/* if sending is ongoing wait for the queue to
				   stop. Otherwise wait for the queue-start
				   signal. It could happen that the queue
				   could not start, then check also the error
				   happened signal */
				if (modest_tny_send_queue_sending_in_progress (queue)) {
					run_queue_start (TNY_SEND_QUEUE (queue), helper);
				} else {
					helper->start_handler = g_signal_connect (queue, "queue-start", 
										  G_CALLBACK (run_queue_start), 
										  helper);
					helper->error_handler = g_signal_connect (queue, "error-happened", 
										  G_CALLBACK (run_queue_error_happened), 
										  helper);
				}
			} else {
				/* Finalize this mail operation */
				modest_mail_operation_notify_end (self);
			}
			g_object_unref (trans_account);
		} else {
			g_warning ("No transport account for the operation");
		}
	}

	g_object_unref (helper->mail_op);
	g_slice_free (SendNewMailHelper, helper);
}

static gboolean
idle_create_msg_cb (gpointer idle_data)
{
	CreateMsgIdleInfo *info = (CreateMsgIdleInfo *) idle_data;

	/* This is a GDK lock because we are an idle callback and
 	 * info->callback can contain Gtk+ code */

	gdk_threads_enter (); /* CHECKED */
	info->callback (info->mail_op, info->msg, info->userdata);

	g_object_unref (info->mail_op);
	if (info->msg)
		g_object_unref (info->msg);
	g_slice_free (CreateMsgIdleInfo, info);
	gdk_threads_leave (); /* CHECKED */

	return FALSE;
}

static gpointer 
create_msg_thread (gpointer thread_data)
{
	CreateMsgInfo *info = (CreateMsgInfo *) thread_data;
	TnyMsg *new_msg = NULL;
	ModestMailOperationPrivate *priv;
	gint attached = 0;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(info->mail_op);
	if (info->html_body == NULL) {
		new_msg = modest_tny_msg_new (info->to, info->from, info->cc, 
					      info->bcc, info->subject, 
					      info->references, info->in_reply_to,
					      info->plain_body, 
					      info->attachments_list, &attached,
					      &(priv->error));
	} else {
		new_msg = modest_tny_msg_new_html_plain (info->to, info->from, info->cc,
							 info->bcc, info->subject, 
							 info->references, info->in_reply_to,
							 info->html_body,
							 info->plain_body, info->attachments_list,
							 info->images_list, &attached,
							 &(priv->error));
	}

	if (new_msg) {
		TnyHeader *header;

		/* Set priority flags in message */
		header = tny_msg_get_header (new_msg);
		tny_header_set_flag (header, info->priority_flags);

		/* Set attachment flags in message */
		if (info->attachments_list != NULL && attached > 0)
			tny_header_set_flag (header, TNY_HEADER_FLAG_ATTACHMENTS);

		g_object_unref (G_OBJECT(header));
	} else {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		if (!priv->error)
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_INSTANCE_CREATION_FAILED,
				     "modest: failed to create a new msg\n");
	}


	g_free (info->to);
	g_free (info->from);
	g_free (info->cc);
	g_free (info->bcc);
	g_free (info->plain_body);
	g_free (info->html_body);
	g_free (info->subject);
	g_free (info->references);
	g_free (info->in_reply_to);
	g_list_foreach (info->attachments_list, (GFunc) g_object_unref, NULL);
	g_list_free (info->attachments_list);
	g_list_foreach (info->images_list, (GFunc) g_object_unref, NULL);
	g_list_free (info->images_list);

	if (info->callback) {
		CreateMsgIdleInfo *idle_info;
		idle_info = g_slice_new0 (CreateMsgIdleInfo);
		idle_info->mail_op = g_object_ref (info->mail_op);
		idle_info->msg = (new_msg) ? g_object_ref (new_msg) : NULL;
		idle_info->callback = info->callback;
		idle_info->userdata = info->userdata;
		g_idle_add (idle_create_msg_cb, idle_info);
	} else {
		g_idle_add (idle_notify_queue, g_object_ref (info->mail_op));
	}

	g_object_unref (info->mail_op);
	g_slice_free (CreateMsgInfo, info);
	if (new_msg) g_object_unref(new_msg);
	return NULL;
}


void
modest_mail_operation_create_msg (ModestMailOperation *self,
				  const gchar *from, const gchar *to,
				  const gchar *cc, const gchar *bcc,
				  const gchar *subject, const gchar *plain_body,
				  const gchar *html_body,
				  const GList *attachments_list,
				  const GList *images_list,
				  TnyHeaderFlags priority_flags,
				  const gchar *references,
				  const gchar *in_reply_to,
				  ModestMailOperationCreateMsgCallback callback,
				  gpointer userdata)
{
	CreateMsgInfo *info = NULL;

	info = g_slice_new0 (CreateMsgInfo);
	info->mail_op = g_object_ref (self);

	info->from = g_strdup (from);
	info->to = g_strdup (to);
	info->cc = g_strdup (cc);
	info->bcc  = g_strdup (bcc);
	info->subject = g_strdup (subject);
	info->plain_body = g_strdup (plain_body);
	info->html_body = g_strdup (html_body);
	info->references = g_strdup (references);
	info->in_reply_to = g_strdup (in_reply_to);
	info->attachments_list = g_list_copy ((GList *) attachments_list);
	g_list_foreach (info->attachments_list, (GFunc) g_object_ref, NULL);
	info->images_list = g_list_copy ((GList *) images_list);
	g_list_foreach (info->images_list, (GFunc) g_object_ref, NULL);
	info->priority_flags = priority_flags;

	info->callback = callback;
	info->userdata = userdata;

	g_thread_create (create_msg_thread, info, FALSE, NULL);
}

typedef struct
{
	TnyTransportAccount *transport_account;
	TnyMsg *draft_msg;
} SendNewMailInfo;

static void
modest_mail_operation_send_new_mail_cb (ModestMailOperation *self,
					TnyMsg *msg,
					gpointer userdata)
{
	TnySendQueue *send_queue = NULL;
	ModestMailOperationPrivate *priv = NULL;
	SendNewMailInfo *info = (SendNewMailInfo *) userdata;
	TnyFolder *draft_folder = NULL;
	TnyFolder *outbox_folder = NULL;
	TnyHeader *header = NULL;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (!msg) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		modest_mail_operation_notify_end (self);
		goto end;
	}

	if (priv->error && priv->error->code != MODEST_MAIL_OPERATION_ERROR_FILE_IO) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		modest_mail_operation_notify_end (self);
		goto end;
	}

	/* Add message to send queue */
	send_queue = TNY_SEND_QUEUE (modest_runtime_get_send_queue (info->transport_account, TRUE));
	if (!TNY_IS_SEND_QUEUE(send_queue)) {
		if (priv->error) {
			g_error_free (priv->error);
			priv->error = NULL;
		}
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
			     "modest: could not find send queue for account\n");
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		modest_mail_operation_notify_end (self);
		goto end;
	} else {
		SendNewMailHelper *helper = g_slice_new (SendNewMailHelper);
		helper->mail_op = g_object_ref (self);
		helper->notify = (info->draft_msg == NULL);

		/* Add the msg to the queue. The callback will free
		   the helper */
		modest_tny_send_queue_set_requested_send_receive (MODEST_TNY_SEND_QUEUE (send_queue), 
								  FALSE);
		tny_send_queue_add_async (send_queue, msg, send_mail_on_added_to_outbox, 
					  NULL, helper);
	}

	if (info->draft_msg != NULL) {
		TnyList *tmp_headers = NULL;
		TnyFolder *folder = NULL;
		TnyFolder *src_folder = NULL;
		TnyFolderType folder_type;		
		TnyTransportAccount *transport_account = NULL;
		SendNewMailHelper *helper = NULL;

		/* To remove the old mail from its source folder, we need to get the
		 * transport account of the original draft message (the transport account
		 * might have been changed by the user) */
		header = tny_msg_get_header (info->draft_msg);
		transport_account = modest_tny_account_store_get_transport_account_from_outbox_header(
			modest_runtime_get_account_store(), header);
		if (transport_account == NULL)
			transport_account = g_object_ref(info->transport_account);
		draft_folder = modest_tny_account_get_special_folder (TNY_ACCOUNT (transport_account),
								      TNY_FOLDER_TYPE_DRAFTS);
		outbox_folder = modest_tny_account_get_special_folder (TNY_ACCOUNT (transport_account),
								       TNY_FOLDER_TYPE_OUTBOX);
		g_object_unref(transport_account);

		if (!draft_folder) {
			g_warning ("%s: modest_tny_account_get_special_folder(..) returned a NULL drafts folder",
				   __FUNCTION__);
			modest_mail_operation_notify_end (self);
			goto end;
		}
		if (!outbox_folder) {
			g_warning ("%s: modest_tny_account_get_special_folder(..) returned a NULL outbox folder",
				   __FUNCTION__);
			modest_mail_operation_notify_end (self);
			goto end;
		}

		folder = tny_msg_get_folder (info->draft_msg);		
		if (folder == NULL) {
			modest_mail_operation_notify_end (self);
			goto end;
		}
		folder_type = modest_tny_folder_guess_folder_type (folder);

		if (folder_type == TNY_FOLDER_TYPE_INVALID)
			g_warning ("%s: BUG: folder of type TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
		
		if (folder_type == TNY_FOLDER_TYPE_OUTBOX) 
			src_folder = outbox_folder;
		else 
			src_folder = draft_folder;

		/* Note: This can fail (with a warning) if the message is not really already in a folder,
		 * because this function requires it to have a UID. */
		helper = g_slice_new (SendNewMailHelper);
		helper->mail_op = g_object_ref (self);
		helper->notify = TRUE;

		tmp_headers = tny_simple_list_new ();
		tny_list_append (tmp_headers, (GObject*) header);
		tny_folder_remove_msgs_async (src_folder, tmp_headers, NULL, NULL, NULL);
		g_object_unref (tmp_headers);
		tny_folder_sync_async (src_folder, TRUE, send_mail_on_sync_async_cb, 
				       NULL, helper);
		g_object_unref (folder);
	}

end:
	if (header)
		g_object_unref (header);
	if (info->draft_msg)
		g_object_unref (info->draft_msg);
	if (draft_folder)
		g_object_unref (draft_folder);
	if (outbox_folder)
		g_object_unref (outbox_folder);
	if (info->transport_account)
		g_object_unref (info->transport_account);
	g_slice_free (SendNewMailInfo, info);
}

void
modest_mail_operation_send_new_mail (ModestMailOperation *self,
				     TnyTransportAccount *transport_account,
				     TnyMsg *draft_msg,
				     const gchar *from,  const gchar *to,
				     const gchar *cc,  const gchar *bcc,
				     const gchar *subject, const gchar *plain_body,
				     const gchar *html_body,
				     const GList *attachments_list,
				     const GList *images_list,
				     const gchar *references,
				     const gchar *in_reply_to,
				     TnyHeaderFlags priority_flags)
{
	ModestMailOperationPrivate *priv = NULL;
	SendNewMailInfo *info;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_SEND;
	priv->account = TNY_ACCOUNT (g_object_ref (transport_account));
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	modest_mail_operation_notify_start (self);

	/* Check parametters */
	if (to == NULL) {
 		/* Set status failed and set an error */
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_BAD_PARAMETER,
			     _("Error trying to send a mail. You need to set at least one recipient"));
		modest_mail_operation_notify_end (self);
		return;
	}
	info = g_slice_new0 (SendNewMailInfo);
	info->transport_account = transport_account;
	if (transport_account)
		g_object_ref (transport_account);
	info->draft_msg = draft_msg;
	if (draft_msg)
		g_object_ref (draft_msg);


	modest_mail_operation_create_msg (self, from, to, cc, bcc, subject, plain_body, html_body,
					  attachments_list, images_list, priority_flags,
					  references, in_reply_to,
					  modest_mail_operation_send_new_mail_cb, info);

}

typedef struct
{
	ModestMailOperation *mailop;
	TnyMsg *msg;
	SaveToDraftstCallback callback;
	gpointer userdata;
} FinishSaveRemoteDraftInfo;

static void
finish_save_remote_draft (ModestAccountProtocol *protocol,
			  GError *err,
			  const gchar *account_id,
			  TnyMsg *new_remote_msg,
			  TnyMsg *new_msg,
			  TnyMsg *old_msg,
			  gpointer userdata)
{
	FinishSaveRemoteDraftInfo *info = (FinishSaveRemoteDraftInfo *) userdata;
	ModestMailOperationPrivate *priv = NULL;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(info->mailop);

	if (!priv->error && err != NULL) {
		/* Priority for errors in save to local stage */
		priv->error = g_error_copy (err);
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
	}

	if (info->callback)
		info->callback (info->mailop, info->msg, info->userdata);

	if (info->msg)
		g_object_unref (info->msg);

	modest_mail_operation_notify_end (info->mailop);
	g_object_unref (info->mailop);

	g_slice_free (FinishSaveRemoteDraftInfo, info);
}

typedef struct
{
	TnyTransportAccount *transport_account;
	TnyMsg *draft_msg;
	SaveToDraftstCallback callback;
	gpointer user_data;
	TnyFolder *drafts;
	TnyMsg *msg;
	ModestMailOperation *mailop;
} SaveToDraftsAddMsgInfo;

static void
modest_mail_operation_save_to_drafts_add_msg_cb(TnyFolder *self,
						gboolean canceled,
						GError *err,
						gpointer userdata)
{
	ModestMailOperationPrivate *priv = NULL;
	SaveToDraftsAddMsgInfo *info = (SaveToDraftsAddMsgInfo *) userdata;
	GError *io_error = NULL;
	gboolean callback_called = FALSE;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(info->mailop);

	if (priv->error && priv->error->code == MODEST_MAIL_OPERATION_ERROR_FILE_IO) {
		io_error = priv->error;
		priv->error = NULL;
	}
	if (priv->error) {
		g_warning ("%s: priv->error != NULL", __FUNCTION__);
		g_error_free(priv->error);
	}

	priv->error = (err == NULL) ? NULL : g_error_copy(err);

	if ((!priv->error) && (info->draft_msg != NULL)) {
		TnyHeader *header = tny_msg_get_header (info->draft_msg);
		TnyFolder *src_folder = tny_header_get_folder (header);

		g_debug ("--- REMOVE AND SYNC");
		/* Remove the old draft */
		tny_folder_remove_msg (src_folder, header, NULL);

		/* Synchronize to expunge and to update the msg counts */
		tny_folder_sync_async (info->drafts, TRUE, NULL, NULL, NULL);
		tny_folder_sync_async (src_folder, TRUE, NULL, NULL, NULL);
		g_debug ("--- REMOVED - SYNCED");

		g_object_unref (G_OBJECT(header));
		g_object_unref (G_OBJECT(src_folder));
	}

	if (priv->error) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		if (io_error) {
			g_error_free (io_error);
			io_error = NULL;
		}
	} else if (io_error) {
		priv->error = io_error;
		priv->status = MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS;
	} else {
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

	if (info->transport_account) {
		ModestProtocolType transport_protocol_type;
		ModestProtocol *transport_protocol;

		transport_protocol_type = modest_tny_account_get_protocol_type (TNY_ACCOUNT (info->transport_account));

		transport_protocol = modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (),
										    transport_protocol_type);
		if (transport_protocol && MODEST_IS_ACCOUNT_PROTOCOL (transport_protocol)) {
			FinishSaveRemoteDraftInfo *srd_info = g_slice_new (FinishSaveRemoteDraftInfo);
			srd_info->mailop = info->mailop?g_object_ref (info->mailop):NULL;
			srd_info->msg = info->msg?g_object_ref (info->msg):NULL;
			srd_info->callback = info->callback;
			srd_info->userdata = info->user_data;
			modest_account_protocol_save_remote_draft (MODEST_ACCOUNT_PROTOCOL (transport_protocol), 
								   tny_account_get_id (TNY_ACCOUNT (info->transport_account)),
								   info->msg, info->draft_msg,
								   finish_save_remote_draft,
								   srd_info);
								   
			callback_called = TRUE;
		}
	}

	/* Call the user callback */
	if (!callback_called && info->callback)
		info->callback (info->mailop, info->msg, info->user_data);

	if (info->transport_account)
		g_object_unref (G_OBJECT(info->transport_account));
	if (info->draft_msg)
		g_object_unref (G_OBJECT (info->draft_msg));
	if (info->drafts)
		g_object_unref (G_OBJECT(info->drafts));
	if (info->msg)
		g_object_unref (G_OBJECT (info->msg));

	if (!callback_called)
		modest_mail_operation_notify_end (info->mailop);
        if (info->mailop)
		g_object_unref(info->mailop);
	g_slice_free (SaveToDraftsAddMsgInfo, info);
}

typedef struct
{
	TnyTransportAccount *transport_account;
	TnyMsg *draft_msg;
	SaveToDraftstCallback callback;
	gpointer user_data;
} SaveToDraftsInfo;

static void
modest_mail_operation_save_to_drafts_cb (ModestMailOperation *self,
					 TnyMsg *msg,
					 gpointer userdata)
{
	TnyFolder *drafts = NULL;
	ModestMailOperationPrivate *priv = NULL;
	SaveToDraftsInfo *info = (SaveToDraftsInfo *) userdata;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	if (!msg) {
		if (!(priv->error)) {
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_INSTANCE_CREATION_FAILED,
				     "modest: failed to create a new msg\n");
		}
	} else {
		drafts = modest_tny_account_get_special_folder (TNY_ACCOUNT (info->transport_account),
								TNY_FOLDER_TYPE_DRAFTS);
		if (!drafts && !(priv->error)) {
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
				     "modest: failed to create a new msg\n");
		}
	}

	if (!priv->error || priv->error->code == MODEST_MAIL_OPERATION_ERROR_FILE_IO) {
		if (drafts) {
			SaveToDraftsAddMsgInfo *cb_info = g_slice_new(SaveToDraftsAddMsgInfo);
			cb_info->transport_account = g_object_ref(info->transport_account);
			cb_info->draft_msg = info->draft_msg ? g_object_ref(info->draft_msg) : NULL;
			cb_info->callback = info->callback;
			cb_info->user_data = info->user_data;
			cb_info->drafts = g_object_ref(drafts);
			cb_info->msg = g_object_ref(msg);
			cb_info->mailop = g_object_ref(self);
			tny_folder_add_msg_async(drafts, msg, modest_mail_operation_save_to_drafts_add_msg_cb,
						 NULL, cb_info);
		}
	} else {
		/* Call the user callback */
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		if (info->callback)
			info->callback (self, msg, info->user_data);
		modest_mail_operation_notify_end (self);
	}

	if (drafts)
		g_object_unref (G_OBJECT(drafts));
	if (info->draft_msg)
		g_object_unref (G_OBJECT (info->draft_msg));
	if (info->transport_account)
		g_object_unref (G_OBJECT(info->transport_account));
	g_slice_free (SaveToDraftsInfo, info);
}

void
modest_mail_operation_save_to_drafts (ModestMailOperation *self,
				      TnyTransportAccount *transport_account,
				      TnyMsg *draft_msg,
				      const gchar *from,  const gchar *to,
				      const gchar *cc,  const gchar *bcc,
				      const gchar *subject, const gchar *plain_body,
				      const gchar *html_body,
				      const GList *attachments_list,
				      const GList *images_list,
				      TnyHeaderFlags priority_flags,
				      const gchar *references,
				      const gchar *in_reply_to,
				      SaveToDraftstCallback callback,
				      gpointer user_data)
{
	ModestMailOperationPrivate *priv = NULL;
	SaveToDraftsInfo *info = NULL;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Get account and set it into mail_operation */
	priv->account = g_object_ref (transport_account);
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_INFO;

	info = g_slice_new0 (SaveToDraftsInfo);
	info->transport_account = g_object_ref (transport_account);
	info->draft_msg = (draft_msg) ? g_object_ref (draft_msg) : NULL;
	info->callback = callback;
	info->user_data = user_data;

	g_debug ("--- CREATE MESSAGE");
	modest_mail_operation_notify_start (self);
	modest_mail_operation_create_msg (self, from, to, cc, bcc, subject, plain_body, html_body,
					  attachments_list, images_list, priority_flags,
					  references, in_reply_to,
					  modest_mail_operation_save_to_drafts_cb, info);
}

typedef struct
{
	ModestMailOperation *mail_op;
	TnyMimePart *mime_part;
	gssize size;
	GetMimePartSizeCallback callback;
	gpointer userdata;
} GetMimePartSizeInfo;

/***** I N T E R N A L    F O L D E R    O B S E R V E R *****/
/* We use this folder observer to track the headers that have been
 * added to a folder */
typedef struct {
	GObject parent;
	TnyList *new_headers;
} InternalFolderObserver;

typedef struct {
	GObjectClass parent;
} InternalFolderObserverClass;

static void tny_folder_observer_init (TnyFolderObserverIface *idace);

G_DEFINE_TYPE_WITH_CODE (InternalFolderObserver,
			 internal_folder_observer,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE(TNY_TYPE_FOLDER_OBSERVER, tny_folder_observer_init));


static void
foreach_add_item (gpointer header, gpointer user_data)
{
	tny_list_append (TNY_LIST (user_data), G_OBJECT (header));
}

/* This is the method that looks for new messages in a folder */
static void
internal_folder_observer_update (TnyFolderObserver *self, TnyFolderChange *change)
{
	InternalFolderObserver *derived = (InternalFolderObserver *)self;
	
	TnyFolderChangeChanged changed;

	changed = tny_folder_change_get_changed (change);

	if (changed & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS) {
		TnyList *list;

		/* Get added headers */
		list = tny_simple_list_new ();
		tny_folder_change_get_added_headers (change, list);

		/* Add them to the folder observer */
		tny_list_foreach (list, foreach_add_item, 
				  derived->new_headers);

		g_object_unref (G_OBJECT (list));
	}
}

static void
internal_folder_observer_init (InternalFolderObserver *self) 
{
	self->new_headers = tny_simple_list_new ();
}
static void
internal_folder_observer_finalize (GObject *object) 
{
	InternalFolderObserver *self;

	self = (InternalFolderObserver *) object;
	g_object_unref (self->new_headers);

	G_OBJECT_CLASS (internal_folder_observer_parent_class)->finalize (object);
}
static void
tny_folder_observer_init (TnyFolderObserverIface *iface) 
{
	iface->update = internal_folder_observer_update;
}
static void
internal_folder_observer_class_init (InternalFolderObserverClass *klass) 
{
	GObjectClass *object_class;

	internal_folder_observer_parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = internal_folder_observer_finalize;
}

static void
destroy_update_account_info (UpdateAccountInfo *info)
{
	g_free (info->account_name);
	g_object_unref (info->folders);
	g_object_unref (info->mail_op);
	g_slice_free (UpdateAccountInfo, info);
}


static void
update_account_send_mail (UpdateAccountInfo *info)
{
	TnyTransportAccount *transport_account = NULL;
	ModestTnyAccountStore *account_store;

	account_store = modest_runtime_get_account_store ();

	/* We don't try to send messages while sending mails is blocked */
	if (modest_tny_account_store_is_send_mail_blocked (account_store))
		return;

	/* Get the transport account */
	transport_account = (TnyTransportAccount *) 
		modest_tny_account_store_get_server_account (account_store, info->account_name, 
							     TNY_ACCOUNT_TYPE_TRANSPORT);

	if (transport_account) {
		ModestTnySendQueue *send_queue;
		TnyFolder *outbox;
		guint num_messages;

		send_queue = modest_runtime_get_send_queue (transport_account, TRUE);
		g_object_unref (transport_account);

		if (TNY_IS_SEND_QUEUE (send_queue)) {
			/* Get outbox folder */
			outbox = tny_send_queue_get_outbox (TNY_SEND_QUEUE (send_queue));
			if (outbox) { /* this could fail in some cases */
				num_messages = tny_folder_get_all_count (outbox);
				g_object_unref (outbox);
			} else {
				g_warning ("%s: could not get outbox", __FUNCTION__);
				num_messages = 0;
			}

			if (num_messages != 0) {
				ModestMailOperation *mail_op;
				/* Reenable suspended items */
				mail_op = modest_mail_operation_new (NULL);
				modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
								 mail_op);
				modest_mail_operation_queue_wakeup (mail_op, MODEST_TNY_SEND_QUEUE (send_queue));

				/* Try to send */
				modest_tny_send_queue_set_requested_send_receive (MODEST_TNY_SEND_QUEUE (send_queue), 
										  info->interactive);
			}
		}
	}
}

static void
update_account_get_msg_async_cb (TnyFolder *folder, 
				 gboolean canceled, 
				 TnyMsg *msg, 
				 GError *err, 
				 gpointer user_data)
{
	GetMsgInfo *msg_info = (GetMsgInfo *) user_data;
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (msg_info->mail_op);
	priv->done++;

	if (TNY_IS_MSG (msg)) {
		TnyHeader *header = tny_msg_get_header (msg);

		if (header) {
			ModestMailOperationState *state;
			state = modest_mail_operation_clone_state (msg_info->mail_op);
			msg_info->sum_total_bytes += tny_header_get_message_size (header);
			state->bytes_done = msg_info->sum_total_bytes;
			state->bytes_total = msg_info->total_bytes;

			/* Notify the status change. Only notify about changes
			   referred to bytes */
			g_signal_emit (G_OBJECT (msg_info->mail_op), 
				       signals[PROGRESS_CHANGED_SIGNAL], 
				       0, state, NULL);

			g_object_unref (header);
			g_slice_free (ModestMailOperationState, state);
		}
	}

	if (priv->done == priv->total) {
		TnyList *new_headers;
		UpdateAccountInfo *info;

		/* After getting all the messages send the ones in the
		   outboxes */
		info = (UpdateAccountInfo *) msg_info->user_data;
		update_account_send_mail (info);

		/* Check if the operation was a success */
		if (!priv->error)
			priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
		
		/* Call the user callback and free */
		new_headers = tny_iterator_get_list (msg_info->more_msgs);
		update_account_notify_user_and_free (info, new_headers);

		/* Delete the helper */
		if (msg_info->msg)
			g_object_unref (msg_info->msg);
		g_object_unref (msg_info->more_msgs);
		g_object_unref (msg_info->mail_op);
		g_slice_free (GetMsgInfo, msg_info);
	}
}

static void
update_account_notify_user_and_free (UpdateAccountInfo *info, 
				     TnyList *new_headers)
{
	/* Set the account back to not busy */
	modest_account_mgr_set_account_busy (modest_runtime_get_account_mgr (), 
					     info->account_name, FALSE);
	
	/* User callback */
	if (info->callback)
		info->callback (info->mail_op, new_headers, info->user_data);
	
	/* Mail operation end */
	modest_mail_operation_notify_end (info->mail_op);

	/* Frees */
	if (new_headers)
		g_object_unref (new_headers);
	destroy_update_account_info (info);
}

static void
inbox_refreshed_cb (TnyFolder *inbox, 
		    gboolean canceled, 
		    GError *err, 
		    gpointer user_data)
{	
	UpdateAccountInfo *info;
	ModestMailOperationPrivate *priv;
	TnyIterator *new_headers_iter;
	GPtrArray *new_headers_array = NULL;
	gint max_size, retrieve_limit, i;
	ModestAccountMgr *mgr;
	ModestAccountRetrieveType retrieve_type;
	TnyList *new_headers = NULL;
	gboolean headers_only;
	time_t time_to_store;

	info = (UpdateAccountInfo *) user_data;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (info->mail_op);
	mgr = modest_runtime_get_account_mgr ();

	if (canceled || err) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		if (err)
			priv->error = g_error_copy (err);
		else
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_OPERATION_CANCELED,
				     "canceled");

		if (inbox)
			tny_folder_remove_observer (inbox, info->inbox_observer);
		g_object_unref (info->inbox_observer);
		info->inbox_observer = NULL;

		/* Notify the user about the error and then exit */
		update_account_notify_user_and_free (info, NULL);
		return;
	}

	if (!inbox) {
		/* Try to send anyway */
		goto send_mail;
	}

	/* Set the last updated as the current time */
#ifdef MODEST_USE_LIBTIME
	struct tm utc_tm;
	time_get_utc (&utc_tm);
	time_to_store = time_mktime (&utc_tm, "GMT");
#else
	time_to_store = time (NULL);
#endif
	modest_account_mgr_set_last_updated (mgr, tny_account_get_id (priv->account), time_to_store);

	/* Get the message max size */
	max_size  = modest_conf_get_int (modest_runtime_get_conf (),
					 MODEST_CONF_MSG_SIZE_LIMIT, NULL);
	if (max_size == 0)
		max_size = G_MAXINT;
	else
		max_size = max_size * KB;

	/* Create the new headers array. We need it to sort the
	   new headers by date */
	new_headers_array = g_ptr_array_new ();
	if (info->inbox_observer) {
		new_headers_iter = tny_list_create_iterator (((InternalFolderObserver *) info->inbox_observer)->new_headers);
		while (!tny_iterator_is_done (new_headers_iter)) {
			TnyHeader *header = NULL;

			header = TNY_HEADER (tny_iterator_get_current (new_headers_iter));
			/* Apply per-message size limits */
			if (tny_header_get_message_size (header) < max_size)
				g_ptr_array_add (new_headers_array, g_object_ref (header));

			g_object_unref (header);
			tny_iterator_next (new_headers_iter);
		}
		g_object_unref (new_headers_iter);

		tny_folder_remove_observer (inbox, info->inbox_observer);
		g_object_unref (info->inbox_observer);
		info->inbox_observer = NULL;
	}

	if (new_headers_array->len == 0) {
		g_ptr_array_free (new_headers_array, FALSE);
		goto send_mail;
	}

	/* Get per-account message amount retrieval limit */
	retrieve_limit = modest_account_mgr_get_retrieve_limit (mgr, info->account_name);
	if (retrieve_limit == 0)
		retrieve_limit = G_MAXINT;

	/* Get per-account retrieval type */
	retrieve_type = modest_account_mgr_get_retrieve_type (mgr, info->account_name);
	headers_only = (retrieve_type == MODEST_ACCOUNT_RETRIEVE_HEADERS_ONLY);

	/* Order by date */
	g_ptr_array_sort (new_headers_array, (GCompareFunc) compare_headers_by_date);

	/* Copy the headers to a list and free the array */
	new_headers = tny_simple_list_new ();
	for (i=0; i < new_headers_array->len; i++) {
		TnyHeader *header = TNY_HEADER (g_ptr_array_index (new_headers_array, i));
		/* We want the first element to be the most recent
		   one, that's why we reverse the list */
		tny_list_prepend (new_headers, G_OBJECT (header));
	}
	g_ptr_array_foreach (new_headers_array, (GFunc) g_object_unref, NULL);
	g_ptr_array_free (new_headers_array, FALSE);

	if (!headers_only && (tny_list_get_length (new_headers) > 0)) {
		gint msg_num = 0;
		TnyIterator *iter;
		GetMsgInfo *msg_info;

		priv->done = 0;
		priv->total = MIN (tny_list_get_length (new_headers), retrieve_limit);

		iter = tny_list_create_iterator (new_headers);

		/* Create the message info */
		msg_info = g_slice_new0 (GetMsgInfo);
		msg_info->mail_op = g_object_ref (info->mail_op);
		msg_info->total_bytes = compute_message_list_size (new_headers, priv->total);
		msg_info->more_msgs = g_object_ref (iter);
		msg_info->msg = NULL;
		msg_info->user_data = info;

		while ((msg_num < priv->total ) && !tny_iterator_is_done (iter)) {
			TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
			TnyFolder *folder = tny_header_get_folder (header);

			/* Get message in an async way */
			tny_folder_get_msg_async (folder, header, update_account_get_msg_async_cb,
						  NULL, msg_info);

			g_object_unref (folder);
			g_object_unref (header);

			msg_num++;
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
		g_object_unref (new_headers);

		/* The mail operation will finish when the last
		   message is retrieved */
		return;
	}
 send_mail:
	/* If we don't have to retrieve the new messages then
	   simply send mail */
	update_account_send_mail (info);

	/* Check if the operation was a success */
	if (!priv->error)
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;

	/* Call the user callback and free */
	update_account_notify_user_and_free (info, new_headers);
}

static void
inbox_refresh_status_update (GObject *obj,
			     TnyStatus *status,
			     gpointer user_data)
{
	UpdateAccountInfo *info = NULL;
	ModestMailOperation *self = NULL;
	ModestMailOperationPrivate *priv = NULL;
	ModestMailOperationState *state;

	g_return_if_fail (user_data != NULL);
	g_return_if_fail (status != NULL);

	/* Show only the status information we want */
	if (status->code != TNY_FOLDER_STATUS_CODE_REFRESH)
		return;

	info = (UpdateAccountInfo *) user_data;
	self = info->mail_op;
	g_return_if_fail (MODEST_IS_MAIL_OPERATION(self));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	priv->done = status->position;
	priv->total = status->of_total;

	state = modest_mail_operation_clone_state (self);

	/* This is not a GDK lock because we are a Tinymail callback and
	 * Tinymail already acquires the Gdk lock */
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, state, NULL);

	g_slice_free (ModestMailOperationState, state);
}

static void 
recurse_folders_async_cb (TnyFolderStore *folder_store, 
			  gboolean canceled,
			  TnyList *list, 
			  GError *err, 
			  gpointer user_data)
{
	UpdateAccountInfo *info;
	ModestMailOperationPrivate *priv;
    
	info = (UpdateAccountInfo *) user_data;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (info->mail_op);

	if (err || canceled) {
		/* If the error was previosly set by another callback
		   don't set it again */
		if (!priv->error) {
			priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
			if (err)
				priv->error = g_error_copy (err);
			else
				g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
					     MODEST_MAIL_OPERATION_ERROR_OPERATION_CANCELED,
					     "canceled");
		}
	} else { 
		/* We're not getting INBOX children if we don't want to poke all */
		TnyIterator *iter = tny_list_create_iterator (list);
		while (!tny_iterator_is_done (iter)) {
			TnyFolderStore *folder = (TnyFolderStore*) tny_iterator_get_current (iter);

			/* Add to the list of all folders */
			tny_list_append (info->folders, (GObject *) folder);
			
			if (info->poke_all) {
				TnyList *folders = tny_simple_list_new ();
				/* Add pending call */
				info->pending_calls++;
				
				tny_folder_store_get_folders_async (folder, folders, NULL, FALSE,
								    recurse_folders_async_cb, 
								    NULL, info);
				g_object_unref (folders);
			}
			
			g_object_unref (G_OBJECT (folder));
			
			tny_iterator_next (iter);	    
		}
		g_object_unref (G_OBJECT (iter));
	}

	/* Remove my own pending call */
	info->pending_calls--;

	/* This means that we have all the folders */
	if (info->pending_calls == 0) {
		TnyIterator *iter_all_folders;
		TnyFolder *inbox = NULL;

		/* If there was any error do not continue */
		if (priv->error) {
			update_account_notify_user_and_free (info, NULL);
			return;
		}

		iter_all_folders = tny_list_create_iterator (info->folders);

		/* Do a poke status over all folders */
		while (!tny_iterator_is_done (iter_all_folders) &&
		       priv->status == MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS) {
			TnyFolder *folder = NULL;

			folder = TNY_FOLDER (tny_iterator_get_current (iter_all_folders));

			if (tny_folder_get_folder_type (folder) == TNY_FOLDER_TYPE_INBOX) {
				/* Get a reference to the INBOX */
				inbox = g_object_ref (folder);
			} else {
				/* Issue a poke status over the folder */
				if (info->poke_all)
					tny_folder_poke_status (folder);
			}

			/* Free and go to next */
			g_object_unref (folder);
			tny_iterator_next (iter_all_folders);
		}
		g_object_unref (iter_all_folders);

		/* Refresh the INBOX */
		if (inbox) {
			/* Refresh the folder. Our observer receives
			 * the new emails during folder refreshes, so
			 * we can use observer->new_headers
			 */
			info->inbox_observer = g_object_new (internal_folder_observer_get_type (), NULL);
			tny_folder_add_observer (inbox, info->inbox_observer);

			/* Refresh the INBOX */
			tny_folder_refresh_async (inbox, inbox_refreshed_cb, inbox_refresh_status_update, info);
			g_object_unref (inbox);
		} else {
			/* We could not perform the inbox refresh but
			   we'll try to send mails anyway */
			inbox_refreshed_cb (inbox, FALSE, NULL, info);
		}
	}
}

void
modest_mail_operation_update_account (ModestMailOperation *self,
				      const gchar *account_name,
				      gboolean poke_all,
				      gboolean interactive,
				      UpdateAccountCallback callback,
				      gpointer user_data)
{
	UpdateAccountInfo *info = NULL;
	ModestMailOperationPrivate *priv = NULL;
	ModestTnyAccountStore *account_store = NULL;
	TnyList *folders;
	ModestMailOperationState *state;

	/* Init mail operation */
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	priv->total = 0;
	priv->done  = 0;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE;

	/* Get the store account */
	account_store = modest_runtime_get_account_store ();
	priv->account =
		modest_tny_account_store_get_server_account (account_store,
							     account_name,
							     TNY_ACCOUNT_TYPE_STORE);

	/* The above function could return NULL */
	if (!priv->account) {
		/* Check if the operation was a success */
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
			     "no account");
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;

		/* Call the user callback */
		if (callback)
			callback (self, NULL, user_data);

		/* Notify about operation end */
		modest_mail_operation_notify_end (self);

		return;
	}
	
	/* We have once seen priv->account getting finalized during this code,
	 * therefore adding a reference (bug #82296) */
	
	g_object_ref (priv->account);

	/* Create the helper object */
	info = g_slice_new0 (UpdateAccountInfo);
	info->pending_calls = 1;
	info->folders = tny_simple_list_new ();
	info->mail_op = g_object_ref (self);
	info->poke_all = poke_all;
	info->interactive = interactive;
	info->account_name = g_strdup (account_name);
	info->callback = callback;
	info->user_data = user_data;

	/* Set account busy */
	modest_account_mgr_set_account_busy (modest_runtime_get_account_mgr (), account_name, TRUE);
	modest_mail_operation_notify_start (self);

	/* notify about the start of the operation */ 
	state = modest_mail_operation_clone_state (self);
	state->done = 0;
	state->total = 0;

	/* Start notifying progress */
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, state, NULL);
	g_slice_free (ModestMailOperationState, state);
	
	/* Get all folders and continue in the callback */ 
	folders = tny_simple_list_new ();
	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (priv->account),
					    folders, NULL, TRUE,
					    recurse_folders_async_cb, 
					    NULL, info);
	g_object_unref (folders);
	
	g_object_unref (priv->account);
	
}

/*
 * Used to notify the queue from the main
 * loop. We call it inside an idle call to achieve that
 */
static gboolean
idle_notify_queue (gpointer data)
{
	ModestMailOperation *mail_op = MODEST_MAIL_OPERATION (data);

	gdk_threads_enter ();
	modest_mail_operation_notify_end (mail_op);
	gdk_threads_leave ();
	g_object_unref (mail_op);

	return FALSE;
}

static int
compare_headers_by_date (gconstpointer a,
			 gconstpointer b)
{
	TnyHeader **header1, **header2;
	time_t sent1, sent2;

	header1 = (TnyHeader **) a;
	header2 = (TnyHeader **) b;

	sent1 = tny_header_get_date_sent (*header1);
	sent2 = tny_header_get_date_sent (*header2);

	/* We want the most recent ones (greater time_t) at the
	   beginning */
	if (sent1 < sent2)
		return -1;
	else
		return 1;
}


/* ******************************************************************* */
/* ************************** STORE  ACTIONS ************************* */
/* ******************************************************************* */

typedef struct {
	ModestMailOperation *mail_op;
	CreateFolderUserCallback callback;
	gpointer user_data;
} CreateFolderInfo;


static void
create_folder_cb (TnyFolderStore *parent_folder, 
		  gboolean canceled, 
		  TnyFolder *new_folder, 
		  GError *err, 
		  gpointer user_data)
{
	ModestMailOperationPrivate *priv;
	CreateFolderInfo *info;

	info = (CreateFolderInfo *) user_data;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (info->mail_op);

	if (canceled || err) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		if (err)
			priv->error = g_error_copy (err);
		else
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_OPERATION_CANCELED,
				     "canceled");		
	} else {
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

	/* The user will unref the new_folder */
	if (info->callback)
		info->callback (info->mail_op, parent_folder, 
				new_folder, info->user_data);
	
	/* Notify about operation end */
	modest_mail_operation_notify_end (info->mail_op);

	/* Frees */
	g_object_unref (info->mail_op);
	g_slice_free (CreateFolderInfo, info);
}

void
modest_mail_operation_create_folder (ModestMailOperation *self,
				     TnyFolderStore *parent,
				     const gchar *name,
				     CreateFolderUserCallback callback,
				     gpointer user_data)
{
	ModestMailOperationPrivate *priv;

	g_return_if_fail (TNY_IS_FOLDER_STORE (parent));
	g_return_if_fail (name);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_INFO;
	priv->account = (TNY_IS_ACCOUNT (parent)) ? 
		g_object_ref (parent) : 
		modest_tny_folder_get_account (TNY_FOLDER (parent));

	/* Check for already existing folder */
	if (modest_tny_folder_has_subfolder_with_name (parent, name, TRUE)) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
		             MODEST_MAIL_OPERATION_ERROR_FOLDER_EXISTS,
			     _CS("ckdg_ib_folder_already_exists"));
	}

	/* Check parent */
	if (TNY_IS_FOLDER (parent)) {
		/* Check folder rules */
		ModestTnyFolderRules rules = modest_tny_folder_get_rules (TNY_FOLDER (parent));
		if (rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE) {
			/* Set status failed and set an error */
			priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
				     _("mail_in_ui_folder_create_error"));
		}
	}

	if (!priv->error && (!strcmp (name, " ") || strchr (name, '/'))) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
			     _("mail_in_ui_folder_create_error"));
	}

	if (!priv->error) {
		CreateFolderInfo *info;

		info = g_slice_new0 (CreateFolderInfo);
		info->mail_op = g_object_ref (self);
		info->callback = callback;
		info->user_data = user_data;

		modest_mail_operation_notify_start (self);

		/* Create the folder */
		tny_folder_store_create_folder_async (parent, name, create_folder_cb, 
						      NULL, info);
	} else {
		/* Call the user callback anyway */
		if (callback)
			callback (self, parent, NULL, user_data);
		/* Notify about operation end */
		modest_mail_operation_notify_end (self);
	}
}

void
modest_mail_operation_remove_folder (ModestMailOperation *self,
				     TnyFolder           *folder,
				     gboolean             remove_to_trash)
{
	ModestMailOperationPrivate *priv;
	ModestTnyFolderRules rules;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER (folder));
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	
	/* Check folder rules */
	rules = modest_tny_folder_get_rules (TNY_FOLDER (folder));
	if (rules & MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE) {
 		/* Set status failed and set an error */
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
			     _("mail_in_ui_folder_delete_error"));
		goto end;
	}

	/* Get the account */
	priv->account = modest_tny_folder_get_account (folder);
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_DELETE;

	/* Delete folder or move to trash */
	if (remove_to_trash) {
		TnyFolder *trash_folder = NULL;
		trash_folder = modest_tny_account_get_special_folder (priv->account,
								      TNY_FOLDER_TYPE_TRASH);
		/* TODO: error_handling */
		if (trash_folder) {
			modest_mail_operation_notify_start (self);
			modest_mail_operation_xfer_folder (self, folder,
						    TNY_FOLDER_STORE (trash_folder), 
						    TRUE, NULL, NULL);
			g_object_unref (trash_folder);
		} else {
			g_warning ("%s: modest_tny_account_get_special_folder(..) returned a NULL trash folder", __FUNCTION__);
		}
	} else {
		TnyFolderStore *parent = tny_folder_get_folder_store (folder);
		if (parent) {
			modest_mail_operation_notify_start (self);
			tny_folder_store_remove_folder (parent, folder, &(priv->error));
			CHECK_EXCEPTION (priv, MODEST_MAIL_OPERATION_STATUS_FAILED);
			
			if (!priv->error)
				priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;

			g_object_unref (parent);
		} else
			g_warning ("%s: could not get parent folder", __FUNCTION__);
	}

 end:
	/* Notify about operation end */
	modest_mail_operation_notify_end (self);
}

static void
transfer_folder_status_cb (GObject *obj,
			   TnyStatus *status,
			   gpointer user_data)
{
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;
	ModestMailOperationState *state;
	XFerFolderAsyncHelper *helper;

	g_return_if_fail (status != NULL);

	/* Show only the status information we want */
	if (status->code != TNY_FOLDER_STATUS_CODE_COPY_FOLDER)
		return;

	helper = (XFerFolderAsyncHelper *) user_data;
	g_return_if_fail (helper != NULL);

	self = helper->mail_op;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	priv->done = status->position;
	priv->total = status->of_total;

	state = modest_mail_operation_clone_state (self);

	/* This is not a GDK lock because we are a Tinymail callback
	 * which is already GDK locked by Tinymail */

	/* no gdk_threads_enter (), CHECKED */

	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, state, NULL); 

	/* no gdk_threads_leave (), CHECKED */

	g_slice_free (ModestMailOperationState, state);
}

static void
transfer_folder_cb (TnyFolder *folder, 
		    gboolean cancelled, 
		    TnyFolderStore *into, 
		    TnyFolder *new_folder, 
		    GError *err, 
		    gpointer user_data)
{
	XFerFolderAsyncHelper *helper;
	ModestMailOperation *self = NULL;
	ModestMailOperationPrivate *priv = NULL;

	helper = (XFerFolderAsyncHelper *) user_data;
	g_return_if_fail (helper != NULL);

	self = helper->mail_op;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	if (err) {
		priv->error = g_error_copy (err);
		priv->done = 0;
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
	} else if (cancelled) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_OPERATION_CANCELED,
			     _("Transference of %s was cancelled."),
			     tny_folder_get_name (folder));
	} else {
		priv->done = 1;
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

	/* Update state of new folder */
	if (new_folder) {
		tny_folder_refresh_async (new_folder, NULL, NULL, NULL);
		tny_folder_poke_status (new_folder);
	}

	/* Notify about operation end */
	modest_mail_operation_notify_end (self);

	/* If user defined callback function was defined, call it */
	if (helper->user_callback) {

		/* This is not a GDK lock because we are a Tinymail callback
	 	 * which is already GDK locked by Tinymail */

		/* no gdk_threads_enter (), CHECKED */
		helper->user_callback (self, new_folder, helper->user_data);
		/* no gdk_threads_leave () , CHECKED */
	}

	/* Free */
	g_object_unref (helper->mail_op);
	g_slice_free   (XFerFolderAsyncHelper, helper);
}

/**
 *
 * This function checks if the new name is a valid name for our local
 * folders account. The new name could not be the same than then name
 * of any of the mandatory local folders
 *
 * We can not rely on tinymail because tinymail does not check the
 * name of the virtual folders that the account could have in the case
 * that we're doing a rename (because it directly calls Camel which
 * knows nothing about our virtual folders). 
 *
 * In the case of an actual copy/move (i.e. move/copy a folder between
 * accounts) tinymail uses the tny_folder_store_create_account which
 * is reimplemented by our ModestTnyLocalFoldersAccount that indeed
 * checks the new name of the folder, so this call in that case
 * wouldn't be needed. *But* NOTE that if tinymail changes its
 * implementation (if folder transfers within the same account is no
 * longer implemented as a rename) this call will allow Modest to work
 * perfectly
 *
 * If the new name is not valid, this function will set the status to
 * failed and will set also an error in the mail operation
 */
static gboolean
new_name_valid_if_local_account (ModestMailOperationPrivate *priv,
				 TnyFolderStore *into,
				 const gchar *new_name)
{
	if (TNY_IS_ACCOUNT (into) && 
	    modest_tny_account_is_virtual_local_folders (TNY_ACCOUNT (into)) &&
	    modest_tny_local_folders_account_folder_name_in_use (MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (into),
								 new_name)) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_FOLDER_EXISTS,
			     _CS("ckdg_ib_folder_already_exists"));
		return FALSE;
	} else
		return TRUE;
}

void
modest_mail_operation_xfer_folder (ModestMailOperation *self,
				   TnyFolder *folder,
				   TnyFolderStore *parent,
				   gboolean delete_original,
				   XferFolderAsyncUserCallback user_callback,
				   gpointer user_data)
{
	ModestMailOperationPrivate *priv = NULL;
	ModestTnyFolderRules parent_rules = 0, rules; 
	XFerFolderAsyncHelper *helper = NULL;
	const gchar *folder_name = NULL;
	const gchar *error_msg;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER (folder));
	g_return_if_fail (TNY_IS_FOLDER_STORE (parent));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	folder_name = tny_folder_get_name (folder);

	/* Set the error msg */
	error_msg = _("mail_in_ui_folder_move_target_error");

	/* Get account and set it into mail_operation */
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_RECEIVE;
	priv->account = modest_tny_folder_get_account (TNY_FOLDER(folder));
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	/* Get folder rules */
	rules = modest_tny_folder_get_rules (TNY_FOLDER (folder));
	if (TNY_IS_FOLDER (parent))
		parent_rules = modest_tny_folder_get_rules (TNY_FOLDER (parent));
	
	/* Apply operation constraints */
	if ((gpointer) parent == (gpointer) folder ||
	    (!TNY_IS_FOLDER_STORE (parent)) || 
	    (rules & MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE)) {
		/* Folder rules */
		goto error;
	} else if (TNY_IS_FOLDER (parent) && 
		   (parent_rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE)) {
		/* Folder rules */
		goto error;

	} else if (TNY_IS_FOLDER (parent) &&
		   TNY_IS_FOLDER_STORE (folder) &&
		   modest_tny_folder_is_ancestor (TNY_FOLDER (parent), 
						  TNY_FOLDER_STORE (folder))) {
		/* Do not move a parent into a child */
		goto error;
	} else if (TNY_IS_FOLDER_STORE (parent) &&
		   modest_tny_folder_has_subfolder_with_name (parent, folder_name, TRUE)) {
		/* Check that the new folder name is not used by any
		   parent subfolder */
		goto error;	
	} else if (!(new_name_valid_if_local_account (priv, parent, folder_name))) {
		/* Check that the new folder name is not used by any
		   special local folder */
		goto error;
	} else {
		/* Create the helper */
		helper = g_slice_new0 (XFerFolderAsyncHelper);
		helper->mail_op = g_object_ref (self);
		helper->user_callback = user_callback;
		helper->user_data = user_data;
		
		/* Move/Copy folder */
		modest_mail_operation_notify_start (self);
		tny_folder_copy_async (folder,
				       parent,
				       tny_folder_get_name (folder),
				       delete_original,
				       transfer_folder_cb,
				       transfer_folder_status_cb,
				       helper);
		return;
	}

 error:
	/* Set status failed and set an error */
	priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
	g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
		     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
		     "%s", error_msg);

	/* Call the user callback if exists */
	if (user_callback)
		user_callback (self, NULL, user_data);

	/* Notify the queue */
	modest_mail_operation_notify_end (self);
}

void
modest_mail_operation_rename_folder (ModestMailOperation *self,
				     TnyFolder *folder,
				     const gchar *name,
				     XferFolderAsyncUserCallback user_callback,
				     gpointer user_data)
{
	ModestMailOperationPrivate *priv;
	ModestTnyFolderRules rules;
	XFerFolderAsyncHelper *helper;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER_STORE (folder));
	g_return_if_fail (name);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* Get account and set it into mail_operation */
	priv->account = modest_tny_folder_get_account (TNY_FOLDER(folder));
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_INFO;

	/* Check folder rules */
	rules = modest_tny_folder_get_rules (TNY_FOLDER (folder));
	if (rules & MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE) {
		goto error;
	} else if (!strcmp (name, " ") || strchr (name, '/')) {
		goto error;
	} else {
		TnyFolderStore *into;

		into = tny_folder_get_folder_store (folder);	

		/* Check that the new folder name is not used by any
		   special local folder */
		if (new_name_valid_if_local_account (priv, into, name)) {
			/* Create the helper */
			helper = g_slice_new0 (XFerFolderAsyncHelper);
			helper->mail_op = g_object_ref(self);
			helper->user_callback = user_callback;
			helper->user_data = user_data;
		
			/* Rename. Camel handles folder subscription/unsubscription */
			modest_mail_operation_notify_start (self);
			tny_folder_copy_async (folder, into, name, TRUE,
					       transfer_folder_cb,
					       transfer_folder_status_cb,
					       helper);
			g_object_unref (into);
		} else {
			g_object_unref (into);
			goto error;
		}

		return;
	}
 error:
	/* Set status failed and set an error */
	priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
	g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
		     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
		     _("FIXME: unable to rename"));
	
	if (user_callback)
		user_callback (self, NULL, user_data);

	/* Notify about operation end */
	modest_mail_operation_notify_end (self);
}

/* ******************************************************************* */
/* **************************  MSG  ACTIONS  ************************* */
/* ******************************************************************* */

void 
modest_mail_operation_find_msg (ModestMailOperation *self,
				TnyFolder *folder,
				const gchar *msg_uid,
				gboolean progress_feedback,
				GetMsgAsyncUserCallback user_callback,
				gpointer user_data)
{
	GetMsgInfo *helper = NULL;
	ModestMailOperationPrivate *priv;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (msg_uid != NULL);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->total = 1;
	priv->done = 0;

	/* Check memory low */
	if (_check_memory_low (self)) {
		if (user_callback)
			user_callback (self, NULL, FALSE, NULL, priv->error, user_data);
		modest_mail_operation_notify_end (self);
		return;
	}

	/* Get account and set it into mail_operation */
	priv->account = modest_tny_folder_get_account (TNY_FOLDER(folder));
	
	/* Check for cached messages */
	if (progress_feedback) {
		priv->op_type = MODEST_MAIL_OPERATION_TYPE_RECEIVE;
	} else {
		priv->op_type = MODEST_MAIL_OPERATION_TYPE_UNKNOWN;
	}
	
	/* Create the helper */
	helper = g_slice_new0 (GetMsgInfo);
	helper->header = NULL;
	helper->mail_op = g_object_ref (self);
	helper->user_callback = user_callback;
	helper->user_data = user_data;
	helper->destroy_notify = NULL;
	helper->last_total_bytes = 0;
	helper->sum_total_bytes = 0;
	helper->total_bytes = 0;
	helper->more_msgs = NULL;
	helper->get_parts = NULL;
	helper->msg = NULL;

	modest_mail_operation_notify_start (self);
	
	/* notify about the start of the operation */ 
	ModestMailOperationState *state;
	state = modest_mail_operation_clone_state (self);
	state->done = 0;
	state->total = 0;
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 
				0, state, NULL);
	g_slice_free (ModestMailOperationState, state);
	
	tny_folder_find_msg_async (folder, msg_uid, get_msg_async_cb, get_msg_status_cb, helper);
}

void 
modest_mail_operation_get_msg (ModestMailOperation *self,
			       TnyHeader *header,
			       gboolean progress_feedback,
			       GetMsgAsyncUserCallback user_callback,
			       gpointer user_data)
{
	GetMsgInfo *helper = NULL;
	TnyFolder *folder;
	ModestMailOperationPrivate *priv;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_HEADER (header));
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->total = 1;
	priv->done = 0;

	/* Check memory low */
	if (_check_memory_low (self)) {
		if (user_callback)
			user_callback (self, header, FALSE, NULL, priv->error, user_data);
		modest_mail_operation_notify_end (self);
		return;
	}

	/* Get account and set it into mail_operation */
	folder = tny_header_get_folder (header);
	priv->account = modest_tny_folder_get_account (TNY_FOLDER(folder));
	
	/* Check for cached messages */
	if (progress_feedback) {
		if (tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED)
			priv->op_type = MODEST_MAIL_OPERATION_TYPE_OPEN;
		else 
			priv->op_type = MODEST_MAIL_OPERATION_TYPE_RECEIVE;
	} else {
		priv->op_type = MODEST_MAIL_OPERATION_TYPE_UNKNOWN;
	}
	
	/* Create the helper */
	helper = g_slice_new0 (GetMsgInfo);
	helper->header = g_object_ref (header);
	helper->mail_op = g_object_ref (self);
	helper->user_callback = user_callback;
	helper->user_data = user_data;
	helper->destroy_notify = NULL;
	helper->last_total_bytes = 0;
	helper->sum_total_bytes = 0;
	helper->total_bytes = tny_header_get_message_size (header);
	helper->more_msgs = NULL;
	helper->get_parts = NULL;
	helper->msg = NULL;

	modest_mail_operation_notify_start (self);
	
	/* notify about the start of the operation */ 
	ModestMailOperationState *state;
	state = modest_mail_operation_clone_state (self);
	state->done = 0;
	state->total = 0;
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 
				0, state, NULL);
	g_slice_free (ModestMailOperationState, state);
	
	tny_folder_get_msg_async (folder, header, get_msg_async_cb, get_msg_status_cb, helper);

	g_object_unref (G_OBJECT (folder));
}

void 
modest_mail_operation_get_msg_and_parts (ModestMailOperation *self,
					 TnyHeader *header,
					 TnyList *parts,
					 gboolean progress_feedback,
					 GetMsgAsyncUserCallback user_callback,
					 gpointer user_data)
{
	GetMsgInfo *helper = NULL;
	TnyFolder *folder;
	ModestMailOperationPrivate *priv;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_HEADER (header));
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->total = 1;
	priv->done = 0;

	/* Check memory low */
	if (_check_memory_low (self)) {
		if (user_callback)
			user_callback (self, header, FALSE, NULL, priv->error, user_data);
		modest_mail_operation_notify_end (self);
		return;
	}

	/* Get account and set it into mail_operation */
	folder = tny_header_get_folder (header);
	if (folder == NULL && MODEST_IS_MSG_VIEW_WINDOW (priv->source)) {
		const gchar *acc_name;
		acc_name = modest_window_get_active_account (MODEST_WINDOW (priv->source));
		priv->account = modest_tny_account_store_get_server_account
			(modest_runtime_get_account_store (),
			 acc_name,
			 TNY_ACCOUNT_TYPE_STORE);
		folder = modest_tny_folder_store_find_folder_from_uri (TNY_FOLDER_STORE (priv->account), 
								       modest_msg_view_window_get_message_uid (MODEST_MSG_VIEW_WINDOW (priv->source)));
	} else {
		priv->account = modest_tny_folder_get_account (TNY_FOLDER(folder));
	}
	
	/* Check for cached messages */
	if (progress_feedback) {
		if (tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED)
			priv->op_type = MODEST_MAIL_OPERATION_TYPE_OPEN;
		else 
			priv->op_type = MODEST_MAIL_OPERATION_TYPE_RECEIVE;
	} else {
		priv->op_type = MODEST_MAIL_OPERATION_TYPE_UNKNOWN;
	}
	
	/* Create the helper */
	helper = g_slice_new0 (GetMsgInfo);
	helper->header = g_object_ref (header);
	helper->mail_op = g_object_ref (self);
	helper->user_callback = user_callback;
	helper->user_data = user_data;
	helper->destroy_notify = NULL;
	helper->last_total_bytes = 0;
	helper->sum_total_bytes = 0;
	helper->total_bytes = tny_header_get_message_size (header);
	helper->more_msgs = NULL;
	helper->get_parts = tny_list_create_iterator (parts);
	helper->msg = NULL;

	modest_mail_operation_notify_start (self);
	
	/* notify about the start of the operation */ 
	ModestMailOperationState *state;
	state = modest_mail_operation_clone_state (self);
	state->done = 0;
	state->total = 0;
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 
				0, state, NULL);
	g_slice_free (ModestMailOperationState, state);
	
	tny_folder_get_msg_async (folder, header, get_msg_async_cb, get_msg_status_cb, helper);

	g_object_unref (G_OBJECT (folder));
}

static void     
get_msg_status_cb (GObject *obj,
		   TnyStatus *status,  
		   gpointer user_data)
{
	GetMsgInfo *helper = NULL;

	g_return_if_fail (status != NULL);

	/* Show only the status information we want */
	if (status->code != TNY_FOLDER_STATUS_CODE_GET_MSG)
		return;

	helper = (GetMsgInfo *) user_data;
	g_return_if_fail (helper != NULL);       

	/* Notify progress */
	notify_progress_of_multiple_messages (helper->mail_op, status, &(helper->last_total_bytes), 
					      &(helper->sum_total_bytes), helper->total_bytes, FALSE);
}

static void
get_msg_async_get_part_cb (TnyMimePart *self, gboolean cancelled, TnyStream *stream, GError *err, gpointer user_data)
{
	GetMsgInfo *helper;
	TnyFolder *folder = NULL;

	helper = (GetMsgInfo *) user_data;

	if (helper->header) {
		folder = tny_header_get_folder (helper->header);
	}

	get_msg_async_cb (folder, cancelled, helper->msg, err, user_data);

	if (folder) g_object_unref (folder);
}

static void
get_msg_async_cb (TnyFolder *folder, 
		  gboolean canceled, 
		  TnyMsg *msg, 
		  GError *err, 
		  gpointer user_data)
{
	GetMsgInfo *info = NULL;
	ModestMailOperationPrivate *priv = NULL;
	gboolean finished;

	info = (GetMsgInfo *) user_data;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (info->mail_op);
	priv->done++;

	if (info->more_msgs) {
		tny_iterator_next (info->more_msgs);
		finished = (tny_iterator_is_done (info->more_msgs));
	} else if (info->get_parts) {
		tny_iterator_next (info->get_parts);
		finished = (tny_iterator_is_done (info->get_parts));
	} else {
		finished = (priv->done == priv->total) ? TRUE : FALSE;
	}

	/* If canceled by the user, ignore the error given by Tinymail */
	if (canceled) {
		canceled = TRUE;
		finished = TRUE;
		priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
	} else if (err) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS;
		priv->error = g_error_copy ((const GError *) err);
		if (priv->error) {
			priv->error->domain = MODEST_MAIL_OPERATION_ERROR;
		} else {
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
				     "%s", err->message);
		}
	} else if (finished && priv->status == MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS) {
		/* Set the success status before calling the user callback */
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

	if (info->header == NULL && msg)
		info->header = tny_msg_get_header (msg);

	/* Call the user callback */
	if (info->user_callback && (finished || (info->get_parts == NULL)))
		info->user_callback (info->mail_op, info->header, canceled, 
				     msg, err, info->user_data);

	/* Notify about operation end if this is the last callback */
	if (finished) {
		/* Free user data */
		if (info->destroy_notify)
			info->destroy_notify (info->user_data);

		/* Notify about operation end */
		modest_mail_operation_notify_end (info->mail_op);

		/* Clean */
		if (info->msg)
			g_object_unref (info->msg);
		if (info->more_msgs)
			g_object_unref (info->more_msgs);
		if (info->header)
			g_object_unref (info->header);
		g_object_unref (info->mail_op);
		g_slice_free (GetMsgInfo, info);
	} else if (info->get_parts) {
		CamelStream *null_stream;
		TnyStream *tny_null_stream;
		TnyMimePart *part;

		if (info->msg == NULL && msg != NULL)
			info->msg = g_object_ref (msg);

		null_stream = camel_stream_null_new ();
		tny_null_stream = tny_camel_stream_new (null_stream);
		
		part = TNY_MIME_PART (tny_iterator_get_current (info->get_parts));
		tny_mime_part_decode_to_stream_async (part, tny_null_stream, 
						      get_msg_async_get_part_cb,
						      get_msg_status_cb,
						      info);
		g_object_unref (tny_null_stream);
		g_object_unref (part);

	} else if (info->more_msgs) {
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (info->more_msgs));
		TnyFolder *folder = tny_header_get_folder (header);

		g_object_unref (info->header);
		info->header = g_object_ref (header);

		/* Retrieve the next message */
		tny_folder_get_msg_async (folder, header, get_msg_async_cb, get_msg_status_cb, info);

		g_object_unref (header);
		g_object_unref (folder);
	} else {
		g_warning ("%s: finished != TRUE but no messages left", __FUNCTION__);
	}
}

void 
modest_mail_operation_get_msgs_full (ModestMailOperation *self,
				     TnyList *header_list, 
				     GetMsgAsyncUserCallback user_callback,
				     gpointer user_data,
				     GDestroyNotify notify)
{
	ModestMailOperationPrivate *priv = NULL;
	gint msg_list_size;
	TnyIterator *iter = NULL;
	gboolean has_uncached_messages;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));

	/* Init mail operation */
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->done = 0;
	priv->total = tny_list_get_length(header_list);

	/* Check memory low */
	if (_check_memory_low (self)) {
		if (user_callback) {
			TnyHeader *header = NULL;
			TnyIterator *iter;

			if (tny_list_get_length (header_list) > 0) {
				iter = tny_list_create_iterator (header_list);
				header = (TnyHeader *) tny_iterator_get_current (iter);
				g_object_unref (iter);
			}
			user_callback (self, header, FALSE, NULL, priv->error, user_data);
			if (header)
				g_object_unref (header);
		}
		if (notify)
			notify (user_data);
		/* Notify about operation end */
		modest_mail_operation_notify_end (self);
		return;
	}

	/* Check uncached messages */
	for (iter = tny_list_create_iterator (header_list), has_uncached_messages = FALSE;
	     !has_uncached_messages && !tny_iterator_is_done (iter); 
	     tny_iterator_next (iter)) {
		TnyHeader *header;

		header = (TnyHeader *) tny_iterator_get_current (iter);
		if (!(tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED))
			has_uncached_messages = TRUE;
		g_object_unref (header);
	}	
	g_object_unref (iter);
	priv->op_type = has_uncached_messages?MODEST_MAIL_OPERATION_TYPE_RECEIVE:MODEST_MAIL_OPERATION_TYPE_OPEN;

	/* Get account and set it into mail_operation */
	if (tny_list_get_length (header_list) >= 1) {
		TnyIterator *iterator = tny_list_create_iterator (header_list);
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iterator));
		if (header) {
			TnyFolder *folder = tny_header_get_folder (header);
			if (folder) {		
				priv->account = modest_tny_folder_get_account (TNY_FOLDER(folder));
				g_object_unref (folder);
			}
			g_object_unref (header);
		}
		g_object_unref (iterator);
	}

	msg_list_size = compute_message_list_size (header_list, 0);

	modest_mail_operation_notify_start (self);
	iter = tny_list_create_iterator (header_list);
	if (!tny_iterator_is_done (iter)) {
		/* notify about the start of the operation */
		ModestMailOperationState *state;
		state = modest_mail_operation_clone_state (self);
		state->done = 0;
		state->total = 0;
		g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL],
			       0, state, NULL);

		GetMsgInfo *msg_info = NULL;
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
		TnyFolder *folder = tny_header_get_folder (header);

		/* Create the message info */
		msg_info = g_slice_new0 (GetMsgInfo);
		msg_info->mail_op = g_object_ref (self);
		msg_info->header = g_object_ref (header);
		msg_info->more_msgs = g_object_ref (iter);
		msg_info->user_callback = user_callback;
		msg_info->user_data = user_data;
		msg_info->destroy_notify = notify;
		msg_info->last_total_bytes = 0;
		msg_info->sum_total_bytes = 0;
		msg_info->total_bytes = msg_list_size;
		msg_info->msg = NULL;

		/* The callback will call it per each header */
		tny_folder_get_msg_async (folder, header, get_msg_async_cb, get_msg_status_cb, msg_info);

		/* Free and go on */
		g_object_unref (header);
		g_object_unref (folder);
		g_slice_free (ModestMailOperationState, state);
	}
	g_object_unref (iter);
}


static void
remove_msgs_async_cb (TnyFolder *folder, 
		      gboolean canceled, 
		      GError *err, 
		      gpointer user_data)
{
	gboolean expunge;
	const gchar *account_name;
	TnyAccount *account;
	ModestProtocolType account_proto = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;
	ModestProtocolRegistry *protocol_registry;
	SyncFolderHelper *helper;

	self = (ModestMailOperation *) user_data;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	protocol_registry = modest_runtime_get_protocol_registry ();

	if (canceled || err) {
		/* If canceled by the user, ignore the error given by Tinymail */
		if (canceled) {
			priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
		} else if (err) {
			priv->status = MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS;
			priv->error = g_error_copy ((const GError *) err);
			priv->error->domain = MODEST_MAIL_OPERATION_ERROR;
		}
		/* Exit */
		modest_mail_operation_notify_end (self);
		g_object_unref (self);
		return;
	}

	account = modest_tny_folder_get_account (folder);
	account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (account);
	account_proto = modest_tny_account_get_protocol_type (account);
	g_object_unref (account);

	if (modest_protocol_registry_protocol_type_has_leave_on_server (protocol_registry, account_proto)) {
		if (modest_account_mgr_get_leave_on_server (modest_runtime_get_account_mgr (),
							    account_name))
			expunge = FALSE;
		else
			expunge = TRUE;
	} else {
		expunge = TRUE;
	}

	/* Create helper */
	helper = g_slice_new0 (SyncFolderHelper);
	helper->mail_op = g_object_ref (self);
	helper->user_callback = NULL;
	helper->user_data = NULL;

	/* Sync folder */
	tny_folder_sync_async(folder, expunge, sync_folder_finish_callback, NULL, helper);

	/* Remove the extra reference */
	g_object_unref (self);
}

void 
modest_mail_operation_remove_msgs (ModestMailOperation *self,  
				   TnyList *headers,
				   gboolean remove_to_trash /*ignored*/)
{
	TnyFolder *folder = NULL;
	ModestMailOperationPrivate *priv;
	TnyIterator *iter = NULL;
	TnyHeader *header = NULL;
	TnyList *remove_headers = NULL;
	TnyFolderType folder_type = TNY_FOLDER_TYPE_UNKNOWN;
	ModestTnyAccountStore *accstore = modest_runtime_get_account_store();

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_LIST (headers));

	if (remove_to_trash)
		g_warning ("remove to trash is not implemented");

	if (tny_list_get_length(headers) == 0) {
		g_warning ("%s: list of headers is empty\n", __FUNCTION__);
		goto cleanup; /* nothing to do */
	}

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* Get folder from first header and sync it */
	iter = tny_list_create_iterator (headers);
	header = TNY_HEADER (tny_iterator_get_current (iter));
	g_object_unref (iter);

	folder = tny_header_get_folder (header);
	if (!TNY_IS_FOLDER(folder)) {
		g_warning ("%s: could not get folder for header\n", __FUNCTION__);
		goto cleanup;
	}

	/* Use the merged folder if we're removing messages from outbox */
	if (modest_tny_folder_is_local_folder (folder)) {
		ModestTnyLocalFoldersAccount *local_account;

		local_account = (ModestTnyLocalFoldersAccount *)
			modest_tny_account_store_get_local_folders_account (accstore);
		folder_type = modest_tny_folder_get_local_or_mmc_folder_type (folder);
		if (folder_type == TNY_FOLDER_TYPE_OUTBOX) {
			g_object_unref (folder);
			folder = modest_tny_local_folders_account_get_merged_outbox (local_account);
		}
		g_object_unref (local_account);
	}

	if (folder_type == TNY_FOLDER_TYPE_OUTBOX) {
		TnyIterator *headers_iter = tny_list_create_iterator (headers);

		while (!tny_iterator_is_done (headers_iter)) {
			TnyTransportAccount *traccount = NULL;
			TnyHeader *hdr = NULL;

			hdr = TNY_HEADER (tny_iterator_get_current (headers_iter));
			traccount = modest_tny_account_store_get_transport_account_from_outbox_header (accstore,
												       header);
			if (traccount) {
				ModestTnySendQueueStatus status;
				ModestTnySendQueue *send_queue;

				send_queue = modest_runtime_get_send_queue(traccount, TRUE);
				if (TNY_IS_SEND_QUEUE (send_queue)) {
					char *msg_id;

					msg_id = modest_tny_send_queue_get_msg_id (hdr);
					status = modest_tny_send_queue_get_msg_status(send_queue, msg_id);
					if (status != MODEST_TNY_SEND_QUEUE_SENDING) {
						if (G_UNLIKELY (remove_headers == NULL))
							remove_headers = tny_simple_list_new ();
						tny_list_append(remove_headers, G_OBJECT(hdr));
					}
					g_free(msg_id);
				}
				g_object_unref(traccount);
			}
			g_object_unref(hdr);
			tny_iterator_next (headers_iter);
		}
		g_object_unref(headers_iter);
	}

	/* Get account and set it into mail_operation */
	priv->account = modest_tny_folder_get_account (TNY_FOLDER(folder));
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_DELETE;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	if (!remove_headers)
		remove_headers = g_object_ref (headers);

	/* remove message from folder */
	modest_mail_operation_notify_start (self);
	tny_folder_remove_msgs_async (folder, remove_headers, remove_msgs_async_cb, 
				      NULL, g_object_ref (self));

cleanup:
	if (remove_headers)
		g_object_unref (remove_headers);
	if (header)
		g_object_unref (header);
	if (folder)
		g_object_unref (folder);
}

static void
notify_progress_of_multiple_messages (ModestMailOperation *self,
				      TnyStatus *status,
				      gint *last_total_bytes,
				      gint *sum_total_bytes,
				      gint total_bytes, 
				      gboolean increment_done)
{
	ModestMailOperationPrivate *priv;
	ModestMailOperationState *state;
	gboolean is_num_bytes = FALSE;

	priv = 	MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* We know that tinymail sends us information about
	 *  transferred bytes with this particular message
	 */
	if (status->message)
		is_num_bytes = (g_ascii_strcasecmp (status->message, "Retrieving message") == 0);

	state = modest_mail_operation_clone_state (self);
	if (is_num_bytes && !((status->position == 1) && (status->of_total == 100))) {
		/* We know that we're in a different message when the
		   total number of bytes to transfer is different. Of
		   course it could fail if we're transferring messages
		   of the same size, but this is a workarround */
		if (status->of_total != *last_total_bytes) {
			/* We need to increment the done when there is
			   no information about each individual
			   message, we need to do this in message
			   transfers, and we don't do it for getting
			   messages */
			if (increment_done)
				priv->done++;
			*sum_total_bytes += *last_total_bytes;
			*last_total_bytes = status->of_total;
		}
		state->bytes_done += status->position + *sum_total_bytes;
		state->bytes_total = total_bytes;

		/* Notify the status change. Only notify about changes
		   referred to bytes */
		g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 
			       0, state, NULL);
	}

	g_slice_free (ModestMailOperationState, state);
}

static void
transfer_msgs_status_cb (GObject *obj,
			 TnyStatus *status,  
			 gpointer user_data)
{
	XFerMsgsAsyncHelper *helper;

	g_return_if_fail (status != NULL);

	/* Show only the status information we want */
	if (status->code != TNY_FOLDER_STATUS_CODE_XFER_MSGS)
		return;

	helper = (XFerMsgsAsyncHelper *) user_data;
	g_return_if_fail (helper != NULL);       

	/* Notify progress */
	notify_progress_of_multiple_messages (helper->mail_op, status, &(helper->last_total_bytes), 
					      &(helper->sum_total_bytes), helper->total_bytes, TRUE);
}

static void
transfer_msgs_sync_folder_cb (TnyFolder *self, 
			      gboolean cancelled, 
			      GError *err, 
			      gpointer user_data)
{
	XFerMsgsAsyncHelper *helper;
	/* We don't care here about the results of the
	   synchronization */
	helper = (XFerMsgsAsyncHelper *) user_data;

	/* Notify about operation end */
	modest_mail_operation_notify_end (helper->mail_op);

	/* If user defined callback function was defined, call it */
	if (helper->user_callback)
		helper->user_callback (helper->mail_op, helper->user_data);
	
	/* Free */
	if (helper->more_msgs)
		g_object_unref (helper->more_msgs);
	if (helper->headers)
		g_object_unref (helper->headers);
	if (helper->dest_folder)
		g_object_unref (helper->dest_folder);
	if (helper->mail_op)
		g_object_unref (helper->mail_op);
	g_slice_free (XFerMsgsAsyncHelper, helper);
}

static void
transfer_msgs_cb (TnyFolder *folder, gboolean cancelled, GError *err, gpointer user_data)
{
	XFerMsgsAsyncHelper *helper;
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;
	gboolean finished = TRUE;

	helper = (XFerMsgsAsyncHelper *) user_data;
	self = helper->mail_op;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (cancelled) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
	} else if (err) {
		priv->error = g_error_copy (err);
		priv->done = 0;
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;	
	} else if (priv->status != MODEST_MAIL_OPERATION_STATUS_CANCELED) {
		if (helper->more_msgs) {
			/* We'll transfer the next message in the list */
			tny_iterator_next (helper->more_msgs);
			if (!tny_iterator_is_done (helper->more_msgs)) {
				GObject *next_header;
				g_object_unref (helper->headers);
				helper->headers = tny_simple_list_new ();
				next_header = tny_iterator_get_current (helper->more_msgs);
				tny_list_append (helper->headers, next_header);
				g_object_unref (next_header);
				finished = FALSE;
			}
		}
		if (finished) {
			priv->done = 1;
			priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
		}
	}

	if (finished) {
		/* Synchronize the source folder contents. This should
		   be done by tinymail but the camel_folder_sync it's
		   actually disabled in transfer_msgs_thread_clean
		   because it's supposed to cause hangs */
		tny_folder_sync_async (folder, helper->delete, 
				       transfer_msgs_sync_folder_cb, 
				       NULL, helper);
	} else {
		/* Transfer more messages */
		tny_folder_transfer_msgs_async (folder,
						helper->headers,
						helper->dest_folder,
						helper->delete,
						transfer_msgs_cb,
						transfer_msgs_status_cb,
						helper);
	}
}

/* Computes the size of the messages the headers in the list belongs
   to. If num_elements is different from 0 then it only takes into
   account the first num_elements for the calculation */
static guint
compute_message_list_size (TnyList *headers, 
			   guint num_elements)
{
	TnyIterator *iter;
	guint size = 0, element = 0;

	/* If num_elements is not valid then take all into account */
	if ((num_elements <= 0) || (num_elements > tny_list_get_length (headers)))
		num_elements = tny_list_get_length (headers);

	iter = tny_list_create_iterator (headers);
	while (!tny_iterator_is_done (iter) && element < num_elements) {
		TnyHeader *header = TNY_HEADER (tny_iterator_get_current (iter));
		size += tny_header_get_message_size (header);
		g_object_unref (header);
		tny_iterator_next (iter);
		element++;
	}
	g_object_unref (iter);

	return size;
}

void
modest_mail_operation_xfer_msgs (ModestMailOperation *self,
				 TnyList *headers, 
				 TnyFolder *folder, 
				 gboolean delete_original,
				 XferMsgsAsyncUserCallback user_callback,
				 gpointer user_data)
{
	ModestMailOperationPrivate *priv = NULL;
	TnyIterator *iter = NULL;
	TnyFolder *src_folder = NULL;
	XFerMsgsAsyncHelper *helper = NULL;
	TnyHeader *header = NULL;
	ModestTnyFolderRules rules = 0;
	TnyAccount *dst_account = NULL;
	gboolean leave_on_server;
	ModestMailOperationState *state;
	ModestProtocolRegistry *protocol_registry;
	ModestProtocolType account_protocol;

	g_return_if_fail (self && MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (headers && TNY_IS_LIST (headers));
	g_return_if_fail (folder && TNY_IS_FOLDER (folder));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	protocol_registry = modest_runtime_get_protocol_registry ();

	priv->total = tny_list_get_length (headers);
	priv->done = 0;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_RECEIVE;

	/* Apply folder rules */
	rules = modest_tny_folder_get_rules (TNY_FOLDER (folder));
	if (rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE) {
 		/* Set status failed and set an error */
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
			     _CS("ckct_ib_unable_to_paste_here"));
		/* Notify the queue */
		modest_mail_operation_notify_end (self);
		return;
	}
		
	/* Get source folder */
	iter = tny_list_create_iterator (headers);
	header = TNY_HEADER (tny_iterator_get_current (iter));
	if (header) {
		src_folder = tny_header_get_folder (header);
		g_object_unref (header);
	}
	g_object_unref (iter);

	if (src_folder == NULL) {
		/* Notify the queue */
		modest_mail_operation_notify_end (self);

		g_warning ("%s: cannot find folder from header", __FUNCTION__);
		return;
	}

	
	/* Check folder source and destination */
	if (src_folder == folder) {
 		/* Set status failed and set an error */
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_BAD_PARAMETER,
			     _("mail_in_ui_folder_copy_target_error"));
		
		/* Notify the queue */
		modest_mail_operation_notify_end (self);
		
		/* Free */
		g_object_unref (src_folder);		
		return;
	}

	/* Create the helper */
	helper = g_slice_new0 (XFerMsgsAsyncHelper);
	helper->mail_op = g_object_ref(self);
	helper->dest_folder = g_object_ref(folder);
	helper->user_callback = user_callback;
	helper->user_data = user_data;
	helper->last_total_bytes = 0;
	helper->sum_total_bytes = 0;
	helper->total_bytes = compute_message_list_size (headers, 0);

	/* Get account and set it into mail_operation */
	priv->account = modest_tny_folder_get_account (src_folder);
	dst_account = modest_tny_folder_get_account (folder);

	if (priv->account == dst_account) {
		/* Transfer all messages at once using the fast
		 * method. Note that depending on the server this
		 * might not be that fast, and might not be
		 * user-cancellable either */
		helper->headers = g_object_ref (headers);
		helper->more_msgs = NULL;
	} else {
		/* Transfer messages one by one so the user can cancel
		 * the operation */
		GObject *hdr;
		helper->headers = tny_simple_list_new ();
		helper->more_msgs = tny_list_create_iterator (headers);
		hdr = tny_iterator_get_current (helper->more_msgs);
		tny_list_append (helper->headers, hdr);
		g_object_unref (hdr);
	}

	/* If leave_on_server is set to TRUE then don't use
	   delete_original, we always pass FALSE. This is because
	   otherwise tinymail will try to sync the source folder and
	   this could cause an error if we're offline while
	   transferring an already downloaded message from a POP
	   account */
	account_protocol = modest_tny_account_get_protocol_type (priv->account);
        if (modest_protocol_registry_protocol_type_has_leave_on_server (protocol_registry, account_protocol)) {
		const gchar *account_name;

		account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (priv->account);
		leave_on_server = modest_account_mgr_get_leave_on_server (modest_runtime_get_account_mgr (),
									  account_name);
	} else {
		leave_on_server = FALSE;
	}

	/* Do not delete messages if leave on server is TRUE */
	helper->delete = (leave_on_server) ? FALSE : delete_original;

	modest_mail_operation_notify_start (self);

	/* Start notifying progress */
	state = modest_mail_operation_clone_state (self);
	state->done = 0;
	state->total = 0;
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, state, NULL);
	g_slice_free (ModestMailOperationState, state);

	tny_folder_transfer_msgs_async (src_folder, 
					helper->headers, 
					folder, 
					helper->delete, 
					transfer_msgs_cb, 
					transfer_msgs_status_cb,
					helper);
	g_object_unref (src_folder);
	g_object_unref (dst_account);
}


static void
on_refresh_folder (TnyFolder   *folder, 
		   gboolean     cancelled, 
		   GError     *error,
		   gpointer     user_data)
{
	RefreshAsyncHelper *helper = NULL;
	ModestMailOperation *self = NULL;
	ModestMailOperationPrivate *priv = NULL;

	helper = (RefreshAsyncHelper *) user_data;
	self = helper->mail_op;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	g_return_if_fail(priv!=NULL);

	if (error) {
		priv->error = g_error_copy (error);
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		goto out;
	}

	if (cancelled) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
			     _("Error trying to refresh the contents of %s"),
			     tny_folder_get_name (folder));
		goto out;
	}

	priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
 out:

	/* Call user defined callback, if it exists */
	if (helper->user_callback) {

		/* This is not a GDK lock because we are a Tinymail callback and
	 	 * Tinymail already acquires the Gdk lock */
		helper->user_callback (self, folder, helper->user_data);
	}

	/* Free */
	g_slice_free (RefreshAsyncHelper, helper);

	/* Notify about operation end */
	modest_mail_operation_notify_end (self);
	g_object_unref(self);
}

static void
on_refresh_folder_status_update (GObject *obj,
				 TnyStatus *status,
				 gpointer user_data)
{
	RefreshAsyncHelper *helper = NULL;
	ModestMailOperation *self = NULL;
	ModestMailOperationPrivate *priv = NULL;
	ModestMailOperationState *state;

	g_return_if_fail (user_data != NULL);
	g_return_if_fail (status != NULL);

	/* Show only the status information we want */
	if (status->code != TNY_FOLDER_STATUS_CODE_REFRESH)
		return;

	helper = (RefreshAsyncHelper *) user_data;
	self = helper->mail_op;
	g_return_if_fail (MODEST_IS_MAIL_OPERATION(self));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	priv->done = status->position;
	priv->total = status->of_total;

	state = modest_mail_operation_clone_state (self);

	/* This is not a GDK lock because we are a Tinymail callback and
	 * Tinymail already acquires the Gdk lock */
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, state, NULL);

	g_slice_free (ModestMailOperationState, state);
}

void 
modest_mail_operation_refresh_folder  (ModestMailOperation *self,
				       TnyFolder *folder,
				       RefreshAsyncUserCallback user_callback,
				       gpointer user_data)
{
	ModestMailOperationPrivate *priv = NULL;
	RefreshAsyncHelper *helper = NULL;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Check memory low */
	if (_check_memory_low (self)) {
		if (user_callback)
			user_callback (self, folder, user_data);
		/* Notify about operation end */
		modest_mail_operation_notify_end (self);
		return;
	}

	/* Get account and set it into mail_operation */
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->account = modest_tny_folder_get_account  (folder);
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_RECEIVE;

	/* Create the helper */
	helper = g_slice_new0 (RefreshAsyncHelper);
	helper->mail_op = g_object_ref(self);
	helper->user_callback = user_callback;
	helper->user_data = user_data;

	modest_mail_operation_notify_start (self);
	
	/* notify that the operation was started */
	ModestMailOperationState *state;
	state = modest_mail_operation_clone_state (self);
	state->done = 0;
	state->total = 0;
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 
			0, state, NULL);
	g_slice_free (ModestMailOperationState, state);
	
	tny_folder_refresh_async (folder,
				  on_refresh_folder,
				  on_refresh_folder_status_update,
				  helper);
}

static void
run_queue_notify_and_destroy (RunQueueHelper *helper,
			      ModestMailOperationStatus status)
{
	ModestMailOperationPrivate *priv;

	/* Disconnect */
	if (helper->error_handler &&
	    g_signal_handler_is_connected (helper->queue, helper->error_handler))
		g_signal_handler_disconnect (helper->queue, helper->error_handler);
	if (helper->start_handler &&
	    g_signal_handler_is_connected (helper->queue, helper->start_handler))
		g_signal_handler_disconnect (helper->queue, helper->start_handler);
	if (helper->stop_handler &&
	    g_signal_handler_is_connected (helper->queue, helper->stop_handler))
		g_signal_handler_disconnect (helper->queue, helper->stop_handler);

	/* Set status */
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (helper->self);
	priv->status = status;

	/* Notify end */
	modest_mail_operation_notify_end (helper->self);

	/* Free data */
	g_object_unref (helper->queue);
	g_object_unref (helper->self);
	g_slice_free (RunQueueHelper, helper);
}

static void
run_queue_stop (ModestTnySendQueue *queue,
		gpointer user_data)
{
	RunQueueHelper *helper;

	g_debug ("%s sending queue stopped", __FUNCTION__);

	helper = (RunQueueHelper *) user_data;
	run_queue_notify_and_destroy (helper, MODEST_MAIL_OPERATION_STATUS_SUCCESS);
}

void
modest_mail_operation_run_queue (ModestMailOperation *self,
				 ModestTnySendQueue *queue)
{
	ModestMailOperationPrivate *priv;
 	RunQueueHelper *helper;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (MODEST_IS_TNY_SEND_QUEUE (queue));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->account = TNY_ACCOUNT (tny_camel_send_queue_get_transport_account (TNY_CAMEL_SEND_QUEUE (queue)));
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_RUN_QUEUE;

	/* Create the helper */
	helper = g_slice_new0 (RunQueueHelper);
	helper->queue = g_object_ref (queue);
	helper->self = g_object_ref (self);
	helper->stop_handler = g_signal_connect (queue, "queue-stop", 
						 G_CALLBACK (run_queue_stop), 
						 helper);

	/* Notify operation has started */
	modest_mail_operation_notify_start (self);
	g_debug ("%s, run queue started", __FUNCTION__);
}

static void
queue_wakeup_callback (ModestTnySendQueue *queue,
		       gboolean cancelled,
		       GError *error,
		       gpointer userdata)
{
	ModestMailOperation *mail_op;
	ModestMailOperationPrivate *priv;

	mail_op = (ModestMailOperation *) userdata;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (mail_op);

	priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	tny_camel_send_queue_flush (TNY_CAMEL_SEND_QUEUE (queue));

	/* Notify end */
	modest_mail_operation_notify_end (mail_op);
	g_object_unref (mail_op);
}

void
modest_mail_operation_queue_wakeup (ModestMailOperation *self,
				    ModestTnySendQueue *queue)
{
  	ModestMailOperationPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (MODEST_IS_TNY_SEND_QUEUE (queue));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->account = TNY_ACCOUNT (tny_camel_send_queue_get_transport_account (TNY_CAMEL_SEND_QUEUE (queue)));
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_QUEUE_WAKEUP;

	g_object_ref (self);

	modest_tny_send_queue_wakeup (queue, queue_wakeup_callback, self);
	modest_mail_operation_notify_start (self);
}

static void
shutdown_callback (ModestTnyAccountStore *account_store, gpointer userdata)
{
	ModestMailOperation *self = (ModestMailOperation *) userdata;
	ModestMailOperationPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (MODEST_IS_TNY_ACCOUNT_STORE (account_store));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;

	modest_mail_operation_notify_end (self);
	g_object_unref (self);
}

void
modest_mail_operation_shutdown (ModestMailOperation *self, ModestTnyAccountStore *account_store)
{
	ModestMailOperationPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (MODEST_IS_TNY_ACCOUNT_STORE (account_store));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	modest_mail_operation_queue_set_running_shutdown (modest_runtime_get_mail_operation_queue ());

	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->account = NULL;
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_SHUTDOWN;

	modest_mail_operation_notify_start (self);
	g_object_ref (self);
	modest_tny_account_store_shutdown (account_store, shutdown_callback, self);
}

static void
sync_folder_finish_callback (TnyFolder *self,
			     gboolean cancelled,
			     GError *err,
			     gpointer user_data)

{
	ModestMailOperationPrivate *priv;
	SyncFolderHelper *helper;

	helper = (SyncFolderHelper *) user_data;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (helper->mail_op);

	/* If canceled by the user, ignore the error given by Tinymail */
	if (cancelled) {
		priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
	} else if (err) {
		/* If the operation was a sync then the status is
		   failed, but if it's part of another operation then
		   just set it as finished with errors */
		if (priv->op_type == MODEST_MAIL_OPERATION_TYPE_SYNC_FOLDER)
			priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		else
			priv->status = MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS;
		priv->error = g_error_copy ((const GError *) err);
		priv->error->domain = MODEST_MAIL_OPERATION_ERROR;
	} else {
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

	/* User callback */
	if (helper->user_callback)
		helper->user_callback (helper->mail_op, self, helper->user_data);

	modest_mail_operation_notify_end (helper->mail_op);

	/* Frees */
	g_object_unref (helper->mail_op);
	g_slice_free (SyncFolderHelper, helper);
}

void
modest_mail_operation_sync_folder (ModestMailOperation *self,
				   TnyFolder *folder,
				   gboolean expunge,
				   SyncFolderCallback callback,
				   gpointer user_data)
{
	ModestMailOperationPrivate *priv;
	SyncFolderHelper *helper;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER (folder));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	priv->account = modest_tny_folder_get_account (folder);
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_SYNC_FOLDER;

	/* Create helper */
	helper = g_slice_new0 (SyncFolderHelper);
	helper->mail_op = g_object_ref (self);
	helper->user_callback = callback;
	helper->user_data = user_data;

	modest_mail_operation_notify_start (self);
	tny_folder_sync_async (folder, expunge,
			       (TnyFolderCallback) sync_folder_finish_callback,
			       NULL, helper);
}

static void
modest_mail_operation_notify_start (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv = NULL;

	g_return_if_fail (self);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Ensure that all the fields are filled correctly */
	g_return_if_fail (priv->op_type != MODEST_MAIL_OPERATION_TYPE_UNKNOWN);

	/* Notify the observers about the mail operation. We do not
	   wrapp this emission because we assume that this function is
	   always called from within the main lock */
	g_signal_emit (G_OBJECT (self), signals[OPERATION_STARTED_SIGNAL], 0, NULL);
}

/**
 *
 * It's used by the mail operation queue to notify the observers
 * attached to that signal that the operation finished. We need to use
 * that because tinymail does not give us the progress of a given
 * operation when it finishes (it directly calls the operation
 * callback).
 */
static void
modest_mail_operation_notify_end (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv = NULL;

	g_return_if_fail (self);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Notify the observers about the mail operation end. We do
	   not wrapp this emission because we assume that this
	   function is always called from within the main lock */
	g_signal_emit (G_OBJECT (self), signals[OPERATION_FINISHED_SIGNAL], 0, NULL);

	/* Remove the error user data */
	if (priv->error_checking_user_data && priv->error_checking_user_data_destroyer)
		priv->error_checking_user_data_destroyer (priv->error_checking_user_data);
}

TnyAccount *
modest_mail_operation_get_account (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv = NULL;

	g_return_val_if_fail (self, NULL);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	return (priv->account) ? g_object_ref (priv->account) : NULL;
}

void
modest_mail_operation_noop (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv = NULL;

	g_return_if_fail (self);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	priv->op_type = MODEST_MAIL_OPERATION_TYPE_INFO;
	priv->done = 0;
	priv->total = 0;

	/* This mail operation does nothing actually */
	modest_mail_operation_notify_start (self);
	modest_mail_operation_notify_end (self);
}


gchar*
modest_mail_operation_to_string (ModestMailOperation *self)
{
	const gchar *type, *status, *account_id;
	ModestMailOperationPrivate *priv = NULL;
	
	g_return_val_if_fail (self, NULL);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* new operations don't have anything interesting */
	if (priv->op_type == MODEST_MAIL_OPERATION_TYPE_UNKNOWN)
		return g_strdup_printf ("%p <new operation>", self);
	
	switch (priv->op_type) {
	case MODEST_MAIL_OPERATION_TYPE_SEND:    type= "SEND";    break;
	case MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE:    type= "SEND-AND-RECEIVE";    break;
	case MODEST_MAIL_OPERATION_TYPE_RECEIVE: type= "RECEIVE"; break;
	case MODEST_MAIL_OPERATION_TYPE_OPEN:    type= "OPEN";    break;
	case MODEST_MAIL_OPERATION_TYPE_DELETE:  type= "DELETE";  break;
	case MODEST_MAIL_OPERATION_TYPE_INFO:    type= "INFO";    break;
	case MODEST_MAIL_OPERATION_TYPE_RUN_QUEUE: type= "RUN-QUEUE"; break;
	case MODEST_MAIL_OPERATION_TYPE_SYNC_FOLDER: type= "SYNC-FOLDER"; break;
	case MODEST_MAIL_OPERATION_TYPE_SHUTDOWN: type= "SHUTDOWN"; break;
	case MODEST_MAIL_OPERATION_TYPE_UNKNOWN: type= "UNKNOWN"; break;
	default: type = "UNEXPECTED"; break;
	}

	switch (priv->status) {
	case MODEST_MAIL_OPERATION_STATUS_INVALID:              status= "INVALID"; break;
	case MODEST_MAIL_OPERATION_STATUS_SUCCESS:              status= "SUCCESS"; break;
	case MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS: status= "FINISHED-WITH-ERRORS"; break;
	case MODEST_MAIL_OPERATION_STATUS_FAILED:               status= "FAILED"; break;
	case MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS:          status= "IN-PROGRESS"; break;
	case MODEST_MAIL_OPERATION_STATUS_CANCELED:             status= "CANCELLED"; break;
	default:                                                status= "UNEXPECTED"; break;
	} 

	account_id = priv->account ? tny_account_get_id (priv->account) : "";

	return g_strdup_printf ("%p \"%s\" (%s) [%s] {%d/%d} '%s'", self, account_id,type, status,
				priv->done, priv->total,
				priv->error && priv->error->message ? priv->error->message : "");
}

/* 
 * Once the mail operations were objects this will be no longer
 * needed. I don't like it, but we need it for the moment
 */
static gboolean
_check_memory_low (ModestMailOperation *mail_op)
{
	if (modest_platform_check_memory_low (NULL, FALSE)) {
		ModestMailOperationPrivate *priv;

		priv = MODEST_MAIL_OPERATION_GET_PRIVATE (mail_op);
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error),
			     MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_LOW_MEMORY,
			     "Not enough memory to complete the operation");
		return TRUE;
	} else {
		return FALSE;
	}
}

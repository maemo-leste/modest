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

#include "modest-mail-operation.h"
/* include other impl specific header files */
#include <string.h>
#include <stdarg.h>
#include <tny-mime-part.h>
#include <tny-store-account.h>
#include <tny-folder-store.h>
#include <tny-folder-store-query.h>
#include <tny-camel-stream.h>
#include <tny-simple-list.h>
#include <tny-send-queue.h>
#include <tny-status.h>
#include <camel/camel-stream-mem.h>
#include <glib/gi18n.h>
#include <modest-tny-account.h>
#include <modest-tny-send-queue.h>
#include <modest-runtime.h>
#include "modest-text-utils.h"
#include "modest-tny-msg.h"
#include "modest-tny-folder.h"
#include "modest-tny-platform-factory.h"
#include "modest-marshal.h"
#include "modest-error.h"

/* 'private'/'protected' functions */
static void modest_mail_operation_class_init (ModestMailOperationClass *klass);
static void modest_mail_operation_init       (ModestMailOperation *obj);
static void modest_mail_operation_finalize   (GObject *obj);

static void     update_folders_cb    (TnyFolderStore *self, 
				      TnyList *list, 
				      GError **err, 
				      gpointer user_data);
static void     update_folders_status_cb (GObject *obj,
					  TnyStatus *status,  
					  gpointer user_data);

static void     update_process_msg_status_cb (GObject *obj,
					      TnyStatus *status,  
					      gpointer user_data);
static void     get_msg_cb (TnyFolder *folder, 
			    gboolean cancelled, 
			    TnyMsg *msg, 
			    GError **err, 
			    gpointer user_data);

static void     get_msg_status_cb (GObject *obj,
				   TnyStatus *status,  
				   gpointer user_data);


enum _ModestMailOperationSignals 
{
	PROGRESS_CHANGED_SIGNAL,

	NUM_SIGNALS
};

typedef struct _ModestMailOperationPrivate ModestMailOperationPrivate;
struct _ModestMailOperationPrivate {
	guint                      done;
	guint                      total;
	ModestMailOperationStatus  status;	
	ModestMailOperationId      id;		
	GObject                   *source;
	GError                    *error;
};

#define MODEST_MAIL_OPERATION_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_MAIL_OPERATION, \
                                                   ModestMailOperationPrivate))

#define CHECK_EXCEPTION(priv, new_status)  if (priv->error) {\
                                                   priv->status = new_status;\
                                               }

typedef struct _GetMsgAsyncHelper {	
	ModestMailOperation *mail_op;
	GetMsgAsynUserCallback user_callback;	
	guint pending_ops;
	gpointer user_data;
} GetMsgAsyncHelper;

typedef struct _RefreshFolderAsyncHelper
{
	ModestMailOperation *mail_op;
	TnyIterator *iter;
	guint failed;
	guint canceled;

} RefreshFolderAsyncHelper;

typedef struct _XFerMsgAsyncHelper
{
	ModestMailOperation *mail_op;
	TnyList *headers;
	TnyFolder *dest_folder;

} XFerMsgAsyncHelper;


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
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
modest_mail_operation_init (ModestMailOperation *obj)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);

	priv->status   = MODEST_MAIL_OPERATION_STATUS_INVALID;
	priv->id       = MODEST_MAIL_OPERATION_ID_UNKNOWN;
	priv->error    = NULL;
	priv->done     = 0;
	priv->total    = 0;
	priv->source = NULL;
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

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestMailOperation*
modest_mail_operation_new (ModestMailOperationId id, 
			   GObject *source)
{
	ModestMailOperation *obj;
	ModestMailOperationPrivate *priv;
		
	obj = MODEST_MAIL_OPERATION(g_object_new(MODEST_TYPE_MAIL_OPERATION, NULL));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);

	priv->id = id;
	if (source != NULL)
		priv->source = g_object_ref(source);

	return obj;
}


ModestMailOperationId
modest_mail_operation_get_id (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	
	return priv->id;
}

gboolean 
modest_mail_operation_is_mine (ModestMailOperation *self, 
			       GObject *me)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	if (priv->source == NULL) return FALSE;

	return priv->source == me;
}


void
modest_mail_operation_send_mail (ModestMailOperation *self,
				 TnyTransportAccount *transport_account,
				 TnyMsg* msg)
{
	TnySendQueue *send_queue;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));
	g_return_if_fail (TNY_IS_MSG (msg));
	
	send_queue = TNY_SEND_QUEUE (modest_runtime_get_send_queue (transport_account));
	if (!TNY_IS_SEND_QUEUE(send_queue))
		g_printerr ("modest: could not find send queue for account\n");
	else {
		GError *err = NULL;
		tny_send_queue_add (send_queue, msg, &err);
		if (err) {
			g_printerr ("modest: error adding msg to send queue: %s\n",
				    err->message);
			g_error_free (err);
		} else {
			/* g_message ("modest: message added to send queue"); */
		}
	}

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);
}

void
modest_mail_operation_send_new_mail (ModestMailOperation *self,
				     TnyTransportAccount *transport_account,
				     const gchar *from,  const gchar *to,
				     const gchar *cc,  const gchar *bcc,
				     const gchar *subject, const gchar *plain_body,
				     const gchar *html_body,
				     const GList *attachments_list,
				     TnyHeaderFlags priority_flags)
{
	TnyMsg *new_msg;
	ModestMailOperationPrivate *priv = NULL;
	/* GList *node = NULL; */

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Check parametters */
	if (to == NULL) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_BAD_PARAMETER,
			     _("Error trying to send a mail. You need to set at least one recipient"));
		return;
	}

	if (html_body == NULL) {
		new_msg = modest_tny_msg_new (to, from, cc, bcc, subject, plain_body, (GSList *) attachments_list); /* FIXME: attachments */
	} else {
		new_msg = modest_tny_msg_new_html_plain (to, from, cc, bcc, subject, html_body, plain_body, (GSList *) attachments_list);
	}
	if (!new_msg) {
		g_printerr ("modest: failed to create a new msg\n");
		return;
	}

	/* TODO: add priority handling. It's received in the priority_flags operator, and
	   it should have effect in the sending operation */

	/* Call mail operation */
	modest_mail_operation_send_mail (self, transport_account, new_msg);

	/* Free */
	g_object_unref (G_OBJECT (new_msg));
}

void
modest_mail_operation_save_to_drafts (ModestMailOperation *self,
				      TnyTransportAccount *transport_account,
				      const gchar *from,  const gchar *to,
				      const gchar *cc,  const gchar *bcc,
				      const gchar *subject, const gchar *plain_body,
				      const gchar *html_body,
				      const GList *attachments_list,
				      TnyHeaderFlags priority_flags)
{
	TnyMsg *msg = NULL;
	TnyFolder *folder = NULL;
	ModestMailOperationPrivate *priv = NULL;
	GError *err = NULL;

	/* GList *node = NULL; */

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	if (html_body == NULL) {
		msg = modest_tny_msg_new (to, from, cc, bcc, subject, plain_body, (GSList *) attachments_list); /* FIXME: attachments */
	} else {
		msg = modest_tny_msg_new_html_plain (to, from, cc, bcc, subject, html_body, plain_body, (GSList *) attachments_list);
	}
	if (!msg) {
		g_printerr ("modest: failed to create a new msg\n");
		goto cleanup;
	}

	folder = modest_tny_account_get_special_folder (TNY_ACCOUNT (transport_account), TNY_FOLDER_TYPE_DRAFTS);
	if (!folder) {
		g_printerr ("modest: failed to find Drafts folder\n");
		goto cleanup;
	}
	
	tny_folder_add_msg (folder, msg, &err);
	if (err) {
		g_printerr ("modest: error adding msg to Drafts folder: %s",
			    err->message);
		g_error_free (err);
		goto cleanup;
	}

	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);

	/* Free */
cleanup:
	if (msg)
		g_object_unref (G_OBJECT(msg));
	if (folder)
		g_object_unref (G_OBJECT(folder));
}

static void
recurse_folders (TnyFolderStore *store, TnyFolderStoreQuery *query, TnyList *all_folders)
{
	TnyIterator *iter;
	TnyList *folders = tny_simple_list_new ();

	tny_folder_store_get_folders (store, folders, query, NULL);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter)) {

		TnyFolderStore *folder = (TnyFolderStore*) tny_iterator_get_current (iter);

		tny_list_prepend (all_folders, G_OBJECT (folder));

		recurse_folders (folder, query, all_folders);
	    
 		g_object_unref (G_OBJECT (folder));

		tny_iterator_next (iter);
	}
	 g_object_unref (G_OBJECT (iter));
	 g_object_unref (G_OBJECT (folders));
}

static void
update_folders_status_cb (GObject *obj,
			  TnyStatus *status,  
			  gpointer user_data)
{
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (status->code == TNY_FOLDER_STATUS_CODE_REFRESH);

	/* Temporary FIX: useful when tinymail send us status
	   information *after* calling the function callback */
	if (!MODEST_IS_MAIL_OPERATION (user_data))
		return;

	self = MODEST_MAIL_OPERATION (user_data);
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	priv->done = status->position;
	priv->total = status->of_total;

	if (priv->done == 1 && priv->total == 100)
		return;

	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}

static void
update_folders_cb (TnyFolderStore *folder_store, TnyList *list, GError **err, gpointer user_data)
{
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;
	TnyIterator *iter;
	TnyList *all_folders;
	
	self  = MODEST_MAIL_OPERATION (user_data);
	priv  = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* g_message (__FUNCTION__); */
	
	if (*err) {
		priv->error = g_error_copy (*err);
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		goto out;
	}

	/* Get all the folders We can do it synchronously because
	   we're already running in a different thread than the UI */
	all_folders = tny_list_copy (list);
	iter = tny_list_create_iterator (all_folders);
	while (!tny_iterator_is_done (iter)) {
		TnyFolderStore *folder = TNY_FOLDER_STORE (tny_iterator_get_current (iter));

		recurse_folders (folder, NULL, all_folders);
		tny_iterator_next (iter);
	}
	g_object_unref (G_OBJECT (iter));

	/* Refresh folders */
	iter = tny_list_create_iterator (all_folders);
	priv->total = tny_list_get_length (all_folders);

	while (!tny_iterator_is_done (iter) && !priv->error) {

		TnyFolderStore *folder = TNY_FOLDER_STORE (tny_iterator_get_current (iter));

		/* Refresh the folder */
		tny_folder_refresh (TNY_FOLDER (folder), &(priv->error));

		if (priv->error) {
			priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		} else {	
			/* Update status and notify */
			priv->done++;
			g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
		}
    
		g_object_unref (G_OBJECT (folder));
	    
		tny_iterator_next (iter);
	}

	g_object_unref (G_OBJECT (iter));
 out:
	g_object_unref (G_OBJECT (list));

	/* Check if the operation was a success */
	if (priv->done == priv->total && !priv->error)
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;

	/* Free */
	g_object_unref (G_OBJECT (folder_store));

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);
}

gboolean
modest_mail_operation_update_account (ModestMailOperation *self,
				      TnyStoreAccount *store_account)
{
	ModestMailOperationPrivate *priv;
	TnyList *folders;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), FALSE);
	g_return_val_if_fail (TNY_IS_STORE_ACCOUNT(store_account), FALSE);

	/* Pick async call reference */
	g_object_ref (store_account);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	priv->total = 0;
	priv->done  = 0;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;
	
	/* Get subscribed folders & refresh them */
    	folders = TNY_LIST (tny_simple_list_new ());

	/* g_message ("tny_folder_store_get_folders_async"); */
	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (store_account),
					    folders, update_folders_cb, NULL, update_folders_status_cb, self);
	
	return TRUE;
}

ModestMailOperationStatus
modest_mail_operation_get_status (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self, MODEST_MAIL_OPERATION_STATUS_INVALID);
	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self),
			      MODEST_MAIL_OPERATION_STATUS_INVALID);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	return priv->status;
}

const GError *
modest_mail_operation_get_error (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), NULL);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	return priv->error;
}

gboolean 
modest_mail_operation_cancel (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	if (!MODEST_IS_MAIL_OPERATION (self)) {
		g_warning ("%s: invalid parametter", G_GNUC_FUNCTION);
		return FALSE;
	}

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* TODO: Tinymail does not support cancel operation  */
	
	/* Set new status */
	priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);

	return TRUE;
}

guint 
modest_mail_operation_get_task_done (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), 0);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	return priv->done;
}

guint 
modest_mail_operation_get_task_total (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), 0);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	return priv->total;
}

gboolean
modest_mail_operation_is_finished (ModestMailOperation *self)
{
	ModestMailOperationPrivate *priv;
	gboolean retval = FALSE;

	if (!MODEST_IS_MAIL_OPERATION (self)) {
		g_warning ("%s: invalid parametter", G_GNUC_FUNCTION);
		return retval;
	}

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

/* ******************************************************************* */
/* ************************** STORE  ACTIONS ************************* */
/* ******************************************************************* */


TnyFolder *
modest_mail_operation_create_folder (ModestMailOperation *self,
				     TnyFolderStore *parent,
				     const gchar *name)
{
	ModestTnyFolderRules rules;
	ModestMailOperationPrivate *priv;
	TnyFolder *new_folder = NULL;
	gboolean can_create = FALSE;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), NULL);
	g_return_val_if_fail (name, NULL);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* Check parent */
	if (!TNY_IS_FOLDER (parent)) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_BAD_PARAMETER,
			     _("mail_in_ui_folder_create_error"));
	} else {
		/* Check folder rules */
		rules = modest_tny_folder_get_rules (TNY_FOLDER (parent));
		if (rules & MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE)
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
				     _("mail_in_ui_folder_create_error"));
		else
			can_create = TRUE;		
	}

	if (can_create) {
		/* Create the folder */
		new_folder = tny_folder_store_create_folder (parent, name, &(priv->error));
		CHECK_EXCEPTION (priv, MODEST_MAIL_OPERATION_STATUS_FAILED);
	}

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);

	return new_folder;
}

void
modest_mail_operation_remove_folder (ModestMailOperation *self,
				     TnyFolder           *folder,
				     gboolean             remove_to_trash)
{
	TnyAccount *account;
	ModestMailOperationPrivate *priv;
	ModestTnyFolderRules rules;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER (folder));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* Check folder rules */
	rules = modest_tny_folder_get_rules (TNY_FOLDER (folder));
	if (rules & MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
			     _("mail_in_ui_folder_delete_error"));
		goto end;
	}

	/* Get the account */
	account = tny_folder_get_account (folder);

	/* Delete folder or move to trash */
	if (remove_to_trash) {
		TnyFolder *trash_folder, *new_folder;
		trash_folder = modest_tny_account_get_special_folder (account,
								      TNY_FOLDER_TYPE_TRASH);
		/* TODO: error_handling */
		new_folder = modest_mail_operation_xfer_folder (self, folder, 
								TNY_FOLDER_STORE (trash_folder), TRUE);
		g_object_unref (G_OBJECT (new_folder));
	} else {
		TnyFolderStore *parent = tny_folder_get_folder_store (folder);

		tny_folder_store_remove_folder (parent, folder, &(priv->error));
		CHECK_EXCEPTION (priv, MODEST_MAIL_OPERATION_STATUS_FAILED);

		if (parent)
			g_object_unref (G_OBJECT (parent));
	}
	g_object_unref (G_OBJECT (account));

 end:
	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);
}

void
modest_mail_operation_rename_folder (ModestMailOperation *self,
				     TnyFolder *folder,
				     const gchar *name)
{
	ModestMailOperationPrivate *priv;
	ModestTnyFolderRules rules;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER_STORE (folder));
	g_return_if_fail (name);
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* Check folder rules */
	rules = modest_tny_folder_get_rules (TNY_FOLDER (folder));
	if (rules & MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
			     _("FIXME: unable to rename"));
	} else {
		/* Rename. Camel handles folder subscription/unsubscription */
		tny_folder_set_name (folder, name, &(priv->error));
		CHECK_EXCEPTION (priv, MODEST_MAIL_OPERATION_STATUS_FAILED);
	}

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);
 }

TnyFolder *
modest_mail_operation_xfer_folder (ModestMailOperation *self,
				   TnyFolder *folder,
				   TnyFolderStore *parent,
				   gboolean delete_original)
{
	ModestMailOperationPrivate *priv;
	TnyFolder *new_folder = NULL;
	ModestTnyFolderRules rules;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), NULL);
	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), NULL);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), NULL);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* The moveable restriction is applied also to copy operation */
	rules = modest_tny_folder_get_rules (TNY_FOLDER (parent));
	if (rules & MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_FOLDER_RULES,
			     _("FIXME: unable to rename"));
	} else {
		/* Move/Copy folder */
		new_folder = tny_folder_copy (folder,
					      parent,
					      tny_folder_get_name (folder),
					      delete_original, 
					      &(priv->error));
	}

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);

	return new_folder;
}


/* ******************************************************************* */
/* **************************  MSG  ACTIONS  ************************* */
/* ******************************************************************* */

void          modest_mail_operation_get_msg     (ModestMailOperation *self,
						 TnyHeader *header,
						 GetMsgAsynUserCallback user_callback,
						 gpointer user_data)
{
	GetMsgAsyncHelper *helper = NULL;
	TnyFolder *folder;
	ModestMailOperationPrivate *priv;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_HEADER (header));
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	folder = tny_header_get_folder (header);

	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	/* Get message from folder */
	if (folder) {
		helper = g_slice_new0 (GetMsgAsyncHelper);
		helper->mail_op = self;
		helper->user_callback = user_callback;
		helper->pending_ops = 1;
		helper->user_data = user_data;

		tny_folder_get_msg_async (folder, header, get_msg_cb, get_msg_status_cb, helper);

		g_object_unref (G_OBJECT (folder));
	} else {
 		/* Set status failed and set an error */
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
			     _("Error trying to get a message. No folder found for header"));
	}
}

static void
get_msg_cb (TnyFolder *folder, 
	    gboolean cancelled, 
	    TnyMsg *msg, 
	    GError **error, 
	    gpointer user_data)
{
	GetMsgAsyncHelper *helper = NULL;
	ModestMailOperation *self = NULL;
	ModestMailOperationPrivate *priv = NULL;

	helper = (GetMsgAsyncHelper *) user_data;
	g_return_if_fail (helper != NULL);       
	self = helper->mail_op;
	g_return_if_fail (MODEST_IS_MAIL_OPERATION(self));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	
	helper->pending_ops--;

	/* Check errors and cancel */
	if (*error) {
		priv->error = g_error_copy (*error);
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

	/* If user defined callback function was defined, call it */
	if (helper->user_callback) {
		helper->user_callback (priv->source, msg, helper->user_data);
	}

	/* Free */
 out:
	if (helper->pending_ops == 0) {
		g_slice_free (GetMsgAsyncHelper, helper);
		
		/* Notify the queue */
		modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);	
	}
}

static void     
get_msg_status_cb (GObject *obj,
		   TnyStatus *status,  
		   gpointer user_data)
{
	GetMsgAsyncHelper *helper = NULL;
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (status->code == TNY_FOLDER_STATUS_CODE_GET_MSG);

	helper = (GetMsgAsyncHelper *) user_data;
	g_return_if_fail (helper != NULL);       

	/* Temporary FIX: useful when tinymail send us status
	   information *after* calling the function callback */
	if (!MODEST_IS_MAIL_OPERATION (helper->mail_op))
		return;

	self = helper->mail_op;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	priv->done += status->position;
	priv->total = status->of_total;

	if (priv->done == 1 && priv->total == 100)
		return;

	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}


void          modest_mail_operation_process_msg     (ModestMailOperation *self,
						     TnyList *header_list, 
						     GetMsgAsynUserCallback user_callback,
						     gpointer user_data)
{
	ModestMailOperationPrivate *priv = NULL;
	GetMsgAsyncHelper *helper = NULL;
	TnyHeader *header = NULL;
	TnyFolder *folder = NULL;
	TnyIterator *iter = NULL;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	iter = tny_list_create_iterator (header_list); 
	priv->total = tny_list_get_length(header_list);

	helper = g_slice_new0 (GetMsgAsyncHelper);
	helper->mail_op = self;
	helper->user_callback = user_callback;
	helper->pending_ops = priv->total;
	helper->user_data = user_data;

	while (!tny_iterator_is_done (iter)) { 
		
		header = TNY_HEADER (tny_iterator_get_current (iter));		
		folder = tny_header_get_folder (header);
				
		/* Get message from folder */
		if (folder) {
			/* The callback will call it per each header */
			tny_folder_get_msg_async (folder, header, get_msg_cb, update_process_msg_status_cb, helper);
			g_object_unref (G_OBJECT (folder));
		} else {			
			/* Set status failed and set an error */
			priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
				     _("Error trying to get a message. No folder found for header"));

			/* Notify the queue */
			modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);

			/* free */
			g_slice_free (GetMsgAsyncHelper, helper);
			break;
		}

		g_object_unref (header);		
		tny_iterator_next (iter);
	}
}

static void     
update_process_msg_status_cb (GObject *obj,
			      TnyStatus *status,  
			      gpointer user_data)
{
	GetMsgAsyncHelper *helper = NULL;
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (status->code == TNY_FOLDER_STATUS_CODE_GET_MSG);

	helper = (GetMsgAsyncHelper *) user_data;
	g_return_if_fail (helper != NULL);       

	/* Temporary FIX: useful when tinymail send us status
	   information *after* calling the function callback */
	if (!MODEST_IS_MAIL_OPERATION (helper->mail_op))
		return;

	self = helper->mail_op;
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	if (status->of_total > 0)
		priv->done += status->position/status->of_total;

	g_print("TEST: %d/%d", priv->done, priv->total); 

	if (priv->done == 1 && priv->total == 100)
		return;

	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}



void 
modest_mail_operation_remove_msg (ModestMailOperation *self,
				  TnyHeader *header,
				  gboolean remove_to_trash)
{
	TnyFolder *folder;
	ModestMailOperationPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_HEADER (header));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);
	folder = tny_header_get_folder (header);

	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	/* Delete or move to trash */
	if (remove_to_trash) {
		TnyFolder *trash_folder;
		TnyStoreAccount *store_account;

		store_account = TNY_STORE_ACCOUNT (tny_folder_get_account (folder));
		trash_folder = modest_tny_account_get_special_folder (TNY_ACCOUNT(store_account),
								      TNY_FOLDER_TYPE_TRASH);
		if (trash_folder) {
			TnyList *headers;

			/* Create list */
			headers = tny_simple_list_new ();
			tny_list_append (headers, G_OBJECT (header));
			g_object_unref (header);

			/* Move to trash */
			modest_mail_operation_xfer_msgs (self, headers, trash_folder, TRUE);
			g_object_unref (headers);
/* 			g_object_unref (trash_folder); */
		} else {
			ModestMailOperationPrivate *priv;

			/* Set status failed and set an error */
			priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
			priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
			g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
				     MODEST_MAIL_OPERATION_ERROR_ITEM_NOT_FOUND,
				     _("Error trying to delete a message. Trash folder not found"));
		}

		g_object_unref (G_OBJECT (store_account));
	} else {
		tny_folder_remove_msg (folder, header, &(priv->error));
		if (!priv->error)
			tny_folder_sync(folder, TRUE, &(priv->error));
	}

	/* Set status */
	if (!priv->error)
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	else
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;

	/* Free */
	g_object_unref (G_OBJECT (folder));

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);
}

static void
transfer_msgs_status_cb (GObject *obj,
			 TnyStatus *status,  
			 gpointer user_data)
{
}


static void
transfer_msgs_cb (TnyFolder *folder, GError **err, gpointer user_data)
{
	XFerMsgAsyncHelper *helper;
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;

	helper = (XFerMsgAsyncHelper *) user_data;
	self = helper->mail_op;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (*err) {
		priv->error = g_error_copy (*err);
		priv->done = 0;
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
	} else {
		priv->done = 1;
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

	/* Free */
	g_object_unref (helper->headers);
	g_object_unref (helper->dest_folder);
	g_object_unref (folder);
	g_free (helper);

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);
}

void
modest_mail_operation_xfer_msgs (ModestMailOperation *self,
				 TnyList *headers, 
				 TnyFolder *folder, 
				 gboolean delete_original)
{
	ModestMailOperationPrivate *priv;
	TnyIterator *iter;
	TnyFolder *src_folder;
	XFerMsgAsyncHelper *helper;
	TnyHeader *header;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_LIST (headers));
	g_return_if_fail (TNY_IS_FOLDER (folder));

	/* Pick references for async calls */
	g_object_ref (folder);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	priv->total = 1;
	priv->done = 0;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	/* Create the helper */
	helper = g_malloc0 (sizeof (XFerMsgAsyncHelper));
	helper->mail_op = self;
	helper->dest_folder = folder;
	helper->headers = headers;

	/* Get source folder */
	iter = tny_list_create_iterator (headers);
	header = TNY_HEADER (tny_iterator_get_current (iter));
	src_folder = tny_header_get_folder (header);
	g_object_unref (header);
	g_object_unref (iter);

	/* Transfer messages */
	tny_folder_transfer_msgs_async (src_folder, 
					headers, 
					folder, 
					delete_original, 
					transfer_msgs_cb, 
					transfer_msgs_status_cb, 
					helper);
}


static void
on_refresh_folder (TnyFolder   *folder, 
		   gboolean     cancelled, 
		   GError     **error,
		   gpointer     user_data)
{
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;

	self = MODEST_MAIL_OPERATION (user_data);
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	if (*error) {
		priv->error = g_error_copy (*error);
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
	/* Free */
	g_object_unref (folder);

	/* Notify the queue */
	modest_mail_operation_queue_remove (modest_runtime_get_mail_operation_queue (), self);
}

static void
on_refresh_folder_status_update (GObject *obj,
				 TnyStatus *status,
				 gpointer user_data)
{
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (status->code == TNY_FOLDER_STATUS_CODE_REFRESH);

	/* Temporary FIX: useful when tinymail send us status
	   information *after* calling the function callback */
	if (!MODEST_IS_MAIL_OPERATION (user_data))
		return;

	self = MODEST_MAIL_OPERATION (user_data);
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	priv->done = status->position;
	priv->total = status->of_total;

	if (priv->done == 1 && priv->total == 100)
		return;

	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}

void 
modest_mail_operation_refresh_folder  (ModestMailOperation *self,
				       TnyFolder *folder)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Pick a reference */
	g_object_ref (folder);

	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	/* Refresh the folder. TODO: tinymail could issue a status
	   updates before the callback call then this could happen. We
	   must review the design */
	tny_folder_refresh_async (folder,
				  on_refresh_folder,
				  on_refresh_folder_status_update,
				  self);
}

void
_modest_mail_operation_notify_end (ModestMailOperation *self)
{
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}

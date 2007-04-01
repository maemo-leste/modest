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
#include <camel/camel-stream-mem.h>
#include <glib/gi18n.h>
#include <modest-tny-account.h>
#include <modest-tny-send-queue.h>
#include <modest-runtime.h>
#include "modest-text-utils.h"
#include "modest-tny-msg.h"
#include "modest-tny-platform-factory.h"
#include "modest-marshal.h"
#include "modest-error.h"

/* 'private'/'protected' functions */
static void modest_mail_operation_class_init (ModestMailOperationClass *klass);
static void modest_mail_operation_init       (ModestMailOperation *obj);
static void modest_mail_operation_finalize   (GObject *obj);

static void     status_update_cb     (TnyFolder *folder, 
				      const gchar *what, 
				      gint status, 
				      gint oftotal,
				      gpointer user_data);
static void     folder_refresh_cb    (TnyFolder *folder, 
				      gboolean canceled,
				      GError **err,
				      gpointer user_data);
static void     update_folders_cb    (TnyFolderStore *self, 
				      TnyList *list, 
				      GError **err, 
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
	GError                    *error;
};

#define MODEST_MAIL_OPERATION_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_MAIL_OPERATION, \
                                                   ModestMailOperationPrivate))

#define CHECK_EXCEPTION(priv, new_status, op)  if (priv->error) {\
                                                   priv->status = new_status;\
                                                   op;\
                                               }

typedef struct _RefreshFolderAsyncHelper
{
	ModestMailOperation *mail_op;
	TnyIterator *iter;
	guint failed;
	guint canceled;

} RefreshFolderAsyncHelper;

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
		g_signal_new ("progress_changed",
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
	priv->error    = NULL;
	priv->done     = 0;
	priv->total    = 0;
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

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestMailOperation*
modest_mail_operation_new (void)
{
	return MODEST_MAIL_OPERATION(g_object_new(MODEST_TYPE_MAIL_OPERATION, NULL));
}


void
modest_mail_operation_send_mail (ModestMailOperation *self,
				 TnyTransportAccount *transport_account,
				 TnyMsg* msg)
{
	TnySendQueue *send_queue;
	
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));
	
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
		} else
			g_message ("modest: message added to send queue");
	}
}

void
modest_mail_operation_send_new_mail (ModestMailOperation *self,
				     TnyTransportAccount *transport_account,
				     const gchar *from,  const gchar *to,
				     const gchar *cc,  const gchar *bcc,
				     const gchar *subject, const gchar *plain_body,
				     const gchar *html_body,
				     const GList *attachments_list)
{
	TnyMsg *new_msg;
	ModestMailOperationPrivate *priv = NULL;
	GList *node = NULL;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Check parametters */
	if (to == NULL) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_MISSING_PARAMETER,
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
	
	modest_mail_operation_send_mail (self, transport_account, new_msg);

	g_object_unref (G_OBJECT(new_msg));
}

static void
status_update_cb (TnyFolder *folder, const gchar *what, gint status, gint oftotal, gpointer user_data) 
{
	g_print ("%s status: %d, of total %d\n", what, status, oftotal);
}

static void
folder_refresh_cb (TnyFolder *folder, gboolean canceled, GError **err, gpointer user_data)
{
	ModestMailOperation *self = NULL;
	ModestMailOperationPrivate *priv = NULL;
	RefreshFolderAsyncHelper *helper;

	helper = (RefreshFolderAsyncHelper *) user_data;
	self = MODEST_MAIL_OPERATION (helper->mail_op);
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	if ((canceled && *err) || *err) {
		priv->error = g_error_copy (*err);
		helper->failed++;
	} else if (canceled) {
		helper->canceled++;
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_OPERATION_CANCELED,
			     _("Error trying to refresh folder %s. Operation canceled"),
			     tny_folder_get_name (folder));
	} else {
		priv->done++;
	}

	if (priv->done == priv->total)
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	else if ((priv->done + helper->canceled + helper->failed) == priv->total) {
		if (helper->failed == priv->total)
			priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		else if (helper->failed == priv->total)
			priv->status = MODEST_MAIL_OPERATION_STATUS_CANCELED;
		else
			priv->status = MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS;
	}
	tny_iterator_next (helper->iter);
	if (tny_iterator_is_done (helper->iter)) {
		TnyList *list;
		list = tny_iterator_get_list (helper->iter);
		g_object_unref (G_OBJECT (helper->iter));
		g_object_unref (G_OBJECT (list));
		g_slice_free (RefreshFolderAsyncHelper, helper);
	} else {
		TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (helper->iter));
		if (folder) {
			g_message ("modest: refreshing folder %s",
				   tny_folder_get_name (folder));
			tny_folder_refresh_async (folder, folder_refresh_cb, status_update_cb, helper);
			g_object_unref (G_OBJECT(folder)); // FIXME: don't unref yet
		}
	}
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}


static void
update_folders_cb (TnyFolderStore *folder_store, TnyList *list, GError **err, gpointer user_data)
{
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;
	RefreshFolderAsyncHelper *helper;
	TnyFolder *folder;
	
	self  = MODEST_MAIL_OPERATION (user_data);
	priv  = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	if (*err) {
		priv->error = g_error_copy (*err);
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
		return;
	}

	priv->total = tny_list_get_length (list);
	priv->done = 0;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	helper = g_slice_new0 (RefreshFolderAsyncHelper);
	helper->mail_op = self;
	helper->iter = tny_list_create_iterator (list);
	helper->failed = 0;
	helper->canceled = 0;

	/* Async refresh folders */
	folder = TNY_FOLDER (tny_iterator_get_current (helper->iter));
	if (folder) {
		g_message ("modest: refreshing folder %s", tny_folder_get_name (folder));
		tny_folder_refresh_async (folder, folder_refresh_cb,
					  status_update_cb, helper);
	}
	//g_object_unref (G_OBJECT(folder)); /* FIXME -==> don't unref yet... */
}

gboolean
modest_mail_operation_update_account (ModestMailOperation *self,
				      TnyStoreAccount *store_account)
{
	ModestMailOperationPrivate *priv;
	TnyList *folders;
	TnyFolderStoreQuery *query;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), FALSE);
	g_return_val_if_fail (TNY_IS_STORE_ACCOUNT(store_account), FALSE);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Get subscribed folders & refresh them */
    	folders = TNY_LIST (tny_simple_list_new ());
	query = NULL; //tny_folder_store_query_new ();

	/* FIXME: is this needed? */
//	tny_device_force_online (TNY_DEVICE(modest_runtime_get_device()));
	
	/* FIXME: let query be NULL: do it for all */ 
	//tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (store_account),
					    folders, update_folders_cb, query, self);
	//g_object_unref (query); /* FIXME */
	
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
	/* TODO */
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
	ModestMailOperationPrivate *priv;
	TnyFolder *new_folder = NULL;
	//TnyStoreAccount *store_account;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), NULL);
	g_return_val_if_fail (name, NULL);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* Create the folder */
	new_folder = tny_folder_store_create_folder (parent, name, &(priv->error));
	CHECK_EXCEPTION (priv, MODEST_MAIL_OPERATION_STATUS_FAILED, return NULL);

/* 	/\* Subscribe to folder *\/ */
/* 	if (!tny_folder_is_subscribed (new_folder)) { */
/* 		store_account = TNY_STORE_ACCOUNT (tny_folder_get_account (TNY_FOLDER (parent))); */
/* 		tny_store_account_subscribe (store_account, new_folder); */
/* 		g_object_unref (G_OBJECT (store_account)); */
/* 	} */

	return new_folder;
}

void
modest_mail_operation_remove_folder (ModestMailOperation *self,
				     TnyFolder           *folder,
				     gboolean             remove_to_trash)
{
	TnyFolderStore *parent;
	TnyAccount *account;
	ModestMailOperationPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER (folder));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

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
		parent = tny_folder_get_folder_store (folder);

		tny_folder_store_remove_folder (parent, folder, &(priv->error));
		CHECK_EXCEPTION (priv, MODEST_MAIL_OPERATION_STATUS_FAILED, );

		if (parent)
			g_object_unref (G_OBJECT (parent));
	}
	g_object_unref (G_OBJECT (account));
}

void
modest_mail_operation_rename_folder (ModestMailOperation *self,
				     TnyFolder *folder,
				     const gchar *name)
{
	ModestMailOperationPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER_STORE (folder));
	g_return_if_fail (name);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	/* FIXME: better error handling */
	if (strrchr (name, '/') != NULL)
		return;

	/* Rename. Camel handles folder subscription/unsubscription */
	tny_folder_set_name (folder, name, &(priv->error));
	CHECK_EXCEPTION (priv, MODEST_MAIL_OPERATION_STATUS_FAILED, return);
 }

TnyFolder *
modest_mail_operation_xfer_folder (ModestMailOperation *self,
				   TnyFolder *folder,
				   TnyFolderStore *parent,
				   gboolean delete_original)
{
	ModestMailOperationPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), NULL);
	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), NULL);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), NULL);

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (self);

	return tny_folder_copy (folder, 
				parent, 
				tny_folder_get_name (folder), 
				delete_original, 
				&(priv->error));
}


/* ******************************************************************* */
/* **************************  MSG  ACTIONS  ************************* */
/* ******************************************************************* */

void 
modest_mail_operation_remove_msg (ModestMailOperation *self,
				  TnyHeader *header,
				  gboolean remove_to_trash)
{
	TnyFolder *folder;

	g_return_if_fail (TNY_IS_HEADER (header));

	folder = tny_header_get_folder (header);

	/* Delete or move to trash */
	if (remove_to_trash) {
		TnyFolder *trash_folder;
		TnyStoreAccount *store_account;

		store_account = TNY_STORE_ACCOUNT (tny_folder_get_account (folder));
		trash_folder = modest_tny_account_get_special_folder (TNY_ACCOUNT(store_account),
								      TNY_FOLDER_TYPE_TRASH);
		if (trash_folder) {
			modest_mail_operation_xfer_msg (self, header, trash_folder, TRUE);
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
		tny_folder_remove_msg (folder, header, NULL); /* FIXME */
		tny_folder_sync(folder, TRUE, NULL); /* FIXME */
	}

	/* Free */
	g_object_unref (folder);
}

static void
transfer_msgs_cb (TnyFolder *folder, GError **err, gpointer user_data)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(user_data);

	if (*err) {
		priv->error = g_error_copy (*err);
		priv->done = 0;
		priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
	} else {
		priv->done = 1;
		priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;
	}

	g_signal_emit (G_OBJECT (user_data), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}

gboolean
modest_mail_operation_xfer_msg (ModestMailOperation *self,
				TnyHeader *header, 
				TnyFolder *folder, 
				gboolean delete_original)
{
	ModestMailOperationPrivate *priv;
	TnyFolder *src_folder;
	TnyList *headers;

	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (self), FALSE);
	g_return_val_if_fail (TNY_IS_HEADER (header), FALSE);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), FALSE);

	src_folder = tny_header_get_folder (header);
	headers = tny_simple_list_new ();

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	priv->total = 1;
	priv->done = 0;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	tny_list_prepend (headers, G_OBJECT (header));
	tny_folder_transfer_msgs_async (src_folder, headers, folder, 
					delete_original, transfer_msgs_cb, 
					g_object_ref(self));

	/* Free */
	/* FIXME: don't free 'm yet */
	///g_object_unref (headers);
	///g_object_unref (src_folder);

	return TRUE;
}

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
#include <camel/camel-stream-mem.h>
#include <glib/gi18n.h>
#include <modest-tny-account.h>
#include "modest-text-utils.h"
#include "modest-tny-msg.h"
#include "modest-tny-platform-factory.h"
#include "modest-marshal.h"
#include "modest-formatter.h"
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
static void     add_attachments      (TnyMsg *msg, 
				      GList *attachments_list);


static TnyMimePart *         add_body_part    (TnyMsg *msg, 
					       const gchar *body,
					       const gchar *content_type, 
					       gboolean has_attachments);


static void        modest_mail_operation_xfer_folder       (ModestMailOperation *self,
							    TnyFolder *folder,
							    TnyFolderStore *parent,
							    gboolean delete_original);

static gboolean    modest_mail_operation_xfer_msg          (ModestMailOperation *self,
							    TnyHeader *header, 
							    TnyFolder *folder, 
							    gboolean delete_original);

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

typedef struct _RefreshFolderAsyncHelper
{
	ModestMailOperation *mail_op;
	TnyIterator *iter;
	guint failed;
	guint canceled;

} RefreshFolderAsyncHelper;

/* some utility functions */
static char * get_content_type(const gchar *s);
static gboolean is_ascii(const gchar *s);

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
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));

	tny_transport_account_send (transport_account, msg, NULL); /* FIXME */
}

void
modest_mail_operation_send_new_mail (ModestMailOperation *self,
				     TnyTransportAccount *transport_account,
				     const gchar *from,
				     const gchar *to,
				     const gchar *cc,
				     const gchar *bcc,
				     const gchar *subject,
				     const gchar *body,
				     const GList *attachments_list)
{
	TnyPlatformFactory *fact;
	TnyMsg *new_msg;
	TnyHeader *header;
	gchar *content_type;
	ModestMailOperationPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (transport_account));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);

	/* Check parametters */
	if (to == NULL) {
		g_set_error (&(priv->error), MODEST_MAIL_OPERATION_ERROR,
			     MODEST_MAIL_OPERATION_ERROR_MISSING_PARAMETER,
			     _("Error trying to send a mail. You need to set almost one a recipient"));
		return;
	}

	/* Create new msg */
	fact    = modest_tny_platform_factory_get_instance ();
	new_msg = tny_platform_factory_new_msg (fact);
	header  = tny_platform_factory_new_header (fact);

	/* WARNING: set the header before assign values to it */
	tny_msg_set_header (new_msg, header);
	tny_header_set_from (TNY_HEADER (header), from);
	tny_header_set_replyto (TNY_HEADER (header), from);
	tny_header_set_to (TNY_HEADER (header), to);
	tny_header_set_cc (TNY_HEADER (header), cc);
	tny_header_set_bcc (TNY_HEADER (header), bcc);
	tny_header_set_subject (TNY_HEADER (header), subject);

	content_type = get_content_type(body);

	/* Add the body of the new mail */	
	add_body_part (new_msg, body, (const gchar *) content_type,
		       (attachments_list == NULL) ? FALSE : TRUE);

	/* Add attachments */
	add_attachments (new_msg, (GList*) attachments_list);

	/* Send mail */
	tny_transport_account_send (transport_account, new_msg, NULL); /* FIXME */

	/* Clean */
	g_object_unref (header);
	g_object_unref (new_msg);
	g_free(content_type);
}

static void
add_if_attachment (gpointer data, gpointer user_data)
{
	TnyMimePart *part;
	GList *attachments_list;

	part = TNY_MIME_PART (data);
	attachments_list = (GList *) user_data;

	if (tny_mime_part_is_attachment (part))
		attachments_list = g_list_prepend (attachments_list, part);
}


static TnyMsg *
create_reply_forward_mail (TnyMsg *msg, const gchar *from, gboolean is_reply, guint type)
{
	TnyPlatformFactory *fact;
	TnyMsg *new_msg;
	TnyHeader *new_header, *header;
	gchar *new_subject;
	TnyMimePart *body;
	ModestFormatter *formatter;

	/* Get body from original msg. Always look for the text/plain
	   part of the message to create the reply/forwarded mail */
	header = tny_msg_get_header (msg);
	body   = modest_tny_msg_find_body_part (msg, FALSE);

	/* TODO: select the formatter from account prefs */
	formatter = modest_formatter_new ("text/plain");

	/* Format message body */
	if (is_reply) {
		switch (type) {
		case MODEST_MAIL_OPERATION_REPLY_TYPE_CITE:
		default:
			new_msg = modest_formatter_cite  (formatter, body, header);
			break;
		case MODEST_MAIL_OPERATION_REPLY_TYPE_QUOTE:
			new_msg = modest_formatter_quote (formatter, body, header);
			break;
		}
	} else {
		switch (type) {
		case MODEST_MAIL_OPERATION_FORWARD_TYPE_INLINE:
		default:
			new_msg = modest_formatter_inline  (formatter, body, header);
			break;
		case MODEST_MAIL_OPERATION_FORWARD_TYPE_ATTACHMENT:
			new_msg = modest_formatter_attach (formatter, body, header);
			break;
		}
	}
	g_object_unref (G_OBJECT(formatter));
	g_object_unref (G_OBJECT(body));
	
	/* Fill the header */
	fact = modest_tny_platform_factory_get_instance ();
	new_header = TNY_HEADER (tny_platform_factory_new_header (fact));
	tny_msg_set_header (new_msg, new_header);
	tny_header_set_from (new_header, from);
	tny_header_set_replyto (new_header, from);

	/* Change the subject */
	new_subject =
		(gchar *) modest_text_utils_derived_subject (tny_header_get_subject(header),
							     (is_reply) ? _("Re:") : _("Fwd:"));
	tny_header_set_subject (new_header, (const gchar *) new_subject);
	g_free (new_subject);

	/* Clean */
	g_object_unref (G_OBJECT (new_header));
	g_object_unref (G_OBJECT (header));

	return new_msg;
}

TnyMsg* 
modest_mail_operation_create_forward_mail (TnyMsg *msg, 
					   const gchar *from,
					   ModestMailOperationForwardType forward_type)
{
	TnyMsg *new_msg;
	TnyList *parts = NULL;
	GList *attachments_list = NULL;

	new_msg = create_reply_forward_mail (msg, from, FALSE, forward_type);

	/* Add attachments */
	parts = TNY_LIST (tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);
	tny_list_foreach (parts, add_if_attachment, attachments_list);
	add_attachments (new_msg, attachments_list);

	/* Clean */
	if (attachments_list) g_list_free (attachments_list);
	g_object_unref (G_OBJECT (parts));

	return new_msg;
}

TnyMsg* 
modest_mail_operation_create_reply_mail (TnyMsg *msg, 
					 const gchar *from,
					 ModestMailOperationReplyType reply_type,
					 ModestMailOperationReplyMode reply_mode)
{
	TnyMsg *new_msg = NULL;
	TnyHeader *new_header, *header;
	const gchar* reply_to;
	gchar *new_cc = NULL;
	const gchar *cc = NULL, *bcc = NULL;
	GString *tmp = NULL;

	new_msg = create_reply_forward_mail (msg, from, TRUE, reply_type);

	/* Fill the header */
	header = tny_msg_get_header (msg);
	new_header = tny_msg_get_header (new_msg);
	reply_to = tny_header_get_replyto (header);

	if (reply_to)
		tny_header_set_to (new_header, reply_to);
	else
		tny_header_set_to (new_header, tny_header_get_from (header));

	switch (reply_mode) {
	case MODEST_MAIL_OPERATION_REPLY_MODE_SENDER:
		/* Do not fill neither cc nor bcc */
		break;
	case MODEST_MAIL_OPERATION_REPLY_MODE_LIST:
		/* TODO */
		break;
	case MODEST_MAIL_OPERATION_REPLY_MODE_ALL:
		/* Concatenate to, cc and bcc */
		cc = tny_header_get_cc (header);
		bcc = tny_header_get_bcc (header);

		tmp = g_string_new (tny_header_get_to (header));
		if (cc)  g_string_append_printf (tmp, ",%s",cc);
		if (bcc) g_string_append_printf (tmp, ",%s",bcc);

               /* Remove my own address from the cc list. TODO:
                  remove also the To: of the new message, needed due
                  to the new reply_to feature */
		new_cc = (gchar *)
			modest_text_utils_remove_address ((const gchar *) tmp->str,
							  from);
		/* FIXME: remove also the mails from the new To: */
		tny_header_set_cc (new_header, new_cc);

		/* Clean */
		g_string_free (tmp, TRUE);
		g_free (new_cc);
		break;
	}

	/* Clean */
	g_object_unref (G_OBJECT (new_header));
	g_object_unref (G_OBJECT (header));

	return new_msg;
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
		tny_folder_refresh_async (TNY_FOLDER (tny_iterator_get_current (helper->iter)),
					  folder_refresh_cb,
					  status_update_cb, 
					  helper);
	}
	g_signal_emit (G_OBJECT (self), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}


static void
update_folders_cb (TnyFolderStore *folder_store, TnyList *list, GError **err, gpointer user_data)
{
	ModestMailOperation *self;
	ModestMailOperationPrivate *priv;
	RefreshFolderAsyncHelper *helper;

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
	tny_folder_refresh_async (TNY_FOLDER (tny_iterator_get_current (helper->iter)),
				  folder_refresh_cb,
				  status_update_cb, 
				  helper);
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
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (store_account),
					    folders, update_folders_cb, query, self);
	g_object_unref (query);

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
	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), NULL);
	g_return_val_if_fail (name, NULL);

	TnyFolder *new_folder = NULL;
	TnyStoreAccount *store_account;

	/* Create the folder */
	new_folder = tny_folder_store_create_folder (parent, name, NULL); /* FIXME */
	if (!new_folder) 
		return NULL;

	/* Subscribe to folder */
	if (!tny_folder_is_subscribed (new_folder)) {
		store_account = TNY_STORE_ACCOUNT (tny_folder_get_account (TNY_FOLDER (parent)));
		tny_store_account_subscribe (store_account, new_folder);
		g_object_unref (G_OBJECT (store_account));
	}

	return new_folder;
}

void
modest_mail_operation_remove_folder (ModestMailOperation *self,
				     TnyFolder *folder,
				     gboolean remove_to_trash)
{
	TnyFolderStore *folder_store;

	g_return_if_fail (TNY_IS_FOLDER (folder));

	/* Get folder store */
	folder_store = TNY_FOLDER_STORE (tny_folder_get_account (folder));

	/* Delete folder or move to trash */
	if (remove_to_trash) {
		TnyFolder *trash_folder;
		trash_folder = modest_tny_account_get_special_folder (TNY_ACCOUNT(folder_store),
								      TNY_FOLDER_TYPE_TRASH);
		
		/* TODO: error_handling */
		modest_mail_operation_move_folder (self, 
						   folder, 
						   TNY_FOLDER_STORE (trash_folder));
	} else {
		tny_folder_store_remove_folder (folder_store, folder, NULL); /* FIXME */
		g_object_unref (G_OBJECT (folder));
	}

	/* Free instances */
	g_object_unref (G_OBJECT (folder_store));
}

void
modest_mail_operation_rename_folder (ModestMailOperation *self,
				     TnyFolder *folder,
				     const gchar *name)
{
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER_STORE (folder));
	g_return_if_fail (name);

	/* FIXME: better error handling */
	if (strrchr (name, '/') != NULL)
		return;

	/* Rename. Camel handles folder subscription/unsubscription */
	tny_folder_set_name (folder, name, NULL); /* FIXME */
 }

void
modest_mail_operation_move_folder (ModestMailOperation *self,
				   TnyFolder *folder,
				   TnyFolderStore *parent)
{
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER_STORE (parent));
	g_return_if_fail (TNY_IS_FOLDER (folder));
	
	modest_mail_operation_xfer_folder (self, folder, parent, TRUE);
}

void
modest_mail_operation_copy_folder (ModestMailOperation *self,
				   TnyFolder *folder,
				   TnyFolderStore *parent)
{
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (self));
	g_return_if_fail (TNY_IS_FOLDER_STORE (parent));
	g_return_if_fail (TNY_IS_FOLDER (folder));

	modest_mail_operation_xfer_folder (self, folder, parent, FALSE);
}

static void
modest_mail_operation_xfer_folder (ModestMailOperation *self,
				   TnyFolder *folder,
				   TnyFolderStore *parent,
				   gboolean delete_original)
{
	const gchar *folder_name;
	TnyFolder *dest_folder, *child;
	TnyIterator *iter;
	TnyList *folders, *headers;

	g_return_if_fail (TNY_IS_FOLDER (folder));
	g_return_if_fail (TNY_IS_FOLDER_STORE (parent));

	/* Create the destination folder */
	folder_name = tny_folder_get_name (folder);
	dest_folder = modest_mail_operation_create_folder (self, 
							   parent, folder_name);

	/* Transfer messages */
	headers = TNY_LIST (tny_simple_list_new ());
 	tny_folder_get_headers (folder, headers, FALSE, NULL); /* FIXME */
	tny_folder_transfer_msgs (folder, headers, dest_folder, delete_original, NULL); /* FIXME */

	/* Recurse children */
	folders = TNY_LIST (tny_simple_list_new ());
	tny_folder_store_get_folders (TNY_FOLDER_STORE (folder), folders, NULL, NULL ); /* FIXME */
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter)) {

		child = TNY_FOLDER (tny_iterator_get_current (iter));
		modest_mail_operation_xfer_folder (self, child,
						   TNY_FOLDER_STORE (dest_folder),
						   delete_original);
		tny_iterator_next (iter);
	}

	/* Delete source folder (if needed) */
	if (delete_original)
		modest_mail_operation_remove_folder (self, folder, FALSE);

	/* Clean up */
	g_object_unref (G_OBJECT (dest_folder));
	g_object_unref (G_OBJECT (headers));
	g_object_unref (G_OBJECT (folders));
	g_object_unref (G_OBJECT (iter));
}


/* ******************************************************************* */
/* **************************  MSG  ACTIONS  ************************* */
/* ******************************************************************* */

gboolean 
modest_mail_operation_copy_msg (ModestMailOperation *self,
				TnyHeader *header, 
				TnyFolder *folder)
{
	g_return_val_if_fail (TNY_IS_HEADER (header), FALSE);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), FALSE);

	return modest_mail_operation_xfer_msg (self, header, folder, FALSE);
}

gboolean 
modest_mail_operation_move_msg (ModestMailOperation *self,
				TnyHeader *header, 
				TnyFolder *folder)
{
	g_return_val_if_fail (TNY_IS_HEADER (header), FALSE);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), FALSE);

	return modest_mail_operation_xfer_msg (self, header, folder, TRUE);
}

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
			modest_mail_operation_move_msg (self, header, trash_folder);
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
	priv->done = 1;
	priv->status = MODEST_MAIL_OPERATION_STATUS_SUCCESS;

	g_signal_emit (G_OBJECT (user_data), signals[PROGRESS_CHANGED_SIGNAL], 0, NULL);
}

static gboolean
modest_mail_operation_xfer_msg (ModestMailOperation *self,
				TnyHeader *header, 
				TnyFolder *folder, 
				gboolean delete_original)
{
	ModestMailOperationPrivate *priv;
	TnyFolder *src_folder;
	TnyList *headers;

	src_folder = tny_header_get_folder (header);
	headers = tny_simple_list_new ();

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(self);
	priv->total = 1;
	priv->done = 0;
	priv->status = MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS;

	tny_list_prepend (headers, G_OBJECT (header));
	tny_folder_transfer_msgs_async (src_folder, headers, folder, 
					delete_original, transfer_msgs_cb, self);

	/* Free */
	g_object_unref (headers);
	g_object_unref (folder);

	return TRUE;
}


/* ******************************************************************* */
/* ************************* UTILIY FUNCTIONS ************************ */
/* ******************************************************************* */
static gboolean
is_ascii(const gchar *s)
{
	while (s[0]) {
		if (s[0] & 128 || s[0] < 32)
			return FALSE;
		s++;
	}
	return TRUE;
}

static char *
get_content_type(const gchar *s)
{
	GString *type;
	
	type = g_string_new("text/plain");
	if (!is_ascii(s)) {
		if (g_utf8_validate(s, -1, NULL)) {
			g_string_append(type, "; charset=\"utf-8\"");
		} else {
			/* it should be impossible to reach this, but better safe than sorry */
			g_warning("invalid utf8 in message");
			g_string_append(type, "; charset=\"latin1\"");
		}
	}
	return g_string_free(type, FALSE);
}

static void
add_attachments (TnyMsg *msg, GList *attachments_list)
{
	GList *pos;
	TnyMimePart *attachment_part, *old_attachment;
	const gchar *attachment_content_type;
	const gchar *attachment_filename;
	TnyStream *attachment_stream;
	TnyPlatformFactory *fact;

	fact = modest_tny_platform_factory_get_instance ();
	for (pos = (GList *)attachments_list; pos; pos = pos->next) {

		old_attachment = pos->data;
		attachment_filename = tny_mime_part_get_filename (old_attachment);
		attachment_stream = tny_mime_part_get_stream (old_attachment);
		attachment_part = tny_platform_factory_new_mime_part (fact);
		
		attachment_content_type = tny_mime_part_get_content_type (old_attachment);
				 
		tny_mime_part_construct_from_stream (attachment_part,
						     attachment_stream,
						     attachment_content_type);
		tny_stream_reset (attachment_stream);
		
		tny_mime_part_set_filename (attachment_part, attachment_filename);
		
		tny_mime_part_add_part (TNY_MIME_PART (msg), attachment_part);
/* 		g_object_unref (attachment_part); */
	}
}


static TnyMimePart *
add_body_part (TnyMsg *msg, 
	       const gchar *body,
	       const gchar *content_type,
	       gboolean has_attachments)
{
	TnyMimePart *text_body_part = NULL;
	TnyStream *text_body_stream;
	TnyPlatformFactory *fact;

	fact = modest_tny_platform_factory_get_instance ();

	/* Create the stream */
	text_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(body, strlen(body))));

	/* Create body part if needed */
	if (has_attachments)
		text_body_part = tny_platform_factory_new_mime_part (fact);
	else
		text_body_part = TNY_MIME_PART(msg);

	/* Construct MIME part */
	tny_stream_reset (text_body_stream);
	tny_mime_part_construct_from_stream (text_body_part,
					     text_body_stream,
					     content_type);
	tny_stream_reset (text_body_stream);

	/* Add part if needed */
	if (has_attachments) {
		tny_mime_part_add_part (TNY_MIME_PART (msg), text_body_part);
		g_object_unref (G_OBJECT(text_body_part));
	}

	/* Clean */
	g_object_unref (text_body_stream);

	return text_body_part;
}

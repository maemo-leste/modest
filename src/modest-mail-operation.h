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

#ifndef __MODEST_MAIL_OPERATION_H__
#define __MODEST_MAIL_OPERATION_H__

#include <tny-transport-account.h>
#include "modest-tny-attachment.h"
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAIL_OPERATION             (modest_mail_operation_get_type())
#define MODEST_MAIL_OPERATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAIL_OPERATION,ModestMailOperation))
#define MODEST_MAIL_OPERATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MAIL_OPERATION,GObject))
#define MODEST_IS_MAIL_OPERATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAIL_OPERATION))
#define MODEST_IS_MAIL_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MAIL_OPERATION))
#define MODEST_MAIL_OPERATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MAIL_OPERATION,ModestMailOperationClass))

typedef struct _ModestMailOperation      ModestMailOperation;
typedef struct _ModestMailOperationClass ModestMailOperationClass;

/**
 * ModestMailOperationForwardType:
 *
 * How the original message will be forwarded to the recipient
 */
typedef enum _ModestMailOperationForwardType {
	MODEST_MAIL_OPERATION_FORWARD_TYPE_INLINE = 1,
	MODEST_MAIL_OPERATION_FORWARD_TYPE_ATTACHMENT
} ModestMailOperationForwardType;

/**
 * ModestMailOperationReplyType:
 *
 * How the original message will be forwarded to the recipient
 */
typedef enum _ModestMailOperationReplyType {
	MODEST_MAIL_OPERATION_REPLY_TYPE_CITE = 1,
	MODEST_MAIL_OPERATION_REPLY_TYPE_QUOTE
} ModestMailOperationReplyType;

/**
 * ModestMailOperationReplyMode:
 *
 * Who will be the recipients of the replied message
 */
typedef enum _ModestMailOperationReplyMode {
	MODEST_MAIL_OPERATION_REPLY_MODE_SENDER,
	MODEST_MAIL_OPERATION_REPLY_MODE_LIST,
	MODEST_MAIL_OPERATION_REPLY_MODE_ALL
} ModestMailOperationReplyMode;

/**
 * ModestMailOperationStatus:
 *
 * The state of a mail operation
 */
typedef enum _ModestMailOperationStatus {
	MODEST_MAIL_OPERATION_STATUS_INVALID,
	MODEST_MAIL_OPERATION_STATUS_SUCCESS,
	MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS,
	MODEST_MAIL_OPERATION_STATUS_FAILED,
	MODEST_MAIL_OPERATION_STATUS_IN_PROGRESS,
	MODEST_MAIL_OPERATION_STATUS_CANCELED
} ModestMailOperationStatus;

struct _ModestMailOperation {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestMailOperationClass {
	GObjectClass parent_class;

	/* Signals */
	void (*progress_changed) (ModestMailOperation *self, gpointer user_data);
};

/* member functions */
GType        modest_mail_operation_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
ModestMailOperation*    modest_mail_operation_new         (void);

/* fill in other public functions, eg.: */

/**
 * modest_mail_operation_send_mail:
 * @self: a #ModestMailOperation
 * @transport_account: a non-NULL #TnyTransportAccount
 * @msg: a non-NULL #TnyMsg
 * 
 * Sends and already existing message using the provided
 * #TnyTransportAccount. This operation is synchronous, so the
 * #ModestMailOperation should not be added to any
 * #ModestMailOperationQueue
  **/
void    modest_mail_operation_send_mail       (ModestMailOperation *self,
					       TnyTransportAccount *transport_account,
					       TnyMsg* msg);

/**
 * modest_mail_operation_send_new_mail:
 * @self: a #ModestMailOperation
 * @transport_account: a non-NULL #TnyTransportAccount
 * @from: the email address of the mail sender
 * @to: a non-NULL email address of the mail receiver
 * @cc: a comma-separated list of email addresses where to send a carbon copy
 * @bcc: a comma-separated list of email addresses where to send a blind carbon copy
 * @subject: the subject of the new mail
 * @body: the body of the new mail
 * @attachments_list: a #GList of attachments, each attachment must be a #TnyMimePart
 * 
 * Sends a new mail message using the provided
 * #TnyTransportAccount. This operation is synchronous, so the
 * #ModestMailOperation should not be added to any
 * #ModestMailOperationQueue
  **/
void    modest_mail_operation_send_new_mail   (ModestMailOperation *self,
					       TnyTransportAccount *transport_account,
					       const gchar *from,
					       const gchar *to,
					       const gchar *cc,
					       const gchar *bcc,
					       const gchar *subject,
					       const gchar *body,
					       const GList *attachments_list);

/**
 * modest_mail_operation_create_forward_mail:
 * @msg: a valid #TnyMsg instance
 * @forward_type: the type of formatting used to create the forwarded message
 * 
 * Creates a forwarded message from an existing one
 * 
 * Returns: a new #TnyMsg, or NULL in case of error
 **/
TnyMsg* modest_mail_operation_create_forward_mail (TnyMsg *msg, 
						   const gchar *from,
						   ModestMailOperationForwardType forward_type);

/**
 * modest_mail_operation_create_reply_mail:
 * @msg: a valid #TnyMsg instance
 * @reply_type: the type of formatting used to create the reply message
 * @reply_mode: the mode of reply: to the sender only, to a mail list or to all
 * 
 * Creates a new message to reply to an existing one
 * 
 * Returns: Returns: a new #TnyMsg, or NULL in case of error
 **/
TnyMsg* modest_mail_operation_create_reply_mail    (TnyMsg *msg, 
						    const gchar *from,
						    ModestMailOperationReplyType reply_type,
						    ModestMailOperationReplyMode reply_mode);

/**
 * modest_mail_operation_update_account:
 * @self: a #ModestMailOperation
 * @store_account: a #TnyStoreAccount
 * 
 * Asynchronously refreshes the root folders of the given store
 * account. The caller should add the #ModestMailOperation to a
 * #ModestMailOperationQueue and then free it. The caller will be
 * notified by the "progress_changed" signal each time the progress of
 * the operation changes.
 * Example
 * <informalexample><programlisting>
 * queue = modest_tny_platform_factory_get_modest_mail_operation_queue_instance (fact)
 * mail_op = modest_mail_operation_new ();
 * g_signal_connect (G_OBJECT (mail_op), "progress_changed", G_CALLBACK(on_progress_changed), queue);
 * if (modest_mail_operation_update_account (mail_op, account))
 * {
 *     modest_mail_operation_queue_add (queue, mail_op);
 * }
 * g_object_unref (G_OBJECT (mail_op));
 * </programlisting></informalexample>
 * 
 * Returns: TRUE if the mail operation could be started, or FALSE otherwise
 **/
gboolean      modest_mail_operation_update_account (ModestMailOperation *self,
						    TnyStoreAccount *store_account);

/* Functions that perform store operations */

/**
 * modest_mail_operation_create_folder:
 * @self: a #ModestMailOperation
 * @parent: the #TnyFolderStore which is the parent of the new folder
 * @name: the non-NULL new name for the new folder
 * 
 * Creates a new folder as a children of a existing one. If the store
 * account supports subscriptions this method also sets the new folder
 * as subscribed. This operation is synchronous, so the
 * #ModestMailOperation should not be added to any
 * #ModestMailOperationQueue
 * 
 * Returns: a newly created #TnyFolder or NULL in case of error.
 **/
TnyFolder*    modest_mail_operation_create_folder  (ModestMailOperation *self,
						    TnyFolderStore *parent,
						    const gchar *name);

/**
 * modest_mail_operation_remove_folder:
 * @self: a #ModestMailOperation
 * @folder: a #TnyFolder
 * @remove_to_trash: TRUE to move it to trash or FALSE to delete
 * permanently
 * 
 * Removes a folder. This operation is synchronous, so the
 * #ModestMailOperation should not be added to any
 * #ModestMailOperationQueue
 **/
void          modest_mail_operation_remove_folder  (ModestMailOperation *self,
						    TnyFolder *folder,
						    gboolean remove_to_trash);

/**
 * modest_mail_operation_rename_folder:
 * @self: a #ModestMailOperation
 * @folder: a #TnyFolder
 * @name: a non-NULL name without "/"
 * 
 * Renames a given folder with the provided new name. This operation
 * is synchronous, so the #ModestMailOperation should not be added to
 * any #ModestMailOperationQueue
 **/
void          modest_mail_operation_rename_folder  (ModestMailOperation *self,
						    TnyFolder *folder, 
						    const gchar *name);

/**
 * modest_mail_operation_move_folder:
 * @self: a #ModestMailOperation
 * @folder: a #TnyFolder
 * @parent: the new parent of the folder as #TnyFolderStore
 * 
 * Sets the given @folder as child of a provided #TnyFolderStore. This
 * operation moves also all the messages contained in the folder and
 * all of his children folders with their messages as well. This
 * operation is synchronous, so the #ModestMailOperation should not be
 * added to any #ModestMailOperationQueue
 **/
void          modest_mail_operation_move_folder    (ModestMailOperation *self,
						    TnyFolder *folder, 
						    TnyFolderStore *parent);

/**
 * modest_mail_operation_copy_folder:
 * @self: a #ModestMailOperation
 * @folder: a #TnyFolder
 * @parent: a #TnyFolderStore that will be the parent of the copied folder
 * 
 * Sets a copy of the given @folder as child of a provided
 * #TnyFolderStore. This operation copies also all the messages
 * contained in the folder and all of his children folders with their
 * messages as well. This operation is synchronous, so the
 * #ModestMailOperation should not be added to any
 * #ModestMailOperationQueue
 **/
void          modest_mail_operation_copy_folder    (ModestMailOperation *self,
						    TnyFolder *folder, 
						    TnyFolderStore *parent);

/* Functions that performs msg operations */

/**
 * modest_mail_operation_copy_msg:
 * @self: a #ModestMailOperation
 * @header: the #TnyHeader of the message to copy
 * @folder: the #TnyFolder where the message will be copied
 * 
 * Asynchronously copies a message from its current folder to another
 * one. The caller should add the #ModestMailOperation to a
 * #ModestMailOperationQueue and then free it. The caller will be
 * notified by the "progress_changed" when the operation is completed.
 * 
 * Example
 * <informalexample><programlisting>
 * queue = modest_tny_platform_factory_get_modest_mail_operation_queue_instance (fact);
 * mail_op = modest_mail_operation_new ();
 * if (modest_mail_operation_copy_msg (mail_op, account))
 * {
 *     g_signal_connect (G_OBJECT (mail_op), "progress_changed", G_CALLBACK(on_progress_changed), queue);
 *     modest_mail_operation_queue_add (queue, mail_op);
 * }
 * g_object_unref (G_OBJECT (mail_op));
 * </programlisting></informalexample>
 *
 * Returns: TRUE if the mail operation could be started, or FALSE otherwise
 **/
gboolean      modest_mail_operation_copy_msg       (ModestMailOperation *self,
						    TnyHeader *header, 
						    TnyFolder *folder);

/**
 * modest_mail_operation_move_msg:
 * @self: a #ModestMailOperation
 * @header: the #TnyHeader of the message to move
 * @folder: the #TnyFolder where the message will be moved
 * 
 * Asynchronously moves a message from its current folder to another
 * one. The caller should add the #ModestMailOperation to a
 * #ModestMailOperationQueue and then free it. The caller will be
 * notified by the "progress_changed" when the operation is completed.
 * 
 * Example
 * <informalexample><programlisting>
 * queue = modest_tny_platform_factory_get_modest_mail_operation_queue_instance (fact);
 * mail_op = modest_mail_operation_new ();
 * if (modest_mail_operation_move_msg (mail_op, account))
 * {
 *     g_signal_connect (G_OBJECT (mail_op), "progress_changed", G_CALLBACK(on_progress_changed), queue);
 *     modest_mail_operation_queue_add (queue, mail_op);
 * }
 * g_object_unref (G_OBJECT (mail_op));
 * </programlisting></informalexample>
 *
 * Returns: TRUE if the mail operation could be started, or FALSE otherwise
 **/
gboolean      modest_mail_operation_move_msg       (ModestMailOperation *self,
						    TnyHeader *header, 
						    TnyFolder *folder);

/**
 * modest_mail_operation_remove_msg:
 * @self: a #ModestMailOperation
 * @header: the #TnyHeader of the message to move
 * @remove_to_trash: TRUE to move it to trash or FALSE to delete it
 * permanently
 * 
 * Deletes a message. This operation is synchronous, so the
 * #ModestMailOperation should not be added to any
 * #ModestMailOperationQueue
 **/
void          modest_mail_operation_remove_msg     (ModestMailOperation *self,
						    TnyHeader *header,
						    gboolean remove_to_trash);

/* Functions to control mail operations */
/**
 * modest_mail_operation_get_task_done:
 * @self: a #ModestMailOperation
 * 
 * Gets the amount of work done for a given mail operation. This
 * amount of work is an absolute value.
 * 
 * Returns: the amount of work completed
 **/
guint     modest_mail_operation_get_task_done      (ModestMailOperation *self);

/**
 * modest_mail_operation_get_task_total:
 * @self: a #ModestMailOperation
 * 
 * Gets the total amount of work that must be done to complete a given
 * mail operation. This amount of work is an absolute value.
 * 
 * Returns: the total required amount of work
 **/
guint     modest_mail_operation_get_task_total     (ModestMailOperation *self);


/**
 * modest_mail_operation_is_finished:
 * @self: a #ModestMailOperation
 * 
 * Checks if the operation is finished. A #ModestMailOperation is
 * finished if it's in any of the following states:
 * MODEST_MAIL_OPERATION_STATUS_SUCCESS,
 * MODEST_MAIL_OPERATION_STATUS_FAILED,
 * MODEST_MAIL_OPERATION_STATUS_CANCELED or
 * MODEST_MAIL_OPERATION_STATUS_FINISHED_WITH_ERRORS
 * 
 * Returns: TRUE if the operation is finished, FALSE otherwise
 **/
gboolean                  modest_mail_operation_is_finished (ModestMailOperation *self);

/**
 * modest_mail_operation_is_finished:
 * @self: a #ModestMailOperation
 * 
 * Gets the current status of the given mail operation
 *
 * Returns: the current status or MODEST_MAIL_OPERATION_STATUS_INVALID in case of error
 **/
ModestMailOperationStatus modest_mail_operation_get_status  (ModestMailOperation *self);

/**
 * modest_mail_operation_get_error:
 * @self: a #ModestMailOperation
 * 
 * Gets the error associated to the mail operation if exists
 * 
 * Returns: the #GError associated to the #ModestMailOperation if it
 * exists or NULL otherwise
 **/
const GError*             modest_mail_operation_get_error   (ModestMailOperation *self);

/**
 * modest_mail_operation_cancel:
 * @self: a #ModestMailOperation
 *
 * Cancels an active mail operation
 * 
 * Returns: TRUE if the operation was succesfully canceled, FALSE otherwise
 **/
gboolean                  modest_mail_operation_cancel      (ModestMailOperation *self);

G_END_DECLS

#endif /* __MODEST_MAIL_OPERATION_H__ */


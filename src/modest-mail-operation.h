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
#include <tny-folder-store.h>
#include <modest-tny-send-queue.h>
#include <modest-tny-account-store.h>

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

/**
 * ModestMailOperationId:
 *
 * The id for identifying the type of mail operation
 */
typedef enum {
	MODEST_MAIL_OPERATION_TYPE_SEND,
	MODEST_MAIL_OPERATION_TYPE_RECEIVE,
	MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE,
	MODEST_MAIL_OPERATION_TYPE_OPEN,
	MODEST_MAIL_OPERATION_TYPE_DELETE,
	MODEST_MAIL_OPERATION_TYPE_INFO,
	MODEST_MAIL_OPERATION_TYPE_RUN_QUEUE,
	MODEST_MAIL_OPERATION_TYPE_SYNC_FOLDER,
	MODEST_MAIL_OPERATION_TYPE_SHUTDOWN,
	MODEST_MAIL_OPERATION_TYPE_QUEUE_WAKEUP,
	MODEST_MAIL_OPERATION_TYPE_UPDATE_FOLDER_COUNTS,
	MODEST_MAIL_OPERATION_TYPE_UNKNOWN,
} ModestMailOperationTypeOperation;

/**
 * ErrorCheckingUserCallback:
 *
 * @mail_op: the current mail operation.
 * @user_data: generic data passed to user defined function.
 *
 * This function implements required actions to performs under error
 * states.  
 */
typedef void (*ErrorCheckingUserCallback) (ModestMailOperation *mail_op, gpointer user_data);

/**
 * ErrorCheckingUserDataDestroyer:
 *
 * @user_data: generic data passed to user defined function.
 *
 * This function is used to destroy the user_data passed to the error
 * checking user callback function
 */
typedef void (*ErrorCheckingUserDataDestroyer) (gpointer user_data);


/**
 * GetMsgAsyncUserCallback:
 *
 * @mail_op: the current #ModestMailOperation.
 * @header: a #TnyHeader summary item.
 * @msg: a #TnyMsg message retrieved by async operation.
 * @user_data: generic data passed to user defined function.
 *
 * This function will be called after get_msg_cb private function, which is
 * used as tinymail operation callback. The private function fills private 
 * fields of mail operation and calls user defined callback if it exists.
 */
typedef void (*GetMsgAsyncUserCallback) (ModestMailOperation *mail_op, 
					 TnyHeader *header, 
					 gboolean canceled, 
					 TnyMsg *msg, 
					 GError *err,
					 gpointer user_data);

/**
 * GetMimePartSizeCallback:
 *
 * @mail_op: the current #ModestMailOperation.
 * @size: size of the attachment
 * @user_data: generic data passed to user defined function.
 *
 */
typedef void (*GetMimePartSizeCallback) (ModestMailOperation *mail_op, 
					 gssize size,
					 gpointer user_data);

/**
 * XferMsgsAsyncUserCallback:
 *
 * @obj: a #GObject generic object which has created current mail operation.
 * @new_folder: the new instance of the #TnyFolder that has been transferred
 * @user_data: generic data passed to user defined function.
 *
 * This function will be called after transfer_msgs_cb private function, which is
 * used as tinymail operation callback. The private function fills private 
 * fields of mail operation and calls user defined callback if it exists.
 */
typedef void (*XferMsgsAsyncUserCallback) (ModestMailOperation *mail_op, 
					   gpointer user_data);


/**
 * XferFolderAsyncUserCallback:
 *
 * @obj: a #GObject generic object which has created current mail operation.
 * @new_folder: the new instance of the #TnyFolder that has been transferred
 * @user_data: generic data passed to user defined function.
 *
 * This function will be called after transfer_msgs_cb private function, which is
 * used as tinymail operation callback. The private function fills private 
 * fields of mail operation and calls user defined callback if it exists.
 */
typedef void (*XferFolderAsyncUserCallback) (ModestMailOperation *mail_op, 
					     TnyFolder *new_folder,
					     gpointer user_data);


/**
 * RefreshAsyncUserCallback:
 *
 * @mail_op: the current #ModestMailOperation.
 * @folder: a #TnyFolder which has been refreshed .
 * @user_data: generic data passed to user defined function.
 *
 * This function will be called after refresh_folder_async_cb private function, which is
 * used as tinymail operation callback. The private function fills private 
 * fields of mail operation and calls user defined callback if it exists.
 */
typedef void (*RefreshAsyncUserCallback) (ModestMailOperation *mail_op, 
					  TnyFolder *folder, 
					  gpointer user_data);

/**
 * UpdateAccountCallback:
 *
 * @self: a #ModestMailOperation
 * @new_headers: the list of new headers received
 * @user_data: generic data passed to user defined function.
 *
 * This is the callback of the update_account operation. It informs
 * the caller about the amount of new messages that have been
 * downloaded
 */
typedef void (*UpdateAccountCallback) (ModestMailOperation *self, 
				       TnyList *new_headers,
				       gpointer user_data);

/**
 * SaveToDraftsCallback:
 *
 * @self: a #ModestMailOperation
 * @saved_draft: the new draft message that has been saved
 * @user_data: generic data passed to user defined function.
 *
 * This is the callback of the save_to_drafts operation. It returns
 * the newly created msg stored in the Drafts folder
 */
typedef void (*SaveToDraftstCallback) (ModestMailOperation *self, 
				       TnyMsg *saved_draft,
				       gpointer user_data);


typedef gboolean (*RetrieveAllCallback) (GObject *source,
					 guint num_msgs,
					 guint limit);

/**
 * CreateFolderUserCallback:
 *
 * @mail_op: the current #ModestMailOperation.
 * @folder: a #TnyFolder summary item.
 * @user_data: generic data passed to user defined function.
 *
 * This function will be called after get_msg_cb private function, which is
 * used as tinymail operation callback. The private function fills private 
 * fields of mail operation and calls user defined callback if it exists.
 */
typedef void (*CreateFolderUserCallback) (ModestMailOperation *mail_op, 
					  TnyFolderStore *parent_folder,
					  TnyFolder *new_folder, 
					  gpointer user_data);

/**
 * SyncFolderCallback:
 *
 * @self: a #ModestMailOperation
 * @folder: the #TnyFolder to sync
 * @user_data: generic data passed to user defined function.
 *
 * This is the callback of the sync_folder operation.
 */
typedef void (*SyncFolderCallback) (ModestMailOperation *self,
				    TnyFolder *folder,
				    gpointer user_data);


/* This struct represents the internal state of a mail operation in a
   given time */
typedef struct {
	guint      done;
	guint      total;
	gdouble    bytes_done;
	gdouble    bytes_total;
	gboolean   finished;
	ModestMailOperationStatus        status;
	ModestMailOperationTypeOperation op_type;
} ModestMailOperationState;


struct _ModestMailOperation {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestMailOperationClass {
	GObjectClass parent_class;

	/* Signals */
	void (*progress_changed) (ModestMailOperation *self, ModestMailOperationState *state, gpointer user_data);
	void (*operation_started) (ModestMailOperation *self, gpointer user_data);
	void (*operation_finished) (ModestMailOperation *self, gpointer user_data);
};

/* member functions */
GType        modest_mail_operation_get_type    (void) G_GNUC_CONST;

/**
 * modest_mail_operation_new:
 * @source: a #GObject which creates this new operation.
 * 
 * Creates a new instance of class #ModestMailOperation, using parameters
 * to initialize its private structure. @source parameter may be NULL.
 **/
ModestMailOperation*    modest_mail_operation_new     (GObject *source);

/**
 * modest_mail_operation_new_with_error_handling:
 * @id: a #ModestMailOperationId identification of operation type.
 * @source: a #GObject which creates this new operation.
 * @error_handler: a #ErrorCheckingUserCallback function to performs operations when 
 * an error occurs.
 * 
 * Creates a new instance of class #ModestMailOperation, using parameters
 * to initialize its private structure. @source parameter may be NULL. 
 * @error_handler can not be NULL, but it will be returned an mail operation
 * object without error handling capability.
 **/
ModestMailOperation*    modest_mail_operation_new_with_error_handling     (GObject *source,
									   ErrorCheckingUserCallback error_handler,
									   gpointer user_data,
									   ErrorCheckingUserDataDestroyer error_handler_destroyer);
/**
 * modest_mail_operation_execute_error_handler
 * @self: a #ModestMailOperation
 * 
 * Executes error handler if exists. The error handler is the one that
 * MUST free the user data passed to the
 * modest_mail_operation_new_with_error_handling constructor
 **/
void
modest_mail_operation_execute_error_handler (ModestMailOperation *self);

/**
 * modest_mail_operation_get_type_operation
 * @self: a #ModestMailOperation
 * 
 * Gets the private op_type field of mail operation. This op_type
 * identifies the class/type of mail operation.
 **/
ModestMailOperationTypeOperation
modest_mail_operation_get_type_operation (ModestMailOperation *self);

/**
 * modest_mail_operation_is_mine
 * @self: a #ModestMailOperation
 * @source: a #GObject to check if it have created @self operation.
 * 
 * Check if @source object its owner of @self mail operation.
 *
 * returns: TRUE if source its owner, FALSE otherwise.
 **/
gboolean 
modest_mail_operation_is_mine (ModestMailOperation *self, 
			       GObject *me);

/**
 * modest_mail_operation_get_source
 * @self: a #ModestMailOperation
 *
 * returns a new reference to the object that created the mail
 * operation passed to the constructor, or NULL if none. The caller
 * must free the new reference
 **/
GObject *
modest_mail_operation_get_source (ModestMailOperation *self);

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
 * @draft_msg: a #TnyMsg of the origin draft message, if any
 * @from: the email address of the mail sender
 * @to: a non-NULL email address of the mail receiver
 * @cc: a comma-separated list of email addresses where to send a carbon copy
 * @bcc: a comma-separated list of email addresses where to send a blind carbon copy
 * @subject: the subject of the new mail
 * @plain_body: the plain text body of the new mail.
 * @html_body: the html version of the body of the new mail. If NULL, the mail will
 *             be sent with the plain body only.
 * @attachments_list: a #GList of attachments, each attachment must be a #TnyMimePart
 * @images_list: a #GList of image attachments, each attachment must be a #TnyMimePart
 * 
 * Sends a new mail message using the provided
 * #TnyTransportAccount. This operation is synchronous, so the
 * #ModestMailOperation should not be added to any
 * #ModestMailOperationQueue
  **/
void    modest_mail_operation_send_new_mail   (ModestMailOperation *self,
					       TnyTransportAccount *transport_account,
					       TnyMsg *draft_msg,
					       const gchar *from,
					       const gchar *to,
					       const gchar *cc,
					       const gchar *bcc,
					       const gchar *subject,
					       const gchar *plain_body,
					       const gchar *html_body,
					       const GList *attachments_list,
					       const GList *images_list,
					       const gchar *references,
					       const gchar *in_reply_to,
					       TnyHeaderFlags priority_flags);


/**
 * modest_mail_operation_save_to_drafts:
 * @self: a #ModestMailOperation
 * @transport_account: a non-NULL #TnyTransportAccount
 * @draft_msg: the previous draft message, in case it's an update
 * to an existing draft.
 * @from: the email address of the mail sender
 * @to: a non-NULL email address of the mail receiver
 * @cc: a comma-separated list of email addresses where to send a carbon copy
 * @bcc: a comma-separated list of email addresses where to send a blind carbon copy
 * @subject: the subject of the new mail
 * @plain_body: the plain text body of the new mail.
 * @html_body: the html version of the body of the new mail. If NULL, the mail will
 *             be sent with the plain body only.
 * @attachments_list: a #GList of attachments, each attachment must be a #TnyMimePart
 * @images_list: a #GList of image attachments, each attachment must be a #TnyMimePart
 * @callback: the user callback, will be called when the operation finishes
 * @user_data: data that will be passed to the user callback
 *
 * Save a mail message to drafts using the provided
 * #TnyTransportAccount. This operation is asynchronous.
 *
  **/
void modest_mail_operation_save_to_drafts   (ModestMailOperation *self,
					     TnyTransportAccount *transport_account,
					     TnyMsg *draft_msg,
					     const gchar *from,
					     const gchar *to,
					     const gchar *cc,
					     const gchar *bcc,
					     const gchar *subject,
					     const gchar *plain_body,
					     const gchar *html_body,
					     const GList *attachments_list,
					     const GList *images_list,
					     TnyHeaderFlags priority_flags,
					     const gchar *references,
					     const gchar *in_reply_to,
					     SaveToDraftstCallback callback,
					     gpointer user_data);
/**
 * modest_mail_operation_update_account:
 * @self: a #ModestMailOperation
 * @account_name: the id of a Modest account
 * @poke_all: if TRUE it will also do a poke_status over all folders of the account
 * @interactive: if TRUE the update account was scheduled by an interactive send receive
 * 
 * Asynchronously refreshes the root folders of the given store
 * account. The caller should add the #ModestMailOperation to a
 * #ModestMailOperationQueue and then free it. The caller will be
 * notified by the "progress_changed" signal each time the progress of
 * the operation changes.
 *
 * Example
 * <informalexample><programlisting>
 * queue = modest_tny_platform_factory_get_modest_mail_operation_queue_instance (fact)
 * mail_op = modest_mail_operation_new ();
 * g_signal_connect (G_OBJECT (mail_op), "progress_changed", G_CALLBACK(on_progress_changed), NULL);
 * modest_mail_operation_queue_add (queue, mail_op);
 * modest_mail_operation_update_account (mail_op, account_name)
 * g_object_unref (G_OBJECT (mail_op));
 * </programlisting></informalexample>
 *
 * Note that the account_name *MUST* be a modest account name, not a
 * tinymail store account name
 * 
 **/
void          modest_mail_operation_update_account (ModestMailOperation *self,
						    const gchar *account_name,
						    gboolean poke_all,
						    gboolean interactive,
						    UpdateAccountCallback callback,
						    gpointer user_data);

/**
 * modest_mail_operation_update_folder_counts:
 * @self: a #ModestMailOperation
 * @account_name: the id of a Modest account
 * 
 * Asynchronously refreshes the folder counts of the given store
 * account.
 */
void          modest_mail_operation_update_folder_counts (ModestMailOperation *self,
							  const gchar *account_name);

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
void    modest_mail_operation_create_folder  (ModestMailOperation *self,
					      TnyFolderStore *parent,
					      const gchar *name,
					      CreateFolderUserCallback callback,
					      gpointer user_data);

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
						    const gchar *name,
						    XferFolderAsyncUserCallback user_callback,
						    gpointer user_data);

/**
 * modest_mail_operation_xfer_folder:
 * @self: a #ModestMailOperation
 * @folder: a #TnyFolder
 * @parent: the new parent of the folder as #TnyFolderStore
 * @delete_original: wheter or not delete the original folder
 * @user_callback: a #XferFolderAsyncUserCallback function to call after tinymail callback execution.
 * @user_data: generic user data which will be passed to @user_callback function.
 * 
 * Sets the given @folder as child of a provided #TnyFolderStore. This
 * operation also transfers all the messages contained in the folder
 * and all of his children folders with their messages as well. This
 * operation is synchronous, so the #ModestMailOperation should not be
 * added to any #ModestMailOperationQueue.
 *
 * If @delete_original is TRUE this function moves the original
 * folder, if it is FALSE the it just copies it
 *
 **/
void          modest_mail_operation_xfer_folder    (ModestMailOperation *self,
						    TnyFolder *folder,
						    TnyFolderStore *parent,
						    gboolean delete_original,
						    XferFolderAsyncUserCallback user_callback,
						    gpointer user_data);
						    

/* Functions that performs msg operations */

/**
 * modest_mail_operation_xfer_msgs:
 * @self: a #ModestMailOperation
 * @header_list: a #TnyList of #TnyHeader to transfer
 * @folder: the #TnyFolder where the messages will be transferred
 * @delete_original: whether or not delete the source messages
 * @user_callback: a #XferFolderAsyncUserCallback function to call after tinymail callback execution.
 * @user_data: generic user data which will be passed to @user_callback function.
 * 
 * Asynchronously transfers messages from their current folder to
 * another one. The caller should add the #ModestMailOperation to a
 * #ModestMailOperationQueue and then free it. The caller will be
 * notified by the "progress_changed" when the operation is completed.
 *
 * If the @delete_original paramter is TRUE then this function moves
 * the messages between folders, otherwise it copies them.
 * 
 * Example
 * <informalexample><programlisting>
 * queue = modest_tny_platform_factory_get_modest_mail_operation_queue_instance (fact);
 * mail_op = modest_mail_operation_new ();
 * modest_mail_operation_queue_add (queue, mail_op);
 * g_signal_connect (G_OBJECT (mail_op), "progress_changed", G_CALLBACK(on_progress_changed), queue);
 *
 * modest_mail_operation_xfer_msg (mail_op, headers, folder, TRUE);
 * 
 * g_object_unref (G_OBJECT (mail_op));
 * </programlisting></informalexample>
 *
 **/
void          modest_mail_operation_xfer_msgs      (ModestMailOperation *self,
						    TnyList *header_list, 
						    TnyFolder *folder,
						    gboolean delete_original,
						    XferMsgsAsyncUserCallback user_callback,
						    gpointer user_data);

/**
 * modest_mail_operation_remove_msgs:
 * @self: a #ModestMailOperation
 * @headers: the #TnyList of the messages to delete
 * @remove_to_trash: TRUE to move it to trash or FALSE to delete it
 * permanently
 * 
 * Deletes a list of messages.
 **/
void          modest_mail_operation_remove_msgs     (ModestMailOperation *self,
						     TnyList *headers,
						     gboolean remove_to_trash);

/**
 * modest_mail_operation_get_msg_and_parts:
 * @self: a #ModestMailOperation
 * @header_list: the #TnyHeader of the message to get
 * @progress_feedback: a #gboolean. If %TRUE, we'll get progress bar feedback.
 * @user_callback: a #GetMsgAsyncUserCallback function to call after tinymail callback execution.
 * @user_data: generic user data which will be passed to @user_callback function.
 * 
 * Gets a message from header using an user defined @callback function
 * pased as argument. This operation is asynchronous, so the
 * #ModestMailOperation should be added to #ModestMailOperationQueue
 *
 * This operation also retrieves to cache all parts of the message. This is needed for
 * forward operation.
 **/
void          modest_mail_operation_get_msg_and_parts     (ModestMailOperation *self,
							   TnyHeader *header,
							   TnyList *parts,
							   gboolean progress_feedback,
							   GetMsgAsyncUserCallback user_callback,
							   gpointer user_data);
/**
 * modest_mail_operation_get_msg:
 * @self: a #ModestMailOperation
 * @header_list: the #TnyHeader of the message to get
 * @progress_feedback: a #gboolean. If %TRUE, we'll get progress bar feedback.
 * @user_callback: a #GetMsgAsyncUserCallback function to call after tinymail callback execution.
 * @user_data: generic user data which will be passed to @user_callback function.
 * 
 * Gets a message from header using an user defined @callback function
 * pased as argument. This operation is asynchronous, so the
 * #ModestMailOperation should be added to #ModestMailOperationQueue
 **/
void          modest_mail_operation_get_msg     (ModestMailOperation *self,
						 TnyHeader *header,
						 gboolean progress_feedback,
						 GetMsgAsyncUserCallback user_callback,
						 gpointer user_data);
/**
 * modest_mail_operation_find_msg:
 * @self: a #ModestMailOperation
 * @msg_uid: a string
 * @progress_feedback: a #gboolean. If %TRUE, we'll get progress bar feedback.
 * @user_callback: a #GetMsgAsyncUserCallback function to call after tinymail callback execution.
 * @user_data: generic user data which will be passed to @user_callback function.
 * 
 * Gets a message from a uid using an user defined @callback function
 * pased as argument. This operation is asynchronous, so the
 * #ModestMailOperation should be added to #ModestMailOperationQueue
 **/
void          modest_mail_operation_find_msg     (ModestMailOperation *self,
						  TnyFolder *folder,
						  const gchar *msg_uid,
						  gboolean progress_feedback,
						  GetMsgAsyncUserCallback user_callback,
						  gpointer user_data);

/**
 * modest_mail_operation_get_msgs_full:
 * @self: a #ModestMailOperation
 * @header_list: a #TnyList of #TnyHeader objects to get and process
 * @user_callback: a #TnyGetMsgCallback function to call after tinymail operation execution.
 * @user_data: user data passed to both, user_callback and update_status_callback.
 * 
 * Gets messages from headers list and process hem using @callback function
 * pased as argument. This operation is asynchronous, so the
 * #ModestMailOperation should be added to #ModestMailOperationQueue
 **/
void          modest_mail_operation_get_msgs_full   (ModestMailOperation *self,
						     TnyList *headers_list,
						     GetMsgAsyncUserCallback user_callback,
						     gpointer user_data,
						     GDestroyNotify notify);

/**
 * modest_mail_operation_run_queue:
 * @self: a #ModestMailOperation
 * @queue: a #ModestTnySendQueue
 *
 * This mail operation is special. It should be running every time the send queue
 * is running (after queue-start), and we should notify end of the operation
 * after queue-end. Then, we should only set this queue on queue-start signal, and
 * it will clean up the operation (notify end) on queue-end.
 */
void          modest_mail_operation_run_queue       (ModestMailOperation *self,
						     ModestTnySendQueue *queue);

/**
 * modest_mail_operation_queue_wakeup:
 * @self: a #ModestMailOperation
 * @queue: a #ModestTnySendQueue
 *
 * This mail operation is special. It should be running every time the send queue
 * wakeup is running and we should notify end of the operation
 * after wakeup has done msg-sent notification.
 */
void          modest_mail_operation_queue_wakeup       (ModestMailOperation *self,
							ModestTnySendQueue *queue);

/**
 * modest_mail_operation_sync_folder:
 * @self: a #ModestMailOperation
 * @folder: a #TnyFolder
 * @expunge: a #gboolean
 *
 * mail operation wrapper around tny_folder_sync () method, to keep modest
 * running while we do that sync operation.
 */
void          modest_mail_operation_sync_folder     (ModestMailOperation *self,
						     TnyFolder *folder,
						     gboolean expunge,
						     SyncFolderCallback callback,
						     gpointer user_data);

/**
 * modest_mail_operation_shutdown:
 * @self: a #ModestMailOperation
 * @account_store: a #ModestTnyAccountStore
 *
 * disconnects all accounts in the account store (doing the proper syncs).
 */
void          modest_mail_operation_shutdown        (ModestMailOperation *self,
						     ModestTnyAccountStore *account_store);

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
gboolean  modest_mail_operation_is_finished        (ModestMailOperation *self);

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
gboolean  modest_mail_operation_cancel          (ModestMailOperation *self);

/**
 * modest_mail_operation_refresh_folder
 * @self: a #ModestMailOperation
 * @folder: the #TnyFolder to refresh
 * @user_callback: the #RefreshAsyncUserCallback function to be called
 * after internal refresh async callback was being executed.
 * 
 * Refreshes the contents of a folder. After internal callback was executed, 
 * and all interna mail operation field were filled, if exists, it calls an 
 * user callback function to make UI operations which must be done after folder
 * was refreshed.
 */
void      modest_mail_operation_refresh_folder  (ModestMailOperation *self,
						 TnyFolder *folder,
						 RefreshAsyncUserCallback user_callback,
						 gpointer user_data);

/**
 * modest_mail_operation_get_account:
 * @self: a #ModestMailOperation
 * 
 * Gets the account associated to a mail operation
 * 
 * Returns: the #TnyAccount associated to the #ModestMailOperation
 **/
TnyAccount *modest_mail_operation_get_account   (ModestMailOperation *self);


/**
 * modest_mail_operation_noop:
 * @self: a #ModestMailOperation
 * 
 * Does nothing except emitting operation-started and
 * operation-finished
 **/
void modest_mail_operation_noop (ModestMailOperation *self);


/**
 * modest_mail_operation_to_string:
 * @self: a #ModestMailOperation
 * 
 * get a string representation of the mail operation (for debugging)
 *
 * Returns: a newly allocated string
 **/
gchar* modest_mail_operation_to_string (ModestMailOperation *self);


G_END_DECLS

#endif /* __MODEST_MAIL_OPERATION_H__ */


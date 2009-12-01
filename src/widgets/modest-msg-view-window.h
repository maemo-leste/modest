/* Copyright (c) 2006,2007 Nokia Corporation
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

#ifndef __MODEST_MSG_VIEW_WINDOW_H__
#define __MODEST_MSG_VIEW_WINDOW_H__

#include <tny-msg.h>
#include <tny-folder.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-hildon2-window.h>
#else
#include <modest-shell-window.h>
#endif
#include <widgets/modest-window.h>
#include <widgets/modest-header-view.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MSG_VIEW_WINDOW             (modest_msg_view_window_get_type())
#define MODEST_MSG_VIEW_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MSG_VIEW_WINDOW,ModestMsgViewWindow))
#define MODEST_MSG_VIEW_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MSG_VIEW_WINDOW,ModestWindow))
#define MODEST_IS_MSG_VIEW_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MSG_VIEW_WINDOW))
#define MODEST_IS_MSG_VIEW_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MSG_VIEW_WINDOW))
#define MODEST_MSG_VIEW_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MSG_VIEW_WINDOW,ModestMsgVIewWindowClass))


typedef struct {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2Window parent;
#else
	ModestShellWindow parent;
#endif
} ModestMsgViewWindow;
	
typedef struct {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2WindowClass parent_class;
#else
	ModestShellWindowClass parent_class;
#endif

	void (*msg_changed) (ModestMsgViewWindow *self,
			     GtkTreeModel *model,
			     GtkTreeRowReference *row_reference, 
			     gpointer user_data);

	gboolean (*scroll_child) (ModestMsgViewWindow *self,
				   GtkScrollType scroll_type,
				   gboolean horizontal,
				   gpointer userdata);
} ModestMsgViewWindowClass;

/**
 * modest_msg_view_window_get_type:
 * 
 * get the GType for the #ModestMsgViewWindow class
 *
 * Returns: a GType for #ModestMsgViewWindow
 */
GType        modest_msg_view_window_get_type    (void) G_GNUC_CONST;
	

/**
 * modest_msg_view_window_new_for_attachment:
 * @msg: an #TnyMsg instance
 * @modest_account_name: the account name 
 * @mailbox: the mailbox (if any)
 * 
 * instantiates a new #ModestMsgViewWindow widget to view a message that is an
 * attachment in another message.
 * The account name is used to
 * set the proper account when choosing reply/forward from the msg view window
 *
 * Returns: a new #ModestMsgViewWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_view_window_new_for_attachment         (TnyMsg *msg, 
								   const gchar *modest_account_name,
								   const gchar *mailbox,
								   const gchar *msg_uid);

/**
 * modest_msg_view_window_new_with_other_body:
 * @msg: an #TnyMsg instance
 * @modest_account_name: the account name 
 * @mailbox: the mailbox (if any)
 * 
 * instantiates a new #ModestMsgViewWindow widget to view a message that is a different body
 * in another message.
 * The account name is used to
 * set the proper account when choosing reply/forward from the msg view window
 *
 * Returns: a new #ModestMsgViewWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_view_window_new_with_other_body         (TnyMsg *msg,
								   TnyMimePart *other_body,
								   const gchar *modest_account_name,
								   const gchar *mailbox,
								   const gchar *msg_uid);

/**
 * modest_msg_view_window_is_other_body:
 * @self: a #ModestMsgViewWindow
 *
 * tells if the view window is showing other body
 *
 * Returns: %TRUE if showing "not first body"
 */
gboolean modest_msg_view_window_is_other_body (ModestMsgViewWindow *self);

/**
 * modest_msg_view_window_new_with_header_model:
 * @msg: an #TnyMsg instance
 * @modest_account_name: the account name 
 * @mailbox: the mailbox (if any)
 * @model: a #GtkTreeModel, with the format used by #ModestHeaderView
 * @row_reference: a #GtkTreeRowReference, pointing to the position of @msg in @model.
 * 
 * instantiates a new #ModestMsgViewWindow widget. The account name is used to
 * set the proper account when choosing reply/forward from the msg view window.
 * This constructor also passes a reference to the @model of the header view
 * to allow selecting previous/next messages in the message list when appropriate.
 *
 * Returns: a new #ModestMsgViewWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_view_window_new_with_header_model (TnyMsg *msg, 
							      const gchar *modest_account_name, 
							      const gchar *mailbox,
							      const gchar *msg_uid,
							      GtkTreeModel *model, 
							      GtkTreeRowReference *row_reference);

/**
 * modest_msg_view_window_new_from_header_view:
 * @header_view: an #ModestHeaderView instance
 * @modest_account_name: the account name 
 * @mailbox: the mailbox (if any)
 * @msg_uid: the initial uid reserved by this window
 * @row_reference: a #GtkTreeRowReference, pointing to the selected position @model.
 * 
 * instantiates a new #ModestMsgViewWindow widget. The account name is used to
 * set the proper account when choosing reply/forward from the msg view window.
 * It's different from new_with_header_model, as it creates the window and then
 * loads the message in that window.
 *
 * Returns: a new #ModestMsgViewWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_view_window_new_from_header_view (ModestHeaderView *header_view, 
							     const gchar *modest_account_name, 
							     const gchar *mailbox,
							     const gchar *msg_uid,
							     GtkTreeRowReference *row_reference);


/**
 * modest_msg_view_window_new_from_uid:
 */
ModestWindow *
modest_msg_view_window_new_from_uid (const gchar *modest_account_name,
				     const gchar *mailbox,
				     const gchar *msg_uid);
					      
/**
 * modest_msg_view_window_new_for_search_result:
 * @msg: an #TnyMsg instance
 * @modest_account_name: the account name 
 * 
 * instantiates a new #ModestMsgViewWindow widget. The account name is used to
 * set the proper account when choosing reply/forward from the msg view window.
 * This constructor marks the window as being for a search result, which should 
 * cause some UI to be disabled, such as the previous/next buttons.
 *
 * Returns: a new #ModestMsgViewWindow, or NULL in case of error
 */
ModestWindow *
modest_msg_view_window_new_for_search_result (TnyMsg *msg, 
					      const gchar *modest_account_name,
					      const gchar *mailbox,
					      const gchar *msg_uid);
					      
/**
 * modest_msg_view_window_get_header:
 * @window: an #ModestMsgViewWindow instance
 * 
 * get the message header in this msg view. Header instance is get
 * from tree_model of headers list. 
 * 
 * Returns: a new #TnyHeader instance, or NULL in case of error
 */
TnyHeader*
modest_msg_view_window_get_header (ModestMsgViewWindow *self);

/**
 * modest_msg_view_window_get_message:
 * @window: an #ModestMsgViewWindow instance
 * 
 * get a new reference to the message in this msg view. The caller
 * must free this new reference
 * 
 * Returns: a new #TnyMsg instance, or NULL in case of error
 */
TnyMsg*         modest_msg_view_window_get_message     (ModestMsgViewWindow *window);

/**
 * modest_msg_view_window_get_message_uid:
 * @msg: an #ModestMsgViewWindow instance
 * 
 * gets the unique identifier for the message in this msg view. The
 * returned value *must* not be freed
 * 
 * Returns: the id of the #TnyMsg being shown, or NULL in case of error
 */
const gchar*    modest_msg_view_window_get_message_uid (ModestMsgViewWindow *window);

/**
 * modest_msg_view_window_select_next_message:
 * @window: a #ModestMsgViewWindow instance
 *
 * select the next message obtained from the header view this view 
 * was called from
 *
 * Returns: %TRUE if a new message is shown.
 */
gboolean        modest_msg_view_window_select_next_message (ModestMsgViewWindow *window);

/**
 * modest_msg_view_window_select_previous_message:
 * @window: a #ModestMsgViewWindow instance
 *
 * select the previous message obtained from the header view this view 
 * was called from
 *
 * Returns: %TRUE if a new message is shown.
 */
gboolean        modest_msg_view_window_select_previous_message (ModestMsgViewWindow *window);

/**
 * modest_msg_view_window_view_attachment:
 * @window: a #ModestMsgViewWindow
 * @mime_part: a #TnyMimePart
 *
 * Opens @mime_part, or the currently selected attachment if @mime_part is %NULL. 
 * If it's a message, it opens it  for viewing. Otherwise it opens a temporary file 
 * with the contents of the attachment.
 */
void            modest_msg_view_window_view_attachment (ModestMsgViewWindow *window,
							TnyMimePart *mime_part);

/**
 * modest_msg_view_window_get_attachments:
 * @window: a #ModestMsgViewWindow
 *
 * Get selected attachments from #ModetMsgView private object.  
 */
TnyList *         modest_msg_view_window_get_attachments (ModestMsgViewWindow *win);

/**
 * modest_msg_view_window_save_attachments:
 * @window: a #ModestMsgViewWindow
 * @mime_parts: a #TnyList of #TnyMimePart
 *
 * Save the #TnyMimePart attachments in @mime_parts, or currently selected attachments
 * if @mime_parts is %NULL, offering a dialog to the user to choose the location.
 */
void            modest_msg_view_window_save_attachments (ModestMsgViewWindow *window,
							 TnyList *mime_parts);

/**
 * modest_msg_view_window_remove_attachments:
 * @window: a #ModestMsgViewWindow
 * @get_all: a #gboolean. If %TRUE, purges all attachmnents, if %FALSE,
 * purges only selected ones.
 *
 * Removes selected attachments.
 */
void            modest_msg_view_window_remove_attachments (ModestMsgViewWindow *window,
							   gboolean get_all);


/**
 * modest_msg_view_window_toolbar_on_transfer_mode:
 * @window: a #ModestMsgViewWindow
 *
 * Check if toolbar is in transfer mode, which determines whether a
 * transfer operation is being processed.
 */
gboolean  modest_msg_view_window_toolbar_on_transfer_mode     (ModestMsgViewWindow *self);


/**
 * modest_msg_view_window_last_message_selected:
 * @window: a #ModestMsgViewWindow
 *
 * Check message currently viewed is the last message into folder . 
*/
gboolean modest_msg_view_window_last_message_selected (ModestMsgViewWindow *window); 


/**
 * modest_msg_view_window_first_message_selected:
 * @window: a #ModestMsgViewWindow
 *
 * Check message currently viewed is the last message into folder . 
*/
gboolean modest_msg_view_window_first_message_selected (ModestMsgViewWindow *window);

/**
 * modest_msg_view_window_has_headers_model:
 * @window: a #ModestMsgViewWindow
 *
 * Check if window has been created with a full headers model. 
*/
gboolean modest_msg_view_window_has_headers_model (ModestMsgViewWindow *window);

/**
 * modest_msg_view_window_is_search_result:
 * @window: a #ModestMsgViewWindow
 *
 * Check if window has been created to show a search result. 
 */
gboolean modest_msg_view_window_is_search_result (ModestMsgViewWindow *window);


/**
 * modest_msg_view_window_get_folder_type:
 * @window: a #ModestMsgViewWindow
 *
 * Gets folder type of message currently viewed . 
*/
TnyFolderType
modest_msg_view_window_get_folder_type (ModestMsgViewWindow *window);

/**
 * modest_msg_view_window_transfer_mode_enabled:
 * @window: a #ModestMsgViewWindow
 *
 * Determines if some transfer operation is in progress.
 *
 * Returns: TRUE if transfer mode is enabled, FASE otherwise.
*/
gboolean 
modest_msg_view_window_transfer_mode_enabled (ModestMsgViewWindow *self);

/**
 * modest_msg_view_window_add_to_contacts:
 * @self: a #ModestMsgViewWindow
 *
 * activates the add to contacts use case. In Diablo and gnome it gets the
 * clipboard selection current value and tries to add it to the addressbook.
 * In fremantle, it shows the add to contacts dialog to select the recipient
 * to add.
 */
void
modest_msg_view_window_add_to_contacts (ModestMsgViewWindow *self);

/**
 * modest_msg_view_window_get_msg_view:
 * @self: a #ModestMsgViewWindow
 *
 * Tells that external images should be fetched in this window.
 */
void
modest_msg_view_window_fetch_images (ModestMsgViewWindow *self);

/**
 * modest_msg_view_window_has_blocked_external_images:
 * @self: a #ModestMsgViewWindow
 * 
 * checks if the msg currently shown has blocked external images.
 *
 * Returns: %TRUE if external images are blocked, %FALSE otherwise
 */
gboolean modest_msg_view_window_has_blocked_external_images (ModestMsgViewWindow *self);

/**
 * modest_msg_view_window_reload:
 * @self: a #ModestMsgViewWindow
 *
 * Reloads currently loaded message. This is intended to show the message in case it
 * has some update on the previously visible result.
 */
void modest_msg_view_window_reload (ModestMsgViewWindow *self);

G_END_DECLS

#endif /* __MODEST_MSG_VIEW_WINDOW_H__ */


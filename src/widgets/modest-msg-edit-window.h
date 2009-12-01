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

#ifndef __MODEST_MSG_EDIT_WINDOW_H__
#define __MODEST_MSG_EDIT_WINDOW_H__

#include <tny-msg.h>
#include <tny-vfs-stream.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-hildon2-window.h>
#else
#include <modest-shell-window.h>
#endif
#include <widgets/modest-window.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MSG_EDIT_WINDOW             (modest_msg_edit_window_get_type())
#define MODEST_MSG_EDIT_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MSG_EDIT_WINDOW,ModestMsgEditWindow))
#define MODEST_MSG_EDIT_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MSG_EDIT_WINDOW,ModestWindow))
#define MODEST_IS_MSG_EDIT_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MSG_EDIT_WINDOW))
#define MODEST_IS_MSG_EDIT_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MSG_EDIT_WINDOW))
#define MODEST_MSG_EDIT_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MSG_EDIT_WINDOW,ModestMsgEditWindowClass))

typedef struct _ModestMsgEditWindow      ModestMsgEditWindow;
typedef struct _ModestMsgEditWindowClass ModestMsgEditWindowClass;

struct _ModestMsgEditWindow {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2Window parent;
#else
	ModestShellWindow parent;
#endif
	/* insert public members, if any */
};

struct _ModestMsgEditWindowClass {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2WindowClass parent_class;
#else
	ModestShellWindowClass parent_class;
#endif
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestMsgEditWindow* obj); */
};

typedef enum  {
	MODEST_EDIT_TYPE_NEW,
	MODEST_EDIT_TYPE_REPLY,
	MODEST_EDIT_TYPE_FORWARD,
	
	MODEST_EDIT_TYPE_NUM
} ModestEditType;

typedef enum {
	MODEST_MSG_EDIT_FORMAT_TEXT,
	MODEST_MSG_EDIT_FORMAT_HTML
} ModestMsgEditFormat;

typedef enum {
	MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BODY,
	MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_TO,
	MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_CC,
	MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_BCC,
	MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_SUBJECT,
	MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_ATTACHMENTS,
	MODEST_MSG_EDIT_WINDOW_WIDGET_TYPE_NUM,
} ModestMsgEditWindowWidgetType;

/** Get these with modest_msg_edit_window_get_msg_data() 
 * and free them with modest_msg_edit_window_free_msg_data().
 */
typedef struct  {
	gchar *from, *to, *cc, *bcc, *subject, *plain_body, *html_body;
	gchar *references, *in_reply_to;
	GList *attachments;
	GList *images;
	TnyHeaderFlags priority_flags;
	TnyMsg *draft_msg;
	gchar *account_name;
} MsgData;

typedef struct {
	gboolean bold;
	gboolean italics;
	gboolean bullet;
	GdkColor color;
	const gchar *font_family;
	gint font_size;
	GtkJustification justification;
} ModestMsgEditFormatState;


/**
 * modest_msg_edit_window_get_type:
 * 
 * get the GType for the #ModestMsgEditWindow class
 *
 * Returns: a GType for #ModestMsgEditWindow
 */
GType        modest_msg_edit_window_get_type    (void) G_GNUC_CONST;


/**
 * modest_msg_edit_window_new:
 * @msg: a #TnyMsg instance
 * @account_name: the account this message applies to
 * @mailbox: the mailbox (if any)
 * @preserve_is_rich: if @msg is not rich, open the message as plain text
 * 
 * instantiates a new #ModestMsgEditWindow widget
 *
 * Returns: a new #ModestMsgEditWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_edit_window_new         (TnyMsg *msg, 
						    const gchar *account_name, 
						    const gchar *mailbox,
						    gboolean preserve_is_rich);


/**
 * modest_msg_edit_window_get_msg_data:
 * @self: a #ModestMsgEditWindow
 * 
 * gets the message data already present in the edit message
 * window. The message data must be freed with
 * modest_msg_edit_window_free_msg_data
 * 
 * Returns: the message data
 **/
MsgData *               modest_msg_edit_window_get_msg_data          (ModestMsgEditWindow *self);

/**
 * modest_msg_edit_window_free_msg_data:
 * @self: a #ModestMsgEditWindow
 * @data: 
 * 
 * frees the message data passed as argument
 **/
void                    modest_msg_edit_window_free_msg_data         (ModestMsgEditWindow *self,
								      MsgData *data);

/**
 * modest_msg_edit_window_get_format:
 * @self: a #ModestMsgEditWindow
 *
 * obtains the format type of the message body.
 *
 * Returns: a #ModestMsgEditFormat
 **/
ModestMsgEditFormat     modest_msg_edit_window_get_format            (ModestMsgEditWindow *self);

/**
 * modest_msg_edit_window_set_format:
 * @self: a #ModestMsgEditWindow
 * @format: a #ModestMsgEditFormat
 *
 * set the @format of the edit window message body.
 **/
void                    modest_msg_edit_window_set_format            (ModestMsgEditWindow *self,
								      ModestMsgEditFormat format);

/**
 * modest_msg_edit_window_get_format_state:
 * @self: a #ModestMsgEditWindow
 *
 * get the current format state (the format attributes the text user inserts
 * will get).
 *
 * Returns: a #ModestMsgEditFormatState structure that should be freed with g_free().
 **/
ModestMsgEditFormatState *modest_msg_edit_window_get_format_state    (ModestMsgEditWindow *self);

/**
 * modest_msg_edit_window_set_format_state:
 * @self: a #ModestMsgEditWindow
 * @format_state: a #ModestMsgEditWindowFormatState
 *
 * sets a new format state (the format attributes the text user inserts 
 * will get).
 **/
void                    modest_msg_edit_window_set_format_state      (ModestMsgEditWindow *self,
								      const ModestMsgEditFormatState *format_state);

/**
 * modest_msg_edit_window_select_color:
 * @self: a #ModestMsgEditWindow
 *
 * show color selection dialog and update text color
 */
void                    modest_msg_edit_window_select_color          (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_select_background_color:
 * @self: a #ModestMsgEditWindow
 *
 * show color selection dialog and update background color
 */
void                    modest_msg_edit_window_select_background_color          (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_insert_image:
 * @self: a #ModestMsgEditWindow
 *
 * show a file selection dialog to insert an image
 */
void                    modest_msg_edit_window_insert_image          (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_offer_attach_file:
 * @self: a #ModestMsgEditWindow
 *
 * show a file selection dialog to attach a file
 */
void                    modest_msg_edit_window_offer_attach_file           (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_attach_file_one:
 * @self: a #ModestMsgEditWindow
 * @file_uri: The URI of a file to attach to the email message.
 * @allowed_size: max size allowed for this attachment, 0 for unlimited
 *
 * attach a file to a MsgEditWindow non interactively, 
 * without file dialog. This is needed by dbus callbacks.
 *
 * Returns: the filesize (if available)
 */
GnomeVFSFileSize modest_msg_edit_window_attach_file_one           (ModestMsgEditWindow *window, const gchar *file_uri, GnomeVFSFileSize allowed_size);

/**
 * modest_msg_edit_window_remove_attachments:
 * @self: a #ModestMsgEditWindow
 * @att_list: a #GList of #TnyMimePart
 *
 * remove attachments in @att_list, with a confirmation dialog
 */
void                    modest_msg_edit_window_remove_attachments    (ModestMsgEditWindow *window, 
								      TnyList *att_list);

/**
 * modest_msg_edit_window_get_parts_size:
 * @window: a #ModestMsgEditWindow
 * @parts_count: number of attachments and images attached to the message
 * @parts_size: sum of sizes of attachments and images
 */
void                    modest_msg_edit_window_get_parts_size (ModestMsgEditWindow *window,
							       gint *parts_count,
							       guint64 *parts_size);
/**
 * modest_msg_edit_window_add_part:
 * @self: a #ModestMsgEditWindow
 * @part: a #TnyMimePart
 *
 * Adds @part as an attachment
 */
void                    modest_msg_edit_window_add_part (ModestMsgEditWindow *window,
							 TnyMimePart *part);

/**
 * modest_msg_edit_window_show_cc:
 * @window: a #ModestMsgEditWindow
 * @show: a #gboolean
 *
 * Set the CC field as visible (or not visible) depending on @show
 */
void                    modest_msg_edit_window_show_cc               (ModestMsgEditWindow *window, gboolean show);

/**
 * modest_msg_edit_window_show_bcc:
 * @window: a #ModestMsgEditWindow
 * @show: a #gboolean
 *
 * Set the BCC field as visible (or not visible) depending on @show
 */
void                    modest_msg_edit_window_show_bcc               (ModestMsgEditWindow *window, gboolean show);

/**
 * modest_msg_edit_window_set_priority_flags:
 * @window: a #ModestMsgEditWindow
 * @priority_flags: a #TnyHeaderFlags with priority information
 *
 * Updates the icon and priority flag to send.
 */
void            modest_msg_edit_window_set_priority_flags (ModestMsgEditWindow *window,
							   TnyHeaderFlags priority_flags);

/**
 * modest_msg_edit_window_set_file_format:
 * @window: a #ModestMsgEditWindow
 * @file_format: %MODEST_FILE_FORMAT_PLAIN_TEXT or %MODEST_FILE_FORMAT_FORMATTED_TEXT
 *
 * Changes the current file format.
 */
void            modest_msg_edit_window_set_file_format (ModestMsgEditWindow *window,
							gint file_format);

/**
 * modest_msg_edit_window_select_font:
 * @window: a #ModestMsgEditWindow
 *
 * Show the dialog to select the editor font and update the
 * used font in the editor.
 */
void            modest_msg_edit_window_select_font        (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_undo:
 * @window: a #ModestMsgEditWindow
 *
 * Undoes the last operation.
 */
void            modest_msg_edit_window_undo               (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_can_undo:
 * @window: a #ModestMsgEditWindow
 *
 * Checks if an undo operation is available
 *
 * Returns: %TRUE if undo can be done, %FALSE otherwise.
 */
gboolean            modest_msg_edit_window_can_undo               (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_redo:
 * @window: a #ModestMsgEditWindow
 *
 * Revert last undo
 */
void            modest_msg_edit_window_redo               (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_can_redo:
 * @window: a #ModestMsgEditWindow
 *
 * Checks if a redp operation is available
 *
 * Returns: %TRUE if redo can be done, %FALSE otherwise.
 */
gboolean            modest_msg_edit_window_can_redo               (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_select_contacts:
 * @window: a #ModestMsgEditWindow
 *
 * Shows the dialog to add contacts to the currently focused recipient list,
 * or to To: recipient if no recipient list is focused.
 */
void            modest_msg_edit_window_select_contacts    (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_add_to_contacts:
 * @self: a #ModestMsgEditWindow
 *
 * activates the add to contacts use. It shows the add to contacts
 * dialog to select the recipient to add.
 */
void            modest_msg_edit_window_add_to_contacts     (ModestMsgEditWindow *self);

/**
 * modest_msg_edit_window_check_names:
 * @window: a #ModestMsgEditWindow
 * @add_to_addressbook: if TRUE, add valid addresses to the addressbook
 *
 * Validates all the recipients, and shows (if required) dialogs for adding contacts
 * or fixing problems in specific fields.
 *
 * Returns: %TRUE if all fields were validated, %FALSE otherwise
 */
gboolean        modest_msg_edit_window_check_names    (ModestMsgEditWindow *window,
						       gboolean add_to_addressbook);

/**
 * modest_msg_edit_window_has_pending_addresses:
 * @window: a #ModestMsgEditWindow
 * @add_to_addressbook: if TRUE, add valid addresses to the addressbook
 *
 * Validates all the recipients, and checks if there are addresses in
 * any field that could be added to the addressbook
 *
 * Returns: %TRUE if there are valid pending addresses, %FALSE otherwise
 */
gboolean        modest_msg_edit_window_has_pending_addresses    (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_toggle_isearch_toolbar:
 * @window: a #ModestMsgEditWindow
 * @show: a #gboolean
 *
 * Shows/Hides the isearch toolbar
 */
void            modest_msg_edit_window_toggle_isearch_toolbar (ModestMsgEditWindow *window,
							       gboolean show);


/**
 * modest_msg_edit_window_is_modified:
 * @window: a #ModestMsgEditWindow
 *
 * Examines whether or not the message has been modified
 *
 * Returns: %TRUE if any field has been modified, %FALSE otherwise
 */
gboolean        modest_msg_edit_window_is_modified         (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_reset_modified:
 * @window: a #ModestMsgEditWindow
 * @modified: wheter or not we want to make the buffer as modified or not
 *
 * Sets the message as modified or not
 */
void            modest_msg_edit_window_set_modified      (ModestMsgEditWindow *window,
							  gboolean modified);


gboolean        modest_msg_edit_window_get_sent            (ModestMsgEditWindow *window);
void            modest_msg_edit_window_set_sent            (ModestMsgEditWindow *window, 
							    gboolean sent);

/**
 * modest_msg_edit_window_set_draft:
 * @window: a #ModestMsgEditWindow
 * @draft: a #TnyMsg, or %NULL
 *
 * Set @draft as the original draft message of the editor. This
 * message will be removed on saving or sending the message.
 */
void            modest_msg_edit_window_set_draft           (ModestMsgEditWindow *window,
							    TnyMsg *draft);
/**
 * modest_msg_edit_window_get_message_uid:
 * @msg: an #ModestMsgEditWindow instance
 * 
 * gets the unique identifier for the message in this msg editor.
 * This is the identifier of the draft or outbox message the editor was
 * opened from. If it's a new message, then it returns %NULL
 * 
 * Returns: the id of the #TnyMsg being shown, or NULL in case of error
 */
const gchar*    modest_msg_edit_window_get_message_uid (ModestMsgEditWindow *window);

/**
 * modest_msg_edit_window_get_child_widget:
 * @win: a #ModestMsgEditWindow
 * @widget_type: the type of the child to obtain
 *
 * Obtain the child widget of @win of type @widget_type
 *
 * Returns: a #GtkWidget, or %NULL
 */
GtkWidget *
modest_msg_edit_window_get_child_widget (ModestMsgEditWindow *win,
					 ModestMsgEditWindowWidgetType widget_type);

/**
 * modest_msg_edit_window_get_clipboard_text:
 * @win: a #ModestMsgEditWindow
 *
 * Obtains the currently selected text in selection clipboard
 *
 * Returns: a string
 */
const gchar *
modest_msg_edit_window_get_clipboard_text (ModestMsgEditWindow *win);

G_END_DECLS

#endif /* __MODEST_MSG_EDIT_WINDOW_H__ */

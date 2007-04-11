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
#include <widgets/modest-window.h>
#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MSG_VIEW_WINDOW             (modest_msg_view_window_get_type())
#define MODEST_MSG_VIEW_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MSG_VIEW_WINDOW,ModestMsgViewWindow))
#define MODEST_MSG_VIEW_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MSG_VIEW_WINDOW,ModestWindow))
#define MODEST_IS_MSG_VIEW_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MSG_VIEW_WINDOW))
#define MODEST_IS_MSG_VIEW_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MSG_VIEW_WINDOW))
#define MODEST_MSG_VIEW_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MSG_VIEW_WINDOW,ModestMsgVIewWindowClass))


typedef struct {
	 ModestWindow parent;
} ModestMsgViewWindow;
	
typedef struct {
	ModestWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestEditMsgWindow* obj); */
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
 * modest_msg_view_window_new:
 * @msg: an #TnyMsg instance
 * @account: the account name 
 * 
 * instantiates a new #ModestMsgViewWindow widget. The account name is used to
 * set the proper account when choosing reply/forward from the msg view window
 *
 * Returns: a new #ModestMsgViewWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_view_window_new         (TnyMsg *msg, const gchar *account);

/**
 * modest_msg_view_window_new_with_header_model:
 * @msg: an #TnyMsg instance
 * @account: the account name 
 * @model: a #GtkTreeModel, with the format used by #ModestHeaderView
 * @iter: a #GtkTreeIter, pointing to the position of @msg in @model.
 * 
 * instantiates a new #ModestMsgViewWindow widget. The account name is used to
 * set the proper account when choosing reply/forward from the msg view window.
 * This constructor also passes a reference to the @model of the header view
 * to allow selecting previous/next messages.
 *
 * Returns: a new #ModestMsgViewWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_view_window_new_with_header_model (TnyMsg *msg, const gchar *account, GtkTreeModel *model, GtkTreeIter iter);


/**
 * modest_msg_view_window_get_message:
 * @window: an #ModestMsgViewWindow instance
 * 
 * get the message in this msg view
 * 
 * Returns: a new #TnyMsg instance, or NULL in case of error
 */
TnyMsg*         modest_msg_view_window_get_message (ModestMsgViewWindow *window);

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

G_END_DECLS

#endif /* __MODEST_MSG_VIEW_WINDOW_H__ */


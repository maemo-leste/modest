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
	 ModestWindow parent;
	/* insert public members, if any */
};

struct _ModestMsgEditWindowClass {
	ModestWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestMsgEditWindow* obj); */
};

typedef enum  {
	MODEST_EDIT_TYPE_NEW,
	MODEST_EDIT_TYPE_REPLY,
	MODEST_EDIT_TYPE_FORWARD,
	MODEST_EDIT_TYPE_VIEW,
	
	MODEST_EDIT_TYPE_NUM
} ModestEditType;

typedef struct _MsgData {
	gchar *from, *to, *cc, *bcc, *subject, *body;
} MsgData;


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
 * 
 * instantiates a new #ModestMsgEditWindow widget
 *
 * Returns: a new #ModestMsgEditWindow, or NULL in case of error
 */
ModestWindow*   modest_msg_edit_window_new         (ModestEditType type);


/**
 * modest_msg_edit_window_set_msg:
 * @self: a #ModestMsgEditWindow
 * @msg: a #TnyMsg
 * 
 * shows the message @msg in a #ModestMsgEditWindow
 **/
void         modest_msg_edit_window_set_msg     (ModestMsgEditWindow *self, 
						 TnyMsg *msg);


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
G_END_DECLS

#endif /* __MODEST_MSG_EDIT_WINDOW_H__ */


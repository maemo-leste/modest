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

#ifndef __MODEST_MSG_VIEW_H__
#define __MODEST_MSG_VIEW_H__

#include <tny-stream.h>
#include <tny-msg.h>
#include <tny-mime-part.h>
#include <tny-msg-view.h>
#include <widgets/modest-recpt-view.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MSG_VIEW             (modest_msg_view_get_type())
#define MODEST_MSG_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MSG_VIEW,ModestMsgView))
#define MODEST_MSG_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MSG_VIEW,ModestMsgViewClass))
#define MODEST_IS_MSG_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MSG_VIEW))
#define MODEST_IS_MSG_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MSG_VIEW))
#define MODEST_MSG_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MSG_VIEW,ModestMsgViewClass))

typedef struct _ModestMsgView      ModestMsgView;
typedef struct _ModestMsgViewClass ModestMsgViewClass;

struct _ModestMsgView {
	GtkVBox parent;
};

struct _ModestMsgViewClass {
	GtkVBoxClass parent_class;

	void (*link_hover)         (ModestMsgView *msgview, const gchar* link,
				    gpointer user_data);
	void (*link_clicked)       (ModestMsgView *msgview, const gchar* link,
				    gpointer user_data);
	void (*attachment_clicked) (ModestMsgView *msgview, int index,
				    gpointer user_data);
	void (*recpt_activated)    (ModestMsgView *msgview, ModestRecptView *recpt_view,
				    gpointer user_data);
};


/**
 *
 * modest_msg_view_get_type
 *
 * get the GType for the this class
 *
 * Returns: the GType for this class
 */
GType        modest_msg_view_get_type    (void) G_GNUC_CONST;


/**
 * modest_msg_view_new 
 * @tny_msg: a TnyMsg instance, or NULL
 *
 * create a new ModestMsgView widget (a GtkScrolledWindow subclass),
 * and display the @tny_msg e-mail message in it. If @tny_msg is NULL,
 * then a blank page will be displayed
 *  
 * Returns: a new ModestMsgView widget, or NULL if there's an error
 */
GtkWidget*   modest_msg_view_new          (TnyMsg *tny_msg);


/**
 * modest_msg_view_set_message
 * @self: a ModestMsgView instance
 * @tny_msg: a TnyMsg instance, or NULL
 *
 * display the @tny_msg e-mail message. If @tny_msg is NULL,
 * then a blank page will be displayed
 *  */
void         modest_msg_view_set_message  (ModestMsgView *self, TnyMsg *tny_msg);



/**
 * modest_msg_view_set_message
 * @self: a ModestMsgView instance
 *
 * get the @tny_msg e-mail message.
 *
 * Returns: the message or NULL
 */
TnyMsg*      modest_msg_view_get_message  (ModestMsgView *self);



G_END_DECLS

#endif /* __MODEST_MSG_VIEW_H__ */

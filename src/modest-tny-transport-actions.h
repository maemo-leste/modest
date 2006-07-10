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


/* modest-tny-transport-actions.h */

#ifndef __MODEST_TNY_TRANSPORT_ACTIONS_H__
#define __MODEST_TNY_TRANSPORT_ACTIONS_H__

#include <glib-object.h>
#include <tny-transport-account-iface.h>

G_BEGIN_DECLS

/* standard convenience macros */
#define MODEST_TYPE_TNY_TRANSPORT_ACTIONS             (modest_tny_transport_actions_get_type())
#define MODEST_TNY_TRANSPORT_ACTIONS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_TRANSPORT_ACTIONS,ModestTnyTransportActions))
#define MODEST_TNY_TRANSPORT_ACTIONS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_TRANSPORT_ACTIONS,GObject))
#define MODEST_IS_TNY_TRANSPORT_ACTIONS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_TRANSPORT_ACTIONS))
#define MODEST_IS_TNY_TRANSPORT_ACTIONS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_TRANSPORT_ACTIONS))
#define MODEST_TNY_TRANSPORT_ACTIONS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_TRANSPORT_ACTIONS,ModestTnyTransportActionsClass))

typedef struct _ModestTnyTransportActions      ModestTnyTransportActions;
typedef struct _ModestTnyTransportActionsClass ModestTnyTransportActionsClass;

struct _ModestTnyTransportActions {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestTnyTransportActionsClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestTnyTransportActions* obj); */
};

/* member functions */
GType        modest_tny_transport_actions_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget*, */
/*    otherwise probably a GObject*. */
GObject*    modest_tny_transport_actions_new         (void);


/**
 * modest_tny_transport_actions_send_message:
 * @self: a ModestTnyTransportActions object
 * @transport_account: the TnyTransportAccountIface to use for sending this message
 * @from: the email address of the sender
 * @to: the email address of the receiver
 * @cc: the receivers of a copy of the message (comma-seperated)
 * @bcc: the receivers of a blind copy of the message (comma-seperated)
 * @subject: the Subject: of the message
 * @body: a string containing the message body (text)
 *
 * send a email message to @to
 *
 * Returns: TRUE but this will change to whether sending was successful
 */
gboolean modest_tny_transport_actions_send_message (ModestTnyTransportActions *self,
						    TnyTransportAccountIface *transport_account,
						    const gchar *from,
						    const gchar *to,
						    const gchar *cc,
						    const gchar *bcc,
						    const gchar *subject,
						    const gchar *body,
						    const GList *attachments_list);

G_END_DECLS

#endif /* __MODEST_TNY_TRANSPORT_ACTIONS_H__ */


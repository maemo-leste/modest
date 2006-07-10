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


#ifndef _MODEST_UI_MESSAGE_EDITOR_H
#define _MODEST_UI_MESSAGE_EDITOR_H

typedef enum {
	QUOTED_SEND_REPLY,
	QUOTED_SEND_REPLY_ALL,
	QUOTED_SEND_FORWARD,
	QUOTED_SEND_FORWARD_ATTACHED
} quoted_send_type;

/**
 * quoted_send_msg:
 * @modest_ui: a ModestUI instance
 * @qstype: determines whether to REPLY, REPLY_ALL or FORWARD
 *
 * open a new editor window quoting the currently selected message
 * the quote type determines which parts are to be quoted
 */
void quoted_send_msg (ModestUI *modest_ui, quoted_send_type qstype);

/**
 * on_new_mail_clicked:
 * @widget: the button widget that received the signal
 * @user_data: pointer to user-data, here ModestUI
 *
 * callback used in main-window
 * called when user presses the "New Mail" button
 */
void on_new_mail_clicked (GtkWidget *widget, gpointer user_data);

void ui_on_mailto_clicked (GtkWidget *widget, const gchar * uri, gpointer user_data);
#endif /* _MODEST_UI_MESSAGE_EDITOR_H */

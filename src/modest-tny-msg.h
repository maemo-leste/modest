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

#ifndef __MODEST_TNY_MSG_H__
#define __MODEST_TNY_MSG_H__

/**
 * modest_tny_msg_new:
 * @mailto: recipient for the message
 * @mailfrom: sender of this message
 * @cc: Cc: address for the message
 * @bcc: Bcc: address for the message
 * @subject: subject for the messdage
 * @body: body for the message
 * @attachments: a list of attachments (local URIs)
 * 
 * create a new TnyMsg with the given parameters
 * 
 * Returns: a new TnyMsg (free with g_object_unref)
 */	 
TnyMsg* modest_tny_msg_new (const gchar* mailto, const gchar* mailfrom, const gchar *cc,
			    const gchar *bcc, const gchar* subject, const gchar *body,
			    GSList *attachments);

/**
 * modest_tny_msg_find_body_part:
 * @self: a message
 * @want_html: prefer HTML-part when there are multiple body parts?
 * 
 * search a message for the body part. if @want_html is true, try HTML mail
 * first.
 * 
 * Returns: the TnyMsgMimePart for the found part, or NULL if no matching part is found
 */	 
TnyMimePart *modest_tny_msg_find_body_part (TnyMsg * self, gboolean want_html);


/**
 * modest_tny_msg_find_body:
 * @self: 
 * @want_html: 
 * 
 * gets the body of a message as text, if @want_html is true, try HTML mail
 * first.
 * 
 * Returns: the body of the message as text, or NULL if it is not found
 * the text should be freed with 
 **/
gchar* modest_tny_msg_get_body (TnyMsg *self, gboolean want_html);

#endif /* __MODEST_TNY_MSG_H__ */

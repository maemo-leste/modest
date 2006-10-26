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

#ifndef __MODEST_TNY_MSG_ACTIONS_H__
#define __MODEST_TNY_MSG_ACTIONS_H__

/**
 * modest_tny_msg_actions_quote:
 * @self: the message to quote
 * @from: the original sender of the message
 * @sent_date: the date the original message was sent
 * @limit: characters per line limit for the quoted message
 * @to_quote: a string to quote instead of the message body
 * 
 * reply-quotes a message or @to_quote if it's not NULL.

 * Note: @from and @sent_date may be eliminated from the API in future versions
 * 
 * Returns: a newly allocated string containing the quoted message
 */
gchar *modest_tny_msg_actions_quote (TnyMsg * self, const gchar * from,
				     time_t sent_date, gint limit,
				     const gchar *to_quote);

/**
 * modest_tny_msg_actions_find_body_part:
 * @self: a message
 * @want_html: prefer HTML-part when there are multiple body parts?
 * 
 * search a message for the body part. if @want_html is true, try HTML mail
 * first.
 * 
 * Returns: the TnyMsgMimePart for the found part, or NULL if no matching part is found
 */	 
TnyMimePart *modest_tny_msg_actions_find_body_part (TnyMsg * self, gboolean want_html);


/**
 * modest_tny_msg_actions_get_nth_part:
 * @self: a message
 * @index: number (1-based) of the part to retrieve
 * 
 * search for the nth (mime) part in the message
 * 
 * Returns: the TnyMsgMimePart for the found part, or NULL if no matching part is foundi; must be unref'd
 */
TnyMimePart * modest_tny_msg_actions_find_nth_part (TnyMsg *msg, gint index);


#endif /* __MODEST_TNY_MSG_ACTIONS_H__ */

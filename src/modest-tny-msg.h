/* Copyright (c) 2006, 2007, 2008 Nokia Corporation
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
 * ModestTnyMsgForwardType:
 *
 * How the original message will be forwarded to the recipient
 */
typedef enum _ModestTnyMsgForwardType {
	MODEST_TNY_MSG_FORWARD_TYPE_INLINE = 1,
	MODEST_TNY_MSG_FORWARD_TYPE_ATTACHMENT
} ModestTnyMsgForwardType;

/**
 * ModestTnyMsgReplyType:
 *
 * How the original message will be forwarded to the recipient
 */
typedef enum _ModestTnyMsgReplyType {
	MODEST_TNY_MSG_REPLY_TYPE_CITE = 1,
	MODEST_TNY_MSG_REPLY_TYPE_QUOTE
} ModestTnyMsgReplyType;

/**
 * ModestTnyMsgReplyMode:
 *
 * Who will be the recipients of the replied message
 */
typedef enum _ModestTnyMsgReplyMode {
	MODEST_TNY_MSG_REPLY_MODE_SENDER,
	MODEST_TNY_MSG_REPLY_MODE_LIST,
	MODEST_TNY_MSG_REPLY_MODE_ALL
} ModestTnyMsgReplyMode;

/**
 * modest_tny_msg_new:
 * @mailto: recipient for the message
 * @mailfrom: sender of this message
 * @cc: Cc: address for the message
 * @bcc: Bcc: address for the message
 * @subject: subject for the messdage
 * @body: body for the message
 * @attachments: a list of attachments (local URIs)
 * @attached: a #gint pointer, returns the number of attachments really included.
 * @error: a pointer for errors in message creation
 * 
 * create a new TnyMsg with the given parameters
 * 
 * Returns: a new TnyMsg (free with g_object_unref)
 */	 
TnyMsg* modest_tny_msg_new (const gchar* mailto, const gchar* mailfrom, const gchar *cc,
			    const gchar *bcc, const gchar* subject, 
			    const gchar *references, const gchar *in_reply_to,
			    const gchar *body,
			    GList *attachments, gint *attached, GError **err);

/**
 * modest_tny_msg_new_html_plain:
 * @mailto: recipient for the message
 * @mailfrom: sender of this message
 * @cc: Cc: address for the message
 * @bcc: Bcc: address for the message
 * @subject: subject for the message
 * @html_body: body for the message in HTML
 * @plain_body: body for the message in plain text
 * @attachments: a list of attachments (mime parts)
 * @images: a list of images (mime parts)
 * @attached: a #gint pointer, returns the number of attachments really included.
 * @error: a pointer for errors in message creation
 * 
 * create a new TnyMsg with the given parameters
 * 
 * Returns: a new TnyMsg (free with g_object_unref)
 */	 
TnyMsg* modest_tny_msg_new_html_plain (const gchar* mailto, const gchar* mailfrom, const gchar *cc,
				       const gchar *bcc, const gchar* subject,
				       const gchar *references, const gchar *in_reply_to,
				       const gchar *html_body, const gchar *plain_body,
				       GList *attachments, GList *images, gint *attached, GError **err);

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
TnyMimePart*  modest_tny_msg_find_body_part  (TnyMsg * self, gboolean want_html);

/**
 * modest_tny_msg_find_calendar_part:
 * @self: a message
 * 
 * search a message for the calendar part.
 * 
 * Returns: the TnyMimePart for the found part, or NULL if no matching part is found
 */	 
TnyMimePart* modest_tny_msg_find_calendar (TnyMsg *self);


/**
 * modest_tny_msg_find_body:
 * @self: some #TnyMsg
 * @want_html: 
 * @is_html: if the original body was html or plain text
 * 
 * gets the body of a message as text, if @want_html is true, try HTML mail
 * first.
 * 
 * Returns: the body of the message as text, or NULL if it is not found
 * the text should be freed with 
 **/
gchar*        modest_tny_msg_get_body        (TnyMsg *self, gboolean want_html, gboolean *is_html);




/**
 * modest_tny_msg_create_forward_msg:
 * @msg: a valid #TnyMsg instance
 * @from: the sender of the forwarded mail
 * @signature: signature to attach to the reply
 * @forward_type: the type of formatting used to create the forwarded message
 * 
 * Creates a forwarded message from an existing one
 * 
 * Returns: a new #TnyMsg, or NULL in case of error
 **/
TnyMsg*       modest_tny_msg_create_forward_msg   (TnyMsg *msg, 
						   const gchar *from,
						   const gchar *signature,
						   ModestTnyMsgForwardType forward_type);

/**
 * modest_tny_msg_create_reply_calendar_msg:
 * @msg: a valid #TnyMsg instance, or %NULL
 * @header: a valid #TnyHeader instance, or %NULL
 * @from: the sender of the forwarded mail
 * @signature: signature to add to the reply message
 * @headers: #TnyList of #TnyPair with the headers to add
 * 
 * Creates a new message to reply to a calendar event
 * 
 * Returns: Returns: a new #TnyMsg, or NULL in case of error
 **/
TnyMsg*       modest_tny_msg_create_reply_calendar_msg     (TnyMsg *msg,
							    TnyHeader *header,
							    const gchar *from,
							    const gchar *signature,
							    TnyList *headers);

/**
 * modest_tny_msg_create_reply_msg:
 * @msg: a valid #TnyMsg instance, or %NULL
 * @header: a valid #TnyHeader instance, or %NULL
 * @from: the sender of the forwarded mail
 * @signature: signature to add to the reply message
 * @reply_type: the type of formatting used to create the reply message
 * @reply_mode: the mode of reply: to the sender only, to a mail list or to all
 * 
 * Creates a new message to reply to an existing one
 * 
 * Returns: Returns: a new #TnyMsg, or NULL in case of error
 **/
TnyMsg*       modest_tny_msg_create_reply_msg     (TnyMsg *msg,
						   TnyHeader *header,
						   const gchar *from,
						   const gchar *signature,
						   ModestTnyMsgReplyType reply_type,
						   ModestTnyMsgReplyMode reply_mode);


/**
 * modest_tny_msg_get_parent_unique_id
 * @msg: a valid #TnyMsg instance, or %NULL
 * 
 * gets the unique ID of the 'parent' (the original msg replied to or forward)
 * 
 * Returns: Returns: a the parent uid, or NULL if there is none.
 **/
const gchar*  modest_tny_msg_get_parent_uid (TnyMsg *msg);


/**
 * modest_tny_msg_estimate_size:
 * @plain_body: a string
 * @html_body: a string
 * @parts_number: a gint (number of additional parts)
 * @parts_size: a guint64 (sum of size of the additional parts)
 *
 * Estimates the size of the resulting message obtained from the size of the body
 * parts, and adding the estimation of size headers.
 */
guint64
modest_tny_msg_estimate_size (const gchar *plain_body, const gchar *html_body,
			      guint64 parts_count,
			      guint64 parts_size);

/**
 * modest_tny_msg_get_all_recipients_list:
 * @header: a #TnyHeader
 *
 * Obtains a list of all the addresses available in @header.
 *
 * Returns: a newly allocated #GSList of strings. Caller should free strings and list.
 */
GSList *
modest_tny_msg_header_get_all_recipients_list (TnyHeader *header);

/**
 * modest_tny_msg_get_all_recipients_list:
 * @msg: a #TnyMsg
 *
 * Obtains a list of all the addresses available in a message header.
 *
 * Returns: a newly allocated #GSList of strings. Caller should free strings and list.
 */
GSList *
modest_tny_msg_get_all_recipients_list (TnyMsg *msg);

/**
 * modest_tny_msg_get_references:
 * @msg: a #TnyMsg
 * @message_id: a pointer to a string
 * @references: a pointer to a string
 * @in_reply_to: a pointer to a string
 *
 * obtains the Message-ID, References and In-Reply-To fields of a
 * message
 */
void modest_tny_msg_get_references (TnyMsg *msg, gchar **message_id, gchar **references, gchar **in_reply_to);

/**
 * modest_tny_msg_get_attachments_parent:
 * @msg: a #TnyMsg
 *
 * the mime part of the message attachments should be below
 *
 * Returns: the mime part (ref owned by caller)
 */
TnyMimePart *modest_tny_msg_get_attachments_parent (TnyMsg *msg);


#endif /* __MODEST_TNY_MSG_H__ */

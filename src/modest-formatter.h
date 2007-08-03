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

#ifndef __MODEST_FORMATTER_H__
#define __MODEST_FORMATTER_H__

#include <tny-msg.h>
#include <tny-header.h>
#include <tny-mime-part.h>

G_BEGIN_DECLS

#define MODEST_TYPE_FORMATTER             (modest_formatter_get_type ())
#define MODEST_FORMATTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_FORMATTER, ModestFormatter))
#define MODEST_FORMATTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MODEST_TYPE_FORMATTER, ModestFormatterClass))
#define MODEST_IS_FORMATTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_FORMATTER))
#define MODEST_IS_FORMATTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MODEST_TYPE_FORMATTER))
#define MODEST_FORMATTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MODEST_TYPE_FORMATTER, ModestFormatterClass))

typedef struct _ModestFormatter ModestFormatter;
typedef struct _ModestFormatterClass ModestFormatterClass;

struct _ModestFormatter
{
	GObject parent;
};

struct _ModestFormatterClass 
{
	GObjectClass parent;
};

GType modest_formatter_get_type (void);

ModestFormatter* modest_formatter_new (const gchar *content_type, const gchar *signature);

/**
 * modest_formatter_cite:
 * @self: a #ModestFormatter
 * @part: a non-NULL #TnyMimePart with the body of the original message
 * @header: a non-NULL #TnyHeader of the original message
 * 
 * Creates a new message with a text body made from the body of the
 * original message cited. This function is locale-sensitive.
 *
 * Example of cited message:
 * <programlisting><para>
 * Original message
 * ----------------
 * Date: 1970/01/01
 * From: somemailATmodest.org
 * Body: "This is the body of the text"
 * </para><para>
 * Cited message
 * -------------
 * Body:
 * On 1970/01/01 somemailATmodest.org wrote:
 * This is the body of the text
 * </para></programlisting>
 *
 * Returns: a newly formatted #TnyMsg or NULL in case of error
 **/
TnyMsg * modest_formatter_cite   (ModestFormatter *self, TnyMimePart *part, TnyHeader *header);


/**
 * modest_formatter_quote:
 * @self: a #ModestFormatter
 * @part: a non-NULL #TnyMimePart with the body of the original message
 * @header: a non-NULL #TnyHeader of the original message
 * @attachments: a #GList of attachments in original message
 * 
 * Creates a new message with a text body made from the body of the
 * original message quoted. This function is locale-sensitive.
 *
 * Example of cited message:
 * <programlisting><para>
 * Original message
 * ----------------
 * Date: 1970/01/01
 * From: somemailATmodest.org
 * Body: "This is the body of the text"
 * </para><para>
 * Quoted message
 * -------------
 * Body:
 * ------ Original message -----
 * > This is the body of the text
 * > Attachment: file1.txt
 * </para></programlisting>
 *
 * Returns: a newly formatted #TnyMsg or NULL in case of error
 **/
TnyMsg * modest_formatter_quote  (ModestFormatter *self, TnyMimePart *part, TnyHeader *header, GList *attachments);


/**
 * modest_formatter_inline:
 * @self: a #ModestFormatter
 * @part: a non-NULL #TnyMimePart with the body of the original message
 * @header: a non-NULL #TnyHeader of the original message
 * @attachments: a #GList of attachments
 * 
 * Creates a new message with a text body made from the body of the
 * original message inlined ready to be forwarded. This function is
 * locale-sensitive.
 *
 * Example of cited message:
 * <programlisting><para>
 * Original message
 * ----------------
 * Date: 1970/01/01
 * From: somemailATmodest.org
 * To: mymailATmodest.org
 * Subject: Mail subject
 * Body: "This is the body of the text"
 * </para><para>
 * Inlined message
 * -------------
 * Body:
 * -----Forwarded Message-----
 * From: somemailATmodest.org
 * Sent: 1970/01/01
 * To: mymailATmodest.org
 * Subject: Fw: Mail subject
 * On 1970/01/01 somemailATmodest.org wrote:
 * This is the body of the text
 * </para></programlisting>
 *
 * Returns: a newly formatted #TnyMsg or NULL in case of error
 **/
TnyMsg * modest_formatter_inline (ModestFormatter *self, TnyMimePart *part, TnyHeader *header, GList *attachments);

/**
 * modest_formatter_attach:
 * @self: a #ModestFormatter
 * @part: a non-NULL #TnyMimePart with the body of the original message
 * @header: a non-NULL #TnyHeader of the original message
 * 
 * Creates a new message with the original message as attachment
 *
 * Returns: a newly formatted #TnyMsg or NULL in case of error
 **/
TnyMsg * modest_formatter_attach (ModestFormatter *self, TnyMsg *msg, TnyHeader *header);

/**
 * modest_formatter_create_message:
 * @self: a #ModestFormatter
 * @single_body: a #gboolean
 * @has_attachments: a #gboolean
 *
 * Creates an empty #TnyMsg with the expected parts for Modest
 * formatters and body embedders.
 *
 * If @has_attachments is %TRUE, the mail will be formatted as
 * "multipart/mixed", and attachments will be stored as parts
 * of it. The body will be just another multipart. If %FALSE,
 * the bodies will be stored directly as the message.
 *
 * If @single_body is %TRUE, the body will be stored as a
 * "multipart/alternative", and the parts will be the different
 * alternatives. If %FALSE, this body will be stored directly.
 */
TnyMsg * modest_formatter_create_message (ModestFormatter *self, gboolean single_body, gboolean has_attachments);

/**
 * modest_formatter_create_body_part:
 * @self: a #ModestFormatter
 *
 * Obtains a new formatted body part for @msg, or
 * @msg itself if the body is intended to be stored directly
 * in it.
 */ 
TnyMimePart * modest_formatter_create_body_part (ModestFormatter *self, TnyMsg *msg);

G_END_DECLS

#endif

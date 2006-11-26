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

#include "modest-mail-operation.h"
/* include other impl specific header files */
#include <string.h>
#include <stdarg.h>
#include <tny-mime-part.h>
#include <tny-store-account.h>
#include <tny-folder-store.h>
#include <tny-folder-store-query.h>
#include <tny-camel-msg.h>
#include <tny-camel-header.h>
#include <tny-camel-stream.h>
#include <tny-camel-mime-part.h>
#include <tny-simple-list.h>

#include <glib/gi18n.h>

#include <modest-text-utils.h>
#include <modest-tny-msg-actions.h>
#include "modest-tny-platform-factory.h"

/* 'private'/'protected' functions */
static void modest_mail_operation_class_init (ModestMailOperationClass *klass);
static void modest_mail_operation_init       (ModestMailOperation *obj);
static void modest_mail_operation_finalize   (GObject *obj);

#define MODEST_ERROR modest_error_quark ()

typedef enum _ModestMailOperationErrorCode ModestMailOperationErrorCode;
enum _ModestMailOperationErrorCode {
        MODEST_MAIL_OPERATION_ERROR_BAD_ACCOUNT,
        MODEST_MAIL_OPERATION_ERROR_MISSING_PARAMETER,

	MODEST_MAIL_OPERATION_NUM_ERROR_CODES
};

static void       set_error          (ModestMailOperation *mail_operation, 
				      ModestMailOperationErrorCode error_code,
				      const gchar *fmt, ...);
static void       status_update_cb   (TnyFolder *folder, 
				      const gchar *what, 
				      gint status, 
				      gpointer user_data);
static void       folder_refresh_cb  (TnyFolder *folder, 
				      gboolean cancelled, 
				      gpointer user_data);
static void       add_attachments    (TnyMsg *msg, 
				      const GList *attachments_list);


static TnyMimePart *         add_body_part    (TnyMsg *msg, 
					       const gchar *body,
					       const gchar *content_type, 
					       gboolean has_attachments);


/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMailOperationPrivate ModestMailOperationPrivate;
struct _ModestMailOperationPrivate {
	TnyAccount                *account;
	ModestMailOperationStatus  status;
	GError                    *error;
};
#define MODEST_MAIL_OPERATION_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_MAIL_OPERATION, \
                                                   ModestMailOperationPrivate))

/* some utility functions */
static char * get_content_type(const gchar *s);
static gboolean is_ascii(const gchar *s);

/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_mail_operation_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMailOperationClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_mail_operation_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMailOperation),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_mail_operation_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestMailOperation",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_mail_operation_class_init (ModestMailOperationClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_mail_operation_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMailOperationPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_mail_operation_init (ModestMailOperation *obj)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);

	priv->account = NULL;
	priv->status = MODEST_MAIL_OPERATION_STATUS_INVALID;
	priv->error = NULL;
}

static void
modest_mail_operation_finalize (GObject *obj)
{
	ModestMailOperationPrivate *priv;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(obj);

	if (priv->account) {
		g_object_unref (priv->account);
		priv->account = NULL;
	}
	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestMailOperation*
modest_mail_operation_new (TnyAccount *account)
{
	ModestMailOperation *mail_operation;
	ModestMailOperationPrivate *priv;

	mail_operation = 
		MODEST_MAIL_OPERATION(g_object_new(MODEST_TYPE_MAIL_OPERATION, NULL));
	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(mail_operation);

	priv->account = g_object_ref (account);

	return mail_operation;
}


void
modest_mail_operation_send_mail (ModestMailOperation *mail_operation,
				 TnyMsg* msg)
{
	ModestMailOperationPrivate *priv;
	TnyTransportAccount *transport_account;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(mail_operation);

	if (!TNY_IS_TRANSPORT_ACCOUNT (priv->account)) {
		set_error (mail_operation,
			   MODEST_MAIL_OPERATION_ERROR_BAD_ACCOUNT,
			   _("Error trying to send a mail. Use a transport account"));
	}

	transport_account = TNY_TRANSPORT_ACCOUNT (priv->account);

	mail_operation = modest_mail_operation_new (NULL);
	tny_transport_account_send (transport_account, msg);
}

void
modest_mail_operation_send_new_mail (ModestMailOperation *mail_operation,
				     const gchar *from,
				     const gchar *to,
				     const gchar *cc,
				     const gchar *bcc,
				     const gchar *subject,
				     const gchar *body,
				     const GList *attachments_list)
{
	TnyMsg *new_msg;
	TnyHeader *header;
	TnyTransportAccount *transport_account;
	ModestMailOperationPrivate *priv;
	gchar *content_type;

	g_return_if_fail (mail_operation);

	/* Check parametters */
	if (to == NULL) {
		set_error (mail_operation,
			   MODEST_MAIL_OPERATION_ERROR_MISSING_PARAMETER,
			   _("Error trying to send a mail. You need to set almost one a recipient"));
		return;
	}

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(mail_operation);

	if (!TNY_IS_TRANSPORT_ACCOUNT (priv->account)) {
		set_error (mail_operation,
			   MODEST_MAIL_OPERATION_ERROR_BAD_ACCOUNT,
			   _("Error trying to send a mail. Use a transport account"));
		return;
	}

	/* Create new msg */
	transport_account = TNY_TRANSPORT_ACCOUNT (priv->account);

	new_msg          = TNY_MSG (tny_camel_msg_new ());
	header           = TNY_HEADER (tny_camel_header_new ());

	/* WARNING: set the header before assign values to it */
	tny_msg_set_header (new_msg, header);
	tny_header_set_from (TNY_HEADER (header), from);
	tny_header_set_to (TNY_HEADER (header), to);
	tny_header_set_cc (TNY_HEADER (header), cc);
	tny_header_set_bcc (TNY_HEADER (header), bcc);
	tny_header_set_subject (TNY_HEADER (header), subject);

	content_type = get_content_type(body);

	/* Add the body of the new mail */	
	add_body_part (new_msg, body, (const gchar *) content_type,
		       (attachments_list == NULL) ? FALSE : TRUE);

	/* Add attachments */
	add_attachments (new_msg, attachments_list);

	/* Send mail */	
	tny_transport_account_send (transport_account, new_msg);

	/* Clean */
	g_object_unref (header);
	g_object_unref (new_msg);
	g_free(content_type);
}

/**
 * modest_mail_operation_create_forward_mail:
 * @msg: a valid #TnyMsg instance
 * @forward_type: the type of forwarded message
 * 
 * creates a forwarded message from an existing one
 * 
 * Returns: a new #TnyMsg, or NULL in case of error
 **/
TnyMsg* 
modest_mail_operation_create_forward_mail (TnyMsg *msg, 
					   const gchar *from,
					   ModestMailOperationForwardType forward_type)
{
	TnyMsg *new_msg;
	TnyHeader *new_header, *header;
	gchar *new_subject, *new_body, *content_type;
	TnyMimePart *text_body_part = NULL;
	GList *attachments_list;
	TnyList *parts;
	TnyIterator *iter;

	/* Create new objects */
	new_msg          = TNY_MSG (tny_camel_msg_new ());
	new_header       = TNY_HEADER (tny_camel_header_new ());

	header = tny_msg_get_header (msg);

	/* Fill the header */
	tny_msg_set_header (new_msg, new_header);
	tny_header_set_from (new_header, from);

	/* Change the subject */
	new_subject = (gchar *) modest_text_utils_derived_subject (tny_header_get_subject(header),
								   _("Fwd:"));
	tny_header_set_subject (new_header, (const gchar *) new_subject);
	g_free (new_subject);

	/* Get body from original msg */
	new_body = (gchar *) modest_tny_msg_actions_find_body (msg, FALSE);
	if (!new_body) {
		g_object_unref (new_msg);
		return NULL;
	}
	content_type = get_content_type(new_body);

	/* Create the list of attachments */
	parts = TNY_LIST (tny_simple_list_new());
	tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);
	iter = tny_list_create_iterator (parts);
	attachments_list = NULL;

	while (!tny_iterator_is_done(iter)) {
		TnyMimePart *part;

		part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_attachment (part))
			attachments_list = g_list_prepend (attachments_list, part);

		tny_iterator_next (iter);
	}

	/* Add attachments */
	add_attachments (new_msg, attachments_list);

	switch (forward_type) {
		TnyMimePart *attachment_part;
		gchar *inlined_text;

	case MODEST_MAIL_OPERATION_FORWARD_TYPE_INLINE:
		/* Prepend "Original message" text */
		inlined_text = (gchar *) 
			modest_text_utils_inlined_text (tny_header_get_from (header),
							tny_header_get_date_sent (header),
							tny_header_get_to (header),
							tny_header_get_subject (header),
							(const gchar*) new_body);
		g_free (new_body);
		new_body = inlined_text;

		/* Add body part */
		add_body_part (new_msg, new_body, 
			       (const gchar *) content_type, 
			       (tny_list_get_length (parts) > 0) ? TRUE : FALSE);

		break;
	case MODEST_MAIL_OPERATION_FORWARD_TYPE_ATTACHMENT:
		attachment_part = add_body_part (new_msg, new_body, 
						 (const gchar *) content_type, TRUE);

		/* Set the subject as the name of the attachment */
		tny_mime_part_set_filename (attachment_part, tny_header_get_subject (header));
		
		break;
	}

	/* Clean */
	if (attachments_list) g_list_free (attachments_list);
	g_object_unref (parts);
	if (text_body_part) g_free (text_body_part);
	g_free (content_type);
	g_free (new_body);

	return new_msg;
}

/**
 * modest_mail_operation_create_reply_mail:
 * @msg: a valid #TnyMsg instance
 * @reply_type: the format of the new message
 * @reply_mode: the mode of reply, to the sender only, to a mail list or to all
 * 
 * creates a new message to reply to an existing one
 * 
 * Returns: Returns: a new #TnyMsg, or NULL in case of error
 **/
TnyMsg* 
modest_mail_operation_create_reply_mail (TnyMsg *msg, 
					 const gchar *from,
					 ModestMailOperationReplyType reply_type,
					 ModestMailOperationReplyMode reply_mode)
{
	TnyMsg *new_msg;
	TnyHeader *new_header, *header;
	gchar *new_subject, *new_body, *content_type, *quoted;
	TnyMimePart *text_body_part;

	/* Create new objects */
	new_msg          = TNY_MSG (tny_camel_msg_new ());
	new_header       = TNY_HEADER (tny_camel_header_new ());
	header           = tny_msg_get_header (msg);

	/* Fill the header */
	tny_msg_set_header (new_msg, new_header);
	tny_header_set_to (new_header, tny_header_get_from (header));
	tny_header_set_from (new_header, from);

	switch (reply_mode) {
		gchar *new_cc = NULL;
		const gchar *cc = NULL, *bcc = NULL;
		GString *tmp = NULL;

	case MODEST_MAIL_OPERATION_REPLY_MODE_SENDER:
		/* Do not fill neither cc nor bcc */
		break;
	case MODEST_MAIL_OPERATION_REPLY_MODE_LIST:
		/* TODO */
		break;
	case MODEST_MAIL_OPERATION_REPLY_MODE_ALL:
		/* Concatenate to, cc and bcc */
		cc = tny_header_get_cc (header);
		bcc = tny_header_get_bcc (header);

		tmp = g_string_new (tny_header_get_to (header));
		if (cc)  g_string_append_printf (tmp, ",%s",cc);
		if (bcc) g_string_append_printf (tmp, ",%s",bcc);

		/* Remove my own address from the cc list */
		new_cc = (gchar *) 
			modest_text_utils_remove_address ((const gchar *) tmp->str, 
							  (const gchar *) from);
		/* FIXME: remove also the mails from the new To: */
		tny_header_set_cc (new_header, new_cc);

		/* Clean */
		g_string_free (tmp, TRUE);
		g_free (new_cc);
		break;
	}

	/* Change the subject */
	new_subject = (gchar*) modest_text_utils_derived_subject (tny_header_get_subject(header),
								  _("Re:"));
	tny_header_set_subject (new_header, (const gchar *) new_subject);
	g_free (new_subject);

	/* Get body from original msg */
	new_body = (gchar*) modest_tny_msg_actions_find_body (msg, FALSE);
	if (!new_body) {
		g_object_unref (new_msg);
		return NULL;
	}
	content_type = get_content_type(new_body);

	switch (reply_type) {
		gchar *cited_text;

	case MODEST_MAIL_OPERATION_REPLY_TYPE_CITE:
		/* Prepend "Original message" text */
		cited_text = (gchar *) modest_text_utils_cited_text (tny_header_get_from (header),
								     tny_header_get_date_sent (header),
								     (const gchar*) new_body);
		g_free (new_body);
		new_body = cited_text;
		break;
	case MODEST_MAIL_OPERATION_REPLY_TYPE_QUOTE:
		/* FIXME: replace 80 with a value from ModestConf */
		quoted = (gchar*) modest_text_utils_quote (new_body, 
							   tny_header_get_from (header),
							   tny_header_get_date_sent (header),
							   80);
		g_free (new_body);
		new_body = quoted;
		break;
	}
	/* Add body part */
	text_body_part = add_body_part (new_msg, new_body, 
					(const gchar *) content_type, TRUE);

	/* Clean */
/* 	g_free (text_body_part); */
	g_free (content_type);
	g_free (new_body);

	return new_msg;
}

void
modest_mail_operation_update_account (ModestMailOperation *mail_operation)
{
	TnyStoreAccount *storage_account;
	ModestMailOperationPrivate *priv;
	TnyList *folders;
	TnyIterator *ifolders;
	TnyFolder *cur_folder;
	TnyFolderStoreQuery *query;

	g_return_if_fail (mail_operation);
	g_return_if_fail (MODEST_IS_MAIL_OPERATION (mail_operation));

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(mail_operation);

	/* Check that it is a store account */
	if (!TNY_IS_STORE_ACCOUNT (priv->account)) {
		set_error (mail_operation,
			   MODEST_MAIL_OPERATION_ERROR_BAD_ACCOUNT,
			   _("Error trying to update an account. Use a store account"));
		return;
	}
	storage_account = TNY_STORE_ACCOUNT (priv->account);

	/* Get subscribed folders */
    	folders = TNY_LIST (tny_simple_list_new ());
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	tny_folder_store_get_folders (TNY_FOLDER_STORE (storage_account),
				      folders, query);
	g_object_unref (query);
	
	ifolders = tny_list_create_iterator (folders);

	/* Async refresh folders */	
	for (tny_iterator_first (ifolders); 
	     !tny_iterator_is_done (ifolders); 
	     tny_iterator_next (ifolders)) {
		
		cur_folder = TNY_FOLDER (tny_iterator_get_current (ifolders));
		tny_folder_refresh_async (cur_folder, folder_refresh_cb,
					  status_update_cb, mail_operation);
	}
	
	g_object_unref (ifolders);
}

ModestMailOperationStatus
modest_mail_operation_get_status (ModestMailOperation *mail_operation)
{
	ModestMailOperationPrivate *priv;

/* 	g_return_val_if_fail (mail_operation, MODEST_MAIL_OPERATION_STATUS_INVALID); */
/* 	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (mail_operation),  */
/* 			      MODEST_MAIL_OPERATION_STATUS_INVALID); */

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (mail_operation);
	return priv->status;
}

const GError *
modest_mail_operation_get_error (ModestMailOperation *mail_operation)
{
	ModestMailOperationPrivate *priv;

/* 	g_return_val_if_fail (mail_operation, NULL); */
/* 	g_return_val_if_fail (MODEST_IS_MAIL_OPERATION (mail_operation), NULL); */

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE (mail_operation);
	return priv->error;
}

void 
modest_mail_operation_cancel (ModestMailOperation *mail_operation)
{
	/* TODO */
}

static gboolean
is_ascii(const gchar *s)
{
	while (s[0]) {
		if (s[0] & 128 || s[0] < 32)
			return FALSE;
		s++;
	}
	return TRUE;
}

static char *
get_content_type(const gchar *s)
{
	GString *type;
	
	type = g_string_new("text/plain");
	if (!is_ascii(s)) {
		if (g_utf8_validate(s, -1, NULL)) {
			g_string_append(type, "; charset=\"utf-8\"");
		} else {
			/* it should be impossible to reach this, but better safe than sorry */
			g_warning("invalid utf8 in message");
			g_string_append(type, "; charset=\"latin1\"");
		}
	}
	return g_string_free(type, FALSE);
}

static GQuark 
modest_error_quark (void)
{
	static GQuark err_q = 0;
	
	if (err_q == 0)
		err_q = g_quark_from_static_string ("modest-error-quark");
	
	return err_q;
}


static void 
set_error (ModestMailOperation *mail_operation, 
	   ModestMailOperationErrorCode error_code,
	   const gchar *fmt, ...)
{
	ModestMailOperationPrivate *priv;
	GError* error;
	va_list args;
	gchar* orig;

	priv = MODEST_MAIL_OPERATION_GET_PRIVATE(mail_operation);

	va_start (args, fmt);

	orig = g_strdup_vprintf(fmt, args);
	error = g_error_new (MODEST_ERROR, error_code, orig);

	va_end (args);

	if (priv->error)
		g_object_unref (priv->error);

	priv->error = error;
	priv->status = MODEST_MAIL_OPERATION_STATUS_FAILED;
}

static void
status_update_cb (TnyFolder *folder, const gchar *what, gint status, gpointer user_data) 
{
	/* TODO: update main window progress bar */
}

static void
folder_refresh_cb (TnyFolder *folder, gboolean cancelled, gpointer user_data) 
{
	if (cancelled) {
		ModestMailOperation *mail_operation;
		ModestMailOperationPrivate *priv;

		mail_operation = MODEST_MAIL_OPERATION (user_data);
		priv = MODEST_MAIL_OPERATION_GET_PRIVATE(mail_operation);

		priv->status = 	MODEST_MAIL_OPERATION_STATUS_CANCELLED;
	}
}

static void
add_attachments (TnyMsg *msg, const GList *attachments_list)
{
	GList *pos;
	TnyMimePart *attachment_part, *old_attachment;
	const gchar *attachment_content_type;
	const gchar *attachment_filename;
	TnyStream *attachment_stream;

	for (pos = (GList *)attachments_list; pos; pos = pos->next) {

		old_attachment = pos->data;
		attachment_filename = tny_mime_part_get_filename (old_attachment);
		attachment_stream = tny_mime_part_get_stream (old_attachment);
		attachment_part = TNY_MIME_PART (tny_camel_mime_part_new (camel_mime_part_new()));
		
		attachment_content_type = tny_mime_part_get_content_type (old_attachment);
				 
		tny_mime_part_construct_from_stream (attachment_part,
						     attachment_stream,
						     attachment_content_type);
		tny_stream_reset (attachment_stream);
		
		tny_mime_part_set_filename (attachment_part, attachment_filename);
		
		tny_mime_part_add_part (TNY_MIME_PART (msg), attachment_part);
/* 		g_object_unref (attachment_part); */
	}
}


static TnyMimePart *
add_body_part (TnyMsg *msg, 
	       const gchar *body,
	       const gchar *content_type,
	       gboolean has_attachments)
{
	TnyMimePart *text_body_part = NULL;
	TnyStream *text_body_stream;

	/* Create the stream */
	text_body_stream = TNY_STREAM (tny_camel_stream_new
				       (camel_stream_mem_new_with_buffer
					(body, strlen(body))));

	/* Create body part if needed */
	if (has_attachments)
		text_body_part = 
			TNY_MIME_PART (tny_camel_mime_part_new (camel_mime_part_new()));
	else
		text_body_part = TNY_MIME_PART(msg);

	/* Construct MIME part */
	tny_stream_reset (text_body_stream);
	tny_mime_part_construct_from_stream (text_body_part,
					     text_body_stream,
					     content_type);
	tny_stream_reset (text_body_stream);

	/* Add part if needed */
	if (has_attachments) {
		tny_mime_part_add_part (TNY_MIME_PART (msg), text_body_part);
		g_object_unref (G_OBJECT(text_body_part));
	}

	/* Clean */
	g_object_unref (text_body_stream);

	return text_body_part;
}

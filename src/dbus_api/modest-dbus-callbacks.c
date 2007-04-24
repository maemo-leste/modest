/* Copyright (c) 2007, Nokia Corporation
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
 
#include "modest-dbus-callbacks.h"
#include "modest-runtime.h"
#include "modest-account-mgr.h"
#include "modest-account-mgr-helpers.h"
#include "modest-tny-account.h"
#include "widgets/modest-msg-edit-window.h"
#include "modest-tny-msg.h"
#include <stdio.h>

typedef struct 
{
	gchar *to;
 	gchar *cc;
 	gchar *bcc;
 	gchar *subject;
 	gchar *body;
} SendMailIdleData;

static gboolean
on_idle_send_mail(gpointer user_data)
{
	SendMailIdleData *idle_data = (SendMailIdleData*)user_data;
	
	/* Get the TnyTransportAccount so we can instantiate a mail operation: */
 	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr();
	gchar *account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
	}
	
	TnyTransportAccount *transport_account = NULL;
	if (account_mgr) {
		transport_account = TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_tny_account_by_account
				      (modest_runtime_get_account_store(),
				       account_name,
				       TNY_ACCOUNT_TYPE_TRANSPORT));
	}
	
	if (!transport_account) {
		g_printerr ("modest: no transport account found for '%s'\n", account_name);
	}
	
 	/* Create the mail operation: */
 	if (transport_account) {	
 		/* Use the mail operation: */
		gchar * from = modest_account_mgr_get_from_string (account_mgr,
								  account_name);
		if (!from) {
			g_printerr ("modest: no from address for account '%s'\n", account_name);
		} else {
			ModestMailOperation *mail_operation = modest_mail_operation_new ();
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);
			
	 		modest_mail_operation_send_new_mail (mail_operation,
					     transport_account,
					     from, /* from */
					     idle_data->to, idle_data->cc, idle_data->bcc, idle_data->subject, 
					     idle_data->body, /* plain_body */
					     NULL, /* html_body */
					     NULL, /* attachments_list, GSList of TnyMimePart. */
					     (TnyHeaderFlags)0);
					     
			g_free (from);
			g_object_unref (G_OBJECT (mail_operation));
		}
				     
		g_object_unref (G_OBJECT (transport_account));
 	}
 	
 	g_free (account_name);
	
	/* Free the idle data: */
	g_free (idle_data->to);
	g_free (idle_data->cc);
	g_free (idle_data->bcc);
	g_free (idle_data->subject);
	g_free (idle_data->body);
	g_free (idle_data);
	
	return FALSE; /* Do not call this callback again. */
}

static gint on_send_mail(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	if (arguments->len != MODEST_DEBUS_SEND_MAIL_ARGS_COUNT)
     	return OSSO_ERROR;
     	
    /* Use g_idle to context-switch into the application's thread: */
 	SendMailIdleData *idle_data = g_new0(SendMailIdleData, 1); /* Freed in the idle callback. */
 	
    /* Get the arguments: */
 	osso_rpc_t val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_TO);
 	idle_data->to = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_CC);
 	idle_data->cc = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_BCC);
 	idle_data->bcc = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_SUBJECT);
 	idle_data->subject = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_BODY);
 	idle_data->body = g_strdup (val.value.s);
 	
 	/* printf("  debug: to=%s\n", idle_data->to); */
 	g_idle_add(on_idle_send_mail, (gpointer)idle_data);
 	
 	/* Note that we cannot report failures during sending, 
 	 * because that would be asynchronous. */
 	return OSSO_OK;
}


static gboolean
on_idle_mail_to(gpointer user_data)
{
	/* This is based on the implemenation of main.c:start_uil(): */
	
	gchar *uri = (gchar*)user_data;
	
	/* Get the TnyTransportAccount so we can instantiate a mail operation: */
 	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr();
	gchar *account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
	}
	
	TnyAccount *account = NULL;
	if (account_mgr) {
		account = modest_tny_account_store_get_tny_account_by_account (
			modest_runtime_get_account_store(), account_name,
			TNY_ACCOUNT_TYPE_TRANSPORT);
	}
	
	if (!account) {
		g_printerr ("modest: failed to get tny account folder'\n", account_name);
	} else {
		gchar * from = modest_account_mgr_get_from_string (account_mgr,
								  account_name);
		if (!from) {
			g_printerr ("modest: no from address for account '%s'\n", account_name);
		} else {
			TnyMsg *msg  = modest_tny_msg_new (uri /* mailto */, from, 
				NULL /* cc */, NULL /* bcc */,
				NULL /* subject */, NULL /* body */, NULL /* attachments */);
			if (!msg) {
				g_printerr ("modest: failed to create message\n");
			} else
			{
				TnyFolder *folder = modest_tny_account_get_special_folder (account,
									TNY_FOLDER_TYPE_DRAFTS);
				if (!folder) {
					g_printerr ("modest: failed to find Drafts folder\n");
				} else {
			
					tny_folder_add_msg (folder, msg, NULL); /* TODO: check err */
		
					ModestWindow *win = modest_msg_edit_window_new (msg, account_name);
					gtk_widget_show_all (GTK_WIDGET (win));
				
					g_object_unref (G_OBJECT(folder));
				}
			
				g_object_unref (G_OBJECT(msg));
			}
			g_object_unref (G_OBJECT(account));
		
		}
 	}
 	
 	g_free (account_name);
		
	g_free(uri);
	return FALSE; /* Do not call this callback again. */
}

static gint on_mail_to(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	if (arguments->len != MODEST_DEBUS_MAIL_TO_ARGS_COUNT)
     	return OSSO_ERROR;
     	
    /* Use g_idle to context-switch into the application's thread: */
 
    /* Get the arguments: */
 	osso_rpc_t val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_MAIL_TO_ARG_URI);
 	gchar *uri = g_strdup (val.value.s);
 	
 	/* printf("  debug: to=%s\n", idle_data->to); */
 	g_idle_add(on_idle_mail_to, (gpointer)uri);
 	
 	/* Note that we cannot report failures during sending, 
 	 * because that would be asynchronous. */
 	return OSSO_OK;
}


static gboolean
on_idle_open_message(gpointer user_data)
{
	gchar *uri = (gchar*)user_data;
	
	g_message ("not implemented yet %s", __FUNCTION__);
	
	g_free(uri);
	return FALSE; /* Do not call this callback again. */
}

static gint on_open_message(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	if (arguments->len != MODEST_DEBUS_OPEN_MESSAGE_ARGS_COUNT)
     	return OSSO_ERROR;
     	
    /* Use g_idle to context-switch into the application's thread: */

    /* Get the arguments: */
 	osso_rpc_t val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_OPEN_MESSAGE_ARG_URI);
 	gchar *uri = g_strdup (val.value.s);
 	
 	/* printf("  debug: to=%s\n", idle_data->to); */
 	g_idle_add(on_idle_open_message, (gpointer)uri);
 	
 	/* Note that we cannot report failures during sending, 
 	 * because that would be asynchronous. */
 	return OSSO_OK;
}
                      
/* Callback for normal D-BUS messages */
gint modest_dbus_req_handler(const gchar * interface, const gchar * method,
                      GArray * arguments, gpointer data,
                      osso_rpc_t * retval)
{
	/*
	printf("debug: modest_dbus_req_handler()\n");
	printf("debug: method received: %s\n", method);
	*/
	if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_SEND_MAIL) == 0) {
		return on_send_mail (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_MAIL_TO) == 0) {
		return on_mail_to (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_OPEN_MESSAGE) == 0) {
		return on_open_message (arguments, data, retval);
	}
	else
		return OSSO_ERROR;
}

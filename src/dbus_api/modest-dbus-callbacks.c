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
#include <stdio.h>

/* Callback for normal D-BUS messages */
static gint on_send_mail(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	if (arguments->len != MODEST_DEBUS_SEND_MAIL_ARGS_COUNT)
     	return OSSO_ERROR;
     	
    /* Get the arguments: */
 	osso_rpc_t val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_TO);
 	gchar *to = val.value.s;
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_CC);
 	gchar *cc = val.value.s;
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_BCC);
 	gchar *bcc = val.value.s;
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_SUBJECT);
 	gchar *subject = val.value.s;
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_SEND_MAIL_ARG_BODY);
 	gchar *body = val.value.s;
 	
 	printf("  debug: to=%s\n", to);
 	
 	/* Get the TnyTransportAccount so we can instantiate a mail operation: */
 	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr();
	gchar *account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
		return OSSO_ERROR;
	}
	
	TnyTransportAccount *transport_account =
		TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_tny_account_by_account
				      (modest_runtime_get_account_store(),
				       account_name,
				       TNY_ACCOUNT_TYPE_TRANSPORT));
	if (!transport_account) {
		g_printerr ("modest: no transport account found for '%s'\n", account_name);
		g_free (account_name);
		return OSSO_ERROR;
	}
	
 	/* Create the mail operation: */		
	ModestMailOperation *mail_operation = modest_mail_operation_new ();
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_operation);
	
	/* Use the mail operation: */
	gchar * from = modest_account_mgr_get_from_string (account_mgr,
							  account_name);
 	modest_mail_operation_send_new_mail (mail_operation,
				     transport_account,
				     from, /* from */
				     to, cc, bcc, subject, 
				     body, /* plain_body */
				     NULL, /* html_body */
				     NULL, /* attachments_list, GSList of TnyMimePart. */
				     (TnyHeaderFlags)0);
				     
	g_object_unref (G_OBJECT (transport_account));
	g_object_unref (G_OBJECT (mail_operation));
	g_free (from);
	g_free (account_name);
}
                      
/* Callback for normal D-BUS messages */
gint modest_dbus_req_handler(const gchar * interface, const gchar * method,
                      GArray * arguments, gpointer data,
                      osso_rpc_t * retval)
{
	printf("debug: modest_dbus_req_handler()\n");
	printf("debug: method received: %s\n", method);
	
	 if (g_ascii_strcasecmp(method, MODEST_DBUS_EXAMPLE_MESSAGE) == 0) {
	 	
     } else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_SEND_MAIL) == 0) {
     	return on_send_mail (arguments, data, retval);
	 }
    
 	return OSSO_OK;
}

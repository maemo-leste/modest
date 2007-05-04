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
#include "modest-ui-actions.h"
#include "widgets/modest-msg-edit-window.h"
#include "modest-tny-msg.h"
#include <libgnomevfs/gnome-vfs-utils.h>
#include <stdio.h>
#include <string.h>

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
		transport_account = TNY_TRANSPORT_ACCOUNT(modest_tny_account_store_get_transport_account_for_open_connection
				      (modest_runtime_get_account_store(),
				       account_name));
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
			ModestMailOperation *mail_operation = modest_mail_operation_new (MODEST_MAIL_OPERATION_ID_SEND, NULL);
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

/** uri_unescape:
 * @uri An escaped URI. URIs should always be escaped.
 * @len The length of the @uri string, or -1 if the string is null terminated.
 * 
 * Decode a URI, or URI fragment, as per RFC 1738.
 * http://www.ietf.org/rfc/rfc1738.txt
 * 
 * Return value: An unescaped string. This should be freed with g_free().
 */
static gchar* uri_unescape(const gchar* uri, size_t len)
{
	if (!uri)
		return NULL;
		
	if (len == -1)
		len = strlen (uri);
	
	/* Allocate an extra string so we can be sure that it is null-terminated,
	 * so we can use gnome_vfs_unescape_string().
	 * This is not efficient. */
	gchar * escaped_nullterminated = g_strndup (uri, len);
	gchar *result = gnome_vfs_unescape_string (escaped_nullterminated, NULL);
	g_free (escaped_nullterminated);
	
	return result;
}

/** uri_parse_mailto:
 * @mailto A mailto URI, with the mailto: prefix.
 * @list_items_and_values: A pointer to a list that should be filled with item namesand value strings, 
 * with each name item being followed by a value item. This list should be freed with g_slist_free) after 
 * all the string items have been freed. This parameter may be NULL.
 * Parse a mailto URI as per RFC2368.
 * http://www.ietf.org/rfc/rfc2368.txt
 * 
 * Return value: The to address, unescaped. This should be freed with g_free().
 */
static gchar* uri_parse_mailto (const gchar* mailto, GSList** list_items_and_values)
{
	const gchar* start_to = NULL;
	/* Remove the mailto: prefix: 
	 * 7 is the length of "mailto:": */
	if (strncmp (mailto, "mailto:", 7) == 0) {
		start_to = mailto + 7;
	}
	
	if (!start_to)
		return NULL;
	
	/* Look for ?, or the end of the string, marking the end of the to address: */
	const size_t len_to = strcspn (start_to, "?");
	gchar* result_to = uri_unescape (start_to, len_to);
	printf("debug: result_to=%s\n", result_to);
	
	/* Get any other items: */
	const size_t len_mailto = strlen (start_to);
	const gchar* p = start_to + len_to + 1; /* parsed so far. */
	const gchar* end = start_to + len_mailto;
	/* GSList *items = NULL; */
	const gchar* start_item_name = p;
	size_t len_item_name = 0;
	const gchar* start_item_value = NULL;
	while (p < end) {
		
		/* Looking for the end of a name; */
		if (start_item_name) {
			const size_t len = strcspn (p, "="); /* Returns whole string if none found. */
			if (len) {
				/* This marks the end of a name and the start of the value: */
				len_item_name = len;
				
				/* Skip over the name and mark the start of the value: */
				p += (len + 1); /* Skip over the = */
				start_item_value = p;
			}
		}
		
		/* Looking for the end of a value: */
		if (start_item_value) {
			const size_t len = strcspn (p, "?"); /* Returns whole string if none found. */
			/* ? marks the start of a new item: */
			if (len) {
				if (start_item_name && len_item_name) {
					/* Finish the previously-started item: */
					gchar *item_value = uri_unescape (start_item_value, len);
					gchar *item_name = g_strndup (start_item_name, len_item_name);
					/* printf ("debug: item name=%s, value=%s\n", item_name, item_value); */
					
					/* Append the items to the list */
					if(list_items_and_values) {
						*list_items_and_values = g_slist_append (*list_items_and_values, item_name);
						*list_items_and_values = g_slist_append (*list_items_and_values, item_value);
					}
				}
				
				/* Skip over the value and mark the start of a possible new name/value pair: */
				p += (len + 1); /* Skip over the ? */
				start_item_name = p;
				len_item_name = 0;
				start_item_value = NULL;
			}
		}
		
	}
	
	return result_to;
}


static gboolean
on_idle_mail_to(gpointer user_data)
{
	/* This is based on the implemenation of main.c:start_uil(): */
	
	gchar *uri = (gchar*)user_data;
	GSList *list_names_and_values = NULL;
	gchar *to = uri_parse_mailto (uri, &list_names_and_values);
	
	/* Get the TnyTransportAccount so we can instantiate a mail operation: */
 	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr();
	gchar *account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
	}
	
	TnyAccount *account = NULL;
	if (account_mgr) {
		account = modest_tny_account_store_get_transport_account_for_open_connection (
			modest_runtime_get_account_store(), account_name);
	}
	
	if (!account) {
		g_printerr ("modest: failed to get tny account folder'\n", account_name);
	} else {
		gchar * from = modest_account_mgr_get_from_string (account_mgr,
								  account_name);
		if (!from) {
			g_printerr ("modest: no from address for account '%s'\n", account_name);
		} else {
			const gchar *cc = NULL;
			const gchar *bcc = NULL;
			const gchar *subject = NULL;
			const gchar *body = NULL;
			
			/* Get the relevant items from the list: */
			GSList *list = list_names_and_values;
			while (list) {
				const gchar * name = (const gchar*)list->data;
				GSList *list_value = g_slist_next (list);
				const gchar * value = (const gchar*)list_value->data;
				
				if (strcmp (name, "cc") == 0) {
					cc = value;
				} else if (strcmp (name, "bcc") == 0) {
					bcc = value;
				} else if (strcmp (name, "subject") == 0) {
					subject = value;
				} else if (strcmp (name, "body") == 0) {
					body = value;
				}
				
				/* Go to the next pair: */
				if (list_value) {
					list = g_slist_next (list_value);
				} else 
					list = NULL;
			}
			
			/* Create the message: */
			TnyMsg *msg  = modest_tny_msg_new (to, from, 
				cc, bcc, subject, body, 
				NULL /* attachments */);
				
			if (!msg) {
				g_printerr ("modest: failed to create message\n");
			} else
			{
				/* Add the message to a folder and show its UI for editing: */
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
 	
 	/* Free the list, as required by the uri_parse_mailto() documentation: */
 	if (list_names_and_values)
 		g_slist_foreach (list_names_and_values, (GFunc)g_free, NULL);
 	g_slist_free (list_names_and_values);
 	
 	g_free(to);
		
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

static gboolean
on_idle_send_receive(gpointer user_data)
{
	ModestWindow* main_window = modest_window_mgr_get_main_window(
		modest_runtime_get_window_mgr ());
	do_send_receive(main_window);
	
	return FALSE; /* Do not call this callback again. */
}

static gint on_send_receive(GArray * arguments, gpointer data, osso_rpc_t * retval)
{ 	
    /* Use g_idle to context-switch into the application's thread: */

    /* This method has no arguments. */
 	
 	/* printf("  debug: to=%s\n", idle_data->to); */
 	g_idle_add(on_idle_send_receive, NULL);
 	
 	/* Note that we cannot report failures during send/receive, 
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
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_SEND_RECEIVE) == 0) {
		return on_send_receive (arguments, data, retval);
	}
	else
		return OSSO_ERROR;
}

void
modest_osso_cb_hw_state_handler(osso_hw_state_t *state, gpointer data)
{
    printf("%s()\n", __PRETTY_FUNCTION__);

    if(state->system_inactivity_ind)
    {
    }
    else if(state->save_unsaved_data_ind)
    {
    }
    else
    {
    
    }

    printf("debug: %s(): return\n", __PRETTY_FUNCTION__);
}

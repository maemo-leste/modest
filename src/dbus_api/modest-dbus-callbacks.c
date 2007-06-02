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
#include "modest-search.h"
#include "widgets/modest-msg-edit-window.h"
#include "modest-tny-msg.h"
#include <libmodest-dbus-client/libmodest-dbus-client.h>
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

typedef struct 
{
	gchar *to;
 	gchar *cc;
 	gchar *bcc;
 	gchar *subject;
 	gchar *body;
	GSList *attachments;
} ComposeMailIdleData;

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
			ModestMailOperation *mail_operation = modest_mail_operation_new (MODEST_MAIL_OPERATION_TYPE_SEND, NULL);
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
on_idle_compose_mail(gpointer user_data)
{
	ComposeMailIdleData *idle_data = (ComposeMailIdleData*)user_data;

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
			
			/* Create the message: */
			TnyMsg *msg  = modest_tny_msg_new (idle_data->to, from, 
				idle_data->cc, idle_data->bcc, idle_data->subject, idle_data->body, 
				idle_data->attachments);
				
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

 	/* Free the idle data: */
	g_free (idle_data->to);
	g_free (idle_data->cc);
	g_free (idle_data->bcc);
	g_free (idle_data->subject);
	g_free (idle_data->body);
	g_free (idle_data->attachments);
	g_free (idle_data);
	
 	g_free (account_name);
	return FALSE; /* Do not call this callback again. */
}

static gint on_compose_mail(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
	gchar **list = NULL;
	gint i = 0;
	
	if (arguments->len != MODEST_DEBUS_COMPOSE_MAIL_ARGS_COUNT)
     	return OSSO_ERROR;
     	
	/* Use g_idle to context-switch into the application's thread: */
 	ComposeMailIdleData *idle_data = g_new0(ComposeMailIdleData, 1); /* Freed in the idle callback. */
 	
	/* Get the arguments: */
 	osso_rpc_t val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_COMPOSE_MAIL_ARG_TO);
 	idle_data->to = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_COMPOSE_MAIL_ARG_CC);
 	idle_data->cc = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_COMPOSE_MAIL_ARG_BCC);
 	idle_data->bcc = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_COMPOSE_MAIL_ARG_SUBJECT);
 	idle_data->subject = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_COMPOSE_MAIL_ARG_BODY);
 	idle_data->body = g_strdup (val.value.s);
 	
 	val = g_array_index(arguments, osso_rpc_t, MODEST_DEBUS_COMPOSE_MAIL_ARG_ATTACHMENTS);
 	gchar *attachments_str = g_strdup (val.value.s);

	list = g_strsplit(attachments_str, ",", 0);
	for (i=0; list[i] != NULL; i++) {
		idle_data->attachments = g_slist_append(idle_data->attachments, g_strdup(list[i]));
	}
	g_strfreev(list);

 	
 	/* printf("  debug: to=%s\n", idle_data->to); */
 	g_idle_add(on_idle_compose_mail, (gpointer)idle_data);
 	
 	/* Note that we cannot report failures during sending, 
 	 * because that would be asynchronous. */
 	return OSSO_OK;
}


static TnyMsg *
find_message_by_url (const char *uri, TnyAccount **ac_out)
{

	ModestTnyAccountStore *astore;
	TnyAccount            *account;
	TnyFolder             *folder;
	TnyMsg                *msg;

	account = NULL;
	msg = NULL;
	folder = NULL;

	astore = modest_runtime_get_account_store ();
	
	if (astore == NULL) {
		return NULL;
	}
	
	g_debug ("Got AccountStore, lets go");

	account = tny_account_store_find_account (TNY_ACCOUNT_STORE (astore),
						  uri);
	
	if (account == NULL) {
		return NULL;
	}

	g_debug ("Found account");

	if ( ! TNY_IS_STORE_ACCOUNT (account)) {
		goto out;
	}

	g_debug ("Account is store account");

	*ac_out = account;

	folder = tny_store_account_find_folder (TNY_STORE_ACCOUNT (account),
						uri,
						NULL);

	if (folder == NULL) {
		goto out;
	}
	g_debug ("Found folder");
	

	msg = tny_folder_find_msg (folder, uri, NULL);

out:
	if (account && !msg) {
		g_object_unref (account);
		*ac_out = NULL;
	}

	if (folder) {
		g_object_unref (folder);
	}

	return msg;
}

static gboolean
on_idle_open_message (gpointer user_data)
{
	ModestWindow *msg_view;
	TnyMsg       *msg;
	TnyAccount   *account;
	TnyHeader    *header; 
	const char   *msg_uid;
	const char   *account_name;
	char         *uri;
       
	uri = (char *) user_data;

	g_debug ("Trying to find msg by url: %s", uri);	
	msg = find_message_by_url (uri, &account);
	g_free (uri);

	if (msg == NULL) {
		return FALSE;
	}
	g_debug ("Found message");

	header = tny_msg_get_header (msg);
	account_name = tny_account_get_name (account);
	msg_uid = tny_header_get_uid (header);
	
	msg_view = modest_msg_view_window_new (msg,
					       account_name,
					       msg_uid);
	/* TODO: does that leak the msg_view ?! */

	gtk_widget_show_all (GTK_WIDGET (msg_view));

	g_object_unref (header);
	g_object_unref (account);
	
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
	ModestWindow *win;

	/* Pick the main window if it exists */
	win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ());

	/* Send & receive all if "Update automatically" is set */
	/* TODO: check the auto-update parameter in the configuration */
	modest_ui_actions_do_send_receive_all (win);
	
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
	
	printf("debug: modest_dbus_req_handler()\n");
	printf("debug: method received: %s\n", method);
	
	if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_SEND_MAIL) == 0) {
		return on_send_mail (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_MAIL_TO) == 0) {
		return on_mail_to (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_OPEN_MESSAGE) == 0) {
		return on_open_message (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_SEND_RECEIVE) == 0) {
		return on_send_receive (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_COMPOSE_MAIL) == 0) {
		return on_compose_mail (arguments, data, retval);
	}
	else {
		/* We need to return INVALID here so
		 * osso is returning DBUS_HANDLER_RESULT_NOT_YET_HANDLED 
		 * so our modest_dbus_req_filter can kick in!
		 * */
		return OSSO_INVALID;
	}
}

#define SEARCH_HIT_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_UINT64_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_TYPE_INT64_AS_STRING \
	DBUS_STRUCT_END_CHAR_AS_STRING

static DBusMessage *
search_result_to_messsage (DBusMessage *reply,
			   GList       *hits)
{
	DBusMessageIter iter;
	DBusMessageIter array_iter;
	GList          *hit_iter;

	dbus_message_iter_init_append (reply, &iter); 
	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  SEARCH_HIT_DBUS_TYPE,
					  &array_iter); 

	for (hit_iter = hits; hit_iter; hit_iter = hit_iter->next) {
		DBusMessageIter  struct_iter;
		TnyFolder       *tf;
		TnyHeader       *header;
		TnyHeaderFlags   flags;
		char            *msg_url = "";
		const char      *subject = "";
		const char      *folder = "";
		const char      *sender = "";
		guint64          size = 0;
		gboolean         has_attachemnt = FALSE;
		gboolean         is_unread = FALSE;
		gint64           ts = 0;
		char             *furl;
		const char       *uid;

		g_debug ("Marshalling hit ...(%s)",
			 TNY_IS_HEADER (hit_iter->data) ? "yes" : "no");

		header = TNY_HEADER (hit_iter->data);
		tf = tny_header_get_folder (header);
		furl = tny_folder_get_url_string (tf);

		uid = tny_header_get_uid (header);
		msg_url = g_strdup_printf ("%s/%s", furl, uid);
		
		subject = tny_header_get_subject (header);
		folder = furl;
		sender = tny_header_get_from (header);
		size = tny_header_get_message_size (header);

		flags = tny_header_get_flags (header);
		has_attachemnt = flags & TNY_HEADER_FLAG_ATTACHMENTS;
		is_unread = ! (flags & TNY_HEADER_FLAG_SEEN);
		ts = tny_header_get_date_received (header);

		g_debug ("Adding hit: %s", msg_url);	
		
		dbus_message_iter_open_container (&array_iter,
						  DBUS_TYPE_STRUCT,
						  NULL,
						  &struct_iter);

   		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&msg_url);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&subject); 

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&folder);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_STRING,
						&sender);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_UINT64,
						&size);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_BOOLEAN,
						&has_attachemnt);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_BOOLEAN,
						&is_unread);
		
		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_INT64,
						&ts);

		dbus_message_iter_close_container (&array_iter,
						   &struct_iter); 


		g_free (msg_url);
		g_free (furl);
		
		/* Also unref the header, we don't need it anymore */
		g_object_unref (header);
	}

	dbus_message_iter_close_container (&iter, &array_iter);

	return reply;
}

DBusHandlerResult
modest_dbus_req_filter (DBusConnection *con,
			DBusMessage    *message,
			void           *user_data)
{
	gboolean  handled = FALSE;
  	DBusError error;

	if (dbus_message_is_method_call (message,
					 MODEST_DBUS_IFACE,
					 MODEST_DBUS_METHOD_SEARCH)) {
		ModestDBusSearchFlags dbus_flags;
		ModestSearch  search;
		DBusMessage  *reply = NULL;
		dbus_bool_t  res;
		dbus_int64_t sd_v;
		dbus_int64_t ed_v;
		dbus_int32_t flags_v;
		dbus_uint32_t serial;
		dbus_uint32_t size_v;
		char *folder;
		char *query;
		time_t start_date;
		time_t end_date;
		GList *hits;

		handled = TRUE;

		dbus_error_init (&error);

		sd_v = ed_v = 0;
		flags_v = 0;

		res = dbus_message_get_args (message,
					     &error,
					     DBUS_TYPE_STRING, &query,
					     DBUS_TYPE_STRING, &folder,
					     DBUS_TYPE_INT64, &sd_v,
					     DBUS_TYPE_INT64, &ed_v,
					     DBUS_TYPE_INT32, &flags_v,
					     DBUS_TYPE_UINT32, &size_v,
					     DBUS_TYPE_INVALID);
		
		dbus_flags = (ModestDBusSearchFlags) flags_v;
		start_date = (time_t) sd_v;
		end_date = (time_t) ed_v;

		memset (&search, 0, sizeof (search));
		search.query  = query;
		search.before = start_date;
		search.after  = end_date;
		search.flags  = 0;

		if (dbus_flags & MODEST_DBUS_SEARCH_SUBJECT) {
			search.flags |= MODEST_SEARCH_SUBJECT;
			search.subject = query;
		}

		if (dbus_flags & MODEST_DBUS_SEARCH_SENDER) {
			search.flags |=  MODEST_SEARCH_SENDER;
			search.from = query;
		}

		if (dbus_flags & MODEST_DBUS_SEARCH_RECIPIENT) {
			search.flags |= MODEST_SEARCH_RECIPIENT; 
			search.recipient = query;
		}

		if (dbus_flags & MODEST_DBUS_SEARCH_BODY) {
			search.flags |=  MODEST_SEARCH_BODY; 
			search.subject = query;
		}

		if (sd_v > 0) {
			search.flags |= MODEST_SEARCH_BEFORE;
			search.before = start_date;
		}

		if (ed_v > 0) {
			search.flags |= MODEST_SEARCH_AFTER;
			search.after = end_date;
		}

		if (size_v > 0) {
			search.flags |= MODEST_SEARCH_SIZE;
			search.minsize = size_v;
		}

#ifdef MODEST_HAVE_OGS
		search.flags |= MODEST_SEARCH_USE_OGS;
#endif

		hits = modest_search_all_accounts (&search);

		reply = dbus_message_new_method_return (message);

		search_result_to_messsage (reply, hits);
		
		if (reply == NULL) {
			g_warning ("Could not create reply");
		}

		if (reply) {
			dbus_connection_send (con, reply, &serial);
	    		dbus_connection_flush (con);
	    		dbus_message_unref (reply);
		}

	}
	

	return (handled ? 
		DBUS_HANDLER_RESULT_HANDLED :
		DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

void
modest_osso_cb_hw_state_handler(osso_hw_state_t *state, gpointer data)
{
	/* TODO? */
    /* printf("%s()\n", __PRETTY_FUNCTION__); */

    if(state->system_inactivity_ind)
    {
    }
    else if(state->save_unsaved_data_ind)
    {
    }
    else
    {
    
    }

    /* printf("debug: %s(): return\n", __PRETTY_FUNCTION__); */
}

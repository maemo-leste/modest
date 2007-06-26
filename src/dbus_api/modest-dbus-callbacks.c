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
#include <glib/gstdio.h>
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#else
#include <libgnomevfs/gnome-vfs-mime.h>
#endif
#include <tny-fs-stream.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>

typedef struct 
{
	gchar *to;
 	gchar *cc;
 	gchar *bcc;
 	gchar *subject;
 	gchar *body;
 	gchar *attachments;
} SendMailIdleData;

typedef struct 
{
	gchar *to;
 	gchar *cc;
 	gchar *bcc;
 	gchar *subject;
 	gchar *body;
	gchar *attachments;
} ComposeMailIdleData;

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
	gdk_threads_enter();
	
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

	gdk_threads_leave();
	
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
	gdk_threads_enter();
	
	ComposeMailIdleData *idle_data = (ComposeMailIdleData*)user_data;
	gchar **list = NULL;
	gint i = 0;

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
		g_printerr ("modest: failed to get tny account folder'%s'\n", account_name);
	} else {
		gchar * from = modest_account_mgr_get_from_string (account_mgr,
								  account_name);
		if (!from) {
			g_printerr ("modest: no from address for account '%s'\n", account_name);
		} else {
	
			/* Create the message: */
			TnyMsg *msg  = modest_tny_msg_new (idle_data->to, from, 
				idle_data->cc, idle_data->bcc, idle_data->subject, idle_data->body, 
				NULL); /* NULL because m_t_m_n doesn't use it */
				
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

					list = g_strsplit(idle_data->attachments, ",", 0);
					for (i=0; list[i] != NULL; i++) {
						modest_msg_edit_window_attach_file_noninteractive(
								(ModestMsgEditWindow *)win, list[i]);
					}
					g_strfreev(list);
					
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
 	
 	gdk_threads_leave();
 	
	return FALSE; /* Do not call this callback again. */
}

static gint on_compose_mail(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
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
 	idle_data->attachments = g_strdup (val.value.s);

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
	GError *err = NULL;
	account = NULL;
	msg = NULL;
	folder = NULL;

	astore = modest_runtime_get_account_store ();
	
	if (astore == NULL) {
		return NULL;
	}

	account = tny_account_store_find_account (TNY_ACCOUNT_STORE (astore),
						  uri);
	
	if (account == NULL) {
		g_debug ("%s: tny_account_store_find_account() failed.\n", __FUNCTION__);
		return NULL;
	}

	g_debug ("%s: Found account.\n", __FUNCTION__);

	if ( ! TNY_IS_STORE_ACCOUNT (account)) {
		goto out;
	}

	g_debug ("%s: Account is store account.\n", __FUNCTION__);

	*ac_out = account;

	folder = tny_store_account_find_folder (TNY_STORE_ACCOUNT (account),
						uri,
						&err);

	if (folder == NULL) {
		g_debug ("%s: tny_store_account_find_folder() failed\naccount=%s, uri=%s.\n", __FUNCTION__, 
			tny_account_get_id (TNY_ACCOUNT(account)), uri);
		goto out;
	}
	g_debug ("%s: Found folder. (%s)\n",  __FUNCTION__, uri);
	

	msg = tny_folder_find_msg (folder, uri, &err);
	
	if (!msg) {
		g_debug ("%s: tny_folder_find_msg() failed for folder %s\n  with error=%s.\n",
			 __FUNCTION__, tny_folder_get_id (folder), err->message);
	}

out:
	if (err)
		g_error_free (err);

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
	gdk_threads_enter();
	
	ModestWindow *msg_view;
	TnyMsg       *msg;
	TnyAccount   *account;
	TnyHeader    *header; 
	const char   *msg_uid;
	const char   *account_name;
	char         *uri;
       
	uri = (char *) user_data;

	g_debug ("%s: Trying to find msg by url: %s", __FUNCTION__, uri);	
	msg = find_message_by_url (uri, &account);
	g_free (uri);

	if (msg == NULL) {
		g_debug ("  %s: message not found.", __FUNCTION__);
		gdk_threads_leave();
		return FALSE;
	}
	g_debug ("  %s: Found message.", __FUNCTION__);

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
	
	gdk_threads_leave();
	
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


static gint
on_delete_message (GArray *arguments, gpointer data, osso_rpc_t *retval)
{
	TnyList      *headers;
	TnyFolder    *folder;
	TnyIterator  *iter; 
	TnyHeader    *header;
	TnyHeader    *msg_header;
	TnyMsg       *msg;
	TnyAccount   *account;
	GError       *error;
	osso_rpc_t    val;
	const char   *uri;
	const char   *uid;
	gint          res;

	if (arguments->len != MODEST_DEBUS_DELETE_MESSAGE_ARGS_COUNT) {
		return OSSO_ERROR;
	}

	val = g_array_index (arguments,
			     osso_rpc_t,
			     MODEST_DEBUS_DELETE_MESSAGE_ARG_URI);

	uri = (const char *) val.value.s;

	g_debug ("Searching message (delete message)");
	
	msg = find_message_by_url (uri, &account);

	if (msg == NULL) {
		return OSSO_ERROR;
	}

	g_debug ("Found message");
	
	msg_header = tny_msg_get_header (msg);
	uid = tny_header_get_uid (msg_header);
	folder = tny_msg_get_folder (msg);


	/* tny_msg_get_header () flaw:
	 * From tinythingy doc: You can't use the returned instance with the
	 * TnyFolder operations
	 *
	 * To get a header instance that will work with these folder methods,
	 * you can use tny_folder_get_headers.
	 *
	 * Ok, we will do so then. Sigh.
	 * */
	headers = tny_simple_list_new ();

	tny_folder_get_headers (folder, headers, TRUE, NULL);
	iter = tny_list_create_iterator (headers);
	header = NULL;

	g_debug ("Searching header for msg in folder");
	while (!tny_iterator_is_done (iter)) {
		const char *cur_id;

		header = TNY_HEADER (tny_iterator_get_current (iter));
		cur_id = tny_header_get_uid (header);
		
		if (cur_id && uid && g_str_equal (cur_id, uid)) {
			g_debug ("Found correspoding header from folder");
			break;
		}

		header = NULL;
		g_object_unref (header);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (headers);
	
	g_object_unref (msg_header);
	g_object_unref (msg);

	if (header == NULL) {
		g_object_unref (folder);
		return OSSO_ERROR;
	}


	error = NULL;
	res = OSSO_OK;
	tny_folder_remove_msg (folder, header, &error);

	if (error != NULL) {
		res = OSSO_ERROR;
		g_error_free (error);
	}

	g_object_unref (folder);
	return res;
}

static gboolean
on_idle_send_receive(gpointer user_data)
{
	gdk_threads_enter();
	ModestWindow *win;

	/* Pick the main window if it exists */
	win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ());

	/* Send & receive all if "Update automatically" is set */
	/* TODO: check the auto-update parameter in the configuration */
	modest_ui_actions_do_send_receive_all (win);
	
	gdk_threads_leave();
	
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

static gboolean
on_idle_open_default_inbox(gpointer user_data)
{
	gdk_threads_enter();
	
	ModestWindow *win = 
		modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ());

	/* Get the folder view */
	GtkWidget *folder_view = modest_main_window_get_child_widget (MODEST_MAIN_WINDOW (win),
							   MODEST_WIDGET_TYPE_FOLDER_VIEW);
	modest_folder_view_select_first_inbox_or_local (
		MODEST_FOLDER_VIEW (folder_view));
	
	gdk_threads_leave();
	
	return FALSE; /* Do not call this callback again. */
}

static gint on_open_default_inbox(GArray * arguments, gpointer data, osso_rpc_t * retval)
{
    /* Use g_idle to context-switch into the application's thread: */

    /* This method has no arguments. */
 	
 	g_idle_add(on_idle_open_default_inbox, NULL);
 	
 	/* Note that we cannot report failures during send/receive, 
 	 * because that would be asynchronous. */
 	return OSSO_OK;
}
                      
/* Callback for normal D-BUS messages */
gint modest_dbus_req_handler(const gchar * interface, const gchar * method,
                      GArray * arguments, gpointer data,
                      osso_rpc_t * retval)
{
	
	g_debug ("debug: %s\n", __FUNCTION__);
	g_debug ("debug: %s: method received: %s\n", __FUNCTION__, method);
	
	if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_MAIL_TO) == 0) {
		return on_mail_to (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_OPEN_MESSAGE) == 0) {
		return on_open_message (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_SEND_RECEIVE) == 0) {
		return on_send_receive (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_COMPOSE_MAIL) == 0) {
		return on_compose_mail (arguments, data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_DELETE_MESSAGE) == 0) {
		return on_delete_message (arguments,data, retval);
	} else if (g_ascii_strcasecmp(method, MODEST_DBUS_METHOD_OPEN_DEFAULT_INBOX) == 0) {
		return on_open_default_inbox (arguments, data, retval);
	}
	else { 
		/* We need to return INVALID here so
		 * libosso will return DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
		 * so that our modest_dbus_req_filter will then be tried instead.
		 * */
		return OSSO_INVALID;
	}
}
					 
/* A complex D-Bus type (like a struct),
 * used to return various information about a search hit.
 */
#define SEARCH_HIT_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING /* msgid */ \
	DBUS_TYPE_STRING_AS_STRING /* subject */ \
	DBUS_TYPE_STRING_AS_STRING /* folder */ \
	DBUS_TYPE_STRING_AS_STRING /* sender */ \
	DBUS_TYPE_UINT64_AS_STRING /* msize */ \
	DBUS_TYPE_BOOLEAN_AS_STRING /* has_attachment */ \
	DBUS_TYPE_BOOLEAN_AS_STRING /* is_unread */ \
	DBUS_TYPE_INT64_AS_STRING /* timestamp */ \
	DBUS_STRUCT_END_CHAR_AS_STRING

static DBusMessage *
search_result_to_message (DBusMessage *reply,
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
		ModestSearchHit *hit;
		char            *msg_url;
		const char      *subject;
		const char      *folder;
		const char      *sender;
		guint64          size;
		gboolean         has_attachment;
		gboolean         is_unread;
		gint64           ts;

		hit = (ModestSearchHit *) hit_iter->data;

		msg_url = hit->msgid;
		subject = hit->subject;
		folder  = hit->folder;
		sender  = hit->sender;
		size           = hit->msize;
		has_attachment = hit->has_attachment;
		is_unread      = hit->is_unread;
		ts             = hit->timestamp;

		g_debug ("DEBUG: %s: Adding hit: %s", __FUNCTION__, msg_url);	
		
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
						&has_attachment);

		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_BOOLEAN,
						&is_unread);
		
		dbus_message_iter_append_basic (&struct_iter,
						DBUS_TYPE_INT64,
						&ts);

		dbus_message_iter_close_container (&array_iter,
						   &struct_iter); 

		g_free (hit->msgid);
		g_free (hit->subject);
		g_free (hit->folder);
		g_free (hit->sender);

		g_slice_free (ModestSearchHit, hit);
	}

	dbus_message_iter_close_container (&iter, &array_iter);

	return reply;
}


static void
on_dbus_method_search (DBusConnection *con, DBusMessage *message)
{
	ModestDBusSearchFlags dbus_flags;
	DBusMessage  *reply = NULL;
	dbus_bool_t  res;
	dbus_int64_t sd_v;
	dbus_int64_t ed_v;
	dbus_int32_t flags_v;
	dbus_uint32_t size_v;
	const char *folder;
	const char *query;
	time_t start_date;
	time_t end_date;
	GList *hits;

	DBusError error;
	dbus_error_init (&error);

	sd_v = ed_v = 0;
	flags_v = 0;

	res = dbus_message_get_args (message,
				     &error,
				     DBUS_TYPE_STRING, &query,
				     DBUS_TYPE_STRING, &folder, /* e.g. "INBOX/drafts": TODO: Use both an ID and a display name. */
				     DBUS_TYPE_INT64, &sd_v,
				     DBUS_TYPE_INT64, &ed_v,
				     DBUS_TYPE_INT32, &flags_v,
				     DBUS_TYPE_UINT32, &size_v,
				     DBUS_TYPE_INVALID);
	
	dbus_flags = (ModestDBusSearchFlags) flags_v;
	start_date = (time_t) sd_v;
	end_date = (time_t) ed_v;

	ModestSearch search;
	memset (&search, 0, sizeof (search));
	
	/* Remember what folder we are searching in:
	 *
	 * Note that we don't copy the strings, 
	 * because this struct will only be used for the lifetime of this function.
	 */
	search.folder = folder;

   /* Remember the text to search for: */
#ifdef MODEST_HAVE_OGS
	search.query  = query;
#endif

	/* Other criteria: */
	search.before = start_date;
	search.after  = end_date;
	search.flags  = 0;

	/* Text to serach for in various parts of the message: */
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
		search.body = query;
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
	g_debug ("%s: Starting search for %s", __FUNCTION__, search.query);
#endif

	/* Note that this currently gets folders and messages from the servers, 
	 * which can take a long time. libmodest_dbus_client_search() can timeout, 
	 * reporting no results, if this takes a long time: */
	hits = modest_search_all_accounts (&search);

	reply = dbus_message_new_method_return (message);

	search_result_to_message (reply, hits);

	if (reply == NULL) {
		g_warning ("%s: Could not create reply.", __FUNCTION__);
	}

	if (reply) {
		dbus_uint32_t serial = 0;
		dbus_connection_send (con, reply, &serial);
    	dbus_connection_flush (con);
    	dbus_message_unref (reply);
	}

	g_list_free (hits);
}


/* A complex D-Bus type (like a struct),
 * used to return various information about a folder.
 */
#define GET_FOLDERS_RESULT_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING /* Folder Name */ \
	DBUS_TYPE_STRING_AS_STRING /* Folder URI */ \
	DBUS_STRUCT_END_CHAR_AS_STRING

static DBusMessage *
get_folders_result_to_message (DBusMessage *reply,
			   GList *folder_ids)
{
	DBusMessageIter iter;	
	dbus_message_iter_init_append (reply, &iter); 
	
	DBusMessageIter array_iter;
	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  GET_FOLDERS_RESULT_DBUS_TYPE,
					  &array_iter); 

	GList *list_iter = folder_ids;
	for (list_iter = folder_ids; list_iter; list_iter = list_iter->next) {
		
		const gchar *folder_name = (const gchar*)list_iter->data;
		if (folder_name) {
			/* g_debug ("DEBUG: %s: Adding folder: %s", __FUNCTION__, folder_name);	*/
			
			DBusMessageIter struct_iter;
			dbus_message_iter_open_container (&array_iter,
							  DBUS_TYPE_STRUCT,
							  NULL,
							  &struct_iter);
	
			/* name: */
			dbus_message_iter_append_basic (&struct_iter,
							DBUS_TYPE_STRING,
							&folder_name); /* The string will be copied. */
							
			/* URI: This is maybe not needed by osso-global-search: */
			const gchar *folder_uri = "TODO:unimplemented";
			dbus_message_iter_append_basic (&struct_iter,
							DBUS_TYPE_STRING,
							&folder_uri); /* The string will be copied. */
	
			dbus_message_iter_close_container (&array_iter,
							   &struct_iter); 
		}
	}

	dbus_message_iter_close_container (&iter, &array_iter);

	return reply;
}

static void
add_single_folder_to_list (TnyFolder *folder, GList** list)
{
	if (!folder)
		return;
		
	/* Add this folder to the list: */
	/*
	const gchar * folder_name = tny_folder_get_name (folder);
	if (folder_name)
		*list = g_list_append(*list, g_strdup (folder_name));
	else {
	*/
		/* osso-global-search only uses one string,
		 * so ID is the only thing that could possibly identify a folder.
		 * TODO: osso-global search should probably be changed to 
		 * take an ID and a Name.
		 */
		const gchar * id =  tny_folder_get_id (folder);
		if (id && strlen(id))
			*list = g_list_append(*list, g_strdup (id));
		/*
		else {
			g_warning ("DEBUG: %s: folder has no name or ID.\n", __FUNCTION__);	
		}
		
	}
	*/
}

static void
add_folders_to_list (TnyFolderStore *folder_store, GList** list)
{
	if (!folder_store)
		return;
	
	/* Add this folder to the list: */
	if (TNY_IS_FOLDER (folder_store)) {
		add_single_folder_to_list (TNY_FOLDER (folder_store), list);
	}	
	
		
	/* Recurse into child folders: */
		
	/* Get the folders list: */
	/*
	TnyFolderStoreQuery *query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, 
		TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	*/
	TnyList *all_folders = tny_simple_list_new ();
	tny_folder_store_get_folders (folder_store,
				      all_folders,
				      NULL /* query */,
				      NULL /* error */);

	TnyIterator *iter = tny_list_create_iterator (all_folders);
	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder = TNY_FOLDER (tny_iterator_get_current (iter));
		if (TNY_IS_FOLDER_STORE (folder))
			add_folders_to_list (TNY_FOLDER_STORE (folder), list);
		else {
			add_single_folder_to_list (TNY_FOLDER (folder), list);
		}
		
		tny_iterator_next (iter);
	}
	g_object_unref (G_OBJECT (iter));
}

static void
on_dbus_method_get_folders (DBusConnection *con, DBusMessage *message)
{
	DBusMessage  *reply = NULL;
	

	/* Get the TnyStoreAccount so we can get the folders: */
 	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr();
	gchar *account_name = modest_account_mgr_get_default_account (account_mgr);
	if (!account_name) {
		g_printerr ("modest: no account found\n");
	}
	
	TnyAccount *account = NULL;
	if (account_mgr) {
		account = modest_tny_account_store_get_server_account (
			modest_runtime_get_account_store(), account_name, 
			TNY_ACCOUNT_TYPE_STORE);
	}
	
	if (!account) {
		g_printerr ("modest: failed to get tny account folder'%s'\n", account_name);
	} 
		
	printf("DEBUG: %s: Getting folders for account name=%s\n", __FUNCTION__, account_name);
	g_free (account_name);
	account_name = NULL;
	
	GList *folder_names = NULL;
	add_folders_to_list (TNY_FOLDER_STORE (account), &folder_names);

	g_object_unref (account);
	account = NULL;
	
	
	/* Also add the folders from the local folders account,
	 * because they are (currently) used with all accounts:
	 * TODO: This is not working. It seems to get only the Merged Folder (with an ID of "" (not NULL)).
	 */
	TnyAccount *account_local = 
		modest_tny_account_store_get_local_folders_account (
			TNY_ACCOUNT_STORE (modest_runtime_get_account_store()));
	add_folders_to_list (TNY_FOLDER_STORE (account_local), &folder_names);

	g_object_unref (account_local);
	account_local = NULL;


	/* Put the result in a DBus reply: */
	reply = dbus_message_new_method_return (message);

	get_folders_result_to_message (reply, folder_names);

	if (reply == NULL) {
		g_warning ("%s: Could not create reply.", __FUNCTION__);
	}

	if (reply) {
		dbus_uint32_t serial = 0;
		dbus_connection_send (con, reply, &serial);
    	dbus_connection_flush (con);
    	dbus_message_unref (reply);
	}

	g_list_foreach (folder_names, (GFunc)g_free, NULL);
	g_list_free (folder_names);
}


/** This D-Bus handler is used when the main osso-rpc 
 * D-Bus handler has not handled something.
 * We use this for D-Bus methods that need to use more complex types 
 * than osso-rpc supports.
 */
DBusHandlerResult
modest_dbus_req_filter (DBusConnection *con,
			DBusMessage    *message,
			void           *user_data)
{
	gboolean handled = FALSE;

	if (dbus_message_is_method_call (message,
					 MODEST_DBUS_IFACE,
					 MODEST_DBUS_METHOD_SEARCH)) {
		on_dbus_method_search (con, message);
		handled = TRUE;			 	
	} else if (dbus_message_is_method_call (message,
					 MODEST_DBUS_IFACE,
					 MODEST_DBUS_METHOD_GET_FOLDERS)) {
		on_dbus_method_get_folders (con, message);
		handled = TRUE;			 	
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

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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <tny-shared.h>
#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>

#include <libmodest-dbus-client/libmodest-dbus-client.h>

#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-tny-account-store.h"
#include "modest-tny-account.h"
#include "modest-search.h"
#include "modest-runtime.h"

static gchar *
g_strdup_or_null (const gchar *str)
{
	gchar *string = NULL;

	if  (str != NULL) {
		string = g_strdup (str);
	}

	return string;
}

typedef struct
{
	GMainLoop* loop;
	TnyAccount *account;
	gboolean is_online;
	gint count_tries;
} UtilIdleData;

#define NUMBER_OF_TRIES 10 /* Try approx every second, ten times. */

static gboolean 
on_timeout_check_account_is_online(gpointer user_data)
{
	printf ("DEBUG: %s:\n", __FUNCTION__);
	UtilIdleData *data = (UtilIdleData*)user_data;
	
	gboolean stop_trying = FALSE;
	if (data && data->account && 
		(tny_account_get_connection_status (data->account) == TNY_CONNECTION_STATUS_CONNECTED) )
	{
		data->is_online = TRUE;
		
		stop_trying = TRUE;
	}
	else {
		/* Give up if we have tried too many times: */
		if (data->count_tries >= NUMBER_OF_TRIES)
		{
			stop_trying = TRUE;
		}
		else {
			/* Wait for another timeout: */
			++(data->count_tries);
		}
	}
	
	if (stop_trying) {
		/* Allow the function that requested this idle callback to continue: */
		if (data->loop)
			g_main_loop_quit (data->loop);
		
		return FALSE; /* Don't call this again. */
	} else {
		return TRUE; /* Call this timeout callback again. */
	}
}

/* Return TRUE immediately if the account is already online,
 * otherwise check every second for NUMBER_OF_TRIES seconds and return TRUE as 
 * soon as the account is online, or FALSE if the account does 
 * not become online in the NUMBER_OF_TRIES seconds.
 * This is useful when the D-Bus method was run immediately after 
 * the application was started (when using D-Bus activation), 
 * because the account usually takes a short time to go online.
 */
static gboolean
check_and_wait_for_account_is_online(TnyAccount *account)
{
	if (tny_account_get_connection_status (account) == TNY_CONNECTION_STATUS_CONNECTED)
		return TRUE;
		
	/* This blocks on the result: */
	UtilIdleData *data = g_slice_new0 (UtilIdleData);
	data->is_online = FALSE;
		
	GMainContext *context = NULL; /* g_main_context_new (); */
	data->loop = g_main_loop_new (context, FALSE /* not running */);

	g_timeout_add (1000, &on_timeout_check_account_is_online, data);

	/* This main loop will run until the idle handler has stopped it: */
	g_main_loop_run (data->loop);

	g_main_loop_unref (data->loop);
	/* g_main_context_unref (context); */

	g_slice_free (UtilIdleData, data);
	
	return data->is_online;	
}

static GList*
add_hit (GList *list, TnyHeader *header, TnyFolder *folder)
{
	ModestSearchHit *hit;
	TnyHeaderFlags   flags;
	char            *furl;
	char            *msg_url;
	const char      *uid;
	const char      *subject;
	const char      *sender;

	hit = g_slice_new0 (ModestSearchHit);

	furl = tny_folder_get_url_string (folder);
	printf ("DEBUG: %s: folder URL=%s\n", __FUNCTION__, furl);
	if (!furl) {
		g_warning ("%s: tny_folder_get_url_string(): returned NULL for folder. Folder name=%s\n", __FUNCTION__, tny_folder_get_name (folder));
	}
	
	/* Make sure that we use the short UID instead of the long UID,
	 * and/or find out what UID form is used when finding, in camel_data_cache_get().
	 * so we can find what we get. Philip is working on this.
	 */
	uid = tny_header_get_uid (header);
	if (!furl) {
		g_warning ("%s: tny_header_get_uid(): returned NULL for message with subject=%s\n", __FUNCTION__, tny_header_get_subject (header));
	}
	
	msg_url = g_strdup_printf ("%s/%s", furl, uid);
	g_free (furl);
	
	subject = tny_header_get_subject (header);
	sender = tny_header_get_from (header);
	
	flags = tny_header_get_flags (header);

	hit->msgid = msg_url;
	hit->subject = g_strdup_or_null (subject);
	hit->sender = g_strdup_or_null (sender);
	hit->folder = g_strdup_or_null (tny_folder_get_name (folder));
	hit->msize = tny_header_get_message_size (header);
	hit->has_attachment = flags & TNY_HEADER_FLAG_ATTACHMENTS;
	hit->is_unread = ! (flags & TNY_HEADER_FLAG_SEEN);
	hit->timestamp = tny_header_get_date_received (header);
	
	return g_list_prepend (list, hit);
}

/** Call this until it returns FALSE or nread is set to 0.
 * 
 * @result: FALSE is something failed. */
static gboolean
read_chunk (TnyStream *stream, char *buffer, gsize count, gsize *nread)
{
	gsize _nread = 0;
	gssize res = 0;

	while (_nread < count) {
		res = tny_stream_read (stream,
				       buffer + _nread, 
				       count - _nread);
		if (res == -1) { /* error */
			*nread = _nread;
			return FALSE;
		}

		_nread += res;
		
		if (res == 0) { /* no more bytes read. */
			*nread = _nread;
			return TRUE;
		}
	}

	*nread = _nread;
	return TRUE;
}

#ifdef MODEST_HAVE_OGS
static gboolean
search_mime_part_ogs (TnyMimePart *part, ModestSearch *search)
{
	TnyStream *stream = NULL;
	char       buffer[4096];
	const gsize len = sizeof (buffer);
	gsize      nread = 0;
	gboolean   is_text_html = FALSE;
	gboolean   found = FALSE;
	gboolean   res = FALSE;

	gboolean is_text = tny_mime_part_content_type_is (part, "text/*");
	if (!is_text) {
		g_debug ("%s: tny_mime_part_content_type_is() failed to find a "
			"text/* MIME part. Content type is %s", 
	    	__FUNCTION__, "Unknown (calling tny_mime_part_get_content_type(part) causes a deadlock)");
	    	
	    /* Retry with specific MIME types, because the wildcard seems to fail
	     * in tinymail.
	     * Actually I'm not sure anymore that it fails, so we could probalby 
	     * remove this later: murrayc */
	    is_text = (
	    	tny_mime_part_content_type_is (part, "text/plain") ||
	    	tny_mime_part_content_type_is (part, "text/html") );
	    	
	   	if (is_text) {
	   	  g_debug ("%s: Retryting with text/plain or text/html succeeded", 
	   	  	__FUNCTION__);	
	   	}
	}
	
	if (!is_text) {
	    return FALSE;
	}
	
	is_text_html = tny_mime_part_content_type_is (part, "text/html");

	stream = tny_mime_part_get_stream (part);

	res = read_chunk (stream, buffer, len, &nread);
	while (res && (nread > 0)) {
		/* search->text_searcher was instantiated in modest_search_folder(). */
		
		if (is_text_html) {

			found = ogs_text_searcher_search_html (search->text_searcher,
							       buffer,
							       nread,
							       nread < len);
		} else {
			found = ogs_text_searcher_search_text (search->text_searcher,
							       buffer,
							       nread);
		}

		if (found) {
			break;
		}
		
		nread = 0;
		res = read_chunk (stream, buffer, len, &nread);
	}

	if (!found) {
		found = ogs_text_searcher_search_done (search->text_searcher);
	}

	ogs_text_searcher_reset (search->text_searcher);
	
	/* debug stuff:
	if (!found) {
		buffer[len -1] = 0;
		printf ("DEBUG: %s: query %s was not found in message text: %s\n", 
			__FUNCTION__, search->query, buffer);	
		
	} else {
		printf ("DEBUG: %s: found.\n", __FUNCTION__);	
	}
	*/

	return found;
}

#else

static gboolean
search_mime_part_strcmp (TnyMimePart *part, ModestSearch *search)
{
	TnyStream *stream;
	char       buffer[8193];
	char      *chunk[2];
	gssize     len;
	gsize     nread;
	gboolean   found;
	gboolean   res;

	if (! tny_mime_part_content_type_is (part, "text/*")) {
		g_debug ("%s: No text MIME part found.\n", __FUNCTION__);
		return FALSE;
	}

	found = FALSE;
	len = (sizeof (buffer) - 1) / 2;

	if (strlen (search->body) > len) {
		g_warning ("Search term bigger then chunk."
			   "We might not find everything");	
	}

	stream = tny_mime_part_get_stream (part);

	memset (buffer, 0, sizeof (buffer));
	chunk[0] = buffer;
	chunk[1] = buffer + len;

	res = read_chunk (stream, chunk[0], len, &nread);

	if (res == FALSE) {
		goto done;
	}

	found = !modest_text_utils_utf8_strcmp (search->body,
						buffer,
						TRUE);
	if (found) {
		goto done;
	}

	/* This works like this:
	 * buffer: [ooooooooooo|xxxxxxxxxxxx|\0] 
	 *          ^chunk[0]  ^chunk[1]
	 * we have prefilled chunk[0] now we always read into chunk[1]
	 * and then move the content of chunk[1] to chunk[0].
	 * The idea is to prevent not finding search terms that are
	 * spread across 2 reads:	 
	 * buffer: [ooooooooTES|Txxxxxxxxxxx|\0] 
	 * We should catch that because we always search the whole
	 * buffer not only the chunks.
	 *
	 * Of course that breaks for search terms > sizeof (chunk)
	 * but sizeof (chunk) should be big enough I guess (see
	 * the g_warning in this function)
	 * */	
	while ((res = read_chunk (stream, chunk[1], len, &nread))) {
		buffer[len + nread] = '\0';

		found = !modest_text_utils_utf8_strcmp (search->body,
							buffer,
							TRUE);

		if (found) {
			break;
		}

		/* also move the \0 */
		g_memmove (chunk[0], chunk[1], len + 1);
	}

done:
	g_object_unref (stream);
	return found;
}
#endif /*MODEST_HAVE_OGS*/

static gboolean
search_string (const char      *what,
	       const char      *where,
	       ModestSearch    *search)
{
	gboolean found;
#ifdef MODEST_HAVE_OGS
	if (search->flags & MODEST_SEARCH_USE_OGS) {
		found = ogs_text_searcher_search_text (search->text_searcher,
					   	       where,
					   	       strlen (where));

		ogs_text_searcher_reset (search->text_searcher);
	} else {
#endif
		if (what == NULL || where == NULL) {
			return FALSE;
		}

		found = !modest_text_utils_utf8_strcmp (what, where, TRUE);
#ifdef MODEST_HAVE_OGS
	}
#endif
	return found;
}


static gboolean search_mime_part_and_child_parts (TnyMimePart *part, ModestSearch *search)
{
	gboolean found = FALSE;
	#ifdef MODEST_HAVE_OGS
	found = search_mime_part_ogs (part, search);
	#else
	found = search_mime_part_strcmp (part, search);
	#endif

	if (found) {	
		return found;		
	}
	
	/* Check the child part too, recursively: */
	TnyList *child_parts = tny_simple_list_new ();
	tny_mime_part_get_parts (TNY_MIME_PART (part), child_parts);

	TnyIterator *piter = tny_list_create_iterator (child_parts);
	while (!found && !tny_iterator_is_done (piter)) {
		TnyMimePart *pcur = (TnyMimePart *) tny_iterator_get_current (piter);
		if (pcur) {
			found = search_mime_part_and_child_parts (pcur, search);

			g_object_unref (pcur);
		}

		tny_iterator_next (piter);
	}

	g_object_unref (piter);
	g_object_unref (child_parts);
	
	return found;
}

/**
 * modest_search:
 * @folder: a #TnyFolder instance
 * @search: a #ModestSearch query
 *
 * This operation will search @folder for headers that match the query @search,
 * if the folder itself matches the query.
 * It will return a doubly linked list with URIs that point to the message.
 **/
GList *
modest_search_folder (TnyFolder *folder, ModestSearch *search)
{
	/* Check that we should be searching this folder. */
	/* Note that we don't try to search sub-folders. 
	 * Maybe we should, but that should be specified. */
	if (search->folder && strlen (search->folder) && (strcmp (tny_folder_get_id (folder), search->folder) != 0))
		return NULL;
	
	GList *retval = NULL;
	TnyIterator *iter = NULL;
	TnyList *list = NULL;
	
#ifdef MODEST_HAVE_OGS
	if (search->flags & MODEST_SEARCH_USE_OGS) {
	
		if (search->text_searcher == NULL && search->query != NULL) {
			OgsTextSearcher *text_searcher;	

			text_searcher = ogs_text_searcher_new (FALSE);
			ogs_text_searcher_parse_query (text_searcher, search->query);
			search->text_searcher = text_searcher;
		}
	}
#endif

	list = tny_simple_list_new ();
	GError *error = NULL;
	tny_folder_get_headers (folder, list, FALSE /* don't refresh */, &error);
	if (error) {
		g_warning ("%s: tny_folder_get_headers() failed with error=%s.\n", 
		__FUNCTION__, error->message);
		g_error_free (error);
		error = NULL;	
	}

	iter = tny_list_create_iterator (list);

	while (!tny_iterator_is_done (iter)) {
		TnyHeader *cur = (TnyHeader *) tny_iterator_get_current (iter);
		const time_t t = tny_header_get_date_sent (cur);
		gboolean found = FALSE;
		
		/* Ignore deleted (not yet expunged) emails: */
		if (tny_header_get_flags(cur) & TNY_HEADER_FLAG_DELETED)
			goto go_next;
			
		if (search->flags & MODEST_SEARCH_BEFORE)
			if (!(t <= search->end_date))
				goto go_next;

		if (search->flags & MODEST_SEARCH_AFTER)
			if (!(t >= search->start_date))
				goto go_next;

		if (search->flags & MODEST_SEARCH_SIZE)
			if (tny_header_get_message_size (cur) < search->minsize)
				goto go_next;

		if (search->flags & MODEST_SEARCH_SUBJECT) {
			const char *str = tny_header_get_subject (cur);

			if ((found = search_string (search->subject, str, search))) {
			    retval = add_hit (retval, cur, folder);
			}
		}
		
		if (!found && search->flags & MODEST_SEARCH_SENDER) {
			char *str = g_strdup (tny_header_get_from (cur));

			if ((found = search_string (search->from, (const gchar *) str, search))) {
				retval = add_hit (retval, cur, folder);
			}
			g_free (str);
		}
		
		if (!found && search->flags & MODEST_SEARCH_RECIPIENT) {
			const char *str = tny_header_get_to (cur);

			if ((found = search_string (search->recipient, str, search))) {
				retval = add_hit (retval, cur, folder);
			}
		}
	
		if (!found && search->flags & MODEST_SEARCH_BODY) {
			TnyHeaderFlags flags;
			GError      *err = NULL;
			TnyMsg      *msg = NULL;

			flags = tny_header_get_flags (cur);

			if (!(flags & TNY_HEADER_FLAG_CACHED)) {
				goto go_next;
			}

			msg = tny_folder_get_msg (folder, cur, &err);

			if (err != NULL || msg == NULL) {
				g_warning ("%s: Could not get message.\n", __FUNCTION__);
				g_error_free (err);

				if (msg) {
					g_object_unref (msg);
				}
			} else {	
			
				found = search_mime_part_and_child_parts (TNY_MIME_PART (msg), 
									  search);
				if (found) {
					retval = add_hit (retval, cur, folder);
				}
			}
			
			if (msg)
				g_object_unref (msg);
		}

go_next:
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (list);
	return retval;
}

GList *
modest_search_account (TnyAccount *account, ModestSearch *search)
{
	TnyFolderStore      *store;
	TnyIterator         *iter;
	TnyList             *folders;
	GList               *hits;
	GError              *error;

	error = NULL;
	hits = NULL;

	store = TNY_FOLDER_STORE (account);

	folders = tny_simple_list_new ();
	tny_folder_store_get_folders (store, folders, NULL, &error);
	
	if (error != NULL) {
		g_object_unref (folders);
		return NULL;
	}

	iter = tny_list_create_iterator (folders);
	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder = NULL;
		GList     *res = NULL;

		folder = TNY_FOLDER (tny_iterator_get_current (iter));
		if (folder) {
			/* g_debug ("DEBUG: %s: searching folder %s.", 
				__FUNCTION__, tny_folder_get_name (folder)); */
		
			res = modest_search_folder (folder, search);

			if (res != NULL) {
				if (hits == NULL) {
					hits = res;
				} else {
					hits = g_list_concat (hits, res);
				}
			}

			g_object_unref (folder);
		}

		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (folders);

	/* printf ("DEBUG: %s: hits length = %d\n", __FUNCTION__, g_list_length (hits)); */
	return hits;
}

GList *
modest_search_all_accounts (ModestSearch *search)
{
	/* printf ("DEBUG: %s: query=%s\n", __FUNCTION__, search->query); */
	ModestTnyAccountStore *astore;
	TnyList               *accounts;
	TnyIterator           *iter;
	GList                 *hits;

	hits = NULL;
	astore = modest_runtime_get_account_store ();

	accounts = tny_simple_list_new ();
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE (astore),
					accounts,
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	iter = tny_list_create_iterator (accounts);
	while (!tny_iterator_is_done (iter)) {
		TnyAccount *account = NULL;
		GList      *res = NULL;

		account = TNY_ACCOUNT (tny_iterator_get_current (iter));
		if (account) {
			/* g_debug ("DEBUG: %s: Searching account %s",
		  	 __FUNCTION__, tny_account_get_name (account)); */
		  	 
			/* Give the account time to go online if necessary, 
			 * for instance if this is immediately after startup,
			 * after D-Bus activation: */
			if (check_and_wait_for_account_is_online (account)) {
				/* Search: */
				res = modest_search_account (account, search);
				
				if (res != NULL) {	
					if (hits == NULL) {
						hits = res;
					} else {
						hits = g_list_concat (hits, res);
					}
				}
			}
			
			g_object_unref (account);
		}

		tny_iterator_next (iter);
	}

	g_object_unref (accounts);
	g_object_unref (iter);

	/* printf ("DEBUG: %s: end: hits length=%d\n", __FUNCTION__, g_list_length(hits)); */
	return hits;
}



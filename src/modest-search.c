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
	
	subject = tny_header_get_subject (header);
	sender = tny_header_get_from (header);

	flags = tny_header_get_flags (header);

	hit->msgid = msg_url;
	hit->subject = g_strdup_or_null (subject);
	hit->sender = g_strdup_or_null (sender);
	hit->folder = furl; /* We just provide our new instance instead of copying it and freeing it. */
	hit->msize = tny_header_get_message_size (header);
	hit->has_attachment = flags & TNY_HEADER_FLAG_ATTACHMENTS;
	hit->is_unread = ! (flags & TNY_HEADER_FLAG_SEEN);
	hit->timestamp = tny_header_get_date_received (header);
	
	return g_list_prepend (list, hit);
}

static gboolean
read_chunk (TnyStream *stream, char *buffer, gsize count, gsize *nread)
{
	gsize _nread;
	gssize res;

	_nread = 0;
	while (_nread < count) {
		res = tny_stream_read (stream,
				       buffer + _nread, 
				       count - _nread);
		if (res == -1) {
			*nread = _nread;
			return FALSE;
		}

		if (res == 0)
			break;

		_nread += res;
	}

	*nread = _nread;
	return TRUE;


}

#ifdef MODEST_HAVE_OGS
static gboolean
search_mime_part_ogs (TnyMimePart *part, ModestSearch *search)
{
	TnyStream *stream;
	char       buffer[4096];
	gsize      len;
	gsize      nread;
	gboolean   is_html = FALSE;
	gboolean   found;
	gboolean   res;


	if (! tny_mime_part_content_type_is (part, "text/ *") ||
	    ! (is_html = tny_mime_part_content_type_is (part, "text/html"))) {
		return FALSE;
	}

	found = FALSE;
	len = sizeof (buffer);
	stream = tny_mime_part_get_stream (part);

	while ((res = read_chunk (stream, buffer, len, &nread))) {

		if (is_html) {

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

	}

	if (!found) {
		found = ogs_text_searcher_search_done (search->text_searcher);
	}

	ogs_text_searcher_reset (search->text_searcher);

	return found;
}
#endif /*MODEST_HAVE_OGS*/

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

	if (! tny_mime_part_content_type_is (part, "text/ *")) {
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



/**
 * modest_search:
 * @folder: a #TnyFolder instance
 * @search: a #ModestSearch query
 *
 * This operation will search @folder for headers that match the query @search.
 * It will return a doubly linked list with URIs that point to the message.
 **/
GList *
modest_search_folder (TnyFolder *folder, ModestSearch *search)
{
	GList *retval = NULL;
	TnyIterator *iter;
	TnyList *list;
	gboolean (*part_search_func) (TnyMimePart *part, ModestSearch *search);

	part_search_func = search_mime_part_strcmp;

#ifdef MODEST_HAVE_OGS
	if (search->flags & MODEST_SEARCH_USE_OGS) {
	
		if (search->text_searcher == NULL && search->query != NULL) {
			OgsTextSearcher *text_searcher;	

			text_searcher = ogs_text_searcher_new (FALSE);
			ogs_text_searcher_parse_query (text_searcher, search->query);
			search->text_searcher = text_searcher;
		}

		part_search_func = search_mime_part_ogs;
	}
#endif

	list = tny_simple_list_new ();
	tny_folder_get_headers (folder, list, FALSE, NULL);

	iter = tny_list_create_iterator (list);

	while (!tny_iterator_is_done (iter)) {
		TnyHeader *cur = (TnyHeader *) tny_iterator_get_current (iter);
		time_t t = tny_header_get_date_sent (cur);
		gboolean found = FALSE;
		
		if (search->flags & MODEST_SEARCH_BEFORE)
			if (!(t <= search->before))
				goto go_next;

		if (search->flags & MODEST_SEARCH_AFTER)
			if (!(t >= search->after))
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
			TnyIterator *piter;
			TnyList     *parts;

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
			}	

			parts = tny_simple_list_new ();
			tny_mime_part_get_parts (TNY_MIME_PART (msg), parts);

			piter = tny_list_create_iterator (parts);
			while (!found && !tny_iterator_is_done (piter)) {
				TnyMimePart *pcur = (TnyMimePart *) tny_iterator_get_current (piter);

				if ((found = part_search_func (pcur, search))) {
					retval = add_hit (retval, cur, folder);				
				}

				g_object_unref (pcur);
				tny_iterator_next (piter);
			}

			g_object_unref (piter);
			g_object_unref (parts);
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
		TnyFolder *folder;
		GList     *res;

		folder = TNY_FOLDER (tny_iterator_get_current (iter));
		
		res = modest_search_folder (folder, search);

		if (res != NULL) {
			if (hits == NULL) {
				hits = res;
			} else {
				hits = g_list_concat (hits, res);
			}
		}

		g_object_unref (folder);
		tny_iterator_next (iter);
	}

	g_object_unref (iter);
	g_object_unref (folders);

	return hits;
}

GList *
modest_search_all_accounts (ModestSearch *search)
{
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
		TnyAccount *account;
		GList      *res;

		account = TNY_ACCOUNT (tny_iterator_get_current (iter));

		g_debug ("DEBUG: %s: Searching account %s",
			 __FUNCTION__, tny_account_get_name (account));
		res = modest_search_account (account, search);
		
		if (res != NULL) {

			if (hits == NULL) {
				hits = res;
			} else {
				hits = g_list_concat (hits, res);
			}
		}

		g_object_unref (account);
		tny_iterator_next (iter);
	}

	g_object_unref (accounts);
	g_object_unref (iter);

	return hits;
}



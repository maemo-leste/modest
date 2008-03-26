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
#include <tny-camel-imap-store-account.h>
#include <tny-camel-pop-store-account.h>

#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-tny-account-store.h"
#include "modest-tny-account.h"
#include "modest-tny-mime-part.h"
#include "modest-tny-folder.h"
#include "modest-search.h"
#include "modest-runtime.h"
#include "modest-platform.h"

typedef struct 
{
	guint folder_count;
	guint folder_total;
	GList *msg_hits;
	ModestSearch *search;
	ModestSearchCallback callback;
	gpointer user_data;
} SearchHelper;

static SearchHelper *create_helper (ModestSearchCallback callback, 
				    ModestSearch *search,
				    gpointer user_data);

static void          check_search_finished (SearchHelper *helper);

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
	ModestSearchResultHit *hit;
	TnyHeaderFlags   flags;
	char            *furl;
	char            *msg_url;
	char      *uid;
	char      *subject;
	char      *sender;

	hit = g_slice_new0 (ModestSearchResultHit);

	furl = tny_folder_get_url_string (folder);
	g_debug ("%s: folder URL=%s\n", __FUNCTION__, furl);
	if (!furl) {
		g_warning ("%s: tny_folder_get_url_string(): returned NULL for folder. Folder name=%s\n", __FUNCTION__, tny_folder_get_name (folder));
	}
	
	/* Make sure that we use the short UID instead of the long UID,
	 * and/or find out what UID form is used when finding, in camel_data_cache_get().
	 * so we can find what we get. Philip is working on this.
	 */
	uid = tny_header_dup_uid (header);
	if (!furl) {
		gchar *subject = tny_header_dup_subject (header);
		g_warning ("%s: tny_header_get_uid(): returned NULL for message with subject=%s\n", __FUNCTION__, subject);
		g_free (subject);
	}
	
	msg_url = g_strdup_printf ("%s/%s", furl, uid);
	g_free (furl);
	g_free (uid);
	
	subject = tny_header_dup_subject (header);
	sender = tny_header_dup_from (header);
	
	flags = tny_header_get_flags (header);

	hit->msgid = msg_url;
	hit->subject = subject;
	hit->sender = sender;
	hit->folder = g_strdup_or_null (tny_folder_get_name (folder));
	hit->msize = tny_header_get_message_size (header);
	hit->has_attachment = flags & TNY_HEADER_FLAG_ATTACHMENTS;
	hit->is_unread = ! (flags & TNY_HEADER_FLAG_SEEN);
	hit->timestamp = MIN (tny_header_get_date_received (header), tny_header_get_date_sent (header));
	
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
/*
 * This function assumes that the mime part is of type "text / *"
 */
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

		/* HACK: this helps UI refreshes because the search
		   operations could be heavy */
		while (gtk_events_pending ())
			gtk_main_iteration ();

		if (found) {
			break;
		}
		
		nread = 0;
		res = read_chunk (stream, buffer, len, &nread);
	}
	g_object_unref (stream);

	if (!found) {
		found = ogs_text_searcher_search_done (search->text_searcher);
	}

	ogs_text_searcher_reset (search->text_searcher);

	return found;
}

#else

/*
 * This function assumes that the mime part is of type "text / *"
 */
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

		/* HACK: this helps UI refreshes because the search
		   operations could be heavy */
		while (gtk_events_pending ())
			gtk_main_iteration ();

		if ((found)||(nread == 0)) {
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
	gboolean found = FALSE;
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

	/* HACK: this helps UI refreshes because the search
	   operations could be heavy */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	return found;
}


static gboolean 
search_mime_part_and_child_parts (TnyMimePart *part, ModestSearch *search)
{
	gboolean found = FALSE;

	/* Do not search into attachments */
	if (modest_tny_mime_part_is_attachment_for_modest (part))
		return FALSE;

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

static void 
modest_search_folder_get_headers_cb (TnyFolder *folder, 
				     gboolean cancelled, 
				     TnyList *headers, 
				     GError *err, 
				     gpointer user_data)
{
	TnyIterator *iter = NULL;
	SearchHelper *helper;

	helper = (SearchHelper *) user_data;

	if (err || cancelled) {
		goto end;
	}

	iter = tny_list_create_iterator (headers);

	while (!tny_iterator_is_done (iter)) {

		TnyHeader *cur = (TnyHeader *) tny_iterator_get_current (iter);
		const time_t t = tny_header_get_date_sent (cur);
		gboolean found = FALSE;
		
		/* Ignore deleted (not yet expunged) emails: */
		if (tny_header_get_flags(cur) & TNY_HEADER_FLAG_DELETED)
			goto go_next;
			
		if (helper->search->flags & MODEST_SEARCH_BEFORE)
			if (!(t <= helper->search->end_date))
				goto go_next;

		if (helper->search->flags & MODEST_SEARCH_AFTER)
			if (!(t >= helper->search->start_date))
				goto go_next;

		if (helper->search->flags & MODEST_SEARCH_SIZE)
			if (tny_header_get_message_size (cur) < helper->search->minsize)
				goto go_next;

		if (helper->search->flags & MODEST_SEARCH_SUBJECT) {
			char *str = tny_header_dup_subject (cur);

			if ((found = search_string (helper->search->subject, str, helper->search))) {
			    helper->msg_hits = add_hit (helper->msg_hits, cur, folder);
			}
			g_free (str);
		}
		
		if (!found && helper->search->flags & MODEST_SEARCH_SENDER) {
			char *str = tny_header_dup_from (cur);

			if ((found = search_string (helper->search->from, (const gchar *) str, helper->search))) {
				helper->msg_hits = add_hit (helper->msg_hits, cur, folder);
			}
			g_free (str);
		}
		
		if (!found && helper->search->flags & MODEST_SEARCH_RECIPIENT) {
			char *str = tny_header_dup_to (cur);

			if ((found = search_string (helper->search->recipient, str, helper->search))) {
				helper->msg_hits = add_hit (helper->msg_hits, cur, folder);
			}
			g_free (str);
		}
	
		if (!found && helper->search->flags & MODEST_SEARCH_BODY) {
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
				gchar *str;
				str = tny_header_dup_subject (cur);
				g_debug ("Searching in %s\n", str);
				g_free (str);
			
				found = search_mime_part_and_child_parts (TNY_MIME_PART (msg),
									  helper->search);
				if (found) {
					helper->msg_hits = add_hit (helper->msg_hits, cur, folder);
				}
			}
			
			if (msg)
				g_object_unref (msg);
		}
	go_next:
		g_object_unref (cur);
		tny_iterator_next (iter);
	}

	/* Frees */
	g_object_unref (iter);
 end:
	if (headers)
		g_object_unref (headers);

	/* Check search finished */
	helper->folder_count++;
	check_search_finished (helper);
}

static void
_search_folder (TnyFolder *folder, 
		SearchHelper *helper)
{
	TnyList *list = NULL;

	g_debug ("%s: searching folder %s.", __FUNCTION__, tny_folder_get_name (folder));
	
	/* Check that we should be searching this folder. */
	/* Note that we don't try to search sub-folders. 
	 * Maybe we should, but that should be specified. */
	if (helper->search->folder && strlen (helper->search->folder)) {
		if (!strcmp (helper->search->folder, "outbox")) {
			if (modest_tny_folder_guess_folder_type (folder) != TNY_FOLDER_TYPE_OUTBOX) {
				modest_search_folder_get_headers_cb (folder, TRUE, NULL, NULL, helper); 
				return;
			}
		} else if (strcmp (tny_folder_get_id (folder), helper->search->folder) != 0) {
			modest_search_folder_get_headers_cb (folder, TRUE, NULL, NULL, helper); 
			return;
		}
	}
	
#ifdef MODEST_HAVE_OGS
	if (helper->search->flags & MODEST_SEARCH_USE_OGS) {
	
		if (helper->search->text_searcher == NULL && helper->search->query != NULL) {
			OgsTextSearcher *text_searcher;	

			text_searcher = ogs_text_searcher_new (FALSE);
			ogs_text_searcher_parse_query (text_searcher, helper->search->query);
			helper->search->text_searcher = text_searcher;
		}
	}
#endif
	list = tny_simple_list_new ();
	/* Get the headers */
	tny_folder_get_headers_async (folder, list, FALSE, 
				      modest_search_folder_get_headers_cb, 
				      NULL, helper);
}

void
modest_search_folder (TnyFolder *folder, 
		      ModestSearch *search,
		      ModestSearchCallback callback,
		      gpointer user_data)
{
	SearchHelper *helper;

	/* Create the helper */
	helper = create_helper (callback, search, user_data);

	/* Search */
	_search_folder (folder, helper);
}

static void
modest_search_account_get_folders_cb (TnyFolderStore *self, 
				      gboolean cancelled, 
				      TnyList *folders, 
				      GError *err, 
				      gpointer user_data)
{
	TnyIterator *iter;
	SearchHelper *helper;

	helper = (SearchHelper *) user_data;

	if (err || cancelled) {
		goto end;
	}

	iter = tny_list_create_iterator (folders);
	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder = NULL;

		/* Search into folder */
		folder = TNY_FOLDER (tny_iterator_get_current (iter));	
		helper->folder_total++;
		_search_folder (folder, (SearchHelper *) user_data);
		g_object_unref (folder);

		tny_iterator_next (iter);
	}
	g_object_unref (iter);
 end:
	if (folders)
		g_object_unref (folders);

	/* Check search finished */
	check_search_finished (helper);
}

static void
_search_account (TnyAccount *account, 
		 SearchHelper *helper)
{
	TnyList *folders = tny_simple_list_new ();

	g_debug ("%s: Searching account %s", __FUNCTION__, tny_account_get_name (account));

	/* Get folders */
	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (account), folders, NULL, 
					    modest_search_account_get_folders_cb, 
					    NULL, helper);
}

void
modest_search_account (TnyAccount *account, 
		       ModestSearch *search,
		       ModestSearchCallback callback,
		       gpointer user_data)
{
	SearchHelper *helper;

	/* Create the helper */
	helper = create_helper (callback, search, user_data);

	/* Search */
	_search_account (account, helper);
}

void
modest_search_all_accounts (ModestSearch *search,
			    ModestSearchCallback callback,
			    gpointer user_data)
{
	ModestTnyAccountStore *astore;
	TnyList *accounts;
	TnyIterator *iter;
	GList *hits;
	SearchHelper *helper;

	hits = NULL;
	astore = modest_runtime_get_account_store ();

	accounts = tny_simple_list_new ();
	tny_account_store_get_accounts (TNY_ACCOUNT_STORE (astore),
					accounts,
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	/* Create the helper */
	helper = create_helper (callback, search, user_data);

	/* Search through all accounts */
	iter = tny_list_create_iterator (accounts);
	while (!tny_iterator_is_done (iter)) {
		TnyAccount *account = NULL;

		account = TNY_ACCOUNT (tny_iterator_get_current (iter));
		_search_account (account, helper);
		g_object_unref (account);

		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (accounts);
}

static SearchHelper *
create_helper (ModestSearchCallback callback, 
	       ModestSearch *search,
	       gpointer user_data)
{
	SearchHelper *helper;

	helper = g_slice_new0 (SearchHelper);
	helper->folder_count = 0;
	helper->folder_total = 0;
	helper->search = search;
	helper->callback = callback;
	helper->user_data = user_data;
	helper->msg_hits = NULL;

	return helper;
}

void 
modest_search_free (ModestSearch *search)
{
	if (search->folder)
		g_free (search->folder);
	if (search->subject)
		g_free (search->subject);
	if (search->from)
		g_free (search->from);
	if (search->recipient)
		g_free (search->recipient);
	if (search->body)
		g_free (search->body);

#ifdef MODEST_HAVE_OGS
	if (search->query)
		g_free (search->query);
	if (search->text_searcher)
		ogs_text_searcher_free (search->text_searcher);	
#endif
}

static void
check_search_finished (SearchHelper *helper)
{
	/* If there are no more folders to check the account search has finished */
	if (helper->folder_count == helper->folder_total) {
		/* callback */
		helper->callback (helper->msg_hits, helper->user_data);
		
		/* free helper */
		g_list_free (helper->msg_hits);
		g_slice_free (SearchHelper, helper);
	}
}

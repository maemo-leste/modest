#define _GNU_SOURCE

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

#include "modest-text-utils.h"
#include "modest-account-mgr.h"
#include "modest-tny-account-store.h"
#include "modest-tny-account.h"
#include "modest-search.h"
#include "modest-runtime.h"

static GList*
add_header (GList *list, TnyHeader *header, TnyFolder *folder)
{
	TnyFolder *f;
       
	/* TODO: we need this call otherwise it will crash later
	 * when we try to do that call again without having the
	 * folder around, I guess that is a bug in TinyThingy */
	f = tny_header_get_folder (header);

	return g_list_prepend (list, g_object_ref (header));
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
#endif

static gboolean
search_mime_part_strcmp (TnyMimePart *part, ModestSearch *search)
{
	TnyStream *stream;
	char       buffer[8193];
	char      *chunk[2];
	gsize      len;
	gssize     nread;
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
			    retval = add_header (retval, cur, folder);
			}
		}
		
		if (!found && search->flags & MODEST_SEARCH_SENDER) {
			const char *str = tny_header_get_from (cur);

			if ((found = search_string (search->from, str, search))) {
				retval = add_header (retval, cur, folder);
			}
		}
		
		if (!found && search->flags & MODEST_SEARCH_RECIPIENT) {
			const char *str = tny_header_get_to (cur);

			if ((found = search_string (search->recipient, str, search))) {
				retval = add_header (retval, cur, folder);
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
				g_warning ("Could not get message\n");
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
					retval = add_header (retval, cur, folder);				
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
	ModestAccountMgr      *account_mgr;
	ModestTnyAccountStore *astore;
	GSList                *accounts;
	GSList                *iter;
	GList                 *hits;

	account_mgr = modest_runtime_get_account_mgr ();

	accounts = modest_account_mgr_account_names (account_mgr, FALSE);
	astore = modest_runtime_get_account_store ();
	hits = NULL;

	for (iter = accounts; iter; iter = iter->next) {
		GList      *res;
		const char *ac_name;
		TnyAccount *account = NULL;

		ac_name = (const char *) iter->data;

		account = modest_tny_account_store_get_tny_account_by_account (astore,
									       ac_name,
									       TNY_ACCOUNT_TYPE_STORE);

		if (account == NULL) {
			g_warning ("Could not get account for %s", ac_name);
			continue;
		}
		
		res = modest_search_account (account, search);
		
		if (res != NULL) {

			if (hits == NULL) {
				hits = res;
			} else {
				hits = g_list_concat (hits, res);
			}
		}
	}

	return hits;
}



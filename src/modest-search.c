#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <tny-shared.h>
#include <tny-folder.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>

#include "modest-text-utils.h"

#include "modest-search.h"


static GList*
add_header (GList *list, TnyHeader *header, TnyFolder *folder)
{
	gchar *furl = tny_folder_get_url_string (folder);
	const gchar *uid = tny_header_get_uid (header);
	gchar *str  = g_strdup_printf ("%s/%s", furl, uid);
	g_free (furl);
	return g_list_prepend (list, str);
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
modest_search (TnyFolder *folder, ModestSearch *search)
{
	GList *retval = NULL;
	TnyIterator *iter;
	TnyList *list;
	gboolean (*part_search_func) (TnyMimePart *part, ModestSearch *search);

	part_search_func = search_mime_part_strcmp;

#ifdef MODEST_HAVE_OGS
	if (search->flags & MODEST_SEARCH_USE_OGS &&
	    search->text_searcher == NULL && search->query != NULL) {
		OgsTextSearcher *text_searcher;	

		text_searcher = ogs_text_searcher_new (FALSE);
		ogs_text_searcher_parse_query (text_searcher, search->query);
		search->text_searcher = text_searcher;

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
			if (tny_header_get_message_size  (cur) < search->minsize)
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


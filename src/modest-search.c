#define _GNU_SOURCE

#include <string.h>

#include <tny-shared.h>
#include <tny-folder.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>

#include "modest-search.h"
#include "modest-mime-part-search-stream.h"

static GList*
add_header (GList *list, TnyHeader *header, TnyFolder *folder)
{
	gchar *furl = tny_folder_get_url_string (folder);
	const gchar *uid = tny_header_get_uid (header);
	gchar *str  = g_strdup_printf ("%s/%s", furl, uid);
	g_free (furl);
	return g_list_prepend (list, str);
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
	TnyList *list = tny_simple_list_new ();
	tny_folder_get_headers (folder, list, FALSE, NULL);

	iter = tny_list_create_iterator (list);
	while (!tny_iterator_is_done (iter))
	{
		TnyHeader *cur = (TnyHeader *) tny_iterator_get_current (iter);
		time_t t = tny_header_get_date_sent (cur);

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
			if (strcasestr (tny_header_get_subject (cur), search->subject))
				retval = add_header (retval, cur, folder);
		} else if (search->flags & MODEST_SEARCH_SENDER) {
			if (strcasestr (tny_header_get_from (cur), search->subject))
				retval = add_header (retval, cur, folder);
		} else if (search->flags & MODEST_SEARCH_RECIPIENT) {
			if (strcasestr (tny_header_get_to (cur), search->subject))
				retval = add_header (retval, cur, folder);
		} else if (search->flags & MODEST_SEARCH_BODY) 
		{
			TnyHeaderFlags flags = tny_header_get_flags (cur);
		       	
			if (flags & TNY_HEADER_FLAG_CACHED)
			{
				GError *err = NULL;
				TnyMsg *msg = tny_folder_get_msg (folder, cur, &err);
				
				if (err == NULL)
				{
					TnyIterator *piter;
					TnyList *parts = tny_simple_list_new ();
					tny_mime_part_get_parts  (TNY_MIME_PART (msg), parts);
					piter = tny_list_create_iterator (parts);

					while (!tny_iterator_is_done (piter))
					{
						TnyStream *stream;
						TnyMimePart *pcur = (TnyMimePart *) tny_iterator_get_current (piter);

						stream = modest_mime_part_search_stream_new (search->body);

						tny_mime_part_decode_to_stream (pcur, stream);

						if (((ModestMimePartSearchStream *) stream)->found)
							retval = add_header (retval, cur, folder);				

						g_object_unref (stream);
						g_object_unref (pcur);
						tny_iterator_next (piter);
					}
					g_object_unref (piter);
					g_object_unref (parts);
				} else
					g_error_free (err);
				
				if (msg)
					g_object_unref (msg);
			}
		}

			

go_next:
		g_object_unref (cur);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (list);
	
	return retval;
}


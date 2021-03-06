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

#include "modest-pair.h"
#include <string.h> /* For strcmp() */
#include <stdio.h>

ModestPair*
modest_pair_new     (gpointer first, gpointer second, gboolean own)
{
	ModestPair *pair;

	pair = g_slice_new (ModestPair);

	pair->first  = first;
	pair->second = second;
	pair->own    = own;
	
	return pair;
}


void
modest_pair_free (ModestPair *pair)
{
	if (!pair)
		return;

	if (pair->own) {
		g_free (pair->first);
		g_free (pair->second);
	}
	
	g_slice_free (ModestPair, pair);
}



void
modest_pair_list_free (ModestPairList *pairs)
{
	ModestPairList *cursor = pairs;
	while (cursor) {
		modest_pair_free ((ModestPair*)cursor->data);
		cursor = cursor->next;
	}
	g_slist_free (pairs);
}

static gint on_pair_compare_as_string(gconstpointer a, gconstpointer b)
{
	const ModestPair* pair_a = (const ModestPair*)a;
  const gchar* target = (const gchar*)b;
  
	return strcmp ((const gchar*)pair_a->first, target);
}

ModestPair* modest_pair_list_find_by_first_as_string  (ModestPairList *pairs, 
	const gchar* first)
{
	GSList *matching = g_slist_find_custom (pairs, (gconstpointer)first, 
		on_pair_compare_as_string);
	if (matching)
		return (ModestPair*)matching->data;
	else
		return NULL;
}

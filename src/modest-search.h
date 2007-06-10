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

#ifndef MODEST_SEARCH_H
#define MODEST_SEARCH_H

#include <glib.h>
#include <tny-folder.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef MODEST_HAVE_OGS
#include <libogs/ogs-text-searcher.h>
#endif

G_BEGIN_DECLS

typedef enum {
	MODEST_SEARCH_SUBJECT   = (1 << 0),
	MODEST_SEARCH_SENDER    = (1 << 1),
	MODEST_SEARCH_RECIPIENT = (1 << 2),
	MODEST_SEARCH_SIZE 	= (1 << 3),
	MODEST_SEARCH_BEFORE    = (1 << 4),
	MODEST_SEARCH_AFTER     = (1 << 5),
	MODEST_SEARCH_BODY      = (1 << 6),
	MODEST_SEARCH_USE_OGS   = (1 << 7),
} ModestSearchFlags;

typedef struct {
	gchar *subject, *from, *recipient, *body;
	time_t before, after;
	guint32 minsize;
	ModestSearchFlags flags;
#ifdef MODEST_HAVE_OGS
	const gchar     *query;
	OgsTextSearcher *text_searcher;	
#endif
} ModestSearch;

GList * modest_search_folder (TnyFolder *folder, ModestSearch *search);
GList * modest_search_all_accounts (ModestSearch *search);
GList * modest_search_account (TnyAccount *account, ModestSearch *search);
G_END_DECLS

#endif


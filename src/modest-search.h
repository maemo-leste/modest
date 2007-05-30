#ifndef MODEST_SEARCH_H
#define MODEST_SEARCH_H

#include <glib.h>
#include <tny-folder.h>

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
	guint minsize;
	ModestSearchFlags flags;
#ifdef MODEST_HAVE_OGS
	const gchar     *query;
	OgsTextSearcher *text_searcher;	
#endif
} ModestSearch;

GList * modest_search (TnyFolder *folder, ModestSearch *search);

G_END_DECLS

#endif


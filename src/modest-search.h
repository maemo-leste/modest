#ifndef MODEST_SEARCH_H
#define MODEST_SEARCH_H

typedef enum {
	MODEST_SEARCH_SUBJECT = 0<<1,
	MODEST_SEARCH_SENDER = 0<<2,
	MODEST_SEARCH_RECIPIENT = 0<<3,
	MODEST_SEARCH_SIZE = 0<<4,
	MODEST_SEARCH_BEFORE = 0<<5,
	MODEST_SEARCH_AFTER = 0<<6,
	MODEST_SEARCH_BODY = 0<<7
} ModestSearchFlags;

typedef struct {
	gchar *subject, *from, *recipient, *body;
	time_t before, after;
	guint minsize;
	ModestSearchFlags flags;
} ModestSearch;

GList * modest_search (TnyFolder *folder, ModestSearch *search);

#endif


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

#include <config.h>
#include <modest-cache-mgr.h>
#include <stdio.h>

/* 'private'/'protected' functions */
static void modest_cache_mgr_class_init (ModestCacheMgrClass *klass);
static void modest_cache_mgr_init       (ModestCacheMgr *obj);
static void modest_cache_mgr_finalize   (GObject *obj);
/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestCacheMgrPrivate ModestCacheMgrPrivate;
struct _ModestCacheMgrPrivate {
	GHashTable *date_str_cache;
	GHashTable *display_str_cache;
	GHashTable *pixbuf_cache;
	GHashTable *send_queue_cache;
};
#define MODEST_CACHE_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                              MODEST_TYPE_CACHE_MGR, \
                                              ModestCacheMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_cache_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestCacheMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_cache_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestCacheMgr),
			0,		/* n_preallocs */
			(GInstanceInitFunc) modest_cache_mgr_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestCacheMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_cache_mgr_class_init (ModestCacheMgrClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_cache_mgr_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestCacheMgrPrivate));
}


static
void my_object_unref (GObject *obj)
{
	if (obj)
		g_object_unref (obj);
}

static void
modest_cache_mgr_init (ModestCacheMgr *obj)
{
 	ModestCacheMgrPrivate *priv;

 	priv = MODEST_CACHE_MGR_GET_PRIVATE(obj);
	
	priv->date_str_cache =
		g_hash_table_new_full (g_int_hash,  /* time_t */
				       g_int_equal,
				       NULL,        /* int -> no need to free */
				       g_free);     /* gchar* */
	priv->display_str_cache =
		g_hash_table_new_full (g_str_hash,  /* gchar* */
				       g_str_equal,
				       g_free,      /* gchar* */
				       g_free);     /* gchar* */
	priv->pixbuf_cache =
		g_hash_table_new_full (g_str_hash,   /* gchar* */
				       g_str_equal,  
				       g_free,       /* gchar*/
				       (GDestroyNotify)my_object_unref);
	priv->send_queue_cache =
		g_hash_table_new_full (g_direct_hash,   /* ptr */
				       g_direct_equal,  
				       (GDestroyNotify)my_object_unref,   /* ref'd GObject */
				       (GDestroyNotify)my_object_unref);   /* ref'd GObject */  
}


static void
modest_cache_mgr_finalize (GObject *obj)
{
	ModestCacheMgr *self;
	self = MODEST_CACHE_MGR(obj);
	
	ModestCacheMgrPrivate *priv;
 	priv = MODEST_CACHE_MGR_GET_PRIVATE(obj);
	
	modest_cache_mgr_flush_all (self);

	priv->date_str_cache    = NULL;
	priv->display_str_cache = NULL;
	priv->pixbuf_cache      = NULL;
	priv->send_queue_cache  = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static GHashTable*
get_cache (ModestCacheMgrPrivate *priv, ModestCacheMgrCacheType type)
{
	switch (type) {
	case MODEST_CACHE_MGR_CACHE_TYPE_DATE_STRING:
		return priv->date_str_cache;
	case MODEST_CACHE_MGR_CACHE_TYPE_DISPLAY_STRING:
		return priv->display_str_cache;
	case MODEST_CACHE_MGR_CACHE_TYPE_PIXBUF:
		return priv->pixbuf_cache;
	case MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE:
		return priv->send_queue_cache;	
	default:
		g_return_val_if_reached(NULL); /* should not happen */
	}
}


ModestCacheMgr*
modest_cache_mgr_new (void)
{
	return MODEST_CACHE_MGR(g_object_new(MODEST_TYPE_CACHE_MGR, NULL));
}


GHashTable*
modest_cache_mgr_get_cache   (ModestCacheMgr* self, ModestCacheMgrCacheType type)
{
	ModestCacheMgrPrivate *priv;
	GHashTable *cache;
	
	g_return_val_if_fail (self, NULL);
	
	if (!(type >= 0 && type <= MODEST_CACHE_MGR_CACHE_TYPE_NUM)) {
		printf ("DEBUG: %s: incorrect type = %d\n", __FUNCTION__, type);	
	}
	
	g_return_val_if_fail (type >= 0 && type <= MODEST_CACHE_MGR_CACHE_TYPE_NUM, NULL);

	priv  = MODEST_CACHE_MGR_GET_PRIVATE(self);
	
	cache = get_cache (priv, type);
	return cache;
}


void
modest_cache_mgr_flush (ModestCacheMgr *self, ModestCacheMgrCacheType type)
{
	ModestCacheMgrPrivate *priv;
	GHashTable *cache;
	
	g_return_if_fail (self);
	g_return_if_fail (type >= 0 && type <= MODEST_CACHE_MGR_CACHE_TYPE_NUM);
	
	priv  = MODEST_CACHE_MGR_GET_PRIVATE(self);

	cache = get_cache (priv, type);
	if (cache)
		g_hash_table_destroy (cache);
}


void
modest_cache_mgr_flush_all (ModestCacheMgr *self)
{
	int i;
	g_return_if_fail (self);
	
	for (i = 0; i != MODEST_CACHE_MGR_CACHE_TYPE_NUM; ++i)
		modest_cache_mgr_flush (self, i);	
}


guint
modest_cache_mgr_get_size (ModestCacheMgr *self, ModestCacheMgrCacheType type)
{
	ModestCacheMgrPrivate *priv;
	GHashTable *cache;
	
	priv = MODEST_CACHE_MGR_GET_PRIVATE(self);

	g_return_val_if_fail (self, 0);
	g_return_val_if_fail (type >= 0 && type <= MODEST_CACHE_MGR_CACHE_TYPE_NUM, 0);

	cache = get_cache (priv, type);
	return cache ? g_hash_table_size (cache) : 0;
}

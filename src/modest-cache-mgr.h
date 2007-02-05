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


#ifndef __MODEST_CACHE_MGR_H__
#define __MODEST_CACHE_MGR_H__

#include <glib-object.h>
G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_CACHE_MGR         (modest_cache_mgr_get_type())
#define MODEST_CACHE_MGR(obj)         (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_CACHE_MGR,ModestCacheMgr))
#define MODEST_CACHE_MGR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_CACHE_MGR,GObject))
#define MODEST_IS_CACHE_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_CACHE_MGR))
#define MODEST_IS_CACHE_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_CACHE_MGR))
#define MODEST_CACHE_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_CACHE_MGR,ModestCacheMgrClass))

typedef struct _ModestCacheMgr      ModestCacheMgr;
typedef struct _ModestCacheMgrClass ModestCacheMgrClass;

struct _ModestCacheMgr {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestCacheMgrClass {
	GObjectClass parent_class;
};

/*
 * the caches managed by this class
 */
typedef enum {
	MODEST_CACHE_MGR_CACHE_TYPE_DATE_STRING,       /* time_t => string */
	MODEST_CACHE_MGR_CACHE_TYPE_DISPLAY_STRING,    /* gchar* => gchar* */
	MODEST_CACHE_MGR_CACHE_TYPE_PIXBUF,            /* gchar* => GdkPixbuf */
	MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE,        /* TnyAccount* => TnySendQueue* */

	MODEST_CACHE_MGR_CACHE_TYPE_NUM
} ModestCacheMgrCacheType;


/**
 * modest_cache_mgr_get_type:
 * 
 * get the GType for ModestCacheMgr
 *  
 * Returns: the GType
 */
GType        modest_cache_mgr_get_type    (void) G_GNUC_CONST;


/**
 * modest_cache_mgr_new:
 *
 * instantiate a new cache_mgr object
 * 
 * Returns: a new cache_mgr or NULL in case of error
 */
ModestCacheMgr*    modest_cache_mgr_new          (void);

/**
 * modest_cache_mgr_get_cache:
 * @self: a valid cache mgr obj
 * @type: a valid cache mgr cache type
 * 
 * get the cache (GHashTable) of the requested type
 * 
 * Returns: the requested cache (GHashTable) 
 * 
 * the returned  hashtable should NOT be destroyed or unref'd.
 */
GHashTable*     modest_cache_mgr_get_cache    (ModestCacheMgr* self, ModestCacheMgrCacheType type);


/**
 * modest_cache_mgr_flush
 * @self: a valid cache mgr obj
 * @type: a valid cache mgr cache type
 *
 * flush the cache (hashtable) of the given type  
 */
void            modest_cache_mgr_flush        (ModestCacheMgr *self, ModestCacheMgrCacheType type);


/**
 * modest_cache_mgr_flush
 * @self: a valid cache mgr obj
 *
 * flush all caches  
 */
void            modest_cache_mgr_flush_all    (ModestCacheMgr *self);


/**
 * modest_cache_mgr_get_size
 * @self: a valid cache mgr obj
 * @type: a valid cache mgr cache type
 *
 * get the size (number of <key,value>-pairs) in the cache
 * 
 * Returns: the size of the give cache type 
 */
guint           modest_cache_mgr_get_size     (ModestCacheMgr *self, ModestCacheMgrCacheType type);

G_END_DECLS

#endif /* __MODEST_CACHE_MGR_H__ */


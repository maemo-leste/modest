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

/* modest-tny-platform-factory.h */

#ifndef __MODEST_TNY_PLATFORM_FACTORY_H__
#define __MODEST_TNY_PLATFORM_FACTORY_H__

#include <glib-object.h>

#include <tny-platform-factory.h>
#include <modest-account-mgr.h>
#include <modest-conf.h>
#include <modest-mail-operation-queue.h>
#include <modest-cache-mgr.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_PLATFORM_FACTORY             (modest_tny_platform_factory_get_type())
#define MODEST_TNY_PLATFORM_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_PLATFORM_FACTORY,ModestTnyPlatformFactory))
#define MODEST_TNY_PLATFORM_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_PLATFORM_FACTORY,GObject))
#define MODEST_IS_TNY_PLATFORM_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_PLATFORM_FACTORY))
#define MODEST_IS_TNY_PLATFORM_FACTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_PLATFORM_FACTORY))
#define MODEST_TNY_PLATFORM_FACTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_PLATFORM_FACTORY,ModestTnyPlatformFactoryClass))

typedef struct _ModestTnyPlatformFactory      ModestTnyPlatformFactory;
typedef struct _ModestTnyPlatformFactoryClass ModestTnyPlatformFactoryClass;

struct _ModestTnyPlatformFactory {
	 GObject parent;
};

struct _ModestTnyPlatformFactoryClass {
	GObjectClass parent_class;
};

/* member functions */
GType        modest_tny_platform_factory_get_type    (void) G_GNUC_CONST;

/**
 * modest_tny_platform_factory_get_instance:
 * 
 * Gets a new instance of the platform factory if it is the first call
 * to the function, or the current one otherwise. This object is
 * supposed to be a singleton
 * 
 * Returns: the instance of a #ModestTnyPlatformFactory
 **/
TnyPlatformFactory*    modest_tny_platform_factory_get_instance         (void);

/**
 * modest_tny_platform_factory_get_account_mgr_instance:
 * @fact: the #TnyPlatformFactory that holds the #ModestAccountMgr instance
 * 
 * Gets a new instance of a #ModestAccountMgr if it is the first call
 * to the function, or the current instantiated one otherwise. This
 * object is supposed to be a singleton
 * 
 * Returns: an instance of a #ModestAccountMgr
 **/
ModestAccountMgr*  modest_tny_platform_factory_get_account_mgr_instance (ModestTnyPlatformFactory *fact);

/**
 * modest_tny_platform_factory_get_conf_instance:
 * @fact: the #TnyPlatformFactory that holds the #ModestConf instance
 * 
 * Gets a new instance of a #ModestConf if it is the first call to the
 * function, or the current instantiated one otherwise. This object is
 * supposed to be a singleton
 * 
 * Returns: an instance of a #ModestConf
 **/
ModestConf*     modest_tny_platform_factory_get_conf_instance (ModestTnyPlatformFactory *fact);

/**
 * modest_tny_platform_factory_get_mail_operation_queue_instance:
 * @fact: the #TnyPlatformFactory that holds the #ModestMailOperationQueue instance
 * 
 * Gets a new instance of a #ModestMailOperationQueue if it is the
 * first call to the function, or the current instantiated one
 * otherwise. This object is supposed to be a singleton
 * 
 * Returns: an instance of a #ModestMailOperationQueue
 **/
ModestMailOperationQueue*   modest_tny_platform_factory_get_mail_operation_queue_instance (ModestTnyPlatformFactory *fact);


/**
 * modest_tny_platform_factory_get_cache_mgr_instance:
 * @fact: the #TnyPlatformFactory that holds the #ModestCacheMgr instance
 * 
 * Gets a new instance of a #ModestCacheMgr if it is the
 * first call to the function, or the current instantiated one
 * otherwise. This object is supposed to be a singleton
 * 
 * Returns: an instance of a #ModestCacheMgr.
 **/
ModestCacheMgr*   modest_tny_platform_factory_get_cache_mgr_instance (ModestTnyPlatformFactory *fact);




G_END_DECLS

#endif /* __MODEST_TNY_PLATFORM_FACTORY_H__ */

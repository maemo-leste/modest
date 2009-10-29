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

#ifndef __MODEST_RUNTIME_H__
#define __MODEST_RUNTIME_H__

#include <glib.h>
#include <glib-object.h>
#include <modest-conf.h>
#include <modest-account-mgr.h>
#include <modest-cache-mgr.h>
#include <modest-email-clipboard.h>
#include <modest-mail-operation-queue.h>
#include <modest-tny-account-store.h>
#include <modest-tny-send-queue.h>
#include <modest-signal-mgr.h>
#include <tny-platform-factory.h>
#include "widgets/modest-window-mgr.h"
#include <modest-protocol-registry.h>
#include <tny-stream-cache.h>
#include <modest-plugin-factory.h>
#include <widgets/modest-toolkit-factory.h>

G_BEGIN_DECLS

#define MODEST_DEBUG "MODEST_DEBUG"

typedef enum {
	MODEST_RUNTIME_DEBUG_ABORT_ON_WARNING      = 1 << 0,
	MODEST_RUNTIME_DEBUG_LOG_ACTIONS           = 1 << 1, /* not in use atm */
	MODEST_RUNTIME_DEBUG_OBJECTS               = 1 << 2, /* for g_type_init */
	MODEST_RUNTIME_DEBUG_SIGNALS               = 1 << 3, /* for g_type_init */
	MODEST_RUNTIME_DEBUG_FACTORY_SETTINGS      = 1 << 4, /* reset to factory defaults */
	MODEST_RUNTIME_DEBUG_CODE                  = 1 << 5  /* print various debugging messages */
} ModestRuntimeDebugFlags;

/**
 * modest_runtime_init:
 *
 * initialize the modest runtime system (which sets up the
 * environment, instantiates singletons and so on)
 * modest_runtime_init should only be called once, and
 * when done with it, modest_runtime_uninit should be called
 *
 * TRUE if this succeeded, FALSE otherwise.
 */
gboolean modest_runtime_init (void);


/**
 * modest_runtime_uinit:
 *
 * uninitialize the modest runtime system; free all the
 * resources and so on.
 *
 * TRUE if this succeeded, FALSE otherwise
 */
gboolean modest_runtime_uninit (void);


/**
 * modest_runtime_get_debug_flags
 *
 * get the debug flags for modest; they are read from the MODEST_DEBUG
 * environment variable; the flags specified as strings, separated by ':'.
 * Possible values are:
 * - "abort-on-warning": abort the program when a gtk/glib/.. warning occurs.
 * useful when running in debugger
 * - "log-actions": log user actions (not in use atm)
 * - "debug-objects": track the use of (g)objects in the program. this option influences
 *  g_type_init_with_debug_flags
 *  - "debug-signals": track the use of (g)signals in the program. this option influences
 *  g_type_init_with_debug_flags
 * if you would want to track signals and log actions, you could do something like:
 *  MODEST_DEBUG="log-actions:track-signals" ./modest
 * NOTE that the flags will stay the same during the run of the program, even
 * if the environment variable changes.
 * 
 * Returns: the bitwise OR of the debug flags
 */
ModestRuntimeDebugFlags modest_runtime_get_debug_flags  (void) G_GNUC_CONST;

/**
 * modest_runtime_get_conf:
 * 
 * get the ModestConf singleton instance
 * 
 * Returns: the ModestConf singleton. This should NOT be unref'd.
 **/
ModestConf*         modest_runtime_get_conf   (void);


/**
 * modest_runtime_get_account_mgr:
 * 
 * get the ModestAccountMgr singleton instance
 * 
 * Returns: the ModestAccountMgr singleton. This should NOT be unref'd.
 **/
ModestAccountMgr*         modest_runtime_get_account_mgr   (void);

/**
 * modest_runtime_get_images_cache:
 * 
 * get the images #TnyStreamCache singleton instance
 * 
 * Returns: the images #TnyStreamCache singleton. This should NOT be unref'd.
 **/
TnyStreamCache*         modest_runtime_get_images_cache   (void);

/**
 * modest_runtime_get_email_clipboard:
 * 
 * get the #ModestEmailClipboard singleton instance
 * 
 * Returns: the #ModestEamilClipboard singleton. This should NOT be unref'd.
 **/
ModestEmailClipboard*     modest_runtime_get_email_clipboard   (void);

/**
 * modest_runtime_get_account_store:
 * 
 * get the ModestTnyAccountStore singleton instance
 *
 * Returns: the ModestTnyAccountStore singleton. This should NOT be unref'd.
 **/
ModestTnyAccountStore*    modest_runtime_get_account_store (void);


/**
 * modest_runtime_get_cache_mgr:
 * 
 * get the ModestCacheMgr singleton instance
 *
 * Returns: the #ModestCacheMgr singleton. This should NOT be unref'd.
 **/
ModestCacheMgr*           modest_runtime_get_cache_mgr     (void);



/**
 * modest_runtime_get_device:
 * 
 * get the #TnyDevice singleton instance
 *
 * Returns: the #TnyDevice singleton. This should NOT be unref'd.
 **/
TnyDevice*                    modest_runtime_get_device     (void);


/**
 * modest_runtime_get_platform_factory:
 * 
 * get the #TnyPlatformFactory singleton instance
 *
 * Returns: the #TnyPlatformFactory singleton. This should NOT be unref'd.
 **/
TnyPlatformFactory*           modest_runtime_get_platform_factory     (void);

/**
 * modest_runtime_get_toolkit_factory:
 * 
 * get the #ModestToolkitFactory singleton instance
 *
 * Returns: the #ModestToolkitFactory singleton. This should NOT be unref'd.
 **/
ModestToolkitFactory*           modest_runtime_get_toolkit_factory     (void);

/**
 * modest_runtime_get_mail_operation_queue:
 * 
 * get the #ModestMailOperationQueue singleton instance
 *
 * Returns: the #ModestMailOperationQueue singleton. This should NOT be unref'd.
 **/
ModestMailOperationQueue* modest_runtime_get_mail_operation_queue (void);


/**
 * modest_runtime_get_send_queue:
 * @account: a valid TnyTransportAccount
 * @create: whether or not a new send queue should be created if it does not exist
 * 
 * get the send queue for the given account
 *
 * Returns: the #ModestTnySendQueue singleton instance for this account
 * (ie., one singleton per account). This should NOT be unref'd.
 **/
ModestTnySendQueue* modest_runtime_get_send_queue        (TnyTransportAccount *account,
							  gboolean create);

/**
 * modest_runtime_remove_send_queue:
 * @account: a #TnyTransportAccount
 * 
 * Removes the send queue associated with the given transport account
 **/
void modest_runtime_remove_send_queue (TnyTransportAccount *account);

/**
 * modest_runtime_remove_all_send_queues:
 * 
 * Removes all send queues, ready for them to be recreated, for instance 
 * with a new connection.
 **/
void modest_runtime_remove_all_send_queues ();

/**
 * modest_runtime_get_window_mgr:
 *
 * get the #ModestWindowMgr singleton instance
 *
 * Returns: the #ModestWindowMgr singleton. This should NOT be unref'd.
 **/
ModestWindowMgr* modest_runtime_get_window_mgr (void);

/**
 * modest_runtime_get_protocol_registry:
 *
 * get the #ModestProtocolRegistry singleton instance
 *
 * Returns: the #ModestProtocolRegistry singleton. This should NOT be unref'd.
 **/
ModestProtocolRegistry* modest_runtime_get_protocol_registry (void);

/**
 * modest_runtime_get_plugin_factory:
 *
 * get the #ModestPluginFactory singleton instance
 *
 * Returns: the #ModestPluginFactory singleton. This should NOT be unref'd.
 **/
ModestPluginFactory * modest_runtime_get_plugin_factory (void);

/**
 * modest_runtime_get_allow_shutdown:
 *
 * tell if Modest will shutdown when window list and mail op queue are empty
 *
 * Returns: %TRUE if modest shutdown is allowed, %FALSE otherwise
 */
gboolean modest_runtime_get_allow_shutdown (void);

/**
 * modest_runtime_get_allow_shutdown:
 * @allow: a #gboolean
 *
 * set if Modest will shutdown when window list and mail op queue are empty
 *
 */
void modest_runtime_set_allow_shutdown (gboolean allow);

G_END_DECLS

#endif /*__MODEST_RUNTIME_H__*/

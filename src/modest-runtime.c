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
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <modest-runtime.h>
#include <modest-runtime-priv.h>
#include <modest-defs.h>
#include <modest-singletons.h>
#include <widgets/modest-header-view.h>
#include <widgets/modest-folder-view.h>
#include <modest-tny-platform-factory.h>
#include <modest-platform.h>
#include <modest-widget-memory.h>
#include <modest-widget-memory-priv.h>
#include <modest-local-folder-info.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-icon-names.h>
#include <modest-ui-actions.h>
#include <modest-debug.h>

static ModestSingletons       *_singletons    = NULL;

// we get the account store here instead of in Singletons
// as it leads to various chicken & problems with initialization
static ModestTnyAccountStore  *_account_store  = NULL;

/* Signal handlers for the send queues */
static GSList *_sig_handlers = NULL;

/* will modest shut down when window list and mail op queue are empty? */
static gboolean _allow_shutdown = FALSE;

/*
 * private functions declared in modest-runtime-priv.h -
 * only to be called from modest-init.c 
 */
/*-----------------------------------------------------------------------------*/
gboolean
modest_runtime_init (void)
{
	if (_singletons) {
		g_printerr ("modest: modest_runtime_init can only be run once\n");
		return FALSE;
	}

	_singletons = modest_singletons_new ();
	if (!_singletons) {
		g_printerr ("modest: failed to create singletons\n");
		return FALSE;
	}

	return TRUE;
}

gboolean
modest_runtime_uninit (void)
{
	if (!_singletons)
		return TRUE; 	/* uninit maybe called if runtime_init failed */
	
	g_return_val_if_fail (MODEST_IS_SINGLETONS(_singletons), FALSE);

	g_debug ("%s: cleaning up", __FUNCTION__);

	if (_sig_handlers) {
		modest_signal_mgr_disconnect_all_and_destroy (_sig_handlers);
		_sig_handlers = NULL;
	}

	g_debug ("%s: cleaned up signal manager", __FUNCTION__);

	MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(_singletons,"");
	g_object_unref(_singletons);
	_singletons = NULL;

	g_debug ("%s: cleaned up singletons", __FUNCTION__);

	if (_account_store) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(_account_store,"");
		g_object_unref(_account_store);
		_account_store = NULL;
	}

	g_debug ("%s: cleaned up the account store", __FUNCTION__);

	
	g_debug ("%s: all cleaned up", __FUNCTION__);

	
	return TRUE;
}
/*-----------------------------------------------------------------------------*/
	

ModestAccountMgr*
modest_runtime_get_account_mgr   (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_account_mgr (_singletons);
}

TnyStreamCache*
modest_runtime_get_images_cache   (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_images_cache (_singletons);
}

ModestEmailClipboard*
modest_runtime_get_email_clipboard   (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_email_clipboard (_singletons);
}

ModestTnyAccountStore*
modest_runtime_get_account_store   (void)
{
	g_return_val_if_fail (_singletons, NULL);
	/* we get the account store here instead of in Singletons as
	   it leads to various chicken & egg problems with
	   initialization */
	if (!_account_store) {
		_account_store  = modest_tny_account_store_new (modest_runtime_get_account_mgr(),
								modest_runtime_get_device());
		if (!_account_store) {
			g_printerr ("modest: cannot create modest tny account store instance\n");
			return NULL;
		}
	}
	return _account_store;
}

ModestConf*
modest_runtime_get_conf (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_conf (_singletons);
}


ModestCacheMgr*
modest_runtime_get_cache_mgr (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_cache_mgr (_singletons);
}


ModestMailOperationQueue*
modest_runtime_get_mail_operation_queue (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_mail_operation_queue (_singletons);
}



TnyDevice*
modest_runtime_get_device (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_device (_singletons);
}


TnyPlatformFactory*
modest_runtime_get_platform_factory  (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_platform_factory (_singletons);
}

ModestTnySendQueue*
modest_runtime_get_send_queue  (TnyTransportAccount *account,
				gboolean create)
{
	ModestCacheMgr *cache_mgr;
	GHashTable     *send_queue_cache;
	gpointer       orig_key = NULL, send_queue = NULL;
	
	g_return_val_if_fail (_singletons, NULL);
	g_return_val_if_fail (TNY_IS_TRANSPORT_ACCOUNT(account), NULL);

	cache_mgr = modest_singletons_get_cache_mgr (_singletons);
	send_queue_cache = modest_cache_mgr_get_cache (cache_mgr,
						       MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);

	/* Each modest account has its own send queue.
	 * Note that each modest account will have its own outbox folder, 
	 * returned by TnySendQueue::get_outbox_func().
	 */
	if (!g_hash_table_lookup_extended (send_queue_cache, account, &orig_key, &send_queue) &&
	    create) {
		/* Note that this send queue will start sending messages from its outbox 
		 * as soon as it is instantiated: */
		send_queue = modest_tny_send_queue_new (TNY_CAMEL_TRANSPORT_ACCOUNT(account));

		if (send_queue) {
			_sig_handlers = 
				modest_signal_mgr_connect (_sig_handlers, 
							   send_queue, 
							   "error_happened",
							   G_CALLBACK (modest_ui_actions_on_send_queue_error_happened), 
							   NULL);

			_sig_handlers = 
				modest_signal_mgr_connect (_sig_handlers, 
							   send_queue, 
							   "status_changed",
							   G_CALLBACK (modest_ui_actions_on_send_queue_status_changed), 
							   NULL);

			g_hash_table_insert (send_queue_cache, 
					     g_object_ref (account), 
					     send_queue);
		}
	}

	return (send_queue) ? MODEST_TNY_SEND_QUEUE(send_queue) : NULL;
}

void modest_runtime_remove_all_send_queues ()
{
	ModestCacheMgr *cache_mgr = modest_singletons_get_cache_mgr (_singletons);
	
	modest_cache_mgr_flush (cache_mgr, MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);
}

void 
modest_runtime_remove_send_queue (TnyTransportAccount *account)
{

	ModestCacheMgr *cache_mgr;
	GHashTable     *send_queue_cache;

	g_return_if_fail (TNY_IS_TRANSPORT_ACCOUNT (account));	
	g_return_if_fail (_singletons);

	cache_mgr = modest_singletons_get_cache_mgr (_singletons);
	send_queue_cache = modest_cache_mgr_get_cache (cache_mgr,
						       MODEST_CACHE_MGR_CACHE_TYPE_SEND_QUEUE);	

	if (g_hash_table_lookup (send_queue_cache, account))
		g_hash_table_remove (send_queue_cache, account);
}

ModestWindowMgr *
modest_runtime_get_window_mgr (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_window_mgr (_singletons);
}

ModestPluginFactory *
modest_runtime_get_plugin_factory (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_plugin_factory (_singletons);
}

ModestProtocolRegistry *
modest_runtime_get_protocol_registry (void)
{
	g_return_val_if_fail (_singletons, NULL);
	return modest_singletons_get_protocol_registry (_singletons);
}

/* http://primates.ximian.com/~federico/news-2006-04.html#memory-debugging-infrastructure*/
ModestRuntimeDebugFlags
modest_runtime_get_debug_flags ()
{
	static const GDebugKey debug_keys[] = {
		{ "abort-on-warning",   MODEST_RUNTIME_DEBUG_ABORT_ON_WARNING },
		{ "log-actions",        MODEST_RUNTIME_DEBUG_LOG_ACTIONS },
		{ "debug-objects",      MODEST_RUNTIME_DEBUG_OBJECTS },
		{ "debug-signals",      MODEST_RUNTIME_DEBUG_SIGNALS },
		{ "factory-settings",   MODEST_RUNTIME_DEBUG_FACTORY_SETTINGS},
		{ "debug-code",         MODEST_RUNTIME_DEBUG_CODE}
	};
	const gchar *str;
	static ModestRuntimeDebugFlags debug_flags = -1;

	if (debug_flags != -1)
		return debug_flags;
	
	str = g_getenv (MODEST_DEBUG);
	
	if (str != NULL)
		debug_flags = g_parse_debug_string (str, debug_keys, G_N_ELEMENTS (debug_keys));
	else
		debug_flags = 0;
	
	return debug_flags;
}

gboolean 
modest_runtime_get_allow_shutdown (void)
{
	return _allow_shutdown;
}

void 
modest_runtime_set_allow_shutdown (gboolean allow)
{
	_allow_shutdown = allow;
}

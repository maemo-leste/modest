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

#include "modest-singletons.h"
#include "modest-runtime.h"
#include "modest-defs.h"
#include "modest-debug.h"
#ifdef MODEST_TOOLKIT_HILDON2
#include "hildon2/modest-hildon2-window-mgr.h"
#else
#include "gtk/modest-gtk-window-mgr.h"
#endif
#include <tny-fs-stream-cache.h>

/* 'private'/'protected' functions */
static void modest_singletons_class_init (ModestSingletonsClass *klass);
static void modest_singletons_init       (ModestSingletons *obj);
static void modest_singletons_finalize   (GObject *obj);

typedef struct _ModestSingletonsPrivate ModestSingletonsPrivate;
struct _ModestSingletonsPrivate {
	ModestConf                *conf;
	ModestAccountMgr          *account_mgr;
	ModestEmailClipboard      *email_clipboard;
	ModestCacheMgr            *cache_mgr;	
	ModestMailOperationQueue  *mail_op_queue;
	TnyPlatformFactory        *platform_fact;
	TnyDevice                 *device;
	ModestWindowMgr           *window_mgr;
	ModestProtocolRegistry    *protocol_registry;
	ModestPluginFactory   *plugin_factory;
	ModestToolkitFactory      *toolkit_factory;
	TnyStreamCache            *images_cache;
};
#define MODEST_SINGLETONS_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                               MODEST_TYPE_SINGLETONS, \
                                               ModestSingletonsPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_singletons_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestSingletonsClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_singletons_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestSingletons),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_singletons_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestSingletons",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_singletons_class_init (ModestSingletonsClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_singletons_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestSingletonsPrivate));
}

static void
modest_singletons_init (ModestSingletons *obj)
{
	ModestSingletonsPrivate *priv;
	gchar *images_cache_path;
	priv = MODEST_SINGLETONS_GET_PRIVATE(obj);

	priv->conf            = NULL;
	priv->account_mgr     = NULL;
	priv->email_clipboard = NULL;
	priv->cache_mgr       = NULL;
	priv->mail_op_queue   = NULL;
	priv->platform_fact   = NULL;
	priv->device          = NULL;
	priv->window_mgr      = NULL;
	priv->protocol_registry = NULL;
	priv->plugin_factory = NULL;
	priv->toolkit_factory = NULL;

	priv->protocol_registry = modest_protocol_registry_new ();
	if (!priv->protocol_registry) {
		g_printerr ("modest:cannot create protocol registry instance\n");
		return;
	}
	modest_protocol_registry_set_to_default (priv->protocol_registry);
	priv->images_cache    = NULL;
	
	priv->conf           = modest_conf_new ();
	if (!priv->conf) {
		g_printerr ("modest: cannot create modest conf instance\n");
		return;
	}

	priv->account_mgr    = modest_account_mgr_new (priv->conf);
	if (!priv->account_mgr) {
		g_printerr ("modest: cannot create modest account mgr instance\n");
		return;
	}

	priv->email_clipboard    = modest_email_clipboard_new ();
	if (!priv->email_clipboard) {
		g_printerr ("modest: cannot create modest email clipboard instance\n");
		return;
	}

	priv->platform_fact  = modest_tny_platform_factory_get_instance ();
	if (!priv->platform_fact) {
		g_printerr ("modest: cannot create platform factory instance\n");
		return;
	}

	priv->toolkit_factory  = modest_toolkit_factory_get_instance ();
	if (!priv->toolkit_factory) {
		g_printerr ("modest: cannot create toolkit factory instance\n");
		return;
	}

	priv->device  = tny_platform_factory_new_device (priv->platform_fact);
	if (!priv->device) {
		g_printerr ("modest: cannot create tny device instance\n");
		return;
	}
	
	priv->cache_mgr     = modest_cache_mgr_new ();
	if (!priv->cache_mgr) {
		g_printerr ("modest: cannot create modest cache mgr instance\n");
		return;
	}

	priv->mail_op_queue  = modest_mail_operation_queue_new ();
	if (!priv->mail_op_queue) {
		g_printerr ("modest: cannot create modest mail operation queue instance\n");
		return;
	}

#if MODEST_TOOLKIT_HILDON2
	priv->window_mgr = modest_hildon2_window_mgr_new ();
#else
	priv->window_mgr = modest_gtk_window_mgr_new ();
#endif
	if (!priv->window_mgr) {
		g_printerr ("modest: cannot create modest window manager instance\n");
		return;
	}

	priv->plugin_factory = modest_plugin_factory_new ();
	if (!priv->plugin_factory) {
		g_printerr ("modest: cannot create modest mail plugin factory instance\n");
		return;
	}

	images_cache_path = g_build_filename (g_get_home_dir (), MODEST_DIR, MODEST_IMAGES_CACHE_DIR, NULL);
	priv->images_cache = tny_fs_stream_cache_new (images_cache_path, MODEST_IMAGES_CACHE_SIZE);
	g_free (images_cache_path);
	if (!priv->images_cache) {
		g_printerr ("modest: cannot create images cache instance\n");
		return;
	}

}

static void
modest_singletons_finalize (GObject *obj)
{
	ModestSingletonsPrivate *priv;
		
	priv = MODEST_SINGLETONS_GET_PRIVATE(obj);

	if (priv->images_cache) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF (priv->images_cache, "");
		g_object_unref (G_OBJECT (priv->images_cache));
		priv->images_cache = NULL;
	}
	
	if (priv->window_mgr) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->window_mgr,"");
		g_object_unref (G_OBJECT(priv->window_mgr));
		priv->window_mgr = NULL;
	}
	
	if (priv->mail_op_queue) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->mail_op_queue,"");
		g_object_unref (G_OBJECT(priv->mail_op_queue));
		priv->mail_op_queue = NULL;
	}

	if (priv->cache_mgr) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->cache_mgr,"");
		g_object_unref (G_OBJECT(priv->cache_mgr));
		priv->cache_mgr = NULL;
	}

	if (priv->device) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->device,"");
		g_object_unref (G_OBJECT(priv->device));
		priv->device = NULL;
	}

	if (priv->platform_fact) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->platform_fact,"");
		g_object_unref (G_OBJECT(priv->platform_fact));
		priv->platform_fact = NULL;
	}
	
	if (priv->email_clipboard) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->email_clipboard,"");
		g_object_unref (G_OBJECT(priv->email_clipboard));
		priv->email_clipboard = NULL;
	}

	if (priv->protocol_registry) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->protocol_registry,"");
		g_object_unref (G_OBJECT(priv->protocol_registry));
		priv->protocol_registry = NULL;
	}

	if (priv->plugin_factory) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->plugin_factory,"");
		g_object_unref (G_OBJECT(priv->plugin_factory));
		priv->plugin_factory = NULL;
	}

	/* It is important that the account manager is uninitialized after
	 * the mail op queue is uninitialized because the mail op queue
	 * cancells any mail operations which in turn access the account
	 * manager (see modest_mail_operation_notify_end()). */
	if (priv->account_mgr) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->account_mgr,"");
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}
	
	if (priv->conf) {
		MODEST_DEBUG_VERIFY_OBJECT_LAST_REF(priv->conf,"");
		g_object_unref (G_OBJECT(priv->conf));
		priv->conf = NULL;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestSingletons*
modest_singletons_new (void)
{
	ModestSingletonsPrivate *priv;
	ModestSingletons *self;
	static gboolean invoked = FALSE;

	if (invoked) {
		g_printerr ("%s: modest: modest_singletons_new may only be called once, aborting...\n",
			    __FUNCTION__);
		abort();
		return NULL;
	}
	
	self = MODEST_SINGLETONS(g_object_new(MODEST_TYPE_SINGLETONS, NULL));
	priv = MODEST_SINGLETONS_GET_PRIVATE(self);
	
	/* widget_factory will still be NULL, as it is initialized lazily */
	if (!(priv->conf && priv->account_mgr && priv->email_clipboard && 
	      priv->cache_mgr && priv->mail_op_queue && priv->device && 
	      priv->platform_fact && priv->plugin_factory)) {
		g_printerr ("modest: failed to create singletons object\n");
		g_object_unref (G_OBJECT(self));
		self = NULL;
	}

	invoked = TRUE;
	return self;
}


ModestConf*
modest_singletons_get_conf (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->conf;
}

ModestAccountMgr*
modest_singletons_get_account_mgr (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->account_mgr;
}

ModestEmailClipboard*
modest_singletons_get_email_clipboard (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->email_clipboard;
}

ModestCacheMgr*
modest_singletons_get_cache_mgr (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->cache_mgr;
}

ModestMailOperationQueue*
modest_singletons_get_mail_operation_queue (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->mail_op_queue;
}

TnyDevice*
modest_singletons_get_device (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->device;
}


TnyPlatformFactory*
modest_singletons_get_platform_factory (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->platform_fact;
}

ModestToolkitFactory*
modest_singletons_get_toolkit_factory (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->toolkit_factory;
}

ModestWindowMgr* 
modest_singletons_get_window_mgr (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->window_mgr;
}

ModestProtocolRegistry* 
modest_singletons_get_protocol_registry (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->protocol_registry;
}

TnyStreamCache* 
modest_singletons_get_images_cache (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->images_cache;
}

ModestPluginFactory *
modest_singletons_get_plugin_factory (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);

	return MODEST_SINGLETONS_GET_PRIVATE (self)->plugin_factory;
}

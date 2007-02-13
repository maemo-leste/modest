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

/* 'private'/'protected' functions */
static void modest_singletons_class_init (ModestSingletonsClass *klass);
static void modest_singletons_init       (ModestSingletons *obj);
static void modest_singletons_finalize   (GObject *obj);

typedef struct _ModestSingletonsPrivate ModestSingletonsPrivate;
struct _ModestSingletonsPrivate {
	ModestConf                *conf;
	ModestAccountMgr          *account_mgr;
	ModestTnyAccountStore     *account_store;
	ModestCacheMgr            *cache_mgr;	
	ModestMailOperationQueue  *mail_op_queue;
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
	priv = MODEST_SINGLETONS_GET_PRIVATE(obj);

	priv->conf           = NULL;
	priv->account_mgr    = NULL;
	priv->account_store  = NULL;
	priv->cache_mgr      = NULL;
	priv->mail_op_queue  = NULL;
	
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
	
	priv->account_store  = modest_tny_account_store_new (priv->account_mgr);
	if (!priv->account_store) {
		g_printerr ("modest: cannot create modest tny account store instance\n");
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
}


static void
check_object_is_dead (GObject *obj, gchar *name)
{
	if (G_IS_OBJECT(obj))
		g_warning ("BUG: %s is still alive\n", name);
}

static void
modest_singletons_finalize (GObject *obj)
{
	ModestSingletonsPrivate *priv;
	priv = MODEST_SINGLETONS_GET_PRIVATE(obj);

	if (priv->account_store) {
		g_object_unref (G_OBJECT(priv->account_store));
		check_object_is_dead ((GObject*)priv->account_store,
				      "priv->account_store");
		priv->account_store = NULL;
	}

	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		check_object_is_dead ((GObject*)priv->account_mgr,
				      "priv->account_mgr");
		priv->account_mgr = NULL;
	}

	if (priv->conf) {
		g_object_unref (G_OBJECT(priv->conf));
		check_object_is_dead ((GObject*)priv->conf,
				      "priv->conf");
		priv->conf = NULL;
	}

	if (priv->cache_mgr) {
		g_object_unref (G_OBJECT(priv->cache_mgr));
		check_object_is_dead ((GObject*)priv->cache_mgr,
				      "priv->cache_mgr");
		priv->cache_mgr = NULL;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestSingletons*
modest_singletons_new (void)
{
	ModestSingletonsPrivate *priv;
	ModestSingletons *self;
	
	self = MODEST_SINGLETONS(g_object_new(MODEST_TYPE_SINGLETONS, NULL));
	priv = MODEST_SINGLETONS_GET_PRIVATE(self);

	/* widget_factory will still be NULL, as it is initialized lazily */
	if (!(priv->conf && priv->account_mgr && priv->account_store &&
	      priv->cache_mgr && priv->mail_op_queue)) {
		g_printerr ("modest: failed to create singletons instance\n");
		g_object_unref (G_OBJECT(self));
		self = NULL;
	}
	
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

ModestTnyAccountStore*
modest_singletons_get_account_store (ModestSingletons *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_SINGLETONS_GET_PRIVATE(self)->account_store;
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

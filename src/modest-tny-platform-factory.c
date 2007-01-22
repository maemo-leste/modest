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

/* modest-tny-platform-factory.c */
#include <config.h>

#include <tny-camel-header.h>
#include <tny-camel-mime-part.h>
#include <tny-camel-msg.h>

/* MODES_PLATFORM_ID: 1 ==> gtk, 2==> maemo */
#if MODEST_PLATFORM_ID==1   
#include <tny-gnome-device.h>
#elif MODEST_PLATFORM_ID==2
#include <tny-maemo-device.h>
#endif

#include "modest-tny-platform-factory.h"
#include "modest-tny-account-store.h"

/* 'private'/'protected' functions */
static void modest_tny_platform_factory_class_init (ModestTnyPlatformFactoryClass *klass);
static void modest_tny_platform_factory_init       (ModestTnyPlatformFactory *obj);
static void modest_tny_platform_factory_finalize   (GObject *obj);
static GObject *modest_tny_platform_factory_constructor (GType type, guint n_construct_params,
							 GObjectConstructParam *construct_params);
static void tny_platform_factory_init (gpointer g, gpointer iface_data);

static TnyAccountStore* modest_tny_platform_factory_new_account_store (TnyPlatformFactory *self);
static TnyDevice*       modest_tny_platform_factory_new_device        (TnyPlatformFactory *self);
static TnyMsgView*      modest_tny_platform_factory_new_msg_view      (TnyPlatformFactory *self);
static TnyMsg*          modest_tny_platform_factory_new_msg           (TnyPlatformFactory *self);
static TnyMimePart*     modest_tny_platform_factory_new_mime_part     (TnyPlatformFactory *self);
static TnyHeader*       modest_tny_platform_factory_new_header        (TnyPlatformFactory *self);


/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestTnyPlatformFactoryPrivate ModestTnyPlatformFactoryPrivate;
struct _ModestTnyPlatformFactoryPrivate {
	ModestTnyAccountStore    *account_store;
	ModestConf               *conf;
	ModestAccountMgr         *account_mgr;
	ModestMailOperationQueue *mail_op_queue;
	ModestCacheMgr           *cache_mgr;
};

#define MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                         MODEST_TYPE_TNY_PLATFORM_FACTORY, \
                                                         ModestTnyPlatformFactoryPrivate))
/* globals */
static GObjectClass *parent_class = NULL;
static ModestTnyPlatformFactory *singleton = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_tny_platform_factory_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyPlatformFactoryClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_platform_factory_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyPlatformFactory),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_platform_factory_init,
			NULL
		};

		static const GInterfaceInfo tny_platform_factory_info = {
			(GInterfaceInitFunc) tny_platform_factory_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestTnyPlatformFactory",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_PLATFORM_FACTORY, 
					     &tny_platform_factory_info);
	}
	return my_type;
}

static void
modest_tny_platform_factory_class_init (ModestTnyPlatformFactoryClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;
	parent_class            = g_type_class_peek_parent (klass);

	gobject_class->finalize = modest_tny_platform_factory_finalize;
	gobject_class->constructor = modest_tny_platform_factory_constructor;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyPlatformFactoryPrivate));
}

static void
modest_tny_platform_factory_init (ModestTnyPlatformFactory *obj)
{
	ModestTnyPlatformFactoryPrivate *priv;
	priv = MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(obj);

	priv->account_mgr   = NULL;
	priv->conf          = NULL;
	priv->account_store = NULL;
	priv->mail_op_queue = NULL;
	priv->cache_mgr     = NULL;
}

static GObject*
modest_tny_platform_factory_constructor (GType type, guint n_construct_params,
					 GObjectConstructParam *construct_params)
{
	GObject *object;

	if (!singleton)	{
		object = G_OBJECT_CLASS (parent_class)->constructor (type,
				n_construct_params, construct_params);

		singleton = MODEST_TNY_PLATFORM_FACTORY (object);
	} else {
		object = G_OBJECT (singleton);
		g_object_freeze_notify (G_OBJECT (singleton));
	}

	return object;
}

static void
modest_tny_platform_factory_finalize (GObject *obj)
{
	ModestTnyPlatformFactoryPrivate *priv;

	priv = MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(obj);

	if (priv->account_mgr)
		g_object_unref (G_OBJECT(priv->account_mgr));

	if (priv->conf)
		g_object_unref (G_OBJECT(priv->conf));

	if (priv->account_store)
		g_object_unref (G_OBJECT(priv->account_store));

	if (priv->mail_op_queue)
		g_object_unref (G_OBJECT(priv->mail_op_queue));

	if (priv->cache_mgr)
		g_object_unref (G_OBJECT(priv->cache_mgr));
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
tny_platform_factory_init (gpointer g, gpointer iface_data)
{
	TnyPlatformFactoryIface *klass = (TnyPlatformFactoryIface *)g;

	klass->new_account_store_func = modest_tny_platform_factory_new_account_store;
	klass->new_device_func        = modest_tny_platform_factory_new_device;
	klass->new_msg_view_func      = modest_tny_platform_factory_new_msg_view;
	klass->new_msg_func           = modest_tny_platform_factory_new_msg;
	klass->new_mime_part_func     = modest_tny_platform_factory_new_mime_part;
	klass->new_header_func        = modest_tny_platform_factory_new_header;

	return;
}

TnyPlatformFactory *
modest_tny_platform_factory_get_instance (void)
{
	ModestTnyPlatformFactory *self = g_object_new (MODEST_TYPE_TNY_PLATFORM_FACTORY, NULL);

	return TNY_PLATFORM_FACTORY (self);
}

static TnyAccountStore *
modest_tny_platform_factory_new_account_store (TnyPlatformFactory *self)
{
	ModestTnyPlatformFactoryPrivate *priv;

	priv = MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(self);

	if (!priv->account_store) {
		if (!priv->account_mgr)
			modest_tny_platform_factory_get_account_mgr_instance (MODEST_TNY_PLATFORM_FACTORY (self));

		priv->account_store = modest_tny_account_store_new (priv->account_mgr);
	}

	return TNY_ACCOUNT_STORE (priv->account_store);
}

static TnyDevice *
modest_tny_platform_factory_new_device (TnyPlatformFactory *self)
{
/* MODES_PLATFORM_ID: 1 ==> gtk, 2==> maemo */
#if MODEST_PLATFORM_ID==1   
	return TNY_DEVICE (tny_gnome_device_new ());
#elif MODEST_PLATFORM_ID==2
	return TNY_DEVICE (tny_maemo_device_new ());
#else
	g_return_val_if_reached (NULL);
#endif /* MODEST_PLATFORM */
}

static TnyMsgView*
modest_tny_platform_factory_new_msg_view (TnyPlatformFactory *self)
{
	/* TODO */
	return NULL;
}

static TnyMsg*
modest_tny_platform_factory_new_msg (TnyPlatformFactory *self)
{
	return tny_camel_msg_new ();
}


static TnyMimePart*
modest_tny_platform_factory_new_mime_part (TnyPlatformFactory *self)
{
	return tny_camel_mime_part_new ();
}


static TnyHeader*
modest_tny_platform_factory_new_header (TnyPlatformFactory *self)
{
	return tny_camel_header_new ();
}


ModestAccountMgr *
modest_tny_platform_factory_get_account_mgr_instance (ModestTnyPlatformFactory *fact)
{
	ModestTnyPlatformFactoryPrivate *priv;

	g_return_val_if_fail (fact, NULL);
	
	priv = MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(fact);

	if (!priv->account_mgr) {
		if (!priv->conf)
			modest_tny_platform_factory_get_conf_instance (fact);
		priv->account_mgr = modest_account_mgr_new (priv->conf);
	}

	return priv->account_mgr;
}

ModestConf *
modest_tny_platform_factory_get_conf_instance (ModestTnyPlatformFactory *fact)
{
	ModestTnyPlatformFactoryPrivate *priv;
	
	g_return_val_if_fail (fact, NULL);
	
	priv = MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(fact);

	if (!priv->conf)
		priv->conf = modest_conf_new ();

	return priv->conf;
}

ModestMailOperationQueue*   
modest_tny_platform_factory_get_mail_operation_queue_instance (ModestTnyPlatformFactory *fact)
{
	ModestTnyPlatformFactoryPrivate *priv;

	g_return_val_if_fail (fact, NULL);
	
	priv = MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(fact);

	if (!priv->mail_op_queue)
		priv->mail_op_queue = modest_mail_operation_queue_new ();

	return priv->mail_op_queue;
}



ModestCacheMgr*
modest_tny_platform_factory_get_cache_mgr_instance (ModestTnyPlatformFactory *fact)
{
	ModestTnyPlatformFactoryPrivate *priv;

	g_return_val_if_fail (fact, NULL);
	
	priv = MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(fact);

	if (!priv->cache_mgr)
		priv->cache_mgr = modest_cache_mgr_new ();
		
	return priv->cache_mgr;
}

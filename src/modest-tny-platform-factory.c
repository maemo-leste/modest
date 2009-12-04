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

#include <modest-runtime.h>
#include <modest-platform.h>

#include <tny-camel-header.h>
#include <tny-camel-mime-part.h>
#include <tny-camel-msg.h>

#include "modest-tny-platform-factory.h"
#include "modest-tny-account-store.h"
#ifdef MODEST_USE_WEBKIT
#include <widgets/modest-webkit-msg-view.h>
#else
#include <widgets/modest-gtkhtml-msg-view.h>
#endif

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

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

/* PRIVATE area commented as it's empty now. If you enable this again remember to enable also
 * private area registration in class init */

/* typedef struct _ModestTnyPlatformFactoryPrivate ModestTnyPlatformFactoryPrivate; */
/* struct _ModestTnyPlatformFactoryPrivate {}; */

/* #define MODEST_TNY_PLATFORM_FACTORY_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \ */
/*                                                          MODEST_TYPE_TNY_PLATFORM_FACTORY, \ */
/*                                                          ModestTnyPlatformFactoryPrivate)) */

/* globals */
static GObjectClass *parent_class = NULL;
static ModestTnyPlatformFactory *singleton = NULL;

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

/* 	g_type_class_add_private (gobject_class, sizeof(ModestTnyPlatformFactoryPrivate)); */
}

static void
modest_tny_platform_factory_init (ModestTnyPlatformFactory *obj)
{
	/* empty */
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
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
tny_platform_factory_init (gpointer g, gpointer iface_data)
{
	TnyPlatformFactoryIface *klass = (TnyPlatformFactoryIface *)g;

	klass->new_account_store = modest_tny_platform_factory_new_account_store;
	klass->new_device        = modest_tny_platform_factory_new_device;
	klass->new_msg_view      = modest_tny_platform_factory_new_msg_view;
	klass->new_msg           = modest_tny_platform_factory_new_msg;
	klass->new_mime_part     = modest_tny_platform_factory_new_mime_part;
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
	return TNY_ACCOUNT_STORE(modest_tny_account_store_new
				 (modest_runtime_get_account_mgr(),
				  modest_runtime_get_device()));
}

static TnyDevice *
modest_tny_platform_factory_new_device (TnyPlatformFactory *self)
{
	return modest_platform_get_new_device ();
}

static TnyMsgView*
modest_tny_platform_factory_new_msg_view (TnyPlatformFactory *self)
{
	/* Here we'll select one of the implementations available */
#ifdef MODEST_USE_WEBKIT
	return g_object_new (MODEST_TYPE_WEBKIT_MSG_VIEW, NULL);
#else
	return g_object_new (MODEST_TYPE_GTKHTML_MSG_VIEW, NULL);
#endif
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



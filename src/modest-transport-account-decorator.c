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


#include <modest-transport-account-decorator.h>
#include <modest-tny-account-store.h>
#include <modest-tny-account.h>
#include <modest-runtime.h>
#include <tny-simple-list.h>
#include <tny-iterator.h>
#include <tny-folder.h>
#include <modest-marshal.h>
#include <string.h> /* strcmp */

/* 'private'/'protected' functions */
static void modest_transport_account_decorator_class_init (ModestTransportAccountDecoratorClass *klass);
static void modest_transport_account_decorator_finalize   (GObject *obj);
static void modest_transport_account_decorator_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_transport_account_decorator_send (TnyTransportAccount *self, TnyMsg *msg, GError **err);

/* list my signals  */
/* enum { */
/* 	/\* MY_SIGNAL_1, *\/ */
/* 	/\* MY_SIGNAL_2, *\/ */
/* 	LAST_SIGNAL */
/* }; */

/* typedef struct _ModestTransportAccountDecoratorPrivate ModestTransportAccountDecoratorPrivate; */
/* struct _ModestTransportAccountDecoratorPrivate { */
/* }; */

/* #define MODEST_TRANSPORT_ACCOUNT_DECORATOR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \ */
/*                                                    MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR, \ */
/*                                                    ModestTransportAccountDecoratorPrivate)) */

/* globals */
static TnyCamelTransportAccountClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

/*
 * this thread actually tries to send all the mails in the outbox and keeps
 * track of their state.
 */
static void
modest_transport_account_decorator_send (TnyTransportAccount *self, TnyMsg *msg, GError **err)
{
	TnyTransportAccount *connection_specific_account = NULL;
/* 	ModestTransportAccountDecoratorPrivate *priv = MODEST_TRANSPORT_ACCOUNT_DECORATOR_GET_PRIVATE (self); */
	ModestTnyAccountStore *store = modest_runtime_get_account_store ();
	const gchar *account_name;

	account_name = modest_tny_account_get_parent_modest_account_name_for_server_account (TNY_ACCOUNT (self));
	if (account_name) {
		connection_specific_account = TNY_TRANSPORT_ACCOUNT 
			(modest_tny_account_store_get_smtp_specific_transport_account_for_open_connection (store, account_name));
	}
	
	if (connection_specific_account) {
		tny_transport_account_send (connection_specific_account, msg, err);
	} else {
		TNY_CAMEL_TRANSPORT_ACCOUNT_CLASS(parent_class)->send_func (self, msg, err);
	}
}

static void
modest_transport_account_decorator_class_init (ModestTransportAccountDecoratorClass *klass)
{
	GObjectClass *gobject_class;
	TnyCamelTransportAccountClass *transport_class;

	gobject_class = (GObjectClass*) klass;
	transport_class = (TnyCamelTransportAccountClass *) klass;
	
	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_transport_account_decorator_finalize;

	transport_class->send_func = modest_transport_account_decorator_send;

/* 	g_type_class_add_private (gobject_class, sizeof(ModestTransportAccountDecoratorPrivate)); */
}

static void
modest_transport_account_decorator_instance_init (GTypeInstance *instance, gpointer g_class)
{
/* 	ModestTransportAccountDecoratorPrivate *priv; */

/* 	priv = MODEST_TRANSPORT_ACCOUNT_DECORATOR_GET_PRIVATE (instance); */
}

static void
modest_transport_account_decorator_finalize (GObject *obj)
{
/* 	ModestTransportAccountDecoratorPrivate *priv; */
		
/* 	priv = MODEST_TRANSPORT_ACCOUNT_DECORATOR_GET_PRIVATE (obj); */

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

ModestTransportAccountDecorator*
modest_transport_account_decorator_new (void)
{
	ModestTransportAccountDecorator *self;
/* 	ModestTransportAccountDecoratorPrivate *priv; */
	
	self = MODEST_TRANSPORT_ACCOUNT_DECORATOR(g_object_new(MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR, NULL));
/* 	priv = MODEST_TRANSPORT_ACCOUNT_DECORATOR_GET_PRIVATE (self); */

	return self;
}

GType
modest_transport_account_decorator_get_type (void)
{
	static GType my_type = 0;

	if (my_type == 0) {
		static const GTypeInfo my_info = {
			sizeof(ModestTransportAccountDecoratorClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_transport_account_decorator_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTransportAccountDecorator),
			0,		/* n_preallocs */
			(GInstanceInitFunc) modest_transport_account_decorator_instance_init,
			NULL
		};
		
		my_type = g_type_register_static (TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT,
						  "ModestTransportAccountDecorator",
						  &my_info, 0);
	}
	return my_type;
}


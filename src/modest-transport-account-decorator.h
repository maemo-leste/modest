/* Copyright (c) 2007, Nokia Corporation
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

#include <glib.h>
#include <glib-object.h>
#include <tny-send-queue.h>
#include <tny-msg.h>
#include <tny-camel-transport-account.h>

#ifndef __MODEST_TRANSPORT_ACCOUNT_DECORATOR_H__
#define __MODEST_TRANSPORT_ACCOUNT_DECORATOR_H__

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR             (modest_transport_account_decorator_get_type())
#define MODEST_TRANSPORT_ACCOUNT_DECORATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR,ModestTransportAccountDecorator))
#define MODEST_TRANSPORT_ACCOUNT_DECORATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR,TransportAccountDecorator))
#define MODEST_IS_TRANSPORT_ACCOUNT_DECORATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR))
#define MODEST_IS_TRANSPORT_ACCOUNT_DECORATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR))
#define MODEST_TRANSPORT_ACCOUNT_DECORATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TRANSPORT_ACCOUNT_DECORATOR,ModestTransportAccountDecoratorClass))

typedef struct _ModestTransportAccountDecorator      ModestTransportAccountDecorator;
typedef struct _ModestTransportAccountDecoratorClass ModestTransportAccountDecoratorClass;

struct _ModestTransportAccountDecorator {
	TnyCamelTransportAccount  parent;
};

struct _ModestTransportAccountDecoratorClass {
	TnyCamelTransportAccountClass parent_class;

};

/**
 * modest_transport_account_decorator_get_type:
 * 
 * get the #GType for #ModestTransportAccountDecorator
 *  
 * Returns: the #GType
 */
GType        modest_transport_account_decorator_get_type    (void) G_GNUC_CONST;


/**
 * modest_transport_account_decorator_new:
 * @account: a valid transport account
 * 
 * create a new #ModestTransportAccountDecorator decorating @account
 * 
 * Returns: a new #ModestTransportAccountDecorator instance, or NULL in case
 * of any error
 */
ModestTransportAccountDecorator*    modest_transport_account_decorator_new        (void);

G_END_DECLS

#endif /* __MODEST_TRANSPORT_ACCOUNT_DECORATOR_H__ */


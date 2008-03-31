#ifndef MODEST_DEFAULT_CONNECTION_POLICY_H
#define MODEST_DEFAULT_CONNECTION_POLICY_H

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

#include <glib-object.h>

#include <tny-connection-policy.h>

G_BEGIN_DECLS

#define MODEST_TYPE_DEFAULT_CONNECTION_POLICY             (modest_default_connection_policy_get_type ())
#define MODEST_DEFAULT_CONNECTION_POLICY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_DEFAULT_CONNECTION_POLICY, ModestDefaultConnectionPolicy))
#define MODEST_DEFAULT_CONNECTION_POLICY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MODEST_TYPE_DEFAULT_CONNECTION_POLICY, ModestDefaultConnectionPolicyClass))
#define MODEST_IS_DEFAULT_CONNECTION_POLICY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_DEFAULT_CONNECTION_POLICY))
#define MODEST_IS_DEFAULT_CONNECTION_POLICY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MODEST_TYPE_DEFAULT_CONNECTION_POLICY))
#define MODEST_DEFAULT_CONNECTION_POLICY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MODEST_TYPE_DEFAULT_CONNECTION_POLICY, ModestDefaultConnectionPolicyClass))

typedef struct _ModestDefaultConnectionPolicy ModestDefaultConnectionPolicy;
typedef struct _ModestDefaultConnectionPolicyClass ModestDefaultConnectionPolicyClass;


struct _ModestDefaultConnectionPolicy
{
	GObject parent;

};

struct _ModestDefaultConnectionPolicyClass
{
	GObjectClass parent_class;
};

GType modest_default_connection_policy_get_type (void);
TnyConnectionPolicy* modest_default_connection_policy_new (void);

G_END_DECLS

#endif

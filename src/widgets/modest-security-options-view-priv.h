/* Copyright (c) 2008, Nokia Corporation
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

#ifndef __MODEST_SECURITY_OPTIONS_VIEW_PRIV_H__
#define __MODEST_SECURITY_OPTIONS_VIEW_PRIV_H__

#include <glib.h>
#include <glib-object.h>
#include "modest-account-settings.h"
#include "modest-protocol.h"

G_BEGIN_DECLS

typedef struct _ModestSecurityOptionsState {
	ModestProtocolType security;
	ModestProtocolType auth;
	gint port;
	const gchar *user;
	const gchar *pwd;
} ModestSecurityOptionsState;

typedef struct _ModestSecurityOptionsViewPrivate ModestSecurityOptionsViewPrivate;
struct _ModestSecurityOptionsViewPrivate {
	/* Common widgets */
	GtkWidget *security_view;
	GtkWidget *port_view;
	GtkWidget *auth_view;

	/* outgoing specific widgets */
	GtkWidget *user_entry;
	GtkWidget *pwd_entry;

	gboolean full; 	/* full=TRUE means all options */
	gboolean changed;

	ModestSecurityOptionsState initial_state;
};

#define MODEST_SECURITY_OPTIONS_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                          MODEST_TYPE_SECURITY_OPTIONS_VIEW, \
                                                          ModestSecurityOptionsViewPrivate))

G_END_DECLS

#endif /* __MODEST_SECURITY_OPTIONS_VIEW_PRIV_H__ */


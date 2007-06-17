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

#ifndef __MODEST_ACCOUNT_MGR_PRIV_H__
#define __MODEST_ACCOUNT_MGR_PRIV_H__

#include <glib.h>
#include <modest-conf.h>

/*
 * private functions, only for use in modest-account-mgr and
 * modest-account-mgr-helpers
 */

G_BEGIN_DECLS

gchar* _modest_account_mgr_account_from_key (const gchar *key, gboolean *is_account_key,
					     gboolean *is_server_account);
gchar * _modest_account_mgr_get_account_keyname (const gchar *account_name, const gchar * name,
						 gboolean server_account);

/* below is especially very _private_ stuff */
typedef struct _ModestAccountMgrPrivate ModestAccountMgrPrivate;
struct _ModestAccountMgrPrivate {
	ModestConf        *modest_conf;
	
	/* We store these as they change, and send notifications every X seconds: */
	GSList *changed_conf_keys; 
	guint timeout;
	gulong key_changed_handler_uid;
	GSList* busy_accounts;
};

#define MODEST_ACCOUNT_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_ACCOUNT_MGR, \
                                                ModestAccountMgrPrivate))

G_END_DECLS
#endif /* __MODEST_ACCOUNT_MGR_PRIV_H__ */

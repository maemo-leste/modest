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

#ifndef __MODEST_PROTOCOL_MGR_H__
#define __MODEST_PROTOCOL_MGR_H__

#include <glib-object.h>
#include <modest-pair.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_PROTOCOL_MGR             (modest_protocol_mgr_get_type())
#define MODEST_PROTOCOL_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_PROTOCOL_MGR,ModestProtocolMgr))
#define MODEST_PROTOCOL_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_PROTOCOL_MGR,GObject))
#define MODEST_IS_PROTOCOL_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_PROTOCOL_MGR))
#define MODEST_IS_PROTOCOL_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_PROTOCOL_MGR))
#define MODEST_PROTOCOL_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_PROTOCOL_MGR,ModestProtocolMgrClass))

typedef struct _ModestProtocolMgr      ModestProtocolMgr;
typedef struct _ModestProtocolMgrClass ModestProtocolMgrClass;

struct _ModestProtocolMgr {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestProtocolMgrClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestProtocolMgr* obj); */
};

/* transport */
#define MODEST_PROTOCOL_TRANSPORT_SENDMAIL "sendmail"
#define MODEST_PROTOCOL_TRANSPORT_SMTP     "smtp"

/* store */
#define MODEST_PROTOCOL_STORE_NONE     "none"
#define MODEST_PROTOCOL_STORE_POP      "pop"
#define MODEST_PROTOCOL_STORE_IMAP     "imap"
#define MODEST_PROTOCOL_STORE_MAILDIR  "maildir"
#define MODEST_PROTOCOL_STORE_MBOX     "mbox"

/* security */
#define MODEST_PROTOCOL_SECURITY_NONE  "none"
#define MODEST_PROTOCOL_SECURITY_SSL   "ssl"
#define MODEST_PROTOCOL_SECURITY_TLS   "tls"

/* authentication */
#define MODEST_PROTOCOL_AUTH_NONE      "none"
#define MODEST_PROTOCOL_AUTH_PASSWORD  "password"

typedef enum _ModestProtocolType {
	MODEST_PROTOCOL_TYPE_TRANSPORT,
	MODEST_PROTOCOL_TYPE_STORE,
	MODEST_PROTOCOL_TYPE_SECURITY,
	MODEST_PROTOCOL_TYPE_AUTH,
	MODEST_PROTOCOL_TYPE_ANY,	
	MODEST_PROTOCOL_TYPE_NUM
} ModestProtocolType;
/* typedef enum _ModestProtocolType ModestProtocolType; */

/* member functions */
GType        modest_protocol_mgr_get_type    (void) G_GNUC_CONST;

ModestProtocolMgr*    modest_protocol_mgr_new         (void);

const GSList*   modest_protocol_mgr_get_transport_protocols (ModestProtocolMgr *self);
const GSList*   modest_protocol_mgr_get_store_protocols     (ModestProtocolMgr *self);
const GSList*   modest_protocol_mgr_get_security_protocols  (ModestProtocolMgr *self);
const GSList*   modest_protocol_mgr_get_auth_protocols      (ModestProtocolMgr *self);

gboolean modest_protocol_mgr_protocol_is_valid (ModestProtocolMgr *self, const gchar* proto,
						ModestProtocolType type);

G_END_DECLS
#endif /* __MODEST_PROTOCOL_MGR_H__ */


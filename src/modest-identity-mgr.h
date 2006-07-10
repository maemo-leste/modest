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


/* modest-identity-mgr.h */

#ifndef __MODEST_IDENTITY_MGR_H__
#define __MODEST_IDENTITY_MGR_H__

#include <glib-object.h>
#include "modest-conf.h"
#include "modest-identity-keys.h"
#include "modest-proto.h"

G_BEGIN_DECLS
/* convenience macros */
#define MODEST_TYPE_IDENTITY_MGR             (modest_identity_mgr_get_type())
#define MODEST_IDENTITY_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_IDENTITY_MGR,ModestIdentityMgr))
#define MODEST_IDENTITY_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_IDENTITY_MGR,GObject))
#define MODEST_IS_IDENTITY_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_IDENTITY_MGR))
#define MODEST_IS_IDENTITY_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_IDENTITY_MGR))
#define MODEST_IDENTITY_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_IDENTITY_MGR,ModestIdentityMgrClass))
typedef struct _ModestIdentityMgr ModestIdentityMgr;
typedef struct _ModestIdentityMgrClass ModestIdentityMgrClass;


struct _ModestIdentityMgr {
	GObject parent;
	/* insert public members, if any */
};

struct _ModestIdentityMgrClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestIdentityMgr* obj); */
};


/**
 * modest_ui_get_type:
 *
 * get the GType for ModestIdentityMgr
 *
 * Returns: the GType
 */
GType modest_identity_mgr_get_type (void) G_GNUC_CONST;


/**
 * modest_identity_mgr_new:
 * @modest_conf: a ModestConf instance
 *
 * Returns: a new ModestIdentityMgr, or NULL in case of error
 */
GObject * modest_identity_mgr_new (ModestConf * modest_conf);


/**
 * modest_identity_mgr_add_identity:
 * @self: a ModestIdentityMgr instance
 * @name: the name (id) for the identity
 * @realname: the real name of the user
 * @email: the user's email address which is used when sending email
 * @replyto: the default replyto address
 * @signature: the signature for this identity
 * @use_signature: whether to use this signature instead of the default one
 * @id_via: the transport to send emails for this identity via
 * @use_id_via: whether to use this via instead of the default one
 *
 * add a user identity to the configuration
 *
 * Returns: TRUE if  succeeded, FALSE otherwise,
 */
gboolean modest_identity_mgr_add_identity (ModestIdentityMgr * self,
					   const gchar * name,
					   const gchar * realname,
					   const gchar * email,
					   const gchar * replyto,
					   const gchar * signature,
					   const gboolean use_signature,
					   const gchar * id_via,
					   const gboolean use_id_via);


/**
 * modest_identity_mgr_remove_identity:
 * @self: a ModestIdentityMgr instance
 * @name: the name of the identity to remove
 * @err: a GError ptr, or NULL to ignore.
 *
 * remove identity from the configuration
 * the identity with @name should exist
 *
 * Returns: TRUE if the removal succeeded, FALSE otherwise,
 * @err gives details in case of error
 */
gboolean modest_identity_mgr_remove_identity (ModestIdentityMgr * self,
											  const gchar * name,
											  GError ** err);


/**
 * modest_identity_mgr_identity_names:
 * @self: a ModestIdentityMgr instance
 * @err: a GError ptr, or NULL to ignore.
 *
 * list all identities
 *
 * Returns: a newly allocated list of identities, or NULL in case of error or
 * if there are no identities. The caller must free the returned GSList
 * @err gives details in case of error
 */
GSList *modest_identity_mgr_identity_names (ModestIdentityMgr * self,
											GError ** err);


/**
 * modest_identity_mgr_identity_exists:
 * @self: a ModestIdentityMgr instance
 * @err: a GError ptr, or NULL to ignore.
 *
 * check whether identity @name exists
 *
 * Returns: TRUE if the identity exists, FALSE otherwise (or in case of error)
 * @err gives details in case of error
 */
gboolean modest_identity_mgr_identity_exists (ModestIdentityMgr * self,
											  const gchar * name,
											  GError ** err);


/* identity specific functions */

/**
 * modest_identity_mgr_get_identity_string:
 * @self: a ModestIdentityMgr instance
 * @name: the name of the identity
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 *
 * get a config string from an identity
 *
 * Returns: a newly allocated string with the value for the key,
 * or NULL in case of error. @err gives details in case of error
 */
gchar *modest_identity_mgr_get_identity_string (ModestIdentityMgr * self,
												const gchar * name,
												const gchar * key,
												GError ** err);


/**
 * modest_identity_mgr_get_identity_int:
 * @self: a ModestIdentityMgr instance
 * @name: the name of the identity
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 *
 * get a config int from an identity
 *
 * Returns: an integer with the value for the key, or -1 in case of
 * error (but of course -1 does not necessarily imply an error)
 * @err gives details in case of error
 */
gint modest_identity_mgr_get_identity_int (ModestIdentityMgr * self,
										   const gchar * name,
										   const gchar * key,
										   GError ** err);


/**
 * modest_identity_mgr_get_identity_bool:
 * @self: a ModestIdentityMgr instance
 * @name: the name of the identity
 * @key: the key of the value to retrieve
 * @err: a GError ptr, or NULL to ignore.
 *
 * get a config boolean from an identity
 *
 * Returns: an boolean with the value for the key, or FALSE in case of
 * error (but of course FALSE does not necessarily imply an error)
 * @err gives details in case of error
 */
gboolean modest_identity_mgr_get_identity_bool (ModestIdentityMgr * self,
												const gchar * name,
												const gchar * key,
												GError ** err);


/**
 * modest_identity_mgr_set_identity_string:
 * @self: a ModestIdentityMgr instance
 * @name: the name of the identity
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL to ignore.
 *
 * set a config string for an identity
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean modest_identity_mgr_set_identity_string (ModestIdentityMgr *
												  self,
												  const gchar * name,
												  const gchar * key,
												  const gchar * val,
												  GError ** err);


/**
 * modest_identity_mgr_set_identity_int:
 * @self: a ModestIdentityMgr instance
 * @name: the name of the identity
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL to ignore.
 *
 * set a config int for an identity
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean modest_identity_mgr_set_identity_int (ModestIdentityMgr * self,
											   const gchar * name,
											   const gchar * key,
											   gint val, GError ** err);


/**
 * modest_identity_mgr_set_identity_bool:
 * @self: a ModestIdentityMgr instance
 * @name: the name of the identity
 * @key: the key of the value to set
 * @val: the value to set
 * @err: a GError ptr, or NULL to ignore.
 *
 * set a config bool for an identity
 *
 * Returns: TRUE if setting the value succeeded, or FALSE in case of error.
 * @err gives details in case of error
 */
gboolean modest_identity_mgr_set_identity_bool (ModestIdentityMgr * self,
												const gchar * name,
												const gchar * key,
												gboolean val,
												GError ** err);


G_END_DECLS
#endif /* __MODEST_IDENTITY_MGR_H__ */

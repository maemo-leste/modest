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

#ifndef __MODEST_PRESETS_H__
#define __MODEST_PRESETS_H__

#include <glib.h>

struct _ModestPresets {
/* private data: don't touch */
	GKeyFile *keyfile;
};
typedef struct _ModestPresets ModestPresets;

typedef enum _ModestPresetsServerType {
	MODEST_PRESETS_SERVER_TYPE_NONE,
	MODEST_PRESETS_SERVER_TYPE_IMAP,
	MODEST_PRESETS_SERVER_TYPE_POP,
	MODEST_PRESETS_SERVER_TYPE_SMTP
} ModestPresetsServerType;

/** These are flags, which should be ORed.
 */
typedef enum _ModestPresetsSecurity {
	MODEST_PRESETS_SECURITY_NONE = 0,
	MODEST_PRESETS_SECURITY_APOP = 1 << 0,
	MODEST_PRESETS_SECURITY_SECURE_SMTP = 1 << 1,
	MODEST_PRESETS_SECURITY_SECURE_INCOMING = 1 << 2
} ModestPresetsSecurity;

/* typedef enum _ModestPresetsInfo ModestPresetsInfo; */


/**
 * modest_presets_new:
 * @presetfile: the full path to the file with presets (in GKeyFile format)
 * 
 * make a new ModestPresets instance
 *
 * Returns: a new ModestPresets instance, or NULL in case of error.
 */
ModestPresets*            modest_presets_new             (const gchar *presetfile);


/**
 * modest_presets_get_providers:
 * @self: a valid ModestPresets instance
 * @mcc: limit the search to providers with this mcc (Mobile Country Code),
 *       or 0  to get all
 * @include_globals: include (global) providers without MCC (such as GMail, Yahoo)
 * @provider_ids: Output parameter, which will be set to a newly allocated array of strings, or NULL in case of error.
 * 
 * get a list of providers matching certian criteria
 *
 * Returns: The provider names, as a newly allocated array of strings, or NULL in case of error
 * should be freed with g_strfreev
 * 
 **/
gchar **         modest_presets_get_providers   (ModestPresets *self, guint mcc, 
						 gboolean include_globals, gchar ***provider_ids);

/**
 * modest_presets_get_server:
 * @self: a valid ModestPresets instance
 * @provider_id: ID of the provider 
 * @incoming_server: get the incoming mailserver if TRUE, get the
 * outgoing server otherwise
 *
 * get the name of a incoming or outgoing mailserver
 * 
 * Returns: a newly allocated string with the servername, or NULL in case
 * of error, or server not found. (FIXME). Note that if the (incoming) server uses a
 * non-standard port, the port number is appended to the name, eg. pop.foo.fi:995
 */
gchar *                   modest_presets_get_server      (ModestPresets *self,
							  const gchar *provider_id,
							  gboolean incoming_server);
							  
/**
 * modest_presets_get_domain:
 * @self: a valid ModestPresets instance
 * @provider_id: ID of the provider 
 *
 * Get the name of the most-used domain for theis provider. For instance. hotmail.com
 * 
 * Returns: a newly allocated string with the domain name, or NULL in case
 * of error.
 */
gchar *                   modest_presets_get_domain      (ModestPresets *self,
							  const gchar *provider_id);
							  
/**
 * modest_presets_get_info_server_type:
 * @self: a valid ModestPresets instance
 * @provider_id: ID of the provider 
 * @incoming_server: get the incoming mailserver if TRUE, get the
 * outgoing server otherwise
 *
 * get information about some incoming or outgoing mailserver
 *
 * Returns: a ModestPresetsServerType with the required information
 */
ModestPresetsServerType          modest_presets_get_info_server_type (ModestPresets *self,
						    const gchar *provider_id,
						    gboolean incoming_server);

/**
 * modest_presets_get_info_server_security:
 * @self: a valid ModestPresets instance
 * @provider_id: ID of the provider 
 * @incoming_server: get the incoming mailserver if TRUE, get the
 * outgoing server otherwise
 *
 * get information about some incoming or outgoing mailserver
 *
 * Returns: ModestPresetsSecurity ORable flags with the required information
 */					    
ModestPresetsSecurity          modest_presets_get_info_server_security (ModestPresets *self,
						    const gchar *provider_id,
						    gboolean incoming_server);

/**
 * modest_presets_destroy:
 * @self: a valid ModestPresets instance (ie. must not be NULL)
 *
 * destroy ModestPresets instance; this is required after you're done with it.
 */
void                      modest_presets_destroy         (ModestPresets *self);


#endif /*__MODEST_PRESETS__*/



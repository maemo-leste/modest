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

typedef enum _ModestPresetsInfo {
	/* two bits for the server type */
	MODEST_PRESETS_INFO_NONE             = 0x0000,
	MODEST_PRESETS_INFO_IMAP             = 0x0001,
	MODEST_PRESETS_INFO_POP              = 0x0002,
	MODEST_PRESETS_INFO_SMTP             = 0x0003,

	/* one bit for each of these */
	MODEST_PRESETS_INFO_APOP             = 0x0004,
	MODEST_PRESETS_INFO_SECURE_SMTP      = 0x0008,
	MODEST_PRESETS_INFO_SECURE_INCOMING  = 0x000f	
} ModestPresetsInfo;

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
 *       or <0 to get all
 * @include_globals: include providers without MCC (such as GMail, Yahoo) if @mcc != 0?
 * 
 * get a list of providers
 *
 * Returns: a newly allocated array of strings, or NULL in case of error
 * should be freed with g_strvfree
 * 
 **/
gchar **                  modest_presets_get_providers   (ModestPresets *self, gint mcc,
							  gboolean include_globals);

/**
 * modest_presets_get_server:
 * @self: a valid ModestPresets instance
 * @provider: name of the provider 
 * @incoming: get the incoming mailserver if TRUE, get the outgoing server otherwise
 *
 * get the name of a incoming or outgoing mailserver
 * 
 * Returns: a newly allocated string with the servername, or NULL in case
 * of error, or server not found. (FIXME). Note that if the (incoming) server uses a
 * non-standard port, the port number is appended to the name, eg. pop.foo.fi:995
 */
gchar *                   modest_presets_get_server      (ModestPresets *self,
							  const gchar *provider,
							  gboolean incoming_server);
/**
 * modest_presets_get_info:
 * @self: a valid ModestPresets instance
 * @provider: name of the provider 
 * @incoming: get the incoming mailserver if TRUE, get the outgoing server otherwise
 *
 * get information about some incoming or outgoing mailserver
 *
 * Returns: a ModestPresetsInfo with the required information
 */
ModestPresetsInfo          modest_presets_get_info (ModestPresets *self,
						    const gchar *provider,
						    gboolean incoming_server);

/**
 * modest_presets_destroy:
 * @self: a valid ModestPresets instance
 *
 * destroy ModestPresets instance; this is required after you're done with it.
 *
 */
void                      modest_presets_destroy         (ModestPresets *self);


#endif /*__MODEST_PRESETS__*/



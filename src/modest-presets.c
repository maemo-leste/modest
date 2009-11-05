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

#include <string.h> /* for strcmp */
#include <modest-protocol-registry.h>
#include <modest-runtime.h>
#include "modest-presets.h"
#include <stdio.h>

/* Include config.h so that _() works: */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODEST_PRESETS_KEY_NAME                "Name"
#define MODEST_PRESETS_KEY_DOMAIN              "Domain"
#define MODEST_PRESETS_KEY_MCC                 "MCC"
#define MODEST_PRESETS_KEY_INCOMING            "IncomingMailServer"
#define MODEST_PRESETS_KEY_INCOMING_SECURITY   "IncomingSecurity"
#define MODEST_PRESETS_KEY_OUTGOING            "OutgoingMailServer"
#define MODEST_PRESETS_KEY_MAILBOX_TYPE        "MailboxType"
#define MODEST_PRESETS_KEY_APOP                "APOPSecureLogin"
#define MODEST_PRESETS_KEY_SECURE_SMTP         "SecureSmtp"
#define MODEST_PRESETS_KEY_SMTP_PORT           "SmtpPort"
						    

ModestPresets*
modest_presets_new (const gchar *presetfile)
{
	ModestPresets *presets = NULL;
	GError        *err     = NULL;
	
	g_return_val_if_fail (presetfile, NULL);
	
	presets = g_new (ModestPresets, 1);
	presets->keyfile = g_key_file_new ();

	if (!presets->keyfile) {
		g_printerr ("modest: cannot instantiate GKeyFile\n");
		g_free (presets);
		return NULL;
	}
	
	if (!g_key_file_load_from_file (presets->keyfile, presetfile,
					G_KEY_FILE_NONE, &err)) {
		g_printerr ("modest: cannot open keyfile from %s:\n  %s\n", presetfile,
			    err ? err->message : "unknown reason");
		g_error_free (err);
		g_free (presets);
		return NULL;
	}

	return presets;
}

/* cluster mcc's, based on the list
 * http://en.wikipedia.org/wiki/Mobile_country_code
 *
 * This function will return the "effective mcc", which is the
 * normalized mcc for a country - ie. even if the there are multiple
 * entries for the United States with various mcc's, this function will
 * always return 310, even if the real mcc parsed would be 314.
 */
static int
effective_mcc (gint mcc)
{
	switch (mcc) {
	case 405: return 404; /* india */
	case 441: return 440; /* japan */
	case 348: /* NOTE: see below */
	case 235: return 234; /* united kingdom */
	case 289: return 282; /* georgia */
	case 549: /* NOTE: see below */
	case 311:
	case 312:
	case 313:
	case 314:
	case 315:
	case 316: return 310; /* united states */
	default:  return mcc;
	}
	/* NOTE: 348 for UK and 549 for US are not correct, but we do
	   a workaround here as changing operator-wizard package is
	   more difficult */
}

gchar**
modest_presets_get_providers  (ModestPresets *self, guint mcc,
			       gboolean include_globals, gchar ***provider_ids)
{
	gchar **all_providers = NULL;
	gchar **all_provider_ids = NULL;
	gchar **filtered  = NULL;
	gchar **filtered_ids = NULL;
	GError *err       = NULL;
	guint i, j, len;

	g_return_val_if_fail (self && self->keyfile, NULL);

	/* Get all the provider IDs: */
	all_provider_ids = g_key_file_get_groups (self->keyfile, NULL);
	len = g_strv_length(all_provider_ids);

	/* Get the names for all these providers: */
	all_providers = g_new0(gchar*, len + 1); /* Provider names. */
	for (i=0; i != len; ++i) {
		const gchar * provider_id = all_provider_ids[i];
		if(provider_id) {
			gchar* name = g_key_file_get_string(self->keyfile, provider_id, 
				MODEST_PRESETS_KEY_NAME, NULL);

			/* Be forgiving of missing names.
			 * If we use NULL then we will null-terminate the array.
			 */
			if(!name)
				name = g_strdup("");

			all_providers[i] = name;
		}
		else
			all_providers[i] = NULL;
	};

	/* return *all* providers? */
	/*
	if (mcc == 0 && include_globals) {
		*provider_ids = all_provider_ids;
		return all_providers;
	}
	*/

	/* nope: filter them */

	filtered = g_new0(gchar*, len + 1); /* Provider names. */
	filtered_ids = g_new0(gchar*, len + 1); /* Provider IDs */

	for (i=0, j=0; i != len; ++i) {

		int this_mcc;
		this_mcc = g_key_file_get_integer (self->keyfile, all_provider_ids[i],
						   MODEST_PRESETS_KEY_MCC, &err);
		if (err) {
			g_strfreev (all_providers);
			g_strfreev (all_provider_ids);
			g_strfreev (filtered);
			g_strfreev (filtered_ids);

			g_printerr ("modest: error parsing keyfile: %s\n", err->message);
			g_error_free (err);

			return NULL;
		}

		if (this_mcc == mcc ||
		    effective_mcc (this_mcc) == effective_mcc (mcc) ||
		    (this_mcc == 0 && include_globals)) {
			filtered[j]   = all_providers[i];
			filtered_ids[j]   = all_provider_ids[i];
			++j;
			filtered[j] = NULL; /* the array must be NULL-terminated */
			filtered_ids[j] = NULL; /* the array must be NULL-terminated */

			all_providers[i]  = NULL; /*  g_strfreev: leave it alone */
			all_provider_ids[i]  = NULL; /*  g_strfreev: leave it alone */
		}
	}
	g_strfreev (all_providers);
	g_strfreev (all_provider_ids);
	
	*provider_ids = filtered_ids;
	return filtered;
}


gchar*
modest_presets_get_server (ModestPresets *self, const gchar *provider_id,
			   gboolean incoming_server)
{	
	g_return_val_if_fail (self && self->keyfile, NULL);
	g_return_val_if_fail (provider_id, NULL);

	return g_key_file_get_string (self->keyfile, provider_id, 
				      incoming_server ?
				      MODEST_PRESETS_KEY_INCOMING :
				      MODEST_PRESETS_KEY_OUTGOING,
				      NULL);
}

gchar *
modest_presets_get_domain      (ModestPresets *self,
				const gchar *provider_id)
{	
	g_return_val_if_fail (self && self->keyfile, NULL);
	g_return_val_if_fail (provider_id, NULL);

	return g_key_file_get_string (self->keyfile, provider_id, 
				      MODEST_PRESETS_KEY_DOMAIN,
				      NULL);
}		




ModestProtocolType
modest_presets_get_info_server_type (ModestPresets *self,
				     const gchar *provider_id,
				     gboolean incoming_server)
{
	ModestProtocolType protocol_type = MODEST_PROTOCOL_REGISTRY_TYPE_INVALID;
	ModestProtocolRegistry *protocol_registry;
	ModestProtocol *protocol;
	gchar *val = NULL;
	
	g_return_val_if_fail (self && self->keyfile, 0);
	protocol_registry = modest_runtime_get_protocol_registry ();

	if (incoming_server) {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_INCOMING, NULL);
		if (!val)
			return protocol_type;
		
		g_free (val);
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_MAILBOX_TYPE,NULL);
		
		protocol = modest_protocol_registry_get_protocol_by_name (protocol_registry, MODEST_PROTOCOL_REGISTRY_STORE_PROTOCOLS, val);
		if (protocol == NULL)
			return protocol_type;
		protocol_type = modest_protocol_get_type_id (protocol);
	} else {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_OUTGOING, NULL);
		if (!val)
			return protocol_type;

		protocol_type = MODEST_PROTOCOLS_TRANSPORT_SMTP;
	}
	g_free (val);

	/* debug: */
/* 	g_debug ("provider id: %s, server type: %d", provider_id, info); */
	return protocol_type;
}



ModestProtocolType
modest_presets_get_info_server_security (ModestPresets *self, const gchar *provider_id,
					 gboolean incoming_server)
{
	ModestProtocolType protocol_type = MODEST_PROTOCOLS_CONNECTION_NONE;
	gchar *val = NULL;
	
	g_return_val_if_fail (self && self->keyfile, MODEST_PROTOCOLS_CONNECTION_NONE);

	if (incoming_server) {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_INCOMING, NULL);
		if (val) {
			g_free (val);

			val = g_key_file_get_string (self->keyfile, provider_id,
						     MODEST_PRESETS_KEY_INCOMING_SECURITY, NULL);
			if (val && strcmp (val, "1") == 0) {
				protocol_type = MODEST_PROTOCOLS_CONNECTION_TLS;
			} else if (val && strcmp (val, "2") == 0) {
				protocol_type = MODEST_PROTOCOLS_CONNECTION_SSL;
			} else if (val && (strcmp (val, "tls") == 0)) {
				protocol_type = MODEST_PROTOCOLS_CONNECTION_TLS;
			} else if (val && (strcmp (val, "ssl") == 0)) {
				protocol_type = MODEST_PROTOCOLS_CONNECTION_SSL;
			}
			g_free (val);
		}
	} else /* outgoing: */ {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_OUTGOING, NULL);
		if (val) {
			g_free (val);

			val = g_key_file_get_string (self->keyfile, provider_id,
						     MODEST_PRESETS_KEY_SECURE_SMTP, NULL);
			if (val && strcmp(val,"true") == 0)
				protocol_type = MODEST_PROTOCOLS_CONNECTION_SSL;
			else if (val && strcmp (val, "ssl") == 0)
				protocol_type = MODEST_PROTOCOLS_CONNECTION_SSL;
			else if (val && strcmp (val, "2") == 0)
				protocol_type = MODEST_PROTOCOLS_CONNECTION_SSL;
			else if (val && strcmp (val, "tls") == 0)
				protocol_type = MODEST_PROTOCOLS_CONNECTION_TLS;
			else if (val && strcmp (val, "1") == 0)
				protocol_type = MODEST_PROTOCOLS_CONNECTION_TLS;
			g_free(val);
		}
	}

	return protocol_type;
}

gboolean 
modest_presets_get_info_server_use_alternate_port (ModestPresets *self, const gchar *provider_id,
						   gboolean incoming_server)
{
	gboolean result = FALSE;
	gchar *val = NULL;
	
	g_return_val_if_fail (self && self->keyfile, MODEST_PROTOCOLS_CONNECTION_NONE);

	if (incoming_server) {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_INCOMING, NULL);
		if (val) {
			g_free (val);	

			val = g_key_file_get_string (self->keyfile, provider_id,
						     MODEST_PRESETS_KEY_INCOMING_SECURITY, NULL);
			if (val && (strcmp (val, "2") == 0)) {
				result = TRUE;
			}
			g_free (val);
		}
	} 

	return result;
}

ModestProtocolType
modest_presets_get_info_server_auth (ModestPresets *self, const gchar *provider_id,
					 gboolean incoming_server)
{
	ModestProtocolType protocol_type = MODEST_PROTOCOLS_AUTH_NONE;
	gchar *val = NULL;
	
	g_return_val_if_fail (self && self->keyfile, MODEST_PROTOCOLS_AUTH_NONE);

	if (incoming_server) {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_INCOMING, NULL);
		if (val) {
                        g_free (val);
                        val = g_key_file_get_string (self->keyfile, provider_id,
                                                     MODEST_PRESETS_KEY_APOP, NULL);
                        if (val && strcmp(val, "true") == 0)
				protocol_type = MODEST_PROTOCOLS_AUTH_PASSWORD;
                        g_free(val);

		}
	} else /* outgoing: */ {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_OUTGOING, NULL);
		if (val) {
			g_free (val);
			
			val = g_key_file_get_string (self->keyfile, provider_id,
						     MODEST_PRESETS_KEY_SECURE_SMTP, NULL);
			/* printf("debug: %s: provider_id=%s, secure-smtp val=%s\n", __FUNCTION__, provider_id, val); */
			if (val && strcmp(val,"true") == 0)
				protocol_type = MODEST_PROTOCOLS_AUTH_PASSWORD;
			g_free(val);
		}
	}

	return protocol_type;
}

/*
 * at the moment, this only for mac.com, which have a special SMTP port
 */
guint
modest_presets_get_port (ModestPresets *self, const gchar* provider_id,
			 gboolean incoming_server)
{
	guint port;
	
	g_return_val_if_fail (self && self->keyfile, 0);

	if (incoming_server)
		port = 0; /* not used yet */
	else 
		port = (guint)g_key_file_get_integer (self->keyfile, provider_id,
						      MODEST_PRESETS_KEY_SMTP_PORT, NULL);

	return port;
}




	
void
modest_presets_destroy (ModestPresets *self)
{
	if (!self)
		return;

	g_key_file_free (self->keyfile);
	self->keyfile = NULL;
	
	g_free (self);
}

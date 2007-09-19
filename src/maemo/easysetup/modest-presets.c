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
		
		if (this_mcc == mcc || (this_mcc == 0 && include_globals)) {
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




ModestPresetsServerType
modest_presets_get_info_server_type (ModestPresets *self,
				     const gchar *provider_id,
				     gboolean incoming_server)
{
	ModestPresetsServerType info = MODEST_PRESETS_SERVER_TYPE_NONE;
	gchar *val = NULL;
	
	g_return_val_if_fail (self && self->keyfile, 0);

	if (incoming_server) {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_INCOMING, NULL);
		if (!val)
			return info;
		
		g_free (val);
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_MAILBOX_TYPE,NULL);
		if (val && strcmp (val, "pop") == 0)
			info = MODEST_PRESETS_SERVER_TYPE_POP;
		if (val && strcmp (val, "imap") == 0)
			info = MODEST_PRESETS_SERVER_TYPE_IMAP;
	} else {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_OUTGOING, NULL);
		if (!val)
			return info;
		info = MODEST_PRESETS_SERVER_TYPE_SMTP;
	}
	g_free (val);

	/* debug: */
/* 	g_message ("provider id: %s, server type: %d", provider_id, info); */
	return info;
}



ModestPresetsSecurity
modest_presets_get_info_server_security (ModestPresets *self, const gchar *provider_id,
					 gboolean incoming_server)
{
	ModestPresetsSecurity info = MODEST_PRESETS_SECURITY_NONE;
	gchar *val = NULL;
	
	g_return_val_if_fail (self && self->keyfile, MODEST_PRESETS_SECURITY_NONE);

	if (incoming_server) {
		val = g_key_file_get_string (self->keyfile, provider_id,
					     MODEST_PRESETS_KEY_INCOMING, NULL);
		if (val) {
			g_free (val);	
			val = g_key_file_get_string (self->keyfile, provider_id,
						     MODEST_PRESETS_KEY_APOP, NULL);
			if (val && strcmp(val, "true") == 0)
				info |= MODEST_PRESETS_SECURITY_APOP;
			g_free(val);
			
			val = g_key_file_get_string (self->keyfile, provider_id,
						     MODEST_PRESETS_KEY_INCOMING_SECURITY, NULL);
			if (val && strcmp (val, "1") == 0) 
				info |= MODEST_PRESETS_SECURITY_SECURE_INCOMING;
			if (val && strcmp (val, "2") == 0) {
				info |= MODEST_PRESETS_SECURITY_SECURE_INCOMING;
				info |= MODEST_PRESETS_SECURITY_SECURE_INCOMING_ALTERNATE_PORT;
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
			/* printf("debug: %s: provider_id=%s, secure-smtp val=%s\n", __FUNCTION__, provider_id, val); */
			if (val && strcmp(val,"true") == 0)
				info |= MODEST_PRESETS_SECURITY_SECURE_SMTP;
			g_free(val);
		}
	}

	return info;
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

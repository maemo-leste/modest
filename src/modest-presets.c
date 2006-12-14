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

#define MODEST_PRESETS_KEY_MCC               "MCC"
#define MODEST_PRESETS_KEY_INCOMING          "IncomingMailServer"
#define MODEST_PRESETS_KEY_OUTGOING          "OutgoingMailServer"
#define MODEST_PRESETS_KEY_MAILBOX_TYPE      "MailboxType"
#define MODEST_PRESETS_KEY_MAILBOX_TYPE_POP  "pop"
#define MODEST_PRESETS_KEY_MAILBOX_TYPE_IMAP "imap"
#define MODEST_PRESETS_KEY_APOP              "APOPSecureLogin"
#define MODEST_PRESETS_KEY_SECURE_SMTP       "SecureSMTP"
#define MODEST_PRESETS_KEY_TRUE		     "true"

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
		g_printerr ("modest: cannot open keyfile: %s\n",
			    err ? err->message : "unknown reason");
		g_error_free (err);
		g_free (presets);
		return NULL;
	}

	return presets;
}

gchar**
modest_presets_get_providers  (ModestPresets *self, gint mcc, gboolean include_globals)
{
	gchar **providers = NULL;
	gchar **filtered  = NULL;
	
	g_return_val_if_fail (self && self->keyfile, NULL);

	providers = g_key_file_get_groups (self->keyfile, NULL);

	/* return *all* providers? */
	if (mcc < 0 && include_globals)
		return providers;

	/* nope: filter them instead */
	filtered = g_new(gchar*, g_strv_length(providers));

	if (filtered && providers) {
		int i = 0, j = 0;
		while (providers[i]) {

			int this_mcc;
			this_mcc = g_key_file_get_integer (self->keyfile, providers[i],
							   MODEST_PRESETS_KEY_MCC,
							   NULL);
			if (this_mcc == mcc || (this_mcc == 0 && include_globals)) {
				filtered[j++] = providers[i];
				providers[i] = NULL; /*  g_strfreev: leave it alone */
			}
			++i;
		}
	}
	g_strfreev (providers);
	
	return filtered;
}


gchar*
modest_presets_get_server (ModestPresets *self, const gchar *provider,
			   gboolean incoming_server)
{	
	g_return_val_if_fail (self && self->keyfile, NULL);
	g_return_val_if_fail (provider, NULL);

	return g_key_file_get_string (self->keyfile, provider, 
				      incoming_server ?
				      MODEST_PRESETS_KEY_INCOMING :
				      MODEST_PRESETS_KEY_OUTGOING,
				      NULL);
}


ModestPresetsInfo
modest_presets_get_info (ModestPresets *self, const gchar *provider, gboolean incoming_server)
{
	ModestPresetsInfo info = 0;
	gchar *val = NULL;
	
	g_return_val_if_fail (self && self->keyfile, 0);

	val = g_key_file_get_string (self->keyfile, provider,
					MODEST_PRESETS_KEY_INCOMING, NULL);
	if (val) {
		g_free (val);
		val = g_key_file_get_string (self->keyfile, provider,
					     MODEST_PRESETS_KEY_MAILBOX_TYPE, NULL);
		if (strcmp (val, MODEST_PRESETS_KEY_MAILBOX_TYPE_POP) == 0)
			info |= MODEST_PRESETS_INFO_POP;
		if (strcmp (val, MODEST_PRESETS_KEY_MAILBOX_TYPE_IMAP) == 0)
			info |= MODEST_PRESETS_INFO_IMAP;
		g_free (val);

		val = g_key_file_get_string (self->keyfile, provider,
					     MODEST_PRESETS_KEY_APOP, NULL);
		if (val && strcmp(val, MODEST_PRESETS_KEY_TRUE) == 0)
			info |= MODEST_PRESETS_INFO_APOP;
		g_free(val);
	}
		

	val = g_key_file_get_string (self->keyfile, provider,
				     MODEST_PRESETS_KEY_OUTGOING, NULL);
	if (val) {
		g_free (val);
		info |= MODEST_PRESETS_INFO_SMTP;
		
		val = g_key_file_get_string (self->keyfile, provider,
					     MODEST_PRESETS_KEY_SECURE_SMTP, NULL);
		if (val && strcmp(val,MODEST_PRESETS_KEY_TRUE) == 0)
			info |= MODEST_PRESETS_INFO_SECURE_SMTP;
		g_free(val);
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

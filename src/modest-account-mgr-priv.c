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


#include <modest-account-mgr-priv.h>
#include <modest-defs.h>
#include <string.h>
#include <modest-conf.h>
#include <stdio.h>

gchar*
_modest_account_mgr_account_from_key (const gchar *key, gboolean *is_account_key, gboolean *is_server_account)
{
	/* Initialize input parameters: */
	if (is_account_key)
		*is_account_key = FALSE;

	if (is_server_account)
		*is_server_account = FALSE;

	const gchar* account_ns        = MODEST_ACCOUNT_NAMESPACE "/";
	const gchar* server_account_ns = MODEST_SERVER_ACCOUNT_NAMESPACE "/";
	gchar *cursor;
	gchar *account = NULL;

	/* determine whether it's an account or a server account,
	 * based on the prefix */
	if (g_str_has_prefix (key, account_ns)) {

		if (is_server_account)
			*is_server_account = FALSE;
		
		account = g_strdup (key + strlen (account_ns));

	} else if (g_str_has_prefix (key, server_account_ns)) {

		if (is_server_account)
			*is_server_account = TRUE;
		
		account = g_strdup (key + strlen (server_account_ns));	
	} else
		return NULL;

	/* if there are any slashes left in the key, it's not
	 * the toplevel entry for an account
	 */
	cursor = strstr(account, "/");
	
	if (is_account_key && cursor)
		*is_account_key = TRUE;

	/* put a NULL where the first slash was */
	if (cursor)
		*cursor = '\0';

	if (account) {
		/* The key is an escaped string, so unescape it to get the actual account name: */
		gchar *unescaped_name = modest_conf_key_unescape (account);
		g_free (account);
		return unescaped_name;
	} else
		return NULL;
}



/* must be freed by caller */
gchar *
_modest_account_mgr_get_account_keyname (const gchar *account_name, const gchar * name, gboolean server_account)
{
	gchar *retval = NULL;
	
	gchar *namespace = server_account ? MODEST_SERVER_ACCOUNT_NAMESPACE : MODEST_ACCOUNT_NAMESPACE;
	
	if (!account_name)
		return g_strdup (namespace);
	
	/* Always escape the conf keys, so that it is acceptable to gconf: */
	gchar *escaped_account_name = account_name ? modest_conf_key_escape (account_name) : NULL;
	gchar *escaped_name =  name ? modest_conf_key_escape (name) : NULL;

	if (escaped_account_name && escaped_name)
		retval = g_strconcat (namespace, "/", escaped_account_name, "/", escaped_name, NULL);
	else if (escaped_account_name)
		retval = g_strconcat (namespace, "/", escaped_account_name, NULL);

	/* Sanity check: */
	if (!modest_conf_key_is_valid (retval)) {
		g_warning ("%s: Generated conf key was invalid: %s", __FUNCTION__, retval);
		g_free (retval);
		retval = NULL;
	}

	g_free (escaped_name);
	g_free (escaped_account_name);

	return retval;
}

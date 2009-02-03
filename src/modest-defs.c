/* Copyright (c) 2009, Nokia Corporation
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

#include <modest-defs.h>
#include <glib.h>

static GHashTable *hash_dir = NULL;
static GHashTable *hash_namespace = NULL;

static gchar *modest_dir = NULL;
static gchar *modest_namespace = NULL;

static void init_hash_dir (void);
static void init_hash_namespace (void);

const gchar *
modest_defs_dir (const gchar *string)
{
	const gchar *ret_value;

	if (hash_dir == NULL) init_hash_dir ();

	if (string == NULL)
		return (const gchar *) modest_dir;

	ret_value = (const gchar *) g_hash_table_lookup (hash_dir, string);
	if (ret_value == NULL) {
		ret_value = (const gchar *) g_strconcat (modest_dir, string, NULL);
		g_hash_table_insert (hash_dir, (gpointer) string, (gpointer) ret_value);
	}
	return ret_value;
}

const gchar *
modest_defs_namespace (const gchar *string)
{
	const gchar *ret_value;

	if (hash_namespace == NULL) init_hash_namespace ();

	if (string == NULL)
		return (const gchar *) modest_namespace;

	ret_value = (const gchar *) g_hash_table_lookup (hash_namespace, string);
	if (ret_value == NULL) {
		ret_value = (const gchar *) g_strconcat (modest_namespace, string, NULL);
		g_hash_table_insert (hash_namespace, (gpointer) string, (gpointer) ret_value);
	}
	return ret_value;
}

static void 
init_hash_dir (void)
{
	const gchar *env_value;

	if (hash_dir != NULL)
		return;

	hash_dir = g_hash_table_new (g_str_hash, g_str_equal);

	env_value = g_getenv (MODEST_DIR_ENV);

	if (env_value == NULL || env_value[0] == '\0') {
		env_value = MODEST_DEFAULT_DIR;
	}
	modest_dir = g_strdup (env_value);
}

static void 
init_hash_namespace (void)
{
	const gchar *env_value;

	if (hash_namespace != NULL)
		return;

	hash_namespace = g_hash_table_new (g_str_hash, g_str_equal);

	env_value = g_getenv (MODEST_NAMESPACE_ENV);

	if (env_value == NULL || env_value[0] == '\0') {
		env_value = MODEST_DEFAULT_NAMESPACE;
	}
	modest_namespace = g_strdup (env_value);
}


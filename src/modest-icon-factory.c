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


/* modest-icon-factory.c */

#include <string.h>
#include "modest-icon-factory.h"

static GHashTable *icon_hash = NULL;

static
gboolean equal_func (const gchar *s1, const gchar *s2)
{
	return strcmp (s1, s2) == 0;
}

static
void free_pixbuf (GdkPixbuf *pixbuf)
{
	if (pixbuf)
		g_object_unref (G_OBJECT(pixbuf));
}


void
modest_icon_factory_init   (void)
{
	if (icon_hash) {
		g_printerr ("modest: modest_icon_factory_init " 
			    "should be called only once\n");
		return;
	}

	icon_hash = g_hash_table_new_full (g_str_hash,
					   (GEqualFunc)equal_func,
					   (GDestroyNotify)g_free,
					   (GDestroyNotify)free_pixbuf);
}


void
modest_icon_factory_uninit (void)
{
	if (!icon_hash) {
		g_printerr ("modest: modest_icon_factory_uninit "
			   "must only be called with initialized "
			   "ModestIconFactories\n");
		return;
	}

	g_hash_table_destroy (icon_hash);
	icon_hash = NULL;
}



GdkPixbuf*
modest_icon_factory_get_icon (const gchar *name)
{
	GError *err = NULL;
	GdkPixbuf *pixbuf;
	gpointer orig_key;
	
	if (!icon_hash) {
		g_printerr ("modest: ModestIconFactory must be initialized first\n");
		return NULL;
	}

	/* is it already in the hashtable?
	 * note: this can be NULL
	 */
	if (!g_hash_table_lookup_extended (icon_hash, name, &orig_key,
					   (gpointer*)&pixbuf)) {
		pixbuf = gdk_pixbuf_new_from_file (name, &err);
		if (!pixbuf) {
			g_printerr ("modest: error in icon factory: %s\n", err->message);
			g_error_free (err);
		}
		/* if we cannot find it, we still insert, so we get the error
		 * only once */
		g_hash_table_insert (icon_hash, g_strdup(name),
				     (gpointer)pixbuf);
	}
	return pixbuf;
}

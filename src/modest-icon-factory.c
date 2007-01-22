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

#include <modest-icon-factory.h>
#include <modest-tny-platform-factory.h>

static GHashTable*
get_icon_cache (void)
{
	TnyPlatformFactory *fakt;
	ModestCacheMgr     *cache_mgr;

	fakt = modest_tny_platform_factory_get_instance ();
	
	cache_mgr =  modest_tny_platform_factory_get_cache_mgr_instance
		(MODEST_TNY_PLATFORM_FACTORY(fakt));

	return modest_cache_mgr_get_cache (cache_mgr,
					   MODEST_CACHE_MGR_CACHE_TYPE_PIXBUF);
}


GdkPixbuf*
modest_icon_factory_get_icon (const gchar *name)
{
	GError *err = NULL;
	GdkPixbuf *pixbuf;
	gpointer orig_key;
	static GHashTable *icon_cache = NULL;
	
	g_return_val_if_fail (name, NULL);

	if (G_UNLIKELY(!icon_cache))
		icon_cache = get_icon_cache ();
	
	if (!icon_cache || !g_hash_table_lookup_extended (icon_cache, name, &orig_key,
							  (gpointer*)&pixbuf)) {
		pixbuf = gdk_pixbuf_new_from_file (name, &err);
		if (!pixbuf) {
			g_printerr ("modest: error in icon factory while loading '%s': %s\n",
				    name, err->message);
			g_error_free (err);
		}
		/* if we cannot find it, we still insert (if we have a cache), so we get the error
		 * only once */
		if (icon_cache)
			g_hash_table_insert (icon_cache, g_strdup(name),(gpointer)pixbuf);
	}
	return pixbuf;
}



GdkPixbuf*
modest_icon_factory_get_icon_at_size (const gchar *name, guint width, guint height)
{
	/* FIXME, somehow, cache scaled icons as well... */
	GError *err = NULL;
	GdkPixbuf *pixbuf = NULL;
	static GHashTable *icon_cache = NULL;
	
	g_return_val_if_fail (name, NULL);

	if (G_UNLIKELY(!icon_cache))
		icon_cache = get_icon_cache ();
	
	pixbuf = gdk_pixbuf_new_from_file_at_size (name, width, height, &err);
	if (!pixbuf) {
		g_printerr ("modest: error in icon factory while loading '%s'@(%dx%d): %s\n",
			    name, width, height, err->message);
		g_error_free (err);
	}
	
	/* we insert it, so it will be freed... FIXME... */
	if (pixbuf && icon_cache)
		g_hash_table_insert (icon_cache, g_strdup_printf ("%s-%d-%d",name,width,height),
				     (gpointer)pixbuf);
	return pixbuf;
}

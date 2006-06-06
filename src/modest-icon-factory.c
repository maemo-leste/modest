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
		g_warning ("modest_icon_factory_init "
			   "should be called only once");
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
		g_warning ("modest_icon_factory_uninit "
			   "must only be called with initialized "
			   "ModestIconFactories");
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
		g_warning ("ModestIconFactory must be initialized first");
		return NULL;
	}

	/* is it already in the hashtable?
	 * note: this can be NULL
	 */
	if (!g_hash_table_lookup_extended (icon_hash, name, &orig_key,
					   (gpointer*)&pixbuf)) {
		pixbuf = gdk_pixbuf_new_from_file (name, &err);
		if (!pixbuf) {
			g_warning (err->message);
			g_error_free (err);
		}
		/* if we cannot find it, we still insert, so we get the error
		 * only once */
		g_hash_table_insert (icon_hash, g_strdup(name),
				     (gpointer)pixbuf);
	}
	return pixbuf;
}

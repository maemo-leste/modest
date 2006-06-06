/* modest-icon-factory.h */

#ifndef __MODEST_ICON_FACTORY_H__
#define __MODEST_ICON_FACTORY_H__

#include <gtk/gtk.h>

/**
 * modest_icon_factory_init
 *
 * initialize the modest_icon_factory, which is a runtime-wide singleton
 * this should be called only once, before doing anything else with the icon
 * factory
 */
void modest_icon_factory_init   (void);


/**
 * modest_icon_factory_uninit
 *
 * uninitialize the modest_icon_factory. this should be done after the icon
 * factory is no longer needed.
 */
void modest_icon_factory_uninit (void);


/**
 * modest_icon_factory_get_icon:
 * @name: the filename of a certain icon
 *
 * Returns: a GdkPixBuf for this icon, or NULL in case of error
 * You should NOT unref or modify the pixbuf in any way
 */
GdkPixbuf* modest_icon_factory_get_icon (const gchar *name);

#endif /*__MODEST_ICON_FACTORY_H__ */

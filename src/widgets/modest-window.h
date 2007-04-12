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

#ifndef __MODEST_WINDOW_H__
#define __MODEST_WINDOW_H__

#include <glib-object.h>
#include <tny-account-store.h>

G_BEGIN_DECLS

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

/* 
 * admittedly, the ifdefs for gtk and maemo are rather ugly; still
 * this way is probably the easiest to maintain
 */
#ifdef MODEST_PLATFORM_GNOME
#include <gtk/gtkwindow.h>
typedef GtkWindow      ModestWindowParent;
typedef GtkWindowClass ModestWindowParentClass;
#endif /* MODEST_PLATFORM_GNOME */

#ifdef MODEST_PLATFORM_MAEMO
#include <hildon-widgets/hildon-window.h>
typedef HildonWindow      ModestWindowParent;
typedef HildonWindowClass ModestWindowParentClass;
#endif /*MODEST_PLATFORM_MAEMO */

/* convenience macros */
#define MODEST_TYPE_WINDOW             (modest_window_get_type())
#define MODEST_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_WINDOW,ModestWindow))
#define MODEST_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_WINDOW,GObject))
#define MODEST_IS_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_WINDOW))
#define MODEST_IS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_WINDOW))
#define MODEST_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_WINDOW,ModestWindowClass))

typedef struct _ModestWindow      ModestWindow;
typedef struct _ModestWindowClass ModestWindowClass;

struct _ModestWindow {
	 ModestWindowParent parent;
};

struct _ModestWindowClass {
	ModestWindowParentClass parent_class;

	/* virtual methods */
	void (*set_zoom_func) (ModestWindow *self, gdouble zoom);
	gdouble (*get_zoom_func) (ModestWindow *self);
	gboolean (*zoom_plus_func) (ModestWindow *self);
	gboolean (*zoom_minus_func) (ModestWindow *self);
};

/**
 * modest_window_get_type:
 *
 * get the #GType for #ModestWindow
 * 
 * Returns: the type
 */	
GType        modest_window_get_type    (void) G_GNUC_CONST;


/**
 * modest_window_get_active_account:
 * @self: a modest window instance
 * 
 * get the name of the active account
 * 
 * Returns: the active account name as a constant string
 */	
const gchar* modest_window_get_active_account (ModestWindow *self);



/**
 * modest_window_set_active_account:
 * @self: a modest window instance
 * @active_account: a new active account name for this window
 * 
 * set the active account for this window
 * 
 */	
void modest_window_set_active_account (ModestWindow *self, const gchar *active_account);

/**
 * modest_window_set_zoom:
 * @window: a #ModestWindow instance
 * @zoom: the zoom level (1.0 is no zoom)
 *
 * sets the zoom level of the window
 */
void            modest_window_set_zoom    (ModestWindow *window,
					   gdouble value);

/**
 * modest_window_get_zoom:
 * @window: a #ModestWindow instance
 *
 * gets the zoom of the window
 *
 * Returns: the current zoom value (1.0 is no zoom)
 */
gdouble         modest_window_get_zoom    (ModestWindow *window);

/**
 * modest_window_zoom_plus:
 * @window: a #ModestWindow
 *
 * increases one level the zoom.
 *
 * Returns: %TRUE if successful, %FALSE if increasing zoom is not available
 */
gboolean modest_window_zoom_plus (ModestWindow *window);

/**
 * modest_window_zoom_minus:
 * @window: a #ModestWindow
 *
 * decreases one level the zoom.
 *
 * Returns: %TRUE if successful, %FALSE if increasing zoom is not available
 */
gboolean modest_window_zoom_minus (ModestWindow *window);

G_END_DECLS

#endif /* __MODEST_WINDOW_H__ */

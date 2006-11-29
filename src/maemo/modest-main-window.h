/* modest-main-window.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_MAIN_WINDOW_H__
#define __MODEST_MAIN_WINDOW_H__

#include <hildon-widgets/hildon-window.h>
#include <modest-widget-factory.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAIN_WINDOW             (modest_main_window_get_type())
#define MODEST_MAIN_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindow))
#define MODEST_MAIN_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MAIN_WINDOW,GtkWindow))
#define MODEST_IS_MAIN_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_IS_MAIN_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_MAIN_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindowClass))

typedef struct _ModestMainWindow      ModestMainWindow;
typedef struct _ModestMainWindowClass ModestMainWindowClass;

struct _ModestMainWindow {
	 HildonWindow parent;
	/* insert public members, if any */
};

struct _ModestMainWindowClass {
	HildonWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestMainWindow* obj); */
};

/* member functions */
GType        modest_main_window_get_type    (void) G_GNUC_CONST;


GtkWidget*   modest_main_window_new         (ModestWidgetFactory *factory);

G_END_DECLS

#endif /* __MODEST_MAIN_WINDOW_H__ */


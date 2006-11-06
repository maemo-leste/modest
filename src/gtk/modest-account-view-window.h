/* modest-account-view-window.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_ACCOUNT_VIEW_WINDOW_H__
#define __MODEST_ACCOUNT_VIEW_WINDOW_H__

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <modest-widget-factory.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_VIEW_WINDOW             (modest_account_view_window_get_type())
#define MODEST_ACCOUNT_VIEW_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_VIEW_WINDOW,ModestAccountViewWindow))
#define MODEST_ACCOUNT_VIEW_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_VIEW_WINDOW,GtkWindow))
#define MODEST_IS_ACCOUNT_VIEW_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_VIEW_WINDOW))
#define MODEST_IS_ACCOUNT_VIEW_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_VIEW_WINDOW))
#define MODEST_ACCOUNT_VIEW_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_VIEW_WINDOW,ModestAccountViewWindowClass))

typedef struct _ModestAccountViewWindow      ModestAccountViewWindow;
typedef struct _ModestAccountViewWindowClass ModestAccountViewWindowClass;

struct _ModestAccountViewWindow {
	 GtkWindow parent;
	/* insert public members, if any */
};

struct _ModestAccountViewWindowClass {
	GtkWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestAccountViewWindow* obj); */
};

/* member functions */
GType        modest_account_view_window_get_type    (void) G_GNUC_CONST;

GtkWidget*   modest_account_view_window_new         (ModestWidgetFactory *factory);

G_END_DECLS

#endif /* __MODEST_ACCOUNT_VIEW_WINDOW_H__ */


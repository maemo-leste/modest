/* modest-store-widget.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_STORE_WIDGET_H__
#define __MODEST_STORE_WIDGET_H__

#include <gtk/gtk.h>
#include <modest-widget-factory.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_STORE_WIDGET             (modest_store_widget_get_type())
#define MODEST_STORE_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_STORE_WIDGET,ModestStoreWidget))
#define MODEST_STORE_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_STORE_WIDGET,GtkContainer))
#define MODEST_IS_STORE_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_STORE_WIDGET))
#define MODEST_IS_STORE_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_STORE_WIDGET))
#define MODEST_STORE_WIDGET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_STORE_WIDGET,ModestStoreWidgetClass))

typedef struct _ModestStoreWidget      ModestStoreWidget;
typedef struct _ModestStoreWidgetClass ModestStoreWidgetClass;

struct _ModestStoreWidget {
	 GtkVBox parent;
	/* insert public members, if any */
};

struct _ModestStoreWidgetClass {
	GtkVBoxClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestStoreWidget* obj); */
};

/* member functions */
GType        modest_store_widget_get_type    (void) G_GNUC_CONST;

GtkWidget*   modest_store_widget_new         (ModestWidgetFactory *factory,
					      const gchar* proto);

gboolean       modest_store_widget_get_remember_password (ModestStoreWidget *self);
const gchar*   modest_store_widget_get_username          (ModestStoreWidget *self);
const gchar*   modest_store_widget_get_servername        (ModestStoreWidget *self);
const gchar*   modest_store_widget_get_proto             (ModestStoreWidget *self);

G_END_DECLS

#endif /* __MODEST_STORE_WIDGET_H__ */


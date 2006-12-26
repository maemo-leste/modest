/* modest-transport-widget.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TRANSPORT_WIDGET_H__
#define __MODEST_TRANSPORT_WIDGET_H__

#include <gtk/gtk.h>
#include <modest-widget-factory.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TRANSPORT_WIDGET             (modest_transport_widget_get_type())
#define MODEST_TRANSPORT_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TRANSPORT_WIDGET,ModestTransportWidget))
#define MODEST_TRANSPORT_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TRANSPORT_WIDGET,GtkContainer))
#define MODEST_IS_TRANSPORT_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TRANSPORT_WIDGET))
#define MODEST_IS_TRANSPORT_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TRANSPORT_WIDGET))
#define MODEST_TRANSPORT_WIDGET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TRANSPORT_WIDGET,ModestTransportWidgetClass))

typedef struct _ModestTransportWidget      ModestTransportWidget;
typedef struct _ModestTransportWidgetClass ModestTransportWidgetClass;

struct _ModestTransportWidget {
	 GtkVBox parent;
	/* insert public members, if any */
};

struct _ModestTransportWidgetClass {
	GtkVBoxClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestTransportWidget* obj); */
};

/* member functions */
GType        modest_transport_widget_get_type    (void) G_GNUC_CONST;

GtkWidget*   modest_transport_widget_new         (ModestWidgetFactory *factory, const gchar *proto);

gboolean       modest_transport_widget_get_remember_password (ModestTransportWidget *self);
gboolean       modest_transport_widget_get_requires_auth     (ModestTransportWidget *self);
const gchar*   modest_transport_widget_get_username          (ModestTransportWidget *self);
const gchar*   modest_transport_widget_get_servername        (ModestTransportWidget *self);
const gchar*   modest_transport_widget_get_proto             (ModestTransportWidget *self);

G_END_DECLS

#endif /* __MODEST_TRANSPORT_WIDGET_H__ */


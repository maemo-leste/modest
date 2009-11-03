#ifndef                                         __MODEST_TOOLKIT_FACTORY_H__
#define                                         __MODEST_TOOLKIT_FACTORY_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define                                         MODEST_TYPE_TOOLKIT_FACTORY \
                                                (modest_toolkit_factory_get_type())

#define                                         MODEST_TOOLKIT_FACTORY(obj) \
                                                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                MODEST_TYPE_TOOLKIT_FACTORY, ModestToolkitFactory))

#define                                         MODEST_TOOLKIT_FACTORY_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                MODEST_TYPE_TOOLKIT_FACTORY, ModestToolkitFactory))

#define                                         MODEST_IS_TOOLKIT_FACTORY(obj) \
                                                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_TOOLKIT_FACTORY))

#define                                         MODEST_IS_TOOLKIT_FACTORY_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_TYPE ((klass), MODEST_TYPE_TOOLKIT_FACTORY))

#define                                         MODEST_TOOLKIT_FACTORY_GET_CLASS(obj) \
                                                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                                MODEST_TYPE_TOOLKIT_FACTORY, ModestToolkitFactoryClass))

typedef struct                                  _ModestToolkitFactory ModestToolkitFactory;

typedef struct                                  _ModestToolkitFactoryClass ModestToolkitFactoryClass;

struct                                          _ModestToolkitFactoryClass
{
	GObjectClass parent_class;

	GtkWidget * (*create_scrollable) (ModestToolkitFactory *self);
	GtkWidget * (*create_check_button) (ModestToolkitFactory *self, const gchar *label);
	GtkWidget * (*create_check_menu) (ModestToolkitFactory *self, const gchar *label);
};

struct                                          _ModestToolkitFactory
{
    GObject parent;
};


GType
modest_toolkit_factory_get_type                       (void) G_GNUC_CONST;

ModestToolkitFactory *
modest_toolkit_factory_get_instance                            (void);

GtkWidget *
modest_toolkit_factory_create_scrollable              (ModestToolkitFactory *self);

GtkWidget *
modest_toolkit_factory_create_check_button (ModestToolkitFactory *self, const gchar *label);

GtkWidget *
modest_toolkit_factory_create_check_menu (ModestToolkitFactory *self, const gchar *label);

gboolean
modest_togglable_get_active (GtkWidget *widget);

void
modest_togglable_set_active (GtkWidget *widget, gboolean active);

gboolean
modest_is_togglable (GtkWidget *widget);

G_END_DECLS

#endif /* __MODEST_WP_TEXT_VIEW_H__ */

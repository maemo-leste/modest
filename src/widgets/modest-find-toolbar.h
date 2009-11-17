#ifndef                                         __MODEST_FIND_TOOLBAR_H__
#define                                         __MODEST_FIND_TOOLBAR_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <modest-isearch-toolbar.h>

G_BEGIN_DECLS

#define                                         MODEST_TYPE_FIND_TOOLBAR \
                                                (modest_find_toolbar_get_type())

#define                                         MODEST_FIND_TOOLBAR(obj) \
                                                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                MODEST_TYPE_FIND_TOOLBAR, ModestFindToolbar))

#define                                         MODEST_FIND_TOOLBAR_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                MODEST_TYPE_FIND_TOOLBAR, ModestFindToolbar))

#define                                         MODEST_IS_FIND_TOOLBAR(obj) \
                                                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_FIND_TOOLBAR))

#define                                         MODEST_IS_FIND_TOOLBAR_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_TYPE ((klass), MODEST_TYPE_FIND_TOOLBAR))

#define                                         MODEST_FIND_TOOLBAR_GET_CLASS(obj) \
                                                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                                MODEST_TYPE_FIND_TOOLBAR, ModestFindToolbarClass))

typedef struct                                  _ModestFindToolbar ModestFindToolbar;

typedef struct                                  _ModestFindToolbarClass ModestFindToolbarClass;

struct                                          _ModestFindToolbarClass
{
	GtkToolbarClass parent_class;

	/* ModestISearchToolbar interface */
	void (*highlight_entry_func) (ModestISearchToolbar *self, gboolean focus);
	void (*set_label_func) (ModestISearchToolbar *self, const gchar *label);
	const gchar * (*get_search_func) (ModestISearchToolbar *self);
};

struct                                          _ModestFindToolbar
{
	GtkToolbar parent;
};


GType
modest_find_toolbar_get_type                       (void) G_GNUC_CONST;

GtkWidget *
modest_find_toolbar_new                            (const gchar *label);

G_END_DECLS

#endif

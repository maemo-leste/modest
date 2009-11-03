#ifndef                                         __MODEST_HILDON_FIND_TOOLBAR_H__
#define                                         __MODEST_HILDON_FIND_TOOLBAR_H__

#include <glib-object.h>
#include <modest-isearch-toolbar.h>
#include <hildon/hildon.h>

G_BEGIN_DECLS

#define                                         MODEST_TYPE_HILDON_FIND_TOOLBAR \
                                                (modest_hildon_find_toolbar_get_type())

#define                                         MODEST_HILDON_FIND_TOOLBAR(obj) \
                                                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                MODEST_TYPE_HILDON_FIND_TOOLBAR, ModestHildonFindToolbar))

#define                                         MODEST_HILDON_FIND_TOOLBAR_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                MODEST_TYPE_HILDON_FIND_TOOLBAR, ModestHildonFindToolbar))

#define                                         MODEST_IS_HILDON_FIND_TOOLBAR(obj) \
                                                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_HILDON_FIND_TOOLBAR))

#define                                         MODEST_IS_HILDON_FIND_TOOLBAR_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_TYPE ((klass), MODEST_TYPE_HILDON_FIND_TOOLBAR))

#define                                         MODEST_HILDON_FIND_TOOLBAR_GET_CLASS(obj) \
                                                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                                MODEST_TYPE_HILDON_FIND_TOOLBAR, ModestHildonFindToolbarClass))

typedef struct                                  _ModestHildonFindToolbar ModestHildonFindToolbar;

typedef struct                                  _ModestHildonFindToolbarClass ModestHildonFindToolbarClass;

struct                                          _ModestHildonFindToolbarClass
{
	HildonFindToolbarClass parent_class;

	/* ModestISearchToolbar interface */
	void (*highlight_entry_func) (ModestISearchToolbar *self, gboolean focus);
	void (*set_label_func) (ModestISearchToolbar *self, const gchar *label);
	const gchar * (*get_search_func) (ModestISearchToolbar *self);
};

struct                                          _ModestHildonFindToolbar
{
    HildonFindToolbar parent;
};


GType
modest_hildon_find_toolbar_get_type                       (void) G_GNUC_CONST;

GtkWidget *
modest_hildon_find_toolbar_new                            (const gchar *label);

G_END_DECLS

#endif

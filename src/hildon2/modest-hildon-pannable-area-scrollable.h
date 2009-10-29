#ifndef                                         __MODEST_HILDON_PANNABLE_AREA_SCROLLABLE_H__
#define                                         __MODEST_HILDON_PANNABLE_AREA_SCROLLABLE_H__

#include <glib-object.h>
#include <modest-scrollable.h>
#include <hildon/hildon-pannable-area.h>

G_BEGIN_DECLS

#define                                         MODEST_TYPE_HILDON_PANNABLE_AREA_SCROLLABLE \
                                                (modest_hildon_pannable_area_scrollable_get_type())

#define                                         MODEST_HILDON_PANNABLE_AREA_SCROLLABLE(obj) \
                                                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                MODEST_TYPE_HILDON_PANNABLE_AREAS_CROLLABLE, ModestHildonPannableAreaScrollable))

#define                                         MODEST_HILDON_PANNABLE_AREA_SCROLLABLE_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                MODEST_TYPE_HILDON_PANNABLE_AREA_SCROLLABLE, ModestHildonPannableAreaScrollable))

#define                                         MODEST_IS_HILDON_PANNABLE_AREA_SCROLLABLE(obj) \
                                                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_HILDON_PANNABLE_AREA_SCROLLABLE))

#define                                         MODEST_IS_HILDON_PANNABLE_AREA_SCROLLABLE_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_TYPE ((klass), MODEST_TYPE_HILDON_PANNABLE_AREA_SCROLLABLE))

#define                                         MODEST_HILDON_PANNABLE_AREA_SCROLLABLE_GET_CLASS(obj) \
                                                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                                MODEST_TYPE_HILDON_PANNABLE_ARE_SCROLLABLE, ModestHildonPannableAreaScrollableClass))

typedef struct                                  _ModestHildonPannableAreaScrollable ModestHildonPannableAreaScrollable;

typedef struct                                  _ModestHildonPannableAreaScrollableClass ModestHildonPannableAreaScrollableClass;

struct                                          _ModestHildonPannableAreaScrollableClass
{
	HildonPannableAreaClass parent_class;

	/* ModestScrollable interface */
	void (*add_with_viewport_func) (ModestScrollable *self, GtkWidget *widget);
	GtkAdjustment * (*get_vadjustment_func) (ModestScrollable *self);
	GtkAdjustment * (*get_hadjustment_func) (ModestScrollable *self);
	void (*scroll_to_func) (ModestScrollable *self, const gint x, const gint y);
	void (*jump_to_func) (ModestScrollable *self, const gint x, const gint y);
	GtkPolicyType (*get_vertical_policy_func) (ModestScrollable *self);
	GtkPolicyType (*get_horizontal_policy_func) (ModestScrollable *self);
	void (*set_vertical_policy_func) (ModestScrollable *self, GtkPolicyType policy);
	void (*set_horizontal_policy_func) (ModestScrollable *self, GtkPolicyType policy);
};

struct                                          _ModestHildonPannableAreaScrollable
{
    HildonPannableArea parent;
};


GType
modest_hildon_pannable_area_scrollable_get_type                       (void) G_GNUC_CONST;

GtkWidget *
modest_hildon_pannable_area_scrollable_new                            (void);

G_END_DECLS

#endif /* __MODEST_WP_TEXT_VIEW_H__ */

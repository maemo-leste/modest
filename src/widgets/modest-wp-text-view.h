#ifndef                                         __MODEST_WP_TEXT_VIEW_H__
#define                                         __MODEST_WP_TEXT_VIEW_H__

#include                                        <wptextview.h>

G_BEGIN_DECLS

#define                                         MODEST_TYPE_WP_TEXT_VIEW \
                                                (modest_wp_text_view_get_type())

#define                                         MODEST_WP_TEXT_VIEW(obj) \
                                                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                WP_TYPE_TEXT_VIEW, ModestWpTextView))

#define                                         MODEST_WP_TEXT_VIEW_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                WP_TYPE_TEXT_VIEW, ModestWpTextViewClass))

#define                                         MODEST_IS_WP_TEXT_VIEW(obj) \
                                                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WP_TYPE_TEXT_VIEW))

#define                                         MODEST_IS_WP_TEXT_VIEW_CLASS(klass) \
                                                (G_TYPE_CHECK_CLASS_TYPE ((klass), WP_TYPE_TEXT_VIEW))

#define                                         MODEST_WP_TEXT_VIEW_GET_CLASS(obj) \
                                                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                                MODEST_TYPE_WP_TEXT_VIEW, ModestWpTextViewClass))

typedef struct                                  _ModestWpTextView ModestWpTextView;

typedef struct                                  _ModestWpTextViewClass ModestWpTextViewClass;

struct                                          _ModestWpTextViewClass
{
    WPTextViewClass parent_class;
};

struct                                          _ModestWpTextView
{
    WPTextView parent;
};


GType
modest_wp_text_view_get_type                       (void) G_GNUC_CONST;

GtkWidget *
modest_wp_text_view_new                            (void);


G_END_DECLS

#endif /* __MODEST_WP_TEXT_VIEW_H__ */

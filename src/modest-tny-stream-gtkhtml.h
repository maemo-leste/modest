/* modest-tny-stream-gtkhtml.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_STREAM_GTKHTML_H__
#define __MODEST_TNY_STREAM_GTKHTML_H__

#include <glib-object.h>
#include <gtkhtml/gtkhtml.h>
#include <tny-stream-iface.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_STREAM_GTKHTML             (modest_tny_stream_gtkhtml_get_type())
#define MODEST_TNY_STREAM_GTKHTML(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_STREAM_GTKHTML,ModestTnyStreamGtkhtml))
#define MODEST_TNY_STREAM_GTKHTML_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_STREAM_GTKHTML,GObject))
#define MODEST_IS_TNY_STREAM_GTKHTML(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_STREAM_GTKHTML))
#define MODEST_IS_TNY_STREAM_GTKHTML_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_STREAM_GTKHTML))
#define MODEST_TNY_STREAM_GTKHTML_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_STREAM_GTKHTML,ModestTnyStreamGtkhtmlClass))

typedef struct _ModestTnyStreamGtkhtml      ModestTnyStreamGtkhtml;
typedef struct _ModestTnyStreamGtkhtmlClass ModestTnyStreamGtkhtmlClass;

struct _ModestTnyStreamGtkhtml {
	GObject parent;
};

struct _ModestTnyStreamGtkhtmlClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestTnyStreamGtkhtml* obj); */
};

/* member functions */
GType        modest_tny_stream_gtkhtml_get_type    (void) G_GNUC_CONST;

GObject*    modest_tny_stream_gtkhtml_new         (GtkHTML* gtkhtml);


G_END_DECLS

#endif /* __MODEST_TNY_STREAM_GTKHTML_H__ */


/* modest-viewer-window.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_VIEWER_WINDOW_H__
#define __MODEST_VIEWER_WINDOW_H__

#include <gtk/gtkwindow.h>
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_VIEWER_WINDOW             (modest_viewer_window_get_type())
#define MODEST_VIEWER_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_VIEWER_WINDOW,ModestViewerWindow))
#define MODEST_VIEWER_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_VIEWER_WINDOW,GtkWindow))
#define MODEST_IS_VIEWER_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_VIEWER_WINDOW))
#define MODEST_IS_VIEWER_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_VIEWER_WINDOW))
#define MODEST_VIEWER_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_VIEWER_WINDOW,ModestViewerWindowClass))

typedef struct _ModestViewerWindow      ModestViewerWindow;
typedef struct _ModestViewerWindowClass ModestViewerWindowClass;

struct _ModestViewerWindow {
	 GtkWindow parent;
	/* insert public members, if any */
};

struct _ModestViewerWindowClass {
	GtkWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestViewerWindow* obj); */
};

/* member functions */
GType        modest_viewer_window_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget*, */
/*    otherwise probably a GObject*. */
GtkWidget*   modest_viewer_window_new (ModestUI *ui, TnyMsgIface *msg);


/* fill in other public functions, eg.: */
/* 	void       modest_viewer_window_do_something (ModestViewerWindow *self, const gchar* param); */
/* 	gboolean   modest_viewer_window_has_foo      (ModestViewerWindow *self, gint value); */


G_END_DECLS

#endif /* __MODEST_VIEWER_WINDOW_H__ */


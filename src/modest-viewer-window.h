/* modest-viewer-window.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_VIEWER_WINDOW_H__
#define __MODEST_VIEWER_WINDOW_H__

#include <gtk/gtkwindow.h>

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
};

struct _ModestViewerWindowClass {
	GtkWindowClass parent_class;
};

/* member functions */
GType        modest_viewer_window_get_type    (void) G_GNUC_CONST;

GtkWidget*   modest_viewer_window_new (ModestUI *ui, TnyMsgIface *msg);

/**
 * modest_viewer_window_get_data:
 * @viewer_win: a ModestViewerWindow instance
 *
 * Retrieves the data pointer that was set at creation of this instance
 *
 * Returns: the data pointer
 */
gpointer modest_viewer_window_get_data(ModestViewerWindow *viewer_win);

/**
 * modest_viewer_window_get_tiny_msg_view:
 * @viewer_win: a ModestViewerWindow instance
 *
 * Returns: the ModestTnyMsgView widget from the viewer instance
 */
ModestTnyMsgView *modest_viewer_window_get_tiny_msg_view(ModestViewerWindow *viewer_win);


G_END_DECLS

#endif /* __MODEST_VIEWER_WINDOW_H__ */


/* modest-main-window.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_MAIN_WINDOW_H__
#define __MODEST_MAIN_WINDOW_H__


#include <gtk/gtk.h>
#include "../modest-conf.h"
#include "../modest-account-mgr.h"

/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAIN_WINDOW             (modest_main_window_get_type())
#define MODEST_MAIN_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindow))
#define MODEST_MAIN_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MAIN_WINDOW,GtkWidget))
#define MODEST_IS_MAIN_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_IS_MAIN_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_MAIN_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindowClass))

typedef struct _ModestMainWindow      ModestMainWindow;
typedef struct _ModestMainWindowClass ModestMainWindowClass;


struct _ModestMainWindow {
	 GtkWindow parent;
	/* insert public members, if any */
};

struct _ModestMainWindowClass {
	GtkWindowClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestMainWindow* obj); */
};



/* member functions */
GType        modest_main_window_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget*, */
/*    otherwise probably a GObject*. */
GtkWidget*    modest_main_window_new         (ModestConf *modest_conf,
					      ModestAccountMgr *modest_acc_mgr);

/* fill in other public functions, eg.: */
/* 	void       modest_main_window_do_something (ModestMainWindow *self, const gchar* param); */
/* 	gboolean   modest_main_window_has_foo      (ModestMainWindow *self, gint value); */


G_END_DECLS

#endif /* __MODEST_MAIN_WINDOW_H__ */


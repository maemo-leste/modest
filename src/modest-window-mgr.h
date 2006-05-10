/* modest-window-mgr.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_WINDOW_MGR_H__
#define __MODEST_WINDOW_MGR_H__

#include <glib-object.h>
/* other include files */

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_WINDOW_MGR             (modest_window_mgr_get_type())
#define MODEST_WINDOW_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_WINDOW_MGR,ModestWindowMgr))
#define MODEST_WINDOW_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_WINDOW_MGR,GObject))
#define MODEST_IS_WINDOW_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_WINDOW_MGR))
#define MODEST_IS_WINDOW_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_WINDOW_MGR))
#define MODEST_WINDOW_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_WINDOW_MGR,ModestWindowMgrClass))

enum {
	MODEST_MAIN_WINDOW,        /* the main window */
	MODEST_EDIT_WINDOW,        /* a window to edit a mail */
	MODEST_ACCOUNT_WINDOW,     /* a window to edit account information */
	MODEST_VIEW_WINDOW         /* a window to view mails */
};
typedef guint ModestWindowType;


typedef struct _ModestOpenWindow ModestOpenWindow;
struct _ModestOpenWindow {
	GObject          *win;
	ModestWindowType type;
	guint            id;
};


typedef struct _ModestWindowMgr      ModestWindowMgr;
typedef struct _ModestWindowMgrClass ModestWindowMgrClass;

struct _ModestWindowMgr {
	GObject parent;
};

struct _ModestWindowMgrClass {
	GObjectClass parent_class;
	
	void (* last_window_closed) (ModestWindowMgr* obj);
};

/* member functions */
GType        modest_window_mgr_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
/* if this is a kind of GtkWidget, it should probably return at GtkWidget*, */
/*    otherwise probably a GObject*. */
GObject*    modest_window_mgr_new         (void);

gboolean    modest_window_mgr_register   (ModestWindowMgr *self, GObject *win,
					  ModestWindowType type, guint window_id);
gboolean    modest_window_mgr_unregister (ModestWindowMgr *self, GObject *win);
GObject*    modest_window_mgr_find_by_type (ModestWindowMgr *self, ModestWindowType type);
GObject*    modest_window_mgr_find_by_id (ModestWindowMgr *self, gint window_id);

G_END_DECLS

#endif /* __MODEST_WINDOW_MGR_H__ */


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

/**
 * modest_window_mgr_new:
 *
 * creates a new ModestWindowMgr instance
 *
 * Returns: a new ModestWindowMgr, or NULL in case of error
 */
GObject*    modest_window_mgr_new         (void);

/**
 * modest_window_mgr_register:
 * @self: a ModestWindowMgr instance
 * @win: the GObject of the window to register
 * @type: ModestWindowType of the window to register
 * @window_id: a guint window_id of the window
 *
 * register a window with the ModestWindowMgr instance *self
 *
 * Returns: TRUE on success, else FALSE
 */
gboolean    modest_window_mgr_register   (ModestWindowMgr *self, GObject *win,
					  ModestWindowType type, guint window_id);

/**
 * modest_window_mgr_unregister:
 * @self: a ModestWindowMgr instance
 * @win: the GObject of the window to register
 *
 * unregister a window from the ModestWindowMgr instance *self
 *
 * Returns: TRUE on success, else FALSE
 */
gboolean    modest_window_mgr_unregister (ModestWindowMgr *self, GObject *win);

/**
 * modest_window_mgr_find_by_type:
 * @self: a ModestWindowMgr instance
 * @type: the ModestWindowType to search for
 *
 * search for a window of type 'type' in the ModestWindowMgr instance *self
 *
 * Returns: the GObject of the window, else NULL
 */
GObject*    modest_window_mgr_find_by_type (ModestWindowMgr *self, ModestWindowType type);

/**
 * modest_window_mgr_find_by_id:
 * @self: a ModestWindowMgr instance
 * @window_id: the ModestWindowType to search for
 *
 * search for a window with a specific 'window_id' in the ModestWindowMgr instance *self
 *
 * Returns: the GObject of the window, else NULL
 */
GObject*    modest_window_mgr_find_by_id (ModestWindowMgr *self, gint window_id);

G_END_DECLS

#endif /* __MODEST_WINDOW_MGR_H__ */


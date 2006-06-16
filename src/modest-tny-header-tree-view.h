/* modest-tny-header-tree-view.h */
/* insert (c)/licensing information) */

#ifndef __MODEST_TNY_HEADER_TREE_VIEW_H__
#define __MODEST_TNY_HEADER_TREE_VIEW_H__

#include <gtk/gtk.h>
#include <tny-msg-folder-iface.h>
#include <tny-account-tree-model.h>
#include <tny-msg-iface.h>
#include <tny-msg-header-iface.h>
#include <tny-msg-header-list-model.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_HEADER_TREE_VIEW             (modest_tny_header_tree_view_get_type())
#define MODEST_TNY_HEADER_TREE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_HEADER_TREE_VIEW,ModestTnyHeaderTreeView))
#define MODEST_TNY_HEADER_TREE_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_HEADER_TREE_VIEW,ModestTnyHeaderTreeViewClass))
#define MODEST_IS_TNY_HEADER_TREE_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_HEADER_TREE_VIEW))
#define MODEST_IS_TNY_HEADER_TREE_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_HEADER_TREE_VIEW))
#define MODEST_TNY_HEADER_TREE_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_HEADER_TREE_VIEW,ModestTnyHeaderTreeViewClass))

typedef struct _ModestTnyHeaderTreeView      ModestTnyHeaderTreeView;
typedef struct _ModestTnyHeaderTreeViewClass ModestTnyHeaderTreeViewClass;

struct _ModestTnyHeaderTreeView {
	 GtkTreeView parent;
	/* insert public members, if any */
};

struct _ModestTnyHeaderTreeViewClass {
	GtkTreeViewClass parent_class;

	void (*message_selected) (ModestTnyHeaderTreeView* self,
				  TnyMsgIface *msg,
				  gpointer user_data);

	/* msg == NULL implies that the operation is finished, ie.
	 * the progress indictation can be hidden */
	void (*status_update) (ModestTnyHeaderTreeView* self,
			       const gchar* msg,
			       gint status,
			       gpointer user_data);
};


enum {
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_FROM,
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_TO,
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_SUBJECT,
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_SENT_DATE,
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_RECEIVED_DATE,
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_MSGTYPE,
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_ATTACH,
	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_COMPACT_HEADER,

	MODEST_TNY_HEADER_TREE_VIEW_COLUMN_NUM
};
typedef guint ModestTnyHeaderTreeViewColumn;

enum {
	MODEST_TNY_HEADER_TREE_VIEW_STYLE_NORMAL,
	MODEST_TNY_HEADER_TREE_VIEW_STYLE_COMPACT,
	
	MODEST_TNY_HEADER_TREE_VIEW_STYLE_NUM
};
typedef guint ModestTnyHeaderTreeViewStyle;



/**
 * modest_tny_header_tree_view_get_type:
 * 
 * get the GType for ModestTnyHeaderTreeView
 *  
 * Returns: the GType
 */
GType        modest_tny_header_tree_view_get_type    (void) G_GNUC_CONST;


/**
 * modest_tny_header_tree_view_new:
 * @folder: a TnyMsgFolderIface object
 * @columns: a list of ModestTnyHeaderTreeViewColumn
 * @style: a ModestTnyHeaderTreeViewColumn with the style of this listview
 *  (	MODEST_TNY_HEADER_TREE_VIEW_STYLE_NORMAL or MODEST_TNY_HEADER_TREE_VIEW_STYLE_COMPACT)
 * 
 * create a new ModestTnyHeaderTreeView instance, based on a folder iface
 *   
 * Returns: a new GtkWidget (a GtkTreeView-subclass)
 */
GtkWidget*   modest_tny_header_tree_view_new        (TnyMsgFolderIface *folder,
						     GSList *columns,
						     ModestTnyHeaderTreeViewStyle style);

/**
 * modest_tny_header_tree_view_set_folder:
 * @self: a ModestTnyHeaderTreeView instance
 * @folder: a TnyMsgFolderIface object
 * 
 * set the folder for this ModestTnyHeaderTreeView
 *  
 * Returns: TRUE if it succeeded, FALSE otherwise
 */
gboolean     modest_tny_header_tree_view_set_folder (ModestTnyHeaderTreeView *self,
						      TnyMsgFolderIface *folder);


/**
 * modest_tny_header_tree_view_set_columns:
 * @self: a ModestTnyHeaderTreeView instance
 * @columns: a list of ModestTnyHeaderTreeViewColumn
 * 
 * set the columns for this ModestTnyHeaderTreeView
 *  
 * Returns: TRUE if it succeeded, FALSE otherwise
 */
gboolean     modest_tny_header_tree_view_set_columns (ModestTnyHeaderTreeView *self,
						      GSList *columns);
/**
 * modest_tny_header_tree_view_get_columns:
 * @self: a ModestTnyHeaderTreeView instance
 * @folder: a TnyMsgFolderIface object
 * 
 * get the columns for this ModestTnyHeaderTreeView
 *  
 * Returns: list of columms, or NULL in case of no columns or error
 */
const GSList*   modest_tny_header_tree_view_get_columns (ModestTnyHeaderTreeView *self);
	

/**
 * modest_tny_header_tree_view_set_style:
 * @self: a ModestTnyHeaderTreeView instance
 * @style: the style for this tree view
 * 
 * set the folder for this ModestTnyHeaderTreeView
 *  
 * Returns: TRUE if it succeeded, FALSE otherwise
 */
gboolean   modest_tny_header_tree_view_set_style (ModestTnyHeaderTreeView *self,
						  ModestTnyHeaderTreeViewStyle style);

/**
 * modest_tny_header_tree_view_set_folder:
 * @self: a ModestTnyHeaderTreeView instance
 * @folder: a TnyMsgFolderIface object
 * 
 * set the folder for this ModestTnyHeaderTreeView
 *  
 * Returns: TRUE if it succeeded, FALSE otherwise
 */
ModestTnyHeaderTreeViewStyle   modest_tny_header_tree_view_get_style (ModestTnyHeaderTreeView *self);

G_END_DECLS





#endif /* __MODEST_TNY_HEADER_TREE_VIEW_H__ */


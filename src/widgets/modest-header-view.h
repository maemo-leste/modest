/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MODEST_HEADER_VIEW_H__
#define __MODEST_HEADER_VIEW_H__

#include <gtk/gtk.h>
#include <tny-folder.h>
#include <tny-gtk-account-list-model.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-gtk-header-list-model.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_HEADER_VIEW             (modest_header_view_get_type())
#define MODEST_HEADER_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_HEADER_VIEW,ModestHeaderView))
#define MODEST_HEADER_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_HEADER_VIEW,ModestHeaderViewClass))
#define MODEST_IS_HEADER_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_HEADER_VIEW))
#define MODEST_IS_HEADER_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_HEADER_VIEW))
#define MODEST_HEADER_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_HEADER_VIEW,ModestHeaderViewClass))

typedef struct _ModestHeaderView      ModestHeaderView;
typedef struct _ModestHeaderViewClass ModestHeaderViewClass;

struct _ModestHeaderView {
	 GtkTreeView parent;
	/* insert public members, if any */
};

#define MODEST_HEADER_VIEW_COLUMN    "header-view-column"

typedef enum _ModestHeaderViewColumn {
	MODEST_HEADER_VIEW_COLUMN_FROM            = 0,
	MODEST_HEADER_VIEW_COLUMN_TO,
	MODEST_HEADER_VIEW_COLUMN_SUBJECT,
	MODEST_HEADER_VIEW_COLUMN_SENT_DATE,
	MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE,
	MODEST_HEADER_VIEW_COLUMN_MSGTYPE,
	MODEST_HEADER_VIEW_COLUMN_ATTACH,
	MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER,
	MODEST_HEADER_VIEW_COLUMN_NUM
} ModestHeaderViewColumn;

typedef enum _ModestHeaderViewStyle {
	MODEST_HEADER_VIEW_STYLE_NORMAL,
	MODEST_HEADER_VIEW_STYLE_COMPACT,
	MODEST_HEADER_VIEW_STYLE_NUM
} ModestHeaderViewStyle;

typedef enum _ModestItemType {
	MODEST_ITEM_TYPE_MESSAGE,
	MODEST_ITEM_TYPE_FOLDER,
	MODEST_ITEM_TYPE_NUM
} ModestItemType;

struct _ModestHeaderViewClass {
	GtkTreeViewClass parent_class;

	void (*message_selected) (ModestHeaderView* self,
				  TnyMsg *msg,
				  gpointer user_data);

	void (*item_not_found) (ModestHeaderView* self,
				ModestItemType type,
				gpointer user_data);
	
	/* msg == NULL implies that the operation is finished, ie.
	 * the progress indictation can be hidden */
	void (*status_update) (ModestHeaderView* self,
			       const gchar* msg,
			       gint num, gint total,
			       gpointer user_data);
};

/**
 * modest_header_view_get_type:
 * 
 * get the GType for ModestHeaderView
 *  
 * Returns: the GType
 */
GType        modest_header_view_get_type    (void) G_GNUC_CONST;


/**
 * modest_header_view_new:
 * @folder: a TnyMsgFolder object
 * @columns: a list of ModestHeaderViewColumn
 * @style: a ModestHeaderViewColumn with the style of this listview
 *  (	MODEST_HEADER_VIEW_STYLE_NORMAL or MODEST_HEADER_VIEW_STYLE_COMPACT)
 * 
 * create a new ModestHeaderView instance, based on a folder iface
 *   
 * Returns: a new GtkWidget (a GtkTreeView-subclass)
 */
GtkWidget*   modest_header_view_new        (TnyFolder *folder,
					    const GList *columns,
					    ModestHeaderViewStyle style);

/**
 * modest_header_view_set_folder:
 * @self: a ModestHeaderView instance
 * @folder: a TnyFolder object
 * 
 * set the folder for this ModestHeaderView
 *  
 * Returns: TRUE if it succeeded, FALSE otherwise
 */
gboolean     modest_header_view_set_folder (ModestHeaderView *self,
					    TnyFolder *folder);




/**
 * modest_header_view_get_folder:
 * @self: a ModestHeaderView instance
 * 
 * get the folder in this ModestHeaderView
 *  
 * Returns: the tny folder instance or NULL if there is none
 */
TnyFolder *modest_header_view_get_folder (ModestHeaderView *self);


/**
 * modest_header_view_set_columns:
 * @self: a ModestHeaderView instance
 * @columns: a list of ModestHeaderViewColumn
 * 
 * set the columns for this ModestHeaderView.
 *  
 * Returns: TRUE if it succeeded, FALSE otherwise
 */
gboolean modest_header_view_set_columns (ModestHeaderView *self,
					 const GList *columns);
/**
 * modest_header_view_get_columns:
 * @self: a ModestHeaderView instance
 * @folder: a TnyFolder object
 * 
 * get the GtkTreeColumns for this ModestHeaderView. Each one of the
 * tree columns will have property  than can be retrieved
 * with g_object_get_data MODEST_HEADER_VIEW_COLUMN (#define),
 * and which contains the corresponding ModestHeaderViewColumn
 *  
 * Returns: newly allocated list of column ids, or NULL in case of no columns or error
 * You must free the list with g_list_free
 */
GList*  modest_header_view_get_columns (ModestHeaderView *self);
	

/**
 * modest_header_view_set_style:
 * @self: a ModestHeaderView instance
 * @style: the style for this tree view
 * 
 * set the style this ModestHeaderView
 *  
 * Returns: TRUE if it succeeded, FALSE otherwise
 */
gboolean   modest_header_view_set_style (ModestHeaderView *self,
					 ModestHeaderViewStyle style);

/**
 * modest_header_view_set_folder:
 * @self: a ModestHeaderView instance
 * 
 * get the style for this ModestHeaderView
 *  
 * Returns: the current style
 */
ModestHeaderViewStyle   modest_header_view_get_style (ModestHeaderView *self);

/**
 * modest_header_view_get_selected_headers:
 * @self: a ModestHeaderView instance
 * 
 * get the list of the currently selected TnyHeader's
 *  
 * Returns: the list with the currently selected headers
 */
TnyList* modest_header_view_get_selected_headers (ModestHeaderView *self);

G_END_DECLS


#endif /* __MODEST_HEADER_VIEW_H__ */


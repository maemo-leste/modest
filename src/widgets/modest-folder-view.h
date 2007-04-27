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

#ifndef __MODEST_FOLDER_VIEW_H__
#define __MODEST_FOLDER_VIEW_H__

#include <glib-object.h>
#include <tny-gtk-account-list-model.h>
#include <tny-account-store.h>
#include <modest-tny-account-store.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_FOLDER_VIEW             (modest_folder_view_get_type())
#define MODEST_FOLDER_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_FOLDER_VIEW,ModestFolderView))
#define MODEST_FOLDER_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_FOLDER_VIEW,ModestFolderViewClass))
#define MODEST_IS_FOLDER_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_FOLDER_VIEW))
#define MODEST_IS_FOLDER_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_FOLDER_VIEW))
#define MODEST_FOLDER_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_FOLDER_VIEW,ModestFolderViewClass))

typedef enum _ModestFolderViewStyle {
	MODEST_FOLDER_VIEW_STYLE_SHOW_ALL,
	MODEST_FOLDER_VIEW_STYLE_SHOW_ONE
} ModestFolderViewStyle;

typedef struct _ModestFolderView      ModestFolderView;
typedef struct _ModestFolderViewClass ModestFolderViewClass;

struct _ModestFolderView {
	 GtkTreeView parent;
	/* insert public members, if any */
};

struct _ModestFolderViewClass {
	GtkTreeViewClass parent_class;

	/* emitted when a folder is selected or unselected */
	void     (*folder_selection_changed) (ModestFolderView* self,
					      TnyFolder *folder,
					      gboolean selected,
					      gpointer user_data);

	void     (*folder_display_name_changed) (ModestFolderView* self,
						 const gchar *display_name,
						 gpointer user_data);
};

/**
 * modest_folder_view_get_type:
 * 
 * get the GType for ModestFolderView
 *  
 * Returns: the GType
 */
GType        modest_folder_view_get_type        (void) G_GNUC_CONST;



/**
 * modest_folder_view_new:
 * @query: a #TnyFolderStoreQuery that specifies the folders to show
 * 
 * create a new #ModestFolderView instance
 *  
 * Returns: a new #GtkWidget (a #GtkTreeView subclass)
 */
GtkWidget*    modest_folder_view_new            (TnyFolderStoreQuery *query);

/**
 * modest_folder_view_set_title:
 * @self: a ModestFolderView instance
 * @title: the new title
 * 
 * set the title for the folder view; if title is NULL, the title column
 * header will be hidden
 */
void          modest_folder_view_set_title       (ModestFolderView *self, 
						  const gchar *title);


/**
 * modest_folder_view_get_selected:
 * @self: a #ModestFolderView
 * 
 * gets a new reference to the #TnyFolderStore that is already
 * selected. The caller must free this reference
 * 
 * Returns: the selected #TnyFolderStore or NULL if none is selected
 **/
TnyFolderStore*    modest_folder_view_get_selected    (ModestFolderView *self);


/**
 * modest_folder_view_update_model:
 * @self: a #ModestFolderView
 * 
 * refresh the current model
 * 
 * Returns: TRUE if the model was succesfully updated
 **/
gboolean      modest_folder_view_update_model    (ModestFolderView *self,
						  TnyAccountStore *account_store);

/**
 * modest_folder_view_get_selected_display_name:
 * @self: a #ModestFolderView
 * 
 * returns a the display name of the currently selected
 * #TnyFolder. Note that it could be different to the name returned by
 * the #TnyFolder that could be get using
 * modest_folder_view_get_selected, because it for example could
 * contain the unread messages between brackets
 * 
 * Returns: the display name of the currently selected #TnyFolder or
 * NULL if none is selected
 **/
const gchar* modest_folder_view_get_selected_display_name (ModestFolderView *self);


/**
 * modest_folder_view_set_style:
 * @self: a #ModestFolderView
 * @style: a #ModestFolderViewStyle
 * 
 * Sets the folder view style. There are currently two available,
 * MODEST_FOLDER_VIEW_STYLE_SHOW_ALL shows all the active accounts,
 * and MODEST_FOLDER_VIEW_STYLE_SHOW_ONE (Maemo style) shows the local
 * account the mmc and only one of the available active server
 * accounts

 **/
void         modest_folder_view_set_style         (ModestFolderView *self,
						   ModestFolderViewStyle style);

G_END_DECLS

#endif /* __MODEST_FOLDER_VIEW_H__ */

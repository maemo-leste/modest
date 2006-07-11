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

#ifndef __MODEST_TNY_FOLDER_TREE_VIEW_H__
#define __MODEST_TNY_FOLDER_TREE_VIEW_H__

#include <tny-account-tree-model.h>
#include <tny-account-store-iface.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_TNY_FOLDER_TREE_VIEW             (modest_tny_folder_tree_view_get_type())
#define MODEST_TNY_FOLDER_TREE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_TNY_FOLDER_TREE_VIEW,ModestTnyFolderTreeView))
#define MODEST_TNY_FOLDER_TREE_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_TNY_FOLDER_TREE_VIEW,ModestTnyFolderTreeViewClass))
#define MODEST_IS_TNY_FOLDER_TREE_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_TNY_FOLDER_TREE_VIEW))
#define MODEST_IS_TNY_FOLDER_TREE_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_TNY_FOLDER_TREE_VIEW))
#define MODEST_TNY_FOLDER_TREE_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_TNY_FOLDER_TREE_VIEW,ModestTnyFolderTreeViewClass))

typedef struct _ModestTnyFolderTreeView      ModestTnyFolderTreeView;
typedef struct _ModestTnyFolderTreeViewClass ModestTnyFolderTreeViewClass;

struct _ModestTnyFolderTreeView {
	 GtkTreeView parent;
	/* insert public members, if any */
};

struct _ModestTnyFolderTreeViewClass {
	GtkTreeViewClass parent_class;

	/* emitted when a folder is clicked */
	void (*folder_selected) (ModestTnyFolderTreeView* self,
				 TnyMsgFolderIface *folder,
				 gpointer user_data);
				 
	gboolean (*update_model) (ModestTnyFolderTreeView *self, 
	                          TnyAccountStoreIface *iface);

};


/**
 * modest_tny_folder_tree_view_get_type:
 * 
 * get the GType for ModestTnyFolderTreeView
 *  
 * Returns: the GType
 */
GType        modest_tny_folder_tree_view_get_type    (void) G_GNUC_CONST;


/**
 * modest_tny_folder_tree_view_new:
 * @iface: a TnyAccountStoreIface object
 * 
 * create a new ModestTnyFolderTreeView instance, based on an account store
 *  
 * Returns: a new GtkWidget (a GtkTreeView-subclass)
 */

GtkWidget*   modest_tny_folder_tree_view_new         (TnyAccountStoreIface *iface);


/**
 * modest_tny_folder_tree_view_is_empty:
 * @self: a ModestTnyFolderTreeView instance
 * 
 * check to see of the view is empty. Note that when it is empty,
 * there will still be one item, telling "(empty)" or similar
 *  
 * Returns: TRUE if the tree view is empty, FALSE otherwise
 */
gboolean     modest_tny_folder_tree_view_is_empty    (ModestTnyFolderTreeView *self);


/**
 * modest_tny_folder_tree_view_update_model:
 * @self: a #ModestTnyFolderTreeView instance
 * @iface: a #TnyAccountStoreIface instance
 * 
 * Update the thee model from a given account store.
 *  
 * Returns: TRUE on success, FALSE otherwise
 */
gboolean     modest_tny_folder_tree_view_update_model(ModestTnyFolderTreeView *self, 
						      TnyAccountStoreIface *iface);


G_END_DECLS

#endif /* __MODEST_TNY_FOLDER_TREE_VIEW_H__ */


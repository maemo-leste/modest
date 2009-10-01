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
#include <modest-tny-folder.h>

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

typedef enum _ModestFolderViewCellStyle {
	MODEST_FOLDER_VIEW_CELL_STYLE_DEFAULT,
	MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT
} ModestFolderViewCellStyle;

typedef enum _ModestFolderViewFilter {
	MODEST_FOLDER_VIEW_FILTER_NONE = 0,
	MODEST_FOLDER_VIEW_FILTER_CAN_HAVE_FOLDERS = 1 << 0,
	MODEST_FOLDER_VIEW_FILTER_HIDE_MANDATORY_FOLDERS = 1 << 1,
	MODEST_FOLDER_VIEW_FILTER_RENAMEABLE = 1 << 2,
	MODEST_FOLDER_VIEW_FILTER_MOVEABLE = 1 << 3,
	MODEST_FOLDER_VIEW_FILTER_DELETABLE = 1 << 4,
	MODEST_FOLDER_VIEW_FILTER_HIDE_ACCOUNTS = 1 << 5,
	MODEST_FOLDER_VIEW_FILTER_HIDE_FOLDERS = 1 << 6,
	MODEST_FOLDER_VIEW_FILTER_HIDE_LOCAL_FOLDERS = 1 << 7,
	MODEST_FOLDER_VIEW_FILTER_HIDE_MCC_FOLDERS = 1 << 8,
	MODEST_FOLDER_VIEW_FILTER_SHOW_ONLY_MAILBOXES = 1<< 9,
} ModestFolderViewFilter;

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
					      TnyFolderStore *folder,
					      gboolean selected,
					      gpointer user_data);

	void     (*folder_display_name_changed) (ModestFolderView* self,
						 const gchar *display_name,
						 gpointer user_data);

	void     (*folder_activated) (ModestFolderView *self,
				      TnyFolderStore *folder,
				      gpointer userdata);

	void     (*visible_account_changed) (ModestFolderView* self,
					     const gchar *account_id,
					     gpointer user_data);
	void     (*activity_changed) (ModestFolderView* self,
				      gboolean activity,
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
 * modest_folder_view_new_full:
 * @query: a #TnyFolderStoreQuery that specifies the folders to show
 * @do_refresh: do auto refresh on loading (may be slow)
 * 
 * create a new #ModestFolderView instance
 *  
 * Returns: a new #GtkWidget (a #GtkTreeView subclass)
 */
GtkWidget*    modest_folder_view_new_full            (TnyFolderStoreQuery *query, gboolean do_refresh);

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

/**
 * modest_folder_view_set_account_id_of_visible_server_account:
 * @self: a #ModestFolderView
 * @account_id: the remote server account id
 * 
 * sets the server account id (value returned by tny_account_get_id())
 * to the string passed as argument. The remote server with the
 * specified id will be the unique visible account if the folder view
 * is configured in MODEST_FOLDER_VIEW_STYLE_SHOW_ONE
 **/
void         modest_folder_view_set_account_id_of_visible_server_account (ModestFolderView *self,
									  const gchar *account_id);

/**
 * modest_folder_view_set_visible_mailbox:
 * @self: a #ModestFolderView
 * @account_id: the remote account mailbox to show
 * 
 * if set an account id to filter, this filters also to show only
 * folders of a specific mailbox.
 **/
void         modest_folder_view_set_mailbox (ModestFolderView *self,
					     const gchar *mailbox);

/**
 * modest_folder_view_get_mailbox:
 * @self: a #ModestFolderView
 *
 * Return the current mailbox set for filtering in folder view
 *
 * Returns: a string, or %NULL
 */
const gchar *modest_folder_view_get_mailbox (ModestFolderView *self);

/**
 * modest_folder_view_get_account_id_of_visible_server_account:
 * @self: a #ModestFolderView
 * 
 * gets the account id of the currently visible server account id
 * 
 * Return value: the visible server account id or NULL if none set
 **/
const gchar* modest_folder_view_get_account_id_of_visible_server_account (ModestFolderView *self);


void         modest_folder_view_select_first_inbox_or_local  (ModestFolderView *self);

/**
 * modest_folder_view_copy_selection:
 * @self: a #ModestFolderView
 * 
 * Stores a #TnyList of selected folders in the own clibpoard of 
 * @self folder view.
 **/
void modest_folder_view_copy_selection (ModestFolderView *folder_view);

/**
 * modest_folder_view_cut_selection:
 * @self: a #ModestFolderView
 * 
 * Stores a #TnyList of selected folders in the own clibpoard of 
 * @self folder view and filter them into folders tree model to
 * hide these rows in treeview.
 **/
void modest_folder_view_cut_selection (ModestFolderView *folder_view);


/**
 * modest_folder_view_select_folder
 * @self: a #ModestFolderView
 * @folder: a #TnyFolder
 * @after_change: should we select first change to the view (TRUE), or just now (FALSE)
 *
 * select the given TnyFolder in the folder;
 * return TRUE if it succeeded, FALSE otherwise.
 **/
gboolean modest_folder_view_select_folder (ModestFolderView *self, TnyFolder *folder, gboolean after_change);

/**
 * modest_folder_view_paste_selection:
 * @self: a #ModestFolderView
 * @folders: ouput parameter with a #TnyList of folders which will be returned.
 * @delete: output parameter with indication about delete or not the selected folders. 
 * 
 * Gets the selected folders to copy/cut.
 **/
void modest_folder_view_paste_selection (ModestFolderView *folder_view, TnyList **folders, gboolean *delete);

/*
 * modest_folder_view_show_non_move_folders:
 * @self: a #ModestFolderView
 * @show: show or hide the folders
 * 
 * Whether to show folders where no messages can be moved to 
 **/
void modest_folder_view_show_non_move_folders (ModestFolderView *folder_view, gboolean show);

/*
 * modest_folder_view_copy_model:
 * @folder_view_src: a #ModestFolderView
 * @folder_view_dst: a #ModestFolderView
 * 
 * Get model from @folder_view_src and builds a new 
 * #GtkTreeFilterModel object for that model. This copied
 * model will be asigned to @folder_view_dst. 
 **/
void modest_folder_view_copy_model (ModestFolderView *folder_view_src, ModestFolderView *folder_view_dst);

/*
 * modest_folder_disable_next_folder_selection:
 * @folder_view: a #ModestFolderView
 * 
 * Checks if folder_to_select private field is set and 
 * unref it in this case, assigning it to NULL to avoid 
 * next call to on_row_inserted_maybe_select_folder does 
 * not select any folder.
 * 
 **/
void modest_folder_view_disable_next_folder_selection (ModestFolderView *self);

/**
 * modest_folder_view_set_cell_style:
 * @self: a #ModestFolderView
 * @cell_style: a #ModestFolderViewCellStyle
 *
 * Sets the way cells are shown
 */
void modest_folder_view_set_cell_style (ModestFolderView *self,
					ModestFolderViewCellStyle cell_style);

/**
 * modest_folder_view_set_filter:
 * @self: a #ModestFolderView
 * @filter: a filter mask to be applied to files
 *
 * sets the special filter to be applied (affects visibility of items).
 * It's a mask, and filters applied are applied with an AND.
 */
void modest_folder_view_set_filter (ModestFolderView *self,
				    ModestFolderViewFilter filter);

/**
 * modest_folder_view_unset_filter:
 * @self: a #ModestFolderView
 * @filter: a filter mask to be unapplied to files
 *
 * Unsets the special filter to be applied (affects visibility of
 * items).  It's a mask, and filters applied are applied with an AND.
 */
void modest_folder_view_unset_filter (ModestFolderView *self,
				      ModestFolderViewFilter filter);

gboolean modest_folder_view_any_folder_fulfils_rules (ModestFolderView *self,
						      ModestTnyFolderRules rules);

/**
 * modest_folder_view_set_list_to_move:
 * @self: a #ModestFolderView
 * @list: a #TnyList, or %NULL for unsetting the current list.
 * 
 * list of folders or messages we're moving. This has some effects on 
 * "show_non_move_to".
 */
void modest_folder_view_set_list_to_move (ModestFolderView *self,
					  TnyList *list);

/**
 * modest_folder_view_show_message_count:
 * @self: a #ModestFolderView
 * @show: a #gboolean
 *
 * Set if the message count should be shown or not
 */
void modest_folder_view_show_message_count (ModestFolderView *self,
					    gboolean show);

/**
 * modest_folder_view_get_activity:
 * @self: a #ModestFolderView
 *
 * tells if widget is retrieving information
 *
 * Returns: %TRUE if retrieving, %FALSE otherwise
 */
gboolean modest_folder_view_get_activity (ModestFolderView *self);

/**
 * modest_folder_view_get_model_tny_list:
 * @self: a #ModestFolderView
 *
 * obtains the #TnyList model containing the accounts in the folder view.
 *
 * Returns: a caller owner #TnyList
 */
TnyList *modest_folder_view_get_model_tny_list (ModestFolderView *self);

G_END_DECLS

#endif /* __MODEST_FOLDER_VIEW_H__ */

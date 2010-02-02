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

#ifndef __MODEST_ACCOUNT_VIEW_H__
#define __MODEST_ACCOUNT_VIEW_H__

#include "modest-account-mgr.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNT_VIEW             (modest_account_view_get_type())
#define MODEST_ACCOUNT_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNT_VIEW,ModestAccountView))
#define MODEST_ACCOUNT_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNT_VIEW,GtkTreeView))
#define MODEST_IS_ACCOUNT_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNT_VIEW))
#define MODEST_IS_ACCOUNT_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNT_VIEW))
#define MODEST_ACCOUNT_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNT_VIEW,ModestAccountViewClass))

typedef struct _ModestAccountView      ModestAccountView;
typedef struct _ModestAccountViewClass ModestAccountViewClass;

struct _ModestAccountView {
	 GtkTreeView parent;
};

struct _ModestAccountViewClass {
	GtkTreeViewClass parent_class;
};


/**
 * modest_account_view_get_type
 *
 * Gets the #GType for the account view class
 * 
 * Returns: the #GType
 **/
GType        modest_account_view_get_type    (void) G_GNUC_CONST;


/**
 * modest_account_view_new:
 * @account_view: a #ModestAccountView
 * 
 * Create a new acccount view widget = ie, a list with accounts
 * 
 * Returns: a new account view widget, or NULL in case of error
 **/
ModestAccountView*   modest_account_view_new         (ModestAccountMgr *account_mgr);


/**
 * modest_account_view_get_selected_account:
 * @account_view: a #ModestAccountView
 * 
 * Gets the name of the account currently selected
 * 
 * Returns: the name of the selected account or NULL if none is
 * selected
 **/
gchar*   modest_account_view_get_selected_account    (ModestAccountView *account_view);

/**
 * modest_account_view_get_path_account:
 * @account_view: a #ModestAccountView
 * @path: a #GtkTreePath
 * 
 * Gets the name of the account pointed by @path
 * 
 * Returns: the name of the account or NULL if no account in path
 **/
gchar *  modest_account_view_get_path_account (ModestAccountView *self, GtkTreePath *path);

/**
 * modest_account_view_set_picker_mode:
 * @self: a #ModestAccountView
 * @enable: a #gboolean
 *
 * Sets the account view to be shown in picker mode (only names shown, centered).
 * This is used in Hildon2.2 to show accounts as in a picker
 */
void modest_account_view_set_picker_mode (ModestAccountView *self, gboolean enable);

/**
 * modest_account_view_get_picker_mode:
 * @self: a #ModestAccountView
 *
 * Gets if account view is in picker mode.
 *
 * Returns: %TRUE if @self in picker mode, %FALSE otherwise
 */
gboolean modest_account_view_get_picker_mode (ModestAccountView *self);

/**
 * modest_account_view_set_show_last_update:
 * @self: a #ModestAccountView
 * @show: a #gboolean
 *
 * show/hide the last update column
 */
void modest_account_view_set_show_last_update (ModestAccountView *self, gboolean show);

/**
 * modest_account_view_get_show_last_update:
 * @self: a #ModestAccountView
 *
 * tell if it's showing  the last update column
 *
 * Returns: a #gboolean
 */
gboolean modest_account_view_get_show_last_update (ModestAccountView *self);

/**
 * modest_account_view_get_model:
 * @self: a #ModestAccountView
 *
 * obtains the inner tree model
 */
GtkTreeModel *modest_account_view_get_model (ModestAccountView *self);

/**
 * modest_account_view_get_model:
 * @self: a #ModestAccountView
 *
 * obtains the filter of the tree view
 */
GtkTreeModel *modest_account_view_get_filter (ModestAccountView *self);

#ifdef MODEST_TOOLKIT_HILDON2
/**
 * modest_account_view_setup_live_search:
 * @self: a #ModestAccountView
 *
 * obtains a properly set up #HildonLiveSearch for @self
 *
 * Returns: a #HildonLiveSearch
 */
GtkWidget *modest_account_view_setup_live_search (ModestAccountView *self);
#endif

G_END_DECLS

#endif /* __MODEST_ACCOUNT_VIEW_H__ */


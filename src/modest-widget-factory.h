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

#ifndef __MODEST_WIDGET_FACTORY_H__
#define __MODEST_WIDGET_FACTORY_H__

#include <glib-object.h>
#include <glib/gi18n.h>
#include <modest-account-mgr.h>
#include <modest-tny-account-store.h>

#include <widgets/modest-header-view.h>
#include <widgets/modest-folder-view.h>
#include <widgets/modest-msg-view.h>
#include <widgets/modest-account-view.h>
#include <widgets/modest-toolbar.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_WIDGET_FACTORY             (modest_widget_factory_get_type())
#define MODEST_WIDGET_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_WIDGET_FACTORY,ModestWidgetFactory))
#define MODEST_WIDGET_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_WIDGET_FACTORY,GObject))
#define MODEST_IS_WIDGET_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_WIDGET_FACTORY))
#define MODEST_IS_WIDGET_FACTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_WIDGET_FACTORY))
#define MODEST_WIDGET_FACTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_WIDGET_FACTORY,ModestWidgetFactoryClass))

typedef struct _ModestWidgetFactory      ModestWidgetFactory;
typedef struct _ModestWidgetFactoryClass ModestWidgetFactoryClass;

struct _ModestWidgetFactory {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestWidgetFactoryClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestWidgetFactory* obj); */
};


enum _ModestComboBoxType {
	MODEST_COMBO_BOX_TYPE_STORE_PROTOS,
	MODEST_COMBO_BOX_TYPE_TRANSPORT_PROTOS,
	MODEST_COMBO_BOX_TYPE_SECURITY_PROTOS,
	MODEST_COMBO_BOX_TYPE_AUTH_PROTOS,
};
typedef enum _ModestComboBoxType ModestComboBoxType;


/**
 * modest_widget_factory_get_type
 *
 * get the GType for ModestWidgetFactory
 *
 * Returns: the GType
 */
GType        modest_widget_factory_get_type    (void) G_GNUC_CONST;


/**
 * modest_widget_factory_new
 * @conf: a modest conf instance
 * @acc_store: a modest account store instance
 * @acc_mgr: a modest account mgr instance
 *
 * instantiates a ModestWidgetFactory
 *
 * Returns: a new ModestWidgetFactory, or NULL in case of error
 */
ModestWidgetFactory*      modest_widget_factory_new   (ModestConf *conf,
						       ModestTnyAccountStore *acc_store,
						       ModestAccountMgr *account_mgr);
/**
 * modest_widget_factory_get_folder_view
 * @self: a ModestWidgetFactory instance
 * 
 * return the folder tree widget (ie. the widget with the list of folders);
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: a folder tree view, or NULL in case of error
 */
ModestFolderView*    modest_widget_factory_get_folder_view (ModestWidgetFactory *self);


/**
 * modest_widget_factory_get_header_view
 * @self: a ModestWidgetFactory instance
 * 
 * return the header tree widget (ie. the widget with the list of headers);
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: a header tree view, or NULL in case of error
 */
ModestHeaderView*    modest_widget_factory_get_header_view (ModestWidgetFactory *self);


/**
 * modest_widget_factory_get_msg_preview
 * @self: a ModestWidgetFactory instance
 * 
 * return the message preview widget (ie. the widget with shows the currently selected message);
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: a header tree view, or NULL in case of error
 */
ModestMsgView*           modest_widget_factory_get_msg_preview (ModestWidgetFactory *self);


/**
 * modest_widget_factory_get_account_view
 * @self: a ModestWidgetFactory instance
 * 
 * return an account view widget (ie. the widget that shows a list of accounts)
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: the account view, or NULL in case of error
 */
ModestAccountView*    modest_widget_factory_get_account_view (ModestWidgetFactory *self);


/**
 * modest_widget_factory_get_progress_bar
 * @self: a ModestWidgetFactory instance
 * 
 * return an progress bar widget 
 * if the widget factory was created with 'auto_connect', then this progress bar
 * will automatically update for changes in the other widgets
 * NOTE the naming inconsistency: GtkProgressBar vs GtkStatusbar 
 * 
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: the progress bar widget
 */
GtkWidget*       modest_widget_factory_get_progress_bar (ModestWidgetFactory *self);



/**
 * modest_widget_factory_get_status_bar
 * @self: a ModestWidgetFactory instance
 * 
 * return an status bar widget 
 * if the widget factory was created with 'auto_connect', then this status bar
 * will automatically update for changes in the other widgets
 * NOTE the naming inconsistency: GtkProgressBar vs GtkStatusbar 
 * 
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: the status bar widget
 */
GtkWidget*     modest_widget_factory_get_status_bar (ModestWidgetFactory *self);

/**
 * modest_widget_factory_get_store
 * @self: a ModestWidgetFactory instance
 * @type: the type of items we want a combo box for
 * 
 * return a combobox with with the given items
 *  
 * Returns: the combo box
 */
GtkWidget*     modest_widget_factory_get_combo_box (ModestWidgetFactory *self,
						    ModestComboBoxType type);


/**
 * modest_widget_factory_get_online_combo
 * @self: a ModestWidgetFactory instance
 * 
 * return a toggle which with one can see whether online/offline mode is active.
 * In case of auto-connect, this will automatically be sync'd with the
 * account_store / device
 *  
 * Returns: the combo box
 */
GtkWidget*  modest_widget_factory_get_online_toggle (ModestWidgetFactory *self);




/**
 * modest_widget_factory_get_folder_info_label
 * @self: a ModestWidgetFactory instance
 * 
 * return a label with the number of items, unread items in the current folder
 *  
 * Returns: the label
 */
GtkWidget* modest_widget_factory_get_folder_info_label (ModestWidgetFactory *self);


/**
 * modest_widget_factory_get_main_toolbar
 * @self: a ModestWidgetFactory instance
 * @items: a list of ModestToolbarButtons (button_ids)
 *
 * returns the main toolbar widget; their enabled/disabled state synchronized with
 * the other widgets. Note that after the first calling, this function will
 * always return the same toolbar, regardless of the items
 *  
 * Returns: the toolbar
 */
ModestToolbar *modest_widget_factory_get_main_toolbar (ModestWidgetFactory *self, 
						       GSList *items);

/**
 * modest_widget_factory_get_edit_toolbar
 * @self: a ModestWidgetFactory instance
 * @items: a list of ModestToolbarButtons (button_ids)
 *
 * returns the toolbar widget for edit windows; the enabled/disabled
 * state synchronized with the other widgets.
 *  
 * Returns: the toolbar
 */
ModestToolbar *modest_widget_factory_get_edit_toolbar (ModestWidgetFactory *self, 
						       GSList *items);


G_END_DECLS

#endif /* __MODEST_WIDGET_FACTORY_H__ */


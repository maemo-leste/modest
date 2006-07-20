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
#include "modest-account-mgr.h"
#include "modest-tny-account-store.h"
#include "modest-tny-header-tree-view.h"
#include "modest-tny-folder-tree-view.h"
#include "modest-tny-msg-view.h"
#include "modest-account-view.h"


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
 * @autoconnect: should we autoconnect the widgets (ie. depedent widgets are update
 * automagically)
 *
 * instantiates a ModestWidgetFactory
 *
 * Returns: a new ModestWidgetFactory, or NULL in case of error
 */
ModestWidgetFactory*      modest_widget_factory_new   (ModestConf *conf,
						       ModestTnyAccountStore *acc_store,
						       ModestAccountMgr *account_mgr,
						       gboolean auto_connect);
/**
 * modest_widget_factory_get_folder_tree_widget
 * @self: a ModestWidgetFactory instance
 * 
 * return the folder tree widget (ie. the widget with the list of folders);
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: a folder tree view, or NULL in case of error
 */
ModestTnyFolderTreeView*    modest_widget_factory_get_folder_tree_widget (ModestWidgetFactory *self);


/**
 * modest_widget_factory_get_header_tree_widget
 * @self: a ModestWidgetFactory instance
 * 
 * return the header tree widget (ie. the widget with the list of headers);
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: a header tree view, or NULL in case of error
 */
ModestTnyHeaderTreeView*    modest_widget_factory_get_header_tree_widget (ModestWidgetFactory *self);


/**
 * modest_widget_factory_get_header_tree_widget
 * @self: a ModestWidgetFactory instance
 * 
 * return the message preview widget (ie. the widget with shows the currently selected message);
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: a header tree view, or NULL in case of error
 */
ModestTnyMsgView*           modest_widget_factory_get_msg_preview_widget (ModestWidgetFactory *self);



/**
 * modest_widget_factory_get_account_view_widget
 * @self: a ModestWidgetFactory instance
 * 
 * return an account view widget (ie. the widget that shows a list of accounts)
 *
 * This factory will always return the
 * same widget, and takes care of its lifetime - users should *not* destroy it.
 *
 * Returns: the account view, or NULL in case of error
 */
ModestAccountView*    modest_widget_factory_get_account_view_widget (ModestWidgetFactory *self);





G_END_DECLS

#endif /* __MODEST_WIDGET_FACTORY_H__ */


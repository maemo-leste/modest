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

#ifndef __MODEST_UI_H__
#define __MODEST_UI_H__

#include <glib-object.h>
#include <tny-account-store.h>
#include <gtk/gtkactiongroup.h>
#include "modest-window.h"
#include "modest-edit-msg-window.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_UI             (modest_ui_get_type())
#define MODEST_UI(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_UI,ModestUI))
#define MODEST_UI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_UI,GObject))
#define MODEST_IS_UI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_UI))
#define MODEST_IS_UI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_UI))
#define MODEST_UI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_UI,ModestUIClass))

typedef struct _ModestUI      ModestUI;
typedef struct _ModestUIClass ModestUIClass;

struct _ModestUI {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestUIClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
};

/**
 * modest_ui_get_type:
 * 
 * get the GType for ModestUI
 *  
 * Returns: the GType
 */
GType        modest_ui_get_type        (void) G_GNUC_CONST;

/**
 * modest_ui_new:
 * @account_store: a #TnyAccountStore
 *  
 * Returns: a new ModestUI, or NULL in case of error
 */
ModestUI*     modest_ui_new            (TnyAccountStore *account_store);


/**
 * modest_ui_main_window:
 * @modest_ui: a ModestUI instance 
 *  
 * Creates an new main window and returns it. If there is already a
 * main window then the current one is returned and no new window is
 * created
 *
 * Returns: a #ModestMainWindow, or NULL in case of error
 */
ModestWindow*    modest_ui_main_window    (ModestUI *modest_ui);


/**
 * modest_edit_msg_ui_main_window:
 * @modest_edit_msg_ui: a ModestEditMsgUI instance 
 * @edit_type: the type of edit window
 *  
 * Creates an new main window and returns it. If there is already a
 * main window then the current one is returned and no new window is
 * created
 *
 * Returns: a #ModestEditMsgWindow, or NULL in case of error
 */
ModestWindow*    modest_ui_edit_window    (ModestUI       *modest_ui,
					   ModestEditType  edit_type);


G_END_DECLS
#endif /* __MODEST_UI_H__ */

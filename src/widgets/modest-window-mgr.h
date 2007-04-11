/* Copyright (c) 2006,2007 Nokia Corporation
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

#ifndef __MODEST_WINDOW_MGR_H__
#define __MODEST_WINDOW_MGR_H__

#include <glib-object.h>
#include "widgets/modest-msg-view-window.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_WINDOW_MGR             (modest_window_mgr_get_type())
#define MODEST_WINDOW_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_WINDOW_MGR,ModestWindowMgr))
#define MODEST_WINDOW_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_WINDOW_MGR,GObject))
#define MODEST_IS_WINDOW_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_WINDOW_MGR))
#define MODEST_IS_WINDOW_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_WINDOW_MGR))
#define MODEST_WINDOW_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_WINDOW_MGR,ModestWindowMgrClass))

typedef struct _ModestWindowMgr      ModestWindowMgr;
typedef struct _ModestWindowMgrClass ModestWindowMgrClass;

struct _ModestWindowMgr {
	 GObject parent;
	/* insert public members, if any */
};

struct _ModestWindowMgrClass {
	GObjectClass parent_class;
	/* insert signal callback declarations, eg. */
	/* void (* my_event) (ModestWindowMgr* obj); */
};

/* member functions */
GType        modest_window_mgr_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
ModestWindowMgr*    modest_window_mgr_new         (void);


/**
 * modest_window_mgr_register_window:
 * @self: the #ModestWindowMgr
 * @window: a #ModestWindow
 * 
 * Registers a new window in the window manager. The window manager
 * will keep a reference.
 **/
void           modest_window_mgr_register_window       (ModestWindowMgr *self, 
							ModestWindow *window);

/**
 * modest_window_mgr_unregister_window:
 * @self: the #ModestWindowMgr
 * @window: a #ModestWindow
 * 
 * Unegisters a given window from the window manager. The window
 * manager will free its reference to it.
 **/
void           modest_window_mgr_unregister_window     (ModestWindowMgr *self, 
							ModestWindow *window);


/**
 * modest_window_mgr_find_window_by_msguid:
 * @self: the #ModestWindowMgr
 * @msgid: the message uid
 * 
 * Looks for a #ModestWindow that shows the message specified by the
 * message uid passed as argument
 *
 * Return value: the #ModestWindow if found, else NULL
 **/
ModestWindow*  modest_window_mgr_find_window_by_msguid (ModestWindowMgr *self, 
							const gchar *msguid);

/**
 * modest_window_mgr_set_fullscreen_mode:
 * @self: 
 * @on: 
 * 
 * sets/unsets the application windows in fullscreen mode
 **/
void           modest_window_mgr_set_fullscreen_mode   (ModestWindowMgr *self,
							gboolean on);
G_END_DECLS

#endif /* __MODEST_WINDOW_MGR_H__ */


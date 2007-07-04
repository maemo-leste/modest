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
 * Unregisters a given window from the window manager. The window
 * manager will free its reference to it.
 **/
void           modest_window_mgr_unregister_window     (ModestWindowMgr *self, 
							ModestWindow *window);



/**
 * modest_window_mgr_set_fullscreen_mode:
 * @self: a #ModestWindowMgr
 * @on: a #gboolean
 * 
 * sets/unsets the application windows in fullscreen mode
 **/
void           modest_window_mgr_set_fullscreen_mode   (ModestWindowMgr *self,
							gboolean on);

/**
 * modest_window_mgr_get_fullscreen_mode:
 * @self: a #ModestWindowMgr
 * 
 * gets the application current fullscreen mode
 *
 * Return value: TRUE is the application is in fullscrenn mode,
 * otherwise FALSE
 **/
gboolean       modest_window_mgr_get_fullscreen_mode   (ModestWindowMgr *self);


/**
 * modest_window_mgr_show_toolbars:
 * @self: a #ModestWindowMgr
 * @show_toolbar: whether or not the toolbars should be shown
 * @fullscreen: TRUE/FALSE for show/hide in fullscreen mode, otherwise
 * it applies to normal mode
 * 
 * shows or hides the toolbars of the registered windows. Note that if
 * the #fullscreen attribute is TRUE and the application is in normal
 * mode, you will not see the changes until the application switches
 * to fullscreen mode and viceversa
 **/
void           modest_window_mgr_show_toolbars         (ModestWindowMgr *self,
							gboolean show_toolbars,
							gboolean fullscreen);

ModestWindow*  modest_window_mgr_get_main_window       (ModestWindowMgr *self);

/**
 * modest_window_mgr_prevent_hibernation_while_window_is_shown:
 * @self: a #ModestWindowMgr
 * @window: The window that should prevent hibernation while it is shown.
 * 
 * Call this if hibernation should not be allowed because 
 * windows are open whose state cannot sensible be saved, such as Account 
 * Settings dialogs.
 * This causes modest_window_mgr_get_hibernation_is_prevented() to return TRUE 
 * until all such windows have been closed. That means, until the windows have 
 * been hidden - window destruction or other memory management is not relevant.
 **/
void modest_window_mgr_prevent_hibernation_while_window_is_shown (ModestWindowMgr *self, 
	GtkWindow *window);


/**
 * modest_window_mgr_find_registered_header
 * @self: a #ModestWindowMgr
 * @header: a valid #TnyHeader
 * 
 * search for the given uid in both the list of preregistered uids and in the window list;
 * if it's available in the window list, fill the *win out-param
 *
 * returns TRUE if found, FALSE otherwise
 **/
gboolean modest_window_mgr_find_registered_header (ModestWindowMgr *self,  TnyHeader *header,
					       ModestWindow **win);


/**
 * modest_window_mgr_register_header
 * @self: a #ModestWindowMgr
 * @header: a valid #TnyHeader
 * 
 * register the uid, even before the window is created. thus, we know when
 * some window creation might already be underway. the uid will automatically be
 * removed when the window itself will registered
 * 
 **/
void  modest_window_mgr_register_header   (ModestWindowMgr *self,  TnyHeader *header);
	

/**
 * modest_window_mgr_unregister_header
 * @self: a #ModestWindowMgr
 * @header: a valid #TnyHeader
 * 
 * unregister the uid. We could need to do that if there is any error
 * retrieving a message. In that case the window will not be
 * registered and thus the header will not be removed, so we must do
 * it manually
 **/
void  modest_window_mgr_unregister_header (ModestWindowMgr *self,  TnyHeader *header);

/**
 * modest_window_mgr_get_hibernation_is_prevented:
 * @self: a #ModestWindowMgr
 * @result: Whether any windows are currently preventing hibernation.
 * 
 * Use this to discover whether hibernation should not be allowed because 
 * windows are open whose state cannot sensible be saved, such as Account 
 * Settings dialogs. This function will return true after someone has called 
 * modest_window_mgr_prevent_hibernation_while_window_is_shown() and before 
 * that window has been closed.
 **/
gboolean modest_window_mgr_get_hibernation_is_prevented (ModestWindowMgr *self);

/**
 * modest_window_mgr_save_state_for_all_windows:
 * @self: a #ModestWindowMgr
 * 
 * Save any state for all windows. For instance, call this before allowing 
 * application hibernation.
 **/
void modest_window_mgr_save_state_for_all_windows (ModestWindowMgr *self);
	
G_END_DECLS

#endif /* __MODEST_WINDOW_MGR_H__ */


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
#include "modest-window.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_WINDOW_MGR             (modest_window_mgr_get_type())
#define MODEST_WINDOW_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_WINDOW_MGR,ModestWindowMgr))
#define MODEST_WINDOW_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_WINDOW_MGR,ModestWindowMgrClass))
#define MODEST_IS_WINDOW_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_WINDOW_MGR))
#define MODEST_IS_WINDOW_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_WINDOW_MGR))
#define MODEST_WINDOW_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_WINDOW_MGR,ModestWindowMgrClass))

typedef struct _ModestWindowMgr      ModestWindowMgr;
typedef struct _ModestWindowMgrClass ModestWindowMgrClass;

struct _ModestWindowMgr {
	 GObject parent;
};

struct _ModestWindowMgrClass {
	GObjectClass parent_class;

	/* Virtuals */
	gboolean              (*register_window)                (ModestWindowMgr *self,
								 ModestWindow *window,
								 ModestWindow *parent);
	void                  (*unregister_window)              (ModestWindowMgr *self,
								 ModestWindow *window);
	void                  (*set_fullscreen_mode)            (ModestWindowMgr *self,
								 gboolean on);
	gboolean              (*get_fullscreen_mode)            (ModestWindowMgr *self);
	void                  (*show_toolbars)                  (ModestWindowMgr *self,
								 GType window_type,
								 gboolean show_toolbars,
								 gboolean fullscreen);
#ifndef MODEST_TOOLKIT_HILDON2
	ModestWindow *        (*get_main_window)                (ModestWindowMgr *self,
								 gboolean show);
#endif
	gboolean              (*close_all_windows)              (ModestWindowMgr *self);
	GtkWindow *           (*get_modal)                      (ModestWindowMgr *self);
	void                  (*set_modal)                      (ModestWindowMgr *self,
								 GtkWindow *window,
								 GtkWindow *parent);
	gboolean              (*find_registered_header)         (ModestWindowMgr *self,
								 TnyHeader *header,
								 ModestWindow **win);
	gboolean              (*find_registered_message_uid)    (ModestWindowMgr *self,
								 const gchar *msg_uid,
								 ModestWindow **win);
	GList *               (*get_window_list)                (ModestWindowMgr *self);
	ModestWindow *        (*show_initial_window)            (ModestWindowMgr *self);
	ModestWindow *        (*get_current_top)                (ModestWindowMgr *self);
	gboolean              (*close_all_but_initial)          (ModestWindowMgr *self);

	gboolean              (*screen_is_on)                   (ModestWindowMgr *self);
	void                  (*create_caches)                  (ModestWindowMgr *self);

	/* Signals */
	void (*window_list_empty) (ModestWindowMgr *self);
	void (*progress_list_changed) (ModestWindowMgr *self);
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
gboolean           modest_window_mgr_register_window       (ModestWindowMgr *self, 
							    ModestWindow *window,
							    ModestWindow *parent);

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
 * modest_window_mgr_register_banner:
 * @self: a #ModestWindowMgr
 *
 * Increase the count of pending banners in the window manager
 */
void           modest_window_mgr_register_banner (ModestWindowMgr *self);

/**
 * modest_window_mgr_unregister_banner:
 * @self: a #ModestWindowMgr
 *
 * Decrease the count of pending banners in the window manager
 */
void           modest_window_mgr_unregister_banner (ModestWindowMgr *self);

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
 * @window_type: apply the show toolbars command only to the windows of this type
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
							GType window_type,
							gboolean show_toolbars,
							gboolean fullscreen);
#ifndef MODEST_TOOLKIT_HILDON2
/**
 * modest_window_mgr_get_main_window:
 * @self: a #ModestWindowMgr
 * @show: if TRUE, create the main window and show it if it was not existing.
 *
 * get the main window, and depending on @create, create one if it does not exist yet
 *
 * Returns: the main window or NULL in case of error, or the main-window
 * did not yet exist
 **/
ModestWindow*  modest_window_mgr_get_main_window       (ModestWindowMgr *self,
							gboolean show);

/**
 * modest_window_mgr_set_main_window:
 * @self: a #ModestWindowMgr
 * @main_win: a #ModestMainWindow
 */
void modest_window_mgr_set_main_window (ModestWindowMgr *self, ModestWindow *main_win);


/**
 * modest_window_mgr_main_window_exists:
 * @self: a #ModestWindowMgr
 *
 * do we have a main window?
 *
 * Returns: TRUE if there's a main window, FALSE otherwise
 **/
gboolean  modest_window_mgr_main_window_exists       (ModestWindowMgr *self);
#endif

/**
 * modest_window_mgr_get_modal:
 * @self: a #ModestWindowMgr
 *
 * get the modal window; if it's NULL, there's no active modal window
 *
 * Returns: the modal window or NULL
 **/
GtkWindow*    modest_window_mgr_get_modal  (ModestWindowMgr *self);


/**
 * modest_window_mgr_get_easysetup_dialog:
 * @self: a #ModestWindowMgr
 *
 * set the modal dialog; set it to NULL after destroying the dialog
 *
 **/
void          modest_window_mgr_set_modal  (ModestWindowMgr *self,
					    GtkWindow *window,
					    GtkWindow *parent);

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
 * modest_window_mgr_register_help_id
 * @self: a #ModestWindowMgr
 * @win: some window
 * @help_id: the help_id for this window
 * 
 * register a help id for a window
 **/
void
modest_window_mgr_register_help_id (ModestWindowMgr *self, GtkWindow *win, const gchar* help_id);


/**
 * modest_window_mgr_get_help_id:
 * @self: a #ModestWindowMgr
 * @win: some window
 * 
 * get the help id for a window; if the window is the main-window and some folder is
 * selected, it will return the proper help_id for that
 *
 * Returns: a help _id
 **/
const gchar*
modest_window_mgr_get_help_id (ModestWindowMgr *self, GtkWindow *win);



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
 * modest_window_mgr_find_registered_header
 * @self: a #ModestWindowMgr
 * @msg_uid: a message uid
 * 
 * search for the given uid in both the list of preregistered uids and in the window list;
 * if it's available in the window list, fill the *win out-param
 *
 * returns TRUE if found, FALSE otherwise
 **/
gboolean modest_window_mgr_find_registered_message_uid (ModestWindowMgr *self,  const gchar *msg_uid,
							ModestWindow **win);

/**
 * modest_window_mgr_get_window_list:
 * @self: a #ModestWindowMgr
 *
 * get the list of windows registered in window mgr.
 *
 * Returns: a #GList, that caller should free
 */
GList *modest_window_mgr_get_window_list (ModestWindowMgr *self);

/**
 * modest_window_mgr_close_all_windows
 * @self: a #ModestWindowMgr
 * 
 * Close all registered windows. If failed (for example because
 * one confirmation dialog rejected the operation), return %FALSE
 **/
gboolean modest_window_mgr_close_all_windows (ModestWindowMgr *self);

/**
 * modest_window_mgr_register_header
 * @self: a #ModestWindowMgr
 * @header: a valid #TnyHeader
 * @alt_uid: alternative uid in case @header does not provide one
 * 
 * register the uid, even before the window is created. thus, we know when
 * some window creation might already be underway. the uid will automatically be
 * removed when the window itself will registered
 * 
 **/
void  modest_window_mgr_register_header   (ModestWindowMgr *self,  TnyHeader *header, const gchar *alt_uid);
	

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

/**
 * modest_window_mgr_get_num_windows:
 * @self: a #ModestWindowMgr
 * 
 * Gets the number of already registered windows, and pending banners
 *
 * Returns: the number of already registered windows, and pending banners
 **/
guint modest_window_mgr_get_num_windows (ModestWindowMgr *self);

/**
 * modest_window_mgr_get_msg_view_window:
 * @self: a #ModestWindowMgr
 *
 * obtain a cached #ModestMsgViewWindow if available, or construct a new
 * one.
 *
 * Returns: a #ModestMsgViewWindow
 */
GtkWidget *   modest_window_mgr_get_msg_view_window (ModestWindowMgr *self);

/**
 * modest_window_mgr_get_msg_edit_window:
 * @self: a #ModestWindowMgr
 *
 * obtain a cached #ModestMsgEditWindow if available, or construct a new
 * one.
 *
 * Returns: a #ModestMsgEditWindow
 */
GtkWidget *   modest_window_mgr_get_msg_edit_window (ModestWindowMgr *self);

/**
 * modest_window_mgr_show_initial_window:
 * @self: a #ModestWindowMgr
 *
 * This function determines which is the window that should be
 * initially launched. After that it shows that window. If there is no
 * account available this function also launches the account setup
 * wizard.
 *
 * Returns: the initial window. NULL if something goes wrong
 **/
ModestWindow* modest_window_mgr_show_initial_window (ModestWindowMgr *self);

/**
 * modest_window_mgr_get_current_top:
 * @self: a #ModestWindowMgr
 * 
 * this function returns the ModestWindow that is currently on top of all the others
 * 
 * Returns: the topmost #ModestWindow
 **/
ModestWindow* modest_window_mgr_get_current_top (ModestWindowMgr *self);

/**
 * modest_window_mgr_screen_is_on:
 * @self: a #ModestWindowMgr
 *
 * this function returns if device display is on or not
 *
 * Returns: %TRUE if screen is on, %FALSE if not
 */
gboolean modest_window_mgr_screen_is_on (ModestWindowMgr *self);

/**
 * modest_window_mgr_get_progress_operations:
 * @self: a #ModestWindowMgr
 *
 * get a list of operations in progress
 *
 * Returns: a newly allocated #TnyList
 */
TnyList *modest_window_mgr_get_progress_operations (ModestWindowMgr *self);

/**
 * modest_window_mgr_has_progress_operation:
 * @self: a #ModestWindowMgr
 *
 * is there are send/receive or receive operations in progress for any account.
 *
 * Returns: %TRUE if receive progress operations are in progress.
 */
gboolean modest_window_mgr_has_progress_operation (ModestWindowMgr *self);

/**
 * modest_window_mgr_has_progress_operation_on_account:
 * @self: a #ModestWindowMgr
 * @account_name: a string
 *
 * is there are send/receive or receive operations in progress for @account_name.
 *
 * Returns: %TRUE if receive progress operations are in progress.
 */
gboolean modest_window_mgr_has_progress_operation_on_account (ModestWindowMgr *self,
							      const gchar *account_name);

/**
 * modest_window_mgr_create_caches:
 * @self: a #ModestWindowMgr
 *
 * creates cached windows. This is called in modest main, just before the show-ui
 * code. So it should be valid for the two startup ways (with --show-ui and without).
 */
void modest_window_mgr_create_caches (ModestWindowMgr *self);

/**
 * modest_window_mgr_close_all_but_initial:
 * @self: a #ModestWindowMgr
 *
 * closes all but the initial window.
 *
 * Returns: %TRUE if the initial window is returned in @initial_window
 * and it does not have any other dialog or window on top, otherwhise
 * %FALSE
 */
gboolean modest_window_mgr_close_all_but_initial (ModestWindowMgr *self);


G_END_DECLS

#endif /* __MODEST_WINDOW_MGR_H__ */

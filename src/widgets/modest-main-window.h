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


#ifndef __MODEST_MAIN_WINDOW_H__
#define __MODEST_MAIN_WINDOW_H__

#include <gtk/gtk.h>
#include <widgets/modest-window.h>
#include <widgets/modest-header-view.h>
#include <widgets/modest-folder-view.h>
#include <widgets/modest-msg-view.h>
#include <widgets/modest-msg-view-window.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAIN_WINDOW             (modest_main_window_get_type())
#define MODEST_MAIN_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindow))
#define MODEST_MAIN_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MAIN_WINDOW,ModestWindow))

#define MODEST_IS_MAIN_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_IS_MAIN_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MAIN_WINDOW))
#define MODEST_MAIN_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MAIN_WINDOW,ModestMainWindowClass))

typedef struct _ModestMainWindow      ModestMainWindow;
typedef struct _ModestMainWindowClass ModestMainWindowClass;

struct _ModestMainWindow {
	ModestWindow parent;
};

struct _ModestMainWindowClass {
	ModestWindowClass parent_class;
};

/*
 * MODEST_MAIN_WINDOW_STYLE_SIMPLE: shows only the header list
 * MODEST_MAIN_WINDOW_STYLE_SPLIT: shows a right pane with the folder
 * tree and a left pane with the header list
 */
typedef enum _ModestMainWindowStyle {
	MODEST_MAIN_WINDOW_STYLE_SIMPLE,
	MODEST_MAIN_WINDOW_STYLE_SPLIT
} ModestMainWindowStyle;

/*
 * MODEST_MAIN_WINDOW_FOLDER_CONTENTS_STYLE_HEADERS
 * MODEST_MAIN_WINDOW_FOLDER_CONTENTS_STYLE_HEADERS
 */
typedef enum _ModestMainWindowContentsStyle {
	MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS,
	MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS,
	MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY,
	MODEST_MAIN_WINDOW_CONTENTS_STYLE_FOLDERS,
} ModestMainWindowContentsStyle;

/* toolbar modes  */
typedef enum _ModestToolBarModes {
	TOOLBAR_MODE_NORMAL,
	TOOLBAR_MODE_TRANSFER,	
} ModestToolBarModes;

/**
 * modest_main_window_get_type:
 * 
 * get the GType for the ModestMainWindow class
 *
 * Returns: a GType for ModestMainWindow
 */
GType modest_main_window_get_type (void) G_GNUC_CONST;


/**
 * modest_main_window_new
 * 
 * instantiates a new ModestMainWindow widget
 *
 * Returns: a new ModestMainWindow, or NULL in case of error
 */
ModestWindow* modest_main_window_new (void);

/*
 * we could use the GType instead, but that would require
 * that there only on widget of a certain type; that is
 * true now, but might not be. Therefore, these types
 */
typedef enum {
	MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW,
	MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW,
	MODEST_MAIN_WINDOW_WIDGET_TYPE_MSG_PREVIEW,
	
	MODEST_MAIN_WINDOW_WIDGET_TYPE_NUM,
} ModestMainWindowWidgetType;


/**
 * modest_main_window_get_child_widget
 * 
 * get a child window for this window
 *
 * Returns: a child window or NULL
 */
GtkWidget* modest_main_window_get_child_widget (ModestMainWindow *self,
						ModestMainWindowWidgetType widget_type);


/**
 * modest_main_window_set_style:
 * @self: the #ModestMainWindow
 * @style: a #ModestMainWindowStyle that will be set
 * 
 * sets the style of the main window, a splitview with folders at
 * the left and messages at the right, or the simple view, with just
 * messages.
 **/
void       modest_main_window_set_style        (ModestMainWindow *self, 
						ModestMainWindowStyle style);

/**
 * modest_main_window_get_style:
 * @self: 
 * 
 * gets the current show style of the main window
 * 
 * Return value: the current #ModestWindowStyle
 **/
ModestMainWindowStyle       modest_main_window_get_style        (ModestMainWindow *self);

/**
 * modest_main_window_set_contents_style:
 * @self: the #ModestMainWindow
 * @style: a #ModestMainWindowContentsStyle that will be set. Either headers or details.
 * 
 * Shows either the folder details, or the header list of the current
 * selected folder.
 **/
void       modest_main_window_set_contents_style       (ModestMainWindow *self, 
							ModestMainWindowContentsStyle style);

/**
 * modest_main_window_get_contents_style:
 * @self: the #ModestMainWindow
 * 
 * Gets the currently selected #ModestMainWindowContentsStyle
 * 
 * Returns: the #ModestMainWindowContentsStyle of the main window
 **/
ModestMainWindowContentsStyle modest_main_window_get_contents_style (ModestMainWindow *self);



/**
 * modest_main_window_notify_send_receive_initied:
 * @self: the #ModestMainWindow
 * 
 * Determines if send&receive operaiton is currently in 
 * progress.
 *
 * Returns: TRUE if send$receive operaton is in 
 * progress, FALSE otherwise.
 **/
gboolean  modest_main_window_send_receive_in_progress       (ModestMainWindow *self);

/**
 * modest_main_window_notify_send_receive_initied:
 * @self: the #ModestMainWindow
 * 
 * Notifies main window that send/receive operaiton was just started. 
 **/
void      modest_main_window_notify_send_receive_initied    (ModestMainWindow *self);

/**
 * modest_main_window_notify_send_receive_completed:
 * @self: the #ModestMainWindow
 * 
 * Notifies main window that send/receive operaiton was completed. 
 **/
void      modest_main_window_notify_send_receive_completed  (ModestMainWindow *self);


gboolean  modest_main_window_on_msg_view_window_msg_changed (ModestMsgViewWindow *view_window,
							     GtkTreeModel *model,
							     GtkTreeRowReference *row_reference,
							     ModestMainWindow *self);

/**
 * modest_main_window_transfer_mode_enabled:
 * @window: a #ModestMainWindow
 *
 * Determines if some transfer operation is in progress.
 *
 * Returns: TRUE if transfer mode is enabled, FASE otherwise.
*/
gboolean  modest_main_window_transfer_mode_enabled (ModestMainWindow *self);

gboolean  modest_main_window_screen_is_on (ModestMainWindow *self);

G_END_DECLS

#endif /* __MODEST_MAIN_WINDOW_H__ */

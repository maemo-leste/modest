/* Copyright (c) 2008 Nokia Corporation
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

#ifndef __MODEST_FOLDER_WINDOW_H__
#define __MODEST_FOLDER_WINDOW_H__

#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-hildon2-window.h>
#endif
#include <widgets/modest-window.h>
#include <widgets/modest-folder-view.h>
#include <tny-folder-store-query.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_FOLDER_WINDOW             (modest_folder_window_get_type())
#define MODEST_FOLDER_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_FOLDER_WINDOW,ModestFolderWindow))
#define MODEST_FOLDER_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_FOLDER_WINDOW,ModestWindow))

#define MODEST_IS_FOLDER_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_FOLDER_WINDOW))
#define MODEST_IS_FOLDER_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_FOLDER_WINDOW))
#define MODEST_FOLDER_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_FOLDER_WINDOW,ModestFolderWindowClass))

typedef struct _ModestFolderWindow      ModestFolderWindow;
typedef struct _ModestFolderWindowClass ModestFolderWindowClass;

struct _ModestFolderWindow {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2Window parent;
#else
	ModestWindow parent;
#endif
};

struct _ModestFolderWindowClass {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2WindowClass parent_class;
#else
	ModestWindowClass parent_class;
#endif
};

/**
 * modest_folder_window_get_type:
 * 
 * get the GType for the ModestFolderWindow class
 *
 * Returns: a GType for ModestFolderWindow
 */
GType modest_folder_window_get_type (void) G_GNUC_CONST;


/**
 * modest_folder_window_new:
 * @query: a #TnyFolderStoreQuery that specifies the folders to show
 * 
 * instantiates a new ModestFolderWindow widget
 *
 * Returns: a new ModestFolderWindow, or NULL in case of error
 */
ModestWindow* modest_folder_window_new (TnyFolderStoreQuery *query);

/**
 * modest_folder_window_get_folder_view:
 * @self: a #ModestFolderWindow
 *
 * get the folder view inside the folder window
 */
ModestFolderView *modest_folder_window_get_folder_view (ModestFolderWindow *self);

/**
 * modest_folder_window_set_account:
 * @self: a #ModestFolderWindow
 * @account_name: a string
 *
 * Sets the current active account in the folder window.
 */
void modest_folder_window_set_account (ModestFolderWindow *self,
				       const gchar *account_name);

/**
 * modest_folder_window_set_mailbox:
 * @self: a #ModestFolderWindow
 * @mailbox: a string
 *
 * Sets the current mailbox in the folder window.
 */
void modest_folder_window_set_mailbox (ModestFolderWindow *self,
				       const gchar *mailbox);

/**
 * modest_folder_Window_transfer_mode_enabled:
 * @self: a #ModestFolderWindow
 *
 * if @self is in transfer mode (progress hint visible)
 *
 * Returns: %TRUE if progress hint should be visible
 */
gboolean modest_folder_window_transfer_mode_enabled (ModestFolderWindow *self);

G_END_DECLS

#endif

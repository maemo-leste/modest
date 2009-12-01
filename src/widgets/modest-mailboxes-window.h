/* Copyright (c) 2009 Nokia Corporation
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

#ifndef __MODEST_MAILBOXES_WINDOW_H__
#define __MODEST_MAILBOXES_WINDOW_H__

#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-hildon2-window.h>
#else
#include <modest-shell-window.h>
#endif
#include <widgets/modest-window.h>
#include <widgets/modest-folder-view.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAILBOXES_WINDOW             (modest_mailboxes_window_get_type())
#define MODEST_MAILBOXES_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAILBOXES_WINDOW,ModestMailboxesWindow))
#define MODEST_MAILBOXES_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MAILBOXES_WINDOW,ModestWindow))

#define MODEST_IS_MAILBOXES_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAILBOXES_WINDOW))
#define MODEST_IS_MAILBOXES_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MAILBOXES_WINDOW))
#define MODEST_MAILBOXES_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MAILBOXES_WINDOW,ModestMailboxesWindowClass))

typedef struct _ModestMailboxesWindow      ModestMailboxesWindow;
typedef struct _ModestMailboxesWindowClass ModestMailboxesWindowClass;

struct _ModestMailboxesWindow {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2Window parent;
#else
	ModestWindow parent;
#endif
};

struct _ModestMailboxesWindowClass {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2WindowClass parent_class;
#else
	ModestShellWindowClass parent_class;
#endif
};

/**
 * modest_mailboxes_window_get_type:
 * 
 * get the GType for the ModestMailboxesWindow class
 *
 * Returns: a GType for ModestMailboxesWindow
 */
GType modest_mailboxes_window_get_type (void) G_GNUC_CONST;


/**
 * modest_mailboxes_window_new:
 * @query: a #TnyMailboxesStoreQuery that specifies the mailboxess to show
 * 
 * instantiates a new ModestMailboxesWindow widget
 *
 * Returns: a new ModestMailboxesWindow, or NULL in case of error
 */
ModestWindow* modest_mailboxes_window_new (const gchar *account_name);

/**
 * modest_mailboxes_window_get_mailboxes_view:
 * @self: a #ModestMailboxesWindow
 *
 * get the mailboxes view inside the mailboxes window
 */
ModestFolderView *modest_mailboxes_window_get_folder_view (ModestMailboxesWindow *self);

/**
 * modest_mailboxes_window_set_account:
 * @self: a #ModestMailboxesWindow
 * @account_name: a string
 *
 * Sets the current active account in the mailboxes window.
 */
void modest_mailboxes_window_set_account (ModestMailboxesWindow *self,
					  const gchar *account_name);

G_END_DECLS

#endif

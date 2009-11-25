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

#ifndef __MODEST_ACCOUNTS_WINDOW_H__
#define __MODEST_ACCOUNTS_WINDOW_H__

#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-hildon2-window.h>
#endif
#include <widgets/modest-window.h>
#include <widgets/modest-account-view.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_ACCOUNTS_WINDOW             (modest_accounts_window_get_type())
#define MODEST_ACCOUNTS_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_ACCOUNTS_WINDOW,ModestAccountsWindow))
#define MODEST_ACCOUNTS_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_ACCOUNTS_WINDOW,ModestWindow))

#define MODEST_IS_ACCOUNTS_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_ACCOUNTS_WINDOW))
#define MODEST_IS_ACCOUNTS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_ACCOUNTS_WINDOW))
#define MODEST_ACCOUNTS_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_ACCOUNTS_WINDOW,ModestAccountsWindowClass))

typedef struct _ModestAccountsWindow      ModestAccountsWindow;
typedef struct _ModestAccountsWindowClass ModestAccountsWindowClass;

struct _ModestAccountsWindow {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2Window parent;
#else
	ModestWindow parent;
#endif
};

struct _ModestAccountsWindowClass {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2WindowClass parent_class;
#else
	ModestWindowClass parent_class;
#endif
};

/**
 * modest_accounts_window_get_type:
 * 
 * get the GType for the ModestAccountsWindow class
 *
 * Returns: a GType for ModestAccountsWindow
 */
GType modest_accounts_window_get_type (void) G_GNUC_CONST;


/**
 * modest_accounts_window_new:
 * 
 * instantiates a new ModestAccountsWindow widget
 *
 * Returns: a new ModestAccountsWindow, or NULL in case of error
 */
ModestWindow* modest_accounts_window_new ();

/**
 * modest_accounts_window_get_accounts_view:
 * @self: a #ModestAccountsWindow
 *
 * get the account view inside the accounts window
 */
ModestAccountView *modest_accounts_window_get_account_view (ModestAccountsWindow *self);

/**
 * modest_accounts_window_pre_create:
 *
 * Creates an instance of #ModestAccountsWindow that will be used in the next call
 * to modest_accounts_window_new (). Should be called in the initialisation process
 *
 */
void modest_accounts_window_pre_create (void);

G_END_DECLS

#endif

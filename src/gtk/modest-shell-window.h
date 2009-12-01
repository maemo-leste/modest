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

#ifndef __MODEST_SHELL_WINDOW_H__
#define __MODEST_SHELL_WINDOW_H__

#include <widgets/modest-window.h>
#include <widgets/modest-account-view.h>
#include <modest-dimming-rule.h>
#include <modest-shell.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_SHELL_WINDOW             (modest_shell_window_get_type())
#define MODEST_SHELL_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_SHELL_WINDOW,ModestShellWindow))
#define MODEST_SHELL_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_SHELL_WINDOW,ModestWindow))

#define MODEST_IS_SHELL_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_SHELL_WINDOW))
#define MODEST_IS_SHELL_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_SHELL_WINDOW))
#define MODEST_SHELL_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_SHELL_WINDOW,ModestShellWindowClass))

typedef struct _ModestShellWindow      ModestShellWindow;
typedef struct _ModestShellWindowClass ModestShellWindowClass;

struct _ModestShellWindow {
	ModestWindow parent;
};

struct _ModestShellWindowClass {
	ModestWindowClass parent_class;

};

/**
 * modest_shell_window_get_type:
 * 
 * get the GType for the ModestShellWindow class
 *
 * Returns: a GType for ModestShellWindow
 */
GType modest_shell_window_get_type (void) G_GNUC_CONST;


void modest_shell_window_set_shell (ModestShellWindow *self,
				    ModestShell *shell);

GtkWidget *modest_shell_window_get_menu (ModestShellWindow *self);

G_END_DECLS

#endif

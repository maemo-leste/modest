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

#ifndef __MODEST_HEADER_WINDOW_H__
#define __MODEST_HEADER_WINDOW_H__

#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-hildon2-window.h>
#else
#include <modest-shell-window.h>
#endif
#include <widgets/modest-window.h>
#include <widgets/modest-header-view.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_HEADER_WINDOW             (modest_header_window_get_type())
#define MODEST_HEADER_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_HEADER_WINDOW,ModestHeaderWindow))
#define MODEST_HEADER_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_HEADER_WINDOW,ModestWindow))

#define MODEST_IS_HEADER_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_HEADER_WINDOW))
#define MODEST_IS_HEADER_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_HEADER_WINDOW))
#define MODEST_HEADER_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_HEADER_WINDOW,ModestHeaderWindowClass))

typedef struct _ModestHeaderWindow      ModestHeaderWindow;
typedef struct _ModestHeaderWindowClass ModestHeaderWindowClass;

struct _ModestHeaderWindow {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2Window parent;
#else
	ModestShellWindow parent;
#endif
};

struct _ModestHeaderWindowClass {
#ifdef MODEST_TOOLKIT_HILDON2
	ModestHildon2WindowClass parent_class;
#else
	ModestShellWindowClass parent_class;
#endif
};

/**
 * modest_header_window_get_type:
 * 
 * get the GType for the ModestHeaderWindow class
 *
 * Returns: a GType for ModestHeaderWindow
 */
GType modest_header_window_get_type (void) G_GNUC_CONST;


/**
 * modest_header_window_new:
 * @folder: a #TnyFolder that specifies the folder to show headers
 * 
 * instantiates a new ModestHeaderWindow widget
 *
 * Returns: a new ModestHeaderWindow, or NULL in case of error
 */
ModestWindow* modest_header_window_new (TnyFolder *folder, const gchar *account_name, const gchar *mailbox);

/**
 * modest_header_window_get_header_view:
 * @self: a #ModestHeaderWindow
 *
 * get the header view inside the header window
 */
ModestHeaderView *modest_header_window_get_header_view (ModestHeaderWindow *self);

gboolean  modest_header_window_toolbar_on_transfer_mode     (ModestHeaderWindow *self);
gboolean  modest_header_window_transfer_mode_enabled (ModestHeaderWindow *self);


G_END_DECLS

#endif

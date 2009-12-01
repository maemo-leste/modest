/* Copyright (c) 2009, Nokia Corporation
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

#ifndef __MODEST_SHELL_H__
#define __MODEST_SHELL_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <widgets/modest-window.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_SHELL             (modest_shell_get_type())
#define MODEST_SHELL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_SHELL,ModestShell))
#define MODEST_SHELL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_SHELL,ModestShellClass))
#define MODEST_IS_SHELL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_SHELL))
#define MODEST_IS_SHELL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_SHELL))
#define MODEST_SHELL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_SHELL,ModestShellClass))

typedef struct _ModestShell      ModestShell;
typedef struct _ModestShellClass ModestShellClass;

struct _ModestShell {
	GtkWindow parent;
};

struct _ModestShellClass {
	GtkWindowClass parent_class;
};


/* member functions */
GType        modest_shell_get_type    (void) G_GNUC_CONST;

/* typical parameter-less _new function */
GtkWidget*    modest_shell_new  (void);

ModestWindow *modest_shell_peek_window (ModestShell *shell);
gboolean modest_shell_delete_window (ModestShell *shell, ModestWindow *window);
void modest_shell_add_window (ModestShell *shell, ModestWindow *window);
gint modest_shell_count_windows (ModestShell *shell);

void modest_shell_set_title (ModestShell *shell, ModestWindow *window, const gchar *title);
void modest_shell_show_progress (ModestShell *shell, ModestWindow *window, gboolean show);


G_END_DECLS

#endif /* __MODEST_SHELL_H__ */


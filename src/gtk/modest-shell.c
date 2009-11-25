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

#include <string.h>
#include <modest-shell.h>

/* 'private'/'protected' functions */
static void modest_shell_class_init (ModestShellClass *klass);
static void modest_shell_instance_init (ModestShell *obj);
static void modest_shell_finalize   (GObject *obj);


typedef struct _ModestShellPrivate ModestShellPrivate;
struct _ModestShellPrivate {
	GtkWidget *main_vbox;
	GtkWidget *notebook;
};
#define MODEST_SHELL_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
								      MODEST_TYPE_SHELL, \
								      ModestShellPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

GType
modest_shell_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestShellClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_shell_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestShell),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_shell_instance_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
		                                  "ModestShell",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_shell_class_init (ModestShellClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_shell_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestShellPrivate));

}

static void
modest_shell_instance_init (ModestShell *obj)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE(obj);

	priv->main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->main_vbox);

	priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_widget_show (priv->notebook);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), priv->notebook, TRUE, TRUE, 0);

}

static void
modest_shell_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_shell_new (void)
{
	return (GtkWidget *) g_object_new(MODEST_TYPE_SHELL, NULL);
}

ModestWindow *
modest_shell_peek_window (ModestShell *shell)
{
	ModestShellPrivate *priv;
	gint count;

	priv = MODEST_SHELL_GET_PRIVATE (shell);
	count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));

	if (count > 0) {
		return (ModestWindow *) gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), count - 1);
	} else {
		return NULL;
	}
}

gboolean
modest_shell_delete_window (ModestShell *shell, ModestWindow *window)
{
	ModestShellPrivate *priv;
	gboolean ret_value;

	priv = MODEST_SHELL_GET_PRIVATE (shell);
	g_signal_emit_by_name (G_OBJECT (window), "delete-event", NULL, &ret_value);
	if (ret_value == FALSE) {
		gint page_num;
		
		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (priv->notebook), GTK_WIDGET (window));
		if (page_num != -1) {
			gtk_notebook_remove_page (GTK_NOTEBOOK (priv->notebook), page_num);
		}
	}

	return ret_value;
}

void
modest_shell_add_window (ModestShell *shell, ModestWindow *window)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), GTK_WIDGET (window), NULL);
}

gint
modest_shell_count_windows (ModestShell *shell)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);

	return gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
}

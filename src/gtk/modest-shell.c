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
#include <modest-shell-window.h>

/* 'private'/'protected' functions */
static void modest_shell_class_init (ModestShellClass *klass);
static void modest_shell_instance_init (ModestShell *obj);
static void modest_shell_finalize   (GObject *obj);

static void update_title (ModestShell *self);

static void on_back_button_clicked (GtkToolButton *button, ModestShell *self);


typedef struct _ModestShellPrivate ModestShellPrivate;
struct _ModestShellPrivate {
	GtkWidget *main_vbox;
	GtkWidget *notebook;
	GtkWidget *top_toolbar;
	GtkToolItem *back_button;
	GtkToolItem *title_button;
	GtkWidget *title_label;
	GtkWidget *subtitle_label;
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
	GtkWidget *title_vbox;

	priv = MODEST_SHELL_GET_PRIVATE(obj);

	priv->main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->main_vbox);

	priv->top_toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_style (GTK_TOOLBAR (priv->top_toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_widget_show (priv->top_toolbar);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), priv->top_toolbar, FALSE, FALSE, 0);

	priv->back_button = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->top_toolbar), priv->back_button, -1);
	gtk_widget_show (GTK_WIDGET (priv->back_button));
	g_signal_connect (G_OBJECT (priv->back_button), "clicked", G_CALLBACK (on_back_button_clicked), obj);

	title_vbox = gtk_vbox_new (FALSE, 0);
	priv->title_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title_label), 0.0, 1.0);
	priv->subtitle_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->subtitle_label), 0.0, 0.0);
	gtk_widget_show (priv->title_label);
	gtk_widget_show (priv->subtitle_label);
	gtk_box_pack_start (GTK_BOX (title_vbox), priv->title_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (title_vbox), priv->subtitle_label, FALSE, FALSE, 0);
	gtk_widget_show (title_vbox);

	priv->title_button = gtk_tool_button_new (NULL, NULL);
	gtk_widget_show (GTK_WIDGET (priv->title_button));
	gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (priv->title_button), title_vbox);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->top_toolbar), priv->title_button, -1);
	gtk_container_child_set (GTK_CONTAINER (priv->top_toolbar), GTK_WIDGET (priv->title_button), "expand", TRUE, NULL);
	g_object_set (priv->title_button, "is-important", TRUE, NULL);

	priv->notebook = gtk_notebook_new ();
	gtk_widget_show (priv->notebook);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), priv->notebook, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (obj), priv->main_vbox);

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

	update_title (shell);

	return ret_value;
}

void
modest_shell_add_window (ModestShell *shell, ModestWindow *window)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), GTK_WIDGET (window), NULL);
	gtk_widget_show (GTK_WIDGET (window));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), -1);
	modest_shell_window_set_shell (MODEST_SHELL_WINDOW (window), shell);
	update_title (shell);
}

gint
modest_shell_count_windows (ModestShell *shell)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);

	return gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
}

void
modest_shell_set_title (ModestShell *shell, ModestWindow *window, const gchar *title)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);

	gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (priv->notebook), GTK_WIDGET (window), title);

	update_title (shell);
}

void
modest_shell_show_progress (ModestShell *shell, ModestWindow *window, gboolean show)
{
}

static void
update_title (ModestShell *self)
{
	gint n_pages, i;
	ModestShellPrivate *priv;
	GtkWidget *child;
	GString *title_buffer;
	GString *subtitle_buffer;

	priv = MODEST_SHELL_GET_PRIVATE (self);

	n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
	if (n_pages == 0) {
		gtk_label_set_text (GTK_LABEL (priv->title_label), "");
		gtk_label_set_text (GTK_LABEL (priv->subtitle_label), "");
		return;
	}

	child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), n_pages - 1);
	title_buffer = g_string_new ("");
	title_buffer = g_string_append (title_buffer, "<b>");
	title_buffer = g_string_append (title_buffer, gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (priv->notebook), child));
	title_buffer = g_string_append (title_buffer, "</b>");
	gtk_label_set_markup (GTK_LABEL (priv->title_label), 
			      title_buffer->str);
	g_string_free (title_buffer, TRUE);

	subtitle_buffer = g_string_new ("");
	subtitle_buffer = g_string_append (subtitle_buffer, "<small>");
	for (i = 0; i < n_pages - 1; i++) {
	child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), i);
		if (i != 0) {
			subtitle_buffer = g_string_append (subtitle_buffer, " / ");
		}
		subtitle_buffer = g_string_append (subtitle_buffer,
						   gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (priv->notebook), child));
	}
	subtitle_buffer = g_string_append (subtitle_buffer, "</small>");
	gtk_label_set_markup (GTK_LABEL (priv->subtitle_label), 
			      subtitle_buffer->str);
	g_string_free (subtitle_buffer, TRUE);
}

static void
on_back_button_clicked (GtkToolButton *button, ModestShell *self)
{
	ModestShellPrivate *priv;
	gint n_pages;
	gboolean delete_event_retval;
	GtkWidget *child;

	priv = MODEST_SHELL_GET_PRIVATE (self);

	n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
	if (n_pages < 1)
		return;

	child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), -1);
	g_signal_emit_by_name (G_OBJECT (child), "delete-event", NULL, &delete_event_retval);

	if (!delete_event_retval) {
		update_title (self);
	}
}

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
#include <modest-icon-names.h>
#include <modest-ui-actions.h>

/* 'private'/'protected' functions */
static void modest_shell_class_init (ModestShellClass *klass);
static void modest_shell_instance_init (ModestShell *obj);
static void modest_shell_finalize   (GObject *obj);

static void update_title (ModestShell *self);

static void on_back_button_clicked (GtkToolButton *button, ModestShell *self);
static void on_title_button_clicked (GtkToolButton *button, ModestShell *self);
static void on_new_msg_button_clicked (GtkToolButton *button, ModestShell *self);
static void on_style_set (GtkWidget *widget, GtkStyle *old_style, ModestShell *shell);
static gboolean on_key_pressed (GtkWidget *widget, GdkEventKey *event, ModestShell *shell);


typedef struct _ModestShellPrivate ModestShellPrivate;
struct _ModestShellPrivate {
	GtkWidget *main_vbox;
	GtkWidget *notebook;
	GtkWidget *top_toolbar;
	GtkToolItem *new_message_button;
	GtkToolItem *back_button;
	GtkToolItem *title_button;
	GtkWidget *title_label;
	GtkWidget *subtitle_label;

	GtkWidget *progress_icon;
	GdkPixbuf **progress_frames;
	gint next_frame;
	guint progress_timeout_id;

	GtkWidget *banners_box;
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
	GtkWidget *title_arrow;
	GtkWidget *new_message_icon;
	GtkToolItem *separator_toolitem;
	GtkWidget *top_hbox;
	GtkWidget *separator;

	priv = MODEST_SHELL_GET_PRIVATE(obj);
	priv->progress_frames = g_malloc0 (sizeof(GdkPixbuf *)*31);
	priv->progress_timeout_id = 0;
	priv->next_frame = 0;

	priv->main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->main_vbox);

	top_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (top_hbox);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), top_hbox, FALSE, FALSE, 0);

	separator = gtk_hseparator_new ();
	gtk_widget_show (separator);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), separator, FALSE, FALSE, 0);

	priv->top_toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_style (GTK_TOOLBAR (priv->top_toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (priv->top_toolbar), FALSE);
	gtk_widget_show (priv->top_toolbar);
	gtk_box_pack_start (GTK_BOX (top_hbox), priv->top_toolbar, TRUE, TRUE, 0);

	priv->progress_icon = gtk_image_new ();
	gtk_widget_show (priv->progress_icon);
	gtk_box_pack_start (GTK_BOX (top_hbox), priv->progress_icon, FALSE, FALSE, 0);

	priv->banners_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->banners_box);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), priv->banners_box, FALSE, FALSE, 0);

	new_message_icon = gtk_image_new_from_icon_name (MODEST_TOOLBAR_ICON_NEW_MAIL, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (new_message_icon);
	priv->new_message_button = gtk_tool_button_new (new_message_icon, _("mcen_va_new_email"));
	g_object_set (priv->new_message_button, "is-important", TRUE, NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->top_toolbar), priv->new_message_button, -1);
	gtk_widget_show (GTK_WIDGET (priv->new_message_button));
	g_signal_connect (G_OBJECT (priv->new_message_button), "clicked", G_CALLBACK (on_new_msg_button_clicked), obj);

	priv->back_button = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
	g_object_set (priv->back_button, "is-important", TRUE, NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->top_toolbar), priv->back_button, -1);
	gtk_widget_show (GTK_WIDGET (priv->back_button));
	g_signal_connect (G_OBJECT (priv->back_button), "clicked", G_CALLBACK (on_back_button_clicked), obj);

	separator_toolitem = gtk_separator_tool_item_new ();
	gtk_toolbar_insert (GTK_TOOLBAR (priv->top_toolbar), separator_toolitem, -1);
	gtk_widget_show (GTK_WIDGET (separator_toolitem));

	title_vbox = gtk_vbox_new (FALSE, 0);
	priv->title_label = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (priv->title_label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment (GTK_MISC (priv->title_label), 0.0, 1.0);
	priv->subtitle_label = gtk_label_new (NULL);
	gtk_label_set_ellipsize (GTK_LABEL (priv->subtitle_label), PANGO_ELLIPSIZE_START);
	gtk_misc_set_alignment (GTK_MISC (priv->subtitle_label), 0.0, 0.0);
	gtk_widget_show (priv->title_label);
	gtk_widget_show (priv->subtitle_label);
	gtk_box_pack_start (GTK_BOX (title_vbox), priv->title_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (title_vbox), priv->subtitle_label, FALSE, FALSE, 0);
	gtk_widget_show (title_vbox);

	priv->title_button = gtk_tool_button_new (NULL, NULL);
	gtk_widget_show (GTK_WIDGET (priv->title_button));
	title_arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_widget_show (title_arrow);
	gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (priv->title_button), title_arrow);
	gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (priv->title_button), title_vbox);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->top_toolbar), priv->title_button, -1);
	gtk_container_child_set (GTK_CONTAINER (priv->top_toolbar), GTK_WIDGET (priv->title_button), "expand", TRUE, NULL);
	g_object_set (priv->title_button, "is-important", TRUE, NULL);
	g_signal_connect (G_OBJECT (priv->title_button), "clicked", G_CALLBACK (on_title_button_clicked), obj);

	priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs ((GtkNotebook *)priv->notebook, FALSE);
	gtk_notebook_set_show_border ((GtkNotebook *)priv->notebook, FALSE);
	gtk_widget_show (priv->notebook);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), priv->notebook, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (obj), priv->main_vbox);

	g_signal_connect (G_OBJECT (obj), "style-set", G_CALLBACK (on_style_set), obj);

	guint accel_key;
	GdkModifierType accel_mods;
	GtkAccelGroup *accel_group;
	accel_group = gtk_accel_group_new ();
	gtk_accelerator_parse ("<Control>n", &accel_key, &accel_mods);
	gtk_widget_add_accelerator (GTK_WIDGET (priv->new_message_button), "clicked", accel_group,
				    accel_key, accel_mods, 0);
	gtk_accelerator_parse ("Esc", &accel_key, &accel_mods);
	gtk_widget_add_accelerator (GTK_WIDGET (priv->back_button), "clicked", accel_group,
				    accel_key, accel_mods, 0);
	gtk_accelerator_parse ("F10", &accel_key, &accel_mods);
	gtk_widget_add_accelerator (GTK_WIDGET (priv->title_button), "clicked", accel_group,
				    accel_key, accel_mods, 0);
	gtk_window_add_accel_group (GTK_WINDOW (obj), accel_group);

	g_signal_connect (G_OBJECT (obj), 
			  "key-press-event", 
			  G_CALLBACK (on_key_pressed), obj);

	gtk_window_set_default_size (GTK_WINDOW (obj), 
				     640, 480);


}

static void
modest_shell_finalize (GObject *obj)
{
	ModestShellPrivate *priv;
	int n;

	priv = MODEST_SHELL_GET_PRIVATE (obj);

	if (priv->progress_timeout_id) {
		g_source_remove (priv->progress_timeout_id);
	}
	for (n = 0; n < 31; n++) {
		if (priv->progress_frames[n]) {
			g_object_unref (priv->progress_frames[n]);
		}
	}
	g_free (priv->progress_frames);

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

static void
show_next_frame (ModestShell *shell)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);

	gtk_image_set_from_pixbuf (GTK_IMAGE (priv->progress_icon), priv->progress_frames[priv->next_frame]);

	priv->next_frame++;
	if (priv->next_frame >= 31)
		priv->next_frame = 0;
}

static gboolean
on_progress_timeout (ModestShell *shell)
{
	show_next_frame (shell);
	return TRUE;
}

void
modest_shell_show_progress (ModestShell *shell, ModestWindow *window, gboolean show)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);

	if (show) {
		if (priv->progress_timeout_id == 0) {
			priv->progress_timeout_id = g_timeout_add (100, (GSourceFunc) on_progress_timeout, shell);
			show_next_frame (shell);
		}
		gtk_widget_show (priv->progress_icon);
	} else {
		if (priv->progress_timeout_id) {
			g_source_remove (priv->progress_timeout_id);
			priv->progress_timeout_id = 0;
		}
		gtk_widget_hide (priv->progress_icon);
	}
}

static void
update_title (ModestShell *self)
{
	gint n_pages, i;
	ModestShellPrivate *priv;
	GtkWidget *child;
	GString *title_buffer;
	GString *subtitle_buffer;
	const gchar *tab_label_text;

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
	tab_label_text = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (priv->notebook), child);
	if (tab_label_text)
		title_buffer = g_string_append (title_buffer, tab_label_text);
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

static void
menu_position_cb (GtkMenu *menu,
		  gint *x,
		  gint *y,
		  gboolean *push_in,
		  ModestShell *self)
{
	ModestShellPrivate *priv;
	GtkAllocation *alloc;
	GdkWindow *parent_window;
	gint pos_x, pos_y;

	priv = MODEST_SHELL_GET_PRIVATE (self);

	alloc = &(GTK_WIDGET (priv->title_button)->allocation);
	parent_window = gtk_widget_get_parent_window (GTK_WIDGET (priv->title_button));
	gdk_window_get_position (parent_window, &pos_x, &pos_y);
	*x = pos_x + alloc->x;
	*y = pos_y + alloc->y + alloc->height;
	*push_in = TRUE;
	
}

static void
on_title_button_clicked (GtkToolButton *button, ModestShell *self)
{
	ModestShellPrivate *priv;
	gint n_pages;
	GtkWidget *child;
	GtkWidget *menu;

	priv = MODEST_SHELL_GET_PRIVATE (self);

	n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
	if (n_pages < 1)
		return;

	child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), -1);
	menu = modest_shell_window_get_menu (MODEST_SHELL_WINDOW (child));

	if (menu) {
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 
				(GtkMenuPositionFunc) menu_position_cb, (gpointer) self,
				1, gtk_get_current_event_time ());
	}
}

static void
on_new_msg_button_clicked (GtkToolButton *button, ModestShell *self)
{
	ModestShellPrivate *priv;
	gint n_pages;
	GtkWidget *child;

	priv = MODEST_SHELL_GET_PRIVATE (self);

	n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
	if (n_pages < 1)
		return;

	child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), -1);

	modest_ui_actions_on_new_msg (NULL, MODEST_WINDOW (child));
}

static void
on_style_set (GtkWidget *widget,
	      GtkStyle *old_style,
	      ModestShell *self)
{
	ModestShellPrivate *priv;
	gint icon_w, icon_h;
	GdkPixbuf *progress_pixbuf;
	int n;

	priv = MODEST_SHELL_GET_PRIVATE (self);

	if (!gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &icon_w, &icon_h))
		return;
	progress_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), "process-working", icon_w, 0, NULL);

	for (n = 0; n < 31; n++) {
		if (priv->progress_frames[n] != NULL) {
			g_object_unref (priv->progress_frames[n]);
		}
		priv->progress_frames[n] = NULL;
	}

	if (progress_pixbuf) {
		gint max_x, max_y;
		gint i, j;

		icon_w = gdk_pixbuf_get_width (progress_pixbuf) / 8;

		n = 0;
		max_x = 8;
		max_y = 4;
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 8; j++) {
					GdkPixbuf *frame;

					if ((i == 0) && (j == 0))
						continue;
					frame = gdk_pixbuf_new_subpixbuf  (progress_pixbuf,
									   j*icon_w, i*icon_w,
									   icon_w, icon_w);
					priv->progress_frames[n] = frame;
					n++;
				}
			}
		g_object_unref (progress_pixbuf);
	}

}

static gboolean
on_key_pressed (GtkWidget *widget,
		GdkEventKey *event,
		ModestShell *shell)
{
	ModestShellPrivate *priv;
	gboolean retval;
	GtkWidget *current_window;

	priv = MODEST_SHELL_GET_PRIVATE (shell);

	current_window = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), -1);

	g_signal_emit_by_name (current_window, "key-press-event", event, &retval);

	return retval;
	
}

void
modest_shell_add_banner (ModestShell *shell, ModestShellBanner *banner)
{
	ModestShellPrivate *priv;

	priv = MODEST_SHELL_GET_PRIVATE (shell);
	gtk_box_pack_start (GTK_BOX (priv->banners_box), GTK_WIDGET (banner), FALSE, FALSE, 0);
}

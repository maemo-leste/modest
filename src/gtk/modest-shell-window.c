/* Copyright (c) 2008, Nokia Corporation
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

#include <modest-platform.h>
#include "modest-marshal.h"
#include <modest-defs.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>
#include <modest-shell-window.h>
#include <modest-ui-actions.h>
#include "modest-text-utils.h"

/* 'private'/'protected' functions */
static void modest_shell_window_class_init  (gpointer klass, gpointer class_data);
static void modest_shell_window_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_shell_window_dispose     (GObject *obj);

static gboolean on_zoom_minus_plus_not_implemented (ModestWindow *window);
static void modest_shell_window_show_progress (ModestWindow *window,
						 gboolean show);
static void modest_shell_window_show_toolbar (ModestWindow *self,
						 gboolean show_toolbar);
static void modest_shell_window_add_toolbar (ModestWindow *self,
					       GtkToolbar *toolbar);
static void modest_shell_window_add_to_menu (ModestWindow *window,
					       const gchar *label,
					       const gchar *accelerator,
					       ModestWindowMenuCallback callback,
					       ModestDimmingCallback dimming_callback);
static void modest_shell_window_add_item_to_menu (ModestWindow *window,
						    GtkWidget *item,
						    ModestDimmingCallback dimming_callback);
static void modest_shell_window_set_title (ModestWindow *self,
					     const gchar *title);

typedef struct _ModestShellWindowPrivate ModestShellWindowPrivate;
struct _ModestShellWindowPrivate {

	GtkWidget *shell;
	ModestDimmingRulesGroup *app_menu_dimming_group;
	GtkAccelGroup *accel_group;

	GtkWidget *menu;

};
#define MODEST_SHELL_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									    MODEST_TYPE_SHELL_WINDOW, \
									    ModestShellWindowPrivate))

/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

/************************************************************************/

GType
modest_shell_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestShellWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_shell_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestShellWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_shell_window_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
		                                  "ModestShellWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_shell_window_class_init (gpointer klass, gpointer class_data)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->dispose  = modest_shell_window_dispose;

	g_type_class_add_private (gobject_class, sizeof(ModestShellWindowPrivate));
	
	modest_window_class->zoom_minus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->zoom_plus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->show_toolbar_func = modest_shell_window_show_toolbar;
	modest_window_class->add_toolbar_func = modest_shell_window_add_toolbar;
	modest_window_class->add_to_menu_func = modest_shell_window_add_to_menu;
	modest_window_class->add_item_to_menu_func = modest_shell_window_add_item_to_menu;
	modest_window_class->set_title_func = modest_shell_window_set_title;
	modest_window_class->show_progress_func = modest_shell_window_show_progress;

}

static void
modest_shell_window_dispose (GObject *obj)
{
	ModestShellWindowPrivate *priv;

	priv = MODEST_SHELL_WINDOW_GET_PRIVATE(obj);

	if (priv->app_menu_dimming_group) {
		g_object_unref (priv->app_menu_dimming_group);
		priv->app_menu_dimming_group = NULL;
	}

	if (priv->menu) {
		gtk_widget_destroy (priv->menu);
		priv->menu = NULL;
	}

	G_OBJECT_CLASS(parent_class)->dispose (obj);
}

static void
modest_shell_window_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestShellWindow *self = NULL;	
	ModestWindowPrivate *parent_priv = NULL;
	ModestShellWindowPrivate *priv = NULL;

	self = (ModestShellWindow *) instance;
	parent_priv = MODEST_WINDOW_GET_PRIVATE (self);
	priv = MODEST_SHELL_WINDOW_GET_PRIVATE (self);

	priv->accel_group = gtk_accel_group_new ();

	priv->app_menu_dimming_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);
	gtk_window_add_accel_group (GTK_WINDOW (self), priv->accel_group);

	priv->menu = gtk_menu_new ();

	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, 
						      priv->app_menu_dimming_group);

	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */
}

static gboolean
on_zoom_minus_plus_not_implemented (ModestWindow *window)
{
	g_return_val_if_fail (MODEST_IS_SHELL_WINDOW (window), FALSE);

	modest_platform_information_banner (NULL, NULL, _CS("ckct_ib_cannot_zoom_here"));
	return FALSE;
}
void 
modest_shell_window_add_item_to_menu (ModestWindow *self,
					GtkWidget *button,
					ModestDimmingCallback dimming_callback)
{
	ModestShellWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_SHELL_WINDOW(self));
	g_return_if_fail (GTK_IS_BUTTON (button));
	priv = MODEST_SHELL_WINDOW_GET_PRIVATE (self);

	modest_ui_dimming_manager_set_widget_dimming_mode (GTK_WIDGET (button),
							   MODEST_UI_DIMMING_MODE_HIDE);

	if (dimming_callback)
		modest_dimming_rules_group_add_widget_rule (priv->app_menu_dimming_group,
							    GTK_WIDGET (button),
							    (GCallback) dimming_callback,
							    MODEST_WINDOW (self));
	if (priv->menu) {
		gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), button);
	} else {
		gtk_widget_destroy (button);
	}

	gtk_widget_show (GTK_WIDGET (button));
}

static void
modest_shell_window_add_to_menu (ModestWindow *self,
				   const gchar *label,
				   const gchar *accelerator,
				   ModestWindowMenuCallback callback,
				   ModestDimmingCallback dimming_callback)
{
	ModestShellWindowPrivate *priv = NULL;
	GtkWidget *menu_item;

	g_return_if_fail (MODEST_IS_SHELL_WINDOW(self));
	g_return_if_fail (label && label[0] != '\0');
	g_return_if_fail (callback != NULL);

	priv = MODEST_SHELL_WINDOW_GET_PRIVATE (self);

	menu_item = gtk_menu_item_new_with_label (label);
	g_signal_connect_after (G_OBJECT (menu_item), "activate-item",
				G_CALLBACK (callback), (gpointer) self);

	if (accelerator != NULL) {
		guint accel_key;
		GdkModifierType accel_mods;

		gtk_accelerator_parse (accelerator, &accel_key, &accel_mods);
		gtk_widget_add_accelerator (menu_item, "clicked", priv->accel_group,
					    accel_key, accel_mods, 0);
	}

	if (priv->menu) {
		gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), menu_item);
	} else {
		gtk_widget_destroy (menu_item);
	}
}

static void
modest_shell_window_show_toolbar (ModestWindow *self,
				    gboolean show_toolbar)
{
	/* Empty implementation: Hildon 2.2 implementation
	 * doesn't switch toolbar visibility */
}

static void
modest_shell_window_add_toolbar (ModestWindow *self,
				 GtkToolbar *toolbar)
{
	gtk_box_pack_end (GTK_BOX (self), GTK_WIDGET (toolbar), FALSE, FALSE, 0);
}

static void
modest_shell_window_set_title (ModestWindow *self,
			       const gchar *title)
{
	ModestShellWindowPrivate *priv = NULL;

	priv = MODEST_SHELL_WINDOW_GET_PRIVATE (self);
	modest_shell_set_title (MODEST_SHELL (priv->shell),
				MODEST_WINDOW (self),
				title);
}

static void
modest_shell_window_show_progress (ModestWindow *self,
				     gboolean show)
{
	ModestShellWindowPrivate *priv = NULL;

	priv = MODEST_SHELL_WINDOW_GET_PRIVATE (self);
	modest_shell_show_progress (MODEST_SHELL (priv->shell),
				    self,
				    show);
}

void
modest_shell_window_set_shell (ModestShellWindow *self,
			       ModestShell *shell)
{
	ModestShellWindowPrivate *priv = NULL;

	priv = MODEST_SHELL_WINDOW_GET_PRIVATE (self);

	if (priv->shell) {
		modest_shell_delete_window (MODEST_SHELL (shell), MODEST_WINDOW (self));
		g_object_unref (priv->shell);
	}

	priv->shell = g_object_ref (shell);
}

GtkWidget *
modest_shell_window_get_menu (ModestShellWindow *self)
{
	ModestShellWindowPrivate *priv = NULL;

	priv = MODEST_SHELL_WINDOW_GET_PRIVATE (self);

	return priv->menu;
}

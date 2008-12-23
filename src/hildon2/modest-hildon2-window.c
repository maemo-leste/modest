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

#include <hildon/hildon-banner.h>
#include <modest-platform.h>
#include <hildon/hildon-program.h>
#include <modest-maemo-utils.h>
#include <modest-defs.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>
#include <modest-hildon2-window.h>
#include <modest-ui-actions.h>

/* 'private'/'protected' functions */
static void modest_hildon2_window_class_init  (gpointer klass, gpointer class_data);
static void modest_hildon2_window_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_hildon2_window_finalize    (GObject *obj);

static gboolean on_zoom_minus_plus_not_implemented (ModestWindow *window);
static void setup_menu (ModestHildon2Window *self);

static void modest_hildon2_window_show_toolbar (ModestWindow *self,
						 gboolean show_toolbar);
static gboolean modest_hildon2_window_toggle_menu (HildonWindow *window,
						    guint button,
						    guint32 time);
static void modest_hildon2_window_pack_toolbar_not_implemented (ModestHildon2Window *self,
								GtkPackType pack_type,
								GtkWidget *toolbar);

typedef struct _ModestHildon2WindowPrivate ModestHildon2WindowPrivate;
struct _ModestHildon2WindowPrivate {

	GtkWidget *app_menu;
	ModestDimmingRulesGroup *app_menu_dimming_group;

};
#define MODEST_HILDON2_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									    MODEST_TYPE_HILDON2_WINDOW, \
									    ModestHildon2WindowPrivate))

/* globals */
static GtkWindowClass *parent_class = NULL;

/************************************************************************/

GType
modest_hildon2_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestHildon2WindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_hildon2_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestHildon2Window),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_hildon2_window_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
		                                  "ModestHildon2Window",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_hildon2_window_class_init (gpointer klass, gpointer class_data)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;
	HildonWindowClass *hildon_window_class = (HildonWindowClass *) klass;
	ModestHildon2WindowClass *modest_hildon2_window_class = (ModestHildon2WindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_hildon2_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestHildon2WindowPrivate));
	
	hildon_window_class->toggle_menu = modest_hildon2_window_toggle_menu;

	modest_window_class->zoom_minus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->zoom_plus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->show_toolbar_func = modest_hildon2_window_show_toolbar;

	modest_hildon2_window_class->pack_toolbar_func = modest_hildon2_window_pack_toolbar_not_implemented;
}

static void
modest_hildon2_window_finalize (GObject *obj)
{
	ModestHildon2WindowPrivate *priv;

	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE(obj);

	g_object_unref (priv->app_menu_dimming_group);
	priv->app_menu_dimming_group = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_hildon2_window_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestHildon2Window *self = NULL;	
	ModestWindowPrivate *parent_priv = NULL;
	ModestHildon2WindowPrivate *priv = NULL;

	self = (ModestHildon2Window *) instance;
	parent_priv = MODEST_WINDOW_GET_PRIVATE (self);
	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);

	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();
	priv->app_menu_dimming_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);

	setup_menu (self);

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
	g_return_val_if_fail (MODEST_IS_HILDON2_WINDOW (window), FALSE);

	hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_cannot_zoom_here"));
	return FALSE;

}

static void 
modest_hildon2_window_pack_toolbar_not_implemented (ModestHildon2Window *self,
						    GtkPackType pack_type,
						    GtkWidget *toolbar)
{
	g_return_if_fail (MODEST_IS_HILDON2_WINDOW (self));

	g_warning ("%s not implemented", __FUNCTION__);
}

void
modest_hildon2_window_pack_toolbar (ModestHildon2Window *self,
				    GtkPackType pack_type,
				    GtkWidget *toolbar)
{
	g_return_if_fail (MODEST_IS_HILDON2_WINDOW (self));

	MODEST_HILDON2_WINDOW_GET_CLASS (self)->pack_toolbar_func (self, pack_type, toolbar);
}

void 
modest_hildon2_window_add_button_to_menu (ModestHildon2Window *self,
					  GtkButton *button,
					  ModestDimmingCallback dimming_callback)
{
	ModestHildon2WindowPrivate *priv;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW(self));
	g_return_if_fail (GTK_IS_BUTTON (button));
	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);

	if (dimming_callback)
		modest_dimming_rules_group_add_widget_rule (priv->app_menu_dimming_group,
							    GTK_WIDGET (button),
							    (GCallback) dimming_callback,
							    MODEST_WINDOW (self));
	hildon_app_menu_append (HILDON_APP_MENU (priv->app_menu), GTK_BUTTON (button));
}

void 
modest_hildon2_window_add_to_menu (ModestHildon2Window *self,
				   gchar *label,
				   ModestHildon2AppMenuCallback callback,
				   ModestDimmingCallback dimming_callback)
{
	ModestHildon2WindowPrivate *priv = NULL;
	GtkWidget *button;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW(self));
	g_return_if_fail (label && label[0] != '\0');
	g_return_if_fail (callback != NULL);

	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);

	button = gtk_button_new_with_label (label);
	g_signal_connect_after (G_OBJECT (button), "clicked",
				G_CALLBACK (callback), (gpointer) self);

	modest_hildon2_window_add_button_to_menu (self, GTK_BUTTON (button), dimming_callback);
}

static void setup_menu (ModestHildon2Window *self)
{
	ModestHildon2WindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW(self));

	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);

	priv->app_menu = hildon_app_menu_new ();

	/* we expect that the app menu is filled in children using the expected 
	 * add_to_menu methods */

	hildon_stackable_window_set_main_menu (HILDON_STACKABLE_WINDOW (self), 
					       HILDON_APP_MENU (priv->app_menu));
}

static gboolean 
modest_hildon2_window_toggle_menu (HildonWindow *window,
				    guint button,
				    guint32 time)
{
	ModestHildon2WindowPrivate *priv = NULL;

	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (window);

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));

	gtk_widget_queue_resize (priv->app_menu);

	return HILDON_WINDOW_CLASS (parent_class)->toggle_menu (window, button, time);
}

static void
modest_hildon2_window_show_toolbar (ModestWindow *self,
				    gboolean show_toolbar)
{
	/* Empty implementation: Hildon 2.2 implementation
	 * doesn't switch toolbar visibility */
}

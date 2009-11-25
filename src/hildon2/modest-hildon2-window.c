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
#include "modest-marshal.h"
#include <modest-maemo-utils.h>
#include <modest-defs.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>
#include <modest-hildon2-window.h>
#include <modest-ui-actions.h>
#include "modest-text-utils.h"
#include <hildon/hildon-edit-toolbar.h>

typedef struct _EditModeRegister {
	gchar *description;
	gchar *button_label;
	GtkWidget *tree_view;
	GtkSelectionMode mode;
	ModestHildon2EditModeCallback action;
} EditModeRegister;

/* 'private'/'protected' functions */
static void modest_hildon2_window_class_init  (gpointer klass, gpointer class_data);
static void modest_hildon2_window_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_hildon2_window_dispose     (GObject *obj);

static gboolean on_zoom_minus_plus_not_implemented (ModestWindow *window);
static void modest_hildon2_window_show_progress (ModestWindow *window,
						 gboolean show);
static void setup_menu (ModestHildon2Window *self);

static void modest_hildon2_window_show_toolbar (ModestWindow *self,
						 gboolean show_toolbar);
static void modest_hildon2_window_add_toolbar (ModestWindow *self,
					       GtkToolbar *toolbar);
static void modest_hildon2_window_add_to_menu (ModestWindow *window,
					       const gchar *label,
					       const gchar *accelerator,
					       ModestWindowMenuCallback callback,
					       ModestDimmingCallback dimming_callback);
static void modest_hildon2_window_add_item_to_menu (ModestWindow *window,
						    GtkWidget *item,
						    ModestDimmingCallback dimming_callback);
static void modest_hildon2_window_set_title (ModestWindow *self,
					     const gchar *title);
static gboolean modest_hildon2_window_toggle_menu (HildonWindow *window,
						    guint button,
						    guint32 time);
static EditModeRegister *edit_mode_register_new (const gchar *description,
						 const gchar *button_label,
						 GtkTreeView *tree_view,
						 GtkSelectionMode mode,
						 ModestHildon2EditModeCallback action);
static void edit_mode_register_destroy (gpointer data);
static void edit_toolbar_button_clicked (HildonEditToolbar *toolbar,
					 ModestHildon2Window *self);
static void edit_toolbar_arrow_clicked (HildonEditToolbar *toolbar,
					ModestHildon2Window *self);

typedef struct _ModestHildon2WindowPrivate ModestHildon2WindowPrivate;
struct _ModestHildon2WindowPrivate {

	GtkWidget *app_menu;
	ModestDimmingRulesGroup *app_menu_dimming_group;
	GtkAccelGroup *accel_group;

	/* Edit mode support */
	gboolean edit_mode;
	gint edit_command;
	GtkWidget *edit_toolbar;
	GtkWidget *current_edit_tree_view;
	GHashTable *edit_mode_registry;
};
#define MODEST_HILDON2_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									    MODEST_TYPE_HILDON2_WINDOW, \
									    ModestHildon2WindowPrivate))

/* list my signals */
enum {
	EDIT_MODE_CHANGED_SIGNAL,
	LAST_SIGNAL
};

/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

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

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->dispose  = modest_hildon2_window_dispose;

	signals[EDIT_MODE_CHANGED_SIGNAL] =
		g_signal_new ("edit-mode-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHildon2WindowClass, edit_mode_changed),
			      NULL, NULL,
			      modest_marshal_VOID__INT_BOOLEAN,
			      G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_BOOLEAN);

	g_type_class_add_private (gobject_class, sizeof(ModestHildon2WindowPrivate));
	
	hildon_window_class->toggle_menu = modest_hildon2_window_toggle_menu;

	modest_window_class->zoom_minus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->zoom_plus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->show_toolbar_func = modest_hildon2_window_show_toolbar;
	modest_window_class->add_toolbar_func = modest_hildon2_window_add_toolbar;
	modest_window_class->add_to_menu_func = modest_hildon2_window_add_to_menu;
	modest_window_class->add_item_to_menu_func = modest_hildon2_window_add_item_to_menu;
	modest_window_class->set_title_func = modest_hildon2_window_set_title;
	modest_window_class->show_progress_func = modest_hildon2_window_show_progress;

}

static void
modest_hildon2_window_dispose (GObject *obj)
{
	ModestHildon2WindowPrivate *priv;

	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE(obj);

	if (priv->app_menu_dimming_group) {
		g_object_unref (priv->app_menu_dimming_group);
		priv->app_menu_dimming_group = NULL;
	}

	if (priv->edit_mode_registry) {
		g_hash_table_unref (priv->edit_mode_registry);
		priv->edit_mode_registry = NULL;
	}

	G_OBJECT_CLASS(parent_class)->dispose (obj);
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

	priv->accel_group = gtk_accel_group_new ();

	priv->edit_mode = FALSE;
	priv->edit_toolbar = NULL;
	priv->current_edit_tree_view = NULL;
	priv->edit_command = MODEST_HILDON2_WINDOW_EDIT_MODE_NONE;
	priv->edit_mode_registry = g_hash_table_new_full (g_direct_hash, g_direct_equal,
							  NULL, edit_mode_register_destroy);

	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();
	priv->app_menu_dimming_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);
	gtk_window_add_accel_group (GTK_WINDOW (self), priv->accel_group);

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

	hildon_banner_show_information (NULL, NULL, _CS("ckct_ib_cannot_zoom_here"));
	return FALSE;
}
void 
modest_hildon2_window_add_item_to_menu (ModestWindow *self,
					GtkWidget *button,
					ModestDimmingCallback dimming_callback)
{
	ModestHildon2WindowPrivate *priv;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW(self));
	g_return_if_fail (GTK_IS_BUTTON (button));
	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);

	modest_ui_dimming_manager_set_widget_dimming_mode (GTK_WIDGET (button),
							   MODEST_UI_DIMMING_MODE_HIDE);

	if (dimming_callback)
		modest_dimming_rules_group_add_widget_rule (priv->app_menu_dimming_group,
							    GTK_WIDGET (button),
							    (GCallback) dimming_callback,
							    MODEST_WINDOW (self));
	hildon_app_menu_append (HILDON_APP_MENU (priv->app_menu), GTK_BUTTON (button));
	gtk_widget_show (GTK_WIDGET (button));
}

static void
modest_hildon2_window_add_to_menu (ModestWindow *self,
				   const gchar *label,
				   const gchar *accelerator,
				   ModestWindowMenuCallback callback,
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

	if (accelerator != NULL) {
		guint accel_key;
		GdkModifierType accel_mods;

		gtk_accelerator_parse (accelerator, &accel_key, &accel_mods);
		gtk_widget_add_accelerator (button, "clicked", priv->accel_group,
					    accel_key, accel_mods, 0);
	}

	modest_window_add_item_to_menu (MODEST_WINDOW (self), GTK_WIDGET (button), dimming_callback);
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

static void
modest_hildon2_window_add_toolbar (ModestWindow *self,
				   GtkToolbar *toolbar)
{
	hildon_window_add_toolbar (HILDON_WINDOW (self),
				   toolbar);
}

static void
modest_hildon2_window_set_title (ModestWindow *self,
				 const gchar *title)
{
	gtk_window_set_title (GTK_WINDOW (self),
			      title);
}

static void
modest_hildon2_window_show_progress (ModestWindow *self,
				     gboolean show)
{
	hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self),
						  show);
}

void 
modest_hildon2_window_register_edit_mode (ModestHildon2Window *self,
					  gint edit_mode_id,
					  const gchar *description,
					  const gchar *button_label,
					  GtkTreeView *tree_view,
					  GtkSelectionMode mode,
					  ModestHildon2EditModeCallback action)
{
	ModestHildon2WindowPrivate *priv = NULL;
	EditModeRegister *reg;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW (self));
	g_return_if_fail (edit_mode_id >= 0);
	g_return_if_fail (description);
	g_return_if_fail (button_label);
	g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));

	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);

	reg = (EditModeRegister *) g_hash_table_lookup (priv->edit_mode_registry, GINT_TO_POINTER (edit_mode_id));
	g_return_if_fail (reg == NULL);

	reg = edit_mode_register_new (description, button_label, tree_view, mode, action);
	g_hash_table_insert (priv->edit_mode_registry, GINT_TO_POINTER (edit_mode_id), (gpointer) reg);
}

void
modest_hildon2_window_set_edit_mode (ModestHildon2Window *self,
				     gint edit_mode_id)
{
	ModestHildon2WindowPrivate *priv = NULL;
	EditModeRegister *reg;
	GtkTreeSelection *selection;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW (self));
	g_return_if_fail (edit_mode_id >= 0);

	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);
	reg = (EditModeRegister *) g_hash_table_lookup (priv->edit_mode_registry, GINT_TO_POINTER (edit_mode_id));
	g_return_if_fail (reg != NULL);

	if (priv->edit_mode) {
		modest_hildon2_window_unset_edit_mode (self);
	}

	priv->edit_mode = TRUE;
	priv->edit_command = edit_mode_id;

	priv->current_edit_tree_view = reg->tree_view;
	g_object_set (G_OBJECT (priv->current_edit_tree_view),
		      "hildon-ui-mode", HILDON_UI_MODE_EDIT,
		      NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->current_edit_tree_view));
	gtk_tree_selection_set_mode (selection, reg->mode);
	if (reg->mode == GTK_SELECTION_SINGLE || reg->mode == GTK_SELECTION_BROWSE) {
		GtkTreeModel *model;
		GtkTreeIter iter;

		model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->current_edit_tree_view));
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (priv->current_edit_tree_view), 0, 0);
			gtk_tree_selection_select_iter (selection, &iter);
		}
	} else {
		gtk_tree_selection_unselect_all (selection);
	}

	priv->edit_toolbar = hildon_edit_toolbar_new ();
	hildon_edit_toolbar_set_label (HILDON_EDIT_TOOLBAR (priv->edit_toolbar),
				       reg->description);
	hildon_edit_toolbar_set_button_label (HILDON_EDIT_TOOLBAR (priv->edit_toolbar),
					      reg->button_label);
	modest_window_pack_toolbar (MODEST_WINDOW (self), GTK_PACK_START,
				    priv->edit_toolbar);

	g_signal_connect (G_OBJECT (priv->edit_toolbar), "button-clicked",
			  G_CALLBACK (edit_toolbar_button_clicked), (gpointer) self);
	g_signal_connect (G_OBJECT (priv->edit_toolbar), "arrow-clicked",
			  G_CALLBACK (edit_toolbar_arrow_clicked), (gpointer) self);

	gtk_widget_show (priv->edit_toolbar);
	gtk_widget_queue_resize (priv->current_edit_tree_view);
	gtk_window_fullscreen (GTK_WINDOW (self));

	g_signal_emit (G_OBJECT (self), signals[EDIT_MODE_CHANGED_SIGNAL], 0,
		       priv->edit_command, priv->edit_mode);
}

void 
modest_hildon2_window_unset_edit_mode (ModestHildon2Window *self)
{
	ModestHildon2WindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW (self));
	priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);

	if (priv->edit_toolbar) {
		gtk_widget_destroy (priv->edit_toolbar);
		priv->edit_toolbar = NULL;
	}

	if (priv->edit_mode) {
		priv->edit_mode = FALSE;
		if (priv->current_edit_tree_view) {
			g_object_set (G_OBJECT (priv->current_edit_tree_view), 
				      "hildon-ui-mode", HILDON_UI_MODE_NORMAL, 
				      NULL);
			gtk_widget_queue_resize (priv->current_edit_tree_view);
			priv->current_edit_tree_view = NULL;
		}
		gtk_window_unfullscreen (GTK_WINDOW (self));
		g_signal_emit (G_OBJECT (self), signals[EDIT_MODE_CHANGED_SIGNAL], 0,
			       priv->edit_command, priv->edit_mode);
		priv->edit_command = MODEST_HILDON2_WINDOW_EDIT_MODE_NONE;
	}
}

static EditModeRegister *
edit_mode_register_new (const gchar *description,
			const gchar *button_label,
			GtkTreeView *tree_view,
			GtkSelectionMode mode,
			ModestHildon2EditModeCallback action)
{
	EditModeRegister *reg;

	reg = g_slice_new (EditModeRegister);

	reg->description = g_strdup (description);
	reg->button_label = g_strdup (button_label);
	reg->tree_view = g_object_ref (tree_view);
	reg->mode = mode;
	reg->action = action;

	return reg;
}

static void 
edit_mode_register_destroy (gpointer data)
{
	EditModeRegister *reg = (EditModeRegister *) data;

	g_free (reg->description);
	g_free (reg->button_label);
	g_object_unref (reg->tree_view);

	g_slice_free (EditModeRegister, reg);
}

static void
edit_toolbar_button_clicked (HildonEditToolbar *toolbar,
			     ModestHildon2Window *self)
{
	ModestHildon2WindowPrivate *priv = MODEST_HILDON2_WINDOW_GET_PRIVATE (self);
	EditModeRegister *reg;

	g_return_if_fail (MODEST_IS_HILDON2_WINDOW (self));

	reg = (EditModeRegister *) g_hash_table_lookup (priv->edit_mode_registry, 
							GINT_TO_POINTER (priv->edit_command));

	if (reg) {
		if ((reg->action == NULL) || reg->action (MODEST_WINDOW (self)))
			modest_hildon2_window_unset_edit_mode (self);
	} else {
		modest_hildon2_window_unset_edit_mode (self);
	}
}

static void
edit_toolbar_arrow_clicked (HildonEditToolbar *toolbar,
			    ModestHildon2Window *self)
{
	g_return_if_fail (MODEST_IS_HILDON2_WINDOW (self));

	modest_hildon2_window_unset_edit_mode (self);
}


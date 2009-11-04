/* Copyright (c) 2009, Igalia
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

#include <glib/gi18n.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon.h>
#include <modest-number-editor.h>
#endif
#include "modest-toolkit-factory.h"

#ifndef MODEST_TOOLKIT_HILDON2
#define USE_SCROLLED_WINDOW
#define USE_GTK_FIND_TOOLBAR
#define USE_GTK_CHECK_BUTTON
#define USE_GTK_CHECK_MENU
#define USE_GTK_ENTRY
#define USE_GTK_SPIN_BUTTON
#endif

#ifdef USE_SCROLLED_WINDOW
#include <modest-scrolled-window-scrollable.h>
#else
#include <modest-hildon-pannable-area-scrollable.h>
#endif

#ifdef USE_GTK_TOOLBAR
#include <modest-find-toolbar.h>
#else
#include <modest-hildon-find-toolbar.h>
#endif

static void modest_toolkit_factory_class_init (ModestToolkitFactoryClass *klass);
static void modest_toolkit_factory_init (ModestToolkitFactory *self);

/* GObject interface */
static GtkWidget * modest_toolkit_factory_create_scrollable_default (ModestToolkitFactory *self);
static GtkWidget * modest_toolkit_factory_create_check_button_default (ModestToolkitFactory *self, const gchar *label);
static GtkWidget * modest_toolkit_factory_create_check_menu_default (ModestToolkitFactory *self, const gchar *label);
static GtkWidget * modest_toolkit_factory_create_isearch_toolbar_default (ModestToolkitFactory *self, const gchar *label);
static GtkWidget * modest_toolkit_factory_create_entry_default (ModestToolkitFactory *self);
static GtkWidget * modest_toolkit_factory_create_number_entry_default (ModestToolkitFactory *self, gint min, gint max);
/* globals */
static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE    (ModestToolkitFactory,
		  modest_toolkit_factory,
		  G_TYPE_OBJECT);

ModestToolkitFactory *
modest_toolkit_factory_get_instance                            (void)
{
    GObject* self = g_object_new (MODEST_TYPE_TOOLKIT_FACTORY, NULL);

    return (ModestToolkitFactory *) self;
}

static void
modest_toolkit_factory_class_init (ModestToolkitFactoryClass *klass)
{
	parent_class = g_type_class_peek_parent (klass);

	klass->create_scrollable = modest_toolkit_factory_create_scrollable_default;
	klass->create_check_button = modest_toolkit_factory_create_check_button_default;
	klass->create_check_menu = modest_toolkit_factory_create_check_menu_default;
	klass->create_isearch_toolbar = modest_toolkit_factory_create_isearch_toolbar_default;
	klass->create_entry = modest_toolkit_factory_create_entry_default;
	klass->create_number_entry = modest_toolkit_factory_create_number_entry_default;
}

static void
modest_toolkit_factory_init (ModestToolkitFactory *self)
{
}

GtkWidget *
modest_toolkit_factory_create_scrollable (ModestToolkitFactory *self)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_scrollable (self);
}

static GtkWidget *
modest_toolkit_factory_create_scrollable_default (ModestToolkitFactory *self)
{
#ifdef USE_SCROLLED_WINDOW
	return modest_scrolled_window_scrollable_new ();
#else
	return modest_hildon_pannable_area_scrollable_new ();
#endif
}

GtkWidget *
modest_toolkit_factory_create_check_button (ModestToolkitFactory *self, const gchar *label)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_check_button (self, label);
}

static GtkWidget *
modest_toolkit_factory_create_check_button_default (ModestToolkitFactory *self, const gchar *label)
{
	GtkWidget *result;
#ifdef USE_GTK_CHECK_BUTTON
	result = gtk_check_button_new_with_label (label);
#else
	result = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (result), label);
	gtk_button_set_alignment (GTK_BUTTON (result), 0.0, 0.5);
#endif
	return result;
}

GtkWidget *
modest_toolkit_factory_create_check_menu (ModestToolkitFactory *self, const gchar *label)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_check_menu (self, label);
}

static GtkWidget *
modest_toolkit_factory_create_check_menu_default (ModestToolkitFactory *self, const gchar *label)
{
	GtkWidget *result;
#ifdef USE_GTK_CHECK_MENU
	result = gtk_check_menu_item_new_with_label (label);
#else
	result = hildon_check_button_new (0);
	gtk_button_set_label (GTK_BUTTON (result), label);
	gtk_button_set_alignment (GTK_BUTTON (result), 0.5, 0.5);
#endif
	return result;
}

gboolean
modest_togglable_get_active (GtkWidget *widget)
{
	if (GTK_IS_CHECK_MENU_ITEM (widget)) {
		return gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
	} else if (GTK_IS_TOGGLE_BUTTON (widget)) {
		return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (HILDON_IS_CHECK_BUTTON (widget)) {
		return hildon_check_button_get_active (HILDON_CHECK_BUTTON (widget));
#endif
	} else {
		g_return_val_if_reached (FALSE);
	}
}

void
modest_togglable_set_active (GtkWidget *widget, gboolean active)
{
	if (GTK_IS_CHECK_MENU_ITEM (widget)) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), active);
	} else if (GTK_IS_TOGGLE_BUTTON (widget)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), active);
#ifdef MODEST_TOOLKIT_HILDON2
	} else if (HILDON_IS_CHECK_BUTTON (widget)) {
		hildon_check_button_set_active (HILDON_CHECK_BUTTON (widget), active);
#endif
	}
}

gboolean
modest_is_togglable (GtkWidget *widget)
{
	return GTK_IS_CHECK_MENU_ITEM (widget) 
		|| GTK_IS_TOGGLE_BUTTON (widget)
#ifdef MODEST_TOOLKIT_HILDON2
		|| HILDON_IS_CHECK_BUTTON (widget)
#endif
		;
}

GtkWidget *
modest_toolkit_factory_create_isearch_toolbar (ModestToolkitFactory *self, const gchar *label)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_isearch_toolbar (self, label);
}

static GtkWidget *
modest_toolkit_factory_create_isearch_toolbar_default (ModestToolkitFactory *self, const gchar *label)
{
	GtkWidget *result;
#ifdef USE_GTK_FIND_TOOLBAR
	result = modest_find_toolbar_new (label);
#else
	result = modest_hildon_find_toolbar_new (label);
#endif
	return result;
}

GtkWidget *
modest_toolkit_factory_create_entry (ModestToolkitFactory *self)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_entry (self);
}

static GtkWidget *
modest_toolkit_factory_create_entry_default (ModestToolkitFactory *self)
{
#ifdef USE_GTK_ENTRY
	return gtk_entry_new ();
#else
	return hildon_entry_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
#endif
}

void
modest_entry_set_text (GtkWidget *widget, const gchar *text)
{
#ifdef MODEST_TOOLKIT_HILDON2
	hildon_entry_set_text (HILDON_ENTRY (widget), text);
#else
	gtk_entry_set_text (GTK_ENTRY (widget), text);
#endif
}

const gchar *
modest_entry_get_text (GtkWidget *widget)
{
#ifdef MODEST_TOOLKIT_HILDON2
	return hildon_entry_get_text (HILDON_ENTRY (widget));
#else
	return gtk_entry_set_text (GTK_ENTRY (widget));
#endif
}

void 
modest_entry_set_hint (GtkWidget *widget, const gchar *hint)
{
#ifdef MODEST_TOOLKIT_HILDON2
	hildon_entry_set_placeholder (HILDON_ENTRY (widget), hint);
#else
	gtk_widget_set_tooltip_text (widget, hint);
#endif
}

gboolean
modest_is_entry (GtkWidget *widget)
{
#ifdef MODEST_TOOLKIT_HILDON2
	return HILDON_IS_ENTRY (widget);
#else
	return GTK_IS_ENTRY (widget);
#endif
}

GtkWidget *
modest_toolkit_factory_create_number_entry (ModestToolkitFactory *self, gint min, gint max)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_number_entry (self, min, max);
}

static GtkWidget *
modest_toolkit_factory_create_number_entry_default (ModestToolkitFactory *self, gint min, gint max)
{
	GtkWidget *result;
#ifdef USE_GTK_SPIN_BUTTON
	result = gtk_spin_button_new_with_range (min, max, 1.0);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (result), 0);
#else
	result = modest_number_editor_new (min, max);
#endif
	return result;
}

void
modest_number_entry_set_value (GtkWidget *widget, gint value)
{
#ifdef USE_GTK_SPIN_BUTTON
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), value);
#else
	modest_number_editor_set_value (MODEST_NUMBER_EDITOR (widget), value);
#endif
}

gint
modest_number_entry_get_value (GtkWidget *widget)
{
#ifdef USE_GTK_SPIN_BUTTON
	return gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
#else
	return modest_number_editor_get_value (MODEST_NUMBER_EDITOR (widget));
#endif
}

gboolean 
modest_number_entry_is_valid (GtkWidget *widget)
{
#ifdef USE_GTK_SPIN_BUTTON
	return TRUE;
#else
	return modest_number_editor_is_valid (MODEST_NUMBER_EDITOR (widget));
#endif
}

gboolean
modest_is_number_entry (GtkWidget *widget)
{
#ifdef USE_GTK_SPIN_BUTTON
	return GTK_IS_SPIN_BUTTON (widget);
#else
	return MODEST_IS_NUMBER_EDITOR (widget);
#endif
}


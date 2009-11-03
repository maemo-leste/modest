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
#include <modest-hildon-pannable-area-scrollable.h>
#include <modest-hildon-find-toolbar.h>
#include <hildon/hildon.h>
#else
#include <modest-scrolled-window-scrollable.h>
#endif
#include "modest-toolkit-factory.h"

static void modest_toolkit_factory_class_init (ModestToolkitFactoryClass *klass);
static void modest_toolkit_factory_init (ModestToolkitFactory *self);

/* GObject interface */
static GtkWidget * modest_toolkit_factory_create_scrollable_default (ModestToolkitFactory *self);
static GtkWidget * modest_toolkit_factory_create_check_button_default (ModestToolkitFactory *self, const gchar *label);
static GtkWidget * modest_toolkit_factory_create_check_menu_default (ModestToolkitFactory *self, const gchar *label);
static GtkWidget * modest_toolkit_factory_create_isearch_toolbar_default (ModestToolkitFactory *self, const gchar *label);
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
#ifdef MODEST_TOOLKIT_HILDON2
	return modest_hildon_pannable_area_scrollable_new ();
#else
	return modest_scrolled_window_scrollable_new ();
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
#ifdef MODEST_TOOLKIT_HILDON2
	result = hildon_check_button_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label (GTK_BUTTON (result), label);
	gtk_button_set_alignment (GTK_BUTTON (result), 0.0, 0.5);
#else
	result = gtk_check_button_new_with_label (label);
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
#ifdef MODEST_TOOLKIT_HILDON2
	result = hildon_check_button_new (0);
	gtk_button_set_label (GTK_BUTTON (result), label);
	gtk_button_set_alignment (GTK_BUTTON (result), 0.5, 0.5);
#else
	result = gtk_check_menu_item_new_with_label (label);
#endif
	return result;
}

gboolean
modest_togglable_get_active (GtkWidget *widget)
{
#ifdef MODEST_TOOLKIT_HILDON2
	return hildon_check_button_get_active (HILDON_CHECK_BUTTON (widget));
#else
	if (GTK_IS_CHECK_MENU_ITEM (widget)) {
		return gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget));
	} else if (GTK_IS_TOGGLE_BUTTON (widget)) {
		return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	}
#endif
}

void
modest_togglable_set_active (GtkWidget *widget, gboolean active)
{
#ifdef MODEST_TOOLKIT_HILDON2
	hildon_check_button_set_active (HILDON_CHECK_BUTTON (widget), active);
#else
	if (GTK_IS_CHECK_MENU_ITEM (widget)) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), active);
	} else if (GTK_IS_TOGGLE_BUTTON (widget)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), active);
	}
#endif
}

gboolean
modest_is_togglable (GtkWidget *widget)
{
#ifdef MODEST_TOOLKIT_HILDON2
	return HILDON_IS_CHECK_BUTTON (widget);
#else
	return GTK_IS_CHECK_MENU_ITEM (widget) || GTK_IS_TOGGLE_BUTTON (widget);
#endif
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
#ifdef MODEST_TOOLKIT_HILDON2
	result = modest_hildon_find_toolbar_new (label);
#else
	/* TODO: create gtk-only based isearch toolbar implementation */
	result = gtk_toolbar_new ();
#endif
	return result;
}


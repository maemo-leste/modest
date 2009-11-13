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
#include <hildon/hildon-file-chooser-dialog.h>
#include <modest-number-editor.h>
#include <modest-ui-constants.h>
#endif
#include "modest-toolkit-factory.h"

#ifndef MODEST_TOOLKIT_HILDON2
#define USE_SCROLLED_WINDOW
#define USE_GTK_FIND_TOOLBAR
#define USE_GTK_CHECK_BUTTON
#define USE_GTK_CHECK_MENU
#define USE_GTK_ENTRY
#define USE_GTK_FILE_CHOOSER
#define USE_COUNTRY_COMBOBOX
#define USE_SERVERSECURITY_PICKER
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

#ifdef USE_COUNTRY_COMBOBOX
#include <modest-country-combo-box.h>
#else
#include <modest-country-picker.h>
#endif

#ifdef USE_PROVIDER_COMBOBOX
#include <modest-provider-combo-box.h>
#else
#include <modest-provider-picker.h>
#endif

#ifdef USE_SERVERTYPE_COMBOBOX
#include <modest-servertype-combo-box.h>
#else
#include <modest-servertype-picker.h>
#endif

#ifdef USE_SERVERSECURITY_COMBOBOX
#include <modest-serversecurity-combo-box.h>
#else
#include <modest-serversecurity-picker.h>
#endif

static void modest_toolkit_factory_class_init (ModestToolkitFactoryClass *klass);
static void modest_toolkit_factory_init (ModestToolkitFactory *self);

/* GObject interface */
static GtkWidget * modest_toolkit_factory_create_scrollable_default           (ModestToolkitFactory *self);
static GtkWidget * modest_toolkit_factory_create_check_button_default         (ModestToolkitFactory *self,
									       const gchar *label);
static GtkWidget * modest_toolkit_factory_create_check_menu_default           (ModestToolkitFactory *self,
									       const gchar *label);
static GtkWidget * modest_toolkit_factory_create_isearch_toolbar_default      (ModestToolkitFactory *self,
									       const gchar *label);
static GtkWidget * modest_toolkit_factory_create_entry_default                (ModestToolkitFactory *self);
static GtkWidget * modest_toolkit_factory_create_number_entry_default         (ModestToolkitFactory *self,
									       gint min,
									       gint max);
static GtkWidget * modest_toolkit_factory_create_file_chooser_dialog_default  (ModestToolkitFactory *self,
									       const gchar *title,
									       GtkWindow *parent,
									       GtkFileChooserAction action);
static GtkWidget * modest_toolkit_factory_create_country_selector_default     (ModestToolkitFactory *self);
static GtkWidget * modest_toolkit_factory_create_provider_selector_default    (ModestToolkitFactory *self);
static GtkWidget * modest_toolkit_factory_create_servertype_selector_default  (ModestToolkitFactory *self,
									       gboolean filter_providers);
static GtkWidget * modest_toolkit_factory_create_security_selector_default    (ModestToolkitFactory *self);
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
	klass->create_file_chooser_dialog = modest_toolkit_factory_create_file_chooser_dialog_default;
	klass->create_country_selector = modest_toolkit_factory_create_country_selector_default;
	klass->create_provider_selector = modest_toolkit_factory_create_provider_selector_default;
	klass->create_servertype_selector = modest_toolkit_factory_create_servertype_selector_default;
	klass->create_serversecurity_selector = modest_toolkit_factory_create_serversecurity_selector_default;
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

GtkWidget *
modest_toolkit_factory_create_file_chooser_dialog (ModestToolkitFactory *self,
						   const gchar *title,
						   GtkWindow *parent,
						   GtkFileChooserAction action)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_file_chooser_dialog (self, title, parent, action);
}

static GtkWidget *
modest_toolkit_factory_create_file_chooser_dialog_default (ModestToolkitFactory *self,
							   const gchar *title,
							   GtkWindow *parent,
							   GtkFileChooserAction action)
{
	GtkWidget *result;
#ifdef USE_GTK_FILE_CHOOSER
	result = gtk_file_chooser_dialog_new (title, parent, action,
					      (action == GTK_FILE_CHOOSER_ACTION_OPEN) ? GTK_STOCK_OPEN : GTK_STOCK_SAVE,
					      GTK_RESPONSE_OK,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      NULL);
#else
	result = hildon_file_chooser_dialog_new (parent, action);
	gtk_window_set_title ((GtkWindow *) result, title);
#endif
	return result;
}

GtkWidget *
modest_toolkit_factory_create_country_selector (ModestToolkitFactory *self)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_country_selector (self);
}

static GtkWidget *
modest_toolkit_factory_create_country_selector_default (ModestToolkitFactory *self)
{
	GtkWidget *result;
#ifdef USE_COUNTRY_COMBOBOX
	result = modest_country_combo_box_new ();
#else
	result = GTK_WIDGET (modest_country_picker_new (MODEST_EDITABLE_SIZE, 
							HILDON_BUTTON_ARRANGEMENT_HORIZONTAL));
#endif
	return result;
}

gint
modest_country_selector_get_active_country_mcc (GtkWidget *widget)
{
#ifdef USE_COUNTRY_COMBOBOX
	return modest_country_combo_box_get_active_country_mcc (MODEST_COUNTRY_COMBO_BOX (widget));
#else
	return modest_country_picker_get_active_country_mcc (MODEST_COUNTRY_PICKER (widget));
#endif
}

void
modest_country_selector_load_data (GtkWidget *widget)
{
#ifdef USE_COUNTRY_COMBOBOX
	modest_country_combo_box_load_data (MODEST_COUNTRY_COMBO_BOX (widget));
#else
	modest_country_picker_load_data (MODEST_COUNTRY_PICKER (widget));
#endif
}

gboolean
modest_country_selector_set_active_country_locale (GtkWidget *widget)
{
#ifdef USE_COUNTRY_COMBOBOX
	return modest_country_combo_box_set_active_country_locale (MODEST_COUNTRY_COMBO_BOX (widget));
#else
	return modest_country_picker_set_active_country_locale (MODEST_COUNTRY_PICKER (widget));
#endif
}

GtkWidget *
modest_toolkit_factory_create_provider_selector (ModestToolkitFactory *self)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_provider_selector (self);
}

static GtkWidget *
modest_toolkit_factory_create_provider_selector_default (ModestToolkitFactory *self)
{
	GtkWidget *result;
#ifdef USE_PROVIDER_COMBOBOX
	result = modest_provider_combo_box_new ();
#else
	result = GTK_WIDGET (modest_provider_picker_new (MODEST_EDITABLE_SIZE, 
							 HILDON_BUTTON_ARRANGEMENT_HORIZONTAL));
#endif
	return result;
}

void
modest_provider_selector_fill (GtkWidget *widget,
			       ModestPresets *presets,
			       gint mcc)
{
#ifdef USE_PROVIDER_COMBOBOX
	modest_provider_combo_box_fill (MODEST_PROVIDER_COMBO_BOX (widget),
					presets,
					mcc);
#else
	modest_provider_picker_fill (MODEST_PROVIDER_PICKER (widget),
				     presets,
				     mcc);
#endif
}

gchar *
modest_provider_selector_get_active_provider_id (GtkWidget *widget)
{
#ifdef USE_PROVIDER_COMBOBOX
	return modest_provider_combo_box_get_active_provider_id (MODEST_PROVIDER_COMBO_BOX (widget));
#else
	return modest_provider_picker_get_active_provider_id (MODEST_PROVIDER_PICKER (widget));
#endif
}

gchar *
modest_provider_selector_get_active_provider_label (GtkWidget *widget)
{
#ifdef USE_PROVIDER_COMBOBOX
	
	return modest_provider_combo_box_get_active_provider_label (MODEST_PROVIDER_COMBO_BOX (widget));
#else
	GtkWidget *selector;
	
	selector = GTK_WIDGET (hildon_picker_button_get_selector (HILDON_PICKER_BUTTON (widget)));
	return g_strdup (hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector)));
#endif
}

ModestProviderSelectorIdType
modest_provider_selector_get_active_id_type (GtkWidget *widget)
{
	ModestProviderSelectorIdType result;
#ifdef USE_PROVIDER_COMBOBOX
	ModestProviderComboBoxIdType id_type;

	id_type = modest_provider_combo_box_get_active_id_type (MODEST_PROVIDER_COMBO_BOX (widget));
	switch (id_type) {
	case MODEST_PROVIDER_COMBO_BOX_ID_PROVIDER:
		result = MODEST_PROVIDER_SELECTOR_ID_PROVIDER;
		break;
	case MODEST_PROVIDER_COMBO_BOX_ID_OTHER:
		result = MODEST_PROVIDER_SELECTOR_ID_OTHER;
		break;
	case MODEST_PROVIDER_COMBO_BOX_ID_PLUGIN_PROTOCOL:
		result = MODEST_PROVIDER_SELECTOR_ID_PLUGIN_PROTOCOL;
		break;
	default:
		result = MODEST_PROVIDER_SELECTOR_ID_OTHER;
	}
	return result;
#else
	ModestProviderPickerIdType id_type;

	id_type = modest_provider_picker_get_active_id_type (MODEST_PROVIDER_PICKER (widget));
	switch (id_type) {
	case MODEST_PROVIDER_PICKER_ID_PROVIDER:
		result = MODEST_PROVIDER_SELECTOR_ID_PROVIDER;
		break;
	case MODEST_PROVIDER_PICKER_ID_OTHER:
		result = MODEST_PROVIDER_SELECTOR_ID_OTHER;
		break;
	case MODEST_PROVIDER_PICKER_ID_PLUGIN_PROTOCOL:
		result = MODEST_PROVIDER_SELECTOR_ID_PLUGIN_PROTOCOL;
		break;
	default:
		result = MODEST_PROVIDER_SELECTOR_ID_OTHER;
	}
#endif
	return result;
}

void
modest_provider_selector_set_others_provider (GtkWidget *self)
{
#ifdef USE_PROVIDER_COMBOBOX
	modest_provider_combo_box_set_others_provider (MODEST_PROVIDER_COMBO_BOX (self));
#else
	modest_provider_picker_set_others_provider (MODEST_PROVIDER_PICKER (self));
#endif
}

GtkWidget *
modest_toolkit_factory_create_servertype_selector (ModestToolkitFactory *self, gboolean filter_providers)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_servertype_selector (self, filter_providers);
}

static GtkWidget *
modest_toolkit_factory_create_servertype_selector_default (ModestToolkitFactory *self, gboolean filter_providers)
{
	GtkWidget *result;
#ifdef USE_PROVIDER_COMBOBOX
	result = GTK_WIDGET (modest_servertype_combo_box_new (filter_providers));
#else
	result = GTK_WIDGET (modest_servertype_picker_new (MODEST_EDITABLE_SIZE, 
							 HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
							 filter_providers));
#endif
	return result;
}

ModestProtocolType
modest_servertype_selector_get_active_servertype (GtkWidget *self)
{
#ifdef USE_SERVERTYPE_COMBOBOX
	return modest_servertype_combo_box_get_active_servertype (MODEST_SERVERTYPE_COMBO_BOX (self));
#else
	return modest_servertype_picker_get_active_servertype (MODEST_SERVERTYPE_PICKER (self));
#endif
}

void
modest_servertype_selector_set_active_servertype (GtkWidget *self,
						  ModestProtocolType protocol_type_id)
{
#ifdef USE_SERVERTYPE_COMBOBOX
	modest_servertype_combo_box_set_active_servertype (MODEST_SERVERTYPE_COMBO_BOX (self), protocol_type_id);
#else
	modest_servertype_picker_set_active_servertype (MODEST_SERVERTYPE_PICKER (self), protocol_type_id);
#endif
}

GtkWidget *
modest_toolkit_factory_create_serversecurity_selector (ModestToolkitFactory *self)
{
	return MODEST_TOOLKIT_FACTORY_GET_CLASS (self)->create_servertype_selector (self);
}

static GtkWidget *
modest_toolkit_factory_create_serversecurity_selector_default (ModestToolkitFactory *self)
{
	GtkWidget *result;
#ifdef USE_PROVIDER_COMBOBOX
	result = GTK_WIDGET (modest_serversecurity_combo_box_new ());
#else
	result = GTK_WIDGET (modest_serversecurity_picker_new (MODEST_EDITABLE_SIZE, 
							       HILDON_BUTTON_ARRANGEMENT_HORIZONTAL));
#endif
	return result;
}

void
modest_serversecurity_selector_fill (GtkWidget *combobox, 
				     ModestProtocolType protocol)
{
#ifdef USE_SERVERSECURITY_COMBOBOX
	modest_serversecurity_combo_box_fill (MODEST_SERVERSECURITY_COMBO_BOX (combobox),
					      protocol);
#else
	modest_serversecurity_picker_fill (MODEST_SERVERSECURITY_PICKER (combobox),
					   protocol);
#endif
}

ModestProtocolType
modest_serversecurity_selector_get_active_serversecurity (GtkWidget *combobox)
{
#ifdef USE_SERVERSECURITY_COMBOBOX
	return modest_serversecurity_combo_box_get_active_serversecurity (MODEST_SERVERSECURITY_COMBO_BOX (combobox));
#else
	return modest_serversecurity_picker_get_active_serversecurity (MODEST_SERVERSECURITY_PICKER (combobox));
#endif
}

gboolean
modest_serversecurity_selector_set_active_serversecurity (GtkWidget *combobox,
							  ModestProtocolType serversecurity)
{
#ifdef USE_SERVERSECURITY_COMBOBOX
	return modest_serversecurity_combo_box_set_active_serversecurity (MODEST_SERVERSECURITY_COMBO_BOX (combobox),
									  serversecurity);
#else
	return modest_serversecurity_picker_set_active_serversecurity (MODEST_SERVERSECURITY_PICKER (combobox),
								       serversecurity);
#endif
}

gint
modest_serversecurity_selector_get_active_serversecurity_port (GtkWidget *combobox)
{
#ifdef USE_SERVERSECURITY_COMBOBOX
	return modest_serversecurity_combo_box_get_active_serversecurity_port (MODEST_SERVERSECURITY_COMBO_BOX (combobox));
#else
	return modest_serversecurity_picker_get_active_serversecurity_port (MODEST_SERVERSECURITY_PICKER (combobox));
#endif
}

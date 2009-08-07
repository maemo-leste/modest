/* Copyright (c) 2006, Nokia Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "widgets/modest-global-settings-dialog-priv.h"
#include "widgets/modest-combo-box.h"
#include "modest-runtime.h"
#include "modest-defs.h"
#include "gnome/modest-gnome-global-settings-dialog.h"
#include "widgets/modest-ui-constants.h"


/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_gnome_global_settings_dialog_class_init (ModestGnomeGlobalSettingsDialogClass *klass);
static void modest_gnome_global_settings_dialog_init       (ModestGnomeGlobalSettingsDialog *obj);
static void modest_gnome_global_settings_dialog_finalize   (GObject *obj);

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

static void modest_gnome_global_settings_dialog_load_settings (ModestGlobalSettingsDialog *self);
static GtkWidget* create_updating_page  (ModestGnomeGlobalSettingsDialog *self);
static GtkWidget* create_composing_page (ModestGnomeGlobalSettingsDialog *self);
static ModestConnectedVia current_connection (void);

typedef struct _ModestGnomeGlobalSettingsDialogPrivate ModestGnomeGlobalSettingsDialogPrivate;
struct _ModestGnomeGlobalSettingsDialogPrivate {
};
#define MODEST_GNOME_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                           MODEST_TYPE_GNOME_GLOBAL_SETTINGS_DIALOG, \
                                                           ModestGnomeGlobalSettingsDialogPrivate))
/* globals */
static GtkDialogClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_gnome_global_settings_dialog_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestGnomeGlobalSettingsDialogClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_gnome_global_settings_dialog_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestGnomeGlobalSettingsDialog),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_gnome_global_settings_dialog_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_GLOBAL_SETTINGS_DIALOG,
		                                  "ModestGnomeGlobalSettingsDialog",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_gnome_global_settings_dialog_class_init (ModestGnomeGlobalSettingsDialogClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_gnome_global_settings_dialog_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestGnomeGlobalSettingsDialogPrivate));

	MODEST_GLOBAL_SETTINGS_DIALOG_CLASS (klass)->current_connection_func = current_connection;
}

static void
modest_gnome_global_settings_dialog_init (ModestGnomeGlobalSettingsDialog *self)
{
	ModestGlobalSettingsDialogPrivate *ppriv;
/* 	GdkGeometry *geometry; */

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);

	ppriv->updating_page = create_updating_page (self);
	ppriv->composing_page = NULL;
    
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), ppriv->updating_page);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (self)->vbox), 12);
	gtk_container_set_border_width (GTK_CONTAINER (self), 12);
	gtk_window_set_default_size (GTK_WINDOW (self), 480, -1);

	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (self)->action_area), 0);

	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_SAVE, GTK_RESPONSE_OK);
    
	gtk_widget_show_all (ppriv->updating_page);
}

static void
modest_gnome_global_settings_dialog_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_gnome_global_settings_dialog_new (void)
{
	GtkWidget *self = GTK_WIDGET(g_object_new(MODEST_TYPE_GNOME_GLOBAL_SETTINGS_DIALOG, NULL));

	/* Load settings */
	modest_gnome_global_settings_dialog_load_settings (MODEST_GLOBAL_SETTINGS_DIALOG (self));

	return self;
}


/* 
 * Adds the two widgets to a new row in the table
 */
static void
add_to_table (GtkTable *table,
	      GtkWidget *left,
	      GtkWidget *right)
{
	guint n_rows = 0;

	g_object_get (G_OBJECT (table), "n-rows", &n_rows,NULL);

	/* Attach label and value */
	gtk_table_attach (table, 
			  left, 0, 1, 
			  n_rows, n_rows + 1, 
			  GTK_FILL, 
			  GTK_FILL, 
			  0, 0);
	gtk_table_attach (table, 
			  right, 1, 2, 
			  n_rows, n_rows + 1, 
			  GTK_EXPAND | GTK_FILL, 
			  GTK_FILL, 
			  0, 0);
}

/* 
 * We need this because the translations are comming without ":" 
 */
static GtkWidget *
create_label (const gchar *text)
{
	gchar *label_name;
	GtkWidget *label;

	label_name = g_strdup_printf ("%s:", text);
	label = gtk_label_new (label_name);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	g_free (label_name);

	return label;
}

/*
 * Creates the updating page
 */
static GtkWidget*
create_updating_page (ModestGnomeGlobalSettingsDialog *self)
{
	GtkWidget *vbox, *table_update, *table_limit;
	GtkWidget *label;
	ModestGlobalSettingsDialogPrivate *ppriv;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	vbox = gtk_vbox_new (FALSE, 0);
	table_update = gtk_table_new (3, 2, FALSE);
	table_limit = gtk_table_new (2, 2, FALSE);
	/* FIXME: set proper values (HIG) */
	gtk_table_set_row_spacings (GTK_TABLE (table_update), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table_update), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table_limit), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table_limit), 12);

	/* Autoupdate */
	label = create_label (_("mcen_fi_options_autoupdate"));
	ppriv->auto_update = gtk_check_button_new ();
	add_to_table (GTK_TABLE (table_update), label, ppriv->auto_update);

	/* Connected via */
	label = create_label (_("mcen_fi_options_connectiontype"));

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	ppriv->connect_via_list = _modest_global_settings_dialog_get_connected_via ();
	ppriv->connect_via = modest_combo_box_new (ppriv->connect_via_list, g_int_equal);

	add_to_table (GTK_TABLE (table_update), label, ppriv->connect_via);

	/* Update interval */
	label = create_label (_("mcen_fi_options_updateinterval"));

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	ppriv->update_interval_list = _modest_global_settings_dialog_get_update_interval ();
	ppriv->update_interval = modest_combo_box_new (ppriv->update_interval_list, g_int_equal);

	add_to_table (GTK_TABLE (table_update), label, ppriv->update_interval);

	/* Add to vbox */
	gtk_box_pack_start (GTK_BOX (vbox), table_update, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Separator */
	gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Limits */
	label = create_label (_("mcen_fi_advsetup_sizelimit"));
	ppriv->size_limit = gtk_spin_button_new (GTK_ADJUSTMENT (gtk_adjustment_new (1000, 1, 5000, 1, 1, 16)), 
						 1, 0);
	add_to_table (GTK_TABLE (table_limit), label, ppriv->size_limit);

	label = create_label (_("mcen_fi_options_playsound"));
	ppriv->play_sound = gtk_check_button_new ();
	add_to_table (GTK_TABLE (table_limit), label, ppriv->play_sound);

	/* Add to vbox */
	gtk_box_pack_start (GTK_BOX (vbox), table_limit, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

	return vbox;
}

/*
 * Creates the composing page
 */
static GtkWidget* 
create_composing_page (ModestGnomeGlobalSettingsDialog *self)
{
	GtkWidget *vbox, *table;
	GtkWidget *label;

	vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	table = gtk_table_new (2, 2, FALSE);
	/* FIXME: set proper values */
	gtk_table_set_row_spacings (GTK_TABLE (table), 3);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);

	/* Update interval */
	label = create_label (_("mcen_fi_options_messageformat"));

	ModestGlobalSettingsDialogPrivate *ppriv = 
		MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */
	ppriv->msg_format_list = _modest_global_settings_dialog_get_msg_formats ();
	ppriv->msg_format = modest_combo_box_new (ppriv->msg_format_list, g_int_equal);

	add_to_table (GTK_TABLE (table), label, ppriv->msg_format);

	/* Add to vbox */
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

	return vbox;
}

static ModestConnectedVia 
current_connection (void)
{
	return MODEST_CONNECTED_VIA_ANY;
}

static void
modest_gnome_global_settings_dialog_load_settings (ModestGlobalSettingsDialog *self)
{
	ModestConf *conf;
	gboolean checked;
	gint combo_id, value;
	GError *error = NULL;
	ModestGlobalSettingsDialogPrivate *priv;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	conf = modest_runtime_get_conf ();

	/* Autoupdate */
	checked = modest_conf_get_bool (conf, MODEST_CONF_AUTO_UPDATE, &error);
	if (error) {
		g_clear_error (&error);
		error = NULL;
		checked = FALSE;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->auto_update), checked);
	priv->initial_state.auto_update = checked;

	/* Connected by */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_CONNECTED_VIA_WLAN_OR_WIMAX;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->connect_via),
					(gpointer) &combo_id);
	priv->initial_state.connect_via = combo_id;

	/* Update interval */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_INTERVAL, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_UPDATE_INTERVAL_15_MIN;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->update_interval),
					(gpointer) &combo_id);
	priv->initial_state.update_interval = combo_id;

	/* Size limit */
	value  = modest_conf_get_int (conf, MODEST_CONF_MSG_SIZE_LIMIT, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		value = 1000;
	}
	/* It's better to do this in the subclasses, but it's just one
	   line, so we'll leave it here for the moment */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->size_limit), value);
	priv->initial_state.size_limit = value;

	/* Play sound */
	checked = modest_conf_get_bool (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		checked = FALSE;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->play_sound), checked);
	priv->initial_state.play_sound = checked;

	/* Msg format */
	checked = modest_conf_get_bool (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_FILE_FORMAT_FORMATTED_TEXT;
	}
	combo_id = (checked) ? MODEST_FILE_FORMAT_FORMATTED_TEXT : MODEST_FILE_FORMAT_PLAIN_TEXT;
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->msg_format),
					(gpointer) &combo_id);
	priv->initial_state.prefer_formatted_text = checked;
}

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

#include <glib/gi18n.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkstock.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktogglebutton.h>
#include "widgets/modest-global-settings-dialog.h"
#include "widgets/modest-global-settings-dialog-priv.h"
#include "modest-defs.h"
#include "modest-conf.h"
#include "modest-runtime.h"
#include "modest-ui-constants.h"
#include "modest-tny-msg.h"
#include "modest-platform.h"
#include "widgets/modest-combo-box.h"
#ifdef MODEST_PLATFORM_MAEMO
#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <hildon-widgets/hildon-number-editor.h>
#else
#include <hildon/hildon-number-editor.h>
#endif /*MODEST_HAVE_HILDON0_WIDGETS*/
#endif
/* include other impl specific header files */

#define RETURN_FALSE_ON_ERROR(error) if (error) { g_clear_error (&error); return FALSE; }

/* 'private'/'protected' functions */
static void modest_global_settings_dialog_class_init (ModestGlobalSettingsDialogClass *klass);
static void modest_global_settings_dialog_init       (ModestGlobalSettingsDialog *obj);
static void modest_global_settings_dialog_finalize   (GObject *obj);

static void on_response (GtkDialog *dialog,
			 gint arg1,
			 gpointer user_data);
static void get_current_settings (ModestGlobalSettingsDialogPrivate *priv, 
				  ModestGlobalSettingsState *state);

static ModestConnectedVia current_connection_default (void);

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

/* globals */
static GtkDialogClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_global_settings_dialog_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestGlobalSettingsDialogClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_global_settings_dialog_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestGlobalSettingsDialog),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_global_settings_dialog_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_DIALOG,
		                                  "ModestGlobalSettingsDialog",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_global_settings_dialog_class_init (ModestGlobalSettingsDialogClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_global_settings_dialog_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestGlobalSettingsDialogPrivate));

	klass->current_connection_func = current_connection_default;
}

static void
modest_global_settings_dialog_init (ModestGlobalSettingsDialog *self)
{
	ModestGlobalSettingsDialogPrivate *priv;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	priv->notebook = gtk_notebook_new ();
	priv->changed = FALSE;

	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    
	/* Connect to the dialog's response signal: */
	g_signal_connect (G_OBJECT (self), "response", G_CALLBACK (on_response), self);

	/* Set title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_options"));
}

static void
modest_global_settings_dialog_finalize (GObject *obj)
{
	ModestGlobalSettingsDialogPrivate *priv = 
		MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (obj);

	/* These had to stay alive as long as the comboboxes that used them: */
	modest_pair_list_free (priv->connect_via_list);
	modest_pair_list_free (priv->update_interval_list);
	modest_pair_list_free (priv->msg_format_list);
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

/*
 * Creates a pair list (number,string) and adds it to the given list
 */
static void
add_to_modest_pair_list (const gint num, const gchar *str, GSList **list)
{
	gint *number;
	ModestPair *pair;

	number = g_malloc0 (sizeof (gint));
	*number = num;
	pair = modest_pair_new (number, g_strdup (str), FALSE);
	*list = g_slist_prepend (*list, pair);
}

/*
 * Gets a list of pairs 
 */
ModestPairList *
_modest_global_settings_dialog_get_connected_via (void)
{
	GSList *list = NULL;

	add_to_modest_pair_list (MODEST_CONNECTED_VIA_WLAN, 
				 _("mcen_va_options_connectiontype_wlan"), 
				 &list);
	add_to_modest_pair_list (MODEST_CONNECTED_VIA_ANY, 
				 _("mcen_va_options_connectiontype_all"), 
				 &list);

	return (ModestPairList *) g_slist_reverse (list);
}

/*
 * Gets a list of pairs of update intervals
 */
ModestPairList *
_modest_global_settings_dialog_get_update_interval (void)
{
	GSList *list = NULL;

	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_5_MIN, 
				 _("mcen_va_options_updateinterval_5min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_10_MIN, 
				 _("mcen_va_options_updateinterval_10min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_15_MIN, 
				 _("mcen_va_options_updateinterval_15min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_30_MIN, 
				 _("mcen_va_options_updateinterval_30min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_1_HOUR, 
				 _("mcen_va_options_updateinterval_1h"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_2_HOUR, 
				 _("mcen_va_options_updateinterval_2h"), 
				 &list);

	return (ModestPairList *) g_slist_reverse (list);
}

/*
 * Gets a list of pairs 
 */
ModestPairList *
_modest_global_settings_dialog_get_msg_formats (void)
{
	GSList *list = NULL;

	add_to_modest_pair_list (MODEST_FILE_FORMAT_FORMATTED_TEXT, 
				 _("mcen_va_options_messageformat_html"), 
				 &list);
	add_to_modest_pair_list (MODEST_FILE_FORMAT_PLAIN_TEXT, 
				 _("mcen_va_options_messageformat_plain"), 
				 &list);

	return (ModestPairList *) g_slist_reverse (list);
}

void   
_modest_global_settings_dialog_load_conf (ModestGlobalSettingsDialog *self)
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
		combo_id = MODEST_CONNECTED_VIA_WLAN;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->connect_via), 
					(gpointer) &combo_id);
	priv->initial_state.connect_via = combo_id;

	/* Emit toggled to update the visibility of connect_by caption */
	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (priv->auto_update));

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
#ifdef MODEST_PLATFORM_MAEMO
	hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (priv->size_limit), value);
#else
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->size_limit), value);
#endif
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

static void 
get_current_settings (ModestGlobalSettingsDialogPrivate *priv, 
		      ModestGlobalSettingsState *state) 
{
	gint *id;

	/* Get values from UI */
	state->auto_update = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->auto_update));
	id = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->connect_via));
	state->connect_via = *id;
#ifdef MODEST_PLATFORM_MAEMO
	state->size_limit = hildon_number_editor_get_value (HILDON_NUMBER_EDITOR (priv->size_limit));
#else
	state->size_limit = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->size_limit));
#endif
	id = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->update_interval));
	state->update_interval = *id;
	state->play_sound = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->play_sound));
	id = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->msg_format));
	state->prefer_formatted_text = (*id == MODEST_FILE_FORMAT_FORMATTED_TEXT) ? TRUE : FALSE;
}

gboolean
_modest_global_settings_dialog_save_conf (ModestGlobalSettingsDialog *self)
{
	ModestConf *conf;
	ModestGlobalSettingsState current_state;
	GError *error = NULL;
	ModestGlobalSettingsDialogPrivate *priv;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	conf = modest_runtime_get_conf ();

	get_current_settings (priv, &current_state);

	/* Save configuration */
	modest_conf_set_bool (conf, MODEST_CONF_AUTO_UPDATE, current_state.auto_update, &error);
	RETURN_FALSE_ON_ERROR(error);
	modest_conf_set_int (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, current_state.connect_via, NULL);
	RETURN_FALSE_ON_ERROR(error);
	modest_conf_set_int (conf, MODEST_CONF_UPDATE_INTERVAL, current_state.update_interval, NULL);
	RETURN_FALSE_ON_ERROR(error);
	modest_conf_set_int (conf, MODEST_CONF_MSG_SIZE_LIMIT, current_state.size_limit, NULL);
	RETURN_FALSE_ON_ERROR(error);
	modest_conf_set_bool (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, current_state.play_sound, NULL);
	RETURN_FALSE_ON_ERROR(error);
	modest_conf_set_bool (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, current_state.prefer_formatted_text, NULL);
	RETURN_FALSE_ON_ERROR(error);

	/* Apply changes */
	if (priv->initial_state.auto_update != current_state.auto_update || 
	    priv->initial_state.connect_via != current_state.connect_via ||
	    priv->initial_state.update_interval != current_state.update_interval) {

		TnyAccountStore *account_store;
		TnyDevice *device;

		if (!current_state.auto_update) {
			modest_platform_set_update_interval (0);
			/* To avoid a new indentation level */
			goto exit;
		}

		account_store = TNY_ACCOUNT_STORE (modest_runtime_get_account_store ());
		device = tny_account_store_get_device (account_store);

		if (tny_device_is_online (device)) {
			/* If connected via any then set update interval */
			if (current_state.connect_via == MODEST_CONNECTED_VIA_ANY) {
				modest_platform_set_update_interval (current_state.update_interval);
			} else {
				/* Set update interval only if we
				   selected the same connect_via
				   method than the one already used by
				   the device */
				ModestConnectedVia connect_via = 
					MODEST_GLOBAL_SETTINGS_DIALOG_GET_CLASS(self)->current_connection_func ();
				
				if (current_state.connect_via == connect_via)
					modest_platform_set_update_interval (current_state.update_interval);
				else
					modest_platform_set_update_interval (0);
			}
		} else {
			/* Disable autoupdate in offline mode */
			modest_platform_set_update_interval (0);
		}
		g_object_unref (device);		
	}

exit:
	return TRUE;
}

static gboolean
settings_changed (ModestGlobalSettingsState initial_state,
		  ModestGlobalSettingsState current_state)
{
	if (initial_state.auto_update != current_state.auto_update ||
	    initial_state.connect_via != current_state.connect_via ||
	    initial_state.update_interval != current_state.update_interval ||
	    initial_state.size_limit != current_state.size_limit ||
	    initial_state.play_sound != current_state.play_sound ||
	    initial_state.prefer_formatted_text != current_state.prefer_formatted_text)
		return TRUE;
	else
		return FALSE;
}

static void
on_response (GtkDialog *dialog,
	     gint arg1,
	     gpointer user_data)
{
	ModestGlobalSettingsDialogPrivate *priv;
	ModestGlobalSettingsState current_state;
	gboolean changed;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (user_data);

	get_current_settings (priv, &current_state);
	changed = settings_changed (priv->initial_state, current_state);

	if (arg1 == GTK_RESPONSE_OK) {
		if (changed) {
			gboolean saved;

			saved = _modest_global_settings_dialog_save_conf (MODEST_GLOBAL_SETTINGS_DIALOG (dialog));
			if (saved) {
				modest_platform_run_information_dialog (GTK_WINDOW (user_data),
									_("mcen_ib_advsetup_settings_saved"));
			} else {
				modest_platform_run_information_dialog (GTK_WINDOW (user_data),
									_("mail_ib_setting_failed"));
			}
		}
	} else {
		if (changed) {
			gint response;
			response = modest_platform_run_confirmation_dialog (GTK_WINDOW (user_data), 
									    _("imum_nc_wizard_confirm_lose_changes"));
			/* Do not close if the user Cancels */
			if (response == GTK_RESPONSE_CANCEL)
				g_signal_stop_emission_by_name (dialog, "response");
		}
	}
}

static ModestConnectedVia 
current_connection_default (void)
{
	g_warning ("You must implement %s", __FUNCTION__);
	g_return_val_if_reached (MODEST_CONNECTED_VIA_ANY);
}

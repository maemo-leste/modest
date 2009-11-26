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
#include <gtk/gtk.h>
#include <string.h>
#include "widgets/modest-global-settings-dialog.h"
#include "widgets/modest-global-settings-dialog-priv.h"
#include "modest-defs.h"
#include "modest-conf.h"
#include "modest-runtime.h"
#include "modest-ui-constants.h"
#include "modest-tny-msg.h"
#include "modest-platform.h"
#ifdef MODEST_TOOLKIT_HILDON2
#include "modest-hildon-includes.h"
#endif
#ifndef MODEST_TOOLKIT_GTK
#include <hildon/hildon-number-editor.h>
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
static gboolean on_delete_event (GtkWidget *widget,
				 GdkEvent  *event,
				 gpointer   user_data);

static void get_current_settings (ModestGlobalSettingsDialogPrivate *priv,
				  ModestGlobalSettingsState *state);

static ModestConnectedVia current_connection_default (void);

static gboolean modest_global_settings_dialog_save_settings_default (ModestGlobalSettingsDialog *self);

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
	klass->save_settings_func = modest_global_settings_dialog_save_settings_default;
}

static void
modest_global_settings_dialog_init (ModestGlobalSettingsDialog *self)
{
	ModestGlobalSettingsDialogPrivate *priv;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	priv->notebook = gtk_notebook_new ();
	priv->default_account_selector = NULL;
	priv->accounts_list = NULL;

	/* Connect to the dialog's "response" and "delete-event" signals */
	g_signal_connect (G_OBJECT (self), "response", G_CALLBACK (on_response), self);
	g_signal_connect (G_OBJECT (self), "delete-event", G_CALLBACK (on_delete_event), self);

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
	modest_pair_list_free (priv->accounts_list);

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

ModestPairList *
_modest_global_settings_dialog_get_connected_via (void)
{
	GSList *list = NULL;
	const gchar *message;

#ifndef MODEST_TOOLKIT_GTK
	const gchar *env_var = getenv ("OSSO_PRODUCT_HARDWARE");
	/* Check if WIMAX is available */
	if (env_var && !strncmp (env_var, "RX-48", 5))
		message = _("mcen_va_options_connectiontype_wlan_wimax");
	else
		message = _("mcen_va_options_connectiontype_wlan");
#else
	message = _("mcen_va_options_connectiontype_wlan");
#endif
	add_to_modest_pair_list (MODEST_CONNECTED_VIA_WLAN_OR_WIMAX, message, &list);
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

static void
get_current_settings (ModestGlobalSettingsDialogPrivate *priv,
		      ModestGlobalSettingsState *state)
{
	gint *id;

	/* Get values from UI */
	state->notifications = modest_togglable_get_active (HILDON_CHECK_BUTTON (priv->notifications));
	state->add_to_contacts = modest_togglabale_get_active (HILDON_CHECK_BUTTON (priv->add_to_contacts));
	state->auto_update = modest_togglable_get_active (priv->auto_update);
	id = modest_selector_get_active_id (priv->connect_via);
	state->default_account = modest_selector_get_active_id (priv->default_account_selector);
	state->connect_via = *id;
	state->size_limit = modest_number_entry_get_value (priv->size_limit);

	id = modest_selector_get_active_id (priv->update_interval);
	state->update_interval = *id;
	id = modest_selector_get_active_id (priv->msg_format);
	state->play_sound = priv->initial_state.play_sound;
	state->prefer_formatted_text = (*id == MODEST_FILE_FORMAT_FORMATTED_TEXT) ? TRUE : FALSE;
}

static gboolean
modest_global_settings_dialog_save_settings_default (ModestGlobalSettingsDialog *self)
{
	ModestConf *conf;
	ModestGlobalSettingsState current_state = {0,};
	GError *error = NULL;
	ModestGlobalSettingsDialogPrivate *priv;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	conf = modest_runtime_get_conf ();

	get_current_settings (priv, &current_state);

	/* Save configuration */
	modest_conf_set_bool (conf, MODEST_CONF_NOTIFICATIONS, current_state.notifications, &error);
	RETURN_FALSE_ON_ERROR(error);
	modest_conf_set_bool (conf, MODEST_CONF_AUTO_ADD_TO_CONTACTS, current_state.add_to_contacts, &error);
	RETURN_FALSE_ON_ERROR(error);
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
	if (current_state.default_account &&
	    (!priv->initial_state.default_account ||
	     strcmp (current_state.default_account, priv->initial_state.default_account)!= 0)) {
		modest_account_mgr_set_default_account (modest_runtime_get_account_mgr (),
							current_state.default_account);
	}

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
	    initial_state.notifications != current_state.notifications ||
	    initial_state.add_to_contacts != current_state.add_to_contacts ||
	    initial_state.connect_via != current_state.connect_via ||
	    initial_state.update_interval != current_state.update_interval ||
	    initial_state.size_limit != current_state.size_limit ||
	    initial_state.play_sound != current_state.play_sound ||
	    initial_state.prefer_formatted_text != current_state.prefer_formatted_text ||
	    (current_state.default_account &&
	     (!initial_state.default_account || 
	      strcmp (current_state.default_account, initial_state.default_account)!= 0)))
		return TRUE;
	else
		return FALSE;
}

static gboolean
on_delete_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   user_data)
{
	ModestGlobalSettingsDialogPrivate *priv;
	ModestGlobalSettingsState current_state;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (user_data);

	/* If settings changed, them the response method already asked
	   the user, because it's always executed before (see
	   GtkDialog code). If it's not then simply close */
	get_current_settings (priv, &current_state);
	return settings_changed (priv->initial_state, current_state);
}

static void
on_response (GtkDialog *dialog,
	     gint arg1,
	     gpointer user_data)
{
	ModestGlobalSettingsDialogPrivate *priv;
	ModestGlobalSettingsState current_state = {0,};
	gboolean changed = FALSE;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (user_data);

	get_current_settings (priv, &current_state);
	changed = settings_changed (priv->initial_state, current_state);

	if (arg1 == GTK_RESPONSE_OK) {
		if (changed) {
			gboolean saved;

			saved = modest_global_settings_dialog_save_settings (MODEST_GLOBAL_SETTINGS_DIALOG (dialog));
			if (saved) {
				modest_platform_information_banner (NULL, NULL,
								    _("mcen_ib_advsetup_settings_saved"));
			} else {
				modest_platform_information_banner (NULL, NULL,
								    _("mail_ib_setting_failed"));
			}
		}
	} else {
		if (changed) {
			gint response;
			response = modest_platform_run_confirmation_dialog (GTK_WINDOW (user_data),
									    _("imum_nc_wizard_confirm_lose_changes"));
			/* Do not close if the user Cancels */
			if (response != GTK_RESPONSE_OK)
				g_signal_stop_emission_by_name (user_data, "response");
		}
	}
}

static ModestConnectedVia
current_connection_default (void)
{
	g_warning ("You must implement %s", __FUNCTION__);
	g_return_val_if_reached (MODEST_CONNECTED_VIA_ANY);
}

gboolean
modest_global_settings_dialog_save_settings (ModestGlobalSettingsDialog *self)
{
	g_return_val_if_fail (MODEST_IS_GLOBAL_SETTINGS_DIALOG (self), FALSE);

	return MODEST_GLOBAL_SETTINGS_DIALOG_GET_CLASS(self)->save_settings_func (self);
}

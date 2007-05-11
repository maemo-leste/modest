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
#include "widgets/modest-combo-box.h"
#ifdef MODEST_PLATFORM_MAEMO
#ifdef MODEST_HILDON_VERSION_0
#include <hildon-widgets/hildon-number-editor.h>
#else
#include <hildon/hildon-number-editor.h>
#endif /*MODEST_HILDON_VERSION_0*/
#endif
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_global_settings_dialog_class_init (ModestGlobalSettingsDialogClass *klass);
static void modest_global_settings_dialog_init       (ModestGlobalSettingsDialog *obj);
static void modest_global_settings_dialog_finalize   (GObject *obj);

static void on_response (GtkDialog *dialog,
			 gint arg1,
			 gpointer user_data);

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
}

static void
modest_global_settings_dialog_init (ModestGlobalSettingsDialog *self)
{
	ModestGlobalSettingsDialogPrivate *priv;
/* 	GdkGeometry *geometry; */

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	priv->notebook = gtk_notebook_new ();
           
	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    
	/* Connect to the dialog's response signal: */
	g_signal_connect (G_OBJECT (self), "response",
			  G_CALLBACK (on_response), self);

	/* Set title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_options"));

	/* Set geometry */
/* 	geometry = g_malloc0(sizeof (GdkGeometry)); */
/* 	geometry->max_width = MODEST_DIALOG_WINDOW_MAX_WIDTH; */
/* 	geometry->min_width = MODEST_DIALOG_WINDOW_MIN_WIDTH; */
/* 	geometry->max_height = MODEST_DIALOG_WINDOW_MAX_HEIGHT; */
/* 	geometry->min_height = MODEST_DIALOG_WINDOW_MIN_HEIGHT; */
/* 	gtk_window_set_geometry_hints (GTK_WINDOW (self), */
/* 				       GTK_WIDGET (self), */
/* 				       geometry, */
/* 				       GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE); */
	gtk_widget_set_size_request (GTK_WIDGET (self), 
				     MODEST_DIALOG_WINDOW_MAX_WIDTH, 
				     MODEST_DIALOG_WINDOW_MAX_HEIGHT);
}

static void
modest_global_settings_dialog_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
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
_modest_global_settings_dialog_load_conf (ModestGlobalSettingsDialogPrivate *priv)
{
	ModestConf *conf;
	gboolean checked;
	gint combo_id, value;
	GError *error = NULL;

	conf = modest_runtime_get_conf ();

	/* Autoupdate */
	checked = modest_conf_get_bool (conf, MODEST_CONF_AUTO_UPDATE, &error);
	if (error) {
		g_clear_error (&error);
		error = NULL;
		checked = FALSE;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->auto_update), checked);

	/* Connected by */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_CONNECTED_VIA_WLAN;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->connect_via), 
					(gpointer) &combo_id);

	/* Update interval */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_INTERVAL, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_UPDATE_INTERVAL_15_MIN;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->update_interval), 
					(gpointer) &combo_id);

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
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->size_limit, value);
#endif

	/* TODO Fix with the value */

	/* Play sound */
	checked = modest_conf_get_bool (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		checked = FALSE;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->play_sound), checked);

	/* Msg format */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_FILE_FORMAT_FORMATTED_TEXT;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (priv->msg_format), 
					(gpointer) &combo_id);

	/* Include reply */
	value = modest_conf_get_int (conf, MODEST_CONF_REPLY_TYPE, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		value = MODEST_TNY_MSG_REPLY_TYPE_QUOTE;
	}
	if (value == MODEST_TNY_MSG_REPLY_TYPE_QUOTE)
		checked = TRUE;
	else
		checked = FALSE;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->include_reply), checked);
}

void
_modest_global_settings_dialog_save_conf (ModestGlobalSettingsDialogPrivate *priv)
{
	ModestConf *conf;
	gboolean checked;
	gint *combo_id, value;

	conf = modest_runtime_get_conf ();

	/* Autoupdate */
	checked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->auto_update));
	modest_conf_set_bool (conf, MODEST_CONF_AUTO_UPDATE, checked, NULL);

	/* Connected by */
	combo_id = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->connect_via));
	modest_conf_set_int (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, *combo_id, NULL);

	/* Update interval */
	combo_id = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->update_interval));
	modest_conf_set_int (conf, MODEST_CONF_UPDATE_INTERVAL, *combo_id, NULL);

	/* Size limit */	
	/* It's better to do this in the subclasses, but it's just one
	   line, so we'll leave it here for the moment */
#ifdef MODEST_PLATFORM_MAEMO
	value = hildon_number_editor_get_value (HILDON_NUMBER_EDITOR (priv->size_limit));
#else
	value = gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->size_limit);
#endif
	modest_conf_set_int (conf, MODEST_CONF_MSG_SIZE_LIMIT, value, NULL);

	/* Play sound */
	checked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->play_sound));
	modest_conf_set_bool (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, checked, NULL);

	/* Msg format */
	combo_id = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->msg_format));
	modest_conf_set_int (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, *combo_id, NULL);

	/* Include reply */
	checked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->include_reply));
	modest_conf_set_int (conf, MODEST_CONF_REPLY_TYPE, 
			     (checked) ? MODEST_TNY_MSG_REPLY_TYPE_QUOTE : 
			     MODEST_TNY_MSG_REPLY_TYPE_CITE, NULL);
}

static void
on_response (GtkDialog *dialog,
	     gint arg1,
	     gpointer user_data)
{
	ModestGlobalSettingsDialogPrivate *priv;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (user_data);

	if (arg1 == GTK_RESPONSE_OK)
		_modest_global_settings_dialog_save_conf (priv);
}

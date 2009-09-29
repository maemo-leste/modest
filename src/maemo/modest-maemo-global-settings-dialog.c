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

#include <modest-hildon-includes.h>
#include <modest-maemo-utils.h>

#include <glib/gi18n.h>
#include <string.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkhseparator.h>
#include "modest-runtime.h"
#include "modest-defs.h"
#include "widgets/modest-global-settings-dialog-priv.h"
#include "widgets/modest-combo-box.h"
#include "maemo/modest-maemo-global-settings-dialog.h"
#include "widgets/modest-ui-constants.h"
#include <tny-account-store.h>


#define MSG_SIZE_MAX_VAL 5000
#define MSG_SIZE_DEF_VAL 1000
#define MSG_SIZE_MIN_VAL 1

#define DEFAULT_FOCUS_WIDGET "default-focus-widget"

/* 'private'/'protected' functions */
static void modest_maemo_global_settings_dialog_class_init (ModestMaemoGlobalSettingsDialogClass *klass);
static void modest_maemo_global_settings_dialog_init       (ModestMaemoGlobalSettingsDialog *obj);
static void modest_maemo_global_settings_dialog_finalize   (GObject *obj);

static ModestConnectedVia current_connection (void);

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

static GtkWidget* create_updating_page   (ModestMaemoGlobalSettingsDialog *self);
static GtkWidget* create_composing_page  (ModestMaemoGlobalSettingsDialog *self);

static gboolean   on_range_error         (HildonNumberEditor *editor, 
					  HildonNumberEditorErrorType type,
					  gpointer user_data);

static void       on_size_notify         (HildonNumberEditor *editor, 
					  GParamSpec *arg1,
					  gpointer user_data);

static void       on_auto_update_toggled (GtkToggleButton *togglebutton,
					  gpointer user_data);

static gboolean   on_inner_tabs_key_pressed (GtkWidget *widget,
					     GdkEventKey *event,
					     gpointer user_data);

static void modest_maemo_global_settings_dialog_load_settings (ModestGlobalSettingsDialog *self);

typedef struct _ModestMaemoGlobalSettingsDialogPrivate ModestMaemoGlobalSettingsDialogPrivate;
struct _ModestMaemoGlobalSettingsDialogPrivate {
	ModestPairList *connect_via_list;
	gint switch_handler;
};
#define MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                           MODEST_TYPE_MAEMO_GLOBAL_SETTINGS_DIALOG, \
                                                           ModestMaemoGlobalSettingsDialogPrivate))
/* globals */
static GtkDialogClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_maemo_global_settings_dialog_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMaemoGlobalSettingsDialogClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_maemo_global_settings_dialog_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMaemoGlobalSettingsDialog),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_maemo_global_settings_dialog_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_GLOBAL_SETTINGS_DIALOG,
		                                  "ModestMaemoGlobalSettingsDialog",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_maemo_global_settings_dialog_class_init (ModestMaemoGlobalSettingsDialogClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_maemo_global_settings_dialog_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMaemoGlobalSettingsDialogPrivate));

	MODEST_GLOBAL_SETTINGS_DIALOG_CLASS (klass)->current_connection_func = current_connection;
}

typedef struct {
	ModestMaemoGlobalSettingsDialog *dia;
	GtkWidget *focus_widget;
} SwitchPageHelper;

static gboolean
idle_select_default_focus (gpointer data) 
{
	ModestGlobalSettingsDialogPrivate *ppriv;
	ModestMaemoGlobalSettingsDialogPrivate *priv;
	SwitchPageHelper *helper;

	helper = (SwitchPageHelper *) data;
	priv  = MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (helper->dia);
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (helper->dia);

	/* Grab focus, we need to block in order to prevent a
	   recursive call to this callback */
	g_signal_handler_block (G_OBJECT (ppriv->notebook), priv->switch_handler);

	/* This is a GDK lock because we are an idle callback and
	 * the code below is or does Gtk+ code */

	gdk_threads_enter (); /* CHECKED */
	gtk_widget_grab_focus (helper->focus_widget);
	gdk_threads_leave (); /* CHECKED */

	g_signal_handler_unblock (G_OBJECT (ppriv->notebook), priv->switch_handler);
	g_free (helper);

	return FALSE;
}


static void
on_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
	/* grab the focus to the default element in the current page */
	GtkWidget *selected_page = NULL, *focus_item = NULL;
	SwitchPageHelper *helper;

	selected_page = gtk_notebook_get_nth_page (notebook, page_num);
	focus_item = GTK_WIDGET(g_object_get_data (G_OBJECT(selected_page), DEFAULT_FOCUS_WIDGET));
	if (!focus_item) {
		g_printerr ("modest: cannot get focus item\n");
		return;
	}

	/* Create the helper */
	helper = g_malloc0 (sizeof (SwitchPageHelper));
	helper->dia = MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG (user_data);
	helper->focus_widget = focus_item;

	/* Focus the widget in an idle. We need to do this in an idle,
	   because this handler is executed *before* the page was
	   really switched, so the focus is not placed in the right
	   widget */
	g_idle_add (idle_select_default_focus, helper);
}


static void
modest_maemo_global_settings_dialog_init (ModestMaemoGlobalSettingsDialog *self)
{
	ModestMaemoGlobalSettingsDialogPrivate *priv;
	ModestGlobalSettingsDialogPrivate *ppriv;

	priv  = MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	ppriv->updating_page = create_updating_page (self);
	ppriv->composing_page = create_composing_page (self);

	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), _("mcen_bd_dialog_ok"), GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (self), _("mcen_bd_dialog_cancel"), GTK_RESPONSE_CANCEL);

	/* Set the default focusable widgets */
	g_object_set_data (G_OBJECT(ppriv->updating_page), DEFAULT_FOCUS_WIDGET,
			   (gpointer)ppriv->auto_update);
	g_object_set_data (G_OBJECT(ppriv->composing_page), DEFAULT_FOCUS_WIDGET,
			   (gpointer)ppriv->msg_format);

	/* Add the notebook pages: */
	gtk_notebook_append_page (GTK_NOTEBOOK (ppriv->notebook), ppriv->updating_page, 
				  gtk_label_new (_("mcen_ti_options_updating")));
	gtk_notebook_append_page (GTK_NOTEBOOK (ppriv->notebook), ppriv->composing_page, 
				  gtk_label_new (_("mcen_ti_options_composing")));

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), ppriv->notebook);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (self)->vbox), MODEST_MARGIN_HALF);

	gtk_window_set_default_size (GTK_WINDOW (self), 700, 300);

	g_signal_connect (G_OBJECT (self), "key-press-event",
			  G_CALLBACK (on_inner_tabs_key_pressed), self);
	priv->switch_handler = g_signal_connect (G_OBJECT(ppriv->notebook), "switch-page",
						 G_CALLBACK(on_switch_page), self);

	/* Set first page */
	gtk_notebook_set_current_page (GTK_NOTEBOOK (ppriv->notebook), 0);

	hildon_help_dialog_help_enable (GTK_DIALOG(self), "applications_email_options_dialog",
					modest_maemo_utils_get_osso_context());
}

static void
modest_maemo_global_settings_dialog_finalize (GObject *obj)
{
	ModestGlobalSettingsDialogPrivate *ppriv;
	ModestMaemoGlobalSettingsDialogPrivate *priv;

	priv = MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (obj);
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (obj);

	if (priv->switch_handler && ppriv->notebook) {
		/* TODO: This causes a g_warning and a valgrind mem error: */
		/* g_signal_handler_disconnect (G_OBJECT (ppriv->notebook), priv->switch_handler);*/
		priv->switch_handler = 0;
	}

/* 	free/unref instance resources here */
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_maemo_global_settings_dialog_new (void)
{
	GtkWidget *self = GTK_WIDGET(g_object_new(MODEST_TYPE_MAEMO_GLOBAL_SETTINGS_DIALOG, NULL));

	/* Load settings */
	modest_maemo_global_settings_dialog_load_settings (MODEST_GLOBAL_SETTINGS_DIALOG (self));

	return self;
}

/*
 * Creates the updating page
 */
static GtkWidget*
create_updating_page (ModestMaemoGlobalSettingsDialog *self)
{
	GtkWidget *vbox, *vbox_update, *vbox_limit, *caption;
	GtkSizeGroup *size_group;
	ModestGlobalSettingsDialogPrivate *ppriv;
	GtkWidget *scrollwin = NULL;
	GtkAdjustment *focus_adjustment = NULL;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	scrollwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), 
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);

	vbox_update = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Auto update */
	ppriv->auto_update = gtk_check_button_new ();
	caption = hildon_caption_new (size_group, 
				      _("mcen_fi_options_autoupdate"), 
				      ppriv->auto_update, 
				      NULL, 
				      HILDON_CAPTION_MANDATORY);
	gtk_box_pack_start (GTK_BOX (vbox_update), caption, FALSE, FALSE, MODEST_MARGIN_HALF);
	g_signal_connect (ppriv->auto_update, "toggled", G_CALLBACK (on_auto_update_toggled), self);

	/* Connected via */

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->connect_via_list = _modest_global_settings_dialog_get_connected_via ();
	ppriv->connect_via = modest_combo_box_new (ppriv->connect_via_list, g_int_equal);

	caption = hildon_caption_new (size_group, 
				      _("mcen_fi_options_connectiontype"),
				      ppriv->connect_via, 
				      NULL, 
				      HILDON_CAPTION_MANDATORY);
	gtk_box_pack_start (GTK_BOX (vbox_update), caption, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Update interval */

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->update_interval_list = _modest_global_settings_dialog_get_update_interval ();
	ppriv->update_interval = modest_combo_box_new (ppriv->update_interval_list, g_int_equal);

	caption = hildon_caption_new (size_group, 
				      _("mcen_fi_options_updateinterval"),
				      ppriv->update_interval, 
				      NULL, 
				      HILDON_CAPTION_MANDATORY);
	gtk_box_pack_start (GTK_BOX (vbox_update), caption, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Add to vbox */
	gtk_box_pack_start (GTK_BOX (vbox), vbox_update, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Separator */
	gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Limits */
	vbox_limit = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Size limit */
	ppriv->size_limit = hildon_number_editor_new (MSG_SIZE_MIN_VAL, MSG_SIZE_MAX_VAL);
	hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (ppriv->size_limit), MSG_SIZE_DEF_VAL);
	g_signal_connect (ppriv->size_limit, "range_error", G_CALLBACK (on_range_error), self);
	g_signal_connect (ppriv->size_limit, "notify", G_CALLBACK (on_size_notify), self);
	caption = hildon_caption_new (size_group, 
				      _("mcen_fi_advsetup_sizelimit"), 
				      ppriv->size_limit,
				      NULL, 
				      HILDON_CAPTION_MANDATORY);
	gtk_box_pack_start (GTK_BOX (vbox_limit), caption, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Play sound */
	ppriv->play_sound = gtk_check_button_new ();
	caption = hildon_caption_new (size_group, 
				      _("mcen_fi_options_playsound"), 
				      ppriv->play_sound, 
				      NULL, 
				      HILDON_CAPTION_MANDATORY);
	gtk_box_pack_start (GTK_BOX (vbox_limit), caption, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Add to vbox */
	gtk_box_pack_start (GTK_BOX (vbox), vbox_limit, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwin), vbox);
	focus_adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrollwin));
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (vbox), focus_adjustment);
	gtk_widget_show (scrollwin);
	
	return scrollwin;
}

/*
 * Creates the composing page
 */
static GtkWidget* 
create_composing_page (ModestMaemoGlobalSettingsDialog *self)
{
	GtkWidget *vbox;
	GtkSizeGroup *size_group;
	ModestGlobalSettingsDialogPrivate *ppriv;
	GtkWidget *caption;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);

	/* Update interval */

	/* Note: This ModestPairList* must exist for as long as the combo
	 * that uses it, because the ModestComboBox uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->msg_format_list = _modest_global_settings_dialog_get_msg_formats ();
	ppriv->msg_format = modest_combo_box_new (ppriv->msg_format_list, g_int_equal);

	caption = hildon_caption_new (size_group, 
				      _("mcen_fi_options_messageformat"),
				      ppriv->msg_format, 
				      NULL, 
				      HILDON_CAPTION_MANDATORY);
	gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, MODEST_MARGIN_HALF);

	return vbox;
}

static void
on_auto_update_toggled (GtkToggleButton *togglebutton,
			gpointer user_data)
{
	ModestGlobalSettingsDialogPrivate *ppriv;
	GtkWidget *caption1, *caption2;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (user_data);
	caption1 = gtk_widget_get_ancestor (ppriv->connect_via, HILDON_TYPE_CAPTION);
	caption2 = gtk_widget_get_ancestor (ppriv->update_interval, HILDON_TYPE_CAPTION);

	if (gtk_toggle_button_get_active (togglebutton)) {
		gtk_widget_set_sensitive (caption1, TRUE);
		gtk_widget_set_sensitive (caption2, TRUE);
	} else {
		gtk_widget_set_sensitive (caption1, FALSE);
		gtk_widget_set_sensitive (caption2, FALSE);
	}
}

static gboolean
on_range_error (HildonNumberEditor *editor, 
		HildonNumberEditorErrorType type,
		gpointer user_data)
{
	gchar *msg;
	gint new_val;

	switch (type) {
	case HILDON_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED:
		msg = g_strdup_printf (dgettext("hildon-libs", "ckct_ib_maximum_value"), MSG_SIZE_MAX_VAL);
		new_val = MSG_SIZE_MAX_VAL;
		break;
	case HILDON_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED:
		msg = g_strdup_printf (dgettext("hildon-libs", "ckct_ib_minimum_value"), MSG_SIZE_MIN_VAL);
		new_val = MSG_SIZE_MIN_VAL;
		break;
	case HILDON_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE:
		msg = g_strdup_printf (dgettext("hildon-libs", "ckct_ib_set_a_value_within_range"), 
				       MSG_SIZE_MIN_VAL, 
				       MSG_SIZE_MAX_VAL);
		/* FIXME: use the previous */
		new_val = MSG_SIZE_DEF_VAL;
		break;
	default:
		g_return_val_if_reached (FALSE);
	}

	/* Restore value */
	hildon_number_editor_set_value (editor, new_val);

	/* Show error */
	hildon_banner_show_information (GTK_WIDGET (user_data), NULL, msg);

	/* Free */
	g_free (msg);

	return TRUE;
}

static void       
on_size_notify         (HildonNumberEditor *editor, 
			GParamSpec *arg1,
			gpointer user_data)
{
	ModestMaemoGlobalSettingsDialog *dialog = MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG (user_data);
	gint value = hildon_number_editor_get_value (editor);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, value > 0);
}

static ModestConnectedVia
current_connection (void)
{
	return modest_platform_get_current_connection ();
}

static gboolean
on_inner_tabs_key_pressed (GtkWidget *widget,
			   GdkEventKey *event,
			   gpointer user_data)
{
	ModestGlobalSettingsDialogPrivate *ppriv;
	gboolean retval = FALSE;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (user_data);

	if (widget == ppriv->notebook) {
		if (event->keyval == GDK_Right) {
			gtk_notebook_next_page (GTK_NOTEBOOK (ppriv->notebook));
			retval = TRUE;
		} else if (event->keyval == GDK_Left) {
			gtk_notebook_prev_page (GTK_NOTEBOOK (ppriv->notebook));
			retval = TRUE;
		}
	}

	return retval;
}

static void 
modest_maemo_global_settings_dialog_load_settings (ModestGlobalSettingsDialog *self)
{
	ModestConf *conf;
	gboolean checked;
	gint combo_id, value;
	GError *error = NULL;
	ModestGlobalSettingsDialogPrivate *ppriv;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	conf = modest_runtime_get_conf ();

	/* Autoupdate */
	checked = modest_conf_get_bool (conf, MODEST_CONF_AUTO_UPDATE, &error);
	if (error) {
		g_clear_error (&error);
		error = NULL;
		checked = FALSE;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ppriv->auto_update), checked);
	ppriv->initial_state.auto_update = checked;

	/* Connected by */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_CONNECTED_VIA_WLAN_OR_WIMAX;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (ppriv->connect_via), 
					(gpointer) &combo_id);
	ppriv->initial_state.connect_via = combo_id;

	/* Emit toggled to update the visibility of connect_by caption */
	gtk_toggle_button_toggled (GTK_TOGGLE_BUTTON (ppriv->auto_update));

	/* Update interval */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_INTERVAL, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_UPDATE_INTERVAL_15_MIN;
	}
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (ppriv->update_interval), 
					(gpointer) &combo_id);
	ppriv->initial_state.update_interval = combo_id;

	/* Size limit */
	value  = modest_conf_get_int (conf, MODEST_CONF_MSG_SIZE_LIMIT, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		value = 1000;
	}
	/* It's better to do this in the subclasses, but it's just one
	   line, so we'll leave it here for the moment */
	hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (ppriv->size_limit), value);
	ppriv->initial_state.size_limit = value;

	/* Play sound */
	checked = modest_conf_get_bool (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		checked = FALSE;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ppriv->play_sound), checked);
	ppriv->initial_state.play_sound = checked;

	/* Msg format */
	checked = modest_conf_get_bool (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_FILE_FORMAT_FORMATTED_TEXT;
	}
	combo_id = (checked) ? MODEST_FILE_FORMAT_FORMATTED_TEXT : MODEST_FILE_FORMAT_PLAIN_TEXT;
	modest_combo_box_set_active_id (MODEST_COMBO_BOX (ppriv->msg_format),
					(gpointer) &combo_id);
	ppriv->initial_state.prefer_formatted_text = checked;
}

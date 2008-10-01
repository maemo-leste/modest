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
#include "widgets/modest-global-settings-dialog-priv.h"
#include "modest-selector-picker.h"
#include "hildon/hildon-check-button.h"
#include "hildon/hildon-pannable-area.h"
#include "modest-maemo-global-settings-dialog.h"
#include "widgets/modest-ui-constants.h"
#include "modest-text-utils.h"
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

static GtkWidget* create_updating_page   (ModestMaemoGlobalSettingsDialog *self);

static gboolean   on_range_error         (HildonNumberEditor *editor, 
					  HildonNumberEditorErrorType type,
					  gpointer user_data);

static void       on_size_notify         (HildonNumberEditor *editor, 
					  GParamSpec *arg1,
					  gpointer user_data);

static void       on_auto_update_clicked (GtkButton *button,
					  gpointer user_data);

typedef struct _ModestMaemoGlobalSettingsDialogPrivate ModestMaemoGlobalSettingsDialogPrivate;
struct _ModestMaemoGlobalSettingsDialogPrivate {
	ModestPairList *connect_via_list;
};
#define MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                           MODEST_TYPE_MAEMO_GLOBAL_SETTINGS_DIALOG, \
                                                           ModestMaemoGlobalSettingsDialogPrivate))
/* globals */
static GtkDialogClass *parent_class = NULL;

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


static void
modest_maemo_global_settings_dialog_init (ModestMaemoGlobalSettingsDialog *self)
{
	ModestMaemoGlobalSettingsDialogPrivate *priv;
	ModestGlobalSettingsDialogPrivate *ppriv;

	priv  = MODEST_MAEMO_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	ppriv->updating_page = create_updating_page (self);

	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), _HL("wdgt_bd_save "), GTK_RESPONSE_OK);

	/* Set the default focusable widgets */
	g_object_set_data (G_OBJECT(ppriv->updating_page), DEFAULT_FOCUS_WIDGET,
			   (gpointer)ppriv->auto_update);

	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), ppriv->updating_page);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (self)->vbox), MODEST_MARGIN_HALF);

	/* gtk_window_set_default_size (GTK_WINDOW (self), 700, 300); */

	/* Load current config */
	_modest_global_settings_dialog_load_conf (MODEST_GLOBAL_SETTINGS_DIALOG (self));

	/* Set first page */
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

/* 	free/unref instance resources here */
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_maemo_global_settings_dialog_new (void)
{
	return GTK_WIDGET(g_object_new(MODEST_TYPE_MAEMO_GLOBAL_SETTINGS_DIALOG, NULL));
}

/*
 * Creates the updating page
 */
static GtkWidget*
create_updating_page (ModestMaemoGlobalSettingsDialog *self)
{
	GtkWidget *vbox, *vbox_update, *vbox_limit, *label, *hbox;
	GtkSizeGroup *size_group;
	ModestGlobalSettingsDialogPrivate *ppriv;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);

	vbox_update = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Auto update */
	ppriv->auto_update = hildon_check_button_new (HILDON_SIZE_AUTO);
	gtk_button_set_label (GTK_BUTTON (ppriv->auto_update), _("mcen_fi_options_autoupdate"));
	gtk_box_pack_start (GTK_BOX (vbox_update), ppriv->auto_update, FALSE, FALSE, MODEST_MARGIN_HALF);
	g_signal_connect (ppriv->auto_update, "clicked", G_CALLBACK (on_auto_update_clicked), self);

	/* Connected via */

	/* Note: This ModestPairList* must exist for as long as the picker
	 * that uses it, because the ModestSelectorPicker uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->connect_via_list = _modest_global_settings_dialog_get_connected_via ();
	ppriv->connect_via = modest_selector_picker_new (ppriv->connect_via_list, g_int_equal);
	hildon_button_set_title (HILDON_BUTTON (ppriv->connect_via), _("mcen_fi_options_connectiontype"));
	gtk_box_pack_start (GTK_BOX (vbox_update), ppriv->connect_via, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Update interval */

	/* Note: This ModestPairList* must exist for as long as the picker
	 * that uses it, because the ModestSelectorPicker uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->update_interval_list = _modest_global_settings_dialog_get_update_interval ();
	ppriv->update_interval = modest_selector_picker_new (ppriv->update_interval_list, g_int_equal);
	hildon_button_set_title (HILDON_BUTTON (ppriv->update_interval), _("mcen_fi_options_updateinterval"));
	gtk_box_pack_start (GTK_BOX (vbox_update), ppriv->update_interval, FALSE, FALSE, MODEST_MARGIN_HALF);

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
	label = gtk_label_new (_("mcen_fi_advsetup_sizelimit"));
	hbox = gtk_hbox_new (FALSE, MODEST_MARGIN_HALF);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), ppriv->size_limit, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox_limit), hbox, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_box_pack_start (GTK_BOX (vbox), vbox_limit, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show_all (vbox_limit);

	/* Separator */
	gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Note: This ModestPairList* must exist for as long as the picker
	 * that uses it, because the ModestSelectorPicker uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->msg_format_list = _modest_global_settings_dialog_get_msg_formats ();
	ppriv->msg_format = modest_selector_picker_new (ppriv->msg_format_list, g_int_equal);
	hildon_button_set_title (HILDON_BUTTON (ppriv->msg_format), _("mcen_fi_options_messageformat"));

	gtk_box_pack_start (GTK_BOX (vbox), ppriv->msg_format, FALSE, FALSE, MODEST_MARGIN_HALF);

	return vbox;
}


static void
on_auto_update_clicked (GtkButton *button,
			gpointer user_data)
{
	ModestGlobalSettingsDialogPrivate *ppriv;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (user_data);

	if (hildon_check_button_get_active (button)) {
		gtk_widget_set_sensitive (ppriv->connect_via, TRUE);
		gtk_widget_set_sensitive (ppriv->update_interval, TRUE);
	} else {
		gtk_widget_set_sensitive (ppriv->connect_via, FALSE);
		gtk_widget_set_sensitive (ppriv->update_interval, FALSE);
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
#ifdef MODEST_HAVE_HILDON0_WIDGETS
	case MAXIMUM_VALUE_EXCEED:
#else
	case HILDON_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED:
#endif
		msg = g_strdup_printf (dgettext("hildon-libs", "ckct_ib_maximum_value"), MSG_SIZE_MAX_VAL);
		new_val = MSG_SIZE_MAX_VAL;
		break;
#ifdef MODEST_HAVE_HILDON0_WIDGETS
	case MINIMUM_VALUE_EXCEED:
#else
	case HILDON_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED:
#endif
		msg = g_strdup_printf (dgettext("hildon-libs", "ckct_ib_minimum_value"), MSG_SIZE_MIN_VAL);
		new_val = MSG_SIZE_MIN_VAL;
		break;
#ifdef MODEST_HAVE_HILDON0_WIDGETS
	case ERRONEOUS_VALUE:
#else
	case HILDON_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE:
#endif
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


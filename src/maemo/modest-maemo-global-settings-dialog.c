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

#ifdef MODEST_HILDON_VERSION_0
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-number-editor.h>
#include <hildon-widgets/hildon-banner.h>
#else
#include <hildon/hildon-caption.h>
#include <hildon/hildon-number-editor.h>
#include <hildon/hildon-banner.h>
#endif /*MODEST_HILDON_VERSION_0*/

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
#include "widgets/modest-combo-box.h"
#include "maemo/modest-maemo-global-settings-dialog.h"
#include "widgets/modest-ui-constants.h"
#include <tny-account-store.h>
#include <tny-maemo-conic-device.h>

#define MSG_SIZE_MAX_VAL 5000
#define MSG_SIZE_DEF_VAL 1000
#define MSG_SIZE_MIN_VAL 1

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

static void       on_auto_update_toggled (GtkToggleButton *togglebutton,
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

static void
modest_maemo_global_settings_dialog_init (ModestMaemoGlobalSettingsDialog *self)
{
	ModestGlobalSettingsDialogPrivate *ppriv;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	ppriv->updating_page = create_updating_page (self);
	ppriv->composing_page = create_composing_page (self);
    
	/* Add the notebook pages: */
	gtk_notebook_append_page (GTK_NOTEBOOK (ppriv->notebook), ppriv->updating_page, 
		gtk_label_new (_("mcen_ti_options_updating")));
	gtk_notebook_append_page (GTK_NOTEBOOK (ppriv->notebook), ppriv->composing_page, 
		gtk_label_new (_("mcen_ti_options_composing")));
		
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), ppriv->notebook);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (self)->vbox), MODEST_MARGIN_HALF);

	/* Load current config */
	_modest_global_settings_dialog_load_conf (MODEST_GLOBAL_SETTINGS_DIALOG (self));
	gtk_widget_show_all (ppriv->notebook);
}

static void
modest_maemo_global_settings_dialog_finalize (GObject *obj)
{
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
	GtkWidget *vbox, *vbox_update, *vbox_limit, *caption;
	GtkSizeGroup *size_group;
	ModestGlobalSettingsDialogPrivate *ppriv;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

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
	ppriv->size_limit = hildon_number_editor_new (MSG_SIZE_MIN_VAL, MSG_SIZE_MAX_VAL);;
	hildon_number_editor_set_value (HILDON_NUMBER_EDITOR (ppriv->size_limit), MSG_SIZE_DEF_VAL);;
	g_signal_connect (ppriv->size_limit, "range_error", G_CALLBACK (on_range_error), self);
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

	return vbox;
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

	/* Reply */
	ppriv->include_reply = gtk_check_button_new ();
	caption = hildon_caption_new (size_group, 
				      _("mcen_va_options_include_original_inreply"), 
				      ppriv->include_reply, 
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
#ifdef MODEST_HILDON_VERSION_0
	case MAXIMUM_VALUE_EXCEED:
#else
	case HILDON_NUMBER_EDITOR_ERROR_MAXIMUM_VALUE_EXCEED:
#endif
		msg = g_strdup_printf (_("ckct_ib_maximum_value"), MSG_SIZE_MAX_VAL);
		new_val = MSG_SIZE_MAX_VAL;
		break;
#ifdef MODEST_HILDON_VERSION_0
	case MINIMUM_VALUE_EXCEED:
#else
	case HILDON_NUMBER_EDITOR_ERROR_MINIMUM_VALUE_EXCEED:
#endif
		msg = g_strdup_printf (_("ckct_ib_minimum_value"), MSG_SIZE_MIN_VAL);
		new_val = MSG_SIZE_MIN_VAL;
		break;
#ifdef MODEST_HILDON_VERSION_0
	case ERRONEOUS_VALUE:
#else
	case HILDON_NUMBER_EDITOR_ERROR_ERRONEOUS_VALUE:
#endif
		msg = g_strdup_printf (_("ckct_ib_set_a_value_within_range"), 
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

static ModestConnectedVia
current_connection (void)
{
	TnyAccountStore *account_store;
	TnyDevice *device;
	ModestConnectedVia retval = MODEST_CONNECTED_VIA_ANY;
	
	account_store = TNY_ACCOUNT_STORE (modest_runtime_get_account_store ());
	device = tny_account_store_get_device (account_store);

	/* Get iap id */
	const gchar *iap_id = tny_maemo_conic_device_get_current_iap_id (TNY_MAEMO_CONIC_DEVICE (device));
	if (iap_id) {
		ConIcIap *iap = tny_maemo_conic_device_get_iap (
			TNY_MAEMO_CONIC_DEVICE (device), iap_id);
		const gchar *bearer_type = con_ic_iap_get_bearer_type (iap);
			
		if (!strcmp (bearer_type, CON_IC_BEARER_WLAN_INFRA) ||
		    !strcmp (bearer_type, CON_IC_BEARER_WLAN_ADHOC))
			retval = MODEST_CONNECTED_VIA_WLAN;
		else
			retval = MODEST_CONNECTED_VIA_ANY;
	
		g_object_unref (iap);
	}
	g_object_unref (device);

	return retval;
}

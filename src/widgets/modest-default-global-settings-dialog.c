/* Copyright (c) 2006, 2009, Nokia Corporation
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

#include <modest-scrollable.h>

#include <glib/gi18n.h>
#include <string.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcheckbutton.h>
#include "modest-runtime.h"
#include <modest-global-settings-dialog-priv.h>
#include <modest-default-global-settings-dialog.h>
#include <modest-ui-constants.h>
#include "modest-text-utils.h"
#include "modest-defs.h"
#include <tny-account-store.h>
#include <modest-account-mgr-helpers.h>
#include <modest-toolkit-utils.h>


#define MSG_SIZE_MAX_VAL 5000
#define MSG_SIZE_DEF_VAL 1000
#define MSG_SIZE_MIN_VAL 1

#define DEFAULT_FOCUS_WIDGET "default-focus-widget"

/* 'private'/'protected' functions */
static void modest_default_global_settings_dialog_class_init (ModestDefaultGlobalSettingsDialogClass *klass);
static void modest_default_global_settings_dialog_init       (ModestDefaultGlobalSettingsDialog *obj);
static void modest_default_global_settings_dialog_finalize   (GObject *obj);

static ModestConnectedVia current_connection (void);

static GtkWidget* create_updating_page   (ModestDefaultGlobalSettingsDialog *self);

static void       on_auto_update_clicked (GtkButton *button,
					  gpointer user_data);
static void       update_sensitive       (ModestGlobalSettingsDialog *dialog);
static ModestPairList * get_accounts_list (void);

static void modest_default_global_settings_dialog_load_settings (ModestGlobalSettingsDialog *self);

typedef struct _ModestDefaultGlobalSettingsDialogPrivate ModestDefaultGlobalSettingsDialogPrivate;
struct _ModestDefaultGlobalSettingsDialogPrivate {
	ModestPairList *connect_via_list;
};
#define MODEST_DEFAULT_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                           MODEST_TYPE_DEFAULT_GLOBAL_SETTINGS_DIALOG, \
                                                           ModestDefaultGlobalSettingsDialogPrivate))
/* globals */
static GtkDialogClass *parent_class = NULL;

GType
modest_default_global_settings_dialog_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestDefaultGlobalSettingsDialogClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_default_global_settings_dialog_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestDefaultGlobalSettingsDialog),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_default_global_settings_dialog_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_GLOBAL_SETTINGS_DIALOG,
		                                  "ModestDefaultGlobalSettingsDialog",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_default_global_settings_dialog_class_init (ModestDefaultGlobalSettingsDialogClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_default_global_settings_dialog_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestDefaultGlobalSettingsDialogPrivate));

	MODEST_GLOBAL_SETTINGS_DIALOG_CLASS (klass)->current_connection_func = current_connection;
}

typedef struct {
	ModestDefaultGlobalSettingsDialog *dia;
	GtkWidget *focus_widget;
} SwitchPageHelper;


static void
modest_default_global_settings_dialog_init (ModestDefaultGlobalSettingsDialog *self)
{
	ModestDefaultGlobalSettingsDialogPrivate *priv;
	ModestGlobalSettingsDialogPrivate *ppriv;
	GtkWidget *align;
	GtkWidget *top_vbox;

	priv  = MODEST_DEFAULT_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	ppriv->updating_page = create_updating_page (self);
	top_vbox = gtk_vbox_new (FALSE, 0);
	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 
				   MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE, 
				   MODEST_MARGIN_DOUBLE, MODEST_MARGIN_TRIPLE);

	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (self), _HL_SAVE, GTK_RESPONSE_OK);
	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);

	/* Set the default focusable widgets */
	g_object_set_data (G_OBJECT(ppriv->updating_page), DEFAULT_FOCUS_WIDGET,
			   (gpointer)ppriv->auto_update);

	gtk_container_add (GTK_CONTAINER (top_vbox), ppriv->updating_page);
	gtk_container_add (GTK_CONTAINER (align), top_vbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), align);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (self)->vbox), MODEST_MARGIN_HALF);
	gtk_window_set_default_size (GTK_WINDOW (self), MODEST_DIALOG_WINDOW_MAX_WIDTH, MODEST_DIALOG_WINDOW_MAX_HEIGHT);

	gtk_widget_show (align);
	gtk_widget_show (top_vbox);
}

static void
modest_default_global_settings_dialog_finalize (GObject *obj)
{
	ModestGlobalSettingsDialogPrivate *ppriv;
	ModestDefaultGlobalSettingsDialogPrivate *priv;

	priv = MODEST_DEFAULT_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (obj);
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (obj);

/* 	free/unref instance resources here */
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_default_global_settings_dialog_new (void)
{
	GtkWidget *self = GTK_WIDGET(g_object_new(MODEST_TYPE_DEFAULT_GLOBAL_SETTINGS_DIALOG, NULL));

	/* Load settings */
	modest_default_global_settings_dialog_load_settings (MODEST_GLOBAL_SETTINGS_DIALOG (self));

	return self;
}

/*
 * Creates the updating page
 */
static GtkWidget*
create_updating_page (ModestDefaultGlobalSettingsDialog *self)
{
	GtkWidget *vbox;
	GtkSizeGroup *title_size_group;
	GtkSizeGroup *value_size_group;
	ModestGlobalSettingsDialogPrivate *ppriv;
	GtkWidget *scrollable, *separator;
	ModestDefaultGlobalSettingsDialogPrivate *priv;
	PangoAttrList *attr_list;

	priv = MODEST_DEFAULT_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_HALF);

	title_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	value_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Default account selector */
	ppriv->accounts_list = get_accounts_list ();
	ppriv->default_account_selector = modest_toolkit_factory_create_selector (modest_runtime_get_toolkit_factory (),
										  ppriv->accounts_list,
										  g_str_equal, TRUE);
	if (ppriv->accounts_list == NULL) {
		gtk_widget_set_sensitive (GTK_WIDGET (ppriv->default_account_selector), FALSE);
	} else {
		gchar *default_account;

		default_account = modest_account_mgr_get_default_account (
			modest_runtime_get_account_mgr ());
		if (default_account) {
			modest_selector_set_active_id (ppriv->default_account_selector,
						       default_account);
			ppriv->initial_state.default_account = default_account;
		}
	}
	if (GTK_IS_COMBO_BOX (ppriv->default_account_selector)) {
		GtkWidget *caption;

		caption = modest_toolkit_utils_create_vcaptioned (title_size_group,
								  _("mcen_ti_default_account"), FALSE,
								  ppriv->default_account_selector);
		gtk_widget_show (caption);
		gtk_box_pack_start (GTK_BOX (vbox), caption,
				    FALSE, FALSE, 0);
	} else {
		modest_toolkit_utils_set_vbutton_layout (title_size_group,
							 _("mcen_ti_default_account"),
							 ppriv->default_account_selector);
		gtk_box_pack_start (GTK_BOX (vbox), ppriv->default_account_selector,
				    FALSE, FALSE, 0);
	}

	/* Message format */
	/* Note: This ModestPairList* must exist for as long as the picker
	 * that uses it, because the ModestSelectorPicker uses the ID opaquely,
	 * so it can't know how to manage its memory. */
	ppriv->msg_format_list = _modest_global_settings_dialog_get_msg_formats ();
	ppriv->msg_format = modest_toolkit_factory_create_selector (modest_runtime_get_toolkit_factory (),
								    ppriv->msg_format_list, g_int_equal, TRUE);
	if (GTK_IS_COMBO_BOX (ppriv->msg_format)) {
		GtkWidget *caption;
		caption = modest_toolkit_utils_create_vcaptioned (title_size_group,
								  _("mcen_fi_options_messageformat"), FALSE,
								  ppriv->msg_format);
		gtk_widget_show (caption);
		gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, 0);
	} else {
		modest_toolkit_utils_set_vbutton_layout (title_size_group,
							 _("mcen_fi_options_messageformat"),
							 ppriv->msg_format);
		gtk_box_pack_start (GTK_BOX (vbox), ppriv->msg_format, FALSE, FALSE, 0);
	}


	/* Incoming notifications */
	ppriv->notifications = modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
									   _("mcen_fi_options_incoming_notifications"));
	gtk_box_pack_start (GTK_BOX (vbox), ppriv->notifications, FALSE, FALSE, 0);

	/* Automatic add to contacts */
	ppriv->add_to_contacts = modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
									     _("mcen_fi_options_automatic_add"));
	gtk_box_pack_start (GTK_BOX (vbox), ppriv->add_to_contacts, FALSE, FALSE, 0);

	/* Tree view */
	ppriv->tree_view = modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
									     _("mcen_fi_options_tree_view"));
	gtk_box_pack_start (GTK_BOX (vbox), ppriv->tree_view, FALSE, FALSE, 0);

	/* Separator label */
	separator = gtk_label_new (_("mcen_ti_updating"));
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
	gtk_label_set_attributes (GTK_LABEL (separator), attr_list);
	pango_attr_list_unref (attr_list);
	gtk_label_set_justify ((GtkLabel *) separator, GTK_JUSTIFY_CENTER);
	gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, MODEST_MARGIN_DEFAULT);

	/* Auto update */
	ppriv->auto_update = modest_toolkit_factory_create_check_button (modest_runtime_get_toolkit_factory (),
									 _("mcen_fi_options_autoupdate"));
	gtk_box_pack_start (GTK_BOX (vbox), ppriv->auto_update, FALSE, FALSE, 0);
	g_signal_connect (ppriv->auto_update, "clicked", G_CALLBACK (on_auto_update_clicked), self);

	/* Connected via */

	/* Note: This ModestPairList* must exist for as long as the picker
	 * that uses it, because the ModestSelectorPicker uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->connect_via_list = _modest_global_settings_dialog_get_connected_via ();
	ppriv->connect_via = modest_toolkit_factory_create_selector (modest_runtime_get_toolkit_factory (),
								     ppriv->connect_via_list, g_int_equal, TRUE);
	if (GTK_IS_COMBO_BOX (ppriv->connect_via)) {
		GtkWidget *caption;
		caption = modest_toolkit_utils_create_vcaptioned (title_size_group,
								  _("mcen_fi_options_connectiontype"), FALSE,
								  ppriv->connect_via);
		gtk_widget_show (caption);
		gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, 0);
	} else {
		modest_toolkit_utils_set_vbutton_layout (title_size_group, 
							 _("mcen_fi_options_connectiontype"),
							 ppriv->connect_via);
		gtk_box_pack_start (GTK_BOX (vbox), ppriv->connect_via, FALSE, FALSE, 0);
	}

	/* Update interval */

	/* Note: This ModestPairList* must exist for as long as the picker
	 * that uses it, because the ModestSelectorPicker uses the ID opaquely, 
	 * so it can't know how to manage its memory. */ 
	ppriv->update_interval_list = _modest_global_settings_dialog_get_update_interval ();
	ppriv->update_interval = modest_toolkit_factory_create_selector (modest_runtime_get_toolkit_factory (),
									 ppriv->update_interval_list, g_int_equal, TRUE);
	if (GTK_IS_COMBO_BOX (ppriv->update_interval)) {
		GtkWidget *caption;
		caption = modest_toolkit_utils_create_vcaptioned (title_size_group,
								  _("mcen_fi_options_updateinterval"), FALSE,
								  ppriv->update_interval);
		gtk_widget_show (caption);
		gtk_box_pack_start (GTK_BOX (vbox), caption, FALSE, FALSE, 0);
	} else {
		modest_toolkit_utils_set_vbutton_layout (title_size_group, 
							 _("mcen_fi_options_updateinterval"), 
							 ppriv->update_interval);
		gtk_box_pack_start (GTK_BOX (vbox), ppriv->update_interval, FALSE, FALSE, 0);
	}

	scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());

	modest_scrollable_add_with_viewport (MODEST_SCROLLABLE (scrollable), vbox);
	gtk_widget_show (vbox);
	gtk_widget_show (scrollable);

	g_object_unref (title_size_group);
	g_object_unref (value_size_group);

	return scrollable;
}


static void
update_sensitive (ModestGlobalSettingsDialog *dialog)
{
	ModestGlobalSettingsDialogPrivate *ppriv;

	g_return_if_fail (MODEST_IS_GLOBAL_SETTINGS_DIALOG (dialog));
	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (dialog);

	if (modest_togglable_get_active (ppriv->auto_update)) {
		gtk_widget_set_sensitive (ppriv->connect_via, TRUE);
		gtk_widget_set_sensitive (ppriv->update_interval, TRUE);
	} else {
		gtk_widget_set_sensitive (ppriv->connect_via, FALSE);
		gtk_widget_set_sensitive (ppriv->update_interval, FALSE);
	}
}

static void
on_auto_update_clicked (GtkButton *button,
			gpointer user_data)
{
	g_return_if_fail (MODEST_IS_GLOBAL_SETTINGS_DIALOG (user_data));
	update_sensitive ((ModestGlobalSettingsDialog *) user_data);
}

static ModestConnectedVia
current_connection (void)
{
	return modest_platform_get_current_connection ();
}

static gint
order_by_acc_name (gconstpointer a,
		   gconstpointer b)
{
	ModestPair *pair_a, *pair_b;

	pair_a = (ModestPair *) a;
	pair_b = (ModestPair *) b;

	if (pair_a->second && pair_b->second) {
		gint compare = g_utf8_collate ((gchar *) pair_a->second,
					       (gchar *) pair_b->second);
		if (compare > 0)
			compare = -1;
		else if (compare < 0)
			compare = 1;

		return compare;
	} else {
		return 0;
	}
}

static ModestPairList *
get_accounts_list (void)
{
	GSList *list = NULL;
	GSList *cursor, *account_names;
	ModestAccountMgr *account_mgr;

	account_mgr = modest_runtime_get_account_mgr ();

	cursor = account_names = modest_account_mgr_account_names (account_mgr, TRUE /*only enabled*/);
	while (cursor) {
		gchar *account_name;
		ModestAccountSettings *settings;
		ModestServerAccountSettings *store_settings;

		account_name = (gchar*)cursor->data;

		settings = modest_account_mgr_load_account_settings (account_mgr, account_name);
		if (!settings) {
			g_printerr ("modest: failed to get account data for %s\n", account_name);
			cursor = cursor->next;
			continue;
		}
		store_settings = modest_account_settings_get_store_settings (settings);

		/* don't display accounts without stores */
		if (modest_server_account_settings_get_account_name (store_settings) != NULL) {

			if (modest_account_settings_get_enabled (settings)) {
				ModestPair *pair;

				pair = modest_pair_new (
					g_strdup (account_name),
					g_strdup (modest_account_settings_get_display_name (settings)),
					FALSE);
				list = g_slist_insert_sorted (list, pair, order_by_acc_name);
			}
		}

		g_object_unref (store_settings);
		g_object_unref (settings);
		cursor = cursor->next;
	}

	return (ModestPairList *) g_slist_reverse (list);
}


static void
modest_default_global_settings_dialog_load_settings (ModestGlobalSettingsDialog *self)
{
	ModestConf *conf;
	gboolean checked;
	gint combo_id;
	GError *error = NULL;
	ModestGlobalSettingsDialogPrivate *ppriv;

	ppriv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);
	conf = modest_runtime_get_conf ();

	/* Incoming notifications */
	checked = modest_conf_get_bool (conf, MODEST_CONF_NOTIFICATIONS, &error);
	if (error) {
		g_clear_error (&error);
		error = NULL;
		checked = FALSE;
	}
	modest_togglable_set_active (ppriv->notifications, checked);
	ppriv->initial_state.notifications = checked;

	/* Add to contacts */
	checked = modest_conf_get_bool (conf, MODEST_CONF_AUTO_ADD_TO_CONTACTS, &error);
	if (error) {
		g_clear_error (&error);
		error = NULL;
		checked = FALSE;
	}
	modest_togglable_set_active (ppriv->add_to_contacts, checked);
	ppriv->initial_state.add_to_contacts = checked;

	/* Tree view */
	checked = modest_conf_get_bool (conf, MODEST_CONF_TREE_VIEW, &error);
	if (error) {
		g_clear_error (&error);
		error = NULL;
		checked = FALSE;
	}
	modest_togglable_set_active (ppriv->tree_view, checked);
	ppriv->initial_state.tree_view = checked;

	/* Autoupdate */
	checked = modest_conf_get_bool (conf, MODEST_CONF_AUTO_UPDATE, &error);
	if (error) {
		g_clear_error (&error);
		error = NULL;
		checked = FALSE;
	}
	modest_togglable_set_active (ppriv->auto_update, checked);
	ppriv->initial_state.auto_update = checked;

	/* Connected by */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_WHEN_CONNECTED_BY, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_CONNECTED_VIA_WLAN_OR_WIMAX;
	}
	modest_selector_set_active_id (ppriv->connect_via,
				       (gpointer) &combo_id);
	ppriv->initial_state.connect_via = combo_id;

	/* Update interval */
	combo_id = modest_conf_get_int (conf, MODEST_CONF_UPDATE_INTERVAL, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_UPDATE_INTERVAL_15_MIN;
	}
	modest_selector_set_active_id (ppriv->update_interval,
				       (gpointer) &combo_id);
	ppriv->initial_state.update_interval = combo_id;

	/* Play sound */
	checked = modest_conf_get_bool (conf, MODEST_CONF_PLAY_SOUND_MSG_ARRIVE, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		checked = FALSE;
	}
	ppriv->initial_state.play_sound = checked;

	/* Msg format */
	checked = modest_conf_get_bool (conf, MODEST_CONF_PREFER_FORMATTED_TEXT, &error);
	if (error) {
		g_error_free (error);
		error = NULL;
		combo_id = MODEST_FILE_FORMAT_FORMATTED_TEXT;
	}
	combo_id = (checked) ? MODEST_FILE_FORMAT_FORMATTED_TEXT : MODEST_FILE_FORMAT_PLAIN_TEXT;
	modest_selector_set_active_id (ppriv->msg_format,
				       (gpointer) &combo_id);
	ppriv->initial_state.prefer_formatted_text = checked;

	/* force update of sensitiveness */
	update_sensitive (MODEST_GLOBAL_SETTINGS_DIALOG (self));
}

/* Copyright (c) 2008, Nokia Corporation
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

#include <modest-folder-window.h>
#include <modest-osso-state-saving.h>
#include <libosso.h>
#include <hildon/hildon-pannable-area.h>
#include <modest-window-mgr.h>
#include <modest-signal-mgr.h>
#include <modest-runtime.h>
#include <modest-platform.h>
#include <modest-maemo-utils.h>
#include <modest-icon-names.h>
#include <modest-ui-constants.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-defs.h>
#include <modest-ui-actions.h>
#include <modest-window.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-button.h>
#include <tny-account-store-view.h>
#include <modest-header-window.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>
#include "modest-text-utils.h"
#include "modest-tny-account.h"

typedef enum {
	EDIT_MODE_COMMAND_MOVE = 1,
	EDIT_MODE_COMMAND_DELETE = 2,
	EDIT_MODE_COMMAND_RENAME = 3,
} EditModeCommand;

/* 'private'/'protected' functions */
static void modest_folder_window_class_init  (ModestFolderWindowClass *klass);
static void modest_folder_window_init        (ModestFolderWindow *obj);
static void modest_folder_window_finalize    (GObject *obj);

static void connect_signals (ModestFolderWindow *self);
static void modest_folder_window_disconnect_signals (ModestWindow *self);

static void on_folder_activated (ModestFolderView *folder_view,
				 TnyFolder *folder,
				 gpointer userdata);
static void setup_menu (ModestFolderWindow *self);

static void set_delete_edit_mode (GtkButton *button,
				  ModestFolderWindow *self);
static void set_moveto_edit_mode (GtkButton *button,
				  ModestFolderWindow *self);
static void set_rename_edit_mode (GtkButton *button,
				  ModestFolderWindow *self);
static void modest_folder_window_pack_toolbar (ModestHildon2Window *self,
					       GtkPackType pack_type,
					       GtkWidget *toolbar);
static void edit_mode_changed (ModestFolderWindow *folder_window,
			       gint edit_mode_id,
			       gboolean enabled,
			       ModestFolderWindow *self);
static void on_progress_list_changed (ModestWindowMgr *mgr,
				      ModestFolderWindow *self);
static gboolean on_map_event (GtkWidget *widget,
			      GdkEvent *event,
			      gpointer userdata);
static void update_progress_hint (ModestFolderWindow *self);

typedef struct _ModestFolderWindowPrivate ModestFolderWindowPrivate;
struct _ModestFolderWindowPrivate {

	GtkWidget *folder_view;
	GtkWidget *top_vbox;
	GtkWidget *new_message_button;

	/* signals */
	GSList *sighandlers;

	gchar *current_store_account;
	gboolean progress_hint;
};
#define MODEST_FOLDER_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									  MODEST_TYPE_FOLDER_WINDOW, \
									  ModestFolderWindowPrivate))

/* globals */
static GtkWindowClass *parent_class = NULL;

/************************************************************************/

GType
modest_folder_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestFolderWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_folder_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestFolderWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_folder_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_HILDON2_WINDOW,
		                                  "ModestFolderWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_folder_window_class_init (ModestFolderWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;
	ModestHildon2WindowClass *modest_hildon2_window_class = (ModestHildon2WindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_folder_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestFolderWindowPrivate));
	
	modest_window_class->disconnect_signals_func = modest_folder_window_disconnect_signals;
	modest_hildon2_window_class->pack_toolbar_func = modest_folder_window_pack_toolbar;
}

static void
modest_folder_window_init (ModestFolderWindow *obj)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(obj);

	priv->sighandlers = NULL;
	
	priv->folder_view = NULL;

	priv->top_vbox = NULL;

	modest_window_mgr_register_help_id (modest_runtime_get_window_mgr(),
					    GTK_WINDOW(obj),
					    "applications_email_folderview");
	priv->progress_hint = FALSE;
	priv->current_store_account = NULL;
}

static void
modest_folder_window_finalize (GObject *obj)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(obj);

	if (priv->current_store_account) {
		g_free (priv->current_store_account);
		priv->current_store_account = NULL;
	}

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_folder_window_disconnect_signals (MODEST_WINDOW (obj));	

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_folder_window_disconnect_signals (ModestWindow *self)
{
	ModestFolderWindowPrivate *priv;
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;
}

static void
on_visible_account_changed (ModestFolderView *folder_view,
			    const gchar *account_id,
			    gpointer user_data)
{
	if (account_id) {
		TnyAccount *acc = 
			modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store(),
								     MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
								     account_id);
		if (acc) {
			const gchar *name;
			gchar *title = NULL;

			name = modest_tny_account_get_parent_modest_account_name_for_server_account (acc);
			title = modest_account_mgr_get_display_name (modest_runtime_get_account_mgr(),
								     name);
			if (title) {
				gtk_window_set_title (GTK_WINDOW (user_data), title);
				g_free (title);
			} else {
				gtk_window_set_title (GTK_WINDOW (user_data), _("mcen_ap_name"));
			}
			g_object_unref (acc);
		}
	} else {
		gtk_window_set_title (GTK_WINDOW (user_data), _("mcen_ap_name"));
	}
}

static void
connect_signals (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);

	/* folder view */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
						       G_OBJECT (priv->folder_view), "folder-activated", 
						       G_CALLBACK (on_folder_activated), self);

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (modest_runtime_get_window_mgr ()),
						       "progress-list-changed",
						       G_CALLBACK (on_progress_list_changed), self);

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (priv->folder_view),
						       "visible-account-changed",
						       G_CALLBACK (on_visible_account_changed), self);

	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->new_message_button),
					   "clicked",
					   G_CALLBACK (modest_ui_actions_on_new_msg), self);

}

ModestWindow *
modest_folder_window_new (TnyFolderStoreQuery *query)
{
	ModestFolderWindow *self = NULL;	
	ModestFolderWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	GtkWidget *pannable;
	GtkWidget *action_area_box;
	GdkPixbuf *new_message_pixbuf;
	
	self  = MODEST_FOLDER_WINDOW(g_object_new(MODEST_TYPE_FOLDER_WINDOW, NULL));
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);

	pannable = hildon_pannable_area_new ();
	priv->folder_view  = modest_platform_create_folder_view (query);
	modest_folder_view_set_cell_style (MODEST_FOLDER_VIEW (priv->folder_view),
					   MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT);
	modest_folder_view_set_filter (MODEST_FOLDER_VIEW (priv->folder_view), 
				       MODEST_FOLDER_VIEW_FILTER_HIDE_ACCOUNTS);

	g_signal_connect (G_OBJECT (self), "edit-mode-changed",
			  G_CALLBACK (edit_mode_changed), (gpointer) self);

	action_area_box = hildon_tree_view_get_action_area_box (GTK_TREE_VIEW (priv->folder_view));
	priv->new_message_button = hildon_button_new (0, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	hildon_button_set_title (HILDON_BUTTON (priv->new_message_button), _("mcen_ti_new_message"));
	new_message_pixbuf = modest_platform_get_icon ("general_add", MODEST_ICON_SIZE_BIG);
	hildon_button_set_image (HILDON_BUTTON (priv->new_message_button), gtk_image_new_from_pixbuf (new_message_pixbuf));
	g_object_unref (new_message_pixbuf);

	gtk_box_pack_start (GTK_BOX (action_area_box), priv->new_message_button, TRUE, TRUE, 0);
	gtk_widget_show_all (priv->new_message_button);
	hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (priv->folder_view), TRUE);
	
	setup_menu (self);

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	priv->top_vbox = gtk_vbox_new (0, FALSE);

	gtk_container_add (GTK_CONTAINER (pannable), priv->folder_view);
	gtk_box_pack_end (GTK_BOX (priv->top_vbox), pannable, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (self), priv->top_vbox);

	gtk_widget_show (priv->folder_view);
	gtk_widget_show (pannable);
	gtk_widget_show (priv->top_vbox);

	connect_signals (MODEST_FOLDER_WINDOW (self));

	/* Load previous osso state, for instance if we are being restored from 
	 * hibernation:  */
	modest_osso_load_state ();

	/* Get device name */
	modest_maemo_utils_get_device_name ();

	app = hildon_program_get_instance ();
	hildon_program_add_window (app, HILDON_WINDOW (self));
	
	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_ICON, MODEST_ICON_SIZE_BIG);
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (self), window_icon);
		g_object_unref (window_icon);
	}

	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */

	/* Register edit modes */
	modest_hildon2_window_register_edit_mode (MODEST_HILDON2_WINDOW (self), 
						  EDIT_MODE_COMMAND_DELETE,
						  _("mcen_ti_edit_folder_delete"), 
						  _HL("wdgt_bd_delete"),
						  GTK_TREE_VIEW (priv->folder_view),
						  GTK_SELECTION_SINGLE,
						  EDIT_MODE_CALLBACK (modest_ui_actions_on_edit_mode_delete_folder));
	modest_hildon2_window_register_edit_mode (MODEST_HILDON2_WINDOW (self), 
						  EDIT_MODE_COMMAND_MOVE,
						  _("mcen_ti_edit_move_folder"), 
						  _HL("wdgt_bd_move"),
						  GTK_TREE_VIEW (priv->folder_view),
						  GTK_SELECTION_SINGLE,
						  EDIT_MODE_CALLBACK (modest_ui_actions_on_edit_mode_move_to));	
	modest_hildon2_window_register_edit_mode (MODEST_HILDON2_WINDOW (self), 
						  EDIT_MODE_COMMAND_RENAME,
						  _("mcen_ti_edit_rename_folder"), 
						  _HL("wdgt_bd_rename"),
						  GTK_TREE_VIEW (priv->folder_view),
						  GTK_SELECTION_SINGLE,
						  EDIT_MODE_CALLBACK (modest_ui_actions_on_edit_mode_rename_folder));
	
	g_signal_connect (G_OBJECT (self), "map-event",
			  G_CALLBACK (on_map_event),
			  G_OBJECT (self));
	update_progress_hint (self);

	return MODEST_WINDOW(self);
}

ModestFolderView *
modest_folder_window_get_folder_view (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_FOLDER_WINDOW(self), FALSE);

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);
	
	return MODEST_FOLDER_VIEW (priv->folder_view);
}

void
modest_folder_window_set_account (ModestFolderWindow *self,
				  const gchar *account_name)
{
	ModestFolderWindowPrivate *priv = NULL;
	ModestAccountMgr *mgr;
	ModestAccountSettings *settings = NULL;
	ModestServerAccountSettings *store_settings = NULL;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	/* Get account data */
	mgr = modest_runtime_get_account_mgr ();

	settings = modest_account_mgr_load_account_settings (mgr, account_name);
	if (!settings)
		goto free_refs;

	store_settings = modest_account_settings_get_store_settings (settings);
	if (!store_settings)
		goto free_refs;

	if (priv->current_store_account != NULL)
		g_free (priv->current_store_account);
	priv->current_store_account = g_strdup (modest_server_account_settings_get_account_name (store_settings));

	modest_folder_view_set_account_id_of_visible_server_account
		(MODEST_FOLDER_VIEW (priv->folder_view),
		 priv->current_store_account);

	modest_window_set_active_account (MODEST_WINDOW (self), account_name);
	update_progress_hint (self);

free_refs:
	if (store_settings)
		g_object_unref (store_settings);
	if (settings)
		g_object_unref (settings);

}

static void
setup_menu (ModestFolderWindow *self)
{
	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));

	/* folders actions */
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_new_folder"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_new_folder),
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_rename_folder"), NULL,
					   APP_MENU_CALLBACK (set_rename_edit_mode),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_rename_folder));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_move_folder"), NULL,
					   APP_MENU_CALLBACK (set_moveto_edit_mode),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_folder_window_move_to));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_delete_folder"), NULL,
					   APP_MENU_CALLBACK (set_delete_edit_mode),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_folder_window_delete));

	/* send receive actions should be only one visible always */
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), 
					   _("mcen_me_inbox_sendandreceive"), 
					   NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_send_receive),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_send_receive));

	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_outbox_cancelsend"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_cancel_send),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_cancel_sending_all));
	/* new message */
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), 
					   _("mcen_me_new_message"), 
					   "<Ctrl>n",
					   APP_MENU_CALLBACK (modest_ui_actions_on_new_msg),
					   NULL);

}

static void
on_folder_activated (ModestFolderView *folder_view,
		     TnyFolder *folder,
		     gpointer userdata)
{
	ModestFolderWindowPrivate *priv = NULL;
	ModestWindow *headerwin;
	ModestFolderWindow *self = (ModestFolderWindow *) userdata;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	if (!folder)
		return;

	if (!TNY_IS_FOLDER (folder))
		return;

	/* We cannot open noselect folders (fake ones) */
	if (tny_folder_get_caps (folder) & TNY_FOLDER_CAPS_NOSELECT)
		return;

	headerwin = modest_header_window_new (folder, modest_window_get_active_account (MODEST_WINDOW (self)));

	if (modest_window_mgr_register_window (modest_runtime_get_window_mgr (),
					       MODEST_WINDOW (headerwin),
					       MODEST_WINDOW (self))) {
		gtk_widget_show (GTK_WIDGET (headerwin));
	} else {
		gtk_widget_destroy (GTK_WIDGET (headerwin));
		headerwin = NULL;
	}
}

static void
set_delete_edit_mode (GtkButton *button,
		      ModestFolderWindow *self)
{
	modest_hildon2_window_set_edit_mode (MODEST_HILDON2_WINDOW (self), EDIT_MODE_COMMAND_DELETE);
}

static void
set_moveto_edit_mode (GtkButton *button,
		      ModestFolderWindow *self)
{
	modest_hildon2_window_set_edit_mode (MODEST_HILDON2_WINDOW (self), EDIT_MODE_COMMAND_MOVE);
}

static void
set_rename_edit_mode (GtkButton *button,
		      ModestFolderWindow *self)
{
	modest_hildon2_window_set_edit_mode (MODEST_HILDON2_WINDOW (self), EDIT_MODE_COMMAND_RENAME);
}

static void
modest_folder_window_pack_toolbar (ModestHildon2Window *self,
				   GtkPackType pack_type,
				   GtkWidget *toolbar)
{
	ModestFolderWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW (self));
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	if (pack_type == GTK_PACK_START) {
		gtk_box_pack_start (GTK_BOX (priv->top_vbox), toolbar, FALSE, FALSE, 0);
	} else {
		gtk_box_pack_end (GTK_BOX (priv->top_vbox), toolbar, FALSE, FALSE, 0);
	}
}

static void 
edit_mode_changed (ModestFolderWindow *folder_window,
		   gint edit_mode_id,
		   gboolean enabled,
		   ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv;
	ModestFolderViewFilter filter = MODEST_FOLDER_VIEW_FILTER_NONE;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW (self));
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	switch (edit_mode_id) {
	case EDIT_MODE_COMMAND_MOVE:
		filter = MODEST_FOLDER_VIEW_FILTER_MOVEABLE;
		break;
	case EDIT_MODE_COMMAND_DELETE:
		filter = MODEST_FOLDER_VIEW_FILTER_DELETABLE;
		break;
	case EDIT_MODE_COMMAND_RENAME:
		filter = MODEST_FOLDER_VIEW_FILTER_RENAMEABLE;
		break;
	case MODEST_HILDON2_WINDOW_EDIT_MODE_NONE:
		filter = MODEST_FOLDER_VIEW_FILTER_NONE;
		break;
	}

	hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (priv->folder_view), !enabled);
	if (enabled) {
		modest_folder_view_set_filter (MODEST_FOLDER_VIEW (priv->folder_view), 
					       filter);
	} else {
		GtkTreeSelection *sel;

		/* Unselect all. This will prevent us from keeping a
		   reference to a TnyObject that we don't want to
		   have */
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->folder_view));
		gtk_tree_selection_unselect_all (sel);

		modest_folder_view_unset_filter (MODEST_FOLDER_VIEW (priv->folder_view), 
						 filter);
	}
}

static gboolean 
on_map_event (GtkWidget *widget,
	      GdkEvent *event,
	      gpointer userdata)
{
	ModestFolderWindow *self = (ModestFolderWindow *) userdata;
	ModestFolderWindowPrivate *priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	if (priv->progress_hint) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), TRUE);
	}

	return FALSE;
}

static void
update_progress_hint (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	if (!priv->current_store_account)
		return;

	priv->progress_hint = modest_window_mgr_has_progress_operation_on_account (modest_runtime_get_window_mgr (),
										   priv->current_store_account);

	if (GTK_WIDGET_VISIBLE (self)) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), priv->progress_hint ? 1:0);
	}
}

static void
on_progress_list_changed (ModestWindowMgr *mgr,
			  ModestFolderWindow *self)
{
	update_progress_hint (self);
}

gboolean
modest_folder_window_transfer_mode_enabled (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	return priv->progress_hint;
}

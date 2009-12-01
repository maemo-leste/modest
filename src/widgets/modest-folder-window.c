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
#include <modest-scrollable.h>
#include <modest-window-mgr.h>
#include <modest-signal-mgr.h>
#include <modest-runtime.h>
#include <modest-platform.h>
#include <modest-icon-names.h>
#include <modest-ui-constants.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-defs.h>
#include <modest-ui-actions.h>
#include <modest-window.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <modest-maemo-utils.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-button.h>
#endif
#include <tny-account-store-view.h>
#include <tny-gtk-folder-list-store.h>
#include <modest-header-window.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>
#include "modest-text-utils.h"
#include "modest-tny-account.h"
#include "modest-account-protocol.h"
#include <gdk/gdkkeysyms.h>

#ifdef MODEST_TOOLKIT_HILDON2
typedef enum {
	EDIT_MODE_COMMAND_MOVE = 1,
	EDIT_MODE_COMMAND_DELETE = 2,
	EDIT_MODE_COMMAND_RENAME = 3,
} EditModeCommand;
#endif

/* 'private'/'protected' functions */
static void modest_folder_window_class_init  (ModestFolderWindowClass *klass);
static void modest_folder_window_init        (ModestFolderWindow *obj);
static void modest_folder_window_finalize    (GObject *obj);
static void modest_folder_window_dispose     (GObject *obj);

static void connect_signals (ModestFolderWindow *self);
static void modest_folder_window_disconnect_signals (ModestWindow *self);

static void on_folder_activated (ModestFolderView *folder_view,
				 TnyFolder *folder,
				 gpointer userdata);
static void setup_menu (ModestFolderWindow *self);

#ifdef MODEST_TOOLKIT_HILDON2
static void set_delete_edit_mode (GtkButton *button,
				  ModestFolderWindow *self);
static void set_moveto_edit_mode (GtkButton *button,
				  ModestFolderWindow *self);
static void set_rename_edit_mode (GtkButton *button,
				  ModestFolderWindow *self);
#endif
static void modest_folder_window_pack_toolbar (ModestWindow *self,
					       GtkPackType pack_type,
					       GtkWidget *toolbar);
#ifdef MODEST_TOOLKIT_HILDON2
static void edit_mode_changed (ModestFolderWindow *folder_window,
			       gint edit_mode_id,
			       gboolean enabled,
			       ModestFolderWindow *self);
#endif
static void on_progress_list_changed (ModestWindowMgr *mgr,
				      ModestFolderWindow *self);
static gboolean on_map_event (GtkWidget *widget,
			      GdkEvent *event,
			      gpointer userdata);
static void update_progress_hint (ModestFolderWindow *self);
static void on_queue_changed    (ModestMailOperationQueue *queue,
				 ModestMailOperation *mail_op,
				 ModestMailOperationQueueNotification type,
				 ModestFolderWindow *self);
static void on_activity_changed (ModestFolderView *view,
				 gboolean activity,
				 ModestFolderWindow *folder_window);
static void on_visible_account_changed (ModestFolderView *folder_view,
					const gchar *account_id,
					gpointer user_data);
static void on_account_changed (TnyAccountStore *account_store,
				TnyAccount *account,
				gpointer user_data);
static gboolean on_key_press(GtkWidget *widget,
				GdkEventKey *event,
				gpointer user_data);

typedef struct _ModestFolderWindowPrivate ModestFolderWindowPrivate;
struct _ModestFolderWindowPrivate {

	GtkWidget *folder_view;
	GtkWidget *top_vbox;
#ifdef MODEST_TOOLKIT_HILDON2
	GtkWidget *new_message_button;
#endif

	/* signals */
	GSList *sighandlers;

	gchar *current_store_account;
	gboolean progress_hint;
	guint queue_change_handler;
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
		my_type = g_type_register_static (
#ifdef MODEST_TOOLKIT_HILDON2
						  MODEST_TYPE_HILDON2_WINDOW,
#else
						  MODEST_TYPE_SHELL_WINDOW,
#endif
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

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_folder_window_finalize;
	gobject_class->dispose = modest_folder_window_dispose;

	g_type_class_add_private (gobject_class, sizeof(ModestFolderWindowPrivate));
	
	modest_window_class->disconnect_signals_func = modest_folder_window_disconnect_signals;
	modest_window_class->pack_toolbar_func = modest_folder_window_pack_toolbar;
}

static void
modest_folder_window_init (ModestFolderWindow *obj)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(obj);

	priv->sighandlers = NULL;
	
	priv->folder_view = NULL;

	priv->top_vbox = NULL;

	priv->progress_hint = FALSE;
	priv->current_store_account = NULL;
	priv->queue_change_handler = 0;
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
modest_folder_window_dispose (GObject *obj)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(obj);
	if (priv->folder_view) {
		TnyList *list;

		list = modest_folder_view_get_model_tny_list (MODEST_FOLDER_VIEW (priv->folder_view));

		if (list) {
			TnyIterator *iter;

			iter = tny_list_create_iterator (list);
			while (!tny_iterator_is_done (iter)) {
				GObject *item = tny_iterator_get_current (iter);

				if (TNY_IS_ACCOUNT (item)) {
					if (TNY_IS_FOLDER_STORE (item) && 
					    modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (item))) {
						tny_account_cancel (TNY_ACCOUNT (item));
					}
				}
				g_object_unref (item);
				tny_iterator_next (iter);
			}
			g_object_unref (iter);
		}

		if (list && TNY_IS_GTK_FOLDER_LIST_STORE (list)) {
			g_object_run_dispose (G_OBJECT (list));
		}

		g_object_unref (list);
		priv->folder_view = NULL;
	}	

	G_OBJECT_CLASS(parent_class)->dispose (obj);
}

static void
modest_folder_window_disconnect_signals (ModestWindow *self)
{
	ModestFolderWindowPrivate *priv;
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);

	if (g_signal_handler_is_connected (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					   priv->queue_change_handler))
		g_signal_handler_disconnect (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					     priv->queue_change_handler);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;
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

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (priv->folder_view),
						       "activity-changed",
						       G_CALLBACK (on_activity_changed), self);

#ifdef MODEST_TOOLKIT_HILDON2
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (priv->new_message_button),
						       "clicked",
						       G_CALLBACK (modest_ui_actions_on_new_msg), self);
#endif

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (modest_runtime_get_account_store()),
						       "account-changed",
						       G_CALLBACK (on_account_changed), self);


	g_signal_connect(G_OBJECT(self), "key-press-event",
			G_CALLBACK(on_key_press), self);
}

ModestWindow *
modest_folder_window_new (TnyFolderStoreQuery *query)
{
	ModestFolderWindow *self = NULL;	
	ModestFolderWindowPrivate *priv = NULL;
	GdkPixbuf *window_icon;
	GtkWidget *scrollable;
	guint accel_key;
	GdkModifierType accel_mods;
	GtkWidget *top_alignment;
	
	self  = MODEST_FOLDER_WINDOW(g_object_new(MODEST_TYPE_FOLDER_WINDOW, NULL));
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);

	scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());

	priv->queue_change_handler =
		g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
				  "queue-changed",
				  G_CALLBACK (on_queue_changed),
				  self);

	priv->folder_view  = modest_platform_create_folder_view (query);
	modest_folder_view_set_cell_style (MODEST_FOLDER_VIEW (priv->folder_view),
					   MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT);
	modest_folder_view_set_filter (MODEST_FOLDER_VIEW (priv->folder_view), 
				       MODEST_FOLDER_VIEW_FILTER_HIDE_ACCOUNTS);

#ifdef MODEST_TOOLKIT_HILDON2
	GtkWidget *action_area_box;
	GdkPixbuf *new_message_pixbuf;
	g_signal_connect (G_OBJECT (self), "edit-mode-changed",
			  G_CALLBACK (edit_mode_changed), (gpointer) self);

	action_area_box = hildon_tree_view_get_action_area_box (GTK_TREE_VIEW (priv->folder_view));
	priv->new_message_button = hildon_button_new (MODEST_EDITABLE_SIZE, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	hildon_button_set_title (HILDON_BUTTON (priv->new_message_button), _("mcen_ti_new_message"));
	new_message_pixbuf = modest_platform_get_icon ("general_add", MODEST_ICON_SIZE_BIG);
	hildon_button_set_image (HILDON_BUTTON (priv->new_message_button), gtk_image_new_from_pixbuf (new_message_pixbuf));
	g_object_unref (new_message_pixbuf);

	gtk_box_pack_start (GTK_BOX (action_area_box), priv->new_message_button, TRUE, TRUE, 0);
	gtk_widget_show_all (priv->new_message_button);
	hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (priv->folder_view), TRUE);
#endif
	
	setup_menu (self);

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	priv->top_vbox = gtk_vbox_new (0, FALSE);
	top_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (top_alignment),
				   MODEST_MARGIN_HALF, 0,
				   MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE);

	gtk_container_add (GTK_CONTAINER (scrollable), priv->folder_view);
	gtk_container_add (GTK_CONTAINER (top_alignment), scrollable);
	gtk_box_pack_end (GTK_BOX (priv->top_vbox), top_alignment, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (self), priv->top_vbox);

	gtk_widget_show (priv->folder_view);
	gtk_widget_show (scrollable);
	gtk_widget_show (priv->top_vbox);
	gtk_widget_show (top_alignment);

	connect_signals (MODEST_FOLDER_WINDOW (self));

	/* Get device name */
#ifdef MODEST_TOOLKIT_HILDON2
	HildonProgram *app;
	modest_maemo_utils_get_device_name ();

	app = hildon_program_get_instance ();
	hildon_program_add_window (app, HILDON_WINDOW (self));
#endif
	
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
#ifdef MODEST_TOOLKIT_HILDON2
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
#endif
	
	g_signal_connect (G_OBJECT (self), "map-event",
			  G_CALLBACK (on_map_event),
			  G_OBJECT (self));
	update_progress_hint (self);

#ifdef MODEST_TOOLKIT_HILDON2
	GtkAccelGroup *accel_group;
	accel_group = gtk_accel_group_new ();
	gtk_accelerator_parse ("<Control>n", &accel_key, &accel_mods);
	gtk_widget_add_accelerator (priv->new_message_button, "clicked", accel_group,
				    accel_key, accel_mods, 0);
	gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);
#endif

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

void
modest_folder_window_set_mailbox (ModestFolderWindow *self,
				  const gchar *mailbox)
{
	ModestFolderWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	modest_folder_view_set_mailbox (MODEST_FOLDER_VIEW (priv->folder_view), mailbox);
	modest_window_set_active_mailbox (MODEST_WINDOW (self), mailbox);
}

static void
edit_account (GtkButton *button,
	      ModestFolderWindow *self)
{
	const gchar *account_name;

	account_name = modest_window_get_active_account ((ModestWindow *) self);
	if (modest_ui_actions_check_for_active_account ((ModestWindow *) self, account_name)) {
		/* Show the account settings dialog */
		ModestAccountProtocol *proto;
		ModestProtocolType proto_type;

		/* Get proto */
		proto_type = modest_account_mgr_get_store_protocol (modest_runtime_get_account_mgr (),
								    account_name);
		proto = (ModestAccountProtocol *)
			modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (),
								       proto_type);

		/* Create and show the dialog */
		if (proto && MODEST_IS_ACCOUNT_PROTOCOL (proto)) {
			ModestAccountSettingsDialog *dialog =
				modest_account_protocol_get_account_settings_dialog (proto, account_name);

			if (dialog) {
				modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
							     (GtkWindow *) dialog,
							     (GtkWindow *) self);
				gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), FALSE);
				gtk_widget_show (GTK_WIDGET (dialog));
			}
		}
	}
}

static void
setup_menu (ModestFolderWindow *self)
{
	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));

	/* folders actions*/
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_new_folder"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_on_new_folder),
				   NULL);
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_inbox_sendandreceive"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_on_send_receive),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_send_receive));
#ifdef MODEST_TOOLKIT_HILDON2
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_rename_folder"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (set_rename_edit_mode),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_rename_folder));
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_move_folder"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (set_moveto_edit_mode),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_folder_window_move_to));
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_delete_folder"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (set_delete_edit_mode),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_folder_window_delete));
#endif

	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_outbox_cancelsend"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_cancel_send),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_cancel_sending_all));

	modest_window_add_to_menu (MODEST_WINDOW (self),
				   dngettext(GETTEXT_PACKAGE,
					     "mcen_me_edit_account",
					     "mcen_me_edit_accounts",
					     1),
				   NULL, MODEST_WINDOW_MENU_CALLBACK (edit_account),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_edit_accounts));
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

	if (!folder || !TNY_IS_FOLDER (folder))
		return;

	/* We cannot open noselect folders (fake ones) */
	if (tny_folder_get_caps (folder) & TNY_FOLDER_CAPS_NOSELECT)
		return;

	headerwin = modest_header_window_new (folder, 
					      modest_window_get_active_account (MODEST_WINDOW (self)),
					      modest_window_get_active_mailbox (MODEST_WINDOW (self)));

	if (modest_window_mgr_register_window (modest_runtime_get_window_mgr (),
					       MODEST_WINDOW (headerwin),
					       MODEST_WINDOW (self))) {
		gtk_widget_show (GTK_WIDGET (headerwin));
	} else {
		gtk_widget_destroy (GTK_WIDGET (headerwin));
		headerwin = NULL;
	}
}

#ifdef MODEST_TOOLKIT_HILDON2
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
#endif

static void
modest_folder_window_pack_toolbar (ModestWindow *self,
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

#ifdef MODEST_TOOLKIT_HILDON2
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
#endif

static gboolean 
on_map_event (GtkWidget *widget,
	      GdkEvent *event,
	      gpointer userdata)
{
	ModestFolderWindow *self = (ModestFolderWindow *) userdata;
	ModestFolderWindowPrivate *priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	if (priv->progress_hint) {
		modest_window_show_progress (MODEST_WINDOW (self), TRUE);
	}

	return FALSE;
}

static gboolean
has_active_operations (ModestFolderWindow *self)
{
	GSList *operations = NULL, *node;
	ModestMailOperationQueue *queue;
	gboolean has_active = FALSE;

	queue = modest_runtime_get_mail_operation_queue ();
	operations = modest_mail_operation_queue_get_by_source (queue, G_OBJECT (self));

	for (node = operations; node != NULL; node = g_slist_next (node)) {
		if (!modest_mail_operation_is_finished (MODEST_MAIL_OPERATION (node->data))) {
			has_active = TRUE;
			break;
		}
	}

	if (operations) {
		g_slist_foreach (operations, (GFunc) g_object_unref, NULL);
		g_slist_free (operations);
	}

	return has_active;
}

static void
update_progress_hint (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	if (has_active_operations (self)) {
		priv->progress_hint = TRUE;
	} else {
		priv->progress_hint = FALSE;
	}

	if (!priv->progress_hint && priv->current_store_account) {
		priv->progress_hint = modest_window_mgr_has_progress_operation_on_account (modest_runtime_get_window_mgr (),
											   priv->current_store_account);
	}

	if (!priv->progress_hint) {
		priv->progress_hint = modest_folder_view_get_activity (MODEST_FOLDER_VIEW (priv->folder_view));
	}

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));

	if (GTK_WIDGET_VISIBLE (self)) {
		modest_window_show_progress (MODEST_WINDOW (self), priv->progress_hint ? 1:0);
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

static void 
on_mail_operation_started (ModestMailOperation *mail_op,
			   gpointer user_data)
{
	ModestFolderWindow *self;
	ModestMailOperationTypeOperation op_type;
	GObject *source = NULL;

	self = MODEST_FOLDER_WINDOW (user_data);
	op_type = modest_mail_operation_get_type_operation (mail_op);
	source = modest_mail_operation_get_source(mail_op);
	if (G_OBJECT (self) == source) {
		update_progress_hint (self);
	}
	g_object_unref (source);
}

static void 
on_mail_operation_finished (ModestMailOperation *mail_op,
			    gpointer user_data)
{
	ModestFolderWindow *self;

	self = MODEST_FOLDER_WINDOW (user_data);

	/* Don't disable the progress hint if there are more pending
	   operations from this window */
	update_progress_hint (self);

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	/* If this operations was created by another window, do nothing */
	if (!modest_mail_operation_is_mine (mail_op, G_OBJECT(self))) 
		return;

	if (type == MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED) {
		priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
							       G_OBJECT (mail_op),
							       "operation-started",
							       G_CALLBACK (on_mail_operation_started),
							       self);
		priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
							       G_OBJECT (mail_op),
							       "operation-finished",
							       G_CALLBACK (on_mail_operation_finished),
							       self);
	} else if (type == MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED) {
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers,
								  G_OBJECT (mail_op),
								  "operation-started");
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers,
								  G_OBJECT (mail_op),
								  "operation-finished");
	}
}

static void
on_activity_changed (ModestFolderView *view,
		     gboolean activity,
		     ModestFolderWindow *folder_window)
{
	g_return_if_fail (MODEST_IS_FOLDER_WINDOW (folder_window));

	update_progress_hint (folder_window);
}


static void
update_window_title (ModestFolderWindow *self,
		     TnyAccount *account)
{
	if (account) {
		const gchar *name;
		const gchar *mailbox;
		gchar *title = NULL;
		ModestFolderWindowPrivate *priv;

		priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

		mailbox = modest_folder_view_get_mailbox (MODEST_FOLDER_VIEW (priv->folder_view));
		if (mailbox) {
			title = g_strdup (mailbox);
		} else {
			name = modest_tny_account_get_parent_modest_account_name_for_server_account (account);
			title = modest_account_mgr_get_display_name (modest_runtime_get_account_mgr(),
								     name);
		}
		if (title) {
			modest_window_set_title (MODEST_WINDOW (self), title);
			g_free (title);
		} else {
			modest_window_set_title (MODEST_WINDOW (self), _("mcen_ap_name"));
		}
	} else {
		modest_window_set_title (MODEST_WINDOW (self), _("mcen_ap_name"));
	}
}

static void
on_visible_account_changed (ModestFolderView *folder_view,
			    const gchar *account_id,
			    gpointer user_data)
{
	TnyAccount *account;

	if (!account_id)
		return;

	account = modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store(),
							       MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
							       account_id);

	/* Update window title */
	if (account) {
		update_window_title (MODEST_FOLDER_WINDOW (user_data), account);
		g_object_unref (account);
	}
}

static void
on_account_changed (TnyAccountStore *account_store,
		    TnyAccount *account,
		    gpointer user_data)
{
	const gchar *acc_id, *visible;
	ModestFolderWindowPrivate *priv;

	if (!TNY_IS_STORE_ACCOUNT (account))
		return;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (user_data);

	acc_id = tny_account_get_id (account);
	visible = modest_folder_view_get_account_id_of_visible_server_account (MODEST_FOLDER_VIEW (priv->folder_view));

	/* Update title if the visible account is the one that have just changed */
	if (acc_id && visible && !g_utf8_collate (acc_id, visible))
		update_window_title (MODEST_FOLDER_WINDOW (user_data), account);
}


static gboolean
on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	ModestFolderWindowPrivate *priv;
	ModestScrollable *scrollable;

	if (event->type == GDK_KEY_RELEASE)
		return FALSE;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (user_data);

	scrollable = MODEST_SCROLLABLE (gtk_widget_get_parent (priv->folder_view));

	switch (event->keyval) {

	case GDK_Up:
		modest_scrollable_scroll (scrollable, 0, -1);
		break;

	case GDK_Down:
		modest_scrollable_scroll (scrollable, 0, 1);
		break;
	}

	return FALSE;
}

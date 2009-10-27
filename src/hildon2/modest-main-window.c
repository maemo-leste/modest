/* Copyright (c) 2006, 2008 Nokia Corporation
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
#include <gtk/gtktreeviewcolumn.h>
#include <tny-account-store-view.h>
#include <tny-simple-list.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-error.h>
#include "modest-hildon-includes.h"
#include "modest-defs.h"
#include <string.h>
#include "widgets/modest-header-view-priv.h"
#include "widgets/modest-main-window.h"
#include "widgets/modest-msg-edit-window.h"
#include "widgets/modest-account-view-window.h"
#include "modest-runtime.h"
#include "modest-account-mgr-helpers.h"
#include "modest-platform.h"
#include "modest-widget-memory.h"
#include "modest-window-priv.h"
#include "modest-main-window-ui.h"
#include "modest-main-window-ui-dimming.h"
#include "modest-account-mgr.h"
#include "modest-tny-account.h"
#include "modest-tny-folder.h"
#include "modest-conf.h"
#include <modest-utils.h>
#include <modest-maemo-utils.h>
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg.h"
#include "modest-mail-operation.h"
#include "modest-icon-names.h"
#include "modest-progress-bar.h"
#include "modest-text-utils.h"
#include "modest-ui-dimming-manager.h"
#include "modest-text-utils.h"
#include "modest-signal-mgr.h"
#include <tny-gtk-folder-store-tree-model.h>
#include <modest-accounts-window.h>

#define MODEST_MAIN_WINDOW_ACTION_GROUP_ADDITIONS "ModestMainWindowActionAdditions"

#define XALIGN 0.5
#define YALIGN 0.0
#define XSPACE 1
#define YSPACE 0

/* 'private'/'protected' functions */
static void modest_main_window_class_init  (ModestMainWindowClass *klass);
static void modest_main_window_init        (ModestMainWindow *obj);
static void modest_main_window_finalize    (GObject *obj);

static void connect_signals (ModestMainWindow *self);

static void modest_main_window_disconnect_signals (ModestWindow *self);

static void restore_settings (ModestMainWindow *self, 
			      gboolean do_folder_view_too);

static void save_state (ModestWindow *self);

static void update_menus (ModestMainWindow* self);

static void modest_main_window_show_toolbar   (ModestWindow *window,
					       gboolean show_toolbar);

static void cancel_progressbar (GtkToolButton *toolbutton,
				ModestMainWindow *self);

static void on_queue_changed   (ModestMailOperationQueue *queue,
				ModestMailOperation *mail_op,
				ModestMailOperationQueueNotification type,
				ModestMainWindow *self);

static gboolean on_zoom_minus_plus_not_implemented (ModestWindow *window);

static void on_account_inserted (TnyAccountStore *accoust_store,
				 TnyAccount *account,
				 gpointer user_data);

static void on_account_removed  (TnyAccountStore *accoust_store,
				 TnyAccount *account,
				 gpointer user_data);

static void on_account_changed  (TnyAccountStore *account_store,
				 TnyAccount *account,
				 gpointer user_data);

static void on_default_account_changed (ModestAccountMgr* mgr,
					gpointer user_data);

static gboolean on_inner_widgets_key_pressed  (GtkWidget *widget,
					       GdkEventKey *event,
					       gpointer user_data);

static void on_configuration_key_changed      (ModestConf* conf, 
					       const gchar *key, 
					       ModestConfEvent event,
					       ModestConfNotificationId id,
					       ModestMainWindow *self);

static void set_toolbar_mode                  (ModestMainWindow *self, 
					       ModestToolBarModes mode);

static gboolean set_toolbar_transfer_mode     (ModestMainWindow *self); 

static void on_show_account_action_toggled      (GtkToggleAction *action,
						 gpointer user_data);

static void on_refresh_account_action_activated   (GtkAction *action,
						   gpointer user_data);

static void on_send_receive_csm_activated         (GtkMenuItem *item,
						   gpointer user_data);

static void on_folder_view_row_activated (GtkTreeView *tree_view,
					  GtkTreePath *tree_path,
					  GtkTreeViewColumn *column,
					  gpointer userdata);
static void on_msg_count_changed (ModestHeaderView *header_view,
				  TnyFolder *folder,
				  TnyFolderChange *change,
				  ModestMainWindow *main_window);

static void modest_main_window_cleanup_queue_error_signals (ModestMainWindow *self);


static GtkWidget * create_empty_view (void);

static gboolean  on_folder_view_focus_in (GtkWidget *widget,
					  GdkEventFocus *event,
					  gpointer userdata);

static gboolean  on_header_view_focus_in (GtkWidget *widget,
					  GdkEventFocus *event,
					  gpointer userdata);

static void      on_folder_selection_changed (ModestFolderView *folder_view,
					      TnyFolderStore *folder_store, 
					      gboolean selected,
					      ModestMainWindow *main_window);

static void set_at_least_one_account_visible(ModestMainWindow *self);

static void on_updating_msg_list (ModestHeaderView *header_view,
				  gboolean starting,
				  gpointer user_data);

static gboolean show_opening_banner (gpointer user_data);

static void on_window_destroy (GtkObject *widget,
			       gpointer userdata);

static void on_window_hide (GObject    *gobject,
			    GParamSpec *arg1,
			    gpointer    user_data);

typedef struct _ModestMainWindowPrivate ModestMainWindowPrivate;
struct _ModestMainWindowPrivate {
	GtkWidget *main_vbox;
	GtkWidget *contents_widget;
	GtkWidget *empty_view;

	/* Progress observers */
	GtkWidget   *progress_bar;
	GSList      *progress_widgets;

	/* Tollbar items */
	GtkWidget   *progress_toolitem;
	GtkWidget   *cancel_toolitem;
	GtkWidget   *sort_toolitem;
	GtkWidget   *refresh_toolitem;
	ModestToolBarModes current_toolbar_mode;

	/* Merge ids used to add/remove accounts to the Accounts Menu*/
	GByteArray *merge_ids;
	GtkActionGroup *view_additions_group;

	/* On-demand widgets */
	GtkWidget *accounts_popup;
	GtkWidget *details_widget;

	/* Optimized view enabled */
	gboolean optimized_view;

	/* Optimized view enabled */
	gboolean send_receive_in_progress;

	ModestHeaderView *header_view;
	ModestFolderView *folder_view;

	ModestMainWindowStyle style;
	ModestMainWindowContentsStyle contents_style;
	gboolean wait_for_settings;

	guint progress_bar_timeout;

	/* Signal handler UIDs */
	GList *queue_err_signals;
	GSList *sighandlers;

	/* "Updating" banner for header view */
	GtkWidget *updating_banner;
	guint updating_banner_timeout;

	/* "Opening" banner for header view */
	GtkWidget *opening_banner;
	guint opening_banner_timeout;

	/* Display state */
	osso_display_state_t display_state;
};
#define MODEST_MAIN_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_MAIN_WINDOW, \
                                                ModestMainWindowPrivate))

typedef struct _GetMsgAsyncHelper {
	ModestMainWindowPrivate *main_window_private;
	guint action;
	ModestTnyMsgReplyType reply_type;
	ModestTnyMsgForwardType forward_type;
	gchar *from;
	TnyIterator *iter;
} GetMsgAsyncHelper;


/* globals */
static GtkWindowClass *parent_class = NULL;


/* Private actions */
/* This is the context sensitive menu: */
static const GtkActionEntry modest_folder_view_action_entries [] = {

	/* Folder View CSM actions */
	{ "FolderViewCSMNewFolder", NULL, N_("ckdg_fi_new_folder_name"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "FolderViewCSMRenameFolder", NULL, N_("mcen_me_rename_folder"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_rename_folder) },
	{ "FolderViewCSMPasteMsgs", NULL, N_("mcen_me_inbox_paste"), NULL, NULL,  G_CALLBACK (modest_ui_actions_on_paste)},
	{ "FolderViewCSMDeleteFolder", NULL, N_("mcen_me_delete_messages"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_delete_folder) },
};

static const GtkActionEntry modest_header_view_action_entries [] = {

	/* Header View CSM actions */
	{ "HeaderViewCSMOpen",          NULL,  N_("mcen_me_inbox_open"),        NULL,       NULL, G_CALLBACK (modest_ui_actions_on_open) },
	{ "HeaderViewCSMReply",         NULL,  N_("mcen_me_inbox_reply"),       NULL,      NULL, G_CALLBACK (modest_ui_actions_on_reply) },
	{ "HeaderViewCSMReplyAll",      NULL,  N_("mcen_me_inbox_replytoall"),  NULL,      NULL, G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "HeaderViewCSMForward",       NULL,  N_("mcen_me_inbox_forward"),     NULL,      NULL, G_CALLBACK (modest_ui_actions_on_forward) },
	{ "HeaderViewCSMCut",           NULL,  N_("mcen_me_inbox_cut"),         "<CTRL>X", NULL, G_CALLBACK (modest_ui_actions_on_cut) },
	{ "HeaderViewCSMCopy",          NULL,  N_("mcen_me_inbox_copy"),        "<CTRL>C", NULL, G_CALLBACK (modest_ui_actions_on_copy) },
	{ "HeaderViewCSMPaste",         NULL,  N_("mcen_me_inbox_paste"),       "<CTRL>V", NULL, G_CALLBACK (modest_ui_actions_on_paste) },
	{ "HeaderViewCSMDelete",        NULL,  N_("mcen_me_delete_messages"),      NULL,      NULL, G_CALLBACK (modest_ui_actions_on_delete_message) },
	{ "HeaderViewCSMCancelSending", NULL,  N_("mcen_me_outbox_cancelsend"), NULL,      NULL, G_CALLBACK (modest_ui_actions_cancel_send) },
};

static const GtkToggleActionEntry modest_main_window_toggle_action_entries [] = {
	{ "ToggleFolders",     MODEST_STOCK_SPLIT_VIEW, N_("mcen_me_inbox_hidefolders"), "<CTRL>t", NULL, G_CALLBACK (modest_ui_actions_toggle_folders_view), TRUE },
};

/************************************************************************/

GType
modest_main_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMainWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_main_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMainWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_main_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
		                                  "ModestMainWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_main_window_class_init (ModestMainWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_main_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMainWindowPrivate));
	
	modest_window_class->show_toolbar_func = modest_main_window_show_toolbar;
	modest_window_class->save_state_func = save_state;
	modest_window_class->zoom_minus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->zoom_plus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->disconnect_signals_func = modest_main_window_disconnect_signals;
}

static void
modest_main_window_init (ModestMainWindow *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	priv->queue_err_signals = NULL;
	priv->main_vbox    = NULL;
	priv->header_view  = NULL;
	priv->folder_view  = NULL;
	priv->contents_widget  = NULL;
	priv->accounts_popup  = NULL;
	priv->details_widget  = NULL;
	priv->empty_view  = NULL;
	priv->progress_widgets  = NULL;
	priv->progress_bar = NULL;
	priv->current_toolbar_mode = TOOLBAR_MODE_NORMAL;
	priv->style  = MODEST_MAIN_WINDOW_STYLE_SPLIT;
	priv->wait_for_settings = TRUE;
	priv->contents_style  = -1; /* invalid contents style. We need this to select it for the first time */
	priv->merge_ids = NULL;
	priv->optimized_view  = FALSE;
	priv->send_receive_in_progress  = FALSE;
	priv->progress_bar_timeout = 0;
	priv->sighandlers = NULL;
	priv->updating_banner = NULL;
	priv->updating_banner_timeout = 0;
	priv->opening_banner = NULL;
	priv->opening_banner_timeout = 0;
	priv->display_state = OSSO_DISPLAY_ON;
	
	modest_window_mgr_register_help_id (modest_runtime_get_window_mgr(),
					    GTK_WINDOW(obj),
					    "applications_email_mainview");
}

static void
modest_main_window_finalize (GObject *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_main_window_disconnect_signals (MODEST_WINDOW (obj));	
	modest_main_window_cleanup_queue_error_signals ((ModestMainWindow *) obj);

	if (priv->empty_view) {
		g_object_unref (priv->empty_view);
		priv->empty_view = NULL;
	}
	
	if (priv->header_view) {
		g_object_unref (priv->header_view);
		priv->header_view = NULL;
	}
	
	g_slist_free (priv->progress_widgets);

	g_byte_array_free (priv->merge_ids, TRUE);

	if (priv->progress_bar_timeout > 0) {
		g_source_remove (priv->progress_bar_timeout);
		priv->progress_bar_timeout = 0;
	}

	if (priv->updating_banner_timeout > 0) {
		g_source_remove (priv->updating_banner_timeout);
		priv->updating_banner_timeout = 0;
	}

	if (priv->updating_banner) {
		gtk_widget_destroy (priv->updating_banner);
		priv->updating_banner = NULL;
	}

	if (priv->opening_banner_timeout > 0) {
		g_source_remove (priv->opening_banner_timeout);
		priv->opening_banner_timeout = 0;
	}

	if (priv->opening_banner) {
		gtk_widget_destroy (priv->opening_banner);
		priv->opening_banner = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_main_window_get_child_widget (ModestMainWindow *self,
				     ModestMainWindowWidgetType widget_type)
{
	ModestMainWindowPrivate *priv;
	GtkWidget *widget;
	
	g_return_val_if_fail (self && MODEST_IS_MAIN_WINDOW(self), NULL);
	g_return_val_if_fail (widget_type >= 0 && widget_type < MODEST_MAIN_WINDOW_WIDGET_TYPE_NUM,
			      NULL);
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	switch (widget_type) {
	case MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW:
		widget = (GtkWidget*)priv->header_view; break;
	case MODEST_MAIN_WINDOW_WIDGET_TYPE_FOLDER_VIEW:
		widget = (GtkWidget*)priv->folder_view; break;
	default:
		return NULL;
	}

	/* Note that the window could have been destroyed, and so
	   their children, but still have some references */
	return (widget && GTK_IS_WIDGET(widget)) ? GTK_WIDGET(widget) : NULL;
}


static void
restore_settings (ModestMainWindow *self, gboolean do_folder_view_too)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	conf = modest_runtime_get_conf ();

	modest_widget_memory_restore (conf, G_OBJECT(self),
				      MODEST_CONF_MAIN_WINDOW_KEY);

	modest_widget_memory_restore (conf, G_OBJECT(priv->header_view),
				      MODEST_CONF_HEADER_VIEW_KEY);

	if (do_folder_view_too)
		modest_widget_memory_restore (conf, G_OBJECT(priv->folder_view),
				      MODEST_CONF_FOLDER_VIEW_KEY);

	gtk_widget_show (GTK_WIDGET (self));
}


static void
save_state (ModestWindow *window)
{
	ModestConf *conf;
	ModestMainWindow* self = MODEST_MAIN_WINDOW(window);
	ModestMainWindowPrivate *priv;
		
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_runtime_get_conf ();
	
	modest_widget_memory_save (conf,G_OBJECT(self), 
				   MODEST_CONF_MAIN_WINDOW_KEY);
	modest_widget_memory_save (conf, G_OBJECT(priv->folder_view), 
				   MODEST_CONF_FOLDER_VIEW_KEY);
}

static gint
compare_display_names (ModestAccountSettings *a,
		       ModestAccountSettings *b)
{
	return g_utf8_collate (modest_account_settings_get_display_name (a),
			       modest_account_settings_get_display_name (b));

}

/* We use this function to prevent the send&receive CSM to be shown
   when there are less than two account */
static gboolean
tap_and_hold_query_cb (GtkWidget *widget, GdkEvent *event)
{
	return TRUE;
}

static void
update_menus (ModestMainWindow* self)
{
	GSList *account_names, *iter, *accounts;
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	ModestAccountMgr *mgr;
	gint i, num_accounts;
	GList *groups;
	gchar *default_account;
	const gchar *active_account_name;
	GtkWidget *send_receive_button, *item;
	GtkAction *send_receive_all = NULL;
	GSList *radio_group;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (self);

	/* Get enabled account IDs */
	mgr = modest_runtime_get_account_mgr ();
	account_names = modest_account_mgr_account_names (mgr, TRUE);
	iter = account_names;
	accounts = NULL;

	while (iter) {
		ModestAccountSettings *settings = 
			modest_account_mgr_load_account_settings (mgr, (gchar*) iter->data);
		accounts = g_slist_prepend (accounts, settings);

		iter = iter->next;
	}
	modest_account_mgr_free_account_names (account_names);
	account_names = NULL;

	/* Order the list of accounts by its display name */
	accounts = g_slist_sort (accounts, (GCompareFunc) compare_display_names);
	num_accounts = g_slist_length (accounts);

	send_receive_all = gtk_ui_manager_get_action (parent_priv->ui_manager, 
						      "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveAllMenu");
	gtk_action_set_visible (send_receive_all, num_accounts > 0);

	/* Delete old send&receive popup items. We can not just do a
	   menu_detach because it does not work well with
	   tap_and_hold */
	if (priv->accounts_popup)
		gtk_container_foreach (GTK_CONTAINER (priv->accounts_popup), 
				       (GtkCallback) gtk_widget_destroy, NULL);

	/* Delete old entries in the View menu. Do not free groups, it
	   belongs to Gtk+ */
	groups = gtk_ui_manager_get_action_groups (parent_priv->ui_manager);
	while (groups) {
		if (!strcmp (MODEST_MAIN_WINDOW_ACTION_GROUP_ADDITIONS,
			     gtk_action_group_get_name (GTK_ACTION_GROUP (groups->data)))) {
			gtk_ui_manager_remove_action_group (parent_priv->ui_manager, 
							    GTK_ACTION_GROUP (groups->data));
			groups = NULL;
			/* Remove uis */
			if (priv->merge_ids) {
				for (i = 0; i < priv->merge_ids->len; i++)
					gtk_ui_manager_remove_ui (parent_priv->ui_manager, priv->merge_ids->data[i]);
				g_byte_array_free (priv->merge_ids, TRUE);
			}
			/* We need to call this in order to ensure
			   that the new actions are added in the right
			   order (alphabetical) */
			gtk_ui_manager_ensure_update (parent_priv->ui_manager);
		} else 
			groups = g_list_next (groups);
	}
	priv->merge_ids = g_byte_array_sized_new (num_accounts);

	/* Get send receive button */
	send_receive_button = gtk_ui_manager_get_widget (parent_priv->ui_manager,
							  "/ToolBar/ToolbarSendReceive");

	/* Create the menu */
	if (num_accounts > 1) {
		if (!priv->accounts_popup)
			priv->accounts_popup = gtk_menu_new ();
		item = gtk_menu_item_new_with_label (_("mcen_me_toolbar_sendreceive_all"));
		gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));
		g_signal_connect (G_OBJECT (item), 
				  "activate", 
				  G_CALLBACK (on_send_receive_csm_activated),
				  NULL);
		item = gtk_separator_menu_item_new ();
		gtk_menu_shell_prepend (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));
	}

	/* Create a new action group */
	default_account = modest_account_mgr_get_default_account (mgr);
	active_account_name = modest_window_get_active_account (MODEST_WINDOW (self));

	if (!active_account_name) 
		modest_window_set_active_account (MODEST_WINDOW (self), default_account);

	priv->view_additions_group = gtk_action_group_new (MODEST_MAIN_WINDOW_ACTION_GROUP_ADDITIONS);
	radio_group = NULL;
	for (i = 0; i < num_accounts; i++) {
		gchar *display_name = NULL;	
		const gchar *account_name;
		ModestAccountSettings *settings = (ModestAccountSettings *) g_slist_nth_data (accounts, i);

		if (!settings) {
			g_warning ("%s: BUG: account_data == NULL", __FUNCTION__);
			continue;
		}
		account_name = modest_account_settings_get_account_name (settings);

		if (default_account && account_name && 
		    !(strcmp (default_account, account_name) == 0)) {
			display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_default"), 
							modest_account_settings_get_display_name (settings));
		} else {
			display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_mailbox_n"), 
							modest_account_settings_get_display_name (settings));
		}



		/* Create action and add it to the action group. The
		   action name must be the account name, this way we
		   could know in the handlers the account to show */
		if (settings && account_name) {
			gchar* item_name, *refresh_action_name;
			guint8 merge_id = 0;
			GtkAction *view_account_action, *refresh_account_action;
			gchar *escaped_display_name;

			escaped_display_name = modest_text_utils_escape_mnemonics (display_name);

			view_account_action = GTK_ACTION (gtk_radio_action_new (account_name,
										escaped_display_name, NULL, NULL, 0));
			g_free (escaped_display_name);
			gtk_action_group_add_action (priv->view_additions_group, view_account_action);
			gtk_radio_action_set_group (GTK_RADIO_ACTION (view_account_action), radio_group);
			radio_group = gtk_radio_action_get_group (GTK_RADIO_ACTION (view_account_action));

			if (active_account_name) {
				if (active_account_name && account_name && 
				    (strcmp (active_account_name, account_name) == 0)) {
					gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (view_account_action), TRUE);
				}
			}

			/* Add ui from account data. We allow 2^9-1 account
			   changes in a single execution because we're
			   downcasting the guint to a guint8 in order to use a
			   GByteArray. It should be enough :-) */
			item_name = g_strconcat (account_name, "Menu", NULL);
			merge_id = (guint8) gtk_ui_manager_new_merge_id (parent_priv->ui_manager);
			priv->merge_ids = g_byte_array_append (priv->merge_ids, &merge_id, 1);
			gtk_ui_manager_add_ui (parent_priv->ui_manager,
					       merge_id,
					       "/MenuBar/AccountsMenu/AccountsMenuAdditions",
					       item_name,
					       account_name,
					       GTK_UI_MANAGER_MENUITEM,
					       FALSE);

			/* Connect the action signal "activate" */
			g_signal_connect_after (G_OBJECT (view_account_action),
						"toggled",
						G_CALLBACK (on_show_account_action_toggled),
						self);

			/* Create the items for the Tools->Send&Receive submenu */
			refresh_action_name = g_strconcat ("SendReceive", account_name, NULL);
			refresh_account_action = gtk_action_new ((const gchar*) refresh_action_name, 
								 display_name, NULL, NULL);
			gtk_action_group_add_action (priv->view_additions_group, refresh_account_action);

			merge_id = (guint8) gtk_ui_manager_new_merge_id (parent_priv->ui_manager);
			priv->merge_ids = g_byte_array_append (priv->merge_ids, &merge_id, 1);
			gtk_ui_manager_add_ui (parent_priv->ui_manager, 
					       merge_id,
					       "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsMenuAdditions",
					       item_name,
					       refresh_action_name,
					       GTK_UI_MANAGER_MENUITEM,
					       FALSE);
			g_free (refresh_action_name);

			g_signal_connect_data (G_OBJECT (refresh_account_action), 
					       "activate", 
					       G_CALLBACK (on_refresh_account_action_activated), 
					       g_strdup (account_name),
					       (GClosureNotify) g_free,
					       0);

			/* Create item and add it to the send&receive
			   CSM. If there is only one account then
			   it'll be no menu */
			if (num_accounts > 1) {
				GtkWidget *label = gtk_label_new(NULL);
				gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
				if (default_account && (strcmp(account_name, default_account) == 0)) {
					gchar *escaped = g_markup_printf_escaped ("<b>%s</b>", display_name);
					gtk_label_set_markup (GTK_LABEL (label), escaped);
					g_free (escaped);
				} else {
					gtk_label_set_text (GTK_LABEL (label), display_name);
				}

				item = gtk_menu_item_new ();
				gtk_container_add (GTK_CONTAINER (item), label);

				gtk_menu_shell_prepend (GTK_MENU_SHELL (priv->accounts_popup), 
							GTK_WIDGET (item));
				g_signal_connect_data (G_OBJECT (item), 
						       "activate", 
						       G_CALLBACK (on_send_receive_csm_activated),
						       g_strdup (account_name),
						       (GClosureNotify) g_free,
						       0);
			}
			g_free (item_name);
		}

		/* Frees */
		g_free (display_name);
	}

	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, priv->view_additions_group, 1);

	/* We cannot do this in the loop above because this relies on the action
	 * group being inserted. This makes the default account appear in bold.
	 * I agree it is a rather ugly way, but I don't see another possibility. armin. */
	for (i = 0; i < num_accounts; i++) {
		gchar *item_name, *path;
		GtkWidget *item;
		ModestAccountSettings *settings;
		const gchar *account_name;
		gboolean is_default;

		settings = (ModestAccountSettings *) g_slist_nth_data (accounts, i);
		account_name = modest_account_settings_get_account_name (settings);
		is_default = (account_name && default_account && !strcmp (account_name, default_account));

		/* Get the item of the view menu */
		item_name = g_strconcat (account_name, "Menu", NULL);
		path = g_strconcat ("/MenuBar/AccountsMenu/AccountsMenuAdditions/", item_name, NULL);
		item = gtk_ui_manager_get_widget (parent_priv->ui_manager, path);
		g_free(path);

		if (item) {
			GtkWidget *child = gtk_bin_get_child (GTK_BIN (item));
			if (GTK_IS_LABEL (child)) {
				const gchar *cur_name = gtk_label_get_text (GTK_LABEL (child));
				if (is_default) {
					gchar *bold_name = g_markup_printf_escaped("<b>%s</b>", cur_name);
					gtk_label_set_markup (GTK_LABEL (child), bold_name);
					g_free (bold_name);
				}
			}
		}

		/* Get the item of the tools menu */
		path = g_strconcat("/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsMenuAdditions/", item_name, NULL);
		item = gtk_ui_manager_get_widget (parent_priv->ui_manager, path);
		g_free (path);

		if (item) {
			GtkWidget *child = gtk_bin_get_child (GTK_BIN (item));
			if (GTK_IS_LABEL (child)) {
				const gchar *cur_name = gtk_label_get_text (GTK_LABEL (child));
				if (is_default) {
					gchar *bold_name = g_markup_printf_escaped("<b>%s</b>", cur_name);
					gtk_label_set_markup (GTK_LABEL (child), bold_name);
					g_free (bold_name);
				}
				gtk_label_set_ellipsize (GTK_LABEL (child),  PANGO_ELLIPSIZE_END);
			}
		}

		g_free(item_name);
		g_object_unref (settings);
	}

	if (num_accounts > 1) {
		/* Disconnect the tap-and-hold-query if it's connected */
		if (modest_signal_mgr_is_connected (priv->sighandlers, 
						    G_OBJECT (send_receive_button),
						    "tap-and-hold-query"))
			priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers, 
									  G_OBJECT (send_receive_button),
									  "tap-and-hold-query");

		/* Mandatory in order to view the menu contents */
		gtk_widget_show_all (priv->accounts_popup);

		/* Setup tap_and_hold just if was not done before*/
		if (!gtk_menu_get_attach_widget (GTK_MENU (priv->accounts_popup)))
			gtk_widget_tap_and_hold_setup (send_receive_button, priv->accounts_popup, NULL, 0);
	} else {
		/* Connect the tap-and-hold-query in order not to show the CSM */
		if (!modest_signal_mgr_is_connected (priv->sighandlers, 
						     G_OBJECT (send_receive_button),
						     "tap-and-hold-query"))
			priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
								       G_OBJECT (send_receive_button),
								       "tap-and-hold-query",
								       G_CALLBACK (tap_and_hold_query_cb), 
								       NULL);
	}

	/* Frees */
	g_slist_free (accounts);
	g_free (default_account);


	/* Make sure that at least one account is viewed if there are any 
	 * accounts, for instance when adding the first account: */
	set_at_least_one_account_visible (self);
}

static void
wrap_in_scrolled_window (GtkWidget *win, GtkWidget *widget)
{
	if (!gtk_widget_set_scroll_adjustments (widget, NULL, NULL))
		gtk_scrolled_window_add_with_viewport
			(GTK_SCROLLED_WINDOW(win), widget);
	else
		gtk_container_add (GTK_CONTAINER(win),
				   widget);
}


typedef struct {
	TnySendQueue *queue;
	guint signal;
} QueueErrorSignal;

static void
modest_main_window_cleanup_queue_error_signals (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);

	GList *oerrsignals = priv->queue_err_signals;
	while (oerrsignals) {
		QueueErrorSignal *esignal = (QueueErrorSignal *) oerrsignals->data;
		g_signal_handler_disconnect (esignal->queue, esignal->signal);
		g_slice_free (QueueErrorSignal, esignal);
		oerrsignals = g_list_next (oerrsignals);
	}
	g_list_free (priv->queue_err_signals);
	priv->queue_err_signals = NULL;
}


static void
modest_main_window_disconnect_signals (ModestWindow *self)
{	
	ModestMainWindowPrivate *priv;	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;	
}

static void
connect_signals (ModestMainWindow *self)
{	
	ModestWindowPrivate *parent_priv;
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	/* folder view */
	
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT(priv->folder_view), "key-press-event",
					   G_CALLBACK(on_inner_widgets_key_pressed), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers, G_OBJECT(priv->folder_view), 
					   "folder_selection_changed",
					   G_CALLBACK (on_folder_selection_changed), 
					   self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->folder_view), 
					   "folder-display-name-changed",
					   G_CALLBACK (modest_ui_actions_on_folder_display_name_changed), 
					   self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (priv->folder_view), 
					   "focus-in-event", 
					   G_CALLBACK (on_folder_view_focus_in), 
					   self);

	/* folder view row activated */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, G_OBJECT(priv->folder_view), "row-activated",
						       G_CALLBACK(on_folder_view_row_activated),
						       self);

	/* header view */
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "header_selected",
					   G_CALLBACK(modest_ui_actions_on_header_selected), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "header_activated",
					   G_CALLBACK(modest_ui_actions_on_header_activated), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "item_not_found",
					   G_CALLBACK(modest_ui_actions_on_item_not_found), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "key-press-event",
					   G_CALLBACK(on_inner_widgets_key_pressed), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "msg_count_changed",
					   G_CALLBACK(on_msg_count_changed), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (priv->header_view), "focus-in-event",
					   G_CALLBACK (on_header_view_focus_in), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->header_view), 
					   "updating-msg-list",
					   G_CALLBACK (on_updating_msg_list), 
					   self);
	
	/* window */
	/* we don't register this in sighandlers, as it should be run after disconnecting all signals,
	 * in destroy stage */
	g_signal_connect (G_OBJECT (self), "destroy", G_CALLBACK (on_window_destroy), NULL);

	g_signal_connect (G_OBJECT (self), "notify::visible", G_CALLBACK (on_window_hide), NULL);

	/* Mail Operation Queue */
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (modest_runtime_get_mail_operation_queue ()),
					   "queue-changed", 
					   G_CALLBACK (on_queue_changed), self);
	
	/* Track changes in the device name */
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT(modest_runtime_get_conf ()),
					   "key_changed", 
					   G_CALLBACK (on_configuration_key_changed), 
					   self);
	
	/* Track account changes. We need to refresh the toolbar */
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (modest_runtime_get_account_store ()),
					   "account_inserted", 
					   G_CALLBACK (on_account_inserted),
					   self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (modest_runtime_get_account_store ()),
					   "account_removed", 
					   G_CALLBACK (on_account_removed),
					   self);

	/* We need to refresh the send & receive menu to change the bold
	 * account when the default account changes. */
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (modest_runtime_get_account_mgr ()),
					   "default_account_changed", 
					   G_CALLBACK (on_default_account_changed),
					   self);

	/* Account store */
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (modest_runtime_get_account_store ()),
					   "account_changed", 
					   G_CALLBACK (on_account_changed),
					   self);
}

static void 
on_hildon_program_is_topmost_notify(GObject *self,
				    GParamSpec *propert_param, 
				    gpointer user_data)
{
	HildonProgram *app = HILDON_PROGRAM (self);
	
	/* Note that use of hildon_program_set_can_hibernate() 
	 * is generally referred to as "setting the killable flag", 
	 * though hibernation does not seem equal to death.
	 * murrayc */
		 
	if (hildon_program_get_is_topmost (app)) {
		/* Prevent hibernation when the progam comes to the foreground,
		 * because hibernation should only happen when the application 
		 * is in the background: */
		hildon_program_set_can_hibernate (app, FALSE);

		/* Remove new mail visual notifications */
		modest_platform_remove_new_mail_notifications (TRUE);
	} else {
		/* Allow hibernation if the program has gone to the background: */
		
		/* However, prevent hibernation while the settings are being changed: */
		const gboolean hibernation_prevented = 
			modest_window_mgr_get_hibernation_is_prevented (
									modest_runtime_get_window_mgr ()); 
    	
		if (hibernation_prevented)
			hildon_program_set_can_hibernate (app, FALSE);
		else {
			/* Allow hibernation, after saving the state: */
			hildon_program_set_can_hibernate (app, TRUE);
		}
	}	
}

typedef struct
{
	gulong handler_id;
} ShowHelper;

static void
modest_main_window_on_show (GtkWidget *self, gpointer user_data)
{
	ShowHelper *helper = (ShowHelper *) user_data;
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	priv->folder_view = MODEST_FOLDER_VIEW (modest_platform_create_folder_view (NULL));
	modest_main_window_set_contents_style (MODEST_MAIN_WINDOW (self), 
					       MODEST_MAIN_WINDOW_CONTENTS_STYLE_FOLDERS);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (priv->folder_view));

	gtk_widget_show (GTK_WIDGET (priv->folder_view));

	/* Connect signals */
	connect_signals (MODEST_MAIN_WINDOW (self));

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	/* Restore window & widget settings */
	priv->wait_for_settings = TRUE;
	restore_settings (MODEST_MAIN_WINDOW(self), TRUE);
	priv->wait_for_settings = FALSE;

	/* Check if accounts exist and show the account wizard if not */
	gboolean accounts_exist =
		modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), TRUE);

	if (!accounts_exist) {
		/* This is necessary to have the main window shown behind the dialog
		It's an ugly hack... jschmid */
		gtk_widget_show_all(GTK_WIDGET(self));
		modest_ui_actions_on_accounts (NULL, MODEST_WINDOW(self));
	} else {
		update_menus (MODEST_MAIN_WINDOW (self));
	}

	/* Never call this function again (NOTE that it could happen
	   as we hide the main window instead of closing it while
	   there are operations ongoing) and free the helper */
	g_signal_handler_disconnect (self, helper->handler_id);
	g_slice_free (ShowHelper, helper);
}

static void 
osso_display_event_cb (osso_display_state_t state, 
		       gpointer data)
{
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE (data);

	priv->display_state = state;

	/* Stop blinking if the screen becomes on */
	if (priv->display_state == OSSO_DISPLAY_ON)
		modest_platform_remove_new_mail_notifications (TRUE);
}

ModestWindow *
modest_main_window_new (void)
{
	ModestMainWindow *self = NULL;	
	ModestMainWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;
	ModestDimmingRulesGroup *menu_rules_group = NULL;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	GtkActionGroup *action_group = NULL;
	GError *error = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	ShowHelper *helper;
	
	self  = MODEST_MAIN_WINDOW(g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	parent_priv->ui_manager = gtk_ui_manager_new();
	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();

	action_group = gtk_action_group_new ("ModestMainWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	menu_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);
	toolbar_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_TOOLBAR, TRUE);

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
				      self);

	gtk_action_group_add_actions (action_group,
				      modest_folder_view_action_entries,
				      G_N_ELEMENTS (modest_folder_view_action_entries),
				      self);

	gtk_action_group_add_actions (action_group,
				      modest_header_view_action_entries,
				      G_N_ELEMENTS (modest_header_view_action_entries),
				      self);

	gtk_action_group_add_toggle_actions (action_group,
					     modest_main_window_toggle_action_entries,
					     G_N_ELEMENTS (modest_main_window_toggle_action_entries),
					     self);

	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 0);
	g_object_unref (action_group);

	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager,
					 MODEST_UIDIR "modest-main-window-ui.xml", &error);
	if (error != NULL) {
		g_warning ("Could not merge modest-ui.xml: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	/* Add common dimming rules */
	modest_dimming_rules_group_add_rules (menu_rules_group, 
					      modest_main_window_menu_dimming_entries,
					      G_N_ELEMENTS (modest_main_window_menu_dimming_entries),
					      MODEST_WINDOW (self));
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_main_window_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_main_window_toolbar_dimming_entries),
					      MODEST_WINDOW (self));

	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, menu_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	g_object_unref (menu_rules_group);
	g_object_unref (toolbar_rules_group);
	
	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (self), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));

	/* Menubar. Update the state of some toggles */
	parent_priv->menubar = modest_maemo_utils_get_manager_menubar_as_menu (parent_priv->ui_manager, "/MenuBar");
	hildon_window_set_menu (HILDON_WINDOW (self), GTK_MENU (parent_priv->menubar));
	gtk_widget_show (parent_priv->menubar);

	/* Get device name */
	modest_maemo_utils_get_device_name ();

	/* header view */
	priv->header_view =
		MODEST_HEADER_VIEW (modest_header_view_new (NULL, MODEST_HEADER_VIEW_STYLE_DETAILS));
	g_object_ref (priv->header_view);
	if (!priv->header_view)
		g_printerr ("modest: cannot instantiate header view\n");
	modest_header_view_set_style (priv->header_view, MODEST_HEADER_VIEW_STYLE_TWOLINES);
	modest_widget_memory_restore (modest_runtime_get_conf (), G_OBJECT(priv->header_view),
				      MODEST_CONF_HEADER_VIEW_KEY);

	/* Other style properties of header view */
	g_object_set (G_OBJECT (priv->header_view), 
		      "rules-hint", FALSE,
		      NULL);
	/* gtk_widget_show (priv->header_view); */

	/* Empty view */ 
	priv->empty_view = create_empty_view ();
	gtk_widget_show (priv->empty_view);
	g_object_ref (priv->empty_view);
		 
	/* Create scrolled windows */
	priv->contents_widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->contents_widget),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	/* gtk_widget_show (priv->contents_widget); */

	/* putting it all together... */
	priv->main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(priv->main_vbox), priv->contents_widget, TRUE, TRUE,0);
	gtk_widget_show (priv->main_vbox);
	
	gtk_container_add (GTK_CONTAINER(self), priv->main_vbox);
	
	app = hildon_program_get_instance ();
	hildon_program_add_window (app, HILDON_WINDOW (self));
	
	g_signal_connect (G_OBJECT(app), "notify::is-topmost",
		G_CALLBACK (on_hildon_program_is_topmost_notify), self);

	/* Connect to "show" action. We delay the creation of some
	   elements until that moment */
	helper = g_slice_new0 (ShowHelper);
	helper->handler_id = g_signal_connect (G_OBJECT(self), "show",
					       G_CALLBACK (modest_main_window_on_show), 
					       helper);
	
	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_ICON, MODEST_ICON_SIZE_BIG);
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (self), window_icon);
		g_object_unref (window_icon);
	}

	/* Listen for changes in the screen, we don't want to show a
	   led pattern when the display is on for example */
	osso_hw_set_display_event_cb (modest_maemo_utils_get_osso_context (),
				      osso_display_event_cb,
				      self); 

	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */

	return MODEST_WINDOW(self);
}

void 
modest_main_window_set_style (ModestMainWindow *self, 
			      ModestMainWindowStyle style)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	/* We only provide simple style */
	return;
}

ModestMainWindowStyle
modest_main_window_get_style (ModestMainWindow *self)
{
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (self), -1);

	return MODEST_MAIN_WINDOW_STYLE_SIMPLE;
}

static void
toolbar_resize (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;
	GtkWidget *widget;
	gint static_button_size;
	ModestWindowMgr *mgr;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	mgr = modest_runtime_get_window_mgr ();
	static_button_size = modest_window_mgr_get_fullscreen_mode (mgr)?118:108;

	if (parent_priv->toolbar) {
		/* left size buttons */
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageNew");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageReply");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarDeleteMessage");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToggleFolders");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		
 		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->progress_toolitem), FALSE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->refresh_toolitem), TRUE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->refresh_toolitem), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->sort_toolitem), TRUE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->sort_toolitem), TRUE);
	}
		
}

static void 
modest_main_window_show_toolbar (ModestWindow *self,
				 gboolean show_toolbar)
{
	ModestMainWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;	
	GtkWidget *reply_button = NULL, *menu = NULL;
	GtkWidget *placeholder = NULL;
	gint insert_index;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	/* Set optimized view status */
	priv->optimized_view = !show_toolbar;

	if (!parent_priv->toolbar) {
		parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
								  "/ToolBar");
		gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);

		priv->progress_toolitem = GTK_WIDGET (gtk_tool_item_new ());
		priv->cancel_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarCancel");
		priv->refresh_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSendReceive");
		priv->sort_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSort");
		toolbar_resize (MODEST_MAIN_WINDOW (self));
		
		/* Add ProgressBar (Transfer toolbar) */ 
		priv->progress_bar = modest_progress_bar_new ();
		gtk_widget_set_no_show_all (priv->progress_bar, TRUE);
		placeholder = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ProgressBarView");
		insert_index = gtk_toolbar_get_item_index(GTK_TOOLBAR (parent_priv->toolbar), GTK_TOOL_ITEM(placeholder));
		gtk_container_add (GTK_CONTAINER (priv->progress_toolitem), priv->progress_bar);
		gtk_toolbar_insert(GTK_TOOLBAR(parent_priv->toolbar), GTK_TOOL_ITEM (priv->progress_toolitem), insert_index);
		
		/* Connect cancel 'clicked' signal to abort progress mode */
		g_signal_connect(priv->cancel_toolitem, "clicked",
				 G_CALLBACK(cancel_progressbar),
				 self);
		
		/* Add it to the observers list */
		priv->progress_widgets = g_slist_prepend(priv->progress_widgets, priv->progress_bar);

		/* Add to window */
		hildon_window_add_toolbar (HILDON_WINDOW (self), 
					   GTK_TOOLBAR (parent_priv->toolbar));

		/* Set reply button tap and hold menu */
		reply_button = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
							  "/ToolBar/ToolbarMessageReply");
		menu = gtk_ui_manager_get_widget (parent_priv->ui_manager,
						  "/ToolbarReplyCSM");
		gtk_widget_tap_and_hold_setup (GTK_WIDGET (reply_button), menu, NULL, 0);

		/* Set send & receive button tap and hold menu */
		update_menus (MODEST_MAIN_WINDOW (self));
	}
	
	if (show_toolbar) {
		/* Quick hack: this prevents toolbar icons "dance" when progress bar show status is changed */ 
		/* TODO: resize mode migth be GTK_RESIZE_QUEUE, in order to avoid unneccesary shows */
		gtk_container_set_resize_mode (GTK_CONTAINER(parent_priv->toolbar), GTK_RESIZE_IMMEDIATE);

		gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
		if (modest_main_window_transfer_mode_enabled (MODEST_MAIN_WINDOW(self)))
			set_toolbar_mode (MODEST_MAIN_WINDOW(self), TOOLBAR_MODE_TRANSFER);
		else
			set_toolbar_mode (MODEST_MAIN_WINDOW(self), TOOLBAR_MODE_NORMAL);
	} else {
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));

	}
}

static void
on_account_inserted (TnyAccountStore *accoust_store,
                     TnyAccount *account,
                     gpointer user_data)
{
	/* Transport accounts and local ones (MMC and the Local
	   folders account do now cause menu changes */
	if (TNY_IS_STORE_ACCOUNT (account) && 
	    modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (account))) {
		/* Update menus */
		update_menus (MODEST_MAIN_WINDOW (user_data));
	}
}

static void
on_default_account_changed (ModestAccountMgr* mgr,
			    gpointer user_data)
{
	update_menus (MODEST_MAIN_WINDOW (user_data));
}

static void
on_account_removed (TnyAccountStore *accoust_store,
                     TnyAccount *account,
                     gpointer user_data)
{
	/* Transport accounts and local ones (MMC and the Local
	   folders account do now cause menu changes */
	if (TNY_IS_STORE_ACCOUNT (account) && 
	    modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (account)))
		update_menus (MODEST_MAIN_WINDOW (user_data));
}

static void
on_account_changed (TnyAccountStore *account_store,
                    TnyAccount *account,
                    gpointer user_data)
{
	ModestMainWindow *win = MODEST_MAIN_WINDOW (user_data);

	/* Transport accounts and local ones (MMC and the Local
	   folders account do now cause menu changes */
	if (TNY_IS_STORE_ACCOUNT (account)) {
		/* Update the menus as well, name could change */
		if (modest_tny_folder_store_is_remote (TNY_FOLDER_STORE (account)))
			update_menus (MODEST_MAIN_WINDOW (win));
	}
}

/* 
 * This function manages the key events used to navigate between
 * header and folder views (when the window is in split view)
 *
 * FROM         KEY        ACTION
 * -------------------------------------------------
 * HeaderView   GDK_Left   Move focus to folder view
 * FolderView   GDK_Right  Move focus to header view
 *
 * There is no need to scroll to selected row, the widgets will be the
 * responsibles of doing that (probably managing the focus-in event
 */
static gboolean 
on_inner_widgets_key_pressed (GtkWidget *widget,
			      GdkEventKey *event,
			      gpointer user_data)
{
	ModestMainWindowPrivate *priv;

	if (event->type == GDK_KEY_RELEASE)
		return FALSE;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (user_data);

	/* Do nothing if we're in SIMPLE style */
	if (priv->style == MODEST_MAIN_WINDOW_STYLE_SIMPLE)
		return FALSE;

	if (MODEST_IS_HEADER_VIEW (widget)) {
		if (event->keyval == GDK_Left)
			gtk_widget_grab_focus (GTK_WIDGET (priv->folder_view));
		else if ((event->keyval == GDK_Return)||(event->keyval == GDK_KP_Enter)) {
			guint selected_headers = modest_header_view_count_selected_headers (MODEST_HEADER_VIEW (widget));
			if (selected_headers > 1) {
				hildon_banner_show_information (NULL, NULL, _("mcen_ib_select_one_message"));
				return TRUE;
			} else {
				GtkTreePath * cursor_path;
				gtk_tree_view_get_cursor (GTK_TREE_VIEW (widget), &cursor_path, NULL);
				if (cursor_path == NULL) {
					GtkTreeSelection *selection;
					GList *list;
					selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
					list = gtk_tree_selection_get_selected_rows (selection, NULL);

					if (list != NULL)
						gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget), (GtkTreePath *) list->data, NULL, FALSE);
					g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
					g_list_free (list);
				}
			}
		}
	} else if (MODEST_IS_FOLDER_VIEW (widget) && (event->keyval == GDK_Right || event->keyval == GDK_Left)) {
#if GTK_CHECK_VERSION(2, 8, 0) /* TODO: gtk_tree_view_get_visible_range() is only available in GTK+ 2.8 */
		GtkTreePath *selected_path = NULL;
		GtkTreePath *start_path = NULL;
		GtkTreePath *end_path = NULL;
		GList *selected;
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->header_view));
		selected = gtk_tree_selection_get_selected_rows (selection, NULL);
		if (selected != NULL) {
			selected_path = (GtkTreePath *) selected->data;
			if (gtk_tree_view_get_visible_range (GTK_TREE_VIEW (priv->header_view),
							     &start_path,
							     &end_path)) {
				
				if ((gtk_tree_path_compare (start_path, selected_path) != -1) ||
				    (gtk_tree_path_compare (end_path, selected_path) != 1)) {
					
					/* Scroll to first path */
					gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->header_view),
								      selected_path,
								      NULL,
								      TRUE,
								      0.5,
								      0.0);
				}
			}
			if (start_path)
				gtk_tree_path_free (start_path);
			if (end_path)
				gtk_tree_path_free (end_path);
			g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (selected);
		}
#endif /* GTK_CHECK_VERSION */
			/* fix scroll */
		gtk_widget_grab_focus (GTK_WIDGET (priv->header_view));
	}

	return FALSE;
}

static GtkWidget *
create_empty_view (void)
{
	GtkLabel *label = NULL;
	GtkWidget *align = NULL;

	align = gtk_alignment_new(XALIGN, YALIGN, XSPACE, YSPACE);
	label = GTK_LABEL(gtk_label_new (_("mcen_ia_nomessages")));
	gtk_label_set_justify (label, GTK_JUSTIFY_CENTER);	
	gtk_container_add (GTK_CONTAINER (align), GTK_WIDGET(label));

	return GTK_WIDGET(align);
}


gboolean
modest_main_window_send_receive_in_progress (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv = NULL;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (self), FALSE);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	return priv->send_receive_in_progress;
}

void 
modest_main_window_notify_send_receive_initied (ModestMainWindow *self)
{
	GtkAction *action = NULL;
	GtkWidget *widget = NULL;
	ModestMainWindowPrivate *priv = NULL;
	        
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	priv->send_receive_in_progress  = TRUE;

        action = modest_window_get_action (MODEST_WINDOW(self), "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveAllMenu");	
	gtk_action_set_sensitive (action, FALSE);
/*         action = modest_window_get_action (MODEST_WINDOW(self), "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveCancelSendingMenu"); */
/* 	gtk_action_set_sensitive (action, FALSE); */
        widget = modest_window_get_action_widget (MODEST_WINDOW(self), "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsMenuAdditions");	
	gtk_widget_set_sensitive (widget, FALSE);
} 

void 
modest_main_window_notify_send_receive_completed (ModestMainWindow *self)
{
	GtkAction *action = NULL;
	GtkWidget *widget = NULL;
	ModestMainWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	priv->send_receive_in_progress  = FALSE;

        action = modest_window_get_action (MODEST_WINDOW(self), "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveAllMenu");
	gtk_action_set_sensitive (action, TRUE);
/*         action = modest_window_get_action (MODEST_WINDOW(self), "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveCancelSendingMenu");	 */
/* 	gtk_action_set_sensitive (action, TRUE); */
        widget = modest_window_get_action_widget (MODEST_WINDOW(self), "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsMenuAdditions");
	gtk_widget_set_sensitive (widget, TRUE);
}


static void
on_msg_count_changed (ModestHeaderView *header_view,
		      TnyFolder *folder,
		      TnyFolderChange *change,
		      ModestMainWindow *main_window)
{
	gboolean refilter = FALSE;
	gboolean folder_empty = FALSE;
	gboolean all_marked_as_deleted = FALSE;
	ModestMainWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (main_window));
	g_return_if_fail (TNY_IS_FOLDER(folder));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (main_window);

	if (change != NULL) {
		TnyFolderChangeChanged changed;

		changed = tny_folder_change_get_changed (change);
		/* If something changes */
		if ((changed) & TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT)
			folder_empty = (((guint) tny_folder_change_get_new_all_count (change)) == 0);
		else
			folder_empty = (((guint) tny_folder_get_all_count (TNY_FOLDER (folder))) == 0);

		/* Play a sound (if configured) and make the LED blink  */
		if (changed & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS) {
			modest_platform_push_email_notification ();
		}

		if ((changed) & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS)
			refilter = TRUE;
	} else {
		folder_empty = (((guint) tny_folder_get_all_count (TNY_FOLDER (folder))) == 0);
	}

	/* Check if all messages are marked to be deleted */
	all_marked_as_deleted = modest_header_view_is_empty (header_view);
	folder_empty = folder_empty || all_marked_as_deleted;

	/* Set contents style of headers view */
	if (priv->contents_style == MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY ||
	    priv->contents_style == MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS) {
		if (folder_empty)  {
			modest_main_window_set_contents_style (main_window,
							       MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY);
			gtk_widget_grab_focus (GTK_WIDGET (priv->folder_view));
		} else {
			modest_main_window_set_contents_style (main_window,
							       MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS);
		}
	}

	if (refilter)
		modest_header_view_refilter (header_view);
}


void 
modest_main_window_set_contents_style (ModestMainWindow *self, 
				       ModestMainWindowContentsStyle style)
{
	ModestMainWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* We allow to set the same content style than the previously
	   set if there are details, because it could happen when we're
	   selecting different accounts consecutively */
	if ((priv->contents_style == style) &&
	    (priv->contents_style != MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS))
		return;

	/* Remove previous child. Delete it if it was an account
	   details widget */
	GtkWidget *content = gtk_bin_get_child (GTK_BIN (priv->contents_widget));
	if (content) {
		if (priv->contents_style == MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY) {
			gtk_container_remove (GTK_CONTAINER (content), priv->empty_view);
		}
		
		gtk_container_remove (GTK_CONTAINER (priv->contents_widget), content);
	}

	priv->contents_style = style;

	switch (priv->contents_style) {
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_FOLDERS:
		wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET (priv->folder_view));
		modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->contents_widget),
						      TRUE);
		gtk_widget_grab_focus (GTK_WIDGET (priv->folder_view));
		break;
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS:
		wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET (priv->header_view));
		modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->contents_widget),
						      TRUE);
		gtk_widget_grab_focus (GTK_WIDGET (priv->header_view));
		gtk_widget_show (GTK_WIDGET (priv->header_view));
		break;
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS:
		g_warning ("This view is not supported in Fremantle style");
		break;
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY:
		wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET (priv->empty_view));
		modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->contents_widget),
						      FALSE);
		if (priv->style == MODEST_MAIN_WINDOW_STYLE_SIMPLE)
			gtk_widget_grab_focus (GTK_WIDGET (priv->empty_view));
		break;
	default:
		g_return_if_reached ();
	}

	/* Show */
	gtk_widget_show_all (priv->contents_widget);
}

ModestMainWindowContentsStyle
modest_main_window_get_contents_style (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (self), -1);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	return priv->contents_style;
}


static void 
on_configuration_key_changed (ModestConf* conf, 
			      const gchar *key, 
			      ModestConfEvent event,
			      ModestConfNotificationId id, 
			      ModestMainWindow *self)
{
	/* TODO: remove this handler. Now we don't support details view, 
	 *  so this must be removed */

	return;
}

static gboolean
set_toolbar_transfer_mode (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv = NULL;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (self), FALSE);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	set_toolbar_mode (self, TOOLBAR_MODE_TRANSFER);
	
	if (priv->progress_bar_timeout > 0) {
		g_source_remove (priv->progress_bar_timeout);
		priv->progress_bar_timeout = 0;
	}

	return FALSE;
}

static void 
set_toolbar_mode (ModestMainWindow *self, 
		  ModestToolBarModes mode)
{
	ModestWindowPrivate *parent_priv = NULL;
	ModestMainWindowPrivate *priv = NULL;
	GtkAction *sort_action = NULL, *refresh_action = NULL, *cancel_action = NULL;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* In case this was called before the toolbar exists: */
	if (!(parent_priv->toolbar))
		return;

	g_return_if_fail (GTK_IS_TOOLBAR(parent_priv->toolbar)); 
	
	sort_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarSort");
	refresh_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarSendReceive");
	cancel_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarCancel");

	/* Sets current toolbar mode */
	priv->current_toolbar_mode = mode;

        /* Checks the dimming rules */
        modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (self));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));

	/* Show and hide toolbar items */
	switch (mode) {
	case TOOLBAR_MODE_NORMAL:
		if (sort_action)
			gtk_action_set_visible (sort_action, TRUE);
		if (refresh_action)
			gtk_action_set_visible (refresh_action, TRUE);
		if (priv->progress_toolitem) {
			gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), FALSE);
			gtk_widget_hide (priv->progress_toolitem);
		}
		if (priv->progress_bar)
			gtk_widget_hide (priv->progress_bar);
		
		if (cancel_action)
			gtk_action_set_visible (cancel_action, FALSE);

		/* Hide toolbar if optimized view is enabled */
		if (priv->optimized_view)
			gtk_widget_hide (GTK_WIDGET(parent_priv->toolbar));
		break;
	case TOOLBAR_MODE_TRANSFER:
		if (sort_action)
			gtk_action_set_visible (sort_action, FALSE);
		if (refresh_action)
			gtk_action_set_visible (refresh_action, FALSE);
		if (cancel_action)
			gtk_action_set_visible (cancel_action, TRUE);
		if (priv->progress_bar)
			gtk_widget_show (priv->progress_bar);
		if (priv->progress_toolitem) {
			gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
			gtk_widget_show (priv->progress_toolitem);
		}

		/* Show toolbar if it's hiden (optimized view ) */
		if (priv->optimized_view)
			gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
		break;
	default:
		g_return_if_reached ();
	}
}

gboolean
modest_main_window_transfer_mode_enabled (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (self), FALSE);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	return priv->current_toolbar_mode == TOOLBAR_MODE_TRANSFER;
}

static void
cancel_progressbar (GtkToolButton *toolbutton,
		    ModestMainWindow *self)
{
	GSList *tmp;
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* Get operation observers and cancel all the operations */
	tmp = priv->progress_widgets;
	while (tmp) {
		modest_progress_object_cancel_all_operations (MODEST_PROGRESS_OBJECT(tmp->data));
		tmp=g_slist_next(tmp);
	}
}

static gboolean
observers_empty (ModestMainWindow *self)
{
	GSList *tmp = NULL;
	ModestMainWindowPrivate *priv;
	gboolean is_empty = TRUE;
	guint pending_ops = 0;
 
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	tmp = priv->progress_widgets;

	/* Check all observers */
	while (tmp && is_empty)  {
		pending_ops = modest_progress_object_num_pending_operations (MODEST_PROGRESS_OBJECT(tmp->data));
		is_empty = pending_ops == 0;
		
		tmp = g_slist_next(tmp);
	}
	
	return is_empty;
}


/**
 * Gets the toolbar mode needed for each mail operation. It stores in
 * @mode_changed if the toolbar mode has changed or not
 */
static ModestToolBarModes
get_toolbar_mode_from_mail_operation (ModestMainWindow *self,
				      ModestMailOperation *mail_op,
				      gboolean *mode_changed)
{
	ModestToolBarModes mode;
	ModestMainWindowPrivate *priv;

	*mode_changed = FALSE;
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);

	/* Get toolbar mode from operation id*/
	switch (modest_mail_operation_get_type_operation (mail_op)) {
	case MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE:
	case MODEST_MAIL_OPERATION_TYPE_RECEIVE:
		mode = TOOLBAR_MODE_TRANSFER;
		if (priv->current_toolbar_mode == TOOLBAR_MODE_NORMAL)
			*mode_changed = TRUE;
		break;
	default:
		mode = TOOLBAR_MODE_NORMAL;		
	}
	return mode;
}

static void 
on_mail_operation_started (ModestMailOperation *mail_op,
			   gpointer user_data)
{
	ModestMainWindow *self;
	ModestMailOperationTypeOperation op_type;
	ModestMainWindowPrivate *priv;
	ModestToolBarModes mode;
	GSList *tmp;
	gboolean mode_changed = FALSE;
	TnyAccount *account = NULL;

	self = MODEST_MAIN_WINDOW (user_data);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);

	/* Do not show progress for receiving operations if the
	   account is the local account or the MMC one */
	op_type = modest_mail_operation_get_type_operation (mail_op);
	account = modest_mail_operation_get_account (mail_op);
	if (account && op_type == MODEST_MAIL_OPERATION_TYPE_OPEN) {
		gboolean is_remote;

		is_remote = !(modest_tny_account_is_virtual_local_folders (account) ||
			      modest_tny_account_is_memory_card_account (account));
		if (!is_remote) {
			g_object_unref (account);
			return;
		}

		/* Show information banner. Remove old timeout */
		if (priv->opening_banner_timeout > 0) {
			g_source_remove (priv->opening_banner_timeout);
			priv->opening_banner_timeout = 0;
		}
		/* Create a new timeout */
		priv->opening_banner_timeout = 
			g_timeout_add (2000, show_opening_banner, self);
	}

	/* Not every mail operation has account, noop does not */
	if (account)
		g_object_unref (account);
	       
	/* Get toolbar mode from operation id*/
	mode = get_toolbar_mode_from_mail_operation (self, mail_op, &mode_changed);

	/* Add operation observers and change toolbar if neccessary*/
	tmp = priv->progress_widgets;
	if (mode == TOOLBAR_MODE_TRANSFER) {
		if (mode_changed) {
			GObject *source = modest_mail_operation_get_source(mail_op);
			if (G_OBJECT (self) == source) {
				set_toolbar_transfer_mode(self);
			}
			g_object_unref (source);
		}

		while (tmp) {
			modest_progress_object_add_operation (MODEST_PROGRESS_OBJECT (tmp->data),
							      mail_op);
			tmp = g_slist_next (tmp);
		}
	}

	/* Update the main menu as well, we need to explicitely do
	   this in order to enable/disable accelerators */
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
}

static void 
on_mail_operation_finished (ModestMailOperation *mail_op,
			    gpointer user_data)
{
	ModestToolBarModes mode;
	ModestMailOperationTypeOperation op_type;
	GSList *tmp = NULL;
	ModestMainWindow *self;
	gboolean mode_changed;
	TnyAccount *account = NULL;
	ModestMainWindowPrivate *priv;

	self = MODEST_MAIN_WINDOW (user_data);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);

	/* The mail operation was not added to the progress objects if
	   the account was the local account or the MMC one */
	op_type = modest_mail_operation_get_type_operation (mail_op);
	account = modest_mail_operation_get_account (mail_op);
	if (account && op_type == MODEST_MAIL_OPERATION_TYPE_OPEN) {
		gboolean is_remote;

		is_remote = !(modest_tny_account_is_virtual_local_folders (account) ||
			      modest_tny_account_is_memory_card_account (account));
		if (!is_remote) {
			g_object_unref (account);
			return;
		}

		/* Remove old timeout */
		if (priv->opening_banner_timeout > 0) {
			g_source_remove (priv->opening_banner_timeout);
			priv->opening_banner_timeout = 0;
		}

		/* Remove the banner if exists */
		if (priv->opening_banner) {
			gtk_widget_destroy (priv->opening_banner);
			priv->opening_banner = NULL;
		}
	}

	/* Not every mail operation has account, noop does not */
	if (account)
		g_object_unref (account);

	/* Get toolbar mode from operation id*/
	mode = get_toolbar_mode_from_mail_operation (self, mail_op, &mode_changed);

	/* Change toolbar mode */
	tmp = priv->progress_widgets;
	if (mode == TOOLBAR_MODE_TRANSFER) {			
		while (tmp) {
			modest_progress_object_remove_operation (MODEST_PROGRESS_OBJECT (tmp->data),
								 mail_op);
			tmp = g_slist_next (tmp);
		}
		
		/* If no more operations are being observed, NORMAL mode is enabled again */
		if (observers_empty (self)) {
			set_toolbar_mode (self, TOOLBAR_MODE_NORMAL);				
		}
	}
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);

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
set_account_visible(ModestMainWindow *self, const gchar *acc_name)
{
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	GtkAction *action;
	ModestAccountMgr *mgr;
	ModestAccountSettings *settings;
	ModestServerAccountSettings *store_settings = NULL;

/* 	GtkWidget *folder_window; */

/* 	folder_window = GTK_WIDGET (modest_folder_window_new (NULL)); */
/* 	modest_window_mgr_register_window (modest_runtime_get_window_mgr (),  */
/* 					   MODEST_WINDOW (folder_window), */
/* 					   MODEST_WINDOW (self)); */
/* 	gtk_widget_show (folder_window); */
	GtkWidget *accounts_window;

	accounts_window = GTK_WIDGET (modest_accounts_window_new ());
	modest_window_mgr_register_window (modest_runtime_get_window_mgr (),
					   MODEST_WINDOW (accounts_window),
					   MODEST_WINDOW (self));
	gtk_widget_show (accounts_window);

	/* Get account data */
	mgr = modest_runtime_get_account_mgr ();
	settings = modest_account_mgr_load_account_settings (mgr, acc_name);
	if (settings)
		store_settings = modest_account_settings_get_store_settings (settings);

	/* Set the new visible & active account */
	if (settings && (modest_server_account_settings_get_account_name (store_settings)!= NULL)) { 
		const gchar *account_name;

		account_name = modest_account_settings_get_account_name (settings);

		modest_folder_view_set_account_id_of_visible_server_account 
			(priv->folder_view,
			 modest_server_account_settings_get_account_name (store_settings));
		modest_folder_view_select_first_inbox_or_local (priv->folder_view);
		modest_window_set_active_account (MODEST_WINDOW (self), account_name);

/* 		modest_folder_window_set_account (MODEST_FOLDER_WINDOW (folder_window), acc_name); */

		action = gtk_action_group_get_action (priv->view_additions_group, account_name);
		if (action != NULL) {
			if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action))) {
				modest_utils_toggle_action_set_active_block_notify (
					GTK_TOGGLE_ACTION (action),
					TRUE);
			}
		}
	}
	
	/* Free */
	if (settings) {
		g_object_unref (store_settings);
		g_object_unref (settings);
	}

}

/* Make sure that at least one account is "viewed": */
static void
set_at_least_one_account_visible(ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr();

	if (!(priv->folder_view)) {
		/* It is too early to do this. */
		return;	
	}
	
	const gchar *active_server_account_name = 
		modest_folder_view_get_account_id_of_visible_server_account (priv->folder_view);
	
	if (!active_server_account_name ||
		!modest_account_mgr_account_exists (account_mgr, active_server_account_name, TRUE))
	{
		gchar* first_modest_name = modest_account_mgr_get_first_account_name (account_mgr);
		gchar* default_modest_name = modest_account_mgr_get_default_account (account_mgr);
		if (default_modest_name) {
			set_account_visible (self, default_modest_name);
		} else if (first_modest_name) {
			set_account_visible (self, first_modest_name);
		}
		g_free (first_modest_name);
		g_free (default_modest_name);
	}
}

static void 
on_show_account_action_toggled  (GtkToggleAction *action,
				   gpointer user_data)
{
	ModestMainWindow *self = MODEST_MAIN_WINDOW (user_data);

	const gchar *acc_name = gtk_action_get_name (GTK_ACTION (action));
	if (gtk_toggle_action_get_active (action))
		set_account_visible (self, acc_name);
}

static void
refresh_account (const gchar *account_name)
{
	ModestWindow *win;
	
	/* win must already exists here, obviously */ 
	win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr (),
						 FALSE);
	if (!win) {
		g_warning ("%s: BUG: no main window!", __FUNCTION__);
		return;
	}
	
	/* If account_name == NULL, we must update all (option All) */
	if (!account_name)
		modest_ui_actions_do_send_receive_all (win, TRUE, TRUE, TRUE);
	else
		modest_ui_actions_do_send_receive (account_name, TRUE, TRUE, TRUE, win);
	
}

static void 
on_refresh_account_action_activated  (GtkAction *action,
				      gpointer user_data)
{
	refresh_account ((const gchar*) user_data);
}

static void
on_send_receive_csm_activated (GtkMenuItem *item,
			       gpointer user_data)
{
	refresh_account ((const gchar*) user_data);
}

static gboolean
on_zoom_minus_plus_not_implemented (ModestWindow *window)
{
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (window), FALSE);

	hildon_banner_show_information (NULL, NULL, _CS("ckct_ib_cannot_zoom_here"));
	return FALSE;

}

static gboolean
on_folder_view_focus_in (GtkWidget *widget, GdkEventFocus *event, gpointer userdata)
{
	ModestMainWindow *main_window = NULL;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (userdata), FALSE);
	main_window = MODEST_MAIN_WINDOW (userdata);
	
	/* Update toolbar dimming state */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (main_window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (main_window));

	return FALSE;
}

static gboolean
on_header_view_focus_in (GtkWidget *widget,
			 GdkEventFocus *event,
			 gpointer userdata)
{
	ModestMainWindow *main_window = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (userdata), FALSE);

	main_window = MODEST_MAIN_WINDOW (userdata);

	/* Update toolbar dimming state */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (main_window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (main_window));

	return FALSE;
}

static void 
on_folder_selection_changed (ModestFolderView *folder_view,
			     TnyFolderStore *folder_store, 
			     gboolean selected,
			     ModestMainWindow *main_window)
{
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (main_window);
	GtkAction *action = NULL;
	gboolean show_reply = TRUE;
	gboolean show_forward = TRUE;
	gboolean show_cancel_send = FALSE;
	gboolean show_clipboard = TRUE;
	gboolean show_delete = TRUE;

	if (selected) {
		if (TNY_IS_ACCOUNT (folder_store)) {
			show_reply = show_forward = show_cancel_send = show_clipboard = show_delete = FALSE;
		} else if (TNY_IS_FOLDER (folder_store)) {
			if (modest_tny_folder_is_local_folder (TNY_FOLDER (folder_store))) {
				TnyFolderType folder_type = modest_tny_folder_get_local_or_mmc_folder_type (
					TNY_FOLDER (folder_store));
				switch (folder_type) {
				case TNY_FOLDER_TYPE_DRAFTS:
					show_clipboard = show_delete = TRUE;
					show_reply = show_forward = show_cancel_send = FALSE;
					break;
				case TNY_FOLDER_TYPE_SENT:
					show_forward = show_clipboard = show_delete = TRUE;
					show_reply = show_cancel_send = FALSE;
					break;
				case TNY_FOLDER_TYPE_OUTBOX:
					show_clipboard = show_delete = show_cancel_send = TRUE;
					show_reply = show_forward = FALSE;
					break;
				case TNY_FOLDER_TYPE_INVALID:
					g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
					break;
				default:
					show_reply = show_forward = show_clipboard = show_delete = TRUE;
					show_cancel_send = FALSE;
				}
			} else {
				show_reply = show_forward = show_clipboard = show_delete = TRUE;
				show_cancel_send = FALSE;
			}
		}
	}

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMReply");
	gtk_action_set_visible (action, show_reply);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMReplyAll");
	gtk_action_set_visible (action, show_reply);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMForward");
	gtk_action_set_visible (action, show_forward);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMCancelSending");
	gtk_action_set_visible (action, show_cancel_send);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMDelete");
	gtk_action_set_visible (action, show_delete);

}

gboolean 
modest_main_window_on_msg_view_window_msg_changed (ModestMsgViewWindow *view_window,
						   GtkTreeModel *model,
						   GtkTreeRowReference *row_reference,
						   ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv = NULL;
	GtkTreeModel *header_model = NULL;
 	GtkTreePath *path = NULL;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (view_window), FALSE);
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (self), FALSE);
	g_return_val_if_fail (gtk_tree_row_reference_valid (row_reference), FALSE);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);
	header_model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->header_view));

	/* Do nothing if we changed the folder in the main view */
	if (header_model != model)
		return FALSE;

	/* Select the message in the header view */
	path = gtk_tree_row_reference_get_path (row_reference);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->header_view),
				  path, NULL, FALSE);
	gtk_tree_path_free (path);

	return TRUE;
}

static void
updating_banner_destroyed (gpointer data,
			   GObject *where_the_object_was)
{
	ModestMainWindowPrivate *priv = NULL;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (data);

	priv->updating_banner = NULL;
}

static gboolean
show_updating_banner (gpointer user_data)
{
	ModestMainWindowPrivate *priv = NULL;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (user_data);

	if (priv->updating_banner == NULL) {

		/* We're outside the main lock */
		gdk_threads_enter ();
		priv->updating_banner = 
			modest_platform_animation_banner (GTK_WIDGET (user_data), NULL,
							  _CS ("ckdg_pb_updating"));

		/* We need this because banners in Maemo could be
		   destroyed by dialogs so we need to properly update
		   our reference to it */
		g_object_weak_ref (G_OBJECT (priv->updating_banner),
				   updating_banner_destroyed,
				   user_data);
		gdk_threads_leave ();
	}

	/* Remove timeout */
	priv->updating_banner_timeout = 0;
	return FALSE;
}

/**
 * We use this function to show/hide a progress banner showing
 * "Updating" while the header view is being filled. We're not showing
 * it unless the update takes more than 2 seconds
 *
 * If starting = TRUE then the refresh is starting, otherwise it means
 * that is has just finished
 */
static void 
on_updating_msg_list (ModestHeaderView *header_view,
		      gboolean starting,
		      gpointer user_data)
{
	ModestMainWindowPrivate *priv = NULL;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (user_data);
	
	/* Remove old timeout */
	if (priv->updating_banner_timeout > 0) {
		g_source_remove (priv->updating_banner_timeout);
		priv->updating_banner_timeout = 0;
	}

	/* Create a new timeout */
	if (starting) {
		priv->updating_banner_timeout = 
			g_timeout_add (2000, show_updating_banner, user_data);
	} else {
		/* Remove the banner if exists */
		if (priv->updating_banner) {
			gtk_widget_destroy (priv->updating_banner);
			priv->updating_banner = NULL;
		}
	}
}

static void on_folder_view_row_activated (GtkTreeView *tree_view,
					  GtkTreePath *tree_path,
					  GtkTreeViewColumn *column,
					  gpointer userdata)
{
	GtkTreeModel  *model;
	GtkTreeIter iter;
	TnyFolderStore *folder_store = NULL;
	ModestMainWindow *self = (ModestMainWindow *) userdata;
	ModestMainWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW(self));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

	if (gtk_tree_model_get_iter (model, &iter, tree_path)) {
		gtk_tree_model_get (model, &iter,
				    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder_store,
				    -1);
		if (folder_store && TNY_IS_FOLDER (folder_store)) {
			modest_header_view_set_folder (MODEST_HEADER_VIEW (priv->header_view), 
						       TNY_FOLDER (folder_store), TRUE, MODEST_WINDOW (self),
						       NULL, NULL);
			modest_main_window_set_contents_style (MODEST_MAIN_WINDOW (self),
							       MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS);
			modest_widget_memory_restore (modest_runtime_get_conf (), 
						      G_OBJECT(priv->header_view),
						      MODEST_CONF_HEADER_VIEW_KEY);
			on_msg_count_changed (MODEST_HEADER_VIEW (priv->header_view), TNY_FOLDER (folder_store), NULL, self);
		}

		if (folder_store)
			g_object_unref (folder_store);
	}

	g_debug ("FOLDER VIEW CELL ACTIVATED");
	
}

gboolean
modest_main_window_screen_is_on (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW(self), FALSE);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);
	
	return (priv->display_state == OSSO_DISPLAY_ON) ? TRUE : FALSE;
}

static void
remove_banners (ModestMainWindow *window)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (window);

	if (priv->opening_banner_timeout > 0) {
		g_source_remove (priv->opening_banner_timeout);
		priv->opening_banner_timeout = 0;
	}

	if (priv->opening_banner != NULL) {
		gtk_widget_destroy (priv->opening_banner);
		priv->opening_banner = NULL;
	}
	
	if (priv->updating_banner_timeout > 0) {
		g_source_remove (priv->updating_banner_timeout);
		priv->updating_banner_timeout = 0;
	}

	if (priv->updating_banner != NULL) {
		gtk_widget_destroy (priv->updating_banner);
		priv->updating_banner = NULL;
	}	
}


static void
on_window_hide (GObject    *gobject,
		GParamSpec *arg1,
		gpointer    user_data)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (gobject));

	if (!GTK_WIDGET_VISIBLE (gobject)) {
		TnyFolderStore *folder_store;
		ModestMainWindowPrivate *priv;
		
		/* Remove the currently shown banners */
		remove_banners (MODEST_MAIN_WINDOW (gobject));

		/* Force the folder view to sync the currently selected folder
		   to save the read/unread status and to expunge messages */
		priv = MODEST_MAIN_WINDOW_GET_PRIVATE (gobject);
		folder_store = modest_folder_view_get_selected (priv->folder_view);
		if (TNY_IS_FOLDER (folder_store)) {
			ModestMailOperation *mail_op;
			
			mail_op = modest_mail_operation_new (NULL);
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
							 mail_op);
			modest_mail_operation_sync_folder (mail_op, TNY_FOLDER (folder_store), FALSE, NULL, NULL);
			g_object_unref (mail_op);
			g_object_unref (folder_store);
		}
	}
}

static void
on_window_destroy (GtkObject *widget, 
		   gpointer user_data)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (widget));

	remove_banners (MODEST_MAIN_WINDOW (widget));
}

static void
opening_banner_destroyed (gpointer data,
			   GObject *where_the_object_was)
{
	ModestMainWindowPrivate *priv = NULL;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (data);

	priv->opening_banner = NULL;
}

static gboolean
show_opening_banner (gpointer user_data)
{
	ModestMainWindowPrivate *priv = NULL;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (user_data);

	if (priv->opening_banner == NULL) {

		/* We're outside the main lock */
		gdk_threads_enter ();
		priv->opening_banner = 
			modest_platform_animation_banner (GTK_WIDGET (user_data), NULL,
							  _("mail_me_opening"));

		/* We need this because banners in Maemo could be
		   destroyed by dialogs so we need to properly update
		   our reference to it */
		g_object_weak_ref (G_OBJECT (priv->opening_banner),
				   opening_banner_destroyed,
				   user_data);

		/* We need this because banners in Maemo could be
		   destroyed by dialogs so we need to properly update
		   our reference to it */
		g_object_weak_ref (G_OBJECT (priv->updating_banner),
				   updating_banner_destroyed,
				   user_data);
		gdk_threads_leave ();
	}

	/* Remove timeout */
	priv->opening_banner_timeout = 0;
	return FALSE;
}

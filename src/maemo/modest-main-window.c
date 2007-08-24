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
#include <gtk/gtktreeviewcolumn.h>
#include <tny-account-store-view.h>
#include <tny-simple-list.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-maemo-conic-device.h>
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
#include <modest-maemo-utils.h>
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg.h"
#include "modest-mail-operation.h"
#include "modest-icon-names.h"
#include "modest-progress-bar-widget.h"
#include "modest-text-utils.h"
#include "modest-ui-dimming-manager.h"
#include "maemo/modest-osso-state-saving.h"
#include "modest-text-utils.h"
#include "modest-signal-mgr.h"

#ifdef MODEST_HAVE_HILDON0_WIDGETS
#include <hildon-widgets/hildon-program.h>
#else
#include <hildon/hildon-program.h>
#endif /*MODEST_HAVE_HILDON0_WIDGETS*/

#define MODEST_MAIN_WINDOW_ACTION_GROUP_ADDITIONS "ModestMainWindowActionAdditions"

#define XALIGN 0.5
#define YALIGN 0.0
#define XSPACE 1
#define YSPACE 0

/* 'private'/'protected' functions */
static void modest_main_window_class_init  (ModestMainWindowClass *klass);
static void modest_main_window_init        (ModestMainWindow *obj);
static void modest_main_window_finalize    (GObject *obj);

static gboolean modest_main_window_window_state_event (GtkWidget *widget, 
							   GdkEventWindowState *event, 
							   gpointer userdata);

static void connect_signals (ModestMainWindow *self);

static void modest_main_window_disconnect_signals (ModestWindow *self);

static void restore_settings (ModestMainWindow *self, 
			      gboolean do_folder_view_too);

static void save_state (ModestWindow *self);

static void
update_menus (ModestMainWindow* self);

static void modest_main_window_show_toolbar   (ModestWindow *window,
					       gboolean show_toolbar);

static void cancel_progressbar (GtkToolButton *toolbutton,
				ModestMainWindow *self);

static void on_queue_changed   (ModestMailOperationQueue *queue,
				ModestMailOperation *mail_op,
				ModestMailOperationQueueNotification type,
				ModestMainWindow *self);

static gboolean on_zoom_minus_plus_not_implemented (ModestWindow *window);

static void
on_account_inserted (TnyAccountStore *accoust_store,
                     TnyAccount *account,
                     gpointer user_data);

static void
on_account_removed (TnyAccountStore *accoust_store,
                    TnyAccount *account,
                    gpointer user_data);

static void
on_account_changed (ModestAccountMgr* mgr,
                    const gchar* account,
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

static void on_show_account_action_activated      (GtkAction *action,
						   gpointer user_data);

static void on_refresh_account_action_activated   (GtkAction *action,
						   gpointer user_data);

static void on_send_receive_csm_activated         (GtkMenuItem *item,
						   gpointer user_data);

static void
_on_msg_count_changed (ModestHeaderView *header_view,
		       TnyFolder *folder,
		       TnyFolderChange *change,
		       ModestMainWindow *main_window);

static void
modest_main_window_cleanup_queue_error_signals (ModestMainWindow *self);


static GtkWidget * create_empty_view (void);

static gboolean
on_folder_view_focus_in (GtkWidget *widget,
			 GdkEventFocus *event,
			 gpointer userdata);
static gboolean
on_header_view_focus_in (GtkWidget *widget,
			 GdkEventFocus *event,
			 gpointer userdata);
static void 
modest_main_window_on_folder_selection_changed (ModestFolderView *folder_view,
						TnyFolderStore *folder_store, 
						gboolean selected,
						ModestMainWindow *main_window);
						
static void
set_at_least_one_account_visible(ModestMainWindow *self);


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMainWindowPrivate ModestMainWindowPrivate;
struct _ModestMainWindowPrivate {
	GtkWidget *msg_paned;
	GtkWidget *main_paned;
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

	/* Merge ids used to add/remove accounts to the ViewMenu*/
	GByteArray *merge_ids;

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

	guint progress_bar_timeout;

	/* Signal handler UIDs */
	GList *queue_err_signals;
	GSList *sighandlers;
	
	ModestConfNotificationId notification_id;
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
	{ "FolderViewCSMNewFolder", NULL, N_("mcen_ti_new_folder"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "FolderViewCSMRenameFolder", NULL, N_("mcen_me_user_renamefolder"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_rename_folder) },
	{ "FolderViewCSMPasteMsgs", NULL, N_("mcen_me_inbox_paste"), NULL, NULL,  G_CALLBACK (modest_ui_actions_on_paste)},
	{ "FolderViewCSMDeleteFolder", NULL, N_("mcen_me_inbox_delete"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_delete_folder) },
	{ "FolderViewCSMSearchMessages", NULL, N_("mcen_me_inbox_search"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_search_messages) },
	{ "FolderViewCSMHelp", NULL, N_("mcen_me_inbox_help"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_help) },
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
	{ "HeaderViewCSMDelete",        NULL,  N_("mcen_me_inbox_delete"),      NULL,      NULL, G_CALLBACK (modest_ui_actions_on_delete_message) },
	{ "HeaderViewCSMCancelSending", NULL,  N_("mcen_me_outbox_cancelsend"), NULL,      NULL, G_CALLBACK (modest_ui_actions_cancel_send) },
	{ "HeaderViewCSMHelp",          NULL,  N_("mcen_me_inbox_help"),        NULL,      NULL, G_CALLBACK (modest_ui_actions_on_help) },
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
	priv->msg_paned    = NULL;
	priv->main_paned   = NULL;	
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
	priv->contents_style  = -1; /* invalid contents style. We need this to select it for the first time */
	priv->merge_ids = NULL;
	priv->optimized_view  = FALSE;
	priv->send_receive_in_progress  = FALSE;
	priv->progress_bar_timeout = 0;
	priv->sighandlers = NULL;
}

static void
modest_main_window_finalize (GObject *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	if (priv->notification_id) {
		modest_conf_forget_namespace (modest_runtime_get_conf (),
					      MODEST_CONF_NAMESPACE,
					      priv->notification_id);
	}
	
	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_main_window_disconnect_signals (MODEST_WINDOW (obj));

	modest_main_window_cleanup_queue_error_signals ((ModestMainWindow *) obj);

	g_slist_free (priv->progress_widgets);

	g_byte_array_free (priv->merge_ids, TRUE);

	if (priv->progress_bar_timeout > 0) {
		g_source_remove (priv->progress_bar_timeout);
		priv->progress_bar_timeout = 0;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_main_window_get_child_widget (ModestMainWindow *self,
				     ModestWidgetType widget_type)
{
	ModestMainWindowPrivate *priv;
	GtkWidget *widget;
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (widget_type >= 0 && widget_type < MODEST_WIDGET_TYPE_NUM,
			      NULL);
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	switch (widget_type) {
	case MODEST_WIDGET_TYPE_HEADER_VIEW:
		widget = (GtkWidget*)priv->header_view; break;
	case MODEST_WIDGET_TYPE_FOLDER_VIEW:
		widget = (GtkWidget*)priv->folder_view; break;
	default:
		return NULL;
	}

	return widget ? GTK_WIDGET(widget) : NULL;
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

	modest_widget_memory_restore (conf, G_OBJECT(priv->main_paned),
				      MODEST_CONF_MAIN_PANED_KEY);

	/* We need to force a redraw here in order to get the right
	   position of the horizontal paned separator */
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
	modest_widget_memory_save (conf, G_OBJECT(priv->main_paned), 
				   MODEST_CONF_MAIN_PANED_KEY);
	//	modest_widget_memory_save (conf, G_OBJECT(priv->header_view), 
 	//			   MODEST_CONF_HEADER_VIEW_KEY);
	modest_widget_memory_save (conf, G_OBJECT(priv->folder_view), 
				   MODEST_CONF_FOLDER_VIEW_KEY);
}

static gint
compare_display_names (ModestAccountData *a,
		       ModestAccountData *b)
{
	return strcmp (a->display_name, b->display_name);
}

static void
update_menus (ModestMainWindow* self)
{	
	GSList *account_names, *iter, *accounts;
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	ModestAccountMgr *mgr;
	gint i, num_accounts;
	GtkActionGroup *action_group;
	GList *groups;
	gchar *default_account;
	GtkWidget *send_receive_button, *item;
	GtkAction *send_receive_all = NULL;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (self);

	/* Get enabled account IDs */
	mgr = modest_runtime_get_account_mgr ();
	account_names = modest_account_mgr_account_names (mgr, TRUE);
	iter = account_names;
	accounts = NULL;

	while (iter) {
		ModestAccountData *account_data = 
			modest_account_mgr_get_account_data (mgr, (gchar*) iter->data);
		accounts = g_slist_prepend (accounts, account_data);

		iter = iter->next;
	}
	modest_account_mgr_free_account_names (account_names);
	account_names = NULL;

	/* Order the list of accounts by its display name */
	accounts = g_slist_sort (accounts, (GCompareFunc) compare_display_names);
	num_accounts = g_slist_length (accounts);

	send_receive_all = gtk_ui_manager_get_action (parent_priv->ui_manager, 
						      "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveAllMenu");
	gtk_action_set_visible (send_receive_all, num_accounts > 1);

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
			   order (alphabetical */
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
	action_group = gtk_action_group_new (MODEST_MAIN_WINDOW_ACTION_GROUP_ADDITIONS);
	for (i = 0; i < num_accounts; i++) {
		gchar *display_name = NULL;
		
		ModestAccountData *account_data = (ModestAccountData *) g_slist_nth_data (accounts, i);

		/* Create display name. The UI specification specifies a different format string 
		 * to use for the default account, though both seem to be "%s", so 
		 * I don't see what the point is. murrayc. */
		if (default_account && account_data->account_name && 
			!(strcmp (default_account, account_data->account_name) == 0)) {
			display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_default"), 
							account_data->display_name);
		}
		else {
			display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_mailbox_n"), 
							account_data->display_name);
		}

		/* Create action and add it to the action group. The
		   action name must be the account name, this way we
		   could know in the handlers the account to show */
		if(account_data && account_data->account_name) {
			gchar* item_name, *refresh_action_name;
			guint8 merge_id = 0;
			GtkAction *view_account_action, *refresh_account_action;

			view_account_action = gtk_action_new (account_data->account_name,
							      display_name, NULL, NULL);
			gtk_action_group_add_action (action_group, view_account_action);

			/* Add ui from account data. We allow 2^9-1 account
			   changes in a single execution because we're
			   downcasting the guint to a guint8 in order to use a
			   GByteArray. It should be enough. */
			item_name = g_strconcat (account_data->account_name, "Menu", NULL);
			merge_id = (guint8) gtk_ui_manager_new_merge_id (parent_priv->ui_manager);
			priv->merge_ids = g_byte_array_append (priv->merge_ids, &merge_id, 1);
			gtk_ui_manager_add_ui (parent_priv->ui_manager,
					       merge_id,
					       "/MenuBar/ViewMenu/ViewMenuAdditions",
					       item_name,
					       account_data->account_name,
					       GTK_UI_MANAGER_MENUITEM,
					       FALSE);

			/* Connect the action signal "activate" */
			g_signal_connect (G_OBJECT (view_account_action),
					  "activate",
					  G_CALLBACK (on_show_account_action_activated),
					  self);

			/* Create the items for the Tools->Send&Receive submenu */
			refresh_action_name = g_strconcat ("SendReceive", account_data->account_name, NULL);
			refresh_account_action = gtk_action_new ((const gchar*) refresh_action_name, 
								 display_name, NULL, NULL);
			printf("DEBUG: %s: menu display_name=%s\n", __FUNCTION__, display_name);
			gtk_action_group_add_action (action_group, refresh_account_action);

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
					       g_strdup (account_data->account_name),
					       (GClosureNotify) g_free,
					       0);

			/* Create item and add it to the send&receive
			   CSM. If there is only one account then
			   it'll be no menu */
			if (priv->accounts_popup) {
				GtkWidget *label = gtk_label_new(NULL);
				gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
				if (default_account && (strcmp(account_data->account_name, default_account) == 0))
				{
					gchar *escaped = g_markup_printf_escaped ("<b>%s</b>", display_name);
					gtk_label_set_markup (GTK_LABEL (label), escaped);
					g_free (escaped);
				}
				else
				{
					gtk_label_set_text (GTK_LABEL (label), display_name);
				}

				item = gtk_menu_item_new ();
				gtk_container_add (GTK_CONTAINER (item), label);

				gtk_menu_shell_prepend (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));
				g_signal_connect_data (G_OBJECT (item), 
						       "activate", 
						       G_CALLBACK (on_send_receive_csm_activated),
						       g_strdup (account_data->account_name),
						       (GClosureNotify) g_free,
						       0);
			}
			g_free (item_name);
		}

		/* Frees */
		g_free (display_name);
	}

	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 1);

	/* We cannot do this in the loop above because this relies on the action
	 * group being inserted. This makes the default account appear in bold.
	 * I agree it is a rather ugly way, but I don't see another possibility. armin. */
	for (i = 0; i < num_accounts; i++) {
		ModestAccountData *account_data = (ModestAccountData *) g_slist_nth_data (accounts, i);

		if(account_data->account_name && default_account &&
		   strcmp (account_data->account_name, default_account) == 0) {
			gchar *item_name = g_strconcat (account_data->account_name, "Menu", NULL);

			gchar *path = g_strconcat ("/MenuBar/ViewMenu/ViewMenuAdditions/", item_name, NULL);
			GtkWidget *item = gtk_ui_manager_get_widget (parent_priv->ui_manager, path);
			g_free(path);

			if (item) {
				GtkWidget *child = gtk_bin_get_child (GTK_BIN (item));
				if (GTK_IS_LABEL (child)) {
					const gchar *cur_name = gtk_label_get_text (GTK_LABEL (child));
					gchar *bold_name = g_markup_printf_escaped("<b>%s</b>", cur_name);
					gtk_label_set_markup (GTK_LABEL (child), bold_name);
					g_free (bold_name);
				}
			}

			path = g_strconcat("/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsMenuAdditions/", item_name, NULL);
			item = gtk_ui_manager_get_widget (parent_priv->ui_manager, path);
			g_free (path);

			if (item) {
				GtkWidget *child = gtk_bin_get_child (GTK_BIN (item));
				if (GTK_IS_LABEL (child)) {
					const gchar *cur_name = gtk_label_get_text (GTK_LABEL (child));
					gchar *bold_name = g_markup_printf_escaped("<b>%s</b>", cur_name);
					gtk_label_set_markup (GTK_LABEL (child), bold_name);
					g_free (bold_name);
				}
			}

			g_free(item_name);
		}

		modest_account_mgr_free_account_data (mgr, account_data);
	}

	if (priv->accounts_popup) {
		/* Mandatory in order to view the menu contents */
		gtk_widget_show_all (priv->accounts_popup);

		/* Setup tap_and_hold just if was not done before*/
		if (!gtk_menu_get_attach_widget (GTK_MENU (priv->accounts_popup)))
			gtk_widget_tap_and_hold_setup (send_receive_button, priv->accounts_popup, NULL, 0);
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


/* static gboolean */
/* on_delete_event (GtkWidget *widget, GdkEvent  *event, ModestMainWindow *self) */
/* { */
/* 	modest_window_save_state (MODEST_WINDOW(self)); */
/* 	return FALSE; */
/* } */

/* static void */
/* on_sendqueue_error_happened (TnySendQueue *self, TnyHeader *header, TnyMsg *msg, GError *err, ModestMainWindow *user_data) */
/* { */
/* 	if (err) { */
/* 		printf ("DEBUG: %s: err->code=%d, err->message=%s\n", __FUNCTION__, err->code, err->message); */

/* 		if (err->code == TNY_ACCOUNT_ERROR_TRY_CONNECT_USER_CANCEL) */
/* 			/\* Don't show waste the user's time by showing him a dialog telling him */
/* 			 * that he has just cancelled something: *\/ */
/* 			return; */
/* 	} */

/* 	/\* Get the server name: *\/ */
/* 	const gchar* server_name = NULL; */
	
/* 	TnyCamelTransportAccount* server_account = tny_camel_send_queue_get_transport_account ( */
/* 		TNY_CAMEL_SEND_QUEUE (self)); */
/* 	if (server_account) { */
/* 		server_name = tny_account_get_hostname (TNY_ACCOUNT (server_account)); */
			
/* 		g_object_unref (server_account); */
/* 		server_account = NULL; */
/* 	} */
	
/* 	if (!server_name) */
/* 		server_name = _("Unknown Server");	 */

/* 	/\* Show the appropriate message text for the GError: *\/ */
/* 	gchar *message = NULL; */
/* 	if (err) { */
/* 		switch (err->code) { */
/* 			case TNY_TRANSPORT_ACCOUNT_ERROR_SEND_HOST_LOOKUP_FAILED: */
/* 				message = g_strdup_printf (_("emev_ib_ui_smtp_server_invalid"), server_name); */
/* 				break; */
/* 			case TNY_TRANSPORT_ACCOUNT_ERROR_SEND_SERVICE_UNAVAILABLE: */
/* 				message = g_strdup_printf (_("emev_ib_ui_smtp_server_invalid"), server_name); */
/* 				break; */
/* 			case TNY_TRANSPORT_ACCOUNT_ERROR_SEND_AUTHENTICATION_NOT_SUPPORTED: */
/* 				/\* TODO: This logical ID seems more suitable for a wrong username or password than for a  */
/* 				 * wrong authentication method. The user is unlikely to guess at the real cause. */
/* 				 *\/ */
/* 				message = g_strdup_printf (_("eemev_ni_ui_smtp_authentication_fail_error"), server_name); */
/* 				break; */
/* 			case TNY_TRANSPORT_ACCOUNT_ERROR_SEND: */
/* 				/\* TODO: Tinymail is still sending this sometimes when it should  */
/* 				 * send TNY_ACCOUNT_ERROR_TRY_CONNECT_USER_CANCEL. *\/ */
/* 			default: */
/* 				message = g_strdup (_("emev_ib_ui_smtp_send_error")); */
/* 				break; */
/* 		} */
/* 	} else { */
/* 		message = g_strdup (_("emev_ib_ui_smtp_send_error")); */
/* 	} */
	
/* 	modest_maemo_show_information_note_and_forget (GTK_WINDOW (user_data), message); */
/* 	g_free (message); */
	
/* 	/\* TODO: Offer to remove the message, to avoid messages in future? *\/ */
/* 	/\* */
/* 	TnyFolder *outbox = tny_send_queue_get_outbox (queue); */
/* 	tny_folder_remove_msg (outbox, header, NULL); */
/* 	tny_folder_sync (outbox, TRUE, NULL); */
/* 	g_object_unref (outbox); */
/* 	*\/ */
/* } */

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

/* static void */
/* on_account_store_connecting_finished (TnyAccountStore *store, ModestMainWindow *self) */
/* { */
/* 	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self); */

/* 	/\* When going online, do the equivalent of pressing the send/receive button,  */
/* 	 * as per the specification: */
/* 	 * (without the check for >0 accounts, though that is not specified): *\/ */

/* 	TnyDevice *device = tny_account_store_get_device (store); */

/* 	/\* modest_folder_view_update_model (MODEST_FOLDER_VIEW (priv->folder_view), store); *\/ */
	
/* 	/\* Check that we are really online. */
/* 	 * This signal should not be emitted when we are not connected,  */
/* 	 * but it seems to happen sometimes: *\/ */
/* 	 if (!tny_device_is_online (device)) */
/* 	 	return; */
	 	
/* 	const gchar *iap_id = tny_maemo_conic_device_get_current_iap_id (TNY_MAEMO_CONIC_DEVICE (device)); */
/* 	printf ("DEBUG: %s: connection id=%s\n", __FUNCTION__, iap_id); */
	
/* 	/\* Stop the existing send queues: *\/ */
/* 	modest_runtime_remove_all_send_queues (); */
	
/* 	/\* Create the send queues again, using the appropriate transport accounts  */
/* 	 * for this new connection. */
/* 	 * This could be the first time that they are created if this is the first  */
/* 	 * connection. *\/ */
/* 	/\* TODO: Does this really destroy the TnySendQueues and their threads */
/* 	 * We do not want 2 TnySendQueues to exist with the same underlying  */
/* 	 * outbox directory. *\/ */

/* 	modest_main_window_cleanup_queue_error_signals (self); */

/* 	GSList *account_names = modest_account_mgr_account_names ( */
/* 		modest_runtime_get_account_mgr(),  */
/* 		TRUE /\* enabled accounts only *\/); */
/* 	GSList *iter = account_names; */
/* 	while (iter) { */
/* 		const gchar *account_name = (const gchar*)(iter->data); */
/* 			if (account_name) { */
/* 			TnyTransportAccount *account = TNY_TRANSPORT_ACCOUNT ( */
/* 				modest_tny_account_store_get_transport_account_for_open_connection */
/* 						 (modest_runtime_get_account_store(), account_name)); */
/* 			if (account) { */
/* 				/\* Q: Is this the first location where the send-queues are requested? *\/ */
/* 				QueueErrorSignal *esignal = g_slice_new (QueueErrorSignal); */
/* 				printf ("debug: %s:\n  Transport account for %s: %s\n", __FUNCTION__, account_name,  */
/* 					tny_account_get_id(TNY_ACCOUNT(account))); */
/* 				esignal->queue = TNY_SEND_QUEUE (modest_runtime_get_send_queue (account)); */
/* 				esignal->signal = g_signal_connect (G_OBJECT (esignal->queue), "error-happened", */
/* 					G_CALLBACK (on_sendqueue_error_happened), self); */
/* 				priv->queue_err_signals = g_list_prepend (priv->queue_err_signals, esignal); */
/* 			} */
/* 		} */
		
/* 		iter = g_slist_next (iter); */
/* 	} */

/* 	modest_account_mgr_free_account_names (account_names); */
/* 	account_names = NULL; */
	
/* 	modest_ui_actions_do_send_receive (NULL, MODEST_WINDOW (self)); */
/* } */

static void
_folder_view_csm_menu_activated (GtkWidget *widget, gpointer user_data)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (user_data));

	/* Update dimmed */	
	modest_window_check_dimming_rules_group (MODEST_WINDOW (user_data), "ModestMenuDimmingRules");	
}

static void
_header_view_csm_menu_activated (GtkWidget *widget, gpointer user_data)
{
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (user_data));

	/* Update visibility */

	/* Update dimmed */	
	modest_window_check_dimming_rules_group (MODEST_WINDOW (user_data), "ModestMenuDimmingRules");	
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
	GtkWidget *menu;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	/* folder view */
	
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT(priv->folder_view), "key-press-event",
						       G_CALLBACK(on_inner_widgets_key_pressed), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->folder_view), "folder_selection_changed",
						       G_CALLBACK(modest_main_window_on_folder_selection_changed), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->folder_view), "folder-display-name-changed",
						       G_CALLBACK(modest_ui_actions_on_folder_display_name_changed), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (priv->folder_view), "focus-in-event", 
						       G_CALLBACK (on_folder_view_focus_in), self);

	/* Folder view CSM */
	menu = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/FolderViewCSM");
	gtk_widget_tap_and_hold_setup (GTK_WIDGET (priv->folder_view), menu, NULL, 0);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, G_OBJECT(priv->folder_view), "tap-and-hold",
						       G_CALLBACK(_folder_view_csm_menu_activated),
						       self);
	/* header view */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "header_selected",
						       G_CALLBACK(modest_ui_actions_on_header_selected), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "header_activated",
						       G_CALLBACK(modest_ui_actions_on_header_activated), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "item_not_found",
						       G_CALLBACK(modest_ui_actions_on_item_not_found), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "key-press-event",
						       G_CALLBACK(on_inner_widgets_key_pressed), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "msg_count_changed",
						       G_CALLBACK(_on_msg_count_changed), self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (priv->header_view), "focus-in-event",
						       G_CALLBACK (on_header_view_focus_in), self);
	
	/* Header view CSM */
	menu = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/HeaderViewCSM");
	gtk_widget_tap_and_hold_setup (GTK_WIDGET (priv->header_view), menu, NULL, 0);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), "tap-and-hold",
						       G_CALLBACK(_header_view_csm_menu_activated),
						       self);
	
	/* window */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (self), "window-state-event",
			  G_CALLBACK (modest_main_window_window_state_event),
			  NULL);
	
	/* Mail Operation Queue */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (modest_runtime_get_mail_operation_queue ()),
						       "queue-changed", G_CALLBACK (on_queue_changed), self);
	
	/* Track changes in the device name */
	priv->notification_id =  modest_conf_listen_to_namespace (modest_runtime_get_conf (), 
								  MODEST_CONF_NAMESPACE);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(modest_runtime_get_conf ()),
						       "key_changed", G_CALLBACK (on_configuration_key_changed), 
						       self);
	
	/* Track account changes. We need to refresh the toolbar */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (modest_runtime_get_account_store ()),
						       "account_inserted", G_CALLBACK (on_account_inserted),
						       self);
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (modest_runtime_get_account_store ()),
						       "account_removed", G_CALLBACK (on_account_removed),
						       self);

	/* We need to refresh the send & receive menu to change the bold
	 * account when the default account changes. */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (modest_runtime_get_account_mgr ()),
	                                               "account_changed", G_CALLBACK (on_account_changed),
	                                               self);

	/* Account store */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,G_OBJECT (modest_runtime_get_account_store()), 
						       "password_requested",
						       G_CALLBACK (modest_ui_actions_on_password_requested), self);
}

#if 0
/** Idle handler, to send/receive at startup .*/
gboolean
sync_accounts_cb (ModestMainWindow *win)
{
	modest_ui_actions_do_send_receive (NULL, MODEST_WINDOW (win));
	return FALSE; /* Do not call this idle handler again. */
}
#endif

static void on_hildon_program_is_topmost_notify(GObject *self,
	GParamSpec *propert_param, gpointer user_data)
{
	HildonProgram *app = HILDON_PROGRAM (self);
	
	/*
	ModestWindow* self = MODEST_WINDOW(user_data);
	*/
	
	/* Note that use of hildon_program_set_can_hibernate() 
	 * is generally referred to as "setting the killable flag", 
	 * though hibernation does not seem equal to death.
	 * murrayc */
		 
	if (hildon_program_get_is_topmost (app)) {
		/* Prevent hibernation when the progam comes to the foreground,
		 * because hibernation should only happen when the application 
		 * is in the background: */
		hildon_program_set_can_hibernate (app, FALSE);
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
			modest_osso_save_state();
			hildon_program_set_can_hibernate (app, TRUE);
		}
	}
	
}

static void
modest_main_window_on_show (GtkWidget *self, gpointer user_data)
{
	GtkWidget *folder_win = (GtkWidget *) user_data;
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	priv->folder_view = MODEST_FOLDER_VIEW (modest_platform_create_folder_view (NULL));
	wrap_in_scrolled_window (folder_win, GTK_WIDGET(priv->folder_view));
/* 	wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET(priv->header_view)); */

	gtk_widget_show (GTK_WIDGET (priv->folder_view));

	/* Connect signals */
	connect_signals ((ModestMainWindow*)self);

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	/* Load previous osso state, for instance if we are being restored from 
	 * hibernation:  */
	modest_osso_load_state ();

	/* Restore window & widget settings */
	
	restore_settings (MODEST_MAIN_WINDOW(self), TRUE);

	/* The UI spec wants us to show a connection dialog when the application is 
	 * started by the user, if there is no connection.
	 * Do this before showing the account wizard, 
	 * because wizard needs a connection to discover capabilities. */
	 modest_platform_connect_and_wait (GTK_WINDOW (self), NULL);
	 
	/* Check if accounts exist and show the account wizard if not */
	gboolean accounts_exist = 
		modest_account_mgr_has_accounts(modest_runtime_get_account_mgr(), TRUE);

	if (!accounts_exist) {
		/* This is necessary to have the main window shown behind the dialog 
		It's an ugly hack... jschmid */
		gtk_widget_show_all(GTK_WIDGET(self));
		modest_ui_actions_on_accounts (NULL, MODEST_WINDOW(self));
	} else {
		GSList *accounts;
		GtkAction *send_receive_all;
		ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (self);
		accounts = modest_account_mgr_account_names (modest_runtime_get_account_mgr (), TRUE);
		send_receive_all = gtk_ui_manager_get_action (parent_priv->ui_manager, 
							      "/MenuBar/ToolsMenu/ToolsSendReceiveMainMenu/ToolsSendReceiveAllMenu");
		gtk_action_set_visible (send_receive_all, g_slist_length (accounts) > 1);
		modest_account_mgr_free_account_names (accounts);
	}
}

ModestWindow *
modest_main_window_new (void)
{
	ModestMainWindow *self = NULL;	
	ModestMainWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;
	GtkWidget *folder_win = NULL;
	ModestDimmingRulesGroup *menu_rules_group = NULL;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	GtkActionGroup *action_group = NULL;
	GError *error = NULL;
	ModestConf *conf = NULL;
	GtkAction *action = NULL;
	GdkPixbuf *window_icon;
	
	self  = MODEST_MAIN_WINDOW(g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	parent_priv->ui_manager = gtk_ui_manager_new();
	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();

	action_group = gtk_action_group_new ("ModestMainWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	menu_rules_group = modest_dimming_rules_group_new ("ModestMenuDimmingRules", FALSE);
	toolbar_rules_group = modest_dimming_rules_group_new ("ModestToolbarDimmingRules", TRUE);

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
					     modest_toggle_action_entries,
					     G_N_ELEMENTS (modest_toggle_action_entries),
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
					      self);
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_main_window_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_main_window_toolbar_dimming_entries),
					      self);

	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, menu_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	g_object_unref (menu_rules_group);
	g_object_unref (toolbar_rules_group);
	
	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (self), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));

	/* Menubar. Update the state of some toggles */
	parent_priv->menubar = modest_maemo_utils_menubar_to_menu (parent_priv->ui_manager);
	conf = modest_runtime_get_conf ();
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
					    "/MenuBar/ViewMenu/ViewShowToolbarMainMenu/ViewShowToolbarNormalScreenMenu");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      modest_conf_get_bool (conf, MODEST_CONF_SHOW_TOOLBAR, NULL));
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
					    "/MenuBar/ViewMenu/ViewShowToolbarMainMenu/ViewShowToolbarFullScreenMenu");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      modest_conf_get_bool (conf, MODEST_CONF_SHOW_TOOLBAR_FULLSCREEN, NULL));
	hildon_window_set_menu (HILDON_WINDOW (self), GTK_MENU (parent_priv->menubar));
	gtk_widget_show (parent_priv->menubar);

	/* Get device name */
	modest_maemo_utils_get_device_name ();

	/* header view */
	priv->header_view =
		MODEST_HEADER_VIEW (modest_header_view_new (NULL, MODEST_HEADER_VIEW_STYLE_DETAILS));
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
		 
	/* Create scrolled windows */
	folder_win = gtk_scrolled_window_new (NULL, NULL);
	priv->contents_widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (folder_win),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->contents_widget),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	/* gtk_widget_show (priv->contents_widget); */

	/* paned */
	priv->main_paned = gtk_hpaned_new ();
	gtk_paned_pack1 (GTK_PANED(priv->main_paned), folder_win, TRUE, TRUE);
	gtk_paned_pack2 (GTK_PANED(priv->main_paned), priv->contents_widget, TRUE, TRUE);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(priv->header_view));

	/* putting it all together... */
	priv->main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(priv->main_vbox), priv->main_paned, TRUE, TRUE,0);
	gtk_widget_show (priv->main_vbox);
	
	gtk_container_add (GTK_CONTAINER(self), priv->main_vbox);
	
	HildonProgram *app = hildon_program_get_instance ();
	hildon_program_add_window (app, HILDON_WINDOW (self));
	
	g_signal_connect (G_OBJECT(app), "notify::is-topmost",
		G_CALLBACK (on_hildon_program_is_topmost_notify), self);

	g_signal_connect (G_OBJECT(self), "show",
			  G_CALLBACK (modest_main_window_on_show), folder_win);
		
	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_ICON);
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (self), window_icon);
		g_object_unref (window_icon);
	}

	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */
	/* restore_settings (MODEST_MAIN_WINDOW(self), FALSE); */

	return MODEST_WINDOW(self);
}

gboolean 
modest_main_window_close_all (ModestMainWindow *self)
{
	GtkWidget *note;
	GtkResponseType response;

	/* Create the confirmation dialog MSG-NOT308 */
	note = hildon_note_new_confirmation_add_buttons (GTK_WINDOW (self),
							 _("emev_nc_close_windows"),
							 _("mcen_bd_yes"), GTK_RESPONSE_YES,
							 _("mcen_bd_no"), GTK_RESPONSE_NO,
							 NULL);

	response = gtk_dialog_run (GTK_DIALOG (note));
	gtk_widget_destroy (GTK_WIDGET (note));

	if (response == GTK_RESPONSE_YES)
		return TRUE;
	else
		return FALSE;
}


void 
modest_main_window_set_style (ModestMainWindow *self, 
			      ModestMainWindowStyle style)
{
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkAction *action;
	gboolean active;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	/* no change -> nothing to do */
	if (priv->style == style)
		return;

       /* Get toggle button and update the state if needed. This will
	  happen only when the set_style is not invoked from the UI,
	  for example when it's called from widget memory */
       action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToggleFolders");
       active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
       if ((active && style == MODEST_MAIN_WINDOW_STYLE_SIMPLE) ||
	   (!active && style == MODEST_MAIN_WINDOW_STYLE_SPLIT)) {
	       g_signal_handlers_block_by_func (action, modest_ui_actions_toggle_folders_view, self);
	       gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), !active);
	       g_signal_handlers_unblock_by_func (action, modest_ui_actions_toggle_folders_view, self);
       }

	priv->style = style;
	switch (style) {
	case MODEST_MAIN_WINDOW_STYLE_SIMPLE:
		/* Remove main paned */
		g_object_ref (priv->main_paned);
		gtk_container_remove (GTK_CONTAINER (priv->main_vbox), priv->main_paned);

		/* Reparent the contents widget to the main vbox */
		gtk_widget_reparent (priv->contents_widget, priv->main_vbox);

		break;
	case MODEST_MAIN_WINDOW_STYLE_SPLIT:
		/* Remove header view */
		g_object_ref (priv->contents_widget);
		gtk_container_remove (GTK_CONTAINER (priv->main_vbox), priv->contents_widget);

		/* Reparent the main paned */
		gtk_paned_add2 (GTK_PANED (priv->main_paned), priv->contents_widget);
		gtk_container_add (GTK_CONTAINER (priv->main_vbox), priv->main_paned);

		break;
	default:
		g_return_if_reached ();
	}

	/* Let header view grab the focus if it's being shown */
	if (priv->contents_style == MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS)
		gtk_widget_grab_focus (GTK_WIDGET (priv->header_view));
	else 
		gtk_widget_grab_focus (GTK_WIDGET (priv->contents_widget));

	/* Show changes */
	gtk_widget_show_all (GTK_WIDGET (priv->main_vbox));
}

ModestMainWindowStyle
modest_main_window_get_style (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (self), -1);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	return priv->style;
}



static gboolean
modest_main_window_window_state_event (GtkWidget *widget, GdkEventWindowState *event, gpointer userdata)
{
	if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
		ModestWindowPrivate *parent_priv;
		ModestWindowMgr *mgr;
		gboolean is_fullscreen;
		GtkAction *fs_toggle_action;
		gboolean active;
		
		mgr = modest_runtime_get_window_mgr ();
		
		is_fullscreen = modest_window_mgr_get_fullscreen_mode (mgr);

		parent_priv = MODEST_WINDOW_GET_PRIVATE (widget);
		
		fs_toggle_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewToggleFullscreenMenu");
		active = (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (fs_toggle_action)))?1:0;
		if (is_fullscreen != active) {
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fs_toggle_action), is_fullscreen);
		}
	}

	return FALSE;

}

static void
set_homogeneous (GtkWidget *widget,
		 gpointer data)
{
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), TRUE);
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

		/* Set homogeneous toolbar */
		gtk_container_foreach (GTK_CONTAINER (parent_priv->toolbar), 
				       set_homogeneous, NULL);
	
		priv->progress_toolitem = GTK_WIDGET (gtk_tool_item_new ());
		priv->cancel_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarCancel");
		priv->refresh_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSendReceive");
		priv->sort_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSort");
 		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);

		/* Add ProgressBar (Transfer toolbar) */ 
		priv->progress_bar = modest_progress_bar_widget_new ();
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
		set_toolbar_mode (MODEST_MAIN_WINDOW(self), TOOLBAR_MODE_NORMAL);
	} else
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));

}

static void
on_account_inserted (TnyAccountStore *accoust_store,
                     TnyAccount *account,
                     gpointer user_data)
{
	update_menus (MODEST_MAIN_WINDOW (user_data));
}

static void
on_account_changed (ModestAccountMgr* mgr,
                    const gchar* account,
                    gpointer user_data)
{
	gchar *default_account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr ());

	/* Actually, we only want to know when another account has become
	 * the default account, but there is no default_account_changed
	 * signal in ModestAccountMgr. */
	if(strcmp(account, default_account) == 0)
		update_menus (MODEST_MAIN_WINDOW (user_data));

	g_free (default_account);
}

static void
on_account_removed (TnyAccountStore *accoust_store,
                     TnyAccount *account,
                     gpointer user_data)
{
	update_menus (MODEST_MAIN_WINDOW (user_data));
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

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (user_data);

	/* Do nothing if we're in SIMPLE style */
	if (priv->style == MODEST_MAIN_WINDOW_STYLE_SIMPLE)
		return FALSE;

	if (MODEST_IS_HEADER_VIEW (widget) && event->keyval == GDK_Left)
		gtk_widget_grab_focus (GTK_WIDGET (priv->folder_view));
	else if (MODEST_IS_FOLDER_VIEW (widget) && event->keyval == GDK_Right)
		gtk_widget_grab_focus (GTK_WIDGET (priv->header_view));

	return FALSE;
}

static void
set_alignment (GtkWidget *widget,
	       gpointer data)
{
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.0);
	gtk_misc_set_padding (GTK_MISC (widget), 0, 0);
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

static GtkWidget *
create_details_widget (GtkWidget *styled_widget, TnyAccount *account)
{
	/* TODO: Clean up this function. It's a mess, with lots of copy/paste. murrayc. */
	
	GtkWidget *vbox;
	GtkWidget *label_w;
	gchar *label;
	gchar *gray_color_markup;

	vbox = gtk_vbox_new (FALSE, 0);

	/* Obtain the secondary text color. We need a realized widget, that's why 
	   we get styled_widget from outside */
#ifndef MODEST_HAVE_HILDON0_WIDGETS
	GdkColor color;
	gtk_style_lookup_color (styled_widget->style, "SecondaryTextColor", &color);
	gray_color_markup = modest_text_utils_get_color_string (&color);
#else
	// gray_color_markup is freed below
	gray_color_markup = g_strdup ("#BBBBBB");
#endif	
	/* Account description: */
	
	if (modest_tny_account_is_virtual_local_folders (account)
		|| (modest_tny_account_is_memory_card_account (account))) {
		gchar *tmp;
		/* Local folders: */
	
		/* Get device name */
		gchar *device_name = NULL;
		if (modest_tny_account_is_virtual_local_folders (account))
			device_name = modest_conf_get_string (modest_runtime_get_conf(),
						      MODEST_CONF_DEVICE_NAME, NULL);
		else
			device_name = g_strdup (tny_account_get_name (account));
						      
		tmp = g_strdup_printf (_("mcen_fi_localroot_description"), ""); //TODO: Why the ""?
		label = g_markup_printf_escaped ("<span color='%s'>%s</span>%s",
						 gray_color_markup, tmp, device_name);
		g_free (tmp);
		label_w = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (label_w), label);
		gtk_box_pack_start (GTK_BOX (vbox), label_w, FALSE, FALSE, 0);
		g_free (device_name);
		g_free (label);
	} else {
		if(!strcmp (tny_account_get_id (account), MODEST_MMC_ACCOUNT_ID)) {
			gtk_box_pack_start (GTK_BOX (vbox), 
				gtk_label_new (tny_account_get_name (account)), 
				FALSE, FALSE, 0);
		} else {
			/* Other accounts, such as IMAP and POP: */
			
			GString *proto;
			gchar *tmp;
	
			/* Put proto in uppercase */
			proto = g_string_new (tny_account_get_proto (account));
			proto = g_string_ascii_up (proto);
			
			/* note: mcen_fi_localroot_description is something like "%s account"
			 * however, we should display "%s account: %s"... therefore, ugly tmp */
			tmp   = g_strdup_printf (_("mcen_fi_remoteroot_account"),proto->str);
			label = g_markup_printf_escaped ("<span color='%s'>%s:</span> %s", 
							 gray_color_markup, tmp, tny_account_get_name (account));
			g_free (tmp);

			label_w = gtk_label_new (NULL);
			gtk_label_set_markup (GTK_LABEL (label_w), label);
			gtk_box_pack_start (GTK_BOX (vbox), label_w, FALSE, FALSE, 0);
			g_string_free (proto, TRUE);
			g_free (label);
		}
	}

	/* Message count */
	TnyFolderStore *folder_store = TNY_FOLDER_STORE (account);
	label = g_markup_printf_escaped ("<span color='%s'>%s:</span> %d", 
					 gray_color_markup, _("mcen_fi_rootfolder_messages"), 
					 modest_tny_folder_store_get_message_count (folder_store));
	label_w = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label_w), label);
	gtk_box_pack_start (GTK_BOX (vbox), label_w, FALSE, FALSE, 0);
	g_free (label);

	/* Folder count */
	label = g_markup_printf_escaped ("<span color='%s'>%s</span>: %d", 
					 gray_color_markup, 
					 _("mcen_fi_rootfolder_folders"), 
					 modest_tny_folder_store_get_folder_count (folder_store));
	label_w = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label_w), label);
	gtk_box_pack_start (GTK_BOX (vbox), label_w, FALSE, FALSE, 0);
	g_free (label);

	/* Size / Date */
	if (modest_tny_account_is_virtual_local_folders (account)
		|| modest_tny_account_is_memory_card_account (account)) {

		gchar *size = modest_text_utils_get_display_size (
			modest_tny_folder_store_get_local_size (folder_store));
		
		label = g_markup_printf_escaped ("<span color='%s'>%s:</span> %s", 
						 gray_color_markup, _("mcen_fi_rootfolder_size"),
						 size);
		g_free (size);
		
		label_w = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (label_w), label);
		gtk_box_pack_start (GTK_BOX (vbox), label_w, FALSE, FALSE, 0);
		g_free (label);
	} else if (TNY_IS_ACCOUNT(folder_store)) {
		TnyAccount *account = TNY_ACCOUNT(folder_store);
		
		time_t last_updated;
		gchar *last_updated_string;
		/* Get last updated from configuration */
		last_updated = modest_account_mgr_get_int (modest_runtime_get_account_mgr (), 
							  tny_account_get_id (account), 
							  MODEST_ACCOUNT_LAST_UPDATED, 
							  TRUE);
		if (last_updated > 0) 
			last_updated_string = modest_text_utils_get_display_date(last_updated);
		else
			last_updated_string = g_strdup (_("mcen_va_never"));

		label = g_markup_printf_escaped ("<span color='%s'>%s:</span> %s", 
						 gray_color_markup, _("mcen_ti_lastupdated"), last_updated_string);
		label_w = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (label_w), label);
		gtk_box_pack_start (GTK_BOX (vbox), label_w, FALSE, FALSE, 0);
		g_free (last_updated_string);
		g_free (label);
	}

	g_free (gray_color_markup);

	/* Set alignment */
	gtk_container_foreach (GTK_CONTAINER (vbox), (GtkCallback) set_alignment, NULL);

	return vbox;
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
_on_msg_count_changed (ModestHeaderView *header_view,
		       TnyFolder *folder,
		       TnyFolderChange *change,
		       ModestMainWindow *main_window)
{
	printf ("DEBUG: %s\n", __FUNCTION__);
	gboolean folder_empty = FALSE;
	gboolean all_marked_as_deleted = FALSE;
	TnyFolderChangeChanged changed;	
	ModestMainWindowPrivate *priv;
	
	g_return_if_fail (MODEST_IS_MAIN_WINDOW (main_window));
	g_return_if_fail (TNY_IS_FOLDER(folder));
	g_return_if_fail (TNY_IS_FOLDER_CHANGE(change));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (main_window);
	
	changed = tny_folder_change_get_changed (change);
	
	/* If something changes */
	if ((changed) & TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT)
		folder_empty = (tny_folder_change_get_new_all_count (change) == 0);	
	else
		folder_empty = (tny_folder_get_all_count (TNY_FOLDER (folder)) == 0);
	
	printf ("DEBUG: %s: folder_empty=%d\n", __FUNCTION__, folder_empty);

/*  	Check header removed  (hide marked as DELETED headers) */
	if (changed & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS) {
		modest_header_view_refilter (MODEST_HEADER_VIEW(priv->header_view));
	}

	/* Check if all messages are marked to be deleted */
	all_marked_as_deleted = modest_header_view_is_empty (header_view);
	folder_empty = folder_empty || all_marked_as_deleted ;
	
	/* Set contents style of headers view */
	if (folder_empty)  {
		modest_main_window_set_contents_style (main_window,
						       MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY);
		gtk_widget_grab_focus (GTK_WIDGET (priv->folder_view));
	}
	else {
		modest_main_window_set_contents_style (main_window,
						       MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS);
	}	
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
		if (priv->contents_style == MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS)
			g_object_ref (content);
		else if (priv->contents_style == MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY) {
			g_object_ref (priv->empty_view);
			gtk_container_remove (GTK_CONTAINER (content), priv->empty_view);
		}
		
		gtk_container_remove (GTK_CONTAINER (priv->contents_widget), content);
	}

	priv->contents_style = style;

	switch (priv->contents_style) {
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS:
		wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET (priv->header_view));
		modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->contents_widget),
						      TRUE);
		break;
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS:
	{
		TnyFolderStore *selected_folderstore = 
			modest_folder_view_get_selected (priv->folder_view);
		if (TNY_IS_ACCOUNT (selected_folderstore)) {	
		  priv->details_widget = create_details_widget (GTK_WIDGET (self),
								TNY_ACCOUNT (selected_folderstore));

			wrap_in_scrolled_window (priv->contents_widget, 
					 priv->details_widget);
		}
		g_object_unref (selected_folderstore);
		modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->contents_widget),
						      FALSE);

		
		break;
	}
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_EMPTY:
		wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET (priv->empty_view));
		modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->contents_widget),
						      FALSE);
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
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	TnyAccount *account;

	if (!key || 
	    priv->notification_id != id ||
	    strcmp (key, MODEST_CONF_DEVICE_NAME))
		return;

	if (priv->contents_style != MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS)
		return;

	account = (TnyAccount *) modest_folder_view_get_selected (priv->folder_view);
	if (TNY_IS_ACCOUNT (account) &&
	    !strcmp (tny_account_get_id (account), MODEST_LOCAL_FOLDERS_ACCOUNT_ID)) {
		GList *children;
		GtkLabel *label;
		const gchar *device_name;
		gchar *new_text;
		
		/* Get label */
		children = gtk_container_get_children (GTK_CONTAINER (priv->details_widget));
		label = GTK_LABEL (children->data);
		
		device_name = modest_conf_get_string (modest_runtime_get_conf(),
						      MODEST_CONF_DEVICE_NAME, NULL);
		
		new_text = g_strdup_printf ("%s: %s",
					    _("mcen_fi_localroot_description"),
					    device_name);
		
		gtk_label_set_text (label, new_text);
		gtk_widget_show (GTK_WIDGET (label));
		
		g_free (new_text);
		g_list_free (children);
	}
	g_object_unref (account);
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
		if (priv->progress_toolitem) {
			gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
			gtk_widget_show (priv->progress_toolitem);
		}
		if (priv->progress_bar)
			gtk_widget_show (priv->progress_bar);

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

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;
	ModestMailOperationTypeOperation op_type;
	ModestToolBarModes mode;
	GSList *tmp;
	gboolean mode_changed = FALSE;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	       
	/* Get toolbar mode from operation id*/
	op_type = modest_mail_operation_get_type_operation (mail_op);
	switch (op_type) {
	case MODEST_MAIL_OPERATION_TYPE_RECEIVE:
	case MODEST_MAIL_OPERATION_TYPE_OPEN:
		mode = TOOLBAR_MODE_TRANSFER;
		if (priv->current_toolbar_mode == TOOLBAR_MODE_NORMAL)
			mode_changed = TRUE;
		break;
	default:
		mode = TOOLBAR_MODE_NORMAL;
		
	}
		
		       
	/* Add operation observers and change toolbar if neccessary*/
	tmp = priv->progress_widgets;
	switch (type) {
	case MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED:
		if (mode == TOOLBAR_MODE_TRANSFER) {
			if (mode_changed) {
				set_toolbar_transfer_mode(self);		    
			}
			while (tmp) {
				modest_progress_object_add_operation (MODEST_PROGRESS_OBJECT (tmp->data),
								      mail_op);
				tmp = g_slist_next (tmp);
			}
		}
		break;
	case MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED:
		/* Change toolbar mode */
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

		break;
	}	

}

static void
set_account_visible(ModestMainWindow *self, const gchar *acc_name)
{
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* Get account data */
	ModestAccountMgr *mgr = modest_runtime_get_account_mgr ();
	ModestAccountData *acc_data = modest_account_mgr_get_account_data (mgr, acc_name);

	/* Set the new visible & active account */
	if (acc_data && acc_data->store_account) { 
		modest_folder_view_set_account_id_of_visible_server_account (priv->folder_view,
									     acc_data->store_account->account_name);
		modest_window_set_active_account (MODEST_WINDOW (self), acc_data->account_name);
	}
	
	modest_folder_view_select_first_inbox_or_local (priv->folder_view);

	/* Free */
	if (acc_data)
		modest_account_mgr_free_account_data (mgr, acc_data);
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
		if (first_modest_name) {
			set_account_visible (self, first_modest_name);
			g_free (first_modest_name);
		}
	}
}

static void 
on_show_account_action_activated  (GtkAction *action,
				   gpointer user_data)
{
	ModestMainWindow *self = MODEST_MAIN_WINDOW (user_data);

	const gchar *acc_name = gtk_action_get_name (action);
	set_account_visible (self, acc_name);
}

static void
refresh_account (const gchar *account_name)
{
	ModestWindow *win;

	win = MODEST_WINDOW (modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ()));

	/* If account_name == NULL, we must update all (option All) */
	if (!account_name)
		modest_ui_actions_do_send_receive_all (win);
	else
		modest_ui_actions_do_send_receive (account_name, win);
	
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

	hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_cannot_zoom_here"));
	return FALSE;

}

static gboolean
on_folder_view_focus_in (GtkWidget *widget,
			 GdkEventFocus *event,
			 gpointer userdata)
{
	ModestMainWindow *main_window = NULL;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (userdata), FALSE);
	main_window = MODEST_MAIN_WINDOW (userdata);
	
	/* Update toolbar dimming state */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (main_window));

	return FALSE;
}

static gboolean
on_header_view_focus_in (GtkWidget *widget,
			 GdkEventFocus *event,
			 gpointer userdata)
{
	ModestMainWindow *main_window = NULL;
	ModestMainWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (userdata), FALSE);
	main_window = MODEST_MAIN_WINDOW (userdata);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (main_window);

	/* Update toolbar dimming state */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (main_window));

	return FALSE;
}

static void 
modest_main_window_on_folder_selection_changed (ModestFolderView *folder_view,
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
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMCut");
	gtk_action_set_visible (action, show_clipboard);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMCopy");
	gtk_action_set_visible (action, show_clipboard);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMPaste");
	gtk_action_set_visible (action, show_clipboard);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/HeaderViewCSM/HeaderViewCSMDelete");
	gtk_action_set_visible (action, show_delete);

	/* We finally call to the ui actions handler, after updating properly
	 * the header view CSM */
	modest_ui_actions_on_folder_selection_changed (folder_view, folder_store, selected, main_window);

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


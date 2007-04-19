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

#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-note.h>

#include <glib/gi18n.h>
#include <gtk/gtktreeviewcolumn.h>
#include <tny-account-store-view.h>
#include <tny-simple-list.h>

#include <string.h>

#include "widgets/modest-main-window.h"
#include "widgets/modest-msg-edit-window.h"
#include "widgets/modest-account-view-window.h"
#include "modest-runtime.h"
#include "modest-account-mgr-helpers.h"
#include "modest-platform.h"
#include "modest-widget-memory.h"
#include "modest-window-priv.h"
#include "modest-main-window-ui.h"
#include "modest-account-mgr.h"
#include "modest-conf.h"
#include <modest-maemo-utils.h>
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg.h"
#include "modest-mail-operation.h"
#include "modest-icon-names.h"

/* 'private'/'protected' functions */
static void modest_main_window_class_init    (ModestMainWindowClass *klass);
static void modest_main_window_init          (ModestMainWindow *obj);
static void modest_main_window_finalize      (GObject *obj);
static gboolean modest_main_window_window_state_event (GtkWidget *widget, 
							   GdkEventWindowState *event, 
							   gpointer userdata);

static void connect_signals (ModestMainWindow *self);
static void restore_sizes (ModestMainWindow *self);
static void save_sizes (ModestMainWindow *self);

static void modest_main_window_show_toolbar   (ModestWindow *window,
					       gboolean show_toolbar);

static void on_account_update                 (TnyAccountStore *account_store, 
					       gchar *account_name,
					       gpointer user_data);

static gboolean on_inner_widgets_key_pressed  (GtkWidget *widget,
					       GdkEventKey *event,
					       gpointer user_data);

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
	GtkWidget *accounts_popup;

	ModestHeaderView *header_view;
	ModestFolderView *folder_view;

	ModestMainWindowStyle style;
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

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

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
	ModestWindowClass *modest_window_class;

	modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_main_window_finalize;

	modest_window_class->show_toolbar_func = modest_main_window_show_toolbar;

	g_type_class_add_private (gobject_class, sizeof(ModestMainWindowPrivate));
}

static void
modest_main_window_init (ModestMainWindow *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	priv->msg_paned    = NULL;
	priv->main_paned   = NULL;	
	priv->main_vbox    = NULL;
	priv->header_view  = NULL;
	priv->folder_view  = NULL;
	priv->accounts_popup  = NULL;
	priv->style  = MODEST_MAIN_WINDOW_STYLE_SPLIT;
}

static void
modest_main_window_finalize (GObject *obj)
{
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
restore_sizes (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	conf = modest_runtime_get_conf ();
	
	modest_widget_memory_restore (conf, G_OBJECT(priv->main_paned),
				      "modest-main-paned");
	modest_widget_memory_restore (conf, G_OBJECT(priv->header_view),
				      "header-view");
	modest_widget_memory_restore (conf, G_OBJECT(self), 
				      "modest-main-window");
}


static void
save_sizes (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;
		
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_runtime_get_conf ();
	
	modest_widget_memory_save (conf,G_OBJECT(self), "modest-main-window");
	modest_widget_memory_save (conf, G_OBJECT(priv->main_paned),
				   "modest-main-paned");
	modest_widget_memory_save (conf, G_OBJECT(priv->header_view), "header-view");
}

static GtkWidget*
wrapped_in_scrolled_window (GtkWidget *widget, gboolean needs_viewport)
{
	GtkWidget *win;

	win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy
		(GTK_SCROLLED_WINDOW (win),GTK_POLICY_NEVER,
		 GTK_POLICY_AUTOMATIC);
	
	if (needs_viewport)
		gtk_scrolled_window_add_with_viewport
			(GTK_SCROLLED_WINDOW(win), widget);
	else
		gtk_container_add (GTK_CONTAINER(win),
				   widget);

	return win;
}


static gboolean
on_delete_event (GtkWidget *widget, GdkEvent  *event, ModestMainWindow *self)
{
	save_sizes (self);
	return FALSE;
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
	g_signal_connect (G_OBJECT(priv->folder_view), "key-press-event",
			  G_CALLBACK(on_inner_widgets_key_pressed), self);
	g_signal_connect (G_OBJECT(priv->folder_view), "folder_selection_changed",
			  G_CALLBACK(modest_ui_actions_on_folder_selection_changed), self);
	g_signal_connect (G_OBJECT(priv->folder_view), "folder-display-name-changed",
			  G_CALLBACK(modest_ui_actions_on_folder_display_name_changed), self);

	menu = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/FolderViewContextMenu");
	gtk_widget_tap_and_hold_setup (GTK_WIDGET (priv->folder_view), menu, NULL, 0);

	/* header view */
/* 	g_signal_connect (G_OBJECT(priv->header_view), "status_update", */
/* 			  G_CALLBACK(modest_ui_actions_on_header_status_update), self); */
	g_signal_connect (G_OBJECT(priv->header_view), "header_selected",
			  G_CALLBACK(modest_ui_actions_on_header_selected), self);
	g_signal_connect (G_OBJECT(priv->header_view), "header_activated",
			  G_CALLBACK(modest_ui_actions_on_header_activated), self);
	g_signal_connect (G_OBJECT(priv->header_view), "item_not_found",
			  G_CALLBACK(modest_ui_actions_on_item_not_found), self);
	g_signal_connect (G_OBJECT(priv->header_view), "key-press-event",
			  G_CALLBACK(on_inner_widgets_key_pressed), self);

	/* window */
	g_signal_connect (G_OBJECT(self), "delete-event", G_CALLBACK(on_delete_event), self);
	g_signal_connect (G_OBJECT (self), "window-state-event",
			  G_CALLBACK (modest_main_window_window_state_event),
			  NULL);
	g_signal_connect (G_OBJECT(self), "delete-event", G_CALLBACK(on_delete_event), self);

	/* Track account changes. We need to refresh the toolbar */
	g_signal_connect (G_OBJECT (modest_runtime_get_account_store ()),
			  "account_update",
			  G_CALLBACK (on_account_update),
			  self);
}


gboolean
sync_accounts_cb (ModestMainWindow *win)
{
	modest_ui_actions_on_send_receive (NULL, MODEST_WINDOW(win));
	return FALSE;
}


ModestWindow*
modest_main_window_new (void)
{
	ModestMainWindow *self;	
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkWidget *header_win, *folder_win;
	GtkActionGroup *action_group;
	GError *error = NULL;
	TnyFolderStoreQuery *query;
	GdkPixbuf *window_icon;
	ModestConf *conf;
	GtkAction *action;

	self  = MODEST_MAIN_WINDOW(g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMainWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
				      self);

	gtk_action_group_add_toggle_actions (action_group,
					     modest_toggle_action_entries,
					     G_N_ELEMENTS (modest_toggle_action_entries),
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

	/* folder view */
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL,
					 TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	priv->folder_view = MODEST_FOLDER_VIEW(modest_folder_view_new (query));
	if (!priv->folder_view)
		g_printerr ("modest: cannot instantiate folder view\n");	
	g_object_unref (G_OBJECT (query));	
	modest_maemo_utils_get_device_name ();

	/* header view */
	priv->header_view  =
		MODEST_HEADER_VIEW(modest_header_view_new (NULL, MODEST_HEADER_VIEW_STYLE_DETAILS));
	if (!priv->header_view)
		g_printerr ("modest: cannot instantiate header view\n");
	modest_header_view_set_style (priv->header_view, MODEST_HEADER_VIEW_STYLE_TWOLINES);
	
	folder_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->folder_view), FALSE);
	header_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->header_view), FALSE);

	/* paned */
	priv->main_paned = gtk_hpaned_new ();
	gtk_paned_add1 (GTK_PANED(priv->main_paned), folder_win);
	gtk_paned_add2 (GTK_PANED(priv->main_paned), header_win);
	gtk_widget_show (GTK_WIDGET(priv->header_view));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(priv->header_view));

	/* putting it all together... */
	priv->main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(priv->main_vbox), priv->main_paned, TRUE, TRUE,0);

	gtk_container_add (GTK_CONTAINER(self), priv->main_vbox);
	restore_sizes (MODEST_MAIN_WINDOW(self));

	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_ICON);
	gtk_window_set_icon (GTK_WINDOW (self), window_icon);

	/* Connect signals */
	connect_signals (self);

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));
	g_idle_add ((GSourceFunc)sync_accounts_cb, self);
	/* do send & receive when we are idle */	

	g_message ("online? %s",
		   tny_device_is_online (modest_runtime_get_device()) ? "yes" : "no");
		
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
							 _("mcen_db_yes"), GTK_RESPONSE_YES,
							 _("mcen_db_no"), GTK_RESPONSE_NO,
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
	GtkWidget *scrolled_win;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* no change -> nothing to do */
	if (priv->style == style)
		return;

	priv->style = style;
	scrolled_win = gtk_widget_get_parent (GTK_WIDGET (priv->header_view));

	switch (style) {
	case MODEST_MAIN_WINDOW_STYLE_SIMPLE:
		/* Remove main paned */
		g_object_ref (priv->main_paned);
		gtk_container_remove (GTK_CONTAINER (priv->main_vbox), priv->main_paned);

		/* Reparent header view with scrolled window */
		gtk_widget_reparent (scrolled_win, priv->main_vbox);

		break;
	case MODEST_MAIN_WINDOW_STYLE_SPLIT:
		/* Remove header view */
		g_object_ref (scrolled_win);
		gtk_container_remove (GTK_CONTAINER (priv->main_vbox), scrolled_win);

		/* Reparent the main paned */
		gtk_paned_add2 (GTK_PANED (priv->main_paned), scrolled_win);
		gtk_container_add (GTK_CONTAINER (priv->main_vbox), priv->main_paned);
		break;
	default:
		g_return_if_reached ();
	}

	/* Let header view grab the focus */
	gtk_widget_grab_focus (GTK_WIDGET (priv->header_view));

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
	ModestWindowPrivate *parent_priv;
	GtkWidget *reply_button = NULL, *menu = NULL;
	
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	if (!parent_priv->toolbar && show_toolbar) {
		parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
								  "/ToolBar");

		/* Set homogeneous toolbar */
		gtk_container_foreach (GTK_CONTAINER (parent_priv->toolbar), 
				       set_homogeneous, NULL);

		/* Add to window */
		hildon_window_add_toolbar (HILDON_WINDOW (self), 
					   GTK_TOOLBAR (parent_priv->toolbar));

		/* Set reply button tap and hold menu */
		reply_button = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
							  "/ToolBar/ToolbarMessageReply");
		menu = gtk_ui_manager_get_widget (parent_priv->ui_manager,
						  "/ToolbarReplyContextMenu");
		gtk_widget_tap_and_hold_setup (GTK_WIDGET (reply_button), menu, NULL, 0);

		/* Set send & receive button tap and hold menu */
		on_account_update (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
				   NULL, self);
	}


	if (show_toolbar)
		gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
	else
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
}

/*
 * TODO: modify the menu dinamically. Add handlers to each item of the
 * menu when created
 */
static void 
on_account_update (TnyAccountStore *account_store, 
		   gchar *accout_name,
		   gpointer user_data)
{
	ModestMainWindow *self;
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	TnyList *account_list;
	GtkWidget *item, *send_receive_button;
	TnyIterator *iter;
	ModestAccountMgr *mgr;
	gchar *default_account;

	self = MODEST_MAIN_WINDOW (user_data);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (self);

	/* If there is no toolbar then exit */
	if (!parent_priv->toolbar)
		return;

	if (priv->accounts_popup)
		gtk_menu_detach (GTK_MENU (priv->accounts_popup));

	/* Get accounts */
	account_list = tny_simple_list_new ();
	tny_account_store_get_accounts (account_store, 
					account_list, 
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	/* If there is only one account do not show any menu */
	if (tny_list_get_length (account_list) <= 1)
		goto free;
	
	/* Get send receive button */
	send_receive_button = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
							  "/ToolBar/ToolbarSendReceive");

	/* Create the menu */
	priv->accounts_popup = gtk_menu_new ();
	item = gtk_menu_item_new_with_label (_("mcen_me_toolbar_sendreceive_all"));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));
	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));

	iter = tny_list_create_iterator (account_list);
	mgr = modest_runtime_get_account_mgr ();
	default_account = modest_account_mgr_get_default_account (mgr);

	do {
		TnyAccount *acc;
		const gchar *acc_name;
		gchar *display_name;

		/* Create tool item */
		acc = TNY_ACCOUNT (tny_iterator_get_current (iter));
		acc_name = tny_account_get_name (acc);

		/* Create display name */
		if (!strcmp (default_account, acc_name))
			display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_default"), acc_name);
		else
			display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_mailbox_n"), acc_name);

		item = gtk_menu_item_new_with_label (display_name);

		/* Free */
		g_free (display_name);
		g_object_unref (acc);

		/* Append item */
		gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));

		/* Go to next */
		tny_iterator_next (iter);

	} while (!tny_iterator_is_done (iter));

	g_object_unref (iter);

	/* Mandatory in order to view the menu contents */
	gtk_widget_show_all (priv->accounts_popup);

	/* Setup tap_and_hold */
	gtk_widget_tap_and_hold_setup (send_receive_button, priv->accounts_popup, NULL, 0);

 free:

	/* Free */
	g_object_unref (account_list);
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

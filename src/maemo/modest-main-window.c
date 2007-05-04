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
#include "modest-hildon-includes.h"
#include "modest-defs.h"
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
#include "modest-tny-account.h"
#include "modest-conf.h"
#include <modest-maemo-utils.h>
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg.h"
#include "modest-mail-operation.h"
#include "modest-icon-names.h"
#include "modest-progress-bar-widget.h"
#include <hildon-widgets/hildon-program.h>
#include "maemo/modest-osso-state-saving.h"

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

static void cancel_progressbar (GtkToolButton *toolbutton,
				ModestMainWindow *self);

static void         on_queue_changed                     (ModestMailOperationQueue *queue,
							  ModestMailOperation *mail_op,
							  ModestMailOperationQueueNotification type,
							  ModestMainWindow *self);

static void on_account_update                 (TnyAccountStore *account_store, 
					       gchar *account_name,
					       gpointer user_data);

static gboolean on_inner_widgets_key_pressed  (GtkWidget *widget,
					       GdkEventKey *event,
					       gpointer user_data);

static void on_configuration_key_changed      (ModestConf* conf, 
					       const gchar *key, 
					       ModestConfEvent event, 
					       ModestMainWindow *self);

static void 
set_toolbar_mode (ModestMainWindow *self, 
		  ModestToolBarModes mode);

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

	/* Progress observers */
	GtkWidget        *progress_bar;
	GSList           *progress_widgets;

	/* Tollbar items */
	GtkWidget   *progress_toolitem;
	GtkWidget   *cancel_toolitem;
	GtkWidget   *sort_toolitem;
	GtkWidget   *refresh_toolitem;
	ModestToolBarModes current_toolbar_mode;

	/* On-demand widgets */
	GtkWidget *accounts_popup;
	GtkWidget *details_widget;


	ModestHeaderView *header_view;
	ModestFolderView *folder_view;

	ModestMainWindowStyle style;
	ModestMainWindowContentsStyle contents_style;
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
static const GtkActionEntry modest_folder_view_action_entries [] = {

	/* Folder View CSM actions */
	{ "FolderViewCSMNewFolder", NULL, N_("FIXME: New Folder"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_new_folder) },
	{ "FolderViewCSMRenameFolder", NULL, N_("mcen_me_user_renamefolder"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_rename_folder) },
	{ "FolderViewCSMPasteMsgs", NULL, N_("FIXME: Paste"), NULL, NULL, NULL },
	{ "FolderViewCSMDeleteFolder", NULL, N_("FIXME: Delete"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_delete_folder) },
	{ "FolderViewCSMSearchMessages", NULL, N_("mcen_me_inbox_search"), NULL, NULL, NULL },
	{ "FolderViewCSMHelp", NULL, N_("mcen_me_inbox_help"), NULL, NULL, NULL },
};


static const GtkToggleActionEntry modest_main_window_toggle_action_entries [] = {
	{ "ToolbarToggleView", MODEST_STOCK_SPLIT_VIEW, N_("gqn_toolb_rss_fldonoff"), "<CTRL>t", NULL, G_CALLBACK (modest_ui_actions_toggle_folders_view), FALSE },
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
	priv->contents_widget  = NULL;
	priv->accounts_popup  = NULL;
	priv->details_widget  = NULL;

	priv->progress_widgets  = NULL;
	priv->progress_bar = NULL;
	priv->current_toolbar_mode = TOOLBAR_MODE_NORMAL;

	priv->style  = MODEST_MAIN_WINDOW_STYLE_SPLIT;
	priv->contents_style  = MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS;
}

static void
modest_main_window_finalize (GObject *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	g_slist_free (priv->progress_widgets);

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
				      MODEST_CONF_MAIN_PANED_KEY);
	modest_widget_memory_restore (conf, G_OBJECT(priv->header_view),
				      MODEST_CONF_HEADER_VIEW_KEY);
	modest_widget_memory_restore (conf, G_OBJECT(self), 
				      MODEST_CONF_MAIN_WINDOW_KEY);
}


static void
save_sizes (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;
		
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_runtime_get_conf ();
	
	modest_widget_memory_save (conf,G_OBJECT(self), 
				   MODEST_CONF_MAIN_WINDOW_KEY);
	modest_widget_memory_save (conf, G_OBJECT(priv->main_paned), 
				   MODEST_CONF_MAIN_PANED_KEY);
	modest_widget_memory_save (conf, G_OBJECT(priv->header_view), 
				   MODEST_CONF_HEADER_VIEW_KEY);
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


static gboolean
on_delete_event (GtkWidget *widget, GdkEvent  *event, ModestMainWindow *self)
{
	save_sizes (self);
	return FALSE;
}


static void
on_connection_changed (TnyDevice *device, gboolean online, ModestMainWindow *self)
{
	/* When going online, do the equivalent of pressing the send/receive button, 
	 * as per the specification:
	 * (without the check for >0 accounts, though that is not specified): */
	if (online) {
		do_send_receive (MODEST_WINDOW (self));
	}
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

	menu = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/FolderViewCSM");
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

	/* Mail Operation Queue */
	g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
			  "queue-changed",
			  G_CALLBACK (on_queue_changed),
			  self);

	/* Track changes in the device name */
	g_signal_connect (G_OBJECT(modest_runtime_get_conf ()),
			  "key_changed",
			  G_CALLBACK (on_configuration_key_changed), 
			  self);

	/* Track account changes. We need to refresh the toolbar */
	g_signal_connect (G_OBJECT (modest_runtime_get_account_store ()),
			  "account_update",
			  G_CALLBACK (on_account_update),
			  self);

	/* Device */
	g_signal_connect (G_OBJECT(modest_runtime_get_device()), "connection_changed",
			  G_CALLBACK(on_connection_changed), self);
}

/** Idle handler, to send/receive at startup .*/
gboolean
sync_accounts_cb (ModestMainWindow *win)
{
	do_send_receive (MODEST_WINDOW(win));
	return FALSE; /* Do not call this idle handler again. */
}

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
		gboolean settings_dialog_is_open = FALSE;
		
		if (settings_dialog_is_open)
			hildon_program_set_can_hibernate (app, FALSE);
		else {
			
			/* Allow hibernation, after saving the state: */
			modest_osso_save_state();
			hildon_program_set_can_hibernate (app, TRUE);
		}
	}
	
}



ModestWindow*
modest_main_window_new (void)
{
	ModestMainWindow *self;	
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkWidget *folder_win;
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

	gtk_action_group_add_actions (action_group,
				      modest_folder_view_action_entries,
				      G_N_ELEMENTS (modest_folder_view_action_entries),
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
	modest_folder_view_set_style (priv->folder_view,
				      MODEST_FOLDER_VIEW_STYLE_SHOW_ONE);

	/* Get device name */
	modest_maemo_utils_get_device_name ();

	/* header view */
	priv->header_view  =
		MODEST_HEADER_VIEW(modest_header_view_new (NULL, MODEST_HEADER_VIEW_STYLE_DETAILS));
	if (!priv->header_view)
		g_printerr ("modest: cannot instantiate header view\n");
	modest_header_view_set_style (priv->header_view, MODEST_HEADER_VIEW_STYLE_TWOLINES);
	
	/* Create scrolled windows */
	folder_win = gtk_scrolled_window_new (NULL, NULL);
	priv->contents_widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (folder_win),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->contents_widget),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);

	wrap_in_scrolled_window (folder_win, GTK_WIDGET(priv->folder_view));
	wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET(priv->header_view));

	/* paned */
	priv->main_paned = gtk_hpaned_new ();
	gtk_paned_add1 (GTK_PANED(priv->main_paned), folder_win);
	gtk_paned_add2 (GTK_PANED(priv->main_paned), priv->contents_widget);
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

	/* do send & receive when we are idle */
	g_idle_add ((GSourceFunc)sync_accounts_cb, self);
	

	HildonProgram *app = hildon_program_get_instance ();
	hildon_program_add_window (app, HILDON_WINDOW (self));
	
	/* Register HildonProgram  signal handlers: */
	/* These are apparently deprecated, according to the 
	 * "HildonApp/HildonAppView to HildonProgram/HildonWindow migration guide",
	 * though the API reference does not mention that:
	 *
	g_signal_connect (G_OBJECT(app), "topmost_status_lose",
		G_CALLBACK (on_hildon_program_save_state), self);
	g_signal_connect (G_OBJECT(app), "topmost_status_acquire",
		G_CALLBACK (on_hildon_program_status_acquire), self);
    */
	g_signal_connect (G_OBJECT(app), "notify::is-topmost",
		G_CALLBACK (on_hildon_program_is_topmost_notify), self);
		
	/* Load previous osso state, for instance if we are being restored from 
	 * hibernation:  */
	modest_osso_load_state();

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

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	/* no change -> nothing to do */
	if (priv->style == style)
		return;

	/* Get toggle button */
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarToggleView");

	priv->style = style;

	switch (style) {
	case MODEST_MAIN_WINDOW_STYLE_SIMPLE:
		/* Remove main paned */
		g_object_ref (priv->main_paned);
		gtk_container_remove (GTK_CONTAINER (priv->main_vbox), priv->main_paned);

		/* Reparent the contents widget to the main vbox */
		gtk_widget_reparent (priv->contents_widget, priv->main_vbox);

		g_signal_handlers_block_by_func (action, modest_ui_actions_toggle_folders_view, self);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
		g_signal_handlers_unblock_by_func (action, modest_ui_actions_toggle_folders_view, self);

		break;
	case MODEST_MAIN_WINDOW_STYLE_SPLIT:
		/* Remove header view */
		g_object_ref (priv->contents_widget);
		gtk_container_remove (GTK_CONTAINER (priv->main_vbox), priv->contents_widget);

		/* Reparent the main paned */
		gtk_paned_add2 (GTK_PANED (priv->main_paned), priv->contents_widget);
		gtk_container_add (GTK_CONTAINER (priv->main_vbox), priv->main_paned);

		g_signal_handlers_block_by_func (action, modest_ui_actions_toggle_folders_view, self);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
		g_signal_handlers_unblock_by_func (action, modest_ui_actions_toggle_folders_view, self);

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

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	if (!parent_priv->toolbar && show_toolbar) {
		parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
								  "/ToolBar");

		/* Set homogeneous toolbar */
		gtk_container_foreach (GTK_CONTAINER (parent_priv->toolbar), 
				       set_homogeneous, NULL);
	
		priv->progress_toolitem = GTK_WIDGET (gtk_tool_item_new ());
		priv->cancel_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarCancel");
		priv->refresh_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSendReceive");
		priv->sort_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarSort");
 		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->progress_toolitem), FALSE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), FALSE);
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
		on_account_update (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
				   NULL, self);
	}

	/* TODO: Why is this sometimes NULL? murrayc */
	if (parent_priv->toolbar) {
		if (show_toolbar) {
			/* Quick hack: this prevents toolbar icons "dance" when progress bar show status is changed */
			/* TODO: resize mode migth be GTK_RESIZE_QUEUE, in order to avoid unneccesary shows */
			gtk_container_set_resize_mode (GTK_CONTAINER(parent_priv->toolbar), GTK_RESIZE_IMMEDIATE);

			gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
			set_toolbar_mode (MODEST_MAIN_WINDOW(self), TOOLBAR_MODE_NORMAL);
		} else
			gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
	}
}

/*
 * TODO: modify the menu dynamically. Add handlers to each item of the
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

	if (priv->accounts_popup && gtk_menu_get_attach_widget (GTK_MENU (priv->accounts_popup)) ) {
		/* gtk_menu_detach will also unreference the popup, 
		 * so we can forget about this instance, and create a new one later:
		 */
		gtk_menu_detach (GTK_MENU (priv->accounts_popup));
		priv->accounts_popup = NULL;
	}

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
		TnyAccount *acc = NULL;
		const gchar *acc_name = NULL;

		/* Create tool item */
		acc = TNY_ACCOUNT (tny_iterator_get_current (iter));
		if (acc)
			acc_name = tny_account_get_name (acc);

		/* Create display name */
		gchar *display_name = NULL;
		if (acc_name) {
			if (default_account && !(strcmp (default_account, acc_name) == 0))
				display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_default"), acc_name);
			else
				display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_mailbox_n"), acc_name);
		}
		else
		{
			/* TODO: This probably should never happen: */
			display_name = g_strdup_printf (_("mcen_me_toolbar_sendreceive_default"), "");
		}
		

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

static void
set_alignment (GtkWidget *widget,
	       gpointer data)
{
	gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.0);
	gtk_misc_set_padding (GTK_MISC (widget), 0, 0);
}

static GtkWidget *
create_details_widget (TnyAccount *account)
{
	GtkWidget *vbox;
	gchar *label;

	vbox = gtk_vbox_new (FALSE, 0);

	/* Account description */
	if (!strcmp (tny_account_get_id (account), MODEST_LOCAL_FOLDERS_ACCOUNT_ID)) {
		gchar *device_name;

		/* Get device name */
		device_name = modest_conf_get_string (modest_runtime_get_conf(),
						      MODEST_CONF_DEVICE_NAME, NULL);
   
		label = g_strdup_printf ("%s: %s",
					 _("mcen_fi_localroot_description"),
					 device_name);
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (label), FALSE, FALSE, 0);
		g_free (device_name);
		g_free (label);
	} else if (!strcmp (tny_account_get_id (account), MODEST_MMC_ACCOUNT_ID)) {
		/* TODO: MMC ? */
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new ("FIXME: MMC ?"), FALSE, FALSE, 0);
	} else {
		GString *proto;

		/* Put proto in uppercase */
		proto = g_string_new (tny_account_get_proto (account));
		proto = g_string_ascii_up (proto);

		label = g_strdup_printf ("%s %s: %s", 
					 proto->str,
					 _("mcen_fi_remoteroot_account"),
					 tny_account_get_name (account));
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (label), FALSE, FALSE, 0);
		g_string_free (proto, TRUE);
		g_free (label);
	}

	/* Message count */
	label = g_strdup_printf ("%s: %d", _("mcen_fi_rootfolder_messages"), 
				 modest_tny_account_get_message_count (account));
	gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (label), FALSE, FALSE, 0);
	g_free (label);

	/* Folder count */
	label = g_strdup_printf ("%s: %d", _("mcen_fi_rootfolder_folders"), 
				 modest_tny_account_get_folder_count (account));
	gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (label), FALSE, FALSE, 0);
	g_free (label);

	/* Size / Date */
	if (!strcmp (tny_account_get_id (account), MODEST_LOCAL_FOLDERS_ACCOUNT_ID)) {
		/* FIXME: format size */
		label = g_strdup_printf ("%s: %d", _("mcen_fi_rootfolder_size"), 
					 modest_tny_account_get_local_size (account));
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (label), FALSE, FALSE, 0);
		g_free (label);
	} else if (!strcmp (tny_account_get_id (account), MODEST_MMC_ACCOUNT_ID)) {
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new ("FIXME: MMC ?"), FALSE, FALSE, 0);
	} else {
		label = g_strdup_printf ("%s: %s", _("mcen_ti_lastupdated"), "08/08/08");
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (label), FALSE, FALSE, 0);
		g_free (label);
	}

	/* Set alignment */
	gtk_container_foreach (GTK_CONTAINER (vbox), (GtkCallback) set_alignment, NULL);

	return vbox;
}

void 
modest_main_window_set_contents_style (ModestMainWindow *self, 
				       ModestMainWindowContentsStyle style)
{
	ModestMainWindowPrivate *priv;
	GtkWidget *content;
	TnyAccount *account;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* We allow to set the same content style than the previously
	   set if there are details, because it could happen when we're
	   selecting different accounts consecutively */
	if ((priv->contents_style == style) &&
	    (priv->contents_style == MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS))
		return;

	/* Remove previous child. Delete it if it was an account
	   details widget */
	content = gtk_bin_get_child (GTK_BIN (priv->contents_widget));
	if (priv->contents_style != MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS)
		g_object_ref (content);
	gtk_container_remove (GTK_CONTAINER (priv->contents_widget), content);

	priv->contents_style = style;

	switch (priv->contents_style) {
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_HEADERS:
		wrap_in_scrolled_window (priv->contents_widget, GTK_WIDGET (priv->header_view));
		break;
	case MODEST_MAIN_WINDOW_CONTENTS_STYLE_DETAILS:
		/* TODO: show here account details */
		account = TNY_ACCOUNT (modest_folder_view_get_selected (priv->folder_view));
		priv->details_widget = create_details_widget (account);

		wrap_in_scrolled_window (priv->contents_widget, 
					 priv->details_widget);
		break;
	default:
		g_return_if_reached ();
	}

	/* Show */
	gtk_widget_show_all (priv->contents_widget);
}

static void 
on_configuration_key_changed (ModestConf* conf, 
			      const gchar *key, 
			      ModestConfEvent event, 
			      ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;
	TnyAccount *account;

	if (!key || strcmp (key, MODEST_CONF_DEVICE_NAME))
		return;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

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
}

static void 
set_toolbar_mode (ModestMainWindow *self, 
		  ModestToolBarModes mode)
{
	ModestWindowPrivate *parent_priv;
	ModestMainWindowPrivate *priv;
	GtkAction *sort_action, *refresh_action, *cancel_action;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	sort_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarSort");
	refresh_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarSendReceive");
	cancel_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarCancel");

	/* Sets current toolbar mode */
	priv->current_toolbar_mode = mode;

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
		break;
	default:
		g_return_if_reached ();
	}

	gtk_widget_show_all (GTK_WIDGET (self));
}

static void
cancel_progressbar (GtkToolButton *toolbutton,
		    ModestMainWindow *self)
{
	GSList *tmp;
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* Get operation observers and cancel its current operation */
	tmp = priv->progress_widgets;
	while (tmp) {
		modest_progress_object_cancel_current_operation (MODEST_PROGRESS_OBJECT(tmp->data));
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
	ModestMailOperationId op_id;
	ModestToolBarModes mode;
	GSList *tmp;
	gboolean mode_changed = FALSE;

	g_return_if_fail (MODEST_IS_MAIN_WINDOW (self));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	       
	/* Get toolbar mode from operation id*/
	op_id = modest_mail_operation_get_id (mail_op);
	switch (op_id) {
	case MODEST_MAIL_OPERATION_ID_SEND:
	case MODEST_MAIL_OPERATION_ID_RECEIVE:
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
		if (mode_changed)
			set_toolbar_mode (self, mode);
		if (mode == TOOLBAR_MODE_TRANSFER) {
			while (tmp) {
				modest_progress_object_add_operation (MODEST_PROGRESS_OBJECT (tmp->data),
								      mail_op);
				tmp = g_slist_next (tmp);
			}
		}
		break;
	case MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED:
		if (mode == TOOLBAR_MODE_TRANSFER) {
			while (tmp) {
				modest_progress_object_remove_operation (MODEST_PROGRESS_OBJECT (tmp->data),
									 mail_op);
				tmp = g_slist_next (tmp);
			}
			
			/* If no more operations are being observed, NORMAL mode is enabled again */
			if (observers_empty (self))
				set_toolbar_mode (self, TOOLBAR_MODE_NORMAL);
		}
		break;
	}	
}

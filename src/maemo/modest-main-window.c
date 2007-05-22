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
#include "modest-text-utils.h"
#include "maemo/modest-osso-state-saving.h"

#ifdef MODEST_HILDON_VERSION_0
#include <hildon-widgets/hildon-program.h>
#else
#include <hildon/hildon-program.h>
#endif /*MODEST_HILDON_VERSION_0*/

#define MODEST_MAIN_WINDOW_ACTION_GROUP_ADDITIONS "ModestMainWindowActionAdditions"

/* 'private'/'protected' functions */
static void modest_main_window_class_init    (ModestMainWindowClass *klass);
static void modest_main_window_init          (ModestMainWindow *obj);
static void modest_main_window_finalize      (GObject *obj);
static gboolean modest_main_window_window_state_event (GtkWidget *widget, 
							   GdkEventWindowState *event, 
							   gpointer userdata);

static void connect_signals (ModestMainWindow *self);

static void restore_settings (ModestMainWindow *self);
static void save_state (ModestWindow *self);

static void modest_main_window_show_toolbar   (ModestWindow *window,
					       gboolean show_toolbar);

static void cancel_progressbar (GtkToolButton *toolbutton,
				ModestMainWindow *self);

static void         on_queue_changed                     (ModestMailOperationQueue *queue,
							  ModestMailOperation *mail_op,
							  ModestMailOperationQueueNotification type,
							  ModestMainWindow *self);

static void on_account_update                 (TnyAccountStore *account_store, 
					       const gchar *account_name,
					       gpointer user_data);

static gboolean on_inner_widgets_key_pressed  (GtkWidget *widget,
					       GdkEventKey *event,
					       gpointer user_data);

static void on_configuration_key_changed      (ModestConf* conf, 
					       const gchar *key, 
					       ModestConfEvent event, 
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

	/* Merge ids used to add/remove accounts to the ViewMenu*/
	GByteArray *merge_ids;

	/* On-demand widgets */
	GtkWidget *accounts_popup;
	GtkWidget *details_widget;

	/* Optimized view enabled */
	gboolean optimized_view;

	ModestHeaderView *header_view;
	ModestFolderView *folder_view;

	ModestMainWindowStyle style;
	ModestMainWindowContentsStyle contents_style;

	guint progress_bar_timeout;

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
	{ "FolderViewCSMPasteMsgs", NULL, N_("mcen_me_inbox_paste"), NULL, NULL, NULL },
	{ "FolderViewCSMDeleteFolder", NULL, N_("mcen_me_inbox_delete"), NULL, NULL, G_CALLBACK (modest_ui_actions_on_delete_folder) },
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
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_main_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMainWindowPrivate));
	
	modest_window_class->show_toolbar_func = modest_main_window_show_toolbar;
	modest_window_class->save_state_func = save_state;
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

	priv->merge_ids = NULL;

	priv->optimized_view  = FALSE;
	priv->progress_bar_timeout = 0;
}

static void
modest_main_window_finalize (GObject *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

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
restore_settings (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	conf = modest_runtime_get_conf ();
	
	modest_widget_memory_restore (conf, G_OBJECT(self), 
				      MODEST_CONF_MAIN_WINDOW_KEY);
	modest_widget_memory_restore (conf, G_OBJECT(priv->main_paned),
				      MODEST_CONF_MAIN_PANED_KEY);
	modest_widget_memory_restore (conf, G_OBJECT(priv->header_view),
				      MODEST_CONF_HEADER_VIEW_KEY);
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
	modest_window_save_state (MODEST_WINDOW(self));
	return FALSE;
}


static void
on_account_store_connecting_finished (TnyAccountStore *store, ModestMainWindow *self)
{
	/* When going online, do the equivalent of pressing the send/receive button, 
	 * as per the specification:
	 * (without the check for >0 accounts, though that is not specified): */
	modest_ui_actions_do_send_receive (NULL, MODEST_WINDOW (self));
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

	/* Account store */
	g_signal_connect (G_OBJECT (modest_runtime_get_account_store()), "password_requested",
			  G_CALLBACK (modest_ui_actions_on_password_requested), self);
			  
	/* Device */
	g_signal_connect (G_OBJECT(modest_runtime_get_account_store()), "connecting-finished",
			  G_CALLBACK(on_account_store_connecting_finished), self);
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

	/* Get device name */
	modest_maemo_utils_get_device_name ();

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
	restore_settings (MODEST_MAIN_WINDOW(self));

	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_ICON);
	gtk_window_set_icon (GTK_WINDOW (self), window_icon);
	
	/* Connect signals */
	connect_signals (self);

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	/* Do send & receive when we are idle */
	/* TODO: Enable this again. I have commented it out because, 
	 * at least in scratchbox, this can cause us to start a second 
	 * update (in response to a connection change) when we are already 
	 * doing an update (started here, at startup). Tinymail doesn't like that.
	 * murrayc.
	 */
	/* g_idle_add ((GSourceFunc)sync_accounts_cb, self); */
	
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
		on_account_update (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
				   NULL, self);
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

static gint
compare_display_names (ModestAccountData *a,
		       ModestAccountData *b)
{
	return strcmp (a->display_name, b->display_name);
}

static void 
on_account_update (TnyAccountStore *account_store, 
		   const gchar *account_name,
		   gpointer user_data)
{
	GSList *account_names, *iter, *accounts;
	ModestMainWindow *self;
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	ModestAccountMgr *mgr;
	gint i, num_accounts;					
	GtkActionGroup *action_group;
	GList *groups;
	gchar *default_account;
	GtkWidget *send_receive_button, *item;
		
	self = MODEST_MAIN_WINDOW (user_data);
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
	g_slist_free (account_names);

	/* Order the list of accounts by its display name */
	accounts = g_slist_sort (accounts, (GCompareFunc) compare_display_names);
	num_accounts = g_slist_length (accounts);

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
		gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));
	}

	/* Create a new action group */
	default_account = modest_account_mgr_get_default_account (mgr);
	action_group = gtk_action_group_new (MODEST_MAIN_WINDOW_ACTION_GROUP_ADDITIONS);
	for (i = 0; i < num_accounts; i++) {
		gchar *display_name = NULL;
		
		ModestAccountData *account_data = (ModestAccountData *) g_slist_nth_data (accounts, i);

		/* Create display name. The default account is shown differently */
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
		if(account_data->account_name) {
			gchar* item_name, *refresh_action_name;
			guint8 merge_id;
			GtkAction *view_account_action, *refresh_account_action;

			view_account_action = gtk_action_new (account_data->account_name,
							      display_name, NULL, NULL);
			gtk_action_group_add_action (action_group, view_account_action);

			/* Add ui from account data. We allow 2^9-1 account
			   changes in a single execution because we're
			   downcasting the guint to a guint8 in order to use a
			   GByteArray, it should be enough */
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
				item = gtk_menu_item_new_with_label (display_name);
				gtk_menu_shell_append (GTK_MENU_SHELL (priv->accounts_popup), GTK_WIDGET (item));
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
		modest_account_mgr_free_account_data (mgr, account_data);
	}
	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 1);

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
			last_updated_string = g_strdup (_("FIXME: Never"));

		label = g_strdup_printf ("%s: %s", _("mcen_ti_lastupdated"), last_updated_string);
		gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (label), FALSE, FALSE, 0);
		g_free (last_updated_string);
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

	g_return_if_fail (GTK_IS_TOOLBAR(parent_priv->toolbar)); 
	
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
	ModestMailOperationStatus status;

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
		if (mode == TOOLBAR_MODE_TRANSFER) {
			if (mode_changed)
				set_toolbar_transfer_mode(self);		    
			while (tmp) {
				modest_progress_object_add_operation (MODEST_PROGRESS_OBJECT (tmp->data),
								      mail_op);
				tmp = g_slist_next (tmp);
			}
		}
		break;
	case MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED:
		/* If mail_op is mine, check errors */
		status = modest_mail_operation_get_status (mail_op);
		if (status != MODEST_MAIL_OPERATION_STATUS_SUCCESS)
			modest_mail_operation_execute_error_handler (mail_op);

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
on_show_account_action_activated  (GtkAction *action,
				   gpointer user_data)
{
	ModestAccountData *acc_data;
	ModestMainWindow *self;
	ModestMainWindowPrivate *priv;
	ModestAccountMgr *mgr;
	const gchar *acc_name;

	self = MODEST_MAIN_WINDOW (user_data);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* Get account data */
	acc_name = gtk_action_get_name (action);
	mgr = modest_runtime_get_account_mgr ();
	acc_data = modest_account_mgr_get_account_data (mgr, acc_name);

	/* Set the new visible & active account */
	if (acc_data->store_account) { 
		modest_folder_view_set_account_id_of_visible_server_account (priv->folder_view,
									     acc_data->store_account->account_name);
		modest_window_set_active_account (MODEST_WINDOW (self), acc_data->account_name);
	}

	/* Free */
	modest_account_mgr_free_account_data (mgr, acc_data);
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

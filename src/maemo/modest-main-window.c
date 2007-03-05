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

#include <glib/gi18n.h>
#include <gtk/gtktreeviewcolumn.h>
#include <tny-account-store-view.h>
#include <modest-runtime.h>
#include <string.h>

#include <widgets/modest-main-window.h>
#include <widgets/modest-msg-edit-window.h>
#include <widgets/modest-account-view-window.h>


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

static void connect_signals (ModestMainWindow *self);
static void restore_sizes (ModestMainWindow *self);
static void save_sizes (ModestMainWindow *self);

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
	GtkWidget *progress_bar;

	ModestHeaderView *header_view;
	ModestFolderView *folder_view;

};


#define MODEST_MAIN_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_MAIN_WINDOW, \
                                                ModestMainWindowPrivate))

typedef struct _GetMsgAsyncHelper {
	ModestMainWindowPrivate *main_window_private;
	guint action;
	ModestMailOperationReplyType reply_type;
	ModestMailOperationForwardType forward_type;
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

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_main_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMainWindowPrivate));
}


static void
on_key_changed (ModestConf* conf, const gchar *key, ModestConfEvent event, ModestMainWindow *win)
{
	TnyAccount *account;
	
	if (!key || strcmp (key, MODEST_CONF_DEVICE_NAME) != 0)
		return; /* wrong key */
	
	/* ok, the device name changed; thus, we have to update the
	 * local folder account name*/
	account =
		modest_tny_account_store_get_tny_account_by_account (modest_runtime_get_account_store(),
								     MODEST_LOCAL_FOLDERS_ACCOUNT_ID,
								     TNY_ACCOUNT_TYPE_STORE);
	if (!account) {
		g_printerr ("modest: could not get account\n");
		return;
	}

	if (event == MODEST_CONF_EVENT_KEY_UNSET) 
		tny_account_set_name (account, MODEST_LOCAL_FOLDERS_DEFAULT_DISPLAY_NAME);
	else {
		gchar *device_name = modest_conf_get_string (modest_runtime_get_conf(),
							     MODEST_CONF_DEVICE_NAME, NULL);
		tny_account_set_name (account, device_name);
		g_free (device_name);
	}
	g_object_unref (G_OBJECT(account));	
}

static void
modest_main_window_init (ModestMainWindow *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	priv->msg_paned    = NULL;
	priv->main_paned   = NULL;	
	priv->header_view  = NULL;
	priv->folder_view  = NULL;

	/* progress bar */
	priv->progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(priv->progress_bar), 1.0);
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR(priv->progress_bar),
					PANGO_ELLIPSIZE_END);
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
	
	modest_widget_memory_restore (conf,G_OBJECT(self),
				      "modest-main-window");
	modest_widget_memory_restore (conf, G_OBJECT(priv->main_paned),
				      "modest-main-paned");
	modest_widget_memory_restore (conf, G_OBJECT(priv->header_view),
				      "header-view");
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


static GtkWidget*
get_toolbar (ModestMainWindow *self)
{
	GtkWidget   *toolbar, *progress_box, *progress_alignment;
	GtkToolItem *progress_item;
	ModestWindowPrivate *parent_priv;
	ModestMainWindowPrivate *priv;
	GtkWidget   *stop_icon;
	
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	/* Toolbar */
	toolbar             = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	progress_box        = gtk_hbox_new (FALSE, HILDON_MARGIN_DEFAULT);
	progress_alignment  = gtk_alignment_new (0.5, 0.5, 1, 0);
	
	gtk_container_add  (GTK_CONTAINER(progress_alignment), priv->progress_bar);
	gtk_box_pack_start (GTK_BOX(progress_box), progress_alignment, TRUE, TRUE, 0);
	
	progress_item  = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER(progress_item), progress_box);
	gtk_tool_item_set_homogeneous (progress_item, FALSE);
	gtk_tool_item_set_expand(progress_item, TRUE);
	
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), progress_item,
			    gtk_toolbar_get_n_items(GTK_TOOLBAR(toolbar)));

	stop_icon = gtk_image_new_from_icon_name("qgn_toolb_gene_stop", GTK_ICON_SIZE_BUTTON);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), gtk_tool_button_new(stop_icon, NULL),
			    gtk_toolbar_get_n_items(GTK_TOOLBAR(toolbar)));

	gtk_widget_show_all (toolbar);
	return toolbar;
}


static void
on_destroy (GtkWidget *widget, GdkEvent  *event, ModestMainWindow *self)
{
	gtk_main_quit();
}

static void
connect_signals (ModestMainWindow *self)
{	
	ModestWindowPrivate *parent_priv;
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	
	/* folder view */
	g_signal_connect (G_OBJECT(priv->folder_view), "folder_selection_changed",
			  G_CALLBACK(modest_ui_actions_on_folder_selection_changed), self);

	/* header view */
	g_signal_connect (G_OBJECT(priv->header_view), "status_update",
			  G_CALLBACK(modest_ui_actions_on_header_status_update), self);
	g_signal_connect (G_OBJECT(priv->header_view), "header_selected",
			  G_CALLBACK(modest_ui_actions_on_header_selected), self);
	g_signal_connect (G_OBJECT(priv->header_view), "header_activated",
			  G_CALLBACK(modest_ui_actions_on_header_activated), self);
	g_signal_connect (G_OBJECT(priv->header_view), "item_not_found",
			  G_CALLBACK(modest_ui_actions_on_item_not_found), self);

	/* window */
	g_signal_connect (G_OBJECT(self), "destroy", G_CALLBACK(on_destroy), NULL);
	g_signal_connect (G_OBJECT(self), "delete-event", G_CALLBACK(on_delete_event), self);

	
	/* modest_maemo_utils_get_device_name will probably change
	 * MODEST_CONF_DEVICE_NAME. If that happens, we update the local folders
	 * account name in the callback
	 */
	g_signal_connect (G_OBJECT(modest_runtime_get_conf()), "key_changed",
			  G_CALLBACK(on_key_changed), self);
	
	g_signal_connect (G_OBJECT(self), "delete-event", G_CALLBACK(on_delete_event), self);
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
	GtkWidget *main_vbox;
	GtkWidget *header_win, *folder_win;
	GtkActionGroup *action_group;
	GError *error = NULL;
	TnyFolderStoreQuery     *query;

	self  = MODEST_MAIN_WINDOW(g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL));
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMainWindowActions");

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
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

	/* add the toolbar */
	parent_priv->toolbar = get_toolbar(self);
	hildon_window_add_toolbar (HILDON_WINDOW (self), GTK_TOOLBAR (parent_priv->toolbar));

	/* Menubar */
	parent_priv->menubar = modest_maemo_utils_menubar_to_menu (parent_priv->ui_manager);
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
	main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_paned, TRUE, TRUE,0);

	gtk_container_add (GTK_CONTAINER(self), main_vbox);
	restore_sizes (MODEST_MAIN_WINDOW(self));	
	
	gtk_window_set_title (GTK_WINDOW(self), _("Modest"));
	gtk_window_set_icon_from_file (GTK_WINDOW(self), MODEST_APP_ICON, NULL);
	gtk_widget_show_all (main_vbox);

	/* should we hide the toolbar? */
	if (!modest_conf_get_bool (modest_runtime_get_conf (), MODEST_CONF_SHOW_TOOLBAR, NULL))
		gtk_widget_hide (parent_priv->toolbar);

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







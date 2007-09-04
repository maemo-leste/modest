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
#include <string.h>
#include <tny-account-store.h>
#include <tny-simple-list.h>
#include <tny-msg.h>
#include <tny-mime-part.h>
#include <tny-vfs-stream.h>
#include "modest-marshal.h"
#include "modest-platform.h"
#include <modest-maemo-utils.h>
#include <modest-tny-msg.h>
#include <modest-msg-view-window.h>
#include <modest-attachments-view.h>
#include <modest-main-window-ui.h>
#include "modest-msg-view-window-ui-dimming.h"
#include <modest-widget-memory.h>
#include <modest-runtime.h>
#include <modest-window-priv.h>
#include <modest-tny-folder.h>
#include <modest-text-utils.h>
#include <modest-account-mgr-helpers.h>
#include "modest-progress-bar-widget.h"
#include "modest-defs.h"
#include "modest-hildon-includes.h"
#include <gtkhtml/gtkhtml-search.h>
#include "modest-ui-dimming-manager.h"
#include <gdk/gdkkeysyms.h>
#include <modest-tny-account.h>

#define DEFAULT_FOLDER "MyDocs/.documents"

static void  modest_msg_view_window_class_init   (ModestMsgViewWindowClass *klass);
static void  modest_msg_view_window_init         (ModestMsgViewWindow *obj);
static void  modest_header_view_observer_init(
		ModestHeaderViewObserverIface *iface_class);
static void  modest_msg_view_window_finalize     (GObject *obj);
static void  modest_msg_view_window_toggle_find_toolbar (GtkToggleAction *obj,
							 gpointer data);
static void  modest_msg_view_window_find_toolbar_close (GtkWidget *widget,
							ModestMsgViewWindow *obj);
static void  modest_msg_view_window_find_toolbar_search (GtkWidget *widget,
							ModestMsgViewWindow *obj);

static void modest_msg_view_window_disconnect_signals (ModestWindow *self);
static void modest_msg_view_window_set_zoom (ModestWindow *window,
					     gdouble zoom);
static gdouble modest_msg_view_window_get_zoom (ModestWindow *window);
static gboolean modest_msg_view_window_zoom_minus (ModestWindow *window);
static gboolean modest_msg_view_window_zoom_plus (ModestWindow *window);
static gboolean modest_msg_view_window_key_release_event (GtkWidget *window,
							  GdkEventKey *event,
							  gpointer userdata);
static gboolean modest_msg_view_window_window_state_event (GtkWidget *widget, 
							   GdkEventWindowState *event, 
							   gpointer userdata);
static void modest_msg_view_window_scroll_up (ModestWindow *window);
static void modest_msg_view_window_scroll_down (ModestWindow *window);
static void modest_msg_view_window_update_priority (ModestMsgViewWindow *window);

static void modest_msg_view_window_show_toolbar   (ModestWindow *window,
						   gboolean show_toolbar);

static void modest_msg_view_window_clipboard_owner_change (GtkClipboard *clipboard,
							   GdkEvent *event,
							   ModestMsgViewWindow *window);
void modest_msg_view_window_on_row_changed(
		GtkTreeModel *header_model,
		GtkTreePath *arg1,
		GtkTreeIter *arg2,
		ModestMsgViewWindow *window);

void modest_msg_view_window_on_row_deleted(
		GtkTreeModel *header_model,
		GtkTreePath *arg1,
		ModestMsgViewWindow *window);

void modest_msg_view_window_on_row_inserted(
		GtkTreeModel *header_model,
		GtkTreePath *tree_path,
		GtkTreeIter *tree_iter,
		ModestMsgViewWindow *window);

void modest_msg_view_window_on_row_reordered(
		GtkTreeModel *header_model,
		GtkTreePath *arg1,
		GtkTreeIter *arg2,
		gpointer arg3,
		ModestMsgViewWindow *window);

void modest_msg_view_window_update_model_replaced(
		ModestHeaderViewObserver *window,
		GtkTreeModel *model,
		const gchar *tny_folder_id);

static void cancel_progressbar  (GtkToolButton *toolbutton,
				 ModestMsgViewWindow *self);

static void on_queue_changed    (ModestMailOperationQueue *queue,
				 ModestMailOperation *mail_op,
				 ModestMailOperationQueueNotification type,
				 ModestMsgViewWindow *self);

static void on_account_removed  (TnyAccountStore *account_store, 
				 TnyAccount *account,
				 gpointer user_data);

static void view_msg_cb         (ModestMailOperation *mail_op, 
				 TnyHeader *header, 
				 TnyMsg *msg, 
				 gpointer user_data);

static void set_toolbar_mode    (ModestMsgViewWindow *self, 
				 ModestToolBarModes mode);

static void update_window_title (ModestMsgViewWindow *window);

static gboolean set_toolbar_transfer_mode     (ModestMsgViewWindow *self); 


/* list my signals */
enum {
	MSG_CHANGED_SIGNAL,
	LAST_SIGNAL
};

static const GtkToggleActionEntry msg_view_toggle_action_entries [] = {
	{ "FindInMessage",    MODEST_TOOLBAR_ICON_FIND,    N_("qgn_toolb_gene_find"), NULL, NULL, G_CALLBACK (modest_msg_view_window_toggle_find_toolbar), FALSE },
	{ "ToolsFindInMessage", NULL, N_("mcen_me_viewer_find"), NULL, NULL, G_CALLBACK (modest_msg_view_window_toggle_find_toolbar), FALSE },
};

static const GtkRadioActionEntry msg_view_zoom_action_entries [] = {
	{ "Zoom50", NULL, N_("mcen_me_viewer_50"), NULL, NULL, 50 },
	{ "Zoom80", NULL, N_("mcen_me_viewer_80"), NULL, NULL, 80 },
	{ "Zoom100", NULL, N_("mcen_me_viewer_100"), NULL, NULL, 100 },
	{ "Zoom120", NULL, N_("mcen_me_viewer_120"), NULL, NULL, 120 },
	{ "Zoom150", NULL, N_("mcen_me_viewer_150"), NULL, NULL, 150 },
	{ "Zoom200", NULL, N_("mcen_me_viewer_200"), NULL, NULL, 200 }
};

typedef struct _ModestMsgViewWindowPrivate ModestMsgViewWindowPrivate;
struct _ModestMsgViewWindowPrivate {

	GtkWidget   *msg_view;
	GtkWidget   *main_scroll;
	GtkWidget   *find_toolbar;
	gchar       *last_search;

	/* Progress observers */
	GtkWidget        *progress_bar;
	GSList           *progress_widgets;

	/* Tollbar items */
	GtkWidget   *progress_toolitem;
	GtkWidget   *cancel_toolitem;
	GtkWidget   *prev_toolitem;
	GtkWidget   *next_toolitem;
	ModestToolBarModes current_toolbar_mode;

	/* Optimized view enabled */
	gboolean optimized_view;

	/* Whether this was created via the *_new_for_search_result() function. */
	gboolean is_search_result;
	
	/* A reference to the @model of the header view 
	 * to allow selecting previous/next messages,
	 * if the message is currently selected in the header view.
	 */
	const gchar *header_folder_id;
	GtkTreeModel *header_model;
	GtkTreeRowReference *row_reference;
	GtkTreeRowReference *next_row_reference;

	gulong clipboard_change_handler;
	gulong queue_change_handler;
	gulong account_removed_handler;
	gulong row_changed_handler;
	gulong row_deleted_handler;
	gulong row_inserted_handler;
	gulong rows_reordered_handler;

	guint purge_timeout;
	GtkWidget *remove_attachment_banner;

	guint progress_bar_timeout;

	gchar *msg_uid;
};

#define MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_MSG_VIEW_WINDOW, \
                                                    ModestMsgViewWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

GType
modest_msg_view_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMsgViewWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_msg_view_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMsgViewWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_msg_view_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
		                                  "ModestMsgViewWindow",
		                                  &my_info, 0);

		static const GInterfaceInfo modest_header_view_observer_info = 
		{
			(GInterfaceInitFunc) modest_header_view_observer_init,
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

		g_type_add_interface_static (my_type,
				MODEST_TYPE_HEADER_VIEW_OBSERVER,
				&modest_header_view_observer_info);
	}
	return my_type;
}

static void
save_state (ModestWindow *self)
{
	modest_widget_memory_save (modest_runtime_get_conf (),
				   G_OBJECT(self), 
				   MODEST_CONF_MSG_VIEW_WINDOW_KEY);
}


static void
restore_settings (ModestMsgViewWindow *self)
{
	modest_widget_memory_restore (modest_runtime_get_conf (),
				      G_OBJECT(self), 
				      MODEST_CONF_MSG_VIEW_WINDOW_KEY);
}

static void
modest_msg_view_window_class_init (ModestMsgViewWindowClass *klass)
{
	GObjectClass *gobject_class;
	ModestWindowClass *modest_window_class;
	gobject_class = (GObjectClass*) klass;
	modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_view_window_finalize;

	modest_window_class->set_zoom_func = modest_msg_view_window_set_zoom;
	modest_window_class->get_zoom_func = modest_msg_view_window_get_zoom;
	modest_window_class->zoom_minus_func = modest_msg_view_window_zoom_minus;
	modest_window_class->zoom_plus_func = modest_msg_view_window_zoom_plus;
	modest_window_class->show_toolbar_func = modest_msg_view_window_show_toolbar;
	modest_window_class->disconnect_signals_func = modest_msg_view_window_disconnect_signals;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewWindowPrivate));

	modest_window_class->save_state_func = save_state;

	signals[MSG_CHANGED_SIGNAL] =
		g_signal_new ("msg-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestMsgViewWindowClass, msg_changed),
			      NULL, NULL,
			      modest_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);
}

static void modest_header_view_observer_init(
		ModestHeaderViewObserverIface *iface_class)
{
	iface_class->update_func = modest_msg_view_window_update_model_replaced;
}

static void
modest_msg_view_window_init (ModestMsgViewWindow *obj)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);

	priv->is_search_result = FALSE;

	priv->msg_view      = NULL;
	priv->header_model  = NULL;
	priv->header_folder_id  = NULL;
	priv->clipboard_change_handler = 0;
	priv->queue_change_handler = 0;
	priv->account_removed_handler = 0;
	priv->row_changed_handler = 0;
	priv->row_deleted_handler = 0;
	priv->row_inserted_handler = 0;
	priv->rows_reordered_handler = 0;
	priv->current_toolbar_mode = TOOLBAR_MODE_NORMAL;

	priv->optimized_view  = FALSE;
	priv->progress_bar_timeout = 0;
	priv->purge_timeout = 0;
	priv->remove_attachment_banner = NULL;
	priv->msg_uid = NULL;
}


static gboolean
set_toolbar_transfer_mode (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv = NULL;
	
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	set_toolbar_mode (self, TOOLBAR_MODE_TRANSFER);
	
	if (priv->progress_bar_timeout > 0) {
		g_source_remove (priv->progress_bar_timeout);
		priv->progress_bar_timeout = 0;
	}
	
	return FALSE;
}

static void 
set_toolbar_mode (ModestMsgViewWindow *self, 
		  ModestToolBarModes mode)
{
	ModestWindowPrivate *parent_priv;
	ModestMsgViewWindowPrivate *priv;
/* 	GtkWidget *widget = NULL; */

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);
			
	/* Sets current toolbar mode */
	priv->current_toolbar_mode = mode;

	/* Update toolbar dimming state */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (self));

	switch (mode) {
	case TOOLBAR_MODE_NORMAL:		
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarMessageReply"); */
/* 		gtk_action_set_sensitive (widget, TRUE); */
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarDeleteMessage"); */
/* 		gtk_action_set_sensitive (widget, TRUE); */
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarMessageMoveTo"); */
/* 		gtk_action_set_sensitive (widget, TRUE); */
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/FindInMessage"); */
/* 		gtk_action_set_sensitive (widget, TRUE); */

		if (priv->prev_toolitem)
			gtk_widget_show (priv->prev_toolitem);
		
		if (priv->next_toolitem)
			gtk_widget_show (priv->next_toolitem);
			
		if (priv->progress_toolitem)
			gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), FALSE);
		if (priv->progress_bar)
			gtk_widget_hide (priv->progress_bar);
			
		if (priv->cancel_toolitem)
			gtk_widget_hide (priv->cancel_toolitem);

		/* Hide toolbar if optimized view is enabled */
		if (priv->optimized_view) {
			gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);
			gtk_widget_hide (GTK_WIDGET(parent_priv->toolbar));
		}

		break;
	case TOOLBAR_MODE_TRANSFER:
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarMessageReply"); */
/* 		gtk_action_set_sensitive (widget, FALSE); */
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarDeleteMessage"); */
/* 		gtk_action_set_sensitive (widget, FALSE); */
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarMessageMoveTo"); */
/* 		gtk_action_set_sensitive (widget, FALSE); */
/* 		widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/FindInMessage"); */
/* 		gtk_action_set_sensitive (widget, FALSE); */

		if (priv->prev_toolitem)
			gtk_widget_hide (priv->prev_toolitem);
		
		if (priv->next_toolitem)
			gtk_widget_hide (priv->next_toolitem);
		
		if (priv->progress_toolitem)
			gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
		if (priv->progress_bar)
			gtk_widget_show (priv->progress_bar);
			
		if (priv->cancel_toolitem)
			gtk_widget_show (priv->cancel_toolitem);

		/* Show toolbar if it's hiden (optimized view ) */
		if (priv->optimized_view) {
			gtk_widget_set_no_show_all (parent_priv->toolbar, FALSE);
			gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
		}

		break;
	default:
		g_return_if_reached ();
	}

}


static GtkWidget *
menubar_to_menu (GtkUIManager *ui_manager)
{
	GtkWidget *main_menu;
	GtkWidget *menubar;
	GList *iter, *children;

	/* Create new main menu */
	main_menu = gtk_menu_new();

	/* Get the menubar from the UI manager */
	menubar = gtk_ui_manager_get_widget (ui_manager, "/MenuBar");

	iter = children = gtk_container_get_children (GTK_CONTAINER (menubar));
	while (iter) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent(menu, main_menu);
		
		iter = g_list_next (iter);
	}
	g_list_free (children);
		     
	return main_menu;
}

static void
init_window (ModestMsgViewWindow *obj, TnyMsg *msg)
{
	GtkWidget *main_vbox;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	ModestConf *conf;
	GtkAction *action = NULL;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_view = modest_msg_view_new (msg);
	modest_msg_view_set_shadow_type (MODEST_MSG_VIEW (priv->msg_view), GTK_SHADOW_NONE);
	main_vbox = gtk_vbox_new  (FALSE, 6);

	/* Menubar */
	parent_priv->menubar = menubar_to_menu (parent_priv->ui_manager);
	conf = modest_runtime_get_conf ();
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
					    "/MenuBar/ViewMenu/ViewShowToolbarMenu/ViewShowToolbarNormalScreenMenu");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      modest_conf_get_bool (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR, NULL));
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
					    "/MenuBar/ViewMenu/ViewShowToolbarMenu/ViewShowToolbarFullScreenMenu");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      modest_conf_get_bool (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR_FULLSCREEN, NULL));
	hildon_window_set_menu    (HILDON_WINDOW(obj), GTK_MENU(parent_priv->menubar));
	gtk_widget_show (GTK_WIDGET(parent_priv->menubar));

	priv->main_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->main_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->main_scroll), GTK_SHADOW_NONE);
	modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->main_scroll), TRUE);

	gtk_container_add (GTK_CONTAINER (priv->main_scroll), priv->msg_view);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_scroll, TRUE, TRUE, 0);
	gtk_container_add   (GTK_CONTAINER(obj), main_vbox);

	priv->find_toolbar = hildon_find_toolbar_new (NULL);
	hildon_window_add_toolbar (HILDON_WINDOW (obj), GTK_TOOLBAR (priv->find_toolbar));
	gtk_widget_set_no_show_all (priv->find_toolbar, TRUE);
	g_signal_connect (G_OBJECT (priv->find_toolbar), "close", G_CALLBACK (modest_msg_view_window_find_toolbar_close), obj);
	g_signal_connect (G_OBJECT (priv->find_toolbar), "search", G_CALLBACK (modest_msg_view_window_find_toolbar_search), obj);
	
	priv->clipboard_change_handler = g_signal_connect (G_OBJECT (gtk_clipboard_get (GDK_SELECTION_PRIMARY)), "owner-change", G_CALLBACK (modest_msg_view_window_clipboard_owner_change), obj);
	gtk_widget_show_all (GTK_WIDGET(main_vbox));
}

static void
modest_msg_view_window_disconnect_signals (ModestWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	ModestHeaderView *header_view = NULL;
	ModestMainWindow *main_window = NULL;
	ModestWindowMgr *window_mgr = NULL;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	if (g_signal_handler_is_connected (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
					   priv->clipboard_change_handler)) 
		g_signal_handler_disconnect (gtk_clipboard_get (GDK_SELECTION_PRIMARY), 
					     priv->clipboard_change_handler);

	if (g_signal_handler_is_connected (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					   priv->queue_change_handler))
		g_signal_handler_disconnect (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					     priv->queue_change_handler);

	if (g_signal_handler_is_connected (G_OBJECT (modest_runtime_get_account_store ()), 
					   priv->account_removed_handler))
		g_signal_handler_disconnect (G_OBJECT (modest_runtime_get_account_store ()), 
					     priv->account_removed_handler);

	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->row_changed_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->row_changed_handler);

	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->row_deleted_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->row_deleted_handler);

	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->row_inserted_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->row_inserted_handler);

	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->rows_reordered_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->rows_reordered_handler);

	window_mgr = modest_runtime_get_window_mgr();
	g_assert(window_mgr != NULL);

	main_window = MODEST_MAIN_WINDOW(
			modest_window_mgr_get_main_window(window_mgr));
	
	if(main_window == NULL)
		return;

	header_view = MODEST_HEADER_VIEW(
			modest_main_window_get_child_widget(
			main_window, MODEST_WIDGET_TYPE_HEADER_VIEW));

	if (header_view == NULL)
		return;
	
	modest_header_view_remove_observer(header_view,
			MODEST_HEADER_VIEW_OBSERVER(self));
}	

static void
modest_msg_view_window_finalize (GObject *obj)
{
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_msg_view_window_disconnect_signals (MODEST_WINDOW (obj));

	if (priv->header_model != NULL) {
		g_object_unref (priv->header_model);
		priv->header_model = NULL;
	}

	if (priv->progress_bar_timeout > 0) {
		g_source_remove (priv->progress_bar_timeout);
		priv->progress_bar_timeout = 0;
	}

	if (priv->remove_attachment_banner) {
		gtk_widget_destroy (priv->remove_attachment_banner);
		g_object_unref (priv->remove_attachment_banner);
		priv->remove_attachment_banner = NULL;
	}

	if (priv->purge_timeout > 0) {
		g_source_remove (priv->purge_timeout);
		priv->purge_timeout = 0;
	}

	if (priv->row_reference) {
		gtk_tree_row_reference_free (priv->row_reference);
		priv->row_reference = NULL;
	}

	if (priv->next_row_reference) {
		gtk_tree_row_reference_free (priv->next_row_reference);
		priv->next_row_reference = NULL;
	}

	if (priv->msg_uid) {
		g_free (priv->msg_uid);
		priv->msg_uid = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static gboolean
select_next_valid_row (GtkTreeModel *model,
		       GtkTreeRowReference **row_reference,
		       gboolean cycle)
{
	GtkTreeIter tmp_iter;
	GtkTreePath *path, *next;
	gboolean retval = FALSE;

	g_return_val_if_fail (gtk_tree_row_reference_valid (*row_reference), FALSE);

	path = gtk_tree_row_reference_get_path (*row_reference);
	gtk_tree_model_get_iter (model, &tmp_iter, path);
	gtk_tree_row_reference_free (*row_reference);
	*row_reference = NULL;

	if (gtk_tree_model_iter_next (model, &tmp_iter)) {
		next = gtk_tree_model_get_path (model, &tmp_iter);
		*row_reference = gtk_tree_row_reference_new (model, next);
		retval = TRUE;
	} else if (cycle && gtk_tree_model_get_iter_first (model, &tmp_iter)) {
		next = gtk_tree_model_get_path (model, &tmp_iter);

		/* Ensure that we are not selecting the same */
		if (gtk_tree_path_compare (path, next) != 0) {
			*row_reference = gtk_tree_row_reference_new (model, next);
			retval = TRUE;
		}
	}

	/* Free */
	gtk_tree_path_free (path);

	return retval;
}

/* TODO: This should be in _init(), with the parameters as properties. */
static void
modest_msg_view_window_construct (ModestMsgViewWindow *self, TnyMsg *msg, 
			    const gchar *modest_account_name,
			    const gchar *msg_uid)
{
	GObject *obj = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;
	ModestDimmingRulesGroup *menu_rules_group = NULL;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	GtkActionGroup *action_group = NULL;
	GError *error = NULL;
	GdkPixbuf *window_icon;

	g_return_if_fail (msg);
	
	obj = G_OBJECT (self);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_uid = g_strdup (msg_uid);

	parent_priv->ui_manager = gtk_ui_manager_new();
	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();

	action_group = gtk_action_group_new ("ModestMsgViewWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	menu_rules_group = modest_dimming_rules_group_new ("ModestMenuDimmingRules", FALSE);
	toolbar_rules_group = modest_dimming_rules_group_new ("ModestToolbarDimmingRules", TRUE);

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
				      obj);
	gtk_action_group_add_toggle_actions (action_group,
					     modest_toggle_action_entries,
					     G_N_ELEMENTS (modest_toggle_action_entries),
					     obj);
	gtk_action_group_add_toggle_actions (action_group,
					     msg_view_toggle_action_entries,
					     G_N_ELEMENTS (msg_view_toggle_action_entries),
					     obj);
	gtk_action_group_add_radio_actions (action_group,
					    msg_view_zoom_action_entries,
					    G_N_ELEMENTS (msg_view_zoom_action_entries),
					    100,
					    G_CALLBACK (modest_ui_actions_on_change_zoom),
					    obj);

	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 0);
	g_object_unref (action_group);

	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager, MODEST_UIDIR "modest-msg-view-window-ui.xml",
					 &error);
	if (error) {
		g_printerr ("modest: could not merge modest-msg-view-window-ui.xml: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	}
	/* ****** */

	/* Add common dimming rules */
	modest_dimming_rules_group_add_rules (menu_rules_group, 
					      modest_msg_view_menu_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_menu_dimming_entries),
					      self);
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_msg_view_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_toolbar_dimming_entries),
					      self);

	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, menu_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	g_object_unref (menu_rules_group);
	g_object_unref (toolbar_rules_group);

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));
	
	/* Init window */
	init_window (MODEST_MSG_VIEW_WINDOW(obj), msg);
	restore_settings (MODEST_MSG_VIEW_WINDOW(obj));
	
	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_MSG_VIEW_ICON); 
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (obj), window_icon);
		g_object_unref (window_icon);
	}

 	/* g_signal_connect (G_OBJECT(obj), "delete-event", G_CALLBACK(on_delete_event), obj); */

	g_signal_connect (G_OBJECT(priv->msg_view), "link_clicked",
			  G_CALLBACK (modest_ui_actions_on_msg_link_clicked), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "link_hover",
			  G_CALLBACK (modest_ui_actions_on_msg_link_hover), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "attachment_clicked",
			  G_CALLBACK (modest_ui_actions_on_msg_attachment_clicked), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "recpt_activated",
			  G_CALLBACK (modest_ui_actions_on_msg_recpt_activated), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "link_contextual",
			  G_CALLBACK (modest_ui_actions_on_msg_link_contextual), obj);

	g_signal_connect (G_OBJECT (obj), "key-release-event",
			  G_CALLBACK (modest_msg_view_window_key_release_event),
			  NULL);

	g_signal_connect (G_OBJECT (obj), "window-state-event",
			  G_CALLBACK (modest_msg_view_window_window_state_event),
			  NULL);

	/* Mail Operation Queue */
	priv->queue_change_handler = g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
						       "queue-changed",
						       G_CALLBACK (on_queue_changed),
						       obj);

	/* Account manager */
	priv->account_removed_handler = g_signal_connect (G_OBJECT (modest_runtime_get_account_store ()),
	                                                  "account_removed",
	                                                  G_CALLBACK(on_account_removed),
	                                                  obj);

	modest_window_set_active_account (MODEST_WINDOW(obj), modest_account_name);

	priv->last_search = NULL;

	/* Init the clipboard actions dim status */
	modest_msg_view_grab_focus(MODEST_MSG_VIEW (priv->msg_view));

	update_window_title (MODEST_MSG_VIEW_WINDOW (obj));

	/* Check toolbar dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (obj));
	
}

ModestWindow *
modest_msg_view_window_new_with_header_model (TnyMsg *msg, 
					      const gchar *modest_account_name,
					      const gchar *msg_uid,
					      GtkTreeModel *model, 
					      GtkTreeRowReference *row_reference)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	TnyFolder *header_folder = NULL;
	ModestHeaderView *header_view = NULL;
	ModestMainWindow *main_window = NULL;
	ModestWindowMgr *window_mgr = NULL;

	window = g_object_new(MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);
	modest_msg_view_window_construct (window, msg, modest_account_name, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Remember the message list's TreeModel so we can detect changes 
	 * and change the list selection when necessary: */
	window_mgr = modest_runtime_get_window_mgr();
	g_assert(window_mgr != NULL);
	main_window = MODEST_MAIN_WINDOW(
			modest_window_mgr_get_main_window(window_mgr));
	g_assert(main_window != NULL);
	header_view = MODEST_HEADER_VIEW(modest_main_window_get_child_widget(
			main_window, MODEST_WIDGET_TYPE_HEADER_VIEW));
	if (header_view != NULL){
		header_folder = modest_header_view_get_folder(header_view);
		g_assert(header_folder != NULL);
		priv->header_folder_id = tny_folder_get_id(header_folder);
		g_assert(priv->header_folder_id != NULL);
		g_object_unref(header_folder);
	}

	priv->header_model = g_object_ref(model);
	priv->row_reference = gtk_tree_row_reference_copy (row_reference);
	priv->next_row_reference = gtk_tree_row_reference_copy (row_reference);
	select_next_valid_row (model, &(priv->next_row_reference), TRUE);

	priv->row_changed_handler = g_signal_connect(
			GTK_TREE_MODEL(model), "row-changed",
			G_CALLBACK(modest_msg_view_window_on_row_changed),
			window);
	priv->row_deleted_handler = g_signal_connect(
			GTK_TREE_MODEL(model), "row-deleted",
			G_CALLBACK(modest_msg_view_window_on_row_deleted),
			window);
	priv->row_inserted_handler = g_signal_connect (
			GTK_TREE_MODEL(model), "row-inserted",
			G_CALLBACK(modest_msg_view_window_on_row_inserted),
			window);
	priv->rows_reordered_handler = g_signal_connect(
			GTK_TREE_MODEL(model), "rows-reordered",
			G_CALLBACK(modest_msg_view_window_on_row_reordered),
			window);

	if (header_view != NULL){
		modest_header_view_add_observer(header_view,
				MODEST_HEADER_VIEW_OBSERVER(window));
	}

	modest_msg_view_window_update_priority (window);

	/* Check toolbar dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));

	return MODEST_WINDOW(window);
}

ModestWindow *
modest_msg_view_window_new_for_search_result (TnyMsg *msg, 
					      const gchar *modest_account_name,
					      const gchar *msg_uid)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;

	window = g_object_new(MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);
	modest_msg_view_window_construct (window, msg, modest_account_name, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Remember that this is a search result, 
	 * so we can disable some UI appropriately: */
	priv->is_search_result = TRUE;

	return MODEST_WINDOW(window);
}

ModestWindow *
modest_msg_view_window_new_for_attachment (TnyMsg *msg, 
			    const gchar *modest_account_name,
			    const gchar *msg_uid)
{
	GObject *obj = NULL;
	g_return_val_if_fail (msg, NULL);
	
	obj = g_object_new(MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	modest_msg_view_window_construct (MODEST_MSG_VIEW_WINDOW (obj), 
		msg, modest_account_name, msg_uid);

	return MODEST_WINDOW(obj);
}

void modest_msg_view_window_on_row_changed(
		GtkTreeModel *header_model,
		GtkTreePath *arg1,
		GtkTreeIter *arg2,
		ModestMsgViewWindow *window){
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
}

void modest_msg_view_window_on_row_deleted(
		GtkTreeModel *header_model,
		GtkTreePath *arg1,
		ModestMsgViewWindow *window){
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
}

/* On insertions we check if the folder still has the message we are
 * showing or do not. If do not, we do nothing. Which means we are still
 * not attached to any header folder and thus next/prev buttons are
 * still dimmed. Once the message that is shown by msg-view is found, the
 * new model of header-view will be attached and the references will be set.
 * On each further insertions dimming rules will be checked. However
 * this requires extra CPU time at least works.
 * (An message might be deleted from TnyFolder and thus will not be
 * inserted into the model again for example if it is removed by the
 * imap server and the header view is refreshed.)
 */
void modest_msg_view_window_on_row_inserted(
		GtkTreeModel *new_model,
		GtkTreePath *tree_path,
		GtkTreeIter *tree_iter,
		ModestMsgViewWindow *window){
	ModestMsgViewWindowPrivate *priv = NULL; 
	TnyHeader *header = NULL;
	gchar *uid = NULL;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	
	/* If we already has a model attached then the message shown by
	 * msg-view is in it, and thus we do not need any actions but
	 * to check the dimming rules.*/
	if(priv->header_model != NULL){
		gtk_tree_row_reference_free(priv->next_row_reference);
		priv->next_row_reference = gtk_tree_row_reference_copy(
				priv->row_reference);
		select_next_valid_row (priv->header_model,
				&(priv->next_row_reference), FALSE);
		modest_ui_actions_check_toolbar_dimming_rules (
				MODEST_WINDOW (window));
		return;
	}

	/* Check if the newly inserted message is the same we are actually
	 * showing. IF not, we should remain detached from the header model
	 * and thus prev and next toolbarbuttons should remain dimmed. */
	gtk_tree_model_get (new_model, tree_iter, 
			TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &header, -1);
	uid = modest_tny_folder_get_header_unique_id(header);
	if(!g_str_equal(priv->msg_uid, uid)){
		g_free(uid);
		return;
	}
	g_free(uid);

	/* Setup row_reference for the actual msg. */
	priv->row_reference = gtk_tree_row_reference_new(
			new_model, tree_path);
	if(priv->row_reference == NULL){
		g_warning("No reference for msg header item.");
		return;
	}

	/* Attach new_model and connect some callback to it to become able
	 * to detect changes in header-view. */
	priv->header_model = g_object_ref(new_model);
	g_signal_connect (new_model, "row-changed",
			G_CALLBACK (modest_msg_view_window_on_row_changed),
			window);
	g_signal_connect (new_model, "row-deleted",
			G_CALLBACK (modest_msg_view_window_on_row_deleted),
			window);
	g_signal_connect (new_model, "rows-reordered",
			G_CALLBACK (modest_msg_view_window_on_row_reordered),
			window);

	/* Now set up next_row_reference. */
	priv->next_row_reference = gtk_tree_row_reference_copy(
			priv->row_reference);
	select_next_valid_row (priv->header_model,
			&(priv->next_row_reference), FALSE);

	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
}

void modest_msg_view_window_on_row_reordered(
		GtkTreeModel *header_model,
		GtkTreePath *arg1,
		GtkTreeIter *arg2,
		gpointer arg3,
		ModestMsgViewWindow *window){
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
}

/* The modest_msg_view_window_update_model_replaced implements update
 * function for ModestHeaderViewObserver. Checks whether the TnyFolder
 * actually belongs to the header-view is the same as the TnyFolder of
 * the message of msg-view or not. If they are different, there is
 * nothing to do. If they are the same, then the model has replaced and
 * the reference in msg-view shall be replaced from the old model to
 * the new model. In this case the view will be detached from it's
 * header folder. From this point the next/prev buttons are dimmed.
 */
void modest_msg_view_window_update_model_replaced(
		ModestHeaderViewObserver *observer,
		GtkTreeModel *model,
		const gchar *tny_folder_id){
	ModestMsgViewWindowPrivate *priv = NULL; 
	ModestMsgViewWindow *window = NULL;

	g_assert(MODEST_IS_HEADER_VIEW_OBSERVER(observer));
	g_assert(MODEST_IS_MSG_VIEW_WINDOW(observer));

	window = MODEST_MSG_VIEW_WINDOW(observer);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(window);

	/* If there is an other folder in the header-view then we do
	 * not care about it's model (msg list). Else if the
	 * header-view shows the folder the msg shown by us is in, we
	 * shall replace our model reference and make some check. */
	if(tny_folder_id == NULL ||
			!g_str_equal(tny_folder_id, priv->header_folder_id))
		return;

	/* Model is changed(replaced), so we should forget the old
	 * one. Because there might be other references and there
	 * might be some change on the model even if we unreferenced
	 * it, we need to disconnect our signals here. */
	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->row_changed_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->row_changed_handler);
	priv->row_changed_handler = 0;
	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->row_deleted_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->row_deleted_handler);
	priv->row_deleted_handler = 0;
	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->row_inserted_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->row_inserted_handler);
	priv->row_inserted_handler = 0;
	if (g_signal_handler_is_connected(G_OBJECT (priv->header_model), 
					   priv->rows_reordered_handler))
		g_signal_handler_disconnect(G_OBJECT (priv->header_model), 
					     priv->rows_reordered_handler);
	priv->rows_reordered_handler = 0;
	g_object_unref(priv->header_model);
	priv->header_model = NULL;
	g_object_unref(priv->row_reference);
	priv->row_reference = NULL;
	g_object_unref(priv->next_row_reference);
	priv->next_row_reference = NULL;

	modest_ui_actions_check_toolbar_dimming_rules(MODEST_WINDOW(window));

	if(tny_folder_id == NULL)
		return;

	g_assert(model != NULL);

	/* Also we must connect to the new model for row insertions.
	 * Only for insertions now. We will need other ones only after
	 * the msg is show by msg-view is added to the new model. */
	priv->row_inserted_handler = g_signal_connect (
			model, "row-inserted",
			G_CALLBACK(modest_msg_view_window_on_row_inserted),
			window);
}

gboolean 
modest_msg_view_window_toolbar_on_transfer_mode     (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv= NULL; 

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	return priv->current_toolbar_mode == TOOLBAR_MODE_TRANSFER;
}

TnyHeader*
modest_msg_view_window_get_header (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv= NULL; 
	TnyMsg *msg = NULL;
	TnyHeader *header = NULL;
 	GtkTreePath *path = NULL;
 	GtkTreeIter iter;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), NULL);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	/* If the message was not obtained from a treemodel,
	 * for instance if it was opened directly by the search UI:
	 */
	if (priv->header_model == NULL) {
		msg = modest_msg_view_window_get_message (self);
		header = tny_msg_get_header (msg);
		g_object_unref (msg);
		return header;
	}

	/* Get iter of the currently selected message in the header view: */
	/* TODO: Why not just give this window a ref of the TnyHeader or TnyMessage,
	 * instead of sometimes retrieving it from the header view?
	 * Then we wouldn't be dependent on the message actually still being selected 
	 * in the header view. murrayc. */
	path = gtk_tree_row_reference_get_path (priv->row_reference);
	g_return_val_if_fail (path != NULL, NULL);
	gtk_tree_model_get_iter (priv->header_model, 
				 &iter, 
				 path);

	/* Get current message header */
	gtk_tree_model_get (priv->header_model, &iter, 
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
			    &header, -1);

	gtk_tree_path_free (path);
	return header;
}

TnyMsg*
modest_msg_view_window_get_message (ModestMsgViewWindow *self)
{
	ModestMsgView *msg_view;
	ModestMsgViewWindowPrivate *priv;

	g_return_val_if_fail (self, NULL);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	msg_view = MODEST_MSG_VIEW (priv->msg_view);

	return modest_msg_view_get_message (msg_view);
}

const gchar*
modest_msg_view_window_get_message_uid (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv  = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	return (const gchar*) priv->msg_uid;
}

static void 
modest_msg_view_window_toggle_find_toolbar (GtkToggleAction *toggle,
					    gpointer data)
{
	ModestMsgViewWindow *window = MODEST_MSG_VIEW_WINDOW (data);
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	gboolean is_active;
	GtkAction *action;

	is_active = gtk_toggle_action_get_active (toggle);

	if (is_active) {
		gtk_widget_show (priv->find_toolbar);
		hildon_find_toolbar_highlight_entry (HILDON_FIND_TOOLBAR (priv->find_toolbar), TRUE);
	} else {
		gtk_widget_hide (priv->find_toolbar);
	}

	/* update the toggle buttons status */
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/FindInMessage");
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), is_active);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ToolsMenu/ToolsFindInMessageMenu");
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), is_active);
	
}

static void
modest_msg_view_window_find_toolbar_close (GtkWidget *widget,
					   ModestMsgViewWindow *obj)
{
	GtkToggleAction *toggle;
	ModestWindowPrivate *parent_priv;
	parent_priv = MODEST_WINDOW_GET_PRIVATE (obj);
	
	toggle = GTK_TOGGLE_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/FindInMessage"));
	gtk_toggle_action_set_active (toggle, FALSE);
}

static void
modest_msg_view_window_find_toolbar_search (GtkWidget *widget,
					   ModestMsgViewWindow *obj)
{
	gchar *current_search;
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);

	if (modest_msg_view_get_message_is_empty (MODEST_MSG_VIEW (priv->msg_view))) {
		hildon_banner_show_information (NULL, NULL, _("mail_ib_nothing_to_find"));
		return;
	}

	g_object_get (G_OBJECT (widget), "prefix", &current_search, NULL);

	if ((current_search == NULL) || (strcmp (current_search, "") == 0)) {
		g_free (current_search);
		hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ecdg_ib_find_rep_enter_text"));
		return;
	}

	if ((priv->last_search == NULL) || (strcmp (priv->last_search, current_search) != 0)) {
		gboolean result;
		g_free (priv->last_search);
		priv->last_search = g_strdup (current_search);
		result = modest_msg_view_search (MODEST_MSG_VIEW (priv->msg_view),
						 priv->last_search);
		if (!result) {
			hildon_banner_show_information (NULL, NULL, dgettext("hildon-libs", "ckct_ib_find_no_matches"));
			g_free (priv->last_search);
			priv->last_search = NULL;
		} 
	} else {
		if (!modest_msg_view_search_next (MODEST_MSG_VIEW (priv->msg_view))) {
			hildon_banner_show_information (NULL, NULL, dgettext("hildon-libs", "ckct_ib_find_search_complete"));
			g_free (priv->last_search);
			priv->last_search = NULL;
		}
	}
	
	g_free (current_search);
		
}

static void
modest_msg_view_window_set_zoom (ModestWindow *window,
				 gdouble zoom)
{
	ModestMsgViewWindowPrivate *priv;
     
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	modest_msg_view_set_zoom (MODEST_MSG_VIEW (priv->msg_view), zoom);
}

static gdouble
modest_msg_view_window_get_zoom (ModestWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
     
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), 1.0);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	return modest_msg_view_get_zoom (MODEST_MSG_VIEW (priv->msg_view));
}

static gboolean
modest_msg_view_window_zoom_plus (ModestWindow *window)
{
	ModestWindowPrivate *parent_priv;
	GtkRadioAction *zoom_radio_action;
	GSList *group, *node;

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	zoom_radio_action = GTK_RADIO_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, 
									 "/MenuBar/ViewMenu/ZoomMenu/Zoom50Menu"));

	group = gtk_radio_action_get_group (zoom_radio_action);

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (group->data))) {
		hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_max_zoom_level_reached"));
		return FALSE;
	}

	for (node = group; node != NULL; node = g_slist_next (node)) {
		if ((node->next != NULL) && gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (node->next->data))) {
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (node->data), TRUE);
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
modest_msg_view_window_zoom_minus (ModestWindow *window)
{
	ModestWindowPrivate *parent_priv;
	GtkRadioAction *zoom_radio_action;
	GSList *group, *node;

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	zoom_radio_action = GTK_RADIO_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, 
									 "/MenuBar/ViewMenu/ZoomMenu/Zoom50Menu"));

	group = gtk_radio_action_get_group (zoom_radio_action);

	for (node = group; node != NULL; node = g_slist_next (node)) {
		if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (node->data))) {
			if (node->next != NULL) {
				gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (node->next->data), TRUE);
				return TRUE;
			} else {
			  hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_min_zoom_level_reached"));
				return FALSE;
			}
			break;
		}
	}
	return FALSE;
}

static gboolean
modest_msg_view_window_key_release_event (GtkWidget *window,
					  GdkEventKey *event,
					  gpointer userdata)
{
	if (event->type == GDK_KEY_RELEASE) {
		switch (event->keyval) {
		case GDK_Up:
			modest_msg_view_window_scroll_up (MODEST_WINDOW (window));
			return TRUE;
			break;
		case GDK_Down:
			modest_msg_view_window_scroll_down (MODEST_WINDOW (window));
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		};
	} else {
		return FALSE;
	}
}

static void
modest_msg_view_window_scroll_up (ModestWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	gboolean return_value;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	g_signal_emit_by_name (G_OBJECT (priv->main_scroll), "scroll-child", GTK_SCROLL_STEP_UP, FALSE, &return_value);
}

static void
modest_msg_view_window_scroll_down (ModestWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	gboolean return_value;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	g_signal_emit_by_name (G_OBJECT (priv->main_scroll), "scroll-child", GTK_SCROLL_STEP_DOWN, FALSE, &return_value);
}

gboolean
modest_msg_view_window_last_message_selected (ModestMsgViewWindow *window)
{
	GtkTreePath *path;
	ModestMsgViewWindowPrivate *priv;
	GtkTreeIter tmp_iter;
	gboolean is_last_selected;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), TRUE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/*if no model (so no rows at all), then virtually we are the last*/
	if (!priv->header_model)
		return TRUE;

	path = gtk_tree_row_reference_get_path (priv->row_reference);
	if (path == NULL)
		return TRUE;

	is_last_selected = TRUE;
	while (is_last_selected) {
		TnyHeader *header;
		gtk_tree_path_next (path);
		if (!gtk_tree_model_get_iter (priv->header_model, &tmp_iter, path))
			break;
		gtk_tree_model_get (priv->header_model, &tmp_iter,
				TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				&header, -1);
		if (!(tny_header_get_flags(header) & TNY_HEADER_FLAG_DELETED))
			is_last_selected = FALSE;
	}
	gtk_tree_path_free (path);
	return is_last_selected;
}

gboolean
modest_msg_view_window_has_headers_model (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), TRUE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	return priv->header_model != NULL;
}

gboolean
modest_msg_view_window_is_search_result (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), TRUE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	return priv->is_search_result;
}

gboolean
modest_msg_view_window_first_message_selected (ModestMsgViewWindow *window)
{
	GtkTreePath *path;
	ModestMsgViewWindowPrivate *priv;
	gboolean is_first_selected;
	GtkTreeIter tmp_iter;
/*	gchar * path_string;*/

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), TRUE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/*if no model (so no rows at all), then virtually we are the first*/
	if (!priv->header_model)
		return TRUE;

	path = gtk_tree_row_reference_get_path (priv->row_reference);
	if (!path)
		return TRUE;

/*	path_string = gtk_tree_path_to_string (path);
	is_first_selected = strcmp (path_string, "0");

	g_free (path_string);
	gtk_tree_path_free (path);

	return is_first_selected;*/

	is_first_selected = TRUE;
	while (is_first_selected) {
		TnyHeader *header;
		if(!gtk_tree_path_prev (path))
			break;
		/* Here the 'if' is needless for logic, but let make sure
		 * iter is valid for gtk_tree_model_get. */
		if (!gtk_tree_model_get_iter (priv->header_model, &tmp_iter, path))
			break;
		gtk_tree_model_get (priv->header_model, &tmp_iter,
				TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				&header, -1);
		if (!(tny_header_get_flags(header) & TNY_HEADER_FLAG_DELETED))
			is_first_selected = FALSE;
	}
	gtk_tree_path_free (path);
	return is_first_selected;
}

/**
 * Reads the message whose summary item is @header. It takes care of
 * several things, among others:
 *
 * If the message was not previously downloaded then ask the user
 * before downloading. If there is no connection launch the connection
 * dialog. Update toolbar dimming rules.
 *
 * Returns: TRUE if the mail operation was started, otherwise if the
 * user do not want to download the message, or if the user do not
 * want to connect, then the operation is not issued
 **/
static gboolean
message_reader (ModestMsgViewWindow *window,
		ModestMsgViewWindowPrivate *priv,
		TnyHeader *header,
		GtkTreePath *path)
{
	ModestMailOperation *mail_op = NULL;
	ModestMailOperationTypeOperation op_type;
	gboolean already_showing = FALSE;
	ModestWindow *msg_window = NULL;
	ModestWindowMgr *mgr;

	g_return_val_if_fail (path != NULL, FALSE);

	mgr = modest_runtime_get_window_mgr ();
	already_showing = modest_window_mgr_find_registered_header (mgr, header, &msg_window);
	if (already_showing && (msg_window != MODEST_WINDOW (window))) {
		gboolean retval;
		if (msg_window)
			gtk_window_present (GTK_WINDOW (msg_window));
		g_signal_emit_by_name (G_OBJECT (window), "delete-event", NULL, &retval);
		return TRUE;
	}

	/* Msg download completed */
	if (tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED) {
		op_type = MODEST_MAIL_OPERATION_TYPE_OPEN;
	} else {
		op_type = MODEST_MAIL_OPERATION_TYPE_RECEIVE;

		/* Ask the user if he wants to download the message if
		   we're not online */
		if (!tny_device_is_online (modest_runtime_get_device())) {
			TnyFolder *folder = NULL;
			GtkResponseType response;

			response = modest_platform_run_confirmation_dialog (GTK_WINDOW (window),
									    _("mcen_nc_get_msg"));
			if (response == GTK_RESPONSE_CANCEL)
				return FALSE;
		
			/* Offer the connection dialog if necessary */
			folder = tny_header_get_folder (header);
			if (folder) {
				if (!modest_platform_connect_and_wait_if_network_folderstore (NULL, 
											      TNY_FOLDER_STORE (folder))) {
					g_object_unref (folder);
					return FALSE;
				}
				g_object_unref (folder);
			}
		}
	}

	/* New mail operation */
	mail_op = modest_mail_operation_new_with_error_handling (op_type, 
								 G_OBJECT(window),
								 modest_ui_actions_get_msgs_full_error_handler, 
								 NULL);
				
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	modest_mail_operation_get_msg (mail_op, header, view_msg_cb, path);
	g_object_unref (mail_op);

	/* Update toolbar dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));

	return TRUE;
}

gboolean        
modest_msg_view_window_select_next_message (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	GtkTreePath *path= NULL;
	GtkTreeIter tmp_iter;
	TnyHeader *header;
	gboolean retval = TRUE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Update the next row reference if it's not valid. This could
	   happen if for example the header which it was pointing to,
	   was deleted. The best place to do it is in the row-deleted
	   handler but the tinymail model do not work like the glib
	   tree models and reports the deletion when the row is still
	   there */
	if (!gtk_tree_row_reference_valid (priv->next_row_reference)) {
		if (gtk_tree_row_reference_valid (priv->row_reference)) {
			priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
			select_next_valid_row (priv->header_model, &(priv->next_row_reference), FALSE);
		}
	}
	if (priv->next_row_reference)
		path = gtk_tree_row_reference_get_path (priv->next_row_reference);
	if (path == NULL)
		return FALSE;

	gtk_tree_model_get_iter (priv->header_model,
				 &tmp_iter,
				 path);

	gtk_tree_model_get (priv->header_model, &tmp_iter, 
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);
	
	/* Read the message & show it */
	if (!message_reader (window, priv, header, path)) {
		retval = FALSE;
		gtk_tree_path_free (path);
	}

	/* Free */
	g_object_unref (header);

	return retval;       	
}

gboolean 
modest_msg_view_window_select_first_message (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv = NULL;
	TnyHeader *header = NULL;
	GtkTreeIter iter;
	GtkTreePath *path;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	/* Check that the model is not empty */
	if (!gtk_tree_model_get_iter_first (priv->header_model, &iter))
		return FALSE;

	/* Get the header */
	gtk_tree_model_get (priv->header_model, 
			    &iter, 
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);
	g_return_val_if_fail (TNY_IS_HEADER (header), FALSE);
	if (tny_header_get_flags (header) & TNY_HEADER_FLAG_DELETED) {
		g_object_unref (header);
		return modest_msg_view_window_select_next_message (self);
	}
	
	path = gtk_tree_model_get_path (priv->header_model, &iter);
	
	/* Read the message & show it */
	message_reader (self, priv, header, path);
	
	/* Free */
	g_object_unref (header);

	return TRUE;
}
 
gboolean        
modest_msg_view_window_select_previous_message (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv = NULL;
	GtkTreePath *path;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Return inmediatly if there is no header model */
	if (!priv->header_model)
		return FALSE;

	path = gtk_tree_row_reference_get_path (priv->row_reference);
	while (gtk_tree_path_prev (path)) {
		TnyHeader *header;
		GtkTreeIter iter;

		gtk_tree_model_get_iter (priv->header_model, &iter, path);
		gtk_tree_model_get (priv->header_model, &iter, 
				    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				    &header, -1);
		if (!header)
			break;
		if (tny_header_get_flags (header) & TNY_HEADER_FLAG_DELETED) {
			g_object_unref (header);
			continue;
		}

		/* Read the message & show it */
		if (!message_reader (window, priv, header, path)) {
			g_object_unref (header);
			break;
		}

		g_object_unref (header);

		return TRUE;
	}

	gtk_tree_path_free (path);
	return FALSE;
}

static void
view_msg_cb (ModestMailOperation *mail_op, 
	     TnyHeader *header, 
	     TnyMsg *msg, 
	     gpointer user_data)
{
	ModestMsgViewWindow *self = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	GtkTreePath *path;

	/* If there was any error */
	path = (GtkTreePath *) user_data;
	if (!modest_ui_actions_msg_retrieval_check (mail_op, header, msg)) {
		gtk_tree_path_free (path);			
		return;
	}

	/* Get the window */ 
	self = (ModestMsgViewWindow *) modest_mail_operation_get_source (mail_op);
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	/* Update the row reference */
	gtk_tree_row_reference_free (priv->row_reference);
	priv->row_reference = gtk_tree_row_reference_new (priv->header_model, path);
	priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
	select_next_valid_row (priv->header_model, &(priv->next_row_reference), TRUE);
	gtk_tree_path_free (path);

	/* Mark header as read */
	if (!(tny_header_get_flags (header) & TNY_HEADER_FLAG_SEEN))
		tny_header_set_flags (header, TNY_HEADER_FLAG_SEEN);

	/* Set new message */
	modest_msg_view_set_message (MODEST_MSG_VIEW (priv->msg_view), msg);
	modest_msg_view_window_update_priority (self);
	update_window_title (MODEST_MSG_VIEW_WINDOW (self));
	modest_msg_view_grab_focus (MODEST_MSG_VIEW (priv->msg_view));

	/* Set the new message uid of the window  */
	if (priv->msg_uid) {
		g_free (priv->msg_uid);
		priv->msg_uid = modest_tny_folder_get_header_unique_id (header);
	}

	/* Notify the observers */
	g_signal_emit (G_OBJECT (self), signals[MSG_CHANGED_SIGNAL], 
		       0, priv->header_model, priv->row_reference);

	/* Free new references */
	g_object_unref (self);
}

TnyFolderType
modest_msg_view_window_get_folder_type (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	TnyMsg *msg;
	TnyFolderType folder_type;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	folder_type = TNY_FOLDER_TYPE_UNKNOWN;

	msg = modest_msg_view_get_message (MODEST_MSG_VIEW (priv->msg_view));
	if (msg) {
		TnyFolder *folder;

		folder = tny_msg_get_folder (msg);
		
		if (folder) {
			folder_type = tny_folder_get_folder_type (folder);
			
			if (folder_type == TNY_FOLDER_TYPE_NORMAL || folder_type == TNY_FOLDER_TYPE_UNKNOWN) {
				const gchar *fname = tny_folder_get_name (folder);
				folder_type = modest_tny_folder_guess_folder_type_from_name (fname);
			}

			g_object_unref (folder);
		}
		g_object_unref (msg);
	}

	return folder_type;
}


static void
modest_msg_view_window_update_priority (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	TnyHeaderFlags flags = 0;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->header_model) {
		TnyHeader *header;
		GtkTreeIter iter;
		GtkTreePath *path = NULL;

		path = gtk_tree_row_reference_get_path (priv->row_reference);
		g_return_if_fail (path != NULL);
		gtk_tree_model_get_iter (priv->header_model, 
					 &iter, 
					 gtk_tree_row_reference_get_path (priv->row_reference));

		gtk_tree_model_get (priv->header_model, &iter, TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				    &header, -1);
		flags = tny_header_get_flags (header);
		gtk_tree_path_free (path);
	}

	modest_msg_view_set_priority (MODEST_MSG_VIEW(priv->msg_view), flags);

}

static gboolean
modest_msg_view_window_window_state_event (GtkWidget *widget, GdkEventWindowState *event, gpointer userdata)
{
	if (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) {
		ModestWindowPrivate *parent_priv;
		ModestWindowMgr *mgr;
		gboolean is_fullscreen;
		GtkAction *fs_toggle_action;
		gboolean active;

		mgr = modest_runtime_get_window_mgr ();
		is_fullscreen = (modest_window_mgr_get_fullscreen_mode (mgr))?1:0;

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
	if (GTK_IS_TOOL_ITEM (widget)) {
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), TRUE);
	}
}

static void
modest_msg_view_window_show_toolbar (ModestWindow *self,
				     gboolean show_toolbar)
{
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv;
	GtkWidget *reply_button = NULL, *menu = NULL;
	GtkWidget *placeholder = NULL;
	gint insert_index;
	const gchar *action_name;
	GtkAction *action;
	
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	/* Set optimized view status */
	priv->optimized_view = !show_toolbar;

	if (!parent_priv->toolbar) {
		parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
								  "/ToolBar");
		gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);

		/* Set homogeneous toolbar */
		gtk_container_foreach (GTK_CONTAINER (parent_priv->toolbar), 
				       set_homogeneous, NULL);

		priv->progress_toolitem = GTK_WIDGET (gtk_tool_item_new ());
		priv->cancel_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarCancel");
		priv->next_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageNext");
		priv->prev_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageBack");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
 		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);

		/* Add ProgressBar (Transfer toolbar) */ 
		priv->progress_bar = modest_progress_bar_widget_new ();
		gtk_widget_set_no_show_all (priv->progress_bar, TRUE);
		placeholder = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ProgressbarView");
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
	}

	if (show_toolbar) {
		/* Quick hack: this prevents toolbar icons "dance" when progress bar show status is changed */ 
		/* TODO: resize mode migth be GTK_RESIZE_QUEUE, in order to avoid unneccesary shows */
		gtk_container_set_resize_mode (GTK_CONTAINER(parent_priv->toolbar), GTK_RESIZE_IMMEDIATE);
		
		gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
		set_toolbar_mode (MODEST_MSG_VIEW_WINDOW(self), TOOLBAR_MODE_NORMAL);			
		
	} else {
		gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
	}

	/* Update also the actions (to update the toggles in the
	   menus), we have to do it manually because some other window
	   of the same time could have changed it (remember that the
	   toolbar fullscreen mode is shared by all the windows of the
	   same type */
	if (modest_window_mgr_get_fullscreen_mode (modest_runtime_get_window_mgr ()))
		action_name = "/MenuBar/ViewMenu/ViewShowToolbarMenu/ViewShowToolbarFullScreenMenu";
	else
		action_name = "/MenuBar/ViewMenu/ViewShowToolbarMenu/ViewShowToolbarNormalScreenMenu";

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, action_name);
	modest_maemo_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action),
							    show_toolbar);
}

static void 
modest_msg_view_window_clipboard_owner_change (GtkClipboard *clipboard,
					       GdkEvent *event,
					       ModestMsgViewWindow *window)
{
	ModestWindowPrivate *parent_priv;
/* 	GtkAction *action; */
	gboolean is_address;
	gchar *selection;
	GtkWidget *focused;

	if (!GTK_WIDGET_VISIBLE (window))
		return;

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	selection = gtk_clipboard_wait_for_text (clipboard);

	is_address = ((selection != NULL) && (modest_text_utils_validate_recipient (selection, NULL)));
	
/* 	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ToolsMenu/ToolsAddToContactsMenu"); */
/* 	gtk_action_set_sensitive (action, is_address); */

	focused = gtk_window_get_focus (GTK_WINDOW (window));

/* 	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/EditCopyMenu"); */
/* 	gtk_action_set_sensitive (action, (selection != NULL) && (!MODEST_IS_ATTACHMENTS_VIEW (focused))); */

/* 	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/EditMenu/EditCutMenu"); */
/* 	gtk_action_set_sensitive (action, (selection != NULL) && (!MODEST_IS_ATTACHMENTS_VIEW (focused))); */

	g_free (selection);
/* 	modest_msg_view_window_update_dimmed (window); */
	
}

gboolean 
modest_msg_view_window_transfer_mode_enabled (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	return priv->current_toolbar_mode == TOOLBAR_MODE_TRANSFER;
}

static void
cancel_progressbar (GtkToolButton *toolbutton,
		    ModestMsgViewWindow *self)
{
	GSList *tmp;
	ModestMsgViewWindowPrivate *priv;
	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	/* Get operation observers and cancel its current operation */
	tmp = priv->progress_widgets;
	while (tmp) {
		modest_progress_object_cancel_current_operation (MODEST_PROGRESS_OBJECT(tmp->data));
		tmp=g_slist_next(tmp);
	}
}
static gboolean
observers_empty (ModestMsgViewWindow *self)
{
	GSList *tmp = NULL;
	ModestMsgViewWindowPrivate *priv;
	gboolean is_empty = TRUE;
	guint pending_ops = 0;
 
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);
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
on_account_removed (TnyAccountStore *account_store, 
		    TnyAccount *account,
		    gpointer user_data)
{
	/* Do nothing if it's a transport account, because we only
	   show the messages of a store account */
	if (tny_account_get_account_type(account) == TNY_ACCOUNT_TYPE_STORE) {
		const gchar *parent_acc = NULL;
		const gchar *our_acc = NULL;

		our_acc = modest_window_get_active_account (MODEST_WINDOW (user_data));
		parent_acc = modest_tny_account_get_parent_modest_account_name_for_server_account (account);

		/* Close this window if I'm showing a message of the removed account */
		if (strcmp (parent_acc, our_acc) == 0)
			modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (user_data));
	}
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestMsgViewWindow *self)
{
	GSList *tmp;
	ModestMsgViewWindowPrivate *priv;
	ModestMailOperationTypeOperation op_type;
	ModestToolBarModes mode;
	
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	/* If this operations was created by another window, do nothing */
	if (!modest_mail_operation_is_mine (mail_op, G_OBJECT(self))) 
	    return;

	/* Get toolbar mode from operation id*/
	op_type = modest_mail_operation_get_type_operation (mail_op);
	switch (op_type) {
/* 	case MODEST_MAIL_OPERATION_TYPE_SEND: */
	case MODEST_MAIL_OPERATION_TYPE_RECEIVE:
	case MODEST_MAIL_OPERATION_TYPE_OPEN:
		mode = TOOLBAR_MODE_TRANSFER;
		break;
	default:
		mode = TOOLBAR_MODE_NORMAL;
		
	}
		
	/* Add operation observers and change toolbar if neccessary*/
	tmp = priv->progress_widgets;
	switch (type) {
	case MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED:
		if (mode == TOOLBAR_MODE_TRANSFER) {
			/* Enable transfer toolbar mode */
			set_toolbar_transfer_mode(self);
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
			if (observers_empty (self)) {
				set_toolbar_mode (self, TOOLBAR_MODE_NORMAL);
			}
		}
		break;
	}
}

GList *
modest_msg_view_window_get_attachments (ModestMsgViewWindow *win) 
{
	ModestMsgViewWindowPrivate *priv;
	GList *selected_attachments = NULL;
	
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win), NULL);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (win);

	selected_attachments = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
	
	return selected_attachments;
}

void
modest_msg_view_window_view_attachment (ModestMsgViewWindow *window, TnyMimePart *mime_part)
{
	ModestMsgViewWindowPrivate *priv;
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	g_return_if_fail (TNY_IS_MIME_PART (mime_part) || (mime_part == NULL));

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (mime_part == NULL) {
		gboolean error = FALSE;
		GList *selected_attachments = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
		if (selected_attachments == NULL) {
			error = TRUE;
		} else if (g_list_length (selected_attachments) > 1) {
			hildon_banner_show_information (NULL, NULL, _("mcen_ib_unable_to_display_more"));
			error = TRUE;
		} else {
			mime_part = (TnyMimePart *) selected_attachments->data;
			g_object_ref (mime_part);
		}
		g_list_foreach (selected_attachments, (GFunc) g_object_unref, NULL);
		g_list_free (selected_attachments);

		if (error)
			return;
	} else {
		g_object_ref (mime_part);
	}

	if (tny_mime_part_is_purged (mime_part)) {
		g_object_unref (mime_part);
		return;
	}

	if (!TNY_IS_MSG (mime_part)) {
		gchar *filepath = NULL;
		const gchar *att_filename = tny_mime_part_get_filename (mime_part);
		TnyFsStream *temp_stream = NULL;

		temp_stream = modest_maemo_utils_create_temp_stream (att_filename, &filepath);
		
		if (temp_stream) {
			const gchar *content_type;
			content_type = tny_mime_part_get_content_type (mime_part);
			tny_mime_part_decode_to_stream (mime_part, TNY_STREAM (temp_stream));
			
			modest_platform_activate_file (filepath, content_type);
			g_object_unref (temp_stream);
			g_free (filepath);
			/* NOTE: files in the temporary area will be automatically
			 * cleaned after some time if they are no longer in use */
		}
	} else {
		/* message attachment */
		TnyHeader *header = NULL;
		ModestWindowMgr *mgr;
		ModestWindow *msg_win = NULL;
		gboolean found;
		
		header = tny_msg_get_header (TNY_MSG (mime_part));
		mgr = modest_runtime_get_window_mgr ();		
		found = modest_window_mgr_find_registered_header (mgr, header, &msg_win);

		if (found) {
			if (msg_win) 				/* there is already a window for this uid; top it */
				gtk_window_present (GTK_WINDOW(msg_win));
			else 
				/* if it's found, but there is no msg_win, it's probably in the process of being created;
				 * thus, we don't do anything */
				g_warning ("window for is already being created");
		} else { 
			/* it's not found, so create a new window for it */
			modest_window_mgr_register_header (mgr, header); /* register the uid before building the window */
			gchar *account = g_strdup (modest_window_get_active_account (MODEST_WINDOW (window)));
			if (!account)
				account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr ());
			msg_win = modest_msg_view_window_new_for_attachment (TNY_MSG (mime_part), account, NULL);
			modest_window_set_zoom (MODEST_WINDOW (msg_win), 
						modest_window_get_zoom (MODEST_WINDOW (window)));
			modest_window_mgr_register_window (mgr, msg_win);
			gtk_window_set_transient_for (GTK_WINDOW (msg_win), GTK_WINDOW (window));
			gtk_widget_show_all (GTK_WIDGET (msg_win));
		}
	}
	g_object_unref (mime_part);
}

typedef struct
{
	gchar *filename;
	TnyMimePart *part;
} SaveMimePartPair;

typedef struct
{
	GList *pairs;
	GtkWidget *banner;
	gboolean result;
} SaveMimePartInfo;

static void save_mime_part_info_free (SaveMimePartInfo *info, gboolean with_struct);
static gboolean idle_save_mime_part_show_result (SaveMimePartInfo *info);
static gpointer save_mime_part_to_file (SaveMimePartInfo *info);
static void save_mime_parts_to_file_with_checks (SaveMimePartInfo *info);

static void 
save_mime_part_info_free (SaveMimePartInfo *info, gboolean with_struct)
{
	
	GList *node;
	for (node = info->pairs; node != NULL; node = g_list_next (node)) {
		SaveMimePartPair *pair = (SaveMimePartPair *) node->data;
		g_free (pair->filename);
		g_object_unref (pair->part);
		g_slice_free (SaveMimePartPair, pair);
	}
	g_list_free (info->pairs);
	info->pairs = NULL;
	if (with_struct) {
		gtk_widget_destroy (info->banner);
		g_object_unref (info->banner);
		g_slice_free (SaveMimePartInfo, info);
	}
}

static gboolean
idle_save_mime_part_show_result (SaveMimePartInfo *info)
{
	if (info->pairs != NULL) {
		save_mime_part_to_file (info);
	} else {
		gboolean result;
		result = info->result;

		/* This is a GDK lock because we are an idle callback and
	 	 * hildon_banner_show_information is or does Gtk+ code */

		gdk_threads_enter (); /* CHECKED */
		save_mime_part_info_free (info, TRUE);
		if (result) {
			hildon_banner_show_information (NULL, NULL, _CS("sfil_ib_saved"));
		} else {
			hildon_banner_show_information (NULL, NULL, _("mail_ib_file_operation_failed"));
		}
		gdk_threads_leave (); /* CHECKED */
	}

	return FALSE;
}

static gpointer
save_mime_part_to_file (SaveMimePartInfo *info)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	TnyStream *stream;
	SaveMimePartPair *pair = (SaveMimePartPair *) info->pairs->data;

	result = gnome_vfs_create (&handle, pair->filename, GNOME_VFS_OPEN_WRITE, FALSE, 0777);
	if (result == GNOME_VFS_OK) {
		stream = tny_vfs_stream_new (handle);
		tny_mime_part_decode_to_stream (pair->part, stream);
		g_object_unref (G_OBJECT (stream));
		g_object_unref (pair->part);
		g_slice_free (SaveMimePartPair, pair);
		info->pairs = g_list_delete_link (info->pairs, info->pairs);
		info->result = TRUE;
	} else {
		save_mime_part_info_free (info, FALSE);
		info->result = FALSE;
	}

	g_idle_add ((GSourceFunc) idle_save_mime_part_show_result, info);
	return NULL;
}

static void
save_mime_parts_to_file_with_checks (SaveMimePartInfo *info)
{
	gboolean is_ok = TRUE;
        gint replaced_files = 0;
        const GList *files = info->pairs;
        const GList *iter;

        for (iter = files; (iter != NULL) && (replaced_files < 2); iter = g_list_next(iter)) {
                SaveMimePartPair *pair = iter->data;
                if (modest_maemo_utils_file_exists (pair->filename)) {
                        replaced_files++;
                }
        }
	if (replaced_files) {
		GtkWidget *confirm_overwrite_dialog;
                const gchar *message = (replaced_files == 1) ?
                        _FM("docm_nc_replace_file") : _FM("docm_nc_replace_multiple");
                confirm_overwrite_dialog = hildon_note_new_confirmation (NULL, message);
		if (gtk_dialog_run (GTK_DIALOG (confirm_overwrite_dialog)) != GTK_RESPONSE_OK) {
			is_ok = FALSE;
		}
		gtk_widget_destroy (confirm_overwrite_dialog);
	}

	if (!is_ok) {
		save_mime_part_info_free (info, TRUE);
	} else {
		g_thread_create ((GThreadFunc)save_mime_part_to_file, info, FALSE, NULL);
	}

}


void
modest_msg_view_window_save_attachments (ModestMsgViewWindow *window, GList *mime_parts)
{
	gboolean clean_list = FALSE;
	ModestMsgViewWindowPrivate *priv;
	GList *files_to_save = NULL;
	GtkWidget *save_dialog = NULL;
	gchar *folder = NULL;
	gboolean canceled = FALSE;
	const gchar *filename = NULL;
	gchar *save_multiple_str = NULL;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (mime_parts == NULL) {
		mime_parts = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
		if (mime_parts == NULL)
			return;
		clean_list = TRUE;
	}

	/* prepare dialog */
	if (mime_parts->next == NULL) {
		/* only one attachment selected */
		TnyMimePart *mime_part = (TnyMimePart *) mime_parts->data;
		if (!TNY_IS_MSG (mime_part) && tny_mime_part_is_attachment (mime_part)) {
			filename = tny_mime_part_get_filename (mime_part);
		} else {
			g_warning ("Tried to save a non-file attachment");
			canceled = TRUE;
		}
	} else {
		save_multiple_str = g_strdup_printf (_FM("sfil_va_number_of_objects_attachments"), 
						     g_list_length (mime_parts));
	}
	
	save_dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (window), 
						      GTK_FILE_CHOOSER_ACTION_SAVE);

	/* set folder */
	folder = g_build_filename (g_get_home_dir (), DEFAULT_FOLDER, NULL);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (save_dialog), folder);
	g_free (folder);

	/* set filename */
	if (filename != NULL)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save_dialog), 
						   filename);

	/* if multiple, set multiple string */
	if (save_multiple_str) {
		g_object_set (G_OBJECT (save_dialog), "save-multiple", save_multiple_str, NULL);
	}
		
	/* show dialog */
	if (gtk_dialog_run (GTK_DIALOG (save_dialog)) == GTK_RESPONSE_OK) {
		gchar *chooser_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (save_dialog));

		if (!modest_maemo_utils_folder_writable (chooser_uri)) {
			hildon_banner_show_information 
				(NULL, NULL, dgettext("hildon-fm", "sfil_ib_readonly_location"));
		} else {
			GList *node = NULL;

			for (node = mime_parts; node != NULL; node = g_list_next (node)) {
				TnyMimePart *mime_part = (TnyMimePart *) node->data;
				
				if (tny_mime_part_is_attachment (mime_part)) {
					SaveMimePartPair *pair;

					if ((mime_parts->next != NULL) &&
					    (tny_mime_part_get_filename (mime_part) == NULL))
						continue;
					
					pair = g_slice_new0 (SaveMimePartPair);
					if (mime_parts->next == NULL) {
						pair->filename = g_strdup (chooser_uri);
					} else {
						pair->filename = 
							g_build_filename (chooser_uri,
									  tny_mime_part_get_filename (mime_part), NULL);
					}
					pair->part = g_object_ref (mime_part);
					files_to_save = g_list_prepend (files_to_save, pair);
				}
			}
		}
		g_free (chooser_uri);
	}

	gtk_widget_destroy (save_dialog);

	if (clean_list) {
		g_list_foreach (mime_parts, (GFunc) g_object_unref, NULL);
		g_list_free (mime_parts);
	}

	if (files_to_save != NULL) {
		SaveMimePartInfo *info = g_slice_new0 (SaveMimePartInfo);
		GtkWidget *banner = hildon_banner_show_animation (NULL, NULL, 
								  _CS("sfil_ib_saving"));
		info->pairs = files_to_save;
		info->banner = banner;
		info->result = TRUE;
		g_object_ref (banner);
		save_mime_parts_to_file_with_checks (info);
	}
}

static gboolean
show_remove_attachment_information (gpointer userdata)
{
	ModestMsgViewWindow *window = (ModestMsgViewWindow *) userdata;
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->remove_attachment_banner != NULL) {
		gtk_widget_destroy (priv->remove_attachment_banner);
		g_object_unref (priv->remove_attachment_banner);
	}

	priv->remove_attachment_banner = g_object_ref (
		hildon_banner_show_animation (NULL, NULL, _("mcen_ib_removing_attachment")));

	return FALSE;
}

void
modest_msg_view_window_remove_attachments (ModestMsgViewWindow *window, gboolean get_all)
{
	ModestMsgViewWindowPrivate *priv;
	GList *mime_parts = NULL, *node;
	gchar *confirmation_message;
	gint response;
	gint n_attachments;
	TnyMsg *msg;
/* 	TnyFolder *folder; */

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (get_all)
		mime_parts = modest_msg_view_get_attachments (MODEST_MSG_VIEW (priv->msg_view));
	else
		mime_parts = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
		
	/* Remove already purged messages from mime parts list */
	node = mime_parts;
	while (node != NULL) {
		TnyMimePart *part = TNY_MIME_PART (node->data);
		if (tny_mime_part_is_purged (part)) {
			GList *deleted_node = node;
			node = g_list_next (node);
			g_object_unref (part);
			mime_parts = g_list_delete_link (mime_parts, deleted_node);
		} else {
			node = g_list_next (node);
		}
	}

	if (mime_parts == NULL)
		return;

	n_attachments = g_list_length (mime_parts);
	if (n_attachments == 1) {
		const gchar *filename;

		if (TNY_IS_MSG (mime_parts->data)) {
			TnyHeader *header;
			header = tny_msg_get_header (TNY_MSG (mime_parts->data));
			filename = tny_header_get_subject (header);
			g_object_unref (header);
			if (filename == NULL)
				filename = _("mail_va_no_subject");
		} else {
			filename = tny_mime_part_get_filename (TNY_MIME_PART (mime_parts->data));
		}
		confirmation_message = g_strdup_printf (_("mcen_nc_purge_file_text"), filename);
	} else {
		confirmation_message = g_strdup_printf (ngettext("mcen_nc_purge_file_text", 
								 "mcen_nc_purge_files_text", 
								 n_attachments), n_attachments);
	}
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (window),
							    confirmation_message);
	g_free (confirmation_message);

	if (response != GTK_RESPONSE_OK)
		return;

	priv->purge_timeout = g_timeout_add (2000, show_remove_attachment_information, window);
/* 	folder = tny_msg_get_folder (msg); */
/* 	tny_msg_uncache_attachments (msg); */
/* 	tny_folder_refresh (folder, NULL); */
/* 	g_object_unref (folder); */
	
	for (node = mime_parts; node != NULL; node = g_list_next (node)) {
		tny_mime_part_set_purged (TNY_MIME_PART (node->data));
/* 		modest_msg_view_remove_attachment (MODEST_MSG_VIEW (priv->msg_view), node->data); */
	}

	msg = modest_msg_view_get_message (MODEST_MSG_VIEW (priv->msg_view));
	modest_msg_view_set_message (MODEST_MSG_VIEW (priv->msg_view), NULL);
	tny_msg_rewrite_cache (msg);
	modest_msg_view_set_message (MODEST_MSG_VIEW (priv->msg_view), msg);

	g_list_foreach (mime_parts, (GFunc) g_object_unref, NULL);
	g_list_free (mime_parts);

	if (priv->purge_timeout > 0) {
		g_source_remove (priv->purge_timeout);
		priv->purge_timeout = 0;
	}

	if (priv->remove_attachment_banner) {
		gtk_widget_destroy (priv->remove_attachment_banner);
		g_object_unref (priv->remove_attachment_banner);
		priv->remove_attachment_banner = NULL;
	}


}


static void
update_window_title (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	TnyMsg *msg = NULL;
	TnyHeader *header = NULL;
	const gchar *subject = NULL;

	msg = modest_msg_view_get_message (MODEST_MSG_VIEW (priv->msg_view));
	if (msg != NULL) {
		header = tny_msg_get_header (msg);
		subject = tny_header_get_subject (header);
		g_object_unref (msg);
	}

	if ((subject == NULL)||(subject[0] == '\0'))
		subject = _("mail_va_no_subject");

	gtk_window_set_title (GTK_WINDOW (window), subject);
}

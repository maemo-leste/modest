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
#include <modest-utils.h>
#include <modest-maemo-utils.h>
#include <modest-tny-msg.h>
#include <modest-msg-view-window.h>
#include <modest-main-window-ui.h>
#include "modest-msg-view-window-ui-dimming.h"
#include <modest-widget-memory.h>
#include <modest-runtime.h>
#include <modest-window-priv.h>
#include <modest-tny-folder.h>
#include <modest-text-utils.h>
#include <modest-account-mgr-helpers.h>
#include "modest-progress-bar.h"
#include "modest-defs.h"
#include "modest-hildon-includes.h"
#include "modest-ui-dimming-manager.h"
#include <gdk/gdkkeysyms.h>
#include <modest-tny-account.h>
#include <modest-mime-part-view.h>
#include <modest-isearch-view.h>
#include <modest-tny-mime-part.h>
#include <math.h>
#include <errno.h>
#include <glib/gstdio.h>
#include <modest-debug.h>

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
static gboolean modest_msg_view_window_key_event (GtkWidget *window,
						  GdkEventKey *event,
						  gpointer userdata);
static gboolean modest_msg_view_window_window_state_event (GtkWidget *widget, 
							   GdkEventWindowState *event, 
							   gpointer userdata);
static void modest_msg_view_window_update_priority (ModestMsgViewWindow *window);

static void modest_msg_view_window_show_toolbar   (ModestWindow *window,
						   gboolean show_toolbar);

static void modest_msg_view_window_clipboard_owner_change (GtkClipboard *clipboard,
							   GdkEvent *event,
							   ModestMsgViewWindow *window);

static void modest_msg_view_window_on_row_changed (GtkTreeModel *header_model,
						   GtkTreePath *arg1,
						   GtkTreeIter *arg2,
						   ModestMsgViewWindow *window);

static void modest_msg_view_window_on_row_deleted (GtkTreeModel *header_model,
						   GtkTreePath *arg1,
						   ModestMsgViewWindow *window);

static void modest_msg_view_window_on_row_inserted (GtkTreeModel *header_model,
						    GtkTreePath *tree_path,
						    GtkTreeIter *tree_iter,
						    ModestMsgViewWindow *window);

static void modest_msg_view_window_on_row_reordered (GtkTreeModel *header_model,
						     GtkTreePath *arg1,
						     GtkTreeIter *arg2,
						     gpointer arg3,
						     ModestMsgViewWindow *window);

static void modest_msg_view_window_update_model_replaced (ModestHeaderViewObserver *window,
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

static void on_move_focus (GtkWidget *widget,
			   GtkDirectionType direction,
			   gpointer userdata);

static void view_msg_cb         (ModestMailOperation *mail_op, 
				 TnyHeader *header, 
				 gboolean canceled,
				 TnyMsg *msg, 
				 GError *error,
				 gpointer user_data);

static void set_toolbar_mode    (ModestMsgViewWindow *self, 
				 ModestToolBarModes mode);

static void update_window_title (ModestMsgViewWindow *window);

static gboolean set_toolbar_transfer_mode     (ModestMsgViewWindow *self); 
static void init_window (ModestMsgViewWindow *obj);

static gboolean msg_is_visible (TnyHeader *header, gboolean check_outbox);

static void check_dimming_rules_after_change (ModestMsgViewWindow *window);

static gboolean on_fetch_image (ModestMsgView *msgview,
				const gchar *uri,
				TnyStream *stream,
				ModestMsgViewWindow *window);

static gboolean modest_msg_view_window_scroll_child (ModestMsgViewWindow *self,
						     GtkScrollType scroll_type,
						     gboolean horizontal,
						     gpointer userdata);

/* list my signals */
enum {
	MSG_CHANGED_SIGNAL,
	SCROLL_CHILD_SIGNAL,
	LAST_SIGNAL
};

static const GtkToggleActionEntry msg_view_toggle_action_entries [] = {
	{ "FindInMessage",    MODEST_TOOLBAR_ICON_FIND,    N_("qgn_toolb_gene_find"), NULL, NULL, G_CALLBACK (modest_msg_view_window_toggle_find_toolbar), FALSE },
	{ "ToolsFindInMessage", NULL, N_("mcen_me_viewer_find"), "<CTRL>F", NULL, G_CALLBACK (modest_msg_view_window_toggle_find_toolbar), FALSE },
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

	/* Whether the message is in outbox */
	gboolean is_outbox;
	
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
	TnyMimePart *other_body;
	
	GSList *sighandlers;
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
	ModestConf *conf;
	ModestWindowPrivate *parent_priv = MODEST_WINDOW_GET_PRIVATE (self);
	GtkAction *action;

	conf = modest_runtime_get_conf ();
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
					    "/MenuBar/ViewMenu/ViewShowToolbarMenu/ViewShowToolbarNormalScreenMenu");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      modest_conf_get_bool (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR, NULL));
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
					    "/MenuBar/ViewMenu/ViewShowToolbarMenu/ViewShowToolbarFullScreenMenu");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      modest_conf_get_bool (conf, MODEST_CONF_MSG_VIEW_WINDOW_SHOW_TOOLBAR_FULLSCREEN, NULL));
	modest_widget_memory_restore (conf,
				      G_OBJECT(self), 
				      MODEST_CONF_MSG_VIEW_WINDOW_KEY);
}

static gboolean modest_msg_view_window_scroll_child (ModestMsgViewWindow *self,
						     GtkScrollType scroll_type,
						     gboolean horizontal,
						     gpointer userdata)
{
	ModestMsgViewWindowPrivate *priv;
	gboolean return_value;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);
	g_signal_emit_by_name (priv->main_scroll, "scroll-child", scroll_type, horizontal, &return_value);
	return return_value;
}

static void
add_scroll_binding (GtkBindingSet *binding_set,
		    guint keyval,
		    GtkScrollType scroll)
{
	guint keypad_keyval = keyval - GDK_Left + GDK_KP_Left;
	
	gtk_binding_entry_add_signal (binding_set, keyval, 0,
				      "scroll_child", 2,
				      GTK_TYPE_SCROLL_TYPE, scroll,
				      G_TYPE_BOOLEAN, FALSE);
	gtk_binding_entry_add_signal (binding_set, keypad_keyval, 0,
				      "scroll_child", 2,
				      GTK_TYPE_SCROLL_TYPE, scroll,
				      G_TYPE_BOOLEAN, FALSE);
}

static void
modest_msg_view_window_class_init (ModestMsgViewWindowClass *klass)
{
	GObjectClass *gobject_class;
	ModestWindowClass *modest_window_class;
	GtkBindingSet *binding_set;

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

	modest_window_class->save_state_func = save_state;

	klass->scroll_child = modest_msg_view_window_scroll_child;

	signals[MSG_CHANGED_SIGNAL] =
		g_signal_new ("msg-changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestMsgViewWindowClass, msg_changed),
			      NULL, NULL,
			      modest_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

	signals[SCROLL_CHILD_SIGNAL] =
		g_signal_new ("scroll-child",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (ModestMsgViewWindowClass, scroll_child),
			      NULL, NULL,
			      modest_marshal_BOOLEAN__ENUM_BOOLEAN,
			      G_TYPE_BOOLEAN, 2, GTK_TYPE_SCROLL_TYPE, G_TYPE_BOOLEAN);

	binding_set = gtk_binding_set_by_class (klass);
	add_scroll_binding (binding_set, GDK_Up, GTK_SCROLL_STEP_UP);
	add_scroll_binding (binding_set, GDK_Down, GTK_SCROLL_STEP_DOWN);
	add_scroll_binding (binding_set, GDK_Page_Up, GTK_SCROLL_PAGE_UP);
	add_scroll_binding (binding_set, GDK_Page_Down, GTK_SCROLL_PAGE_DOWN);
	add_scroll_binding (binding_set, GDK_Home, GTK_SCROLL_START);
	add_scroll_binding (binding_set, GDK_End, GTK_SCROLL_END);

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewWindowPrivate));

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
	ModestWindowPrivate *parent_priv = NULL;
	GtkActionGroup *action_group = NULL;
	GError *error = NULL;
	GdkPixbuf *window_icon;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);
	parent_priv->ui_manager = gtk_ui_manager_new();

	action_group = gtk_action_group_new ("ModestMsgViewWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

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

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));
	
	priv->is_search_result = FALSE;
	priv->is_outbox = FALSE;

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
	priv->other_body = NULL;
	
	priv->sighandlers = NULL;
	
	/* Init window */
	init_window (MODEST_MSG_VIEW_WINDOW(obj));
	
	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_MSG_VIEW_ICON, MODEST_ICON_SIZE_BIG); 
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (obj), window_icon);
		g_object_unref (window_icon);
	}	
	
	hildon_program_add_window (hildon_program_get_instance(),
				   HILDON_WINDOW(obj));

	modest_window_mgr_register_help_id (modest_runtime_get_window_mgr(),
					    GTK_WINDOW(obj),"applications_email_viewer");
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
		if (priv->progress_toolitem) {
			gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), FALSE);
			gtk_widget_hide (priv->progress_toolitem);
		}

		if (priv->progress_bar)
			gtk_widget_hide (priv->progress_bar);
			
		if (priv->cancel_toolitem)
			gtk_widget_hide (priv->cancel_toolitem);

		if (priv->prev_toolitem)
			gtk_widget_show (priv->prev_toolitem);
		
		if (priv->next_toolitem)
			gtk_widget_show (priv->next_toolitem);
			
		/* Hide toolbar if optimized view is enabled */
		if (priv->optimized_view) {
			gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);
			gtk_widget_hide (GTK_WIDGET(parent_priv->toolbar));
		}

		break;
	case TOOLBAR_MODE_TRANSFER:
		if (priv->prev_toolitem)
			gtk_widget_hide (priv->prev_toolitem);
		
		if (priv->next_toolitem)
			gtk_widget_hide (priv->next_toolitem);
		
		if (priv->progress_bar)
			gtk_widget_show (priv->progress_bar);

		if (priv->progress_toolitem) {
			gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
			gtk_widget_show (priv->progress_toolitem);
		}
			
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


static void
init_window (ModestMsgViewWindow *obj)
{
	GtkWidget *main_vbox;
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);

	priv->msg_view = GTK_WIDGET (tny_platform_factory_new_msg_view (modest_tny_platform_factory_get_instance ()));
	modest_msg_view_set_shadow_type (MODEST_MSG_VIEW (priv->msg_view), GTK_SHADOW_NONE);
	main_vbox = gtk_vbox_new  (FALSE, 6);

#ifdef MODEST_USE_MOZEMBED
	priv->main_scroll = priv->msg_view;
	gtk_widget_set_size_request (priv->msg_view, -1, 1600);
#else
	priv->main_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (priv->main_scroll), priv->msg_view);
#endif
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->main_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->main_scroll), GTK_SHADOW_NONE);
	modest_maemo_set_thumbable_scrollbar (GTK_SCROLLED_WINDOW(priv->main_scroll), TRUE);

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_scroll, TRUE, TRUE, 0);
	gtk_container_add   (GTK_CONTAINER(obj), main_vbox);

	priv->find_toolbar = hildon_find_toolbar_new (NULL);
	hildon_window_add_toolbar (HILDON_WINDOW (obj), GTK_TOOLBAR (priv->find_toolbar));
	gtk_widget_set_no_show_all (priv->find_toolbar, TRUE);
	
	gtk_widget_show_all (GTK_WIDGET(main_vbox));
}

static void
modest_msg_view_window_disconnect_signals (ModestWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	ModestHeaderView *header_view = NULL;
	ModestWindow *main_window = NULL;
	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	if (gtk_clipboard_get (GDK_SELECTION_PRIMARY) &&
	    g_signal_handler_is_connected (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
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

	if (priv->header_model) {
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
	}

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;
	
	main_window = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr(),
							 FALSE); /* don't create */
	if (!main_window)
		return;
	
	header_view = MODEST_HEADER_VIEW(
			modest_main_window_get_child_widget(
				MODEST_MAIN_WINDOW(main_window),
				MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW));
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

	if (priv->other_body != NULL) {
		g_object_unref (priv->other_body);
		priv->other_body = NULL;
	}

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
		       gboolean cycle,
		       gboolean is_outbox)
{
	GtkTreeIter tmp_iter;
	GtkTreePath *path;
	GtkTreePath *next = NULL;
	gboolean retval = FALSE, finished;

	g_return_val_if_fail (gtk_tree_row_reference_valid (*row_reference), FALSE);

	path = gtk_tree_row_reference_get_path (*row_reference);
	gtk_tree_model_get_iter (model, &tmp_iter, path);
	gtk_tree_row_reference_free (*row_reference);
	*row_reference = NULL;

	finished = FALSE;
	do {
		TnyHeader *header = NULL;

		if (gtk_tree_model_iter_next (model, &tmp_iter)) {
			gtk_tree_model_get (model, &tmp_iter, 
					    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
					    &header, -1);

			if (header) {
				if (msg_is_visible (header, is_outbox)) {
					next = gtk_tree_model_get_path (model, &tmp_iter);
					*row_reference = gtk_tree_row_reference_new (model, next);
					gtk_tree_path_free (next);
					retval = TRUE;
					finished = TRUE;
				}
				g_object_unref (header);
				header = NULL;
			}
		} else if (cycle && gtk_tree_model_get_iter_first (model, &tmp_iter)) {
			next = gtk_tree_model_get_path (model, &tmp_iter);
			
			/* Ensure that we are not selecting the same */
			if (gtk_tree_path_compare (path, next) != 0) {
				gtk_tree_model_get (model, &tmp_iter, 
						    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
						    &header, -1);				
				if (header) {
					if (msg_is_visible (header, is_outbox)) {
						*row_reference = gtk_tree_row_reference_new (model, next);
						retval = TRUE;
						finished = TRUE;
					}
					g_object_unref (header);
					header = NULL;
				}
			} else {
				/* If we ended up in the same message
				   then there is no valid next
				   message */
				finished = TRUE;
			}
			gtk_tree_path_free (next);
		} else {
			/* If there are no more messages and we don't
			   want to start again in the first one then
			   there is no valid next message */
			finished = TRUE;
		}
	} while (!finished);

	/* Free */
	gtk_tree_path_free (path);

	return retval;
}

/* TODO: This should be in _init(), with the parameters as properties. */
static void
modest_msg_view_window_construct (ModestMsgViewWindow *self, 
				  const gchar *modest_account_name,
				  const gchar *msg_uid)
{
	GObject *obj = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;
	ModestDimmingRulesGroup *menu_rules_group = NULL;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	ModestDimmingRulesGroup *clipboard_rules_group = NULL;

	obj = G_OBJECT (self);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_uid = g_strdup (msg_uid);

	/* Menubar */
	parent_priv->menubar = modest_maemo_utils_get_manager_menubar_as_menu (parent_priv->ui_manager, "/MenuBar");
	hildon_window_set_menu    (HILDON_WINDOW(obj), GTK_MENU(parent_priv->menubar));
	gtk_widget_show (parent_priv->menubar);
	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();

	menu_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);
	toolbar_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_TOOLBAR, TRUE);
	clipboard_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_CLIPBOARD, FALSE);

	/* Add common dimming rules */
	modest_dimming_rules_group_add_rules (menu_rules_group, 
					      modest_msg_view_menu_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_menu_dimming_entries),
					      MODEST_WINDOW (self));
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_msg_view_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_toolbar_dimming_entries),
					      MODEST_WINDOW (self));
	modest_dimming_rules_group_add_rules (clipboard_rules_group, 
					      modest_msg_view_clipboard_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_clipboard_dimming_entries),
					      MODEST_WINDOW (self));

	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, menu_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, clipboard_rules_group);
	g_object_unref (menu_rules_group);
	g_object_unref (toolbar_rules_group);
	g_object_unref (clipboard_rules_group);

	restore_settings (MODEST_MSG_VIEW_WINDOW(obj));
	
 	/* g_signal_connect (G_OBJECT(obj), "delete-event", G_CALLBACK(on_delete_event), obj); */

	priv->clipboard_change_handler = g_signal_connect (G_OBJECT (gtk_clipboard_get (GDK_SELECTION_PRIMARY)), "owner-change", G_CALLBACK (modest_msg_view_window_clipboard_owner_change), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "activate_link",
			  G_CALLBACK (modest_ui_actions_on_msg_link_clicked), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "link_hover",
			  G_CALLBACK (modest_ui_actions_on_msg_link_hover), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "attachment_clicked",
			  G_CALLBACK (modest_ui_actions_on_msg_attachment_clicked), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "recpt_activated",
			  G_CALLBACK (modest_ui_actions_on_msg_recpt_activated), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "link_contextual",
			  G_CALLBACK (modest_ui_actions_on_msg_link_contextual), obj);
	g_signal_connect (G_OBJECT (priv->msg_view), "fetch_image",
			  G_CALLBACK (on_fetch_image), obj);

	g_signal_connect (G_OBJECT (obj), "key-release-event",
			  G_CALLBACK (modest_msg_view_window_key_event),
			  NULL);

	g_signal_connect (G_OBJECT (obj), "key-press-event",
			  G_CALLBACK (modest_msg_view_window_key_event),
			  NULL);

	g_signal_connect (G_OBJECT (obj), "window-state-event",
			  G_CALLBACK (modest_msg_view_window_window_state_event),
			  NULL);

	g_signal_connect (G_OBJECT (obj), "move-focus",
			  G_CALLBACK (on_move_focus), obj);

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

	g_signal_connect (G_OBJECT (priv->find_toolbar), "close", G_CALLBACK (modest_msg_view_window_find_toolbar_close), obj);
	g_signal_connect (G_OBJECT (priv->find_toolbar), "search", G_CALLBACK (modest_msg_view_window_find_toolbar_search), obj);
	priv->last_search = NULL;

	/* Init the clipboard actions dim status */
	modest_msg_view_grab_focus(MODEST_MSG_VIEW (priv->msg_view));

	update_window_title (MODEST_MSG_VIEW_WINDOW (obj));


}

/* FIXME: parameter checks */
ModestWindow *
modest_msg_view_window_new_with_header_model (TnyMsg *msg, 
					      const gchar *modest_account_name,
					      const gchar *mailbox, /*ignored */
					      const gchar *msg_uid,
					      GtkTreeModel *model, 
					      GtkTreeRowReference *row_reference)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	TnyFolder *header_folder = NULL;
	ModestHeaderView *header_view = NULL;
	ModestWindow *main_window = NULL;
	ModestWindowMgr *mgr = NULL;

	MODEST_DEBUG_BLOCK (
	       modest_tny_mime_part_to_string (TNY_MIME_PART (msg), 0);
	);

	mgr = modest_runtime_get_window_mgr ();
	window = MODEST_MSG_VIEW_WINDOW (modest_window_mgr_get_msg_view_window (mgr));
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);

	modest_msg_view_window_construct (window, modest_account_name, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Remember the message list's TreeModel so we can detect changes 
	 * and change the list selection when necessary: */

	main_window = modest_window_mgr_get_main_window(mgr, FALSE); /* don't create */
	if (main_window) {
		header_view = MODEST_HEADER_VIEW(modest_main_window_get_child_widget(
							 MODEST_MAIN_WINDOW(main_window),
							 MODEST_MAIN_WINDOW_WIDGET_TYPE_HEADER_VIEW));
	}
	
	if (header_view != NULL){
		header_folder = modest_header_view_get_folder(header_view);
		/* This could happen if the header folder was
		   unseleted before opening this msg window (for
		   example if the user selects an account in the
		   folder view of the main window */
		if (header_folder) {
			priv->is_outbox = (modest_tny_folder_guess_folder_type (header_folder) == TNY_FOLDER_TYPE_OUTBOX);
			priv->header_folder_id = tny_folder_get_id(header_folder);
			g_assert(priv->header_folder_id != NULL);
			g_object_unref(header_folder);
		}
	}

	/* Setup row references and connect signals */
	priv->header_model = g_object_ref (model);

	if (row_reference) {
		priv->row_reference = gtk_tree_row_reference_copy (row_reference);
		priv->next_row_reference = gtk_tree_row_reference_copy (row_reference);
		select_next_valid_row (model, &(priv->next_row_reference), TRUE, priv->is_outbox);
	} else {
		priv->row_reference = NULL;
		priv->next_row_reference = NULL;
	}

	/* Connect signals */
	priv->row_changed_handler = 
		g_signal_connect (GTK_TREE_MODEL(model), "row-changed",
				  G_CALLBACK(modest_msg_view_window_on_row_changed),
				  window);
	priv->row_deleted_handler = 
		g_signal_connect (GTK_TREE_MODEL(model), "row-deleted",
				  G_CALLBACK(modest_msg_view_window_on_row_deleted),
				  window);
	priv->row_inserted_handler = 
		g_signal_connect (GTK_TREE_MODEL(model), "row-inserted",
				  G_CALLBACK(modest_msg_view_window_on_row_inserted),
				  window);
	priv->rows_reordered_handler = 
		g_signal_connect(GTK_TREE_MODEL(model), "rows-reordered",
				 G_CALLBACK(modest_msg_view_window_on_row_reordered),
				 window);

	if (header_view != NULL){
		modest_header_view_add_observer(header_view,
				MODEST_HEADER_VIEW_OBSERVER(window));
	}

	tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
	update_window_title (MODEST_MSG_VIEW_WINDOW (window));
	gtk_widget_show_all (GTK_WIDGET (window));
	modest_msg_view_window_update_priority (window);

	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);

	return MODEST_WINDOW(window);
}

ModestWindow *
modest_msg_view_window_new_for_search_result (TnyMsg *msg, 
					      const gchar *modest_account_name,
					      const gchar *mailbox, /*ignored*/
					      const gchar *msg_uid)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowMgr *mgr = NULL;

	mgr = modest_runtime_get_window_mgr ();
	window = MODEST_MSG_VIEW_WINDOW (modest_window_mgr_get_msg_view_window (mgr));
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);
	modest_msg_view_window_construct (window, modest_account_name, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Remember that this is a search result, 
	 * so we can disable some UI appropriately: */
	priv->is_search_result = TRUE;

	tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
	
	update_window_title (window);
	gtk_widget_show_all (GTK_WIDGET (window));
	modest_msg_view_window_update_priority (window);

	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);

	return MODEST_WINDOW(window);
}

gboolean
modest_msg_view_window_is_other_body (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	return (priv->other_body != NULL);
}

ModestWindow *
modest_msg_view_window_new_with_other_body (TnyMsg *msg, 
					    TnyMimePart *other_body,
					    const gchar *modest_account_name,
					    const gchar *mailbox, /* ignored */
					    const gchar *msg_uid)
{
	GObject *obj = NULL;
	ModestMsgViewWindowPrivate *priv;	
	ModestWindowMgr *mgr = NULL;

	g_return_val_if_fail (msg, NULL);
	mgr = modest_runtime_get_window_mgr ();
	obj = G_OBJECT (modest_window_mgr_get_msg_view_window (mgr));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);
	modest_msg_view_window_construct (MODEST_MSG_VIEW_WINDOW (obj), 
		modest_account_name, msg_uid);

	if (other_body) {
		priv->other_body = g_object_ref (other_body);
		modest_msg_view_set_msg_with_other_body (MODEST_MSG_VIEW (priv->msg_view), msg, other_body);
	} else {
		tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
	}
	update_window_title (MODEST_MSG_VIEW_WINDOW (obj));

	gtk_widget_show_all (GTK_WIDGET (obj));

	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (obj));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (obj));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (obj), MODEST_DIMMING_RULES_CLIPBOARD);

	return MODEST_WINDOW(obj);
}

ModestWindow *
modest_msg_view_window_new_for_attachment (TnyMsg *msg, 
					   const gchar *modest_account_name,
					   const gchar *mailbox, /* ignored */
					   const gchar *msg_uid)
{
	return modest_msg_view_window_new_with_other_body (msg, NULL, modest_account_name, mailbox, msg_uid);

}

static void
modest_msg_view_window_on_row_changed (GtkTreeModel *header_model,
				       GtkTreePath *arg1,
				       GtkTreeIter *arg2,
				       ModestMsgViewWindow *window)
{
	check_dimming_rules_after_change (window);
}

static void 
modest_msg_view_window_on_row_deleted(GtkTreeModel *header_model,
				      GtkTreePath *arg1,
				      ModestMsgViewWindow *window)
{
	check_dimming_rules_after_change (window);
}
	/* The window could have dissapeared */

static void
check_dimming_rules_after_change (ModestMsgViewWindow *window)
{
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
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
static void 
modest_msg_view_window_on_row_inserted (GtkTreeModel *model,
				        GtkTreePath *tree_path,
					GtkTreeIter *tree_iter,
					ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv = NULL; 
	TnyHeader *header = NULL;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	g_assert (model == priv->header_model);
	
	/* Check if the newly inserted message is the same we are actually
	 * showing. IF not, we should remain detached from the header model
	 * and thus prev and next toolbar buttons should remain dimmed. */
	gtk_tree_model_get (model, tree_iter, 
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, 
			    &header, -1);

	if (TNY_IS_HEADER (header)) {
		gchar *uid = NULL;

		uid = modest_tny_folder_get_header_unique_id (header);
		if (!g_str_equal(priv->msg_uid, uid)) {
			check_dimming_rules_after_change (window);
			g_free(uid);
			g_object_unref (G_OBJECT(header));
			return;
		}
		g_free(uid);
		g_object_unref(G_OBJECT(header));
	}

	if (priv->row_reference) {
		gtk_tree_row_reference_free (priv->row_reference); 
	}

	/* Setup row_reference for the actual msg. */
	priv->row_reference = gtk_tree_row_reference_new (priv->header_model, tree_path);
	if (priv->row_reference == NULL) {
		g_warning("No reference for msg header item.");
		return;
	}

	/* Now set up next_row_reference. */
	if (priv->next_row_reference) {
		gtk_tree_row_reference_free (priv->next_row_reference); 
	}

	priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
	select_next_valid_row (priv->header_model,
			       &(priv->next_row_reference), FALSE, priv->is_outbox);

	/* Connect the remaining callbacks to become able to detect
	 * changes in header-view. */
	priv->row_changed_handler = 
		g_signal_connect (priv->header_model, "row-changed",
				  G_CALLBACK (modest_msg_view_window_on_row_changed),
				  window);
	priv->row_deleted_handler = 
		g_signal_connect (priv->header_model, "row-deleted",
				  G_CALLBACK (modest_msg_view_window_on_row_deleted),
				  window);
	priv->rows_reordered_handler = 
		g_signal_connect (priv->header_model, "rows-reordered",
				  G_CALLBACK (modest_msg_view_window_on_row_reordered),
				  window);

	check_dimming_rules_after_change (window);	
}

static void 
modest_msg_view_window_on_row_reordered (GtkTreeModel *header_model,
					 GtkTreePath *arg1,
					 GtkTreeIter *arg2,
					 gpointer arg3,
					 ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv = NULL; 
	gboolean already_changed = FALSE;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(window);

	/* If the current row was reordered select the proper next
	   valid row. The same if the next row reference changes */
	if (priv->row_reference && 
	    gtk_tree_row_reference_valid (priv->row_reference)) {
		GtkTreePath *path;
		path = gtk_tree_row_reference_get_path (priv->row_reference);
		if (gtk_tree_path_compare (path, arg1) == 0) {
			if (priv->next_row_reference) {
				gtk_tree_row_reference_free (priv->next_row_reference);
			}
			priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
			select_next_valid_row (header_model, &(priv->next_row_reference), FALSE, priv->is_outbox);
			already_changed = TRUE;
		}
		gtk_tree_path_free (path);
	}
	if (!already_changed &&
	    priv->next_row_reference &&
	    gtk_tree_row_reference_valid (priv->next_row_reference)) {
		GtkTreePath *path;
		path = gtk_tree_row_reference_get_path (priv->next_row_reference);
		if (gtk_tree_path_compare (path, arg1) == 0) {
			if (priv->next_row_reference) {
				gtk_tree_row_reference_free (priv->next_row_reference);
			}
			priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
			select_next_valid_row (header_model, &(priv->next_row_reference), FALSE, priv->is_outbox);
		}
		gtk_tree_path_free (path);
	}
	check_dimming_rules_after_change (window);
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
static void 
modest_msg_view_window_update_model_replaced (ModestHeaderViewObserver *observer,
					      GtkTreeModel *model,
					      const gchar *tny_folder_id)
{
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
	if(model == NULL || tny_folder_id == NULL || 
	   (priv->header_folder_id && !g_str_equal(tny_folder_id, priv->header_folder_id)))
		return;

	/* Model is changed(replaced), so we should forget the old
	 * one. Because there might be other references and there
	 * might be some change on the model even if we unreferenced
	 * it, we need to disconnect our signals here. */
	if (priv->header_model) {
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

		/* Frees */
		if (priv->row_reference)
			gtk_tree_row_reference_free (priv->row_reference);
		if (priv->next_row_reference)
			gtk_tree_row_reference_free (priv->next_row_reference);
		g_object_unref(priv->header_model);

		/* Initialize */
		priv->row_changed_handler = 0;
		priv->row_deleted_handler = 0;
		priv->row_inserted_handler = 0;
		priv->rows_reordered_handler = 0;
		priv->next_row_reference = NULL;
		priv->row_reference = NULL;
		priv->header_model = NULL;
	}

	priv->header_model = g_object_ref (model);

	/* Also we must connect to the new model for row insertions.
	 * Only for insertions now. We will need other ones only after
	 * the msg is show by msg-view is added to the new model. */
	priv->row_inserted_handler =
		g_signal_connect (priv->header_model, "row-inserted",
				  G_CALLBACK(modest_msg_view_window_on_row_inserted),
				  window);

	modest_ui_actions_check_menu_dimming_rules(MODEST_WINDOW(window));
	modest_ui_actions_check_toolbar_dimming_rules(MODEST_WINDOW(window));
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
	if (priv->header_model == NULL || 
	    priv->row_reference == NULL ||
	    !gtk_tree_row_reference_valid (priv->row_reference)) {
		msg = modest_msg_view_window_get_message (self);
		if (msg) {
			header = tny_msg_get_header (msg);
			g_object_unref (msg);
		}
		return header;
	}

	/* Get iter of the currently selected message in the header view: */
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
	ModestMsgViewWindowPrivate *priv;
	
	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);
	
	return tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
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
		modest_msg_view_grab_focus (MODEST_MSG_VIEW (priv->msg_view));
	}

	/* update the toggle buttons status */
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/FindInMessage");
	modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), is_active);
	action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ToolsMenu/ToolsFindInMessageMenu");
	modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action), is_active);
	
}

static void
modest_msg_view_window_find_toolbar_close (GtkWidget *widget,
					   ModestMsgViewWindow *obj)
{
	GtkToggleAction *toggle;
	ModestWindowPrivate *parent_priv;
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (obj);
	
	toggle = GTK_TOGGLE_ACTION (gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/FindInMessage"));
	gtk_toggle_action_set_active (toggle, FALSE);
	modest_msg_view_grab_focus (MODEST_MSG_VIEW (priv->msg_view));
}

static void
modest_msg_view_window_find_toolbar_search (GtkWidget *widget,
					   ModestMsgViewWindow *obj)
{
	gchar *current_search;
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);

	if (modest_mime_part_view_is_empty (MODEST_MIME_PART_VIEW (priv->msg_view))) {
		hildon_banner_show_information (NULL, NULL, _("mail_ib_nothing_to_find"));
		return;
	}

	g_object_get (G_OBJECT (widget), "prefix", &current_search, NULL);

	if ((current_search == NULL) || (strcmp (current_search, "") == 0)) {
		g_free (current_search);
		hildon_banner_show_information (NULL, NULL, _CS("ecdg_ib_find_rep_enter_text"));
		return;
	}

	if ((priv->last_search == NULL) || (strcmp (priv->last_search, current_search) != 0)) {
		gboolean result;
		g_free (priv->last_search);
		priv->last_search = g_strdup (current_search);
		result = modest_isearch_view_search (MODEST_ISEARCH_VIEW (priv->msg_view),
						     priv->last_search);
		if (!result) {
			hildon_banner_show_information (NULL, NULL, _HL("ckct_ib_find_no_matches"));
			g_free (priv->last_search);
			priv->last_search = NULL;
		} else {
			modest_msg_view_grab_focus (MODEST_MSG_VIEW (priv->msg_view));
			hildon_find_toolbar_highlight_entry (HILDON_FIND_TOOLBAR (priv->find_toolbar), TRUE);
		}
	} else {
		if (!modest_isearch_view_search_next (MODEST_ISEARCH_VIEW (priv->msg_view))) {
			hildon_banner_show_information (NULL, NULL, _HL("ckct_ib_find_search_complete"));
			g_free (priv->last_search);
			priv->last_search = NULL;
		} else {
			modest_msg_view_grab_focus (MODEST_MSG_VIEW (priv->msg_view));
			hildon_find_toolbar_highlight_entry (HILDON_FIND_TOOLBAR (priv->find_toolbar), TRUE);
		}
	}
	
	g_free (current_search);
		
}

static void
modest_msg_view_window_set_zoom (ModestWindow *window,
				 gdouble zoom)
{
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkAction *action = NULL;
	gint int_zoom = (gint) rint (zoom*100.0+0.1);
     
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	modest_zoomable_set_zoom (MODEST_ZOOMABLE (priv->msg_view), zoom);

	action = gtk_ui_manager_get_action (parent_priv->ui_manager, 
					    "/MenuBar/ViewMenu/ZoomMenu/Zoom50Menu");

	gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action), int_zoom);
}

static gdouble
modest_msg_view_window_get_zoom (ModestWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
     
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), 1.0);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	return modest_zoomable_get_zoom (MODEST_ZOOMABLE (priv->msg_view));
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
		hildon_banner_show_information (NULL, NULL, _CS("ckct_ib_max_zoom_level_reached"));
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
			  hildon_banner_show_information (NULL, NULL, 
							  _CS("ckct_ib_min_zoom_level_reached"));
				return FALSE;
			}
			break;
		}
	}
	return FALSE;
}

static gboolean
modest_msg_view_window_key_event (GtkWidget *window,
				  GdkEventKey *event,
				  gpointer userdata)
{
	GtkWidget *focus;

	focus = gtk_window_get_focus (GTK_WINDOW (window));

	/* for the find toolbar case */
	if (focus && GTK_IS_ENTRY (focus)) {
		if (event->keyval == GDK_BackSpace) {
			GdkEvent *copy;
			copy = gdk_event_copy ((GdkEvent *) event);
			gtk_widget_event (focus, copy);
			gdk_event_free (copy);
			return TRUE;
		} else 
			return FALSE;
	}
	if (event->keyval == GDK_Up || event->keyval == GDK_KP_Up ||
	    event->keyval == GDK_Down || event->keyval == GDK_KP_Down ||
	    event->keyval == GDK_Page_Up || event->keyval == GDK_KP_Page_Up ||
	    event->keyval == GDK_Page_Down || event->keyval == GDK_KP_Page_Down ||
	    event->keyval == GDK_Home || event->keyval == GDK_KP_Home ||
	    event->keyval == GDK_End || event->keyval == GDK_KP_End) {
		/* ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window); */
		/* gboolean return_value; */

		if (event->type == GDK_KEY_PRESS) {
			GtkScrollType scroll_type;
			
			switch (event->keyval) {
			case GDK_Up: 
			case GDK_KP_Up:
				scroll_type = GTK_SCROLL_STEP_UP; break;
			case GDK_Down: 
			case GDK_KP_Down:
				scroll_type = GTK_SCROLL_STEP_DOWN; break;
			case GDK_Page_Up:
			case GDK_KP_Page_Up:
				scroll_type = GTK_SCROLL_PAGE_UP; break;
			case GDK_Page_Down:
			case GDK_KP_Page_Down:
				scroll_type = GTK_SCROLL_PAGE_DOWN; break;
			case GDK_Home:
			case GDK_KP_Home:
				scroll_type = GTK_SCROLL_START; break;
			case GDK_End:
			case GDK_KP_End:
				scroll_type = GTK_SCROLL_END; break;
			default: scroll_type = GTK_SCROLL_NONE;
			}
			
			/* g_signal_emit_by_name (G_OBJECT (priv->main_scroll), "scroll-child",  */
			/* 		       scroll_type, FALSE, &return_value); */
			return FALSE;
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}
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
	if (!priv->header_model || !priv->row_reference)
		return TRUE;

	if (!gtk_tree_row_reference_valid (priv->row_reference))
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
		if (header) {
			if (msg_is_visible (header, priv->is_outbox))
				is_last_selected = FALSE;
			g_object_unref(G_OBJECT(header));
		}
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

static gboolean
msg_is_visible (TnyHeader *header, gboolean check_outbox)
{
	if ((tny_header_get_flags(header) & TNY_HEADER_FLAG_DELETED))
		return FALSE;
	if (!check_outbox) {
		return TRUE;
	} else {
		ModestTnySendQueueStatus status;
		status = modest_tny_all_send_queues_get_msg_status (header);
		return ((status != MODEST_TNY_SEND_QUEUE_FAILED) &&
			(status != MODEST_TNY_SEND_QUEUE_SENDING));
	}
}

gboolean
modest_msg_view_window_first_message_selected (ModestMsgViewWindow *window)
{
	GtkTreePath *path;
	ModestMsgViewWindowPrivate *priv;
	gboolean is_first_selected;
	GtkTreeIter tmp_iter;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), TRUE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/*if no model (so no rows at all), then virtually we are the first*/
	if (!priv->header_model || !priv->row_reference)
		return TRUE;

	if (!gtk_tree_row_reference_valid (priv->row_reference))
		return TRUE;

	path = gtk_tree_row_reference_get_path (priv->row_reference);
	if (!path)
		return TRUE;

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
		if (header) {
			if (msg_is_visible (header, priv->is_outbox))
				is_first_selected = FALSE;
			g_object_unref(G_OBJECT(header));
		}
	}
	gtk_tree_path_free (path);
	return is_first_selected;
}

typedef struct {
	TnyHeader *header;
	GtkTreeRowReference *row_reference;
} MsgReaderInfo;

static void
message_reader_performer (gboolean canceled, 
			  GError *err,
			  GtkWindow *parent_window, 
			  TnyAccount *account, 
			  gpointer user_data)
{
	ModestMailOperation *mail_op = NULL;
	MsgReaderInfo *info;

	info = (MsgReaderInfo *) user_data;
	if (canceled || err) {
		goto frees;
	}

	/* Register the header - it'll be unregistered in the callback */
	modest_window_mgr_register_header (modest_runtime_get_window_mgr (), info->header, NULL);

	/* New mail operation */
	mail_op = modest_mail_operation_new_with_error_handling (G_OBJECT(parent_window),
								 modest_ui_actions_disk_operations_error_handler, 
								 NULL, NULL);
				
	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	modest_mail_operation_get_msg (mail_op, info->header, TRUE, view_msg_cb, info->row_reference);
	g_object_unref (mail_op);

	/* Update dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (parent_window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (parent_window));

 frees:
	/* Frees. The row_reference will be freed by the view_msg_cb callback */
	g_object_unref (info->header);
	g_slice_free (MsgReaderInfo, info);
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
		GtkTreeRowReference *row_reference)
{
	gboolean already_showing = FALSE;
	ModestWindow *msg_window = NULL;
	ModestWindowMgr *mgr;
	TnyAccount *account;
	TnyFolder *folder;
	MsgReaderInfo *info;

	g_return_val_if_fail (row_reference != NULL, FALSE);

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
	if (!(tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED)) {
		/* Ask the user if he wants to download the message if
		   we're not online */
		if (!tny_device_is_online (modest_runtime_get_device())) {
			GtkResponseType response;

			response = modest_platform_run_confirmation_dialog (GTK_WINDOW (window),
									    _("mcen_nc_get_msg"));
			if (response == GTK_RESPONSE_CANCEL)
				return FALSE;
		
			folder = tny_header_get_folder (header);
			info = g_slice_new (MsgReaderInfo);
			info->header = g_object_ref (header);
			info->row_reference = gtk_tree_row_reference_copy (row_reference);

			/* Offer the connection dialog if necessary */
			modest_platform_connect_if_remote_and_perform ((GtkWindow *) window, 
								       TRUE,
								       TNY_FOLDER_STORE (folder),
								       message_reader_performer, 
								       info);
			g_object_unref (folder);
			return TRUE;
		}
	}
	
	folder = tny_header_get_folder (header);
	account = tny_folder_get_account (folder);
	info = g_slice_new (MsgReaderInfo);
	info->header = g_object_ref (header);
	info->row_reference = gtk_tree_row_reference_copy (row_reference);
	
	message_reader_performer (FALSE, NULL, (GtkWindow *) window, account, info);
	g_object_unref (account);
	g_object_unref (folder);

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
	GtkTreeRowReference *row_reference = NULL;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (!priv->row_reference)
		return FALSE;

	/* Update the next row reference if it's not valid. This could
	   happen if for example the header which it was pointing to,
	   was deleted. The best place to do it is in the row-deleted
	   handler but the tinymail model do not work like the glib
	   tree models and reports the deletion when the row is still
	   there */
	if (!gtk_tree_row_reference_valid (priv->next_row_reference)) {
		if (gtk_tree_row_reference_valid (priv->row_reference)) {
			priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
			select_next_valid_row (priv->header_model, &(priv->next_row_reference), FALSE, priv->is_outbox);
		}
	}
	if (priv->next_row_reference)
		path = gtk_tree_row_reference_get_path (priv->next_row_reference);
	if (path == NULL)
		return FALSE;

	row_reference = gtk_tree_row_reference_copy (priv->next_row_reference);

	gtk_tree_model_get_iter (priv->header_model,
				 &tmp_iter,
				 path);
	gtk_tree_path_free (path);

	gtk_tree_model_get (priv->header_model, &tmp_iter, 
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);
	
	/* Read the message & show it */
	if (!message_reader (window, priv, header, row_reference)) {
		retval = FALSE;
	}
	gtk_tree_row_reference_free (row_reference);

	/* Free */
	g_object_unref (header);

	return retval;
}

gboolean        
modest_msg_view_window_select_previous_message (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv = NULL;
	GtkTreePath *path;
	gboolean finished = FALSE;
	gboolean retval = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Return inmediatly if there is no header model */
	if (!priv->header_model || !priv->row_reference)
		return FALSE;

	path = gtk_tree_row_reference_get_path (priv->row_reference);
	while (!finished && gtk_tree_path_prev (path)) {
		TnyHeader *header;
		GtkTreeIter iter;

		gtk_tree_model_get_iter (priv->header_model, &iter, path);
		gtk_tree_model_get (priv->header_model, &iter, 
				    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				    &header, -1);
		finished = TRUE;
		if (header) {
			if (msg_is_visible (header, priv->is_outbox)) {
				GtkTreeRowReference *row_reference;
				row_reference = gtk_tree_row_reference_new (priv->header_model, path);
				/* Read the message & show it */
				retval = message_reader (window, priv, header, row_reference);
				gtk_tree_row_reference_free (row_reference);
			} else {
				finished = FALSE;
			}
			g_object_unref (header);
		}
	}

	gtk_tree_path_free (path);
	return retval;
}

static void
view_msg_cb (ModestMailOperation *mail_op, 
	     TnyHeader *header, 
	     gboolean canceled,
	     TnyMsg *msg, 
	     GError *error,
	     gpointer user_data)
{
	ModestMsgViewWindow *self = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	GtkTreeRowReference *row_reference = NULL;

	/* Unregister the header (it was registered before creating the mail operation) */
	modest_window_mgr_unregister_header (modest_runtime_get_window_mgr (), header);

	row_reference = (GtkTreeRowReference *) user_data;
	if (canceled) {
		gtk_tree_row_reference_free (row_reference);
		return;
	}
	
	/* If there was any error */
	if (!modest_ui_actions_msg_retrieval_check (mail_op, header, msg)) {
		gtk_tree_row_reference_free (row_reference);			
		return;
	}

	/* Get the window */ 
	self = (ModestMsgViewWindow *) modest_mail_operation_get_source (mail_op);
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	/* Update the row reference */
	if (priv->row_reference != NULL) {
		gtk_tree_row_reference_free (priv->row_reference);
		priv->row_reference = gtk_tree_row_reference_copy (row_reference);
		if (priv->next_row_reference != NULL) {
			gtk_tree_row_reference_free (priv->next_row_reference);
		}
		priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
		select_next_valid_row (priv->header_model, &(priv->next_row_reference), TRUE, priv->is_outbox);
	}

	/* Mark header as read */
	if (!(tny_header_get_flags (header) & TNY_HEADER_FLAG_SEEN))
		tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);

	/* Set new message */
	if (priv->msg_view != NULL && TNY_IS_MSG_VIEW (priv->msg_view)) {
		tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
		modest_msg_view_window_update_priority (self);
		update_window_title (MODEST_MSG_VIEW_WINDOW (self));
		modest_msg_view_grab_focus (MODEST_MSG_VIEW (priv->msg_view));
	}

	/* Set the new message uid of the window  */
	if (priv->msg_uid) {
		g_free (priv->msg_uid);
		priv->msg_uid = modest_tny_folder_get_header_unique_id (header);
	}

	/* Notify the observers */
	g_signal_emit (G_OBJECT (self), signals[MSG_CHANGED_SIGNAL], 
		       0, priv->header_model, priv->row_reference);

	/* Frees */
	g_object_unref (self);
	gtk_tree_row_reference_free (row_reference);		
}

TnyFolderType
modest_msg_view_window_get_folder_type (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	TnyMsg *msg;
	TnyFolderType folder_type;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	folder_type = TNY_FOLDER_TYPE_UNKNOWN;

	msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
	if (msg) {
		TnyFolder *folder;

		folder = tny_msg_get_folder (msg);
		if (folder) {
			folder_type = modest_tny_folder_guess_folder_type (folder);
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
	TnyHeader *header = NULL;
	TnyHeaderFlags flags = 0;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->header_model && priv->row_reference) {
		GtkTreeIter iter;
		GtkTreePath *path = NULL;

		path = gtk_tree_row_reference_get_path (priv->row_reference);
		g_return_if_fail (path != NULL);
		gtk_tree_model_get_iter (priv->header_model, 
					 &iter, 
					 gtk_tree_row_reference_get_path (priv->row_reference));

		gtk_tree_model_get (priv->header_model, &iter, TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				    &header, -1);
		gtk_tree_path_free (path);
	} else {
		TnyMsg *msg;
		msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
		if (msg) {
			header = tny_msg_get_header (msg);
			g_object_unref (msg);
		}
	}

	if (header) {
		flags = tny_header_get_flags (header);
		g_object_unref(G_OBJECT(header));
	}

	modest_msg_view_set_priority (MODEST_MSG_VIEW(priv->msg_view), flags);

}

static void
toolbar_resize (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;
	GtkWidget *widget;
	gint static_button_size;
	ModestWindowMgr *mgr;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	mgr = modest_runtime_get_window_mgr ();
	static_button_size = modest_window_mgr_get_fullscreen_mode (mgr)?118:108;

	if (parent_priv->toolbar) {
		/* left size buttons */
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageReply");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageMoveTo");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarDeleteMessage");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/FindInMessage");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
		gtk_widget_set_size_request (GTK_WIDGET (widget), static_button_size, -1);
		
 		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->progress_toolitem), FALSE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->progress_toolitem), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->cancel_toolitem), FALSE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->next_toolitem), TRUE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->next_toolitem), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->prev_toolitem), TRUE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->prev_toolitem), TRUE);
	}
		
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
		toolbar_resize (MODEST_MSG_VIEW_WINDOW (widget));
	}

	return FALSE;

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

		priv->progress_toolitem = GTK_WIDGET (gtk_tool_item_new ());
		priv->cancel_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarCancel");
		priv->next_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageNext");
		priv->prev_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageBack");
		toolbar_resize (MODEST_MSG_VIEW_WINDOW (self));

		/* Add ProgressBar (Transfer toolbar) */ 
		priv->progress_bar = modest_progress_bar_new ();
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
		if (modest_msg_view_window_transfer_mode_enabled (MODEST_MSG_VIEW_WINDOW (self))) 
			set_toolbar_mode (MODEST_MSG_VIEW_WINDOW (self), TOOLBAR_MODE_TRANSFER);
		else
			set_toolbar_mode (MODEST_MSG_VIEW_WINDOW (self), TOOLBAR_MODE_NORMAL);

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
	modest_utils_toggle_action_set_active_block_notify (GTK_TOGGLE_ACTION (action),
							    show_toolbar);
}

static void 
modest_msg_view_window_clipboard_owner_change (GtkClipboard *clipboard,
					       GdkEvent *event,
					       ModestMsgViewWindow *window)
{
	if (!GTK_WIDGET_VISIBLE (window))
		return;

	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);
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
on_mail_operation_started (ModestMailOperation *mail_op,
			   gpointer user_data)
{
	ModestMsgViewWindow *self;
	ModestMailOperationTypeOperation op_type;
	GSList *tmp;
	ModestMsgViewWindowPrivate *priv;
	GObject *source = NULL;

	self = MODEST_MSG_VIEW_WINDOW (user_data);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);
	op_type = modest_mail_operation_get_type_operation (mail_op);
	tmp = priv->progress_widgets;
	source = modest_mail_operation_get_source(mail_op);
	if (G_OBJECT (self) == source) {
		if (op_type == MODEST_MAIL_OPERATION_TYPE_RECEIVE ) {
			set_toolbar_transfer_mode(self);
			while (tmp) {
				modest_progress_object_add_operation (
						MODEST_PROGRESS_OBJECT (tmp->data),
						mail_op);
				tmp = g_slist_next (tmp);
			}
		}
	}
	g_object_unref (source);
}

static void 
on_mail_operation_finished (ModestMailOperation *mail_op,
			    gpointer user_data)
{
	ModestMsgViewWindow *self;
	ModestMailOperationTypeOperation op_type;
	GSList *tmp;
	ModestMsgViewWindowPrivate *priv;
	
	self = MODEST_MSG_VIEW_WINDOW (user_data);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);
	op_type = modest_mail_operation_get_type_operation (mail_op);
	tmp = priv->progress_widgets;
	
	if (op_type == MODEST_MAIL_OPERATION_TYPE_RECEIVE ) {
		while (tmp) {
			modest_progress_object_remove_operation (MODEST_PROGRESS_OBJECT (tmp->data),
								 mail_op);
			tmp = g_slist_next (tmp);
		}

		/* If no more operations are being observed, NORMAL mode is enabled again */
		if (observers_empty (self)) {
			set_toolbar_mode (self, TOOLBAR_MODE_NORMAL);
		}

		/* Update dimming rules. We have to do this right here
		   and not in view_msg_cb because at that point the
		   transfer mode is still enabled so the dimming rule
		   won't let the user delete the message that has been
		   readed for example */
		modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (self));
		modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
	}
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestMsgViewWindow *self)
{	
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

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

TnyList *
modest_msg_view_window_get_attachments (ModestMsgViewWindow *win) 
{
	ModestMsgViewWindowPrivate *priv;
	TnyList *selected_attachments = NULL;
	
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (win), NULL);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (win);

	selected_attachments = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
	
	return selected_attachments;
}

typedef struct {
	gchar *filepath;
	GtkWidget *banner;
	guint banner_idle_id;
} DecodeAsyncHelper;

static gboolean
decode_async_banner_idle (gpointer user_data)
{
	DecodeAsyncHelper *helper = (DecodeAsyncHelper *) user_data;

	helper->banner_idle_id = 0;
	helper->banner = hildon_banner_show_animation (NULL, NULL, _("mail_me_opening"));
	g_object_ref (helper->banner);

	return FALSE;
}

static void
on_decode_to_stream_async_handler (TnyMimePart *mime_part, 
				   gboolean cancelled, 
				   TnyStream *stream, 
				   GError *err, 
				   gpointer user_data)
{
	DecodeAsyncHelper *helper = (DecodeAsyncHelper *) user_data;

	if (helper->banner_idle_id > 0) {
		g_source_remove (helper->banner_idle_id);
		helper->banner_idle_id = 0;
	}
	if (helper->banner) {
		gtk_widget_destroy (helper->banner);
	}
	if (cancelled || err) {
		modest_platform_information_banner (NULL, NULL, 
						    _("mail_ib_file_operation_failed"));
		goto free;
	}

	/* make the file read-only */
	g_chmod(helper->filepath, 0444);
	
	/* Activate the file */
	modest_platform_activate_file (helper->filepath, modest_tny_mime_part_get_content_type (mime_part));

 free:
	/* Frees */
	g_free (helper->filepath);
	g_object_unref (helper->banner);
	g_slice_free (DecodeAsyncHelper, helper);
}

void
modest_msg_view_window_view_attachment (ModestMsgViewWindow *window, 
					TnyMimePart *mime_part)
{
	ModestMsgViewWindowPrivate *priv;
	const gchar *msg_uid;
	gchar *attachment_uid = NULL;
	gint attachment_index = 0;
	TnyList *attachments;
	TnyMimePart *window_msg;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	g_return_if_fail (TNY_IS_MIME_PART (mime_part) || (mime_part == NULL));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	msg_uid = modest_msg_view_window_get_message_uid (MODEST_MSG_VIEW_WINDOW (window));
	attachments = modest_msg_view_get_attachments (MODEST_MSG_VIEW (priv->msg_view));
	attachment_index = modest_list_index (attachments, (GObject *) mime_part);
	g_object_unref (attachments);
	
	if (msg_uid && attachment_index >= 0) {
		attachment_uid = g_strdup_printf ("%s/%d", msg_uid, attachment_index);
	}

	if (mime_part == NULL) {
		gboolean error = FALSE;
		TnyList *selected_attachments = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
		if (selected_attachments == NULL || tny_list_get_length (selected_attachments) == 0) {
			error = TRUE;
		} else if (tny_list_get_length (selected_attachments) > 1) {
			hildon_banner_show_information (NULL, NULL, _("mcen_ib_unable_to_display_more"));
			error = TRUE;
		} else {
			TnyIterator *iter;
			iter = tny_list_create_iterator (selected_attachments);
			mime_part = (TnyMimePart *) tny_iterator_get_current (iter);
			g_object_unref (iter);
		}
		if (selected_attachments)
		  g_object_unref (selected_attachments);

		if (error)
			return;
	} else {
		g_object_ref (mime_part);
	}

	if (tny_mime_part_is_purged (mime_part)) {
		g_object_unref (mime_part);
		return;
	}

	/* we also check for mime_part == priv->msg, as this means it's a direct attachment
	 * shown as attachment, so it should behave as a file */
	window_msg = TNY_MIME_PART (tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view)));
	if ((!modest_tny_mime_part_is_msg (mime_part) && tny_mime_part_get_filename (mime_part)) ||
	    mime_part == window_msg) {
		gchar *filepath = NULL;
		const gchar *att_filename = tny_mime_part_get_filename (mime_part);
		gboolean show_error_banner = FALSE;
		TnyFsStream *temp_stream = NULL;
		temp_stream = modest_utils_create_temp_stream (att_filename, attachment_uid,
							       &filepath);
		
		if (temp_stream != NULL) {
			DecodeAsyncHelper *helper = g_slice_new (DecodeAsyncHelper);
			helper->filepath = g_strdup (filepath);
			helper->banner = NULL;
			helper->banner_idle_id = g_timeout_add (1000, decode_async_banner_idle, helper);
			tny_mime_part_decode_to_stream_async (mime_part, TNY_STREAM (temp_stream), 
							      on_decode_to_stream_async_handler, 
							      NULL, 
							      helper);
			g_object_unref (temp_stream);
			/* NOTE: files in the temporary area will be automatically
			 * cleaned after some time if they are no longer in use */
		} else {
			if (filepath) {
				const gchar *content_type;
				/* the file may already exist but it isn't writable,
				 * let's try to open it anyway */
				content_type = modest_tny_mime_part_get_content_type (mime_part);
				modest_platform_activate_file (filepath, content_type);
			} else {
				g_warning ("%s: modest_utils_create_temp_stream failed", __FUNCTION__);
				show_error_banner = TRUE;
			}
		}
		if (filepath)
			g_free (filepath);
		if (show_error_banner)
			modest_platform_information_banner (NULL, NULL, _("mail_ib_file_operation_failed"));
	} else if (!modest_tny_mime_part_is_msg (mime_part)) {
		ModestWindowMgr *mgr;
		ModestWindow *msg_win = NULL;
		TnyMsg *current_msg;
		gboolean found;
		TnyHeader *header;

		current_msg = modest_msg_view_window_get_message (MODEST_MSG_VIEW_WINDOW (window));
		mgr = modest_runtime_get_window_mgr ();
		header = tny_msg_get_header (TNY_MSG (current_msg));
		found = modest_window_mgr_find_registered_message_uid (mgr,
								       attachment_uid,
								       &msg_win);
		
		if (found) {
			g_debug ("window for this body is already being created");
		} else {

			/* it's not found, so create a new window for it */
			modest_window_mgr_register_header (mgr, header, attachment_uid); /* register the uid before building the window */
			gchar *account = g_strdup (modest_window_get_active_account (MODEST_WINDOW (window)));
			const gchar *mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (window));
			if (!account)
				account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr ());
			
			msg_win = modest_msg_view_window_new_with_other_body (TNY_MSG (current_msg), TNY_MIME_PART (mime_part),
									      account, mailbox, attachment_uid);
			
			modest_window_set_zoom (MODEST_WINDOW (msg_win),
						modest_window_get_zoom (MODEST_WINDOW (window)));
			if (modest_window_mgr_register_window (mgr, msg_win, MODEST_WINDOW (window)))
				gtk_widget_show_all (GTK_WIDGET (msg_win));
			else
				gtk_widget_destroy (GTK_WIDGET (msg_win));
		}
		g_object_unref (current_msg);		
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
				g_debug ("window for is already being created");
		} else { 
			/* it's not found, so create a new window for it */
			modest_window_mgr_register_header (mgr, header, attachment_uid); /* register the uid before building the window */
			gchar *account = g_strdup (modest_window_get_active_account (MODEST_WINDOW (window)));
			if (!account)
				account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr ());
			msg_win = modest_msg_view_window_new_for_attachment (TNY_MSG (mime_part), account, 
									     NULL, attachment_uid);
			modest_window_set_zoom (MODEST_WINDOW (msg_win), 
						modest_window_get_zoom (MODEST_WINDOW (window)));
			modest_window_mgr_register_window (mgr, msg_win, MODEST_WINDOW (window));
			gtk_widget_show_all (GTK_WIDGET (msg_win));
		}
	}
	g_object_unref (window_msg);
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
	GnomeVFSResult result;
} SaveMimePartInfo;

static void save_mime_part_info_free (SaveMimePartInfo *info, gboolean with_struct);
static gboolean idle_save_mime_part_show_result (SaveMimePartInfo *info);
static gpointer save_mime_part_to_file (SaveMimePartInfo *info);
static void save_mime_parts_to_file_with_checks (GtkWindow *parent, SaveMimePartInfo *info);

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
		g_slice_free (SaveMimePartInfo, info);
	}
}

static gboolean
idle_save_mime_part_show_result (SaveMimePartInfo *info)
{
	if (info->pairs != NULL) {
		save_mime_part_to_file (info);
	} else {
		/* This is a GDK lock because we are an idle callback and
	 	 * hildon_banner_show_information is or does Gtk+ code */

		gdk_threads_enter (); /* CHECKED */
		save_mime_part_info_free (info, TRUE);
		if (info->result == GNOME_VFS_OK) {
			hildon_banner_show_information (NULL, NULL, _CS("sfil_ib_saved"));
		} else if (info->result == GNOME_VFS_ERROR_NO_SPACE) {
			hildon_banner_show_information (NULL, NULL, 
							_KR("cerm_device_memory_full"));
		} else {
			hildon_banner_show_information (NULL, NULL, 
							_("mail_ib_file_operation_failed"));
		}
		gdk_threads_leave (); /* CHECKED */
	}

	return FALSE;
}

static gpointer
save_mime_part_to_file (SaveMimePartInfo *info)
{
	GnomeVFSHandle *handle;
	TnyStream *stream;
	SaveMimePartPair *pair = (SaveMimePartPair *) info->pairs->data;

	info->result = gnome_vfs_create (&handle, pair->filename, GNOME_VFS_OPEN_WRITE, FALSE, 0644);
	if (info->result == GNOME_VFS_OK) {
		GError *error = NULL;
		stream = tny_vfs_stream_new (handle);
		if (tny_mime_part_decode_to_stream (pair->part, stream, &error) < 0) {
			g_warning ("modest: could not save attachment %s: %d (%s)\n", pair->filename, error?error->code:-1, error?error->message:"Unknown error");
			
			info->result = GNOME_VFS_ERROR_IO;
		}
		g_object_unref (G_OBJECT (stream));
		g_object_unref (pair->part);
		g_slice_free (SaveMimePartPair, pair);
		info->pairs = g_list_delete_link (info->pairs, info->pairs);
	} else {
		g_warning ("modest: could not create save attachment %s: %s\n", pair->filename, gnome_vfs_result_to_string (info->result));
		save_mime_part_info_free (info, FALSE);
	}

	g_idle_add ((GSourceFunc) idle_save_mime_part_show_result, info);
	return NULL;
}

static void
save_mime_parts_to_file_with_checks (GtkWindow *parent, SaveMimePartInfo *info)
{
	gboolean is_ok = TRUE;
        gint replaced_files = 0;
        const GList *files = info->pairs;
        const GList *iter;

        for (iter = files; (iter != NULL) && (replaced_files < 2); iter = g_list_next(iter)) {
                SaveMimePartPair *pair = iter->data;
                if (modest_utils_file_exists (pair->filename)) {
			replaced_files++;
                }
        }
	if (replaced_files) {
		gint response;
                const gchar *message = (replaced_files == 1) ?
                        _FM("docm_nc_replace_file") : _FM("docm_nc_replace_multiple");
                response = modest_platform_run_confirmation_dialog (parent, message);
		if (response != GTK_RESPONSE_OK)
			is_ok = FALSE;
	}

	if (!is_ok) {
		save_mime_part_info_free (info, TRUE);
	} else {
		GtkWidget *banner = hildon_banner_show_animation (NULL, NULL, 
								  _CS("sfil_ib_saving"));
		info->banner = banner;
		g_thread_create ((GThreadFunc)save_mime_part_to_file, info, FALSE, NULL);
	}

}

static void
save_attachments_response (GtkDialog *dialog,
			   gint       arg1,
			   gpointer   user_data)  
{
	TnyList *mime_parts;
	gchar *chooser_uri;
	GList *files_to_save = NULL;

	mime_parts = TNY_LIST (user_data);

	if (arg1 != GTK_RESPONSE_OK)
		goto end;

	chooser_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

	if (!modest_utils_folder_writable (chooser_uri)) {
		hildon_banner_show_information 
			(NULL, NULL, _FM("sfil_ib_readonly_location"));
	} else {
		TnyIterator *iter;

		iter = tny_list_create_iterator (mime_parts);
		while (!tny_iterator_is_done (iter)) {
			TnyMimePart *mime_part = (TnyMimePart *) tny_iterator_get_current (iter);

			if ((modest_tny_mime_part_is_attachment_for_modest (mime_part)) &&
			    !tny_mime_part_is_purged (mime_part) &&
			    (tny_mime_part_get_filename (mime_part) != NULL)) {
				SaveMimePartPair *pair;

				pair = g_slice_new0 (SaveMimePartPair);

				if (tny_list_get_length (mime_parts) > 1) {
					gchar *escaped = 
						gnome_vfs_escape_slashes (tny_mime_part_get_filename (mime_part));
					pair->filename = g_build_filename (chooser_uri, escaped, NULL);
					g_free (escaped);
				} else {
					pair->filename = g_strdup (chooser_uri);
				}
				pair->part = mime_part;
				files_to_save = g_list_prepend (files_to_save, pair);
			}
			tny_iterator_next (iter);
		}
		g_object_unref (iter);
	}
	g_free (chooser_uri);

	if (files_to_save != NULL) {
		SaveMimePartInfo *info = g_slice_new0 (SaveMimePartInfo);
		info->pairs = files_to_save;
		info->result = TRUE;
		save_mime_parts_to_file_with_checks ((GtkWindow*) dialog, info);
	}

 end:
	/* Free and close the dialog */
	g_object_unref (mime_parts);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
modest_msg_view_window_save_attachments (ModestMsgViewWindow *window, TnyList *mime_parts)
{
	ModestMsgViewWindowPrivate *priv;
	GtkWidget *save_dialog = NULL;
	gchar *folder = NULL;
	gchar *filename = NULL;
	gchar *save_multiple_str = NULL;
	TnyMsg *window_msg;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (mime_parts == NULL) {
		mime_parts = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
		if (mime_parts == NULL || tny_list_get_length (mime_parts) == 0)
			return;
	} else {
		g_object_ref (mime_parts);
	}

	window_msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
	/* prepare dialog */
	if (tny_list_get_length (mime_parts) == 1) {
		TnyIterator *iter;
		/* only one attachment selected */
		iter = tny_list_create_iterator (mime_parts);
		TnyMimePart *mime_part = (TnyMimePart *) tny_iterator_get_current (iter);
		g_object_unref (iter);
		if (!modest_tny_mime_part_is_msg (mime_part) && 
		    modest_tny_mime_part_is_attachment_for_modest (mime_part) &&
		    !tny_mime_part_is_purged (mime_part)) {
			filename = g_strdup (tny_mime_part_get_filename (mime_part));
		} else {
			/* TODO: show any error? */
			g_warning ("Tried to save a non-file attachment");
			g_object_unref (mime_parts);
			return;
		}
		g_object_unref (mime_part);
	} else {
		save_multiple_str = g_strdup_printf (_FM("sfil_va_number_of_objects_attachments"), 
						     tny_list_get_length (mime_parts));
	}
	g_object_unref (window_msg);
	
	save_dialog = hildon_file_chooser_dialog_new (GTK_WINDOW (window), 
						      GTK_FILE_CHOOSER_ACTION_SAVE);

	/* set folder */
	folder = g_build_filename (g_get_home_dir (), DEFAULT_FOLDER, NULL);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (save_dialog), folder);
	g_free (folder);

	/* set filename */
	if (filename) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save_dialog), 
						   filename);
		g_free (filename);
	}

	/* if multiple, set multiple string */
	if (save_multiple_str) {
		g_object_set (G_OBJECT (save_dialog), "save-multiple", save_multiple_str, NULL);
		gtk_window_set_title (GTK_WINDOW (save_dialog), _FM("sfil_ti_save_objects_files"));
	}

	/* We must run this asynchronously, because the hildon dialog
	   performs a gtk_dialog_run by itself which leads to gdk
	   deadlocks */
	g_signal_connect (save_dialog, "response", 
			  G_CALLBACK (save_attachments_response), mime_parts);

	gtk_widget_show_all (save_dialog);
}

static gboolean
show_remove_attachment_information (gpointer userdata)
{
	ModestMsgViewWindow *window = (ModestMsgViewWindow *) userdata;
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* We're outside the main lock */
	gdk_threads_enter ();

	if (priv->remove_attachment_banner != NULL) {
		gtk_widget_destroy (priv->remove_attachment_banner);
		g_object_unref (priv->remove_attachment_banner);
	}

	priv->remove_attachment_banner = g_object_ref (
		hildon_banner_show_animation (NULL, NULL, _("mcen_ib_removing_attachment")));

	gdk_threads_leave ();

	return FALSE;
}

void
modest_msg_view_window_remove_attachments (ModestMsgViewWindow *window, gboolean get_all)
{
	ModestMsgViewWindowPrivate *priv;
	TnyList *mime_parts = NULL;
	gchar *confirmation_message;
	gint response;
	gint n_attachments;
	TnyMsg *msg;
	TnyIterator *iter;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (get_all)
		mime_parts = modest_msg_view_get_attachments (MODEST_MSG_VIEW (priv->msg_view));
	else
		mime_parts = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));
		
	/* Remove already purged messages from mime parts list */
	iter = tny_list_create_iterator (mime_parts);
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
		tny_iterator_next (iter);
		if (tny_mime_part_is_purged (part)) {
			tny_list_remove (mime_parts, (GObject *) part);
		}
		g_object_unref (part);
	}
	g_object_unref (iter);

	if (tny_list_get_length (mime_parts) == 0) {
		g_object_unref (mime_parts);
		return;
	}

	n_attachments = tny_list_get_length (mime_parts);
	if (n_attachments == 1) {
		gchar *filename;
		TnyMimePart *part;

		iter = tny_list_create_iterator (mime_parts);
		part = (TnyMimePart *) tny_iterator_get_current (iter);
		g_object_unref (iter);
		if (modest_tny_mime_part_is_msg (part)) {
			TnyHeader *header;
			header = tny_msg_get_header (TNY_MSG (part));
			filename = tny_header_dup_subject (header);
			g_object_unref (header);
			if (filename == NULL)
				filename = g_strdup (_("mail_va_no_subject"));
		} else {
			filename = g_strdup (tny_mime_part_get_filename (TNY_MIME_PART (part)));
		}
		confirmation_message = g_strdup_printf (_("mcen_nc_purge_file_text"), filename);
		g_free (filename);
		g_object_unref (part);
	} else {
		confirmation_message = g_strdup_printf (ngettext("mcen_nc_purge_file_text", 
								 "mcen_nc_purge_files_text", 
								 n_attachments), n_attachments);
	}
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (window),
							    confirmation_message);
	g_free (confirmation_message);

	if (response != GTK_RESPONSE_OK) {
		g_object_unref (mime_parts);
		return;
	}

	priv->purge_timeout = g_timeout_add (2000, show_remove_attachment_information, window);
	
	iter = tny_list_create_iterator (mime_parts);
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part;

		part = (TnyMimePart *) tny_iterator_get_current (iter);
		tny_mime_part_set_purged (TNY_MIME_PART (part));
		g_object_unref (part);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);

	msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
	tny_msg_view_clear (TNY_MSG_VIEW (priv->msg_view));
	tny_msg_rewrite_cache (msg);
	tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
	g_object_unref (msg);

	g_object_unref (mime_parts);

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
	gchar *subject = NULL;
	
	msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));

	if (priv->other_body) {
		gchar *description;

		description = modest_tny_mime_part_get_header_value (priv->other_body, "Content-Description");
		if (description) {
			g_strstrip (description);
			subject = description;
		}
	} else if (msg != NULL) {
		header = tny_msg_get_header (msg);
		subject = tny_header_dup_subject (header);
		g_object_unref (header);
		g_object_unref (msg);
	}

	if ((subject == NULL)||(subject[0] == '\0')) {
		g_free (subject);
		subject = g_strdup (_("mail_va_no_subject"));
	}

	gtk_window_set_title (GTK_WINDOW (window), subject);
}


static void on_move_focus (GtkWidget *widget,
			   GtkDirectionType direction,
			   gpointer userdata)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "move-focus");
}

static TnyStream *
fetch_image_open_stream (TnyStreamCache *self, gint64 *expected_size, gchar *uri)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle = NULL;
	GnomeVFSFileInfo *info = NULL;
	TnyStream *stream;

	result = gnome_vfs_open (&handle, uri, GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
		*expected_size = 0;
		return NULL;
	}
	
	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info_from_handle (handle, info, GNOME_VFS_FILE_INFO_DEFAULT);
	if (result != GNOME_VFS_OK || ! (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SIZE)) {
		/* We put a "safe" default size for going to cache */
		*expected_size = (300*1024);
	} else {
		*expected_size = info->size;
	}
	gnome_vfs_file_info_unref (info);

	stream = tny_vfs_stream_new (handle);

	return stream;

}

typedef struct {
	gchar *uri;
	gchar *cache_id;
	TnyStream *output_stream;
	GtkWidget *msg_view;
} FetchImageData;

gboolean
on_fetch_image_idle_refresh_view (gpointer userdata)
{

	FetchImageData *fidata = (FetchImageData *) userdata;
	g_debug ("REFRESH VIEW");
	if (GTK_WIDGET_DRAWABLE (fidata->msg_view)) {
		g_debug ("QUEUING DRAW");
		gtk_widget_queue_draw (fidata->msg_view);
	}
	g_object_unref (fidata->msg_view);
	g_slice_free (FetchImageData, fidata);
	return FALSE;
}

static gpointer
on_fetch_image_thread (gpointer userdata)
{
	FetchImageData *fidata = (FetchImageData *) userdata;
	TnyStreamCache *cache;
	TnyStream *cache_stream;

	cache = modest_runtime_get_images_cache ();
	cache_stream = tny_stream_cache_get_stream (cache, fidata->cache_id, (TnyStreamCacheOpenStreamFetcher) fetch_image_open_stream, (gpointer) fidata->uri);
	g_free (fidata->cache_id);
	g_free (fidata->uri);

	if (cache_stream != NULL) {
		tny_stream_write_to_stream (cache_stream, fidata->output_stream);
		tny_stream_close (cache_stream);
		g_object_unref (cache_stream);
	}

	tny_stream_close (fidata->output_stream);
	g_object_unref (fidata->output_stream);


	gdk_threads_enter ();
	g_idle_add (on_fetch_image_idle_refresh_view, fidata);
	gdk_threads_leave ();

	return NULL;
}

static gboolean
on_fetch_image (ModestMsgView *msgview,
		const gchar *uri,
		TnyStream *stream,
		ModestMsgViewWindow *window)
{
	const gchar *current_account;
	ModestMsgViewWindowPrivate *priv;
	FetchImageData *fidata;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	current_account = modest_window_get_active_account (MODEST_WINDOW (window));

	fidata = g_slice_new0 (FetchImageData);
	fidata->msg_view = g_object_ref (msgview);
	fidata->uri = g_strdup (uri);
	fidata->cache_id = modest_images_cache_get_id (current_account, uri);
	fidata->output_stream = g_object_ref (stream);

	if (g_thread_create (on_fetch_image_thread, fidata, FALSE, NULL) == NULL) {
		g_object_unref (fidata->output_stream);
		g_free (fidata->cache_id);
		g_free (fidata->uri);
		g_object_unref (fidata->msg_view);
		g_slice_free (FetchImageData, fidata);
		tny_stream_close (stream);
		return FALSE;
	}

	return TRUE;;
}

void
modest_msg_view_window_add_to_contacts (ModestMsgViewWindow *self)
{
	modest_ui_actions_on_add_to_contacts (NULL, MODEST_WINDOW (self));
}


void
modest_msg_view_window_fetch_images (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	modest_msg_view_request_fetch_images (MODEST_MSG_VIEW (priv->msg_view));
}

gboolean 
modest_msg_view_window_has_blocked_external_images (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);

	return modest_msg_view_has_blocked_external_images (MODEST_MSG_VIEW (priv->msg_view));
}

void 
modest_msg_view_window_reload (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	TnyHeader *header;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);
	header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (self));	

	if (!message_reader (self, priv, header, priv->row_reference)) {
		g_warning ("Shouldn't happen, trying to reload a message failed");
	}

	g_object_unref (header);
}

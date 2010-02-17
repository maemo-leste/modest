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
#include <tny-error.h>
#include "modest-marshal.h"
#include "modest-platform.h"
#include <modest-utils.h>
#include <modest-toolkit-utils.h>
#include <modest-tny-msg.h>
#include <modest-msg-view-window.h>
#include "modest-msg-view-window-ui-dimming.h"
#include <modest-widget-memory.h>
#include <modest-progress-object.h>
#include <modest-runtime.h>
#include <modest-window-priv.h>
#include <modest-tny-folder.h>
#include <modest-text-utils.h>
#include <modest-account-mgr-helpers.h>
#include <modest-toolkit-factory.h>
#include <modest-scrollable.h>
#include <modest-isearch-toolbar.h>
#include "modest-defs.h"
#include "modest-ui-dimming-manager.h"
#include <gdk/gdkkeysyms.h>
#include <modest-tny-account.h>
#include <modest-mime-part-view.h>
#include <modest-isearch-view.h>
#include <modest-tny-mime-part.h>
#include <modest-address-book.h>
#include <math.h>
#include <errno.h>
#include <glib/gstdio.h>
#include <modest-debug.h>
#include <modest-header-window.h>
#include <modest-account-protocol.h>
#include <modest-icon-names.h>
#include <modest-ui-actions.h>
#include <modest-window-mgr.h>
#include <tny-camel-msg.h>
#include <modest-icon-names.h>

#ifdef MODEST_PLATFORM_MAEMO
#include <modest-maemo-utils.h>
#endif

#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xdmcp.h>
#endif

#include <tny-camel-bs-mime-part.h>
#include <tny-camel-bs-msg.h>

#define MYDOCS_ENV "MYDOCSDIR"
#define DOCS_FOLDER ".documents"

typedef struct _ModestMsgViewWindowPrivate ModestMsgViewWindowPrivate;
struct _ModestMsgViewWindowPrivate {

	GtkWidget   *msg_view;
	GtkWidget   *main_scroll;
	GtkWidget   *isearch_toolbar;
	gchar       *last_search;

	/* Progress observers */
	GSList           *progress_widgets;

	/* Tollbar items */
	GtkWidget   *prev_toolitem;
	GtkWidget   *next_toolitem;
	gboolean    progress_hint;
	gint        fetching_images;

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
	gulong fetch_image_redraw_handler;

	guint purge_timeout;
	GtkWidget *remove_attachment_banner;

	gchar *msg_uid;
	TnyMimePart *other_body;
	TnyMsg * top_msg;

	GSList *sighandlers;
};

static void  modest_msg_view_window_class_init   (ModestMsgViewWindowClass *klass);
static void  modest_msg_view_window_init         (ModestMsgViewWindow *obj);
static void  modest_header_view_observer_init    (ModestHeaderViewObserverIface *iface_class);
static void  modest_msg_view_window_finalize     (GObject *obj);
static void  modest_msg_view_window_show_isearch_toolbar   (GtkWidget *obj, gpointer data);
static void  modest_msg_view_window_isearch_toolbar_close  (GtkWidget *widget,
							    ModestMsgViewWindow *obj);
static void  modest_msg_view_window_isearch_toolbar_search (GtkWidget *widget,
							    ModestMsgViewWindow *obj);
static void  modest_msg_view_window_toggle_isearch_toolbar (GtkWidget *obj,
							    gpointer data);
static void modest_msg_view_window_disconnect_signals (ModestWindow *self);

static gdouble modest_msg_view_window_get_zoom    (ModestWindow *window);
static void modest_msg_view_window_set_zoom       (ModestWindow *window,
						   gdouble zoom);
static gboolean modest_msg_view_window_zoom_minus (ModestWindow *window);
static gboolean modest_msg_view_window_zoom_plus  (ModestWindow *window);
static gboolean modest_msg_view_window_key_event  (GtkWidget *window,
						   GdkEventKey *event,
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

static void set_progress_hint    (ModestMsgViewWindow *self, 
				  gboolean enabled);

static void update_window_title (ModestMsgViewWindow *window);

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
static gboolean message_reader (ModestMsgViewWindow *window,
				ModestMsgViewWindowPrivate *priv,
				TnyHeader *header,
				const gchar *msg_uid,
				TnyFolder *folder,
				GtkTreeRowReference *row_reference);

static void setup_menu (ModestMsgViewWindow *self);
static gboolean _modest_msg_view_window_map_event (GtkWidget *widget,
						   GdkEvent *event,
						   gpointer userdata);
static void update_branding (ModestMsgViewWindow *self);
static void sync_flags      (ModestMsgViewWindow *self);
static gboolean on_handle_calendar (ModestMsgView *msgview, TnyMimePart *calendar_part, 
				    GtkContainer *container, ModestMsgViewWindow *self);

static gboolean on_realize (GtkWidget *widget,
			    gpointer userdata);

/* list my signals */
enum {
	MSG_CHANGED_SIGNAL,
	SCROLL_CHILD_SIGNAL,
	LAST_SIGNAL
};

static const GtkActionEntry msg_view_toolbar_action_entries [] = {

	/* Toolbar items */
	{ "ToolbarMessageReply",      MODEST_STOCK_REPLY,     N_("mcen_me_inbox_reply"),      "<CTRL>R", NULL,  G_CALLBACK (modest_ui_actions_on_reply) },
	{ "ToolbarMessageReplyAll",   MODEST_STOCK_REPLY_ALL,     N_("mcen_me_inbox_replytoall"),         NULL, NULL,  G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "ToolbarMessageForward",    MODEST_STOCK_FORWARD,     N_("mcen_me_inbox_forward"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_forward) },
	{ "ToolbarDeleteMessage",     MODEST_STOCK_DELETE,     N_("qgn_toolb_gene_deletebutton"),             NULL, NULL,  G_CALLBACK (modest_ui_actions_on_delete_message_or_folder) },
	{ "ToolbarMessageBack",       MODEST_TOOLBAR_ICON_PREV,    N_("qgn_toolb_gene_back"),         NULL, NULL, G_CALLBACK (modest_ui_actions_on_prev) },
	{ "ToolbarMessageNext",    MODEST_TOOLBAR_ICON_NEXT, N_("qgn_toolb_gene_forward"),      NULL, NULL, G_CALLBACK (modest_ui_actions_on_next) },
	{ "ToolbarDownloadExternalImages", MODEST_TOOLBAR_ICON_DOWNLOAD_IMAGES, N_("mail_bd_external_images"),      NULL, NULL,  G_CALLBACK (modest_ui_actions_on_fetch_images) },
};

static const GtkToggleActionEntry msg_view_toggle_action_entries [] = {
	{ "FindInMessage",    MODEST_TOOLBAR_ICON_FIND,    N_("qgn_toolb_gene_find"), "<CTRL>F", NULL, G_CALLBACK (modest_msg_view_window_toggle_isearch_toolbar), FALSE },
};

#define MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_MSG_VIEW_WINDOW, \
                                                    ModestMsgViewWindowPrivate))
/* globals */
static ModestWindowParentClass *parent_class = NULL;

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
#ifndef MODEST_TOOLKIT_HILDON2
		my_type = g_type_register_static (MODEST_TYPE_SHELL_WINDOW,
		                                  "ModestMsgViewWindow",
		                                  &my_info, 0);
#else
		my_type = g_type_register_static (MODEST_TYPE_HILDON2_WINDOW,
		                                  "ModestMsgViewWindow",
		                                  &my_info, 0);
#endif

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

static gboolean
modest_msg_view_window_scroll_child (ModestMsgViewWindow *self,
				     GtkScrollType scroll_type,
				     gboolean horizontal,
				     gpointer userdata)
{
	ModestMsgViewWindowPrivate *priv;
	gint step = 0;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	switch (scroll_type) {
	case GTK_SCROLL_STEP_UP:
		step = -1;
		break;
	case GTK_SCROLL_STEP_DOWN:
		step = +1;
		break;
	case GTK_SCROLL_PAGE_UP:
		step = -6;
		break;
	case GTK_SCROLL_PAGE_DOWN:
		step = +6;
		break;
	case GTK_SCROLL_START:
		step = -100;
		break;
	case GTK_SCROLL_END:
		step = +100;
		break;
	default:
		step = 0;
	}

	if (step)
		modest_scrollable_scroll ((ModestScrollable *) priv->main_scroll, 0, step);

	return (gboolean) step;
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
	modest_window_class->zoom_plus_func = modest_msg_view_window_zoom_plus;
	modest_window_class->zoom_minus_func = modest_msg_view_window_zoom_minus;
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

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);
	parent_priv->ui_manager = gtk_ui_manager_new();

	action_group = gtk_action_group_new ("ModestMsgViewWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      msg_view_toolbar_action_entries,
				      G_N_ELEMENTS (msg_view_toolbar_action_entries),
				      obj);
	gtk_action_group_add_toggle_actions (action_group,
					     msg_view_toggle_action_entries,
					     G_N_ELEMENTS (msg_view_toggle_action_entries),
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
	priv->fetch_image_redraw_handler = 0;
	priv->progress_hint = FALSE;
	priv->fetching_images = 0;

	priv->optimized_view  = FALSE;
	priv->purge_timeout = 0;
	priv->remove_attachment_banner = NULL;
	priv->msg_uid = NULL;
	priv->other_body = NULL;

	priv->sighandlers = NULL;

	/* Init window */
	init_window (MODEST_MSG_VIEW_WINDOW(obj));

#ifdef MODEST_TOOLKIT_HILDON2
	/* Grab the zoom keys, it will be used for Zoom and not for
	   changing volume */
       g_signal_connect (G_OBJECT (obj), "realize",
                         G_CALLBACK (on_realize),
                         NULL);
#endif
}

static void
update_progress_hint (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	if (GTK_WIDGET_VISIBLE (self)) {
		modest_window_show_progress (MODEST_WINDOW (self),
					     (priv->progress_hint || (priv->fetching_images > 0))?1:0);
	}
}

static void 
set_progress_hint (ModestMsgViewWindow *self, 
		   gboolean enabled)
{
	ModestWindowPrivate *parent_priv;
	ModestMsgViewWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);
			
	/* Sets current progress hint */
	priv->progress_hint = enabled;

	update_progress_hint (self);

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

	priv->main_scroll = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());
	modest_scrollable_set_horizontal_policy (MODEST_SCROLLABLE (priv->main_scroll), GTK_POLICY_AUTOMATIC);
        g_object_set (G_OBJECT (priv->main_scroll),
		      "movement-mode", MODEST_MOVEMENT_MODE_BOTH,
		      "horizontal-max-overshoot", 0,
		      NULL);
	gtk_container_add (GTK_CONTAINER (priv->main_scroll), priv->msg_view);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_scroll, TRUE, TRUE, 0);
	gtk_container_add   (GTK_CONTAINER(obj), main_vbox);

	/* NULL-ize fields if the window is destroyed */
	g_signal_connect (priv->msg_view, "destroy", G_CALLBACK (gtk_widget_destroyed), &(priv->msg_view));

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
}

static void
modest_msg_view_window_disconnect_signals (ModestWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	GtkWidget *header_view = NULL;
	GtkWindow *parent_window = NULL;
	
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

	parent_window = gtk_window_get_transient_for (GTK_WINDOW (self));
	if (parent_window && MODEST_IS_HEADER_WINDOW (parent_window)) {
		header_view = GTK_WIDGET (modest_header_window_get_header_view (MODEST_HEADER_WINDOW (parent_window)));
		if (header_view) {
			modest_header_view_remove_observer(MODEST_HEADER_VIEW (header_view),
							   MODEST_HEADER_VIEW_OBSERVER(self));
		}
	}
}

static void
modest_msg_view_window_finalize (GObject *obj)
{
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_msg_view_window_disconnect_signals (MODEST_WINDOW (obj));

	if (priv->fetch_image_redraw_handler > 0) {
		g_source_remove (priv->fetch_image_redraw_handler);
		priv->fetch_image_redraw_handler = 0;
	}

	if (priv->other_body != NULL) {
		g_object_unref (priv->other_body);
		priv->other_body = NULL;
	}

	if (priv->top_msg != NULL) {
		g_object_unref (priv->top_msg);
		priv->top_msg = NULL;
	}

	if (priv->header_model != NULL) {
		g_object_unref (priv->header_model);
		priv->header_model = NULL;
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
				  const gchar *mailbox,
				  const gchar *msg_uid)
{
	GObject *obj = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv = NULL;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	ModestDimmingRulesGroup *clipboard_rules_group = NULL;

	obj = G_OBJECT (self);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_uid = g_strdup (msg_uid);

	/* Menubar */
	parent_priv->menubar = NULL;

	toolbar_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_TOOLBAR, TRUE);
	clipboard_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_CLIPBOARD, FALSE);

	setup_menu (self);
	/* Add common dimming rules */
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_msg_view_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_toolbar_dimming_entries),
					      MODEST_WINDOW (self));
	modest_dimming_rules_group_add_rules (clipboard_rules_group, 
					      modest_msg_view_clipboard_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_clipboard_dimming_entries),
					      MODEST_WINDOW (self));

	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, clipboard_rules_group);
	g_object_unref (toolbar_rules_group);
	g_object_unref (clipboard_rules_group);

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
	g_signal_connect (G_OBJECT(priv->msg_view), "show_details",
			  G_CALLBACK (modest_ui_actions_on_details), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "link_contextual",
			  G_CALLBACK (modest_ui_actions_on_msg_link_contextual), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "limit_error",
			  G_CALLBACK (modest_ui_actions_on_limit_error), obj);
	g_signal_connect (G_OBJECT(priv->msg_view), "handle_calendar",
			  G_CALLBACK (on_handle_calendar), obj);
	g_signal_connect (G_OBJECT (priv->msg_view), "fetch_image",
			  G_CALLBACK (on_fetch_image), obj);

	g_signal_connect (G_OBJECT (obj), "key-release-event",
			  G_CALLBACK (modest_msg_view_window_key_event),
			  NULL);

	g_signal_connect (G_OBJECT (obj), "key-press-event",
			  G_CALLBACK (modest_msg_view_window_key_event),
			  NULL);

	g_signal_connect (G_OBJECT (obj), "move-focus",
			  G_CALLBACK (on_move_focus), obj);

	g_signal_connect (G_OBJECT (obj), "map-event",
			  G_CALLBACK (_modest_msg_view_window_map_event),
			  G_OBJECT (obj));

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
	modest_window_set_active_mailbox (MODEST_WINDOW(obj), mailbox);

	/* First add out toolbar ... */
	modest_msg_view_window_show_toolbar (MODEST_WINDOW (obj), TRUE);

	priv->isearch_toolbar = modest_toolkit_factory_create_isearch_toolbar (modest_runtime_get_toolkit_factory (),
									       NULL);
	modest_window_add_toolbar (MODEST_WINDOW (obj), GTK_TOOLBAR (priv->isearch_toolbar));
	gtk_widget_set_no_show_all (priv->isearch_toolbar, TRUE);
	g_signal_connect (G_OBJECT (priv->isearch_toolbar), "isearch-close", 
			  G_CALLBACK (modest_msg_view_window_isearch_toolbar_close), obj);
	g_signal_connect (G_OBJECT (priv->isearch_toolbar), "isearch-search", 
			  G_CALLBACK (modest_msg_view_window_isearch_toolbar_search), obj);
	priv->last_search = NULL;

	/* Init the clipboard actions dim status */
	modest_msg_view_grab_focus(MODEST_MSG_VIEW (priv->msg_view));

	update_window_title (MODEST_MSG_VIEW_WINDOW (obj));


}

/* FIXME: parameter checks */
ModestWindow *
modest_msg_view_window_new_with_header_model (TnyMsg *msg, 
					      const gchar *modest_account_name,
					      const gchar *mailbox,
					      const gchar *msg_uid,
					      GtkTreeModel *model, 
					      GtkTreeRowReference *row_reference)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	TnyFolder *header_folder = NULL;
	ModestHeaderView *header_view = NULL;
	ModestWindowMgr *mgr = NULL;

	MODEST_DEBUG_BLOCK (
	       modest_tny_mime_part_to_string (TNY_MIME_PART (msg), 0);
	);

	mgr = modest_runtime_get_window_mgr ();
	window = MODEST_MSG_VIEW_WINDOW (modest_window_mgr_get_msg_view_window (mgr));
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);

	modest_msg_view_window_construct (window, modest_account_name, mailbox, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	priv->top_msg = NULL;

	/* Remember the message list's TreeModel so we can detect changes
	 * and change the list selection when necessary: */
	header_folder = modest_header_view_get_folder (header_view);
	if (header_folder) {
		priv->is_outbox = (modest_tny_folder_guess_folder_type (header_folder) ==
				   TNY_FOLDER_TYPE_OUTBOX);
		priv->header_folder_id = tny_folder_get_id (header_folder);
		g_object_unref(header_folder);
	}

	/* Setup row references and connect signals */
	priv->header_model = g_object_ref (model);

	if (row_reference && gtk_tree_row_reference_valid (row_reference)) {
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
	update_branding (MODEST_MSG_VIEW_WINDOW (window));

	/* gtk_widget_show_all (GTK_WIDGET (window)); */
	modest_msg_view_window_update_priority (window);
	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);

	return MODEST_WINDOW(window);
}

ModestWindow *
modest_msg_view_window_new_from_uid (const gchar *modest_account_name,
				     const gchar *mailbox,
				     const gchar *msg_uid)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowMgr *mgr = NULL;
	gboolean is_merge;
	TnyAccount *account = NULL;

	mgr = modest_runtime_get_window_mgr ();
	window = MODEST_MSG_VIEW_WINDOW (modest_window_mgr_get_msg_view_window (mgr));
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);

	modest_msg_view_window_construct (window, modest_account_name, mailbox, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	priv->top_msg = NULL;

	is_merge = g_str_has_prefix (msg_uid, "merge:");

	/* Get the account */
	if (!is_merge)
		account = tny_account_store_find_account (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
							  msg_uid);

	if (is_merge || account) {
		TnyFolder *folder = NULL;

		/* Try to get the message, if it's already downloaded
		   we don't need to connect */
		if (account) {
			folder = modest_tny_folder_store_find_folder_from_uri (TNY_FOLDER_STORE (account), msg_uid);
		} else {
			ModestTnyAccountStore *account_store;
			ModestTnyLocalFoldersAccount *local_folders_account;

			account_store = modest_runtime_get_account_store ();
			local_folders_account = MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (
				modest_tny_account_store_get_local_folders_account (account_store));
			folder = modest_tny_local_folders_account_get_merged_outbox (local_folders_account);
			g_object_unref (local_folders_account);
		}
		if (folder) {
			TnyDevice *device;
			gboolean device_online;

			device = modest_runtime_get_device();
			device_online = tny_device_is_online (device);
			if (device_online) {
				message_reader (window, priv, NULL, msg_uid, folder, NULL);
			} else {
				TnyMsg *msg = tny_folder_find_msg (folder, msg_uid, NULL);
				if (msg) {
					tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
					update_window_title (MODEST_MSG_VIEW_WINDOW (window));
					update_branding (MODEST_MSG_VIEW_WINDOW (window));
					g_object_unref (msg);
					/* Sync flags to server */
					sync_flags (MODEST_MSG_VIEW_WINDOW (window));
				} else {
					message_reader (window, priv, NULL, msg_uid, folder, NULL);
				}
			}
			g_object_unref (folder);
		}

 	}

	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);

	return MODEST_WINDOW(window);
}

ModestWindow *
modest_msg_view_window_new_from_header_view (ModestHeaderView *header_view,
					     const gchar *modest_account_name,
					     const gchar *mailbox,
					     const gchar *msg_uid,
					     GtkTreeRowReference *row_reference)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	TnyFolder *header_folder = NULL;
	ModestWindowMgr *mgr = NULL;
	GtkTreePath *path;
	GtkTreeIter iter;

	mgr = modest_runtime_get_window_mgr ();
	window = MODEST_MSG_VIEW_WINDOW (modest_window_mgr_get_msg_view_window (mgr));
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);

	modest_msg_view_window_construct (window, modest_account_name, mailbox, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	priv->top_msg = NULL;

	/* Remember the message list's TreeModel so we can detect changes 
	 * and change the list selection when necessary: */

	if (header_view != NULL){
		header_folder = modest_header_view_get_folder(header_view);
		/* This could happen if the header folder was
		   unseleted before opening this msg window (for
		   example if the user selects an account in the
		   folder view of the main window */
		if (header_folder) {
			priv->is_outbox = (modest_tny_folder_guess_folder_type (header_folder) == 
					   TNY_FOLDER_TYPE_OUTBOX);
			priv->header_folder_id = tny_folder_get_id(header_folder);
			g_object_unref(header_folder);
		}
	}

	/* Setup row references and connect signals */
	priv->header_model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
	g_object_ref (priv->header_model);

	if (row_reference && gtk_tree_row_reference_valid (row_reference)) {
		priv->row_reference = gtk_tree_row_reference_copy (row_reference);
		priv->next_row_reference = gtk_tree_row_reference_copy (row_reference);
		select_next_valid_row (priv->header_model, &(priv->next_row_reference), TRUE, priv->is_outbox);
	} else {
		priv->row_reference = NULL;
		priv->next_row_reference = NULL;
	}

	/* Connect signals */
	priv->row_changed_handler = 
		g_signal_connect (GTK_TREE_MODEL(priv->header_model), "row-changed",
				  G_CALLBACK(modest_msg_view_window_on_row_changed),
				  window);
	priv->row_deleted_handler = 
		g_signal_connect (GTK_TREE_MODEL(priv->header_model), "row-deleted",
				  G_CALLBACK(modest_msg_view_window_on_row_deleted),
				  window);
	priv->row_inserted_handler = 
		g_signal_connect (GTK_TREE_MODEL(priv->header_model), "row-inserted",
				  G_CALLBACK(modest_msg_view_window_on_row_inserted),
				  window);
	priv->rows_reordered_handler = 
		g_signal_connect(GTK_TREE_MODEL(priv->header_model), "rows-reordered",
				 G_CALLBACK(modest_msg_view_window_on_row_reordered),
				 window);

	if (header_view != NULL){
		modest_header_view_add_observer(header_view,
						MODEST_HEADER_VIEW_OBSERVER(window));
	}

	tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), NULL);
	update_branding (MODEST_MSG_VIEW_WINDOW (window));

	if (priv->row_reference) {
		path = gtk_tree_row_reference_get_path (priv->row_reference);
		if (gtk_tree_model_get_iter (priv->header_model, &iter, path)) {
			TnyHeader *header;
			gtk_tree_model_get (priv->header_model, &iter, 
					    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header, -1);
			message_reader (window, priv, header, NULL, NULL, priv->row_reference);
			g_object_unref (header);
		}
		gtk_tree_path_free (path);
	}
	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (window), MODEST_DIMMING_RULES_CLIPBOARD);

	return MODEST_WINDOW(window);
}

ModestWindow *
modest_msg_view_window_new_for_search_result (TnyMsg *msg,
					      const gchar *modest_account_name,
					      const gchar *mailbox,
					      const gchar *msg_uid)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowMgr *mgr = NULL;

	mgr = modest_runtime_get_window_mgr ();
	window = MODEST_MSG_VIEW_WINDOW (modest_window_mgr_get_msg_view_window (mgr));
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);
	modest_msg_view_window_construct (window, modest_account_name, mailbox, msg_uid);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	priv->top_msg = NULL;

	/* Remember that this is a search result, 
	 * so we can disable some UI appropriately: */
	priv->is_search_result = TRUE;

	tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
	update_branding (MODEST_MSG_VIEW_WINDOW (window));
	
	update_window_title (window);
	/* gtk_widget_show_all (GTK_WIDGET (window));*/
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
					    TnyMsg *top_msg,
					    const gchar *modest_account_name,
					    const gchar *mailbox,
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
					  modest_account_name, mailbox, msg_uid);

	if (other_body) {
		priv->other_body = g_object_ref (other_body);
		modest_msg_view_set_msg_with_other_body (MODEST_MSG_VIEW (priv->msg_view), msg, other_body);
	} else {
		tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
	}
	if (top_msg) {
		priv->top_msg = g_object_ref (top_msg);
	} else {
		priv->top_msg = NULL;
	}
	update_window_title (MODEST_MSG_VIEW_WINDOW (obj));
	update_branding (MODEST_MSG_VIEW_WINDOW (obj));

	/* gtk_widget_show_all (GTK_WIDGET (obj)); */

	/* Check dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (obj));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (obj));
	modest_window_check_dimming_rules_group (MODEST_WINDOW (obj), MODEST_DIMMING_RULES_CLIPBOARD);

	return MODEST_WINDOW(obj);
}

ModestWindow *
modest_msg_view_window_new_for_attachment (TnyMsg *msg,
					   TnyMsg *top_msg,
					   const gchar *modest_account_name,
					   const gchar *mailbox,
					   const gchar *msg_uid)
{
	return modest_msg_view_window_new_with_other_body (msg, NULL, top_msg, modest_account_name, mailbox, msg_uid);
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
		g_warning("%s: No reference for msg header item.", __FUNCTION__);
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
	if (!priv->row_reference ||
	    !gtk_tree_row_reference_valid (priv->row_reference))
		return;

	if (priv->next_row_reference &&
	    gtk_tree_row_reference_valid (priv->next_row_reference)) {
		GtkTreePath *cur, *next;
		/* Check that the order is still the correct one */
		cur = gtk_tree_row_reference_get_path (priv->row_reference);
		next = gtk_tree_row_reference_get_path (priv->next_row_reference);
		gtk_tree_path_next (cur);
		if (gtk_tree_path_compare (cur, next) != 0) {
			gtk_tree_row_reference_free (priv->next_row_reference);
			priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
			select_next_valid_row (header_model, &(priv->next_row_reference), FALSE, priv->is_outbox);
			already_changed = TRUE;
		}
		gtk_tree_path_free (cur);
		gtk_tree_path_free (next);
	} else {
		if (priv->next_row_reference)
			gtk_tree_row_reference_free (priv->next_row_reference);
		/* Update next row reference */
		priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
		select_next_valid_row (header_model, &(priv->next_row_reference), FALSE, priv->is_outbox);
		already_changed = TRUE;
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

	return priv->progress_hint;
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

TnyMsg*
modest_msg_view_window_get_top_message (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	
	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	if (priv->top_msg)
		return g_object_ref (priv->top_msg);
	else
		return NULL;
}

const gchar*
modest_msg_view_window_get_message_uid (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;

	g_return_val_if_fail (self, NULL);
	
	priv  = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	return (const gchar*) priv->msg_uid;
}

/* Used for the Ctrl+F accelerator */
static void
modest_msg_view_window_toggle_isearch_toolbar (GtkWidget *obj,
					       gpointer data)
{
	ModestMsgViewWindow *window = MODEST_MSG_VIEW_WINDOW (data);
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (GTK_WIDGET_VISIBLE (priv->isearch_toolbar)) {
		modest_msg_view_window_isearch_toolbar_close (obj, data);
       } else {
		modest_msg_view_window_show_isearch_toolbar (obj, data);
       }
}

/* Handler for menu option */
static void
modest_msg_view_window_show_isearch_toolbar (GtkWidget *obj,
					     gpointer data)
{
	ModestMsgViewWindow *window = MODEST_MSG_VIEW_WINDOW (data);
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	gtk_widget_show (priv->isearch_toolbar);
	modest_isearch_toolbar_highlight_entry (MODEST_ISEARCH_TOOLBAR (priv->isearch_toolbar), TRUE);
}

/* Handler for click on the "X" close button in isearch toolbar */
static void
modest_msg_view_window_isearch_toolbar_close (GtkWidget *widget,
					      ModestMsgViewWindow *obj)
{
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);

	/* Hide toolbar */
	gtk_widget_hide (priv->isearch_toolbar);
	modest_msg_view_grab_focus (MODEST_MSG_VIEW (priv->msg_view));
}

static void
modest_msg_view_window_isearch_toolbar_search (GtkWidget *widget,
					       ModestMsgViewWindow *obj)
{
	const gchar *current_search;
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);

	if (modest_mime_part_view_is_empty (MODEST_MIME_PART_VIEW (priv->msg_view))) {
		modest_platform_system_banner (NULL, NULL, _("mail_ib_nothing_to_find"));
		return;
	}

	current_search = modest_isearch_toolbar_get_search (MODEST_ISEARCH_TOOLBAR (widget));

	if ((current_search == NULL) || (strcmp (current_search, "") == 0)) {
		modest_platform_system_banner (NULL, NULL, _CS_FIND_REP_ENTER_TEXT);
		return;
	}

	if ((priv->last_search == NULL) || (strcmp (priv->last_search, current_search) != 0)) {
		gboolean result;
		g_free (priv->last_search);
		priv->last_search = g_strdup (current_search);
		result = modest_isearch_view_search (MODEST_ISEARCH_VIEW (priv->msg_view),
						     priv->last_search);
		if (!result) {
			modest_platform_system_banner (NULL, NULL, 
							_HL_IB_FIND_NO_MATCHES);
			g_free (priv->last_search);
			priv->last_search = NULL;
		} else {
			modest_isearch_toolbar_highlight_entry (MODEST_ISEARCH_TOOLBAR (priv->isearch_toolbar), TRUE);
		}
	} else {
		if (!modest_isearch_view_search_next (MODEST_ISEARCH_VIEW (priv->msg_view))) {
			modest_platform_system_banner (NULL, NULL, 
							_HL_IB_FIND_COMPLETE);
			g_free (priv->last_search);
			priv->last_search = NULL;
		} else {
			modest_isearch_toolbar_highlight_entry (MODEST_ISEARCH_TOOLBAR (priv->isearch_toolbar), TRUE);
		}
	}
	
}

static void
modest_msg_view_window_set_zoom (ModestWindow *window,
				 gdouble zoom)
{
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
     
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
	modest_zoomable_set_zoom (MODEST_ZOOMABLE (priv->msg_view), zoom);

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
	gdouble zoom_level;
	ModestMsgViewWindowPrivate *priv;
	gint int_zoom;
	gchar *banner_text;
     
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), 1.0);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
  
	zoom_level =  modest_zoomable_get_zoom (MODEST_ZOOMABLE (priv->msg_view));

	if (zoom_level >= 2.0) {
		modest_platform_system_banner (NULL, NULL, 
						_CS_MAX_ZOOM_LEVEL_REACHED);
		return FALSE;
	} else if (zoom_level >= 1.5) {
		zoom_level = 2.0;
	} else if (zoom_level >= 1.2) {
		zoom_level = 1.5;
	} else if (zoom_level >= 1.0) {
		zoom_level = 1.2;
	} else if (zoom_level >= 0.8) {
		zoom_level = 1.0;
	} else if (zoom_level >= 0.5) {
		zoom_level = 0.8;
	} else {
		zoom_level = 0.5;
	}

	/* set zoom level */
	int_zoom = (gint) rint (zoom_level*100.0+0.1);
	banner_text = g_strdup_printf (_HL_IB_ZOOM, int_zoom);
	modest_platform_information_banner (GTK_WIDGET (window), NULL, banner_text);
	g_free (banner_text);
	modest_zoomable_set_zoom (MODEST_ZOOMABLE (priv->msg_view), zoom_level);

	return TRUE;
}

static gboolean
modest_msg_view_window_zoom_minus (ModestWindow *window)
{
	gdouble zoom_level;
	ModestMsgViewWindowPrivate *priv;
	gint int_zoom;
	gchar *banner_text;
     
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), 1.0);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
  
	zoom_level =  modest_zoomable_get_zoom (MODEST_ZOOMABLE (priv->msg_view));

	if (zoom_level <= 0.5) {
		modest_platform_system_banner (NULL, NULL, 
						_CS_MIN_ZOOM_LEVEL_REACHED);
		return FALSE;
	} else if (zoom_level <= 0.8) {
		zoom_level = 0.5;
	} else if (zoom_level <= 1.0) {
		zoom_level = 0.8;
	} else if (zoom_level <= 1.2) {
		zoom_level = 1.0;
	} else if (zoom_level <= 1.5) {
		zoom_level = 1.2;
	} else if (zoom_level <= 2.0) {
		zoom_level = 1.5;
	} else {
		zoom_level = 2.0;
	}

	/* set zoom level */
	int_zoom = (gint) rint (zoom_level*100.0+0.1);
	banner_text = g_strdup_printf (_HL_IB_ZOOM, int_zoom);
	modest_platform_information_banner (GTK_WIDGET (window), NULL, banner_text);
	g_free (banner_text);
	modest_zoomable_set_zoom (MODEST_ZOOMABLE (priv->msg_view), zoom_level);

	return TRUE;
}

static gboolean
modest_msg_view_window_key_event (GtkWidget *window,
				  GdkEventKey *event,
				  gpointer userdata)
{
	GtkWidget *focus;

	focus = gtk_container_get_focus_child ((GtkContainer *) window);

	/* for the isearch toolbar case */
	if (focus && GTK_IS_ENTRY (focus)) {
		if (event->keyval == GDK_BackSpace) {
			GdkEvent *copy;
			copy = gdk_event_copy ((GdkEvent *) event);
			gtk_widget_event (focus, copy);
			gdk_event_free (copy);
			return TRUE;
		} else {
			return FALSE;
		}
	}
	return FALSE;
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
	gchar *msg_uid;
	TnyFolder *folder;
	GtkTreeRowReference *row_reference;
} MsgReaderInfo;

static void
message_reader_performer (gboolean canceled, 
			  GError *err,
			  ModestWindow *parent_window,
			  TnyAccount *account, 
			  gpointer user_data)
{
	ModestMailOperation *mail_op = NULL;
	MsgReaderInfo *info;

	info = (MsgReaderInfo *) user_data;
	if (canceled || err) {
		update_window_title (MODEST_MSG_VIEW_WINDOW (parent_window));
		modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (parent_window));
		goto frees;
	}

	/* Register the header - it'll be unregistered in the callback */
	if (info->header)
		modest_window_mgr_register_header (modest_runtime_get_window_mgr (), info->header, NULL);

	/* New mail operation */
	mail_op = modest_mail_operation_new_with_error_handling (G_OBJECT(parent_window),
								 modest_ui_actions_disk_operations_error_handler, 
								 NULL, NULL);

	modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (), mail_op);
	if (info->header)
		modest_mail_operation_get_msg (mail_op, info->header, TRUE, view_msg_cb, info->row_reference);
	else
		modest_mail_operation_find_msg (mail_op, info->folder, info->msg_uid, TRUE, view_msg_cb, NULL);
	g_object_unref (mail_op);

	/* Update dimming rules */
	modest_ui_actions_check_toolbar_dimming_rules (MODEST_WINDOW (parent_window));
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (parent_window));

 frees:
	/* Frees. The row_reference will be freed by the view_msg_cb callback */
	g_free (info->msg_uid);
	if (info->folder)
		g_object_unref (info->folder);
	if (info->header)
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
		const gchar *msg_uid,
		TnyFolder *folder,
		GtkTreeRowReference *row_reference)
{
	ModestWindowMgr *mgr;
	TnyAccount *account = NULL;
	MsgReaderInfo *info;

	/* We set the header from model while we're loading */
	tny_header_view_set_header (TNY_HEADER_VIEW (priv->msg_view), header);
	modest_window_set_title (MODEST_WINDOW (window), _CS_UPDATING);

	if (header)
		folder = NULL;

	if (folder)
		g_object_ref (folder);

	mgr = modest_runtime_get_window_mgr ();
	/* Msg download completed */
	if (!header || !(tny_header_get_flags (header) & TNY_HEADER_FLAG_CACHED)) {

		/* Ask the user if he wants to download the message if
		   we're not online */
		if (!tny_device_is_online (modest_runtime_get_device())) {
			GtkResponseType response;
			GtkWindow *toplevel;

			toplevel = (GtkWindow *) gtk_widget_get_toplevel ((GtkWidget *) window);
			response = modest_platform_run_confirmation_dialog (toplevel, _("mcen_nc_get_msg"));
			if (response == GTK_RESPONSE_CANCEL) {
				update_window_title (window);
				return FALSE;
			}

			if (header) {
				folder = tny_header_get_folder (header);
			}
			info = g_slice_new (MsgReaderInfo);
			info->msg_uid = g_strdup (msg_uid);
			if (header)
				info->header = g_object_ref (header);
			else
				info->header = NULL;	
			if (folder)
				info->folder = g_object_ref (folder);
			else
				info->folder = NULL;
			if (row_reference) {
				info->row_reference = gtk_tree_row_reference_copy (row_reference);
			} else {
				info->row_reference = NULL;
			}

			/* Offer the connection dialog if necessary */
			modest_platform_connect_if_remote_and_perform ((ModestWindow *) window,
								       TRUE,
								       TNY_FOLDER_STORE (folder),
								       message_reader_performer, 
								       info);
			if (folder)
				g_object_unref (folder);
			return TRUE;
		}
	}

	if (header) {
		folder = tny_header_get_folder (header);
	}
        if (folder)
		account = tny_folder_get_account (folder);

	info = g_slice_new (MsgReaderInfo);
	info->msg_uid = g_strdup (msg_uid);
	if (folder)
		info->folder = g_object_ref (folder);
	else
		info->folder = NULL;
	if (header)
		info->header = g_object_ref (header);
	else
		info->header = NULL;
	if (row_reference)
		info->row_reference = gtk_tree_row_reference_copy (row_reference);
	else
		info->row_reference = NULL;

	message_reader_performer (FALSE, NULL, (ModestWindow *) window, account, info);
        if (account)
		g_object_unref (account);
	if (folder)
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
		if (priv->next_row_reference) {
			gtk_tree_row_reference_free (priv->next_row_reference);
		}
		if (gtk_tree_row_reference_valid (priv->row_reference)) {
			priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
			select_next_valid_row (priv->header_model, &(priv->next_row_reference), FALSE, priv->is_outbox);
		} else {
			priv->next_row_reference = NULL;
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
	if (!message_reader (window, priv, header, NULL, NULL, row_reference)) {
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

	if (priv->row_reference && !gtk_tree_row_reference_valid (priv->row_reference)) {
		gtk_tree_row_reference_free (priv->row_reference);
		priv->row_reference = NULL;
	}

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
				retval = message_reader (window, priv, header, NULL, NULL, row_reference);
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
	self = (ModestMsgViewWindow *) modest_mail_operation_get_source (mail_op);
	if (canceled || !self || MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self)->msg_view == NULL ) {
		if (row_reference)
			gtk_tree_row_reference_free (row_reference);
		if (self) {
			/* Restore window title */
			update_window_title (self);
			modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (self));
			g_object_unref (self);
		}
		return;
	}

	/* If there was any error */
	if (!modest_ui_actions_msg_retrieval_check (mail_op, header, msg)) {
		if (row_reference)
			gtk_tree_row_reference_free (row_reference);
		if (self) {
			priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);
			/* First we check if the parent is a folder window */
			if (priv->msg_uid && !modest_window_mgr_get_folder_window (MODEST_WINDOW_MGR (modest_runtime_get_window_mgr ()))) {
				gboolean is_merge;
				TnyAccount *account = NULL;
				GtkWidget *header_window = NULL;

				is_merge = g_str_has_prefix (priv->msg_uid, "merge:");

				/* Get the account */
				if (!is_merge)
					account = tny_account_store_find_account (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
										  priv->msg_uid);

				if (is_merge || account) {
					TnyFolder *folder = NULL;

					/* Try to get the message, if it's already downloaded
					   we don't need to connect */
					if (account) {
						folder = modest_tny_folder_store_find_folder_from_uri (TNY_FOLDER_STORE (account), 
												       priv->msg_uid);
					} else {
						ModestTnyAccountStore *account_store;
						ModestTnyLocalFoldersAccount *local_folders_account;

						account_store = modest_runtime_get_account_store ();
						local_folders_account = MODEST_TNY_LOCAL_FOLDERS_ACCOUNT (
							modest_tny_account_store_get_local_folders_account (account_store));
						folder = modest_tny_local_folders_account_get_merged_outbox (local_folders_account);
						g_object_unref (local_folders_account);
					}
					if (account) g_object_unref (account);

					if (folder) {
						header_window = (GtkWidget *)
							modest_header_window_new (
								folder, 
								modest_window_get_active_account (MODEST_WINDOW (self)), 
								modest_window_get_active_mailbox (MODEST_WINDOW (self)));
						if (!modest_window_mgr_register_window (modest_runtime_get_window_mgr (),
											MODEST_WINDOW (header_window),
											NULL)) {
							gtk_widget_destroy (GTK_WIDGET (header_window));
						} else {
							gtk_widget_show_all (GTK_WIDGET (header_window));
						}
						g_object_unref (folder);
					}
				}
			}


			/* Restore window title */
			update_window_title (self);
			modest_ui_actions_on_close_window (NULL, MODEST_WINDOW (self));
			g_object_unref (self);
		}
		return;
	}

	/* Get the window */ 
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	/* Update the row reference */
	if (priv->row_reference != NULL) {
		gtk_tree_row_reference_free (priv->row_reference);
		priv->row_reference = (row_reference && gtk_tree_row_reference_valid (row_reference))?gtk_tree_row_reference_copy (row_reference):NULL;
		if (priv->next_row_reference != NULL) {
			gtk_tree_row_reference_free (priv->next_row_reference);
		}
		if (priv->row_reference) {
			priv->next_row_reference = gtk_tree_row_reference_copy (priv->row_reference);
			select_next_valid_row (priv->header_model, &(priv->next_row_reference), TRUE, priv->is_outbox);
		} else {
			priv->next_row_reference = NULL;
		}
	}

	/* Mark header as read */
	if (!(tny_header_get_flags (header) & TNY_HEADER_FLAG_SEEN)) {
		gchar *uid;

		tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);
		uid = modest_tny_folder_get_header_unique_id (header);
		modest_platform_emit_msg_read_changed_signal (uid, TRUE);
		g_free (uid);
	}

	/* Set new message */
	if (priv->msg_view != NULL && TNY_IS_MSG_VIEW (priv->msg_view)) {
		tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
		modest_msg_view_window_update_priority (self);
		update_window_title (MODEST_MSG_VIEW_WINDOW (self));
		update_branding (MODEST_MSG_VIEW_WINDOW (self));
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

	/* Sync the flags if the message is not opened from a header
	   model, i.e, if it's opened from a notification */
	if (!priv->header_model)
		sync_flags (self);

	/* Frees */
	g_object_unref (self);
	if (row_reference)
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

	if (priv->header_model && priv->row_reference && gtk_tree_row_reference_valid (priv->row_reference)) {
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
	static_button_size = modest_window_mgr_get_fullscreen_mode (mgr)?120:120;

	if (parent_priv->toolbar) {
		/* Set expandable and homogeneous tool buttons */
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageReply");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), TRUE);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageReplyAll");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), TRUE);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageForward");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), TRUE);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarDeleteMessage");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), TRUE);
		widget = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarDownloadExternalImages");
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (widget), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->next_toolitem), TRUE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->next_toolitem), TRUE);
		gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (priv->prev_toolitem), TRUE);
		gtk_tool_item_set_expand (GTK_TOOL_ITEM (priv->prev_toolitem), TRUE);
	}
}

static void
modest_msg_view_window_show_toolbar (ModestWindow *self,
				     gboolean show_toolbar)
{
	ModestMsgViewWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv;

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self);

	/* Set optimized view status */
	priv->optimized_view = !show_toolbar;

	if (!parent_priv->toolbar) {
		parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, 
								  "/ToolBar");

#ifdef MODEST_TOOLKIT_HILDON2
		gtk_toolbar_set_icon_size (GTK_TOOLBAR (parent_priv->toolbar), HILDON_ICON_SIZE_FINGER);
#else
		gtk_toolbar_set_icon_size (GTK_TOOLBAR (parent_priv->toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR);
#endif
		gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);

		priv->next_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageNext");
		priv->prev_toolitem = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar/ToolbarMessageBack");
		toolbar_resize (MODEST_MSG_VIEW_WINDOW (self));

		modest_window_add_toolbar (MODEST_WINDOW (self), 
					   GTK_TOOLBAR (parent_priv->toolbar));

	}

	if (show_toolbar) {
		/* Quick hack: this prevents toolbar icons "dance" when progress bar show status is changed */ 
		/* TODO: resize mode migth be GTK_RESIZE_QUEUE, in order to avoid unneccesary shows */
		gtk_container_set_resize_mode (GTK_CONTAINER(parent_priv->toolbar), GTK_RESIZE_IMMEDIATE);

		gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
		if (modest_msg_view_window_transfer_mode_enabled (MODEST_MSG_VIEW_WINDOW (self))) 
			set_progress_hint (MODEST_MSG_VIEW_WINDOW (self), TRUE);
		else
			set_progress_hint (MODEST_MSG_VIEW_WINDOW (self), FALSE);

	} else {
		gtk_widget_set_no_show_all (parent_priv->toolbar, TRUE);
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
	}
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

	return priv->progress_hint;
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
		if (our_acc && parent_acc && strcmp (parent_acc, our_acc) == 0)
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
		if (op_type == MODEST_MAIL_OPERATION_TYPE_RECEIVE ||
		    op_type == MODEST_MAIL_OPERATION_TYPE_OPEN ||
		    op_type == MODEST_MAIL_OPERATION_TYPE_DELETE) {
			set_progress_hint (self, TRUE);
			while (tmp) {
				modest_progress_object_add_operation (
						MODEST_PROGRESS_OBJECT (tmp->data),
						mail_op);
				tmp = g_slist_next (tmp);
			}
		}
	}
	g_object_unref (source);

	/* Update dimming rules */
	check_dimming_rules_after_change (self);
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

	if (op_type == MODEST_MAIL_OPERATION_TYPE_RECEIVE ||
	    op_type == MODEST_MAIL_OPERATION_TYPE_OPEN ||
	    op_type == MODEST_MAIL_OPERATION_TYPE_DELETE) {
		while (tmp) {
			modest_progress_object_remove_operation (MODEST_PROGRESS_OBJECT (tmp->data),
								 mail_op);
			tmp = g_slist_next (tmp);
		}

		/* If no more operations are being observed, NORMAL mode is enabled again */
		if (observers_empty (self)) {
			set_progress_hint (self, FALSE);
		}
	}

	/* Update dimming rules. We have to do this right here
	   and not in view_msg_cb because at that point the
	   transfer mode is still enabled so the dimming rule
	   won't let the user delete the message that has been
	   readed for example */
	check_dimming_rules_after_change (self);
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

	/* In Hildon 2.2 as there's no selection we assume we have all attachments selected */
	selected_attachments = modest_msg_view_get_attachments (MODEST_MSG_VIEW (priv->msg_view));
	
	return selected_attachments;
}

typedef struct {
	ModestMsgViewWindow *self;
	gchar *file_path;
	gchar *attachment_uid;
} DecodeAsyncHelper;

static void
on_decode_to_stream_async_handler (TnyMimePart *mime_part, 
				   gboolean cancelled, 
				   TnyStream *stream, 
				   GError *err, 
				   gpointer user_data)
{
	DecodeAsyncHelper *helper = (DecodeAsyncHelper *) user_data;
	const gchar *content_type;
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (helper->self);

	if (cancelled || err) {
		if (err) {
			gchar *msg;
			if ((err->domain == TNY_ERROR_DOMAIN) && 
			    (err->code == TNY_IO_ERROR_WRITE) &&
			    (errno == ENOSPC)) {
				msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
			} else {
				msg = g_strdup (_("mail_ib_file_operation_failed"));
			}
			modest_platform_information_banner (NULL, NULL, msg);
			g_free (msg);
		}
		goto free;
	}

	/* It could happen that the window was closed. So we
	   assume it is a cancelation */
	if (!GTK_WIDGET_VISIBLE (helper->self))
		goto free;

	/* Remove the progress hint */
	set_progress_hint (helper->self, FALSE);

	content_type = tny_mime_part_get_content_type (mime_part);
	if (content_type && g_str_has_prefix (content_type, "message/rfc822")) {
		ModestWindowMgr *mgr;
		ModestWindow *msg_win = NULL;
		TnyMsg * msg;
		gchar *account;
		const gchar *mailbox;
		TnyStream *file_stream;
		gint fd;

		fd = g_open (helper->file_path, O_RDONLY, 0644);
		if (fd != -1) {
			TnyMsg *top_msg;
			file_stream = tny_fs_stream_new (fd);

			mgr = modest_runtime_get_window_mgr ();

			account = g_strdup (modest_window_get_active_account (MODEST_WINDOW (helper->self)));
			mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (helper->self));

			if (!account)
				account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr ());

			msg = tny_camel_msg_new ();
			tny_camel_msg_parse ((TnyCamelMsg *) msg, file_stream);

			if (priv->top_msg)
				top_msg = g_object_ref (priv->top_msg);
			else
				top_msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));

			msg_win = modest_msg_view_window_new_for_attachment (TNY_MSG (msg), top_msg, 
									     account, mailbox, helper->attachment_uid);
			if (top_msg) g_object_unref (top_msg);
			modest_window_set_zoom (MODEST_WINDOW (msg_win),
						modest_window_get_zoom (MODEST_WINDOW (helper->self)));
			if (modest_window_mgr_register_window (mgr, msg_win, MODEST_WINDOW (helper->self)))
				gtk_widget_show_all (GTK_WIDGET (msg_win));
			else
				gtk_widget_destroy (GTK_WIDGET (msg_win));
			g_object_unref (msg);
			g_object_unref (file_stream);
		} else {
			modest_platform_information_banner (NULL, NULL, _("mail_ib_file_operation_failed"));
		}

	} else {

		/* make the file read-only */
		g_chmod(helper->file_path, 0444);

		/* Activate the file */
		modest_platform_activate_file (helper->file_path, content_type);
	}

 free:
	/* Frees */
	g_object_unref (helper->self);
	g_free (helper->file_path);
	g_free (helper->attachment_uid);
	g_slice_free (DecodeAsyncHelper, helper);
}

static void
view_attachment_connect_handler (gboolean canceled,
				 GError *err,
				 GtkWindow *parent_window,
				 TnyAccount *account,
				 TnyMimePart *part)
{

	if (canceled || err) {
		g_object_unref (part);
		return;
	}

	modest_msg_view_window_view_attachment (MODEST_MSG_VIEW_WINDOW (parent_window),
						part);
	g_object_unref (part);
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
			modest_platform_system_banner (NULL, NULL, _("mcen_ib_unable_to_display_more"));
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
			goto frees;
	} else {
		g_object_ref (mime_part);
	}

	if (tny_mime_part_is_purged (mime_part))
		goto frees;

	if (TNY_IS_CAMEL_BS_MIME_PART (mime_part) &&
	    !tny_camel_bs_mime_part_is_fetched (TNY_CAMEL_BS_MIME_PART (mime_part))) {
		gboolean is_merge;
		TnyAccount *account;

		is_merge = g_str_has_prefix (priv->msg_uid, "merge:");
		account = NULL;
		/* Get the account */
		if (!is_merge)
			account = tny_account_store_find_account (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
								  priv->msg_uid);

		if (!tny_device_is_online (modest_runtime_get_device())) {
			modest_platform_connect_and_perform ((ModestWindow *) window,
							     TRUE,
							     TNY_ACCOUNT (account),
							     (ModestConnectedPerformer) view_attachment_connect_handler,
							     g_object_ref (mime_part));
			goto frees;
		}
	}

	if (!modest_tny_mime_part_is_msg (mime_part) && tny_mime_part_get_filename (mime_part)) {
		gchar *filepath = NULL;
		const gchar *att_filename = tny_mime_part_get_filename (mime_part);
		gboolean show_error_banner = FALSE;
		TnyFsStream *temp_stream = NULL;
		temp_stream = modest_utils_create_temp_stream (att_filename, attachment_uid,
							       &filepath);

		if (temp_stream != NULL) {
			ModestAccountMgr *mgr;
			DecodeAsyncHelper *helper;
			gboolean decode_in_provider;
			ModestProtocol *protocol;
			const gchar *account; 

			/* Activate progress hint */
			set_progress_hint (window, TRUE);

			helper = g_slice_new0 (DecodeAsyncHelper);
			helper->self = g_object_ref (window);
			helper->file_path = g_strdup (filepath);
			helper->attachment_uid = g_strdup (attachment_uid);

			decode_in_provider = FALSE;
			mgr = modest_runtime_get_account_mgr ();
			account = modest_window_get_active_account (MODEST_WINDOW (window));
			if (modest_account_mgr_account_is_multimailbox (mgr, account, &protocol)) {
				if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
					gchar *uri;
					uri = g_strconcat ("file://", filepath, NULL);
					decode_in_provider = 
						modest_account_protocol_decode_part_to_stream_async (
							MODEST_ACCOUNT_PROTOCOL (protocol),
							mime_part,
							filepath,
							TNY_STREAM (temp_stream),
							on_decode_to_stream_async_handler,
							NULL,
							helper);
					g_free (uri);
				}
			}

			if (!decode_in_provider)
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
				content_type = tny_mime_part_get_content_type (mime_part);
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
			TnyMsg *top_msg;

			/* it's not found, so create a new window for it */
			modest_window_mgr_register_header (mgr, header, attachment_uid); /* register the uid before building the window */
			gchar *account = g_strdup (modest_window_get_active_account (MODEST_WINDOW (window)));
			const gchar *mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (window));
			if (!account)
				account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr ());

			if (priv->top_msg)
				top_msg = g_object_ref (priv->top_msg);
			else
				top_msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
			
			msg_win = modest_msg_view_window_new_with_other_body (TNY_MSG (current_msg), TNY_MIME_PART (mime_part), top_msg,
									      account, mailbox, attachment_uid);

			if (top_msg) g_object_unref (top_msg);
			
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
			/* if it's found, but there is no msg_win, it's probably in the process of being created;
			 * thus, we don't do anything */
			g_debug ("window for is already being created");
		} else {
			TnyMsg *top_msg;
			/* it's not found, so create a new window for it */
			modest_window_mgr_register_header (mgr, header, attachment_uid); /* register the uid before building the window */
			gchar *account = g_strdup (modest_window_get_active_account (MODEST_WINDOW (window)));
			const gchar *mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (window));
			if (!account)
				account = modest_account_mgr_get_default_account (modest_runtime_get_account_mgr ());
			if (priv->top_msg)
				top_msg = g_object_ref (priv->top_msg);
			else
				top_msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
			msg_win = modest_msg_view_window_new_for_attachment (
				TNY_MSG (mime_part), top_msg, account, 
				mailbox, attachment_uid);
			modest_window_set_zoom (MODEST_WINDOW (msg_win),
						modest_window_get_zoom (MODEST_WINDOW (window)));
			if (modest_window_mgr_register_window (mgr, msg_win, MODEST_WINDOW (window)))
				gtk_widget_show_all (GTK_WIDGET (msg_win));
			else
				gtk_widget_destroy (GTK_WIDGET (msg_win));
		}
	}

 frees:
	if (attachment_uid)
		g_free (attachment_uid);
	if (mime_part)
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
	GnomeVFSResult result;
	gchar *uri;
	ModestMsgViewWindow *window;
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
	g_free (info->uri);
	g_object_unref (info->window);
	info->window = NULL;
	if (with_struct) {
		g_slice_free (SaveMimePartInfo, info);
	}
}

static gboolean
idle_save_mime_part_show_result (SaveMimePartInfo *info)
{
	/* This is a GDK lock because we are an idle callback and
	 * modest_platform_system_banner is or does Gtk+ code */

	gdk_threads_enter (); /* CHECKED */
	if (info->result == GNOME_VFS_ERROR_CANCELLED) {
		/* nothing */
	} else if (info->result == GNOME_VFS_OK) {
		modest_platform_system_banner (NULL, NULL, _CS_SAVED);
	} else if (info->result == GNOME_VFS_ERROR_NO_SPACE) {
		gchar *msg = NULL;

		/* Check if the uri belongs to the external mmc */
		if (g_str_has_prefix (info->uri, g_getenv (MODEST_MMC1_VOLUMEPATH_ENV)))
			msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
		else
			msg = g_strdup (_KR("cerm_memory_card_full"));
		modest_platform_information_banner (NULL, NULL, msg);
		g_free (msg);
	} else {
		modest_platform_system_banner (NULL, NULL, _("mail_ib_file_operation_failed"));
	}
	set_progress_hint (info->window, FALSE);
	save_mime_part_info_free (info, FALSE);
	gdk_threads_leave (); /* CHECKED */

	return FALSE;
}

static void
save_mime_part_to_file_connect_handler (gboolean canceled,
					GError *err,
					GtkWindow *parent_window,
					TnyAccount *account,
					SaveMimePartInfo *info)
{
	if (canceled || err) {
		if (canceled && !err) {
			info->result = GNOME_VFS_ERROR_CANCELLED;
		}
		g_idle_add ((GSourceFunc) idle_save_mime_part_show_result, info);
	} else {
		g_thread_create ((GThreadFunc)save_mime_part_to_file, info, FALSE, NULL);
	}
}

static gboolean
save_mime_part_to_file_connect_idle (SaveMimePartInfo *info)
{
	gboolean is_merge;
	TnyAccount *account;
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (info->window);

	is_merge = g_str_has_prefix (priv->msg_uid, "merge:");
	account = NULL;

	/* Get the account */
	if (!is_merge)
		account = tny_account_store_find_account (TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()),
							  priv->msg_uid);

	modest_platform_connect_and_perform ((ModestWindow *) info->window,
					     TRUE,
					     TNY_ACCOUNT (account),
					     (ModestConnectedPerformer) save_mime_part_to_file_connect_handler,
					     info);

	if (account)
		g_object_unref (account);

	return FALSE;
}

static gpointer
save_mime_part_to_file (SaveMimePartInfo *info)
{
	GnomeVFSHandle *handle;
	TnyStream *stream;
	SaveMimePartPair *pair = (SaveMimePartPair *) info->pairs->data;

	if (TNY_IS_CAMEL_BS_MIME_PART (pair->part) &&
	    !tny_camel_bs_mime_part_is_fetched (TNY_CAMEL_BS_MIME_PART (pair->part))) {
		gboolean check_online = TRUE;
		ModestMsgViewWindowPrivate *priv = NULL;

		/* Check if we really need to connect to save the mime part */
		priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (info->window);
		if (g_str_has_prefix (priv->msg_uid, "merge:")) {
			check_online = FALSE;
		} else {
			TnyAccountStore *acc_store;
			TnyAccount *account = NULL;

			acc_store = (TnyAccountStore*) modest_runtime_get_account_store ();
			account = tny_account_store_find_account (acc_store, priv->msg_uid);

			if (account) {
				if (tny_account_get_connection_status (account) ==
				    TNY_CONNECTION_STATUS_CONNECTED)
					check_online = FALSE;
				g_object_unref (account);
			} else {
				check_online = !tny_device_is_online (tny_account_store_get_device (acc_store));
			}
		}

		if (check_online) {
			g_idle_add ((GSourceFunc) save_mime_part_to_file_connect_idle, info);
			return NULL;
		}
	}

	info->result = gnome_vfs_create (&handle, pair->filename, GNOME_VFS_OPEN_WRITE, FALSE, 0644);
	if (info->result == GNOME_VFS_OK) {
		GError *error = NULL;
		gboolean decode_in_provider;
		gssize written;
		ModestAccountMgr *mgr;
		const gchar *account;
		ModestProtocol *protocol = NULL;

		stream = tny_vfs_stream_new (handle);

		decode_in_provider = FALSE;
		mgr = modest_runtime_get_account_mgr ();
		account = modest_window_get_active_account (MODEST_WINDOW (info->window));
		if (modest_account_mgr_account_is_multimailbox (mgr, account, &protocol)) {
			if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
				decode_in_provider = 
					modest_account_protocol_decode_part_to_stream (
						MODEST_ACCOUNT_PROTOCOL (protocol),
						pair->part,
						pair->filename,
						stream,
						&written,
						&error);
			}
		}
		if (!decode_in_provider)
			written = tny_mime_part_decode_to_stream (pair->part, stream, &error);

		if (written < 0) {
			g_warning ("modest: could not save attachment %s: %d (%s)\n", pair->filename, error?error->code:-1, error?error->message:"Unknown error");

			if (error && (error->domain == TNY_ERROR_DOMAIN) &&
			    (error->code == TNY_IO_ERROR_WRITE) &&
			    (errno == ENOSPC)) {
				info->result = GNOME_VFS_ERROR_NO_SPACE;
			} else {
				info->result = GNOME_VFS_ERROR_IO;
			}
		}
		g_object_unref (G_OBJECT (stream));
	} else {
		g_warning ("Could not create save attachment %s: %s\n", 
			   pair->filename, gnome_vfs_result_to_string (info->result));
	}

	/* Go on saving remaining files */
	info->pairs = g_list_remove_link (info->pairs, info->pairs);
	if (info->pairs != NULL) {
		save_mime_part_to_file (info);
	} else {
		g_idle_add ((GSourceFunc) idle_save_mime_part_show_result, info);
	}

	return NULL;
}

static void
save_mime_parts_to_file_with_checks (GtkWindow *parent,
				     SaveMimePartInfo *info)
{
	gboolean is_ok = TRUE;
        gint replaced_files = 0;
        const GList *files = info->pairs;
        const GList *iter, *to_replace = NULL;

        for (iter = files; (iter != NULL) && (replaced_files < 2); iter = g_list_next(iter)) {
                SaveMimePartPair *pair = iter->data;
		gchar *unescaped = g_uri_unescape_string (pair->filename, NULL);

                if (modest_utils_file_exists (unescaped)) {
			replaced_files++;
			if (replaced_files == 1)
				to_replace = iter;
                }
		g_free (unescaped);
        }
	if (replaced_files) {
		gint response;

		if (replaced_files == 1) {
			SaveMimePartPair *pair = to_replace->data;
			const gchar *basename = strrchr (pair->filename, G_DIR_SEPARATOR) + 1;
			gchar *escaped_basename, *message;

			escaped_basename = g_uri_unescape_string (basename, NULL);
			message = g_strdup_printf ("%s\n%s",
						   _FM_REPLACE_FILE,
						   (escaped_basename) ? escaped_basename : "");
			response = modest_platform_run_confirmation_dialog (parent, message);
			g_free (message);
			g_free (escaped_basename);
		} else {
			response = modest_platform_run_confirmation_dialog (parent,
									    _FM_REPLACE_MULTIPLE);
		}
		if (response != GTK_RESPONSE_OK)
			is_ok = FALSE;
	}

	if (!is_ok) {
		save_mime_part_info_free (info, TRUE);
	} else {
		/* Start progress and launch thread */
		set_progress_hint (info->window, TRUE);
		g_thread_create ((GThreadFunc)save_mime_part_to_file, info, FALSE, NULL);
	}

}

typedef struct _SaveAttachmentsInfo {
	TnyList *attachments_list;
	ModestMsgViewWindow *window;
} SaveAttachmentsInfo;

static void
save_attachments_response (GtkDialog *dialog,
			   gint       arg1,
			   gpointer   user_data)  
{
	TnyList *mime_parts;
	gchar *chooser_uri;
	GList *files_to_save = NULL;
	gchar *current_folder;
	SaveAttachmentsInfo *sa_info = (SaveAttachmentsInfo *) user_data;

	mime_parts = TNY_LIST (sa_info->attachments_list);

	if (arg1 != GTK_RESPONSE_OK)
		goto end;

	chooser_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
	current_folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dialog));
	if (current_folder && *current_folder != '\0') {
		GError *err = NULL;
		modest_conf_set_string (modest_runtime_get_conf (), MODEST_CONF_LATEST_SAVE_ATTACHMENT_PATH,
					current_folder,&err);
		if (err != NULL) {
			g_debug ("Error storing latest used folder: %s", err->message);
			g_error_free (err);
		}
	}
	g_free (current_folder);

	if (!modest_utils_folder_writable (chooser_uri)) {
		const gchar *err_msg;

#ifdef MODEST_PLATFORM_MAEMO
		if (modest_maemo_utils_in_usb_mode ()) {
			err_msg = dgettext ("hildon-status-bar-usb", "usbh_ib_mmc_usb_connected");
		} else {
			err_msg = _FM_READ_ONLY_LOCATION;
		}
#else
		err_msg = _FM_READ_ONLY_LOCATION;
#endif
		modest_platform_system_banner (NULL, NULL, err_msg);
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

	if (files_to_save != NULL) {
		SaveMimePartInfo *info = g_slice_new0 (SaveMimePartInfo);
		info->pairs = files_to_save;
		info->result = TRUE;
		info->uri = g_strdup (chooser_uri);
		info->window = g_object_ref (sa_info->window);
		save_mime_parts_to_file_with_checks ((GtkWindow *) dialog, info);
	}
	g_free (chooser_uri);

 end:
	/* Free and close the dialog */
	g_object_unref (mime_parts);
	g_object_unref (sa_info->window);
	g_slice_free (SaveAttachmentsInfo, sa_info);
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static gboolean
msg_is_attachment (TnyList *mime_parts)
{
	TnyIterator *iter;
	gboolean retval = FALSE;

	if (tny_list_get_length (mime_parts) > 1)
		return FALSE;

	iter = tny_list_create_iterator (mime_parts);
	if (iter) {
		TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (part) {
			if (TNY_IS_MSG (part))
				retval = TRUE;
			g_object_unref (part);
		}
		g_object_unref (iter);
	}
	return retval;
}

void
modest_msg_view_window_save_attachments (ModestMsgViewWindow *window, 
					 TnyList *mime_parts)
{
	ModestMsgViewWindowPrivate *priv;
	GtkWidget *save_dialog = NULL;
	gchar *conf_folder = NULL;
	gchar *filename = NULL;
	gchar *save_multiple_str = NULL;
	const gchar *root_folder = "file:///";

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (mime_parts == NULL) {
		gboolean allow_msgs = FALSE;

		/* In Hildon 2.2 save and delete operate over all the attachments as there's no
		 * selection available */
		mime_parts = modest_msg_view_get_attachments (MODEST_MSG_VIEW (priv->msg_view));

		/* Check if the message is composed by an unique MIME
		   part whose content disposition is attachment. There
		   could be messages like this:

		   Date: Tue, 12 Jan 2010 20:40:59 +0000
		   From: <sender@example.org>
		   To: <recipient@example.org>
		   Subject: no body
		   Content-Type: image/jpeg
		   Content-Disposition: attachment; filename="bug7718.jpeg"

		   whose unique MIME part is the message itself whose
		   content disposition is attachment
		*/
		if (mime_parts && msg_is_attachment (mime_parts))
			allow_msgs = TRUE;

		if (mime_parts && !modest_toolkit_utils_select_attachments (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (window))), mime_parts, allow_msgs)) {
			g_object_unref (mime_parts);
			return;
		}

		if (mime_parts == NULL || tny_list_get_length (mime_parts) == 0) {
			if (mime_parts) {
				g_object_unref (mime_parts);
				mime_parts = NULL;
			}
			return;
		}
	} else {
		g_object_ref (mime_parts);
	}

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
			g_warning ("%s: Tried to save a non-file attachment", __FUNCTION__);
			g_object_unref (mime_parts);
			return;
		}
		g_object_unref (mime_part);
	} else {
		gint num = tny_list_get_length (mime_parts);
		save_multiple_str = g_strdup_printf (dngettext("hildon-fm",
							       "sfil_va_number_of_objects_attachment",
							      "sfil_va_number_of_objects_attachments",
							      num), num);
	}

	/* Creation of hildon file chooser dialog for saving */
	save_dialog = modest_toolkit_factory_create_file_chooser_dialog (modest_runtime_get_toolkit_factory (),
									 "",
									 (GtkWindow *) window,
									 GTK_FILE_CHOOSER_ACTION_SAVE);

	/* Get last used folder */
	conf_folder = modest_conf_get_string (modest_runtime_get_conf (),
					      MODEST_CONF_LATEST_SAVE_ATTACHMENT_PATH, NULL);

	/* File chooser stops working if we select "file:///" as current folder */
	if (conf_folder && g_ascii_strcasecmp (root_folder, conf_folder) != 0) {
		g_free (conf_folder);
		conf_folder = NULL;
	}

	if (conf_folder && conf_folder[0] != '\0') {
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (save_dialog), conf_folder);
	} else {
		gchar *docs_folder;
		/* Set the default folder to documents folder */
		docs_folder = (gchar *) g_strdup(g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS));
		if (!docs_folder) {
			/* fallback */
			docs_folder = g_build_filename (g_getenv (MYDOCS_ENV), DOCS_FOLDER, NULL);
		}
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (save_dialog), docs_folder);
		g_free (docs_folder);
	}
	g_free (conf_folder);

	/* set filename */
	if (filename) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (save_dialog), 
						   filename);
		g_free (filename);
	}

	/* if multiple, set multiple string */
	if (save_multiple_str) {
		g_object_set (G_OBJECT (save_dialog), "save-multiple", save_multiple_str, NULL);
		gtk_window_set_title (GTK_WINDOW (save_dialog), _FM_SAVE_OBJECT_FILES);
		g_free (save_multiple_str);
	}

	/* We must run this asynchronously, because the hildon dialog
	   performs a gtk_dialog_run by itself which leads to gdk
	   deadlocks */
	SaveAttachmentsInfo *sa_info;
	sa_info = g_slice_new (SaveAttachmentsInfo);
	sa_info->attachments_list = mime_parts;
	sa_info->window = g_object_ref (window);
	g_signal_connect (save_dialog, "response", 
			  G_CALLBACK (save_attachments_response), sa_info);

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
		modest_platform_animation_banner (NULL, NULL, _("mcen_me_inbox_remove_attachments")));

	gdk_threads_leave ();

	return FALSE;
}

void
modest_msg_view_window_remove_attachments (ModestMsgViewWindow *window, gboolean get_all)
{
	ModestMsgViewWindowPrivate *priv;
	TnyList *mime_parts = NULL, *tmp;
	gchar *confirmation_message;
	gint response;
	gint n_attachments;
	TnyMsg *msg;
	TnyIterator *iter;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window));
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

#ifdef MODEST_TOOLKIT_HILDON2
	/* In hildon 2.2 we ignore the get_all flag as we always get all attachments. This is
	 * because we don't have selection
	 */
	mime_parts = modest_msg_view_get_attachments (MODEST_MSG_VIEW (priv->msg_view));

	/* Remove already purged messages from mime parts list. We use
	   a copy of the list to remove items in the original one */
	tmp = tny_list_copy (mime_parts);
	iter = tny_list_create_iterator (tmp);
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_purged (part))
			tny_list_remove (mime_parts, (GObject *) part);

		g_object_unref (part);
		tny_iterator_next (iter);
	}
	g_object_unref (tmp);
	g_object_unref (iter);

	if (!modest_toolkit_utils_select_attachments (GTK_WINDOW (window), mime_parts, TRUE) ||
	    tny_list_get_length (mime_parts) == 0) {
		g_object_unref (mime_parts);
		return;
	}
#else
	/* In gtk we get only selected attachments for the operation.
	 */
	mime_parts = modest_msg_view_get_selected_attachments (MODEST_MSG_VIEW (priv->msg_view));

	/* Remove already purged messages from mime parts list. We use
	   a copy of the list to remove items in the original one */
	tmp = tny_list_copy (mime_parts);
	iter = tny_list_create_iterator (tmp);
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_purged (part))
			tny_list_remove (mime_parts, (GObject *) part);

		g_object_unref (part);
		tny_iterator_next (iter);
	}
	g_object_unref (tmp);
	g_object_unref (iter);

	if (tny_list_get_length (mime_parts) == 0) {
		g_object_unref (mime_parts);
		return;
	}
#endif

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
	response = modest_platform_run_confirmation_dialog (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (window))),
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
	update_branding (MODEST_MSG_VIEW_WINDOW (window));

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
	ModestMsgViewWindowPrivate *priv;
	TnyMsg *msg = NULL;
	TnyHeader *header = NULL;
	gchar *subject = NULL;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	/* Note that if the window is closed while we're retrieving
	   the message, this widget could de deleted */
	if (!priv->msg_view)
		return;

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

	modest_window_set_title (MODEST_WINDOW (window), subject);
}


static void
on_move_focus (GtkWidget *widget,
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
	GtkWidget *window;
} FetchImageData;

gboolean
on_fetch_image_timeout_refresh_view (gpointer userdata)
{
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (userdata);
	update_progress_hint (MODEST_MSG_VIEW_WINDOW (userdata));
	/* Note that priv->msg_view is set to NULL when this window is
	   distroyed */
	if (priv->msg_view && GTK_WIDGET_DRAWABLE (priv->msg_view)) {
		gtk_widget_queue_draw (GTK_WIDGET (priv->msg_view));
	}
	priv->fetch_image_redraw_handler = 0;
	g_object_unref (userdata);
	return FALSE;
}

gboolean
on_fetch_image_idle_refresh_view (gpointer userdata)
{

	FetchImageData *fidata = (FetchImageData *) userdata;

	gdk_threads_enter ();
	if (GTK_WIDGET_DRAWABLE (fidata->msg_view)) {
		ModestMsgViewWindowPrivate *priv;

		priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (fidata->window);
		priv->fetching_images--;
		if (priv->fetch_image_redraw_handler == 0) {
			priv->fetch_image_redraw_handler = g_timeout_add (500, on_fetch_image_timeout_refresh_view, g_object_ref (fidata->window));
		}

	}
	gdk_threads_leave ();

	g_object_unref (fidata->msg_view);
	g_object_unref (fidata->window);
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
	cache_stream = 
		tny_stream_cache_get_stream (cache, 
					     fidata->cache_id, 
					     (TnyStreamCacheOpenStreamFetcher) fetch_image_open_stream, 
					     (gpointer) fidata->uri);
	g_free (fidata->cache_id);
	g_free (fidata->uri);

	if (cache_stream != NULL) {
		char buffer[4096];

		while (G_LIKELY (!tny_stream_is_eos (cache_stream))) {
			gssize nb_read;

			nb_read = tny_stream_read (cache_stream, buffer, sizeof (buffer));
			if (G_UNLIKELY (nb_read < 0)) {
				break;
			} else if (G_LIKELY (nb_read > 0)) {
				gssize nb_written = 0;

				while (G_UNLIKELY (nb_written < nb_read)) {
					gssize len;

					len = tny_stream_write (fidata->output_stream, buffer + nb_written,
								nb_read - nb_written);
					if (G_UNLIKELY (len < 0))
						break;
					nb_written += len;
				}
			}
		}
		tny_stream_close (cache_stream);
		g_object_unref (cache_stream);
	}

	tny_stream_close (fidata->output_stream);
	g_object_unref (fidata->output_stream);

	g_idle_add (on_fetch_image_idle_refresh_view, fidata);

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
	fidata->window = g_object_ref (window);
	fidata->uri = g_strdup (uri);
	fidata->cache_id = modest_images_cache_get_id (current_account, uri);
	fidata->output_stream = g_object_ref (stream);

	priv->fetching_images++;
	if (g_thread_create (on_fetch_image_thread, fidata, FALSE, NULL) == NULL) {
		g_object_unref (fidata->output_stream);
		g_free (fidata->cache_id);
		g_free (fidata->uri);
		g_object_unref (fidata->msg_view);
		g_slice_free (FetchImageData, fidata);
		tny_stream_close (stream);
		priv->fetching_images--;
		update_progress_hint (window);
		return FALSE;
	}
	update_progress_hint (window);

	return TRUE;
}

static void 
setup_menu (ModestMsgViewWindow *self)
{
	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW(self));

	/* Settings menu buttons */
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_new_message"), "<Control>n",
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_on_new_msg),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_new_msg));

	modest_window_add_to_menu (MODEST_WINDOW (self),
				   dngettext(GETTEXT_PACKAGE,
					     "mcen_me_move_message",
					     "mcen_me_move_messages",
					     1),
				   NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_on_move_to),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_move_to));

	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_inbox_mark_as_read"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_on_mark_as_read),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_mark_as_read_msg_in_view));
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_inbox_mark_as_unread"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_on_mark_as_unread),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_mark_as_unread_msg_in_view));

	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_viewer_save_attachments"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_save_attachments),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_save_attachments));
	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_inbox_remove_attachments"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_remove_attachments),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_remove_attachments));

	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_viewer_addtocontacts"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_add_to_contacts),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_add_to_contacts));

	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_ti_message_properties"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_ui_actions_on_details),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_details));

	modest_window_add_to_menu (MODEST_WINDOW (self), _("mcen_me_viewer_find"), NULL,
				   MODEST_WINDOW_MENU_CALLBACK (modest_msg_view_window_show_isearch_toolbar),
				   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_find_in_msg));
}

void
modest_msg_view_window_add_to_contacts (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);
	GSList *recipients = NULL;
	TnyMsg *msg = NULL;

	msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
	if (msg == NULL) {
		TnyHeader *header;

		header = modest_msg_view_window_get_header (self);
		if (header == NULL)
			return;
		recipients = modest_tny_msg_header_get_all_recipients_list (header);
		g_object_unref (header);
	} else {
		recipients = modest_tny_msg_get_all_recipients_list (msg);
		g_object_unref (msg);
	}

	if (recipients) {
		/* Offer the user to add recipients to the address book */
		modest_address_book_add_address_list_with_selector (recipients, (GtkWindow *) self);
		g_slist_foreach (recipients, (GFunc) g_free, NULL); g_slist_free (recipients);
	}
}

static gboolean 
_modest_msg_view_window_map_event (GtkWidget *widget,
				   GdkEvent *event,
				   gpointer userdata)
{
	ModestMsgViewWindow *self = (ModestMsgViewWindow *) userdata;

	update_progress_hint (self);

	return FALSE;
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
	const gchar *msg_uid;
	TnyHeader *header = NULL;
	TnyFolder *folder = NULL;

	g_return_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self));

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	header = modest_msg_view_window_get_header (MODEST_MSG_VIEW_WINDOW (self));
	if (!header)
		return;

	folder = tny_header_get_folder (header);
	g_object_unref (header);

	if (!folder)
		return;

	msg_uid = modest_msg_view_window_get_message_uid (self);
        if (msg_uid) {
		GtkTreeRowReference *row_reference;

		if (priv->row_reference && gtk_tree_row_reference_valid (priv->row_reference)) {
			row_reference = priv->row_reference;
		} else {
			row_reference = NULL;
		}
		if (!message_reader (self, priv, NULL, msg_uid, folder, row_reference))
			g_warning ("Shouldn't happen, trying to reload a message failed");
	}

	g_object_unref (folder);
}

static void
update_branding (ModestMsgViewWindow *self)
{
	const gchar *account; 
	const gchar *mailbox;
	ModestAccountMgr *mgr;
	ModestProtocol *protocol = NULL;
	gchar *service_name = NULL;
	const GdkPixbuf *service_icon = NULL;
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	account = modest_window_get_active_account (MODEST_WINDOW (self));
	mailbox = modest_window_get_active_mailbox (MODEST_WINDOW (self));

	mgr = modest_runtime_get_account_mgr ();

	if (modest_account_mgr_account_is_multimailbox (mgr, account, &protocol)) {
		if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
			service_name = modest_account_protocol_get_service_name (MODEST_ACCOUNT_PROTOCOL (protocol),
										 account, mailbox);
			service_icon = modest_account_protocol_get_service_icon (MODEST_ACCOUNT_PROTOCOL (protocol),
										 account, mailbox, MODEST_ICON_SIZE_SMALL);
		}
	}

	modest_msg_view_set_branding (MODEST_MSG_VIEW (priv->msg_view), service_name, service_icon);
	g_free (service_name);
}

static void
sync_flags (ModestMsgViewWindow *self)
{
	TnyHeader *header = NULL;

	header = modest_msg_view_window_get_header (self);
	if (!header) {
		TnyMsg *msg = modest_msg_view_window_get_message (self);
		if (msg) {
			header = tny_msg_get_header (msg);
			g_object_unref (msg);
		}
	}

	if (header) {
		TnyFolder *folder = tny_header_get_folder (header);

		if (folder) {
			ModestMailOperation *mail_op;

			/* Sync folder, we need this to save the seen flag */
			mail_op = modest_mail_operation_new (NULL);
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
							 mail_op);
			modest_mail_operation_sync_folder (mail_op, folder, FALSE, NULL, NULL);
			g_object_unref (mail_op);
			g_object_unref (folder);
		}
		g_object_unref (header);
	}
}

#ifdef MODEST_TOOLKIT_HILDON2
static gboolean
on_realize (GtkWidget *widget,
	    gpointer userdata)
{
	GdkDisplay *display;
	Atom atom;
	unsigned long val = 1;

	display = gdk_drawable_get_display (widget->window);
	atom = gdk_x11_get_xatom_by_name_for_display (display, "_HILDON_ZOOM_KEY_ATOM");
	XChangeProperty (GDK_DISPLAY_XDISPLAY (display),
			 GDK_WINDOW_XID (widget->window), atom,
			 XA_INTEGER, 32, PropModeReplace,
			 (unsigned char *) &val, 1);

	return FALSE;
}
#endif

static gboolean
on_handle_calendar (ModestMsgView *msgview, TnyMimePart *calendar_part, GtkContainer *container, ModestMsgViewWindow *self)
{
	const gchar *account_name;
	ModestProtocolType proto_type;
	ModestProtocol *protocol;
	gboolean retval = FALSE;

	account_name = modest_window_get_active_account (MODEST_WINDOW (self));
	
	/* Get proto */
	proto_type = modest_account_mgr_get_store_protocol (modest_runtime_get_account_mgr (), 
							    account_name);
	protocol = 
		modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (), 
							       proto_type);

	if (MODEST_IS_ACCOUNT_PROTOCOL (protocol)) {
		retval = modest_account_protocol_handle_calendar (MODEST_ACCOUNT_PROTOCOL (protocol), MODEST_WINDOW (self),
								  calendar_part, container);
	}
	return retval;
}

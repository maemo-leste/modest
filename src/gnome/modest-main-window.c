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
#include <tny-error.h>

#include <widgets/modest-main-window.h>
#include <widgets/modest-window-priv.h>
#include <widgets/modest-msg-edit-window.h>
#include <widgets/modest-account-view-window.h>

#include <modest-runtime.h>
#include "modest-widget-memory.h"
#include "modest-ui-actions.h"
#include "modest-main-window-ui.h"
#include "modest-account-mgr.h"
#include "modest-conf.h"
#include <modest-tny-msg.h>
#include "modest-mail-operation.h"
#include "modest-icon-names.h"
#include "modest-gnome-info-bar.h"

/* 'private'/'protected' functions */
static void modest_main_window_class_init    (ModestMainWindowClass *klass);
static void modest_main_window_init          (ModestMainWindow *obj);
static void modest_main_window_finalize      (GObject *obj);

static void restore_sizes (ModestMainWindow *self);
static void save_sizes (ModestMainWindow *self);

static gboolean     on_header_view_button_press_event   (ModestHeaderView *header_view,
							 GdkEventButton   *event,
							 ModestMainWindow *self);

static gboolean     on_folder_view_button_press_event   (ModestFolderView *folder_view,
							 GdkEventButton   *event,
							 ModestMainWindow *self);

static gboolean     show_context_popup_menu             (ModestMainWindow *window,
							 GtkTreeView      *tree_view,
							 GdkEventButton   *event,
							 GtkWidget        *menu);

static void         connect_signals                      (ModestMainWindow *self);

static void         on_queue_changed                     (ModestMailOperationQueue *queue,
							  ModestMailOperation *mail_op,
							  ModestMailOperationQueueNotification type,
							  ModestMainWindow *self);

static void         on_header_status_update              (ModestHeaderView *header_view, 
							  const gchar *msg, 
							  gint num, 
							  gint total,  
							  ModestMainWindow *main_window);

static void         on_header_selected                   (ModestHeaderView *header_view, 
							  TnyHeader *header,
							  ModestMainWindow *main_window);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMainWindowPrivate ModestMainWindowPrivate;
struct _ModestMainWindowPrivate {

	GtkWidget        *folder_paned;
	GtkWidget        *msg_paned;
	GtkWidget        *main_paned;
	
	GtkWidget        *online_toggle;
	GtkWidget        *folder_info_label;

	ModestHeaderView *header_view;
	ModestFolderView *folder_view;
	ModestMsgView    *msg_preview;

	GtkWidget        *status_bar;
	GtkWidget        *progress_bar;

	GSList           *progress_widgets;
	GtkWidget        *main_bar;
};


#define MODEST_MAIN_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_MAIN_WINDOW, \
                                                ModestMainWindowPrivate))

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
modest_main_window_init (ModestMainWindow *obj)
{
	TnyAccountStore         *account_store;
	ModestMainWindowPrivate *priv;
	TnyFolderStoreQuery     *query;
	GtkWidget               *icon;
	gboolean                online;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);
	
	priv->folder_paned = NULL;
	priv->msg_paned    = NULL;
	priv->main_paned   = NULL;	
	priv->progress_widgets = NULL;

	account_store = TNY_ACCOUNT_STORE (modest_runtime_get_account_store ());

	/* online/offline toggle */
	priv->online_toggle = gtk_toggle_button_new ();
	online  = tny_device_is_online (modest_runtime_get_device());
	icon    = gtk_image_new_from_icon_name (online ? GTK_STOCK_CONNECT : GTK_STOCK_DISCONNECT,
						GTK_ICON_SIZE_BUTTON);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->online_toggle), online);
	gtk_button_set_image (GTK_BUTTON(priv->online_toggle),icon);

	/* Paned */
	priv->folder_paned = gtk_vpaned_new ();
	priv->main_paned = gtk_hpaned_new ();
	priv->msg_paned = gtk_vpaned_new ();

	/* Main bar */
	priv->folder_info_label = gtk_label_new (NULL);
	priv->main_bar = modest_gnome_info_bar_new ();
	priv->progress_widgets = g_slist_prepend (priv->progress_widgets, 
							  priv->main_bar);

	/* msg preview */
	priv->msg_preview = MODEST_MSG_VIEW(modest_msg_view_new (NULL));
	if (!priv->msg_preview)
		g_printerr ("modest: cannot instantiate msgpreiew\n");

	/* header view */
	priv->header_view  =
		MODEST_HEADER_VIEW(modest_header_view_new (NULL, MODEST_HEADER_VIEW_STYLE_DETAILS));
	if (!priv->header_view)
		g_printerr ("modest: cannot instantiate header view\n");

	/* folder view */
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL,
					 TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);

	priv->folder_view = MODEST_FOLDER_VIEW (modest_folder_view_new (query));
	if (!priv->folder_view)
		g_printerr ("modest: cannot instantiate folder view\n");	
	g_object_unref (G_OBJECT (query));
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
	case MODEST_WIDGET_TYPE_MSG_PREVIEW:
		widget = (GtkWidget*)priv->msg_preview; break;
	default:
		g_return_val_if_reached (NULL);
		return NULL;
	}

	return widget ? GTK_WIDGET(widget) : NULL;
}


static void
restore_sizes (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	conf = modest_runtime_get_conf ();
	
	modest_widget_memory_restore (conf, G_OBJECT(priv->folder_paned),
				      MODEST_CONF_FOLDER_PANED_KEY);
	modest_widget_memory_restore (conf, G_OBJECT(priv->msg_paned),
				      MODEST_CONF_MSG_PANED_KEY);
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
	ModestWindowPrivate *parent_priv;
	ModestMainWindowPrivate *priv;
	ModestConf *conf;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	conf = modest_runtime_get_conf ();
	
	modest_widget_memory_save (conf, G_OBJECT(priv->folder_paned),
				   MODEST_CONF_FOLDER_PANED_KEY);
	modest_widget_memory_save (conf, G_OBJECT(priv->msg_paned),
				   MODEST_CONF_MSG_PANED_KEY);
	modest_widget_memory_save (conf, G_OBJECT(priv->main_paned),
				   MODEST_CONF_MAIN_PANED_KEY);
	modest_widget_memory_save (conf, G_OBJECT(priv->header_view),
				   MODEST_CONF_HEADER_VIEW_KEY);
	modest_widget_memory_save (conf, G_OBJECT(self), 
				   MODEST_CONF_MAIN_WINDOW_KEY);
}


static void
on_connection_changed (TnyDevice *device, gboolean online, ModestMainWindow *self)
{
	GtkWidget *icon;
	const gchar *icon_name;
	ModestMainWindowPrivate *priv;
	
	g_return_if_fail (device);
	g_return_if_fail (self);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	icon_name = online ? GTK_STOCK_CONNECT : GTK_STOCK_DISCONNECT;
	icon      = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);

	/* Block handlers in order to avoid unnecessary calls */
	//g_signal_handler_block (G_OBJECT (priv->online_toggle), priv->toggle_button_signal);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->online_toggle), online);
	//g_signal_handler_unblock (G_OBJECT (online_toggle), priv->toggle_button_signal);

	gtk_button_set_image (GTK_BUTTON(priv->online_toggle), icon);
	//statusbar_push (widget_factory, 0, online ? _("Modest went online") : _("Modest went offline"));
	
	/* If Modest has became online and the header view has a
	   header selected then show it */
	/* FIXME: there is a race condition if some account needs to
	   ask the user for a password */

/* 	if (online) { */
/* 		GtkTreeSelection *selected; */

/* 		selected = gtk_tree_view_get_selection (GTK_TREE_VIEW (header_view)); */
/* 		_modest_header_view_change_selection (selected, header_view); */
/* 	} */
}

void
on_online_toggle_toggled (GtkToggleButton *toggle, ModestMainWindow *self)
{
	gboolean online;
	TnyDevice *device;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	device = modest_runtime_get_device ();
	online  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->online_toggle));

	if (online) {
		/* TODO: Just attempt to go online, instead of forcing the online status: */
		tny_device_force_online (device);
	}
	else
		tny_device_force_offline (device);
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
	ModestTnyAccountStore *account_store;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	account_store = modest_runtime_get_account_store ();
	
	/* folder view */
	g_signal_connect (G_OBJECT(priv->folder_view), "button-press-event",
			  G_CALLBACK (on_folder_view_button_press_event),self);
	g_signal_connect (G_OBJECT(priv->folder_view), "folder_selection_changed",
			  G_CALLBACK(modest_ui_actions_on_folder_selection_changed), self);
	g_signal_connect (G_OBJECT(priv->folder_view), "folder-display-name-changed",
			  G_CALLBACK(modest_ui_actions_on_folder_display_name_changed), self);

	/* header view */
	g_signal_connect (G_OBJECT(priv->header_view), "status_update",
			  G_CALLBACK(on_header_status_update), self);
	g_signal_connect (G_OBJECT(priv->header_view), "header_selected",
			  G_CALLBACK(modest_ui_actions_on_header_selected), self);
	g_signal_connect (G_OBJECT(priv->header_view), "header_selected",
			  G_CALLBACK(on_header_selected), self);
	g_signal_connect (G_OBJECT(priv->header_view), "header_activated",
			  G_CALLBACK(modest_ui_actions_on_header_activated), self);
	g_signal_connect (G_OBJECT(priv->header_view), "item_not_found",
			  G_CALLBACK(modest_ui_actions_on_item_not_found), self);
	g_signal_connect (G_OBJECT(priv->header_view), "button-press-event",
			  G_CALLBACK (on_header_view_button_press_event), self);
	g_signal_connect (G_OBJECT(priv->header_view),"popup-menu",
			  G_CALLBACK (on_header_view_button_press_event), self);
		
	/* msg preview */
	g_signal_connect (G_OBJECT(priv->msg_preview), "link_clicked",
			  G_CALLBACK(modest_ui_actions_on_msg_link_clicked), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "link_hover",
			  G_CALLBACK(modest_ui_actions_on_msg_link_hover), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "attachment_clicked",
			  G_CALLBACK(modest_ui_actions_on_msg_attachment_clicked), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "recpt-activated",
			  G_CALLBACK(modest_ui_actions_on_msg_recpt_activated), self);

	/* Account store */
	g_signal_connect (G_OBJECT (modest_runtime_get_account_store()), "password_requested",
			  G_CALLBACK (modest_ui_actions_on_password_requested), self);
	
	/* Device */
	g_signal_connect (G_OBJECT(modest_runtime_get_device()), "connection_changed",
			  G_CALLBACK(on_connection_changed), self);
	g_signal_connect (G_OBJECT(priv->online_toggle), "toggled",
			  G_CALLBACK(on_online_toggle_toggled), self);

	/* Mail Operation Queue */
	g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
			  "queue-changed",
			  G_CALLBACK (on_queue_changed),
			  self);
	
	/* window */
	g_signal_connect (G_OBJECT(self), "delete-event", G_CALLBACK(on_delete_event), self);
}


static GtkWidget*
wrapped_in_scrolled_window (GtkWidget *widget, gboolean needs_viewport)
{
	GtkWidget *win;

	win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy
		(GTK_SCROLLED_WINDOW (win),GTK_POLICY_NEVER,
		 GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (win),
					     GTK_SHADOW_IN);
	
	if (needs_viewport)
		gtk_scrolled_window_add_with_viewport
			(GTK_SCROLLED_WINDOW(win), widget);
	else
		gtk_container_add (GTK_CONTAINER(win),
				   widget);

	return win;
}




ModestWindow *
modest_main_window_new (void)
{
	GObject *obj;
	ModestMainWindow *self;
	ModestMainWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkWidget *main_vbox;
	GtkWidget *status_hbox;
	GtkWidget *header_win, *folder_win;
	GtkWidget *preview_scroll;
	GtkActionGroup *action_group;
	GError *error = NULL;
		
	obj  = g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL);
	self = MODEST_MAIN_WINDOW(obj);
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	
	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMainWindowActions");
	
	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
				      obj);

	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 0);
	g_object_unref (action_group);

	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager,
					 MODEST_UIDIR "modest-main-window-ui.xml", &error);
	if (error != NULL) {
		g_printerr ("modest: could not merge modest-main-window-ui.xml: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));

	/* Toolbar / Menubar */
	parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	parent_priv->menubar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar");

	gtk_toolbar_set_tooltips (GTK_TOOLBAR (parent_priv->toolbar), TRUE);
	folder_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->folder_view), FALSE);
	header_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->header_view), FALSE);

	/* Paned */
	preview_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (preview_scroll),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_add1 (GTK_PANED(priv->main_paned), folder_win);
	gtk_paned_add2 (GTK_PANED(priv->main_paned), priv->msg_paned);
	gtk_paned_add1 (GTK_PANED(priv->msg_paned), header_win);
	gtk_container_add (GTK_CONTAINER (preview_scroll),
			   GTK_WIDGET(priv->msg_preview));
	gtk_paned_add2 (GTK_PANED(priv->msg_paned), preview_scroll);

	/* Main Bar */
	status_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox), priv->folder_info_label, FALSE,FALSE, 6);
	gtk_box_pack_start (GTK_BOX(status_hbox), priv->main_bar, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox), priv->online_toggle,FALSE, FALSE, 0);

	/* putting it all together... */
	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), parent_priv->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), parent_priv->toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_paned, TRUE, TRUE,0);
	gtk_box_pack_start (GTK_BOX(main_vbox), status_hbox, FALSE, FALSE, 0);
	
	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
	restore_sizes (MODEST_MAIN_WINDOW(obj));	

	gtk_window_set_title (GTK_WINDOW(obj), _("Modest"));
	gtk_window_set_icon_from_file  (GTK_WINDOW(obj), MODEST_APP_ICON, NULL);	
	gtk_widget_show_all (main_vbox);
	
	/* Connect signals */
	connect_signals (MODEST_MAIN_WINDOW(obj));

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	return (ModestWindow *) obj;
}

static gboolean 
on_header_view_button_press_event (ModestHeaderView *header_view,
				   GdkEventButton   *event,
				   ModestMainWindow *self)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		GtkWidget *menu;
		ModestWindowPrivate *parent_priv;
	
		parent_priv = MODEST_WINDOW_GET_PRIVATE (self);
		menu = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/HeaderViewCSM");

		return show_context_popup_menu (self,
						GTK_TREE_VIEW (header_view), 
						event, 
						menu);
        }

        return FALSE;
}

static gboolean 
on_folder_view_button_press_event (ModestFolderView *folder_view,
				   GdkEventButton   *event,
				   ModestMainWindow *self)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		GtkWidget *menu;
		ModestWindowPrivate *parent_priv;
	
		parent_priv = MODEST_WINDOW_GET_PRIVATE (self);
		menu = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/FolderViewCSM");

		return show_context_popup_menu (self,
						GTK_TREE_VIEW (folder_view), 
						event, 
						menu);
        }

        return FALSE;
}

static gboolean 
show_context_popup_menu (ModestMainWindow *window,
			 GtkTreeView *tree_view,
			 GdkEventButton   *event,			 
			 GtkWidget *menu)
{
	g_return_val_if_fail (menu, FALSE);

        if (event != NULL) {
		/* Ensure that the header is selected */
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (tree_view);
	
		if (gtk_tree_selection_count_selected_rows (selection) <= 1) {
			GtkTreePath *path;
		
			/* Get tree path for row that was clicked */
			if (gtk_tree_view_get_path_at_pos (tree_view,
							   (gint) event->x, 
							   (gint) event->y,
							   &path, 
							   NULL, NULL, NULL)) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
				gtk_tree_path_free (path);
			}
		}

		/* Show popup */
		if (gtk_tree_selection_count_selected_rows(selection) == 1)
			gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
					NULL, NULL,
					event->button, event->time);
	}
	return TRUE;
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestMainWindow *self)
{
	GSList *tmp;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	tmp = priv->progress_widgets;

	switch (type) {
	case MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED:
		while (tmp) {
			modest_progress_object_add_operation (MODEST_PROGRESS_OBJECT (tmp->data),
							      mail_op);
			tmp = g_slist_next (tmp);
		}
		break;
	case MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED:
		while (tmp) {
			modest_progress_object_remove_operation (MODEST_PROGRESS_OBJECT (tmp->data),
								 mail_op);
			tmp = g_slist_next (tmp);
		}
		break;
	}
}

static void
on_header_status_update (ModestHeaderView *header_view, 
			 const gchar *msg, gint num, 
			 gint total,  ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;
	gchar *txt;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	/* Set progress */
	txt = g_strdup_printf (_("Downloading %d of %d"), num, total);
	modest_gnome_info_bar_set_progress (MODEST_GNOME_INFO_BAR (priv->main_bar), 
					    (const gchar*) txt,
					    num, total);
	g_free (txt);
	
	/* Set status message */
	modest_gnome_info_bar_set_message (MODEST_GNOME_INFO_BAR (priv->main_bar), msg);
}

gboolean 
modest_main_window_close_all (ModestMainWindow *self)
{
	/* TODO: show a dialog to ask the user for permission to close
	   all */
	return TRUE;
}

void 
modest_main_window_set_style (ModestMainWindow *self, 
			      ModestMainWindowStyle style)
{
	/* TODO */
}


ModestMainWindowStyle
modest_main_window_get_style (ModestMainWindow *self)
{
	/* TODO */
	return MODEST_MAIN_WINDOW_STYLE_SPLIT;
}

void 
modest_main_window_set_contents_style (ModestMainWindow *self, 
				       ModestMainWindowContentsStyle style)
{
	/* TODO */
}

static void
get_msg_callback (TnyFolder *folder, 
		  TnyMsg *msg, 
		  GError **err, 
		  gpointer user_data)
{
	if (!(*err)) {
		ModestMsgView *msg_preview;

		msg_preview = MODEST_MSG_VIEW (user_data);
		modest_msg_view_set_message (msg_preview, msg);
	}

	/* Frees */
	g_object_unref (folder);
}

static void 
on_header_selected (ModestHeaderView *header_view, 
		    TnyHeader *header,
		    ModestMainWindow *main_window)
{
	TnyFolder *folder;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (main_window);

	if (!header)
		return;

	folder = tny_header_get_folder (header);

	/* FIXME: do not use this directly. Use a mail operation
	   instead in order to get progress info */
	tny_folder_get_msg_async (folder, 
				  header, 
				  get_msg_callback, 
				  NULL, 
				  priv->msg_preview);
}

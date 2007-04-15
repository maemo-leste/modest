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
#include <tny-header.h>
#include "modest-platform.h"
#include <modest-tny-msg.h>
#include <modest-msg-view-window.h>
#include <modest-main-window-ui.h>
#include <modest-widget-memory.h>
#include <modest-runtime.h>
#include <modest-window-priv.h>
#include <modest-tny-folder.h>
#include <hildon-widgets/hildon-find-toolbar.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <gtkhtml/gtkhtml-search.h>
#include <gdk/gdkkeysyms.h>

static void  modest_msg_view_window_class_init   (ModestMsgViewWindowClass *klass);
static void  modest_msg_view_window_init         (ModestMsgViewWindow *obj);
static void  modest_msg_view_window_finalize     (GObject *obj);
static void  modest_msg_view_window_toggle_find_toolbar (GtkToggleAction *obj,
							 gpointer data);
static void  modest_msg_view_window_find_toolbar_close (GtkWidget *widget,
							ModestMsgViewWindow *obj);
static void  modest_msg_view_window_find_toolbar_search (GtkWidget *widget,
							ModestMsgViewWindow *obj);

static void  modest_msg_view_window_set_zoom (ModestWindow *window,
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
static gboolean modest_msg_view_window_is_last_message (ModestMsgViewWindow *window);
static gboolean modest_msg_view_window_is_first_message (ModestMsgViewWindow *window);
static TnyFolderType modest_msg_view_window_get_folder_type (ModestMsgViewWindow *window);
static void modest_msg_view_window_update_dimmed (ModestMsgViewWindow *window);
static void modest_msg_view_window_update_priority (ModestMsgViewWindow *window);

static void modest_msg_view_window_show_toolbar   (ModestWindow *window,
						   gboolean show_toolbar);


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

static const GtkToggleActionEntry msg_view_toggle_action_entries [] = {
	{ "FindInMessage",    GTK_STOCK_FIND,    N_("qgn_toolb_gene_find"), NULL, NULL, G_CALLBACK (modest_msg_view_window_toggle_find_toolbar), FALSE },
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

	GtkTreeModel *header_model;
	GtkTreeIter   iter;
};

#define MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_MSG_VIEW_WINDOW, \
                                                    ModestMsgViewWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

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
	}
	return my_type;
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

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewWindowPrivate));
}

static void
modest_msg_view_window_init (ModestMsgViewWindow *obj)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);

	priv->msg_view      = NULL;
	priv->header_model  = NULL;
}

static void
save_settings (ModestMsgViewWindow *self)
{
	modest_widget_memory_save (modest_runtime_get_conf (),
				    G_OBJECT(self), "modest-msg-view-window");
}


static void
restore_settings (ModestMsgViewWindow *self)
{
	modest_widget_memory_restore (modest_runtime_get_conf (),
				      G_OBJECT(self), "modest-msg-view-window");
}



static GtkWidget *
menubar_to_menu (GtkUIManager *ui_manager)
{
	GtkWidget *main_menu;
	GtkWidget *menubar;
	GList *iter;

	/* Create new main menu */
	main_menu = gtk_menu_new();

	/* Get the menubar from the UI manager */
	menubar = gtk_ui_manager_get_widget (ui_manager, "/MenuBar");

	iter = gtk_container_get_children (GTK_CONTAINER (menubar));
	while (iter) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent(menu, main_menu);

		iter = g_list_next (iter);
	}
	return main_menu;
}

static void
init_window (ModestMsgViewWindow *obj, TnyMsg *msg)
{
	GtkWidget *main_vbox;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_view = modest_msg_view_new (msg);
	modest_msg_view_set_shadow_type (MODEST_MSG_VIEW (priv->msg_view), GTK_SHADOW_NONE);
	main_vbox = gtk_vbox_new  (FALSE, 6);

	/* Menubar */
	parent_priv->menubar = menubar_to_menu (parent_priv->ui_manager);
	gtk_widget_show_all (GTK_WIDGET(parent_priv->menubar));
	hildon_window_set_menu    (HILDON_WINDOW(obj), GTK_MENU(parent_priv->menubar));

	priv->main_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->main_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->main_scroll), GTK_SHADOW_NONE);

	gtk_container_add (GTK_CONTAINER (priv->main_scroll), priv->msg_view);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_scroll, TRUE, TRUE, 0);
	gtk_container_add   (GTK_CONTAINER(obj), main_vbox);

	priv->find_toolbar = hildon_find_toolbar_new (NULL);
	gtk_widget_set_no_show_all (priv->find_toolbar, TRUE);
	g_signal_connect (G_OBJECT (priv->find_toolbar), "close", G_CALLBACK (modest_msg_view_window_find_toolbar_close), obj);
	g_signal_connect (G_OBJECT (priv->find_toolbar), "search", G_CALLBACK (modest_msg_view_window_find_toolbar_search), obj);
	
	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	gtk_box_pack_end (GTK_BOX (main_vbox), priv->find_toolbar, FALSE, FALSE, 0);
}	


static void
modest_msg_view_window_finalize (GObject *obj)
{
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (obj);
	if (priv->header_model != NULL) {
		g_object_unref (priv->header_model);
		priv->header_model = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, ModestMsgViewWindow *self)
{
	save_settings (self);
	return FALSE;
}

ModestWindow *
modest_msg_view_window_new_with_header_model (TnyMsg *msg, const gchar *account_name,
					      GtkTreeModel *model, GtkTreeIter iter)
{
	ModestMsgViewWindow *window = NULL;
	ModestMsgViewWindowPrivate *priv = NULL;

	window = MODEST_MSG_VIEW_WINDOW(modest_msg_view_window_new (msg, account_name));
	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), NULL);

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	g_object_ref (model);
	priv->header_model = model;
	priv->iter = iter;

	modest_msg_view_window_update_priority (window);

	modest_msg_view_window_update_dimmed (window);

	return MODEST_WINDOW(window);
}


ModestWindow *
modest_msg_view_window_new (TnyMsg *msg, const gchar *account_name)
{
	GObject *obj;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkActionGroup *action_group;
	GError *error = NULL;
	GdkPixbuf *window_icon = NULL;

	g_return_val_if_fail (msg, NULL);
	
	obj = g_object_new(MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
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
	
	/* Init window */
	init_window (MODEST_MSG_VIEW_WINDOW(obj), msg);
	restore_settings (MODEST_MSG_VIEW_WINDOW(obj));
	
	g_signal_connect (G_OBJECT(obj), "delete-event", G_CALLBACK(on_delete_event), obj);

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

	modest_window_set_active_account (MODEST_WINDOW(obj), account_name);

	priv->last_search = NULL;

	modest_msg_view_window_update_dimmed (MODEST_MSG_VIEW_WINDOW (obj));

	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_MSG_VIEW_ICON);
	gtk_window_set_icon (GTK_WINDOW (obj), window_icon);

	gtk_widget_grab_focus (priv->msg_view);

	return MODEST_WINDOW(obj);
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
	TnyMsg *msg;
	TnyHeader *header;
	const gchar *retval = NULL;

	msg = modest_msg_view_window_get_message (self);

	if (!msg)
		return NULL;

	header = tny_msg_get_header (msg);
	if (header) {
		retval = tny_header_get_uid (header);
		g_object_unref (header);
	}
	return retval;
}

static void 
modest_msg_view_window_toggle_find_toolbar (GtkToggleAction *toggle,
					    gpointer data)
{
	ModestMsgViewWindow *window = MODEST_MSG_VIEW_WINDOW (data);
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (gtk_toggle_action_get_active (toggle)) {
		gtk_widget_show (priv->find_toolbar);
	} else {
		gtk_widget_hide (priv->find_toolbar);
	}

	
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

	g_object_get (G_OBJECT (widget), "prefix", &current_search, NULL);

	if ((current_search == NULL) && (strcmp (current_search, "") == 0)) {
		g_free (current_search);
		return;
	}

	if ((priv->last_search == NULL) || (strcmp (priv->last_search, current_search) != 0)) {
		gboolean result;
		g_free (priv->last_search);
		priv->last_search = g_strdup (current_search);
		result = modest_msg_view_search (MODEST_MSG_VIEW (priv->msg_view),
						 priv->last_search);
	} else {
		modest_msg_view_search_next (MODEST_MSG_VIEW (priv->msg_view));
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
		hildon_banner_show_information (NULL, NULL, _("mcen_ib_max_zoom_level"));
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
				hildon_banner_show_information (NULL, NULL, _("mcen_ib_min_zoom_level"));
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

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	g_signal_emit_by_name (G_OBJECT (priv->main_scroll), "scroll-child", GTK_SCROLL_STEP_UP, FALSE);
}

static void
modest_msg_view_window_scroll_down (ModestWindow *window)
{
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);
	g_signal_emit_by_name (G_OBJECT (priv->main_scroll), "scroll-child", GTK_SCROLL_STEP_DOWN, FALSE);
}

static gboolean 
modest_msg_view_window_is_last_message (ModestMsgViewWindow *window)
{
	GtkTreePath *path;
	ModestMsgViewWindowPrivate *priv;
	GtkTreeIter tmp_iter;
	gboolean has_next = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), TRUE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->header_model) {
		path = gtk_tree_model_get_path (priv->header_model, &priv->iter);
		if (!path)
			return TRUE;
		while (!has_next) {
			TnyHeader *header;
			gtk_tree_path_next (path);
			if (!gtk_tree_model_get_iter (priv->header_model, &tmp_iter, path))
				break;
			gtk_tree_model_get (priv->header_model, &tmp_iter, TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header, -1);
			if (!(tny_header_get_flags(header)&TNY_HEADER_FLAG_DELETED)) {
				has_next = TRUE;
				break;
			}
			
		}
		gtk_tree_path_free (path);
		return !has_next;
	} else {
		return TRUE;
	}
	
}

static gboolean 
modest_msg_view_window_is_first_message (ModestMsgViewWindow *window)
{
	GtkTreePath *path;
	ModestMsgViewWindowPrivate *priv;
	gboolean result;
	GtkTreeIter tmp_iter;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), TRUE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->header_model) {
		gchar * path_string;
		path = gtk_tree_model_get_path (priv->header_model, &priv->iter);
		if (!path)
			return TRUE;

		path_string = gtk_tree_path_to_string (path);
		result = (strcmp (path_string, "0")==0);
		if (result) {
			g_free (path_string);
			gtk_tree_path_free (path);
			return result;
		}

		while (result) {
			TnyHeader *header;

			gtk_tree_path_prev (path);
			
			if (!gtk_tree_model_get_iter (priv->header_model, &tmp_iter, path))
				break;
			gtk_tree_model_get (priv->header_model, &tmp_iter, TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header, -1);
			if (!(tny_header_get_flags(header)&TNY_HEADER_FLAG_DELETED)) {
				result = FALSE;
				break;
			}

			path_string = gtk_tree_path_to_string (path);
			if (strcmp(path_string, "0")==0) {
				g_free (path_string);
				break;
			}
			g_free (path_string);
		}
		gtk_tree_path_free (path);
		return result;
	} else {
		return TRUE;
	}
	
}

gboolean        
modest_msg_view_window_select_next_message (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	GtkTreeIter tmp_iter;
	gboolean has_next = FALSE;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->header_model) {
		tmp_iter = priv->iter;
		while (gtk_tree_model_iter_next (priv->header_model, &tmp_iter)) {
			TnyHeader *header;
			TnyFolder *folder;
			TnyMsg *msg;

			priv->iter = tmp_iter;
			gtk_tree_model_get (priv->header_model, &(priv->iter), TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header, -1);
			if (!header)
				break;
			if (tny_header_get_flags (header) & TNY_HEADER_FLAG_DELETED)
				continue;

			folder = tny_header_get_folder (header);
			if (!folder)
				break;
			msg = tny_folder_get_msg (folder, header, NULL);
			if (!msg) {
				g_object_unref (folder);
				break;
			}
			has_next = TRUE;
			modest_msg_view_set_message (MODEST_MSG_VIEW (priv->msg_view), msg);
			modest_msg_view_window_update_dimmed (window);
			modest_msg_view_window_update_priority (window);
			gtk_widget_grab_focus (priv->msg_view);

			g_object_unref (msg);
			break;
		}

		return has_next;
	} else {
		return FALSE;
	}
}

gboolean        
modest_msg_view_window_select_previous_message (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (window), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->header_model) {
		GtkTreePath *path;
		gboolean has_prev = FALSE;

		path = gtk_tree_model_get_path (priv->header_model, &(priv->iter));
		while (gtk_tree_path_prev (path)) {
			TnyHeader *header;
			TnyFolder *folder;
			TnyMsg *msg;

			gtk_tree_model_get_iter (priv->header_model, &(priv->iter), path);
			gtk_tree_model_get (priv->header_model, &(priv->iter), TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header, -1);
			if (!header)
				break;
			if (tny_header_get_flags (header) & TNY_HEADER_FLAG_DELETED)
				continue;
			folder = tny_header_get_folder (header);
			if (!folder)
				break;
			msg = tny_folder_get_msg (folder, header, NULL);
			if (!msg) {
				g_object_unref (folder);
				break;
			}
			has_prev = TRUE;
			modest_msg_view_set_message (MODEST_MSG_VIEW (priv->msg_view), msg);
			modest_msg_view_window_update_dimmed (window);
			modest_msg_view_window_update_priority (window);
			gtk_widget_grab_focus (priv->msg_view);

			g_object_unref (msg);
			break;
		}
		gtk_tree_path_free (path);
		return has_prev;
	} else {
		return FALSE;
	}
}

static TnyFolderType
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
	}

	return folder_type;
}

static void
modest_msg_view_window_update_dimmed (ModestMsgViewWindow *window)
{
	ModestWindowPrivate *parent_priv;
	GtkAction *widget;
	gboolean is_first, is_last;
	TnyFolderType folder_type;
	gboolean is_not_sent;

	parent_priv = MODEST_WINDOW_GET_PRIVATE (window);

	is_first = modest_msg_view_window_is_first_message (window);
	is_last = modest_msg_view_window_is_last_message (window);

	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarMessageBack");
	gtk_action_set_sensitive (widget, !is_first);
	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewPreviousMessageMenu");
	gtk_action_set_sensitive (widget, !is_first);
		
	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarMessageForward");
	gtk_action_set_sensitive (widget, !is_last);
	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewNextMessageMenu");
	gtk_action_set_sensitive (widget, !is_last);

	folder_type = modest_msg_view_window_get_folder_type (MODEST_MSG_VIEW_WINDOW (window));
	is_not_sent = ((folder_type == TNY_FOLDER_TYPE_DRAFTS)||(folder_type == TNY_FOLDER_TYPE_OUTBOX));
	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/ToolBar/ToolbarMessageReply");
	gtk_action_set_sensitive (widget, !is_not_sent);
	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/MessageMenu/MessageReplyMenu");
	gtk_action_set_sensitive (widget, !is_not_sent);
	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/MessageMenu/MessageReplyAllMenu");
	gtk_action_set_sensitive (widget, !is_not_sent);
	widget = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/MessageMenu/MessageForwardMenu");
	gtk_action_set_sensitive (widget, !is_not_sent);
		
}

static void
modest_msg_view_window_update_priority (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	TnyHeaderFlags flags = 0;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	if (priv->header_model) {
		TnyHeader *header;

		gtk_tree_model_get (priv->header_model, &(priv->iter), TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
				    &header, -1);
		flags = tny_header_get_flags (header);
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

void
modest_msg_view_window_toggle_fullscreen (ModestMsgViewWindow *window)
{
		ModestWindowPrivate *parent_priv;
		GtkAction *fs_toggle_action;
		parent_priv = MODEST_WINDOW_GET_PRIVATE (window);
		
		fs_toggle_action = gtk_ui_manager_get_action (parent_priv->ui_manager, "/MenuBar/ViewMenu/ViewToggleFullscreenMenu");
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fs_toggle_action),
					      !gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (fs_toggle_action)));
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
						  "/ToolbarReplyCSM");
		gtk_widget_tap_and_hold_setup (GTK_WIDGET (reply_button), menu, NULL, 0);
	}


	if (show_toolbar)
		gtk_widget_show (GTK_WIDGET (parent_priv->toolbar));
	else
		gtk_widget_hide (GTK_WIDGET (parent_priv->toolbar));
}

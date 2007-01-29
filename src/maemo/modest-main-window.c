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
#include <modest-runtime.h>
#include <widgets/modest-main-window.h>
#include <widgets/modest-edit-msg-window.h>
#include <modest-widget-factory.h>
#include "modest-widget-memory.h"
#include "modest-window-priv.h"
#include "modest-icon-factory.h"
#include "modest-ui.h"
#include "modest-main-window-ui.h"
#include "modest-account-view-window.h"
#include "modest-account-mgr.h"
#include "modest-conf.h"

#include "modest-tny-platform-factory.h"
#include "modest-tny-msg-actions.h"
#include "modest-mail-operation.h"
#include "modest-icon-names.h"

/* 'private'/'protected' functions */
static void modest_main_window_class_init    (ModestMainWindowClass *klass);
static void modest_main_window_init          (ModestMainWindow *obj);
static void modest_main_window_finalize      (GObject *obj);

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
	
	ModestHeaderView *header_view;
	ModestFolderView *folder_view;
	ModestMsgView    *msg_preview;
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
modest_main_window_init (ModestMainWindow *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	priv->msg_paned    = NULL;
	priv->main_paned   = NULL;	
	priv->header_view  = NULL;
	priv->folder_view  = NULL;
	priv->msg_preview  = NULL;	
}

static void
modest_main_window_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static ModestHeaderView*
header_view_new (ModestMainWindow *self)
{
	ModestHeaderView *header_view;
	
	header_view = modest_widget_factory_get_header_view
		(modest_runtime_get_widget_factory());
	modest_header_view_set_style (header_view, 0); /* don't show headers */
							     
	return header_view;
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

static GtkWidget*
get_toolbar (ModestMainWindow *self)
{
	GtkWidget   *progress_bar,
		    *toolbar, *progress_box, *progress_alignment;
	GtkToolItem *progress_item;
	ModestWindowPrivate *parent_priv;
	ModestWidgetFactory *widget_factory;
	GtkWidget   *stop_icon;
	
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	widget_factory = modest_runtime_get_widget_factory();
	
	/* Toolbar */
	progress_bar  = modest_widget_factory_get_progress_bar(widget_factory);
	toolbar       = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");

	gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress_bar), "Connecting...");

	progress_box        = gtk_hbox_new (FALSE, HILDON_MARGIN_DEFAULT);
	progress_alignment  = gtk_alignment_new (0.5, 0.5, 1, 0);
	
	gtk_container_add  (GTK_CONTAINER(progress_alignment), progress_bar);
	gtk_box_pack_start (GTK_BOX(progress_box), progress_alignment, TRUE, TRUE, 0);
	
	progress_item  = gtk_tool_item_new ();
	gtk_container_add (GTK_CONTAINER(progress_item), progress_box);
	gtk_tool_item_set_homogeneous (progress_item, FALSE);
	gtk_tool_item_set_expand(progress_item, TRUE);
	
	stop_icon = gtk_image_new_from_icon_name("qgn_toolb_gene_stop", GTK_ICON_SIZE_BUTTON);

	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), gtk_tool_button_new(stop_icon, NULL),
			    gtk_toolbar_get_n_items(GTK_TOOLBAR(toolbar)));

	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), progress_item,
			    gtk_toolbar_get_n_items(GTK_TOOLBAR(toolbar)));
	gtk_widget_show_all (toolbar);
	return toolbar;
}
	
ModestWindow*
modest_main_window_new (void)
{
	GObject *obj;
	ModestMainWindowPrivate *priv;
	ModestWidgetFactory *widget_factory;
	ModestWindowPrivate *parent_priv;
	GtkWidget *main_vbox;
	GtkWidget *header_win, *folder_win;
	GtkActionGroup *action_group;
	GError *error = NULL;

	obj  = g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	widget_factory = modest_runtime_get_widget_factory ();
	
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
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager, MODEST_UIDIR "modest-ui.xml", &error);
	if (error != NULL) {
		g_warning ("Could not merge modest-ui.xml: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));

	/* add the toolbar */
	parent_priv->toolbar = get_toolbar(MODEST_MAIN_WINDOW(obj));
	hildon_window_add_toolbar (HILDON_WINDOW (obj), GTK_TOOLBAR (parent_priv->toolbar));

	/* Menubar */
	parent_priv->menubar = menubar_to_menu (parent_priv->ui_manager);
	hildon_window_set_menu (HILDON_WINDOW (obj), GTK_MENU (parent_priv->menubar));


	/* widgets from factory */
	priv->folder_view = modest_widget_factory_get_folder_view (widget_factory);
	priv->header_view = header_view_new (MODEST_MAIN_WINDOW(obj));
	priv->msg_preview = modest_widget_factory_get_msg_preview (widget_factory);
	
	folder_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->folder_view),
						 FALSE);
	header_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->header_view),
						 FALSE);			   

	/* paned */
	priv->main_paned = gtk_hpaned_new ();
	gtk_paned_add1 (GTK_PANED(priv->main_paned), folder_win);
	gtk_paned_add2 (GTK_PANED(priv->main_paned), header_win);
	gtk_widget_show (GTK_WIDGET(priv->header_view));
	
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(priv->header_view));

	/* putting it all together... */
	main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_paned, TRUE, TRUE,0);

	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
	restore_sizes (MODEST_MAIN_WINDOW(obj));	

	gtk_window_set_title (GTK_WINDOW(obj), _("Modest"));
	gtk_window_set_icon  (GTK_WINDOW(obj),
			      modest_icon_factory_get_icon (MODEST_APP_ICON));
	
	gtk_widget_show_all (main_vbox);

	g_signal_connect (G_OBJECT(obj), "delete-event",
			  G_CALLBACK(on_delete_event), obj);
	
	return (ModestWindow *) obj;
}

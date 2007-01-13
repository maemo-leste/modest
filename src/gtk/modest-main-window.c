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

#include "modest-main-window.h"
#include "modest-widget-factory.h"
#include "modest-widget-memory.h"
#include "modest-icon-factory.h"
#include "modest-ui.h"
#include "modest-main-window-ui.h"
#include "modest-account-view-window.h"
#include "modest-account-mgr.h"
#include "modest-conf.h"
#include "modest-edit-msg-window.h"
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

	GtkUIManager *ui_manager;
	ModestWidgetFactory *widget_factory;
	TnyPlatformFactory *factory;
	TnyAccountStore *account_store;
	
	GtkWidget *toolbar;
	GtkWidget *menubar;

	GtkWidget *folder_paned;
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
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
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

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_main_window_init (ModestMainWindow *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);
	
	priv->factory = modest_tny_platform_factory_get_instance ();
	priv->widget_factory = NULL;
	priv->ui_manager = NULL;
	priv->account_store = NULL;
}

static void
modest_main_window_finalize (GObject *obj)
{
	ModestMainWindowPrivate *priv;	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);
	if (priv->widget_factory) {
		g_object_unref (G_OBJECT(priv->widget_factory));
		priv->widget_factory = NULL;
	}
	if (priv->ui_manager) {
		g_object_unref (G_OBJECT(priv->ui_manager));
		priv->ui_manager = NULL;
	}
	if (priv->account_store) {
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static ModestHeaderView*
header_view_new (ModestMainWindow *self)
{
	int i;
	GList *columns = NULL;
	ModestHeaderView *header_view;
	ModestMainWindowPrivate *priv;
	ModestHeaderViewColumn cols[] = {
		MODEST_HEADER_VIEW_COLUMN_MSGTYPE,
		MODEST_HEADER_VIEW_COLUMN_ATTACH,
/* 		MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER, */
		MODEST_HEADER_VIEW_COLUMN_FROM,
 		MODEST_HEADER_VIEW_COLUMN_SUBJECT,
		MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE
	};
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	for (i = 0 ; i != sizeof(cols) / sizeof(ModestHeaderViewColumn); ++i)
		columns = g_list_append (columns, GINT_TO_POINTER(cols[i]));

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	modest_header_view_set_columns (header_view, columns);
	modest_header_view_set_style (header_view,
				      MODEST_HEADER_VIEW_STYLE_SHOW_HEADERS);
	g_list_free (columns);
	return header_view;
}


static void
restore_sizes (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_modest_conf_instance (priv->factory);

	modest_widget_memory_restore (conf, G_OBJECT(priv->folder_paned),
				      "modest-folder-paned");
	modest_widget_memory_restore (conf, G_OBJECT(priv->msg_paned),
				      "modest-msg-paned");
	modest_widget_memory_restore (conf, G_OBJECT(priv->main_paned),
				      "modest-main-paned");
	modest_widget_memory_restore (conf, G_OBJECT(priv->header_view),
				      "header-view");
	modest_widget_memory_restore (conf,G_OBJECT(self),
				      "modest-main-window");
}


static void
save_sizes (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;
	ModestConf *conf;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_modest_conf_instance (priv->factory);
	
	modest_widget_memory_save (conf,G_OBJECT(self), "modest-main-window");
	modest_widget_memory_save (conf, G_OBJECT(priv->folder_paned),
				   "modest-folder-paned");
	modest_widget_memory_save (conf, G_OBJECT(priv->msg_paned),
				   "modest-msg-paned");
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


ModestWindow *
modest_main_window_new (ModestWidgetFactory *widget_factory,
			TnyAccountStore *account_store)
{
	GObject *obj;
	ModestMainWindowPrivate *priv;
	GtkWidget *main_vbox;
	GtkWidget *status_hbox;
	GtkWidget *header_win, *folder_win;
	GtkActionGroup *action_group;
	GError *error = NULL;
	
	g_return_val_if_fail (widget_factory, NULL);

	obj  = g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	priv->widget_factory = g_object_ref (widget_factory);
	priv->account_store  = g_object_ref (account_store);

	/* ***************** */
	priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMainWindowActions");

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
				      obj);

	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);
	g_object_unref (action_group);

	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (priv->ui_manager, MODEST_UIDIR "modest-ui.xml", &error);
	if (error != NULL) {
		g_warning ("Could not merge modest-ui.xml: %s", error->message);
		g_error_free (error);
		error = NULL;
	}
	/* *************** */
/* 	priv->ui_manager = g_object_ref (ui_manager); */

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (priv->ui_manager));


	/* Toolbar / Menubar */
	priv->toolbar = gtk_ui_manager_get_widget (priv->ui_manager, "/ToolBar");
	priv->menubar = gtk_ui_manager_get_widget (priv->ui_manager, "/MenuBar");

	gtk_toolbar_set_tooltips (GTK_TOOLBAR (priv->toolbar), TRUE);

	/* widgets from factory */
	priv->folder_view = modest_widget_factory_get_folder_view (widget_factory);
	priv->header_view = header_view_new (MODEST_MAIN_WINDOW(obj));
	priv->msg_preview = modest_widget_factory_get_msg_preview (widget_factory);
	
	folder_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->folder_view),
						 FALSE);
	header_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->header_view),
						 FALSE);			   

	/* paned */
	priv->folder_paned = gtk_vpaned_new ();
	priv->msg_paned = gtk_vpaned_new ();
	priv->main_paned = gtk_hpaned_new ();
	gtk_paned_add1 (GTK_PANED(priv->main_paned), folder_win);
	gtk_paned_add2 (GTK_PANED(priv->main_paned), priv->msg_paned);
	gtk_paned_add1 (GTK_PANED(priv->msg_paned), header_win);
	gtk_paned_add2 (GTK_PANED(priv->msg_paned), GTK_WIDGET(priv->msg_preview));

	gtk_widget_show (GTK_WIDGET(priv->header_view));
	
	/* status bar / progress */
	status_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			    modest_widget_factory_get_folder_info_label (widget_factory),
			    FALSE,FALSE, 6);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			    modest_widget_factory_get_status_bar(widget_factory),
			    TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			    modest_widget_factory_get_progress_bar(widget_factory),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			  modest_widget_factory_get_online_toggle(widget_factory),
			  FALSE, FALSE, 0);

	/* putting it all together... */
	main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_paned, TRUE, TRUE,0);
	gtk_box_pack_start (GTK_BOX(main_vbox), status_hbox, FALSE, FALSE, 0);
	
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

ModestWidgetFactory *
modest_main_window_get_widget_factory (ModestMainWindow *main_window)
{
	ModestMainWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (main_window), NULL);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (main_window);

	return g_object_ref (priv->widget_factory);
}

TnyAccountStore * 
modest_main_window_get_account_store (ModestMainWindow *main_window)
{
	ModestMainWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_MAIN_WINDOW (main_window), NULL);

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (main_window);

	return g_object_ref (priv->account_store);
}

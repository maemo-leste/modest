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
#include <modest-tny-msg.h>
#include <modest-msg-view-window.h>
#include <modest-main-window-ui.h>
#include <modest-widget-memory.h>
#include <modest-runtime.h>
#include <modest-window-priv.h>

static void  modest_msg_view_window_class_init   (ModestMsgViewWindowClass *klass);
static void  modest_msg_view_window_init         (ModestMsgViewWindow *obj);
static void  modest_msg_view_window_finalize     (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMsgViewWindowPrivate ModestMsgViewWindowPrivate;
struct _ModestMsgViewWindowPrivate {

	GtkWidget   *toolbar;
	GtkWidget   *menubar;
	GtkWidget   *msg_view;
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
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_view_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewWindowPrivate));
}

static void
modest_msg_view_window_init (ModestMsgViewWindow *obj)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);

	priv->toolbar       = NULL;
	priv->menubar       = NULL;
	priv->msg_view      = NULL;
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
	main_vbox = gtk_vbox_new  (FALSE, 6);

	/* Toolbar / Menubar */
	
	parent_priv->menubar = menubar_to_menu (parent_priv->ui_manager);
	gtk_widget_show_all (GTK_WIDGET(parent_priv->menubar));
	hildon_window_set_menu    (HILDON_WINDOW(obj), GTK_MENU(parent_priv->menubar));

	parent_priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	gtk_widget_show_all (GTK_WIDGET(parent_priv->toolbar));
	hildon_window_add_toolbar (HILDON_WINDOW(obj), GTK_TOOLBAR(parent_priv->toolbar));

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->msg_view, TRUE, TRUE, 6);
	gtk_container_add   (GTK_CONTAINER(obj), main_vbox);
	
	gtk_widget_show_all (GTK_WIDGET(main_vbox));
}	


static void
modest_msg_view_window_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, ModestMsgViewWindow *self)
{
	save_settings (self);
	return FALSE;
}



ModestWindow *
modest_msg_view_window_new (TnyMsg *msg)
{
	GObject *obj;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkActionGroup *action_group;
	GError *error = NULL;

	g_return_val_if_fail (msg, NULL);
	
	obj = g_object_new(MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);
	
	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMsgViewWindowActions");

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
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
	
	gtk_window_set_title (GTK_WINDOW(obj), "Modest");
	gtk_window_set_icon_from_file (GTK_WINDOW(obj), MODEST_APP_ICON, NULL);

	g_signal_connect (G_OBJECT(obj), "delete-event", G_CALLBACK(on_delete_event), obj);

	return (ModestWindow *) (obj);
}

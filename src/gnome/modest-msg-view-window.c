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
#include "modest-icon-names.h"
#include "modest-ui-actions.h"
#include <modest-widget-memory.h>
#include <modest-runtime.h>

#include <widgets/modest-msg-view-window.h>
#include <widgets/modest-window-priv.h>
#include "widgets/modest-msg-view.h"


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

/* Action entries */
static const GtkActionEntry modest_action_entries [] = {

	/* Toplevel menus */
	{ "Edit", NULL, N_("_Edit") },
	{ "Actions", NULL, N_("_Actions") },
	{ "Help", NULL, N_("_Help") },
	{ "Email", NULL, N_("E_mail") },

	/* EDIT */
	{ "EditUndo",        GTK_STOCK_UNDO,   N_("_Undo"), "<CTRL>Z",        N_("Undo last action"),  NULL },
	{ "EditRedo",        GTK_STOCK_REDO,   N_("_Redo"), "<shift><CTRL>Z", N_("Redo previous action"),  NULL },
	{ "Cut",         GTK_STOCK_CUT,    N_("Cut"),   "<CTRL>X",        N_("_Cut"), G_CALLBACK (modest_ui_actions_on_cut)   },
	{ "Copy",        GTK_STOCK_COPY,   N_("Copy"),  "<CTRL>C",        N_("Copy"), G_CALLBACK (modest_ui_actions_on_copy) },
	{ "Paste",       GTK_STOCK_PASTE,  N_("Paste"), "<CTRL>V",        N_("Paste"), G_CALLBACK (modest_ui_actions_on_paste) },
	{ "EditDelete",      GTK_STOCK_DELETE, N_("_Delete"),      "<CTRL>Q",	      N_("Delete"), NULL },
	{ "SelectAll",   NULL, 	       N_("Select all"),   "<CTRL>A",	      N_("Select all"), G_CALLBACK (modest_ui_actions_on_select_all) },
	{ "EditDeselectAll", NULL,             N_("Deselect all"), "<Shift><CTRL>A",  N_("Deselect all"), NULL },

	/* ACTIONS */
	{ "ActionsNewMessage",  MODEST_STOCK_NEW_MAIL, N_("_New"), "<CTRL>N", N_("Compose new message"), G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "ActionsReply",       MODEST_STOCK_REPLY, N_("_Reply"),         NULL, N_("Reply to a message"), G_CALLBACK (modest_ui_actions_on_reply) },
	{ "ActionsReplyAll",    MODEST_STOCK_REPLY_ALL, N_("Reply to all"),   NULL, N_("Reply to all"), G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "ActionsForward",     MODEST_STOCK_FORWARD, N_("_Forward"),       NULL, N_("Forward a message"), G_CALLBACK (modest_ui_actions_on_forward) },
	{ "ActionsBounce",      NULL, N_("_Bounce"),        NULL, N_("Bounce a message"),          NULL },
	{ "ActionsSendReceive", GTK_STOCK_REFRESH, N_("Send/Receive"),   NULL, N_("Send and receive messages"), NULL },
	{ "ActionsDelete",      MODEST_STOCK_DELETE, N_("Delete message"), NULL, N_("Delete messages"), G_CALLBACK (modest_ui_actions_on_delete) },

	/* HELP */
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("About"), NULL, N_("About Modest"), G_CALLBACK (modest_ui_actions_on_about) },
};


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


static void
init_window (ModestMsgViewWindow *obj, TnyMsg *msg)
{
	GtkWidget *main_vbox, *scrolled_window;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_view = modest_msg_view_new (msg);
	main_vbox = gtk_vbox_new  (FALSE, 6);
	
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, FALSE, 0);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), 
			   priv->msg_view);
	gtk_box_pack_start (GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 6);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	gtk_container_add   (GTK_CONTAINER(obj), main_vbox);
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
modest_msg_view_window_new (TnyMsg *msg, const gchar *account)
{
	GObject *obj;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkActionGroup *action_group;
	GError *error = NULL;
	TnyHeader *header = NULL;
	const gchar *subject = NULL;

	g_return_val_if_fail (msg, NULL);

	obj = g_object_new(MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	modest_window_set_active_account (MODEST_WINDOW(obj), account);
	
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
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager,
					 MODEST_UIDIR "modest-msg-view-window-ui.xml",
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

	/* Toolbar / Menubar */
	priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	priv->menubar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar");

	gtk_toolbar_set_tooltips (GTK_TOOLBAR (priv->toolbar), TRUE);

	/* Init window */
	init_window (MODEST_MSG_VIEW_WINDOW(obj), msg);
	restore_settings (MODEST_MSG_VIEW_WINDOW(obj));

	header = tny_msg_get_header (msg);
	if (header)
		subject = tny_header_get_subject (header);
	
	if (subject != NULL)
		gtk_window_set_title (GTK_WINDOW (obj), subject);
	else
		gtk_window_set_title (GTK_WINDOW(obj), "Modest");

	if (header)
		g_object_unref (header);

	gtk_window_set_icon_from_file (GTK_WINDOW(obj), MODEST_APP_ICON, NULL);

	g_signal_connect (G_OBJECT(obj), "delete-event", G_CALLBACK(on_delete_event), obj);

	return MODEST_WINDOW(obj);
}


TnyMsg*
modest_msg_view_window_get_message (ModestMsgViewWindow *self)
{
	GtkWidget *msg_view;	
	g_return_val_if_fail (self, NULL);

	msg_view = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self)->msg_view;

	return modest_msg_view_get_message (MODEST_MSG_VIEW(msg_view));
}

ModestWindow*   
modest_msg_view_window_new_with_header_model (TnyMsg *msg, 
					      const gchar *account, 
					      GtkTreeModel *model, 
					      GtkTreeIter iter)
{
	/* Currently we simply redirect to new constructor. It should store a
	   reference to the header list model, to enable next/prev message
	   actions */
	g_message ("partially implemented %s", __FUNCTION__);

	return modest_msg_view_window_new (msg, account);
}


gboolean
modest_msg_view_window_select_next_message (ModestMsgViewWindow *window)
{
	g_message ("not implemented %s", __FUNCTION__);
	return FALSE;
}

gboolean
modest_msg_view_window_select_previous_message (ModestMsgViewWindow *window)
{
	g_message ("not implemented %s", __FUNCTION__);
	return FALSE;
}

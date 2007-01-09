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

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <glib/gi18n.h>
#include <gtk/gtkuimanager.h>
#include <tny-error.h>
#include <tny-simple-list.h>
#include "modest-ui.h"
#include "modest-tny-account-store.h"
#include "modest-account-mgr.h"
#include "modest-widget-factory.h"
#include "modest-tny-platform-factory.h"

#include "modest-edit-msg-window.h"
#include "modest-account-view-window.h"
#include "modest-icon-names.h"
#include "modest-main-window.h"

typedef struct _ModestUIPrivate ModestUIPrivate;
struct _ModestUIPrivate {
	ModestWidgetFactory  *widget_factory;	
	TnyAccountStore      *account_store;
	GtkWidget            *main_window;
};

#define MODEST_UI_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MODEST_TYPE_UI, \
                                       ModestUIPrivate))

typedef struct _GetMsgAsyncHelper {
	ModestUIPrivate *priv;
	TnyIterator *iter;
	GFunc func;
	gpointer user_data;
} GetMsgAsyncHelper;

typedef enum _ReplyForwardAction {
	ACTION_REPLY,
	ACTION_REPLY_TO_ALL,
	ACTION_FORWARD
} ReplyForwardAction;

typedef struct _ReplyForwardHelper {
	guint reply_forward_type;
	ReplyForwardAction action;
	gchar *from;
} ReplyForwardHelper;

/* globals */
static GObjectClass *parent_class = NULL;

/* 'private'/'protected' functions */
static void   modest_ui_class_init     (ModestUIClass *klass);
static void   modest_ui_init           (ModestUI *obj);
static void   modest_ui_finalize       (GObject *obj);

static void   register_stock_icons        ();
static void   connect_signals             (ModestUI *self);
static void   connect_main_window_signals (ModestUI *self);
static GtkUIManager *create_ui_manager ();

static void   reply_forward_func (gpointer data, gpointer user_data);
static void   read_msg_func      (gpointer data, gpointer user_data);
static void   get_msg_cb         (TnyFolder *folder, 
				  TnyMsg *msg, 
				  GError **err, 
				  gpointer user_data);

static void   reply_forward      (GtkWidget *widget,
				  ReplyForwardAction action,
				  ModestUIPrivate *priv);

/* Menu & toolbar actions */
static void     modest_ui_actions_on_about         (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_delete        (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_quit          (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_accounts      (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_new_msg       (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_reply         (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_forward       (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_reply_all     (GtkWidget *widget, ModestUIPrivate *priv);

static void     modest_ui_actions_on_next          (GtkWidget *widget, ModestUIPrivate *priv);

/* Widget actions */
static void     modest_ui_actions_on_header_selected          (ModestHeaderView *folder_view, 
							       TnyHeader *header,
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
							       TnyFolder *folder, 
							       gboolean selected,
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_password_requested       (ModestTnyAccountStore *account_store, 
							       const gchar* account_name,
							       gchar **password, 
							       gboolean *cancel, 
							       gboolean *remember, 
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_connection_changed       (TnyDevice *device, 
							       gboolean online,
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_online_toggle_toggled    (GtkToggleButton *toggle,
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_item_not_found           (ModestHeaderView *header_view,
							       ModestItemType type,
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_header_status_update     (ModestHeaderView *header_view, 
							       const gchar *msg,
							       gint num, 
							       gint total, 
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_msg_link_hover           (ModestMsgView *msgview, 
							       const gchar* link,
							       ModestUIPrivate *priv);


static void     modest_ui_actions_on_msg_link_clicked         (ModestMsgView *msgview, 
							       const gchar* link,
							       ModestUIPrivate *priv);

static void     modest_ui_actions_on_msg_attachment_clicked   (ModestMsgView *msgview, 
							       int index,
							       ModestUIPrivate *priv);


/* Action entries */
static const GtkActionEntry modest_action_entries [] = {

	/* Toplevel */
	{ "File", NULL, N_("_File") },
	{ "Edit", NULL, N_("_Edit") },
	{ "Actions", NULL, N_("_Actions") },
	{ "Options", NULL, N_("_Options") },
	{ "Help", NULL, N_("_Help") },

	/* FILE menu */
	{ "FileNew",    GTK_STOCK_NEW,     N_("_New"),	   "<CTRL>N", N_("Compose new message"),  G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "FileOpen",   GTK_STOCK_OPEN,    N_("_Open"),	   "<CTRL>O", N_("Open a message"),       NULL },
	{ "FileSave",   GTK_STOCK_SAVE,    N_("_Save"),	   "<CTRL>S", N_("Save a message"),       NULL },
	{ "FileSaveAs", GTK_STOCK_SAVE_AS, N_("Save _As"), NULL,      N_("Save a message as"),    NULL },
	{ "FileQuit",   GTK_STOCK_QUIT,    N_("_Quit"),	   "<CTRL>Q", N_("Exit the application"), G_CALLBACK (modest_ui_actions_on_quit) },

	/* EDIT menu */
	{ "EditUndo",        GTK_STOCK_UNDO,   N_("_Undo"), "<CTRL>Z",        N_("Undo last action"),  NULL },
	{ "EditRedo",        GTK_STOCK_REDO,   N_("_Redo"), "<shift><CTRL>Z", N_("Redo previous action"),  NULL },
	{ "EditCut",         GTK_STOCK_CUT,    N_("Cut"),   "<CTRL>X",        N_("_Cut"), NULL   },
	{ "EditCopy",        GTK_STOCK_COPY,   N_("Copy"),  "<CTRL>C",        N_("Copy"), NULL },
	{ "EditPaste",       GTK_STOCK_PASTE,  N_("Paste"), "<CTRL>V",        N_("Paste"), NULL },
	{ "EditDelete",      GTK_STOCK_DELETE, N_("_Delete"),      "<CTRL>Q",	      N_("Delete"), NULL },
	{ "EditSelectAll",   NULL, 	       N_("Select all"),   "<CTRL>A",	      N_("Select all"), NULL },
	{ "EditDeselectAll", NULL,             N_("Deselect all"), "<Shift><CTRL>A",  N_("Deselect all"), NULL },

	/* ACTIONS menu */
	{ "ActionsNew",         MODEST_STOCK_MAIL_SEND, N_("_New Message"),   NULL, N_("Compose a new message"), G_CALLBACK (modest_ui_actions_on_new_msg) },
	{ "ActionsReply",       MODEST_STOCK_REPLY, N_("_Reply"),         NULL, N_("Reply to a message"), G_CALLBACK (modest_ui_actions_on_reply) },
	{ "ActionsReplyAll",    MODEST_STOCK_REPLY_ALL, N_("Reply to all"),   NULL, N_("Reply to all"), G_CALLBACK (modest_ui_actions_on_reply_all) },
	{ "ActionsForward",     MODEST_STOCK_FORWARD, N_("_Forward"),       NULL, N_("Forward a message"), G_CALLBACK (modest_ui_actions_on_forward) },
	{ "ActionsBounce",      NULL, N_("_Bounce"),        NULL, N_("Bounce a message"),          NULL },
	{ "ActionsSendReceive", MODEST_STOCK_SEND_RECEIVE, N_("Send/Receive"),   NULL, N_("Send and receive messages"), NULL },
	{ "ActionsDelete",      MODEST_STOCK_DELETE, N_("Delete message"), NULL, N_("Delete messages"), G_CALLBACK (modest_ui_actions_on_delete) },

	/* GOTO menu */
	{ "GotoPrevious", MODEST_STOCK_PREV, N_("Previous"), NULL, N_("Go to previous message"), NULL },
	{ "GotoNext",     MODEST_STOCK_NEXT, N_("Next"),     NULL, N_("Go to next message"), G_CALLBACK (modest_ui_actions_on_next) },

	/* OPTIONS menu */
	{ "OptionsAccounts",  NULL, N_("_Accounts"), NULL, N_("Manage accounts"), G_CALLBACK (modest_ui_actions_on_accounts) },
	{ "OptionsContacts",  NULL, N_("_Contacts"), NULL, N_("Manage contacts"), NULL },
	
	/* HELP menu */
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("About"), NULL, N_("About Modest"), G_CALLBACK (modest_ui_actions_on_about) },
};

GType
modest_ui_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestUIClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_ui_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestUI),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_ui_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestUI",
		                                  &my_info, 0);
	}
	return my_type;
}


static void
modest_ui_class_init (ModestUIClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_ui_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestUIPrivate));

}


static void
modest_ui_init (ModestUI *obj)
{
 	ModestUIPrivate *priv;

	priv = MODEST_UI_GET_PRIVATE(obj);

	priv->widget_factory = NULL;
	priv->main_window    = NULL;
	priv->account_store  = NULL;
}


static void
modest_ui_finalize (GObject *obj)
{
	
	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);
	
	if (priv->widget_factory) {
		g_object_unref (G_OBJECT(priv->widget_factory));
		priv->widget_factory = NULL;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestUI*
modest_ui_new (TnyAccountStore *account_store)
{
	GObject *obj;
	ModestUIPrivate *priv;
	TnyPlatformFactory *fact;
	ModestAccountMgr *account_mgr;

	obj  = g_object_new(MODEST_TYPE_UI, NULL);
	priv = MODEST_UI_GET_PRIVATE(obj);

	/* Get the platform-dependent instances */
	fact = modest_tny_platform_factory_get_instance ();
	
	priv->account_store = account_store;

	account_mgr = modest_tny_platform_factory_get_modest_account_mgr_instance (fact);
	if (!account_mgr) {
		g_printerr ("modest: could not create ModestAccountMgr instance\n");
		g_object_unref (obj);
		return NULL;
        }

	priv->widget_factory = modest_widget_factory_new ();
	if (!priv->widget_factory) {
		g_printerr ("modest: could not initialize widget factory\n");
		return NULL;
	}

	/* Connect signals */
	connect_signals (MODEST_UI (obj));
		
	return MODEST_UI(obj);
}

static gboolean
on_main_window_destroy (GtkObject *widget, ModestUI *self)
{
	/* FIXME: check if there any viewer/editing windows opened */
	gtk_main_quit ();
	return FALSE;
}


GtkWidget*
modest_ui_main_window (ModestUI *self)
{
	ModestUIPrivate *priv;
	GtkUIManager *ui_manager;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_UI_GET_PRIVATE(self);

	if (!priv->main_window) {
		TnyDevice *device;

		/* Register our own icons as stock icons in order to
		   use them with the UI manager */
		register_stock_icons ();

		/* Create UI manager */ 
		ui_manager = create_ui_manager (self);

		/* Create main window */
		priv->main_window = modest_main_window_new (priv->widget_factory, ui_manager);
		g_signal_connect (G_OBJECT(priv->main_window), "destroy",
				  G_CALLBACK(on_main_window_destroy), self);

		/* Connect Main Window signals */
		connect_main_window_signals (self);

		g_object_unref (G_OBJECT (ui_manager));
	}
		
	if (!priv->main_window)
		g_printerr ("modest: could not create main window\n");
	
	return priv->main_window;
}

static GtkUIManager *
create_ui_manager (ModestUI *self)
{
	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;
	GError *error = NULL;
	ModestUIPrivate *priv;

	priv = MODEST_UI_GET_PRIVATE(self);

	/* Create UI manager */
	ui_manager = gtk_ui_manager_new();

	/* Create action group */
	action_group = gtk_action_group_new ("ModestMainWindowActions");
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
				      priv);
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
	g_object_unref (action_group);

	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (ui_manager, MODEST_UIDIR "modest-ui.xml", &error);
	if (error != NULL) {
		g_warning ("Could not merge modest-ui.xml: %s", error->message);
		g_error_free (error);
	}
	return ui_manager;
}

/* 
 *  This function registers our custom toolbar icons, so they can be
 *  themed. The idea of this function was taken from the gtk-demo
 */
static void
register_stock_icons ()
{
	static gboolean registered = FALSE;
  
	if (!registered) {
		GdkPixbuf *pixbuf;
		GtkIconFactory *factory;
		gchar *filename;
		gint i;

		static GtkStockItem items[] = {
			{ MODEST_STOCK_MAIL_SEND, "send mail", 0, 0, NULL },
			{ MODEST_STOCK_NEW_MAIL, "new mail", 0, 0, NULL },
			{ MODEST_STOCK_SEND_RECEIVE, "send receive", 0, 0, NULL },
			{ MODEST_STOCK_REPLY, "reply", 0, 0, NULL },
			{ MODEST_STOCK_REPLY_ALL, "reply all", 0, 0, NULL },
			{ MODEST_STOCK_FORWARD, "forward", 0, 0, NULL },
			{ MODEST_STOCK_DELETE, "delete", 0, 0, NULL },
			{ MODEST_STOCK_NEXT, "next", 0, 0, NULL },
			{ MODEST_STOCK_PREV, "prev", 0, 0, NULL },
			{ MODEST_STOCK_STOP, "stop", 0, 0, NULL }
		};
      
		static gchar *items_names [] = {
			MODEST_TOOLBAR_ICON_MAIL_SEND,
			MODEST_TOOLBAR_ICON_NEW_MAIL,		
			MODEST_TOOLBAR_ICON_SEND_RECEIVE,
			MODEST_TOOLBAR_ICON_REPLY,	
			MODEST_TOOLBAR_ICON_REPLY_ALL,
			MODEST_TOOLBAR_ICON_FORWARD,
			MODEST_TOOLBAR_ICON_DELETE,
			MODEST_TOOLBAR_ICON_NEXT,
			MODEST_TOOLBAR_ICON_PREV,
			MODEST_TOOLBAR_ICON_STOP
		};

		registered = TRUE;

		/* Register our stock items */
		gtk_stock_add (items, G_N_ELEMENTS (items));
      
		/* Add our custom icon factory to the list of defaults */
		factory = gtk_icon_factory_new ();
		gtk_icon_factory_add_default (factory);

		/* Register icons to accompany stock items */
		for (i = 0; i < G_N_ELEMENTS (items); i++) {
			pixbuf = NULL;
			pixbuf = gdk_pixbuf_new_from_file (items_names[i], NULL);

			if (pixbuf != NULL) {
				GtkIconSet *icon_set;
				GdkPixbuf *transparent;

				transparent = gdk_pixbuf_add_alpha (pixbuf, TRUE, 0xff, 0xff, 0xff);

				icon_set = gtk_icon_set_new_from_pixbuf (transparent);
				gtk_icon_factory_add (factory, items[i].stock_id, icon_set);
				gtk_icon_set_unref (icon_set);
				g_object_unref (pixbuf);
				g_object_unref (transparent);
			}
			else
				g_warning ("failed to load %s icon", items_names[i]);
		}
		/* Drop our reference to the factory, GTK will hold a reference. */
		g_object_unref (factory);
	}
}

/* FIXME: uninit these as well */
static void
connect_signals (ModestUI *self)
{
	TnyDevice *device;
	ModestUIPrivate *priv;
	ModestFolderView *folder_view;
	ModestHeaderView *header_view;
	ModestMsgView *msg_view;
	GtkWidget *toggle;
	
	priv = MODEST_UI_GET_PRIVATE(self);

	folder_view = modest_widget_factory_get_folder_view (priv->widget_factory);
	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	msg_view    = modest_widget_factory_get_msg_preview (priv->widget_factory);
	toggle      = modest_widget_factory_get_online_toggle (priv->widget_factory);
	device      = tny_account_store_get_device (priv->account_store);

	/* folder view */
	g_signal_connect (G_OBJECT(folder_view), "folder_selection_changed",
			  G_CALLBACK(modest_ui_actions_on_folder_selection_changed),
			  priv);
/* 	g_signal_connect (G_OBJECT(folder_view), "key-press-event", */
/* 			  G_CALLBACK(on_folder_key_press_event), priv->widget_factory); */

	/* header view */
	g_signal_connect (G_OBJECT(header_view), "status_update",
			  G_CALLBACK(modest_ui_actions_on_header_status_update), 
			  priv);
	g_signal_connect (G_OBJECT(header_view), "header_selected",
			  G_CALLBACK(modest_ui_actions_on_header_selected), 
			  priv);
	g_signal_connect (G_OBJECT(header_view), "item_not_found",
			  G_CALLBACK(modest_ui_actions_on_item_not_found), 
			  priv);

	
	/* msg preview */
	g_signal_connect (G_OBJECT(msg_view), "link_clicked",
			  G_CALLBACK(modest_ui_actions_on_msg_link_clicked), 
			  priv);
	g_signal_connect (G_OBJECT(msg_view), "link_hover",
			  G_CALLBACK(modest_ui_actions_on_msg_link_hover), 
			  priv);
	g_signal_connect (G_OBJECT(msg_view), "attachment_clicked",
			  G_CALLBACK(modest_ui_actions_on_msg_attachment_clicked), 
			  priv);

	/* Device */
	g_signal_connect (G_OBJECT(device), "connection_changed",
			  G_CALLBACK(modest_ui_actions_on_connection_changed), 
			  priv);
	g_signal_connect (G_OBJECT(toggle), "toggled",
			  G_CALLBACK(modest_ui_actions_on_online_toggle_toggled),
			  priv);
		
	/* Init toggle in correct state */
	modest_ui_actions_on_connection_changed (device,
						 tny_device_is_online (device),
						 priv);
}


static void
connect_main_window_signals (ModestUI *self)
{
	ModestUIPrivate *priv;
	
	priv = MODEST_UI_GET_PRIVATE(self);

	/* account store */
	g_signal_connect (G_OBJECT (priv->account_store), 
			  "password_requested",
			  G_CALLBACK(modest_ui_actions_on_password_requested),
			  priv);
}

/* ***************************************************************** */
/*                M O D E S T    U I    A C T I O N S                */
/* ***************************************************************** */
static void     
modest_ui_actions_on_about (GtkWidget *widget, 
			    ModestUIPrivate *priv)
{
	GtkWidget *about;
	const gchar *authors[] = {
		"Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>",
		NULL
	};	
	about = gtk_about_dialog_new ();
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG(about), PACKAGE_NAME);
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about),PACKAGE_VERSION);
	gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG(about),
					_("Copyright (c) 2006, Nokia Corporation\n"
					  "All rights reserved."));
	gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG(about),
				       _("a modest e-mail client\n\n"
					 "design and implementation: Dirk-Jan C. Binnema\n"
					 "contributions from the fine people at KernelConcepts and Igalia\n"
					 "uses the tinymail email framework written by Philip van Hoof"));
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(about), authors);
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(about), "http://modest.garage.maemo.org");

	gtk_dialog_run (GTK_DIALOG (about));
	gtk_widget_destroy(about);
}

static void
modest_ui_actions_on_delete (GtkWidget *widget, 
			     ModestUIPrivate *priv)
{
	ModestWidgetFactory *widget_factory;
	ModestHeaderView *header_view;
	TnyList *header_list;
	TnyIterator *iter;
	GtkTreeModel *model;

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	header_list = modest_header_view_get_selected_headers (header_view);
	
	if (header_list) {
		iter = tny_list_create_iterator (header_list);
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
		if (GTK_IS_TREE_MODEL_SORT (model))
			model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model));
		do {
			TnyHeader *header;
			ModestMailOperation *mail_op;

			header = TNY_HEADER (tny_iterator_get_current (iter));
			/* TODO: thick grain mail operation involving
			   a list of objects. Composite pattern ??? */
			mail_op = modest_mail_operation_new ();

			/* TODO: add confirmation dialog */

			/* Move to trash */
			modest_mail_operation_remove_msg (mail_op, header, TRUE);

			/* Remove from tree model */
			if (modest_mail_operation_get_status (mail_op) == 
			    MODEST_MAIL_OPERATION_STATUS_SUCCESS)
				tny_list_remove (TNY_LIST (model), G_OBJECT (header));
			else {
				/* TODO: error handling management */
				const GError *error;
				error = modest_mail_operation_get_error (mail_op);
				g_warning (error->message);
			}

			g_object_unref (G_OBJECT (mail_op));
			g_object_unref (header);
			tny_iterator_next (iter);

		} while (!tny_iterator_is_done (iter));
	}
}

static void
modest_ui_actions_on_quit (GtkWidget *widget, 
			   ModestUIPrivate *priv)
{
	/* FIXME: save size of main window */
/* 	save_sizes (main_window); */
	gtk_widget_destroy (GTK_WIDGET (priv->main_window));
}

static void
modest_ui_actions_on_accounts (GtkWidget *widget, 
			       ModestUIPrivate *priv)
{
	GtkWidget *account_win;

	account_win = modest_account_view_window_new (priv->widget_factory);

	gtk_window_set_transient_for (GTK_WINDOW (account_win),
				      GTK_WINDOW (priv->main_window));
				      
	gtk_widget_show (account_win);
}

static void
modest_ui_actions_on_new_msg (GtkWidget *widget, 
			      ModestUIPrivate *priv)
{
	GtkWidget *msg_win;

	msg_win = modest_edit_msg_window_new (priv->widget_factory,
					      MODEST_EDIT_TYPE_NEW);
	gtk_widget_show (msg_win);
}

static void
reply_forward_func (gpointer data, gpointer user_data)
{
	TnyHeader *new_header;
	TnyMsg *msg, *new_msg;
	GetMsgAsyncHelper *helper;
	ReplyForwardHelper *rf_helper;
	GtkWidget *msg_win;
	ModestEditType edit_type;

	msg = TNY_MSG (data);
	helper = (GetMsgAsyncHelper *) user_data;
	rf_helper = (ReplyForwardHelper *) helper->user_data;

	/* Create reply mail */
	switch (rf_helper->action) {
	case ACTION_REPLY:
		new_msg = 
			modest_mail_operation_create_reply_mail (msg, 
								 rf_helper->from, 
								 rf_helper->reply_forward_type,
								 MODEST_MAIL_OPERATION_REPLY_MODE_SENDER);
	case ACTION_REPLY_TO_ALL:
		new_msg = 
			modest_mail_operation_create_reply_mail (msg, rf_helper->from, rf_helper->reply_forward_type,
								 MODEST_MAIL_OPERATION_REPLY_MODE_ALL);
		edit_type = MODEST_EDIT_TYPE_REPLY;
		break;
	case ACTION_FORWARD:
		new_msg = 
			modest_mail_operation_create_forward_mail (msg, rf_helper->from, rf_helper->reply_forward_type);
		edit_type = MODEST_EDIT_TYPE_FORWARD;
		break;
	}

	/* Set from */
	new_header = tny_msg_get_header (new_msg);
	tny_header_set_from (new_header, rf_helper->from);
	g_object_unref (G_OBJECT (new_header));
		
	/* Show edit window */
	msg_win = modest_edit_msg_window_new (helper->priv->widget_factory,
					      edit_type);
	modest_edit_msg_window_set_msg (MODEST_EDIT_MSG_WINDOW (msg_win),
					new_msg);
	gtk_widget_show (msg_win);
	
	/* Clean */
	g_object_unref (new_msg);
	g_free (rf_helper->from);
	g_slice_free (ReplyForwardHelper, rf_helper);
}

/*
 * Common code for the reply and forward actions
 */
static void
reply_forward (GtkWidget *widget,
	       ReplyForwardAction action,
	       ModestUIPrivate *priv)
{
	ModestHeaderView *header_view;
	TnyList *header_list;
	guint reply_forward_type;
	ModestConf *conf;
	TnyPlatformFactory *plat_factory;
	TnyHeader *header;
	TnyFolder *folder;
	gchar *from, *key;
	ModestFolderView *folder_view;
	GetMsgAsyncHelper *helper;
	ReplyForwardHelper *rf_helper;

	/* Get ModestConf */
	plat_factory = modest_tny_platform_factory_get_instance ();
	conf = modest_tny_platform_factory_get_modest_conf_instance (plat_factory);

	/* Get reply or forward type */
	key = g_strdup_printf ("%s/%s", MODEST_CONF_NAMESPACE, 
			       (action == ACTION_FORWARD) ? MODEST_CONF_FORWARD_TYPE : MODEST_CONF_REPLY_TYPE);
	reply_forward_type = modest_conf_get_int (conf, key, NULL);
	g_free (key);

	/* Get the list of headers */
	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	header_list = modest_header_view_get_selected_headers (header_view);

	if (!header_list)
		return;

	/* We assume that we can only select messages of the
	   same folder and that we reply all of them from the
	   same account. In fact the interface currently only
	   allows single selection */

	/* TODO: get the from string from account */
	from = g_strdup ("Invalid");
	
	/* Fill helpers */
	rf_helper = g_slice_new0 (ReplyForwardHelper);
	rf_helper->reply_forward_type = reply_forward_type;
	rf_helper->action = action;
	rf_helper->from = from;
	
	helper = g_slice_new0 (GetMsgAsyncHelper);
	helper->priv = priv;
	helper->func = reply_forward_func;
	helper->iter = tny_list_create_iterator (header_list);
	helper->user_data = rf_helper;
	
	header = TNY_HEADER (tny_iterator_get_current (helper->iter));
	folder = tny_header_get_folder (header);
	
	/* The callback will call it per each header */
	tny_folder_get_msg_async (folder, header, get_msg_cb, helper);
	
	/* Clean */
	g_object_unref (G_OBJECT (folder));
}

static void
modest_ui_actions_on_reply (GtkWidget *widget,
			    ModestUIPrivate *priv)
{
	reply_forward (widget, ACTION_REPLY, priv);
}

static void
modest_ui_actions_on_forward (GtkWidget *widget,
			      ModestUIPrivate *priv)
{
	reply_forward (widget, ACTION_FORWARD, priv);
}

static void
modest_ui_actions_on_reply_all (GtkWidget *widget,
				ModestUIPrivate *priv)
{
	reply_forward (widget, ACTION_REPLY_TO_ALL, priv);
}

static void 
modest_ui_actions_on_next (GtkWidget *widget, 
			   ModestUIPrivate *priv)
{
	ModestHeaderView *header_view;

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	modest_header_view_select_next (header_view);
}

/*
 * Marks a message as read and passes it to the msg preview widget
 */
static void
read_msg_func (gpointer data, gpointer user_data)
{
	ModestMsgView *msg_view;
	TnyMsg *msg;
	TnyHeader *header;
	GetMsgAsyncHelper *helper;
	TnyHeaderFlags header_flags;

	msg = TNY_MSG (data);
	helper = (GetMsgAsyncHelper *) user_data;

	/* mark message as seen; _set_flags crashes, bug in tinymail? */
	header = TNY_HEADER (tny_iterator_get_current (helper->iter));
	header_flags = tny_header_get_flags (header);
	tny_header_set_flags (header, header_flags | TNY_HEADER_FLAG_SEEN);
	g_object_unref (G_OBJECT (header));

	/* Set message on msg view */
	msg_view = modest_widget_factory_get_msg_preview (helper->priv->widget_factory);
	modest_msg_view_set_message (msg_view, msg);
}

/*
 * This function is a generic handler for the tny_folder_get_msg_async
 * call. It expects as user_data a #GetMsgAsyncHelper. This helper
 * contains a user provided function that is called inside this
 * method. This will allow us to use this callback in many different
 * places. This callback performs the common actions for the
 * get_msg_async call, more specific actions will be done by the user
 * function
 */
static void
get_msg_cb (TnyFolder *folder, TnyMsg *msg, GError **err, gpointer user_data)
{
	GetMsgAsyncHelper *helper;

	helper = (GetMsgAsyncHelper *) user_data;

	if (*err && ((*err)->code == TNY_FOLDER_ERROR_GET_MSG)) {
		ModestHeaderView *header_view;

		header_view = modest_widget_factory_get_header_view (helper->priv->widget_factory);
		modest_ui_actions_on_item_not_found (header_view, MODEST_ITEM_TYPE_MESSAGE, helper->priv);
		return;
	}

	if (!msg)
		return;

	/* Call user function */
	helper->func (msg, user_data);

	/* Process next element (if exists) */
	tny_iterator_next (helper->iter);
	if (tny_iterator_is_done (helper->iter)) {
		TnyList *headers;
		headers = tny_iterator_get_list (helper->iter);
		/* Free resources */
		g_object_unref (G_OBJECT (headers));
		g_object_unref (G_OBJECT (helper->iter));
		g_slice_free (GetMsgAsyncHelper, helper);
	} else
		tny_folder_get_msg_async (folder, 
					  TNY_HEADER (tny_iterator_get_current (helper->iter)), 
					  get_msg_cb, helper);
}

static void 
modest_ui_actions_on_header_selected (ModestHeaderView *folder_view, 
				      TnyHeader *header,
				      ModestUIPrivate *priv)
{
	TnyFolder *folder;
	GetMsgAsyncHelper *helper;
	TnyList *list;
	TnyIterator *iter;

	if (!header)
		return;

	folder = tny_header_get_folder (TNY_HEADER(header));

	/* Create list */
	list = tny_simple_list_new ();
	tny_list_prepend (list, G_OBJECT (header));

	/* Fill helper data */
	helper = g_slice_new0 (GetMsgAsyncHelper);
	helper->priv = priv;
	helper->iter = tny_list_create_iterator (list);
	helper->func = read_msg_func;

	tny_folder_get_msg_async (TNY_FOLDER(folder),
				  header,
				  get_msg_cb,
				  helper);

	/* Frees */
	g_object_unref (G_OBJECT (folder));
}

static void 
modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
					       TnyFolder *folder, 
					       gboolean selected,
					       ModestUIPrivate *priv)
{
	GtkLabel *folder_info_label;
	TnyPlatformFactory *factory;
	gchar *txt;	
	ModestConf *conf;
	ModestHeaderView *header_view;

	folder_info_label = 
		GTK_LABEL (modest_widget_factory_get_folder_info_label (priv->widget_factory));

	if (!folder) {
		gtk_label_set_label (GTK_LABEL(folder_info_label), "");
		return;
	}

	factory = modest_tny_platform_factory_get_instance ();
	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	conf = modest_tny_platform_factory_get_modest_conf_instance (factory);

	if (!selected) { /* the folder was unselected; save it's settings  */
		modest_widget_memory_save (conf, G_OBJECT (header_view),
					   "header-view");
	} else {  /* the folder was selected */
		guint num, unread;
		num    = tny_folder_get_all_count    (folder);
		unread = tny_folder_get_unread_count (folder);
			
		txt = g_strdup_printf (_("%d %s, %d unread"),
				       num, num==1 ? _("item") : _("items"), unread);		
		gtk_label_set_label (GTK_LABEL(folder_info_label), txt);
		g_free (txt);
			
		modest_header_view_set_folder (header_view, folder);
		modest_widget_memory_restore (conf, G_OBJECT(header_view),
					      "header-view");
	}
}

static void
modest_ui_actions_on_password_requested (ModestTnyAccountStore *account_store, 
					 const gchar* account_name,
					 gchar **password, 
					 gboolean *cancel, 
					 gboolean *remember, 
					 ModestUIPrivate *priv)
{
	gchar *txt;
	GtkWidget *dialog, *entry, *remember_pass_check;

	dialog = gtk_dialog_new_with_buttons (_("Password requested"),
					      GTK_WINDOW (priv->main_window),
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);

	txt = g_strdup_printf (_("Please enter your password for %s"), account_name);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), gtk_label_new(txt),
			    FALSE, FALSE, 0);
	g_free (txt);

	entry = gtk_entry_new_with_max_length (40);
	gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(entry), 0x2022); /* bullet unichar */
	
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), entry,
			    TRUE, FALSE, 0);	

	remember_pass_check = gtk_check_button_new_with_label (_("Remember password"));
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), remember_pass_check,
			    TRUE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	
	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		*password = g_strdup (gtk_entry_get_text (GTK_ENTRY(entry)));
		*cancel   = FALSE;
	} else {
		*password = NULL;
		*cancel   = TRUE;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (remember_pass_check)))
		*remember = TRUE;
	else
		*remember = FALSE;

	gtk_widget_destroy (dialog);

	while (gtk_events_pending ())
		gtk_main_iteration ();
}

/****************************************************/
/*
 * below some stuff to clearup statusbar messages after 1,5 seconds....
 */
typedef struct {
	GtkWidget *status_bar;
	GtkWidget *progress_bar;
	guint     msg_id;
} StatusRemoveData;

static gboolean
on_statusbar_remove_msg (StatusRemoveData *data)
{
	/* we need to test types, as this callback maybe called after the
	 * widgets have been destroyed
	 */
	if (GTK_IS_STATUSBAR(data->status_bar)) 
		gtk_statusbar_remove (GTK_STATUSBAR(data->status_bar),
				      0, data->msg_id);
	if (GTK_IS_PROGRESS_BAR(data->progress_bar))
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(data->progress_bar),
					       1.0);
	g_free (data);
	return FALSE;
}

static void
statusbar_push (ModestWidgetFactory *factory, guint context_id, const gchar *msg)
{
	guint id;
	StatusRemoveData *data;
	GtkWidget *status_bar, *progress_bar;
	
	if (!msg)
		return;

	progress_bar = modest_widget_factory_get_progress_bar (factory);
	status_bar   = modest_widget_factory_get_status_bar (factory);
	
	id = gtk_statusbar_push (GTK_STATUSBAR(status_bar), 0, msg);

	data = g_new (StatusRemoveData, 1);
	data->status_bar   = status_bar;
	data->progress_bar = progress_bar;
	data->msg_id     = id;

	g_timeout_add (1500, (GSourceFunc)on_statusbar_remove_msg, data);
}
/****************************************************************************/

static void
modest_ui_actions_on_connection_changed (TnyDevice *device, 
					 gboolean online,
					 ModestUIPrivate *priv)
{
	GtkWidget *online_toggle;
	ModestHeaderView *header_view;

	header_view   = modest_widget_factory_get_header_view (priv->widget_factory);
	online_toggle = modest_widget_factory_get_online_toggle (priv->widget_factory);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(online_toggle),
				      online);
	gtk_button_set_label (GTK_BUTTON(online_toggle),
			      online ? _("Online") : _("Offline"));

	statusbar_push (priv->widget_factory, 0, 
			online ? _("Modest went online") : _("Modest went offline"));

	/* If Modest has became online and the header view has a
	   header selected then show it */
	if (online) {
		GtkTreeSelection *selected;

		selected = gtk_tree_view_get_selection (GTK_TREE_VIEW (header_view));
		_modest_header_view_change_selection (selected, header_view);
	}
}

static void
modest_ui_actions_on_online_toggle_toggled (GtkToggleButton *toggle,
					    ModestUIPrivate *priv)
{
	gboolean online;
	TnyDevice *device;

	online  = gtk_toggle_button_get_active (toggle);
	device = tny_account_store_get_device (priv->account_store);

	if (online)
		tny_device_force_online (device);
	else
		tny_device_force_offline (device);
}

static void 
modest_ui_actions_on_item_not_found (ModestHeaderView *header_view,
				     ModestItemType type,
				     ModestUIPrivate *priv)
{
	GtkWidget *dialog;
	gchar *txt, *item;
	gboolean online;
	TnyDevice *device;

	item = (type == MODEST_ITEM_TYPE_FOLDER) ? "folder" : "message";
	device = tny_account_store_get_device (priv->account_store);
	
	gdk_threads_enter ();
	online = tny_device_is_online (device);

	if (online) {
		/* already online -- the item is simply not there... */
		dialog = gtk_message_dialog_new (GTK_WINDOW (priv->main_window),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_OK,
						 _("The %s you selected cannot be found"),
						 item);
		gtk_dialog_run (GTK_DIALOG(dialog));
	} else {

		dialog = gtk_dialog_new_with_buttons (_("Connection requested"),
						      GTK_WINDOW (priv->main_window),
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_REJECT,
						      GTK_STOCK_OK,
						      GTK_RESPONSE_ACCEPT,
						      NULL);

		txt = g_strdup_printf (_("This %s is not available in offline mode.\n"
					 "Do you want to get online?"), item);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
				    gtk_label_new (txt), FALSE, FALSE, 0);
		gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
		g_free (txt);

		gtk_window_set_default_size (GTK_WINDOW(dialog), 300, 300);
		if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			tny_device_force_online (device);
		}
	}
	gtk_widget_destroy (dialog);
	gdk_threads_leave ();
}

static void
modest_ui_actions_on_header_status_update (ModestHeaderView *header_view, const gchar *msg,
					   gint num, gint total, ModestUIPrivate *priv)
{
	GtkWidget *progress_bar;
	
	progress_bar = modest_widget_factory_get_progress_bar (priv->widget_factory);

	if (total != 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar),
					       (gdouble)num/(gdouble)total);
	else
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progress_bar));

	statusbar_push (priv->widget_factory, 0, msg);
}


static void
modest_ui_actions_on_msg_link_hover (ModestMsgView *msgview, const gchar* link,
				     ModestUIPrivate *priv)
{
	
	statusbar_push (priv->widget_factory, 0, link);

	/* TODO: do something */
}	


static void
modest_ui_actions_on_msg_link_clicked (ModestMsgView *msgview, const gchar* link,
				       ModestUIPrivate *priv)
{
	gchar *msg;
	msg = g_strdup_printf (_("Opening %s..."), link);

	statusbar_push (priv->widget_factory, 0, msg);
	g_free (msg);

	/* TODO: do something */
}

static void
modest_ui_actions_on_msg_attachment_clicked (ModestMsgView *msgview, int index,
					     ModestUIPrivate *priv)
{
	gchar *msg;
	
	msg = g_strdup_printf (_("Opening attachment %d..."), index);
	statusbar_push (priv->widget_factory, 0, msg);
	
	g_free (msg);

	/* TODO: do something */
}

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <glib/gi18n.h>
#include <string.h>
#include <modest-runtime.h>
#include "modest-ui-priv.h"
#include "modest-ui.h"
#include "modest-ui-actions.h"
#include "modest-icon-names.h"
#include "modest-tny-platform-factory.h"
#include "modest-account-view-window.h"
#include "modest-account-mgr-helpers.h"
#include "modest-main-window.h"
#include "modest-mail-operation.h"
#include <modest-widget-memory.h>
#include <tny-error.h>
#include <tny-simple-list.h>
#include <tny-msg-view.h>
#include <tny-device.h>

#define MODEST_UI_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MODEST_TYPE_UI, \
                                       ModestUIPrivate))

typedef struct _GetMsgAsyncHelper {
	ModestMainWindow *main_window;
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
static void     modest_ui_class_init   (ModestUIClass *klass);
static void     modest_ui_init         (ModestUI *obj);
static void     modest_ui_finalize     (GObject *obj);

static void     register_stock_icons   ();
static void     connect_signals        (ModestUI *self);

static void     reply_forward_func     (gpointer data, 
					gpointer user_data);
static void     read_msg_func          (gpointer data, 
					gpointer user_data);
static void     get_msg_cb             (TnyFolder *folder, 
					TnyMsg *msg, 
					GError **err, 
					gpointer user_data);

static void     reply_forward          (GtkWidget *widget,
					ReplyForwardAction action,
					ModestMainWindow *main_window);

static gchar*   ask_for_folder_name    (GtkWindow *parent_window,
					const gchar *title);

static void     _modest_ui_actions_on_accounts_reloaded (TnyAccountStore *store, 
							 gpointer user_data);

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

	priv->main_window    = NULL;
}


static void
modest_ui_finalize (GObject *obj)
{
	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);
	
	if (priv->ui_manager) {
		g_object_unref (G_OBJECT(priv->ui_manager));
		priv->ui_manager = NULL;
	}

	priv->main_window = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


ModestUI*
modest_ui_new (void)
{
	GObject *obj;
	ModestUIPrivate *priv;

	obj  = g_object_new(MODEST_TYPE_UI, NULL);
	priv = MODEST_UI_GET_PRIVATE(obj);
	
	/* Register our own icons as stock icons in order to
	   use them with the UI manager */
	register_stock_icons ();
		
	return MODEST_UI(obj);
}

static gboolean
on_main_window_destroy (GtkObject *widget, ModestUI *self)
{
	/* FIXME: check if there any viewer/editing windows opened */
	gtk_main_quit ();
	return FALSE;
}


ModestWindow *
modest_ui_main_window (ModestUI *self)
{
	ModestUIPrivate *priv;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_UI_GET_PRIVATE(self);

	if (!priv->main_window) {
		priv->main_window = modest_main_window_new ();
		connect_signals (self);
	}
		
	if (!priv->main_window)
		g_printerr ("modest: could not create main window\n");
	
	return priv->main_window;
}

ModestWindow *
modest_ui_edit_window (ModestUI *self, ModestEditType edit_type)
{
	ModestUIPrivate *priv;
	ModestWindow *edit_window;

	g_return_val_if_fail (self, NULL);
	priv = MODEST_UI_GET_PRIVATE(self);

	/* Create window */
	edit_window = modest_edit_msg_window_new (edit_type);
	
	/* Connect Edit Window signals */
/* 	connect_edit_window_signals (self); */
		
	return edit_window;
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
/* 			{ MODEST_STOCK_STOP, "stop", 0, 0, NULL } */
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
/* 			MODEST_TOOLBAR_ICON_STOP */
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
	TnyAccountStore *account_store;
	ModestUIPrivate *priv;
	ModestFolderView *folder_view;
	ModestHeaderView *header_view;
	ModestMsgView *msg_view;
	GtkWidget *toggle;
	ModestWidgetFactory *widget_factory;
	
	priv = MODEST_UI_GET_PRIVATE(self);

	widget_factory = modest_runtime_get_widget_factory (); 
	
	folder_view   = modest_widget_factory_get_folder_view (widget_factory);
	header_view   = modest_widget_factory_get_header_view (widget_factory);
	msg_view      = modest_widget_factory_get_msg_preview (widget_factory);
	toggle        = modest_widget_factory_get_online_toggle (widget_factory);
	account_store = TNY_ACCOUNT_STORE(modest_runtime_get_account_store());
	device        = tny_account_store_get_device (account_store);

	/* folder view */
	g_signal_connect (G_OBJECT(folder_view), "folder_selection_changed",
			  G_CALLBACK(_modest_ui_actions_on_folder_selection_changed),
			  priv->main_window);	
	/* header view */
	g_signal_connect (G_OBJECT(header_view), "status_update",
			  G_CALLBACK(_modest_ui_actions_on_header_status_update), 
			  priv->main_window);
	g_signal_connect (G_OBJECT(header_view), "header_selected",
			  G_CALLBACK(_modest_ui_actions_on_header_selected), 
			  priv->main_window);
	g_signal_connect (G_OBJECT(header_view), "item_not_found",
			  G_CALLBACK(_modest_ui_actions_on_item_not_found), 
			  priv->main_window);
	/* msg preview */
	g_signal_connect (G_OBJECT(msg_view), "link_clicked",
			  G_CALLBACK(_modest_ui_actions_on_msg_link_clicked), 
			  priv->main_window);
	g_signal_connect (G_OBJECT(msg_view), "link_hover",
			  G_CALLBACK(_modest_ui_actions_on_msg_link_hover), 
			  priv->main_window);
	g_signal_connect (G_OBJECT(msg_view), "attachment_clicked",
			  G_CALLBACK(_modest_ui_actions_on_msg_attachment_clicked), 
			  priv->main_window);

	/* Account store */
	g_signal_connect (G_OBJECT (account_store), "accounts_reloaded",
			  G_CALLBACK (_modest_ui_actions_on_accounts_reloaded),
			  priv->main_window);

	/* Device */
	g_signal_connect (G_OBJECT(device), "connection_changed",
			  G_CALLBACK(_modest_ui_actions_on_connection_changed), 
			  priv->main_window);
	g_signal_connect (G_OBJECT(toggle), "toggled",
			  G_CALLBACK(_modest_ui_actions_on_online_toggle_toggled),
			  priv->main_window);
		
	/* Destroy window */
	g_signal_connect (G_OBJECT(priv->main_window), 
			  "destroy",
			  G_CALLBACK(on_main_window_destroy), 
			  NULL);

	/* Init toggle in correct state */
	_modest_ui_actions_on_connection_changed (device,
						 tny_device_is_online (device),
						 MODEST_MAIN_WINDOW (priv->main_window));
}


/* ***************************************************************** */
/*                M O D E S T    U I    A C T I O N S                */
/* ***************************************************************** */
void   
_modest_ui_actions_on_about (GtkWidget *widget, 
			     ModestMainWindow *main_window)
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

void
_modest_ui_actions_on_delete (GtkWidget *widget, 
			     ModestMainWindow *main_window)
{
	ModestWidgetFactory *widget_factory;
	ModestHeaderView *header_view;
	TnyList *header_list;
	TnyIterator *iter;
	GtkTreeModel *model;

	widget_factory = modest_runtime_get_widget_factory ();
	header_view = modest_widget_factory_get_header_view (widget_factory);
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

void
_modest_ui_actions_on_quit (GtkWidget *widget, 
			   ModestMainWindow *main_window)
{
	/* FIXME: save size of main window */
/* 	save_sizes (main_window); */
	gtk_widget_destroy (GTK_WIDGET (main_window));
}

void
_modest_ui_actions_on_accounts (GtkWidget *widget, 
				ModestMainWindow *main_window)
{
	GtkWidget *account_win;
	account_win = modest_account_view_window_new (modest_runtime_get_widget_factory());

	gtk_window_set_transient_for (GTK_WINDOW (account_win),
				      GTK_WINDOW (main_window));
				      
	gtk_widget_show (account_win);
}

void
_modest_ui_actions_on_new_msg (GtkWidget *widget, 
			       ModestMainWindow *main_window)
{
	ModestWindow *msg_win;
	msg_win = modest_edit_msg_window_new (MODEST_EDIT_TYPE_NEW);
	gtk_widget_show_all (GTK_WIDGET (msg_win));
}

static void
reply_forward_func (gpointer data, gpointer user_data)
{
	TnyMsg *msg, *new_msg;
	GetMsgAsyncHelper *helper;
	ReplyForwardHelper *rf_helper;
	ModestWindow *msg_win;
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
		break;
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
	default:
		g_return_if_reached ();
	}

	if (!new_msg) {
		g_warning ("Unable to create a message");
		goto cleanup;
	}
		
	/* Show edit window */
	msg_win = modest_edit_msg_window_new (MODEST_EDIT_TYPE_NEW);
	modest_edit_msg_window_set_msg (MODEST_EDIT_MSG_WINDOW (msg_win),
					new_msg);
	gtk_widget_show_all (GTK_WIDGET (msg_win));
	
	/* Clean */
	g_object_unref (G_OBJECT (new_msg));

 cleanup:
	g_free (rf_helper->from);
	g_slice_free (ReplyForwardHelper, rf_helper);
}

/*
 * Common code for the reply and forward actions
 */
static void
reply_forward (GtkWidget *widget,
	       ReplyForwardAction action,
	       ModestMainWindow *main_window)
{
	ModestHeaderView *header_view;
	ModestAccountMgr *account_mgr;
	TnyList *header_list;
	guint reply_forward_type;
	ModestConf *conf;	
	ModestAccountData *default_account_data;
	TnyHeader *header;
	TnyFolder *folder;
	gchar *from, *key, *default_account_name;
	GetMsgAsyncHelper *helper;
	ReplyForwardHelper *rf_helper;

	conf = modest_runtime_get_conf ();
	
	/* Get reply or forward type */
	key = g_strdup_printf ("%s/%s", MODEST_CONF_NAMESPACE, 
			       (action == ACTION_FORWARD) ? MODEST_CONF_FORWARD_TYPE : MODEST_CONF_REPLY_TYPE);
	reply_forward_type = modest_conf_get_int (conf, key, NULL);
	g_free (key);

	/* Get the list of headers */
	header_view = modest_widget_factory_get_header_view (modest_runtime_get_widget_factory());
	header_list = modest_header_view_get_selected_headers (header_view);	
	if (!header_list)
		return;

	/* We assume that we can only select messages of the
	   same folder and that we reply all of them from the
	   same account. In fact the interface currently only
	   allows single selection */
	account_mgr = modest_runtime_get_account_mgr();
	default_account_name = modest_account_mgr_get_default_account (account_mgr);
	default_account_data = 
		modest_account_mgr_get_account_data (account_mgr,
						     (const gchar*) default_account_name);
	from = g_strdup (default_account_data->email);
	modest_account_mgr_free_account_data (account_mgr, default_account_data);
	g_free (default_account_name);
	
	/* Fill helpers */
	rf_helper = g_slice_new0 (ReplyForwardHelper);
	rf_helper->reply_forward_type = reply_forward_type;
	rf_helper->action = action;
	rf_helper->from = from;
	
	helper = g_slice_new0 (GetMsgAsyncHelper);
	helper->main_window = main_window;
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

void
_modest_ui_actions_on_reply (GtkWidget *widget,
			    ModestMainWindow *main_window)
{
	reply_forward (widget, ACTION_REPLY, main_window);
}

void
_modest_ui_actions_on_forward (GtkWidget *widget,
			      ModestMainWindow *main_window)
{
	reply_forward (widget, ACTION_FORWARD, main_window);
}

void
_modest_ui_actions_on_reply_all (GtkWidget *widget,
				ModestMainWindow *main_window)
{
	reply_forward (widget, ACTION_REPLY_TO_ALL, main_window);
}

void 
_modest_ui_actions_on_next (GtkWidget *widget, 
			   ModestMainWindow *main_window)
{
	ModestHeaderView *header_view;

	header_view = modest_widget_factory_get_header_view
		(modest_runtime_get_widget_factory());

	modest_header_view_select_next (header_view);
}

void
_modest_ui_actions_toggle_view (GtkWidget *widget,
				ModestMainWindow *main_window)
{
	ModestConf *conf;
	ModestHeaderView *header_view;

	header_view = modest_widget_factory_get_header_view
		(modest_runtime_get_widget_factory());

	conf = modest_runtime_get_conf ();
	
	/* what is saved/restored is depending on the style; thus; we save with
	 * old style, then update the style, and restore for this new style*/
	modest_widget_memory_save (conf, G_OBJECT(header_view), "header-view");
	
	if (modest_header_view_get_style (header_view) == MODEST_HEADER_VIEW_STYLE_DETAILS)
		modest_header_view_set_style (header_view, MODEST_HEADER_VIEW_STYLE_TWOLINES);
	else
		modest_header_view_set_style (header_view, MODEST_HEADER_VIEW_STYLE_DETAILS);

	modest_widget_memory_restore (conf, G_OBJECT(header_view), "header-view");
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
	msg_view = modest_widget_factory_get_msg_preview
		(modest_runtime_get_widget_factory());
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

	if ((*err && ((*err)->code == TNY_FOLDER_ERROR_GET_MSG)) || !msg) {
		ModestHeaderView *header_view;
		header_view = modest_widget_factory_get_header_view
			(modest_runtime_get_widget_factory());
		_modest_ui_actions_on_item_not_found (header_view,
						      MODEST_ITEM_TYPE_MESSAGE,
						      helper->main_window);
		return;
	}

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

void 
_modest_ui_actions_on_header_selected (ModestHeaderView *folder_view, 
				      TnyHeader *header,
				      ModestMainWindow *main_window)
{
	TnyFolder *folder;
	GetMsgAsyncHelper *helper;
	TnyList *list;

	/* when there's no header, clear the msgview */
	if (!header) {
		ModestMsgView *msg_view;
		msg_view       = modest_widget_factory_get_msg_preview
			(modest_runtime_get_widget_factory());
		modest_msg_view_set_message (msg_view, NULL);
		return;
	}

	folder = tny_header_get_folder (TNY_HEADER(header));

	/* Create list */
	list = tny_simple_list_new ();
	tny_list_prepend (list, G_OBJECT (header));

	/* Fill helper data */
	helper = g_slice_new0 (GetMsgAsyncHelper);
	helper->main_window = main_window;
	helper->iter = tny_list_create_iterator (list);
	helper->func = read_msg_func;

	tny_folder_get_msg_async (TNY_FOLDER(folder),
				  header, get_msg_cb,
				  helper);

	/* Frees */
	g_object_unref (G_OBJECT (folder));
}

void 
_modest_ui_actions_on_folder_selection_changed (ModestFolderView *folder_view,
					       TnyFolder *folder, 
					       gboolean selected,
					       ModestMainWindow *main_window)
{
	GtkLabel *folder_info_label;
	gchar *txt;	
	ModestConf *conf;
	ModestHeaderView *header_view;

	folder_info_label = 
		GTK_LABEL (modest_widget_factory_get_folder_info_label
			   (modest_runtime_get_widget_factory()));

	if (!folder) {
		gtk_label_set_label (GTK_LABEL(folder_info_label), "");
		return;
	}
	
	header_view = modest_widget_factory_get_header_view (modest_runtime_get_widget_factory());
	conf = modest_runtime_get_conf ();

	if (!selected) { /* the folder was unselected; save it's settings  */
		modest_widget_memory_save (conf, G_OBJECT (header_view),
					   "header-view");
		gtk_window_set_title (GTK_WINDOW(main_window), "Modest");
		modest_header_view_set_folder (header_view, NULL);
	} else {  /* the folder was selected */
		if (folder) { /* folder may be NULL */
			guint num, unread;
			gchar *title;

			num    = tny_folder_get_all_count    (folder);
			unread = tny_folder_get_unread_count (folder);
			
			title = g_strdup_printf ("Modest: %s",
						 tny_folder_get_name (folder));
			
			gtk_window_set_title (GTK_WINDOW(main_window), title);
			g_free (title);
			
			txt = g_strdup_printf (_("%d %s, %d unread"),
				       num, num==1 ? _("item") : _("items"), unread);		
			gtk_label_set_label (GTK_LABEL(folder_info_label), txt);
			g_free (txt);
		}
		modest_header_view_set_folder (header_view, folder);
		modest_widget_memory_restore (conf, G_OBJECT(header_view),
					      "header-view");
	}
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
progress_bar_clean (GtkWidget *bar)
{
	if (GTK_IS_PROGRESS_BAR(bar)) {
		gtk_progress_bar_set_text     (GTK_PROGRESS_BAR(bar), "");
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(bar), 1.0);
	}
	return FALSE;
}

static gboolean
statusbar_clean (GtkWidget *bar)
{
	if (GTK_IS_STATUSBAR(bar))
		gtk_statusbar_push (GTK_STATUSBAR(bar), 0, "");
	return FALSE;
}


static void
statusbar_push (ModestWidgetFactory *factory, guint context_id, const gchar *msg)
{
	GtkWidget *status_bar, *progress_bar;
	
	if (!msg)
		return;

	progress_bar = modest_widget_factory_get_progress_bar (factory);
	status_bar   = modest_widget_factory_get_status_bar (factory);

	gtk_widget_show (GTK_WIDGET(status_bar));
	gtk_widget_show (GTK_WIDGET(progress_bar));

	gtk_statusbar_push (GTK_STATUSBAR(status_bar), 0, msg);

	g_timeout_add (1500, (GSourceFunc)statusbar_clean, status_bar);
	g_timeout_add (3000, (GSourceFunc)progress_bar_clean, progress_bar);
}
/****************************************************************************/

void
_modest_ui_actions_on_connection_changed (TnyDevice *device, gboolean online,
					  ModestMainWindow *main_window)
{
	GtkWidget *online_toggle;
	ModestHeaderView *header_view;
	ModestWidgetFactory *widget_factory;
	GtkWidget *icon;
	const gchar *icon_name;

	g_return_if_fail (device);
	g_return_if_fail (main_window);

	icon_name = online ? GTK_STOCK_CONNECT : GTK_STOCK_DISCONNECT;
	icon      = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);

	widget_factory = modest_runtime_get_widget_factory ();
	header_view   = modest_widget_factory_get_header_view (widget_factory);
	online_toggle = modest_widget_factory_get_online_toggle (widget_factory);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(online_toggle),
				      online);
	gtk_button_set_image (GTK_BUTTON(online_toggle), icon);
	statusbar_push (widget_factory, 0, 
			online ? _("Modest went online") : _("Modest went offline"));
	
	/* If Modest has became online and the header view has a
	   header selected then show it */
	if (online) {
		GtkTreeSelection *selected;

		selected = gtk_tree_view_get_selection (GTK_TREE_VIEW (header_view));
		_modest_header_view_change_selection (selected, header_view);
	}
}

void
_modest_ui_actions_on_online_toggle_toggled (GtkToggleButton *toggle,
					     ModestMainWindow *main_window)
{
	gboolean online;
	TnyDevice *device;

	device = tny_account_store_get_device
		(TNY_ACCOUNT_STORE(modest_runtime_get_account_store()));

	online  = gtk_toggle_button_get_active (toggle);

	if (online)
		tny_device_force_online (device);
	else
		tny_device_force_offline (device);
}

void 
_modest_ui_actions_on_item_not_found (ModestHeaderView *header_view,
				     ModestItemType type,
				     ModestMainWindow *main_window)
{
	GtkWidget *dialog;
	gchar *txt, *item;
	gboolean online;
	TnyDevice *device;
	TnyPlatformFactory *factory;
	TnyAccountStore *account_store;

	item = (type == MODEST_ITEM_TYPE_FOLDER) ? "folder" : "message";

	/* Get device. Do not ask the platform factory for it, because
	   it returns always a new one */
	factory = modest_tny_platform_factory_get_instance ();
	account_store = tny_platform_factory_new_account_store (factory);
	device = tny_account_store_get_device (account_store);

	if (g_main_depth > 0)	
		gdk_threads_enter ();
	online = tny_device_is_online (device);

	if (online) {
		/* already online -- the item is simply not there... */
		dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_OK,
						 _("The %s you selected cannot be found"),
						 item);
		gtk_dialog_run (GTK_DIALOG(dialog));
	} else {

		dialog = gtk_dialog_new_with_buttons (_("Connection requested"),
						      GTK_WINDOW (main_window),
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
	if (g_main_depth > 0)	
		gdk_threads_leave ();
}



void
_modest_ui_actions_on_header_status_update (ModestHeaderView *header_view, 
					    const gchar *msg,
					    gint num, 
					    gint total, 
					    ModestMainWindow *main_window)
{
	GtkWidget *progress_bar;
	char* txt;
	
	progress_bar = modest_widget_factory_get_progress_bar
		(modest_runtime_get_widget_factory());
	if (total != 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar),
					       (gdouble)num/(gdouble)total);
	else
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progress_bar));

	txt = g_strdup_printf (_("Downloading %d of %d"), num, total);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress_bar), txt);
	g_free (txt);
	
	statusbar_push (modest_runtime_get_widget_factory(), 0, msg);
}



void
_modest_ui_actions_on_msg_link_hover (ModestMsgView *msgview, 
				      const gchar* link,
				      ModestMainWindow *main_window)
{
	statusbar_push (modest_runtime_get_widget_factory(), 0, link);

	/* TODO: do something */
}	


void
_modest_ui_actions_on_msg_link_clicked (ModestMsgView *msgview, 
					const gchar* link,
					ModestMainWindow *main_window)
{
	gchar *msg;

	msg = g_strdup_printf (_("Opening %s..."), link);
	statusbar_push (modest_runtime_get_widget_factory(), 0, msg);
	g_free (msg);

	/* TODO: do something */
}

void
_modest_ui_actions_on_msg_attachment_clicked (ModestMsgView *msgview, 
					      int index,
					      ModestMainWindow *main_window)
{
	gchar *msg;
	
	msg = g_strdup_printf (_("Opening attachment %d..."), index);
	statusbar_push (modest_runtime_get_widget_factory(), 0, msg);
	
	g_free (msg);
	/* TODO: do something */
}

void
_modest_ui_actions_on_send (GtkWidget *widget, 
			    ModestEditMsgWindow *edit_window)
{
	TnyTransportAccount *transport_account;
	ModestMailOperation *mail_operation;
	MsgData *data;

	data = modest_edit_msg_window_get_msg_data (edit_window);

	/* FIXME: Code added just for testing. The final version will
	   use the send queue provided by tinymail and some
	   classifier */
	{
		TnyList *accounts;
		TnyIterator *iter;
		
		accounts = TNY_LIST(tny_simple_list_new ());
		tny_account_store_get_accounts (TNY_ACCOUNT_STORE(modest_runtime_get_account_store()),
						accounts,
						TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS);

		iter = tny_list_create_iterator(accounts);
		tny_iterator_first (iter);
		if (tny_iterator_is_done (iter)) {
			/* FIXME: Add error handling through mail operation */
			g_printerr("modest: no transport accounts defined\n");
			modest_edit_msg_window_free_msg_data (edit_window, data);
			return;
		}
		transport_account = TNY_TRANSPORT_ACCOUNT (tny_iterator_get_current(iter));
		g_object_ref (transport_account);

		tny_list_foreach (accounts, (GFunc) g_object_unref, NULL);
		g_object_unref (G_OBJECT (accounts));
		g_object_unref (G_OBJECT (iter));
	}

	mail_operation = modest_mail_operation_new ();

	modest_mail_operation_send_new_mail (mail_operation,
					     transport_account,
					     data->from, 
					     data->to, 
					     data->cc, 
					     data->bcc,
					     data->subject, 
					     data->body, 
					     NULL);
	/* Frees */
	g_object_unref (G_OBJECT (mail_operation));
	g_object_unref (G_OBJECT (transport_account));
	modest_edit_msg_window_free_msg_data (edit_window, data);

	/* Save settings and close the window */
	/* save_settings (edit_window) */
	gtk_widget_destroy (GTK_WIDGET (edit_window));
}

/*
 * Shows a dialog with an entry that asks for some text. The returned
 * value must be freed by the caller. The dialog window title will be
 * set to @title.
 */
static gchar *
ask_for_folder_name (GtkWindow *parent_window,
		     const gchar *title)
{
	GtkWidget *dialog, *entry;
	gchar *folder_name = NULL;

	/* Ask for folder name */
	dialog = gtk_dialog_new_with_buttons (_("New Folder Name"),
					      parent_window,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    gtk_label_new(title),
			    FALSE, FALSE, 0);
		
	entry = gtk_entry_new_with_max_length (40);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
			    entry,
			    TRUE, FALSE, 0);	
	
	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	
	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)		
		folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

	gtk_widget_destroy (dialog);

	return folder_name;
}
	
void 
_modest_ui_actions_on_new_folder (GtkWidget *widget,
				  ModestMainWindow *main_window)
{
	TnyFolder *parent_folder;
	ModestFolderView *folder_view;

	folder_view = modest_widget_factory_get_folder_view
		(modest_runtime_get_widget_factory());
	parent_folder = modest_folder_view_get_selected (folder_view);
	
	if (parent_folder) {
		gchar *folder_name;

		folder_name = ask_for_folder_name (GTK_WINDOW (main_window),
						   _("Please enter a name for the new folder"));

		if (folder_name != NULL && strlen (folder_name) > 0) {
			TnyFolder *new_folder;
			ModestMailOperation *mail_op;

			mail_op = modest_mail_operation_new ();
			new_folder = modest_mail_operation_create_folder (mail_op,
									  TNY_FOLDER_STORE (parent_folder),
									  (const gchar *) folder_name);
			if (new_folder) {
				/* Do anything more? The model
				   is automatically updated */
				g_object_unref (new_folder);
			}
			g_object_unref (mail_op);
		}
		g_object_unref (parent_folder);
	}
}

void 
_modest_ui_actions_on_rename_folder (GtkWidget *widget,
				     ModestMainWindow *main_window)
{
	TnyFolder *folder;
	ModestFolderView *folder_view;
	
	folder_view = modest_widget_factory_get_folder_view (modest_runtime_get_widget_factory());
	folder = modest_folder_view_get_selected (folder_view);

	if (folder) {
		gchar *folder_name;

		folder_name = ask_for_folder_name (GTK_WINDOW (main_window),
						   _("Please enter a new name for the folder"));

		if (folder_name != NULL && strlen (folder_name) > 0) {
			ModestMailOperation *mail_op;

			mail_op = modest_mail_operation_new ();
			modest_mail_operation_rename_folder (mail_op,
							     folder,
							     (const gchar *) folder_name);
			g_object_unref (mail_op);
		}
		g_object_unref (folder);
	}
}

static void
delete_folder (ModestMainWindow *main_window,
	       gboolean move_to_trash) 
{
	TnyFolder *folder;
	ModestFolderView *folder_view;
	ModestMailOperation *mail_op;
	
	folder_view = modest_widget_factory_get_folder_view (modest_runtime_get_widget_factory());
	folder = modest_folder_view_get_selected (folder_view);

	mail_op = modest_mail_operation_new ();
	modest_mail_operation_remove_folder (mail_op, folder, move_to_trash);
	g_object_unref (mail_op);
}

void 
_modest_ui_actions_on_delete_folder (GtkWidget *widget,
				     ModestMainWindow *main_window)
{
	delete_folder (main_window, FALSE);
}

void 
_modest_ui_actions_on_move_to_trash_folder (GtkWidget *widget,
					    ModestMainWindow *main_window)
{
	delete_folder (main_window, TRUE);
}

static void
_modest_ui_actions_on_accounts_reloaded (TnyAccountStore *store, gpointer user_data)
{
	ModestFolderView *folder_view;
	
	folder_view = modest_widget_factory_get_folder_view (modest_runtime_get_widget_factory());
	modest_folder_view_update_model (folder_view, store);
}

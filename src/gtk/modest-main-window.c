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
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtktreeviewcolumn.h>

#include <modest-widget-memory.h>
#include <modest-icon-factory.h>

#include <widgets/modest-toolbar.h>

#include "modest-main-window.h"
#include "modest-account-view-window.h"
#include "modest-account-mgr.h"
#include "modest-conf.h"
#include "modest-edit-msg-window.h"
#include "modest-icon-names.h"
#include "modest-tny-platform-factory.h"
#include "modest-mail-operation.h"

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

	GtkWidget *toolbar;
	GtkWidget *menubar;

	GtkWidget *folder_paned;
	GtkWidget *msg_paned;
	GtkWidget *main_paned;
	
	ModestWidgetFactory *widget_factory;
	TnyPlatformFactory *factory;
  
	ModestHeaderView *header_view;
	ModestFolderView *folder_view;
	ModestMsgView    *msg_preview;
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

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void
on_menu_about (GtkWidget *widget, gpointer data)
{
	GtkWidget *about;
	const gchar *authors[] = {
		"Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>",
		NULL
	};	
	about = gtk_about_dialog_new ();
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG(about), PACKAGE_NAME);
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about),PACKAGE_VERSION);
	gtk_about_dialog_set_copyright (
		GTK_ABOUT_DIALOG(about),
		_("Copyright (c) 2006, Nokia Corporation\n"
		  "All rights reserved."));
	gtk_about_dialog_set_comments (	GTK_ABOUT_DIALOG(about),
		_("a modest e-mail client\n\n"
		  "design and implementation: Dirk-Jan C. Binnema\n"
		  "contributions from the fine people at KernelConcepts\n\n"
		  "uses the tinymail email framework written by Philip van Hoof"));
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(about), authors);
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(about), "http://modest.garage.maemo.org");

	gtk_dialog_run (GTK_DIALOG (about));
	gtk_widget_destroy(about);
}


static void
on_menu_accounts (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	GtkWidget *account_win;
	ModestMainWindowPrivate *priv;

	g_return_if_fail (widget);
	g_return_if_fail (self);
	
	priv        = MODEST_MAIN_WINDOW_GET_PRIVATE(self);	
	account_win = modest_account_view_window_new (priv->widget_factory);

	gtk_window_set_transient_for (GTK_WINDOW(account_win),
				      GTK_WINDOW(self));
				      
	gtk_widget_show (account_win);
}


static void
on_menu_new_message (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	GtkWidget *msg_win;
	ModestMainWindowPrivate *priv;

	priv  = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	msg_win = modest_edit_msg_window_new (priv->widget_factory,
					      MODEST_EDIT_TYPE_NEW,
					      NULL);
	gtk_widget_show (msg_win);
}

static void
on_menu_reply_forward (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	GtkWidget *msg_win;
	ModestMainWindowPrivate *priv;
	ModestHeaderView *header_view;
	TnyList *header_list;
	TnyIterator *iter;

	priv  = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	header_list = modest_header_view_get_selected_headers (header_view);

	if (header_list) {
		iter = tny_list_create_iterator (header_list);
		do {
			TnyHeader *header;
			TnyFolder *folder;
			TnyMsg    *msg, *new_msg;
			ModestEditType edit_type;

			/* Get msg from header */
			header = TNY_HEADER (tny_iterator_get_current (iter));
			folder = tny_header_get_folder (header);
			msg = tny_folder_get_msg (folder, header);

			/* FIXME: select proper action */
			switch (action) {
			case 1:
				/* TODO: get reply type from config */
				new_msg = 
					modest_mail_operation_create_reply_mail (msg,
										 MODEST_MAIL_OPERATION_REPLY_TYPE_CITE,
										 MODEST_MAIL_OPERATION_REPLY_MODE_SENDER);
				edit_type = MODEST_EDIT_TYPE_REPLY;
				break;
			case 2:
				/* TODO: get reply type from config */
				new_msg = 
					modest_mail_operation_create_reply_mail (msg,
										 MODEST_MAIL_OPERATION_REPLY_TYPE_QUOTE,
										 MODEST_MAIL_OPERATION_REPLY_MODE_ALL);
				edit_type = MODEST_EDIT_TYPE_REPLY;
				break;
			case 3:
				/* TODO: get forward type from config */
				new_msg = 
					modest_mail_operation_create_forward_mail (msg,
										   MODEST_MAIL_OPERATION_FORWARD_TYPE_INLINE);
				edit_type = MODEST_EDIT_TYPE_FORWARD;
				break;
			}
			/* Show edit window */
			msg_win = modest_edit_msg_window_new (priv->widget_factory,
							      edit_type,
							      new_msg);
			gtk_widget_show (msg_win);

			/* Clean and go on */
			g_object_unref (new_msg);
			tny_iterator_next (iter);

		} while (!tny_iterator_is_done (iter));
	}
}

static void
on_menu_quit (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	save_sizes (self);
	gtk_widget_destroy (GTK_WIDGET(self));
}


/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static GtkItemFactoryEntry menu_items[] = {
	{ "/_File",		NULL,			NULL,           0, "<Branch>", NULL },
	{ "/File/_New",		"<control>N",		NULL,		0, "<StockItem>", GTK_STOCK_NEW },
	{ "/File/_Open",	"<control>O",		NULL,		0, "<StockItem>", GTK_STOCK_OPEN },
	{ "/File/_Save",	"<control>S",		NULL,		0, "<StockItem>", GTK_STOCK_SAVE },
	{ "/File/Save _As",	NULL,			NULL,           0, "<Item>", NULL },
	{ "/File/sep1",		NULL,			NULL,           0, "<Separator>", NULL },
	{ "/File/_Quit",	"<CTRL>Q",		on_menu_quit,  0, "<StockItem>", GTK_STOCK_QUIT },

	{ "/_Edit",		NULL,			NULL,           0, "<Branch>", NULL },
	{ "/Edit/_Undo",	"<CTRL>Z",		NULL,		0, "<StockItem>", GTK_STOCK_UNDO },
	{ "/Edit/_Redo",	"<shift><CTRL>Z",	NULL,		0, "<StockItem>", GTK_STOCK_REDO },
	{ "/File/sep1",		NULL,			NULL,           0, "<Separator>", NULL },
	{ "/Edit/Cut",		"<control>X",		NULL,		0, "<StockItem>", GTK_STOCK_CUT  },
	{ "/Edit/Copy",		"<CTRL>C",		NULL,           0, "<StockItem>", GTK_STOCK_COPY },
	{ "/Edit/Paste",	NULL,			NULL,           0, "<StockItem>", GTK_STOCK_PASTE},
	{ "/Edit/sep1",		NULL,			NULL,           0, "<Separator>", NULL },
	{ "/Edit/Delete",	"<CTRL>Q",		NULL,           0, "<Item>" ,NULL},
	{ "/Edit/Select all",	"<CTRL>A",		NULL,           0, "<Item>" ,NULL},
	{ "/Edit/Deelect all",  "<Shift><CTRL>A",	NULL,           0, "<Item>" ,NULL},

	{ "/_Actions",                NULL,		NULL,		0, "<Branch>" ,NULL},
	{ "/Actions/_New Message",    NULL,		on_menu_new_message,		0, "<Item>",NULL },
	{ "/Actions/_Reply",    NULL,			NULL,		0, "<Item>" ,NULL},
	{ "/Actions/_Forward",  NULL,			NULL,		0, "<Item>" ,NULL},
	{ "/Actions/_Bounce",   NULL,			NULL,		0, "<Item>",NULL },	
	
	{ "/_Options",		 NULL,			NULL,		0, "<Branch>" ,NULL},
	{ "/Options/_Accounts",  NULL,			on_menu_accounts,0, "<Item>" ,NULL},
	{ "/Options/_Contacts",  NULL,			NULL,		0, "<Item>" ,NULL },


	{ "/_Help",         NULL,                       NULL,           0, "<Branch>" ,NULL},
	{ "/_Help/About",   NULL,                       on_menu_about,  0, "<StockItem>", GTK_STOCK_ABOUT},
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


static GtkWidget *
menubar_new (ModestMainWindow *self)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	
	/* Make an accelerator group (shortcut keys) */
	accel_group = gtk_accel_group_new ();
	
	/* Make an ItemFactory (that makes a menubar) */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
					     accel_group);
	
	/* This function generates the menu items. Pass the item factory,
	   the number of items in the array, the array itself, and any
	   callback data for the the menu items. */
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, self);
	
	///* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);
	
	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}




static ModestHeaderView*
header_view_new (ModestMainWindow *self)
{
	int i;
	GSList *columns = NULL;
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
		columns = g_slist_append (columns, GINT_TO_POINTER(cols[i]));

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	modest_header_view_set_columns (header_view, columns);
	g_slist_free (columns);

	return header_view;
}

static void
on_toolbar_button_clicked (ModestToolbar *toolbar, ModestToolbarButton button_id,
			   ModestMainWindow *self)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	switch (button_id) {
	case MODEST_TOOLBAR_BUTTON_NEW_MAIL:
		on_menu_new_message (self, 0, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_REPLY:
		on_menu_reply_forward (self, 1, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_REPLY_ALL:
		on_menu_reply_forward (self, 2, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_FORWARD:
		on_menu_reply_forward (self, 3, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_SEND_RECEIVE:
		

	case MODEST_TOOLBAR_BUTTON_NEXT:
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->header_view));
		if (sel) {
			gtk_tree_selection_get_selected (sel, &model, &iter);
			gtk_tree_model_iter_next (model, &iter);
			gtk_tree_selection_select_iter (sel, &iter);
		}
		
	case MODEST_TOOLBAR_BUTTON_PREV:
	/* 	if (sel) { */
/* 			gtk_tree_selection_get_selected (sel, &model, &iter); */
/* 			gtk_tree_model_iter_prev (model, &iter); */
/* 			gtk_tree_selection_select_iter (sel, &iter); */
/* 		} */

		break;
	case MODEST_TOOLBAR_BUTTON_DELETE:

	default:
		g_printerr ("modest: key %d pressed\n", button_id);
	}
}

static ModestToolbar*
toolbar_new (ModestMainWindow *self)
{
	int i;
	ModestToolbar *toolbar;
	GSList *buttons = NULL;
	ModestMainWindowPrivate *priv;

	ModestToolbarButton button_ids[] = {
		MODEST_TOOLBAR_BUTTON_NEW_MAIL,
		MODEST_TOOLBAR_BUTTON_REPLY,
		MODEST_TOOLBAR_BUTTON_REPLY_ALL,
		MODEST_TOOLBAR_BUTTON_FORWARD,
		MODEST_TOOLBAR_SEPARATOR,
		MODEST_TOOLBAR_BUTTON_SEND_RECEIVE,
		MODEST_TOOLBAR_SEPARATOR,
		MODEST_TOOLBAR_BUTTON_PREV,
		MODEST_TOOLBAR_BUTTON_NEXT,
		MODEST_TOOLBAR_SEPARATOR,		
		MODEST_TOOLBAR_BUTTON_DELETE
	};		
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	for (i = 0 ; i != sizeof(button_ids) / sizeof(ModestToolbarButton); ++i)
		buttons = g_slist_append (buttons, GINT_TO_POINTER(button_ids[i]));
	
	toolbar = modest_widget_factory_get_main_toolbar (priv->widget_factory, buttons);
	g_slist_free (buttons);
	
	g_signal_connect (G_OBJECT(toolbar), "button_clicked",
			  G_CALLBACK(on_toolbar_button_clicked), self);
	
	return toolbar;
}



static void
restore_sizes (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_modest_conf_instance (priv->factory);

	modest_widget_memory_restore_settings (conf,GTK_WIDGET(self),
					       "modest-main-window");
	modest_widget_memory_restore_settings (conf, GTK_WIDGET(priv->folder_paned),
					       "modest-folder-paned");
	modest_widget_memory_restore_settings (conf, GTK_WIDGET(priv->msg_paned),
					       "modest-msg-paned");
	modest_widget_memory_restore_settings (conf, GTK_WIDGET(priv->main_paned),
					       "modest-main-paned");
}


static void
save_sizes (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;
	ModestConf *conf;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_modest_conf_instance (priv->factory);
	
	modest_widget_memory_save_settings (conf,GTK_WIDGET(self),
					    "modest-main-window");
	modest_widget_memory_save_settings (conf, GTK_WIDGET(priv->folder_paned),
					    "modest-folder-paned");
	modest_widget_memory_save_settings (conf, GTK_WIDGET(priv->msg_paned),
					    "modest-msg-paned");
	modest_widget_memory_save_settings (conf, GTK_WIDGET(priv->main_paned),
					    "modest-main-paned");
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

static GtkWidget*
favorites_view ()
{
	GtkWidget *favorites;
	GtkTreeStore *store;
	GtkTreeViewColumn *col;

	store = gtk_tree_store_new (1, G_TYPE_STRING);
	favorites = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));
	col = gtk_tree_view_column_new_with_attributes (_("Favorites"),
							gtk_cell_renderer_text_new(),
							"text", 0, NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW(favorites), col);
	gtk_widget_show_all (favorites);

	g_object_unref (G_OBJECT(store));

	return favorites;
}



GtkWidget*
modest_main_window_new (ModestWidgetFactory *widget_factory)
{
	GObject *obj;
	ModestMainWindowPrivate *priv;
	
	GtkWidget *main_vbox;
	GtkWidget *status_hbox;
	GtkWidget *header_win, *folder_win, *favorites_win;
	
	g_return_val_if_fail (widget_factory, NULL);

	obj  = g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	g_object_ref (widget_factory);
	priv->widget_factory = widget_factory;

	/* widgets from factory */
	priv->folder_view = modest_widget_factory_get_folder_view (widget_factory);
	priv->header_view = header_view_new (MODEST_MAIN_WINDOW(obj));
	priv->msg_preview = modest_widget_factory_get_msg_preview (widget_factory);
	
	folder_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->folder_view),
						 FALSE);
	header_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->header_view),
						 FALSE);			   
	favorites_win = wrapped_in_scrolled_window (favorites_view(),FALSE);			   
	
	/* tool/menubar */
	priv->menubar = menubar_new (MODEST_MAIN_WINDOW(obj));
	priv->toolbar = GTK_WIDGET(toolbar_new (MODEST_MAIN_WINDOW(obj)));

	/* paned */
	priv->folder_paned = gtk_vpaned_new ();
	priv->msg_paned = gtk_vpaned_new ();
	priv->main_paned = gtk_hpaned_new ();
	gtk_paned_add1 (GTK_PANED(priv->main_paned), priv->folder_paned);
	gtk_paned_add2 (GTK_PANED(priv->main_paned), priv->msg_paned);
	gtk_paned_add1 (GTK_PANED(priv->folder_paned), favorites_win);
	gtk_paned_add2 (GTK_PANED(priv->folder_paned), folder_win);
	gtk_paned_add1 (GTK_PANED(priv->msg_paned), header_win);
	gtk_paned_add2 (GTK_PANED(priv->msg_paned), GTK_WIDGET(priv->msg_preview));

	gtk_widget_show (GTK_WIDGET(priv->header_view));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(priv->header_view));

	
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

	gtk_window_set_title (GTK_WINDOW(obj), "Modest");
	gtk_window_set_icon  (GTK_WINDOW(obj),
			      modest_icon_factory_get_icon (MODEST_APP_ICON));
	
	gtk_widget_show_all (main_vbox);

	g_signal_connect (G_OBJECT(obj), "delete-event",
			  G_CALLBACK(on_delete_event), obj);
	
	return GTK_WIDGET(obj);
}

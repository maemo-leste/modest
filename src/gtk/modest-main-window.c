/* modest-main-window.c */

/* insert (c)/licensing information) */
#include <gtk/gtk.h>
#include <string.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <glib/gi18n.h>
#include "modest-main-window.h"



#include "../modest-tny-account-store.h"
#include "../modest-tny-folder-tree-view.h"
#include "../modest-tny-header-tree-view.h"
#include "../modest-tny-msg-view.h"



/* 'private'/'protected' functions */
static void   modest_main_window_class_init    (ModestMainWindowClass *klass);
static void   modest_main_window_init          (ModestMainWindow *obj,
						ModestConf *conf,
						ModestAccountMgr *modest_acc_mgr);
static void   modest_main_window_finalize      (GObject *obj);


static GtkWidget* modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr);
static GtkWidget* modest_main_window_header_tree (TnyMsgFolderIface *folder);

static GtkWidget* modest_main_window_toolbar (void);
static GtkWidget* modest_main_window_favorite_folder_list (void);
static GtkWidget* modest_main_window_message_preview (void);
static GtkWidget * modest_main_window_get_ui (ModestTnyFolderTreeView *folder_view,
					      ModestTnyHeaderTreeView  *header_view,
					      ModestTnyMsgView *message_view);
static void on_newmail_clicked (GtkToolButton *button, gpointer data);
static void on_refresh_clicked (GtkToolButton *button, gpointer data);
static void on_reply_clicked (GtkToolButton *button, gpointer data);
static void on_forward_clicked (GtkToolButton *button, gpointer data);
static void on_delmail_clicked (GtkToolButton *button, gpointer data);
static void on_cut_clicked (GtkToolButton *button, gpointer data);
static void on_copy_clicked (GtkToolButton *button, gpointer data);
static void on_paste_clicked (GtkToolButton *button, gpointer data);
static void on_quit_clicked (GtkToolButton *button, gpointer data);

static void on_folder_clicked (ModestTnyFolderTreeView *self,
			       TnyMsgFolderIface *folder,
			       gpointer data);
static void on_message_clicked (ModestTnyFolderTreeView *folder_tree,
				TnyMsgIface *message,
				gpointer data);
			       


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMainWindowPrivate ModestMainWindowPrivate;
struct _ModestMainWindowPrivate {

	ModestConf       *modest_conf;
	ModestAccountMgr *modest_acc_mgr;

	ModestTnyFolderTreeView  *folder_view;
	ModestTnyHeaderTreeView  *header_view;
	ModestTnyMsgView         *message_view;
};
#define MODEST_MAIN_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_MAIN_WINDOW, \
                                                ModestMainWindowPrivate))
/* globals */
static GtkWidgetClass *parent_class = NULL;

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

static GtkWidget*
wrapped_in_scrolled_win (GtkWidget *child)
{
	GtkWidget *scrolled_win;

	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_win),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_win),
					     GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER(scrolled_win), child);
	return scrolled_win;
}

	

static void
modest_main_window_init (ModestMainWindow *obj, ModestConf *modest_conf,
			 ModestAccountMgr *modest_acc_mgr)
{
}





static void
modest_main_window_finalize (GObject *obj)
{
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE (obj);

	priv->modest_conf    = NULL;
	priv->modest_acc_mgr = NULL;
}



GtkWidget*
modest_main_window_new (ModestConf *modest_conf, ModestAccountMgr *modest_acc_mgr)
{
	
	GtkWidget *ui;
	ModestMainWindow *self =
		MODEST_MAIN_WINDOW(g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL));
	ModestMainWindowPrivate *priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);

	priv->modest_conf    = modest_conf;
	priv->modest_acc_mgr = modest_acc_mgr;	
	
	priv->header_view    =  MODEST_TNY_HEADER_TREE_VIEW(modest_main_window_header_tree (NULL));
	g_signal_connect (priv->header_view, "message_selected", 
			  G_CALLBACK(on_message_clicked),
 			  self);
	
	priv->folder_view    = 	MODEST_TNY_FOLDER_TREE_VIEW(modest_main_window_folder_tree
							    (modest_acc_mgr));
	g_signal_connect (priv->folder_view, "folder_selected", 
 			  G_CALLBACK(on_folder_clicked),
			  self);

	priv->message_view    = MODEST_TNY_MSG_VIEW (modest_main_window_message_preview());
	
	ui = modest_main_window_get_ui(priv->folder_view,
				       priv->header_view,
				       priv->message_view);
	gtk_container_add (GTK_CONTAINER(self), ui);

	return GTK_WIDGET(self);
}
	



static GtkWidget *
modest_main_window_get_ui (ModestTnyFolderTreeView *folder_view,
			   ModestTnyHeaderTreeView  *header_view,
			   ModestTnyMsgView         *message_view)

{
	GtkWidget *vbox;
	GtkWidget *msg_vpaned,*folder_vpaned, *folder_msg_hpaned;
	GtkWidget *toolbar, *fav_folder_list;
	GtkWidget *status_bar;
	GtkWidget *swin_msg_list,*swin_folder_list, *swin_favfolder_list;
	GtkWidget *folder_label, *message_label;
	GtkWidget *folder_vbox, *message_vbox;
	
	toolbar     = modest_main_window_toolbar ();
	fav_folder_list = modest_main_window_favorite_folder_list ();
	vbox   = gtk_vbox_new  (FALSE,2);

	folder_vbox  = gtk_vbox_new  (FALSE,2);
	message_vbox = gtk_vbox_new  (FALSE,2);

	msg_vpaned = gtk_vpaned_new ();
	folder_vpaned = gtk_vpaned_new ();
	folder_msg_hpaned = gtk_hpaned_new  ();
	
	swin_msg_list    = wrapped_in_scrolled_win (GTK_WIDGET(header_view));
	swin_folder_list = wrapped_in_scrolled_win (GTK_WIDGET(folder_view));
	swin_favfolder_list = wrapped_in_scrolled_win (fav_folder_list);

	folder_label  = gtk_label_new (_("Folders"));
	gtk_label_set_justify (GTK_LABEL(folder_label), GTK_JUSTIFY_LEFT);
	
	gtk_paned_add1 (GTK_PANED(folder_vpaned), swin_favfolder_list);
	gtk_paned_add2 (GTK_PANED(folder_vpaned), swin_folder_list);
	gtk_box_pack_start (GTK_BOX(folder_vbox), folder_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(folder_vbox), folder_vpaned, TRUE, TRUE, 0);

	message_label=  gtk_label_new (_("Inbox"));
	gtk_label_set_justify (GTK_LABEL(message_label), GTK_JUSTIFY_LEFT);
	gtk_paned_add1 (GTK_PANED(msg_vpaned), swin_msg_list);
	gtk_paned_add2 (GTK_PANED(msg_vpaned), GTK_WIDGET(message_view));
	gtk_box_pack_start (GTK_BOX(message_vbox), message_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(message_vbox), msg_vpaned, TRUE, TRUE, 0);

	
	gtk_paned_add1 (GTK_PANED(folder_msg_hpaned), folder_vbox);
	gtk_paned_add2 (GTK_PANED(folder_msg_hpaned), message_vbox);

	gtk_box_pack_start (GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox), folder_msg_hpaned, TRUE, TRUE, 0);

	status_bar = gtk_statusbar_new ();
	gtk_box_pack_start (GTK_BOX(vbox), status_bar, FALSE, FALSE, 0);
	
	gtk_widget_show_all (GTK_WIDGET(vbox));
	return vbox;
}



static GtkWidget*
modest_main_window_header_tree (TnyMsgFolderIface *folder)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new (); 
	GtkWidget *header_tree;
	
	header_tree = GTK_WIDGET(modest_tny_header_tree_view_new(folder));
	if (!header_tree) {
		g_warning ("could not create header tree");
		return NULL;
	}

	column =  gtk_tree_view_column_new_with_attributes(_("Date"), renderer,
							   "text",
							   TNY_MSG_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
 							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(header_tree), column);



	column =  gtk_tree_view_column_new_with_attributes(_("From"), renderer,
							   "text",
							   TNY_MSG_HEADER_LIST_MODEL_FROM_COLUMN,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(header_tree), column);


	column =  gtk_tree_view_column_new_with_attributes(_("Subject"), renderer,
							   "text",
							   TNY_MSG_HEADER_LIST_MODEL_SUBJECT_COLUMN,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(header_tree), column);

	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(header_tree), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(header_tree), TRUE);
	
	return GTK_WIDGET(header_tree);
}



static GtkWidget*
modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new (); 
	GtkWidget *folder_tree;
	TnyAccountStoreIface *account_store_iface =
		TNY_ACCOUNT_STORE_IFACE(modest_tny_account_store_new (modest_acc_mgr));
	if (!account_store_iface) {
		g_warning ("could not initialze ModestTnyAccountStore");
		return NULL;
	}
	
	folder_tree = modest_tny_folder_tree_view_new (account_store_iface);
	if (!folder_tree) {
		g_warning ("could not create folder list");
		return NULL;
	}

	column = gtk_tree_view_column_new_with_attributes(_("All Mail Folders"),
							  renderer,"text",
							  TNY_ACCOUNT_TREE_MODEL_NAME_COLUMN,
							  NULL);	
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(folder_tree), column);

	column = gtk_tree_view_column_new_with_attributes(_("Unread"),
							  renderer, "text",
							  TNY_ACCOUNT_TREE_MODEL_UNREAD_COLUMN,
							  NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(folder_tree), column);

	
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(folder_tree), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(folder_tree), TRUE);

	return folder_tree;
}


static GtkWidget*
modest_main_window_favorite_folder_list (void)
{
	GtkWidget *folder_list;
	GtkListStore *folder_store;
	GtkTreeIter iter;
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new (); 

	folder_store = gtk_list_store_new (1, G_TYPE_STRING);
	folder_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL(folder_store));
	
	renderer = gtk_cell_renderer_text_new();
	
	gtk_tree_view_insert_column (GTK_TREE_VIEW(folder_list),
				     gtk_tree_view_column_new_with_attributes(_("Favorite Folders"),
									      renderer,
									      "text", 0,
									      NULL),
				     0);
	gtk_list_store_insert_with_values (GTK_LIST_STORE(folder_store),
					   &iter, -1, 0, _("Stuff"), -1);	
	return folder_list;
}


static GtkWidget*
modest_main_window_toolbar (void)
{
	GtkWidget *toolbar;
	GtkToolItem *newmail, *refresh, *reply, *forward,
		*cut, *copy, *paste, *delmail, *quit;

	gpointer modest_ui = NULL; /* FIXME */
	
	toolbar = gtk_toolbar_new ();

	newmail = gtk_tool_button_new_from_stock (GTK_STOCK_NEW); 
	delmail = gtk_tool_button_new_from_stock (GTK_STOCK_DELETE);

	reply   = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
	forward = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_FORWARD);

	refresh = gtk_tool_button_new_from_stock (GTK_STOCK_REFRESH);
	
	cut  = gtk_tool_button_new_from_stock (GTK_STOCK_CUT);	
	copy  = gtk_tool_button_new_from_stock (GTK_STOCK_COPY);
	paste = gtk_tool_button_new_from_stock (GTK_STOCK_PASTE);
	
	quit  = gtk_tool_button_new_from_stock (GTK_STOCK_QUIT);

	g_signal_connect (newmail, "clicked", G_CALLBACK(on_newmail_clicked),
			  modest_ui);
	g_signal_connect (refresh, "clicked", G_CALLBACK(on_refresh_clicked),
			  modest_ui);
	g_signal_connect (reply, "clicked", G_CALLBACK(on_reply_clicked),
			  modest_ui);
	g_signal_connect (forward, "clicked", G_CALLBACK(on_forward_clicked),
			  modest_ui);
	g_signal_connect (cut, "clicked", G_CALLBACK(on_cut_clicked),
			  modest_ui);
	g_signal_connect (copy, "clicked", G_CALLBACK(on_copy_clicked),
			  modest_ui);
	g_signal_connect (paste, "clicked", G_CALLBACK(on_paste_clicked),
			  modest_ui);	
	g_signal_connect (delmail, "clicked", G_CALLBACK(on_delmail_clicked),
			  modest_ui);
	g_signal_connect (quit, "clicked", G_CALLBACK(on_quit_clicked),
			  modest_ui);

	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(newmail), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar),
			    GTK_TOOL_ITEM(gtk_separator_tool_item_new()),
			    -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(refresh), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar),
			    GTK_TOOL_ITEM(gtk_separator_tool_item_new()),
			    -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(reply), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(forward), -1);
	
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar),
			    GTK_TOOL_ITEM(gtk_separator_tool_item_new()),
			    -1);

	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(cut), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(copy), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(paste), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(delmail), -1);
	
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar),
			    GTK_TOOL_ITEM(gtk_separator_tool_item_new()),
			    -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(quit), -1);

	return toolbar;
}


static GtkWidget*
modest_main_window_message_preview ()
{
	ModestTnyMsgView *msg_view;

	msg_view = MODEST_TNY_MSG_VIEW(modest_tny_msg_view_new (NULL));
	
	return GTK_WIDGET(msg_view);	
}


static void
on_newmail_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}

static void
on_refresh_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}
static void
on_reply_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}
static void
on_forward_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}
static void
on_delmail_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}

static void
on_cut_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);

}
static void
on_copy_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);

}
static void
on_paste_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}


static void
on_quit_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}



static void on_folder_clicked (ModestTnyFolderTreeView *folder_tree,
			       TnyMsgFolderIface *folder,
			       gpointer data)
{
	ModestMainWindow *self;
	ModestMainWindowPrivate *priv; 

	g_return_if_fail (folder);
	g_return_if_fail (data);
	
	self = MODEST_MAIN_WINDOW(data);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);
		
	modest_tny_header_tree_view_set_folder (priv->header_view,
						folder);	
}



static void on_message_clicked (ModestTnyFolderTreeView *folder_tree,
				TnyMsgIface *message,
				gpointer data)
{
	ModestMainWindow *self;
	ModestMainWindowPrivate *priv; 

	g_return_if_fail (message);
	g_return_if_fail (data);

	self = MODEST_MAIN_WINDOW(data);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE (self);
 		
	modest_tny_msg_view_set_message (priv->message_view,
					 message);
}


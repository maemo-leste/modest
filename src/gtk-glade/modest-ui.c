/* modest-ui.c */

/* insert (c)/licensing information) */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include "../modest-ui.h"
#include "../modest-window-mgr.h"
#include "../modest-account-mgr.h"

#include "../modest-tny-account-store.h"
#include "../modest-tny-folder-tree-view.h"
#include "../modest-tny-header-tree-view.h"
#include "../modest-tny-msg-view.h"

//#include "modest.glade.h"

#define MODEST_GLADE          "modest.glade"
#define MODEST_GLADE_MAIN_WIN "main"
#define MODEST_GLADE_EDIT_WIN "new_mail"


/* 'private'/'protected' functions */
static void   modest_ui_class_init     (ModestUIClass *klass);
static void   modest_ui_init           (ModestUI *obj);
static void   modest_ui_finalize       (GObject *obj);

static void    modest_ui_window_destroy    (GtkWidget *win, gpointer data);
static void    modest_ui_last_window_closed (GObject *obj, gpointer data);

static GtkWidget* modest_main_window_toolbar (void);
static GtkWidget* modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr);
static GtkWidget* modest_main_window_header_tree (TnyMsgFolderIface *folder);

static void on_folder_clicked (ModestTnyFolderTreeView *folder_tree,
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


typedef struct _ModestUIPrivate ModestUIPrivate;
struct _ModestUIPrivate {
	
	ModestConf           *modest_conf;
	ModestAccountMgr     *modest_acc_mgr;
	ModestWindowMgr      *modest_window_mgr;
	
	GtkWindow	     *main_window;
	GSList*		     *edit_window_list;	     

	GladeXML             *glade_xml;


};
#define MODEST_UI_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MODEST_TYPE_UI, \
                                       ModestUIPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

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

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_ui_init (ModestUI *obj)
{
 	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);

	priv->modest_acc_mgr    = NULL;
	priv->modest_conf       = NULL;
	priv->modest_window_mgr = NULL;
	priv->glade_xml         = NULL;
	
}

static void
modest_ui_finalize (GObject *obj)
{
	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);	
	
	if (priv->modest_acc_mgr)
		g_object_unref (priv->modest_acc_mgr);
	priv->modest_acc_mgr = NULL;
	
	if (priv->modest_conf)
		g_object_unref (priv->modest_conf);
	priv->modest_conf = NULL;
	
	if (priv->modest_window_mgr)
		g_object_unref (priv->modest_window_mgr);
	priv->modest_window_mgr = NULL;
}
	
GObject*
modest_ui_new (ModestConf *modest_conf)
{
	GObject *obj;
	ModestUIPrivate *priv;
	ModestAccountMgr *modest_acc_mgr;

	g_return_val_if_fail (modest_conf, NULL);

	obj = g_object_new(MODEST_TYPE_UI, NULL);	
	priv = MODEST_UI_GET_PRIVATE(obj);

	modest_acc_mgr =
		MODEST_ACCOUNT_MGR(modest_account_mgr_new (modest_conf));
	if (!modest_acc_mgr) {
		g_warning ("could not create ModestAccountMgr instance");
		g_object_unref (obj);
		return NULL;
	}

	glade_init ();
	priv->glade_xml = glade_xml_new (MODEST_GLADE,
					 MODEST_GLADE_MAIN_WIN,
					 NULL);
	if (!priv->glade_xml) {
		g_warning ("failed to do glade stuff");
		g_object_unref (obj);
		return NULL;
	}
	
	priv->modest_acc_mgr = modest_acc_mgr;
	g_object_ref (priv->modest_conf = modest_conf);

	priv->modest_window_mgr = MODEST_WINDOW_MGR(modest_window_mgr_new());
	g_signal_connect (priv->modest_window_mgr, "last_window_closed",
			  G_CALLBACK(modest_ui_last_window_closed),
			  NULL);
	return obj;
}


gboolean
modest_ui_show_main_window (ModestUI *modest_ui)
{
	GtkWidget       *win;
	int              height, width;
	ModestUIPrivate *priv;
	GtkWidget     *folder_view, *header_view;
	GtkWidget     *message_view;
	
	GtkWidget  *folder_view_holder,
		*header_view_holder,
		*message_view_holder;

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	
	height = modest_conf_get_int (priv->modest_conf,
				      MODEST_CONF_MAIN_WINDOW_HEIGHT,NULL);
	width  = modest_conf_get_int (priv->modest_conf,
				      MODEST_CONF_MAIN_WINDOW_WIDTH,NULL);
	
	win = glade_xml_get_widget (priv->glade_xml, "main");
	if (!win) {
		g_warning ("could not create main window");
		return FALSE;
	}

	folder_view =
		GTK_WIDGET(modest_main_window_folder_tree(priv->modest_acc_mgr));
	folder_view_holder = glade_xml_get_widget (priv->glade_xml, "folders");
	if (!folder_view||!folder_view_holder) {
		g_warning ("failed to create folder tree");
		return FALSE;
	}
	gtk_container_add (GTK_CONTAINER(folder_view_holder), folder_view);
	
	header_view  =  GTK_WIDGET(modest_main_window_header_tree (NULL));
	header_view_holder = glade_xml_get_widget (priv->glade_xml, "mail_list");
	if (!header_view) {
		g_warning ("failed to create header tree");
		return FALSE;
	}
	gtk_container_add (GTK_CONTAINER(header_view_holder), header_view);

	g_signal_connect (G_OBJECT(folder_view), "folder_selected", 
 			  G_CALLBACK(on_folder_clicked), modest_ui);
	
	message_view  = GTK_WIDGET(modest_tny_msg_view_new (NULL));
	message_view_holder = glade_xml_get_widget (priv->glade_xml, "mail_view");
	if (!message_view) {
		g_warning ("failed to create message view");
		return FALSE;
	}
	gtk_container_add (GTK_CONTAINER(message_view_holder), message_view);

	g_signal_connect (header_view, "message_selected", 
			  G_CALLBACK(on_message_clicked),
 			  modest_ui);
	
	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_MAIN_WINDOW, 0);
	g_signal_connect (win, "destroy", G_CALLBACK(modest_ui_window_destroy),
			  modest_ui);
	gtk_widget_set_usize (GTK_WIDGET(win), height, width);
	gtk_window_set_title (GTK_WINDOW(win), PACKAGE_STRING);
	
	gtk_widget_show_all (win);
	return TRUE;
}


gboolean
modest_ui_show_edit_window (ModestUI *modest_ui, const gchar* to,
			    const gchar* cc, const gchar* bcc,
			    const gchar* subject, const gchar *body,
			    const GSList* att)
{
	GtkWidget       *win;
	ModestUIPrivate *priv;
	
	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	int height = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_HEIGHT,NULL);
	int width  = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_WIDTH,NULL);

	win = glade_xml_get_widget (priv->glade_xml, "new_mail");
	if (!win) {
		g_warning ("could not create edit window");
		return FALSE;
	}
	
	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_EDIT_WINDOW, 0);

	g_signal_connect (win, "destroy", G_CALLBACK(modest_ui_window_destroy),
			  modest_ui);

	gtk_widget_set_usize (GTK_WIDGET(win), height, width);
	gtk_window_set_title (GTK_WINDOW(win),
			      subject ? subject : "Untitled");

	gtk_widget_show_all (win);

	return TRUE;
}


static void
modest_ui_window_destroy (GtkWidget *win, gpointer data)
{
	ModestUIPrivate *priv;

	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE((ModestUI*)data);
	if (!modest_window_mgr_unregister (priv->modest_window_mgr, G_OBJECT(win)))
		g_warning ("modest window mgr: failed to unregister %p",
			   G_OBJECT(win));
}


static void
modest_ui_last_window_closed (GObject *obj, gpointer data)
{
	gtk_main_quit ();
}




static void on_folder_clicked (ModestTnyFolderTreeView *folder_tree,
			       TnyMsgFolderIface *folder,
			       gpointer data)
{
	GtkWidget *win;
	ModestTnyHeaderTreeView *tree_view;
	ModestUIPrivate *priv;
	GtkWidget *scrollview;
		
	g_return_if_fail (folder);
	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE(data);
	scrollview = glade_xml_get_widget (priv->glade_xml,"mail_list");

	tree_view = MODEST_TNY_HEADER_TREE_VIEW(
		gtk_bin_get_child(GTK_BIN(scrollview)));
	win = glade_xml_get_widget (priv->glade_xml, "main");
	gtk_window_set_title (GTK_WINDOW(win),
			      tny_msg_folder_iface_get_name(folder));

	modest_tny_header_tree_view_set_folder (tree_view,
						folder);	
}



static void on_message_clicked (ModestTnyFolderTreeView *folder_tree,
				TnyMsgIface *message,
				gpointer data)
{
	GtkWidget *scrollview;
	ModestTnyMsgView *msg_view;
	ModestUIPrivate *priv;
	
	g_return_if_fail (message);
	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE(data);
	scrollview = glade_xml_get_widget (priv->glade_xml,"mail_view");

	msg_view = MODEST_TNY_MSG_VIEW(
		gtk_bin_get_child(GTK_BIN(scrollview)));

	modest_tny_msg_view_set_message (msg_view,
					 message);
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

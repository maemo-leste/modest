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
#include "../modest-tny-transport-actions.h"


//#include "modest.glade.h"

#define MODEST_GLADE          PREFIX "/share/modest/glade/modest.glade"
#define MODEST_GLADE_MAIN_WIN "main"
#define MODEST_GLADE_EDIT_WIN "new_mail"


/* 'private'/'protected' functions */
static void   modest_ui_class_init     (ModestUIClass *klass);
static void   modest_ui_init           (ModestUI *obj);
static void   modest_ui_finalize       (GObject *obj);

static void    modest_ui_window_destroy    (GtkWidget *win, gpointer data);
static void    modest_ui_last_window_closed (GObject *obj, gpointer data);

static GtkWidget* modest_main_window_toolbar (void);
static GtkWidget* modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr,
						  TnyAccountStoreIface *account_store);
static GtkWidget* modest_main_window_header_tree (TnyMsgFolderIface *folder);


static void on_account_settings1_activate (GtkMenuItem *,
					   gpointer);
static void on_password_requested (ModestTnyAccountStore *account_store,
				   const gchar *account_name, gpointer user_data);

static void on_folder_clicked (ModestTnyFolderTreeView *folder_tree,
			       TnyMsgFolderIface *folder,
			       gpointer data);
static void on_message_clicked (ModestTnyFolderTreeView *folder_tree,
				TnyMsgIface *message,
				gpointer data);
static void on_new_mail_clicked (GtkWidget *widget, ModestUI *modest_ui);

static void on_reply_clicked (GtkWidget *widget, ModestUI *modest_ui);

static void on_send_button_clicked (GtkWidget *widget, ModestUI *modest_ui);

static void register_toolbar_callbacks (ModestUI *modest_ui);


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
	TnyAccountStoreIface *account_store;

	GtkWindow	     *main_window;
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
	TnyAccountStoreIface *account_store_iface;

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

	account_store_iface =
		TNY_ACCOUNT_STORE_IFACE(modest_tny_account_store_new (modest_acc_mgr));
	if (!account_store_iface) {
		g_warning ("could not initialze ModestTnyAccountStore");
		return NULL;
	}
	g_signal_connect (account_store_iface, "password_requested",
			  G_CALLBACK(on_password_requested),
			  NULL);
	glade_init ();
	priv->glade_xml = glade_xml_new (MODEST_GLADE,
					 NULL,NULL);
	if (!priv->glade_xml) {
		g_warning ("failed to do glade stuff");
		g_object_unref (obj);
		return NULL;
	}

	/* FIXME: could be used, but doesn't work atm.
	 * glade_xml_signal_autoconnect(priv->glade_xml);
	 */

	priv->modest_acc_mgr = modest_acc_mgr;
	g_object_ref (priv->modest_conf = modest_conf);

	priv->account_store = account_store_iface;

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
	GtkWidget *account_settings_item;

	GtkWidget  *folder_view_holder,
		*header_view_holder,
		*mail_paned;

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
		GTK_WIDGET(modest_main_window_folder_tree(priv->modest_acc_mgr,
							  priv->account_store));
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
	mail_paned = glade_xml_get_widget (priv->glade_xml, "mail_paned");
	if (!message_view) {
		g_warning ("failed to create message view");
		return FALSE;
	}
	gtk_paned_add2 (GTK_PANED(mail_paned), message_view);

	g_signal_connect (header_view, "message_selected",
			  G_CALLBACK(on_message_clicked),
			  modest_ui);

	account_settings_item = glade_xml_get_widget (priv->glade_xml, "account_settings1");
	if (!account_settings_item)
	{
		g_warning ("The account settings item isn't available!\n");
		return FALSE;
	}

	g_signal_connect (account_settings_item, "activate",
			  G_CALLBACK(on_account_settings1_activate),
			  modest_ui);

	register_toolbar_callbacks (modest_ui);

	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_MAIN_WINDOW, 0);
	g_signal_connect (win, "destroy", G_CALLBACK(modest_ui_window_destroy),
			  modest_ui);
	gtk_widget_set_usize (GTK_WIDGET(win), height, width);
	gtk_window_set_title (GTK_WINDOW(win), PACKAGE_STRING);

	gtk_widget_show_all (win);
	return TRUE;
}


static void
register_toolbar_callbacks (ModestUI *modest_ui)
{
	ModestUIPrivate *priv;
	GtkWidget *button;

	g_return_if_fail (modest_ui);

	priv = MODEST_UI_GET_PRIVATE (modest_ui);

	button = glade_xml_get_widget (priv->glade_xml, "toolb_new_mail");
	if (button)
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_new_mail_clicked), modest_ui);

	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply");
	if (button)
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_reply_clicked), modest_ui);
}



static void
hide_edit_window (GtkWidget *win, gpointer data)
{
	gtk_widget_hide (win);
}




gboolean
modest_ui_show_edit_window (ModestUI *modest_ui, const gchar* to,
			    const gchar* cc, const gchar* bcc,
			    const gchar* subject, const gchar *body,
			    const GSList* att)
{
	GtkWidget       *win;
	ModestUIPrivate *priv;
	GtkWidget       *btn;

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	int height = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_HEIGHT,NULL);
	int width  = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_WIDTH,NULL);

	win = glade_xml_get_widget (priv->glade_xml, "new_mail");
	if (!win) {
		g_warning ("could not create new mail  window");
		return FALSE;
	}

	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_EDIT_WINDOW, 0);

	g_signal_connect (win, "destroy", G_CALLBACK(hide_edit_window),
			  NULL);

	gtk_widget_set_usize (GTK_WIDGET(win), height, width);
	gtk_window_set_title (GTK_WINDOW(win),
			      subject ? subject : "Untitled");


	btn = glade_xml_get_widget (priv->glade_xml, "toolb_send");
	g_signal_connect (btn, "clicked", G_CALLBACK(on_send_button_clicked),
			  modest_ui);

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


static void
on_account_settings1_activate (GtkMenuItem *menuitem,
			       gpointer user_data)
{
	GtkWidget *advanced_account_setup;
	ModestUIPrivate *priv;
	gint retval;

	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(user_data));

	advanced_account_setup = glade_xml_get_widget(priv->glade_xml, "mailbox_setup_advanced");

	gtk_widget_show_all(GTK_WIDGET(advanced_account_setup));

	retval=gtk_dialog_run(GTK_DIALOG(advanced_account_setup));

        gtk_widget_hide(GTK_WIDGET(advanced_account_setup));
}


static void
on_folder_clicked (ModestTnyFolderTreeView *folder_tree,
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
	GtkWidget *paned;
	ModestTnyMsgView *msg_view;
	ModestUIPrivate *priv;

	g_return_if_fail (message);
	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE(data);
	paned = glade_xml_get_widget (priv->glade_xml,"mail_paned");
	msg_view = MODEST_TNY_MSG_VIEW(gtk_paned_get_child2 (GTK_PANED(paned)));

	modest_tny_msg_view_set_message (msg_view,
					 message);
}

static void
on_password_requested (ModestTnyAccountStore *account_store,
		       const gchar *account_name, gpointer user_data)
{

	GtkWidget *passdialog;
	GtkWidget *vbox;
	GtkWidget *infolabel;
	GtkWidget *passentry;
	gint retval;
	const gchar *infostring=g_strconcat("Please enter the password for ", account_name, ".", NULL);

	passdialog = gtk_dialog_new_with_buttons("MyDialog",
						 NULL,
						 GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_STOCK_OK,
						 GTK_RESPONSE_ACCEPT,
						 GTK_STOCK_CANCEL,
						 GTK_RESPONSE_REJECT,
						 NULL);

	vbox=gtk_vbox_new(FALSE, 0);

	infolabel=gtk_label_new(infostring);
	passentry=gtk_entry_new();

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(passdialog)->vbox), infolabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(passdialog)->vbox), passentry, FALSE, FALSE, 0);
	gtk_widget_show_all(passdialog);

	retval = gtk_dialog_run (GTK_DIALOG(passdialog));

	switch (retval)
	{
	case GTK_RESPONSE_ACCEPT:
		modest_account_mgr_set_server_account_string(modest_tny_account_store_get_accout_mgr(account_store),
							     account_name,
							     "password",
							     gtk_entry_get_text(GTK_ENTRY(passentry)),
							     NULL);
		break;
	case GTK_RESPONSE_CANCEL:
		/* FIXME:
		 * What happens, if canceled?"
		 */
		break;
	}

	gtk_widget_destroy (passdialog);
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
modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr,
				TnyAccountStoreIface *account_store)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkWidget *folder_tree;

	folder_tree = modest_tny_folder_tree_view_new (account_store);
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


static void
on_new_mail_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	g_return_if_fail (modest_ui);
	modest_ui_show_edit_window (modest_ui, "", "", "", "", "", NULL);
}

/* WIP, testing az */
static void
on_reply_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	g_return_if_fail (modest_ui);
	modest_ui_show_edit_window (modest_ui, "replyto", "cc", "bcc", "sub", "body-quote", NULL);
}


/* FIXME: truly evil --> we cannot really assume that
 * there is only one edit window open...
 */
static void
on_send_button_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	ModestTnyTransportActions *actions;
	ModestUIPrivate *priv;
	GtkWidget *to_entry, *subject_entry, *body_view;
	const gchar *to, *subject;
	gchar *body;
	GtkTextIter start, end;
	GtkTextBuffer *buf;
	TnyAccountStoreIface *account_store;
	const GList *transport_accounts;
	TnyTransportAccountIface *transport_account;

	g_return_if_fail (modest_ui);

	actions = MODEST_TNY_TRANSPORT_ACTIONS
		(modest_tny_transport_actions_new ());
	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	account_store = priv->account_store;
	transport_accounts =
		tny_account_store_iface_get_transport_accounts (account_store);
	if (!transport_accounts) {
		g_message ("cannot send message: no transport account defined");
		return;
	} else /* take the first one! */
		transport_account =
			TNY_TRANSPORT_ACCOUNT_IFACE(transport_accounts->data);

	to_entry      = glade_xml_get_widget (priv->glade_xml, "to_entry");
	subject_entry = glade_xml_get_widget (priv->glade_xml, "subject_entry");
	body_view     = glade_xml_get_widget (priv->glade_xml, "body_view");

	to      = gtk_entry_get_text (GTK_ENTRY(to_entry));
	subject = gtk_entry_get_text (GTK_ENTRY(subject_entry));

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(body_view));
	gtk_text_buffer_get_bounds (buf, &start, &end);
	body    = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

	g_message ("sending %s ==> %s", subject, to);
	modest_tny_transport_actions_send_message (actions,
						   transport_account,
						   "dirk-jan.binnema@nokia.com",
						   to, "", "", subject,
						   body);
	g_free (body);
	g_object_unref (G_OBJECT(actions));

	gtk_entry_set_text (GTK_ENTRY(to_entry), "");
	gtk_entry_set_text (GTK_ENTRY(subject_entry), "");
	gtk_text_buffer_set_text (buf, "", 0);

	gtk_widget_hide (glade_xml_get_widget (priv->glade_xml, "new_mail"));
}

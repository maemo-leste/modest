/* modest-ui.c */

/* insert (c)/licensing information) */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

/* TODO: put in auto* */
#include <tny-text-buffer-stream.h>
#include <tny-msg-folder.h>

#include "../modest-ui.h"
#include "../modest-window-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-identity-mgr.h"

#include "../modest-tny-account-store.h"
#include "../modest-tny-folder-tree-view.h"
#include "../modest-tny-header-tree-view.h"
#include "../modest-tny-msg-view.h"
#include "../modest-tny-transport-actions.h"
#include "../modest-tny-store-actions.h"

#include "../modest-text-utils.h"
#include "../modest-tny-msg-actions.h"

#include "../modest-editor-window.h"

#include "modest-ui-glade.h"
#include "modest-ui-wizard.h"

/* 'private'/'protected' functions */
static void   modest_ui_class_init     (ModestUIClass *klass);
static void   modest_ui_init           (ModestUI *obj);
static void   modest_ui_finalize       (GObject *obj);

static void    modest_ui_window_destroy    (GtkWidget *win, GdkEvent *event, gpointer data);
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

static void on_forward_clicked (GtkWidget *widget, ModestUI *modest_ui);

static void on_delete_clicked (GtkWidget *widget, ModestUI *modest_ui);

#if 1
static void on_send_button_clicked (GtkWidget *widget, ModestEditorWindow *modest_editwin);
#else
static void on_send_button_clicked (GtkWidget *widget, ModestUI *modest_ui);
#endif

static void on_sendreceive_button_clicked (GtkWidget *widget, ModestUI *modest_ui);

static void register_toolbar_callbacks (ModestUI *modest_ui);

typedef enum {
	QUOTED_SEND_REPLY,
	QUOTED_SEND_REPLY_ALL,
	QUOTED_SEND_FORWARD
} quoted_send_type;

static void quoted_send_msg (ModestUI *modest_ui, quoted_send_type qstype);


typedef struct {
	ModestUI *modest_ui;
	ModestEditorWindow *edit_win;
	GladeXML *glade_xml;
} EditWinData;


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

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
	priv->modest_id_mgr     = NULL;
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

	if (priv->modest_id_mgr)
		g_object_unref (priv->modest_id_mgr);
	priv->modest_id_mgr = NULL;

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
	ModestIdentityMgr *modest_id_mgr;
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

	modest_id_mgr =
		MODEST_IDENTITY_MGR(modest_identity_mgr_new (modest_conf));
	if (!modest_id_mgr) {
		g_warning ("could not create ModestIdentityMgr instance");
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
	priv->glade_xml = glade_xml_new (MODEST_GLADE, NULL,NULL);
	if (!priv->glade_xml) {
		g_warning ("failed to do glade stuff");
		g_object_unref (obj);
		return NULL;
	}

	/* FIXME: could be used, but doesn't work atm.
	 * glade_xml_signal_autoconnect(priv->glade_xml);
	 */

	priv->modest_acc_mgr = modest_acc_mgr;
	priv->modest_id_mgr  = modest_id_mgr;
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
	gint              height, width;
	ModestUIPrivate *priv;
	GtkWidget     *folder_view, *header_view;
	GtkWidget     *message_view;
	GtkWidget     *account_settings_item;
	GtkWidget     *new_account_item;
        GtkWidget     *delete_item;
        GtkWidget     *AccountWizardryMenu;

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
	if (!message_view) {
		g_warning ("failed to create message view");
		return FALSE;
	}
	mail_paned = glade_xml_get_widget (priv->glade_xml, "mail_paned");
	gtk_paned_add2 (GTK_PANED(mail_paned), message_view);

	g_signal_connect (header_view, "message_selected",
			  G_CALLBACK(on_message_clicked),
                          modest_ui);

	account_settings_item = glade_xml_get_widget (priv->glade_xml, "AccountSettingsMenuItem");
	if (!account_settings_item)
	{
		g_warning ("The account settings item isn't available!\n");
                return FALSE;
        }
        g_signal_connect (account_settings_item, "activate",
                          G_CALLBACK(on_account_settings1_activate),
                          modest_ui);

	new_account_item = glade_xml_get_widget (priv->glade_xml, "NewAccountWizardMenuItem");
	if (!new_account_item)
	{
		g_warning ("The new account item isn't available!\n");
		return FALSE;
	}

        g_signal_connect (new_account_item, "activate",
                          G_CALLBACK(new_wizard_account),
                          modest_ui);

	delete_item = glade_xml_get_widget (priv->glade_xml, "delete1");
	if (!delete_item)
	{
		g_warning ("The delete item isn't available!\n");
		return FALSE;
	}

	g_signal_connect (delete_item, "activate", G_CALLBACK(on_delete_clicked),
			  modest_ui);

	register_toolbar_callbacks (modest_ui);

	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_MAIN_WINDOW, 0);
	g_signal_connect (win, "destroy-event", G_CALLBACK(modest_ui_window_destroy),
			  modest_ui);
	g_signal_connect (win, "delete-event", G_CALLBACK(modest_ui_window_destroy),
			  modest_ui);
	gtk_widget_set_usize (GTK_WIDGET(win), width, height);
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
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_reply_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply_all");
	if (button) {
		//g_signal_connect (button, "clicked",
		//		  G_CALLBACK(on_reply_all_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_forward");
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_forward_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_move_to");
	if (button) {
		//g_signal_connect (button, "clicked",
		//		  G_CALLBACK(on_move_to_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_delete");
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_delete_clicked), modest_ui);
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_send_receive");
	if (button) {
		g_signal_connect (button, "clicked",
				  G_CALLBACK(on_sendreceive_button_clicked), modest_ui);
		gtk_widget_set_sensitive(button, TRUE);
	}
}


static void
hide_edit_window (GtkWidget *win, GdkEvent *event, gpointer data)
{
	ModestUIPrivate *priv;

	priv = MODEST_UI_GET_PRIVATE(data);
	gtk_widget_hide (win);
	modest_window_mgr_unregister(priv->modest_window_mgr, G_OBJECT(win));
}


static void
close_edit_window (GtkWidget *win, GdkEvent *event, gpointer data)
{
	ModestEditorWindow *edit_win;
	ModestUI *modest_ui;
	ModestUIPrivate *priv;
	EditWinData *win_data;

	edit_win = (ModestEditorWindow *)data;
	win_data = modest_editor_window_get_data(edit_win);
	priv = MODEST_UI_GET_PRIVATE(win_data->modest_ui);

	gtk_widget_hide (GTK_WIDGET(edit_win));
	modest_window_mgr_unregister(priv->modest_window_mgr, G_OBJECT(edit_win));
	gtk_widget_destroy(GTK_WIDGET(edit_win));
}


GtkContainer
*modest_ui_new_editor_window (ModestUI *modest_ui, gpointer *user_data)
{
	GtkWidget       *top_container, *to_entry, *subject_entry, *body_view;

	ModestUIPrivate *priv;
	GladeXML		*glade_xml;
	GtkWidget       *btn, *dummy;
	GtkTextBuffer	*buf;
	EditWinData		*win_data;

	glade_xml = glade_xml_new(MODEST_GLADE, "new_mail_top_container", NULL);
	if (!glade_xml)
		return NULL;

	win_data = g_malloc(sizeof(EditWinData));
	win_data->modest_ui = modest_ui;
	win_data->glade_xml = glade_xml;
	*user_data = win_data;

	top_container = glade_xml_get_widget(glade_xml, "new_mail_top_container");
	if (!top_container) {
		g_object_unref(G_OBJECT(glade_xml));
		return NULL;
	}

	return GTK_CONTAINER(top_container);
}


gboolean
modest_ui_editor_window_set_to_header(const gchar *to, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;
	w = glade_xml_get_widget(glade_xml, "to_entry");
	gtk_entry_set_text(GTK_ENTRY(w), to);

	return TRUE;
}


gboolean
modest_ui_editor_window_set_cc_header(const gchar *cc, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;
/*
	w = glade_xml_get_widget(glade_xml, "cc_entry");
	gtk_entry_set_text(GTK_ENTRY(w), cc);
*/
	return TRUE;
}


gboolean
modest_ui_editor_window_set_bcc_header(const gchar *bcc, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;
/*
	w = glade_xml_get_widget(glade_xml, "bcc_entry");
	gtk_entry_set_text(GTK_ENTRY(w), bcc);
*/
	return TRUE;
}


gboolean
modest_ui_editor_window_set_subject_header(const gchar *subject, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *w;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;

	w = glade_xml_get_widget(glade_xml, "subject_entry");
	gtk_entry_set_text(GTK_ENTRY(w), subject);

	return TRUE;
}


gboolean
modest_ui_editor_window_set_body(const gchar *body, gpointer window_data)
{
	GladeXML *glade_xml;
	GtkWidget *body_view;
	GtkTextBuffer *buf;
	EditWinData *win_data;

	win_data = (EditWinData *)window_data;
	glade_xml = win_data->glade_xml;

	body_view = glade_xml_get_widget(glade_xml, "body_view");
	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(body_view));

	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buf), body, -1);

	return TRUE;
}


gboolean
modest_ui_new_edit_window (ModestUI *modest_ui, const gchar* to,
			    const gchar* cc, const gchar* bcc,
			    const gchar* subject, const gchar *body,
			    const GSList* att)
{
	GtkWidget       *win, *to_entry, *subject_entry, *body_view;

	ModestUIPrivate *priv;
	GtkWidget       *btn, *dummy;
	GtkTextBuffer	*buf;

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

	/* FIXME: this also assumes that there can be only one edit window! */
	if (!modest_window_mgr_find_by_type(priv->modest_window_mgr, MODEST_EDIT_WINDOW)) {
		/* there already is one edit win, maybe we should preserver its contents */
		modest_window_mgr_register (priv->modest_window_mgr,
									G_OBJECT(win), MODEST_EDIT_WINDOW, 0);
	}

	to_entry      = glade_xml_get_widget (priv->glade_xml, "to_entry");
	subject_entry = glade_xml_get_widget (priv->glade_xml, "subject_entry");
	body_view     = glade_xml_get_widget (priv->glade_xml, "body_view");

	gtk_entry_set_text(GTK_ENTRY(subject_entry), subject);
	gtk_entry_set_text(GTK_ENTRY(to_entry), to);

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(body_view));
	if (body) {
		gtk_text_buffer_set_text(buf, body, -1);
	} else {
		gtk_text_buffer_set_text(buf, "", -1);
	}
	g_signal_connect (win, "destroy-event", G_CALLBACK(hide_edit_window),
			  modest_ui);
	g_signal_connect (win, "delete-event", G_CALLBACK(hide_edit_window),
			  modest_ui);

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
modest_ui_window_destroy (GtkWidget *win, GdkEvent *event, gpointer data)
{
	ModestUIPrivate *priv;

	g_return_if_fail (data);
	g_return_if_fail(MODEST_IS_UI(data));
	priv = MODEST_UI_GET_PRIVATE((ModestUI *)data);
	g_return_if_fail(priv);
	if (!modest_window_mgr_unregister (priv->modest_window_mgr, G_OBJECT(win)))
		g_warning ("modest window mgr: failed to unregister %p",
			   G_OBJECT(win));
	else
		gtk_widget_hide(win);
}


static void
modest_ui_last_window_closed (GObject *obj, gpointer data)
{
	/* FIXME: Other cleanups todo? Finalize Tinymail? */
	gtk_main_quit ();
}


void
on_account_selector_selection_changed (GtkWidget *widget,
				       gpointer user_data)
{
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	GtkTreeIter iter;

	gchar *account_name;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(model),
				   &iter,
				   0, &account_name,
				   -1);
	}
	else
	{
		account_name="empty";
	}

	g_message("Value: '%s'\n", account_name);

	free(account_name);
}


static void
on_account_settings1_activate (GtkMenuItem *menuitem,
			       gpointer user_data)
{
	GladeXML *glade_xml;
	GtkWidget *advanced_account_setup;
	ModestUIPrivate *priv;
	gint retval;
	GSList *account_name_list;
	GSList *account_name_list_iter;
	GtkListStore *account_names;
	GtkTreeIter account_names_iter;
	GtkWidget *account_selector;
	GtkCellRenderer *renderer;


        g_return_if_fail(MODEST_IS_UI(user_data));
	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(user_data));

	glade_xml = glade_xml_new(MODEST_GLADE, "mailbox_setup_advanced", NULL);
	advanced_account_setup = glade_xml_get_widget(glade_xml, "mailbox_setup_advanced");

	account_name_list=modest_account_mgr_account_names(priv->modest_acc_mgr, NULL);
	account_names = gtk_list_store_new(1, G_TYPE_STRING);

	for (account_name_list_iter=account_name_list;
	     account_name_list_iter!=NULL;
	     account_name_list_iter=g_slist_next(account_name_list_iter))
	{
		gtk_list_store_append(account_names, &account_names_iter);
		gtk_list_store_set(account_names, &account_names_iter,
				   0, account_name_list_iter->data,
				   -1);
	}

	g_slist_free(account_name_list);

	account_selector = glade_xml_get_widget(glade_xml, "account_selector");
	gtk_combo_box_set_model(GTK_COMBO_BOX(account_selector), GTK_TREE_MODEL(account_names));

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (account_selector), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (account_selector), renderer,
					"text", 0,
					NULL);

	g_signal_connect(GTK_WIDGET(account_selector), "changed",
			 G_CALLBACK(on_account_selector_selection_changed),
			 GTK_WIDGET(advanced_account_setup));

	gtk_combo_box_set_active(GTK_COMBO_BOX(account_selector), 0);

	gtk_widget_show_all(GTK_WIDGET(advanced_account_setup));

	retval=gtk_dialog_run(GTK_DIALOG(advanced_account_setup));

	g_object_unref(account_names);

	gtk_widget_destroy(GTK_WIDGET(advanced_account_setup));

	g_object_unref(glade_xml);
}


static void
on_folder_clicked (ModestTnyFolderTreeView *folder_tree,
		   TnyMsgFolderIface *folder,
		   gpointer data)
{
	GtkWidget *win;
	GtkWidget *button;
	GtkWidget *paned;
	ModestTnyHeaderTreeView *tree_view;
	ModestTnyMsgView *msg_view;
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

	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply");
	if (button) {
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_forward");
	if (button) {
		gtk_widget_set_sensitive(button, FALSE);
	}

	button = glade_xml_get_widget (priv->glade_xml, "toolb_delete");
	if (button) {
		gtk_widget_set_sensitive(button, FALSE);
	}

	paned = glade_xml_get_widget (priv->glade_xml,"mail_paned");
	g_return_if_fail (paned);

	msg_view = MODEST_TNY_MSG_VIEW(gtk_paned_get_child2 (GTK_PANED(paned)));
	g_return_if_fail (msg_view);

	modest_tny_msg_view_set_message  (msg_view, NULL);
}


static void
on_message_clicked (ModestTnyFolderTreeView *folder_tree,
				TnyMsgIface *message,
				gpointer data)
{
	GtkWidget *paned;
	GtkWidget *button;
	ModestTnyMsgView *msg_view;
	ModestUIPrivate *priv;

	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE(data);
	paned = glade_xml_get_widget (priv->glade_xml,"mail_paned");
	msg_view = MODEST_TNY_MSG_VIEW(gtk_paned_get_child2 (GTK_PANED(paned)));

	modest_tny_msg_view_set_message (msg_view,
					 message);
	button = glade_xml_get_widget (priv->glade_xml, "toolb_reply");
	if (button) {
		gtk_widget_set_sensitive(button, TRUE);
	}
	button = glade_xml_get_widget (priv->glade_xml, "toolb_forward");
	if (button) {
		gtk_widget_set_sensitive(button, TRUE);
	}
	button = glade_xml_get_widget (priv->glade_xml, "toolb_delete");
	if (button) {
		gtk_widget_set_sensitive(button, TRUE);
	}
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
	int i;
	GSList *columns = NULL;
	GtkWidget *header_tree;
	ModestTnyHeaderTreeViewColumn cols[] = {
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_MSGTYPE,
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_ATTACH,
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_COMPACT_HEADER
	};

	for (i = 0 ; i != sizeof(cols)/sizeof(ModestTnyHeaderTreeViewColumn); ++i)
		columns = g_slist_append (columns, GINT_TO_POINTER(cols[i]));

	header_tree = GTK_WIDGET(modest_tny_header_tree_view_new(folder, columns,
								 MODEST_TNY_HEADER_TREE_VIEW_STYLE_NORMAL));
	g_slist_free (columns);

	if (!header_tree) {
		g_warning ("could not create header tree");
		return NULL;
	}

	return GTK_WIDGET(header_tree);
}


static GtkWidget*
modest_main_window_folder_tree (ModestAccountMgr *modest_acc_mgr,
				TnyAccountStoreIface *account_store)
{
	GtkWidget *folder_tree;

	folder_tree = GTK_WIDGET(modest_tny_folder_tree_view_new (account_store));
	if (!folder_tree) {
		g_warning ("could not create folder list");
		return NULL;
	}

	return folder_tree;
}


static void
on_new_mail_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	GtkWidget *edit_win;
	GladeXML *glade_xml;
	GtkWidget *btn;
	EditWinData *windata;
	ModestUIPrivate *priv;
	gint height, width;

	g_return_if_fail (modest_ui);
	//modest_ui_new_edit_window (modest_ui, "", "", "", "", "", NULL);

	edit_win = modest_editor_window_new(modest_ui);
	windata = (EditWinData *)modest_editor_window_get_data(MODEST_EDITOR_WINDOW(edit_win));
	g_return_if_fail(windata);

	glade_xml = windata->glade_xml;
	btn = glade_xml_get_widget (glade_xml, "toolb_send");
	g_signal_connect (btn, "clicked", G_CALLBACK(on_send_button_clicked),
			  edit_win);

	g_signal_connect (edit_win, "destroy-event", G_CALLBACK(close_edit_window),
			  edit_win);
	g_signal_connect (edit_win, "delete-event", G_CALLBACK(close_edit_window),
			  edit_win);

	priv = MODEST_UI_GET_PRIVATE(windata->modest_ui);
	height = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_HEIGHT, NULL);
	width  = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_WIDTH, NULL);

	// g_message("new editor win@%dx%d", width, height);

	gtk_widget_set_usize (GTK_WIDGET(edit_win), width, height);
	gtk_window_set_title (GTK_WINDOW(edit_win), _("Untitled"));
	modest_window_mgr_register(priv->modest_window_mgr, G_OBJECT(edit_win), MODEST_EDIT_WINDOW, 0);
	gtk_widget_show(edit_win);
}


static void
new_editor_with_presets (ModestUI *modest_ui, const gchar *to_header,
							const gchar *cc_header, const gchar *bcc_header,
							const gchar *subject_header, const gchar *body)
{
	GtkWidget *edit_win;
	GladeXML *glade_xml;
	GtkWidget *btn;
	EditWinData *windata;
	ModestUIPrivate *priv;
	gint height, width;

	g_return_if_fail (modest_ui);

	edit_win = modest_editor_window_new(modest_ui);
	windata = (EditWinData *)modest_editor_window_get_data(MODEST_EDITOR_WINDOW(edit_win));
	g_return_if_fail(windata);

	glade_xml = windata->glade_xml;
	btn = glade_xml_get_widget (glade_xml, "toolb_send");
	g_signal_connect (btn, "clicked", G_CALLBACK(on_send_button_clicked),
			  edit_win);

	g_signal_connect (edit_win, "destroy-event", G_CALLBACK(close_edit_window),
			  edit_win);
	g_signal_connect (edit_win, "delete-event", G_CALLBACK(close_edit_window),
			  edit_win);

	priv = MODEST_UI_GET_PRIVATE(windata->modest_ui);
	height = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_HEIGHT, NULL);
	width  = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_WIDTH, NULL);

	// g_message("new editor win@%dx%d", width, height);

	gtk_widget_set_usize (GTK_WIDGET(edit_win), width, height);
	gtk_window_set_title (GTK_WINDOW(edit_win), _("Untitled"));
	modest_window_mgr_register(priv->modest_window_mgr, G_OBJECT(edit_win), MODEST_EDIT_WINDOW, 0);

	modest_editor_window_set_to_header(MODEST_EDITOR_WINDOW(edit_win), to_header);
	modest_editor_window_set_cc_header(MODEST_EDITOR_WINDOW(edit_win), cc_header);
	modest_editor_window_set_bcc_header(MODEST_EDITOR_WINDOW(edit_win), bcc_header);
	modest_editor_window_set_subject_header(MODEST_EDITOR_WINDOW(edit_win), subject_header);
	modest_editor_window_set_body(MODEST_EDITOR_WINDOW(edit_win), body);

	gtk_widget_show(edit_win);
}


static void
quoted_send_msg (ModestUI *modest_ui, quoted_send_type qstype) {
	GtkTreeSelection *sel;
	GtkWidget *paned;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkScrolledWindow *scroll;

	TnyMsgHeaderIface *header;

	ModestTnyHeaderTreeView *header_view;
	ModestTnyMsgView *msg_view;
	ModestUIPrivate *priv;

	const TnyMsgIface *msg;
	const TnyMsgFolderIface *folder;
	GString *re_sub;
	const gchar *subject, *from;
	gchar *unquoted, *quoted;
	time_t sent_date;
	gint line_limit = 76;

	g_return_if_fail (modest_ui);

	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	paned = glade_xml_get_widget (priv->glade_xml,"mail_paned");
	g_return_if_fail (paned);

	scroll = GTK_SCROLLED_WINDOW(gtk_paned_get_child1 (GTK_PANED(paned)));
	g_return_if_fail (scroll);

	msg_view = MODEST_TNY_MSG_VIEW(gtk_paned_get_child2 (GTK_PANED(paned)));
	g_return_if_fail (msg_view);

	header_view = MODEST_TNY_HEADER_TREE_VIEW(gtk_bin_get_child (GTK_BIN(scroll)));
	g_return_if_fail (header_view);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(header_view));
	g_return_if_fail (sel);

	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		g_warning("nothing to reply to");
		return;
	}

	gtk_tree_model_get (model, &iter,
			    TNY_MSG_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);

	if (!header) {
		g_warning("no header");
		return;
	}

	folder = tny_msg_header_iface_get_folder (TNY_MSG_HEADER_IFACE(header));
	if (!folder) {
		g_warning ("cannot find folder");
		return;
	}

	msg = tny_msg_folder_iface_get_message (TNY_MSG_FOLDER_IFACE(folder), header);
	if (!msg) {
		g_warning ("cannot find msg");
		return;
	}

	subject = tny_msg_header_iface_get_subject(header);
	re_sub = g_string_new(subject);
	/* FIXME: honor replyto, cc */
	from = tny_msg_header_iface_get_from(header);
	sent_date = tny_msg_header_iface_get_date_sent(header);

	unquoted = modest_tny_msg_view_get_selected_text(msg_view);
	quoted = modest_tny_msg_actions_quote(msg, from, sent_date, line_limit, unquoted);

	switch (qstype) {
		case QUOTED_SEND_REPLY:
			g_string_prepend(re_sub, _("Re: "));
			// modest_ui_new_edit_window (modest_ui, from, /* cc */ "", /* bcc */ "", re_sub->str, quoted, NULL);
			new_editor_with_presets(modest_ui, from, /* cc */ "", /* bcc */ "", re_sub->str, quoted);
			break;
		case QUOTED_SEND_FORWARD:
			g_string_prepend(re_sub, _("Fwd: "));
			// modest_ui_new_edit_window (modest_ui, /* from */ "", /* cc */ "", /* bcc */ "", re_sub->str, quoted, NULL);
			new_editor_with_presets(modest_ui, /* from */ "", /* cc */ "", /* bcc */ "", re_sub->str, quoted);
			break;
		default:
			break;
	}
	g_free(quoted);
	g_free(unquoted);
	g_string_free(re_sub, TRUE);
}


static void
on_reply_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	quoted_send_msg (modest_ui, QUOTED_SEND_REPLY);
}


static void
on_forward_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	quoted_send_msg (modest_ui, QUOTED_SEND_FORWARD);
}


static void
on_send_button_clicked (GtkWidget *widget, ModestEditorWindow *modest_editwin)
{
	ModestTnyTransportActions *actions;
	ModestUI *modest_ui;
	ModestUIPrivate *priv;
	GtkWidget *to_entry, *subject_entry, *body_view;
	const gchar *to, *subject, *email_from;
	gchar *body;
	GtkTextIter start, end;
	GtkTextBuffer *buf;
	TnyAccountStoreIface *account_store;
	const GList *transport_accounts;
	TnyTransportAccountIface *transport_account;
	ModestConf       *conf;
	ModestIdentityMgr *id_mgr;
	EditWinData *win_data;


	win_data = modest_editor_window_get_data(modest_editwin);
	modest_ui = win_data->modest_ui;

	g_return_if_fail (modest_ui);

	actions = MODEST_TNY_TRANSPORT_ACTIONS
		(modest_tny_transport_actions_new ());

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
#if 0
	account_store = priv->account_store;
	transport_accounts =
		tny_account_store_iface_get_transport_accounts (account_store);
	if (!transport_accounts) {
		g_message ("cannot send message: no transport account defined");
		return;
	} else /* take the first one! */
		transport_account =
			TNY_TRANSPORT_ACCOUNT_IFACE(transport_accounts->data);
#endif
	to_entry      = glade_xml_get_widget (win_data->glade_xml, "to_entry");
	subject_entry = glade_xml_get_widget (win_data->glade_xml, "subject_entry");
	body_view     = glade_xml_get_widget (win_data->glade_xml, "body_view");

	to      = gtk_entry_get_text (GTK_ENTRY(to_entry));
	subject = gtk_entry_get_text (GTK_ENTRY(subject_entry));

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(body_view));
	gtk_text_buffer_get_bounds (buf, &start, &end);
	body    = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

	id_mgr = priv->modest_id_mgr;
	email_from = modest_identity_mgr_get_identity_string(id_mgr,
							     MODEST_IDENTITY_DEFAULT_IDENTITY,
							     MODEST_IDENTITY_EMAIL, NULL);

	g_message("sending \"%s\" %s ==> %s", subject, email_from, to);
/*
	modest_tny_transport_actions_send_message (actions,
						   transport_account,
						   email_from,
						   to, "", "", subject,
						   body);
*/
	g_free (body);
	g_object_unref (G_OBJECT(actions));

	gtk_widget_hide (GTK_WIDGET(modest_editwin));
	modest_window_mgr_unregister(priv->modest_window_mgr, G_OBJECT(modest_editwin));
	if (GTK_IS_WIDGET(modest_editwin)) {
		gtk_widget_destroy(GTK_WIDGET(modest_editwin));
	} else
		g_warning("editor window has vanished!");
}


static void
on_delete_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	GtkTreeSelection *sel;
	GtkWidget *paned;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkScrolledWindow *scroll;
	GtkTreeModel *mymodel, *sortable;

	ModestTnyHeaderTreeView *header_view;
	ModestTnyMsgView *msg_view;
	ModestUIPrivate *priv;

	g_return_if_fail (modest_ui);

	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	paned = glade_xml_get_widget (priv->glade_xml,"mail_paned");
	g_return_if_fail (paned);

	scroll = GTK_SCROLLED_WINDOW(gtk_paned_get_child1 (GTK_PANED(paned)));
	g_return_if_fail (scroll);

	msg_view = MODEST_TNY_MSG_VIEW(gtk_paned_get_child2 (GTK_PANED(paned)));
	g_return_if_fail (msg_view);

	header_view = MODEST_TNY_HEADER_TREE_VIEW(gtk_bin_get_child (GTK_BIN(scroll)));
	g_return_if_fail (header_view);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(header_view));
	g_return_if_fail (sel);

	/* get all selected mails */
	if (G_LIKELY (gtk_tree_selection_get_selected (sel, &model, &iter)))
	{
		TnyMsgHeaderIface *header;

		gtk_tree_model_get (model, &iter, TNY_MSG_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			&header, -1);

		if (G_LIKELY (header))
		{
			TnyMsgFolderIface *folder;
			const TnyMsgIface *msg;

			if (GTK_IS_TREE_MODEL_SORT (model))
			{
				mymodel = gtk_tree_model_sort_get_model
					(GTK_TREE_MODEL_SORT (model));
			} else
				mymodel = model;

			folder = (TnyMsgFolderIface*)tny_msg_header_iface_get_folder (header);

			/* this will make the message as deleted */
			/* 	tny_msg_folder_iface_expunge (folder); will finally delete messages */
			if (TNY_IS_MSG_FOLDER (folder))
				tny_msg_folder_iface_remove_message (folder, header);
			gtk_widget_queue_draw (GTK_WIDGET (header_view));
		}
	}
}

static void
on_sendreceive_button_clicked (GtkWidget *widget, ModestUI *modest_ui)
{
	ModestUIPrivate *priv;
	ModestTnyStoreActions *store_actions;
	TnyAccountStoreIface *account_store;
	const GList *store_accounts;
	const GList *iter;

	g_return_if_fail (modest_ui);

	store_actions = MODEST_TNY_STORE_ACTIONS (modest_tny_store_actions_new ());
	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	account_store = priv->account_store;
	store_accounts =
		tny_account_store_iface_get_store_accounts (account_store);

	for (iter = store_accounts; iter; iter = iter->next)
		modest_tny_store_actions_update_folders (store_actions,
												 TNY_STORE_ACCOUNT_IFACE (iter->data));

	/* TODO, lock, refresh display */

	g_object_unref (store_actions);

}

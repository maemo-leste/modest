/* modest-ui-wizard.c */

/* insert (c)/licensing information) */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include "../modest-account-mgr.h"
#include "../modest-identity-mgr.h"

#include "modest-ui-glade.h"
#include "modest-ui-account-setup.h"

enum {
	IDENTITY_NAME,
	IDENTITY_ADDRESS,
	IDENTITY_COLUMNS
};

enum {
	ACCOUNT_NAME,
	ACCOUNT_HOST,
	ACCOUNT_COLUMNS
};

static GtkTreeModel *
create_identities_model(ModestIdentityMgr *id_mgr) {

	GSList *id_names_list;
	GSList *id_names_list_iter;
	GtkListStore *id_list_store;
	GtkTreeIter id_list_store_iter;
	gchar *tmptext;

	id_names_list = modest_identity_mgr_identity_names(id_mgr, NULL);
	id_list_store = gtk_list_store_new(IDENTITY_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	for (id_names_list_iter=id_names_list;
	     id_names_list_iter!=NULL;
	     id_names_list_iter=g_slist_next(id_names_list_iter)) {
		gtk_list_store_append(id_list_store, &id_list_store_iter);
		tmptext=modest_identity_mgr_get_identity_string(id_mgr,
								id_names_list_iter->data,
								"email",
								NULL);
		gtk_list_store_set(id_list_store, &id_list_store_iter,
				   IDENTITY_NAME, id_names_list_iter->data,
				   IDENTITY_ADDRESS, tmptext,
				   -1);
		g_free(tmptext);
	}

	g_slist_free(id_names_list);

	return GTK_TREE_MODEL(id_list_store);
}

static GtkTreeModel *
create_accounts_model(ModestAccountMgr *acc_mgr) {

	GSList *acc_names_list;
	GSList *acc_names_list_iter;
	GtkListStore *acc_list_store;
	GtkTreeIter acc_list_store_iter;
	gchar *tmptext;

	acc_names_list = modest_account_mgr_server_account_names(acc_mgr,
								 NULL,
								 MODEST_PROTO_TYPE_ANY,
								 NULL,
								 FALSE);
	acc_list_store = gtk_list_store_new(ACCOUNT_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	for (acc_names_list_iter=acc_names_list;
	     acc_names_list_iter!=NULL;
	     acc_names_list_iter=g_slist_next(acc_names_list_iter)) {
		gtk_list_store_append(acc_list_store, &acc_list_store_iter);
		tmptext=modest_account_mgr_get_server_account_string(acc_mgr,
								     acc_names_list_iter->data,
								     "hostname",
								     NULL);
		gtk_list_store_set(acc_list_store, &acc_list_store_iter,
				   ACCOUNT_NAME, acc_names_list_iter->data,
				   ACCOUNT_HOST, tmptext,
				   -1);
		g_free(tmptext);
		g_message("Debug");
	}

	g_slist_free(acc_names_list);

	return GTK_TREE_MODEL(acc_list_store);
}


static void
accounts_and_identities_dialog (gpointer user_data)
{
	GladeXML *glade_xml;
	GtkWidget *main_dialog;
	GtkWidget *identities_tree_view;
	GtkWidget *accounts_tree_view;
	GtkTreeModel *identities_model;
	GtkTreeModel *accounts_model;
	ModestUIPrivate *priv;
	gint retval;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

        g_return_if_fail(MODEST_IS_UI(user_data));
	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(user_data));

	glade_xml = glade_xml_new(MODEST_GLADE, "IdentitiesAndAccountsDialog", NULL);
	main_dialog = glade_xml_get_widget(glade_xml, "IdentitiesAndAccountsDialog");

	accounts_tree_view = glade_xml_get_widget(glade_xml, "AccountsTreeview");
	accounts_model=create_accounts_model(priv->modest_acc_mgr);
	gtk_tree_view_set_model(GTK_TREE_VIEW(accounts_tree_view), accounts_model);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Account",
							   renderer,
							   "text", ACCOUNT_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(accounts_tree_view), column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Hostname",
							   renderer,
							   "text", ACCOUNT_HOST,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(accounts_tree_view), column);

	identities_tree_view = glade_xml_get_widget(glade_xml, "IdentitiesTreeview");
	identities_model=create_identities_model(priv->modest_id_mgr);
	gtk_tree_view_set_model(GTK_TREE_VIEW(identities_tree_view), identities_model);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Identity",
							   renderer,
							   "text", IDENTITY_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(identities_tree_view), column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("E-mail address",
							   renderer,
							   "text", IDENTITY_ADDRESS,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(identities_tree_view), column);

	gtk_widget_show_all(GTK_WIDGET(main_dialog));

	while (TRUE) {
		retval=gtk_dialog_run(GTK_DIALOG(main_dialog));
		if (retval==GTK_RESPONSE_CANCEL)
			break;
	}

	gtk_widget_destroy(GTK_WIDGET(main_dialog));
	g_object_unref(glade_xml);
}

void account_settings (GtkWidget *widget,
                       gpointer user_data)
{
	accounts_and_identities_dialog (MODEST_UI(user_data));
}


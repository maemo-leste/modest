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
	IDENTITY_VIA,
	IDENTITY_COLUMNS
};

enum {
	ACCOUNT_NAME,
	ACCOUNT_HOST,
	ACCOUNT_COLUMNS
};

typedef struct _CallbackData CallbackData;

struct _CallbackData {
	GtkWidget       *id_tree_view;
	GtkWidget       *acc_tree_view;
	ModestUI        *modest_ui;
	GladeXML        *glade_xml;
};

static void
identity_setup_dialog (ModestUI *, GtkListStore *, gchar *);

static void
missing_notification(GtkWindow *, gchar *);

/* CALLBACKS */

static void
edit_action(GtkWidget *button,
	    gpointer userdata)
{
	CallbackData *cb_data;
	GtkListStore *acc_liststore;
	GtkTreeModel *id_liststore;
	GtkTreeSelection *selection;
	GtkTreeIter selected_iter;
	gchar *identity_name;

	cb_data = (CallbackData *) userdata;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(cb_data->id_tree_view));

	gtk_tree_selection_get_selected(selection,
					&id_liststore,
					&selected_iter);
	gtk_tree_model_get(GTK_TREE_MODEL(id_liststore),
			   &selected_iter,
			   ACCOUNT_NAME, &identity_name,
			   -1);
	acc_liststore=GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(cb_data->acc_tree_view)));

	identity_setup_dialog (cb_data->modest_ui, acc_liststore, identity_name);
	g_free(identity_name);
}

static void
create_action(GtkWidget *button,
	      gpointer userdata)
{
	CallbackData *cb_data;
	GtkListStore *acc_liststore;

	cb_data = (CallbackData *) userdata;

	acc_liststore=GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(cb_data->acc_tree_view)));

	identity_setup_dialog(cb_data->modest_ui, acc_liststore, NULL);
}

static void
delete_action(GtkWidget *button,
	      gpointer userdata)
{
	CallbackData *cb_data;
	GtkTreeSelection *selection;
	GtkTreeIter selected_iter;
	GtkTreeModel *id_liststore;
	gchar *identity_name;
	GtkWidget *confirmation_dialog;
	GtkWidget *confirmation_message;
	gint result;

	cb_data = (CallbackData *) userdata;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(cb_data->id_tree_view));

	gtk_tree_selection_get_selected(selection,
					&id_liststore,
					&selected_iter);
	gtk_tree_model_get(GTK_TREE_MODEL(id_liststore),
			   &selected_iter,
			   ACCOUNT_NAME, &identity_name,
			   -1);

	confirmation_dialog = gtk_dialog_new_with_buttons ("Confirm removal of identity",
							   GTK_WINDOW(gtk_widget_get_toplevel(button)),
							   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							   GTK_STOCK_OK,
							   GTK_RESPONSE_OK,
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL,
							   NULL);

	gchar *messy = g_strdup_printf("Remove selected identity '%s'?", identity_name);
	confirmation_message = gtk_label_new_with_mnemonic (messy);

	gtk_container_add(GTK_CONTAINER(confirmation_dialog),
			  confirmation_message);

	gtk_widget_show_all(confirmation_dialog);

	result=gtk_dialog_run(GTK_DIALOG(confirmation_dialog));
	if (result==GTK_RESPONSE_OK)
	{
		modest_identity_mgr_remove_identity(MODEST_UI_GET_PRIVATE(cb_data->modest_ui)->modest_id_mgr,
						    identity_name,
						    NULL);
	}
	gtk_widget_destroy(confirmation_dialog);
	g_free(identity_name);
	g_free(messy);

}

static void
activate_buttons_on_identity(GtkTreeView *tree_view,
			     gpointer user_data)
{
	GtkWidget *button;
	GladeXML *glade_xml;

	glade_xml=(GladeXML *) user_data;

	button = glade_xml_get_widget(GLADE_XML(glade_xml), "IdentityEditButton");
	gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
	button = glade_xml_get_widget(GLADE_XML(glade_xml), "IdentityDeleteButton");
	gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
}

static void
use_sig_toggled(GtkToggleButton *button,
		gpointer userdata) {

	GtkWidget *awidget;
	GladeXML *glade_xml = (GladeXML *) userdata;

	awidget=glade_xml_get_widget(glade_xml, "ISSignatureTextView");
	gtk_widget_set_sensitive(awidget, gtk_toggle_button_get_active(button));
}

/* METHODS */

static void
missing_notification(GtkWindow *parent, gchar *info_message) {
	GtkWidget *DenyDialog;

	DenyDialog=gtk_message_dialog_new(parent,
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_INFO,
					  GTK_BUTTONS_OK,
					  "%s",
					  info_message);

	gtk_dialog_run(GTK_DIALOG(DenyDialog));

	gtk_widget_destroy(DenyDialog);
}

static gboolean
add_identity(GladeXML *glade_xml, ModestUI *modest_ui, GtkListStore *accounts_model)
{
	gchar *reply_to;
	GtkTextBuffer *sigbuff;
	gchar *signature;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	GtkTreeIter transport_iter;
	ModestUIPrivate *priv;
	gchar *transport;
	gint retval;

	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(modest_ui));

	reply_to = g_strdup_printf("%s", gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISReplyToEntry"))));

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(glade_xml, "ISUseSignatureCheckButton"))))
	{
		sigbuff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(glade_xml_get_widget(glade_xml, "ISSignatureTextView")));
		gtk_text_buffer_get_bounds(sigbuff,
					   &start_iter,
					   &end_iter);
		signature = gtk_text_buffer_get_text(sigbuff,
						     &start_iter,
						     &end_iter,
						     FALSE);
	}
	else
		signature = NULL;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(glade_xml, "ISOutServerComboBox")), &transport_iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(accounts_model),
				   &transport_iter,
				   ACCOUNT_NAME, &transport,
				   -1);
	}
	else {
		missing_notification(NULL, "Please select an outgoing server!");
		return FALSE;
	}

	if (modest_identity_mgr_add_identity (priv->modest_id_mgr,
					      gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISIdentityEntry"))),
					      gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISNameEntry"))),
					      gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISEMailAddress"))),
					      reply_to,
					      signature,
					      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(glade_xml, "ISUseSignatureCheckButton"))),
					      transport,
					      FALSE ))
		retval=TRUE;
	else
		retval=FALSE;
	g_free(transport);
	return retval;
}

static void
identity_setup_dialog (ModestUI *modest_ui, GtkListStore *accounts_model, gchar *identity)
{

	GladeXML *glade_xml;
	GtkWidget *id_dialog;
	GtkWidget *outgoing_server;
	GtkWidget *awidget;
	GtkCellRenderer *renderer;
	ModestIdentityMgr *id_mgr;
	GtkTextBuffer *sigbuff;
	gchar *tmptext;
	gint account_added_successfully;
	gint result;

	glade_xml = glade_xml_new(MODEST_GLADE, "IdentitySetupDialog", NULL);
	id_dialog = glade_xml_get_widget(glade_xml, "IdentitySetupDialog");

	outgoing_server = glade_xml_get_widget(glade_xml, "ISOutServerComboBox");
	gtk_combo_box_set_model(GTK_COMBO_BOX(outgoing_server), GTK_TREE_MODEL(accounts_model));
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (outgoing_server), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (outgoing_server), renderer,
					"text", 0,
					NULL);
	awidget=glade_xml_get_widget(glade_xml, "ISUseSignatureCheckButton");
	g_signal_connect(awidget,
			 "toggled",
			 G_CALLBACK(use_sig_toggled),
			 glade_xml);

	if (identity!=NULL) {
		id_mgr = MODEST_UI_GET_PRIVATE(modest_ui)->modest_id_mgr;

		awidget=glade_xml_get_widget(glade_xml, "ISIdentityEntry");
		gtk_widget_set_sensitive(awidget, FALSE);
		gtk_entry_set_text(GTK_ENTRY(awidget), identity);
		tmptext = modest_identity_mgr_get_identity_string(id_mgr,
								  identity,
								  MODEST_IDENTITY_EMAIL,
								  NULL);
		awidget=glade_xml_get_widget(glade_xml, "ISEMailAddress");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);
		g_free(tmptext);

		if (modest_identity_mgr_get_identity_bool(id_mgr,
							  identity,
							  MODEST_IDENTITY_USE_SIGNATURE,
							  NULL)) {
			awidget=glade_xml_get_widget(glade_xml, "ISUseSignatureCheckButton");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(awidget), TRUE);
			awidget=glade_xml_get_widget(glade_xml, "ISSignatureTextView");
			gtk_widget_set_sensitive(awidget, TRUE);
		}

		sigbuff=gtk_text_buffer_new(NULL);
		tmptext = modest_identity_mgr_get_identity_string(id_mgr,
								  identity,
								  MODEST_IDENTITY_SIGNATURE,
								  NULL),
		gtk_text_buffer_set_text(sigbuff, tmptext, -1);
		gtk_text_view_set_buffer(GTK_TEXT_VIEW(glade_xml_get_widget(glade_xml, "ISSignatureTextView")),
						       sigbuff);
		g_object_unref(sigbuff);
		g_free(tmptext);

		tmptext = modest_identity_mgr_get_identity_string(id_mgr,
								  identity,
								  MODEST_IDENTITY_EMAIL,
								  NULL);
		awidget=glade_xml_get_widget(glade_xml, "ISEMailAddress");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);
		g_free(tmptext);

		tmptext = modest_identity_mgr_get_identity_string(id_mgr,
								  identity,
								  MODEST_IDENTITY_REALNAME,
								  NULL);
		awidget=glade_xml_get_widget(glade_xml, "ISNameEntry");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);
		g_free(tmptext);

		tmptext = modest_identity_mgr_get_identity_string(id_mgr,
								  identity,
								  MODEST_IDENTITY_REALNAME,
								  NULL);
		awidget=glade_xml_get_widget(glade_xml, "ISNameEntry");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);

		g_free(tmptext);

		tmptext = modest_identity_mgr_get_identity_string(id_mgr,
								  identity,
								  MODEST_IDENTITY_REPLYTO,
								  NULL);
		awidget=glade_xml_get_widget(glade_xml, "ISReplyToEntry");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);

		g_free(tmptext);
	}

	gtk_widget_show_all(id_dialog);

	do {
		result=gtk_dialog_run(GTK_DIALOG(id_dialog));

		switch (result) {
		case GTK_RESPONSE_OK:
			account_added_successfully = add_identity(glade_xml, modest_ui, accounts_model);
			break;
		default:
			account_added_successfully = FALSE;
			break;
		}
	} while(result!=GTK_RESPONSE_DELETE_EVENT && result!=GTK_RESPONSE_CANCEL && account_added_successfully!=TRUE);

	gtk_widget_destroy(id_dialog);
	g_object_unref(glade_xml);
}


static CallbackData *
setup_callback_data(GtkWidget *id_tree_view,
		    GtkWidget *acc_tree_view,
		    ModestUI *modest_ui)
{
	CallbackData *self;
	self = g_malloc(sizeof(CallbackData));
	self->modest_ui=modest_ui;
	self->id_tree_view=id_tree_view;
	self->acc_tree_view=acc_tree_view;
	return self;
}

static void
free_callback_data(CallbackData *data)
{
	g_free(data);
}

static GtkTreeModel *
create_identities_model(ModestIdentityMgr *id_mgr) {

	GSList *id_names_list;
	GSList *id_names_list_iter;
	GtkListStore *id_list_store;
	GtkTreeIter id_list_store_iter;
	gchar *tmptext1;
	gchar *tmptext2;

	id_names_list = modest_identity_mgr_identity_names(id_mgr, NULL);
	id_list_store = gtk_list_store_new(IDENTITY_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	for (id_names_list_iter=id_names_list;
	     id_names_list_iter!=NULL;
	     id_names_list_iter=g_slist_next(id_names_list_iter)) {
		gtk_list_store_append(id_list_store, &id_list_store_iter);
		tmptext1=modest_identity_mgr_get_identity_string(id_mgr,
								 id_names_list_iter->data,
								 MODEST_IDENTITY_EMAIL,
								 NULL);
		tmptext2=modest_identity_mgr_get_identity_string(id_mgr,
								 id_names_list_iter->data,
								 MODEST_IDENTITY_ID_VIA,
								 NULL);
		gtk_list_store_set(id_list_store, &id_list_store_iter,
				   IDENTITY_NAME, id_names_list_iter->data,
				   IDENTITY_ADDRESS, tmptext1,
				   IDENTITY_VIA, tmptext2,
				   -1);
		g_free(tmptext1);
		g_free(tmptext2);
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
								     MODEST_ACCOUNT_HOSTNAME,
								     NULL);
		gtk_list_store_set(acc_list_store, &acc_list_store_iter,
				   ACCOUNT_NAME, acc_names_list_iter->data,
				   ACCOUNT_HOST, tmptext,
				   -1);
		g_free(tmptext);
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
	ModestUI *modest_ui;
	ModestUIPrivate *priv;
	gint retval;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	CallbackData *cb_data;
	GtkWidget *abutton;

	g_return_if_fail(MODEST_IS_UI(user_data));
	modest_ui = (ModestUI *) user_data;
	priv = MODEST_UI_GET_PRIVATE(modest_ui);

	glade_xml = glade_xml_new(MODEST_GLADE, "IdentitiesAndAccountsDialog", NULL);
	main_dialog = glade_xml_get_widget(glade_xml, "IdentitiesAndAccountsDialog");

	/* Accounts */
	accounts_tree_view = glade_xml_get_widget(glade_xml, "AccountsTreeview");
	accounts_model=create_accounts_model(priv->modest_acc_mgr);
	gtk_tree_view_set_model(GTK_TREE_VIEW(accounts_tree_view), accounts_model);
	/* Account -> Accountname */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Account",
							   renderer,
							   "text", ACCOUNT_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(accounts_tree_view), column);
	/* Account -> Hostname */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Hostname",
							   renderer,
							   "text", ACCOUNT_HOST,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(accounts_tree_view), column);

	/* Identities */
	identities_tree_view = glade_xml_get_widget(glade_xml, "IdentitiesTreeview");
	identities_model=create_identities_model(priv->modest_id_mgr);
	gtk_tree_view_set_model(GTK_TREE_VIEW(identities_tree_view), identities_model);

	/* Identities -> Identityname */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Identity",
							   renderer,
							   "text", IDENTITY_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(identities_tree_view), column);

	/* Identities -> E-mail address */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("E-mail address",
							   renderer,
							   "text", IDENTITY_ADDRESS,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(identities_tree_view), column);

	/* Identities -> Relay */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Transport",
							   renderer,
							   "text", IDENTITY_VIA,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(identities_tree_view), column);

	cb_data=setup_callback_data(identities_tree_view, accounts_tree_view, modest_ui);

	abutton=glade_xml_get_widget(glade_xml, "IdentityCreateButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(create_action),
			 cb_data);
	abutton=glade_xml_get_widget(glade_xml, "IdentityEditButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(edit_action),
			 cb_data);
	abutton=glade_xml_get_widget(glade_xml, "IdentityDeleteButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(delete_action),
			 cb_data);

	/* FIXME: the edit/delete buttons are sensitive by default
	 * but we have no selection by default.
	 */

	activate_buttons_on_identity(NULL, glade_xml);

	gtk_widget_show_all(GTK_WIDGET(main_dialog));

	retval=gtk_dialog_run(GTK_DIALOG(main_dialog));

	gtk_widget_destroy(GTK_WIDGET(main_dialog));
	g_object_unref(accounts_model);
	g_object_unref(identities_model);
	free_callback_data(cb_data);
	g_object_unref(glade_xml);
}

void account_settings (GtkWidget *widget,
		       gpointer user_data)
{
	accounts_and_identities_dialog (MODEST_UI(user_data));
}


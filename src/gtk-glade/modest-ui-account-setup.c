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
	ACCOUNT_PROT,
	ACCOUNT_COLUMNS
};

typedef struct _CallbackData CallbackData;

struct _CallbackData {
	GtkTreeView     *id_tree_view;
	GtkTreeView     *acc_tree_view;
	ModestUI        *modest_ui;
	GladeXML        *glade_xml;
};

static void
identity_setup_dialog (ModestUI *, GtkTreeModel *, gchar *);

static void
account_setup_dialog (ModestUI *, gchar *);

static void
missing_notification(GtkWindow *, gchar *);

static GtkTreeModel *
create_identities_model(ModestIdentityMgr *);

static GtkTreeModel *
create_accounts_model(ModestAccountMgr *);

static void
refresh_identities(ModestIdentityMgr *,
		   GladeXML *);

static void
refresh_accounts(ModestAccountMgr *, GladeXML *glade_xml);

/* CALLBACKS */

static gboolean
filter_transports (GtkTreeModel *model,
		   GtkTreeIter *iter,
		   gpointer userdata) {

	gchar *name;
	gboolean retval;

	gtk_tree_model_get(model,
			   iter,
			   ACCOUNT_PROT, &name,
			   -1);

	retval = strcasecmp(name, "SMTP")==0;
	g_message("Debug: '%s' -- '%s' : '%d'", name, "SMTP", retval);
	g_free(name);
	return retval;
}


static void
account_edit_action(GtkWidget *button,
		    gpointer userdata) {

	CallbackData *cb_data;
	GtkTreeModel *acc_liststore;
	GtkTreeSelection *selection;
	GtkTreeIter selected_iter;
	gchar *account_name;

	cb_data = (CallbackData *) userdata;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(cb_data->acc_tree_view));

	gtk_tree_selection_get_selected(selection,
					&acc_liststore,
					&selected_iter);
	gtk_tree_model_get(GTK_TREE_MODEL(acc_liststore),
			   &selected_iter,
			   ACCOUNT_NAME, &account_name,
			   -1);

	account_setup_dialog (cb_data->modest_ui, account_name);
	g_free(account_name);
}

static void
account_create_action(GtkWidget *button,
		      gpointer userdata) {
	CallbackData *cb_data;

	cb_data = (CallbackData *) userdata;

	account_setup_dialog(cb_data->modest_ui, NULL);
}

static void
account_delete_action(GtkWidget *button,
		      gpointer userdata) {
	CallbackData *cb_data;
	GtkTreeSelection *selection;
	GtkTreeIter selected_iter;
	GtkTreeModel *acc_liststore;
	GtkWidget *confirmation_dialog;
	GtkWidget *confirmation_message;
	ModestUIPrivate *priv;
	gchar *account_name;
	gchar *message;
	gint result;

	cb_data = (CallbackData *) userdata;
	priv = MODEST_UI_GET_PRIVATE(cb_data->modest_ui);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(cb_data->acc_tree_view));

	gtk_tree_selection_get_selected(selection,
					&acc_liststore,
					&selected_iter);
	gtk_tree_model_get(GTK_TREE_MODEL(acc_liststore),
			   &selected_iter,
			   ACCOUNT_NAME, &account_name,
			   -1);

	confirmation_dialog = gtk_dialog_new_with_buttons ("Confirm removal of account",
							   GTK_WINDOW(gtk_widget_get_toplevel(button)),
							   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							   GTK_STOCK_OK,
							   GTK_RESPONSE_OK,
							   GTK_STOCK_CANCEL,
							   GTK_RESPONSE_CANCEL,
							   NULL);

	message = g_strdup_printf("Remove selected account '%s'?", account_name);
	confirmation_message = gtk_label_new_with_mnemonic (message);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(confirmation_dialog)->vbox), confirmation_message, FALSE, FALSE, 10);

	gtk_widget_show_all(confirmation_dialog);

	result=gtk_dialog_run(GTK_DIALOG(confirmation_dialog));
	if (result==GTK_RESPONSE_OK)
	{
		modest_account_mgr_remove_server_account(priv->modest_acc_mgr,
							 account_name,
							 NULL);
	}

	gtk_widget_destroy(confirmation_dialog);
	g_free(account_name);
	g_free(message);
}


static void
identity_edit_action(GtkWidget *button,
		     gpointer userdata)
{
	CallbackData *cb_data;
	GtkTreeModel *acc_liststore;
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
			   IDENTITY_NAME, &identity_name,
			   -1);
	/* We use the available tree model from the accounts page to display a selection
	 * of transports in the identities.
	 */
	acc_liststore=NULL;

	identity_setup_dialog (cb_data->modest_ui, acc_liststore, identity_name);
	g_free(identity_name);
}

static void
identity_create_action(GtkWidget *button,
		       gpointer userdata)
{
	CallbackData *cb_data;
	GtkTreeModel *acc_liststore;

	cb_data = (CallbackData *) userdata;

	acc_liststore = gtk_tree_model_filter_new(gtk_tree_view_get_model(cb_data->acc_tree_view),
						  NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(acc_liststore),
					       filter_transports,
					       NULL,
					       NULL);

	identity_setup_dialog(cb_data->modest_ui, acc_liststore, NULL);
	g_object_unref(acc_liststore);
}

static void
identity_delete_action(GtkWidget *button,
		       gpointer userdata)
{
	CallbackData *cb_data;
	GtkTreeSelection *selection;
	GtkTreeIter selected_iter;
	GtkTreeModel *id_liststore;
	GtkWidget *confirmation_dialog;
	GtkWidget *confirmation_message;
	ModestUIPrivate *priv;
	gchar *identity_name;
	gchar *message;
	gint result;

	cb_data = (CallbackData *) userdata;
	priv = MODEST_UI_GET_PRIVATE(cb_data->modest_ui);
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

	message = g_strdup_printf("Remove selected identity '%s'?", identity_name);
	confirmation_message = gtk_label_new_with_mnemonic (message);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(confirmation_dialog)->vbox), confirmation_message, FALSE, FALSE, 10);

	gtk_widget_show_all(confirmation_dialog);

	result=gtk_dialog_run(GTK_DIALOG(confirmation_dialog));
	if (result==GTK_RESPONSE_OK)
	{
		modest_identity_mgr_remove_identity(priv->modest_id_mgr,
						    identity_name,
						    NULL);
	}

	gtk_widget_destroy(confirmation_dialog);
	g_free(identity_name);
	g_free(message);
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
activate_buttons_on_account(GtkTreeView *tree_view,
			    gpointer user_data)
{
	GtkWidget *button;
	GladeXML *glade_xml;

	glade_xml=(GladeXML *) user_data;

	button = glade_xml_get_widget(GLADE_XML(glade_xml), "AccountEditButton");
	gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
	button = glade_xml_get_widget(GLADE_XML(glade_xml), "AccountDeleteButton");
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

static void
refresh_accounts_on_add(ModestAccountMgr *modest_acc_mgr,
			void *nu1,
			gpointer userdata)
{
	refresh_accounts(modest_acc_mgr, (GladeXML *) userdata);
}

static void
refresh_accounts_on_remove(ModestAccountMgr *modest_acc_mgr,
			void *nu1,
			gpointer userdata) {

	GladeXML *glade_xml = (GladeXML *) userdata;
	GtkWidget *button;

	refresh_accounts(modest_acc_mgr, (GladeXML *) userdata);
	/* Since we loose the selection through the delete operation, we need to
	 * change the buttons sensitivity .
	 */
	button = glade_xml_get_widget(GLADE_XML(glade_xml), "AccountEditButton");
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	button = glade_xml_get_widget(GLADE_XML(glade_xml), "AccountDeleteButton");
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

static void
refresh_accounts_on_change(ModestAccountMgr *modest_acc_mgr,
			   void *nu1,
			   void *nu2,
			   void *nu3,
			   gpointer userdata)
{
	refresh_accounts(modest_acc_mgr, (GladeXML *) userdata);
}

static void
refresh_identities_on_add(ModestIdentityMgr *modest_id_mgr,
			  void *nu1,
			  gpointer userdata) {
	refresh_identities(modest_id_mgr, (GladeXML *) userdata);
}

static void
refresh_identities_on_remove(ModestIdentityMgr *modest_id_mgr,
			     void *nu1,
			     gpointer userdata) {

	GladeXML *glade_xml = (GladeXML *) userdata;
	GtkWidget *button;

	refresh_identities(modest_id_mgr, glade_xml);

	/* Since we loose the selection through the delete operation, we need to
	 * change the buttons sensitivity .
	 */
	button = glade_xml_get_widget(GLADE_XML(glade_xml), "IdentityEditButton");
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	button = glade_xml_get_widget(GLADE_XML(glade_xml), "IdentityDeleteButton");
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

static void
refresh_identities_on_change(ModestIdentityMgr *modest_id_mgr,
			     void *nu1,
			     void *nu2,
			     void *nu3,
			     gpointer userdata) {
	refresh_identities(modest_id_mgr, (GladeXML *) userdata);
}

/* METHODS */

static gboolean
search_model_column_for_string_advanced(GtkTreeModel *model, GtkTreeIter *iter, gint ColNum, gchar *search, gboolean mcase) {

	gchar *tmptext;
	gboolean iter_true;

	iter_true = gtk_tree_model_get_iter_first(model, iter);
	while (iter_true) {
		gtk_tree_model_get(model,
				   iter,
				   ColNum, &tmptext,
				   -1);
		if ((mcase && strcasecmp(tmptext, search)==0)
		    || strcmp(tmptext, search)==0) {
			g_free(tmptext);
			break;
		}
		g_free(tmptext);
		iter_true = gtk_tree_model_iter_next(model, iter);
		if (!iter_true) {
			break;
		}
	}
	return iter_true;
}

static gboolean
search_model_column_for_string(GtkTreeModel *model, GtkTreeIter *iter, gint ColNum, gchar *search) {
	return search_model_column_for_string_advanced(model, iter, ColNum, search, FALSE);
}

static gboolean
case_search_model_column_for_string(GtkTreeModel *model, GtkTreeIter *iter, gint ColNum, gchar *search) {
	return search_model_column_for_string_advanced(model, iter, ColNum, search, TRUE);
}

static void
refresh_identities(ModestIdentityMgr *modest_id_mgr,
		   GladeXML *glade_xml) {

	GtkTreeModel *id_liststore;
	GtkTreeView *id_treeview;

	g_message("Debug before access of Treeview");
	id_treeview = GTK_TREE_VIEW(glade_xml_get_widget(glade_xml, "IdentitiesTreeview"));
	g_message("Debug after access of Treeview");

	id_liststore=create_identities_model(modest_id_mgr);
	gtk_tree_view_set_model(GTK_TREE_VIEW(id_treeview), id_liststore);
}

static void
refresh_accounts(ModestAccountMgr *modest_acc_mgr,
		 GladeXML *glade_xml) {

	GtkTreeModel *acc_liststore;
	GtkTreeView *acc_treeview;

	acc_treeview = GTK_TREE_VIEW(glade_xml_get_widget(glade_xml, "AccountsTreeview"));

	acc_liststore=create_accounts_model(modest_acc_mgr);
	gtk_tree_view_set_model(GTK_TREE_VIEW(acc_treeview), acc_liststore);
}

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
write_identity(GladeXML *glade_xml, ModestUI *modest_ui, GtkTreeModel *accounts_model, gboolean newidentity) {

	GtkTextBuffer *sigbuff;
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	GtkTreeIter transport_iter;
	ModestUIPrivate *priv;
	const gchar *identity;
	gchar *reply_to;
	gchar *signature;
	gchar *transport;

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

	identity = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISIdentityEntry")));

	if (newidentity) {

		if (modest_identity_mgr_add_identity (priv->modest_id_mgr,
						      identity,
						      NULL,
						      gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISEMailAddress"))),
						      NULL,
						      NULL,
						      FALSE,
						      NULL,
						      FALSE));
		else
			return FALSE;
	}
	if (!modest_identity_mgr_set_identity_string(priv->modest_id_mgr,
						     identity,
						     MODEST_IDENTITY_REALNAME,
						     gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISNameEntry"))),
						     NULL))
		return FALSE;
	if (!modest_identity_mgr_set_identity_string(priv->modest_id_mgr,
						     identity,
						     MODEST_IDENTITY_EMAIL,
						     gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ISEMailAddress"))),
						     NULL))
		return FALSE;
	if (!modest_identity_mgr_set_identity_string(priv->modest_id_mgr,
						     identity,
						     MODEST_IDENTITY_REPLYTO,
						     reply_to,
						     NULL))
		return FALSE;
	if (!modest_identity_mgr_set_identity_string(priv->modest_id_mgr,
						     identity,
						     MODEST_IDENTITY_SIGNATURE,
						     signature,
						     NULL))
		return FALSE;
	if (!modest_identity_mgr_set_identity_bool(priv->modest_id_mgr,
						   identity,
						   MODEST_IDENTITY_USE_SIGNATURE,
						   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(glade_xml,
														       "ISUseSignatureCheckButton"))),
						   NULL))
		return FALSE;
	if (!modest_identity_mgr_set_identity_string(priv->modest_id_mgr,
						     identity,
						     MODEST_IDENTITY_ID_VIA,
						     transport,
						     NULL))
		return FALSE;
	if (!modest_identity_mgr_set_identity_bool(priv->modest_id_mgr,
						   identity,
						   MODEST_IDENTITY_USE_ID_VIA,
						   TRUE, /* FIXME: for now */
						   NULL))
		return FALSE;
	g_free(transport);
	return TRUE;
}

static gboolean
write_account(GladeXML *glade_xml, ModestUI *modest_ui, gboolean newaccount) {

	ModestUIPrivate *priv;
	const gchar *account;
	gchar *protocol;
	gchar *tmptext;
	gint retval;

	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(modest_ui));

	account = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ASDisplaynameEntry")));

	if (newaccount) {
		tmptext = gtk_combo_box_get_active_text(GTK_COMBO_BOX(glade_xml_get_widget(glade_xml, "ASProtocolComboBox")));
		protocol = g_utf8_strdown(tmptext, -1);
		g_free(tmptext);

		retval = modest_account_mgr_add_server_account (priv->modest_acc_mgr,
								account,
								gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ASHostnameEntry"))),
								gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ASUsernameEntry"))),
								gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ASPasswordEntry"))),
								protocol);
		g_free(protocol);
		return retval;
	}
	if (!modest_account_mgr_set_server_account_string(priv->modest_acc_mgr,
							  account,
							  MODEST_ACCOUNT_HOSTNAME,
							  gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ASHostnameEntry"))),
							  NULL))
		return FALSE;
	if (!modest_account_mgr_set_server_account_string(priv->modest_acc_mgr,
							  account,
							  MODEST_ACCOUNT_USERNAME,
							  gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ASUsernameEntry"))),
							  NULL))
		return FALSE;
	if (!modest_account_mgr_set_server_account_string(priv->modest_acc_mgr,
							  account,
							  MODEST_ACCOUNT_PASSWORD,
							  gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "ASPasswordEntry"))),
							  NULL))
		return FALSE;
	return TRUE;
}

static void
identity_setup_dialog (ModestUI *modest_ui, GtkTreeModel *accounts_model, gchar *identity)
{

	GladeXML *glade_xml;
	GtkWidget *id_dialog;
	GtkWidget *outgoing_server;
	GtkWidget *awidget;
	GtkCellRenderer *renderer;
	ModestIdentityMgr *id_mgr;
	GtkTextBuffer *sigbuff;
	GtkTreeIter out_iter;
	gchar *outacc_name;
	gchar *tmptext;
	gint identity_added_successfully;
	gint result;
	gboolean newidentity;

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

	newidentity = identity==NULL;
	if (!newidentity) {
		id_mgr = MODEST_UI_GET_PRIVATE(modest_ui)->modest_id_mgr;

		outacc_name = modest_identity_mgr_get_identity_string(id_mgr,
								      identity,
								      MODEST_IDENTITY_ID_VIA,
								      NULL);

		if (search_model_column_for_string(GTK_TREE_MODEL(accounts_model),
						   &out_iter,
						   ACCOUNT_NAME,
						   outacc_name))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(outgoing_server), &out_iter);

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
								  MODEST_IDENTITY_REPLYTO,
								  NULL);
		awidget=glade_xml_get_widget(glade_xml, "ISReplyToEntry");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);

		g_free(tmptext);
		g_free(outacc_name);
	}

	gtk_widget_show_all(id_dialog);

	do {
		result=gtk_dialog_run(GTK_DIALOG(id_dialog));

		switch (result) {
		case GTK_RESPONSE_OK:
			identity_added_successfully = write_identity(glade_xml, modest_ui, accounts_model, newidentity);
			break;
		default:
			identity_added_successfully = FALSE;
			break;
		}
	} while(result!=GTK_RESPONSE_DELETE_EVENT && result!=GTK_RESPONSE_CANCEL && identity_added_successfully!=TRUE);

	gtk_widget_destroy(id_dialog);
	g_object_unref(glade_xml);
}

static void
account_setup_dialog (ModestUI *modest_ui, gchar *account) {

	GladeXML *glade_xml;
	GtkWidget *acc_dialog;
	GtkWidget *awidget;
	ModestAccountMgr *acc_mgr;
	GtkTreeModel *typemodel;
	GtkTreeIter proto_iter;
	gchar *tmptext;
	gint account_added_successfully;
	gint result;
	gboolean newaccount;

	glade_xml = glade_xml_new(MODEST_GLADE, "AccountSetupDialog", NULL);
	acc_dialog = glade_xml_get_widget(glade_xml, "AccountSetupDialog");

	newaccount = account==NULL;
	if (!newaccount) {
		acc_mgr = MODEST_UI_GET_PRIVATE(modest_ui)->modest_acc_mgr;

		awidget=glade_xml_get_widget(glade_xml, "ASDisplaynameEntry");
		gtk_widget_set_sensitive(awidget, FALSE);
		gtk_entry_set_text(GTK_ENTRY(awidget), account);

		tmptext = modest_account_mgr_get_server_account_string(acc_mgr,
								       account,
								       MODEST_ACCOUNT_PROTO,
								       NULL);
		g_message("Proto for account '%s': '%s'", account, tmptext);
		awidget=glade_xml_get_widget(glade_xml, "ASProtocolComboBox");
		gtk_widget_set_sensitive(awidget, FALSE);
		typemodel = gtk_combo_box_get_model(GTK_COMBO_BOX(awidget));
		if (case_search_model_column_for_string(typemodel, &proto_iter, 0, tmptext))
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(awidget), &proto_iter);

		g_free(tmptext);

		tmptext = modest_account_mgr_get_server_account_string(acc_mgr,
								account,
								MODEST_ACCOUNT_HOSTNAME,
								NULL);
		awidget=glade_xml_get_widget(glade_xml, "ASHostnameEntry");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);
		g_free(tmptext);

		tmptext = modest_account_mgr_get_server_account_string(acc_mgr,
								account,
								MODEST_ACCOUNT_USERNAME,
								NULL);
		awidget=glade_xml_get_widget(glade_xml, "ASUsernameEntry");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);
		g_free(tmptext);

		tmptext = modest_account_mgr_get_server_account_string(acc_mgr,
								account,
								MODEST_ACCOUNT_PASSWORD,
								NULL);
		awidget=glade_xml_get_widget(glade_xml, "ASPasswordEntry");
		gtk_entry_set_text(GTK_ENTRY(awidget), tmptext);
		g_free(tmptext);
	}

	gtk_widget_show_all(acc_dialog);

	do {
		result=gtk_dialog_run(GTK_DIALOG(acc_dialog));

		switch (result) {
		case GTK_RESPONSE_OK:
			account_added_successfully = write_account(glade_xml, modest_ui, newaccount);

			break;
		default:
			account_added_successfully = FALSE;
			break;
		}
	} while(result!=GTK_RESPONSE_DELETE_EVENT && result!=GTK_RESPONSE_CANCEL && account_added_successfully!=TRUE);

	gtk_widget_destroy(acc_dialog);
	g_object_unref(glade_xml);
}


static CallbackData *
setup_callback_data(GtkTreeView *id_tree_view,
		    GtkTreeView *acc_tree_view,
		    ModestUI *modest_ui,
		    GladeXML *glade_xml)
{
	CallbackData *self;
	self = g_malloc(sizeof(CallbackData));
	self->modest_ui=modest_ui;
	self->id_tree_view=id_tree_view;
	self->acc_tree_view=acc_tree_view;
	self->glade_xml=glade_xml;
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
	gchar *hostname;
	gchar *protocol;

	acc_names_list = modest_account_mgr_server_account_names(acc_mgr,
								 NULL,
								 MODEST_PROTO_TYPE_ANY,
								 NULL,
								 FALSE);
	acc_list_store = gtk_list_store_new(ACCOUNT_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	for (acc_names_list_iter=acc_names_list;
	     acc_names_list_iter!=NULL;
	     acc_names_list_iter=g_slist_next(acc_names_list_iter)) {
		gtk_list_store_append(acc_list_store, &acc_list_store_iter);
		hostname=modest_account_mgr_get_server_account_string(acc_mgr,
								      acc_names_list_iter->data,
								      MODEST_ACCOUNT_HOSTNAME,
								      NULL);
		protocol=modest_account_mgr_get_server_account_string(acc_mgr,
								      acc_names_list_iter->data,
								      MODEST_ACCOUNT_PROTO,
								      NULL);
		gtk_list_store_set(acc_list_store, &acc_list_store_iter,
				   ACCOUNT_NAME, acc_names_list_iter->data,
				   ACCOUNT_HOST, hostname,
				   ACCOUNT_PROT, protocol,
				   -1);
		g_free(hostname);
		g_free(protocol);
	}

	g_slist_free(acc_names_list);

	return GTK_TREE_MODEL(acc_list_store);
}


static void
accounts_and_identities_dialog (gpointer user_data)
{
	GladeXML *glade_xml;
	GtkWidget *main_dialog;
	GtkTreeView *identities_tree_view;
	GtkTreeView *accounts_tree_view;
	ModestUI *modest_ui;
	ModestUIPrivate *priv;
	gint sig_coll[6];
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
	accounts_tree_view = GTK_TREE_VIEW(glade_xml_get_widget(glade_xml, "AccountsTreeview"));
	/* Account -> Accountname */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Account",
							   renderer,
							   "text", ACCOUNT_NAME,
							   NULL);
	gtk_tree_view_append_column (accounts_tree_view, column);
	/* Account -> Hostname */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Hostname",
							   renderer,
							   "text", ACCOUNT_HOST,
							   NULL);
	gtk_tree_view_append_column (accounts_tree_view, column);

	/* Identities */
	identities_tree_view = GTK_TREE_VIEW(glade_xml_get_widget(glade_xml, "IdentitiesTreeview"));

	/* Identities -> Identityname */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Identity",
							   renderer,
							   "text", IDENTITY_NAME,
							   NULL);
	gtk_tree_view_append_column (identities_tree_view, column);

	/* Identities -> E-mail address */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("E-mail address",
							   renderer,
							   "text", IDENTITY_ADDRESS,
							   NULL);
	gtk_tree_view_append_column (identities_tree_view, column);

	/* Identities -> Relay */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Transport",
							   renderer,
							   "text", IDENTITY_VIA,
							   NULL);
	gtk_tree_view_append_column (identities_tree_view, column);

	cb_data=setup_callback_data(identities_tree_view, accounts_tree_view, modest_ui, glade_xml);

	refresh_accounts(priv->modest_acc_mgr,
			 glade_xml);

	refresh_identities(priv->modest_id_mgr,
			   glade_xml);

	/* Identities */
	abutton=glade_xml_get_widget(glade_xml, "IdentityCreateButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(identity_create_action),
			 cb_data);
	abutton=glade_xml_get_widget(glade_xml, "IdentityEditButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(identity_edit_action),
			 cb_data);
	abutton=glade_xml_get_widget(glade_xml, "IdentityDeleteButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(identity_delete_action),
			 cb_data);

	/* Accounts */
	abutton=glade_xml_get_widget(glade_xml, "AccountCreateButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(account_create_action),
			 cb_data);
	abutton=glade_xml_get_widget(glade_xml, "AccountEditButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(account_edit_action),
			 cb_data);
	abutton=glade_xml_get_widget(glade_xml, "AccountDeleteButton");
	g_signal_connect(abutton,
			 "clicked",
			 G_CALLBACK(account_delete_action),
			 cb_data);

	g_signal_connect(glade_xml_get_widget(glade_xml, "IdentitiesTreeview"),
			 "cursor-changed",
			 G_CALLBACK(activate_buttons_on_identity),
			 glade_xml);
	g_signal_connect(glade_xml_get_widget(glade_xml, "AccountsTreeview"),
			 "cursor-changed",
			 G_CALLBACK(activate_buttons_on_account),
			 glade_xml);

	/*
	sig_coll[0] = g_signal_connect(priv->modest_id_mgr,
				       "identity-change",
				       G_CALLBACK(refresh_identities_on_change),
				       glade_xml);
	sig_coll[1] = g_signal_connect(priv->modest_id_mgr,
				       "identity-add",
				       G_CALLBACK(refresh_identities_on_add),
				       glade_xml);
	sig_coll[2] = g_signal_connect(priv->modest_id_mgr,
				       "identity-remove",
				       G_CALLBACK(refresh_identities_on_remove),
				       glade_xml);
	*/

	sig_coll[3] = g_signal_connect(priv->modest_acc_mgr,
				       "account-change",
				       G_CALLBACK(refresh_accounts_on_change),
				       glade_xml);
	sig_coll[4] = g_signal_connect(priv->modest_acc_mgr,
				       "account-add",
				       G_CALLBACK(refresh_accounts_on_add),
				       glade_xml);
	sig_coll[5] = g_signal_connect(priv->modest_acc_mgr,
				       "account-remove",
				       G_CALLBACK(refresh_accounts_on_remove),
				       glade_xml);

	gtk_widget_show_all(GTK_WIDGET(main_dialog));

	retval=gtk_dialog_run(GTK_DIALOG(main_dialog));

	/*
	g_signal_handler_disconnect(priv->modest_id_mgr, sig_coll[0]);
	g_signal_handler_disconnect(priv->modest_id_mgr, sig_coll[1]);
	g_signal_handler_disconnect(priv->modest_id_mgr, sig_coll[2]);
	*/
	g_signal_handler_disconnect(priv->modest_acc_mgr, sig_coll[3]);
	g_signal_handler_disconnect(priv->modest_acc_mgr, sig_coll[4]);
	g_signal_handler_disconnect(priv->modest_acc_mgr, sig_coll[5]);

	gtk_widget_destroy(GTK_WIDGET(main_dialog));
	free_callback_data(cb_data);
	g_object_unref(glade_xml);
}

void account_settings (GtkWidget *widget,
		       gpointer user_data)
{
	accounts_and_identities_dialog (MODEST_UI(user_data));
}


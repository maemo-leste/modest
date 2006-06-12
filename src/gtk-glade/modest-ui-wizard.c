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

#include "modest-ui-glade.h"
#include "modest-ui-wizard.h"


static void wizard_incoming_button_toggled(GtkWidget *button,
					   gpointer userdata) {
	GtkWidget *awidget;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(button))==TRUE) {
		awidget=glade_xml_get_widget(GLADE_XML(userdata), "AWOutUserNameEntry");
		gtk_widget_set_sensitive(GTK_WIDGET(awidget), FALSE);
		awidget=glade_xml_get_widget(GLADE_XML(userdata), "AWOutPasswordEntry");
		gtk_widget_set_sensitive(GTK_WIDGET(awidget), FALSE);
	}
	else {
		awidget=glade_xml_get_widget(GLADE_XML(userdata), "AWOutUserNameEntry");
		gtk_widget_set_sensitive(GTK_WIDGET(awidget), TRUE);
		awidget=glade_xml_get_widget(GLADE_XML(userdata), "AWOutPasswordEntry");
		gtk_widget_set_sensitive(GTK_WIDGET(awidget), TRUE);
	}
}

void wizard_missing_notification(GtkWindow *parent, gchar *info_message) {
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

gchar *get_text_from_combobox (GtkWidget *combobox){
        /* Remember to free the returned variable after usage! */

	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
	GtkTreeIter iter;

	gchar *value;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combobox), &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(model),
				   &iter,
				   0, &value,
				   -1);
	}

        return value;
}


gboolean advance_sanity_check(GtkWindow *parent, GladeXML *glade_xml, gint cp) {
        gchar *tmptext;

	/* FIXME:
	 * all calls to wizard_missing_notification lack the parent window.
	 */

        switch (cp) {
	case 1:
		/* Only needed if the "mailbox name" field is used in the first page of the wizard.
                 * if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWMailboxnameEntry"))))==0)
                 * {
                 *         wizard_missing_notification(NULL, "Please enter mailbox name");
                 *         return FALSE;
		 * }
		 */
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWEMailAddressEntry"))))==0) {
			wizard_missing_notification(NULL, "Please enter the E-Mail address.");
			return FALSE;
		}
                return TRUE;
		break;
	case 2:
                tmptext=gtk_combo_box_get_active_text(GTK_COMBO_BOX(glade_xml_get_widget(glade_xml, "AWMailboxtypeComboBox")));
                if (tmptext==NULL) {
                        wizard_missing_notification(NULL, "Please select mailbox type.");
                        return FALSE;
		}
		g_free(tmptext);
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWInServerComboEntry"))))==0) {
			wizard_missing_notification(NULL, "Please specify incoming server adress.");
			return FALSE;
		}
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWUserNameEntry"))))==0) {
			wizard_missing_notification(NULL, "Please enter user name.");
			return FALSE;
		}
		return TRUE;
		break;
	case 3:
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWOutServerComboEntry"))))==0) {
			wizard_missing_notification(NULL, "Please specify outgoing server address.");
			return FALSE;
		}
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(glade_xml, "AWUseIncomingCheckButton")))==FALSE
		    && strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWOutUserNameEntry"))))==0) {
			wizard_missing_notification(NULL, "Please enter user name.");
			return FALSE;
		}

		return TRUE;
		break;
        }

        return FALSE;
}

gchar *search_unused_account_or_identity_name(gpointer mgr, gchar *draft) {
	GString *tmpaccount_name;
	gint counter;

	tmpaccount_name=g_string_new("");
	g_string_printf(tmpaccount_name, "%s", draft);
	if(MODEST_IS_ACCOUNT_MGR(mgr)) {
		for(counter=0; modest_account_mgr_server_account_exists(mgr, tmpaccount_name->str, NULL); counter++)
			g_string_printf(tmpaccount_name, "%s%d", draft, counter);
	}
	else
		for(counter=0;      modest_identity_mgr_identity_exists(mgr, tmpaccount_name->str, NULL); counter++)
			g_string_printf(tmpaccount_name, "%s%d", draft, counter);

	return g_string_free(tmpaccount_name, FALSE);
}

gboolean wizard_account_add(GladeXML *glade_xml, ModestUI *modest_ui)
{
	ModestAccountMgr *acc_mgr;
	ModestIdentityMgr *id_mgr;
	gchar *tmpaccount_name;
	ModestUIPrivate *priv;
	ModestConf *conf;
	gchar *tmptext;
	gchar *tmptext2;

	g_return_val_if_fail (MODEST_IS_UI(modest_ui), FALSE);
	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(modest_ui));
	conf = priv->modest_conf;

	acc_mgr = priv->modest_acc_mgr;
	id_mgr = priv->modest_id_mgr;

	tmptext2=get_text_from_combobox(glade_xml_get_widget(glade_xml, "AWMailboxtypeComboBox"));
	tmptext=g_utf8_strdown(tmptext2, -1);
	g_free(tmptext2);

	tmpaccount_name=search_unused_account_or_identity_name(acc_mgr, "incoming");
	modest_account_mgr_add_server_account (acc_mgr,
					       tmpaccount_name,
					       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWInServerComboEntry"))),
					       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWUserNameEntry"))),
					       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWPasswordEntry"))),
					       tmptext);
	g_free(tmpaccount_name);
	g_free(tmptext);

	tmpaccount_name=search_unused_account_or_identity_name(acc_mgr, "outgoing");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(glade_xml, "AWUseIncomingCheckButton")))==TRUE)
		modest_account_mgr_add_server_account (acc_mgr,
						       tmpaccount_name,
						       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWOutServerComboEntry"))),
						       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWUserNameEntry"))),
						       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWPasswordEntry"))),
						       "smtp");
	else
		modest_account_mgr_add_server_account (acc_mgr,
						       tmpaccount_name,
						       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWOutServerComboEntry"))),
						       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWOutUserNameEntry"))),
						       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWOutPasswordEntry"))),
						       "smtp");
	g_free(tmpaccount_name);

	tmpaccount_name=search_unused_account_or_identity_name(id_mgr, MODEST_IDENTITY_DEFAULT_IDENTITY);
	if (!modest_identity_mgr_add_identity (id_mgr,
					       tmpaccount_name,
					       gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWEMailAddressEntry"))),
					       "", "", FALSE, NULL, FALSE ))
		g_warning ("failed to add default identity");

	g_free(tmpaccount_name);
	return TRUE;
}


void wizard_account_dialog(ModestUI *modest_ui)
{
	GladeXML *glade_xml;
	GtkWidget *dialog;
        ModestUIPrivate *priv;
        GtkWidget *finish_button;
        GtkWidget *back_button;
        GtkWidget *next_button;
        GtkWidget *cancel_button;
	GtkWidget *notebook;
	GtkWidget *use_incoming_button;
        gint cp;
        gint result;
	gboolean account_added_successfully=FALSE;

	g_return_if_fail(MODEST_IS_UI(modest_ui));
	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(modest_ui));

	glade_xml = glade_xml_new(MODEST_GLADE, "account_wizard", NULL);

        dialog = glade_xml_get_widget(glade_xml, "account_wizard");

        gtk_widget_show_all(dialog);

        finish_button=glade_xml_get_widget(glade_xml, "AWFinishButton");
        back_button=glade_xml_get_widget(glade_xml, "AWBackButton");
        next_button=glade_xml_get_widget(glade_xml, "AWNextButton");
        cancel_button=glade_xml_get_widget(glade_xml, "AWCancelButton");
        notebook=glade_xml_get_widget(glade_xml, "AWNotebook");

	gtk_widget_set_sensitive(finish_button, FALSE);

	use_incoming_button=glade_xml_get_widget(glade_xml, "AWUseIncomingCheckButton");
	g_signal_connect(use_incoming_button,
			 "toggled",
			 G_CALLBACK(wizard_incoming_button_toggled),
			 glade_xml);

	/* First page not used currently. It's reserved for the account preset. */
	gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 1);

        do {
                cp=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
                switch (cp) {
                case 1:
                        gtk_widget_set_sensitive(back_button, FALSE);
                        break;
                case 2:
                        gtk_widget_set_sensitive(back_button, TRUE);
                        break;
                case 3:
                        gtk_widget_set_sensitive(finish_button, FALSE);
                        gtk_widget_set_sensitive(next_button, TRUE);
                        break;
                case 4:
                        gtk_widget_set_sensitive(finish_button, TRUE);
                        gtk_widget_set_sensitive(next_button, FALSE);
                        break;
                default:
                        g_error("I'm on page %d of notebook AWNotebook, which shouldn't have happened. Pulling emergency breaks.", cp);
                        break;
                }

                result=gtk_dialog_run(GTK_DIALOG(dialog));

                switch (result) {
                case 1:
                        if (advance_sanity_check(NULL, glade_xml, cp)==TRUE)
                                gtk_notebook_next_page(GTK_NOTEBOOK(notebook));
                        break;
                case 2:
                        gtk_notebook_prev_page(GTK_NOTEBOOK(notebook));
			break;
		case GTK_RESPONSE_ACCEPT:
			account_added_successfully=wizard_account_add(glade_xml, modest_ui);
			break;
		default:
			account_added_successfully=FALSE;
                }
        }
        while(result!=GTK_RESPONSE_DELETE_EVENT && result!=GTK_RESPONSE_CANCEL && account_added_successfully!=TRUE);

	gtk_widget_destroy(dialog);
	g_object_unref(glade_xml);
}

void new_wizard_account (GtkWidget *widget,
			 gpointer user_data)
{
	/* This will probably never be used to modify any existing account. */
	wizard_account_dialog(MODEST_UI(user_data));
}



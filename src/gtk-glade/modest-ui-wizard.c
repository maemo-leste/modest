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

void wizard_missing_notification(GtkWindow *parent, gchar *info_message)
{
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

gchar *gtk_combo_box_get_entry_text (GtkWidget *combobox)
{
        /* Remember to free the returned variable after usage! */

	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
	GtkTreeIter iter;

	gchar *value;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combobox), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(model),
				   &iter,
				   0, &value,
				   -1);
	}

        return value;
}


gboolean advance_sanity_check(GtkWindow *parent, GladeXML *glade_xml, gint cp)
{
        GtkDialog *DenyDialog;
        gchar *tmptext;

	/* FIXME:
	 * all calls to wizard_missing_notification lack the parent window.
	 */

        switch (cp)
        {
	case 0:
		/* Only needed if the "mailbox name" field is used in the first page of the wizard.
                 * if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWMailboxnameEntry"))))==0)
                 * {
                 *         wizard_missing_notification(NULL, "Please enter mailbox name");
                 *         return FALSE;
		 * }
		 */
                tmptext=gtk_combo_box_get_active_text(GTK_COMBO_BOX(glade_xml_get_widget(glade_xml, "AWMailboxtypeComboBox")));
                if (tmptext==NULL)
                {
                        wizard_missing_notification(NULL, "Please select mailbox type.");
                        return FALSE;
		}
		free(tmptext);
                return TRUE;
		break;
	case 1:
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWUserNameEntry"))))==0)
		{
			wizard_missing_notification(NULL, "Please enter user name.");
			return FALSE;
		}
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWEMailAddressEntry"))))==0)
		{
			wizard_missing_notification(NULL, "Please enter the E-Mail address.");
			return FALSE;
		}
		return TRUE;
		break;
	case 2:
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWInServerComboEntry"))))==0)
		{
			wizard_missing_notification(NULL, "Please specify incoming server adress.");
			return FALSE;
		}
		if (strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(glade_xml, "AWOutServerComboEntry"))))==0)
		{
			wizard_missing_notification(NULL, "Please specify outgoing server address.");
			return FALSE;
		}
		return TRUE;
		break;
        }

        return FALSE;
}

void on_new_account1_activate (GtkMenuItem *menuitem,
                               gpointer user_data)
{
	GladeXML *glade_xml;
	GtkWidget *dialog;
        ModestUIPrivate *priv;
        GtkWidget *FinishButton;
        GtkWidget *BackButton;
        GtkWidget *NextButton;
        GtkWidget *CancelButton;
        GtkWidget *Notebook;
        GtkWidget *awidget;
        gint cp;
        gint result;
        gint finishallowed=0;

	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(user_data));

	glade_xml = glade_xml_new(MODEST_GLADE, "account_wizard", NULL);

        dialog = glade_xml_get_widget(glade_xml, "account_wizard");

        gtk_widget_show_all(dialog);

        FinishButton=glade_xml_get_widget(glade_xml, "AWFinishButton");
        BackButton=glade_xml_get_widget(glade_xml, "AWBackButton");
        NextButton=glade_xml_get_widget(glade_xml, "AWNextButton");
        CancelButton=glade_xml_get_widget(glade_xml, "AWCancelButton");
        Notebook=glade_xml_get_widget(glade_xml, "AWNotebook");

        gtk_widget_set_sensitive(FinishButton, FALSE);

        do
        {
                cp=gtk_notebook_get_current_page(GTK_NOTEBOOK(Notebook));
                switch (cp)
                {
                case 0:
                        gtk_widget_set_sensitive(BackButton, FALSE);
                        break;
                case 1:
                        gtk_widget_set_sensitive(BackButton, TRUE);
                        break;
                case 2:
                        gtk_widget_set_sensitive(FinishButton, FALSE);
                        gtk_widget_set_sensitive(NextButton, TRUE);
                        break;
                case 3:
                        gtk_widget_set_sensitive(FinishButton, TRUE);
                        gtk_widget_set_sensitive(NextButton, FALSE);
                        break;
                default:
                        g_error("I'm on page %d of notebook AWNotebook, which shouldn't have happened. Pulling Emeregency breaks.", cp);
                        break;
                }

                result=gtk_dialog_run(GTK_DIALOG(dialog));

                switch (result)
                {
                case 1:
                        if (advance_sanity_check(NULL, glade_xml, cp)==TRUE)
                                gtk_notebook_next_page(GTK_NOTEBOOK(Notebook));
                        break;
                case 2:
                        gtk_notebook_prev_page(GTK_NOTEBOOK(Notebook));
                        break;
                }
        }
        while(result!=GTK_RESPONSE_DELETE_EVENT && result!=GTK_RESPONSE_ACCEPT && result!=GTK_RESPONSE_CANCEL);

        if (result==GTK_RESPONSE_ACCEPT)
        {
                /* Do someting with the DATA from the widget */
        }

	gtk_widget_destroy(dialog);
	g_object_unref(glade_xml);
}


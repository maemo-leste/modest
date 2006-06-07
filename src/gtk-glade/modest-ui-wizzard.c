/* modest-ui-wizzard.c */

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
#include "modest-ui-wizzard.h"

gint do_wizzdialog1(GtkDialog *dialog, ModestUIPrivate *priv, void *account_data)
{
	gint result;

	gtk_widget_show_all(GTK_WIDGET(dialog));

	result=gtk_dialog_run(GTK_DIALOG(dialog));

	g_message("Ergebnis des Knopfdrucks: %d", result);

	switch (result)
	{
	default:
		break;
	}

	return TRUE;
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
        gint cp;
	gint result;

	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(user_data));

	glade_xml = glade_xml_new(MODEST_GLADE, "account_wizzard", NULL);

        FinishButton=glade_xml_get_widget(glade_xml, "AWFinishButton");
        BackButton=glade_xml_get_widget(glade_xml, "AWBackButton");
        NextButton=glade_xml_get_widget(glade_xml, "AWNextButton");
        CancelButton=glade_xml_get_widget(glade_xml, "AWCancelButton");
        Notebook=glade_xml_get_widget(glade_xml, "AWNotebook");

        dialog = glade_xml_get_widget(glade_xml, "account_wizzard");

        g_message("Glade-file: %s", MODEST_GLADE);

        gtk_widget_show_all(dialog);

        if (FinishButton==NULL)
                g_error("Is no notebook");

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
                        gtk_widget_set_sensitive(NextButton, TRUE);
                        break;
                case 3:
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
                        gtk_notebook_next_page(GTK_NOTEBOOK(Notebook));
                        break;
                case 2:
                        gtk_notebook_prev_page(GTK_NOTEBOOK(Notebook));
                        break;
                }
        }
        while(result!=GTK_RESPONSE_DELETE_EVENT && result!=GTK_RESPONSE_ACCEPT && result!=GTK_RESPONSE_CANCEL);

	gtk_widget_destroy(dialog);
	g_object_unref(glade_xml);
}


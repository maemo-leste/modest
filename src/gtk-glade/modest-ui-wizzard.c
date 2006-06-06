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

	gtk_widget_show_all(dialog);

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
	gint result;

	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(user_data));

	glade_xml = glade_xml_new(MODEST_GLADE, "account_wizzard", NULL);

	dialog = glade_xml_get_widget(glade_xml, "account_wizzard");

	gtk_widget_show_all(dialog);

	result=gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
	g_object_unref(glade_xml);
}


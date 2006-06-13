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
#include "modest-ui-account-setup.h"

static void
accounts_and_identities_dialog (gpointer user_data)
{
	GladeXML *glade_xml;
	GtkWidget *advanced_account_setup;
	ModestUIPrivate *priv;
	gint retval;


        g_return_if_fail(MODEST_IS_UI(user_data));
	priv = MODEST_UI_GET_PRIVATE(MODEST_UI(user_data));

	glade_xml = glade_xml_new(MODEST_GLADE, "IdentitiesAndAccountsDialog", NULL);
	advanced_account_setup = glade_xml_get_widget(glade_xml, "IdentitiesAndAccountsDialog");


	gtk_widget_show_all(GTK_WIDGET(advanced_account_setup));

	while (TRUE) {
		retval=gtk_dialog_run(GTK_DIALOG(advanced_account_setup));
		if (retval==GTK_RESPONSE_CANCEL)
			break;
	}

	gtk_widget_destroy(GTK_WIDGET(advanced_account_setup));
	g_object_unref(glade_xml);
}

void account_settings (GtkWidget *widget,
                       gpointer user_data)
{
	accounts_and_identities_dialog (MODEST_UI(user_data));
}


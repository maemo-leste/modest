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

#include "../modest-viewer-window.h"

#include "modest-ui-glade.h"
#include "modest-ui-wizard.h"


typedef struct {
	ModestUI *modest_ui;
	ModestViewerWindow *viewer_win;
	GladeXML *glade_xml;
} ViewerWinData;


GtkContainer
*modest_ui_new_viewer_window (ModestUI *modest_ui, GtkWidget *msg_view, TnyMsgIface *msg, gpointer *user_data)
{
	GtkWidget	*top_container;
	GladeXML	*glade_xml;
	ViewerWinData	*win_data;

	glade_xml = glade_xml_new(MODEST_GLADE, "viewer_top_container", NULL);
	if (!glade_xml)
		return NULL;

	win_data = g_malloc(sizeof(ViewerWinData));
	win_data->modest_ui = modest_ui;
	win_data->glade_xml = glade_xml;

	*user_data = win_data;

	top_container = glade_xml_get_widget(glade_xml, "viewer_top_container");
	if (!top_container) {
		g_object_unref(G_OBJECT(glade_xml));
		return NULL;
	}

	return GTK_CONTAINER(top_container);
}


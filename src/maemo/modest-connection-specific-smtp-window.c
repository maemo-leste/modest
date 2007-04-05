/* connection-specific-smtp-window.c */

#include "modest-connection-specific-smtp-window.h"
#include "modest-connection-specific-smtp-edit-window.h"
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkstock.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (ModestConnectionSpecificSmtpWindow, modest_connection_specific_smtp_window, GTK_TYPE_WINDOW);

#define CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_WINDOW, ModestConnectionSpecificSmtpWindowPrivate))

typedef struct _ModestConnectionSpecificSmtpWindowPrivate ModestConnectionSpecificSmtpWindowPrivate;

struct _ModestConnectionSpecificSmtpWindowPrivate
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
};

static void
modest_connection_specific_smtp_window_get_property (GObject *object, guint property_id,
															GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_connection_specific_smtp_window_set_property (GObject *object, guint property_id,
															const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
modest_connection_specific_smtp_window_dispose (GObject *object)
{
	if (G_OBJECT_CLASS (modest_connection_specific_smtp_window_parent_class)->dispose)
		G_OBJECT_CLASS (modest_connection_specific_smtp_window_parent_class)->dispose (object);
}

static void
modest_connection_specific_smtp_window_finalize (GObject *object)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (object);

	g_object_unref (G_OBJECT (priv->model));
	
	G_OBJECT_CLASS (modest_connection_specific_smtp_window_parent_class)->finalize (object);
}

static void
modest_connection_specific_smtp_window_class_init (ModestConnectionSpecificSmtpWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestConnectionSpecificSmtpWindowPrivate));

	object_class->get_property = modest_connection_specific_smtp_window_get_property;
	object_class->set_property = modest_connection_specific_smtp_window_set_property;
	object_class->dispose = modest_connection_specific_smtp_window_dispose;
	object_class->finalize = modest_connection_specific_smtp_window_finalize;
}

enum MODEL_COLS {
	MODEL_COL_NAME = 0,
	MODEL_COL_SERVER_NAME = 1,
	MODEL_COL_ID = 2
};

static void
fill_with_connections (ModestConnectionSpecificSmtpWindow *self)
{
	/* TODO: 
	* When TnyMaemoDevice provides enough of the libconic API to implement this. */
}

	
static void
on_edit_window_hide (GtkWindow *window, gpointer user_data)
{
	/* Destroy the window when it is closed: */
	gtk_widget_destroy (GTK_WIDGET (window));
}

 	
static void
on_button_edit (GtkButton *button, gpointer user_data)
{
	ModestConnectionSpecificSmtpWindow *self = MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (user_data);
	
	/* TODO: Specify the chosen connection to edit: */
	GtkWidget * window = GTK_WIDGET (modest_connection_specific_smtp_edit_window_new ());
	gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (window));
	g_signal_connect (G_OBJECT (window), "hide",
        	G_CALLBACK (on_edit_window_hide), self);
	gtk_widget_show (window);
}

static void
on_button_cancel (GtkButton *button, gpointer user_data)
{
	ModestConnectionSpecificSmtpWindow *self = MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (user_data);

	/* Hide the window.
	 * The code that showed it will respond to the hide signal. */	
	gtk_widget_hide (GTK_WIDGET (self));
}

static void
modest_connection_specific_smtp_window_init (ModestConnectionSpecificSmtpWindow *self)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);

	/* Create a tree model for the tree view:
	 * with a string for the name, a string for the server name, and an int for the ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT));

	/* Setup the tree view: */
	priv->treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (priv->model));

	/* name column:
	 * The ID model column in not shown in the view. */
	GtkTreeViewColumn *view_column = gtk_tree_view_column_new ();
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(view_column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (view_column, renderer, 
	"text", MODEL_COL_NAME, NULL);
	gtk_tree_view_column_set_title (view_column, _("mcen_ia_optionalsmtp_connection_name"));
	gtk_tree_view_append_column (priv->treeview, view_column);

	
	/* server name column: */
	view_column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(view_column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (view_column, renderer, 
	"text", MODEL_COL_SERVER_NAME, NULL);
	gtk_tree_view_column_set_title (view_column, _("mcen_ia_optionalsmtp_servername"));
	gtk_tree_view_append_column (priv->treeview, view_column);
	
	/* Fill the model with rows: */
	fill_with_connections (self);
	
	GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
	
	/* Put the treeview in a scrolled window and add it to the box: */
	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled_window);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->treeview));
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scrolled_window), TRUE, TRUE, 2);
	gtk_widget_show (GTK_WIDGET (priv->treeview));
	
	/* Add the buttons: */
	GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 2);
	gtk_widget_show (hbox);
	
	GtkWidget *button_edit = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_box_pack_start (GTK_BOX (hbox), button_edit, TRUE, FALSE, 2);
	gtk_widget_show (button_edit);
	g_signal_connect (G_OBJECT (button_edit), "clicked",
        	G_CALLBACK (on_button_edit), self);
	
	GtkWidget *button_cancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_box_pack_start (GTK_BOX (hbox), button_cancel, TRUE, FALSE, 2);
	gtk_widget_show (button_cancel);
	g_signal_connect (G_OBJECT (button_edit), "clicked",
        	G_CALLBACK (on_button_cancel), self);
	
	gtk_widget_show (vbox);
}

ModestConnectionSpecificSmtpWindow*
modest_connection_specific_smtp_window_new (void)
{
	return g_object_new (MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_WINDOW, NULL);
}


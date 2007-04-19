/* connection-specific-smtp-window.c */

#include "modest-connection-specific-smtp-window.h"
#include "modest-connection-specific-smtp-edit-window.h"
#include <modest-account-mgr-helpers.h>

#include <modest-runtime.h>
#include <tny-maemo-conic-device.h>

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
	
	ModestAccountMgr *account_manager;
	gchar* account_name;
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
	g_free (priv->account_name);
	
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
	MODEL_COL_NAME = 0, /* libconic IAP Name: a string */
	MODEL_COL_ID = 1, /* libconic IAP ID: a string */
	MODEL_COL_SERVER_ACCOUNT_NAME = 2, /* a string */
	MODEL_COL_SERVER_NAME = 3 /* a string */
};

void
modest_connection_specific_smtp_window_fill_with_connections (ModestConnectionSpecificSmtpWindow *self, ModestAccountMgr *account_manager,
	const gchar* account_name)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);
	priv->account_manager = account_manager;
	priv->account_name = g_strdup (account_name);
	
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	
	TnyDevice *device = modest_runtime_get_device ();
	g_assert (TNY_IS_MAEMO_CONIC_DEVICE (device));
	
	TnyMaemoConicDevice *maemo_device = TNY_MAEMO_CONIC_DEVICE (device);
	
	/* Get the list of Internet Access Points: */
	GSList* list_iaps = tny_maemo_conic_device_get_iap_list (maemo_device);
	printf("debug: list_iaps=%p, list_iaps size = %d\n", list_iaps, g_slist_length(list_iaps));
	
	GSList* iter = list_iaps;
	while (iter) {
		ConIcIap *iap = (ConIcIap*)iter->data;
		if (iap) {
			const gchar *name = con_ic_iap_get_name (iap);
			const gchar *id = con_ic_iap_get_id (iap);
			printf ("debug: iac name=%s, id=%s\n", name, id);
			
			/* Add the row to the model: */
			GtkTreeIter iter;
			gtk_list_store_append (liststore, &iter);
			gtk_list_store_set(liststore, &iter, MODEL_COL_ID, id, MODEL_COL_NAME, name, -1);
		}
		
		iter = g_slist_next (iter);	
	}
		
	if (list_iaps)
		tny_maemo_conic_device_free_iap_list (maemo_device, list_iaps);
}

 	
static void
on_button_edit (GtkButton *button, gpointer user_data)
{
	ModestConnectionSpecificSmtpWindow *self = MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (user_data);
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);
	
	gchar *id = NULL;
	gchar *connection_name = NULL;
	gchar *server_account_name = NULL;
	GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	GtkTreeIter iter;
	GtkTreeModel *model = 0;
	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_ID, &id, 
				    MODEL_COL_NAME, &connection_name, 
				    MODEL_COL_SERVER_ACCOUNT_NAME, &server_account_name,
				    -1);
	
		/* TODO: Is 0 an allowed libconic IAP ID? 
		 * If not then we should check for it. */
		
		GtkWidget * window = GTK_WIDGET (modest_connection_specific_smtp_edit_window_new ());
		modest_connection_specific_smtp_edit_window_set_connection (
			MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (window), id, connection_name);
			
		gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (window));
		gint response = gtk_dialog_run (GTK_DIALOG (window));
		gtk_widget_hide (window);
		
		if (response == GTK_RESPONSE_OK) {
			if (server_account_name) {
				/* Change the existing server account. */
				modest_connection_specific_smtp_edit_window_save_settings (
					MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (window), 
					priv->account_manager, server_account_name);	
			} else {
				/* Add a new server account, building a (non-human-visible) name: */
				gchar *name_start = g_strdup_printf("%s_specific_%s", 
					priv->account_name, connection_name);
				server_account_name = modest_account_mgr_get_unused_account_name (
					priv->account_manager, name_start, TRUE /* server account. */);
				g_free (name_start);
				
				modest_connection_specific_smtp_edit_window_save_settings (
					MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (window), 
					priv->account_manager, server_account_name);
					
				/* Store it in the model so it can be edited again: */
				gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter, 
				    MODEL_COL_SERVER_ACCOUNT_NAME, server_account_name,
				    -1);
			}
		}		
	}
	
	g_free (connection_name);
	g_free (id);
	g_free (server_account_name);
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
	/* This seems to be necessary to make the window show at the front with decoration.
	 * If we use property type=GTK_WINDOW_TOPLEVEL instead of the default GTK_WINDOW_POPUP+decoration, 
	 * then the window will be below the others. */
	gtk_window_set_type_hint (GTK_WINDOW (self),
			    GDK_WINDOW_TYPE_HINT_DIALOG);
			    
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);

	/* Create a tree model for the tree view:
	 * with a string for the name, a string for the server name, and an int for the ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));

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
	
	/* The application must call modest_connection_specific_smtp_window_fill_with_connections(). */
	
	GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
	
	/* Put the treeview in a scrolled window and add it to the box: */
	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolled_window);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->treeview));
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scrolled_window), TRUE, TRUE, 2);
	gtk_widget_show (GTK_WIDGET (priv->treeview));
	
	/* Add the buttons: */
	GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
	gtk_widget_show (hbox);
	
	GtkWidget *button_edit = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_box_pack_start (GTK_BOX (hbox), button_edit, TRUE, FALSE, 2);
	gtk_widget_show (button_edit);
	g_signal_connect (G_OBJECT (button_edit), "clicked",
        	G_CALLBACK (on_button_edit), self);
	
	GtkWidget *button_cancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_box_pack_start (GTK_BOX (hbox), button_cancel, TRUE, FALSE, 2);
	gtk_widget_show (button_cancel);
	g_signal_connect (G_OBJECT (button_cancel), "clicked",
        	G_CALLBACK (on_button_cancel), self);
	
	gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (vbox));
	gtk_widget_show (vbox);
}

ModestConnectionSpecificSmtpWindow*
modest_connection_specific_smtp_window_new (void)
{
	return g_object_new (MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_WINDOW, NULL);
}


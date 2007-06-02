/* connection-specific-smtp-window.c */

#include "modest-connection-specific-smtp-window.h"
#include "modest-connection-specific-smtp-edit-window.h"
#include <modest-account-mgr-helpers.h>
#include "widgets/modest-ui-constants.h"

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
	GtkWidget *button_edit;
	
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

enum MODEL_COLS {
	MODEL_COL_NAME = 0, /* libconic IAP Name: a string */
	MODEL_COL_ID = 1, /* libconic IAP ID: a string */
	MODEL_COL_SERVER_ACCOUNT_NAME = 2, /* a string */
	MODEL_COL_SERVER_NAME = 3, /* a string */
	MODEL_COL_SERVER_ACCOUNT_DATA = 4 /* a gpointer */
};


void update_model_server_names (ModestConnectionSpecificSmtpWindow *self);

static void
modest_connection_specific_smtp_window_finalize (GObject *object)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (object);
	
	/* Free all the data items from the treemodel: */
	GtkTreeIter iter;
  	gboolean valid = gtk_tree_model_get_iter_first (priv->model, &iter);
	while (valid) {
		ModestServerAccountData *data = NULL;
		
		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_SERVER_ACCOUNT_DATA, &data,
				    -1);
				 
		if (data)
			modest_account_mgr_free_server_account_data (priv->account_manager, data);
			
		/* Get next row: */
		valid = gtk_tree_model_iter_next (priv->model, &iter);
	}
	
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

/* libconic does not return a list of connections in scratchbox,
 * so enable this to put a fake row in the list,
 * so we can test other parts of the code. */
/* #define DEBUG_WITHOUT_LIBCONIC 1 */

void
modest_connection_specific_smtp_window_fill_with_connections (ModestConnectionSpecificSmtpWindow *self, ModestAccountMgr *account_manager,
	const gchar* account_name)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);
	priv->account_manager = account_manager;
	priv->account_name = account_name ? g_strdup (account_name) : NULL;
	
	GtkListStore *liststore = GTK_LIST_STORE (priv->model);
	
	TnyDevice *device = modest_runtime_get_device ();
	g_assert (TNY_IS_MAEMO_CONIC_DEVICE (device));
	
	/* Get the list of Internet Access Points: */
	#ifdef DEBUG_WITHOUT_LIBCONIC
	GSList *list_iaps = g_slist_append(NULL, (gpointer)1);
	#else
	TnyMaemoConicDevice *maemo_device = TNY_MAEMO_CONIC_DEVICE (device);
	GSList *list_iaps = tny_maemo_conic_device_get_iap_list (maemo_device);
	#endif
	
	/* printf("debug: list_iaps=%p, list_iaps size = %d\n", list_iaps, g_slist_length(list_iaps)); */
	
	GSList* iter = list_iaps;
	while (iter) {
		ConIcIap *iap = (ConIcIap*)iter->data;
		if (iap) {
			#ifdef DEBUG_WITHOUT_LIBCONIC
			const gchar *name = "debug name";
			const gchar *id = "debug id";
			#else
			const gchar *name = con_ic_iap_get_name (iap);
			const gchar *id = con_ic_iap_get_id (iap);
			#endif
			
			printf ("debug: iac name=%s, id=%s\n", name, id);
			
			/* Get any already-associated connection-specific server account: */
			gchar *server_account_name = NULL;
			if (priv->account_name)
				server_account_name = modest_account_mgr_get_connection_specific_smtp (
					priv->account_manager, priv->account_name, name);
					
			/* Add the row to the model: */
			GtkTreeIter iter;
			gtk_list_store_append (liststore, &iter);
			gtk_list_store_set(liststore, &iter, 
				MODEL_COL_ID, id, 
				MODEL_COL_NAME, name,
				MODEL_COL_SERVER_ACCOUNT_NAME, server_account_name,
				-1);
				
			g_free (server_account_name);
		}
		
		iter = g_slist_next (iter);	
	}
		
	#ifndef DEBUG_WITHOUT_LIBCONIC
	if (list_iaps)
		tny_maemo_conic_device_free_iap_list (maemo_device, list_iaps);
	#endif
		
	update_model_server_names (self);
}
 	
static void
on_button_edit (GtkButton *button, gpointer user_data)
{
	ModestConnectionSpecificSmtpWindow *self = MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (user_data);
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);
	
	gchar *id = NULL;
	gchar *connection_name = NULL;
	gchar *server_account_name = NULL;
	ModestServerAccountData *data = NULL;
	GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
	GtkTreeIter iter;
	GtkTreeModel *model = 0;
	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_ID, &id, 
				    MODEL_COL_NAME, &connection_name, 
				    MODEL_COL_SERVER_ACCOUNT_NAME, &server_account_name,
				    MODEL_COL_SERVER_ACCOUNT_DATA, &data,
				    -1);
	
		/* printf("DEBUG: %s: BEFORE: connection-specific server_account_name=%s\n", __FUNCTION__, server_account_name); */
		/* TODO: Is 0 an allowed libconic IAP ID? 
		 * If not then we should check for it. */
		
		/* Get existing server account data if a server account is already specified: */
		gboolean data_was_retrieved = FALSE;
		if (server_account_name && !data) {
			data = modest_account_mgr_get_server_account_data (priv->account_manager, 
				server_account_name);
			if (data)
				data_was_retrieved = TRUE;
		}
		
		GtkWidget * window = GTK_WIDGET (modest_connection_specific_smtp_edit_window_new ());
		modest_connection_specific_smtp_edit_window_set_connection (
			MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (window), id, connection_name, data);
			
		/* Delete data, unless it was data from the rowmodel: */
		if (data_was_retrieved) {
			modest_account_mgr_free_server_account_data (priv->account_manager, data);
			data = NULL;
		}
			
		gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (window));
		gint response = gtk_dialog_run (GTK_DIALOG (window));
		gtk_widget_hide (window);
		
		if (response == GTK_RESPONSE_OK) {
			/* Delete any previous data for this row: */
			if (data) 
			{
				modest_account_mgr_free_server_account_data (priv->account_manager, data);
				data = NULL;
			}
			
			/* Get the new account data and save it in the row for later:
			 * We free this in finalize(),
			 * and save it to our configuration in 
			 * modest_connection_specific_smtp_window_save_server_accounts(). */
			data = modest_connection_specific_smtp_edit_window_get_settings (
						MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (window), 
						priv->account_manager);
			
			const gchar* server_name = data ? data->hostname : NULL;
			gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter, 
					MODEL_COL_SERVER_ACCOUNT_DATA, data,
					MODEL_COL_SERVER_NAME, server_name,
					-1);
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
on_selection_changed (GtkTreeSelection *sel, ModestConnectionSpecificSmtpWindow *self)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);

	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	const gboolean has_selection =
		gtk_tree_selection_get_selected (sel, &model, &iter);

	gtk_widget_set_sensitive (priv->button_edit, has_selection);
}

static void
modest_connection_specific_smtp_window_init (ModestConnectionSpecificSmtpWindow *self)
{
	ModestWindowMgr *mgr;

	/* Specify a default size, because the GtkTreeView's default requested size  
	 * is not big enough: */
	gtk_window_set_default_size (GTK_WINDOW (self), 500, 300);
	
	/* This seems to be necessary to make the window show at the front with decoration.
	 * If we use property type=GTK_WINDOW_TOPLEVEL instead of the default GTK_WINDOW_POPUP+decoration, 
	 * then the window will be below the others. */
	gtk_window_set_type_hint (GTK_WINDOW (self),
			    GDK_WINDOW_TYPE_HINT_DIALOG);
			    
	ModestConnectionSpecificSmtpWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);

	/* Create a tree model for the tree view:
	 * with a string for the name, a string for the server name, and an int for the ID.
	 * This must match our MODEL_COLS enum constants.
	 */
	priv->model = GTK_TREE_MODEL (gtk_list_store_new (5, 
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER));

	/* Setup the tree view: */
	priv->treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (priv->model));

	/* Show the column headers,
	 * which does not seem to be the default on Maemo.
	 */			
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(priv->treeview), TRUE);
	
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
	
	GtkWidget *vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);

	/* Introductory note: */
	GtkWidget *label = gtk_label_new(_("mcen_ia_optionalsmtp_note"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	/* So that it is shown without being truncated: */
	gtk_label_set_max_width_chars (GTK_LABEL (label), 40);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, MODEST_MARGIN_HALF);
	
	/* Put the treeview in a scrolled window and add it to the box: */
	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), MODEST_MARGIN_DEFAULT);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, 
		GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scrolled_window);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->treeview));
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (scrolled_window), TRUE, TRUE, MODEST_MARGIN_HALF);
	gtk_widget_show (GTK_WIDGET (priv->treeview));
	
	/* Add the buttons: */
	GtkWidget *hbox = gtk_hbox_new (FALSE, MODEST_MARGIN_HALF);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (hbox);
	
	priv->button_edit = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_box_pack_start (GTK_BOX (hbox), priv->button_edit, TRUE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (priv->button_edit);
	g_signal_connect (G_OBJECT (priv->button_edit), "clicked",
        	G_CALLBACK (on_button_edit), self);
	
	GtkWidget *button_cancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_box_pack_start (GTK_BOX (hbox), button_cancel, TRUE, FALSE, MODEST_MARGIN_HALF);
	gtk_widget_show (button_cancel);
	g_signal_connect (G_OBJECT (button_cancel), "clicked",
        	G_CALLBACK (on_button_cancel), self);
	
	gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (vbox));
	gtk_widget_show (vbox);
	
	/* Disable the Edit button when nothing is selected: */
	GtkTreeSelection *sel = gtk_tree_view_get_selection (priv->treeview);
	g_signal_connect (sel, "changed",
			  G_CALLBACK(on_selection_changed), self);
	on_selection_changed (sel, self);
	
	/* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_prevent_hibernation_while_window_is_shown (mgr, 
								     GTK_WINDOW (self)); 

	/* Set window title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_optionalsmtp_servers"));
}

ModestConnectionSpecificSmtpWindow*
modest_connection_specific_smtp_window_new (void)
{
	return g_object_new (MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_WINDOW, NULL);
}

/** The application should call this when the user changes should be saved.
 * @account_name: Specify this again in case it was not previously known.
 */
gboolean
modest_connection_specific_smtp_window_save_server_accounts (ModestConnectionSpecificSmtpWindow *self, 
	const gchar* account_name)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);
	
	
	/* Get the first iter in the list */
	GtkTreeIter iter;
  	gboolean valid = gtk_tree_model_get_iter_first (priv->model, &iter);

	/* Walk through the list, reading each row */
	while (valid) {
      	gchar *id = NULL;
		gchar *connection_name = NULL;
		gchar *server_account_name = NULL;
		ModestServerAccountData *data = NULL;
		
		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_ID, &id, 
				    MODEL_COL_NAME, &connection_name, 
				    MODEL_COL_SERVER_ACCOUNT_NAME, &server_account_name,
				    MODEL_COL_SERVER_ACCOUNT_DATA, &data,
				    -1);
				 
		gboolean success = TRUE;   
		if (id && data) { /* The presence of data suggests that there is something to save. */
			if (!server_account_name) {
				/* Add a new server account, building a (non-human-visible) name: */
				gchar *name_start = g_strdup_printf("%s_specific_%s", 
					priv->account_name, connection_name);
				server_account_name = modest_account_mgr_get_unused_account_name (
					priv->account_manager, name_start, TRUE /* server account. */);
				g_assert (server_account_name);
				g_free (name_start);
				
				success = modest_account_mgr_add_server_account (priv->account_manager,
										 server_account_name,
										 data->hostname, 0,
										 data->username, data->password,
										 MODEST_PROTOCOL_TRANSPORT_SMTP,
										 data->security,
										 data->secure_auth);
					
				/* associate the specific server account with this connection for this account: */
				success = success && modest_account_mgr_set_connection_specific_smtp (
					priv->account_manager, priv->account_name,
					 connection_name, server_account_name);
	
				/* Save the new name in the treemodel, so it can be edited again later: */
				gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter, 
					MODEL_COL_SERVER_ACCOUNT_NAME, server_account_name, -1);
				
			} else {
				/* Change an existing server account: */
				success = modest_account_mgr_set_string (priv->account_manager, server_account_name,
					MODEST_ACCOUNT_HOSTNAME, data->hostname, TRUE /* server account */);
						
				modest_server_account_set_username (priv->account_manager, server_account_name,
					data->username);
							
				modest_server_account_set_password (priv->account_manager, server_account_name,
					data->password);
						
				modest_server_account_set_secure_auth (priv->account_manager, server_account_name, 
					data->secure_auth);
						
				modest_server_account_set_security (priv->account_manager, server_account_name, 
					data->security);
				
				modest_account_mgr_set_int (priv->account_manager, server_account_name,
						MODEST_ACCOUNT_PORT, data->port, TRUE /* server account */);
			}
		}
		
		g_free (connection_name);
		g_free (id);
		g_free (server_account_name);
		
		if (!success)
			return FALSE;
			
		/* Get next row: */
		valid = gtk_tree_model_iter_next (priv->model, &iter);
	}
	
	update_model_server_names (self);
	
	return TRUE;
}

void update_model_server_names (ModestConnectionSpecificSmtpWindow *self)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);

	GtkTreeIter iter;
  	gboolean valid = gtk_tree_model_get_iter_first (priv->model, &iter);
	while (valid) {
		
		gchar *server_account_name = NULL;
		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_SERVER_ACCOUNT_NAME, &server_account_name,
				    -1);
				 
		if (server_account_name) {
			/* Get the server hostname and show it in the treemodel: */	
			gchar *hostname = modest_account_mgr_get_string (priv->account_manager, 
				server_account_name, MODEST_ACCOUNT_HOSTNAME, TRUE /* server account */);
			gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter, 
					    MODEL_COL_SERVER_NAME, hostname,
					    -1);
			g_free (hostname);
		} else {
			gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter,
					    MODEL_COL_SERVER_NAME, _("mcen_ia_optionalsmtp_notdefined"),
					    -1);
		}
			
		/* Get next row: */
		valid = gtk_tree_model_iter_next (priv->model, &iter);
	}
}


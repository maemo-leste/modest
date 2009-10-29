/* Copyright (c) 2006, Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modest-connection-specific-smtp-window.h"
#include "modest-connection-specific-smtp-edit-window.h"
#include <modest-account-mgr-helpers.h>
#include "widgets/modest-ui-constants.h"

#include <modest-runtime.h>

#if MODEST_HAVE_CONIC
#include <tny-maemo-conic-device.h>
#endif

#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkliststore.h>
#include <modest-scrollable.h>
#include <modest-toolkit-factory.h>
#include <hildon/hildon-gtk.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkstock.h>

#include "modest-hildon-includes.h"
#include "modest-platform.h"
#include "modest-maemo-utils.h"

#include <glib/gi18n.h>
#include <string.h>

G_DEFINE_TYPE (ModestConnectionSpecificSmtpWindow, modest_connection_specific_smtp_window,
	       GTK_TYPE_DIALOG);

#define CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_WINDOW, ModestConnectionSpecificSmtpWindowPrivate))

typedef struct _ModestConnectionSpecificSmtpWindowPrivate ModestConnectionSpecificSmtpWindowPrivate;

struct _ModestConnectionSpecificSmtpWindowPrivate
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkWidget *no_connection_label;
	GtkWidget *scrollable;
	ModestAccountMgr *account_manager;
};

static void on_response (GtkDialog *dialog, 
			 gint response, 
			 gpointer user_data);

/* static gboolean on_key_pressed (GtkWidget *self, GdkEventKey *event, gpointer user_data); */

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
	MODEL_COL_SERVER_ACCOUNT_SETTINGS = 4 /* a gpointer */
};


static void update_model_server_names (ModestConnectionSpecificSmtpWindow *self);

static void
modest_connection_specific_smtp_window_finalize (GObject *object)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (object);

	/* Free all the data items from the treemodel: */
	GtkTreeIter iter;
  	gboolean valid = gtk_tree_model_get_iter_first (priv->model, &iter);
	while (valid) {
		ModestServerAccountSettings *server_settings = NULL;

		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_SERVER_ACCOUNT_SETTINGS, &server_settings,
				    -1);

		if (server_settings)
			g_object_unref (server_settings);

		/* Get next row: */
		valid = gtk_tree_model_iter_next (priv->model, &iter);
	}

	g_object_unref (G_OBJECT (priv->model));

	g_object_unref (priv->treeview);
	g_object_unref (priv->no_connection_label);

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
modest_connection_specific_smtp_window_fill_with_connections (ModestConnectionSpecificSmtpWindow *self,
							      ModestAccountMgr *account_manager)
{
	gboolean empty = TRUE;
	ModestConnectionSpecificSmtpWindowPrivate *priv = 
		CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);
	priv->account_manager = account_manager;
#ifdef MODEST_HAVE_CONIC
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
	if (list_iaps != NULL)
		empty = FALSE;

	while (iter) {
		ConIcIap *iap = (ConIcIap*)iter->data;
		if (iap) {
			#ifdef DEBUG_WITHOUT_LIBCONIC
			const gchar *connection_name = "debug name";
			const gchar *connection_id = "debug id";
			#else
			const gchar *connection_name = con_ic_iap_get_name (iap);
			const gchar *connection_id = con_ic_iap_get_id (iap);
			#endif
			
			printf ("debug: iac name=%s, id=%s\n", connection_name, connection_id);
			
			/* Get any already-associated connection-specific server account: */
			gchar *server_account_name = NULL;
			server_account_name = modest_account_mgr_get_connection_specific_smtp (
				priv->account_manager, connection_id);
					
			/* Add the row to the model: */
			GtkTreeIter iter;
			gtk_list_store_append (liststore, &iter);
			gtk_list_store_set(liststore, &iter, 
				MODEL_COL_ID, connection_id, 
				MODEL_COL_NAME, connection_name,
				MODEL_COL_SERVER_ACCOUNT_NAME, server_account_name,
				-1);
				
			if (server_account_name)
				g_free (server_account_name);
		}
		
		iter = g_slist_next (iter);	
	}
		
	#ifndef DEBUG_WITHOUT_LIBCONIC
	if (list_iaps)
		tny_maemo_conic_device_free_iap_list (maemo_device, list_iaps);
	#endif
		
	update_model_server_names (self);
#endif /*MODEST_HAVE_CONIC */

	GtkWidget *child;
	child = gtk_bin_get_child (GTK_BIN (priv->scrollable));
	if (child) {
		gtk_container_remove (GTK_CONTAINER (priv->scrollable), child);
	}

	if (empty) {
		modest_scrollable_add_with_viewport (MODEST_SCROLLABLE (priv->scrollable), 
						     priv->no_connection_label);
		gtk_widget_show (priv->no_connection_label);
	} else {
		gtk_container_add (GTK_CONTAINER (priv->scrollable), GTK_WIDGET (priv->treeview));
		gtk_widget_show (GTK_WIDGET (priv->treeview));
	}
}
	
static void
edit_account (ModestConnectionSpecificSmtpWindow *self, GtkTreePath *path)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);
	ModestAccountMgr *mgr = modest_runtime_get_account_mgr ();
	
	gchar *id = NULL;
	gchar *connection_name = NULL;
	gchar *server_account_name = NULL;
	ModestServerAccountSettings *server_settings = NULL;
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter (priv->model, &iter, path)) {
		gtk_tree_model_get (priv->model, &iter,
				    MODEL_COL_ID, &id,
				    MODEL_COL_NAME, &connection_name,
				    MODEL_COL_SERVER_ACCOUNT_NAME, &server_account_name,
				    MODEL_COL_SERVER_ACCOUNT_SETTINGS, &server_settings,
				    -1);

		/* Get existing server account data if a server account is already specified: */
		gboolean settings_were_retrieved = FALSE;
		if (server_account_name && !server_settings) {
			server_settings = modest_account_mgr_load_server_settings(mgr, server_account_name, TRUE);
			if (server_settings)
				settings_were_retrieved = TRUE;
		}

		GtkWidget * window = GTK_WIDGET (modest_connection_specific_smtp_edit_window_new ());
		modest_connection_specific_smtp_edit_window_set_connection (
			MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (window), id, connection_name, server_settings);

		/* Delete data, unless it was data from the rowmodel: */
		if (settings_were_retrieved) {
			g_object_unref (server_settings);
			server_settings = NULL;
		}

		modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), GTK_WINDOW (window), GTK_WINDOW (self));

		gint response = gtk_dialog_run (GTK_DIALOG (window));
		if (response == GTK_RESPONSE_OK) {

			/* Delete any previous data for this row: */
			if (server_settings) {
				g_object_unref (server_settings);
				server_settings = NULL;
			}

			/* Get the new account data and save it in the row for later:
			 * We free this in finalize(),
			 * and save it to our configuration in
			 * modest_connection_specific_smtp_window_save_server_accounts(). */
			server_settings = modest_connection_specific_smtp_edit_window_get_settings (
												    MODEST_CONNECTION_SPECIFIC_SMTP_EDIT_WINDOW (window));

			if (server_settings) {
				gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter,
						    MODEL_COL_SERVER_ACCOUNT_SETTINGS, server_settings,
						    MODEL_COL_SERVER_NAME, modest_server_account_settings_get_hostname (server_settings),
						    -1);
			} else {
				gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter,
						    MODEL_COL_SERVER_ACCOUNT_SETTINGS, NULL,
						    MODEL_COL_SERVER_NAME, NULL,
						    MODEL_COL_SERVER_ACCOUNT_NAME, NULL,
						    -1);
			}
		}
		gtk_widget_destroy (window);
	}
	g_free (connection_name);
	g_free (id);
	g_free (server_account_name);
	update_model_server_names (self);
}

static void
on_row_activated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, 
		  ModestConnectionSpecificSmtpWindow *self)
{
	edit_account (self, path);
}

static void
modest_connection_specific_smtp_window_init (ModestConnectionSpecificSmtpWindow *self)
{
	ModestWindowMgr *mgr;
	GtkWidget *align;

	/* Specify a default size */
	gtk_window_set_default_size (GTK_WINDOW (self), -1, MODEST_DIALOG_WINDOW_MAX_HEIGHT);

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
	priv->treeview = GTK_TREE_VIEW (hildon_gtk_tree_view_new_with_model (HILDON_UI_MODE_NORMAL, priv->model));
	g_object_ref_sink (G_OBJECT (priv->treeview));

	/* No connections label */
	priv->no_connection_label = gtk_label_new (_("mcen_ia_optionalsmtp_noconnection"));
	g_object_ref_sink (G_OBJECT (priv->no_connection_label));

	/* name column:
	 * The ID model column in not shown in the view. */
	GtkTreeViewColumn *view_column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_expand (view_column, TRUE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xpad", MODEST_MARGIN_DOUBLE, NULL);
	gtk_tree_view_column_pack_start(view_column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (view_column, renderer, 
	"text", MODEL_COL_NAME, NULL);
	gtk_tree_view_append_column (priv->treeview, view_column);

	/* server name column: */
	view_column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_expand (view_column, TRUE);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(view_column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (view_column, renderer, 
	"text", MODEL_COL_SERVER_NAME, NULL);
	gtk_tree_view_append_column (priv->treeview, view_column);

	/* The application must call modest_connection_specific_smtp_window_fill_with_connections(). */

	GtkWidget *vbox = GTK_DIALOG(self)->vbox;

	/* Introductory note: */
	GtkWidget *label = gtk_label_new(_("mcen_ia_optionalsmtp_note"));
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), MODEST_MARGIN_DOUBLE + MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE);
	gtk_widget_set_size_request (label, 600, -1);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	/* Put the treeview in a scrollable and add it to the box: */
	priv->scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());
	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DOUBLE, 0);
	gtk_widget_show (priv->scrollable);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), GTK_WIDGET (priv->scrollable));
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (align), TRUE, TRUE, 0);
	gtk_widget_show (vbox);

	g_signal_connect (G_OBJECT (priv->treeview), "row-activated", G_CALLBACK (on_row_activated), self);

	/* When this window is shown, hibernation should not be possible, 
	 * because there is no sensible way to save the state: */
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_prevent_hibernation_while_window_is_shown (mgr, 
								     GTK_WINDOW (self)); 

	/* Set window title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_optionalsmtp_servers"));

	g_signal_connect (self, "response", G_CALLBACK (on_response), NULL);
}

ModestConnectionSpecificSmtpWindow*
modest_connection_specific_smtp_window_new (void)
{
	return g_object_new (MODEST_TYPE_CONNECTION_SPECIFIC_SMTP_WINDOW, NULL);
}

/** The application should call this when the user changes should be saved.
 */
gboolean
modest_connection_specific_smtp_window_save_server_accounts (ModestConnectionSpecificSmtpWindow *self)
{
	ModestAccountMgr *mgr = modest_runtime_get_account_mgr ();
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
		gchar *server_name = NULL;
		ModestServerAccountSettings *server_settings = NULL;

		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_ID, &id, 
				    MODEL_COL_NAME, &connection_name, 
				    MODEL_COL_SERVER_NAME, &server_name,
				    MODEL_COL_SERVER_ACCOUNT_NAME, &server_account_name,
				    MODEL_COL_SERVER_ACCOUNT_SETTINGS, &server_settings,
				    -1);
 
		gboolean success = TRUE;
		if (id && server_settings) { /* The presence of data suggests that there is something to save. */
			if (!server_account_name) {
				/* Add a new server account, building a (non-human-visible) name: */
				gchar *name_start = g_strdup_printf("specific_%s", connection_name);
				server_account_name = modest_account_mgr_get_unused_account_name (
					priv->account_manager, name_start, TRUE /* server account. */);
				g_assert (server_account_name);
				g_free (name_start);

				modest_server_account_settings_set_account_name (server_settings, server_account_name);
				success = modest_account_mgr_save_server_settings (mgr, server_settings);
				if (success) {
					TnyAccount *account = TNY_ACCOUNT (modest_tny_account_store_new_connection_specific_transport_account 
									   (modest_runtime_get_account_store (),
									    server_account_name));
					if (account)
						g_object_unref (account);
				}

				/* associate the specific server account with this connection for this account: */
				success = success && modest_account_mgr_set_connection_specific_smtp (
					priv->account_manager, id, server_account_name);

				/* Save the new name in the treemodel, so it can be edited again later: */
				gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter, 
					MODEL_COL_SERVER_ACCOUNT_NAME, server_account_name, -1);

			} else {
				/* If the account already exists then update it and notify */
				modest_account_mgr_save_server_settings (mgr, server_settings);
			}
		} else if (id && server_name && 
			   !strcmp (server_name, _("mcen_ia_optionalsmtp_notdefined"))) {
			modest_account_mgr_remove_connection_specific_smtp (priv->account_manager, 
									    id);
			gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter, 
					    MODEL_COL_SERVER_ACCOUNT_NAME, NULL, -1);
		}

		g_free (connection_name);
		g_free (id);
		g_free (server_account_name);
		g_free (server_name);

		if (!success)
			return FALSE;

		/* Get next row: */
		valid = gtk_tree_model_iter_next (priv->model, &iter);
	}

	update_model_server_names (self);

	return TRUE;
}

static void
update_model_server_names (ModestConnectionSpecificSmtpWindow *self)
{
	ModestConnectionSpecificSmtpWindowPrivate *priv = CONNECTION_SPECIFIC_SMTP_WINDOW_GET_PRIVATE (self);

	GtkTreeIter iter;
  	gboolean valid = gtk_tree_model_get_iter_first (priv->model, &iter);
	while (valid) {

		gchar *server_account_name = NULL;
		ModestServerAccountSettings *server_settings = NULL;
		gtk_tree_model_get (priv->model, &iter, 
				    MODEL_COL_SERVER_ACCOUNT_NAME, &server_account_name,
				    MODEL_COL_SERVER_ACCOUNT_SETTINGS, &server_settings,
				    -1);
		if (server_settings && modest_server_account_settings_get_hostname (server_settings)
		    && (modest_server_account_settings_get_hostname (server_settings) [0] != '\0')) {
			gtk_list_store_set (GTK_LIST_STORE (priv->model), &iter, 
					    MODEL_COL_SERVER_NAME, modest_server_account_settings_get_hostname (server_settings),
					    -1);
		} else if (server_account_name) {

			/* Get the server hostname and show it in the treemodel: */
			gchar *hostname = modest_account_mgr_get_server_account_hostname (priv->account_manager,
											  server_account_name);
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

static void
on_response (GtkDialog *dialog,
	     gint response,
	     gpointer user_data)
{
	modest_connection_specific_smtp_window_save_server_accounts (MODEST_CONNECTION_SPECIFIC_SMTP_WINDOW (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

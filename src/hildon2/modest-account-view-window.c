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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <widgets/modest-account-view-window.h>
#include <widgets/modest-account-view.h>

#include <modest-runtime.h>
#include "modest-platform.h"
#include "modest-account-protocol.h"
#include <modest-account-mgr-helpers.h>
#include <string.h>
#include "modest-tny-platform-factory.h"
#include "modest-easysetup-wizard-dialog.h"
#include "modest-account-settings-dialog.h"
#include <modest-utils.h>
#include "widgets/modest-ui-constants.h"
#include <hildon/hildon-pannable-area.h>

/* 'private'/'protected' functions */
static void                            modest_account_view_window_class_init   (ModestAccountViewWindowClass *klass);
static void                            modest_account_view_window_init         (ModestAccountViewWindow *obj);
static void                            modest_account_view_window_finalize     (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestAccountViewWindowPrivate ModestAccountViewWindowPrivate;
struct _ModestAccountViewWindowPrivate {
	GtkWidget           *new_button;
	GtkWidget           *edit_button;
	ModestAccountView   *account_view;
	guint acc_removed_handler;
};
#define MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                        MODEST_TYPE_ACCOUNT_VIEW_WINDOW, \
                                                        ModestAccountViewWindowPrivate))
/* globals */
static GtkDialogClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_account_view_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountViewWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_account_view_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountViewWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_account_view_window_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_DIALOG,
		                                  "ModestAccountViewWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_account_view_window_class_init (ModestAccountViewWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_account_view_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountViewWindowPrivate));
}

static void
modest_account_view_window_finalize (GObject *self)
{
	ModestAccountViewWindowPrivate *priv;
	ModestAccountMgr *mgr;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE (self);
	mgr = modest_runtime_get_account_mgr ();

	if (g_signal_handler_is_connected (mgr, priv->acc_removed_handler))
		g_signal_handler_disconnect (mgr, priv->acc_removed_handler);
	priv->acc_removed_handler = 0;

	G_OBJECT_CLASS(parent_class)->finalize (self);
}


static void
on_selection_changed (GtkTreeSelection *sel, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	GtkTreeModel                   *model;
	GtkTreeIter                     iter;
	gboolean                        has_selection;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	has_selection =
		gtk_tree_selection_get_selected (sel, &model, &iter);

	/* Set the status of the buttons */
	gtk_widget_set_sensitive (priv->edit_button, has_selection);
}

/** Check whether any connections are active, and cancel them if 
 * the user wishes.
 * Returns TRUE is there was no problem, 
 * or if an operation was cancelled so we can continue.
 * Returns FALSE if the user chose to cancel his request instead.
 */
static gboolean
check_for_active_account (ModestAccountViewWindow *self, const gchar* account_name)
{
	ModestTnySendQueue *send_queue;
	ModestTnyAccountStore *acc_store;
	ModestMailOperationQueue* queue;
	TnyConnectionStatus store_conn_status;
	TnyAccount *store_account = NULL, *transport_account = NULL;
	gboolean retval = TRUE, sending = FALSE;

	acc_store = modest_runtime_get_account_store ();
	queue = modest_runtime_get_mail_operation_queue ();

	store_account = 
		modest_tny_account_store_get_server_account (acc_store,
							     account_name,
							     TNY_ACCOUNT_TYPE_STORE);

	/* This could happen if the account was deleted before the
	   call to this function */
	if (!store_account)
		return FALSE;

	transport_account = 
		modest_tny_account_store_get_server_account (acc_store,
							     account_name,
							     TNY_ACCOUNT_TYPE_TRANSPORT);

	/* This could happen if the account was deleted before the
	   call to this function */
	if (!transport_account) {
		g_object_unref (store_account);
		return FALSE;
	}

	/* If the transport account was not used yet, then the send
	   queue could not exist (it's created on demand) */
	send_queue = modest_runtime_get_send_queue (TNY_TRANSPORT_ACCOUNT (transport_account), FALSE);
	if (TNY_IS_SEND_QUEUE (send_queue))
		sending = modest_tny_send_queue_sending_in_progress (send_queue);

	store_conn_status = tny_account_get_connection_status (store_account);
	if (store_conn_status == TNY_CONNECTION_STATUS_CONNECTED || sending) {
		gint response;

		response = modest_platform_run_confirmation_dialog (GTK_WINDOW (self), 
								_("emev_nc_disconnect_account"));
		if (response == GTK_RESPONSE_OK) {
			retval = TRUE;
		} else {
			retval = FALSE;
		}
	}

	if (retval) {

		/* FIXME: We should only cancel those of this account */
		modest_mail_operation_queue_cancel_all (queue);

		/* Also disconnect the account */
		if ((tny_account_get_connection_status (store_account) != TNY_CONNECTION_STATUS_DISCONNECTED) &&
		    (tny_account_get_connection_status (store_account) != TNY_CONNECTION_STATUS_DISCONNECTED_BROKEN)) {
			tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (store_account),
						      FALSE, NULL, NULL);
		}
		if (sending) {
			tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (transport_account),
						      FALSE, NULL, NULL);
		}
	}
		
	/* Frees */
	g_object_unref (store_account);
	g_object_unref (transport_account);
	
	return retval;
}

static void
on_account_settings_dialog_response (GtkDialog *dialog,
				     gint response,
				     gpointer user_data)
{
	TnyAccount *store_account = NULL;
	gchar* account_name = NULL;
	ModestAccountViewWindowPrivate *priv = NULL;

	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE (user_data);
	account_name = modest_account_view_get_selected_account (priv->account_view);
	store_account = modest_tny_account_store_get_server_account (modest_runtime_get_account_store (),
								     account_name,
								     TNY_ACCOUNT_TYPE_STORE);
       
	/* Reconnect the store account, no need to reconnect the
	   transport account because it will connect when needed */
	if (tny_account_get_connection_status (store_account) == 
	    TNY_CONNECTION_STATUS_DISCONNECTED)
		tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (store_account),
					      TRUE, NULL, NULL);

	/* Disconnect this handler */
	g_signal_handlers_disconnect_by_func (dialog, on_account_settings_dialog_response, user_data);

	/* Free */
	g_free (account_name);
	g_object_unref (store_account);
}

static void
on_edit_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE (self);
	
	gchar* account_name = modest_account_view_get_selected_account (priv->account_view);
	if (!account_name)
		return;
		
	/* Check whether any connections are active, and cancel them if 
	 * the user wishes.
	 */
	if (check_for_active_account (self, account_name)) {
		ModestAccountProtocol *proto;
		ModestProtocolType proto_type;

		/* Get proto */
		proto_type = modest_account_mgr_get_store_protocol (modest_runtime_get_account_mgr (), 
								    account_name);
		proto = (ModestAccountProtocol *)
			modest_protocol_registry_get_protocol_by_type (modest_runtime_get_protocol_registry (), 
								       proto_type);

		/* Create and show the dialog */
		if (proto && MODEST_IS_ACCOUNT_PROTOCOL (proto)) {
			ModestAccountSettingsDialog *dialog =
				modest_account_protocol_get_account_settings_dialog (proto, account_name);
			modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), GTK_WINDOW (dialog), GTK_WINDOW (self));
			gtk_widget_show (GTK_WIDGET (dialog));
		}
	}
	g_free (account_name);
}

static void
on_wizard_response (GtkDialog *dialog, 
		    gint response, 
		    gpointer user_data)
{	
	/* The response has already been handled by the wizard dialog itself,
	 * creating the new account.
	 */	 
	if (dialog)
		gtk_widget_destroy (GTK_WIDGET (dialog));

	/* Re-focus the account list view widget */
	if (MODEST_IS_ACCOUNT_VIEW_WINDOW (user_data)) {
		ModestAccountViewWindowPrivate *priv;
		priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE (user_data);
		gtk_widget_grab_focus (GTK_WIDGET (priv->account_view));
	}
}

static void
on_new_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	GtkDialog *wizard;

	/* there is no such wizard yet */
	wizard = GTK_DIALOG (modest_easysetup_wizard_dialog_new ());
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr(), 
				     GTK_WINDOW (wizard), GTK_WINDOW (self));

	gtk_window_set_modal (GTK_WINDOW (wizard), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (wizard), GTK_WINDOW (self));
	/* Destroy the dialog when it is closed: */
	g_signal_connect (G_OBJECT (wizard), "response", G_CALLBACK
			  (on_wizard_response), self);
	gtk_widget_show (GTK_WIDGET (wizard));
}

static void
setup_button_box (ModestAccountViewWindow *self, GtkButtonBox *box)
{
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (box), 6);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (box), 
				   GTK_BUTTONBOX_START);
	
	priv->new_button     = gtk_button_new_from_stock(_("mcen_bd_new"));
	priv->edit_button = gtk_button_new_with_label(_("mcen_bd_edit"));
	
	g_signal_connect (G_OBJECT(priv->new_button), "clicked",
			  G_CALLBACK(on_new_button_clicked),
			  self);
	g_signal_connect (G_OBJECT(priv->edit_button), "clicked",
			  G_CALLBACK(on_edit_button_clicked),
			  self);

	gtk_box_pack_start (GTK_BOX(box), priv->new_button, FALSE, FALSE,2);
	gtk_box_pack_start (GTK_BOX(box), priv->edit_button, FALSE, FALSE,2);

	/* Should has been created by window_vbox_new */
	if (priv->account_view) {
		GtkTreeSelection *sel;
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->account_view));
		if (gtk_tree_selection_count_selected_rows (sel) == 0) {
			gtk_widget_set_sensitive (priv->edit_button, FALSE);
		}
	}

	gtk_widget_show_all (GTK_WIDGET (box));
}

static GtkWidget*
window_vbox_new (ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	GtkWidget *main_vbox, *main_hbox, *pannable;
	GtkTreeSelection *sel;

	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	priv->account_view = modest_account_view_new (modest_runtime_get_account_mgr());

	main_vbox = gtk_vbox_new (FALSE, 6);
	main_hbox = gtk_hbox_new (FALSE, 6);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->account_view));
	g_signal_connect (G_OBJECT(sel), "changed",  G_CALLBACK(on_selection_changed),
			  self);
			  
	pannable = g_object_new (HILDON_TYPE_PANNABLE_AREA, "initial-hint", TRUE, NULL);
	hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA (pannable), 
						GTK_WIDGET (priv->account_view));

	/* Only force the height, the width of the widget will depend
	   on the size of the column titles */
	gtk_widget_set_size_request (pannable, -1, 320);
	
	gtk_box_pack_start (GTK_BOX(main_hbox), pannable, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(main_vbox), main_hbox, TRUE, TRUE, 2);

	gtk_widget_show_all (pannable);
	gtk_widget_show (main_hbox);
	gtk_widget_show (main_vbox);

	return main_vbox;
}


static void
modest_account_view_window_init (ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	priv->acc_removed_handler = 0;
}

static void
on_account_removed (ModestAccountMgr *acc_mgr, 
		    const gchar *account,
		    gpointer user_data)
{
	ModestAccountViewWindowPrivate *priv;

	/* If there is no account left then close the window */
	if (!modest_account_mgr_has_accounts (acc_mgr, TRUE)) {
		gboolean ret_value;
		g_signal_emit_by_name (G_OBJECT (user_data), "delete-event", NULL, &ret_value);
	} else {		
		/* Re-focus the account list view widget */
		priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE (user_data);
		gtk_widget_grab_focus (GTK_WIDGET (priv->account_view));
	}
}

GtkWidget*
modest_account_view_window_new (void)
{
	GObject *self = g_object_new(MODEST_TYPE_ACCOUNT_VIEW_WINDOW, NULL);
	ModestAccountViewWindowPrivate *priv;
	ModestAccountMgr *account_mgr = modest_runtime_get_account_mgr ();

	/* Add widgets */
	gtk_box_pack_start (GTK_BOX((GTK_DIALOG (self)->vbox)), 
			    window_vbox_new (MODEST_ACCOUNT_VIEW_WINDOW (self)), 
			    TRUE, TRUE, 2);
	
	setup_button_box (MODEST_ACCOUNT_VIEW_WINDOW (self), GTK_BUTTON_BOX (GTK_DIALOG (self)->action_area));

	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_emailsetup_accounts"));

	/* Connect signals */
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	priv->acc_removed_handler = g_signal_connect (G_OBJECT(account_mgr), "account_removed",
						      G_CALLBACK (on_account_removed), self);
	
	return GTK_WIDGET (self);
}

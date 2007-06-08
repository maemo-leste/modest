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
#include <modest-account-mgr-helpers.h>
#include <string.h>
#include "modest-tny-platform-factory.h"
#include "maemo/easysetup/modest-easysetup-wizard.h"
#include "maemo/modest-account-settings-dialog.h"
#include "widgets/modest-ui-constants.h"

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
	GtkWidget           *delete_button;
	GtkWidget	    *close_button;
	ModestAccountView   *account_view;
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

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_account_view_window_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void
on_selection_changed (GtkTreeSelection *sel, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	GtkTreeModel                   *model;
	GtkTreeIter                     iter;
	gboolean                        has_selection;
	gchar                          *account_name;
	gchar                          *default_account_name;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	has_selection =
		gtk_tree_selection_get_selected (sel, &model, &iter);

	gtk_widget_set_sensitive (priv->edit_button, has_selection);
	gtk_widget_set_sensitive (priv->delete_button, has_selection);	

	account_name = modest_account_view_get_selected_account (priv->account_view);
	default_account_name = modest_account_mgr_get_default_account(
		modest_runtime_get_account_mgr());

	g_free (account_name);
	g_free (default_account_name);
}

static void
on_delete_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	ModestAccountMgr *account_mgr;
	
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	account_mgr = modest_runtime_get_account_mgr();	
	gchar *account_name = modest_account_view_get_selected_account (priv->account_view);
	if(!account_name)
		return;
		
	gchar *account_title = modest_account_mgr_get_display_name(account_mgr, account_name);

	if (account_name) {
		gboolean removed;
		GtkWidget *dialog;
		gchar *txt;
		
		dialog = gtk_dialog_new_with_buttons (_("Confirmation dialog"),
						      GTK_WINDOW (self),
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_REJECT,
						      GTK_STOCK_OK,
						      GTK_RESPONSE_ACCEPT,
						      NULL);
		txt = g_strdup_printf (_("emev_nc_delete_mailboximap"), 
			account_title);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
				    gtk_label_new (txt), FALSE, FALSE, 0);
		gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
		g_free (txt);

		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			/* Remove account. If succeeded it removes also 
			   the account from the ModestAccountView */
			  
			gboolean is_default = FALSE;
			gchar *default_account_name = modest_account_mgr_get_default_account (account_mgr);
			if (default_account_name && (strcmp (default_account_name, account_name) == 0))
				is_default = TRUE;
			g_free (default_account_name);
				
			removed = modest_account_mgr_remove_account (account_mgr,
									     account_name,
									     FALSE);
									     
			if (removed && is_default) {
				/* Set a different account as the default, so there is always at least one default:
				 * This is not specified, and might be the wrong behaviour. murrayc. */
				modest_account_mgr_set_first_account_as_default (account_mgr);
			}
									 
			if (removed) {
				/* Show confirmation dialog ??? */
			} else {
				/* Show error dialog ??? */
			}
		}
		gtk_widget_destroy (dialog);
		g_free (account_title);
		g_free (account_name);
	}
}

static void
on_edit_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	
	gchar* account_name = modest_account_view_get_selected_account (priv->account_view);
	if (!account_name)
		return;
		
	/* Check whether any connections are active, and cancel them if 
	 * the user wishes.
	 * TODO: Check only for connections with this account, instead of all.
	 * Maybe we need a queue per account.
	 */
	ModestMailOperationQueue* queue = modest_runtime_get_mail_operation_queue ();
	if (modest_mail_operation_queue_num_elements(queue)) {
		GtkWidget *note = hildon_note_new_confirmation (GTK_WINDOW (self), 
			_("emev_nc_disconnect_account"));
		const int response = gtk_dialog_run (GTK_DIALOG(note));
		gtk_widget_destroy (note);
		if (response == GTK_RESPONSE_OK) {
			modest_mail_operation_queue_cancel_all(queue);;
		}
		else
			return;
	}
		
	/* Show the Account Settings window: */
	ModestAccountSettingsDialog *dialog = modest_account_settings_dialog_new ();
	modest_account_settings_dialog_set_account_name (dialog, account_name);
	
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (self));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	g_free (account_name);
}

static void
on_new_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	/* Show the easy-setup wizard: */
	ModestEasysetupWizardDialog *wizard = modest_easysetup_wizard_dialog_new ();
	gtk_window_set_transient_for (GTK_WINDOW (wizard), GTK_WINDOW (self));
	gtk_dialog_run (GTK_DIALOG (wizard));
	gtk_widget_destroy (GTK_WIDGET (wizard));
}


static void
on_close_button_clicked (GtkWidget *button, gpointer user_data)
{		
	ModestAccountViewWindow *self = MODEST_ACCOUNT_VIEW_WINDOW (user_data);

	gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}



static GtkWidget*
button_box_new (ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	
	GtkWidget *button_box = gtk_hbutton_box_new ();
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (button_box), 6);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), 
				   GTK_BUTTONBOX_START);
	
	priv->new_button     = gtk_button_new_from_stock(_("mcen_bd_new"));
	priv->edit_button = gtk_button_new_with_label(_("mcen_bd_edit"));
	priv->delete_button  = gtk_button_new_from_stock(_("mcen_bd_delete"));
	priv->close_button    = gtk_button_new_from_stock(_("mcen_bd_close"));
	
	g_signal_connect (G_OBJECT(priv->new_button), "clicked",
			  G_CALLBACK(on_new_button_clicked),
			  self);
	g_signal_connect (G_OBJECT(priv->delete_button), "clicked",
			  G_CALLBACK(on_delete_button_clicked),
			  self);
	g_signal_connect (G_OBJECT(priv->edit_button), "clicked",
			  G_CALLBACK(on_edit_button_clicked),
			  self);
	g_signal_connect (G_OBJECT(priv->close_button), "clicked",
			  G_CALLBACK(on_close_button_clicked),
			  self);
	
	gtk_box_pack_start (GTK_BOX(button_box), priv->new_button, FALSE, FALSE,2);
	gtk_box_pack_start (GTK_BOX(button_box), priv->edit_button, FALSE, FALSE,2);
	gtk_box_pack_start (GTK_BOX(button_box), priv->delete_button, FALSE, FALSE,2);
	gtk_box_pack_start (GTK_BOX(button_box), priv->close_button, FALSE, FALSE,2);

	gtk_widget_set_sensitive (priv->edit_button, FALSE);
	gtk_widget_set_sensitive (priv->delete_button, FALSE);	

	gtk_widget_show_all (button_box);
	return button_box;
}


static GtkWidget*
window_vbox_new (ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	GtkWidget *main_vbox     = gtk_vbox_new (FALSE, 6);
	GtkWidget *main_hbox     = gtk_hbox_new (FALSE, 6);
	
	priv->account_view = modest_account_view_new (modest_runtime_get_account_mgr());
	gtk_widget_set_size_request (GTK_WIDGET(priv->account_view), 300, 400);
	gtk_widget_show (GTK_WIDGET (priv->account_view));

	GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->account_view));
	g_signal_connect (G_OBJECT(sel), "changed",  G_CALLBACK(on_selection_changed),
			  self);
			  
	GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), MODEST_MARGIN_DEFAULT);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, 
		GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->account_view));
	gtk_widget_show (GTK_WIDGET (scrolled_window));
	
	gtk_box_pack_start (GTK_BOX(main_hbox), GTK_WIDGET(scrolled_window), TRUE, TRUE, 2);
	
	gtk_box_pack_start (GTK_BOX(main_vbox), main_hbox, TRUE, TRUE, 2);
	gtk_widget_show (GTK_WIDGET (main_hbox));
	gtk_widget_show (GTK_WIDGET (main_vbox));

	return main_vbox;
}


static void
modest_account_view_window_init (ModestAccountViewWindow *obj)
{
/*
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(obj);
*/		
	gtk_box_pack_start (GTK_BOX((GTK_DIALOG (obj)->vbox)), GTK_WIDGET (window_vbox_new (obj)), 
		TRUE, TRUE, 2);
	
	gtk_box_pack_start (GTK_BOX((GTK_DIALOG (obj)->action_area)), GTK_WIDGET (button_box_new (obj)), 
		TRUE, TRUE, 2);

	gtk_window_set_title (GTK_WINDOW (obj), _("mcen_ti_emailsetup_accounts"));
}


GtkWidget*
modest_account_view_window_new (void)
{
	GObject *obj = g_object_new(MODEST_TYPE_ACCOUNT_VIEW_WINDOW, NULL);
	
	return GTK_WIDGET(obj);
}

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
#include "modest-ui-actions.h"
#include <modest-runtime.h>
#include <modest-account-mgr-helpers.h>
#include <string.h>
#include "modest-account-assistant.h"
#include "modest-account-protocol.h"
#include "modest-tny-platform-factory.h"
#include "modest-platform.h"

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
	GtkWidget           *add_button;
	GtkWidget           *edit_button;
	GtkWidget           *remove_button;
	GtkWidget	    *default_button;
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
}

static void
modest_account_view_window_init (ModestAccountViewWindow *obj)
{
	/* empty */
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
	GtkTreeIter                    iter;
	gboolean                       has_selection;
	gchar                         *account_name;
	gchar                         *default_account_name;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	has_selection =
		gtk_tree_selection_get_selected (sel, &model, &iter);

	gtk_widget_set_sensitive (priv->edit_button, has_selection);
	gtk_widget_set_sensitive (priv->remove_button, has_selection);	

	account_name = modest_account_view_get_selected_account (priv->account_view);
	default_account_name = modest_account_mgr_get_default_account(
		modest_runtime_get_account_mgr());
	gtk_widget_set_sensitive (priv->default_button,
				  default_account_name == NULL || account_name == NULL ||
				  strcmp (default_account_name, account_name) != 0);
	g_free (account_name);
	g_free (default_account_name);
}

static void
on_remove_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	ModestAccountMgr *account_mgr;
	gchar *account_name, *account_title;
	
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	account_mgr = modest_runtime_get_account_mgr();	
	account_name = modest_account_view_get_selected_account (priv->account_view);

	if (!account_name)
		return;

	account_title = modest_account_mgr_get_display_name(account_mgr, account_name);
	if (!account_title)
		return;

	if (modest_ui_actions_check_for_active_account (self, account_name)) {
		gboolean removed;
		gchar *txt;
		gint response;

		if (modest_account_mgr_get_store_protocol (account_mgr, account_name) 
		    == MODEST_PROTOCOLS_STORE_POP) {
				txt = g_strdup_printf (_("emev_nc_delete_mailbox"), 
						       account_title);
		} else {
			txt = g_strdup_printf (_("emev_nc_delete_mailboximap"), 
					       account_title);
		}
		
		response = modest_platform_run_confirmation_dialog (GTK_WINDOW (self), txt);
		g_free (txt);
		txt = NULL;

		if (response == GTK_RESPONSE_OK) {
			/* Remove account. If succeeded it removes also 
			   the account from the ModestAccountView */
			removed = modest_account_mgr_remove_account (account_mgr,
								     account_name);
			if (removed) {
				/* Show confirmation dialog ??? */
			} else {
				/* Show error dialog ??? */
				g_warning ("Error removing account %s", account_name);
			}
		}
		g_free (account_name);
	}
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
	if (modest_ui_actions_check_for_active_account (self, account_name)) {
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
on_add_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	GtkDialog *wizard;
	GtkWindow *dialog;

	/* Show the easy-setup wizard: */	
	dialog = modest_window_mgr_get_modal (modest_runtime_get_window_mgr());
	if (dialog && MODEST_IS_ACCOUNT_ASSISTANT (dialog)) {
		/* old wizard is active already; 
		 */
		gtk_window_present (dialog);
		return;
	}
	
	/* there is no such wizard yet */
	wizard = GTK_DIALOG (modest_account_assistant_new (modest_runtime_get_account_mgr ()));
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr(), 
				     GTK_WINDOW (wizard), self);

	/* if there is already another modal dialog, make it non-modal */
	if (dialog)
		gtk_window_set_modal (GTK_WINDOW(dialog), FALSE);
	
	gtk_window_set_modal (GTK_WINDOW (wizard), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (wizard), GTK_WINDOW (self));
	/* Destroy the dialog when it is closed: */
	g_signal_connect (G_OBJECT (wizard), "response", G_CALLBACK
			  (on_wizard_response), self);
	gtk_widget_show (GTK_WIDGET (wizard));
}


static void
on_default_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	ModestAccountMgr *account_mgr;
	gchar *account_name;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	account_mgr = modest_runtime_get_account_mgr();	
	account_name = modest_account_view_get_selected_account (priv->account_view);

	modest_account_mgr_set_default_account (account_mgr, account_name);

	g_free (account_name);
}


static GtkWidget*
button_box_new (ModestAccountViewWindow *self)
{

	GtkWidget *button_box;
	ModestAccountViewWindowPrivate *priv;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	
	button_box   = gtk_vbutton_box_new ();
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (button_box), 6);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), 
				   GTK_BUTTONBOX_START);
	
	priv->add_button     = gtk_button_new_from_stock(GTK_STOCK_ADD);
	priv->default_button = gtk_button_new_with_label(_("Make default"));
	priv->remove_button  = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	priv->edit_button    = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	
	g_signal_connect (G_OBJECT(priv->add_button), "clicked",
			  G_CALLBACK(on_add_button_clicked),
			  self);
	g_signal_connect (G_OBJECT(priv->remove_button), "clicked",
			  G_CALLBACK(on_remove_button_clicked),
			  self);
	g_signal_connect (G_OBJECT(priv->edit_button), "clicked",
			  G_CALLBACK(on_edit_button_clicked),
			  self);
	g_signal_connect (G_OBJECT(priv->default_button), "clicked",
			  G_CALLBACK(on_default_button_clicked),
			  self);
	
	gtk_box_pack_start (GTK_BOX(button_box), priv->add_button, FALSE, FALSE,2);
	gtk_box_pack_start (GTK_BOX(button_box), priv->default_button, FALSE, FALSE,2);
	gtk_box_pack_start (GTK_BOX(button_box), priv->remove_button, FALSE, FALSE,2);
	gtk_box_pack_start (GTK_BOX(button_box), priv->edit_button, FALSE, FALSE,2);

	gtk_widget_set_sensitive (priv->edit_button, FALSE);
	gtk_widget_set_sensitive (priv->remove_button, FALSE);	
	gtk_widget_set_sensitive (priv->default_button, FALSE);
	
	return button_box;
}

static GtkWidget*
window_vbox_new (ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	GtkTreeSelection *sel;
	GtkWidget *main_hbox, *main_vbox, *button_box;
	GtkWidget *scrolled_window;

	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	main_vbox     = gtk_vbox_new (FALSE, 0);
	main_hbox     = gtk_hbox_new (FALSE, 12);
	
	button_box = button_box_new (self);
	
	priv->account_view = modest_account_view_new (modest_runtime_get_account_mgr());
	gtk_widget_set_size_request (GTK_WIDGET(priv->account_view), 300, 400);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->account_view));
	on_selection_changed (sel, self);
	g_signal_connect (G_OBJECT(sel), "changed",  G_CALLBACK(on_selection_changed),
			  self);
	
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (priv->account_view));
	gtk_box_pack_start (GTK_BOX(main_hbox), scrolled_window, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(main_hbox), button_box, FALSE, FALSE,0);

	gtk_box_pack_start (GTK_BOX(main_vbox), main_hbox, TRUE, TRUE, 0);

	gtk_widget_show_all (main_vbox);
	return main_vbox;
}


GtkWidget*
modest_account_view_window_new (void)
{
	GObject *obj;
	ModestAccountViewWindowPrivate *priv;
	
	obj  = g_object_new(MODEST_TYPE_ACCOUNT_VIEW_WINDOW, NULL);
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(obj);

	gtk_window_set_resizable (GTK_WINDOW(obj), TRUE);
	gtk_window_set_title (GTK_WINDOW(obj), _("mcen_ti_emailsetup_accounts"));
	gtk_window_set_type_hint (GTK_WINDOW(obj), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size (GTK_WINDOW (obj), 640, 480);
	
	gtk_window_set_modal (GTK_WINDOW(obj), TRUE);
				     
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(obj)->vbox),
			    window_vbox_new (MODEST_ACCOUNT_VIEW_WINDOW(obj)),
			    TRUE, TRUE, 12);

	gtk_dialog_add_button (GTK_DIALOG (obj), GTK_STOCK_CLOSE, GTK_RESPONSE_OK);
		
	return GTK_WIDGET(obj);
}

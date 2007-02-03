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
#include <modest-runtime.h>
#include <modest-account-mgr-helpers.h>
#include <string.h>
#include "modest-account-view-window.h"
#include "modest-account-assistant.h"
#include "modest-tny-platform-factory.h"

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
	ModestWidgetFactory *widget_factory;
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
static GtkWindowClass *parent_class = NULL;

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
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
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
modest_account_view_window_init (ModestAccountViewWindow *obj)
{
	ModestAccountViewWindowPrivate *priv;
		
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(obj);

	priv->widget_factory = NULL;
}

static void
modest_account_view_window_finalize (GObject *obj)
{
	ModestAccountViewWindowPrivate *priv;
		
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(obj);

	if (priv->widget_factory) {
		g_object_unref (G_OBJECT(priv->widget_factory));
		priv->widget_factory = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void
on_selection_changed (GtkTreeSelection *sel, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	GtkTreeModel                   *model;
	GtkTreeIter                    iter;
	gboolean                       has_selection;
	const gchar                   *account_name;
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
	g_free (default_account_name);
}

static void
on_remove_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	ModestAccountMgr *account_mgr;
	const gchar *account_name;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	account_mgr = modest_runtime_get_account_mgr();	
	account_name = modest_account_view_get_selected_account (priv->account_view);

	if (account_name) {
		gboolean removed;
		GError *err = NULL;
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
		txt = g_strdup_printf (_("Do you really want to delete the account %s?"), account_name);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
				    gtk_label_new (txt), FALSE, FALSE, 0);
		gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
		g_free (txt);

		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
			/* Remove account. If succeeded it removes also 
			   the account from the ModestAccountView */
			removed = modest_account_mgr_remove_account (account_mgr,
								     account_name,
								     FALSE,
								     &err);
			if (removed) {
				/* Show confirmation dialog ??? */
			} else {
				/* Show error dialog ??? */
				if (err)
					g_error_free (err);
			}
		}
		gtk_widget_destroy (dialog);
	}
}

static void
on_edit_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	g_message (__FUNCTION__);
}

static void
on_add_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	GtkWidget *assistant;
	ModestAccountViewWindowPrivate *priv;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	assistant = modest_account_assistant_new (modest_runtime_get_account_mgr(),
						  priv->widget_factory);
	gtk_window_set_transient_for (GTK_WINDOW(assistant),
				      GTK_WINDOW(self));

	gtk_widget_show (GTK_WIDGET(assistant));
}


static void
on_default_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	ModestAccountMgr *account_mgr;
	const gchar *account_name;
	
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	account_mgr = modest_runtime_get_account_mgr();	
	account_name = modest_account_view_get_selected_account (priv->account_view);

	modest_account_mgr_set_default_account (account_mgr, account_name);
}



static void
on_close_button_clicked (GtkWidget *button, ModestAccountViewWindow *self)
{
	gtk_widget_destroy (GTK_WIDGET(self));
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
	GtkWidget *close_button;
	GtkWidget *close_hbox;
	ModestAccountView *account_view;

	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);

	main_vbox     = gtk_vbox_new (FALSE, 6);
	main_hbox     = gtk_hbox_new (FALSE, 6);
	
	account_view = modest_widget_factory_get_account_view (priv->widget_factory);
	priv->account_view = account_view;
	gtk_widget_set_size_request (GTK_WIDGET(account_view), 300, 400);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(account_view));
	g_signal_connect (G_OBJECT(sel), "changed",  G_CALLBACK(on_selection_changed),
			  self);
	
	button_box = button_box_new (self);
	
	gtk_box_pack_start (GTK_BOX(main_hbox), GTK_WIDGET(account_view), TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(main_hbox), button_box, FALSE, FALSE,2);

	gtk_box_pack_start (GTK_BOX(main_vbox), main_hbox, TRUE, TRUE, 2);

	close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect (G_OBJECT(close_button), "clicked",
			  G_CALLBACK(on_close_button_clicked),
			  self);
	
	close_hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_end (GTK_BOX(close_hbox),
			  close_button, FALSE, FALSE,2);
	gtk_box_pack_end (GTK_BOX(main_vbox), close_hbox, FALSE, FALSE,2);

	gtk_widget_show_all (main_vbox);
	return main_vbox;
}


GtkWidget*
modest_account_view_window_new (ModestWidgetFactory *factory)
{
	GObject *obj;
	ModestAccountViewWindowPrivate *priv;

	g_return_val_if_fail (factory, NULL);
	
	obj  = g_object_new(MODEST_TYPE_ACCOUNT_VIEW_WINDOW, NULL);
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(obj);

	g_object_ref (G_OBJECT(factory));
	priv->widget_factory = factory;

	gtk_window_set_resizable (GTK_WINDOW(obj), FALSE);

	gtk_window_set_title (GTK_WINDOW(obj), _("Accounts"));
	gtk_window_set_type_hint (GTK_WINDOW(obj), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	gtk_window_set_modal (GTK_WINDOW(obj), TRUE);
		
	gtk_container_add (GTK_CONTAINER(obj),
			   window_vbox_new (MODEST_ACCOUNT_VIEW_WINDOW(obj)));
		
	return GTK_WIDGET(obj);
}

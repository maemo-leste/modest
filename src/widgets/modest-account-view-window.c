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
#include "modest-ui-actions.h"
#include "modest-platform.h"
#include "modest-text-utils.h"
#include "modest-account-protocol.h"
#include <modest-account-mgr-helpers.h>
#include <string.h>
#include "modest-tny-platform-factory.h"
#include "modest-easysetup-wizard-dialog.h"
#include "modest-account-settings-dialog.h"
#include <modest-utils.h>
#include "widgets/modest-ui-constants.h"
#include <modest-scrollable.h>
#include <modest-toolkit-factory.h>

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
on_account_activated (GtkTreeView *account_view,
		      GtkTreePath *path,
		      GtkTreeViewColumn *column,
		      ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE (self);

	gchar* account_name = modest_account_view_get_path_account (priv->account_view, path);
	if (!account_name)
		return;

	/* Check whether any connections are active, and cancel them if
	 * the user wishes.
	 */
	if (modest_ui_actions_check_for_active_account ((ModestWindow *) self, account_name)) {
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

			if (dialog) {
				modest_window_mgr_set_modal (modest_runtime_get_window_mgr (),
							     (GtkWindow *) dialog,
							     (GtkWindow *) self);
				gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), FALSE);
				gtk_widget_show (GTK_WIDGET (dialog));
			}
		}
	}
	g_free (account_name);
}

static void
window_vbox_new (ModestAccountViewWindow *self)
{
}


static void
modest_account_view_window_init (ModestAccountViewWindow *self)
{
	ModestAccountViewWindowPrivate *priv;
	GtkWidget *main_vbox, *scrollable;
	GtkWidget *align;


	/* Specify a default size */
	gtk_window_set_default_size (GTK_WINDOW (self), -1, MODEST_DIALOG_WINDOW_MAX_HEIGHT);

#ifdef MODEST_TOOLKIT_HILDON2
	/* Specify a default size */
	gtk_window_set_default_size (GTK_WINDOW (self), -1, MODEST_DIALOG_WINDOW_MAX_HEIGHT);

	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_widget_hide (GTK_DIALOG (self)->action_area);
#else
	gtk_window_set_default_size (GTK_WINDOW (self), MODEST_DIALOG_WINDOW_MAX_WIDTH, MODEST_DIALOG_WINDOW_MAX_HEIGHT);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
#endif

	
	/* This seems to be necessary to make the window show at the front with decoration.
	 * If we use property type=GTK_WINDOW_TOPLEVEL instead of the default GTK_WINDOW_POPUP+decoration, 
	 * then the window will be below the others. */
	gtk_window_set_type_hint (GTK_WINDOW (self),
			    GDK_WINDOW_TYPE_HINT_DIALOG);

	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	priv->acc_removed_handler = 0;
	priv->account_view = modest_account_view_new (modest_runtime_get_account_mgr());
	modest_account_view_set_picker_mode (MODEST_ACCOUNT_VIEW (priv->account_view), TRUE);

	main_vbox = GTK_DIALOG (self)->vbox;

	scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());
	gtk_widget_show (scrollable);
	gtk_container_add (GTK_CONTAINER (scrollable),
			   GTK_WIDGET (priv->account_view));
	gtk_widget_show (GTK_WIDGET (priv->account_view));

	g_signal_connect (G_OBJECT (priv->account_view), "row-activated",
			  G_CALLBACK (on_account_activated), self);

	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DEFAULT, MODEST_MARGIN_DEFAULT);
	gtk_widget_show (align);

	gtk_container_add (GTK_CONTAINER (align), scrollable);

	gtk_box_pack_start (GTK_BOX(main_vbox), align, TRUE, TRUE, 0);

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
	window_vbox_new (MODEST_ACCOUNT_VIEW_WINDOW (self));
	
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_emailsetup_accounts"));

	/* Connect signals */
	priv = MODEST_ACCOUNT_VIEW_WINDOW_GET_PRIVATE(self);
	priv->acc_removed_handler = g_signal_connect (G_OBJECT(account_mgr), "account_removed",
						      G_CALLBACK (on_account_removed), self);
	
	return GTK_WIDGET (self);
}

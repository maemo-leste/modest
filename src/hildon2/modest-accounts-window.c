/* Copyright (c) 2008, Nokia Corporation
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

#include <modest-accounts-window.h>
#include <modest-osso-state-saving.h>
#include <libosso.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-banner.h>
#include <modest-ui-actions.h>
#include <modest-window-mgr.h>
#include <modest-signal-mgr.h>
#include <modest-runtime.h>
#include <modest-platform.h>
#include <hildon/hildon-program.h>
#include <modest-maemo-utils.h>
#include <modest-icon-names.h>
#include <modest-defs.h>
#include <modest-folder-window.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>


/* 'private'/'protected' functions */
static void modest_accounts_window_class_init  (ModestAccountsWindowClass *klass);
static void modest_accounts_window_instance_init (ModestAccountsWindow *obj);
static void modest_accounts_window_finalize    (GObject *obj);

static void connect_signals (ModestAccountsWindow *self);
static void modest_accounts_window_disconnect_signals (ModestWindow *self);

static void on_account_activated (GtkTreeView *treeview,
				  GtkTreePath *path,
				  GtkTreeViewColumn *column,
				  ModestAccountsWindow *accounts_window);
static void setup_menu (ModestAccountsWindow *self);


typedef struct _ModestAccountsWindowPrivate ModestAccountsWindowPrivate;
struct _ModestAccountsWindowPrivate {

	GtkWidget *account_view;

	/* signals */
	GSList *sighandlers;

};
#define MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									    MODEST_TYPE_ACCOUNTS_WINDOW, \
									    ModestAccountsWindowPrivate))

/* globals */
static GtkWindowClass *parent_class = NULL;

/************************************************************************/

GType
modest_accounts_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestAccountsWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_accounts_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestAccountsWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_accounts_window_instance_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_HILDON2_WINDOW,
		                                  "ModestAccountsWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_accounts_window_class_init (ModestAccountsWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_accounts_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountsWindowPrivate));
	
	modest_window_class->disconnect_signals_func = modest_accounts_window_disconnect_signals;
}

static void
modest_accounts_window_instance_init (ModestAccountsWindow *obj)
{
	ModestAccountsWindowPrivate *priv;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(obj);

	priv->sighandlers = NULL;
	
	priv->account_view = NULL;
	
	modest_window_mgr_register_help_id (modest_runtime_get_window_mgr(),
					    GTK_WINDOW(obj),
					    "applications_email_accountsview");
}

static void
modest_accounts_window_finalize (GObject *obj)
{
	ModestAccountsWindowPrivate *priv;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(obj);

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_accounts_window_disconnect_signals (MODEST_WINDOW (obj));	

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_accounts_window_disconnect_signals (ModestWindow *self)
{	
	ModestAccountsWindowPrivate *priv;	
	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(self);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;	
}

static void
connect_signals (ModestAccountsWindow *self)
{	
	ModestAccountsWindowPrivate *priv;
	
	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(self);

	/* accounts view */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
						       G_OBJECT (priv->account_view), "row-activated", 
						       G_CALLBACK (on_account_activated), self);

	/* window */

	/* we don't register this in sighandlers, as it should be run after disconnecting all signals,
	 * in destroy stage */

	
}

ModestWindow *
modest_accounts_window_new (void)
{
	ModestAccountsWindow *self = NULL;	
	ModestAccountsWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	GtkWidget *pannable;
	
	self  = MODEST_ACCOUNTS_WINDOW(g_object_new(MODEST_TYPE_ACCOUNTS_WINDOW, NULL));
	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(self);

	pannable = hildon_pannable_area_new ();
	priv->account_view  = GTK_WIDGET (modest_account_view_new (modest_runtime_get_account_mgr ()));

	setup_menu (self);

	gtk_container_add (GTK_CONTAINER (pannable), priv->account_view);
	gtk_container_add (GTK_CONTAINER (self), pannable);

	gtk_widget_show (priv->account_view);
	gtk_widget_show (pannable);

	connect_signals (MODEST_ACCOUNTS_WINDOW (self));

	/* Load previous osso state, for instance if we are being restored from 
	 * hibernation:  */
	modest_osso_load_state ();

	/* Get device name */
	modest_maemo_utils_get_device_name ();

	app = hildon_program_get_instance ();
	hildon_program_add_window (app, HILDON_WINDOW (self));
	
	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_ICON, MODEST_ICON_SIZE_BIG);
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (self), window_icon);
		g_object_unref (window_icon);
	}

	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */

	return MODEST_WINDOW(self);
}

ModestAccountView *
modest_accounts_window_get_account_view (ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_ACCOUNTS_WINDOW(self), FALSE);

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);
	
	return MODEST_ACCOUNT_VIEW (priv->account_view);
}

static void 
setup_menu (ModestAccountsWindow *self)
{
	g_return_if_fail (MODEST_IS_ACCOUNTS_WINDOW(self));

	/* Settings menu buttons */
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_new_account"),
					   APP_MENU_CALLBACK (modest_ui_actions_on_new_account), 
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_edit_accounts"),
					   APP_MENU_CALLBACK (modest_ui_actions_on_accounts), 
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_options"),
					   APP_MENU_CALLBACK (modest_ui_actions_on_settings), 
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_globalsmtpservers"),
					   APP_MENU_CALLBACK (modest_ui_actions_on_smtp_servers),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_tools_smtp_servers));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_viewer_newemail"),
					   APP_MENU_CALLBACK (modest_ui_actions_on_new_msg),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_new_msg));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_sendandreceive"),
					   APP_MENU_CALLBACK (modest_ui_actions_on_send_receive),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_send_receive_all));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_outbox_cancelsend"),
					   APP_MENU_CALLBACK (modest_ui_actions_cancel_send),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_cancel_sending_all));
}


static void
on_account_activated (GtkTreeView *account_view,
		      GtkTreePath *path,
		      GtkTreeViewColumn *column,
		      ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv;
	gchar* account_name; 
	GtkWidget *folder_window;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);
	
	account_name = modest_account_view_get_path_account (MODEST_ACCOUNT_VIEW (priv->account_view), path);
	if (!account_name)
		return;

	folder_window = GTK_WIDGET (modest_folder_window_new (NULL));
	modest_window_mgr_register_window (modest_runtime_get_window_mgr (), 
					   MODEST_WINDOW (folder_window),
					   MODEST_WINDOW (self));
	modest_folder_window_set_account (MODEST_FOLDER_WINDOW (folder_window), account_name);
	gtk_widget_show (folder_window);
	g_free (account_name);

}


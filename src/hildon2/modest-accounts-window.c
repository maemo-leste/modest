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

static gboolean on_zoom_minus_plus_not_implemented (ModestWindow *window);
static void on_account_activated (GtkTreeView *treeview,
				  GtkTreePath *path,
				  GtkTreeViewColumn *column,
				  ModestAccountsWindow *accounts_window);
static void add_to_menu (ModestAccountsWindow *self,
			 HildonAppMenu *menu,
			 gchar *label,
			 GCallback callback,
			 ModestDimmingRulesGroup *group,
			 GCallback dimming_callback);
static void setup_menu (ModestAccountsWindow *self,
			ModestDimmingRulesGroup *group);

static void modest_accounts_window_show_toolbar (ModestWindow *self,
						 gboolean show_toolbar);
static gboolean modest_accounts_window_toggle_menu (HildonWindow *window,
						    guint button,
						    guint32 time);

typedef struct _ModestAccountsWindowPrivate ModestAccountsWindowPrivate;
struct _ModestAccountsWindowPrivate {

	GtkWidget *account_view;
	GtkWidget *app_menu;

	/* signals */
	GSList *sighandlers;

	/* Display state */
	osso_display_state_t display_state;
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
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
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
	HildonWindowClass *hildon_window_class = (HildonWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_accounts_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestAccountsWindowPrivate));
	
	hildon_window_class->toggle_menu = modest_accounts_window_toggle_menu;

	modest_window_class->zoom_minus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->zoom_plus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->show_toolbar_func = modest_accounts_window_show_toolbar;
	modest_window_class->disconnect_signals_func = modest_accounts_window_disconnect_signals;
}

static void
modest_accounts_window_instance_init (ModestAccountsWindow *obj)
{
	ModestAccountsWindowPrivate *priv;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(obj);

	priv->sighandlers = NULL;
	priv->display_state = OSSO_DISPLAY_ON;
	
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

static void 
osso_display_event_cb (osso_display_state_t state, 
		       gpointer data)
{
	ModestAccountsWindowPrivate *priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (data);

	priv->display_state = state;

	/* Stop blinking if the screen becomes on */
	if (priv->display_state == OSSO_DISPLAY_ON)
		modest_platform_remove_new_mail_notifications (TRUE);
}

ModestWindow *
modest_accounts_window_new (void)
{
	ModestAccountsWindow *self = NULL;	
	ModestAccountsWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	GtkWidget *pannable;
	ModestWindowPrivate *parent_priv = NULL;
	ModestDimmingRulesGroup *menu_rules_group = NULL;
	
	self  = MODEST_ACCOUNTS_WINDOW(g_object_new(MODEST_TYPE_ACCOUNTS_WINDOW, NULL));
	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(self);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);

	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();
	menu_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);

	pannable = hildon_pannable_area_new ();
	priv->account_view  = GTK_WIDGET (modest_account_view_new (modest_runtime_get_account_mgr ()));

	setup_menu (self, menu_rules_group);

	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, menu_rules_group);
	g_object_unref (menu_rules_group);

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

	/* Listen for changes in the screen, we don't want to show a
	   led pattern when the display is on for example */
	osso_hw_set_display_event_cb (modest_maemo_utils_get_osso_context (),
				      osso_display_event_cb,
				      self); 

	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */

	return MODEST_WINDOW(self);
}

static gboolean
on_zoom_minus_plus_not_implemented (ModestWindow *window)
{
	g_return_val_if_fail (MODEST_IS_ACCOUNTS_WINDOW (window), FALSE);

	hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_cannot_zoom_here"));
	return FALSE;

}

gboolean
modest_accounts_window_screen_is_on (ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_ACCOUNTS_WINDOW(self), FALSE);

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);
	
	return (priv->display_state == OSSO_DISPLAY_ON) ? TRUE : FALSE;
}

ModestAccountView *
modest_accounts_window_get_account_view (ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_ACCOUNTS_WINDOW(self), FALSE);

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);
	
	return MODEST_ACCOUNT_VIEW (priv->account_view);
}

static void add_to_menu (ModestAccountsWindow *self,
			 HildonAppMenu *menu,
			 gchar *label,
			 GCallback callback,
			 ModestDimmingRulesGroup *group,
			 GCallback dimming_callback)
{
	GtkWidget *button;

	button = gtk_button_new_with_label (label);
	g_signal_connect_after (G_OBJECT (button), "clicked",
				callback, (gpointer) self);
	if (dimming_callback)
		modest_dimming_rules_group_add_widget_rule (group,
							    button,
							    dimming_callback,
							    MODEST_WINDOW (self));
	hildon_app_menu_append (menu, GTK_BUTTON (button));
}

static void
on_new_account (GtkAction *action,
		ModestWindow *window)
{
	modest_ui_actions_run_account_setup_wizard (window);
}

static void setup_menu (ModestAccountsWindow *self, ModestDimmingRulesGroup *group)
{
	ModestAccountsWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_ACCOUNTS_WINDOW(self));

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);

	priv->app_menu = hildon_app_menu_new ();

	/* Settings menu buttons */
	add_to_menu (self, HILDON_APP_MENU (priv->app_menu), _("TODO: new account"),
		     G_CALLBACK (on_new_account),
		     group, NULL);
	add_to_menu (self, HILDON_APP_MENU (priv->app_menu), _("TODO: edit accounts"),
		     G_CALLBACK (modest_ui_actions_on_accounts),
		     group, NULL);
	add_to_menu (self, HILDON_APP_MENU (priv->app_menu), _("mcen_me_inbox_options"),
		     G_CALLBACK (modest_ui_actions_on_settings),
		     group, NULL);
	add_to_menu (self, HILDON_APP_MENU (priv->app_menu), _("mcen_me_inbox_globalsmtpservers"),
		     G_CALLBACK (modest_ui_actions_on_smtp_servers),
		     group, G_CALLBACK (modest_ui_dimming_rules_on_tools_smtp_servers));
	add_to_menu (self, HILDON_APP_MENU (priv->app_menu), _("mcen_me_viewer_newemail"),
		     G_CALLBACK (modest_ui_actions_on_new_msg),
		     group, G_CALLBACK (modest_ui_dimming_rules_on_new_msg));
	add_to_menu (self, HILDON_APP_MENU (priv->app_menu), _("mcen_me_inbox_sendandreceive"),
		     G_CALLBACK (modest_ui_actions_on_send_receive),
		     group, G_CALLBACK (modest_ui_dimming_rules_on_send_receive_all));
	add_to_menu (self, HILDON_APP_MENU (priv->app_menu), _("mcen_me_outbox_cancelsend"),
		     G_CALLBACK (modest_ui_actions_cancel_send),
		     group, G_CALLBACK (modest_ui_dimming_rules_on_cancel_sending_all));
	
	hildon_stackable_window_set_main_menu (HILDON_STACKABLE_WINDOW (self), 
					       HILDON_APP_MENU (priv->app_menu));
}

static gboolean 
modest_accounts_window_toggle_menu (HildonWindow *window,
				    guint button,
				    guint32 time)
{
	ModestAccountsWindowPrivate *priv = NULL;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (window);

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (window));

	gtk_widget_queue_resize (priv->app_menu);

	return HILDON_WINDOW_CLASS (parent_class)->toggle_menu (window, button, time);
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

static void
modest_accounts_window_show_toolbar (ModestWindow *self,
				     gboolean show_toolbar)
{
	/* Empty implementation, this view does not show any
	   toolbar */
}

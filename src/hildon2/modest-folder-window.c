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

#include <modest-folder-window.h>
#include <modest-osso-state-saving.h>
#include <libosso.h>
#include <hildon/hildon-pannable-area.h>
#include <modest-window-mgr.h>
#include <modest-signal-mgr.h>
#include <modest-runtime.h>
#include <modest-platform.h>
#include <modest-maemo-utils.h>
#include <modest-icon-names.h>
#include <modest-ui-constants.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-defs.h>
#include <modest-ui-actions.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-banner.h>
#include <tny-account-store-view.h>
#include <modest-header-window.h>

/* 'private'/'protected' functions */
static void modest_folder_window_class_init  (ModestFolderWindowClass *klass);
static void modest_folder_window_init        (ModestFolderWindow *obj);
static void modest_folder_window_finalize    (GObject *obj);

static void connect_signals (ModestFolderWindow *self);
static void modest_folder_window_disconnect_signals (ModestWindow *self);

static gboolean on_zoom_minus_plus_not_implemented (ModestWindow *window);
static void on_folder_activated (ModestFolderView *folder_view,
				 TnyFolder *folder,
				 gpointer userdata);
static void add_to_menu (ModestFolderWindow *self,
			 HildonAppMenu *menu,
			 gchar *label,
			 GCallback callback);
static void setup_menu (ModestFolderWindow *self);

typedef struct _ModestFolderWindowPrivate ModestFolderWindowPrivate;
struct _ModestFolderWindowPrivate {

	GtkWidget *folder_view;

	/* signals */
	GSList *sighandlers;

	/* Display state */
	osso_display_state_t display_state;
};
#define MODEST_FOLDER_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									  MODEST_TYPE_FOLDER_WINDOW, \
									  ModestFolderWindowPrivate))

/* globals */
static GtkWindowClass *parent_class = NULL;

/************************************************************************/

GType
modest_folder_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestFolderWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_folder_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestFolderWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_folder_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
		                                  "ModestFolderWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_folder_window_class_init (ModestFolderWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_folder_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestFolderWindowPrivate));
	
	modest_window_class->zoom_minus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->zoom_plus_func = on_zoom_minus_plus_not_implemented;
	modest_window_class->disconnect_signals_func = modest_folder_window_disconnect_signals;
}

static void
modest_folder_window_init (ModestFolderWindow *obj)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(obj);

	priv->sighandlers = NULL;
	priv->display_state = OSSO_DISPLAY_ON;
	
	priv->folder_view = NULL;
	
	modest_window_mgr_register_help_id (modest_runtime_get_window_mgr(),
					    GTK_WINDOW(obj),
					    "applications_email_folderview");
}

static void
modest_folder_window_finalize (GObject *obj)
{
	ModestFolderWindowPrivate *priv;

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(obj);

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_folder_window_disconnect_signals (MODEST_WINDOW (obj));	

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_folder_window_disconnect_signals (ModestWindow *self)
{	
	ModestFolderWindowPrivate *priv;	
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;	
}

static void
connect_signals (ModestFolderWindow *self)
{	
	ModestFolderWindowPrivate *priv;
	
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);

	/* folder view */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
						       G_OBJECT (priv->folder_view), "folder-activated", 
						       G_CALLBACK (on_folder_activated), self);

	/* TODO: connect folder view activate */
	
	/* window */

	/* we don't register this in sighandlers, as it should be run after disconnecting all signals,
	 * in destroy stage */

	
}

static void 
osso_display_event_cb (osso_display_state_t state, 
		       gpointer data)
{
	ModestFolderWindowPrivate *priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (data);

	priv->display_state = state;

	/* Stop blinking if the screen becomes on */
	if (priv->display_state == OSSO_DISPLAY_ON)
		modest_platform_remove_new_mail_notifications (TRUE);
}

ModestWindow *
modest_folder_window_new (TnyFolderStoreQuery *query)
{
	ModestFolderWindow *self = NULL;	
	ModestFolderWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	GtkWidget *pannable;
	
	self  = MODEST_FOLDER_WINDOW(g_object_new(MODEST_TYPE_FOLDER_WINDOW, NULL));
	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE(self);
	pannable = hildon_pannable_area_new ();
	priv->folder_view  = modest_platform_create_folder_view (query);
	modest_folder_view_set_cell_style (MODEST_FOLDER_VIEW (priv->folder_view),
					   MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT);

	setup_menu (self);

	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	gtk_container_add (GTK_CONTAINER (pannable), priv->folder_view);
	gtk_container_add (GTK_CONTAINER (self), pannable);

	gtk_widget_show (priv->folder_view);
	gtk_widget_show (pannable);

	connect_signals (MODEST_FOLDER_WINDOW (self));

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
	g_return_val_if_fail (MODEST_IS_FOLDER_WINDOW (window), FALSE);

	hildon_banner_show_information (NULL, NULL, dgettext("hildon-common-strings", "ckct_ib_cannot_zoom_here"));
	return FALSE;

}

gboolean
modest_folder_window_screen_is_on (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_FOLDER_WINDOW(self), FALSE);

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);
	
	return (priv->display_state == OSSO_DISPLAY_ON) ? TRUE : FALSE;
}

ModestFolderView *
modest_folder_window_get_folder_view (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_FOLDER_WINDOW(self), FALSE);

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);
	
	return MODEST_FOLDER_VIEW (priv->folder_view);
}

void
modest_folder_window_set_account (ModestFolderWindow *self,
				  const gchar *account_name)
{
	ModestFolderWindowPrivate *priv = NULL;
	ModestAccountMgr *mgr;
	ModestAccountSettings *settings = NULL;
	ModestServerAccountSettings *store_settings = NULL;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);
	
	/* Get account data */
	mgr = modest_runtime_get_account_mgr ();

	settings = modest_account_mgr_load_account_settings (mgr, account_name);
	if (!settings)
		goto free_refs;

	store_settings = modest_account_settings_get_store_settings (settings);
	if (!store_settings)
		goto free_refs;

	modest_folder_view_set_account_id_of_visible_server_account 
		(MODEST_FOLDER_VIEW (priv->folder_view),
		 modest_server_account_settings_get_account_name (store_settings));

	modest_window_set_active_account (MODEST_WINDOW (self), account_name);
	gtk_window_set_title (GTK_WINDOW (self), 
			      modest_account_settings_get_display_name (settings));

free_refs:
	if (store_settings) 
		g_object_unref (store_settings);
	if (settings)
		g_object_unref (settings);

}

static void add_to_menu (ModestFolderWindow *self,
			 HildonAppMenu *menu,
			 gchar *label,
			 GCallback callback)
{
	GtkWidget *button;

	button = gtk_button_new_with_label (label);
	g_signal_connect_after (G_OBJECT (button), "clicked",
				callback, (gpointer) self);
	hildon_app_menu_append (menu, GTK_BUTTON (button));
}

static void setup_menu (ModestFolderWindow *self)
{
	ModestFolderWindowPrivate *priv = NULL;
	GtkWidget *app_menu;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	app_menu = hildon_app_menu_new ();

	add_to_menu (self, HILDON_APP_MENU (app_menu), _("mcen_me_viewer_newemail"),
		     G_CALLBACK (modest_ui_actions_on_new_msg));
	add_to_menu (self, HILDON_APP_MENU (app_menu), _("mcen_me_inbox_sendandreceive"),
		     G_CALLBACK (modest_ui_actions_on_send_receive));

	/* Settings menu buttons */
	add_to_menu (self, HILDON_APP_MENU (app_menu), _("mcen_me_inbox_options"),
		     G_CALLBACK (modest_ui_actions_on_settings));
	add_to_menu (self, HILDON_APP_MENU (app_menu), _("mcen_me_inbox_accounts"),
		     G_CALLBACK (modest_ui_actions_on_accounts));
	add_to_menu (self, HILDON_APP_MENU (app_menu), _("mcen_me_inbox_globalsmtpservers"),
		     G_CALLBACK (modest_ui_actions_on_smtp_servers));
	
	hildon_stackable_window_set_main_menu (HILDON_STACKABLE_WINDOW (self), 
					       HILDON_APP_MENU (app_menu));
}

static void
on_folder_activated (ModestFolderView *folder_view,
		     TnyFolder *folder,
		     gpointer userdata)
{
	ModestFolderWindowPrivate *priv = NULL;
	ModestWindow *headerwin;
	ModestFolderWindow *self = (ModestFolderWindow *) userdata;

	g_return_if_fail (MODEST_IS_FOLDER_WINDOW(self));

	priv = MODEST_FOLDER_WINDOW_GET_PRIVATE (self);

	if (!folder)
		return;

	headerwin = modest_header_window_new (folder);
	modest_window_mgr_register_window (modest_runtime_get_window_mgr (), 
					   MODEST_WINDOW (headerwin),
					   MODEST_WINDOW (self));

	modest_window_set_active_account (MODEST_WINDOW (headerwin), 
					  modest_window_get_active_account (MODEST_WINDOW (self)));
	gtk_widget_show (GTK_WIDGET (headerwin));
}

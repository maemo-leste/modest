/* Copyright (c) 2009, Nokia Corporation
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

#include <modest-mailboxes-window.h>
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
#include <modest-window.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-button.h>
#include <tny-account-store-view.h>
#include <modest-header-window.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>
#include "modest-text-utils.h"
#include "modest-tny-account.h"

/* 'private'/'protected' functions */
static void modest_mailboxes_window_class_init  (ModestMailboxesWindowClass *klass);
static void modest_mailboxes_window_init        (ModestMailboxesWindow *obj);
static void modest_mailboxes_window_finalize    (GObject *obj);

static void connect_signals (ModestMailboxesWindow *self);
static void modest_mailboxes_window_disconnect_signals (ModestWindow *self);

static void on_mailbox_activated (ModestFolderView *mailboxes_view,
				  TnyFolder *folder,
				  gpointer userdata);
typedef struct _ModestMailboxesWindowPrivate ModestMailboxesWindowPrivate;
struct _ModestMailboxesWindowPrivate {

	GtkWidget *folder_view;
	GtkWidget *top_vbox;
	GtkWidget *new_message_button;

	/* signals */
	GSList *sighandlers;

	gchar *current_store_account;
};
#define MODEST_MAILBOXES_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									  MODEST_TYPE_MAILBOXES_WINDOW, \
									  ModestMailboxesWindowPrivate))

/* globals */
static GtkWindowClass *parent_class = NULL;

/************************************************************************/

GType
modest_mailboxes_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMailboxesWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_mailboxes_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMailboxesWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_mailboxes_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_HILDON2_WINDOW,
		                                  "ModestMailboxesWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_mailboxes_window_class_init (ModestMailboxesWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_mailboxes_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMailboxesWindowPrivate));
	
	modest_window_class->disconnect_signals_func = modest_mailboxes_window_disconnect_signals;
}

static void
modest_mailboxes_window_init (ModestMailboxesWindow *obj)
{
	ModestMailboxesWindowPrivate *priv;

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE(obj);

	priv->sighandlers = NULL;
	
	priv->folder_view = NULL;

	priv->top_vbox = NULL;

	priv->current_store_account = NULL;
}

static void
modest_mailboxes_window_finalize (GObject *obj)
{
	ModestMailboxesWindowPrivate *priv;

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE(obj);

	if (priv->current_store_account) {
		g_free (priv->current_store_account);
		priv->current_store_account = NULL;
	}

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_mailboxes_window_disconnect_signals (MODEST_WINDOW (obj));	

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_mailboxes_window_disconnect_signals (ModestWindow *self)
{
	ModestMailboxesWindowPrivate *priv;
	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE(self);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;
}

static void
on_visible_account_changed (ModestFolderView *mailboxes_view,
			    const gchar *account_id,
			    gpointer user_data)
{
	if (account_id) {
		TnyAccount *acc = 
			modest_tny_account_store_get_tny_account_by (modest_runtime_get_account_store(),
								     MODEST_TNY_ACCOUNT_STORE_QUERY_ID,
								     account_id);
		if (acc) {
			const gchar *name;
			gchar *title = NULL;

			name = modest_tny_account_get_parent_modest_account_name_for_server_account (acc);
			title = modest_account_mgr_get_display_name (modest_runtime_get_account_mgr(),
								     name);
			if (title) {
				gtk_window_set_title (GTK_WINDOW (user_data), title);
				g_free (title);
			} else {
				gtk_window_set_title (GTK_WINDOW (user_data), _("mcen_ap_name"));
			}
			g_object_unref (acc);
		}
	} else {
		gtk_window_set_title (GTK_WINDOW (user_data), _("mcen_ap_name"));
	}
}

static void
connect_signals (ModestMailboxesWindow *self)
{
	ModestMailboxesWindowPrivate *priv;

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE(self);

	/* mailboxes view */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers, 
						       G_OBJECT (priv->folder_view), "folder-activated", 
						       G_CALLBACK (on_mailbox_activated), self);

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (priv->folder_view),
						       "visible-account-changed",
						       G_CALLBACK (on_visible_account_changed), self);

	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->new_message_button),
					   "clicked",
					   G_CALLBACK (modest_ui_actions_on_new_msg), self);

}

ModestWindow *
modest_mailboxes_window_new (const gchar *account)
{
	ModestMailboxesWindow *self = NULL;	
	ModestMailboxesWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	GtkWidget *pannable;
	GtkWidget *action_area_box;
	GdkPixbuf *new_message_pixbuf;
	guint accel_key;
	GdkModifierType accel_mods;
	GtkAccelGroup *accel_group;
	
	self  = MODEST_MAILBOXES_WINDOW(g_object_new(MODEST_TYPE_MAILBOXES_WINDOW, NULL));
	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE(self);

	pannable = hildon_pannable_area_new ();
	priv->folder_view  = modest_platform_create_folder_view (NULL);
	modest_folder_view_set_cell_style (MODEST_FOLDER_VIEW (priv->folder_view),
					   MODEST_FOLDER_VIEW_CELL_STYLE_COMPACT);
	modest_folder_view_set_filter (MODEST_FOLDER_VIEW (priv->folder_view), 
				       MODEST_FOLDER_VIEW_FILTER_HIDE_ACCOUNTS);

	action_area_box = hildon_tree_view_get_action_area_box (GTK_TREE_VIEW (priv->folder_view));
	priv->new_message_button = hildon_button_new (0, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	hildon_button_set_title (HILDON_BUTTON (priv->new_message_button), _("mcen_ti_new_message"));
	new_message_pixbuf = modest_platform_get_icon ("general_add", MODEST_ICON_SIZE_BIG);
	hildon_button_set_image (HILDON_BUTTON (priv->new_message_button), gtk_image_new_from_pixbuf (new_message_pixbuf));
	g_object_unref (new_message_pixbuf);

	gtk_box_pack_start (GTK_BOX (action_area_box), priv->new_message_button, TRUE, TRUE, 0);
	gtk_widget_show_all (priv->new_message_button);
	hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (priv->folder_view), TRUE);
	
	/* Set account store */
	tny_account_store_view_set_account_store (TNY_ACCOUNT_STORE_VIEW (priv->folder_view),
						  TNY_ACCOUNT_STORE (modest_runtime_get_account_store ()));

	priv->top_vbox = gtk_vbox_new (0, FALSE);

	gtk_container_add (GTK_CONTAINER (pannable), priv->folder_view);
	gtk_box_pack_end (GTK_BOX (priv->top_vbox), pannable, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (self), priv->top_vbox);

	gtk_widget_show (priv->folder_view);
	gtk_widget_show (pannable);
	gtk_widget_show (priv->top_vbox);

	connect_signals (MODEST_MAILBOXES_WINDOW (self));

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

	/* Register edit modes */
	accel_group = gtk_accel_group_new ();
	gtk_accelerator_parse ("<Control>n", &accel_key, &accel_mods);
	gtk_widget_add_accelerator (priv->new_message_button, "clicked", accel_group,
				    accel_key, accel_mods, 0);
	gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);

	modest_folder_view_set_filter (MODEST_FOLDER_VIEW (priv->folder_view),
				       MODEST_FOLDER_VIEW_FILTER_SHOW_ONLY_MAILBOXES);

	modest_mailboxes_window_set_account (MODEST_MAILBOXES_WINDOW (self), account);

	return MODEST_WINDOW(self);
}

ModestFolderView *
modest_mailboxes_window_get_folder_view (ModestMailboxesWindow *self)
{
	ModestMailboxesWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_MAILBOXES_WINDOW(self), FALSE);

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE (self);
	
	return MODEST_FOLDER_VIEW (priv->folder_view);
}

void
modest_mailboxes_window_set_account (ModestMailboxesWindow *self,
				     const gchar *account_name)
{
	ModestMailboxesWindowPrivate *priv = NULL;
	ModestAccountMgr *mgr;
	ModestAccountSettings *settings = NULL;
	ModestServerAccountSettings *store_settings = NULL;

	g_return_if_fail (MODEST_IS_MAILBOXES_WINDOW(self));

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE (self);

	/* Get account data */
	mgr = modest_runtime_get_account_mgr ();

	settings = modest_account_mgr_load_account_settings (mgr, account_name);
	if (!settings)
		goto free_refs;

	store_settings = modest_account_settings_get_store_settings (settings);
	if (!store_settings)
		goto free_refs;

	if (priv->current_store_account != NULL)
		g_free (priv->current_store_account);
	priv->current_store_account = g_strdup (modest_server_account_settings_get_account_name (store_settings));

	modest_folder_view_set_account_id_of_visible_server_account
		(MODEST_FOLDER_VIEW (priv->folder_view),
		 priv->current_store_account);

	modest_window_set_active_account (MODEST_WINDOW (self), account_name);

free_refs:
	if (store_settings)
		g_object_unref (store_settings);
	if (settings)
		g_object_unref (settings);

}

static void
on_mailbox_activated (ModestFolderView *mailboxes_view,
		      TnyFolder *folder,
		      gpointer userdata)
{
	ModestMailboxesWindowPrivate *priv = NULL;
/* 	ModestWindow *headerwin; */
	ModestMailboxesWindow *self = (ModestMailboxesWindow *) userdata;

	g_return_if_fail (MODEST_IS_MAILBOXES_WINDOW(self));

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE (self);

	if (!folder)
		return;

	if (!TNY_IS_FOLDER (folder))
		return;

	g_message ("MAILBOX SELECTED");

/* 	headerwin = modest_header_window_new (mailboxes, modest_window_get_active_account (MODEST_WINDOW (self))); */

/* 	if (modest_window_mgr_register_window (modest_runtime_get_window_mgr (), */
/* 					       MODEST_WINDOW (headerwin), */
/* 					       MODEST_WINDOW (self))) { */
/* 		gtk_widget_show (GTK_WIDGET (headerwin)); */
/* 	} else { */
/* 		gtk_widget_destroy (GTK_WIDGET (headerwin)); */
/* 		headerwin = NULL; */
/* 	} */
}


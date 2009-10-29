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
#include <modest-scrollable.h>
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
#include <modest-folder-window.h>

/* 'private'/'protected' functions */
static void modest_mailboxes_window_class_init  (ModestMailboxesWindowClass *klass);
static void modest_mailboxes_window_init        (ModestMailboxesWindow *obj);
static void modest_mailboxes_window_finalize    (GObject *obj);

static void connect_signals (ModestMailboxesWindow *self);
static void modest_mailboxes_window_disconnect_signals (ModestWindow *self);

static void on_mailbox_activated (ModestFolderView *mailboxes_view,
				  TnyFolder *folder,
				  gpointer userdata);
static void on_progress_list_changed (ModestWindowMgr *mgr,
				      ModestMailboxesWindow *self);
static gboolean on_map_event (GtkWidget *widget,
			      GdkEvent *event,
			      gpointer userdata);
static void update_progress_hint (ModestMailboxesWindow *self);
static void on_queue_changed    (ModestMailOperationQueue *queue,
				 ModestMailOperation *mail_op,
				 ModestMailOperationQueueNotification type,
				 ModestMailboxesWindow *self);
static void on_activity_changed (ModestFolderView *view,
				 gboolean activity,
				 ModestMailboxesWindow *folder_window);
static gboolean on_key_press(GtkWidget *widget,
				GdkEventKey *event,
				gpointer user_data);

typedef struct _ModestMailboxesWindowPrivate ModestMailboxesWindowPrivate;
struct _ModestMailboxesWindowPrivate {

	GtkWidget *folder_view;
	GtkWidget *top_vbox;
	GtkWidget *new_message_button;

	/* signals */
	GSList *sighandlers;

	gchar *current_store_account;
	gboolean progress_hint;
	guint queue_change_handler;
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
	priv->progress_hint = FALSE;
	priv->queue_change_handler = 0;
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

	if (g_signal_handler_is_connected (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					   priv->queue_change_handler))
		g_signal_handler_disconnect (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					     priv->queue_change_handler);

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
						       G_OBJECT (modest_runtime_get_window_mgr ()),
						       "progress-list-changed",
						       G_CALLBACK (on_progress_list_changed), self);

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (priv->folder_view),
						       "activity-changed",
						       G_CALLBACK (on_activity_changed), self);

	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (priv->folder_view),
						       "visible-account-changed",
						       G_CALLBACK (on_visible_account_changed), self);

	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->new_message_button),
					   "clicked",
					   G_CALLBACK (modest_ui_actions_on_new_msg), self);

	/* connect window keys -> priv->folder_view scroll here? */
	g_signal_connect(G_OBJECT(self), "key-press-event",
			G_CALLBACK(on_key_press), self);
}

ModestWindow *
modest_mailboxes_window_new (const gchar *account)
{
	ModestMailboxesWindow *self = NULL;	
	ModestMailboxesWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	GtkWidget *scrollable;
	GtkWidget *action_area_box;
	GdkPixbuf *new_message_pixbuf;
	guint accel_key;
	GdkModifierType accel_mods;
	GtkAccelGroup *accel_group;
	GtkWidget *top_alignment;
	
	self  = MODEST_MAILBOXES_WINDOW(g_object_new(MODEST_TYPE_MAILBOXES_WINDOW, NULL));
	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE(self);

	scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());
	priv->queue_change_handler =
		g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
				  "queue-changed",
				  G_CALLBACK (on_queue_changed),
				  self);

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
	top_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (top_alignment), 
				   0, 0, 
				   MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE);

	gtk_container_add (GTK_CONTAINER (scrollable), priv->folder_view);
	gtk_box_pack_end (GTK_BOX (priv->top_vbox), scrollable, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (top_alignment), priv->top_vbox);
	gtk_container_add (GTK_CONTAINER (self), top_alignment);

	gtk_widget_show (priv->folder_view);
	gtk_widget_show (scrollable);
	gtk_widget_show (top_alignment);
	gtk_widget_show (priv->top_vbox);

	connect_signals (MODEST_MAILBOXES_WINDOW (self));

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

	g_signal_connect (G_OBJECT (self), "map-event",
			  G_CALLBACK (on_map_event),
			  G_OBJECT (self));
	update_progress_hint (self);

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
	update_progress_hint (self);

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
	ModestMailboxesWindow *self = (ModestMailboxesWindow *) userdata;
	GtkWidget *new_window;
	gboolean registered;
	const gchar *active_account;

	g_return_if_fail (MODEST_IS_MAILBOXES_WINDOW(self));

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE (self);

	if (!folder)
		return;

	if (!TNY_IS_FOLDER (folder))
		return;

	new_window = GTK_WIDGET (modest_folder_window_new (NULL));
	registered = modest_window_mgr_register_window (modest_runtime_get_window_mgr (), 
							MODEST_WINDOW (new_window),
							MODEST_WINDOW (self));

	if (!registered) {
		gtk_widget_destroy (new_window);
		new_window = NULL;
	} else {
		const gchar *name;
		active_account = modest_window_get_active_account (MODEST_WINDOW (self));
		modest_folder_window_set_account (MODEST_FOLDER_WINDOW (new_window), active_account);
		name = tny_folder_get_name (folder);
		if (name) {
			modest_folder_window_set_mailbox (MODEST_FOLDER_WINDOW (new_window), name);
		}
		gtk_widget_show (new_window);
	}
	
}

static gboolean 
on_map_event (GtkWidget *widget,
	      GdkEvent *event,
	      gpointer userdata)
{
	ModestMailboxesWindow *self = (ModestMailboxesWindow *) userdata;
	ModestMailboxesWindowPrivate *priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE (self);

	if (priv->progress_hint) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), TRUE);
	}

	return FALSE;
}

static gboolean
has_active_operations (ModestMailboxesWindow *self)
{
	GSList *operations = NULL, *node;
	ModestMailOperationQueue *queue;
	gboolean has_active = FALSE;

	queue = modest_runtime_get_mail_operation_queue ();
	operations = modest_mail_operation_queue_get_by_source (queue, G_OBJECT (self));

	for (node = operations; node != NULL; node = g_slist_next (node)) {
		if (!modest_mail_operation_is_finished (MODEST_MAIL_OPERATION (node->data))) {
			has_active = TRUE;
			break;
		}
	}

	if (operations) {
		g_slist_foreach (operations, (GFunc) g_object_unref, NULL);
		g_slist_free (operations);
	}

	return has_active;
}

static void
update_progress_hint (ModestMailboxesWindow *self)
{
	ModestMailboxesWindowPrivate *priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE (self);

	if (has_active_operations (self)) {
		priv->progress_hint = TRUE;
	} else {
		priv->progress_hint = FALSE;
	}

	if (!priv->progress_hint && priv->current_store_account) {
		priv->progress_hint = modest_window_mgr_has_progress_operation_on_account (modest_runtime_get_window_mgr (),
											   priv->current_store_account);
	}

	if (!priv->progress_hint) {
		priv->progress_hint = modest_folder_view_get_activity (MODEST_FOLDER_VIEW (priv->folder_view));
	}

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));

	if (GTK_WIDGET_VISIBLE (self)) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), priv->progress_hint ? 1:0);
	}
}

static void
on_progress_list_changed (ModestWindowMgr *mgr,
			  ModestMailboxesWindow *self)
{
	update_progress_hint (self);
}

static void 
on_mail_operation_started (ModestMailOperation *mail_op,
			   gpointer user_data)
{
	ModestMailboxesWindow *self;
	ModestMailOperationTypeOperation op_type;
	GObject *source = NULL;

	self = MODEST_MAILBOXES_WINDOW (user_data);
	op_type = modest_mail_operation_get_type_operation (mail_op);
	source = modest_mail_operation_get_source(mail_op);
	if (G_OBJECT (self) == source) {
		update_progress_hint (self);
	}
	g_object_unref (source);
}

static void 
on_mail_operation_finished (ModestMailOperation *mail_op,
			    gpointer user_data)
{
	ModestMailboxesWindow *self;

	self = MODEST_MAILBOXES_WINDOW (user_data);

	/* Don't disable the progress hint if there are more pending
	   operations from this window */
	update_progress_hint (self);

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestMailboxesWindow *self)
{
	ModestMailboxesWindowPrivate *priv;

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE (self);

	/* If this operations was created by another window, do nothing */
	if (!modest_mail_operation_is_mine (mail_op, G_OBJECT(self))) 
		return;

	if (type == MODEST_MAIL_OPERATION_QUEUE_OPERATION_ADDED) {
		priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
							       G_OBJECT (mail_op),
							       "operation-started",
							       G_CALLBACK (on_mail_operation_started),
							       self);
		priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
							       G_OBJECT (mail_op),
							       "operation-finished",
							       G_CALLBACK (on_mail_operation_finished),
							       self);
	} else if (type == MODEST_MAIL_OPERATION_QUEUE_OPERATION_REMOVED) {
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers,
								  G_OBJECT (mail_op),
								  "operation-started");
		priv->sighandlers = modest_signal_mgr_disconnect (priv->sighandlers,
								  G_OBJECT (mail_op),
								  "operation-finished");
	}
}

static void
on_activity_changed (ModestFolderView *view,
		     gboolean activity,
		     ModestMailboxesWindow *self)
{
	g_return_if_fail (MODEST_IS_MAILBOXES_WINDOW (self));

	update_progress_hint (self);
}


static gboolean
on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	ModestMailboxesWindowPrivate *priv;
	ModestScrollable *scrollable;

	if (event->type == GDK_KEY_RELEASE)
		return FALSE;

	priv = MODEST_MAILBOXES_WINDOW_GET_PRIVATE(user_data);

	scrollable = MODEST_SCROLLABLE (gtk_widget_get_parent (priv->folder_view));

	switch (event->keyval) {

	case GDK_Up:
		modest_scrollable_scroll (scrollable, 0, -1);
		break;

	case GDK_Down:
		modest_scrollable_scroll (scrollable, 0, 1);
		break;
	}

	return FALSE;
}

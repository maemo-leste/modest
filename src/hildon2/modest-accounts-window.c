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
#include <hildon/hildon-helper.h>
#include <modest-ui-actions.h>
#include <modest-window-mgr.h>
#include <modest-signal-mgr.h>
#include <modest-runtime.h>
#include <modest-platform.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-button.h>
#include <modest-maemo-utils.h>
#include <modest-icon-names.h>
#include <modest-defs.h>
#include <modest-folder-window.h>
#include <modest-ui-dimming-rules.h>
#include <modest-ui-dimming-manager.h>
#include <modest-window-priv.h>
#include <modest-ui-constants.h>
#include <modest-account-mgr-helpers.h>
#include <modest-mailboxes-window.h>


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
static void on_progress_list_changed (ModestWindowMgr *mgr,
				      ModestAccountsWindow *self);
static void setup_menu (ModestAccountsWindow *self);
static gboolean _modest_accounts_window_map_event (GtkWidget *widget,
						   GdkEvent *event,
						   gpointer userdata);
static void update_progress_hint (ModestAccountsWindow *self);
static void on_queue_changed    (ModestMailOperationQueue *queue,
				 ModestMailOperation *mail_op,
				 ModestMailOperationQueueNotification type,
				 ModestAccountsWindow *self);
static void on_row_inserted (GtkTreeModel *tree_model,
			     GtkTreePath  *path,
			     GtkTreeIter  *iter,
			     gpointer      user_data);
static void on_row_deleted (GtkTreeModel *tree_model,
			    GtkTreePath  *path,
			    gpointer      user_data);
static void row_count_changed (ModestAccountsWindow *self);

typedef struct _ModestAccountsWindowPrivate ModestAccountsWindowPrivate;
struct _ModestAccountsWindowPrivate {

	GtkWidget *box;
	GtkWidget *pannable;
	GtkWidget *account_view;
	GtkWidget *no_accounts_container;
	GtkWidget *new_message_button;

	/* signals */
	GSList *sighandlers;

	gboolean progress_hint;
	guint queue_change_handler;
};
#define MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									    MODEST_TYPE_ACCOUNTS_WINDOW, \
									    ModestAccountsWindowPrivate))

/* globals */
static GtkWindowClass *parent_class = NULL;
static GtkWidget *pre_created_accounts_window = NULL;

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
	priv->progress_hint = FALSE;
	
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

	if (g_signal_handler_is_connected (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					   priv->queue_change_handler))
		g_signal_handler_disconnect (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					     priv->queue_change_handler);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;	
}

static void
connect_signals (ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv;
	GtkTreeModel *model;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(self);

	/* accounts view */
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->account_view), "row-activated", 
					   G_CALLBACK (on_account_activated), self);

	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (modest_runtime_get_window_mgr ()),
					   "progress-list-changed",
					   G_CALLBACK (on_progress_list_changed), self);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->account_view));

	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (model),
					   "row-inserted",
					   G_CALLBACK (on_row_inserted), self);

	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (model),
					   "row-deleted",
					   G_CALLBACK (on_row_deleted), self);

	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->new_message_button),
					   "clicked",
					   G_CALLBACK (modest_ui_actions_on_new_msg), self);

	/* we don't register this in sighandlers, as it should be run
	 * after disconnecting all signals, in destroy stage */
}

static ModestWindow *
modest_accounts_window_new_real (void)
{
	ModestAccountsWindow *self = NULL;
	ModestAccountsWindowPrivate *priv = NULL;
	GdkPixbuf *window_icon;
	GdkPixbuf *new_message_pixbuf;
	guint accel_key;
	GdkModifierType accel_mods;
	GtkAccelGroup *accel_group;
	GtkWidget *no_accounts_label;
	GtkWidget *empty_view_new_message_button;
	GtkWidget *box_alignment;

	self  = MODEST_ACCOUNTS_WINDOW(g_object_new(MODEST_TYPE_ACCOUNTS_WINDOW, NULL));
	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(self);

	new_message_pixbuf = modest_platform_get_icon ("general_add", MODEST_ICON_SIZE_BIG);

	box_alignment = gtk_alignment_new (0, 0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (box_alignment), 
				   0, 0,
				   HILDON_MARGIN_DOUBLE, HILDON_MARGIN_DOUBLE);
	priv->box = gtk_vbox_new (FALSE, 0);

	no_accounts_label = gtk_label_new (_("mcen_ia_noaccounts"));
	
	gtk_misc_set_alignment (GTK_MISC (no_accounts_label), 0.5, 0.5);
	hildon_helper_set_logical_font (no_accounts_label, "LargeSystemFont");

	empty_view_new_message_button = hildon_button_new (MODEST_EDITABLE_SIZE, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);
	hildon_button_set_title (HILDON_BUTTON (empty_view_new_message_button), _("mcen_ti_new_message"));
	hildon_button_set_image (HILDON_BUTTON (empty_view_new_message_button), gtk_image_new_from_pixbuf (new_message_pixbuf));


	priv->no_accounts_container = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->no_accounts_container), empty_view_new_message_button, FALSE, FALSE, 0);
	gtk_widget_show_all (empty_view_new_message_button);
	gtk_box_pack_end (GTK_BOX (priv->no_accounts_container), no_accounts_label, TRUE, TRUE, 0);
	gtk_widget_show (no_accounts_label);
	gtk_box_pack_start (GTK_BOX (priv->box), priv->no_accounts_container, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (empty_view_new_message_button),
			  "clicked",
			  G_CALLBACK (modest_ui_actions_on_new_msg), self);
	
	priv->pannable = hildon_pannable_area_new ();

	priv->queue_change_handler =
		g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
				  "queue-changed",
				  G_CALLBACK (on_queue_changed),
				  self);

	priv->new_message_button = hildon_button_new (0, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	hildon_button_set_title (HILDON_BUTTON (priv->new_message_button), _("mcen_ti_new_message"));
	hildon_button_set_image (HILDON_BUTTON (priv->new_message_button), gtk_image_new_from_pixbuf (new_message_pixbuf));

	gtk_widget_show_all (priv->new_message_button);

	g_object_unref (new_message_pixbuf);
	setup_menu (self);

	gtk_box_pack_start (GTK_BOX (priv->box), priv->pannable, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (box_alignment), priv->box);
	gtk_container_add (GTK_CONTAINER (self), box_alignment);

	gtk_widget_show (priv->pannable);
	gtk_widget_show (priv->box);
	gtk_widget_show (box_alignment);

	/* Get device name */
	modest_maemo_utils_get_device_name ();

	/* Set window icon */
	window_icon = modest_platform_get_icon (MODEST_APP_ICON, MODEST_ICON_SIZE_BIG);
	if (window_icon) {
		gtk_window_set_icon (GTK_WINDOW (self), window_icon);
		g_object_unref (window_icon);
	}

	accel_group = gtk_accel_group_new ();
	gtk_accelerator_parse ("<Control>n", &accel_key, &accel_mods);
	gtk_widget_add_accelerator (priv->new_message_button, "clicked", accel_group,
				    accel_key, accel_mods, 0);
	gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);

	return MODEST_WINDOW(self);
}

ModestWindow *
modest_accounts_window_new (void)
{
	GtkWidget *action_area_box;
	ModestWindow *self;
	ModestAccountsWindowPrivate *priv = NULL;
	HildonProgram *app;

	if (pre_created_accounts_window) {
		self = MODEST_WINDOW (pre_created_accounts_window);
		pre_created_accounts_window = NULL;
	} else {
		self = modest_accounts_window_new_real ();
	}
	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE(self);
	priv->account_view  = GTK_WIDGET (modest_account_view_new (modest_runtime_get_account_mgr ()));

	action_area_box = hildon_tree_view_get_action_area_box (GTK_TREE_VIEW (priv->account_view));
	gtk_box_pack_start (GTK_BOX (action_area_box), priv->new_message_button, TRUE, TRUE, 0);
	hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (priv->account_view), TRUE);
	gtk_container_add (GTK_CONTAINER (priv->pannable), priv->account_view);

	connect_signals (MODEST_ACCOUNTS_WINDOW (self));

	/* Load previous osso state, for instance if we are being restored from 
	 * hibernation:  */
	modest_osso_load_state ();

	app = hildon_program_get_instance ();
	hildon_program_add_window (app, HILDON_WINDOW (self));
	
	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */

	g_signal_connect (G_OBJECT (self), "map-event",
			  G_CALLBACK (_modest_accounts_window_map_event),
			  G_OBJECT (self));
	update_progress_hint (MODEST_ACCOUNTS_WINDOW (self));

	row_count_changed (MODEST_ACCOUNTS_WINDOW (self));

	return self;
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
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_new_account"), NULL, 
					   APP_MENU_CALLBACK (modest_ui_actions_on_new_account), 
					   NULL);
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_edit_accounts"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_accounts), 
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_edit_accounts));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_globalsmtpservers"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_smtp_servers),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_tools_smtp_servers));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_sendandreceive"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_send_receive),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_send_receive_all));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_outbox_cancelsend"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_cancel_send),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_cancel_sending_all));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_options"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_settings), 
					   NULL);
}


static void
on_account_activated (GtkTreeView *account_view,
		      GtkTreePath *path,
		      GtkTreeViewColumn *column,
		      ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv;
	gchar* account_name; 
	GtkWidget *new_window;
	gboolean registered;
	ModestProtocolType store_protocol;
	gboolean mailboxes_protocol;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);

	account_name = modest_account_view_get_path_account (MODEST_ACCOUNT_VIEW (priv->account_view), path);
	if (!account_name)
		return;

	/* If it's a multimailbox container, we have to show the mailboxes window */
	store_protocol = modest_account_mgr_get_store_protocol (modest_runtime_get_account_mgr (), 
								account_name);
	mailboxes_protocol = 
		modest_protocol_registry_protocol_type_has_tag (modest_runtime_get_protocol_registry (),
								store_protocol,
								MODEST_PROTOCOL_REGISTRY_MULTI_MAILBOX_PROVIDER_PROTOCOLS);
	if (mailboxes_protocol) {
		new_window = GTK_WIDGET (modest_mailboxes_window_new (account_name));
	} else {

		new_window = GTK_WIDGET (modest_folder_window_new (NULL));
	}

	registered = modest_window_mgr_register_window (modest_runtime_get_window_mgr (), 
							MODEST_WINDOW (new_window),
							MODEST_WINDOW (self));

	if (!registered) {
		gtk_widget_destroy (new_window);
		new_window = NULL;
	} else {
		if (!mailboxes_protocol) {
			modest_folder_window_set_account (MODEST_FOLDER_WINDOW (new_window), account_name);
		}
		gtk_widget_show (new_window);
	}
	g_free (account_name);
}

static gboolean
_modest_accounts_window_map_event (GtkWidget *widget,
				   GdkEvent *event,
				   gpointer userdata)
{
	ModestAccountsWindow *self = (ModestAccountsWindow *) userdata;
	ModestAccountsWindowPrivate *priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);

	if (priv->progress_hint) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), TRUE);
	}

	return FALSE;
}

static gboolean
has_active_operations (ModestAccountsWindow *self)
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
update_progress_hint (ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);

	if (has_active_operations (self)) {
		priv->progress_hint = TRUE;
	} else {
		priv->progress_hint = FALSE;
	}

	if (!priv->progress_hint) {
		priv->progress_hint = modest_window_mgr_has_progress_operation (modest_runtime_get_window_mgr ());
	}
	
	if (GTK_WIDGET_VISIBLE (self)) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), priv->progress_hint?1:0);
	}
}

static void
on_progress_list_changed (ModestWindowMgr *mgr,
			  ModestAccountsWindow *self)
{
	update_progress_hint (self);
}

static void
on_row_inserted (GtkTreeModel *tree_model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      user_data)
{
	ModestAccountsWindow *self;

	self = (ModestAccountsWindow *) user_data;

	row_count_changed (self);
}

static void
on_row_deleted (GtkTreeModel *tree_model,
		GtkTreePath  *path,
		gpointer      user_data)
{
	ModestAccountsWindow *self;

	self = (ModestAccountsWindow *) user_data;

	row_count_changed (self);
}

static void 
row_count_changed (ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv;
	GtkTreeModel *model;
	gint count;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->account_view));

	count = gtk_tree_model_iter_n_children (model, NULL);

	if (count == 0) {
		gtk_widget_hide (priv->account_view);
		gtk_widget_show (priv->no_accounts_container);
		g_debug ("%s: hiding accounts view", __FUNCTION__);
	} else {
		gtk_widget_hide (priv->no_accounts_container);
		gtk_widget_show (priv->account_view);
		g_debug ("%s: showing accounts view", __FUNCTION__);
	}
	gtk_container_child_set (GTK_CONTAINER(priv->box), priv->pannable, 
				 "expand", count > 0,
				 "fill", count > 0,
				 NULL);
}

static void 
on_mail_operation_started (ModestMailOperation *mail_op,
			   gpointer user_data)
{
	ModestAccountsWindow *self;
	ModestMailOperationTypeOperation op_type;
	GObject *source = NULL;

	self = MODEST_ACCOUNTS_WINDOW (user_data);
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
	ModestAccountsWindow *self;

	self = MODEST_ACCOUNTS_WINDOW (user_data);

	/* Don't disable the progress hint if there are more pending
	   operations from this window */
	update_progress_hint (self);

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestAccountsWindow *self)
{
	ModestAccountsWindowPrivate *priv;

	priv = MODEST_ACCOUNTS_WINDOW_GET_PRIVATE (self);

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

void 
modest_accounts_window_pre_create (void)
{
	static gboolean pre_created = FALSE;
	if (!pre_created) {
		pre_created = TRUE;
		pre_created_accounts_window = GTK_WIDGET (modest_accounts_window_new_real ());
	}
}

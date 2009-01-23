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

#include <modest-header-window.h>
#include <modest-osso-state-saving.h>
#include <libosso.h>
#include <hildon/hildon-pannable-area.h>
#include <modest-window-mgr.h>
#include <modest-window-priv.h>
#include <modest-signal-mgr.h>
#include <modest-runtime.h>
#include <modest-platform.h>
#include <modest-maemo-utils.h>
#include <modest-icon-names.h>
#include <modest-ui-constants.h>
#include <modest-account-mgr.h>
#include <modest-account-mgr-helpers.h>
#include <modest-defs.h>
#include <modest-widget-memory.h>
#include <modest-ui-actions.h>
#include <modest-platform.h>
#include <modest-text-utils.h>
#include <hildon/hildon-button.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-banner.h>
#include <modest-ui-dimming-rules.h>
#include <modest-tny-folder.h>

typedef enum {
	CONTENTS_STATE_NONE = 0,
	CONTENTS_STATE_EMPTY = 1,
	CONTENTS_STATE_HEADERS = 2
} ContentsState;

typedef enum {
	EDIT_MODE_COMMAND_MOVE = 1,
	EDIT_MODE_COMMAND_DELETE = 2,
} EditModeCommand;

typedef struct _ModestHeaderWindowPrivate ModestHeaderWindowPrivate;
struct _ModestHeaderWindowPrivate {

	GtkWidget *header_view;
	GtkWidget *empty_view;
	GtkWidget *contents_view;
	GtkWidget *top_vbox;

	/* state bar */
	ContentsState contents_state;

	TnyFolder *folder;

	/* autoscroll */
	gboolean autoscroll;

	/* banners */
	GtkWidget *updating_banner;
	guint updating_banner_timeout;

	/* signals */
	GSList *sighandlers;
	gulong queue_change_handler;

	/* progress hint */
	gboolean progress_hint;
};
#define MODEST_HEADER_WINDOW_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									  MODEST_TYPE_HEADER_WINDOW, \
									  ModestHeaderWindowPrivate))

/* 'private'/'protected' functions */
static void modest_header_window_class_init  (ModestHeaderWindowClass *klass);
static void modest_header_window_init        (ModestHeaderWindow *obj);
static void modest_header_window_finalize    (GObject *obj);

static void connect_signals (ModestHeaderWindow *self);
static void modest_header_window_disconnect_signals (ModestWindow *self);

static void setup_menu (ModestHeaderWindow *self);
static GtkWidget *create_empty_view (void);
static GtkWidget *create_header_view (ModestWindow *progress_window,
				      TnyFolder *folder);

static void update_view (ModestHeaderWindow *self,
			 TnyFolderChange *change);
static void set_contents_state (ModestHeaderWindow *window, 
				ContentsState state);

static void on_msg_count_changed (ModestHeaderView *header_view,
				  TnyFolder *folder,
				  TnyFolderChange *change,
				  ModestHeaderWindow *header_window);
static void on_header_activated (ModestHeaderView *header_view,
				 TnyHeader *header,
				 GtkTreePath *path,
				 ModestHeaderWindow *header_window);
static void on_updating_msg_list (ModestHeaderView *header_view,
				  gboolean starting,
				  gpointer user_data);
static void set_delete_edit_mode (GtkButton *button,
				  ModestHeaderWindow *self);
static void set_moveto_edit_mode (GtkButton *button,
				  ModestHeaderWindow *self);
static gboolean on_expose_event(GtkTreeView *header_view,
				GdkEventExpose *event,
				gpointer user_data);
static void on_vertical_movement (HildonPannableArea *area,
				  HildonMovementDirection direction,
				  gdouble x, gdouble y, gpointer user_data);
static void set_progress_hint    (ModestHeaderWindow *self, 
				  gboolean enabled);
static void on_queue_changed    (ModestMailOperationQueue *queue,
				 ModestMailOperation *mail_op,
				 ModestMailOperationQueueNotification type,
				 ModestHeaderWindow *self);
static void modest_header_window_show_toolbar   (ModestWindow *window,
						 gboolean show_toolbar);
static void modest_header_window_pack_toolbar (ModestHildon2Window *self,
					       GtkPackType pack_type,
					       GtkWidget *toolbar);
static void edit_mode_changed (ModestHeaderWindow *header_window,
			       gint edit_mode_id,
			       gboolean enabled,
			       ModestHeaderWindow *self);


/* globals */
static GtkWindowClass *parent_class = NULL;

#define EMPTYVIEW_XALIGN 0.5
#define EMPTYVIEW_YALIGN 0.0
#define EMPTYVIEW_XSPACE 1.0
#define EMPTYVIEW_YSPACE 0.0



/************************************************************************/

GType
modest_header_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestHeaderWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_header_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestHeaderWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_header_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_HILDON2_WINDOW,
		                                  "ModestHeaderWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_header_window_class_init (ModestHeaderWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;
	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;
	ModestHildon2WindowClass *modest_hildon2_window_class = (ModestHildon2WindowClass *) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_header_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestHeaderWindowPrivate));
	
	modest_window_class->show_toolbar_func = modest_header_window_show_toolbar;
	modest_window_class->disconnect_signals_func = modest_header_window_disconnect_signals;
	modest_hildon2_window_class->pack_toolbar_func = modest_header_window_pack_toolbar;
}

static void
modest_header_window_init (ModestHeaderWindow *obj)
{
	ModestHeaderWindowPrivate *priv;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(obj);

	priv->sighandlers = NULL;
	
	priv->header_view = NULL;
	priv->empty_view = NULL;
	priv->top_vbox = NULL;
	priv->contents_view = NULL;
	priv->contents_state = CONTENTS_STATE_NONE;
	priv->folder = NULL;
	priv->updating_banner = NULL;
	priv->updating_banner_timeout = 0;
	priv->autoscroll = TRUE;
	priv->progress_hint = FALSE;
	priv->queue_change_handler = 0;
	
	modest_window_mgr_register_help_id (modest_runtime_get_window_mgr(),
					    GTK_WINDOW(obj),
					    "applications_email_headerview");
}

static void
modest_header_window_finalize (GObject *obj)
{
	ModestHeaderWindowPrivate *priv;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(obj);

	g_object_unref (priv->header_view);
	g_object_unref (priv->empty_view);
	g_object_unref (priv->folder);

	/* Sanity check: shouldn't be needed, the window mgr should
	   call this function before */
	modest_header_window_disconnect_signals (MODEST_WINDOW (obj));	

	if (priv->updating_banner_timeout > 0) {
		g_source_remove (priv->updating_banner_timeout);
		priv->updating_banner_timeout = 0;
	}
	if (priv->updating_banner) {
		gtk_widget_destroy (priv->updating_banner);
		priv->updating_banner = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

static void
modest_header_window_disconnect_signals (ModestWindow *self)
{	
	ModestHeaderWindowPrivate *priv;	
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);

	if (g_signal_handler_is_connected (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					   priv->queue_change_handler))
		g_signal_handler_disconnect (G_OBJECT (modest_runtime_get_mail_operation_queue ()), 
					     priv->queue_change_handler);

	modest_signal_mgr_disconnect_all_and_destroy (priv->sighandlers);
	priv->sighandlers = NULL;

}

static void
connect_signals (ModestHeaderWindow *self)
{	
	ModestHeaderWindowPrivate *priv;
	
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);

	/* header view */

	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,G_OBJECT(priv->header_view), 
					   "msg_count_changed",
					   G_CALLBACK(on_msg_count_changed), self);
	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers, G_OBJECT (priv->header_view),
					   "header-activated",
					   G_CALLBACK (on_header_activated), self);
	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->header_view), 
					   "updating-msg-list",
					   G_CALLBACK (on_updating_msg_list), 
					   self);
	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->header_view),
					   "expose-event",
					   G_CALLBACK (on_expose_event),
					   self);

	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->contents_view), 
					   "vertical-movement", 
					   G_CALLBACK (on_vertical_movement), 
					   self);

	/* Mail Operation Queue */
	priv->queue_change_handler =
		g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
				  "queue-changed",
				  G_CALLBACK (on_queue_changed),
				  self);
}

static GtkWidget *
create_header_view (ModestWindow *self, TnyFolder *folder)
{
	GtkWidget *header_view;

	header_view  = modest_header_view_new (NULL, MODEST_HEADER_VIEW_STYLE_TWOLINES);
	modest_header_view_set_folder (MODEST_HEADER_VIEW (header_view), folder, 
				       TRUE, self, NULL, NULL);
	modest_header_view_set_filter (MODEST_HEADER_VIEW (header_view), 
				       MODEST_HEADER_VIEW_FILTER_NONE);
	modest_widget_memory_restore (modest_runtime_get_conf (), G_OBJECT(header_view),
				      MODEST_CONF_HEADER_VIEW_KEY);

	return header_view;
}

static GtkWidget *
create_empty_view (void)
{
	GtkWidget *label = NULL;
	GtkWidget *align = NULL;

	align = gtk_alignment_new(EMPTYVIEW_XALIGN, EMPTYVIEW_YALIGN, EMPTYVIEW_XSPACE, EMPTYVIEW_YSPACE);
	label = gtk_label_new (_("mcen_ia_nomessages"));
	gtk_widget_show (label);
	gtk_widget_show (align);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);	
	gtk_container_add (GTK_CONTAINER (align), label);

	return align;
}

static void
on_vertical_movement (HildonPannableArea *area,
		      HildonMovementDirection direction,
		      gdouble x, gdouble y, gpointer user_data)
{
	ModestHeaderWindow *self = (ModestHeaderWindow *) user_data;
	ModestHeaderWindowPrivate *priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	priv->autoscroll = FALSE;
}


ModestWindow *
modest_header_window_new (TnyFolder *folder)
{
	ModestHeaderWindow *self = NULL;	
	ModestHeaderWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	
	self  = MODEST_HEADER_WINDOW(g_object_new(MODEST_TYPE_HEADER_WINDOW, NULL));
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);

	priv->folder = g_object_ref (folder);

	priv->contents_view = hildon_pannable_area_new ();

	priv->header_view  = create_header_view (MODEST_WINDOW (self), folder);
	priv->empty_view = create_empty_view ();
	g_object_ref (priv->header_view);
	g_object_ref (priv->empty_view);
	g_signal_connect (G_OBJECT (self), "edit-mode-changed",
			  G_CALLBACK (edit_mode_changed), (gpointer) self);
	setup_menu (self);

        priv->top_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (priv->top_vbox), priv->contents_view, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (self), priv->top_vbox);

	gtk_widget_show (priv->contents_view);
	gtk_widget_show (priv->top_vbox);

	connect_signals (MODEST_HEADER_WINDOW (self));

	update_view (self, NULL);

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

	/* Set window title */
	if (TNY_IS_FOLDER (folder)) {
		gchar *folder_name;

		folder_name = modest_tny_folder_get_display_name (folder);
		gtk_window_set_title (GTK_WINDOW (self), folder_name);
		g_free (folder_name);
	}

	/* Dont't restore settings here, 
	 * because it requires a gtk_widget_show(), 
	 * and we don't want to do that until later,
	 * so that the UI is not visible for non-menu D-Bus activation.
	 */

	/* setup edit modes */
	modest_hildon2_window_register_edit_mode (MODEST_HILDON2_WINDOW (self), EDIT_MODE_COMMAND_DELETE,
						  _("mcen_ti_edit_delete"), _HL("wdgt_bd_delete"),
						  GTK_TREE_VIEW (priv->header_view),
						  GTK_SELECTION_MULTIPLE,
						  EDIT_MODE_CALLBACK (modest_ui_actions_on_edit_mode_delete_message));
	modest_hildon2_window_register_edit_mode (MODEST_HILDON2_WINDOW (self), EDIT_MODE_COMMAND_MOVE,
						  _("mcen_ti_edit_move"), _("mcen_me_move"),
						  GTK_TREE_VIEW (priv->header_view),
						  GTK_SELECTION_MULTIPLE,
						  EDIT_MODE_CALLBACK (modest_ui_actions_on_edit_mode_move_to));

	return MODEST_WINDOW(self);
}

ModestHeaderView *
modest_header_window_get_header_view (ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_HEADER_WINDOW(self), FALSE);

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);
	
	return MODEST_HEADER_VIEW (priv->header_view);
}

static void setup_menu (ModestHeaderWindow *self)
{
	g_return_if_fail (MODEST_IS_HEADER_WINDOW(self));

	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_move_messages"), NULL,
					   APP_MENU_CALLBACK (set_moveto_edit_mode),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_delete));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_delete_messages"), NULL,
					   APP_MENU_CALLBACK (set_delete_edit_mode),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_move_to));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_folder_details"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_details),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_details));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_sort"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_sort),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_sort));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_new_message"), "<Control>n",
					   APP_MENU_CALLBACK (modest_ui_actions_on_new_msg),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_new_msg));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_inbox_sendandreceive"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_send_receive),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_send_receive));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_outbox_cancelsend"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_cancel_send),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_cancel_sending_all));
}

static void 
update_view (ModestHeaderWindow *self,
	     TnyFolderChange *change)
{
	ModestHeaderWindowPrivate *priv = NULL;
	gboolean refilter = FALSE;
	gboolean folder_empty = FALSE;
	gboolean all_marked_as_deleted = FALSE;

	g_return_if_fail (MODEST_IS_HEADER_WINDOW(self));
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);
	g_return_if_fail (priv->folder);

	if (change != NULL) {
		TnyFolderChangeChanged changed;

		changed = tny_folder_change_get_changed (change);
		/* If something changes */
		if ((changed) & TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT)
			folder_empty = (((guint) tny_folder_change_get_new_all_count (change)) == 0);
		else
			folder_empty = (((guint) tny_folder_get_all_count (TNY_FOLDER (priv->folder))) == 0);

		if ((changed) & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS)
			refilter = TRUE;
	} else {
		folder_empty = (((guint) tny_folder_get_all_count (TNY_FOLDER (priv->folder))) == 0);
	}

	/* Check if all messages are marked to be deleted */
	all_marked_as_deleted = modest_header_view_is_empty (MODEST_HEADER_VIEW (priv->header_view));
	folder_empty = folder_empty || all_marked_as_deleted;

	/* Set style of headers view */
	set_contents_state (self, folder_empty?CONTENTS_STATE_EMPTY:CONTENTS_STATE_HEADERS);

	if (refilter)
		modest_header_view_refilter (MODEST_HEADER_VIEW (priv->header_view));
}

static void 
set_contents_state (ModestHeaderWindow *self, 
		    ContentsState state)
{
	ModestHeaderWindowPrivate *priv = NULL;

	g_return_if_fail (MODEST_IS_HEADER_WINDOW(self));
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	if (priv->contents_state == state)
		return;

	/* Remove from container the old content */
	switch (priv->contents_state) {
	case CONTENTS_STATE_EMPTY:
		gtk_container_remove (GTK_CONTAINER (priv->contents_view), priv->empty_view);
		break;
	case CONTENTS_STATE_HEADERS:
		gtk_container_remove (GTK_CONTAINER (priv->contents_view), priv->header_view);
		break;
	case CONTENTS_STATE_NONE:
		break;
	}

	/* Add the new content */
	switch (state) {
	case CONTENTS_STATE_EMPTY:
		gtk_container_add (GTK_CONTAINER (priv->contents_view), priv->empty_view);
		gtk_widget_show (priv->empty_view);
		break;
	case CONTENTS_STATE_HEADERS:
		gtk_container_add (GTK_CONTAINER (priv->contents_view), priv->header_view);
		gtk_widget_show (priv->header_view);
		break;
	case CONTENTS_STATE_NONE:
		break;
	}
	priv->contents_state = state;
	
}

static void
on_msg_count_changed (ModestHeaderView *header_view,
		      TnyFolder *folder,
		      TnyFolderChange *change,
		      ModestHeaderWindow *header_window)
{	
	g_return_if_fail (MODEST_IS_HEADER_WINDOW (header_window));
	
	update_view (MODEST_HEADER_WINDOW (header_window), change);
}

static void 
on_header_activated (ModestHeaderView *header_view,
		     TnyHeader *header,
		     GtkTreePath *path,
		     ModestHeaderWindow *header_window)
{
	modest_ui_actions_on_header_activated (header_view, header, path, MODEST_WINDOW (header_window));
}

static void
updating_banner_destroyed (gpointer data,
			   GObject *where_the_object_was)
{
	ModestHeaderWindowPrivate *priv = NULL;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (data);

	priv->updating_banner = NULL;
}

static gboolean
show_updating_banner (gpointer user_data)
{
	ModestHeaderWindowPrivate *priv = NULL;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (user_data);

	if (priv->updating_banner == NULL) {

		/* We're outside the main lock */
		gdk_threads_enter ();
		priv->updating_banner = 
			modest_platform_animation_banner (GTK_WIDGET (user_data), NULL,
							  _CS ("ckdg_pb_updating"));

		/* We need this because banners in Maemo could be
		   destroyed by dialogs so we need to properly update
		   our reference to it */
		g_object_weak_ref (G_OBJECT (priv->updating_banner),
				   updating_banner_destroyed,
				   user_data);
		gdk_threads_leave ();
	}

	/* Remove timeout */
	priv->updating_banner_timeout = 0;
	return FALSE;
}

/**
 * We use this function to show/hide a progress banner showing
 * "Updating" while the header view is being filled. We're not showing
 * it unless the update takes more than 2 seconds
 *
 * If starting = TRUE then the refresh is starting, otherwise it means
 * that is has just finished
 */
static void 
on_updating_msg_list (ModestHeaderView *header_view,
		      gboolean starting,
		      gpointer user_data)
{
	ModestHeaderWindowPrivate *priv = NULL;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (user_data);
	
	/* Remove old timeout */
	if (priv->updating_banner_timeout > 0) {
		g_source_remove (priv->updating_banner_timeout);
		priv->updating_banner_timeout = 0;
	}

	/* Create a new timeout */
	if (starting) {
		priv->updating_banner_timeout = 
			g_timeout_add (2000, show_updating_banner, user_data);
	} else {
		/* Remove the banner if exists */
		if (priv->updating_banner) {
			gtk_widget_destroy (priv->updating_banner);
			priv->updating_banner = NULL;
		}
	}
}

static void
set_delete_edit_mode (GtkButton *button,
		      ModestHeaderWindow *self)
{
	modest_hildon2_window_set_edit_mode (MODEST_HILDON2_WINDOW (self), EDIT_MODE_COMMAND_DELETE);
}

static void
set_moveto_edit_mode (GtkButton *button,
		    ModestHeaderWindow *self)
{
	modest_hildon2_window_set_edit_mode (MODEST_HILDON2_WINDOW (self), EDIT_MODE_COMMAND_MOVE);
}

static gboolean 
on_expose_event(GtkTreeView *header_view,
		GdkEventExpose *event,
		gpointer user_data)
{
	ModestHeaderWindow *self = (ModestHeaderWindow *) user_data;
	ModestHeaderWindowPrivate *priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	g_return_val_if_fail (MODEST_IS_HEADER_WINDOW (self), FALSE);

	if (priv->autoscroll)
		hildon_pannable_area_jump_to (HILDON_PANNABLE_AREA (priv->contents_view), 0.0, 0.0);

	return FALSE;
}

static gboolean
set_toolbar_transfer_mode (ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv = NULL;
	
	g_return_val_if_fail (MODEST_IS_HEADER_WINDOW (self), FALSE);

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);

	set_progress_hint (self, TRUE);
	
	return FALSE;
}

static void 
set_progress_hint (ModestHeaderWindow *self, 
		   gboolean enabled)
{
	ModestWindowPrivate *parent_priv;
	ModestHeaderWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_HEADER_WINDOW (self));

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);
			
	/* Sets current progress hint */
	priv->progress_hint = enabled;

	if (GTK_WIDGET_VISIBLE (self)) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), enabled?1:0);
	}

}

gboolean 
modest_header_window_toolbar_on_transfer_mode     (ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv= NULL; 

	g_return_val_if_fail (MODEST_IS_HEADER_WINDOW (self), FALSE);
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	return priv->progress_hint;
}

static void
modest_header_window_show_toolbar (ModestWindow *self,
				   gboolean show_toolbar)
{
	ModestHeaderWindowPrivate *priv = NULL;
	ModestWindowPrivate *parent_priv;

	parent_priv = MODEST_WINDOW_GET_PRIVATE(self);
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);

	if (modest_header_window_transfer_mode_enabled (MODEST_HEADER_WINDOW (self))) 
		set_progress_hint (MODEST_HEADER_WINDOW (self), TRUE);
	else
		set_progress_hint (MODEST_HEADER_WINDOW (self), FALSE);
}

gboolean 
modest_header_window_transfer_mode_enabled (ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_HEADER_WINDOW (self), FALSE);	
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);

	return priv->progress_hint;
}

static void 
on_mail_operation_started (ModestMailOperation *mail_op,
			   gpointer user_data)
{
	ModestHeaderWindow *self;
	ModestMailOperationTypeOperation op_type;
	GObject *source = NULL;

	self = MODEST_HEADER_WINDOW (user_data);
	op_type = modest_mail_operation_get_type_operation (mail_op);
	source = modest_mail_operation_get_source(mail_op);
	if (G_OBJECT (self) == source) {
		set_toolbar_transfer_mode(self);
	}
	g_object_unref (source);
}

static void 
on_mail_operation_finished (ModestMailOperation *mail_op,
			    gpointer user_data)
{
	ModestHeaderWindow *self;
	ModestMailOperationTypeOperation op_type;
	GSList *operations = NULL;
	ModestMailOperationQueue *queue;

	self = MODEST_HEADER_WINDOW (user_data);
	op_type = modest_mail_operation_get_type_operation (mail_op);

	queue = modest_runtime_get_mail_operation_queue ();
	operations = modest_mail_operation_queue_get_by_source (queue, user_data);

	/* There should be at least the current one */
	if (!operations)
		g_return_if_reached ();

	/* Don't disable the progress hint if there are more pending
	   operations from this window */
	if (g_slist_length (operations) == 1)
		set_progress_hint (self, FALSE);

	g_slist_free (operations);
	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));
}

static void
on_queue_changed (ModestMailOperationQueue *queue,
		  ModestMailOperation *mail_op,
		  ModestMailOperationQueueNotification type,
		  ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

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
modest_header_window_pack_toolbar (ModestHildon2Window *self,
				   GtkPackType pack_type,
				   GtkWidget *toolbar)
{
	ModestHeaderWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_HEADER_WINDOW (self));
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	if (pack_type == GTK_PACK_START) {
		gtk_box_pack_start (GTK_BOX (priv->top_vbox), toolbar, FALSE, FALSE, 0);
	} else {
		gtk_box_pack_end (GTK_BOX (priv->top_vbox), toolbar, FALSE, FALSE, 0);
	}
}

static void 
edit_mode_changed (ModestHeaderWindow *header_window,
		   gint edit_mode_id,
		   gboolean enabled,
		   ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv;
	ModestHeaderViewFilter filter = MODEST_HEADER_VIEW_FILTER_NONE;

	g_return_if_fail (MODEST_IS_HEADER_WINDOW (self));
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	switch (edit_mode_id) {
	case EDIT_MODE_COMMAND_MOVE:
		filter = MODEST_HEADER_VIEW_FILTER_MOVEABLE;
		break;
	case EDIT_MODE_COMMAND_DELETE:
		filter = MODEST_HEADER_VIEW_FILTER_DELETABLE;
		break;
	case MODEST_HILDON2_WINDOW_EDIT_MODE_NONE:
		filter = MODEST_HEADER_VIEW_FILTER_NONE;
		break;
	}

	if (enabled)
		modest_header_view_set_filter (MODEST_HEADER_VIEW (priv->header_view), 
					       filter);
	else
		modest_header_view_unset_filter (MODEST_HEADER_VIEW (priv->header_view), 
						 filter);
}

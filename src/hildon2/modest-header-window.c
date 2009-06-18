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
#include <hildon/hildon-helper.h>
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
#include <tny-simple-list.h>

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
	GtkWidget *new_message_button;

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
	gchar *current_store_account;

	/* sort button */
	GtkWidget *sort_button;

	/* CSM menu */
	GtkWidget *csm_menu;
	gdouble x_coord;
	gdouble y_coord;
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
static GtkWidget *create_empty_view (ModestWindow *self);
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
static gboolean on_map_event (GtkWidget *widget,
			      GdkEvent *event,
			      gpointer userdata);
static void on_vertical_movement (HildonPannableArea *area,
				  HildonMovementDirection direction,
				  gdouble x, gdouble y, gpointer user_data);
static void on_queue_changed    (ModestMailOperationQueue *queue,
				 ModestMailOperation *mail_op,
				 ModestMailOperationQueueNotification type,
				 ModestHeaderWindow *self);
static void modest_header_window_pack_toolbar (ModestHildon2Window *self,
					       GtkPackType pack_type,
					       GtkWidget *toolbar);
static void edit_mode_changed (ModestHeaderWindow *header_window,
			       gint edit_mode_id,
			       gboolean enabled,
			       ModestHeaderWindow *self);
static void on_progress_list_changed (ModestWindowMgr *mgr,
				      ModestHeaderWindow *self);
static void update_progress_hint (ModestHeaderWindow *self);
static void on_sort_column_changed (GtkTreeSortable *treesortable,
				    gpointer         user_data);
static void update_sort_button (ModestHeaderWindow *self);
static void on_horizontal_movement (HildonPannableArea *hildonpannable,
				    gint                direction,
				    gdouble             initial_x,
				    gdouble             initial_y,
				    gpointer            user_data);

/* globals */
static GtkWindowClass *parent_class = NULL;

#define EMPTYVIEW_XALIGN 0.5
#define EMPTYVIEW_YALIGN 0.5
#define EMPTYVIEW_XSPACE 1.0
#define EMPTYVIEW_YSPACE 1.0



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
	priv->current_store_account = NULL;
	priv->sort_button = NULL;
	priv->new_message_button = NULL;
	priv->x_coord = 0;
	priv->y_coord = 0;

	modest_window_mgr_register_help_id (modest_runtime_get_window_mgr(),
					    GTK_WINDOW(obj),
					    "applications_email_headerview");
}

static void
modest_header_window_finalize (GObject *obj)
{
	ModestHeaderWindowPrivate *priv;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(obj);

	tny_folder_sync_async (TNY_FOLDER (priv->folder),
			       FALSE, NULL, NULL, NULL);

	g_object_unref (priv->folder);
	g_object_unref (priv->header_view);
	g_object_unref (priv->empty_view);

	if (priv->current_store_account) {
		g_free (priv->current_store_account);
		priv->current_store_account = NULL;
	}

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
	GtkTreeSortable *sortable;
	
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

	sortable = GTK_TREE_SORTABLE (gtk_tree_view_get_model (GTK_TREE_VIEW (priv->header_view)));

	priv->sighandlers = 
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (sortable),
					   "sort-column-changed",
					   G_CALLBACK (on_sort_column_changed),
					   self);

	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (self),
					   "map-event",
					   G_CALLBACK (on_map_event),
					   self);

	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->contents_view), 
					   "vertical-movement", 
					   G_CALLBACK (on_vertical_movement), 
					   self);

	/* Mail Operation Queue */
	priv->sighandlers = modest_signal_mgr_connect (priv->sighandlers,
						       G_OBJECT (modest_runtime_get_window_mgr ()),
						       "progress-list-changed",
						       G_CALLBACK (on_progress_list_changed), self);
	priv->sighandlers =
		modest_signal_mgr_connect (priv->sighandlers,
					   G_OBJECT (priv->new_message_button),
					   "clicked",
					   G_CALLBACK (modest_ui_actions_on_new_msg), self);

	/* Delete using horizontal gesture */
	/* DISABLED because it's unreliabile */
	if (FALSE) {
		priv->sighandlers =
			modest_signal_mgr_connect (priv->sighandlers,
						   (GObject *) priv->contents_view,
						   "horizontal-movement",
						   G_CALLBACK (on_horizontal_movement),
						   self);
	}
}

static void
folder_refreshed_cb (ModestMailOperation *mail_op,
		     TnyFolder *folder,
		     gpointer user_data)
{
	/* Update the view (folder could be empty) */
	update_view (MODEST_HEADER_WINDOW (user_data), NULL);
}

static gboolean
tap_and_hold_query_cb (GtkWidget *header_view,
		       GdkEvent *event,
		       gpointer user_data)
{
	ModestHeaderWindow *self;
	ModestHeaderWindowPrivate *priv;

	self = (ModestHeaderWindow *) user_data;
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	if (event->type == GDK_BUTTON_PRESS) {
		TnyHeader *header;

		priv->x_coord = ((GdkEventButton*)event)->x;
		priv->y_coord = ((GdkEventButton*)event)->y;

		/* Enable/Disable mark as (un)read */
		header = modest_header_view_get_header_at_pos ((ModestHeaderView *) header_view,
							       priv->x_coord, priv->y_coord);
		if (header) {
			GList *children;
			GtkWidget *mark_read_item, *mark_unread_item;

			/* Show "mark as read" or "mark as unread" */
			children = gtk_container_get_children (GTK_CONTAINER (priv->csm_menu));
			mark_read_item = (GtkWidget *) g_list_nth_data (children, 1);
			mark_unread_item = (GtkWidget *) g_list_nth_data (children, 2);

			if (tny_header_get_flags (header) & TNY_HEADER_FLAG_SEEN) {
				gtk_widget_show (mark_unread_item);
				gtk_widget_hide (mark_read_item);
			} else {
				gtk_widget_show (mark_read_item);
				gtk_widget_hide (mark_unread_item);
			}
			g_object_unref (header);
		}
	}

	return FALSE;
}

static void
delete_header (GtkWindow *parent,
	       TnyHeader *header)
{
	gint response;
	gchar *subject, *msg;

	subject = tny_header_dup_subject (header);
	if (!subject)
		subject = g_strdup (_("mail_va_no_subject"));

	msg = g_strdup_printf (ngettext("emev_nc_delete_message", "emev_nc_delete_messages", 1),
			       subject);
	g_free (subject);

	/* Confirmation dialog */
	response = modest_platform_run_confirmation_dialog (parent, msg);
	g_free (msg);

	if (response == GTK_RESPONSE_OK) {
		ModestMailOperation *mail_op;
		TnyList *header_list;

		header_list = tny_simple_list_new ();
		tny_list_append (header_list, (GObject *) header);
		mail_op = modest_mail_operation_new ((GObject *) parent);
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
						 mail_op);
		modest_mail_operation_remove_msgs (mail_op, header_list, FALSE);
		g_object_unref (mail_op);
		g_object_unref (header_list);
	}
}


static void
on_delete_csm_activated (GtkMenuItem *item,
			 gpointer user_data)
{
	TnyHeader *header;
	ModestHeaderWindow *self;
	ModestHeaderWindowPrivate *priv;

	self = (ModestHeaderWindow *) user_data;
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	header = modest_header_view_get_header_at_pos ((ModestHeaderView *) priv->header_view,
						       priv->x_coord, priv->y_coord);
	if (header) {
		delete_header ((GtkWindow *) self, header);
		g_object_unref (header);
	}
}

static void
on_mark_read_csm_activated (GtkMenuItem *item,
			    gpointer user_data)
{
	TnyHeader *header;
	ModestHeaderWindow *self;
	ModestHeaderWindowPrivate *priv;

	self = (ModestHeaderWindow *) user_data;
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	header = modest_header_view_get_header_at_pos ((ModestHeaderView *) priv->header_view,
						       priv->x_coord, priv->y_coord);

	if (header) {
		tny_header_set_flag (header, TNY_HEADER_FLAG_SEEN);
		g_object_unref (header);
	}
}

static void
on_mark_unread_csm_activated (GtkMenuItem *item,
			      gpointer user_data)
{
	TnyHeader *header;
	ModestHeaderWindow *self;
	ModestHeaderWindowPrivate *priv;

	self = (ModestHeaderWindow *) user_data;
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	header = modest_header_view_get_header_at_pos ((ModestHeaderView *) priv->header_view,
						       priv->x_coord, priv->y_coord);

	if (header) {
		tny_header_unset_flag (header, TNY_HEADER_FLAG_SEEN);
		g_object_unref (header);
	}
}

static GtkWidget *
create_header_view (ModestWindow *self, TnyFolder *folder)
{
	GtkWidget *header_view;
	GtkWidget *delete_item, *mark_read_item, *mark_unread_item;
	ModestHeaderWindowPrivate *priv;

	header_view  = modest_header_view_new (NULL, MODEST_HEADER_VIEW_STYLE_TWOLINES);
	modest_header_view_set_folder (MODEST_HEADER_VIEW (header_view), folder,
				       TRUE, self, folder_refreshed_cb, self);
	modest_header_view_set_filter (MODEST_HEADER_VIEW (header_view),
				       MODEST_HEADER_VIEW_FILTER_NONE);
	modest_widget_memory_restore (modest_runtime_get_conf (), G_OBJECT(header_view),
				      MODEST_CONF_HEADER_VIEW_KEY);

	/* Create CSM menu */
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);
	priv->csm_menu = gtk_menu_new ();
	delete_item = gtk_menu_item_new_with_label (_HL("wdgt_bd_delete"));
	mark_read_item = gtk_menu_item_new_with_label (_("mcen_me_inbox_mark_as_read"));
	mark_unread_item = gtk_menu_item_new_with_label (_("mcen_me_inbox_mark_as_unread"));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->csm_menu), delete_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->csm_menu), mark_read_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->csm_menu), mark_unread_item);
	hildon_gtk_widget_set_theme_size (delete_item, MODEST_EDITABLE_SIZE);
	hildon_gtk_widget_set_theme_size (mark_unread_item, MODEST_EDITABLE_SIZE);
	hildon_gtk_widget_set_theme_size (mark_read_item, MODEST_EDITABLE_SIZE);
	gtk_widget_show_all (priv->csm_menu);

	/* Connect signals */
	g_signal_connect ((GObject *) header_view, "tap-and-hold-query",
			  G_CALLBACK (tap_and_hold_query_cb), self);
	g_signal_connect ((GObject *) delete_item, "activate",
			  G_CALLBACK (on_delete_csm_activated), self);
	g_signal_connect ((GObject *) mark_read_item, "activate",
			  G_CALLBACK (on_mark_read_csm_activated), self);
	g_signal_connect ((GObject *) mark_unread_item, "activate",
			  G_CALLBACK (on_mark_unread_csm_activated), self);

	/* Add tap&hold handling */
	gtk_widget_tap_and_hold_setup (header_view, priv->csm_menu, NULL, 0);

	return header_view;
}

static GtkWidget *
create_empty_view (ModestWindow *self)
{
	GtkWidget *viewport = NULL;
	GtkWidget *label = NULL;
	GtkWidget *align = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *button = NULL;
	GdkPixbuf *new_message_pixbuf;

	vbox = gtk_vbox_new (0, FALSE);

	align = gtk_alignment_new(EMPTYVIEW_XALIGN, EMPTYVIEW_YALIGN, EMPTYVIEW_XSPACE, EMPTYVIEW_YSPACE);
	label = gtk_label_new (_("mcen_ia_nomessages"));
	hildon_helper_set_logical_font (label, "LargeSystemFont");
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
	gtk_widget_show (label);
	gtk_widget_show (align);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);	
	gtk_container_add (GTK_CONTAINER (align), label);
	gtk_box_pack_end (GTK_BOX (vbox), align, TRUE, TRUE, 0);

	button = hildon_button_new (MODEST_EDITABLE_SIZE, 
				    HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	hildon_button_set_title (HILDON_BUTTON (button), _("mcen_ti_new_message"));
	new_message_pixbuf = modest_platform_get_icon ("general_add", MODEST_ICON_SIZE_BIG);
	hildon_button_set_image (HILDON_BUTTON (button), 
				 gtk_image_new_from_pixbuf (new_message_pixbuf));
	g_object_unref (new_message_pixbuf);
	gtk_widget_show_all (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	gtk_widget_show (vbox);

	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (modest_ui_actions_on_new_msg), self);

	viewport = gtk_viewport_new ((GtkAdjustment *) gtk_adjustment_new (0, 0, 0, 0, 0, 0), 
				     (GtkAdjustment *) gtk_adjustment_new (0, 0, 0, 0, 0, 0));
	gtk_container_add (GTK_CONTAINER (viewport), vbox);

	return viewport;
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
modest_header_window_new (TnyFolder *folder, const gchar *account_name, const gchar *mailbox)
{
	ModestHeaderWindow *self = NULL;	
	ModestHeaderWindowPrivate *priv = NULL;
	HildonProgram *app;
	GdkPixbuf *window_icon;
	ModestAccountMgr *mgr;
	ModestAccountSettings *settings = NULL;
	ModestServerAccountSettings *store_settings = NULL;
	GtkWidget *action_area_box;
	GdkPixbuf *new_message_pixbuf;
	GtkWidget *alignment;
	gchar *account_display_name = NULL;
	
	self  = MODEST_HEADER_WINDOW(g_object_new(MODEST_TYPE_HEADER_WINDOW, NULL));
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE(self);

	priv->folder = g_object_ref (folder);

	priv->contents_view = hildon_pannable_area_new ();
	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
				   0, 0,
				   HILDON_MARGIN_DOUBLE, HILDON_MARGIN_DOUBLE);

	/* We need to do this here to properly listen for mail
	   operations because create_header_view launches a mail
	   operation */
	priv->queue_change_handler =
		g_signal_connect (G_OBJECT (modest_runtime_get_mail_operation_queue ()),
				  "queue-changed",
				  G_CALLBACK (on_queue_changed),
				  self);

	priv->header_view  = create_header_view (MODEST_WINDOW (self), folder);
	priv->empty_view = create_empty_view (MODEST_WINDOW (self));

	/* Transform the floating reference in a "hard" reference. We
	   need to do this because the widgets could be added/removed
	   to containers many times so we always need to keep a
	   reference. It could happen also that some widget is never
	   added to any container */
	g_object_ref_sink (priv->header_view);
	g_object_ref_sink (priv->empty_view);

	g_signal_connect (G_OBJECT (self), "edit-mode-changed",
			  G_CALLBACK (edit_mode_changed), (gpointer) self);

	action_area_box = hildon_tree_view_get_action_area_box (GTK_TREE_VIEW (priv->header_view));
	priv->new_message_button = hildon_button_new (MODEST_EDITABLE_SIZE, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	hildon_button_set_title (HILDON_BUTTON (priv->new_message_button), _("mcen_ti_new_message"));
	new_message_pixbuf = modest_platform_get_icon ("general_add", MODEST_ICON_SIZE_BIG);
	hildon_button_set_image (HILDON_BUTTON (priv->new_message_button), gtk_image_new_from_pixbuf (new_message_pixbuf));
	g_object_unref (new_message_pixbuf);

	gtk_box_pack_start (GTK_BOX (action_area_box), priv->new_message_button, TRUE, TRUE, 0);
	gtk_widget_show_all (priv->new_message_button);
	hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (priv->header_view), TRUE);
	
	setup_menu (self);

        priv->top_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment), priv->contents_view);
	gtk_box_pack_end (GTK_BOX (priv->top_vbox), alignment, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (self), priv->top_vbox);

	gtk_widget_show (alignment);
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
						  _("mcen_ti_edit_move"), _HL("wdgt_bd_move"),
						  GTK_TREE_VIEW (priv->header_view),
						  GTK_SELECTION_MULTIPLE,
						  EDIT_MODE_CALLBACK (modest_ui_actions_on_edit_mode_move_to));


	modest_window_set_active_account (MODEST_WINDOW (self), account_name);
	modest_window_set_active_mailbox (MODEST_WINDOW (self), mailbox);
	mgr = modest_runtime_get_account_mgr ();
	settings = modest_account_mgr_load_account_settings (mgr, account_name);
	if (settings) {
		account_display_name = g_strdup (modest_account_settings_get_display_name (settings));
		store_settings = modest_account_settings_get_store_settings (settings);
		if (store_settings) {
			priv->current_store_account = 
				g_strdup (modest_server_account_settings_get_account_name (store_settings));
			g_object_unref (store_settings);
		}
		g_object_unref (settings);
	}
	/* Set window title */
	if (TNY_IS_FOLDER (folder)) {
		gchar *folder_name;

		if (tny_folder_get_folder_type (folder) == TNY_FOLDER_TYPE_INBOX) {
			const gchar *box_name;
			box_name = mailbox;
			if (box_name == NULL || box_name[0] == '\0') {
				box_name = account_display_name;
			}
			folder_name = g_strconcat (_("mcen_me_folder_inbox"), " - ", box_name, NULL);
		} else {
			folder_name = modest_tny_folder_get_display_name (folder);
		}
		
		gtk_window_set_title (GTK_WINDOW (self), folder_name);
		g_free (folder_name);
	}
	g_free (account_display_name);


	update_progress_hint (self);
	update_sort_button (self);

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
	ModestHeaderWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_HEADER_WINDOW(self));
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_new_message"), "<Control>n",
					   APP_MENU_CALLBACK (modest_ui_actions_on_new_msg),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_new_msg));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_move_messages"), NULL,
					   APP_MENU_CALLBACK (set_moveto_edit_mode),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_move_to));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_delete_messages"), NULL,
					   APP_MENU_CALLBACK (set_delete_edit_mode),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_delete));
	modest_hildon2_window_add_to_menu (MODEST_HILDON2_WINDOW (self), _("mcen_me_folder_details"), NULL,
					   APP_MENU_CALLBACK (modest_ui_actions_on_details),
					   MODEST_DIMMING_CALLBACK (modest_ui_dimming_rules_on_details));
	priv->sort_button = hildon_button_new (MODEST_EDITABLE_SIZE,
					       HILDON_BUTTON_ARRANGEMENT_VERTICAL);
	hildon_button_set_title (HILDON_BUTTON (priv->sort_button), _("mcen_me_sort"));
	g_signal_connect (G_OBJECT (priv->sort_button), "clicked",
			  G_CALLBACK (modest_ui_actions_on_sort), (gpointer) self);
	hildon_button_set_style(HILDON_BUTTON (priv->sort_button), HILDON_BUTTON_STYLE_PICKER);
	hildon_button_set_title_alignment (HILDON_BUTTON (priv->sort_button), 0.5, 0.5);
	hildon_button_set_value_alignment (HILDON_BUTTON (priv->sort_button), 0.5, 0.5);
	modest_hildon2_window_add_button_to_menu (MODEST_HILDON2_WINDOW (self), GTK_BUTTON (priv->sort_button),
						  modest_ui_dimming_rules_on_sort);
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
on_map_event(GtkWidget *widget,
	     GdkEvent *event,
	     gpointer user_data)
{
	ModestHeaderWindow *self = (ModestHeaderWindow *) user_data;
	ModestHeaderWindowPrivate *priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	g_return_val_if_fail (MODEST_IS_HEADER_WINDOW (self), FALSE);

	if (priv->progress_hint) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), TRUE);
	}
	return FALSE;
}

static void
on_progress_list_changed (ModestWindowMgr *mgr,
			  ModestHeaderWindow *self)
{
	update_progress_hint (self);
}

static gboolean
has_active_operations (ModestHeaderWindow *self)
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
update_progress_hint (ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);

	priv->progress_hint = FALSE;

	if (has_active_operations (self)) {
		priv->progress_hint = TRUE;
	} else {
		priv->progress_hint = FALSE;
	}

	if (!priv->progress_hint && priv->current_store_account) {
		priv->progress_hint = 
			modest_window_mgr_has_progress_operation_on_account (modest_runtime_get_window_mgr (),
									     priv->current_store_account);
	}

	modest_ui_actions_check_menu_dimming_rules (MODEST_WINDOW (self));

	if (GTK_WIDGET_VISIBLE (self)) {
		hildon_gtk_window_set_progress_indicator (GTK_WINDOW (self), priv->progress_hint?1:0);
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
		update_progress_hint (self);
	}
	g_object_unref (source);
}

static void 
on_mail_operation_finished (ModestMailOperation *mail_op,
			    gpointer user_data)
{
	ModestHeaderWindow *self;

	self = MODEST_HEADER_WINDOW (user_data);

	/* Don't disable the progress hint if there are more pending
	   operations from this window */
	update_progress_hint (self);

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

	hildon_tree_view_set_action_area_visible (GTK_TREE_VIEW (priv->header_view), !enabled);
	if (enabled) {
		modest_header_view_set_filter (MODEST_HEADER_VIEW (priv->header_view), 
					       filter);
	} else {
		GtkTreeSelection *sel;

		/* Unselect all. This will prevent us from keeping a
		   reference to a TnyObject that we don't want to
		   have */
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->header_view));
		gtk_tree_selection_unselect_all (sel);

		modest_header_view_unset_filter (MODEST_HEADER_VIEW (priv->header_view), 
						 filter);
	}
}

static void 
on_sort_column_changed (GtkTreeSortable *treesortable,
			gpointer         user_data)
{
	update_sort_button (MODEST_HEADER_WINDOW (user_data));
}

static void
update_sort_button (ModestHeaderWindow *self)
{
	ModestHeaderWindowPrivate *priv;
	GtkTreeSortable *sortable;
	gint current_sort_colid = -1;
	GtkSortType current_sort_type;
	const gchar *value = NULL;

	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (self);
	sortable = GTK_TREE_SORTABLE (gtk_tree_view_get_model (GTK_TREE_VIEW (priv->header_view)));

	if (!gtk_tree_sortable_get_sort_column_id (sortable,
						   &current_sort_colid, &current_sort_type)) {
		value =  _("mcen_li_sort_sender_date_newest");
	} else {
		switch (current_sort_colid) {
		case TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN:
		{
			GList *cols = NULL;
			cols = modest_header_view_get_columns (MODEST_HEADER_VIEW (priv->header_view));
			if (cols != NULL) {
				gpointer flags_sort_type_pointer;
				flags_sort_type_pointer = g_object_get_data (G_OBJECT (cols->data), 
									     MODEST_HEADER_VIEW_FLAG_SORT);
				if (GPOINTER_TO_INT (flags_sort_type_pointer) == TNY_HEADER_FLAG_PRIORITY_MASK)
					value = _("mcen_li_sort_priority");
				else
					value = _("mcen_li_sort_attachment");
				g_list_free(cols);	
			}
		} 
		break;
		case TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN:
		case TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN:
			if (current_sort_type == GTK_SORT_ASCENDING)
				value = _("mcen_li_sort_sender_recipient_az");
			else
				value = _("mcen_li_sort_sender_recipient_za");
			break;
		case TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN:
		case TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN:
			if (current_sort_type == GTK_SORT_ASCENDING)
				value = _("mcen_li_sort_date_oldest");
			else
				value = _("mcen_li_sort_date_newest");
			break;
		case TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN:
			if (current_sort_type == GTK_SORT_ASCENDING)
				value = _("mcen_li_sort_subject_az");
			else
				value = _("mcen_li_sort_subject_za");
			break;
		case TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN:
			if (current_sort_type == GTK_SORT_ASCENDING)
				value = _("mcen_li_sort_size_smallest");
			else
				value = _("mcen_li_sort_size_largest");
			break;
		} 
	}

	hildon_button_set_value (HILDON_BUTTON (priv->sort_button), value?value:"");
}

static void
on_horizontal_movement (HildonPannableArea *hildonpannable,
			gint                direction,
			gdouble             initial_x,
			gdouble             initial_y,
			gpointer            user_data)
{
	ModestHeaderWindowPrivate *priv;
	gint dest_x, dest_y;
	TnyHeader *header;

	/* Ignore right to left movement */
	if (direction == HILDON_MOVEMENT_LEFT)
		return;

	/* Get the header to delete */
	priv = MODEST_HEADER_WINDOW_GET_PRIVATE (user_data);

	/* Get tree view coordinates */
	if (!gtk_widget_translate_coordinates ((GtkWidget *) hildonpannable,
					       priv->header_view,
					       initial_x,
					       initial_y,
					       &dest_x,
					       &dest_y))
	    return;

	header = modest_header_view_get_header_at_pos ((ModestHeaderView *) priv->header_view,
						       dest_x, dest_y);
	if (header) {
		delete_header ((GtkWindow *) user_data, header);
		g_object_unref (header);
	}
}

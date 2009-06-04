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
#include <tny-list.h>
#include <tny-simple-list.h>
#include <tny-folder-monitor.h>
#include <tny-folder-change.h>
#include <tny-error.h>
#include <tny-merge-folder.h>
#include <string.h>

#include <modest-header-view.h>
#include <modest-header-view-priv.h>
#include <modest-dnd.h>
#include <modest-tny-folder.h>
#include <modest-debug.h>
#include <modest-main-window.h>
#include <modest-ui-actions.h>
#include <modest-marshal.h>
#include <modest-text-utils.h>
#include <modest-icon-names.h>
#include <modest-runtime.h>
#include "modest-platform.h"
#include <modest-hbox-cell-renderer.h>
#include <modest-vbox-cell-renderer.h>
#include <modest-datetime-formatter.h>
#include <modest-ui-constants.h>

static void modest_header_view_class_init  (ModestHeaderViewClass *klass);
static void modest_header_view_init        (ModestHeaderView *obj);
static void modest_header_view_finalize    (GObject *obj);
static void modest_header_view_dispose     (GObject *obj);

static void          on_header_row_activated (GtkTreeView *treeview, GtkTreePath *path,
					      GtkTreeViewColumn *column, gpointer userdata);

static gint          cmp_rows               (GtkTreeModel *tree_model,
					     GtkTreeIter *iter1,
					     GtkTreeIter *iter2,
					     gpointer user_data);

static gint          cmp_subject_rows       (GtkTreeModel *tree_model,
					     GtkTreeIter *iter1,
					     GtkTreeIter *iter2,
					     gpointer user_data);

static gboolean     filter_row             (GtkTreeModel *model,
					    GtkTreeIter *iter,
					    gpointer data);

static void         on_account_removed     (TnyAccountStore *self,
					    TnyAccount *account,
					    gpointer user_data);

static void          on_selection_changed   (GtkTreeSelection *sel,
					     gpointer user_data);

static gboolean      on_button_press_event  (GtkWidget * self, GdkEventButton * event,
                                             gpointer userdata);

static gboolean      on_button_release_event(GtkWidget * self, GdkEventButton * event,
                                             gpointer userdata);

static void          setup_drag_and_drop    (GtkWidget *self);

static void          enable_drag_and_drop   (GtkWidget *self);

static void          disable_drag_and_drop  (GtkWidget *self);

static GtkTreePath * get_selected_row       (GtkTreeView *self, GtkTreeModel **model);

#ifndef MODEST_TOOLKIT_HILDON2
static gboolean      on_focus_in            (GtkWidget     *sef,
					     GdkEventFocus *event,
					     gpointer       user_data);

static gboolean      on_focus_out            (GtkWidget     *self,
					      GdkEventFocus *event,
					      gpointer       user_data);
#endif

static void          folder_monitor_update  (TnyFolderObserver *self,
					     TnyFolderChange *change);

static void          tny_folder_observer_init (TnyFolderObserverIface *klass);

static void          _clipboard_set_selected_data (ModestHeaderView *header_view, gboolean delete);

static void          _clear_hidding_filter (ModestHeaderView *header_view);

static void          modest_header_view_notify_observers(ModestHeaderView *header_view,
							 GtkTreeModel *model,
							 const gchar *tny_folder_id);

static gboolean      modest_header_view_on_expose_event (GtkTreeView *header_view,
							 GdkEventExpose *event,
							 gpointer user_data);

static void         on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata);
static void         update_style (ModestHeaderView *self);

typedef enum {
	HEADER_VIEW_NON_EMPTY,
	HEADER_VIEW_EMPTY,
	HEADER_VIEW_INIT
} HeaderViewStatus;

typedef struct _ModestHeaderViewPrivate ModestHeaderViewPrivate;
struct _ModestHeaderViewPrivate {
	TnyFolder            *folder;
	ModestHeaderViewStyle style;
	gboolean is_outbox;

	TnyFolderMonitor     *monitor;
	GMutex               *observers_lock;

	/*header-view-observer observer*/
	GMutex *observer_list_lock;
	GSList *observer_list;

	/* not unref this object, its a singlenton */
	ModestEmailClipboard *clipboard;

	/* Filter tree model */
	gchar **hidding_ids;
	guint   n_selected;
	GtkTreeRowReference *autoselect_reference;
	ModestHeaderViewFilter filter;

	gint    sort_colid[2][TNY_FOLDER_TYPE_NUM];
	gint    sort_type[2][TNY_FOLDER_TYPE_NUM];

	gulong  selection_changed_handler;
	gulong  acc_removed_handler;

	GList *drag_begin_cached_selected_rows;

	HeaderViewStatus status;
	guint status_timeout;
	gboolean notify_status; /* whether or not the filter_row should notify about changes in the filtering */

	ModestDatetimeFormatter *datetime_formatter;

	GtkCellRenderer *renderer_subject;
	GtkCellRenderer *renderer_address;
	GtkCellRenderer *renderer_date_status;

	GdkColor active_color;
	GdkColor secondary_color;
};

typedef struct _HeadersCountChangedHelper HeadersCountChangedHelper;
struct _HeadersCountChangedHelper {
	ModestHeaderView *self;
	TnyFolderChange  *change;
};


#define MODEST_HEADER_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
						MODEST_TYPE_HEADER_VIEW, \
                                                ModestHeaderViewPrivate))



#define MODEST_HEADER_VIEW_PTR "modest-header-view"

enum {
	HEADER_SELECTED_SIGNAL,
	HEADER_ACTIVATED_SIGNAL,
	ITEM_NOT_FOUND_SIGNAL,
	MSG_COUNT_CHANGED_SIGNAL,
	UPDATING_MSG_LIST_SIGNAL,
	LAST_SIGNAL
};

/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

GType
modest_header_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestHeaderViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_header_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestHeaderView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_header_view_init,
			NULL
		};

		static const GInterfaceInfo tny_folder_observer_info =
		{
			(GInterfaceInitFunc) tny_folder_observer_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestHeaderView",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, TNY_TYPE_FOLDER_OBSERVER,
					     &tny_folder_observer_info);


	}
	return my_type;
}

static void
modest_header_view_class_init (ModestHeaderViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_header_view_finalize;
	gobject_class->dispose = modest_header_view_dispose;

	g_type_class_add_private (gobject_class, sizeof(ModestHeaderViewPrivate));

	signals[HEADER_SELECTED_SIGNAL] =
		g_signal_new ("header_selected",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass,header_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

	signals[HEADER_ACTIVATED_SIGNAL] =
		g_signal_new ("header_activated",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass,header_activated),
			      NULL, NULL,
			      gtk_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);


	signals[ITEM_NOT_FOUND_SIGNAL] =
		g_signal_new ("item_not_found",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass,item_not_found),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

	signals[MSG_COUNT_CHANGED_SIGNAL] =
		g_signal_new ("msg_count_changed",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass, msg_count_changed),
			      NULL, NULL,
			      modest_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER);

	signals[UPDATING_MSG_LIST_SIGNAL] =
		g_signal_new ("updating-msg-list",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass, updating_msg_list),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

#ifdef MODEST_TOOLKIT_HILDON2
	gtk_rc_parse_string ("class \"ModestHeaderView\" style \"fremantle-touchlist\"");

#endif
}

static void
tny_folder_observer_init (TnyFolderObserverIface *klass)
{
	klass->update = folder_monitor_update;
}

static GtkTreeViewColumn*
get_new_column (const gchar *name, GtkCellRenderer *renderer,
		gboolean resizable, gint sort_col_id, gboolean show_as_text,
		GtkTreeCellDataFunc cell_data_func, gpointer user_data)
{
	GtkTreeViewColumn *column;

	column =  gtk_tree_view_column_new_with_attributes(name, renderer, NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);

	gtk_tree_view_column_set_resizable (column, resizable);
	if (resizable)
		gtk_tree_view_column_set_expand (column, TRUE);

	if (show_as_text)
		gtk_tree_view_column_add_attribute (column, renderer, "text",
						    sort_col_id);
	if (sort_col_id >= 0)
		gtk_tree_view_column_set_sort_column_id (column, sort_col_id);

	gtk_tree_view_column_set_sort_indicator (column, FALSE);
	gtk_tree_view_column_set_reorderable (column, TRUE);

	if (cell_data_func)
		gtk_tree_view_column_set_cell_data_func(column, renderer, cell_data_func,
							user_data, NULL);
	return column;
}


static void
remove_all_columns (ModestHeaderView *obj)
{
	GList *columns, *cursor;

	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW(obj));

	for (cursor = columns; cursor; cursor = cursor->next)
		gtk_tree_view_remove_column (GTK_TREE_VIEW(obj),
					     GTK_TREE_VIEW_COLUMN(cursor->data));
	g_list_free (columns);
}

gboolean
modest_header_view_set_columns (ModestHeaderView *self, const GList *columns, TnyFolderType type)
{
	GtkTreeModel *sortable;
	GtkTreeViewColumn *column=NULL;
	GtkTreeSelection *selection = NULL;
	GtkCellRenderer *renderer_header,
		*renderer_attach, *renderer_compact_date_or_status;
	GtkCellRenderer *renderer_compact_header, *renderer_recpt_box,
		*renderer_subject_box, *renderer_recpt,
		*renderer_priority;
	ModestHeaderViewPrivate *priv;
	GtkTreeViewColumn *compact_column = NULL;
	const GList *cursor;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), FALSE);
	g_return_val_if_fail (type != TNY_FOLDER_TYPE_INVALID, FALSE);

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	priv->is_outbox = (type == TNY_FOLDER_TYPE_OUTBOX);

	/* TODO: check whether these renderers need to be freed */
	renderer_attach  = gtk_cell_renderer_pixbuf_new ();
	renderer_priority  = gtk_cell_renderer_pixbuf_new ();
	renderer_header  = gtk_cell_renderer_text_new ();

	renderer_compact_header = modest_vbox_cell_renderer_new ();
	renderer_recpt_box = modest_hbox_cell_renderer_new ();
	renderer_subject_box = modest_hbox_cell_renderer_new ();
	renderer_recpt = gtk_cell_renderer_text_new ();
	priv->renderer_address = renderer_recpt;
	priv->renderer_subject = gtk_cell_renderer_text_new ();
	renderer_compact_date_or_status  = gtk_cell_renderer_text_new ();
	priv->renderer_date_status = renderer_compact_date_or_status;

	modest_vbox_cell_renderer_append (MODEST_VBOX_CELL_RENDERER (renderer_compact_header), renderer_subject_box, FALSE);
	g_object_set_data (G_OBJECT (renderer_compact_header), "subject-box-renderer", renderer_subject_box);
	modest_vbox_cell_renderer_append (MODEST_VBOX_CELL_RENDERER (renderer_compact_header), renderer_recpt_box, FALSE);
	g_object_set_data (G_OBJECT (renderer_compact_header), "recpt-box-renderer", renderer_recpt_box);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_subject_box), renderer_priority, FALSE);
	g_object_set_data (G_OBJECT (renderer_subject_box), "priority-renderer", renderer_priority);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_subject_box), priv->renderer_subject, TRUE);
	g_object_set_data (G_OBJECT (renderer_subject_box), "subject-renderer", priv->renderer_subject);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_recpt_box), renderer_attach, FALSE);
	g_object_set_data (G_OBJECT (renderer_recpt_box), "attach-renderer", renderer_attach);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_recpt_box), renderer_recpt, TRUE);
	g_object_set_data (G_OBJECT (renderer_recpt_box), "recipient-renderer", renderer_recpt);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_recpt_box), renderer_compact_date_or_status, FALSE);
	g_object_set_data (G_OBJECT (renderer_recpt_box), "date-renderer", renderer_compact_date_or_status);

#ifdef MODEST_TOOLKIT_HILDON2
	g_object_set (G_OBJECT (renderer_compact_header), "xpad", 0, NULL);
#endif
	g_object_set (G_OBJECT (renderer_subject_box), "yalign", 1.0, NULL);
#ifndef MODEST_TOOLKIT_GTK
	gtk_cell_renderer_set_fixed_size (renderer_subject_box, -1, 32);
	gtk_cell_renderer_set_fixed_size (renderer_recpt_box, -1, 32);
#endif
	g_object_set (G_OBJECT (renderer_recpt_box), "yalign", 0.0, NULL);
	g_object_set(G_OBJECT(renderer_header),
		     "ellipsize", PANGO_ELLIPSIZE_END,
		     NULL);
	g_object_set (G_OBJECT (priv->renderer_subject),
		      "ellipsize", PANGO_ELLIPSIZE_END, "yalign", 1.0,
		      NULL);
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (priv->renderer_subject), 1);
	g_object_set (G_OBJECT (renderer_recpt),
		      "ellipsize", PANGO_ELLIPSIZE_END, "yalign", 0.1,
		      NULL);
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (renderer_recpt), 1);
	g_object_set(G_OBJECT(renderer_compact_date_or_status),
		     "xalign", 1.0, "yalign", 0.1,
		     NULL);
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT (renderer_compact_date_or_status), 1);
#ifdef MODEST_TOOLKIT_HILDON2
	g_object_set (G_OBJECT (renderer_priority),
		      "yalign", 0.5,
		      "xalign", 0.0, NULL);
	g_object_set (G_OBJECT (renderer_attach),
		      "yalign", 0.5,
		      "xalign", 0.0, NULL);
#else
	g_object_set (G_OBJECT (renderer_priority),
		      "yalign", 0.5, NULL);
	g_object_set (G_OBJECT (renderer_attach),
		      "yalign", 0.0, NULL);
#endif

#ifdef MODEST_TOOLKIT_HILDON1
	gtk_cell_renderer_set_fixed_size (renderer_attach, 32, 26);
	gtk_cell_renderer_set_fixed_size (renderer_priority, 32, 26);
	gtk_cell_renderer_set_fixed_size (renderer_compact_header, -1, 64);
#elif MODEST_TOOLKIT_HILDON2
	gtk_cell_renderer_set_fixed_size (renderer_attach, 24 + MODEST_MARGIN_DEFAULT, 26);
	gtk_cell_renderer_set_fixed_size (renderer_priority, 24 + MODEST_MARGIN_DEFAULT, 26);
	gtk_cell_renderer_set_fixed_size (renderer_compact_header, -1, 64);
#else
	gtk_cell_renderer_set_fixed_size (renderer_attach, 16, 16);
	gtk_cell_renderer_set_fixed_size (renderer_priority, 16, 16);
	/* gtk_cell_renderer_set_fixed_size (renderer_compact_header, -1, 64); */
#endif

	remove_all_columns (self);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	sortable = gtk_tree_view_get_model (GTK_TREE_VIEW (self));

	/* Add new columns */
	for (cursor = columns; cursor; cursor = g_list_next(cursor)) {
		ModestHeaderViewColumn col =
			(ModestHeaderViewColumn) GPOINTER_TO_INT(cursor->data);

		if (0> col || col >= MODEST_HEADER_VIEW_COLUMN_NUM) {
			g_printerr ("modest: invalid column %d in column list\n", col);
			continue;
		}

		switch (col) {

		case MODEST_HEADER_VIEW_COLUMN_ATTACH:
			column = get_new_column (_("A"), renderer_attach, FALSE,
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_attach_cell_data,
						 NULL);
			gtk_tree_view_column_set_fixed_width (column, 45);
			break;


		case MODEST_HEADER_VIEW_COLUMN_FROM:
			column = get_new_column (_("From"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_sender_receiver_cell_data,
						 GINT_TO_POINTER(TRUE));
			break;

		case MODEST_HEADER_VIEW_COLUMN_TO:
			column = get_new_column (_("To"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_sender_receiver_cell_data,
						 GINT_TO_POINTER(FALSE));
			break;

		case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN:
			column = get_new_column (_("Header"), renderer_compact_header, TRUE,
						     TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,
						     FALSE,
						     (GtkTreeCellDataFunc)_modest_header_view_compact_header_cell_data,
						     GINT_TO_POINTER(MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_IN));
			compact_column = column;
			break;

		case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT:
			column = get_new_column (_("Header"), renderer_compact_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_compact_header_cell_data,
						 GINT_TO_POINTER((type == TNY_FOLDER_TYPE_OUTBOX)?
								 MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_OUTBOX:
								 MODEST_HEADER_VIEW_COMPACT_HEADER_MODE_OUT));
			compact_column = column;
			break;


		case MODEST_HEADER_VIEW_COLUMN_SUBJECT:
			column = get_new_column (_("Subject"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_header_cell_data,
						 NULL);
			break;

		case MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE:
			column = get_new_column (_("Received"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_date_cell_data,
						 GINT_TO_POINTER(TRUE));
			break;

		case MODEST_HEADER_VIEW_COLUMN_SENT_DATE:
			column = get_new_column (_("Sent"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_date_cell_data,
						 GINT_TO_POINTER(FALSE));
			break;

		case MODEST_HEADER_VIEW_COLUMN_SIZE:
			column = get_new_column (_("Size"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_size_cell_data,
						 NULL);
			break;
		case MODEST_HEADER_VIEW_COLUMN_STATUS:
			column = get_new_column (_("Status"), renderer_compact_date_or_status, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_status_cell_data,
						 NULL);
			break;

		default:
			g_return_val_if_reached(FALSE);
		}

		/* we keep the column id around */
		g_object_set_data (G_OBJECT(column), MODEST_HEADER_VIEW_COLUMN,
				   GINT_TO_POINTER(col));

		/* we need this ptr when sorting the rows */
		g_object_set_data (G_OBJECT(column), MODEST_HEADER_VIEW_PTR,
				   self);
		gtk_tree_view_append_column (GTK_TREE_VIEW(self), column);
	}

	if (sortable) {
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_rows,
						 compact_column, NULL);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
						 TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_subject_rows,
						 compact_column, NULL);
	}

	update_style (self);
	g_signal_connect (G_OBJECT (self), "notify::style", G_CALLBACK (on_notify_style), (gpointer) self);

	return TRUE;
}

static void
datetime_format_changed (ModestDatetimeFormatter *formatter,
			 ModestHeaderView *self)
{
	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
modest_header_view_init (ModestHeaderView *obj)
{
	ModestHeaderViewPrivate *priv;
	guint i, j;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(obj);

	priv->folder  = NULL;
	priv->is_outbox = FALSE;

	priv->monitor	     = NULL;
	priv->observers_lock = g_mutex_new ();
	priv->autoselect_reference = NULL;

	priv->status  = HEADER_VIEW_INIT;
	priv->status_timeout = 0;
	priv->notify_status = TRUE;

	priv->observer_list_lock = g_mutex_new();
	priv->observer_list = NULL;

	priv->clipboard = modest_runtime_get_email_clipboard ();
	priv->hidding_ids = NULL;
	priv->n_selected = 0;
	priv->filter = MODEST_HEADER_VIEW_FILTER_NONE;
	priv->selection_changed_handler = 0;
	priv->acc_removed_handler = 0;

	/* Sort parameters */
	for (j=0; j < 2; j++) {
		for (i=0; i < TNY_FOLDER_TYPE_NUM; i++) {
			priv->sort_colid[j][i] = -1;
			priv->sort_type[j][i] = GTK_SORT_DESCENDING;
		}
	}

	priv->datetime_formatter = modest_datetime_formatter_new ();
	g_signal_connect (G_OBJECT (priv->datetime_formatter), "format-changed",
			  G_CALLBACK (datetime_format_changed), (gpointer) obj);

	setup_drag_and_drop (GTK_WIDGET(obj));
}

static void
modest_header_view_dispose (GObject *obj)
{
	ModestHeaderView        *self;
	ModestHeaderViewPrivate *priv;
	GtkTreeSelection *sel;

	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->datetime_formatter) {
		g_object_unref (priv->datetime_formatter);
		priv->datetime_formatter = NULL;
	}

	/* Free in the dispose to avoid unref cycles */
	if (priv->folder) {
		tny_folder_remove_observer (priv->folder, TNY_FOLDER_OBSERVER (obj));
		g_object_unref (G_OBJECT (priv->folder));
		priv->folder = NULL;
	}

	/* We need to do this here in the dispose because the
	   selection won't exist when finalizing */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(self));
	if (sel && g_signal_handler_is_connected (sel, priv->selection_changed_handler)) {
		g_signal_handler_disconnect (sel, priv->selection_changed_handler);
		priv->selection_changed_handler = 0;
	}

	G_OBJECT_CLASS(parent_class)->dispose (obj);
}

static void
modest_header_view_finalize (GObject *obj)
{
	ModestHeaderView        *self;
	ModestHeaderViewPrivate *priv;

	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (g_signal_handler_is_connected (modest_runtime_get_account_store (),
					   priv->acc_removed_handler)) {
		g_signal_handler_disconnect (modest_runtime_get_account_store (),
					     priv->acc_removed_handler);
	}

	/* There is no need to lock because there should not be any
	 * reference to self now. */
	g_mutex_free(priv->observer_list_lock);
	g_slist_free(priv->observer_list);

	g_mutex_lock (priv->observers_lock);
	if (priv->monitor) {
		tny_folder_monitor_stop (priv->monitor);
		g_object_unref (G_OBJECT (priv->monitor));
	}
	g_mutex_unlock (priv->observers_lock);
	g_mutex_free (priv->observers_lock);

	/* Clear hidding array created by cut operation */
	_clear_hidding_filter (MODEST_HEADER_VIEW (obj));

	if (priv->autoselect_reference != NULL) {
		gtk_tree_row_reference_free (priv->autoselect_reference);
		priv->autoselect_reference = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


GtkWidget*
modest_header_view_new (TnyFolder *folder, ModestHeaderViewStyle style)
{
	GObject *obj;
	GtkTreeSelection *sel;
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;

	g_return_val_if_fail (style >= 0 && style < MODEST_HEADER_VIEW_STYLE_NUM,
			      NULL);

	obj  = G_OBJECT(g_object_new(MODEST_TYPE_HEADER_VIEW, NULL));
	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	modest_header_view_set_style   (self, style);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(obj));
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(obj),TRUE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(obj), TRUE);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(obj),
				      TRUE); /* alternating row colors */

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	priv->selection_changed_handler =
		g_signal_connect_after (sel, "changed",
					G_CALLBACK(on_selection_changed), self);

	g_signal_connect (self, "row-activated",
			  G_CALLBACK (on_header_row_activated), NULL);

#ifndef MODEST_TOOLKIT_HILDON2
	g_signal_connect (self, "focus-in-event",
			  G_CALLBACK(on_focus_in), NULL);
	g_signal_connect (self, "focus-out-event",
			  G_CALLBACK(on_focus_out), NULL);
#endif

	g_signal_connect (self, "button-press-event",
			  G_CALLBACK(on_button_press_event), NULL);
	g_signal_connect (self, "button-release-event",
			  G_CALLBACK(on_button_release_event), NULL);

	priv->acc_removed_handler = g_signal_connect (modest_runtime_get_account_store (),
						      "account_removed",
						      G_CALLBACK (on_account_removed),
						      self);

	g_signal_connect (self, "expose-event",
			G_CALLBACK(modest_header_view_on_expose_event),
			NULL);

	return GTK_WIDGET(self);
}


guint
modest_header_view_count_selected_headers (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	guint selected_rows;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), 0);

	/* Get selection object and check selected rows count */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	selected_rows = gtk_tree_selection_count_selected_rows (sel);

	return selected_rows;
}

gboolean
modest_header_view_has_selected_headers (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	gboolean empty;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), FALSE);

	/* Get selection object and check selected rows count */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	empty = gtk_tree_selection_count_selected_rows (sel) == 0;

	return !empty;
}


TnyList *
modest_header_view_get_selected_headers (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	TnyList *header_list = NULL;
	TnyHeader *header;
	GList *list, *tmp = NULL;
	GtkTreeModel *tree_model = NULL;
	GtkTreeIter iter;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), NULL);


	/* Get selected rows */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	list = gtk_tree_selection_get_selected_rows (sel, &tree_model);

	if (list) {
		header_list = tny_simple_list_new();

		list = g_list_reverse (list);
		tmp = list;
		while (tmp) {
			/* get header from selection */
			gtk_tree_model_get_iter (tree_model, &iter, (GtkTreePath *) (tmp->data));
			gtk_tree_model_get (tree_model, &iter,
					    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header, -1);
			/* Prepend to list */
			tny_list_prepend (header_list, G_OBJECT (header));
			g_object_unref (G_OBJECT (header));

			tmp = g_list_next (tmp);
		}
		/* Clean up*/
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	return header_list;
}


/* scroll our list view so the selected item is visible */
static void
scroll_to_selected (ModestHeaderView *self, GtkTreeIter *iter, gboolean up)
{
#ifdef MODEST_TOOLKIT_GTK

	GtkTreePath *selected_path;
	GtkTreePath *start, *end;

	GtkTreeModel *model;

	model         = gtk_tree_view_get_model (GTK_TREE_VIEW(self));
	selected_path = gtk_tree_model_get_path (model, iter);

	start = gtk_tree_path_new ();
	end   = gtk_tree_path_new ();

	gtk_tree_view_get_visible_range (GTK_TREE_VIEW(self), &start, &end);

	if (gtk_tree_path_compare (selected_path, start) < 0 ||
	    gtk_tree_path_compare (end, selected_path) < 0)
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(self),
					      selected_path, NULL, TRUE,
					      up ? 0.0 : 1.0,
					      up ? 0.0 : 1.0);
	gtk_tree_path_free (selected_path);
	gtk_tree_path_free (start);
	gtk_tree_path_free (end);

#endif /* MODEST_TOOLKIT_GTK */
}


void
modest_header_view_select_next (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	g_return_if_fail (self && MODEST_IS_HEADER_VIEW(self));

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	path = get_selected_row (GTK_TREE_VIEW(self), &model);
	if ((path != NULL) && (gtk_tree_model_get_iter(model, &iter, path))) {
		/* Unselect previous path */
		gtk_tree_selection_unselect_path (sel, path);

		/* Move path down and selects new one  */
		if (gtk_tree_model_iter_next (model, &iter)) {
			gtk_tree_selection_select_iter (sel, &iter);
			scroll_to_selected (self, &iter, FALSE);
		}
		gtk_tree_path_free(path);
	}

}

void
modest_header_view_select_prev (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

	g_return_if_fail (self && MODEST_IS_HEADER_VIEW(self));

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	path = get_selected_row (GTK_TREE_VIEW(self), &model);
	if ((path != NULL) && (gtk_tree_model_get_iter(model, &iter, path))) {
		/* Unselect previous path */
		gtk_tree_selection_unselect_path (sel, path);

		/* Move path up */
		if (gtk_tree_path_prev (path)) {
			gtk_tree_model_get_iter (model, &iter, path);

			/* Select the new one */
			gtk_tree_selection_select_iter (sel, &iter);
			scroll_to_selected (self, &iter, TRUE);

		}
		gtk_tree_path_free (path);
	}
}

GList*
modest_header_view_get_columns (ModestHeaderView *self)
{
	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), NULL);

	return gtk_tree_view_get_columns (GTK_TREE_VIEW(self));
}



gboolean
modest_header_view_set_style (ModestHeaderView *self,
			      ModestHeaderViewStyle style)
{
	ModestHeaderViewPrivate *priv;
	gboolean show_col_headers = FALSE;
	ModestHeaderViewStyle old_style;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), FALSE);
	g_return_val_if_fail (style >= 0 && MODEST_HEADER_VIEW_STYLE_NUM,
			      FALSE);

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);
	if (priv->style == style)
		return TRUE; /* nothing to do */

	switch (style) {
	case MODEST_HEADER_VIEW_STYLE_DETAILS:
		show_col_headers = TRUE;
		break;
	case MODEST_HEADER_VIEW_STYLE_TWOLINES:
		break;
	default:
		g_return_val_if_reached (FALSE);
	}
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(self), show_col_headers);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(self), show_col_headers);

	old_style   = priv->style;
	priv->style = style;

	return TRUE;
}


ModestHeaderViewStyle
modest_header_view_get_style (ModestHeaderView *self)
{
	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), FALSE);

	return MODEST_HEADER_VIEW_GET_PRIVATE(self)->style;
}

/* This is used to automatically select the first header if the user
 * has not selected any header yet.
 */
static gboolean
modest_header_view_on_expose_event(GtkTreeView *header_view,
				   GdkEventExpose *event,
				   gpointer user_data)
{
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter tree_iter;
	ModestHeaderViewPrivate *priv;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(header_view);
	model = gtk_tree_view_get_model(header_view);

	if (!model)
		return FALSE;

#ifdef MODEST_TOOLKIT_HILDON2
	return FALSE;
#endif
	sel = gtk_tree_view_get_selection(header_view);
	if(!gtk_tree_selection_count_selected_rows(sel)) {
		if (gtk_tree_model_get_iter_first(model, &tree_iter)) {
			GtkTreePath *tree_iter_path;
			/* Prevent the widget from getting the focus
			   when selecting the first item */
			tree_iter_path = gtk_tree_model_get_path (model, &tree_iter);
			g_object_set(header_view, "can-focus", FALSE, NULL);
			gtk_tree_selection_select_iter(sel, &tree_iter);
			gtk_tree_view_set_cursor (header_view, tree_iter_path, NULL, FALSE);
			g_object_set(header_view, "can-focus", TRUE, NULL);
			if (priv->autoselect_reference) {
				gtk_tree_row_reference_free (priv->autoselect_reference);
			}
			priv->autoselect_reference = gtk_tree_row_reference_new (model, tree_iter_path);
			gtk_tree_path_free (tree_iter_path);
		}
	} else {
		if (priv->autoselect_reference != NULL) {
			gboolean moved_selection = FALSE;
			GtkTreePath * last_path;
			if (gtk_tree_selection_count_selected_rows (sel) != 1) {
				moved_selection = TRUE;
			} else {
				GList *rows;

				rows = gtk_tree_selection_get_selected_rows (sel, NULL);
				last_path = gtk_tree_row_reference_get_path (priv->autoselect_reference);
				if (gtk_tree_path_compare (last_path, (GtkTreePath *) rows->data) != 0)
					moved_selection = TRUE;
				g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
				g_list_free (rows);
				gtk_tree_path_free (last_path);
			}
			if (moved_selection) {
				gtk_tree_row_reference_free (priv->autoselect_reference);
				priv->autoselect_reference = NULL;
			} else {

				if (gtk_tree_model_get_iter_first (model, &tree_iter)) {
					GtkTreePath *current_path;
					current_path = gtk_tree_model_get_path (model, &tree_iter);
					last_path = gtk_tree_row_reference_get_path (priv->autoselect_reference);
					if (gtk_tree_path_compare (current_path, last_path) != 0) {
						g_object_set(header_view, "can-focus", FALSE, NULL);
						gtk_tree_selection_unselect_all (sel);
						gtk_tree_selection_select_iter(sel, &tree_iter);
						gtk_tree_view_set_cursor (header_view, current_path, NULL, FALSE);
						g_object_set(header_view, "can-focus", TRUE, NULL);
						gtk_tree_row_reference_free (priv->autoselect_reference);
						priv->autoselect_reference = gtk_tree_row_reference_new (model, current_path);
					}
					gtk_tree_path_free (current_path);
					gtk_tree_path_free (last_path);
				}
			}
		}
	}

	return FALSE;
}

TnyFolder*
modest_header_view_get_folder (ModestHeaderView *self)
{
	ModestHeaderViewPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), NULL);

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->folder)
		g_object_ref (priv->folder);

	return priv->folder;
}

static void
set_folder_intern_get_headers_async_cb (TnyFolder *folder,
					gboolean cancelled,
					TnyList *headers,
					GError *err,
					gpointer user_data)
{
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;

	g_return_if_fail (MODEST_IS_HEADER_VIEW (user_data));

	self = MODEST_HEADER_VIEW (user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (cancelled || err)
		return;

	/* Add IDLE observer (monitor) and another folder observer for
	   new messages (self) */
	g_mutex_lock (priv->observers_lock);
	if (priv->monitor) {
		tny_folder_monitor_stop (priv->monitor);
		g_object_unref (G_OBJECT (priv->monitor));
	}
	priv->monitor = TNY_FOLDER_MONITOR (tny_folder_monitor_new (folder));
	tny_folder_monitor_add_list (priv->monitor, TNY_LIST (headers));
	tny_folder_monitor_start (priv->monitor);
	g_mutex_unlock (priv->observers_lock);
}

static void
modest_header_view_set_folder_intern (ModestHeaderView *self, TnyFolder *folder)
{
	TnyFolderType type;
	TnyList *headers;
	ModestHeaderViewPrivate *priv;
	GList *cols, *cursor;
	GtkTreeModel *filter_model, *sortable;
	guint sort_colid;
	GtkSortType sort_type;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	headers = TNY_LIST (tny_gtk_header_list_model_new ());

	/* Start the monitor in the callback of the
	   tny_gtk_header_list_model_set_folder call. It's crucial to
	   do it there and not just after the call because we want the
	   monitor to observe only the headers returned by the
	   tny_folder_get_headers_async call that it's inside the
	   tny_gtk_header_list_model_set_folder call. This way the
	   monitor infrastructure could successfully cope with
	   duplicates. For example if a tny_folder_add_msg_async is
	   happening while tny_gtk_header_list_model_set_folder is
	   invoked, then the first call could add a header that will
	   be added again by tny_gtk_header_list_model_set_folder, so
	   we'd end up with duplicate headers. sergio */
	tny_gtk_header_list_model_set_folder (TNY_GTK_HEADER_LIST_MODEL(headers),
					      folder, FALSE,
					      set_folder_intern_get_headers_async_cb,
					      NULL, self);

	/* Create a tree model filter to hide and show rows for cut operations  */
	filter_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (headers), NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
						filter_row, self, NULL);
	g_object_unref (headers);

	/* Init filter_row function to examine empty status */
	priv->status  = HEADER_VIEW_INIT;

	/* Create sortable model */
	sortable = gtk_tree_model_sort_new_with_model (filter_model);
	g_object_unref (filter_model);

	/* install our special sorting functions */
	cursor = cols = gtk_tree_view_get_columns (GTK_TREE_VIEW(self));

	/* Restore sort column id */
	if (cols) {
		type  = modest_tny_folder_guess_folder_type (folder);
		if (type == TNY_FOLDER_TYPE_INVALID)
			g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);

		sort_colid = modest_header_view_get_sort_column_id (self, type);
		sort_type = modest_header_view_get_sort_type (self, type);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortable),
						      sort_colid,
						      sort_type);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_rows,
						 cols->data, NULL);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
						 TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_subject_rows,
						 cols->data, NULL);
	}

	/* Set new model */
	gtk_tree_view_set_model (GTK_TREE_VIEW (self), sortable);
	modest_header_view_notify_observers (self, sortable, tny_folder_get_id (folder));
	g_object_unref (sortable);

	/* Free */
	g_list_free (cols);
}

void
modest_header_view_sort_by_column_id (ModestHeaderView *self,
				      guint sort_colid,
				      GtkSortType sort_type)
{
	ModestHeaderViewPrivate *priv = NULL;
	GtkTreeModel *sortable = NULL;
	TnyFolderType type;

	g_return_if_fail (self && MODEST_IS_HEADER_VIEW(self));
	g_return_if_fail (sort_type == GTK_SORT_ASCENDING || sort_type == GTK_SORT_DESCENDING);

	/* Get model and private data */
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (self);
	sortable = gtk_tree_view_get_model (GTK_TREE_VIEW (self));

	/* Sort tree model */
	type  = modest_tny_folder_guess_folder_type (priv->folder);
	if (type == TNY_FOLDER_TYPE_INVALID)
		g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);
	else {
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortable),
						      sort_colid,
						      sort_type);
		/* Store new sort parameters */
		modest_header_view_set_sort_params (self, sort_colid, sort_type, type);
	}
}

void
modest_header_view_set_sort_params (ModestHeaderView *self,
				    guint sort_colid,
				    GtkSortType sort_type,
				    TnyFolderType type)
{
	ModestHeaderViewPrivate *priv;
	ModestHeaderViewStyle style;

	g_return_if_fail (self && MODEST_IS_HEADER_VIEW(self));
	g_return_if_fail (sort_type == GTK_SORT_ASCENDING || sort_type == GTK_SORT_DESCENDING);
	g_return_if_fail (type != TNY_FOLDER_TYPE_INVALID);

	style = modest_header_view_get_style   (self);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	priv->sort_colid[style][type] = sort_colid;
	priv->sort_type[style][type] = sort_type;
}

gint
modest_header_view_get_sort_column_id (ModestHeaderView *self,
				       TnyFolderType type)
{
	ModestHeaderViewPrivate *priv;
	ModestHeaderViewStyle style;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), 0);
	g_return_val_if_fail (type != TNY_FOLDER_TYPE_INVALID, 0);

	style = modest_header_view_get_style   (self);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	return priv->sort_colid[style][type];
}

GtkSortType
modest_header_view_get_sort_type (ModestHeaderView *self,
				  TnyFolderType type)
{
	ModestHeaderViewPrivate *priv;
	ModestHeaderViewStyle style;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), GTK_SORT_DESCENDING);
	g_return_val_if_fail (type != TNY_FOLDER_TYPE_INVALID, GTK_SORT_DESCENDING);

	style = modest_header_view_get_style   (self);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	return priv->sort_type[style][type];
}

typedef struct {
	ModestHeaderView *header_view;
	RefreshAsyncUserCallback cb;
	gpointer user_data;
} SetFolderHelper;

static void
folder_refreshed_cb (ModestMailOperation *mail_op,
		     TnyFolder *folder,
		     gpointer user_data)
{
	ModestHeaderViewPrivate *priv;
	SetFolderHelper *info;

	info = (SetFolderHelper*) user_data;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(info->header_view);

	/* User callback */
	if (info->cb)
		info->cb (mail_op, folder, info->user_data);

	/* Start the folder count changes observer. We do not need it
	   before the refresh. Note that the monitor could still be
	   called for this refresh but now we know that the callback
	   was previously called */
	g_mutex_lock (priv->observers_lock);
	tny_folder_add_observer (folder, TNY_FOLDER_OBSERVER (info->header_view));
	g_mutex_unlock (priv->observers_lock);

	/* Notify the observers that the update is over */
	g_signal_emit (G_OBJECT (info->header_view),
		       signals[UPDATING_MSG_LIST_SIGNAL], 0, FALSE, NULL);

	/* Allow filtering notifications from now on if the current
	   folder is still the same (if not then the user has selected
	   another one to refresh, we should wait until that refresh
	   finishes) */
	if (priv->folder == folder)
		priv->notify_status = TRUE;

	/* Frees */
	g_object_unref (info->header_view);
	g_free (info);
}

static void
refresh_folder_error_handler (ModestMailOperation *mail_op,
			      gpointer user_data)
{
	const GError *error = modest_mail_operation_get_error (mail_op);

	if (error->code == TNY_SYSTEM_ERROR_MEMORY ||
	    error->code == TNY_IO_ERROR_WRITE ||
	    error->code == TNY_IO_ERROR_READ) {
		ModestMailOperationStatus st = modest_mail_operation_get_status (mail_op);
		/* If the mail op has been cancelled then it's not an error: don't show any message */
		if (st != MODEST_MAIL_OPERATION_STATUS_CANCELED) {
			gchar *msg = g_strdup_printf (_KR("cerm_device_memory_full"), "");
			modest_platform_information_banner (NULL, NULL, msg);
			g_free (msg);
		}
	}
}

void
modest_header_view_set_folder (ModestHeaderView *self,
			       TnyFolder *folder,
			       gboolean refresh,
			       ModestWindow *progress_window,
			       RefreshAsyncUserCallback callback,
			       gpointer user_data)
{
	ModestHeaderViewPrivate *priv;

	g_return_if_fail (self);

	priv =     MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->folder) {
		if (priv->status_timeout) {
			g_source_remove (priv->status_timeout);
			priv->status_timeout = 0;
		}

		g_mutex_lock (priv->observers_lock);
		tny_folder_remove_observer (priv->folder, TNY_FOLDER_OBSERVER (self));
		g_object_unref (priv->folder);
		priv->folder = NULL;
		g_mutex_unlock (priv->observers_lock);
	}

	if (folder) {
		GtkTreeSelection *selection;
		SetFolderHelper *info;
		ModestMailOperation *mail_op = NULL;

		/* Set folder in the model */
		modest_header_view_set_folder_intern (self, folder);

		/* Pick my reference. Nothing to do with the mail operation */
		priv->folder = g_object_ref (folder);

		/* Do not notify about filterings until the refresh finishes */
		priv->notify_status = FALSE;

		/* Clear the selection if exists */
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
		gtk_tree_selection_unselect_all(selection);
		g_signal_emit (G_OBJECT(self), signals[HEADER_SELECTED_SIGNAL], 0, NULL);

		/* Notify the observers that the update begins */
		g_signal_emit (G_OBJECT (self), signals[UPDATING_MSG_LIST_SIGNAL],
			       0, TRUE, NULL);

		/* create the helper */
		info = g_malloc0 (sizeof (SetFolderHelper));
		info->header_view = g_object_ref (self);
		info->cb = callback;
		info->user_data = user_data;

		/* Create the mail operation (source will be the parent widget) */
		if (progress_window)
			mail_op = modest_mail_operation_new_with_error_handling (G_OBJECT(progress_window),
										 refresh_folder_error_handler,
										 NULL, NULL);
		if (refresh) {
			modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
							 mail_op);

			/* Refresh the folder asynchronously */
			modest_mail_operation_refresh_folder (mail_op,
							      folder,
							      folder_refreshed_cb,
							      info);
		} else {
			folder_refreshed_cb (mail_op, folder, info);
		}
		/* Free */
		if (mail_op)
			g_object_unref (mail_op);
	} else {
		g_mutex_lock (priv->observers_lock);

		if (priv->monitor) {
			tny_folder_monitor_stop (priv->monitor);
			g_object_unref (G_OBJECT (priv->monitor));
			priv->monitor = NULL;
		}

		if (priv->autoselect_reference) {
			gtk_tree_row_reference_free (priv->autoselect_reference);
			priv->autoselect_reference = NULL;
		}

		gtk_tree_view_set_model (GTK_TREE_VIEW (self), NULL);

		modest_header_view_notify_observers(self, NULL, NULL);

		g_mutex_unlock (priv->observers_lock);

		/* Notify the observers that the update is over */
		g_signal_emit (G_OBJECT (self), signals[UPDATING_MSG_LIST_SIGNAL],
			       0, FALSE, NULL);
	}
}

static void
on_header_row_activated (GtkTreeView *treeview, GtkTreePath *path,
			 GtkTreeViewColumn *column, gpointer userdata)
{
	ModestHeaderView *self = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model = NULL;
	TnyHeader *header = NULL;
	TnyHeaderFlags flags;

	self = MODEST_HEADER_VIEW (treeview);

	model = gtk_tree_view_get_model (treeview);
	if ((path == NULL) || (!gtk_tree_model_get_iter(model, &iter, path)))
		goto frees;

	/* get the first selected item */
	gtk_tree_model_get (model, &iter,
			    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &header,
			    -1);

	/* Dont open DELETED messages */
	if (flags & TNY_HEADER_FLAG_DELETED) {
		GtkWidget *win;
		gchar *msg;
		win = gtk_widget_get_ancestor (GTK_WIDGET (treeview), GTK_TYPE_WINDOW);
		msg = modest_ui_actions_get_msg_already_deleted_error_msg (MODEST_WINDOW (win));
		modest_platform_information_banner (NULL, NULL, msg);
		g_free (msg);
		goto frees;
	}

	/* Emit signal */
	g_signal_emit (G_OBJECT(self),
		       signals[HEADER_ACTIVATED_SIGNAL],
		       0, header, path);

	/* Free */
 frees:
	if (header != NULL)
		g_object_unref (G_OBJECT (header));

}

static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel *model;
	TnyHeader *header = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	ModestHeaderView *self;
	GList *selected = NULL;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);

	self = MODEST_HEADER_VIEW (user_data);

	selected = gtk_tree_selection_get_selected_rows (sel, &model);
	if (selected != NULL)
		path = (GtkTreePath *) selected->data;
	if ((path == NULL) || (!gtk_tree_model_get_iter(model, &iter, path)))
		return; /* msg was _un_selected */

	gtk_tree_model_get (model, &iter,
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);

	/* Emit signal */
	g_signal_emit (G_OBJECT(self),
		       signals[HEADER_SELECTED_SIGNAL],
		       0, header);

	g_object_unref (G_OBJECT (header));

	/* free all items in 'selected' */
	g_list_foreach (selected, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (selected);
}


/* PROTECTED method. It's useful when we want to force a given
   selection to reload a msg. For example if we have selected a header
   in offline mode, when Modest become online, we want to reload the
   message automatically without an user click over the header */
void
_modest_header_view_change_selection (GtkTreeSelection *selection,
				      gpointer user_data)
{
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (user_data && MODEST_IS_HEADER_VIEW (user_data));

	on_selection_changed (selection, user_data);
}

static gint
compare_priorities (TnyHeaderFlags p1, TnyHeaderFlags p2)
{
	/* HH, LL, NN */
	if (p1 == p2)
		return 0;

	/* HL HN */
	if (p1 == TNY_HEADER_FLAG_HIGH_PRIORITY)
		return 1;

	/* LH LN */
	if (p1 == TNY_HEADER_FLAG_LOW_PRIORITY)
		return -1;

	/* NH */
	if ((p1 == TNY_HEADER_FLAG_NORMAL_PRIORITY) && (p2 == TNY_HEADER_FLAG_HIGH_PRIORITY))
		return -1;

	/* NL */
	return 1;
}

static gint
cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
	  gpointer user_data)
{
	gint col_id;
	gint t1, t2;
	gint val1, val2;
	gint cmp;

	g_return_val_if_fail (GTK_IS_TREE_VIEW_COLUMN(user_data), 0);
	col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(user_data), MODEST_HEADER_VIEW_FLAG_SORT));


	switch (col_id) {
	case TNY_HEADER_FLAG_ATTACHMENTS:

		gtk_tree_model_get (tree_model, iter1, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1, -1);
		gtk_tree_model_get (tree_model, iter2, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2, -1);

		cmp = (val1 & TNY_HEADER_FLAG_ATTACHMENTS) -
			(val2 & TNY_HEADER_FLAG_ATTACHMENTS);

		return cmp ? cmp : t1 - t2;

	case TNY_HEADER_FLAG_PRIORITY_MASK: {
		TnyHeader *header1 = NULL, *header2 = NULL;

		gtk_tree_model_get (tree_model, iter1, TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &header1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &header2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,-1);

		/* This is for making priority values respect the intuitive sort relationship
		 * as HIGH is 01, LOW is 10, and NORMAL is 00 */

		if (header1 && header2) {
			cmp =  compare_priorities (tny_header_get_priority (header1),
				tny_header_get_priority (header2));
			g_object_unref (header1);
			g_object_unref (header2);

			return cmp ? cmp : t1 - t2;
		}

		return t1 - t2;
	}
	default:
		return &iter1 - &iter2; /* oughhhh  */
	}
}

static gint
cmp_subject_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
		  gpointer user_data)
{
	gint t1, t2;
	gchar *val1, *val2;
	gint cmp;
/* 	static int counter = 0; */

	g_return_val_if_fail (GTK_IS_TREE_VIEW_COLUMN(user_data), 0);

	gtk_tree_model_get (tree_model, iter1, TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &val1,
			    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1, -1);
	gtk_tree_model_get (tree_model, iter2, TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &val2,
			    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2, -1);

	cmp = modest_text_utils_utf8_strcmp (val1 + modest_text_utils_get_subject_prefix_len(val1),
					     val2 + modest_text_utils_get_subject_prefix_len(val2),
					     TRUE);
	g_free (val1);
	g_free (val2);
	return cmp;
}

/* Drag and drop stuff */
static void
drag_data_get_cb (GtkWidget *widget,
		  GdkDragContext *context,
		  GtkSelectionData *selection_data,
		  guint info,
		  guint time,
		  gpointer data)
{
	ModestHeaderView *self = NULL;
	ModestHeaderViewPrivate *priv = NULL;

	self = MODEST_HEADER_VIEW (widget);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	/* Set the data. Do not use the current selection because it
	   could be different than the selection at the beginning of
	   the d&d */
	modest_dnd_selection_data_set_paths (selection_data,
					     priv->drag_begin_cached_selected_rows);
}

/**
 * We're caching the selected rows at the beginning because the
 * selection could change between drag-begin and drag-data-get, for
 * example if we have a set of rows already selected, and then we
 * click in one of them (without SHIFT key pressed) and begin a drag,
 * the selection at that moment contains all the selected lines, but
 * after dropping the selection, the release event provokes that only
 * the row used to begin the drag is selected, so at the end the
 * drag&drop affects only one rows instead of all the selected ones.
 *
 */
static void
drag_begin_cb (GtkWidget *widget,
	       GdkDragContext *context,
	       gpointer data)
{
	ModestHeaderView *self = NULL;
	ModestHeaderViewPrivate *priv = NULL;
	GtkTreeSelection *selection;

	self = MODEST_HEADER_VIEW (widget);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	priv->drag_begin_cached_selected_rows =
		gtk_tree_selection_get_selected_rows (selection, NULL);
}

/**
 * We use the drag-end signal to clear the cached selection, we use
 * this because this allways happens, whether or not the d&d was a
 * success
 */
static void
drag_end_cb (GtkWidget *widget,
	     GdkDragContext *dc,
	     gpointer data)
{
	ModestHeaderView *self = NULL;
	ModestHeaderViewPrivate *priv = NULL;

	self = MODEST_HEADER_VIEW (widget);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	/* Free cached data */
	g_list_foreach (priv->drag_begin_cached_selected_rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (priv->drag_begin_cached_selected_rows);
	priv->drag_begin_cached_selected_rows = NULL;
}

/* Header view drag types */
const GtkTargetEntry header_view_drag_types[] = {
	{ GTK_TREE_PATH_AS_STRING_LIST, GTK_TARGET_SAME_APP, MODEST_HEADER_ROW }
};

static void
enable_drag_and_drop (GtkWidget *self)
{
#ifdef MODEST_TOOLKIT_HILDON2
	return;
#endif
	gtk_drag_source_set (self, GDK_BUTTON1_MASK,
			     header_view_drag_types,
			     G_N_ELEMENTS (header_view_drag_types),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);
}

static void
disable_drag_and_drop (GtkWidget *self)
{
#ifdef MODEST_TOOLKIT_HILDON2
	return;
#endif
	gtk_drag_source_unset (self);
}

static void
setup_drag_and_drop (GtkWidget *self)
{
#ifdef MODEST_TOOLKIT_HILDON2
	return;
#endif
	enable_drag_and_drop(self);
	g_signal_connect(G_OBJECT (self), "drag_data_get",
			 G_CALLBACK(drag_data_get_cb), NULL);

	g_signal_connect(G_OBJECT (self), "drag_begin",
			 G_CALLBACK(drag_begin_cb), NULL);

	g_signal_connect(G_OBJECT (self), "drag_end",
			 G_CALLBACK(drag_end_cb), NULL);
}

static GtkTreePath *
get_selected_row (GtkTreeView *self, GtkTreeModel **model)
{
	GtkTreePath *path = NULL;
	GtkTreeSelection *sel = NULL;
	GList *rows = NULL;

	sel   = gtk_tree_view_get_selection(self);
	rows = gtk_tree_selection_get_selected_rows (sel, model);

	if ((rows == NULL) || (g_list_length(rows) != 1))
		goto frees;

	path = gtk_tree_path_copy(g_list_nth_data (rows, 0));


	/* Free */
 frees:
	g_list_foreach(rows,(GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	return path;
}

#ifndef MODEST_TOOLKIT_HILDON2
/*
 * This function moves the tree view scroll to the current selected
 * row when the widget grabs the focus
 */
static gboolean
on_focus_in (GtkWidget     *self,
	     GdkEventFocus *event,
	     gpointer       user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *selected = NULL;
	GtkTreePath *selected_path = NULL;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (!model)
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	/* If none selected yet, pick the first one */
	if (gtk_tree_selection_count_selected_rows (selection) == 0) {
		GtkTreeIter iter;
		GtkTreePath *path;

		/* Return if the model is empty */
		if (!gtk_tree_model_get_iter_first (model, &iter))
			return FALSE;

		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}

	/* Need to get the all the rows because is selection multiple */
	selected = gtk_tree_selection_get_selected_rows (selection, &model);
	if (selected == NULL) return FALSE;
	selected_path = (GtkTreePath *) selected->data;

	/* Frees */
	g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected);

	return FALSE;
}

static gboolean
on_focus_out (GtkWidget     *self,
	     GdkEventFocus *event,
	     gpointer       user_data)
{

	if (!gtk_widget_is_focus (self)) {
		GtkTreeSelection *selection = NULL;
		GList *selected_rows = NULL;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
		if (gtk_tree_selection_count_selected_rows (selection) > 1) {
			selected_rows = gtk_tree_selection_get_selected_rows (selection, NULL);
			g_signal_handlers_block_by_func (selection, on_selection_changed, self);
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, (GtkTreePath *) selected_rows->data);
			g_signal_handlers_unblock_by_func (selection, on_selection_changed, self);
			g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (selected_rows);
		}
	}
	return FALSE;
}
#endif

static gboolean
on_button_release_event(GtkWidget * self, GdkEventButton * event, gpointer userdata)
{
	enable_drag_and_drop(self);
	return FALSE;
}

static gboolean
on_button_press_event(GtkWidget * self, GdkEventButton * event, gpointer userdata)
{
	GtkTreeSelection *selection = NULL;
	GtkTreePath *path = NULL;
	gboolean already_selected = FALSE, already_opened = FALSE;
	ModestTnySendQueueStatus status = MODEST_TNY_SEND_QUEUE_UNKNOWN;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(self), event->x, event->y, &path, NULL, NULL, NULL)) {
		GtkTreeIter iter;
		GtkTreeModel *model;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
		already_selected = gtk_tree_selection_path_is_selected (selection, path);

		/* Get header from model */
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
		if (gtk_tree_model_get_iter (model, &iter, path)) {
			GValue value = {0,};
			TnyHeader *header;

			gtk_tree_model_get_value (model, &iter,
						  TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
						  &value);
			header = (TnyHeader *) g_value_get_object (&value);
			if (TNY_IS_HEADER (header)) {
				status = modest_tny_all_send_queues_get_msg_status (header);
				already_opened = modest_window_mgr_find_registered_header (modest_runtime_get_window_mgr (),
											   header, NULL);
			}
			g_value_unset (&value);
		}
	}

	/* Enable drag and drop only if the user clicks on a row that
	   it's already selected. If not, let him select items using
	   the pointer. If the message is in an OUTBOX and in sending
	   status disable drag and drop as well */
	if (!already_selected ||
	    status == MODEST_TNY_SEND_QUEUE_SENDING ||
	    already_opened)
		disable_drag_and_drop(self);

	if (path != NULL)
		gtk_tree_path_free(path);

	/* If it's already opened then do not let the button-press
	   event go on because it'll perform a message open because
	   we're clicking on to an already selected header */
	return FALSE;
}

static void
folder_monitor_update (TnyFolderObserver *self,
		       TnyFolderChange *change)
{
	ModestHeaderViewPrivate *priv = NULL;
	TnyFolderChangeChanged changed;
	TnyFolder *folder = NULL;

	changed = tny_folder_change_get_changed (change);

	/* Do not notify the observers if the folder of the header
	   view has changed before this call to the observer
	   happens */
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (MODEST_HEADER_VIEW (self));
	folder = tny_folder_change_get_folder (change);
	if (folder != priv->folder)
		goto frees;

	MODEST_DEBUG_BLOCK (
			    if (changed & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS)
				    g_print ("ADDED %d/%d (r/t) \n",
					     tny_folder_change_get_new_unread_count (change),
					     tny_folder_change_get_new_all_count (change));
			    if (changed & TNY_FOLDER_CHANGE_CHANGED_ALL_COUNT)
				    g_print ("ALL COUNT %d\n",
					     tny_folder_change_get_new_all_count (change));
			    if (changed & TNY_FOLDER_CHANGE_CHANGED_UNREAD_COUNT)
				    g_print ("UNREAD COUNT %d\n",
					     tny_folder_change_get_new_unread_count (change));
			    if (changed & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS)
				    g_print ("EXPUNGED %d/%d (r/t) \n",
					     tny_folder_change_get_new_unread_count (change),
					     tny_folder_change_get_new_all_count (change));
			    if (changed & TNY_FOLDER_CHANGE_CHANGED_FOLDER_RENAME)
				    g_print ("FOLDER RENAME\n");
			    if (changed & TNY_FOLDER_CHANGE_CHANGED_MSG_RECEIVED)
				    g_print ("MSG RECEIVED %d/%d (r/t) \n",
					     tny_folder_change_get_new_unread_count (change),
					     tny_folder_change_get_new_all_count (change));
			    g_print ("---------------------------------------------------\n");
			    );

	/* Check folder count */
	if ((changed & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS) ||
	    (changed & TNY_FOLDER_CHANGE_CHANGED_EXPUNGED_HEADERS)) {

		g_mutex_lock (priv->observers_lock);

		/* Emit signal to evaluate how headers changes affects
		   to the window view  */
		g_signal_emit (G_OBJECT(self),
			       signals[MSG_COUNT_CHANGED_SIGNAL],
			       0, folder, change);

		/* Added or removed headers, so data stored on cliboard are invalid  */
		if (modest_email_clipboard_check_source_folder (priv->clipboard, folder))
			modest_email_clipboard_clear (priv->clipboard);

		g_mutex_unlock (priv->observers_lock);
	}

	/* Free */
 frees:
	if (folder != NULL)
		g_object_unref (folder);
}

gboolean
modest_header_view_is_empty (ModestHeaderView *self)
{
	ModestHeaderViewPrivate *priv;

	g_return_val_if_fail (self && MODEST_IS_HEADER_VIEW(self), TRUE);

	priv = MODEST_HEADER_VIEW_GET_PRIVATE (MODEST_HEADER_VIEW (self));

	return priv->status == HEADER_VIEW_EMPTY;
}

void
modest_header_view_clear (ModestHeaderView *self)
{
	g_return_if_fail (self && MODEST_IS_HEADER_VIEW(self));

	modest_header_view_set_folder (self, NULL, FALSE, NULL, NULL, NULL);
}

void
modest_header_view_copy_selection (ModestHeaderView *header_view)
{
	g_return_if_fail (header_view && MODEST_IS_HEADER_VIEW(header_view));

	/* Copy selection */
	_clipboard_set_selected_data (header_view, FALSE);
}

void
modest_header_view_cut_selection (ModestHeaderView *header_view)
{
	ModestHeaderViewPrivate *priv = NULL;
	const gchar **hidding = NULL;
	guint i, n_selected;

	g_return_if_fail (header_view && MODEST_IS_HEADER_VIEW (header_view));

	priv = MODEST_HEADER_VIEW_GET_PRIVATE (header_view);

	/* Copy selection */
	_clipboard_set_selected_data (header_view, TRUE);

	/* Get hidding ids */
	hidding = modest_email_clipboard_get_hidding_ids (priv->clipboard, &n_selected);

	/* Clear hidding array created by previous cut operation */
	_clear_hidding_filter (MODEST_HEADER_VIEW (header_view));

	/* Copy hidding array */
	priv->n_selected = n_selected;
	priv->hidding_ids = g_malloc0(sizeof(gchar *) * n_selected);
	for (i=0; i < n_selected; i++)
		priv->hidding_ids[i] = g_strdup(hidding[i]);

	/* Hide cut headers */
	modest_header_view_refilter (header_view);
}




static void
_clipboard_set_selected_data (ModestHeaderView *header_view,
			      gboolean delete)
{
	ModestHeaderViewPrivate *priv = NULL;
	TnyList *headers = NULL;

	g_return_if_fail (MODEST_IS_HEADER_VIEW (header_view));
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (header_view);

	/* Set selected data on clipboard   */
	g_return_if_fail (MODEST_IS_EMAIL_CLIPBOARD (priv->clipboard));
	headers = modest_header_view_get_selected_headers (header_view);
	modest_email_clipboard_set_data (priv->clipboard, priv->folder, headers, delete);

	/* Free */
	g_object_unref (headers);
}

typedef struct {
	ModestHeaderView *self;
	TnyFolder *folder;
} NotifyFilterInfo;

static gboolean
notify_filter_change (gpointer data)
{
	NotifyFilterInfo *info = (NotifyFilterInfo *) data;

	g_signal_emit (info->self,
		       signals[MSG_COUNT_CHANGED_SIGNAL],
		       0, info->folder, NULL);

	return FALSE;
}

static void
notify_filter_change_destroy (gpointer data)
{
	NotifyFilterInfo *info = (NotifyFilterInfo *) data;
	ModestHeaderViewPrivate *priv;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE (info->self);
	priv->status_timeout = 0;

	g_object_unref (info->self);
	g_object_unref (info->folder);
	g_slice_free (NotifyFilterInfo, info);
}

static gboolean
filter_row (GtkTreeModel *model,
	    GtkTreeIter *iter,
	    gpointer user_data)
{
	ModestHeaderViewPrivate *priv = NULL;
	TnyHeaderFlags flags;
	TnyHeader *header = NULL;
	guint i;
	gchar *id = NULL;
	gboolean visible = TRUE;
	gboolean found = FALSE;
	GValue value = {0,};
	HeaderViewStatus old_status;

	g_return_val_if_fail (MODEST_IS_HEADER_VIEW (user_data), FALSE);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (user_data);

	/* Get header from model */
	gtk_tree_model_get_value (model, iter, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &value);
	flags = (TnyHeaderFlags) g_value_get_int (&value);
	g_value_unset (&value);
	gtk_tree_model_get_value (model, iter, TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &value);
	header = (TnyHeader *) g_value_get_object (&value);
	g_value_unset (&value);

	/* Get message id from header (ensure is a valid id) */
	if (!header) {
		visible = FALSE;
		goto frees;
	}

	/* Hide deleted and mark as deleted heders */
	if (flags & TNY_HEADER_FLAG_DELETED ||
	    flags & TNY_HEADER_FLAG_EXPUNGED) {
		visible = FALSE;
		goto frees;
	}

	if (visible && (priv->filter & MODEST_HEADER_VIEW_FILTER_DELETABLE)) {
		if (priv->is_outbox &&
		    modest_tny_all_send_queues_get_msg_status (header) == MODEST_TNY_SEND_QUEUE_SENDING) {
			visible = FALSE;
			goto frees;
		}
	}

	if (visible && (priv->filter & MODEST_HEADER_VIEW_FILTER_MOVEABLE)) {
		if (priv->is_outbox &&
		    modest_tny_all_send_queues_get_msg_status (header) == MODEST_TNY_SEND_QUEUE_SENDING) {
			visible = FALSE;
			goto frees;
		}
	}

	/* If no data on clipboard, return always TRUE */
	if (modest_email_clipboard_cleared(priv->clipboard)) {
		visible = TRUE;
		goto frees;
	}

	/* Check hiding */
	if (priv->hidding_ids != NULL) {
		id = tny_header_dup_message_id (header);
		for (i=0; i < priv->n_selected && !found; i++)
			if (priv->hidding_ids[i] != NULL && id != NULL)
				found = (!strcmp (priv->hidding_ids[i], id));

		visible = !found;
		g_free(id);
	}

 frees:
	old_status = priv->status;
	priv->status = ((gboolean) priv->status) && !visible;
	if ((priv->notify_status) && (priv->status != old_status)) {
		if (priv->status_timeout)
			g_source_remove (priv->status_timeout);

		if (header) {
			NotifyFilterInfo *info;

			info = g_slice_new0 (NotifyFilterInfo);
			info->self = g_object_ref (G_OBJECT (user_data));
			if (header)
				info->folder = tny_header_get_folder (header);
			priv->status_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT, 1000,
								   notify_filter_change,
								   info,
								   notify_filter_change_destroy);
		}
	}

	return visible;
}

static void
_clear_hidding_filter (ModestHeaderView *header_view)
{
	ModestHeaderViewPrivate *priv = NULL;
	guint i;

	g_return_if_fail (MODEST_IS_HEADER_VIEW (header_view));
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(header_view);

	if (priv->hidding_ids != NULL) {
		for (i=0; i < priv->n_selected; i++)
			g_free (priv->hidding_ids[i]);
		g_free(priv->hidding_ids);
	}
}

void
modest_header_view_refilter (ModestHeaderView *header_view)
{
	GtkTreeModel *model = NULL;
	ModestHeaderViewPrivate *priv = NULL;

	g_return_if_fail (header_view && MODEST_IS_HEADER_VIEW (header_view));
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(header_view);

	/* Hide cut headers */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
	if (GTK_IS_TREE_MODEL_FILTER (model)) {
		priv->status = HEADER_VIEW_INIT;
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
	}
}

/*
 * Called when an account is removed. If I'm showing a folder of the
 * account that has been removed then clear the view
 */
static void
on_account_removed (TnyAccountStore *self,
		    TnyAccount *account,
		    gpointer user_data)
{
	ModestHeaderViewPrivate *priv = NULL;

	/* Ignore changes in transport accounts */
	if (TNY_IS_TRANSPORT_ACCOUNT (account))
		return;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE (user_data);

	if (priv->folder) {
		TnyAccount *my_account;

		if (TNY_IS_MERGE_FOLDER (priv->folder) &&
		    tny_folder_get_folder_type (priv->folder) == TNY_FOLDER_TYPE_OUTBOX) {
			ModestTnyAccountStore *acc_store = modest_runtime_get_account_store ();
			my_account = modest_tny_account_store_get_local_folders_account (acc_store);
		} else {
			my_account = tny_folder_get_account (priv->folder);
		}

		if (my_account) {
			if (my_account == account)
				modest_header_view_clear (MODEST_HEADER_VIEW (user_data));
			g_object_unref (my_account);
		}
	}
}

void
modest_header_view_add_observer(ModestHeaderView *header_view,
				     ModestHeaderViewObserver *observer)
{
	ModestHeaderViewPrivate *priv;

	g_return_if_fail (header_view && MODEST_IS_HEADER_VIEW(header_view));
	g_return_if_fail (observer && MODEST_IS_HEADER_VIEW_OBSERVER(observer));

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(header_view);

	g_mutex_lock(priv->observer_list_lock);
	priv->observer_list = g_slist_prepend(priv->observer_list, observer);
	g_mutex_unlock(priv->observer_list_lock);
}

void
modest_header_view_remove_observer(ModestHeaderView *header_view,
				   ModestHeaderViewObserver *observer)
{
	ModestHeaderViewPrivate *priv;

	g_return_if_fail (header_view && MODEST_IS_HEADER_VIEW(header_view));
	g_return_if_fail (observer && MODEST_IS_HEADER_VIEW_OBSERVER(observer));

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(header_view);

	g_mutex_lock(priv->observer_list_lock);
	priv->observer_list = g_slist_remove(priv->observer_list, observer);
	g_mutex_unlock(priv->observer_list_lock);
}

static void
modest_header_view_notify_observers(ModestHeaderView *header_view,
				    GtkTreeModel *model,
				    const gchar *tny_folder_id)
{
	ModestHeaderViewPrivate *priv = NULL;
	GSList *iter;
	ModestHeaderViewObserver *observer;


	g_return_if_fail (header_view && MODEST_IS_HEADER_VIEW(header_view));

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(header_view);

	g_mutex_lock(priv->observer_list_lock);
	iter = priv->observer_list;
	while(iter != NULL){
		observer = MODEST_HEADER_VIEW_OBSERVER(iter->data);
		modest_header_view_observer_update(observer, model,
				tny_folder_id);
		iter = g_slist_next(iter);
	}
	g_mutex_unlock(priv->observer_list_lock);
}

const gchar *
_modest_header_view_get_display_date (ModestHeaderView *self, time_t date)
{
	ModestHeaderViewPrivate *priv = NULL;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);
	return modest_datetime_formatter_display_datetime (priv->datetime_formatter, date);
}

void
modest_header_view_set_filter (ModestHeaderView *self,
			       ModestHeaderViewFilter filter)
{
	ModestHeaderViewPrivate *priv;
	GtkTreeModel *filter_model;

	g_return_if_fail (MODEST_IS_HEADER_VIEW (self));
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (self);

	priv->filter |= filter;

	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (GTK_IS_TREE_MODEL_FILTER(filter_model)) {
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));
	}
}

void
modest_header_view_unset_filter (ModestHeaderView *self,
				 ModestHeaderViewFilter filter)
{
	ModestHeaderViewPrivate *priv;
	GtkTreeModel *filter_model;

	g_return_if_fail (MODEST_IS_HEADER_VIEW (self));
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (self);

	priv->filter &= ~filter;

	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	if (GTK_IS_TREE_MODEL_FILTER(filter_model)) {
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));
	}
}

static void
on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata)
{
	if (strcmp ("style", spec->name) == 0) {
		update_style (MODEST_HEADER_VIEW (obj));
		gtk_widget_queue_draw (GTK_WIDGET (obj));
	}
}

static void
update_style (ModestHeaderView *self)
{
	ModestHeaderViewPrivate *priv;
	GdkColor style_color;
	GdkColor style_active_color;
	PangoAttrList *attr_list;
	GtkStyle *style;
	PangoAttribute *attr;

	g_return_if_fail (MODEST_IS_HEADER_VIEW (self));
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (self);

	/* Set color */

	attr_list = pango_attr_list_new ();
	if (!gtk_style_lookup_color (GTK_WIDGET (self)->style, "SecondaryTextColor", &style_color)) {
		gdk_color_parse ("grey", &style_color);
	}
	priv->secondary_color = style_color;
	attr = pango_attr_foreground_new (style_color.red, style_color.green, style_color.blue);
	pango_attr_list_insert (attr_list, attr);

	/* set font */
	style = gtk_rc_get_style_by_paths (gtk_widget_get_settings
					   (GTK_WIDGET(self)),
					   "SmallSystemFont", NULL,
					   G_TYPE_NONE);
	if (style) {
		attr = pango_attr_font_desc_new (pango_font_description_copy
						 (style->font_desc));
		pango_attr_list_insert (attr_list, attr);

		g_object_set (G_OBJECT (priv->renderer_address),
			      "foreground-gdk", priv->secondary_color,
			      "foreground-set", TRUE,
			      "attributes", attr_list,
			      NULL);
		g_object_set (G_OBJECT (priv->renderer_date_status),
			      "foreground-gdk", priv->secondary_color,
			      "foreground-set", TRUE,
			      "attributes", attr_list,
			      NULL);
		pango_attr_list_unref (attr_list);
	} else {
		g_object_set (G_OBJECT (priv->renderer_address),
			      "foreground-gdk", priv->secondary_color,
			      "foreground-set", TRUE,
			      "scale", PANGO_SCALE_SMALL,
			      "scale-set", TRUE,
			      NULL);
		g_object_set (G_OBJECT (priv->renderer_date_status),
			      "foreground-gdk", priv->secondary_color,
			      "foreground-set", TRUE,
			      "scale", PANGO_SCALE_SMALL,
			      "scale-set", TRUE,
			      NULL);
	}

	if (gtk_style_lookup_color (GTK_WIDGET (self)->style, "ActiveTextColor", &style_active_color)) {
		priv->active_color = style_active_color;
#ifdef MODEST_TOOLKIT_HILDON2
		g_object_set_data (G_OBJECT (priv->renderer_subject), BOLD_IS_ACTIVE_COLOR, GINT_TO_POINTER (TRUE));
		g_object_set_data_full (G_OBJECT (priv->renderer_subject), ACTIVE_COLOR, new_color, (GDestroyNotify) gdk_color_free);
#endif
	} else {
#ifdef MODEST_TOOLKIT_HILDON2
		g_object_set_data (G_OBJECT (priv->renderer_subject), BOLD_IS_ACTIVE_COLOR, GINT_TO_POINTER (FALSE));
#endif
	}
}

TnyHeader *
modest_header_view_get_header_at_pos (ModestHeaderView *header_view,
				      gint initial_x,
				      gint initial_y)
{
	GtkTreePath *path;
	GtkTreeModel *tree_model;
	GtkTreeIter iter;
	TnyHeader *header;

	/* Get tree path */
	if (!gtk_tree_view_get_dest_row_at_pos ((GtkTreeView *) header_view,
						initial_x,
						initial_y,
						&path,
						NULL))
		return NULL;

	g_debug ("located path: %s", gtk_tree_path_to_string (path));

	/* Get model */
	tree_model = gtk_tree_view_get_model ((GtkTreeView *) header_view);
	if (!gtk_tree_model_get_iter (tree_model, &iter, path))
		return NULL;

	/* Get header */
	gtk_tree_model_get (tree_model, &iter,
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);

	return header;
}

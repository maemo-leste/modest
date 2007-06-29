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
#include <string.h>

#include <modest-header-view.h>
#include <modest-header-view-priv.h>
#include <modest-dnd.h>
#include <modest-tny-folder.h>

#include <modest-main-window.h>

#include <modest-marshal.h>
#include <modest-text-utils.h>
#include <modest-icon-names.h>
#include <modest-runtime.h>
#include "modest-platform.h"
#include <modest-hbox-cell-renderer.h>
#include <modest-vbox-cell-renderer.h>

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

static void          on_selection_changed   (GtkTreeSelection *sel, 
					     gpointer user_data);

static void          setup_drag_and_drop    (GtkTreeView *self);

static GtkTreePath * get_selected_row       (GtkTreeView *self, GtkTreeModel **model);

static gboolean      on_focus_in            (GtkWidget     *sef,
					     GdkEventFocus *event,
					     gpointer       user_data);

static void          folder_monitor_update  (TnyFolderObserver *self, 
					     TnyFolderChange *change);

static void          tny_folder_observer_init (TnyFolderObserverIface *klass);

static void          _clipboard_set_selected_data (ModestHeaderView *header_view, gboolean delete);

static void          _clear_hidding_filter (ModestHeaderView *header_view);


typedef struct _ModestHeaderViewPrivate ModestHeaderViewPrivate;
struct _ModestHeaderViewPrivate {
	TnyFolder            *folder;
	ModestHeaderViewStyle style;

	TnyFolderMonitor     *monitor;
	GMutex               *observers_lock;

	/* not unref this object, its a singlenton */
	ModestEmailClipboard *clipboard;

	/* Filter tree model */
	gchar **hidding_ids;
	guint n_selected;

	gint                  sort_colid[2][TNY_FOLDER_TYPE_NUM];
	gint                  sort_type[2][TNY_FOLDER_TYPE_NUM];


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
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	
	
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
}

static void
tny_folder_observer_init (TnyFolderObserverIface *klass)
{
	klass->update_func = folder_monitor_update;
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
	GtkTreeModel *tree_filter, *sortable;
	GtkTreeViewColumn *column=NULL;
	GtkTreeSelection *selection = NULL;
	GtkCellRenderer *renderer_msgtype,*renderer_header,
		*renderer_attach, *renderer_compact_date_or_status;
	GtkCellRenderer *renderer_compact_header, *renderer_recpt_box, 
		*renderer_subject, *renderer_subject_box, *renderer_recpt,
		*renderer_priority;
	ModestHeaderViewPrivate *priv;
	GtkTreeViewColumn *compact_column = NULL;
	const GList *cursor;
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self); 

	/* FIXME: check whether these renderers need to be freed */
	renderer_msgtype = gtk_cell_renderer_pixbuf_new ();
	renderer_attach  = gtk_cell_renderer_pixbuf_new ();
	renderer_priority  = gtk_cell_renderer_pixbuf_new ();
	renderer_header  = gtk_cell_renderer_text_new ();

	renderer_compact_header = modest_vbox_cell_renderer_new ();
	renderer_recpt_box = modest_hbox_cell_renderer_new ();
	renderer_subject_box = modest_hbox_cell_renderer_new ();
	renderer_recpt = gtk_cell_renderer_text_new ();
	renderer_subject = gtk_cell_renderer_text_new ();
	renderer_compact_date_or_status  = gtk_cell_renderer_text_new ();

	modest_vbox_cell_renderer_append (MODEST_VBOX_CELL_RENDERER (renderer_compact_header), renderer_subject_box, FALSE);
	g_object_set_data (G_OBJECT (renderer_compact_header), "subject-box-renderer", renderer_subject_box);
	modest_vbox_cell_renderer_append (MODEST_VBOX_CELL_RENDERER (renderer_compact_header), renderer_recpt_box, FALSE);
	g_object_set_data (G_OBJECT (renderer_compact_header), "recpt-box-renderer", renderer_recpt_box);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_subject_box), renderer_priority, FALSE);
	g_object_set_data (G_OBJECT (renderer_subject_box), "priority-renderer", renderer_priority);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_subject_box), renderer_subject, TRUE);
	g_object_set_data (G_OBJECT (renderer_subject_box), "subject-renderer", renderer_subject);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_recpt_box), renderer_attach, FALSE);
	g_object_set_data (G_OBJECT (renderer_recpt_box), "attach-renderer", renderer_attach);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_recpt_box), renderer_recpt, TRUE);
	g_object_set_data (G_OBJECT (renderer_recpt_box), "recipient-renderer", renderer_recpt);
	modest_hbox_cell_renderer_append (MODEST_HBOX_CELL_RENDERER (renderer_recpt_box), renderer_compact_date_or_status, FALSE);
	g_object_set_data (G_OBJECT (renderer_recpt_box), "date-renderer", renderer_compact_date_or_status);

	g_object_set(G_OBJECT(renderer_header),
		     "ellipsize", PANGO_ELLIPSIZE_END,
		     NULL);
	g_object_set (G_OBJECT (renderer_subject),
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);
	g_object_set (G_OBJECT (renderer_recpt),
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);
	g_object_set(G_OBJECT(renderer_compact_date_or_status),
		     "xalign", 1.0,
		     NULL);

	gtk_cell_renderer_set_fixed_size (renderer_attach, 32, 32);
	gtk_cell_renderer_set_fixed_size (renderer_priority, 32, 32);
	
	remove_all_columns (self);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
/* 	sortable = gtk_tree_view_get_model (GTK_TREE_VIEW (self)); */
	tree_filter = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	sortable = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER(tree_filter));

	/* Add new columns */
	for (cursor = columns; cursor; cursor = g_list_next(cursor)) {
		ModestHeaderViewColumn col =
			(ModestHeaderViewColumn) GPOINTER_TO_INT(cursor->data);
		
		if (0> col || col >= MODEST_HEADER_VIEW_COLUMN_NUM) {
			g_printerr ("modest: invalid column %d in column list\n", col);
			continue;
		}
		
		switch (col) {
			
		case MODEST_HEADER_VIEW_COLUMN_MSGTYPE:
			column = get_new_column (_("M"), renderer_msgtype, FALSE,
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_msgtype_cell_data,
						 NULL);
			gtk_tree_view_column_set_fixed_width (column, 45);
			break;

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
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable),
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_rows,
						 compact_column, NULL);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortable),
						 TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_subject_rows,
						 compact_column, NULL);
	}


	return TRUE;
}

static void
modest_header_view_init (ModestHeaderView *obj)
{
	ModestHeaderViewPrivate *priv;
	guint i, j;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(obj); 

	priv->folder  = NULL;

	priv->monitor	     = NULL;
	priv->observers_lock = g_mutex_new ();

	priv->clipboard = modest_runtime_get_email_clipboard ();
	priv->hidding_ids = NULL;
	priv->n_selected = 0;

	/* Sort parameters */
	for (j=0; j < 2; j++) {
		for (i=0; i < TNY_FOLDER_TYPE_NUM; i++) {
			priv->sort_colid[j][i] = -1;
			priv->sort_type[j][i] = GTK_SORT_DESCENDING;
		}			
	}

	setup_drag_and_drop (GTK_TREE_VIEW (obj));
}

static void
modest_header_view_dispose (GObject *obj)
{
	ModestHeaderView        *self;
	ModestHeaderViewPrivate *priv;
	
	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->folder) {
		tny_folder_remove_observer (priv->folder, TNY_FOLDER_OBSERVER (obj));
		g_object_unref (G_OBJECT (priv->folder));
		priv->folder = NULL;
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

	g_mutex_lock (priv->observers_lock);
	if (priv->monitor) {
		tny_folder_monitor_stop (priv->monitor);
		g_object_unref (G_OBJECT (priv->monitor));
	}
	g_mutex_unlock (priv->observers_lock);
	g_mutex_free (priv->observers_lock);

	/* Clear hidding array created by cut operation */
	_clear_hidding_filter (MODEST_HEADER_VIEW (obj));

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
/* 	modest_header_view_set_folder (self, NULL, NULL, NULL); */

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(obj));
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(obj),TRUE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(obj), TRUE);
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(obj),
				      TRUE); /* alternating row colors */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	
	g_signal_connect (sel, "changed",
			  G_CALLBACK(on_selection_changed), self);
	
	g_signal_connect (self, "row-activated",
			  G_CALLBACK (on_header_row_activated), NULL);

	g_signal_connect (self, "focus-in-event",
			  G_CALLBACK(on_focus_in), NULL);
	
	return GTK_WIDGET(self);
}


guint
modest_header_view_count_selected_headers (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	guint selected_rows;

	g_return_val_if_fail (self, 0);
	
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

	g_return_val_if_fail (self, FALSE);
	
	/* Get selection object and check selected rows count */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	empty = gtk_tree_selection_count_selected_rows (sel) == 0;
	
	return !empty;
}


TnyList * 
modest_header_view_get_selected_headers (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	ModestHeaderViewPrivate *priv;
	TnyList *header_list = NULL;
	TnyHeader *header;
	GList *list, *tmp = NULL;
	GtkTreeModel *tree_model = NULL;
	GtkTreeIter iter;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

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
#ifdef MODEST_PLATFORM_GNOME 

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

#endif /* MODEST_PLATFORM_GNOME */
}


void 
modest_header_view_select_next (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;

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
 	g_return_val_if_fail (self, FALSE);
	return gtk_tree_view_get_columns (GTK_TREE_VIEW(self)); 
}


gboolean
modest_header_view_is_empty (ModestHeaderView *self)
{
	g_return_val_if_fail (self, FALSE);
	return FALSE; /* FIXME */
}


gboolean
modest_header_view_set_style (ModestHeaderView *self,
			      ModestHeaderViewStyle style)
{
	ModestHeaderViewPrivate *priv;
	gboolean show_col_headers = FALSE;
	ModestHeaderViewStyle old_style;
	
	g_return_val_if_fail (self, FALSE);
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
	g_return_val_if_fail (self, FALSE);
	return MODEST_HEADER_VIEW_GET_PRIVATE(self)->style;
}

/* 
 * This function sets a sortable model in the header view. It's just
 * used for developing purposes, because it only does a
 * gtk_tree_view_set_model
 */
static void
modest_header_view_set_model (GtkTreeView *header_view, GtkTreeModel *model)
{
/* 	GtkTreeModel *old_model_sort = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view)); */
/* 	if (old_model_sort && GTK_IS_TREE_MODEL_SORT (old_model_sort)) { */
/* 		GtkTreeModel *old_model; */
/* 		ModestHeaderViewPrivate *priv; */
/* 		priv = MODEST_HEADER_VIEW_GET_PRIVATE (header_view); */
/* 		old_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (old_model_sort)); */

/* 		/\* Set new model *\/ */
/* 		gtk_tree_view_set_model (header_view, model); */
/* 	} else */
	gtk_tree_view_set_model (header_view, model);
}

TnyFolder*
modest_header_view_get_folder (ModestHeaderView *self)
{
	ModestHeaderViewPrivate *priv;
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->folder)
		g_object_ref (priv->folder);

	return priv->folder;
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

	tny_gtk_header_list_model_set_folder (TNY_GTK_HEADER_LIST_MODEL(headers),
					      folder, FALSE);

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

	sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(headers));
	g_object_unref (G_OBJECT (headers));

	/* Create a tree model filter to hide and show rows for cut operations  */
	filter_model = gtk_tree_model_filter_new (sortable, NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
						filter_row,
						self,
						NULL);
	g_object_unref (G_OBJECT (sortable));

	/* install our special sorting functions */
	cursor = cols = gtk_tree_view_get_columns (GTK_TREE_VIEW(self));

	/* Restore sort column id */
	if (cols) {
		type  = modest_tny_folder_guess_folder_type (folder);
		sort_colid = modest_header_view_get_sort_column_id (self, type); 
		sort_type = modest_header_view_get_sort_type (self, type); 
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(sortable),
						      sort_colid,
						      sort_type);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable),
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_rows,
						 cols->data, NULL);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable),
						 TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN,
						 (GtkTreeIterCompareFunc) cmp_subject_rows,
						 cols->data, NULL);
	}

	/* Set new model */
	modest_header_view_set_model (GTK_TREE_VIEW (self), filter_model);
	g_object_unref (G_OBJECT (filter_model));
/* 	modest_header_view_set_model (GTK_TREE_VIEW (self), sortable); */
/* 	g_object_unref (G_OBJECT (sortable)); */

	/* Free */
	g_list_free (cols);
}

void
modest_header_view_sort_by_column_id (ModestHeaderView *self, 
				      guint sort_colid,
				      GtkSortType sort_type)
{
	ModestHeaderViewPrivate *priv = NULL;
	GtkTreeModel *tree_filter, *sortable = NULL; 
	TnyFolderType type;

	/* Get model and private data */
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);		
	tree_filter = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	sortable = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER(tree_filter));
/* 	sortable = gtk_tree_view_get_model (GTK_TREE_VIEW (self)); */
	
	/* Sort tree model */
	type  = modest_tny_folder_guess_folder_type (priv->folder);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(sortable),
					      sort_colid,
					      sort_type);
	/* Store new sort parameters */
	modest_header_view_set_sort_params (self, sort_colid, sort_type, type);

	/* Save GConf parameters */
/* 	modest_widget_memory_save (modest_runtime_get_conf(), */
/* 				   G_OBJECT(self), "header-view"); */
	
}

void
modest_header_view_set_sort_params (ModestHeaderView *self, 
				    guint sort_colid, 
				    GtkSortType sort_type,
				    TnyFolderType type)
{
	ModestHeaderViewPrivate *priv;
	ModestHeaderViewStyle style;

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
folder_refreshed_cb (const GObject *obj, 
		     TnyFolder *folder, 
		     gpointer user_data)
{
	ModestHeaderViewPrivate *priv;
	SetFolderHelper *info;
 
	info = (SetFolderHelper*) user_data;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(info->header_view);

	/* User callback */
	if (info->cb)
		info->cb (obj, folder, info->user_data);

	/* Start the folder count changes observer. We do not need it
	   before the refresh. Note that the monitor could still be
	   called for this refresh but now we know that the callback
	   was previously called */
	g_mutex_lock (priv->observers_lock);
	tny_folder_add_observer (folder, TNY_FOLDER_OBSERVER (info->header_view));
	g_mutex_unlock (priv->observers_lock);

	/* Frees */
	g_free (info);
}

void
modest_header_view_set_folder (ModestHeaderView *self, 
			       TnyFolder *folder,
			       RefreshAsyncUserCallback callback,
			       gpointer user_data)
{
	ModestHeaderViewPrivate *priv;
	ModestWindowMgr *mgr = NULL;
	GObject *source = NULL;
	SetFolderHelper *info;
 
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->folder) {
		g_mutex_lock (priv->observers_lock);
		tny_folder_remove_observer (priv->folder, TNY_FOLDER_OBSERVER (self));
		g_object_unref (priv->folder);
		priv->folder = NULL;
		g_mutex_unlock (priv->observers_lock);
	}

	if (folder) {
		ModestMailOperation *mail_op = NULL;

		/* Get main window to use it as source of mail operation */
		mgr = modest_runtime_get_window_mgr ();
		source = G_OBJECT (modest_window_mgr_get_main_window (modest_runtime_get_window_mgr ()));

		/* Set folder in the model */
		modest_header_view_set_folder_intern (self, folder);

		/* Pick my reference. Nothing to do with the mail operation */
		priv->folder = g_object_ref (folder);

		/* no message selected */
		g_signal_emit (G_OBJECT(self), signals[HEADER_SELECTED_SIGNAL], 0, NULL);

		info = g_malloc0 (sizeof(SetFolderHelper));
		info->header_view = self;
		info->cb = callback;
		info->user_data = user_data;

		/* Create the mail operation (source will be the parent widget) */
		mail_op = modest_mail_operation_new (MODEST_MAIL_OPERATION_TYPE_RECEIVE, source);
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
						 mail_op);

		/* Refresh the folder asynchronously */
		modest_mail_operation_refresh_folder (mail_op, 
						      folder,
						      folder_refreshed_cb,
						      info);

		/* Free */
		g_object_unref (mail_op);
	} else {
		g_mutex_lock (priv->observers_lock);

		if (priv->monitor) {
			tny_folder_monitor_stop (priv->monitor);
			g_object_unref (G_OBJECT (priv->monitor));
			priv->monitor = NULL;
		}
		modest_header_view_set_model (GTK_TREE_VIEW (self), NULL); 

		g_mutex_unlock (priv->observers_lock);
	}
}

static void
on_header_row_activated (GtkTreeView *treeview, GtkTreePath *path,
			 GtkTreeViewColumn *column, gpointer userdata)
{
	ModestHeaderView *self = NULL;
	ModestHeaderViewPrivate *priv = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model = NULL;
	TnyHeader *header = NULL;
	TnyHeaderFlags flags;

	self = MODEST_HEADER_VIEW (treeview);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

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
		modest_platform_information_banner (NULL, NULL, _("mcen_ib_message_already_deleted"));
		goto frees;
	}

	/* Emit signal */
	g_signal_emit (G_OBJECT(self), 
		       signals[HEADER_ACTIVATED_SIGNAL], 
		       0, header);

	/* Free */
 frees:
	if (header != NULL) 
		g_object_unref (G_OBJECT (header));	

}

static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel *model;
	TnyHeader *header;
	GtkTreePath *path = NULL;	
	GtkTreeIter iter;
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;
	GList *selected = NULL;
	
	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	self = MODEST_HEADER_VIEW (user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);	

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
	g_return_if_fail (MODEST_IS_HEADER_VIEW (user_data));

	on_selection_changed (selection, user_data);
}

static gint compare_priorities (TnyHeaderFlags p1, TnyHeaderFlags p2)
{
	p1 = p1 & TNY_HEADER_FLAG_PRIORITY;
	p2 = p2 & TNY_HEADER_FLAG_PRIORITY;
	if (p1 == 0) 
		p1 = TNY_HEADER_FLAG_LOW_PRIORITY + 1;
	if (p2 == 0) 
		p2 = TNY_HEADER_FLAG_LOW_PRIORITY + 1;
	return p1 - p2;
}

static gint
cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
	  gpointer user_data)
{
	gint col_id;
	gint t1, t2;
	gint val1, val2;
	gint cmp;
/* 	static int counter = 0; */

	g_return_val_if_fail (GTK_IS_TREE_VIEW_COLUMN(user_data), 0);
/* 	col_id = gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (tree_model)); */
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
		
	case TNY_HEADER_FLAG_PRIORITY:
		gtk_tree_model_get (tree_model, iter1, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,-1);

		/* This is for making priority values respect the intuitive sort relationship 
		 * as HIGH is 11, LOW is 01, and we put NORMAL AS 10 (2) */
		cmp =  compare_priorities (val1, val2);

		return cmp ? cmp : t1 - t2;

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
drag_data_get_cb (GtkWidget *widget, GdkDragContext *context, 
		  GtkSelectionData *selection_data, 
		  guint info,  guint time, gpointer data)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	GtkTreePath *source_row = NULL;
	
	source_row = get_selected_row (GTK_TREE_VIEW (widget), &model);
	if ((source_row == NULL) || (!gtk_tree_model_get_iter(model, &iter, source_row))) return;

	switch (info) {
	case MODEST_HEADER_ROW:
		gtk_tree_set_row_drag_data (selection_data, model, source_row);
		break;
	case MODEST_MSG: {
		TnyHeader *hdr;
		gtk_tree_model_get (model, &iter,
				    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &hdr,
				    -1);
		if (hdr) {
			g_object_unref (G_OBJECT(hdr));
		}
		break;
	}
	default:
		g_message ("%s: default switch case.", __FUNCTION__);
	}

	gtk_tree_path_free (source_row);
}

/* Header view drag types */
const GtkTargetEntry header_view_drag_types[] = {
	{ "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_APP, MODEST_HEADER_ROW },
	{ "text/uri-list",      0,                   MODEST_MSG }, 
};

static void
setup_drag_and_drop (GtkTreeView *self)
{
	gtk_drag_source_set (GTK_WIDGET (self),
			     GDK_BUTTON1_MASK,
			     header_view_drag_types,
			     G_N_ELEMENTS (header_view_drag_types),
			     GDK_ACTION_MOVE | GDK_ACTION_COPY);

	g_signal_connect(G_OBJECT (self), "drag_data_get",
			 G_CALLBACK(drag_data_get_cb), NULL);
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

	/* Check if we need to scroll */
	#if GTK_CHECK_VERSION(2, 8, 0) /* TODO: gtk_tree_view_get_visible_range() is only available in GTK+ 2.8 */
	GtkTreePath *start_path = NULL;
	GtkTreePath *end_path = NULL;
	if (gtk_tree_view_get_visible_range (GTK_TREE_VIEW (self),
					     &start_path,
					     &end_path)) {

		if ((gtk_tree_path_compare (start_path, selected_path) != -1) ||
		    (gtk_tree_path_compare (end_path, selected_path) != 1)) {

			/* Scroll to first path */
			gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (self),
						      selected_path,
						      NULL,
						      TRUE,
						      0.5,
						      0.0);
		}
	}
	#endif /* GTK_CHECK_VERSION */

	/* Frees */	
	g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (selected);

	return FALSE;
}

static void
idle_notify_headers_count_changed_destroy (gpointer data)
{
	HeadersCountChangedHelper *helper = NULL;

	g_return_if_fail (data != NULL);
	helper = (HeadersCountChangedHelper *) data; 

	g_object_unref (helper->change);
	g_slice_free (HeadersCountChangedHelper, helper);
}

static gboolean
idle_notify_headers_count_changed (gpointer data)
{
	TnyFolder *folder = NULL;
	ModestHeaderViewPrivate *priv = NULL;
	HeadersCountChangedHelper *helper = NULL;

	g_return_val_if_fail (data != NULL, FALSE);
	helper = (HeadersCountChangedHelper *) data; 
	g_return_val_if_fail (MODEST_IS_HEADER_VIEW(helper->self), FALSE);
	g_return_val_if_fail (TNY_FOLDER_CHANGE(helper->change), FALSE);

	folder = tny_folder_change_get_folder (helper->change);

	priv = MODEST_HEADER_VIEW_GET_PRIVATE (helper->self);

	g_mutex_lock (priv->observers_lock);

	/* Emit signal to evaluate how headers changes affects to the window view  */
	gdk_threads_enter ();
	g_signal_emit (G_OBJECT(helper->self), 
		       signals[MSG_COUNT_CHANGED_SIGNAL], 
		       0, folder, helper->change);
	gdk_threads_leave ();
		
	/* Added or removed headers, so data stored on cliboard are invalid  */
	if (modest_email_clipboard_check_source_folder (priv->clipboard, folder))
	    modest_email_clipboard_clear (priv->clipboard);
	    
	g_mutex_unlock (priv->observers_lock);

	return FALSE;
}

static void
folder_monitor_update (TnyFolderObserver *self, 
		       TnyFolderChange *change)
{
	ModestHeaderViewPrivate *priv;
	TnyFolderChangeChanged changed;
	HeadersCountChangedHelper *helper = NULL;

	changed = tny_folder_change_get_changed (change);
	
	/* Do not notify the observers if the folder of the header
	   view has changed before this call to the observer
	   happens */
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (MODEST_HEADER_VIEW (self));
	if (tny_folder_change_get_folder (change) != priv->folder)
		return;

	/* Check folder count */
	if ((changed & TNY_FOLDER_CHANGE_CHANGED_ADDED_HEADERS) ||
	    (changed & TNY_FOLDER_CHANGE_CHANGED_REMOVED_HEADERS)) {
		helper = g_slice_new0 (HeadersCountChangedHelper);
		helper->self = MODEST_HEADER_VIEW(self);
		helper->change = g_object_ref(change);
		
		g_idle_add_full (G_PRIORITY_DEFAULT, 
				 idle_notify_headers_count_changed, 
				 helper,
				 idle_notify_headers_count_changed_destroy);
	}	
}

void
modest_header_view_clear (ModestHeaderView *self)
{
	modest_header_view_set_folder (self, NULL, NULL, NULL);
}

void 
modest_header_view_copy_selection (ModestHeaderView *header_view)
{
	/* Copy selection */
	_clipboard_set_selected_data (header_view, FALSE);
}

void 
modest_header_view_cut_selection (ModestHeaderView *header_view)
{
	ModestHeaderViewPrivate *priv = NULL;
	GtkTreeModel *model = NULL;
	const gchar **hidding = NULL;
	guint i, n_selected;

	g_return_if_fail (MODEST_IS_HEADER_VIEW (header_view));
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
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
	if (GTK_IS_TREE_MODEL_FILTER (model))
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
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
	
	g_return_val_if_fail (MODEST_IS_HEADER_VIEW (user_data), FALSE);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE (user_data);

	/* Get header from model */
	gtk_tree_model_get (model, iter,
			    TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN, &header,
			    -1);
	
	/* Hide mark as deleted heders */
	if (flags & TNY_HEADER_FLAG_DELETED) {
		visible = FALSE;
		goto frees;
	}

	/* If no data on clipboard, return always TRUE */
	if (modest_email_clipboard_cleared(priv->clipboard)) {
		visible = TRUE;
		goto frees;
	}	    	

	/* Get message id from header (ensure is a valid id) */
	if (!header) return FALSE;
	id = g_strdup(tny_header_get_message_id (header));
	
	/* Check hiding */
	if (priv->hidding_ids != NULL) {
		for (i=0; i < priv->n_selected && !found; i++)
			if (priv->hidding_ids[i] != NULL && id != NULL)
				found = (!strcmp (priv->hidding_ids[i], id));
	
		visible = !found;
	}

	/* Free */
 frees:
	if (header)
		g_object_unref (header);
	g_free(id);

	return visible;
}

static void
_clear_hidding_filter (ModestHeaderView *header_view) 
{
	ModestHeaderViewPrivate *priv;
	guint i;
	
	g_return_if_fail (MODEST_IS_HEADER_VIEW (header_view));	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(header_view);

	if (priv->hidding_ids != NULL) {
		for (i=0; i < priv->n_selected; i++) 
			g_free (priv->hidding_ids[i]);
		g_free(priv->hidding_ids);
	}	
}

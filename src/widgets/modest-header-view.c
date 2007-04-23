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

#include <modest-marshal.h>
#include <modest-text-utils.h>
#include <modest-icon-names.h>
#include <modest-runtime.h>

static void modest_header_view_class_init  (ModestHeaderViewClass *klass);
static void modest_header_view_init        (ModestHeaderView *obj);
static void modest_header_view_finalize    (GObject *obj);

static gboolean      on_header_clicked      (GtkWidget *widget, 
					    GdkEventButton *event, 
					    gpointer user_data);

static gint          cmp_rows               (GtkTreeModel *tree_model, 
					     GtkTreeIter *iter1, 
					     GtkTreeIter *iter2,
					     gpointer user_data);

static void          on_selection_changed   (GtkTreeSelection *sel, 
					     gpointer user_data);

static void          setup_drag_and_drop    (GtkTreeView *self);

static GtkTreePath * get_selected_row       (GtkTreeView *self, GtkTreeModel **model);

static gboolean      on_focus_in            (GtkWidget     *sef,
					     GdkEventFocus *event,
					     gpointer       user_data);

typedef struct _ModestHeaderViewPrivate ModestHeaderViewPrivate;
struct _ModestHeaderViewPrivate {
	TnyFolder            *folder;
	ModestHeaderViewStyle style;

	TnyFolderMonitor     *monitor;
	GMutex               *monitor_lock;
};

#define MODEST_HEADER_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
						MODEST_TYPE_HEADER_VIEW, \
                                                ModestHeaderViewPrivate))



#define MODEST_HEADER_VIEW_PTR "modest-header-view"

enum {
	HEADER_SELECTED_SIGNAL,
	HEADER_ACTIVATED_SIGNAL,
	ITEM_NOT_FOUND_SIGNAL,
	STATUS_UPDATE_SIGNAL,
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
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestHeaderView",
		                                  &my_info, 0);
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

	signals[STATUS_UPDATE_SIGNAL] =
		g_signal_new ("status_update",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestHeaderViewClass,status_update),
			      NULL, NULL,
			      modest_marshal_VOID__STRING_INT_INT,
			      G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
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
modest_header_view_set_columns (ModestHeaderView *self, const GList *columns)
{
	GtkTreeModel *sortable;
	GtkTreeViewColumn *column=NULL;
	GtkTreeSelection *selection = NULL;
	GtkCellRenderer *renderer_msgtype,*renderer_header,
		*renderer_attach,  *renderer_comptact_flag, 
		*renderer_compact_date;
	ModestHeaderViewPrivate *priv;
	const GList *cursor;
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self); 

	/* FIXME: check whether these renderers need to be freed */
	renderer_msgtype = gtk_cell_renderer_pixbuf_new ();
	renderer_attach  = gtk_cell_renderer_pixbuf_new ();
	renderer_header  = gtk_cell_renderer_text_new ();
	renderer_comptact_flag  = gtk_cell_renderer_pixbuf_new ();
	renderer_compact_date  = gtk_cell_renderer_text_new ();

	g_object_set(G_OBJECT(renderer_header),
		     "ellipsize", PANGO_ELLIPSIZE_END,
		     NULL);
	g_object_set(G_OBJECT(renderer_compact_date),
		     "xalign", 1.0,
		     NULL);
	
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
			
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_FLAG:
			column = get_new_column (_("F"), renderer_comptact_flag, FALSE,
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_compact_flag_cell_data,
						 NULL);
			gtk_tree_view_column_set_fixed_width (column, 45);
			break;

		case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN:
			column = get_new_column (_("Header"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_compact_header_cell_data,
						 GINT_TO_POINTER(TRUE));
			break;

		case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT:
			column = get_new_column (_("Header"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_compact_header_cell_data,
						 GINT_TO_POINTER(FALSE));
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
			
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_RECEIVED_DATE:
			column = get_new_column (_("Received"), renderer_compact_date, FALSE,
						 TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_compact_date_cell_data,
						 GINT_TO_POINTER(TRUE));
			gtk_tree_view_column_set_fixed_width (column, 130);
			break;
			
		case MODEST_HEADER_VIEW_COLUMN_COMPACT_SENT_DATE:					      
			column = get_new_column (_("Sent"), renderer_compact_date, FALSE,
						 TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_compact_date_cell_data,
						 GINT_TO_POINTER(FALSE));
			gtk_tree_view_column_set_fixed_width (column, 130);
			break;
		case MODEST_HEADER_VIEW_COLUMN_SIZE:
			column = get_new_column (_("Size"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_size_cell_data,
						 NULL); 
			break;
		case MODEST_HEADER_VIEW_COLUMN_STATUS:
			column = get_new_column (_("Status"), renderer_compact_date, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_status_cell_data,
						 NULL); 
			break;

		default:
			g_return_val_if_reached(FALSE);
		}

		if (sortable)
			gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable),
							 col, (GtkTreeIterCompareFunc)cmp_rows,
							 column, NULL);
		
		/* we keep the column id around */
		g_object_set_data (G_OBJECT(column), MODEST_HEADER_VIEW_COLUMN,
				   GINT_TO_POINTER(col));
		
		/* we need this ptr when sorting the rows */
		g_object_set_data (G_OBJECT(column), MODEST_HEADER_VIEW_PTR,
				   self);
		gtk_tree_view_append_column (GTK_TREE_VIEW(self), column);		
	}	

	return TRUE;
}

static void
modest_header_view_init (ModestHeaderView *obj)
{
	ModestHeaderViewPrivate *priv;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(obj); 

	priv->folder  = NULL;

	priv->monitor	     = NULL;
	priv->monitor_lock   = g_mutex_new ();


	setup_drag_and_drop (GTK_TREE_VIEW (obj));
}

static void
modest_header_view_finalize (GObject *obj)
{
	ModestHeaderView        *self;
	ModestHeaderViewPrivate *priv;
	
	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	g_mutex_lock (priv->monitor_lock);
	if (priv->monitor) {
		tny_folder_monitor_stop (priv->monitor);
		g_object_unref (G_OBJECT (priv->monitor));
	}
	g_mutex_unlock (priv->monitor_lock);
	g_mutex_free (priv->monitor_lock);

	if (priv->folder) {
		g_object_unref (G_OBJECT (priv->folder));
		priv->folder   = NULL;
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
	modest_header_view_set_folder (self, NULL);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(obj));
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(obj),TRUE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(obj), TRUE);
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(obj),
				      TRUE); /* alternating row colors */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	
	g_signal_connect (sel, "changed",
			  G_CALLBACK(on_selection_changed), self);
	
	g_signal_connect (self, "button-press-event",
			  G_CALLBACK(on_header_clicked), NULL);

	g_signal_connect (self, "focus-in-event",
			  G_CALLBACK(on_focus_in), NULL);
	
	return GTK_WIDGET(self);
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
	GtkTreeModel *old_model_sort = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));

	if (old_model_sort && GTK_IS_TREE_MODEL_SORT (old_model_sort)) { 
		GtkTreeModel *old_model;
		ModestHeaderViewPrivate *priv;

		priv = MODEST_HEADER_VIEW_GET_PRIVATE (header_view);
		old_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (old_model_sort));

		/* Set new model */
		gtk_tree_view_set_model (header_view, model);

		modest_runtime_verify_object_death (old_model, "");
		modest_runtime_verify_object_death (old_model_sort, "");
	} else
		gtk_tree_view_set_model (header_view, model);

	return;
}

static void
on_progress_changed (ModestMailOperation *mail_op,
		     ModestHeaderView *self)
{
	GtkTreeModel *sortable; 
	ModestHeaderViewPrivate *priv;
	GList *cols, *cursor;
	TnyList *headers;

	if (!modest_mail_operation_is_finished (mail_op))
		return;

	if (modest_mail_operation_get_error (mail_op))
		return;
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	headers = TNY_LIST (tny_gtk_header_list_model_new ());

	tny_gtk_header_list_model_set_folder (TNY_GTK_HEADER_LIST_MODEL(headers),
					      priv->folder, TRUE);

	sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(headers));
	g_object_unref (G_OBJECT (headers));

	/* install our special sorting functions */
	cursor = cols = gtk_tree_view_get_columns (GTK_TREE_VIEW(self));
	while (cursor) {
		gint col_id = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(cursor->data),
								 MODEST_HEADER_VIEW_COLUMN));
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable),
						 col_id,
						 (GtkTreeIterCompareFunc) cmp_rows,
						 cursor->data, NULL);
		cursor = g_list_next(cursor);
	}
	g_list_free (cols);

	/* Set new model */
	modest_header_view_set_model (GTK_TREE_VIEW (self), sortable);
	g_object_unref (G_OBJECT (sortable));

	/* Add a folder observer */
	g_mutex_lock (priv->monitor_lock);
	priv->monitor = TNY_FOLDER_MONITOR (tny_folder_monitor_new (priv->folder));
	tny_folder_monitor_add_list (priv->monitor, TNY_LIST (headers));
	tny_folder_monitor_start (priv->monitor);
	g_mutex_unlock (priv->monitor_lock);
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
	TnyList *headers;
	ModestHeaderViewPrivate *priv;
	GList *cols, *cursor;
	GtkTreeModel *sortable; 

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	headers = TNY_LIST (tny_gtk_header_list_model_new ());

	tny_gtk_header_list_model_set_folder (TNY_GTK_HEADER_LIST_MODEL(headers),
					      folder, FALSE);

	/* Add a folder observer */
	g_mutex_lock (priv->monitor_lock);
	if (priv->monitor) {
		tny_folder_monitor_stop (priv->monitor);
		g_object_unref (G_OBJECT (priv->monitor));
	}
	priv->monitor = TNY_FOLDER_MONITOR (tny_folder_monitor_new (folder));
	tny_folder_monitor_add_list (priv->monitor, TNY_LIST (headers));
	tny_folder_monitor_start (priv->monitor);
	g_mutex_unlock (priv->monitor_lock);


	sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(headers));
	g_object_unref (G_OBJECT (headers));

	/* install our special sorting functions */
	cursor = cols = gtk_tree_view_get_columns (GTK_TREE_VIEW(self));
	while (cursor) {
		gint col_id = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(cursor->data),
								 MODEST_HEADER_VIEW_COLUMN));
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable),
						 col_id,
						 (GtkTreeIterCompareFunc) cmp_rows,
						 cursor->data, NULL);
		cursor = g_list_next(cursor);
	}
	g_list_free (cols);


	/* Set new model */
	modest_header_view_set_model (GTK_TREE_VIEW (self), sortable);
	g_object_unref (G_OBJECT (sortable));
}

void
modest_header_view_set_folder (ModestHeaderView *self, TnyFolder *folder)
{
	ModestHeaderViewPrivate *priv;

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->folder) {
		g_object_unref (priv->folder);
		priv->folder = NULL;
	}

	if (folder) {
		ModestMailOperation *mail_op;

		modest_header_view_set_folder_intern (self, folder);

		/* Pick my reference. Nothing to do with the mail operation */
		priv->folder = g_object_ref (folder);

		/* no message selected */
		g_signal_emit (G_OBJECT(self), signals[HEADER_SELECTED_SIGNAL], 0, NULL);

		/* Create the mail operation */
		mail_op = modest_mail_operation_new ();
		modest_mail_operation_queue_add (modest_runtime_get_mail_operation_queue (),
						 mail_op);

		/* Register a mail operation observer */
		g_signal_connect (mail_op, "progress-changed", 
				  G_CALLBACK (on_progress_changed), self);

		/* Refresh the folder asynchronously */
		modest_mail_operation_refresh_folder (mail_op, folder);

		/* Free */
		g_object_unref (mail_op);

	} else {
		g_mutex_lock (priv->monitor_lock);
		modest_header_view_set_model (GTK_TREE_VIEW (self), NULL); 

		if (priv->monitor) {
			tny_folder_monitor_stop (priv->monitor);
			g_object_unref (G_OBJECT (priv->monitor));
			priv->monitor = NULL;
		}
		g_mutex_unlock (priv->monitor_lock);
	}
}

static gboolean
on_header_clicked (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	ModestHeaderView *self = NULL;
	ModestHeaderViewPrivate *priv = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model = NULL;
	TnyHeader *header;
	/* ignore everything but doubleclick */
	if (event->type != GDK_2BUTTON_PRESS)
		return FALSE;

	self = MODEST_HEADER_VIEW (widget);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);
	
	path = get_selected_row (GTK_TREE_VIEW(self), &model);
	if ((path == NULL) || (!gtk_tree_model_get_iter(model, &iter, path))) 
		return FALSE;
			
	/* get the first selected item */
	gtk_tree_model_get (model, &iter,
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);
	/* Emit signal */
	g_signal_emit (G_OBJECT(self), 
		       signals[HEADER_ACTIVATED_SIGNAL], 
		       0, header);

	/* Free */
	g_object_unref (G_OBJECT (header));
	gtk_tree_path_free(path);

	return TRUE;
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
	
	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	self = MODEST_HEADER_VIEW (user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);	

	path = get_selected_row (GTK_TREE_VIEW(self), &model);
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
	gtk_tree_path_free(path);
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


static gint
cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
	  gpointer user_data)
{
	gint col_id;
	gint t1, t2;
	gint val1, val2;
	gchar *s1, *s2;
	gint cmp;
	
	static int counter = 0;
	col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(user_data), MODEST_HEADER_VIEW_COLUMN));
	
	if (!(++counter % 100)) {
		GObject *header_view = g_object_get_data(G_OBJECT(user_data),
							 MODEST_HEADER_VIEW_PTR);
		g_signal_emit (header_view,
			       signals[STATUS_UPDATE_SIGNAL],
			       0, _("Sorting..."), 0, 0);
	}	
	switch (col_id) {

		/* first one, we decide based on the time */
	case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_IN:
	case MODEST_HEADER_VIEW_COLUMN_COMPACT_RECEIVED_DATE:
	case MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE:

		gtk_tree_model_get (tree_model, iter1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
				    &t1,-1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
				    &t2,-1);
		return t1 - t2;

	case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT:
	case MODEST_HEADER_VIEW_COLUMN_COMPACT_SENT_DATE:
	case MODEST_HEADER_VIEW_COLUMN_SENT_DATE:
		gtk_tree_model_get (tree_model, iter1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
				    &t1,-1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
				    &t2,-1);
		return t1 - t2;

		
		/* next ones, we try the search criteria first, if they're the same, then we use 'sent date' */
		/* FIXME: what about received-date? */
	case MODEST_HEADER_VIEW_COLUMN_SUBJECT: {

		gtk_tree_model_get (tree_model, iter1,
				    TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &s1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,
				    -1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN, &s2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,
				    -1);

		/* the prefix ('Re:', 'Fwd:' etc.) we ignore */ 
		cmp = modest_text_utils_utf8_strcmp (s1 + modest_text_utils_get_subject_prefix_len(s1),
						     s2 + modest_text_utils_get_subject_prefix_len(s2),
						     TRUE);
		g_free (s1);
		g_free (s2);
		
		return cmp ? cmp : t1 - t2;
	}
		
	case MODEST_HEADER_VIEW_COLUMN_FROM:
		
		gtk_tree_model_get (tree_model, iter1,
				    TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN, &s1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,
				    -1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN, &s2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,
				    -1);
		cmp = modest_text_utils_utf8_strcmp (s1, s2, TRUE);
		g_free (s1);
		g_free (s2);
		
		return cmp ? cmp : t1 - t2;
		
	case MODEST_HEADER_VIEW_COLUMN_TO: 
		
		gtk_tree_model_get (tree_model, iter1,
				    TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN, &s1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,
				    -1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN, &s2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,
				    -1);
		cmp = modest_text_utils_utf8_strcmp (s1, s2, TRUE);
		g_free (s1);
		g_free (s2);
		
		return cmp ? cmp : t1 - t2;

	case MODEST_HEADER_VIEW_COLUMN_ATTACH:

		gtk_tree_model_get (tree_model, iter1, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1, -1);
		gtk_tree_model_get (tree_model, iter2, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2, -1);
		
		cmp = (val1 & TNY_HEADER_FLAG_ATTACHMENTS) -
			(val2 & TNY_HEADER_FLAG_ATTACHMENTS);

		return cmp ? cmp : t1 - t2;
		
	case MODEST_HEADER_VIEW_COLUMN_MSGTYPE:
		gtk_tree_model_get (tree_model, iter1, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN, &val2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN, &t2,-1);
		cmp =  (val1 & TNY_HEADER_FLAG_SEEN) - (val2 & TNY_HEADER_FLAG_SEEN);

		return cmp ? cmp : t1 - t2;

	default:
		return &iter1 - &iter2; /* oughhhh  */
	}
}

/* Drag and drop stuff */
static void
drag_data_get_cb (GtkWidget *widget, GdkDragContext *context, 
		  GtkSelectionData *selection_data, 
		  guint info,  guint time, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *source_row;
	
	source_row = get_selected_row (GTK_TREE_VIEW(widget), &model);
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
		g_message ("default");
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

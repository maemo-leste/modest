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
#include <string.h>

#include <modest-header-view.h>
#include <modest-header-view-priv.h>

#include <modest-marshal.h>
#include <modest-text-utils.h>
#include <modest-icon-names.h>
#include <modest-icon-factory.h>

static void modest_header_view_class_init  (ModestHeaderViewClass *klass);
static void modest_header_view_init        (ModestHeaderView *obj);
static void modest_header_view_finalize    (GObject *obj);

static void on_selection_changed (GtkTreeSelection *sel, gpointer user_data);

static gint cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
		      gpointer user_data);

#define MODEST_HEADER_VIEW_PTR "modest-header-view"

enum {
	HEADER_SELECTED_SIGNAL,
	ITEM_NOT_FOUND_SIGNAL,
	STATUS_UPDATE_SIGNAL,
	LAST_SIGNAL
};


typedef struct _ModestHeaderViewPrivate ModestHeaderViewPrivate;
struct _ModestHeaderViewPrivate {
	TnyFolder            *folder;
	TnyList              *headers;
	GMutex		     *lock;
	ModestHeaderViewStyle style;
	gulong                sig1;
	ModestHeaderViewState state;
};

#define MODEST_HEADER_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
						MODEST_TYPE_HEADER_VIEW, \
                                                ModestHeaderViewPrivate))

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
	if (resizable) {
		gtk_tree_view_column_set_min_width (column, 100);
		//gtk_tree_view_column_set_expand (column, TRUE);
	}
	
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

	gtk_tree_view_set_model (GTK_TREE_VIEW(obj), NULL);
	
	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW(obj));

	for (cursor = columns; cursor; cursor = cursor->next)
		gtk_tree_view_remove_column (GTK_TREE_VIEW(obj),
					     GTK_TREE_VIEW_COLUMN(cursor->data));
	g_list_free (columns);	
}



static gboolean
set_empty (ModestHeaderView *self)
{
	ModestHeaderViewPrivate *priv;	
	GtkTreeViewColumn *column;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkCellRenderer* renderer;
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self); 	
	remove_all_columns (self);

	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, _("(No items in this folder)"), -1); 
	
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT(renderer), "weight", 800, NULL);

	column = gtk_tree_view_column_new_with_attributes ("", renderer, "text", 0, NULL);
	
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW(self), column);

	gtk_tree_view_set_model(GTK_TREE_VIEW(self), GTK_TREE_MODEL(store));
	g_object_unref (store);
	
	priv->state = MODEST_HEADER_VIEW_STATE_IS_EMPTY;
	return TRUE;
}



ModestHeaderViewState
modest_header_view_get_state (ModestHeaderView *self)
{
	g_return_val_if_fail (MODEST_IS_HEADER_VIEW (self), TRUE);
	
	return MODEST_HEADER_VIEW_GET_PRIVATE(self)->state;
}


static void
set_state (ModestHeaderView *self, ModestHeaderViewState state)
{
	ModestHeaderViewState oldstate =
		MODEST_HEADER_VIEW_GET_PRIVATE(self)->state;
	
	if (oldstate != state) {
		if ((oldstate & MODEST_HEADER_VIEW_STATE_IS_EMPTY) !=
		    (state & MODEST_HEADER_VIEW_STATE_IS_EMPTY))
			set_empty (self);
		
		MODEST_HEADER_VIEW_GET_PRIVATE(self)->state = state; 
		/* FIXME: emit signal if the state changed*/
	}
}


static void
update_state (ModestHeaderView *self)
{
	GtkTreePath *path;
	GtkTreeSelection *sel;
	ModestHeaderViewState state = 0;
	ModestHeaderViewPrivate *priv;	

	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self); 

	if (!priv->folder || tny_folder_get_all_count(priv->folder) == 0)
		state = MODEST_HEADER_VIEW_STATE_IS_EMPTY;
	else {
		gtk_tree_view_get_cursor (GTK_TREE_VIEW(self), &path, NULL);
		if (path) {
			GtkTreePath *path2;
			
			state |= MODEST_HEADER_VIEW_STATE_HAS_CURSOR;	
			path2= gtk_tree_path_copy (path);
			
			gtk_tree_path_next (path);
			if (gtk_tree_path_compare (path, path2) != 0)
				state |= MODEST_HEADER_VIEW_STATE_AT_LAST_ITEM;
			
			if (!gtk_tree_path_prev (path2))
				state |= MODEST_HEADER_VIEW_STATE_AT_FIRST_ITEM;
			
			gtk_tree_path_free (path);
			gtk_tree_path_free (path2);
		}
		
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(self));
		if (sel) {
			state |= MODEST_HEADER_VIEW_STATE_HAS_SELECTION;
			if (gtk_tree_selection_count_selected_rows (sel) > 1)
				state |= MODEST_HEADER_VIEW_STATE_HAS_MULTIPLE_SELECTION;
		}
	}

	set_state (self, state);	
}



gboolean
modest_header_view_set_columns (ModestHeaderView *self, const GList *columns)
{
	GtkTreeViewColumn *column=NULL;
	GtkCellRenderer *renderer_msgtype,
		*renderer_header,
		*renderer_attach;

	ModestHeaderViewPrivate *priv;
	const GList *cursor;
	
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self); 

	/* FIXME: check whether these renderers need to be freed */
	renderer_msgtype = gtk_cell_renderer_pixbuf_new ();
	renderer_attach  = gtk_cell_renderer_pixbuf_new ();
	renderer_header  = gtk_cell_renderer_text_new (); 
	
	remove_all_columns (self);
	
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
			gtk_tree_view_column_set_fixed_width (column, 32);
			break;

		case MODEST_HEADER_VIEW_COLUMN_ATTACH:
			column = get_new_column (_("A"), renderer_attach, FALSE,
						 TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
						 FALSE,
						 (GtkTreeCellDataFunc)_modest_header_view_attach_cell_data,
						 NULL);
			gtk_tree_view_column_set_fixed_width (column, 32);
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
						 TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_header_cell_data,
						 NULL);
			break;
			
		case MODEST_HEADER_VIEW_COLUMN_SENT_DATE:
			column = get_new_column (_("Sent"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_header_cell_data,
						 NULL);
			break;
			
		case MODEST_HEADER_VIEW_COLUMN_SIZE:
			column = get_new_column (_("Size"), renderer_header, TRUE,
						 TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN,
						 TRUE,
						 (GtkTreeCellDataFunc)_modest_header_view_size_cell_data,
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
	return TRUE;
}



static void
modest_header_view_init (ModestHeaderView *obj)
{
	ModestHeaderViewPrivate *priv;
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(obj); 

	priv->lock = g_mutex_new ();
	priv->sig1 = 0;
}

static void
modest_header_view_finalize (GObject *obj)
{
	ModestHeaderView        *self;
	ModestHeaderViewPrivate *priv;
	GtkTreeSelection        *sel;
	
	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	if (priv->headers)	
		g_object_unref (G_OBJECT(priv->headers));

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));

	if (sel && priv->sig1 != 0) {
		g_signal_handler_disconnect (G_OBJECT(sel), priv->sig1);
		priv->sig1 = 0;
	}
		
	if (priv->lock) {
		g_mutex_free (priv->lock);
		priv->lock = NULL;
	}

	priv->headers  = NULL;
	priv->folder   = NULL;
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}
	
GtkWidget*
modest_header_view_new (TnyFolder *folder, const GList *columns,
			ModestHeaderViewStyle style)
{
	GObject *obj;
	GtkTreeSelection *sel;
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;
	
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_HEADER_VIEW, NULL));
	self = MODEST_HEADER_VIEW(obj);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	modest_header_view_set_style   (self, style);
	modest_header_view_set_columns (self, columns);

	if (!modest_header_view_set_folder (self, NULL)) {
		g_warning ("could not set the folder");
		g_object_unref (obj);
		return NULL;
	}
		
	/* all cols */
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(obj));
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(obj),TRUE);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(obj),
				      TRUE); /* alternating row colors */

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	
	priv->sig1 = g_signal_connect (sel, "changed",
				       G_CALLBACK(on_selection_changed), self);
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
			gtk_tree_model_get_iter (tree_model,
						 &iter,
						 (GtkTreePath *) (tmp->data));
									  
			gtk_tree_model_get (tree_model, &iter,
					    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
					    &header, -1);

			/* Prepend to list */
			tny_list_prepend (header_list, G_OBJECT (header));
			tmp = g_list_next (tmp);
		}
		/* Clean up*/
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	return header_list;
}

void 
modest_header_view_select_next (ModestHeaderView *self)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	if (sel) {
		gtk_tree_selection_get_selected (sel, &model, &iter);
		gtk_tree_model_iter_next (model, &iter);
		gtk_tree_selection_select_iter (sel, &iter);
	}
}

GList*
modest_header_view_get_columns (ModestHeaderView *self)
{
 	g_return_val_if_fail (self, FALSE);
	return gtk_tree_view_get_columns (GTK_TREE_VIEW(self)); 
}

gboolean
modest_header_view_set_style (ModestHeaderView *self,
			      ModestHeaderViewStyle style)
{
	g_return_val_if_fail (self, FALSE);

	MODEST_HEADER_VIEW_GET_PRIVATE(self)->style = style;
	
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(self),
					     style & MODEST_HEADER_VIEW_STYLE_SHOW_HEADERS);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(self),
					     style & MODEST_HEADER_VIEW_STYLE_SHOW_HEADERS);	

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(self));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(self),
				      TRUE); /* alternating row colors */
	
	return TRUE;
}


ModestHeaderViewStyle
modest_header_view_get_style (ModestHeaderView *self)
{
	g_return_val_if_fail (self, FALSE);

	return MODEST_HEADER_VIEW_GET_PRIVATE(self)->style;
}


static void
on_refresh_folder (TnyFolder *folder, gboolean cancelled, GError **err,
		   gpointer user_data)
{
	GtkTreeModel *sortable; 
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;
	GError *error = NULL;

	if (cancelled)
		return;
	
	self = MODEST_HEADER_VIEW(user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	priv->folder = folder;
	update_state (self);
	
	if (!folder || priv->state & MODEST_HEADER_VIEW_STATE_IS_EMPTY)  
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(self), FALSE);	
	else { /* it's a new one or a refresh */
		GList *cols, *cursor;

		if (priv->headers)
			g_object_unref (priv->headers);

		priv->headers = TNY_LIST(tny_gtk_header_list_model_new ());
		tny_folder_get_headers (folder, priv->headers, FALSE, &error); /* FIXME */
		if (error) {
			g_signal_emit (G_OBJECT(self), signals[ITEM_NOT_FOUND_SIGNAL],
				       0, MODEST_ITEM_TYPE_MESSAGE);
			g_print (error->message);
			g_error_free (error);
			return;
		}

		tny_gtk_header_list_model_set_folder
			(TNY_GTK_HEADER_LIST_MODEL(priv->headers),folder, TRUE); /*async*/

		sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(priv->headers));

		/* install our special sorting functions */
		cursor = cols = gtk_tree_view_get_columns (GTK_TREE_VIEW(self));
		while (cursor) {
			gint col_id = GPOINTER_TO_INT (g_object_get_data(G_OBJECT(cursor->data),
									 MODEST_HEADER_VIEW_COLUMN));
			gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable),
							 col_id,
							 (GtkTreeIterCompareFunc)cmp_rows,
							 cursor->data, NULL);
			cursor = g_list_next(cursor);
		}
		g_list_free (cols);
		
		gtk_tree_view_set_model (GTK_TREE_VIEW (self), sortable);
		modest_header_view_set_style (self, priv->style);
	}
}


static void
on_refresh_folder_status_update (TnyFolder *folder, const gchar *msg,
				 gint num, gint total,  gpointer user_data)
{
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;

	self = MODEST_HEADER_VIEW(user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	g_signal_emit (G_OBJECT(self), signals[STATUS_UPDATE_SIGNAL],
		       0, msg, num, total);
}


TnyFolder*
modest_header_view_get_folder (ModestHeaderView *self)
{
	ModestHeaderViewPrivate *priv;
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	return priv->folder;
}


gboolean
modest_header_view_set_folder (ModestHeaderView *self, TnyFolder *folder)
{
	ModestHeaderViewPrivate *priv;
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);

	priv->folder = folder;
	
	if (!folder)  {/* when there is no folder */
		gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(self), FALSE);
		gtk_tree_view_set_model (GTK_TREE_VIEW (self), NULL);
	} else { /* it's a new one or a refresh */
		tny_folder_refresh_async (folder,
					  on_refresh_folder,
					  on_refresh_folder_status_update,
					  self);
	}
	/* no message selected */
	g_signal_emit (G_OBJECT(self), signals[HEADER_SELECTED_SIGNAL], 0,
		       NULL);
	return TRUE;
}

static void
on_selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel *model;
	TnyHeader *header;
	GtkTreeIter iter;
	ModestHeaderView *self;
	ModestHeaderViewPrivate *priv;
	
	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	self = MODEST_HEADER_VIEW (user_data);
	priv = MODEST_HEADER_VIEW_GET_PRIVATE(self);	

	/* if the folder is empty, nothing to do */
	if (priv->state & MODEST_HEADER_VIEW_STATE_IS_EMPTY) 
		return;
	
	if (!gtk_tree_selection_get_selected (sel, &model, &iter))
		return; /* msg was _un_selected */

	gtk_tree_model_get (model, &iter,
			    TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);

	/* Emit signal */
	g_signal_emit (G_OBJECT(self), 
		       signals[HEADER_SELECTED_SIGNAL], 
		       0, header);
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
	case MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE:
		gtk_tree_model_get (tree_model, iter1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
				    &t1,-1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
				    &t2,-1);
		return t1 - t2;

	case MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER_OUT:
	case MODEST_HEADER_VIEW_COLUMN_SENT_DATE:
		gtk_tree_model_get (tree_model, iter1,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
				    &t1,-1);
		gtk_tree_model_get (tree_model, iter2,
				    TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
				    &t2,-1);
		return t1 - t2;

		
		/* next ones, we try the search criteria first, if they're the same, then we use 'sent date' */
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


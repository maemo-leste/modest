/* modest-tny-header-tree-view.c 
 */
#include <glib/gi18n.h>
#include "modest-tny-header-tree-view.h"
#include <tny-list-iface.h>
#include <string.h>

/* 'private'/'protected' functions */
static void modest_tny_header_tree_view_class_init  (ModestTnyHeaderTreeViewClass *klass);
static void modest_tny_header_tree_view_init        (ModestTnyHeaderTreeView *obj);
static void modest_tny_header_tree_view_finalize    (GObject *obj);

static void selection_changed (GtkTreeSelection *sel, gpointer user_data);
static void column_clicked (GtkTreeViewColumn *treeviewcolumn, gpointer user_data);

#define PIXMAP_PREFIX PREFIX "/share/modest/pixmaps/"

enum {
	MESSAGE_SELECTED_SIGNAL,
	LAST_SIGNAL
};


enum {
	HEADER_ICON_READ  = 1,
	HEADER_ICON_UNREAD,
	HEADER_ICON_ATTACH,
	HEADER_ICON_NUM
};


enum {
	SORT_COLUMN_FROM  = 1,
	SORT_COLUMN_TO,
	SORT_COLUMN_SUBJECT,
	SORT_COLUMN_ATTACH,
	SORT_COLUMN_RECEIVED,
	SORT_COLUMN_SENT,
	SORT_COLUMN_MSGTYPE,
	SORT_COLUMN_NUM
};


typedef struct _ModestTnyHeaderTreeViewPrivate ModestTnyHeaderTreeViewPrivate;
struct _ModestTnyHeaderTreeViewPrivate {
	TnyMsgFolderIface *tny_msg_folder;
	TnyListIface      *headers;

	GdkPixbuf *icons[HEADER_ICON_NUM];
	guint sort_columns[SORT_COLUMN_NUM];
};
#define MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                         MODEST_TYPE_TNY_HEADER_TREE_VIEW, \
                                                         ModestTnyHeaderTreeViewPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
static guint signals[LAST_SIGNAL] = {0};

GType
modest_tny_header_tree_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyHeaderTreeViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_header_tree_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyHeaderTreeView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_header_tree_view_init,
		};
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestTnyHeaderTreeView",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_tny_header_tree_view_class_init (ModestTnyHeaderTreeViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_header_tree_view_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestTnyHeaderTreeViewPrivate));

	signals[MESSAGE_SELECTED_SIGNAL] = 
		g_signal_new ("message_selected",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestTnyHeaderTreeViewClass,message_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER); 	
}





static void
msgtype_cell_data (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
		   GtkTreeModel *tree_model, GtkTreeIter *iter,
		   GdkPixbuf *icons[HEADER_ICON_NUM])
{
	TnyMsgHeaderFlags flags;
	GdkPixbuf *pixbuf;

	gtk_tree_model_get (tree_model, iter, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);

	if (flags & TNY_MSG_HEADER_FLAG_SEEN)
		pixbuf = icons[HEADER_ICON_READ];
	else
		pixbuf = icons[HEADER_ICON_UNREAD];
	
	g_object_set (G_OBJECT (renderer), "pixbuf", pixbuf,
		      NULL);
}

static void
attach_cell_data (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		  GtkTreeModel *tree_model,  GtkTreeIter *iter, GdkPixbuf *icons[HEADER_ICON_NUM])
{
	TnyMsgHeaderFlags flags;
	GdkPixbuf *pixbuf;

	gtk_tree_model_get (tree_model, iter, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);
	if (flags & TNY_MSG_HEADER_FLAG_ATTACHMENTS)
		pixbuf = icons[HEADER_ICON_ATTACH];
	else
		pixbuf = NULL;
	
	g_object_set (G_OBJECT (renderer), "pixbuf", pixbuf, NULL);
}



static void
header_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		  GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data)
{
	GObject *rendobj;
	TnyMsgHeaderFlags flags;
	
	gtk_tree_model_get (tree_model, iter, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
			    &flags, -1);
	rendobj = G_OBJECT(renderer);		
	
	if (!(flags & TNY_MSG_HEADER_FLAG_SEEN))
		g_object_set (rendobj, "weight", 800, NULL);
	else
		g_object_set (rendobj, "weight", 400, NULL); /* default, non-bold */
}



static void
sender_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		   GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer user_data)
{
	GObject *rendobj;
	TnyMsgHeaderFlags flags;
	gchar *from;
	gchar *address;
	
	gtk_tree_model_get (tree_model, iter,
			    TNY_MSG_HEADER_LIST_MODEL_FROM_COLUMN,  &from,
			    TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags,
			    -1);
	rendobj = G_OBJECT(renderer);		

	/* simplistic --> remove <email@address> from display */
	address = g_strstr_len (from, strlen(from), "<");
	if (address) {
		address[0]='\0';
		g_object_set (rendobj, "text", from, NULL);
		g_free (from);
	}
			     
	if (!(flags & TNY_MSG_HEADER_FLAG_SEEN))
		g_object_set (rendobj, "weight", 800, NULL);
	else
		g_object_set (rendobj, "weight", 400, NULL); /* default, non-bold */
}


static void
init_icons (GdkPixbuf *icons[HEADER_ICON_NUM])
{
	icons[HEADER_ICON_READ] =
		gdk_pixbuf_new_from_file (PIXMAP_PREFIX "read.xpm",NULL);
	icons[HEADER_ICON_UNREAD] =
		gdk_pixbuf_new_from_file (PIXMAP_PREFIX "unread.xpm",NULL);
	icons[HEADER_ICON_ATTACH] =
		gdk_pixbuf_new_from_file (PIXMAP_PREFIX "clip.xpm",NULL);
}



static GtkTreeViewColumn*
get_new_column (const gchar *name, GtkCellRenderer *renderer,
		gboolean resizable, gint sort_col_id, gboolean show_as_text,
		GtkTreeCellDataFunc cell_data_func, gpointer user_data)
{
	GtkTreeViewColumn *column;

	column =  gtk_tree_view_column_new_with_attributes(name, renderer, NULL);
	gtk_tree_view_column_set_resizable (column, resizable);

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

/*  	g_signal_connect (G_OBJECT (column), "clicked", */
/* 			  G_CALLBACK (column_clicked), obj);  */

	return column;
}





static void
modest_tny_header_tree_view_init (ModestTnyHeaderTreeView *obj)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer_msgtype,
		*renderer_header,
		*renderer_attach;
	int i;
	ModestTnyHeaderTreeViewPrivate *priv;
	
	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(obj); 

	init_icons (priv->icons);
	
	renderer_msgtype = gtk_cell_renderer_pixbuf_new ();
	renderer_attach  = gtk_cell_renderer_pixbuf_new ();
	renderer_header = gtk_cell_renderer_text_new (); 

	priv->tny_msg_folder = NULL;
	priv->headers        = NULL;

	for (i = 0; i != SORT_COLUMN_NUM; ++i)
		priv->sort_columns[i] = -1;

	/* msgtype */
	column = get_new_column (_("M"), renderer_msgtype, FALSE, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
				 FALSE, (GtkTreeCellDataFunc)msgtype_cell_data, priv->icons);
 	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	priv->sort_columns[SORT_COLUMN_MSGTYPE] =
		gtk_tree_view_column_get_sort_column_id (column);
  	g_signal_connect (G_OBJECT (column), "clicked",G_CALLBACK (column_clicked), obj);  

	
	/* attachment */
	column = get_new_column (_("A"), renderer_attach, FALSE, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
				 FALSE, (GtkTreeCellDataFunc)attach_cell_data, priv->icons);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	priv->sort_columns[SORT_COLUMN_ATTACH] =
		gtk_tree_view_column_get_sort_column_id (column);
	g_signal_connect (G_OBJECT (column), "clicked",G_CALLBACK (column_clicked), obj);  
	
	/* received */
	column = get_new_column (_("Received"), renderer_header, TRUE, TNY_MSG_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
				 TRUE, (GtkTreeCellDataFunc)header_cell_data, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	priv->sort_columns[SORT_COLUMN_RECEIVED] =
		gtk_tree_view_column_get_sort_column_id (column);
	g_signal_connect (G_OBJECT (column), "clicked",G_CALLBACK (column_clicked), obj);  

	
	/* from */
	column = get_new_column (_("From"), renderer_header, TRUE, TNY_MSG_HEADER_LIST_MODEL_FROM_COLUMN,
				 TRUE, (GtkTreeCellDataFunc)sender_cell_data, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	priv->sort_columns[SORT_COLUMN_FROM] =
		gtk_tree_view_column_get_sort_column_id (column);
	g_signal_connect (G_OBJECT (column), "clicked",G_CALLBACK (column_clicked), obj);  


	/* subject */
	column = get_new_column (_("Subject"), renderer_header, TRUE, TNY_MSG_HEADER_LIST_MODEL_SUBJECT_COLUMN,
				 TRUE, (GtkTreeCellDataFunc)header_cell_data, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	priv->sort_columns[SORT_COLUMN_SUBJECT] =
		gtk_tree_view_column_get_sort_column_id (column);
	g_signal_connect (G_OBJECT (column), "clicked",G_CALLBACK (column_clicked), obj);  

	
	/* all cols */
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(obj), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(obj), TRUE);
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(obj), TRUE); /* alternating row colors */
	
}

static void
modest_tny_header_tree_view_finalize (GObject *obj)
{
	ModestTnyHeaderTreeView        *self;
	ModestTnyHeaderTreeViewPrivate *priv;
	int i;
	
	self = MODEST_TNY_HEADER_TREE_VIEW(obj);
	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(self);

	if (priv->headers)	
		g_object_unref (G_OBJECT(priv->headers));
	
	priv->headers = NULL;
	priv->tny_msg_folder    = NULL;

	/* cleanup our icons */
	for (i = 0; i != HEADER_ICON_NUM; ++i) {
		if (priv->icons[i]) {
			g_object_unref (G_OBJECT(priv->icons[i]));
			priv->icons[i] = NULL;
		}
	}
}

GtkWidget*
modest_tny_header_tree_view_new (TnyMsgFolderIface *folder)
{
	GObject *obj;
	GtkTreeSelection *sel;
	ModestTnyHeaderTreeView *self;
		
	obj  = G_OBJECT(g_object_new(MODEST_TYPE_TNY_HEADER_TREE_VIEW, NULL));
	self = MODEST_TNY_HEADER_TREE_VIEW(obj);

	if (!modest_tny_header_tree_view_set_folder (self, NULL)) {
		g_warning ("could not set the folder");
		g_object_unref (obj);
		return NULL;
	}
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	g_signal_connect (sel, "changed",
			  G_CALLBACK(selection_changed), self);

	return GTK_WIDGET(self);
}


static gint
cmp_rows (GtkTreeModel *tree_model, GtkTreeIter *iter1, GtkTreeIter *iter2,
	  gpointer user_data)
{
	gint col_id = GPOINTER_TO_INT (user_data);
	gint val1, val2;

	g_return_val_if_fail (GTK_IS_TREE_MODEL(tree_model), -1);
	
	switch (col_id) {

	case SORT_COLUMN_RECEIVED:
		gtk_tree_model_get (tree_model, iter1, TNY_MSG_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
				    &val1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_MSG_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
				    &val2,-1);

		g_message ("%d %d %d %d", col_id, val1, val2, val1 - val2);
		
		return val1 - val2;
		
	case SORT_COLUMN_SENT:
		gtk_tree_model_get (tree_model, iter1, TNY_MSG_HEADER_LIST_MODEL_DATE_SENT_COLUMN,
				    &val1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_MSG_HEADER_LIST_MODEL_DATE_SENT_COLUMN,
				    &val2,-1);
		return val1 - val2;
	
	case SORT_COLUMN_ATTACH:
		gtk_tree_model_get (tree_model, iter1, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
				    &val1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
				    &val2,-1);
		
		return (val1 & TNY_MSG_HEADER_FLAG_ATTACHMENTS) - (val2 & TNY_MSG_HEADER_FLAG_ATTACHMENTS);
	

	case SORT_COLUMN_MSGTYPE:
		gtk_tree_model_get (tree_model, iter1, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
				    &val1,-1);
		gtk_tree_model_get (tree_model, iter2, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN,
				    &val2,-1);
		
		return (val1 & TNY_MSG_HEADER_FLAG_SEEN) - (val2 & TNY_MSG_HEADER_FLAG_SEEN);

	default:
		g_message ("%p %p", iter1, iter2);
		return &iter1 - &iter2;
	}
}



gboolean
modest_tny_header_tree_view_set_folder (ModestTnyHeaderTreeView *self,
					TnyMsgFolderIface *folder)
{
	int i;
	GtkTreeModel *oldsortable, *sortable, *oldmodel;
	ModestTnyHeaderTreeViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);

	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(self);
	
	if (folder) {
		priv->headers = TNY_LIST_IFACE(tny_msg_header_list_model_new ());
		tny_msg_folder_iface_get_headers (folder, priv->headers,
						  FALSE);
		tny_msg_header_list_model_set_folder (TNY_MSG_HEADER_LIST_MODEL(priv->headers),
						      folder);
		
		oldsortable = gtk_tree_view_get_model(GTK_TREE_VIEW (self));
		if (oldsortable && GTK_IS_TREE_MODEL_SORT(oldsortable)) {
			GtkTreeModel *oldmodel = gtk_tree_model_sort_get_model
				(GTK_TREE_MODEL_SORT(oldsortable));
			if (oldmodel)
				g_object_unref (G_OBJECT(oldmodel));
			g_object_unref (oldsortable);
		}
	
		sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(priv->headers));
		
		/* set special sorting functions */
#if 0 /* FIXME */
		gtk_tree_model_sort_reset_default_sort_func (sortable);

		for (i = 0; i != SORT_COLUMN_NUM; ++i) {
			int col_id = priv->sort_columns[i];
			if (col_id >= 0) {
				g_message ("%d: %p: %p: %d", i, GTK_TREE_SORTABLE(sortable), sortable, col_id);
				gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(sortable), col_id,
								 (GtkTreeIterCompareFunc)cmp_rows,
								 GINT_TO_POINTER(col_id), NULL);
			}
		}
#endif
		gtk_tree_view_set_model (GTK_TREE_VIEW (self), sortable);
		gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(self), TRUE);

		/* no need to unref sortable */
		
	} else /* when there is no folder */
		gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(self), FALSE);
					 
	return TRUE;
}


static void
selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel            *model;
	TnyMsgHeaderIface       *header;
	GtkTreeIter             iter;
	ModestTnyHeaderTreeView *tree_view;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);
	
	if (!gtk_tree_selection_get_selected (sel, &model, &iter))
		return; /* msg was _un_selected */
	
	tree_view = MODEST_TNY_HEADER_TREE_VIEW (user_data);
	
	gtk_tree_model_get (model, &iter,
			    TNY_MSG_HEADER_LIST_MODEL_INSTANCE_COLUMN,
			    &header, -1);
	
	if (header) {
		TnyMsgHeaderFlags flags;
		const TnyMsgIface *msg;
		const TnyMsgFolderIface *folder;
		
		folder = tny_msg_header_iface_get_folder (TNY_MSG_HEADER_IFACE(header));
		if (!folder)
			g_message ("cannot find folder");
		else {
			msg = tny_msg_folder_iface_get_message (TNY_MSG_FOLDER_IFACE(folder),
								header);
			if (!msg) {
				g_message ("cannot find msg");
				/* FIXME: update display */
			}
		}
					
		g_signal_emit (G_OBJECT(tree_view), signals[MESSAGE_SELECTED_SIGNAL], 0,
			       msg);

		/* mark message as seen; _set_flags crashes, bug in tinymail? */
		flags = tny_msg_header_iface_get_flags (TNY_MSG_HEADER_IFACE(header));
		//tny_msg_header_iface_set_flags (header, flags | TNY_MSG_HEADER_FLAG_SEEN);
	}
}

static void
column_clicked (GtkTreeViewColumn *col, gpointer user_data)
{
	GtkTreeView *treeview;
	gint id;

	treeview = GTK_TREE_VIEW (user_data);
	id = gtk_tree_view_column_get_sort_column_id (col);
	
	gtk_tree_view_set_search_column (treeview, id);
}

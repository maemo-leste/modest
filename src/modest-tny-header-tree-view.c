/* modest-tny-header-tree-view.c 
 *
 * This file is part of modest.
 *
 * Copyright (C) 2006 Nokia Corporation
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#include <glib/gi18n.h>
#include "modest-tny-header-tree-view.h"
#include <tny-list-iface.h>


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
	HEADER_ICON_READ,
	HEADER_ICON_UNREAD,
	HEADER_ICON_ATTACH,
	HEADER_ICON_NUM
};



typedef struct _ModestTnyHeaderTreeViewPrivate ModestTnyHeaderTreeViewPrivate;
struct _ModestTnyHeaderTreeViewPrivate {
	TnyMsgFolderIface *tny_msg_folder;
	TnyListIface      *headers;

	GdkPixbuf *icons[HEADER_ICON_NUM];
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
flags_cell_data (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		  GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer data)
{
	TnyMsgHeaderFlags flags;
	static gchar txt[10];
	
	gtk_tree_model_get (tree_model, iter, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN, &flags, -1);
	g_snprintf (txt, 10, "%d", flags);
	g_object_set (G_OBJECT (renderer), "text", txt, NULL);
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
init_icons (GdkPixbuf *icons[HEADER_ICON_NUM])
{
	icons[HEADER_ICON_READ] =
		gdk_pixbuf_new_from_file (PIXMAP_PREFIX "read.xpm",NULL);
	icons[HEADER_ICON_UNREAD] =
		gdk_pixbuf_new_from_file (PIXMAP_PREFIX "unread.xpm",NULL);
	icons[HEADER_ICON_ATTACH] =
		gdk_pixbuf_new_from_file (PIXMAP_PREFIX "clip.xpm",NULL);
}


static void
modest_tny_header_tree_view_init (ModestTnyHeaderTreeView *obj)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer_msgtype,
		*renderer_header,
		*renderer_attach;
	
	ModestTnyHeaderTreeViewPrivate *priv;
	
	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(obj); 

	init_icons (priv->icons);
	
	renderer_msgtype = gtk_cell_renderer_pixbuf_new ();
	renderer_attach  = gtk_cell_renderer_pixbuf_new ();
	renderer_header = gtk_cell_renderer_text_new (); 

	priv->tny_msg_folder = NULL;
	priv->headers        = NULL;

	/* msgtype */
	column =  gtk_tree_view_column_new_with_attributes(_("M"), renderer_msgtype, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN);
	gtk_tree_view_column_set_sort_indicator (column, FALSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	gtk_tree_view_column_set_cell_data_func(column, renderer_msgtype,
						(GtkTreeCellDataFunc)msgtype_cell_data,
						priv->icons, NULL);


	/* attachment */
	column =  gtk_tree_view_column_new_with_attributes(_("A"), renderer_attach, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, TNY_MSG_HEADER_LIST_MODEL_FLAGS_COLUMN);
	gtk_tree_view_column_set_sort_indicator (column, FALSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	gtk_tree_view_column_set_cell_data_func(column, renderer_attach,
						(GtkTreeCellDataFunc)attach_cell_data,
						priv->icons, NULL);

	/* date */
	column =  gtk_tree_view_column_new_with_attributes(_("Date"), renderer_header,
							   "text",
							   TNY_MSG_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
 							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, TNY_MSG_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN);
	gtk_tree_view_column_set_sort_indicator (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	gtk_tree_view_column_set_cell_data_func(column, renderer_header,
						(GtkTreeCellDataFunc)header_cell_data,
						NULL, NULL);
	g_signal_connect (G_OBJECT (column), "clicked", G_CALLBACK (column_clicked), obj);

	
	/* from */
	column =  gtk_tree_view_column_new_with_attributes(_("From"), renderer_header,
							   "text",
							   TNY_MSG_HEADER_LIST_MODEL_FROM_COLUMN,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, TNY_MSG_HEADER_LIST_MODEL_FROM_COLUMN);
	gtk_tree_view_column_set_sort_indicator (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	gtk_tree_view_column_set_cell_data_func(column, renderer_header,
						(GtkTreeCellDataFunc)header_cell_data,
						NULL, NULL);
	g_signal_connect (G_OBJECT (column), "clicked", G_CALLBACK (column_clicked), obj);


	/* subject */
	column =  gtk_tree_view_column_new_with_attributes(_("Subject"), renderer_header,
							   "text",
							   TNY_MSG_HEADER_LIST_MODEL_SUBJECT_COLUMN,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, TNY_MSG_HEADER_LIST_MODEL_SUBJECT_COLUMN);
	gtk_tree_view_column_set_sort_indicator (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj), column);
	g_signal_connect (G_OBJECT (column), "clicked", G_CALLBACK (column_clicked), obj);

	
	/* all cols */
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(obj), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(obj), TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer_header,
						(GtkTreeCellDataFunc)header_cell_data,
						NULL, NULL);

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


gboolean
modest_tny_header_tree_view_set_folder (ModestTnyHeaderTreeView *self,
					TnyMsgFolderIface *folder)
{
	GtkTreeModel *oldsortable, *sortable, *oldmodel;
	ModestTnyHeaderTreeViewPrivate *priv;
	static GtkTreeModel *empty_model = NULL;
	
	g_return_val_if_fail (self, FALSE);

	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(self);
	
	if (folder) {
		priv->headers = TNY_LIST_IFACE(tny_msg_header_list_model_new ());
		tny_msg_folder_iface_get_headers (folder, priv->headers,
						  FALSE);
		tny_msg_header_list_model_set_folder (TNY_MSG_HEADER_LIST_MODEL(priv->headers),
						      folder);
	} else {
		if (!empty_model) 
			empty_model = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));
	}
		
	oldsortable = gtk_tree_view_get_model(GTK_TREE_VIEW (self));
	if (oldsortable && GTK_IS_TREE_MODEL_SORT(oldsortable)) {
		GtkTreeModel *oldmodel = gtk_tree_model_sort_get_model
			(GTK_TREE_MODEL_SORT(oldsortable));
		if (oldmodel)
			g_object_unref (G_OBJECT(oldmodel));
		g_object_unref (oldsortable);
	}
	
	if (folder) 
		sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(priv->headers));
	else
		sortable = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(empty_model));

	gtk_tree_view_set_model (GTK_TREE_VIEW (self), sortable);

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
			if (!msg) 
				g_message ("cannot find msg");		
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

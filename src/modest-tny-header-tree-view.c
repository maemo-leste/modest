/* modest-tny-header-tree-view.c */

/* insert (c)/licensing information) */

#include "modest-tny-header-tree-view.h"

/* 'private'/'protected' functions */
static void modest_tny_header_tree_view_class_init  (ModestTnyHeaderTreeViewClass *klass);
static void modest_tny_header_tree_view_init        (ModestTnyHeaderTreeView *obj);
static void modest_tny_header_tree_view_finalize    (GObject *obj);

static void selection_changed (GtkTreeSelection *sel, gpointer user_data);
	
/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	MESSAGE_SELECTED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyHeaderTreeViewPrivate ModestTnyHeaderTreeViewPrivate;
struct _ModestTnyHeaderTreeViewPrivate {
	TnyMsgFolderIface *tny_msg_folder;
	GtkTreeModel *header_tree_model;
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
modest_tny_header_tree_view_init (ModestTnyHeaderTreeView *obj)
{
 	ModestTnyHeaderTreeViewPrivate *priv;
	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(obj); 

	priv->tny_msg_folder = NULL;
	priv->header_tree_model = NULL;
}

static void
modest_tny_header_tree_view_finalize (GObject *obj)
{
	ModestTnyHeaderTreeView        *self;
	ModestTnyHeaderTreeViewPrivate *priv;

	self = MODEST_TNY_HEADER_TREE_VIEW(obj);
	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(self);

	if (priv->header_tree_model)	
		g_object_unref (G_OBJECT(priv->header_tree_model));

	priv->header_tree_model = NULL;
	priv->tny_msg_folder    = NULL;
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
	GtkTreeModel *sortable;
	ModestTnyHeaderTreeViewPrivate *priv;

	g_return_val_if_fail (self, FALSE);

	priv = MODEST_TNY_HEADER_TREE_VIEW_GET_PRIVATE(self);

	/* clean up old stuff */
	if (priv->header_tree_model)
		g_object_unref (G_OBJECT(priv->header_tree_model));
	priv->header_tree_model = NULL;

	priv->header_tree_model = GTK_TREE_MODEL (tny_msg_header_list_model_new());
	if (folder) {
		tny_msg_header_list_model_set_folder (
			TNY_MSG_HEADER_LIST_MODEL(priv->header_tree_model),
			folder, TRUE); /* FIXME: refresh?*/
		
		sortable = gtk_tree_model_sort_new_with_model (priv->header_tree_model);
		
	} else {
		static GtkTreeModel *empty_model = NULL;
		if (!empty_model)
			empty_model = GTK_TREE_MODEL(gtk_list_store_new(1,G_TYPE_STRING));

		sortable = empty_model;
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (self), sortable);

	if (sortable)
		g_object_unref (G_OBJECT(sortable));
	
	return TRUE;
}


void
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
		const TnyMsgIface *msg;
		const TnyMsgFolderIface *folder;
		
		folder = tny_msg_header_iface_get_folder (TNY_MSG_HEADER_IFACE(header));
		if (!folder) {
			g_warning ("cannot find folder");
			return;
		}
		
		msg = tny_msg_folder_iface_get_message (TNY_MSG_FOLDER_IFACE(folder), header);
		if (!msg) {
			g_warning ("cannot find msg");
			return;
		}
		
		g_signal_emit (G_OBJECT(tree_view), signals[MESSAGE_SELECTED_SIGNAL], 0,
			       msg); 
	}
}

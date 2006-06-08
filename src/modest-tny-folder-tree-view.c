/* modest-tny-folder-tree-view.c */

/* insert (c)/licensing information) */
#include <glib/gi18n.h>

#include <tny-account-tree-model.h>
#include <tny-account-store-iface.h>
#include <tny-account-iface.h>
#include <tny-msg-folder-iface.h>
#include <tny-summary-window-iface.h>

#include "modest-tny-folder-tree-view.h"

#include <modest-icon-names.h>
#include "modest-icon-factory.h"


/* 'private'/'protected' functions */
static void modest_tny_folder_tree_view_class_init  (ModestTnyFolderTreeViewClass *klass);
static void modest_tny_folder_tree_view_init        (ModestTnyFolderTreeView *obj);
static void modest_tny_folder_tree_view_finalize    (GObject *obj);

//static void modest_tny_folder_tree_view_iface_init   (gpointer iface, gpointer data);
static void modest_tny_folder_tree_view_set_account_store (TnySummaryWindowIface *self,
							   TnyAccountStoreIface *account_store);
static gboolean update_model (ModestTnyFolderTreeView *self,TnyAccountStoreIface *iface);
static gboolean update_model_empty (ModestTnyFolderTreeView *self);

static void selection_changed (GtkTreeSelection *sel, gpointer data);

enum {
	FOLDER_SELECTED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestTnyFolderTreeViewPrivate ModestTnyFolderTreeViewPrivate;
struct _ModestTnyFolderTreeViewPrivate {
	TnyAccountStoreIface *tny_account_store;
	TnyMsgFolderIface *cur_folder;
	gboolean view_is_empty;
};
#define MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                         MODEST_TYPE_TNY_FOLDER_TREE_VIEW, \
                                                         ModestTnyFolderTreeViewPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0}; 

GType
modest_tny_folder_tree_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestTnyFolderTreeViewClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_tny_folder_tree_view_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestTnyFolderTreeView),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_tny_folder_tree_view_init,
		};
				
		my_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
		                                  "ModestTnyFolderTreeView",
		                                  &my_info, 0);		
	}
	return my_type;
}

static void
modest_tny_folder_tree_view_class_init (ModestTnyFolderTreeViewClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_tny_folder_tree_view_finalize;

	g_type_class_add_private (gobject_class,
				  sizeof(ModestTnyFolderTreeViewPrivate));
	
 	signals[FOLDER_SELECTED_SIGNAL] = 
		g_signal_new ("folder_selected",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestTnyFolderTreeViewClass,folder_selected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER); 
}



static void
text_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		   GtkTreeModel *tree_model,  GtkTreeIter *iter,  gpointer data)
{
	GObject *rendobj;
	gchar *fname;
	guint unread;
	TnyMsgFolderType type;
	
	gtk_tree_model_get (tree_model, iter,
			    TNY_ACCOUNT_TREE_MODEL_NAME_COLUMN, &fname,
			    TNY_ACCOUNT_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_ACCOUNT_TREE_MODEL_UNREAD_COLUMN, &unread, -1);
	rendobj = G_OBJECT(renderer);

	if (unread > 0) {
		gchar *folder_title = g_strdup_printf ("%s (%d)", fname, unread);
		g_object_set (rendobj,"text", folder_title,  "weight", 800, NULL);
		g_free (folder_title);
	} else 
		g_object_set (rendobj,"text", fname, "weight", 400, NULL);
		
	g_free (fname);
}


static void
icon_cell_data  (GtkTreeViewColumn *column,  GtkCellRenderer *renderer,
		 GtkTreeModel *tree_model,  GtkTreeIter *iter, gpointer data)
{
	GObject *rendobj;
	GdkPixbuf *pixbuf;
	TnyMsgFolderType type;
	int unread;
	
	rendobj = G_OBJECT(renderer);
	gtk_tree_model_get (tree_model, iter,
			    TNY_ACCOUNT_TREE_MODEL_TYPE_COLUMN, &type,
			    TNY_ACCOUNT_TREE_MODEL_UNREAD_COLUMN, &unread, -1);
	rendobj = G_OBJECT(renderer);

	switch (type) {
        case TNY_MSG_FOLDER_TYPE_INBOX:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_INBOX);
                break;
        case TNY_MSG_FOLDER_TYPE_OUTBOX:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_OUTBOX);
                break;
        case TNY_MSG_FOLDER_TYPE_JUNK:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_JUNK);
                break;
        case TNY_MSG_FOLDER_TYPE_SENT:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_SENT);
                break;
	case TNY_MSG_FOLDER_TYPE_NORMAL:
        default:
                pixbuf = modest_icon_factory_get_icon (MODEST_FOLDER_ICON_NORMAL);
                break;
        }

	g_object_set (rendobj,
		      "pixbuf-expander-open",
		      modest_icon_factory_get_icon (MODEST_FOLDER_ICON_OPEN),
		      "pixbuf-expander-closed",
		      modest_icon_factory_get_icon (MODEST_FOLDER_ICON_CLOSED),
		      "pixbuf", pixbuf,
		      NULL);
}

static void
modest_tny_folder_tree_view_init (ModestTnyFolderTreeView *obj)
{
	ModestTnyFolderTreeViewPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;
	
	priv =	MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(obj);
	
	priv->view_is_empty     = TRUE;
	priv->tny_account_store = NULL;
	priv->cur_folder = NULL;

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column,
					_("All Mail Folders"));
	
	gtk_tree_view_append_column (GTK_TREE_VIEW(obj),
				     column);
	
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						icon_cell_data, NULL, NULL);
	
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer,
						text_cell_data, NULL, NULL);
	
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(obj));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

	gtk_tree_view_column_set_spacing (column, 2);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_fixed_width (column, TRUE);		
	gtk_tree_view_set_headers_visible   (GTK_TREE_VIEW(obj), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(obj), FALSE);

}


static void
modest_tny_folder_tree_view_finalize (GObject *obj)
{
	ModestTnyFolderTreeViewPrivate *priv;

	g_return_if_fail (obj);
	
	priv =	MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(obj);
	if (priv->tny_account_store) {
		g_object_unref (G_OBJECT(priv->tny_account_store));
		priv->tny_account_store = NULL;
	}

	(*parent_class->finalize)(obj);
}


static void
modest_tny_folder_tree_view_set_account_store (TnySummaryWindowIface *self,
					       TnyAccountStoreIface *account_store)
{
	ModestTnyFolderTreeViewPrivate *priv;

	g_return_if_fail (self);
	g_return_if_fail (account_store);
	
	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);
	if (priv->tny_account_store) {
		g_object_unref (priv->tny_account_store);
		priv->tny_account_store = NULL;
	}

	g_object_ref (G_OBJECT(priv->tny_account_store = account_store));
}



GtkWidget*
modest_tny_folder_tree_view_new (TnyAccountStoreIface *iface)
{
	GObject *self;
	ModestTnyFolderTreeViewPrivate *priv;
	GtkTreeSelection *sel;

	self = G_OBJECT(g_object_new(MODEST_TYPE_TNY_FOLDER_TREE_VIEW, NULL));
	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);

	g_return_val_if_fail (iface, NULL);
	
	if (!update_model (MODEST_TNY_FOLDER_TREE_VIEW(self), iface))
		g_warning ("failed or update model");

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
	g_signal_connect (sel, "changed",
			  G_CALLBACK(selection_changed), self);
		
	return GTK_WIDGET(self);
}




static gboolean
update_model_empty (ModestTnyFolderTreeView *self)
{
	GtkTreeIter  iter;
	GtkTreeStore *store;
	ModestTnyFolderTreeViewPrivate *priv;
	
	g_return_val_if_fail (self, FALSE);
	
	store = gtk_tree_store_new (1, G_TYPE_STRING);
	gtk_tree_store_append (store, &iter, NULL);

	gtk_tree_store_set (store, &iter, 0,
			    _("(empty)"), -1);

	gtk_tree_view_set_model (GTK_TREE_VIEW(self),
				 GTK_TREE_MODEL(store));
	g_object_unref (store);

	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);
	priv->view_is_empty = TRUE;
	
	return TRUE;
}


static gboolean
update_model (ModestTnyFolderTreeView *self,TnyAccountStoreIface *iface)
{
	const GList *accounts;
	TnyAccountTreeModel *folder_model;
	ModestTnyFolderTreeViewPrivate *priv;
		
	g_return_val_if_fail (iface, FALSE);

	priv =	MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(self);
	priv->view_is_empty = TRUE;

	accounts = tny_account_store_iface_get_store_accounts (iface);
	if (!accounts) {
		g_warning ("no accounts have been defined yet");
		return update_model_empty (self);
	}
	
	folder_model = tny_account_tree_model_new ();
	if (!folder_model) {
		g_warning ("failed to get account tree model");
		return update_model_empty (self);
	}
	
	while (accounts) {
		TnyStoreAccountIface *account =
			TNY_STORE_ACCOUNT_IFACE(accounts->data);
		if (!account) {
			g_warning ("invalid account");
			g_object_unref (folder_model);
			return update_model_empty (self);
		}
		tny_account_tree_model_add (TNY_ACCOUNT_TREE_MODEL (folder_model),
					    account);
		accounts = accounts->next;
	}
	
 	gtk_tree_view_set_model (GTK_TREE_VIEW(self),
				 GTK_TREE_MODEL(folder_model)); 
	g_object_unref (G_OBJECT(folder_model));
	
	priv->view_is_empty = FALSE; /* were not empty anymore! */
	return TRUE;
}


void
selection_changed (GtkTreeSelection *sel, gpointer user_data)
{
	GtkTreeModel            *model;
	TnyMsgFolderIface       *folder = NULL;
	GtkTreeIter             iter;
	ModestTnyFolderTreeView *tree_view;
	ModestTnyFolderTreeViewPrivate *priv;

	g_return_if_fail (sel);
	g_return_if_fail (user_data);

	priv = MODEST_TNY_FOLDER_TREE_VIEW_GET_PRIVATE(user_data);

	/* is_empty means that there is only the 'empty' item */
	if (priv->view_is_empty)
		return;
	
	/* folder was _un_selected if true */
	if (!gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		if (priv->cur_folder) 
			tny_msg_folder_iface_expunge (priv->cur_folder);
		priv->cur_folder = NULL;
		return; 
	}

	tree_view = MODEST_TNY_FOLDER_TREE_VIEW (user_data);

	gtk_tree_model_get (model, &iter,
			    TNY_ACCOUNT_TREE_MODEL_INSTANCE_COLUMN,
			    &folder, -1);

	if (priv->cur_folder) 
		tny_msg_folder_iface_expunge (priv->cur_folder);
	priv->cur_folder = folder;

	/* folder will not be defined if you click eg. on the root node */
	if (folder)
		g_signal_emit (G_OBJECT(tree_view), signals[FOLDER_SELECTED_SIGNAL], 0,
			       folder);
}





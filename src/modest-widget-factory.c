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
#include <gdk/gdkkeysyms.h>
#include <tny-gtk-account-list-model.h>
#include <tny-gtk-folder-store-tree-model.h>
#include <tny-account-store.h>
#include <tny-simple-list.h>
#include <tny-device.h>
#include <tny-folder-store-query.h>
#include "modest-widget-factory.h"
#include "modest-widget-memory.h"
#include <modest-protocol-info.h>
#include "modest-tny-platform-factory.h"
#include "modest-account-mgr.h"
#include "modest-mail-operation.h"
#include "widgets/modest-header-view-priv.h"

/* 'private'/'protected' functions */
static void modest_widget_factory_class_init    (ModestWidgetFactoryClass *klass);
static void modest_widget_factory_init          (ModestWidgetFactory *obj);
static void modest_widget_factory_finalize      (GObject *obj);


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestWidgetFactoryPrivate ModestWidgetFactoryPrivate;
struct _ModestWidgetFactoryPrivate {
	
	TnyPlatformFactory          *fact;
	TnyAccountStore             *account_store;
	
	ModestHeaderView            *header_view;
	ModestFolderView            *folder_view;
	ModestMsgView               *msg_preview;

	GtkWidget                   *progress_bar;
	GtkWidget                   *status_bar;
	GtkWidget	            *folder_info_label;

	GtkWidget	            *online_toggle;
};
#define MODEST_WIDGET_FACTORY_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                   MODEST_TYPE_WIDGET_FACTORY, \
                                                   ModestWidgetFactoryPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_widget_factory_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestWidgetFactoryClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_widget_factory_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestWidgetFactory),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_widget_factory_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestWidgetFactory",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_widget_factory_class_init (ModestWidgetFactoryClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_widget_factory_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestWidgetFactoryPrivate));
}

static void
modest_widget_factory_init (ModestWidgetFactory *obj)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	priv->fact          = modest_tny_platform_factory_get_instance ();
	priv->account_store = tny_platform_factory_new_account_store (priv->fact);
	
	priv->progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(priv->progress_bar),
				       1.0);
	priv->status_bar   = gtk_statusbar_new ();
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR(priv->status_bar),
					   FALSE);
}


static void
modest_widget_factory_finalize (GObject *obj)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static gboolean
init_widgets (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	TnyFolderStoreQuery *query;

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	/* folder view */
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	if (!(priv->folder_view =
	      MODEST_FOLDER_VIEW(modest_folder_view_new (MODEST_TNY_ACCOUNT_STORE (priv->account_store),
							 query)))) {
		g_printerr ("modest: cannot instantiate folder view\n");
		return FALSE;
	}
	g_object_unref (G_OBJECT (query));

	/* header view */
	if (!(priv->header_view =
	      MODEST_HEADER_VIEW(modest_header_view_new (NULL,MODEST_HEADER_VIEW_STYLE_DETAILS)))) {
		g_printerr ("modest: cannot instantiate header view\n");
		return FALSE;
	}
		
	/* msg preview */
	if (!(priv->msg_preview = MODEST_MSG_VIEW(modest_msg_view_new (NULL)))) {
		g_printerr ("modest: cannot instantiate header view\n");
		return FALSE;
	}



	/* online/offline combo */
	priv->online_toggle = gtk_toggle_button_new ();

	/* label with number of items, unread items for 
	   the current folder */
	priv->folder_info_label = gtk_label_new (NULL);
	
/* 	init_signals (self); */
	
	return TRUE;
}


ModestWidgetFactory*
modest_widget_factory_new (void)
{
	GObject *obj;
	ModestWidgetFactoryPrivate *priv;

	obj  = g_object_new (MODEST_TYPE_WIDGET_FACTORY, NULL);	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);
	
	if (!init_widgets (MODEST_WIDGET_FACTORY(obj))) {
		g_printerr ("modest: widget factory failed to init widgets\n");
		g_object_unref (obj);
		return NULL;
	}
	
	return MODEST_WIDGET_FACTORY(obj);
}




ModestFolderView*
modest_widget_factory_get_folder_view (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->folder_view;
}


ModestHeaderView*
modest_widget_factory_get_header_view (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->header_view;
}


ModestMsgView*
modest_widget_factory_get_msg_preview (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->msg_preview;
}


ModestAccountView*
modest_widget_factory_get_account_view (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	ModestAccountMgr *account_mgr;
	
	g_return_val_if_fail (self, NULL);
	priv =  MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	account_mgr = modest_tny_platform_factory_get_account_mgr_instance
		(MODEST_TNY_PLATFORM_FACTORY(priv->fact));
	return modest_account_view_new (account_mgr);
}



GtkWidget*
modest_widget_factory_get_progress_bar (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->progress_bar;
}


GtkWidget*
modest_widget_factory_get_status_bar (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->status_bar;
}



static const GSList*
get_transports (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	ModestAccountMgr *account_mgr;
	GSList *transports = NULL;
	GSList *cursor, *accounts;
	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	account_mgr =
		modest_tny_platform_factory_get_account_mgr_instance
		(MODEST_TNY_PLATFORM_FACTORY(priv->fact));
	cursor = accounts = modest_account_mgr_account_names (account_mgr, NULL);
	while (cursor) {
		ModestAccountData *data;
		gchar *account_name = (gchar*)cursor->data;

		data = modest_account_mgr_get_account_data (account_mgr, account_name);
		if (data && data->transport_account) {
			gchar *display_name = g_strdup_printf ("%s (%s)", data->email, account_name);
			ModestPair *pair = modest_pair_new ((gpointer) data,
							    (gpointer) display_name , TRUE);
			transports = g_slist_append (transports, pair);
		}
		/* don't free account name; it's freed when the transports list is freed */
		cursor = cursor->next;
	}
	g_slist_free (accounts);
	
	return transports;
}


#if 0
static const GSList*
get_stores (ModestWidgetFactory *self, gboolean only_remote)
{
	ModestWidgetFactoryPrivate *priv;
	TnyAccountStore *account_store;
	TnyList *stores;
	TnyIterator *iter;
	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	account_store =
		tny_platform_factory_new_account_store (priv->fact);			

	stores = tny_simple_list_new ();
	tny_account_store_get_accounts (account_store, stores,
					 TNY_ACCOUNT_STORE_STORE_ACCOUNTS);

	/* simply return all the stores */
	if (!only_remote)
		return stores;

	/*  remove the non-remote stores from the list */
	if (only_remote) {
		iter = tny_list_create_iterator (stores);
		while (!tny_iterator_is_done (iter)) {
			TnyAccount *acc = (TnyAccount*)tny_iterator_get_current(iter);
			/* is it a local account? if so, remove */
			ModestProtocol proto = modest_protocol_info_get_protocol (tny_account_get_proto(acc));
			if (modest_protocol_info_protocol_is_local_store(proto))
				tny_list_remove (stores, acc); /* FIXME: iter still valid? */
			tny_iterator_next (iter);
		}
		g_object_unref (G_OBJECT(iter));
	}
	return stores;		
}
#endif

GtkWidget*
modest_widget_factory_get_combo_box (ModestWidgetFactory *self, ModestComboBoxType type)
{
	ModestWidgetFactoryPrivate *priv;
	ModestPairList *protos = NULL;
	GtkWidget* combo_box;
	
	g_return_val_if_fail (self, NULL);

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	switch (type) {
	case MODEST_COMBO_BOX_TYPE_STORE_PROTOS:
		protos = modest_protocol_info_get_protocol_pair_list (MODEST_PROTOCOL_TYPE_STORE);
		break;
	case MODEST_COMBO_BOX_TYPE_TRANSPORT_PROTOS:
		protos = modest_protocol_info_get_protocol_pair_list (MODEST_PROTOCOL_TYPE_TRANSPORT);
		break;
	case MODEST_COMBO_BOX_TYPE_SECURITY_PROTOS:
		protos = modest_protocol_info_get_protocol_pair_list (MODEST_PROTOCOL_TYPE_SECURITY);
		break;
	case MODEST_COMBO_BOX_TYPE_AUTH_PROTOS:
		protos = modest_protocol_info_get_protocol_pair_list (MODEST_PROTOCOL_TYPE_AUTH);
		break;
	case MODEST_COMBO_BOX_TYPE_TRANSPORTS:
		protos = (ModestPairList *) get_transports (self);
		break;
/* 	case MODEST_COMBO_BOX_TYPE_REMOTE_STORES: */
/* 		// FIXME */
/* 		list = get_stores (self, TRUE); /\* get all *remote* stores *\/ */
/* 		combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL(list)); */
/* 		g_object_unref (G_OBJECT(list)); */
/* 		//return combo_box; */
	default:
		g_warning ("invalid combo box type: %d", type);
		return NULL;
	}

	combo_box = modest_combo_box_new (protos);
	modest_pair_list_free (protos);
	
	gtk_combo_box_set_active (GTK_COMBO_BOX(combo_box), 0);
	
	return combo_box;
}



GtkWidget*
modest_widget_factory_get_online_toggle (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->online_toggle;
}



GtkWidget*
modest_widget_factory_get_folder_info_label (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->folder_info_label;
}


/*********************** Test code ********************/
/* static void */
/* on_folder_key_press_event (ModestFolderView *folder_view, GdkEventKey *event, gpointer user_data) */
/* { */
/* 	GtkTreeSelection *selection; */
/* 	GtkTreeModel *model; */
/* 	GtkTreeIter iter; */
/* 	TnyFolderStore *folder; */
/* 	gint type; */
/* 	ModestMailOperation *mail_op; */

/* 	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_view)); */
/* 	gtk_tree_selection_get_selected (selection, &model, &iter); */
	
/* 	gtk_tree_model_get (model, &iter,  */
/* 			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type,  */
/* 			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder,  */
/* 			    -1); */

/* 	mail_op = modest_mail_operation_new (); */

/* 	if (event->keyval == GDK_C || event->keyval == GDK_c) { */
/* 		if (type != TNY_FOLDER_TYPE_ROOT) */
/* 			modest_mail_operation_create_folder (mail_op, folder, "New"); */
/* 	} else if (event->keyval == GDK_D || event->keyval == GDK_d) { */
/* 		if (type != TNY_FOLDER_TYPE_ROOT) */
/* 			modest_mail_operation_remove_folder (mail_op, TNY_FOLDER (folder), FALSE); */
/* 	} else if (event->keyval == GDK_N || event->keyval == GDK_n) { */
/* 		if (type != TNY_FOLDER_TYPE_ROOT) */
/* 			modest_mail_operation_rename_folder (mail_op, TNY_FOLDER (folder), "New Name"); */
/* 	} else if (event->keyval == GDK_T || event->keyval == GDK_t) { */
/* 		if (type != TNY_FOLDER_TYPE_ROOT) */
/* 			modest_mail_operation_remove_folder (mail_op, TNY_FOLDER (folder), TRUE); */
/* 	} */

/* 	g_object_unref (G_OBJECT (mail_op)); */
/* } */

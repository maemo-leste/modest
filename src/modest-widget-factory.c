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

#include "modest-widget-factory.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_widget_factory_class_init    (ModestWidgetFactoryClass *klass);
static void modest_widget_factory_init          (ModestWidgetFactory *obj);
static void modest_widget_factory_finalize      (GObject *obj);

static void on_folder_clicked (ModestTnyFolderTreeView *folder_view, TnyMsgFolderIface *folder,
			       ModestWidgetFactory *self);
static void on_message_selected (ModestTnyFolderTreeView *folder_view, TnyMsgIface *msg,
				 ModestWidgetFactory *self);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestWidgetFactoryPrivate ModestWidgetFactoryPrivate;
struct _ModestWidgetFactoryPrivate {
	
	ModestTnyAccountStore *account_store;
	ModestAccountMgr      *account_mgr;
	ModestConf            *conf;
	
	ModestTnyHeaderTreeView *header_view;
	ModestTnyFolderTreeView *folder_view;
	ModestTnyMsgView        *msg_preview;
	ModestAccountView       *account_view;

	gboolean		auto_connect;
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

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_widget_factory_init (ModestWidgetFactory *obj)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	priv->conf          = NULL;
	priv->account_mgr   = NULL;
	priv->account_store = NULL;
}

static void
modest_widget_factory_finalize (GObject *obj)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	if (priv->account_mgr) {
		g_object_unref (G_OBJECT(priv->account_mgr));
		priv->account_mgr = NULL;
	}

	if (priv->conf) {
		g_object_unref (G_OBJECT(priv->conf));
		priv->conf = NULL;
	}

	if (priv->account_store) {
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}
}

ModestWidgetFactory*
modest_widget_factory_new (ModestConf *conf,
			   ModestTnyAccountStore *account_store,
			   ModestAccountMgr *account_mgr,
			   gboolean auto_connect)
{
	GObject *obj;
	ModestWidgetFactoryPrivate *priv;

	g_return_val_if_fail (account_store, NULL);
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (conf, NULL);
	
	obj  = g_object_new(MODEST_TYPE_WIDGET_FACTORY, NULL);	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	priv->auto_connect = auto_connect;

	g_object_ref (G_OBJECT(conf));
	priv->conf = conf;
	
	g_object_ref (G_OBJECT(account_mgr));
	priv->account_mgr = account_mgr;

	g_object_ref (G_OBJECT(account_store));
	priv->account_store = account_store;
	
	return MODEST_WIDGET_FACTORY(obj);
}




ModestTnyFolderTreeView*
modest_widget_factory_get_folder_tree_widget (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	
	g_return_val_if_fail (self, NULL);

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (!priv->folder_view) {
		priv->folder_view =
			MODEST_TNY_FOLDER_TREE_VIEW(modest_tny_folder_tree_view_new
						    (TNY_ACCOUNT_STORE_IFACE(priv->account_store)));
		if (priv->folder_view && priv->auto_connect)
			g_signal_connect (G_OBJECT(priv->folder_view), "folder_selected",
					  G_CALLBACK(on_folder_clicked), self);
	}
		
	if (!priv->folder_view)
		g_printerr ("modest: cannot instantiate folder view\n");
	
	return priv->folder_view;
}


ModestTnyHeaderTreeView*
modest_widget_factory_get_header_tree_widget (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	
	g_return_val_if_fail (self, NULL);

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (!priv->header_view) {
		priv->header_view =
			MODEST_TNY_HEADER_TREE_VIEW(modest_tny_header_tree_view_new
						    (NULL, NULL,
						     MODEST_TNY_HEADER_TREE_VIEW_STYLE_NORMAL));
		if (priv->header_view && priv->auto_connect)
			g_signal_connect (G_OBJECT(priv->header_view), "message_selected",
					  G_CALLBACK(on_message_selected), self);
	}
	
	if (!priv->header_view)
		g_printerr ("modest: cannot instantiate header view\n");
	
	return priv->header_view;
}


ModestTnyMsgView*
modest_widget_factory_get_msg_preview_widget (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	
	g_return_val_if_fail (self, NULL);

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (!priv->msg_preview)
		priv->msg_preview =
			MODEST_TNY_MSG_VIEW(modest_tny_msg_view_new (NULL));
	
	if (!priv->msg_preview)
		g_printerr ("modest: cannot instantiate header view\n");
	
	return priv->msg_preview;
}


static void
on_folder_clicked (ModestTnyFolderTreeView *folder_view, TnyMsgFolderIface *folder,
		   ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	modest_tny_header_tree_view_set_folder (priv->header_view, folder);
}


static void
on_message_selected (ModestTnyFolderTreeView *folder_view, TnyMsgIface *msg,
		     ModestWidgetFactory *self)
{	
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	modest_tny_msg_view_set_message (priv->msg_preview, msg);
}


ModestAccountView*
modest_widget_factory_get_account_view_widget (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	
	g_return_val_if_fail (self, NULL);

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (!priv->account_view)
		priv->account_view =
			MODEST_ACCOUNT_VIEW(modest_account_view_new (priv->account_mgr));

	if (!priv->account_view)
		g_printerr ("modest: cannot create account view widget\n");
	
	return priv->account_view;	
}

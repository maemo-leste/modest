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

static void connect_widgets (ModestWidgetFactory *self);
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

	ModestTnyHeaderTreeView *header_view;
	ModestTnyFolderTreeView *folder_view;
	ModestTnyMsgView        *msg_preview;
	
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

}

static void
modest_widget_factory_finalize (GObject *obj)
{

}

ModestWidgetFactory*
modest_widget_factory_new (ModestTnyAccountStore *account_store, gboolean autoconnect)
{
	GObject *obj;
	ModestWidgetFactoryPrivate *priv;

	g_return_val_if_fail (account_store, NULL);

	obj  = g_object_new(MODEST_TYPE_WIDGET_FACTORY, NULL);	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	priv->folder_view =
		MODEST_TNY_FOLDER_TREE_VIEW(modest_tny_folder_tree_view_new
					    (TNY_ACCOUNT_STORE_IFACE(account_store)));
	if (!priv->folder_view) {
		g_printerr ("modest: cannot instantiate folder view\n");
		g_object_unref (obj);
		return NULL;
	}
	
	priv->header_view =
		MODEST_TNY_HEADER_TREE_VIEW(modest_tny_header_tree_view_new
					    (NULL, NULL, MODEST_TNY_HEADER_TREE_VIEW_STYLE_NORMAL));
	if (!priv->header_view) {
		g_printerr ("modest: cannot instantiate header view\n");
		g_object_unref (obj);
		return NULL;
	}

	priv->msg_preview =
		MODEST_TNY_MSG_VIEW(modest_tny_msg_view_new (NULL));
	if (!priv->msg_preview) {
		g_printerr ("modest: cannot instantiate msg preview\n");
		g_object_unref (obj);
		return NULL;
	}

	if (autoconnect)
		connect_widgets (MODEST_WIDGET_FACTORY(obj));

	return MODEST_WIDGET_FACTORY(obj);
}




ModestTnyFolderTreeView*
modest_widget_factory_get_folder_tree_widget (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);

	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->folder_view;
	
}


ModestTnyHeaderTreeView*
modest_widget_factory_get_header_tree_widget (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->header_view;
}


ModestTnyMsgView*
modest_widget_factory_get_msg_preview_widget (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);

	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->msg_preview;
}



static void
connect_widgets (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;

	g_return_if_fail (self);

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	/* folder view signals */
	g_signal_connect (G_OBJECT(priv->folder_view), "folder_selected",
 			  G_CALLBACK(on_folder_clicked), self);
	/* header view signals */
	g_signal_connect (G_OBJECT(priv->header_view), "message_selected",
			  G_CALLBACK(on_message_selected), self);
//	g_signal_connect (G_OBJECT(priv->header_view), "row-activated",
//			  G_CALLBACK(on_header_activated), self);
//	g_signal_connect (G_OBJECT(header_view), "status_update",
//			  G_CALLBACK(on_headers_status_update), self);
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


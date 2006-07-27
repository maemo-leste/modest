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
#include <tny-account-store-iface.h>
#include <tny-device-iface.h>

/* 'private'/'protected' functions */
static void modest_widget_factory_class_init    (ModestWidgetFactoryClass *klass);
static void modest_widget_factory_init          (ModestWidgetFactory *obj);
static void modest_widget_factory_finalize      (GObject *obj);


/* callbacks */
static void on_folder_selected         (ModestFolderView *folder_view,
					TnyMsgFolderIface *folder,
					ModestWidgetFactory *self);
static void on_message_selected        (ModestHeaderView *header_view, TnyMsgIface *msg,
					ModestWidgetFactory *self);
static void on_header_status_update    (ModestHeaderView *header_view, const gchar *msg,
					gint status_id, ModestWidgetFactory *self);
static void on_msg_link_hover          (ModestMsgView *msgview, const gchar* link,
					ModestWidgetFactory *self);
static void on_msg_link_clicked        (ModestMsgView *msgview, const gchar* link,
					ModestWidgetFactory *self);
static void on_msg_attachment_clicked  (ModestMsgView *msgview, int index,
					ModestWidgetFactory *self);

static void on_connection_changed (TnyDeviceIface *device, gboolean online,
				   ModestWidgetFactory *self);
static void on_online_toggle_toggled (GtkToggleButton *toggle, ModestWidgetFactory *factory);


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};


enum _StatusID {
	STATUS_ID_HEADER,
	STATUS_ID_FOLDER,
	STATUS_ID_MESSAGE,

	STATUS_ID_NUM
};
typedef enum _StatusID StatusID;

typedef struct _ModestWidgetFactoryPrivate ModestWidgetFactoryPrivate;
struct _ModestWidgetFactoryPrivate {
	
	ModestTnyAccountStore *account_store;
	ModestAccountMgr      *account_mgr;
	ModestConf            *conf;
	
	ModestHeaderView      *header_view;
	ModestFolderView      *folder_view;
	ModestMsgView         *msg_preview;

	GtkWidget             *progress_bar;
	GtkWidget             *status_bar;

	GtkWidget	      *online_toggle;
	StatusID              status_bar_ctx[STATUS_ID_NUM];
	
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
}

static void
modest_widget_factory_init (ModestWidgetFactory *obj)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	priv->conf          = NULL;
	priv->account_mgr   = NULL;
	priv->account_store = NULL;
	
	priv->progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(priv->progress_bar),
				       1.0);
	priv->status_bar   = gtk_statusbar_new ();
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR(priv->status_bar),
					   FALSE);
	
	priv->status_bar_ctx[STATUS_ID_HEADER] =
		gtk_statusbar_get_context_id (GTK_STATUSBAR(priv->status_bar),
					      "header");
	priv->status_bar_ctx[STATUS_ID_MESSAGE] =
		gtk_statusbar_get_context_id (GTK_STATUSBAR(priv->status_bar),
					      "message");	
	priv->status_bar_ctx[STATUS_ID_FOLDER] =
		gtk_statusbar_get_context_id (GTK_STATUSBAR(priv->status_bar),
					      "folder");
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



static void
init_signals (ModestWidgetFactory *self)
{
	
	TnyDeviceIface *device;
	ModestWidgetFactoryPrivate *priv;
	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	g_signal_connect (G_OBJECT(priv->header_view), "message_selected",
			  G_CALLBACK(on_message_selected), self);
	g_signal_connect (G_OBJECT(priv->folder_view), "folder_selected",
			  G_CALLBACK(on_folder_selected), self);
	g_signal_connect (G_OBJECT(priv->header_view), "status_update",
			  G_CALLBACK(on_header_status_update), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "link_clicked",
			  G_CALLBACK(on_msg_link_clicked), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "link_hover",
			  G_CALLBACK(on_msg_link_hover), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "attachment_clicked",
			  G_CALLBACK(on_msg_attachment_clicked), self);
	
	/* FIXME: const casting is evil ==> tinymail */
	device = (TnyDeviceIface*)tny_account_store_iface_get_device
		(TNY_ACCOUNT_STORE_IFACE(priv->account_store));
	if (device) {
		g_signal_connect (G_OBJECT(device), "connection_changed",
				  G_CALLBACK(on_connection_changed), self);
		g_signal_connect (G_OBJECT(priv->online_toggle), "toggled",
				  G_CALLBACK(on_online_toggle_toggled), self);
		
		/* init toggle in correct state */
		on_connection_changed (device,
				       tny_device_iface_is_online (device),
				       self);
	}
}

static gboolean
init_widgets (ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	/* folder view */
	if (!(priv->folder_view =
	      MODEST_FOLDER_VIEW(modest_folder_view_new
				 (TNY_ACCOUNT_STORE_IFACE(priv->account_store))))) {
		g_printerr ("modest: cannot instantiate folder view\n");
		return FALSE;
	}

	/* header view */
	if (!(priv->header_view =
	      MODEST_HEADER_VIEW(modest_header_view_new
				 (NULL, NULL,MODEST_HEADER_VIEW_STYLE_NORMAL)))) {
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
	
	init_signals (self);
	
	return TRUE;
}


ModestWidgetFactory*
modest_widget_factory_new (ModestConf *conf,
			   ModestTnyAccountStore *account_store,
			   ModestAccountMgr *account_mgr)
{
	GObject *obj;
	ModestWidgetFactoryPrivate *priv;

	g_return_val_if_fail (account_store, NULL);
	g_return_val_if_fail (account_mgr, NULL);
	g_return_val_if_fail (conf, NULL);
	
	obj  = g_object_new(MODEST_TYPE_WIDGET_FACTORY, NULL);	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(obj);

	g_object_ref (G_OBJECT(conf));
	priv->conf = conf;
	
	g_object_ref (G_OBJECT(account_mgr));
	priv->account_mgr = account_mgr;
	
	g_object_ref (G_OBJECT(account_store));
	priv->account_store = account_store;

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
	
	g_return_val_if_fail (self, NULL);
	priv =  MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	return modest_account_view_new (priv->account_mgr);
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


GtkWidget*
modest_widget_factory_get_combo_box (ModestWidgetFactory *self, ModestComboBoxType type)
{
	GtkWidget *combo_box;
	GtkListStore *model;
	GtkTreeIter iter;
	const gchar **protos, **cursor; 

	g_return_val_if_fail (self, NULL);

	combo_box = gtk_combo_box_new_text ();
	
	switch (type) {
	case MODEST_COMBO_BOX_TYPE_STORE_PROTOS:
		cursor = protos = modest_proto_store_protos ();
		break;
	case MODEST_COMBO_BOX_TYPE_TRANSPORT_PROTOS:
		cursor = protos = modest_proto_transport_protos ();
		break;
	case MODEST_COMBO_BOX_TYPE_SECURITY_PROTOS:
		cursor = protos = modest_proto_security_protos ();
		break;
	case MODEST_COMBO_BOX_TYPE_AUTH_PROTOS:
		cursor = protos = modest_proto_auth_protos ();
		break;
	default:
		g_assert_not_reached ();
	}
	while (cursor && *cursor) {
		gtk_combo_box_append_text (GTK_COMBO_BOX(combo_box),
					   (const gchar*)*cursor);
		++cursor;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX(combo_box), 0);
	
	return combo_box;
}




GtkWidget*
modest_widget_factory_get_online_toggle (ModestWidgetFactory *self)
{
	g_return_val_if_fail (self, NULL);
	return MODEST_WIDGET_FACTORY_GET_PRIVATE(self)->online_toggle;
}



static void
on_folder_selected (ModestFolderView *folder_view, TnyMsgFolderIface *folder,
		    ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	modest_header_view_set_folder (priv->header_view, folder);
}


static void
on_message_selected (ModestHeaderView *folder_view, TnyMsgIface *msg,
		     ModestWidgetFactory *self)
{	
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	modest_msg_view_set_message (priv->msg_preview, msg);
}


static void
on_header_status_update (ModestHeaderView *header_view, const gchar *msg,
			 gint status_id, ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	if (msg && status_id) {
		gchar *msg = g_strdup_printf ("%s", msg);
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR(priv->progress_bar));
		gtk_statusbar_push (GTK_STATUSBAR(priv->status_bar),
				       priv->status_bar_ctx[STATUS_ID_HEADER],
				       msg);
		g_free (msg);
	} else {
		gtk_statusbar_pop (GTK_STATUSBAR(priv->status_bar),
				   priv->status_bar_ctx[STATUS_ID_HEADER]);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(priv->progress_bar),
					       1.0);
	}
}


static void
on_msg_link_hover (ModestMsgView *msgview, const gchar* link,
		   ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (link)
		gtk_statusbar_push (GTK_STATUSBAR(priv->status_bar),
				    priv->status_bar_ctx[STATUS_ID_MESSAGE],
				    link);
	else
		gtk_statusbar_pop (GTK_STATUSBAR(priv->status_bar),
				   priv->status_bar_ctx[STATUS_ID_MESSAGE]);

}	


static void
on_msg_link_clicked  (ModestMsgView *msgview, const gchar* link,
		      ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (link) {
		gchar *msg = g_strdup_printf (_("Opening %s..."), link);
		gtk_statusbar_push (GTK_STATUSBAR(priv->status_bar),
				    priv->status_bar_ctx[STATUS_ID_MESSAGE],
				    msg);
		g_free (msg);
	}	
}

static void
on_msg_attachment_clicked  (ModestMsgView *msgview, int index,
			    ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	gchar *msg = g_strdup_printf (_("Opening attachment %d..."), index);
	gtk_statusbar_push (GTK_STATUSBAR(priv->status_bar),
			    priv->status_bar_ctx[STATUS_ID_MESSAGE],
			    msg);
	g_free (msg);
}


static void
on_connection_changed (TnyDeviceIface *device, gboolean online,
		       ModestWidgetFactory *self)
{
	gint item;
	ModestWidgetFactoryPrivate *priv;

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->online_toggle),
				      online);
	gtk_button_set_label (GTK_BUTTON(priv->online_toggle),
			      online ? _("Online") : _("Offline"));
}


static void
on_online_toggle_toggled (GtkToggleButton *toggle, ModestWidgetFactory *self)
{
	gboolean online;
	const TnyDeviceIface *device;
	ModestWidgetFactoryPrivate *priv;
	
	priv    = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	online  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->online_toggle));
	device  = tny_account_store_iface_get_device
		(TNY_ACCOUNT_STORE_IFACE(priv->account_store)); 

	/* FIXME: const casting should not be necessary ==> tinymail */
	if (online)  /* we're moving to online state */
		tny_device_iface_force_online ((TnyDeviceIface*)device);
	else  /* we're moving to offline state */
		tny_device_iface_force_offline ((TnyDeviceIface*)device);
}

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
#include "modest-widget-factory.h"
#include <modest-widget-memory.h>
#include <modest-protocol-mgr.h>
#include <tny-gtk-account-list-model.h>
#include <tny-gtk-folder-store-tree-model.h>
#include <tny-account-store.h>
#include <tny-device.h>
#include <tny-folder-store-query.h>

#include "modest-tny-platform-factory.h"
#include "modest-account-mgr.h"
#include "modest-mail-operation.h"


/* 'private'/'protected' functions */
static void modest_widget_factory_class_init    (ModestWidgetFactoryClass *klass);
static void modest_widget_factory_init          (ModestWidgetFactory *obj);
static void modest_widget_factory_finalize      (GObject *obj);


/* callbacks */
static void on_folder_selection_changed         (ModestFolderView *folder_view,
						 TnyFolder *folder, gboolean selected,
						 ModestWidgetFactory *self);
static void on_folder_key_press_event  (ModestFolderView *header_view, 
				        GdkEventKey *event, 
				        gpointer user_data);
static void on_message_selected        (ModestHeaderView *header_view, TnyMsg *msg,
					ModestWidgetFactory *self);
static void on_header_status_update    (ModestHeaderView *header_view, const gchar *msg,
					gint num, gint total, ModestWidgetFactory *self);
static void on_msg_link_hover          (ModestMsgView *msgview, const gchar* link,
					ModestWidgetFactory *self);
static void on_msg_link_clicked        (ModestMsgView *msgview, const gchar* link,
					ModestWidgetFactory *self);
static void on_msg_attachment_clicked  (ModestMsgView *msgview, int index,
					ModestWidgetFactory *self);

static void on_connection_changed (TnyDevice *device, gboolean online,
				   ModestWidgetFactory *self);
static void on_online_toggle_toggled (GtkToggleButton *toggle, ModestWidgetFactory *factory);

static void on_password_requested (ModestTnyAccountStore *account_store, const gchar* account_name,
				   gchar **password, gboolean *cancel, ModestWidgetFactory *self);

static void on_item_not_found     (ModestHeaderView* header_view, ModestItemType type,
				   ModestWidgetFactory *self);


/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestWidgetFactoryPrivate ModestWidgetFactoryPrivate;
struct _ModestWidgetFactoryPrivate {
	
	TnyPlatformFactory          *fact;
	ModestProtocolMgr           *proto_mgr;
	TnyAccountStore             *account_store;
	
	ModestHeaderView            *header_view;
	ModestFolderView            *folder_view;
	ModestMsgView               *msg_preview;
	ModestToolbar               *main_toolbar, *edit_toolbar;

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
	priv->proto_mgr     = modest_protocol_mgr_new ();
	
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

	if (priv->proto_mgr) {
		g_object_unref (G_OBJECT(priv->proto_mgr));
		priv->proto_mgr = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



/* FIXME: uninit these as well */
static void
init_signals (ModestWidgetFactory *self)
{
	TnyDevice *device;
	ModestWidgetFactoryPrivate *priv;
	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	/* folder view */
	g_signal_connect (G_OBJECT(priv->folder_view), "folder_selection_changed",
			  G_CALLBACK(on_folder_selection_changed), self);
	g_signal_connect (G_OBJECT(priv->folder_view), "key-press-event",
			  G_CALLBACK(on_folder_key_press_event), self);

	/* header view */
	g_signal_connect (G_OBJECT(priv->header_view), "status_update",
			  G_CALLBACK(on_header_status_update), self);
	g_signal_connect (G_OBJECT(priv->header_view), "message_selected",
			  G_CALLBACK(on_message_selected), self);
	g_signal_connect (G_OBJECT(priv->header_view), "item_not_found",
			  G_CALLBACK(on_item_not_found), self);

	
	/* msg preview */
	g_signal_connect (G_OBJECT(priv->msg_preview), "link_clicked",
			  G_CALLBACK(on_msg_link_clicked), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "link_hover",
			  G_CALLBACK(on_msg_link_hover), self);
	g_signal_connect (G_OBJECT(priv->msg_preview), "attachment_clicked",
			  G_CALLBACK(on_msg_attachment_clicked), self);

	/* account store */	
	g_signal_connect (G_OBJECT (priv->account_store), "password_requested",
			  G_CALLBACK(on_password_requested), self);	

	/* FIXME: const casting is evil ==> tinymail */
	device = (TnyDevice*) tny_account_store_get_device (priv->account_store);
	if (device) {
		g_signal_connect (G_OBJECT(device), "connection_changed",
				  G_CALLBACK(on_connection_changed), self);
		g_signal_connect (G_OBJECT(priv->online_toggle), "toggled",
				  G_CALLBACK(on_online_toggle_toggled), self);
		
		/* init toggle in correct state */
		on_connection_changed (device,
				       tny_device_is_online (device),
				       self);
	}
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

	/* label with number of items, unread items for 
	   the current folder */
	priv->folder_info_label = gtk_label_new (NULL);
	
	init_signals (self);
	
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

	account_mgr = 
		modest_tny_platform_factory_get_modest_account_mgr_instance (priv->fact);
	
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
		modest_tny_platform_factory_get_modest_account_mgr_instance (priv->fact);
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


GtkWidget*
modest_widget_factory_get_combo_box (ModestWidgetFactory *self, ModestComboBoxType type)
{
	ModestWidgetFactoryPrivate *priv;
	const GSList *list = NULL;
	GtkWidget* combo_box;
	
	g_return_val_if_fail (self, NULL);

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	switch (type) {
	case MODEST_COMBO_BOX_TYPE_STORE_PROTOS:
		list = modest_protocol_mgr_get_store_protocols (priv->proto_mgr);
		break;
	case MODEST_COMBO_BOX_TYPE_TRANSPORT_PROTOS:
		list = modest_protocol_mgr_get_transport_protocols (priv->proto_mgr);
		break;
	case MODEST_COMBO_BOX_TYPE_SECURITY_PROTOS:
		list = modest_protocol_mgr_get_security_protocols (priv->proto_mgr);
		break;
	case MODEST_COMBO_BOX_TYPE_AUTH_PROTOS:
		list = modest_protocol_mgr_get_auth_protocols (priv->proto_mgr);
		break;
	case MODEST_COMBO_BOX_TYPE_TRANSPORTS:
		list = get_transports (self);
		break;
	default:
		g_warning ("invalid combo box type: %d", type);
		return NULL;
	}

	combo_box = modest_combo_box_new (list);
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

ModestToolbar*
modest_widget_factory_get_main_toolbar (ModestWidgetFactory *self, 
					GSList *items)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (priv->main_toolbar)
		return priv->main_toolbar;

	priv->main_toolbar = modest_toolbar_new (items);
	if (!priv->main_toolbar) {
		g_printerr ("modest: failed to create main toolbar\n");
		return NULL;
	}
	
	return priv->main_toolbar;
}


ModestToolbar*
modest_widget_factory_get_edit_toolbar (ModestWidgetFactory *self, 
					GSList *items)
{
	return modest_toolbar_new (items);
}



static void
on_folder_selection_changed (ModestFolderView *folder_view, TnyFolder *folder,
			     gboolean selected,
			     ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	gchar *txt;	

	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (folder) {
		ModestConf *conf;
		
		conf = modest_tny_platform_factory_get_modest_conf_instance (priv->fact);

		if (!selected) { /* the folder was unselected; save it's settings  */
			modest_widget_memory_save (conf, G_OBJECT (priv->header_view),
						   "header-view");
		} else {  /* the folder was selected */
			guint num, unread;
			num    = tny_folder_get_all_count    (folder);
			unread = tny_folder_get_unread_count (folder);
			
			txt = g_strdup_printf (_("%d %s, %d unread"),
					       num, num==1 ? _("item") : _("items"), unread);		
			gtk_label_set_label (GTK_LABEL(priv->folder_info_label), txt);
			g_free (txt);
			
			modest_header_view_set_folder (priv->header_view, folder);
			modest_widget_memory_restore (conf, G_OBJECT(priv->header_view),
							      "header-view");
		}
	} else
		gtk_label_set_label (GTK_LABEL(priv->folder_info_label), "");
}

/*********************** Test code ********************/
static void
on_folder_key_press_event (ModestFolderView *folder_view, GdkEventKey *event, gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	TnyFolderStore *folder;
	gint type;
	ModestMailOperation *mail_op;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (folder_view));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	
	gtk_tree_model_get (model, &iter, 
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN, &type, 
			    TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN, &folder, 
			    -1);

	mail_op = modest_mail_operation_new ();

	if (event->keyval == GDK_C || event->keyval == GDK_c) {
		if (type != TNY_FOLDER_TYPE_ROOT)
			modest_mail_operation_create_folder (mail_op, folder, "New");
	} else if (event->keyval == GDK_D || event->keyval == GDK_d) {
		if (type != TNY_FOLDER_TYPE_ROOT)
			modest_mail_operation_remove_folder (mail_op, TNY_FOLDER (folder), FALSE);
	} else if (event->keyval == GDK_N || event->keyval == GDK_n) {
		if (type != TNY_FOLDER_TYPE_ROOT)
			modest_mail_operation_rename_folder (mail_op, TNY_FOLDER (folder), "New Name");
	} else if (event->keyval == GDK_T || event->keyval == GDK_t) {
		if (type != TNY_FOLDER_TYPE_ROOT)
			modest_mail_operation_remove_folder (mail_op, TNY_FOLDER (folder), TRUE);
	}

	g_object_unref (G_OBJECT (mail_op));
}
/****************************************************/

static void
on_message_selected (ModestHeaderView *folder_view, TnyMsg *msg,
		     ModestWidgetFactory *self)
{	
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	modest_msg_view_set_message (priv->msg_preview, msg);
}


/*
 * below some stuff to clearup statusbar messages after 1,5 seconds....
 */
typedef struct {
	GtkWidget *status_bar;
	GtkWidget *progress_bar;
	guint     msg_id;
} StatusRemoveData;


static gboolean
on_statusbar_remove_msg (StatusRemoveData *data)
{
	/* we need to test types, as this callback maybe called after the
	 * widgets have been destroyed
	 */
	if (GTK_IS_STATUSBAR(data->status_bar)) 
		gtk_statusbar_remove (GTK_STATUSBAR(data->status_bar),
				      0, data->msg_id);
	if (GTK_IS_PROGRESS_BAR(data->progress_bar))
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(data->progress_bar),
					       1.0);
	g_free (data);
	return FALSE;
}


static void
statusbar_push (ModestWidgetFactory *self, guint context_id, const gchar *msg)
{
	guint id;
	StatusRemoveData *data;
	ModestWidgetFactoryPrivate *priv;
	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	if (!msg)
		return;
	
	id = gtk_statusbar_push (GTK_STATUSBAR(priv->status_bar), 0, msg);

	data = g_new (StatusRemoveData, 1);
	data->status_bar   = priv->status_bar;
	data->progress_bar = priv->progress_bar;
	data->msg_id     = id;

	g_timeout_add (1500, (GSourceFunc)on_statusbar_remove_msg, data);
}
/****************************************************************************/


static void
on_header_status_update (ModestHeaderView *header_view, const gchar *msg,
			 gint num, gint total, ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	if (total != 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(priv->progress_bar),
					       (gdouble)num/(gdouble)total);
	else
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR(priv->progress_bar));

	statusbar_push (self, 0, msg);
}


static void
on_msg_link_hover (ModestMsgView *msgview, const gchar* link,
		   ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	
	statusbar_push (self, 0, link);

}	


static void
on_msg_link_clicked  (ModestMsgView *msgview, const gchar* link,
		      ModestWidgetFactory *self)
{
	gchar *msg;
	msg = g_strdup_printf (_("Opening %s..."), link);
	statusbar_push (self, 0, msg);
	g_free (msg);
}

static void
on_msg_attachment_clicked  (ModestMsgView *msgview, int index,
			    ModestWidgetFactory *self)
{
	gchar *msg;
	
	msg = g_strdup_printf (_("Opening attachment %d..."), index);
	statusbar_push (self, 0, msg);
	
	g_free (msg);
}


static void
on_connection_changed (TnyDevice *device, gboolean online,
		       ModestWidgetFactory *self)
{
	ModestWidgetFactoryPrivate *priv;
	priv = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->online_toggle),
				      online);
	gtk_button_set_label (GTK_BUTTON(priv->online_toggle),
			      online ? _("Online") : _("Offline"));

	statusbar_push (self, 0, online ? _("Modest went online") : _("Modest went offline"));
}


static void
on_online_toggle_toggled (GtkToggleButton *toggle, ModestWidgetFactory *self)
{
	gboolean online;
	const TnyDevice *device;
	ModestWidgetFactoryPrivate *priv;
	
	priv    = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	online  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->online_toggle));
	device  = tny_account_store_get_device (priv->account_store); 

	/* FIXME: const casting should not be necessary ==> tinymail */
	if (online)  /* we're moving to online state */
		tny_device_force_online ((TnyDevice*)device);
	else  /* we're moving to offline state */
		tny_device_force_offline ((TnyDevice*)device);
}


static void
on_item_not_found (ModestHeaderView* header_view, ModestItemType type,
		   ModestWidgetFactory *self)
{
	/* FIXME ==> ask from UI... */
	GtkWidget *dialog;
	gchar *txt;
	gboolean online;
	gchar *item = type == MODEST_ITEM_TYPE_FOLDER ? "folder" : "message";
	
	TnyDevice *device;
	ModestWidgetFactoryPrivate *priv;
	
	priv    = MODEST_WIDGET_FACTORY_GET_PRIVATE(self);
	device  = tny_account_store_get_device (priv->account_store);
	
	online = tny_device_is_online (device);
	if (online) {
		/* already online -- the item is simply not there... */
		dialog = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_OK,
						 _("The %s you selected cannot be found"),
						 item);
		gtk_dialog_run (GTK_DIALOG(dialog));
	} else {

		dialog = gtk_dialog_new_with_buttons (_("Connection requested"),
						      NULL,
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_REJECT,
						      GTK_STOCK_OK,
						      GTK_RESPONSE_ACCEPT,
						      NULL);

		txt = g_strdup_printf (_("This %s is not available in offline mode.\n"
					 "Do you want to get online?"), item);
		gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), 
				    gtk_label_new (txt), FALSE, FALSE, 0);
		gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
		g_free (txt);

		gtk_window_set_default_size (GTK_WINDOW(dialog), 300, 300);
		if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
			tny_device_force_online (device);
	}
	gtk_widget_destroy (dialog);
}



static void
on_password_requested (ModestTnyAccountStore *account_store, const gchar* account_name,
		       gchar **password, gboolean *cancel, ModestWidgetFactory *self)
{
	gchar *txt;
	GtkWidget *dialog, *entry, *remember_pass_check;
	
	dialog = gtk_dialog_new_with_buttons (_("Password requested"),
					      NULL,
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_REJECT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_ACCEPT,
					      NULL);

	txt = g_strdup_printf (_("Please enter your password for %s"), account_name);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), gtk_label_new(txt),
			    FALSE, FALSE, 0);
	g_free (txt);

	entry = gtk_entry_new_with_max_length (40);
	gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(entry), 0x2022); /* bullet unichar */
	
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), entry,
			    TRUE, FALSE, 0);	

	remember_pass_check = gtk_check_button_new_with_label (_("Remember password"));
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), remember_pass_check,
			    TRUE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET(GTK_DIALOG(dialog)->vbox));
	
	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		*password = g_strdup(gtk_entry_get_text (GTK_ENTRY(entry)));
		*cancel   = FALSE;
	} else {
		*password = NULL;
		*cancel   = TRUE;
	}
	gtk_widget_destroy (dialog);
}


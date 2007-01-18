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
#include <tny-account-store.h>
#include <widgets/modest-edit-msg-window.h>
#include "modest-icon-names.h"
#include "modest-icon-factory.h"
#include "modest-widget-memory.h"
#include "modest-mail-operation.h"
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg-actions.h"
#include <tny-simple-list.h>

static void  modest_edit_msg_window_class_init   (ModestEditMsgWindowClass *klass);
static void  modest_edit_msg_window_init         (ModestEditMsgWindow *obj);
static void  modest_edit_msg_window_finalize     (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestEditMsgWindowPrivate ModestEditMsgWindowPrivate;
struct _ModestEditMsgWindowPrivate {

	ModestWidgetFactory *widget_factory;
	TnyPlatformFactory *fact;
	GtkUIManager *ui_manager;
	TnyAccountStore *account_store;
	
	GtkWidget      *toolbar, *menubar;
	GtkWidget      *msg_body;
	GtkWidget      *from_field, *to_field, *cc_field, *bcc_field,
		       *subject_field;
};

#define MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_EDIT_MSG_WINDOW, \
                                                    ModestEditMsgWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_edit_msg_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestEditMsgWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_edit_msg_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestEditMsgWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_edit_msg_window_init,
			NULL
		};
		my_type = g_type_register_static (HILDON_TYPE_WINDOW,
		                                  "ModestEditMsgWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_edit_msg_window_class_init (ModestEditMsgWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_edit_msg_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestEditMsgWindowPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_edit_msg_window_init (ModestEditMsgWindow *obj)
{
	ModestEditMsgWindowPrivate *priv;
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	priv->fact = modest_tny_platform_factory_get_instance ();
	priv->widget_factory = NULL;
	priv->toolbar = NULL;
	priv->menubar = NULL;
	priv->ui_manager = NULL;
	priv->account_store = NULL;
}



static void
save_settings (ModestEditMsgWindow *self)
{
	ModestEditMsgWindowPrivate *priv;
	ModestConf *conf;

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_conf_instance
		(MODEST_TNY_PLATFORM_FACTORY(priv->fact));

	modest_widget_memory_save (conf, G_OBJECT(self), "modest-edit-msg-window");
}


static void
restore_settings (ModestEditMsgWindow *self)
{
	ModestEditMsgWindowPrivate *priv;
	ModestConf *conf;

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_conf_instance
		(MODEST_TNY_PLATFORM_FACTORY(priv->fact));

	modest_widget_memory_restore (conf, G_OBJECT(self), "modest-edit-msg-window");
}


static void
init_window (ModestEditMsgWindow *obj)
{
	GtkWidget *to_button, *cc_button, *bcc_button; 
	GtkWidget *header_table;
	GtkWidget *main_vbox;
	
	ModestEditMsgWindowPrivate *priv;
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	to_button     = gtk_button_new_with_label (_("To..."));
	cc_button     = gtk_button_new_with_label (_("Cc..."));
	bcc_button    = gtk_button_new_with_label (_("Bcc..."));

	priv->from_field    = modest_widget_factory_get_combo_box (priv->widget_factory,
								   MODEST_COMBO_BOX_TYPE_TRANSPORTS);
	priv->to_field      = gtk_entry_new_with_max_length (80);
	priv->cc_field      = gtk_entry_new_with_max_length (80);
	priv->bcc_field     = gtk_entry_new_with_max_length (80);
	priv->subject_field = gtk_entry_new_with_max_length (80);
	
	header_table = gtk_table_new (5,2, FALSE);
	
	gtk_table_attach (GTK_TABLE(header_table), gtk_label_new (_("From:")),
			  0,1,0,1, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), to_button,     0,1,1,2, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), cc_button,     0,1,2,3, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), bcc_button,    0,1,3,4, GTK_SHRINK, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(header_table), gtk_label_new (_("Subject:")),
			  0,1,4,5, GTK_SHRINK, 0, 0, 0);

	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->from_field,   1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->to_field,     1,2,1,2);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->cc_field,     1,2,2,3);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->bcc_field,    1,2,3,4);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->subject_field,1,2,4,5);

	priv->msg_body = gtk_text_view_new ();
	
	main_vbox = gtk_vbox_new  (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(main_vbox), priv->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), header_table, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->msg_body, TRUE, TRUE, 6);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
}
	


static void
modest_edit_msg_window_finalize (GObject *obj)
{
	ModestEditMsgWindowPrivate *priv;

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	if (priv->widget_factory) {
		g_object_unref (G_OBJECT(priv->widget_factory));
		priv->widget_factory = NULL;
	}
	if (priv->ui_manager) {
		g_object_unref (G_OBJECT(priv->ui_manager));
		priv->ui_manager = NULL;
	}
	if (priv->account_store) {
		g_object_unref (G_OBJECT(priv->account_store));
		priv->account_store = NULL;
	}
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, ModestEditMsgWindow *self)
{
	save_settings (self);
	return FALSE;
}


ModestWindow*
modest_edit_msg_window_new (ModestWidgetFactory *factory,
			    TnyAccountStore *account_store,
			    ModestEditType type)
{
	GObject *obj;
	ModestEditMsgWindowPrivate *priv;

	g_return_val_if_fail (factory, NULL);
	g_return_val_if_fail (type < MODEST_EDIT_TYPE_NUM, NULL);
	
	obj = g_object_new(MODEST_TYPE_EDIT_MSG_WINDOW, NULL);
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	priv->widget_factory = g_object_ref (factory);
	//priv->ui_manager = g_object_ref (ui_manager);

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (priv->ui_manager));


	/* Toolbar / Menubar */
	priv->toolbar = gtk_ui_manager_get_widget (priv->ui_manager, "/EditMsgWindowToolBar");
	priv->menubar = gtk_ui_manager_get_widget (priv->ui_manager, "/EditMsgWindowMenuBar");

	gtk_toolbar_set_tooltips (GTK_TOOLBAR (priv->toolbar), TRUE);

	/* Init window */
	init_window (MODEST_EDIT_MSG_WINDOW(obj));

	restore_settings (MODEST_EDIT_MSG_WINDOW(obj));
	
	gtk_window_set_title (GTK_WINDOW(obj), "Modest");
	gtk_window_set_icon  (GTK_WINDOW(obj),
			      modest_icon_factory_get_icon (MODEST_APP_ICON));

	g_signal_connect (G_OBJECT(obj), "delete-event",
			  G_CALLBACK(on_delete_event), obj);

	return (ModestWindow*)obj;
}

void
modest_edit_msg_window_set_msg (ModestEditMsgWindow *self, TnyMsg *msg)
{
	TnyHeader *header;
	GtkTextBuffer *buf;
	const gchar *to, *cc, *bcc, *subject;
	ModestEditMsgWindowPrivate *priv;

	g_return_if_fail (MODEST_IS_EDIT_MSG_WINDOW (self));
	g_return_if_fail (TNY_IS_MSG (msg));

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE (self);

	header = tny_msg_get_header (msg);
	to      = tny_header_get_to (header);
	cc      = tny_header_get_cc (header);
	bcc     = tny_header_get_bcc (header);
	subject = tny_header_get_subject (header);

	if (to)
		gtk_entry_set_text (GTK_ENTRY(priv->to_field),  to);
	if (cc)
		gtk_entry_set_text (GTK_ENTRY(priv->cc_field),  cc);
	if (bcc)
		gtk_entry_set_text (GTK_ENTRY(priv->bcc_field), bcc);
	if (subject)
		gtk_entry_set_text (GTK_ENTRY(priv->subject_field), subject);	
	
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(priv->msg_body));
	gtk_text_buffer_set_text (buf,
				  (const gchar *) modest_tny_msg_actions_find_body (msg, FALSE),
				  -1);

	/* TODO: lower priority, select in the From: combo to the
	   value that comes from msg <- not sure, should it be
	   allowed? */
	
	/* TODO: set attachments */
}

ModestWidgetFactory *
modest_edit_msg_window_get_widget_factory (ModestEditMsgWindow *edit_window)
{
	ModestEditMsgWindowPrivate *priv;
			
	g_return_val_if_fail (MODEST_IS_EDIT_MSG_WINDOW (edit_window), NULL);

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE (edit_window);

	return g_object_ref (priv->widget_factory);
}

TnyAccountStore * 
modest_edit_msg_window_get_account_store (ModestEditMsgWindow *edit_window)
{
	ModestEditMsgWindowPrivate *priv;
			
	g_return_val_if_fail (MODEST_IS_EDIT_MSG_WINDOW (edit_window), NULL);

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE (edit_window);

	return g_object_ref (priv->account_store);
}

MsgData * 
modest_edit_msg_window_get_msg_data (ModestEditMsgWindow *edit_window)
{
	MsgData *data;
	ModestAccountData *account_data;
	GtkTextBuffer *buf;
	GtkTextIter b, e;
	ModestEditMsgWindowPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_EDIT_MSG_WINDOW (edit_window), NULL);

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE (edit_window);
									
	account_data = modest_combo_box_get_active_id (MODEST_COMBO_BOX (priv->from_field));
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->msg_body));	
	gtk_text_buffer_get_bounds (buf, &b, &e);

	/* don't free these (except from) */
	data = g_slice_new0 (MsgData);
	data->from    =  g_strdup_printf ("%s <%s>", account_data->fullname, account_data->email) ;
	data->to      =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->to_field));
	data->cc      =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->cc_field));
	data->bcc     =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->bcc_field));
	data->subject =  (gchar*) gtk_entry_get_text (GTK_ENTRY(priv->subject_field));	
	data->body    =  gtk_text_buffer_get_text (buf, &b, &e, FALSE);

	return data;
}

void 
modest_edit_msg_window_free_msg_data (ModestEditMsgWindow *edit_window,
						      MsgData *data)
{
	g_return_if_fail (MODEST_IS_EDIT_MSG_WINDOW (edit_window));

	g_free (data->from);
	g_free (data->body);
	g_slice_free (MsgData, data);
}

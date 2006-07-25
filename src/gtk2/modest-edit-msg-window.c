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

#include "modest-edit-msg-window.h"
#include <widgets/modest-msg-view.h>
#include <modest-widget-memory.h>

/* 'private'/'protected' functions */
static void                        modest_edit_msg_window_class_init   (ModestEditMsgWindowClass *klass);
static void                        modest_edit_msg_window_init         (ModestEditMsgWindow *obj);
static void                        modest_edit_msg_window_finalize     (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestEditMsgWindowPrivate ModestEditMsgWindowPrivate;
struct _ModestEditMsgWindowPrivate {

	ModestConf *conf;
	
	GtkWidget  *msg_body;
	GtkWidget  *to_field, *cc_field, *bcc_field,
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
		};
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
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
	GtkWidget *to_button, *cc_button, *bcc_button, *subject_label; 
	GtkWidget *header_table;
	GtkWidget *main_vbox;
	
	ModestEditMsgWindowPrivate *priv;
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	to_button     = gtk_button_new_with_label (_("To..."));
	cc_button     = gtk_button_new_with_label (_("Cc..."));
	bcc_button    = gtk_button_new_with_label (_("Bcc..."));
	subject_label = gtk_label_new (_("Subject:"));
	
	priv->to_field      = gtk_entry_new_with_max_length (40);
	priv->cc_field      = gtk_entry_new_with_max_length (40);
	priv->bcc_field     = gtk_entry_new_with_max_length (40);
	priv->subject_field = gtk_entry_new_with_max_length (40);

	header_table = gtk_table_new (4,2, FALSE);
	gtk_table_attach_defaults (GTK_TABLE(header_table), to_button,     0,1,0,1);
	gtk_table_attach_defaults (GTK_TABLE(header_table), cc_button,     0,1,1,2);
	gtk_table_attach_defaults (GTK_TABLE(header_table), bcc_button,    0,1,2,3);
	gtk_table_attach_defaults (GTK_TABLE(header_table), subject_label, 0,1,3,4);
 
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->to_field,     1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->cc_field,     1,2,1,2);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->bcc_field,    1,2,2,3);
	gtk_table_attach_defaults (GTK_TABLE(header_table), priv->subject_field,1,2,3,4);

	priv->msg_body = gtk_text_view_new ();
	
	main_vbox = gtk_vbox_new  (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(main_vbox), header_table, TRUE, TRUE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->msg_body, TRUE, TRUE, 6);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
}

static void
modest_edit_msg_window_finalize (GObject *obj)
{
	ModestEditMsgWindowPrivate *priv;

	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	g_object_unref (G_OBJECT(priv->conf));
	priv->conf = NULL;

	G_OBJECT_CLASS(parent_class)->finalize (obj);

}


GtkWidget*
modest_edit_msg_window_new (ModestConf *conf, ModestEditType type,
			    TnyMsgIface *msg)
{
	GObject *obj;
	ModestEditMsgWindowPrivate *priv;

	g_return_val_if_fail (conf, NULL);
	g_return_val_if_fail (type >= 0 && type < MODEST_EDIT_TYPE_NUM, NULL);
	g_return_val_if_fail (!(type==MODEST_EDIT_TYPE_NEW && msg), NULL); 
	g_return_val_if_fail (!(type!=MODEST_EDIT_TYPE_NEW && !msg), NULL); 
	
	
	obj = g_object_new(MODEST_TYPE_EDIT_MSG_WINDOW, NULL);
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	g_object_ref (G_OBJECT(conf));
	priv->conf = conf;
	
	modest_widget_memory_restore_settings (priv->conf, GTK_WIDGET(priv->msg_body),
					       "modest-edit-msg-body");
	return GTK_WIDGET (obj);
}

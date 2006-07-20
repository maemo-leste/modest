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

#include <modest-tny-msg-view.h>
#include "modest-msg-window.h"

/* 'private'/'protected' functions */
static void                   modest_msg_window_class_init    (ModestMsgWindowClass *klass);
static void                   modest_msg_window_init          (ModestMsgWindow *obj);
static void                   modest_msg_window_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMsgWindowPrivate ModestMsgWindowPrivate;
struct _ModestMsgWindowPrivate {
	/* my private members go here, eg. */
	/* gboolean frobnicate_mode; */
};
#define MODEST_MSG_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                               MODEST_TYPE_MSG_WINDOW, \
                                               ModestMsgWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_msg_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMsgWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_msg_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMsgWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_msg_window_init,
		};
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
		                                  "ModestMsgWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_msg_window_class_init (ModestMsgWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgWindowPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_msg_window_init (ModestMsgWindow *obj)
{
	GtkWidget *to_button, *cc_button, *bcc_button, *subject_label;
	GtkWidget *to_field, *cc_field, *bcc_field, *subject_field;
	GtkWidget *header_table;
	GtkWidget *main_vbox;
	GtkWidget *msg_field;
	
	ModestMsgWindowPrivate *priv;
	priv = MODEST_MSG_WINDOW_GET_PRIVATE(obj);

	to_button     = gtk_button_new_with_label (_("To..."));
	cc_button     = gtk_button_new_with_label (_("Cc..."));
	bcc_button    = gtk_button_new_with_label (_("Bcc..."));
	subject_label = gtk_label_new (_("Subject:"));
	
	to_field      = gtk_entry_new ();
	cc_field      = gtk_entry_new ();
	bcc_field     = gtk_entry_new ();
	subject_field = gtk_entry_new ();

	header_table = gtk_table_new (4,2, FALSE);
	gtk_table_attach_defaults (GTK_TABLE(header_table), to_button,     0,1,0,1);
	gtk_table_attach_defaults (GTK_TABLE(header_table), cc_button,     0,1,1,2);
	gtk_table_attach_defaults (GTK_TABLE(header_table), bcc_button,    0,1,2,3);
	gtk_table_attach_defaults (GTK_TABLE(header_table), subject_label, 0,1,3,4);

	gtk_table_attach_defaults (GTK_TABLE(header_table), to_field,      1,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(header_table), cc_field,      1,2,1,2);
	gtk_table_attach_defaults (GTK_TABLE(header_table), bcc_field,     1,2,2,3);
	gtk_table_attach_defaults (GTK_TABLE(header_table), subject_field, 1,2,3,4);

	msg_field = modest_tny_msg_view_new (NULL);
	
	main_vbox = gtk_vbox_new  (FALSE, 6);

	gtk_box_pack_start (GTK_BOX(main_vbox), header_table, FALSE, FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), msg_field,    TRUE, TRUE, 6);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	
	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
}

static void
modest_msg_window_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
}

GtkWidget*
modest_msg_window_new  (ModestMsgWindowType type, TnyMsgIface *msg)
{
	GObject *obj;
	
	g_return_val_if_fail ((type >= 1 && type <= MODEST_MSG_WINDOW_TYPE_NUM), NULL);
	
	obj = g_object_new(MODEST_TYPE_MSG_WINDOW, NULL);

	return GTK_WIDGET (obj);
}


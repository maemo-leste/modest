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
#include <modest-widget-factory.h>
#include "modest-icon-names.h"
#include <modest-tny-transport-actions.h>

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

	ModestConf *conf;
	ModestWidgetFactory *factory;
	
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
	ModestEditMsgWindowPrivate *priv;
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	priv->factory = NULL;
	priv->toolbar = NULL;
	priv->menubar = NULL;
}



static void
save_settings (ModestEditMsgWindow *self)
{
	ModestEditMsgWindowPrivate *priv;
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(self);
	modest_widget_memory_save_settings (priv->conf,
					    GTK_WIDGET(self),
					    "modest-edit-msg-window");
}


static void
restore_settings (ModestEditMsgWindow *self)
{
	ModestEditMsgWindowPrivate *priv;
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(self);
	modest_widget_memory_restore_settings (priv->conf, GTK_WIDGET(self),
					       "modest-edit-msg-window");
}

	

static void
on_menu_quit (ModestEditMsgWindow *self, guint action, GtkWidget *widget)
{
	save_settings (self);
	gtk_widget_destroy (GTK_WIDGET(self));
}





/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static GtkItemFactoryEntry menu_items[] = {
	{ "/_File",		NULL,			NULL,           0, "<Branch>" ,NULL},
	{ "/File/_New",		"<control>N",		NULL,		0, "<StockItem>", GTK_STOCK_NEW },
	{ "/File/_Open",	"<control>O",		NULL,		0, "<StockItem>", GTK_STOCK_OPEN },
	{ "/File/_Save",	"<control>S",		NULL,		0, "<StockItem>", GTK_STOCK_SAVE },
	{ "/File/Save _As",	NULL,			NULL,           0, "<Item>", NULL} ,
	{ "/File/Save Draft",	"<control><shift>S",	NULL,           0, "<Item>",NULL },


	{ "/File/sep1",		NULL,			NULL,           0, "<Separator>" ,NULL },
	{ "/File/_Quit",	"<CTRL>Q",		on_menu_quit,   0, "<StockItem>", GTK_STOCK_QUIT },

	{ "/_Edit",		NULL,			NULL,           0, "<Branch>" ,NULL },
	{ "/Edit/_Undo",	"<CTRL>Z",		NULL,		0, "<StockItem>", GTK_STOCK_UNDO },
	{ "/Edit/_Redo",	"<shift><CTRL>Z",	NULL,		0, "<StockItem>", GTK_STOCK_REDO },
	{ "/File/sep1",		NULL,			NULL,           0, "<Separator>",NULL },
	{ "/Edit/Cut",		"<control>X",		NULL,		0, "<StockItem>", GTK_STOCK_CUT  },
	{ "/Edit/Copy",		"<CTRL>C",		NULL,           0, "<StockItem>", GTK_STOCK_COPY },
	{ "/Edit/Paste",	NULL,			NULL,           0, "<StockItem>", GTK_STOCK_PASTE},
	{ "/Edit/sep1",		NULL,			NULL,           0, "<Separator>",NULL },
	{ "/Edit/Delete",	"<CTRL>Q",		NULL,           0, "<Item>" ,NULL },
	{ "/Edit/Select all",	"<CTRL>A",		NULL,           0, "<Item>" ,NULL },
	{ "/Edit/Deselect all",  "<Shift><CTRL>A",	NULL,           0, "<Item>",NULL },

	{ "/_View",             NULL,		NULL,		        0, "<Branch>",NULL },
	{ "/View/To-field",          NULL,		NULL,		0, "<CheckItem>",NULL },
	
	{ "/View/Cc-field:",          NULL,		NULL,		0, "<CheckItem>",NULL },
	{ "/View/Bcc-field:",          NULL,		NULL,		0, "<CheckItem>",NULL },
	
	
	{ "/_Insert",             NULL,		NULL,		0, "<Branch>",NULL },
/* 	{ "/Actions/_Reply",    NULL,			NULL,		0, "<Item>" }, */
/* 	{ "/Actions/_Forward",  NULL,			NULL,		0, "<Item>" }, */
/* 	{ "/Actions/_Bounce",   NULL,			NULL,		0, "<Item>" },	 */
	
	{ "/_Format",		 NULL,			NULL,		0, "<Branch>",NULL }
/* 	{ "/Options/_Accounts",  NULL,			on_menu_accounts,0, "<Item>" }, */
/* 	{ "/Options/_Contacts",  NULL,			NULL,		0, "<Item>" }, */


/* 	{ "/_Help",         NULL,                       NULL,           0, "<Branch>" }, */
/* 	{ "/_Help/About",   NULL,                       on_menu_about,  0, "<StockItem>", GTK_STOCK_ABOUT}, */
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


static GtkWidget *
menubar_new (ModestEditMsgWindow *self)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	
	/* Make an accelerator group (shortcut keys) */
	accel_group = gtk_accel_group_new ();
	
	/* Make an ItemFactory (that makes a menubar) */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
					     accel_group);
	
	/* This function generates the menu items. Pass the item factory,
	   the number of items in the array, the array itself, and any
	   callback data for the the menu items. */
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, self);
	
	///* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);
	
	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}


static void
send_mail (ModestEditMsgWindow *self)
{
	const gchar *from, *to, *cc, *bcc, *subject;
	gchar *body;
	ModestEditMsgWindowPrivate *priv;
	
	GtkTextBuffer *buf;
	GtkTextIter b, e;
	
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(self);

	/* don't free these */
	from    = "djcb@djcbsoftware.nl";
	to      =  gtk_entry_get_text (GTK_ENTRY(priv->to_field));
	cc      =  gtk_entry_get_text (GTK_ENTRY(priv->cc_field));
	bcc     =  gtk_entry_get_text (GTK_ENTRY(priv->bcc_field));
	subject =  gtk_entry_get_text (GTK_ENTRY(priv->subject_field));
	
	/* don't unref */
	buf   =  gtk_text_view_get_buffer (GTK_TEXT_VIEW(priv->msg_body));
	
	gtk_text_buffer_get_bounds (buf, &b, &e);
	body  = gtk_text_buffer_get_text (buf, &b, &e,
					  FALSE); /* free this one */

//	modest_tny_transport_actions_send_message (transport_account,
//						   from, to, cc, bcc,
//						   subject, *body, NULL);
	g_free (body);
}


static void
on_toolbar_button_clicked (ModestToolbar *toolbar, ModestToolbarButton button_id,
			   ModestEditMsgWindow *self)
{
	switch (button_id) {
	case MODEST_TOOLBAR_BUTTON_MAIL_SEND:
		send_mail (self);
		save_settings (self);
		gtk_widget_destroy (GTK_WIDGET(self));
		break;
		
	case MODEST_TOOLBAR_BUTTON_REPLY:
	case MODEST_TOOLBAR_BUTTON_REPLY_ALL:
	case MODEST_TOOLBAR_BUTTON_FORWARD:
	case MODEST_TOOLBAR_BUTTON_SEND_RECEIVE:
	case MODEST_TOOLBAR_BUTTON_NEXT:
	case MODEST_TOOLBAR_BUTTON_PREV:
	case MODEST_TOOLBAR_BUTTON_DELETE:

	default:
		g_printerr ("modest: key %d pressed\n", button_id);
	}
}




static ModestToolbar*
toolbar_new (ModestEditMsgWindow *self)
{
	int i;
	ModestToolbar *toolbar;
	GSList *buttons = NULL;
	ModestEditMsgWindowPrivate *priv;

	ModestToolbarButton button_ids[] = {
		MODEST_TOOLBAR_BUTTON_MAIL_SEND
	};		
	
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(self);

	for (i = 0 ; i != sizeof(button_ids) / sizeof(ModestToolbarButton); ++i)
		buttons = g_slist_append (buttons, GINT_TO_POINTER(button_ids[i]));
	
	toolbar = modest_widget_factory_get_edit_toolbar (priv->factory, buttons);
	g_slist_free (buttons);

	g_signal_connect (G_OBJECT(toolbar), "button_clicked",
			  G_CALLBACK(on_toolbar_button_clicked), self);
	
	return toolbar;
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

	priv->from_field    = modest_widget_factory_get_combo_box (priv->factory,
								   MODEST_COMBO_BOX_TYPE_TRANSPORTS);
	priv->to_field      = gtk_entry_new_with_max_length (40);
	priv->cc_field      = gtk_entry_new_with_max_length (40);
	priv->bcc_field     = gtk_entry_new_with_max_length (40);
	priv->subject_field = gtk_entry_new_with_max_length (40);
	
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

	priv->menubar = menubar_new (obj);
	priv->toolbar = GTK_WIDGET(toolbar_new (obj));

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

	g_object_unref (G_OBJECT(priv->conf));
	priv->conf = NULL;

	g_object_unref (G_OBJECT(priv->factory));
	priv->factory = NULL;
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);

}



static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, ModestEditMsgWindow *self)
{
	save_settings (self);
	return FALSE;
}


GtkWidget*
modest_edit_msg_window_new (ModestConf *conf, ModestWidgetFactory *factory,
			    ModestEditType type, TnyMsgIface *msg)
{
	GObject *obj;
	ModestEditMsgWindowPrivate *priv;

	g_return_val_if_fail (conf, NULL);
	g_return_val_if_fail (factory, NULL);
	g_return_val_if_fail (type < MODEST_EDIT_TYPE_NUM, NULL);
	g_return_val_if_fail (!(type==MODEST_EDIT_TYPE_NEW && msg), NULL); 
	g_return_val_if_fail (!(type!=MODEST_EDIT_TYPE_NEW && !msg), NULL); 	
	
	obj = g_object_new(MODEST_TYPE_EDIT_MSG_WINDOW, NULL);
	priv = MODEST_EDIT_MSG_WINDOW_GET_PRIVATE(obj);

	g_object_ref (G_OBJECT(conf));
	priv->conf = conf;

	g_object_ref (factory);
	priv->factory = factory;

	init_window (MODEST_EDIT_MSG_WINDOW(obj));

	restore_settings (MODEST_EDIT_MSG_WINDOW(obj));
	
	gtk_window_set_title (GTK_WINDOW(obj), "Modest");
	gtk_window_set_icon  (GTK_WINDOW(obj),
			      modest_icon_factory_get_icon (MODEST_APP_ICON));

	g_signal_connect (G_OBJECT(obj), "delete-event",
			  G_CALLBACK(on_delete_event), obj);
	
	return GTK_WIDGET (obj);
}

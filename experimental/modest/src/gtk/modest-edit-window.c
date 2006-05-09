/* modest-edit-window.c */

/* insert (c)/licensing information) */

#include <gtk/gtk.h>
#include <string.h>
#include "modest-edit-window.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <glib/gi18n.h>

/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                    modest_edit_window_class_init    (ModestEditWindowClass *klass);
static void                    modest_edit_window_init          (ModestEditWindow *obj);
static void                    modest_edit_window_finalize      (GObject *obj);


static GtkWidget* modest_edit_window_toolbar (void);

static void on_send_clicked (GtkToolButton *button, gpointer data);
static void on_cut_clicked (GtkToolButton *button, gpointer data);
static void on_copy_clicked (GtkToolButton *button, gpointer data);
static void on_paste_clicked (GtkToolButton *button, gpointer data);
static void on_quit_clicked (GtkToolButton *button, gpointer data);
static void on_save_clicked (GtkToolButton *button, gpointer data);



/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestEditWindowPrivate ModestEditWindowPrivate;
struct _ModestEditWindowPrivate {

	GtkEntry      *to_entry;
	GtkEntry      *cc_entry;
	GtkEntry      *bcc_entry;
	GtkEntry      *subject_entry;

	GtkTextView   *body_text;
	GtkStatusbar  *status_bar;
	
};
#define MODEST_EDIT_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_EDIT_WINDOW, \
                                                ModestEditWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_edit_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestEditWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_edit_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestEditWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_edit_window_init,
		};
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
		                                  "ModestEditWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_edit_window_class_init (ModestEditWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_edit_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestEditWindowPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_edit_window_init (ModestEditWindow *self)
{
	ModestEditWindowPrivate *priv;
	GtkWidget *vbox;
	GtkWidget *to_button, *cc_button, *bcc_button;
	GtkWidget *subject_label;
	GtkWidget *table;
	GtkWidget *scrolled_win;
	GtkWidget *toolbar;
	
	priv = MODEST_EDIT_WINDOW_GET_PRIVATE(self);
	
	priv->to_entry         = GTK_ENTRY(gtk_entry_new_with_max_length (255));
	priv->cc_entry         = GTK_ENTRY(gtk_entry_new_with_max_length (255));
	priv->bcc_entry        = GTK_ENTRY(gtk_entry_new_with_max_length (255));
	priv->subject_entry    = GTK_ENTRY(gtk_entry_new_with_max_length (255));	
	priv->status_bar       = GTK_STATUSBAR(gtk_statusbar_new ());
	
	toolbar = modest_edit_window_toolbar ();
	
	to_button  = gtk_button_new_with_label (_("To..."));
	cc_button  = gtk_button_new_with_label (_("Cc..."));
	bcc_button = gtk_button_new_with_label (_("Bcc.."));

	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_win),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_win),
					     GTK_SHADOW_IN);
	
	subject_label = gtk_label_new ("Subject:");

	table = gtk_table_new (4, 2, FALSE);

	gtk_table_attach (GTK_TABLE(table), to_button,
			  0, 1, 0, 1, 0, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(table), GTK_WIDGET(priv->to_entry),
			  1, 2, 0, 1, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	
	gtk_table_attach (GTK_TABLE(table), cc_button,
			  0, 1, 1, 2, 0, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(table), GTK_WIDGET(priv->cc_entry),
			  1, 2, 1, 2, GTK_EXPAND|GTK_FILL, 0, 0, 0);

	gtk_table_attach (GTK_TABLE(table), bcc_button,
			  0, 1, 2, 3, 0, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(table), GTK_WIDGET(priv->bcc_entry),
			  1, 2, 2, 3, GTK_EXPAND|GTK_FILL, 0, 0, 0);

	gtk_table_attach (GTK_TABLE(table), subject_label,
			  0, 1, 3, 4, 0, 0, 0, 0);
	gtk_table_attach (GTK_TABLE(table), GTK_WIDGET(priv->subject_entry),
			  1, 2, 3, 4, GTK_EXPAND|GTK_FILL, 0, 0, 0);

	priv->body_text  = GTK_TEXT_VIEW(gtk_text_view_new ());

	gtk_container_add (GTK_CONTAINER(scrolled_win), GTK_WIDGET(priv->body_text));
	
	vbox = gtk_vbox_new (FALSE, 5);

	gtk_box_pack_start (GTK_BOX(vbox), toolbar,      FALSE, TRUE,2);	
	gtk_box_pack_start (GTK_BOX(vbox), table,        FALSE, TRUE,2);
	gtk_box_pack_start (GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(priv->status_bar),
			    FALSE, TRUE, 2);

	/* make everything but the top level container visible,
	 * so gtk_widget_show will work */
	gtk_widget_show_all (vbox);
	gtk_container_add (GTK_CONTAINER(self), vbox);
	
}

static void
modest_edit_window_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
}


GtkWidget*
modest_edit_window_new   (const gchar *to, const gchar *cc,
			  const gchar *bcc, const gchar *subject,
			  const gchar *body, const GSList *attachments)
{

	ModestEditWindowPrivate *priv;
	GObject *obj = g_object_new(MODEST_TYPE_EDIT_WINDOW, NULL);
	
	priv = MODEST_EDIT_WINDOW_GET_PRIVATE(obj);

	/* FIXME: valid utf-8 */

	if (to)
		gtk_entry_set_text (priv->to_entry, to);
	if (cc)
		gtk_entry_set_text (priv->cc_entry, cc);
	if (bcc)
		gtk_entry_set_text (priv->bcc_entry,bcc);

	if (subject)
		gtk_entry_set_text (priv->subject_entry, subject);
	
	if (body)
		gtk_text_buffer_insert (gtk_text_view_get_buffer(priv->body_text),
					NULL, body, strlen(body));

	/* FIXME: attachments */
	
	return GTK_WIDGET(obj);
}




static GtkWidget*
modest_edit_window_toolbar (void)
{
	GtkWidget *toolbar;
	GtkToolItem *send, *save, *cut, *copy, *paste, *quit;

	toolbar = gtk_toolbar_new ();

	/* FIXME: get a better icon */
	send  = gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_PLAY); 
	
	save  = gtk_tool_button_new_from_stock (GTK_STOCK_SAVE);
	cut   = gtk_tool_button_new_from_stock (GTK_STOCK_CUT);
	copy  = gtk_tool_button_new_from_stock (GTK_STOCK_COPY);
	paste = gtk_tool_button_new_from_stock (GTK_STOCK_PASTE);
	quit  = gtk_tool_button_new_from_stock (GTK_STOCK_QUIT);

	g_signal_connect (send, "clicked", G_CALLBACK(on_send_clicked),
			  "send");
	g_signal_connect (save, "clicked", G_CALLBACK(on_save_clicked),
			  "save");
	g_signal_connect (cut, "clicked", G_CALLBACK(on_cut_clicked),
			  "cut");
	g_signal_connect (copy, "clicked", G_CALLBACK(on_copy_clicked),
			  "copy");
	g_signal_connect (paste, "clicked", G_CALLBACK(on_paste_clicked),
			  "send");
	g_signal_connect (quit, "clicked", G_CALLBACK(on_quit_clicked),
			  "quit");

	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(send), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar),
			    GTK_TOOL_ITEM(gtk_separator_tool_item_new()),
			    -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(save), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar),
			    GTK_TOOL_ITEM(gtk_separator_tool_item_new()),
			    -1);
	
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(cut), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(copy), -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(paste), -1);

	gtk_toolbar_insert (GTK_TOOLBAR(toolbar),
			    GTK_TOOL_ITEM(gtk_separator_tool_item_new()),
			    -1);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(quit), -1);

	return toolbar;
}


static void
on_send_clicked (GtkToolButton *button, gpointer data)
{
	g_warning ("clicked: %s", (gchar *)data);
}

static void
on_cut_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}
static void
on_copy_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}
static void
on_paste_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}

static void
on_save_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}



static void
on_quit_clicked (GtkToolButton *button, gpointer data)
{
	g_warning (__FUNCTION__);
}



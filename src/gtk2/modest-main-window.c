/* modest-main-window.c */

/* insert (c)/licensing information) */

#include "modest-main-window.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_main_window_class_init    (ModestMainWindowClass *klass);
static void modest_main_window_init          (ModestMainWindow *obj);
static void modest_main_window_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMainWindowPrivate ModestMainWindowPrivate;
struct _ModestMainWindowPrivate {
	GtkWidget *toolbar;
	GtkWidget *menubar;
	GtkWidget *progress_bar;
	GtkWidget *status_bar;
	GtkWidget *folder_paned;
	GtkWidget *msg_paned;

	ModestWidgetFactory *widget_factory;
	
	ModestTnyHeaderTreeView *header_view;
	ModestTnyFolderTreeView *folder_view;
	ModestTnyMsgView        *msg_preview;
};


#define MODEST_MAIN_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                MODEST_TYPE_MAIN_WINDOW, \
                                                ModestMainWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_main_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMainWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_main_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMainWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_main_window_init,
		};
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
		                                  "ModestMainWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_main_window_class_init (ModestMainWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_main_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMainWindowPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_main_window_init (ModestMainWindow *obj)
{
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	priv->widget_factory = NULL;
}

static void
modest_main_window_finalize (GObject *obj)
{
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	if (priv->widget_factory) {
		g_object_unref (G_OBJECT(priv->widget_factory));
		priv->widget_factory = NULL;
	}
		
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

/* Obligatory basic callback */
static void print_hello( GtkWidget *w,
                         gpointer   data )
{
	g_message ("Hello, World!\n");
}

/* For the check button */
static void print_toggle( gpointer   callback_data,
                          guint      callback_action,
                          GtkWidget *menu_item )
{
   g_message ("Check button state - %d\n",
              GTK_CHECK_MENU_ITEM (menu_item)->active);
}

/* For the radio buttons */
static void print_selected( gpointer   callback_data,
                            guint      callback_action,
                            GtkWidget *menu_item )
{
   if(GTK_CHECK_MENU_ITEM(menu_item)->active)
     g_message ("Radio button %d selected\n", callback_action);
}

/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static GtkItemFactoryEntry menu_items[] = {
	{ "/_File",         NULL,         NULL,           0, "<Branch>" },
	{ "/File/_New",     "<control>N", print_hello,    0, "<StockItem>", GTK_STOCK_NEW },
	{ "/File/_Open",    "<control>O", print_hello,    0, "<StockItem>", GTK_STOCK_OPEN },
	{ "/File/_Save",    "<control>S", print_hello,    0, "<StockItem>", GTK_STOCK_SAVE },
	{ "/File/Save _As", NULL,         NULL,           0, "<Item>" },
	{ "/File/sep1",     NULL,         NULL,           0, "<Separator>" },
	{ "/File/_Quit",    "<CTRL>Q", gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT },
	{ "/_Edit",         NULL,         NULL,           0, "<Branch>" },
	{ "/Edit/_Undo",    "<control>Z", print_hello,    0, "<StockItem>", GTK_STOCK_UNDO },
	{ "/Edit/_Redo",    "<shift><control>Z", print_hello,    0, "<StockItem>", GTK_STOCK_REDO },
	{ "/File/sep1",     NULL,         NULL,           0, "<Separator>" },
	
	{ "/Edit/Cut",    "<control>S", print_hello,    0, "<StockItem>", GTK_STOCK_SAVE },
	{ "/Edit/Copy",    NULL,         NULL,           0, "<Item>" },
	{ "/Edit/Paste",     NULL,         NULL,           0, "<Separator>" },
	{ "/Edit/Delete",    "<CTRL>Q", gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT },
	{ "/Edit/Select all",    "<CTRL>A", gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT },
	{ "/Edit/Deelect all",    "<Shift><CTRL>A", gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT },
	
	{ "/_Options",      NULL,         NULL,           0, "<Branch>" },
	{ "/Options/tear",  NULL,         NULL,           0, "<Tearoff>" },
	{ "/Options/Check", NULL,         print_toggle,   1, "<CheckItem>" },
	{ "/Options/sep",   NULL,         NULL,           0, "<Separator>" },
	{ "/Options/Rad1",  NULL,         print_selected, 1, "<RadioItem>" },
	{ "/Options/Rad2",  NULL,         print_selected, 2, "/Options/Rad1" },
	{ "/Options/Rad3",  NULL,         print_selected, 3, "/Options/Rad1" },
	{ "/_Help",         NULL,         NULL,           0, "<LastBranch>" },
	{ "/_Help/About",   NULL,         NULL,           0, "<Item>" },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


static GtkWidget *
menubar_new (ModestMainWindow *self)
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
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
	
	///* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (self), accel_group);
	
	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}




static ModestTnyHeaderTreeView*
header_view_new (ModestMainWindow *self)
{
	int i;
	GSList *columns = NULL;
	ModestTnyHeaderTreeView *header_view;
	ModestMainWindowPrivate *priv;
	ModestTnyHeaderTreeViewColumn cols[] = {
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_MSGTYPE,
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_ATTACH,
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_FROM,
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_SUBJECT,
		MODEST_TNY_HEADER_TREE_VIEW_COLUMN_RECEIVED_DATE
	};

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	for (i = 0 ; i != sizeof(cols) / sizeof(ModestTnyHeaderTreeViewColumn); ++i)
		columns = g_slist_append (columns, GINT_TO_POINTER(cols[i]));

	header_view = modest_widget_factory_get_header_tree_widget (priv->widget_factory);
	modest_tny_header_tree_view_set_columns (header_view, columns);
	g_slist_free (columns);

	return header_view;
}



GtkWidget*
modest_main_window_new (ModestWidgetFactory *factory)
{
	GObject *obj;
	ModestMainWindowPrivate *priv;
	
	GtkWidget *main_vbox;
	GtkWidget *main_paned;
	GtkWidget *status_hbox;
	GtkWidget *header_win, *folder_win; 
	
	g_return_val_if_fail (factory, NULL);

	obj  = g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	g_object_ref (factory);
	priv->widget_factory = factory;

	/* widgets from factory */
	priv->folder_view = modest_widget_factory_get_folder_tree_widget (factory);
	priv->header_view = header_view_new (MODEST_MAIN_WINDOW(obj));
	priv->msg_preview = modest_widget_factory_get_msg_preview_widget (factory);
	folder_win = gtk_scrolled_window_new (NULL,NULL);
	gtk_container_add (GTK_CONTAINER(folder_win), GTK_WIDGET(priv->folder_view));
	header_win = gtk_scrolled_window_new (NULL,NULL);
	gtk_container_add (GTK_CONTAINER(header_win), GTK_WIDGET(priv->header_view));
			   
	/* tool/menubar */
	priv->menubar = menubar_new (MODEST_MAIN_WINDOW(obj));
	priv->toolbar = gtk_toolbar_new ();

	/* paned */
	priv->folder_paned = gtk_vpaned_new ();
	priv->msg_paned = gtk_vpaned_new ();
	main_paned = gtk_hpaned_new ();
	gtk_paned_add1 (GTK_PANED(main_paned), priv->folder_paned);
	gtk_paned_add2 (GTK_PANED(main_paned), priv->msg_paned);
	gtk_paned_add1 (GTK_PANED(priv->folder_paned), gtk_label_new ("Favourites"));
	gtk_paned_add2 (GTK_PANED(priv->folder_paned), folder_win);
	gtk_paned_add1 (GTK_PANED(priv->msg_paned), header_win);
	gtk_paned_add2 (GTK_PANED(priv->msg_paned), GTK_WIDGET(priv->msg_preview));

	gtk_widget_show (GTK_WIDGET(priv->header_view));
		
	/* status bar / progress */
	priv->status_bar   = gtk_statusbar_new ();
	priv->progress_bar = gtk_progress_bar_new ();
	status_hbox = gtk_hbox_new (TRUE, 5);
	gtk_box_pack_start (GTK_BOX(status_hbox), priv->status_bar, FALSE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX(status_hbox), priv->progress_bar, FALSE, TRUE, 5);

	
	/* putting it all together... */
	main_vbox = gtk_vbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->menubar, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX(main_vbox), main_paned, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(main_vbox), status_hbox, FALSE, FALSE, 5);
	
	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
	gtk_widget_show_all (main_vbox);
	
	gtk_window_set_title (GTK_WINDOW(obj), "modest");
	gtk_window_set_default_size (GTK_WINDOW(obj), 800, 600);
	
	return GTK_WIDGET(obj);
}

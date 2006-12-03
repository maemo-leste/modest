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
#include <string.h>
#include <gtk/gtkaboutdialog.h>
#include <gtk/gtktreeviewcolumn.h>

#include <modest-widget-memory.h>
#include <modest-icon-factory.h>

#include <widgets/modest-toolbar.h>

#include "modest-main-window.h"
#include "modest-account-view-window.h"
#include "modest-account-mgr.h"
#include "modest-conf.h"
#include "modest-edit-msg-window.h"
#include "modest-icon-names.h"
#include "modest-tny-platform-factory.h"
#include "modest-tny-msg-actions.h"
#include "modest-mail-operation.h"

/* 'private'/'protected' functions */
static void modest_main_window_class_init    (ModestMainWindowClass *klass);
static void modest_main_window_init          (ModestMainWindow *obj);
static void modest_main_window_finalize      (GObject *obj);

static void restore_sizes (ModestMainWindow *self);
static void save_sizes (ModestMainWindow *self);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMainWindowPrivate ModestMainWindowPrivate;
struct _ModestMainWindowPrivate {

	GtkUIManager *ui_manager;

	GtkWidget *toolbar;
	GtkWidget *menu;

	//GtkWidget *folder_paned;
	GtkWidget *msg_paned;
	GtkWidget *main_paned;
	
	ModestWidgetFactory *widget_factory;
	TnyPlatformFactory *factory;
  
	ModestHeaderView *header_view;
	ModestFolderView *folder_view;
	ModestMsgView    *msg_preview;
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
			NULL
		};
		my_type = g_type_register_static (HILDON_TYPE_WINDOW,
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
	
	priv->factory = modest_tny_platform_factory_get_instance ();
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


static void
on_menu_about (GtkWidget *widget, gpointer data)
{
	GtkWidget *about;
	const gchar *authors[] = {
		"Dirk-Jan C. Binnema <dirk-jan.binnema@nokia.com>",
		NULL
	};	
	about = gtk_about_dialog_new ();
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG(about), PACKAGE_NAME);
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about),PACKAGE_VERSION);
	gtk_about_dialog_set_copyright (
		GTK_ABOUT_DIALOG(about),
		_("Copyright (c) 2006, Nokia Corporation\n"
		  "All rights reserved."));
	gtk_about_dialog_set_comments (	GTK_ABOUT_DIALOG(about),
		_("a modest e-mail client\n\n"
		  "design and implementation: Dirk-Jan C. Binnema\n"
		  "contributions from the fine people at KernelConcepts\n\n"
		  "uses the tinymail email framework written by Philip van Hoof"));
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(about), authors);
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(about), "http://modest.garage.maemo.org");

	gtk_dialog_run (GTK_DIALOG (about));
	gtk_widget_destroy(about);
}


static void
on_menu_accounts (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	GtkWidget *account_win;
	ModestMainWindowPrivate *priv;

	g_return_if_fail (widget);
	g_return_if_fail (self);
	
	priv        = MODEST_MAIN_WINDOW_GET_PRIVATE(self);	
	account_win = modest_account_view_window_new (priv->widget_factory);

	gtk_window_set_transient_for (GTK_WINDOW(account_win),
				      GTK_WINDOW(self));
				      
	gtk_widget_show (account_win);
}


static void
on_menu_new_message (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	GtkWidget *msg_win;
	ModestMainWindowPrivate *priv;

	priv  = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	msg_win = modest_edit_msg_window_new (priv->widget_factory,
					      MODEST_EDIT_TYPE_NEW,
					      NULL);
	gtk_widget_show (msg_win);
}

static void
on_menu_reply_forward (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	GtkWidget *msg_win;
	ModestMainWindowPrivate *priv;
	ModestHeaderView *header_view;
	TnyList *header_list;
	TnyIterator *iter;
	const gchar *from;
	gchar *reply_key, *forward_key;
	ModestMailOperationReplyType reply_type;
	ModestMailOperationForwardType forward_type;
	ModestConf *conf;
	GError *error;

	priv  = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_modest_conf_instance (priv->factory);

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	header_list = modest_header_view_get_selected_headers (header_view);

	/* Get reply and forward types */
	error = NULL;
	reply_key = g_strdup_printf ("%s/%s", MODEST_CONF_NAMESPACE, MODEST_CONF_REPLY_TYPE);
	reply_type = modest_conf_get_int (conf, reply_key, &error);
	if (error) {
		g_warning ("key %s not defined", reply_key);
		reply_type = MODEST_MAIL_OPERATION_REPLY_TYPE_CITE;
		g_error_free (error);
		error = NULL;
	}
	g_free (reply_key);
	
	forward_key = g_strdup_printf ("%s/%s", MODEST_CONF_NAMESPACE, MODEST_CONF_FORWARD_TYPE);
	forward_type = modest_conf_get_int (conf, forward_key, NULL);
	if (error) {
		g_warning ("key %s not defined", forward_key);
		reply_type = MODEST_MAIL_OPERATION_FORWARD_TYPE_INLINE;
		g_error_free (error);
	}
	g_free (forward_key);
	
	if (header_list) {
		iter = tny_list_create_iterator (header_list);
		do {
			TnyHeader *header, *new_header;
			TnyFolder *folder;
			TnyMsg    *msg, *new_msg;
			ModestEditType edit_type;

			/* Get msg from header */
			header = TNY_HEADER (tny_iterator_get_current (iter));
			folder = tny_header_get_folder (header);
			msg = tny_folder_get_msg (folder, header, NULL); /* FIXME */

			from = modest_folder_view_get_selected_account (priv->folder_view);

			/* FIXME: select proper action */
			switch (action) {
			case 1:
				new_msg = 
					modest_mail_operation_create_reply_mail (msg, from, reply_type,
										 MODEST_MAIL_OPERATION_REPLY_MODE_SENDER);
				edit_type = MODEST_EDIT_TYPE_REPLY;
				break;
			case 2:
				new_msg = 
					modest_mail_operation_create_reply_mail (msg, from, reply_type,
										 MODEST_MAIL_OPERATION_REPLY_MODE_ALL);
				edit_type = MODEST_EDIT_TYPE_REPLY;
				break;
			case 3:
				new_msg = 
					modest_mail_operation_create_forward_mail (msg, from, forward_type);
				edit_type = MODEST_EDIT_TYPE_FORWARD;
				break;
			default:
				g_warning ("unexpected action type: %d", action);
			}

			/* Set from */
			new_header = tny_msg_get_header (new_msg);
			tny_header_set_from (new_header, 
					     modest_folder_view_get_selected_account (priv->folder_view));

			/* Show edit window */
			msg_win = modest_edit_msg_window_new (priv->widget_factory,
							      edit_type,
							      new_msg);
			gtk_widget_show (msg_win);

			/* Clean and go on */
			g_object_unref (new_msg);
			tny_iterator_next (iter);

		} while (!tny_iterator_is_done (iter));
	}
}

static void
on_menu_quit (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	save_sizes (self);
	gtk_widget_destroy (GTK_WIDGET(self));
}

static void
on_menu_delete (ModestMainWindow *self, guint action, GtkWidget *widget)
{
	ModestMainWindowPrivate *priv;
	ModestHeaderView *header_view;
	TnyList *header_list;
	TnyIterator *iter;
	GtkTreeModel *model;

	priv  = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	header_list = modest_header_view_get_selected_headers (header_view);
	
	if (header_list) {
		iter = tny_list_create_iterator (header_list);
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (header_view));
		if (GTK_IS_TREE_MODEL_SORT (model))
			model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model));
		do {
			TnyHeader *header;
			ModestMailOperation *mail_op;

			header = TNY_HEADER (tny_iterator_get_current (iter));
			/* TODO: thick grain mail operation involving
			   a list of objects. Composite pattern ??? */
			mail_op = modest_mail_operation_new ();

			/* Move to trash */
			modest_mail_operation_remove_msg (mail_op, header, TRUE);

			/* Remove from tree model */
			tny_list_remove (TNY_LIST (model), G_OBJECT (header));

			g_object_unref (G_OBJECT (mail_op));
			g_object_unref (header);
			tny_iterator_next (iter);

		} while (!tny_iterator_is_done (iter));
	}
}


/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static const gchar* UI_DEF=
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<ui>"
	"  <popup>"
	"    <menu name=\"Message\" \"MenuMessage\">"
	"      <menuitem name=\"New\" action=\"on_menu_new_message\" />"
	"    </menu>"
//	"    <menu name=\"JustifyMenu\" action=\"JustifyMenuAction\">"
//	"      <menuitem name=\"Left\" action=\"justify-left\"/>"
//	"      <menuitem name=\"Centre\" action=\"justify-center\"/>"
//	"      <menuitem name=\"Right\" action=\"justify-right\"/>"
//	"      <menuitem name=\"Fill\" action=\"justify-fill\"/>"
//	"    </menu>"
	"  </popup>"
	"</ui>";

static GtkMenu *
get_menu (ModestMainWindow *self)
{
	GtkWidget *w;
	int i = 0;
	
	ModestMainWindowPrivate *priv;
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	priv->ui_manager = gtk_ui_manager_new ();

	gtk_ui_manager_add_ui_from_string (priv->ui_manager,
					   UI_DEF, strlen(UI_DEF),
					   NULL);

	w = gtk_ui_manager_get_widget (priv->ui_manager, "/popup");
	g_warning ("==> GtkMenu? ==> %s", GTK_IS_MENU(w) ? "yes" : "no"); 

	return GTK_MENU(w);
}



static ModestHeaderView*
header_view_new (ModestMainWindow *self)
{
	int i;
	GList *columns = NULL;
	ModestHeaderView *header_view;
	ModestMainWindowPrivate *priv;
	ModestHeaderViewColumn cols[] = {
		MODEST_HEADER_VIEW_COLUMN_MSGTYPE,
		MODEST_HEADER_VIEW_COLUMN_ATTACH,
 		MODEST_HEADER_VIEW_COLUMN_COMPACT_HEADER
//		MODEST_HEADER_VIEW_COLUMN_FROM,
//		MODEST_HEADER_VIEW_COLUMN_SUBJECT,
//		MODEST_HEADER_VIEW_COLUMN_RECEIVED_DATE
	};
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	for (i = 0 ; i != sizeof(cols) / sizeof(ModestHeaderViewColumn); ++i)
		columns = g_list_append (columns, GINT_TO_POINTER(cols[i]));

	header_view = modest_widget_factory_get_header_view (priv->widget_factory);
	modest_header_view_set_columns (header_view, columns);
	g_list_free (columns);

	return header_view;
}


static void
on_toolbar_button_clicked (ModestToolbar *toolbar, ModestToolbarButton button_id,
			   ModestMainWindow *self)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	ModestMainWindowPrivate *priv;

	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	
	switch (button_id) {
	case MODEST_TOOLBAR_BUTTON_NEW_MAIL:
		on_menu_new_message (self, 0, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_REPLY:
		on_menu_reply_forward (self, 1, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_REPLY_ALL:
		on_menu_reply_forward (self, 2, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_FORWARD:
		on_menu_reply_forward (self, 3, NULL);
		break;
	case MODEST_TOOLBAR_BUTTON_SEND_RECEIVE:
		

	case MODEST_TOOLBAR_BUTTON_NEXT:
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(priv->header_view));
		if (sel) {
			gtk_tree_selection_get_selected (sel, &model, &iter);
			gtk_tree_model_iter_next (model, &iter);
			gtk_tree_selection_select_iter (sel, &iter);
		}
		
	case MODEST_TOOLBAR_BUTTON_PREV:
	/* 	if (sel) { */
/* 			gtk_tree_selection_get_selected (sel, &model, &iter); */
/* 			gtk_tree_model_iter_prev (model, &iter); */
/* 			gtk_tree_selection_select_iter (sel, &iter); */
/* 		} */

		break;
	case MODEST_TOOLBAR_BUTTON_DELETE:
		on_menu_delete (self, 0, GTK_WIDGET (toolbar));
		break;

	default:
		g_printerr ("modest: key %d pressed\n", button_id);
	}
}

static ModestToolbar*
toolbar_new (ModestMainWindow *self)
{
	int i;
	ModestToolbar *toolbar;
	GSList *buttons = NULL;
	ModestMainWindowPrivate *priv;

	ModestToolbarButton button_ids[] = {
		MODEST_TOOLBAR_BUTTON_NEW_MAIL,
		MODEST_TOOLBAR_BUTTON_REPLY,
		MODEST_TOOLBAR_BUTTON_REPLY_ALL,
		MODEST_TOOLBAR_BUTTON_FORWARD,
		MODEST_TOOLBAR_SEPARATOR,
		MODEST_TOOLBAR_BUTTON_SEND_RECEIVE,
		MODEST_TOOLBAR_SEPARATOR,
		MODEST_TOOLBAR_BUTTON_PREV,
		MODEST_TOOLBAR_BUTTON_NEXT,
		MODEST_TOOLBAR_SEPARATOR,		
		MODEST_TOOLBAR_BUTTON_DELETE
	};		
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);

	for (i = 0 ; i != sizeof(button_ids) / sizeof(ModestToolbarButton); ++i)
		buttons = g_slist_append (buttons, GINT_TO_POINTER(button_ids[i]));
	
	toolbar = modest_widget_factory_get_main_toolbar (priv->widget_factory, buttons);
	g_slist_free (buttons);
	
	g_signal_connect (G_OBJECT(toolbar), "button_clicked",
			  G_CALLBACK(on_toolbar_button_clicked), self);
	
	return toolbar;
}



static void
restore_sizes (ModestMainWindow *self)
{
	ModestConf *conf;
	ModestMainWindowPrivate *priv;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_modest_conf_instance (priv->factory);

	modest_widget_memory_restore_settings (conf,GTK_WIDGET(self),
					       "modest-main-window");
	modest_widget_memory_restore_settings (conf, GTK_WIDGET(priv->msg_paned),
					       "modest-msg-paned");
	modest_widget_memory_restore_settings (conf, GTK_WIDGET(priv->main_paned),
					       "modest-main-paned");
}


static void
save_sizes (ModestMainWindow *self)
{
	ModestMainWindowPrivate *priv;
	ModestConf *conf;
	
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(self);
	conf = modest_tny_platform_factory_get_modest_conf_instance (priv->factory);
	
	modest_widget_memory_save_settings (conf,GTK_WIDGET(self),
					    "modest-main-window");
	modest_widget_memory_save_settings (conf, GTK_WIDGET(priv->msg_paned),
					    "modest-msg-paned");
	modest_widget_memory_save_settings (conf, GTK_WIDGET(priv->main_paned),
					    "modest-main-paned");
}

static GtkWidget*
wrapped_in_scrolled_window (GtkWidget *widget, gboolean needs_viewport)
{
	GtkWidget *win;

	win = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy
		(GTK_SCROLLED_WINDOW (win),GTK_POLICY_NEVER,
		 GTK_POLICY_AUTOMATIC);
	
	if (needs_viewport)
		gtk_scrolled_window_add_with_viewport
			(GTK_SCROLLED_WINDOW(win), widget);
	else
		gtk_container_add (GTK_CONTAINER(win),
				   widget);

	return win;
}


static gboolean
on_delete_event (GtkWidget *widget, GdkEvent  *event, ModestMainWindow *self)
{
	save_sizes (self);
	return FALSE;
}


GtkWidget*
modest_main_window_new (ModestWidgetFactory *widget_factory)
{
	GObject *obj;
	ModestMainWindowPrivate *priv;
	
	GtkWidget *main_vbox;
	GtkWidget *status_hbox;
	GtkWidget *header_win, *folder_win, *favorites_win;
	
	g_return_val_if_fail (widget_factory, NULL);

	obj  = g_object_new(MODEST_TYPE_MAIN_WINDOW, NULL);
	priv = MODEST_MAIN_WINDOW_GET_PRIVATE(obj);

	g_object_ref (widget_factory);
	priv->widget_factory = widget_factory;

	/* widgets from factory */
	priv->folder_view = modest_widget_factory_get_folder_view (widget_factory);
	priv->header_view = header_view_new (MODEST_MAIN_WINDOW(obj));
	priv->msg_preview = modest_widget_factory_get_msg_preview (widget_factory);
	
	folder_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->folder_view),
						 FALSE);
	header_win = wrapped_in_scrolled_window (GTK_WIDGET(priv->header_view),
						 FALSE);			   	
	/*menu */
	priv->menu = get_menu (MODEST_MAIN_WINDOW(obj));
	hildon_window_set_menu (HILDON_WINDOW(obj), GTK_MENU(priv->menu));
	
	priv->toolbar = GTK_WIDGET(toolbar_new (MODEST_MAIN_WINDOW(obj)));
	
	/* paned */
	priv->msg_paned = gtk_vpaned_new ();
	priv->main_paned = gtk_hpaned_new ();

	gtk_paned_add1 (GTK_PANED(priv->main_paned), folder_win);
	gtk_paned_add2 (GTK_PANED(priv->main_paned), priv->msg_paned);

	gtk_paned_add1 (GTK_PANED(priv->msg_paned), header_win);
	gtk_paned_add2 (GTK_PANED(priv->msg_paned), GTK_WIDGET(priv->msg_preview));

	gtk_widget_show (GTK_WIDGET(priv->header_view));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(priv->header_view));

	/* status bar / progress */
	status_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			    modest_widget_factory_get_folder_info_label (widget_factory),
			    FALSE,FALSE, 6);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			    modest_widget_factory_get_status_bar(widget_factory),
			    TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			    modest_widget_factory_get_progress_bar(widget_factory),
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(status_hbox),
			  modest_widget_factory_get_online_toggle(widget_factory),
			  FALSE, FALSE, 0);

	/* putting it all together... */
	main_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->main_paned, TRUE, TRUE,0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, FALSE, 0);	
	gtk_box_pack_start (GTK_BOX(main_vbox), status_hbox, FALSE, FALSE, 0);
	
	gtk_container_add (GTK_CONTAINER(obj), main_vbox);
	restore_sizes (MODEST_MAIN_WINDOW(obj));	

	gtk_window_set_title (GTK_WINDOW(obj), "Modest");
	gtk_window_set_icon  (GTK_WINDOW(obj),
			      modest_icon_factory_get_icon (MODEST_APP_ICON));
	
	gtk_widget_show_all (main_vbox);

	g_signal_connect (G_OBJECT(obj), "delete-event",
			  G_CALLBACK(on_delete_event), obj);
	
	return GTK_WIDGET(obj);
}

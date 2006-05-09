/* modest-ui.c */

/* insert (c)/licensing information) */

#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include "modest-main-window.h"
#include "modest-edit-window.h"

#include "../modest-ui.h"
#include "../modest-window-mgr.h"
#include "../modest-account-mgr.h"


/* include other impl specific header files */

/* 'private'/'protected' functions */
static void   modest_ui_class_init     (ModestUIClass *klass);
static void   modest_ui_init           (ModestUI *obj);
static void   modest_ui_finalize       (GObject *obj);

static void    modest_ui_window_destroy    (GtkWidget *win, gpointer data);
static void    modest_ui_last_window_closed (GObject *obj, gpointer data);



/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};


typedef struct _ModestUIPrivate ModestUIPrivate;
struct _ModestUIPrivate {
	
	ModestConf           *modest_conf;
	ModestAccountMgr     *modest_acc_mgr;
	ModestWindowMgr      *modest_window_mgr;
	
	GtkWindow	     *main_window;
	GSList*		     *edit_window_list;	     

};
#define MODEST_UI_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MODEST_TYPE_UI, \
                                       ModestUIPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_ui_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestUIClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_ui_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestUI),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_ui_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestUI",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_ui_class_init (ModestUIClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_ui_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestUIPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_ui_init (ModestUI *obj)
{
 	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);

	priv->modest_acc_mgr    = NULL;
	priv->modest_conf       = NULL;
	priv->modest_window_mgr = NULL;

}

static void
modest_ui_finalize (GObject *obj)
{
	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);	
	
	if (priv->modest_acc_mgr)
		g_object_unref (priv->modest_acc_mgr);
	priv->modest_acc_mgr = NULL;
	
	if (priv->modest_conf)
		g_object_unref (priv->modest_conf);
	priv->modest_conf = NULL;
	
	if (priv->modest_window_mgr)
		g_object_unref (priv->modest_window_mgr);
	priv->modest_window_mgr = NULL;
}
	
GObject*
modest_ui_new (ModestConf *modest_conf)
{
	GObject *obj;
	ModestUIPrivate *priv;
	ModestAccountMgr *modest_acc_mgr;

	g_return_val_if_fail (modest_conf, NULL);

	obj = g_object_new(MODEST_TYPE_UI, NULL);	
	priv = MODEST_UI_GET_PRIVATE(obj);

	modest_acc_mgr = MODEST_ACCOUNT_MGR(modest_account_mgr_new (modest_conf));
	if (!modest_acc_mgr) {
		g_warning ("could not create ModestAccountMgr instance");
		g_object_unref (obj);
		return NULL;
	}
	
	priv->modest_acc_mgr = modest_acc_mgr;
	g_object_ref (priv->modest_conf = modest_conf);

	priv->modest_window_mgr = MODEST_WINDOW_MGR(modest_window_mgr_new());
	g_signal_connect (priv->modest_window_mgr, "last_window_closed",
			  G_CALLBACK(modest_ui_last_window_closed),
			  NULL);
	return obj;
}


gboolean
modest_ui_show_main_window (ModestUI *modest_ui)
{
	GtkWidget       *win;
	int              height, width;
	ModestUIPrivate *priv;

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	
	height = modest_conf_get_int (priv->modest_conf,
				      MODEST_CONF_MAIN_WINDOW_HEIGHT,NULL);
	width  = modest_conf_get_int (priv->modest_conf,
				      MODEST_CONF_MAIN_WINDOW_WIDTH,NULL);
	
	win = modest_main_window_new (priv->modest_conf,
				      priv->modest_acc_mgr);
	if (!win) {
		g_warning ("could not create main window");
		return FALSE;
	}

	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_MAIN_WINDOW, 0);
	
	g_signal_connect (win, "destroy", G_CALLBACK(modest_ui_window_destroy),
			  modest_ui);
	
	gtk_widget_set_usize (GTK_WIDGET(win), height, width);
	gtk_window_set_title (GTK_WINDOW(win), PACKAGE_STRING);
	
	gtk_widget_show (win);
	return TRUE;
}


gboolean
modest_ui_show_edit_window (ModestUI *modest_ui, const gchar* to,
			    const gchar* cc, const gchar* bcc,
			    const gchar* subject, const gchar *body,
			    const GSList* att)
{
	GtkWidget       *win;
	ModestUIPrivate *priv;

	priv = MODEST_UI_GET_PRIVATE(modest_ui);
	int height = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_HEIGHT,NULL);
	int width  = modest_conf_get_int (priv->modest_conf,
					  MODEST_CONF_EDIT_WINDOW_WIDTH,NULL);
	
	win = modest_edit_window_new (to, cc, bcc, subject, body, att);
	if (!win) {
		g_warning ("could not create edit window");
		return FALSE;
	}
	
	modest_window_mgr_register (priv->modest_window_mgr,
				    G_OBJECT(win), MODEST_EDIT_WINDOW, 0);

	g_signal_connect (win, "destroy", G_CALLBACK(modest_ui_window_destroy),
			  modest_ui);

	gtk_widget_set_usize (GTK_WIDGET(win), height, width);
	gtk_window_set_title (GTK_WINDOW(win),
			      subject ? subject : "Untitled");

	gtk_widget_show (win);

	return TRUE;
}


static void
modest_ui_window_destroy (GtkWidget *win, gpointer data)
{
	ModestUIPrivate *priv;

	g_return_if_fail (data);

	priv = MODEST_UI_GET_PRIVATE((ModestUI*)data);
	if (!modest_window_mgr_unregister (priv->modest_window_mgr, G_OBJECT(win)))
		g_warning ("modest window mgr: failed to unregister %p",
			   G_OBJECT(win));
}


static void
modest_ui_last_window_closed (GObject *obj, gpointer data)
{
	gtk_main_quit ();
}

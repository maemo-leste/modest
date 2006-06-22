/* modest-ui.c */

/* insert (c)/licensing information) */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib/gi18n.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

/* TODO: put in auto* */
#include <tny-text-buffer-stream.h>
#include <tny-msg-folder.h>

#include "../modest-ui.h"
#include "../modest-window-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-account-mgr.h"
#include "../modest-identity-mgr.h"

#include "../modest-tny-account-store.h"
#include "../modest-tny-folder-tree-view.h"
#include "../modest-tny-header-tree-view.h"
#include "../modest-tny-msg-view.h"
#include "../modest-tny-transport-actions.h"
#include "../modest-tny-store-actions.h"

#include "../modest-text-utils.h"
#include "../modest-tny-msg-actions.h"

#include "../modest-editor-window.h"

#include "modest-ui-glade.h"
#include "modest-ui-wizard.h"

/* 'private'/'protected' functions */
static void   modest_ui_class_init     (ModestUIClass *klass);
static void   modest_ui_init           (ModestUI *obj);
static void   modest_ui_finalize       (GObject *obj);

static void   modest_ui_window_destroy    (GtkWidget *win, GdkEvent *event, gpointer data);
static void   modest_ui_last_window_closed (GObject *obj, gpointer data);

gchar *on_password_requested (TnyAccountIface *, const gchar *, gboolean *);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

/* globals */
static GObjectClass *parent_class = NULL;


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

}


static void
modest_ui_init (ModestUI *obj)
{
 	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);

	priv->modest_acc_mgr    = NULL;
	priv->modest_id_mgr     = NULL;
	priv->modest_conf       = NULL;
	priv->modest_window_mgr = NULL;
	priv->glade_xml         = NULL;
	priv->folder_view       = NULL;
	priv->header_view       = NULL;
	priv->message_view      = NULL;
	priv->current_folder    = NULL;
}


static void
modest_ui_finalize (GObject *obj)
{
	ModestUIPrivate *priv = MODEST_UI_GET_PRIVATE(obj);

	if (priv->modest_acc_mgr)
		g_object_unref (priv->modest_acc_mgr);
	priv->modest_acc_mgr = NULL;

	if (priv->modest_id_mgr)
		g_object_unref (priv->modest_id_mgr);
	priv->modest_id_mgr = NULL;

	if (priv->modest_conf)
		g_object_unref (priv->modest_conf);
	priv->modest_conf = NULL;

	if (priv->modest_window_mgr)
		g_object_unref (priv->modest_window_mgr);
	priv->modest_window_mgr = NULL;
}


static void
on_accounts_reloaded (ModestTnyAccountStore *account_store, gpointer user_data)
{
	ModestUIPrivate *priv = user_data;

	g_return_if_fail (MODEST_IS_TNY_FOLDER_TREE_VIEW (priv->folder_view));
	g_return_if_fail (MODEST_IS_TNY_HEADER_TREE_VIEW (priv->header_view));

	modest_tny_header_tree_view_set_folder (priv->header_view, NULL);

  	modest_tny_folder_tree_view_update_model(priv->folder_view, account_store);
}


GObject*
modest_ui_new (ModestConf *modest_conf)
{
	GObject *obj;
	ModestUIPrivate *priv;
	ModestAccountMgr *modest_acc_mgr;
	ModestIdentityMgr *modest_id_mgr;
	TnyAccountStoreIface *account_store_iface;
	GSList *account_names_list;
	GSList *identities_list;

	g_return_val_if_fail (modest_conf, NULL);

	obj = g_object_new(MODEST_TYPE_UI, NULL);
	priv = MODEST_UI_GET_PRIVATE(obj);

	modest_acc_mgr = MODEST_ACCOUNT_MGR(modest_account_mgr_new (modest_conf));
	if (!modest_acc_mgr) {
		g_warning ("could not create ModestAccountMgr instance");
		g_object_unref (obj);
		return NULL;
        }

	modest_id_mgr = MODEST_IDENTITY_MGR(modest_identity_mgr_new (modest_conf));
	if (!modest_id_mgr) {
		g_warning ("could not create ModestIdentityMgr instance");
		g_object_unref (obj);
		return NULL;
	}

	account_store_iface =
		TNY_ACCOUNT_STORE_IFACE(modest_tny_account_store_new (modest_acc_mgr));
	if (!account_store_iface) {
		g_warning ("could not initialze ModestTnyAccountStore");
		return NULL;
        }

        modest_tny_account_store_set_get_pass_func(MODEST_TNY_ACCOUNT_STORE(account_store_iface),
                                                   on_password_requested);

	g_signal_connect (account_store_iface, "accounts_reloaded",
			  G_CALLBACK(on_accounts_reloaded), priv);

	glade_init ();
	priv->glade_xml = glade_xml_new (MODEST_GLADE, NULL, NULL);
	if (!priv->glade_xml) {
		g_warning ("failed to do glade stuff");
		g_object_unref (obj);
		return NULL;
	}

	/* FIXME: could be used, but doesn't work atm.
	 * glade_xml_signal_autoconnect(priv->glade_xml);
	 */

	priv->modest_acc_mgr = modest_acc_mgr;
	priv->modest_id_mgr  = modest_id_mgr;
	g_object_ref (priv->modest_conf = modest_conf);

	priv->account_store = account_store_iface;

	priv->modest_window_mgr = MODEST_WINDOW_MGR(modest_window_mgr_new());
	g_signal_connect (priv->modest_window_mgr, "last_window_closed",
			  G_CALLBACK(modest_ui_last_window_closed),
			  NULL);

	account_names_list = modest_account_mgr_server_account_names(modest_acc_mgr,
					NULL, MODEST_PROTO_TYPE_ANY, NULL, FALSE);
	identities_list = modest_identity_mgr_identity_names(modest_id_mgr, NULL);
	if (!(account_names_list != NULL || identities_list != NULL))
		wizard_account_dialog(MODEST_UI(obj));
	g_slist_free(account_names_list);
	g_slist_free(identities_list);

	return obj;
}


static void
modest_ui_last_window_closed (GObject *obj, gpointer data)
{
	/* FIXME: Other cleanups todo? Finalize Tinymail? */
	gtk_main_quit ();
}


gchar *
on_password_requested (TnyAccountIface *account,
		       const gchar *prompt,
		       gboolean *cancel) {

	GtkWidget *passdialog;
	GtkWidget *vbox;
	GtkWidget *infoscroll;
	GtkWidget *infolabel;
	GtkWidget *passentry;
	GtkTextBuffer *infobuffer;
	gchar *retval;
	gint result;

	passdialog = gtk_dialog_new_with_buttons(_("Password"),
						 NULL,
						 GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_STOCK_OK,
						 GTK_RESPONSE_ACCEPT,
						 GTK_STOCK_CANCEL,
						 GTK_RESPONSE_REJECT,
						 NULL);

	vbox = gtk_vbox_new(FALSE, 0);

	infobuffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_set_text(infobuffer, prompt, -1);
	infoscroll = gtk_scrolled_window_new(NULL, NULL);
	infolabel = gtk_text_view_new_with_buffer(infobuffer);
	gtk_container_add(GTK_CONTAINER(infoscroll), infolabel);
	passentry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(passentry), FALSE);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(passdialog)->vbox), infoscroll, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(passdialog)->vbox), passentry, FALSE, FALSE, 0);
	gtk_widget_show_all(passdialog);

	result = gtk_dialog_run (GTK_DIALOG(passdialog));

	switch (result) {
	case GTK_RESPONSE_ACCEPT:
		retval = g_strdup(gtk_entry_get_text(GTK_ENTRY(passentry)));
		*cancel=FALSE;
		break;
	default:
		retval = g_strdup("");;
		*cancel=TRUE;
		break;
	}

        gtk_widget_hide(passdialog);
	gtk_widget_destroy(passdialog);
	while (gtk_events_pending()){
		gtk_main_iteration();
	}

	return retval;
}


void
on_account_selector_selection_changed (GtkWidget *widget, gpointer user_data)
{
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	GtkTreeIter iter;

	gchar *account_name;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
				   0, &account_name, -1);
	} else {
		account_name="empty";
	}

	free(account_name);
}



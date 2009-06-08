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
#include <tny-account-store.h>
#include <tny-simple-list.h>
#include <modest-tny-msg.h>
#include "modest-icon-names.h"
#include "modest-ui-actions.h"
#include <modest-widget-memory.h>
#include <modest-runtime.h>
#include <modest-main-window-ui.h>
#include "modest-msg-view-window-ui-dimming.h"
#include "modest-defs.h"
#include <widgets/modest-msg-view-window.h>
#include <widgets/modest-window-priv.h>
#include "widgets/modest-msg-view.h"
#include "modest-ui-dimming-manager.h"


static void  modest_msg_view_window_class_init   (ModestMsgViewWindowClass *klass);
static void  modest_msg_view_window_init         (ModestMsgViewWindow *obj);
static void  modest_msg_view_window_finalize     (GObject *obj);

static void  modest_msg_view_window_toggle_find_toolbar (GtkToggleAction *toggle,
							 gpointer data);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestMsgViewWindowPrivate ModestMsgViewWindowPrivate;
struct _ModestMsgViewWindowPrivate {
	GtkWidget   *toolbar;
	GtkWidget   *menubar;
	GtkWidget   *msg_view;

	gchar *msg_uid;
	TnyMimePart *other_body;
};

#define MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                    MODEST_TYPE_MSG_VIEW_WINDOW, \
                                                    ModestMsgViewWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;

static const GtkToggleActionEntry msg_view_toggle_action_entries [] = {
	{ "FindInMessage",    GTK_STOCK_FIND,    N_("mcen_me_viewer_find"), NULL, NULL, G_CALLBACK (modest_msg_view_window_toggle_find_toolbar), FALSE },
	{ "ToolsFindInMessage", NULL, N_("mcen_me_viewer_find"), "<CTRL>F", NULL, G_CALLBACK (modest_msg_view_window_toggle_find_toolbar), FALSE },
};

static const GtkRadioActionEntry msg_view_zoom_action_entries [] = {
	{ "Zoom50", NULL, N_("mcen_me_viewer_50"), NULL, NULL, 50 },
	{ "Zoom80", NULL, N_("mcen_me_viewer_80"), NULL, NULL, 80 },
	{ "Zoom100", NULL, N_("mcen_me_viewer_100"), NULL, NULL, 100 },
	{ "Zoom120", NULL, N_("mcen_me_viewer_120"), NULL, NULL, 120 },
	{ "Zoom150", NULL, N_("mcen_me_viewer_150"), NULL, NULL, 150 },
	{ "Zoom200", NULL, N_("mcen_me_viewer_200"), NULL, NULL, 200 }
};

GType
modest_msg_view_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMsgViewWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_msg_view_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestMsgViewWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_msg_view_window_init,
			NULL
		};
		my_type = g_type_register_static (MODEST_TYPE_WINDOW,
		                                  "ModestMsgViewWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
save_state (ModestWindow *self)
{
	modest_widget_memory_save (modest_runtime_get_conf (),
				   G_OBJECT(self), 
				   MODEST_CONF_MSG_VIEW_WINDOW_KEY);
}


static void
restore_settings (ModestWindow *self)
{
	modest_widget_memory_restore (modest_runtime_get_conf (),
				      G_OBJECT(self), 
				      MODEST_CONF_MSG_VIEW_WINDOW_KEY);
}

static void
modest_msg_view_window_class_init (ModestMsgViewWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_msg_view_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestMsgViewWindowPrivate));

	ModestWindowClass *modest_window_class = (ModestWindowClass *) klass;
	modest_window_class->save_state_func = save_state;
}

static void
modest_msg_view_window_init (ModestMsgViewWindow *obj)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);

	priv->toolbar  = NULL;
	priv->menubar  = NULL;
	priv->msg_view = NULL;
	priv->msg_uid  = NULL;
	priv->other_body = NULL;
}


static void
init_window (ModestMsgViewWindow *obj, TnyMsg *msg, TnyMimePart *other_body)
{
	GtkWidget *main_vbox, *scrolled_window;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_view = GTK_WIDGET (tny_platform_factory_new_msg_view (modest_tny_platform_factory_get_instance ()));
	if (other_body) {
		priv->other_body = g_object_ref (other_body);
		modest_msg_view_set_msg_with_other_body (MODEST_MSG_VIEW (priv->msg_view), msg, other_body);
	} else {
		tny_msg_view_set_msg (TNY_MSG_VIEW (priv->msg_view), msg);
	}
	main_vbox = gtk_vbox_new  (FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->menubar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(main_vbox), priv->toolbar, FALSE, FALSE, 0);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled_window), 
			   priv->msg_view);
	gtk_box_pack_start (GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);

	gtk_widget_show_all (GTK_WIDGET(main_vbox));
	gtk_container_add   (GTK_CONTAINER(obj), main_vbox);
}


static void
modest_msg_view_window_finalize (GObject *obj)
{
	ModestMsgViewWindowPrivate *priv;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);

	if (priv->other_body != NULL) {
		g_object_unref (priv->other_body);
		priv->other_body = NULL;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}



static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, ModestMsgViewWindow *self)
{
	modest_window_save_state (MODEST_WINDOW(self));
	return FALSE;
}


gboolean
modest_msg_view_window_is_other_body (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv = NULL;

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	return (priv->other_body != NULL);
}

ModestWindow *
modest_msg_view_window_new_with_other_body (TnyMsg *msg, 
					    TnyMimePart *other_body,
					    const gchar *modest_account_name, 
					    const gchar *mailbox, /* ignored */
					    const gchar *msg_uid)
{
	GObject *obj;
	ModestMsgViewWindowPrivate *priv;
	ModestWindowPrivate *parent_priv;
	GtkActionGroup *action_group;
	GError *error = NULL;
	TnyHeader *header = NULL;
	gchar *subject = NULL;
	ModestDimmingRulesGroup *menu_rules_group = NULL;
	ModestDimmingRulesGroup *toolbar_rules_group = NULL;
	ModestDimmingRulesGroup *clipboard_rules_group = NULL;

	g_return_val_if_fail (msg, NULL);

	obj = g_object_new(MODEST_TYPE_MSG_VIEW_WINDOW, NULL);
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(obj);
	parent_priv = MODEST_WINDOW_GET_PRIVATE(obj);

	priv->msg_uid = g_strdup (msg_uid);

	modest_window_set_active_account (MODEST_WINDOW(obj), modest_account_name);
	
	parent_priv->ui_manager = gtk_ui_manager_new();
	action_group = gtk_action_group_new ("ModestMsgViewWindowActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	/* Add common actions */
	gtk_action_group_add_actions (action_group,
				      modest_action_entries,
				      G_N_ELEMENTS (modest_action_entries),
				      obj);
	gtk_action_group_add_toggle_actions (action_group,
					     modest_toggle_action_entries,
					     G_N_ELEMENTS (modest_toggle_action_entries),
					     obj);
	gtk_action_group_add_toggle_actions (action_group,
					     msg_view_toggle_action_entries,
					     G_N_ELEMENTS (msg_view_toggle_action_entries),
					     obj);
	gtk_action_group_add_radio_actions (action_group,
					    msg_view_zoom_action_entries,
					    G_N_ELEMENTS (msg_view_zoom_action_entries),
					    100,
					    G_CALLBACK (modest_ui_actions_on_change_zoom),
					    obj);
	gtk_ui_manager_insert_action_group (parent_priv->ui_manager, action_group, 0);
	g_object_unref (action_group);

	
	/* Load the UI definition */
	gtk_ui_manager_add_ui_from_file (parent_priv->ui_manager,
					 MODEST_UIDIR "modest-msg-view-window-ui.xml",
					 &error);
	if (error) {
		g_printerr ("modest: could not merge modest-msg-view-window-ui.xml: %s\n", error->message);
		g_error_free (error);
		error = NULL;
	}
	/* ****** */

	/* Add accelerators */
	gtk_window_add_accel_group (GTK_WINDOW (obj), 
				    gtk_ui_manager_get_accel_group (parent_priv->ui_manager));

	/* Toolbar / Menubar */
	priv->toolbar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/ToolBar");
	priv->menubar = gtk_ui_manager_get_widget (parent_priv->ui_manager, "/MenuBar");

	gtk_toolbar_set_tooltips (GTK_TOOLBAR (priv->toolbar), TRUE);

	/* Init window */
	init_window (MODEST_MSG_VIEW_WINDOW(obj), msg, other_body);
	restore_settings (MODEST_WINDOW(obj));

	header = tny_msg_get_header (msg);
	if (other_body) {
		gchar *description;

		description = modest_tny_mime_part_get_header_value (other_body, "Content-Description");
		if (description) {
			g_strstrip (description);
			subject = description;
		}
	} else {
		if (header)
			subject = tny_header_dup_subject (header);
	}

	
	if (subject != NULL)
		gtk_window_set_title (GTK_WINDOW (obj), subject);
	else
		gtk_window_set_title (GTK_WINDOW(obj), "Modest");
	g_free (subject);

	if (header)
		g_object_unref (header);

	gtk_window_set_icon_from_file (GTK_WINDOW(obj), MODEST_APP_ICON, NULL);

	parent_priv->ui_dimming_manager = modest_ui_dimming_manager_new();

	menu_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_MENU, FALSE);
	toolbar_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_TOOLBAR, TRUE);
	clipboard_rules_group = modest_dimming_rules_group_new (MODEST_DIMMING_RULES_CLIPBOARD, FALSE);

	/* Add common dimming rules */
	modest_dimming_rules_group_add_rules (menu_rules_group, 
					      modest_msg_view_menu_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_menu_dimming_entries),
					      MODEST_WINDOW (obj));
	modest_dimming_rules_group_add_rules (toolbar_rules_group, 
					      modest_msg_view_toolbar_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_toolbar_dimming_entries),
					      MODEST_WINDOW (obj));
	modest_dimming_rules_group_add_rules (clipboard_rules_group, 
					      modest_msg_view_clipboard_dimming_entries,
					      G_N_ELEMENTS (modest_msg_view_clipboard_dimming_entries),
					      MODEST_WINDOW (obj));

	/* Insert dimming rules group for this window */
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, menu_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, toolbar_rules_group);
	modest_ui_dimming_manager_insert_rules_group (parent_priv->ui_dimming_manager, clipboard_rules_group);
	g_object_unref (menu_rules_group);
	g_object_unref (toolbar_rules_group);
	g_object_unref (clipboard_rules_group);

	g_signal_connect (G_OBJECT(obj), "delete-event", G_CALLBACK(on_delete_event), obj);

	return MODEST_WINDOW(obj);
}

ModestWindow *
modest_msg_view_window_new_for_attachment (TnyMsg *msg, 
					   const gchar *modest_account_name, 
					   const gchar *mailbox, /* ignored */
					   const gchar *msg_uid)
{

	return modest_msg_view_window_new_with_other_body (msg, NULL, modest_account_name, mailbox, msg_uid);

}


TnyMsg*
modest_msg_view_window_get_message (ModestMsgViewWindow *self)
{
	GtkWidget *msg_view;	
	g_return_val_if_fail (self, NULL);

	msg_view = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE(self)->msg_view;

	return tny_msg_view_get_msg (TNY_MSG_VIEW(msg_view));
}

const gchar*
modest_msg_view_window_get_message_uid (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);
	TnyMsg *msg;
	TnyHeader *header;
	const gchar *retval = NULL;

	msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
	msg = modest_msg_view_window_get_message (self);

	if (!msg)
		return NULL;

	header = tny_msg_get_header (msg);
	g_free (priv->msg_uid);
	if (header) {
		priv->msg_uid = tny_header_dup_uid (header);
		g_object_unref (header);
	}
	g_object_unref (msg);

	return priv->msg_uid;
}

ModestWindow *
modest_msg_view_window_new_from_uid (const gchar *modest_account_name,
				     const gchar *mailbox,
				     const gchar *msg_uid)
{
	/* NOT IMPLEMENTED */
	return NULL;
}

ModestWindow*   
modest_msg_view_window_new_with_header_model (TnyMsg *msg, 
					      const gchar *modest_account_name, 
					      const gchar *mailbox, /*ignored*/
					      const gchar *msg_uid,
					      GtkTreeModel *model, 
					      GtkTreeRowReference *row_reference)
{
	/* Currently we simply redirect to new constructor. It should store a
	   reference to the header list model, to enable next/prev message
	   actions */
	g_debug ("partially implemented %s", __FUNCTION__);

	return modest_msg_view_window_new_for_attachment (msg, modest_account_name, NULL, msg_uid);
}


gboolean
modest_msg_view_window_select_next_message (ModestMsgViewWindow *window)
{
	g_warning ("not implemented %s", __FUNCTION__);
	return FALSE;
}

gboolean
modest_msg_view_window_select_previous_message (ModestMsgViewWindow *window)
{
	g_warning ("not implemented %s", __FUNCTION__);
	return FALSE;
}

void
modest_msg_view_window_view_attachment (ModestMsgViewWindow *window, TnyMimePart *mime_part)
{
	g_warning ("not implemented %s", __FUNCTION__);
}

void
modest_msg_view_window_save_attachments (ModestMsgViewWindow *window, TnyList *mime_parts)
{
	g_warning ("not implemented %s", __FUNCTION__);
}
void
modest_msg_view_window_remove_attachments (ModestMsgViewWindow *window, gboolean get_all)
{
	g_warning ("not implemented %s", __FUNCTION__);
}

TnyHeader *
modest_msg_view_window_get_header (ModestMsgViewWindow *self)
{
	TnyMsg *msg;
	TnyHeader *header = NULL;

	msg = modest_msg_view_window_get_message (self);
	if (msg) {
		header = tny_msg_get_header (msg);
		g_object_unref (msg);
	}
	return header;
}

TnyFolderType
modest_msg_view_window_get_folder_type (ModestMsgViewWindow *window)
{
	ModestMsgViewWindowPrivate *priv;
	TnyMsg *msg;
	TnyFolderType folder_type;

	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (window);

	folder_type = TNY_FOLDER_TYPE_UNKNOWN;

	msg = tny_msg_view_get_msg (TNY_MSG_VIEW (priv->msg_view));
	if (msg) {
		TnyFolder *folder;

		folder = tny_msg_get_folder (msg);
		if (folder) {
			folder_type = tny_folder_get_folder_type (folder);
			g_object_unref (folder);
		}
		g_object_unref (msg);
	}

	return folder_type;
}

/* NOT IMPLEMENTED METHODS */

gboolean 
modest_msg_view_window_last_message_selected (ModestMsgViewWindow *window)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return TRUE;
}

gboolean 
modest_msg_view_window_first_message_selected (ModestMsgViewWindow *window)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return TRUE;
}

gboolean 
modest_msg_view_window_transfer_mode_enabled (ModestMsgViewWindow *self)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return FALSE;
}

gboolean  
modest_msg_view_window_toolbar_on_transfer_mode     (ModestMsgViewWindow *self)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return FALSE;
}


TnyList *         
modest_msg_view_window_get_attachments (ModestMsgViewWindow *win)
{
	TnyList *result;

	result = tny_simple_list_new ();
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return result;	
}

gboolean 
modest_msg_view_window_is_search_result (ModestMsgViewWindow *window)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return FALSE;	
}

gboolean 
modest_msg_view_window_has_headers_model (ModestMsgViewWindow *window)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return FALSE;	
}

static void 
modest_msg_view_window_toggle_find_toolbar (GtkToggleAction *toggle,
					    gpointer data)
{
	g_warning ("NOT IMPLEMENTED %s", __FUNCTION__);
	return FALSE;	
}

void
modest_msg_view_window_add_to_contacts (ModestMsgViewWindow *self)
{
	modest_ui_actions_on_add_to_contacts (NULL, MODEST_WINDOW (self));
}

void
modest_msg_view_window_fetch_images (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	modest_msg_view_request_fetch_images (MODEST_MSG_VIEW (priv->msg_view));
}

gboolean 
modest_msg_view_window_has_blocked_external_images (ModestMsgViewWindow *self)
{
	ModestMsgViewWindowPrivate *priv;
	priv = MODEST_MSG_VIEW_WINDOW_GET_PRIVATE (self);

	g_return_val_if_fail (MODEST_IS_MSG_VIEW_WINDOW (self), FALSE);

	return modest_msg_view_has_blocked_external_images (MODEST_MSG_VIEW (priv->msg_view));
}

void
modest_msg_view_window_reload (ModestMsgViewWindow *self)
{
	/* Not implemented */
	return;
}

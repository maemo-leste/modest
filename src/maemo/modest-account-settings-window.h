/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _MODEST_ACCOUNT_SETTINGS_DIALOG
#define _MODEST_ACCOUNT_SETTINGS_DIALOG

#include <gtk/gtkdialog.h> 
#include "modest-account-mgr.h"
#include <hildon-widgets/hildon-caption.h>

G_BEGIN_DECLS

#define MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG modest_account_settings_dialog_get_type()

#define MODEST_ACCOUNT_SETTINGS_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, ModestAccountSettingsDialog))

#define MODEST_ACCOUNT_SETTINGS_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, ModestAccountSettingsDialogClass))

#define ACCOUNT_IS_WIZARD_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG))

#define MODEST_EASYSETUP_IS_WIZARD_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG))

#define MODEST_ACCOUNT_SETTINGS_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_ACCOUNT_SETTINGS_DIALOG, ModestAccountSettingsDialogClass))

typedef struct {
	GtkDialog parent;
	
	/* Used by derived widgets to query existing accounts,
	 * and to create new accounts: */
	ModestAccountMgr *account_manager;
	
	GtkWidget *page_account_details;
	GtkWidget *combo_account_country;
	GtkWidget *combo_account_serviceprovider;
	GtkWidget *entry_account_title;
	
	GtkWidget *page_user_details;
	GtkWidget *entry_user_name;
	GtkWidget *entry_user_username;
	GtkWidget *entry_user_password;
	GtkWidget *entry_user_email;
	
	GtkWidget *page_complete_easysetup;
	
	GtkWidget *page_incoming;
	GtkWidget *combo_incoming_servertype;
	GtkWidget *caption_incoming;
	GtkWidget *entry_incomingserver;
	GtkWidget *combo_incoming_security;
	GtkWidget *checkbox_incoming_auth;

	GtkWidget *page_outgoing;
	GtkWidget *entry_outgoingserver;
	GtkWidget *combo_outgoing_security;
	GtkWidget *combo_outgoing_auth;
	GtkWidget *checkbox_outgoing_smtp_specific;
	GtkWidget *button_outgoing_smtp_servers;
	
} ModestAccountSettingsDialog;

typedef struct {
	GtkDialogClass parent_class;
	
} ModestAccountSettingsDialogClass;

GType modest_account_settings_dialog_get_type (void);

ModestAccountSettingsDialog* modest_account_settings_dialog_new (void);

void modest_account_settings_dialog_set_account_name (const gchar* account_name);

G_END_DECLS

#endif /* _MODEST_ACCOUNT_SETTINGS_DIALOG */

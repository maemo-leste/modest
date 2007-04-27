/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
 */

#ifndef _MODEST_EAYSETUP_WIZARD_DIALOG
#define _MODEST_EAYSETUP_WIZARD_DIALOG

/* #include <hildon-widgets/hildon-wizard-dialog.h> */
#include "modest-wizard-dialog.h" /* We use a copied-and-improved HildonWizardDialog. */
#include "modest-account-mgr.h"

#ifdef MODEST_HILDON_VERSION_0
#include <hildon-widgets/hildon-caption.h>
#else
#include <hildon/hildon-caption.h>
#endif /*MODEST_HILDON_VERSION_0*/

G_BEGIN_DECLS

#define MODEST_TYPE_EASYSETUP_WIZARD_DIALOG modest_easysetup_wizard_dialog_get_type()

#define MODEST_EASYSETUP_WIZARD_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialog))

#define MODEST_EASYSETUP_WIZARD_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialogClass))

#define ACCOUNT_IS_WIZARD_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG))

#define MODEST_EASYSETUP_IS_WIZARD_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG))

#define MODEST_EASYSETUP_WIZARD_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialogClass))

typedef struct {
	ModestWizardDialog parent;
	
	/* Used by derived widgets to query existing accounts,
	 * and to create new accounts: */
	ModestAccountMgr *account_manager;
	
	/* Whether we saved the account before we were finished, 
	 * to allow editing via the Advanced Settings dialog.
	 * We might need to delete the account if Finish is never clicked. */
	gchar* saved_account_name;
		
	/* notebook pages: */
	GtkWidget *page_welcome;
	
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
	
	GtkWidget *page_custom_incoming;
	GtkWidget *combo_incoming_servertype;
	GtkWidget *caption_incoming;
	GtkWidget *entry_incomingserver;
	GtkWidget *combo_incoming_security;
	GtkWidget *checkbox_incoming_auth;

	GtkWidget *page_custom_outgoing;
	GtkWidget *entry_outgoingserver;
	GtkWidget *combo_outgoing_security;
	GtkWidget *combo_outgoing_auth;
	GtkWidget *checkbox_outgoing_smtp_specific;
	GtkWidget *button_outgoing_smtp_servers;
	
	GtkWidget *page_complete_customsetup;
	GtkWidget *button_edit;
	
	GtkWidget *specific_window;
	
} ModestEasysetupWizardDialog;

typedef struct {
	ModestWizardDialogClass parent_class;
	
} ModestEasysetupWizardDialogClass;

GType modest_easysetup_wizard_dialog_get_type (void);

ModestEasysetupWizardDialog* modest_easysetup_wizard_dialog_new (void);

G_END_DECLS

#endif /* _MODEST_EAYSETUP_WIZARD_DIALOG */

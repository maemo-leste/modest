/*
 * This is a copy of modest-wizard-dialog.h with a rename and some API additions,
 * for osso-modest-easysetup.
 * 
 * This file was part of modest-libs
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation, all rights reserved.
 *
 */
 
#ifndef __MODEST_WIZARD_DIALOG_H__
#define __MODEST_WIZARD_DIALOG_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define MODEST_TYPE_WIZARD_DIALOG (modest_wizard_dialog_get_type())

#define MODEST_WIZARD_DIALOG(obj) (GTK_CHECK_CAST ((obj), \
            MODEST_TYPE_WIZARD_DIALOG, ModestWizardDialog))

#define MODEST_WIZARD_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), \
            MODEST_TYPE_WIZARD_DIALOG, ModestWizardDialogClass))

#define MODEST_IS_WIZARD_DIALOG(obj) (GTK_CHECK_TYPE ((obj), \
            MODEST_TYPE_WIZARD_DIALOG))

#define MODEST_IS_WIZARD_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
            MODEST_TYPE_WIZARD_DIALOG))
            
#define MODEST_WIZARD_DIALOG_GET_CLASS(obj) \
            (G_TYPE_INSTANCE_GET_CLASS ((obj), \
            MODEST_TYPE_WIZARD_DIALOG, ModestWizardDialogClass))

typedef struct _ModestWizardDialog ModestWizardDialog;

typedef struct _ModestWizardDialogClass ModestWizardDialogClass;

typedef struct _ModestWizardDialogPrivate ModestWizardDialogPrivate;

/* button response IDs */
enum {
    MODEST_WIZARD_DIALOG_CANCEL = GTK_RESPONSE_CANCEL,
    MODEST_WIZARD_DIALOG_PREVIOUS = 0,
    MODEST_WIZARD_DIALOG_NEXT,
    MODEST_WIZARD_DIALOG_FINISH
};

struct _ModestWizardDialog {
    GtkDialog                   parent;
    ModestWizardDialogPrivate   *priv;
};

struct _ModestWizardDialogClass {
    GtkDialogClass          parent_class;
    
	/** Implementations of this vfunc should prepare the next page if necessary, 
	 * and only return TRUE if the navigation should be allowed.
	 * You may even change the next page, via the GtkNotebook API, in the signal handler. */
	gboolean (* before_next) (ModestWizardDialog *dialog, GtkWidget *current_page, GtkWidget *next_page);

	/** Implementations of this vfunc should enable or disable 
	 * the next/forward buttons appropriately, based on the entered data. */
	void (* enable_buttons) (ModestWizardDialog *dialog, GtkWidget *current_page);


    void (*_gtk_reserved2)  (void);
    void (*_gtk_reserved3)  (void);
    void (*_gtk_reserved4)  (void);
};


GType modest_wizard_dialog_get_type   (void) G_GNUC_CONST;

GtkWidget* modest_wizard_dialog_new   (GtkWindow        *parent,
                                       const char       *wizard_name,
                                       GtkNotebook      *notebook);

G_END_DECLS

#endif /* __MODEST_WIZARD_DIALOG_H__ */

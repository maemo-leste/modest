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


#ifndef __MODEST_WIZARD_DIALOG_H__
#define __MODEST_WIZARD_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODEST_TYPE_WIZARD_DIALOG (modest_wizard_dialog_get_type())

#define MODEST_WIZARD_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
            MODEST_TYPE_WIZARD_DIALOG, ModestWizardDialog))

#define MODEST_WIZARD_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
            MODEST_TYPE_WIZARD_DIALOG, ModestWizardDialogClass))

#define MODEST_IS_WIZARD_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
            MODEST_TYPE_WIZARD_DIALOG))

#define MODEST_IS_WIZARD_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
            MODEST_TYPE_WIZARD_DIALOG))
            
#define MODEST_WIZARD_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
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


    void (* update_model)  (ModestWizardDialog *dialog);
    gboolean (*save)  (ModestWizardDialog *dialog);
    void (*_gtk_reserved4)  (void);
};

/*
 * Returning %TRUE means you don't allow further processing of the event in ModestWizardDialog
 */
typedef gboolean (* ModestWizardDialogResponseOverrideFunc) (ModestWizardDialog *dialog, 
							     gint response_id, gint page_number);


GType modest_wizard_dialog_get_type   (void) G_GNUC_CONST;

GtkWidget* modest_wizard_dialog_new   (GtkWindow        *parent,
                                       const char       *wizard_name,
                                       GtkNotebook      *notebook);
                                       
void modest_wizard_dialog_force_title_update (ModestWizardDialog* wizard_dialog);
void modest_wizard_dialog_update_model (ModestWizardDialog *wizard_dialog);
gboolean modest_wizard_dialog_save (ModestWizardDialog *wizard_dialog);

void modest_wizard_dialog_set_response_override_handler (ModestWizardDialog *wizard_dialog,
							 ModestWizardDialogResponseOverrideFunc callback);

G_END_DECLS

#endif /* __MODEST_WIZARD_DIALOG_H__ */

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

#ifndef _MODEST_EAYSETUP_WIZARD_DIALOG
#define _MODEST_EAYSETUP_WIZARD_DIALOG

/* #include <hildon-widgets/hildon-wizard-dialog.h> */
#include "widgets/modest-wizard-dialog.h" /* We use a copied-and-improved HildonWizardDialog. */
#include "modest-account-mgr.h"
#include <config.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <hildon/hildon-caption.h>

G_BEGIN_DECLS

#define MODEST_TYPE_EASYSETUP_WIZARD_DIALOG modest_easysetup_wizard_dialog_get_type()

#define MODEST_EASYSETUP_WIZARD_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialog))

#define MODEST_EASYSETUP_WIZARD_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialogClass))

#define MODEST_IS_EASYSETUP_WIZARD_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),				\
				     MODEST_TYPE_EASYSETUP_WIZARD_DIALOG))

#define MODEST_EASYSETUP_IS_WIZARD_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG))

#define MODEST_EASYSETUP_WIZARD_DIALOG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
	MODEST_TYPE_EASYSETUP_WIZARD_DIALOG, ModestEasysetupWizardDialogClass))

typedef struct {
	ModestWizardDialog parent;
	
} ModestEasysetupWizardDialog;

typedef struct {
	ModestWizardDialogClass parent_class;
	
} ModestEasysetupWizardDialogClass;

GType modest_easysetup_wizard_dialog_get_type (void);

/*
 * NOTE: can be instantiated only once; after that, this function will
 * return NULL, until the one before is destroyed; if it returns NULL
 * it will gtk_window_present the existing one.
 */
ModestEasysetupWizardDialog* modest_easysetup_wizard_dialog_new (void);

G_END_DECLS

#endif /* _MODEST_EAYSETUP_WIZARD_DIALOG */

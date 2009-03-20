/* Copyright (c) 2008, Nokia Corporation
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

#ifndef __MODEST_EASYSETUP_WIZARD_PAGE_H__
#define __MODEST_EASYSETUP_WIZARD_PAGE_H__

/* other include files */
#include <glib.h>
#include <glib-object.h>
#include "modest-account-settings.h"

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_EASYSETUP_WIZARD_PAGE             (modest_easysetup_wizard_page_get_type())
#define MODEST_EASYSETUP_WIZARD_PAGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_EASYSETUP_WIZARD_PAGE,ModestEasysetupWizardPage))
#define MODEST_IS_EASYSETUP_WIZARD_PAGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_EASYSETUP_WIZARD_PAGE))
#define MODEST_EASYSETUP_WIZARD_PAGE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE((obj),MODEST_TYPE_EASYSETUP_WIZARD_PAGE,ModestEasysetupWizardPageClass))

typedef struct _ModestEasysetupWizardPage      ModestEasysetupWizardPage;
typedef struct _ModestEasysetupWizardPageClass ModestEasysetupWizardPageClass;

struct _ModestEasysetupWizardPageClass {
	GTypeInterface parent_class;

	/* Functions to be redefined */
	gboolean (*validate) (ModestEasysetupWizardPage* self);
	void (*save_settings) (ModestEasysetupWizardPage* self, ModestAccountSettings *settings);

	/* Signals */
	void (*missing_mandatory_data) (ModestEasysetupWizardPage* self, 
					gboolean missing, 
					gpointer user_data);

	/* Padding for the future */
	void (*_reserved1) (void);
	void (*_reserved2) (void);
	void (*_reserved3) (void);
	void (*_reserved4) (void);
	void (*_reserved5) (void);
	void (*_reserved6) (void);
	void (*_reserved7) (void);
	void (*_reserved8) (void);
};

GType        modest_easysetup_wizard_page_get_type    (void) G_GNUC_CONST;

gboolean     modest_easysetup_wizard_page_validate    (ModestEasysetupWizardPage *self);

void         modest_easysetup_wizard_page_save_settings    (ModestEasysetupWizardPage *self,
							    ModestAccountSettings *settings);

G_END_DECLS

#endif /* __MODEST_EASYSETUP_WIZARD_PAGE_H__ */

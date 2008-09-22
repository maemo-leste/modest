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

#include "widgets/modest-easysetup-wizard-page.h"

enum {
	MISSING_MANDATORY_DATA_SIGNAL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void modest_easysetup_wizard_page_base_init (gpointer g_class);

gboolean
modest_easysetup_wizard_page_validate (ModestEasysetupWizardPage *self)
{
	return MODEST_EASYSETUP_WIZARD_PAGE_GET_IFACE(self)->validate (self);
}

void
modest_easysetup_wizard_page_save_settings (ModestEasysetupWizardPage *self,
					    ModestAccountSettings *settings)
{
	return MODEST_EASYSETUP_WIZARD_PAGE_GET_IFACE(self)->save_settings (self, settings);
}

static void
modest_easysetup_wizard_page_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
		signals[MISSING_MANDATORY_DATA_SIGNAL] =
			g_signal_new ("missing-mandatory-data",
				      MODEST_TYPE_EASYSETUP_WIZARD_PAGE,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestEasysetupWizardPageClass, missing_mandatory_data),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__BOOLEAN,
				      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
		initialized = TRUE;
	}
}
GType
modest_easysetup_wizard_page_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestEasysetupWizardPageClass),
			modest_easysetup_wizard_page_base_init,		/* base init */
			NULL,		/* base finalize */
			NULL,		/* class_init */
			NULL,		/* class finalize */
			NULL,		/* class data */
			0,
			0,		/* n_preallocs */
			NULL,		/* instance init */
		};
		my_type = g_type_register_static (G_TYPE_INTERFACE,
		                                  "ModestEasysetupWizardPage",
		                                  &my_info, 0);
		g_type_interface_add_prerequisite (my_type, G_TYPE_OBJECT);
	}
	return my_type;
}

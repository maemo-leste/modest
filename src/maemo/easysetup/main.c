/*
 * Copyright (C) 2008 Nokia Corporation, all rights reserved.
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

#include <config.h> /* For GETTEXT_PACKAGE, etc */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmain.h>
#include "modest-easysetup-wizard.h"

/* Copied from modest-main.c: */
typedef enum {
	MODEST_ERR_NONE    = 0,   /* no error */
	MODEST_ERR_OPTIONS = 1,   /* error in the options */
	MODEST_ERR_CONF    = 2,   /* error getting confuration db */
	MODEST_ERR_UI      = 3,   /* error in the UI */
	MODEST_ERR_HILDON  = 4,   /* error with Hildon (maemo-only) */
	MODEST_ERR_RUN     = 5,   /* error running */
	MODEST_ERR_PARAM   = 7,   /* error in one or more of the parameters */
	MODEST_ERR_INIT    = 8    /* error in initialization */
} ModestErrorCode;

static gboolean modest_easysetup_init(int argc, char *argv[])
{
        /* Setup gettext, to use our .po files: */
        /* GETTEXT_PACKAGE is defined in config.h */
        /* OSSO_MODEST_EASYSETUP_LOCALEDIR is defined in config.h */
	bindtextdomain (GETTEXT_PACKAGE, OSSO_MODEST_EASYSETUP_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (!gtk_init_check(&argc, &argv)) {
		g_printerr ("osso-modest-easysetup: failed to initialize GTK+\n");
		return FALSE;
	}

	return TRUE;
}

static gboolean modest_easysetup_uninit()
{
	return TRUE;
}

int
main (int argc, char *argv[])
{	
	if (!modest_easysetup_init (argc, argv)) {
		g_printerr ("osso-modest-easysetup: cannot init runtime\n");
		return MODEST_ERR_INIT;
	}

	ModestEasysetupWizardDialog *wizard = modest_easysetup_wizard_dialog_new ();
	gtk_dialog_run (GTK_DIALOG (wizard));

	if (!modest_easysetup_uninit ()) 
		g_printerr ("osso-modest-easysetup: shutdown failed\n");

	return MODEST_ERR_NONE;
}


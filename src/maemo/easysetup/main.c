/* Copyright (c) 2007, Nokia Corporation
 * All rights reserved.
 *
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
        /* OSSO_MODEST_EASYSETUP_LOCALEDIR is defined in the Makefile.am */
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


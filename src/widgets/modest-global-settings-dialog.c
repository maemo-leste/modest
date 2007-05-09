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
#include "modest-global-settings-dialog.h"
#include "modest-defs.h"
#include "modest-ui-constants.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_global_settings_dialog_class_init (ModestGlobalSettingsDialogClass *klass);
static void modest_global_settings_dialog_init       (ModestGlobalSettingsDialog *obj);
static void modest_global_settings_dialog_finalize   (GObject *obj);
/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

static GtkWidget* create_updating_page  (void);
static GtkWidget* create_composing_page (void);

typedef struct _ModestGlobalSettingsDialogPrivate ModestGlobalSettingsDialogPrivate;
struct _ModestGlobalSettingsDialogPrivate {
	GtkWidget *notebook;
	GtkWidget *updating_page;
	GtkWidget *composing_page;
	gboolean   modified;
};
#define MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                           MODEST_TYPE_GLOBAL_SETTINGS_DIALOG, \
                                                           ModestGlobalSettingsDialogPrivate))
/* globals */
static GtkDialogClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_global_settings_dialog_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestGlobalSettingsDialogClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_global_settings_dialog_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestGlobalSettingsDialog),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_global_settings_dialog_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_DIALOG,
		                                  "ModestGlobalSettingsDialog",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_global_settings_dialog_class_init (ModestGlobalSettingsDialogClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_global_settings_dialog_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestGlobalSettingsDialogPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}

static void
modest_global_settings_dialog_init (ModestGlobalSettingsDialog *self)
{
	ModestGlobalSettingsDialogPrivate *priv;

	priv = MODEST_GLOBAL_SETTINGS_DIALOG_GET_PRIVATE (self);

	priv->modified = FALSE;
	priv->notebook = gtk_notebook_new ();
	priv->updating_page = create_updating_page ();
	priv->composing_page = create_composing_page ();
    
	/* Add the notebook pages: */
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), priv->updating_page, 
		gtk_label_new (_("mcen_ti_options_updating")));
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), priv->composing_page, 
		gtk_label_new (_("mcen_ti_options_composing")));
		
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), priv->notebook);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (self)->vbox), MODEST_MARGIN_HALF);
	gtk_widget_show (priv->notebook);
        
	/* Add the buttons: */
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    
/* 	/\* Connect to the dialog's response signal: *\/ */
/* 	/\* We use connect-before  */
/* 	 * so we can stop the signal emission,  */
/* 	 * to stop the default signal handler from closing the dialog. */
/* 	 *\/ */
/* 	g_signal_connect (G_OBJECT (self), "response", */
/* 			  G_CALLBACK (on_response), self);  */

	/* Set title */
	gtk_window_set_title (GTK_WINDOW (self), _("mcen_ti_options"));
}

static void
modest_global_settings_dialog_finalize (GObject *obj)
{
/* 	free/unref instance resources here */
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget*
modest_global_settings_dialog_new (void)
{
	return GTK_WIDGET(g_object_new(MODEST_TYPE_GLOBAL_SETTINGS_DIALOG, NULL));
}

/* insert many other interesting function implementations */
/* such as modest_global_settings_dialog_do_something, or modest_global_settings_dialog_has_foo */

static GtkWidget*
create_updating_page (void)
{
	GtkWidget *box;

	box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	return box;
}

static GtkWidget* 
create_composing_page (void)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	return box;
}

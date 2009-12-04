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

/**
 * SECTION:modest-wizard-dialog
 * @short_description: A widget to create a guided installation
 * process wizard
 *
 * #ModestWizardDialog is a widget to create a guided installation
 * process. The dialog has four standard buttons, previous, next,
 * finish, cancel, and contains several pages with optional icons.
 * Response buttons are dimmed/undimmed automatically and the standard
 * icon is shown/hidden in response to page navigation. The notebook
 * widget provided by users contains the actual wizard pages.
 */

#include <config.h>
#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-defines.h>
#endif

#include "modest-wizard-dialog.h"
#include "modest-debug.h"
#include "modest-text-utils.h"

static GtkDialogClass *parent_class;

static void class_init              (ModestWizardDialogClass   *wizard_dialog_class);

static void init                    (ModestWizardDialog        *wizard_dialog);

static void create_title            (ModestWizardDialog        *wizard_dialog);

static void set_property            (GObject                   *object,
                                     guint                     property_id,
                                     const GValue              *value,
                                     GParamSpec                *pspec);

static void get_property            (GObject                   *object,
                                     guint                     property_id,
                                     GValue                    *value,
                                     GParamSpec                *pspec);

static void finalize                (GObject                   *object);

static void response                (ModestWizardDialog        *wizard, 
                                     gint                      response_id,
                                     gpointer                  unused);

static void make_buttons_sensitive  (ModestWizardDialog *wizard_dialog,
                                     gboolean           previous,
                                     gboolean           finish,
                                     gboolean next);

static gboolean invoke_before_next_vfunc (ModestWizardDialog *wizard_dialog);
static void invoke_enable_buttons_vfunc (ModestWizardDialog *wizard_dialog);
static void invoke_update_model_vfunc (ModestWizardDialog *wizard_dialog);
static gboolean invoke_save_vfunc (ModestWizardDialog *wizard_dialog);

enum {
    PROP_ZERO,
    PROP_WIZARD_NAME,
    PROP_WIZARD_NOTEBOOK,
    PROP_WIZARD_AUTOTITLE
};

struct _ModestWizardDialogPrivate {
    gchar       *wizard_name;
    GtkNotebook *notebook;
    GtkBox      *box;
    GtkWidget   *image;
    gboolean    autotitle;

    ModestWizardDialogResponseOverrideFunc override_func;
};


GType
modest_wizard_dialog_get_type (void)
{
    static GType wizard_dialog_type = 0;

    if (!wizard_dialog_type) {

        static const GTypeInfo wizard_dialog_info = {
            sizeof (ModestWizardDialogClass),
            NULL,       /* base_init      */
            NULL,       /* base_finalize  */
            (GClassInitFunc) class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data     */
            sizeof (ModestWizardDialog),
            0,          /* n_preallocs    */
            (GInstanceInitFunc) init,
        };

        wizard_dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                     "ModestWizardDialog",
                                                     &wizard_dialog_info,
                                                     0);
    }

    return wizard_dialog_type;
}

static void
class_init (ModestWizardDialogClass *wizard_dialog_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (wizard_dialog_class);

    parent_class = g_type_class_peek_parent (wizard_dialog_class);

    g_type_class_add_private (wizard_dialog_class,
                              sizeof(ModestWizardDialogPrivate));

    /* Override virtual methods */
    object_class->set_property = set_property;
    object_class->get_property = get_property;
    object_class->finalize     = finalize;

    wizard_dialog_class->before_next = NULL;
    wizard_dialog_class->update_model = NULL;
    wizard_dialog_class->save = NULL;
    wizard_dialog_class->enable_buttons = NULL;

    /**
     * ModestWizardDialog:wizard-name:
     *
     * The name of the wizard.
     */
    g_object_class_install_property (object_class, PROP_WIZARD_NAME,
            g_param_spec_string 
            ("wizard-name",
             "Wizard Name",
             "The name of the ModestWizardDialog",
             NULL,
             G_PARAM_READWRITE));

    /**
     * ModestWizardDialog:wizard-notebook:
     *
     * The notebook object, which is used by the ModestWizardDialog.
     */
    g_object_class_install_property(object_class, PROP_WIZARD_NOTEBOOK,
            g_param_spec_object 
            ("wizard-notebook",
             "Wizard Notebook",
             "GtkNotebook object to be used in the "
             "ModestWizardDialog",
             GTK_TYPE_NOTEBOOK, G_PARAM_READWRITE));

    /**
     * ModestWizardDialog:autotitle
     *
     * If the wizard should automatically try to change the window title when changing steps. 
     * Set to FALSE if you'd like to override the default behaviour. 
     *
     * Since: 0.14.5 
     */
    g_object_class_install_property(object_class, PROP_WIZARD_AUTOTITLE,
            g_param_spec_boolean 
            ("autotitle",
             "AutoTitle",
             "If the wizard should autotitle itself",
             TRUE, 
             G_PARAM_READWRITE));
}

static void 
finalize (GObject *object)
{
    ModestWizardDialog *dialog = MODEST_WIZARD_DIALOG (object);
    g_return_if_fail (dialog != NULL);

    if (dialog->priv->wizard_name != NULL)
        g_free (MODEST_WIZARD_DIALOG (object)->priv->wizard_name);
    
    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize(object);
}

/* Disable or enable the Previous, Next and Finish buttons */
static void
make_buttons_sensitive (ModestWizardDialog *wizard_dialog,
                        gboolean previous,
                        gboolean finish,
                        gboolean next)
{
    gtk_dialog_set_response_sensitive (GTK_DIALOG (wizard_dialog),
                                       MODEST_WIZARD_DIALOG_PREVIOUS,
                                       previous);

    gtk_dialog_set_response_sensitive (GTK_DIALOG (wizard_dialog),
                                       MODEST_WIZARD_DIALOG_FINISH,
                                       finish);

    gtk_dialog_set_response_sensitive (GTK_DIALOG (wizard_dialog),
                                       MODEST_WIZARD_DIALOG_NEXT,
                                       next);
}

static void 
init (ModestWizardDialog *wizard_dialog)
{
    /* Initialize private structure for faster member access */
    ModestWizardDialogPrivate *priv =
        G_TYPE_INSTANCE_GET_PRIVATE (wizard_dialog,
                MODEST_TYPE_WIZARD_DIALOG,
                ModestWizardDialogPrivate);

    GtkDialog *dialog = GTK_DIALOG (wizard_dialog);

    /* Init internal widgets */
    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_dialog_set_has_separator (dialog, FALSE);
    wizard_dialog->priv = priv;
    priv->override_func = NULL;
    priv->box = GTK_BOX (gtk_hbox_new (FALSE, 0));
#ifdef MODEST_TOOLKIT_HILDON2
    priv->image = NULL;
#else
#ifdef MODEST_TOOLKIT_GTK
    priv->image = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_DIALOG);
#else /*MODEST_TOOLKIT_GTK*/
    static int icon_size = 0;
    if (!icon_size)
	    icon_size = gtk_icon_size_register("modest_wizard", 50, 50);
    priv->image = gtk_image_new_from_icon_name ("qgn_widg_wizard",
						icon_size);
#endif /*!MODEST_TOOLKIT_GTK*/
#endif /*MODEST_TOOLKIT_HILDON2 */
    /* Default values for user provided properties */
    priv->notebook = NULL;
    priv->wizard_name = NULL;
    priv->autotitle = TRUE;

    /* Build wizard layout */
    gtk_box_pack_start (GTK_BOX (dialog->vbox), GTK_WIDGET (priv->box), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (priv->box), GTK_WIDGET (vbox), FALSE, FALSE, 0);
    gtk_widget_show (vbox);
    gtk_widget_show (GTK_WIDGET (priv->box));
    if (priv->image) {
	    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (priv->image), TRUE, TRUE, 0);
	    gtk_widget_show (priv->image);
    }

    /* Add response buttons: finish, previous, next, cancel */
#ifdef MODEST_TOOLKIT_HILDON1
    gtk_dialog_add_button (dialog, _HL("ecdg_bd_wizard_finish"), MODEST_WIZARD_DIALOG_FINISH);
    gtk_dialog_add_button (dialog, _HL("ecdg_bd_wizard_previous"), MODEST_WIZARD_DIALOG_PREVIOUS);
    gtk_dialog_add_button (dialog, _HL("ecdg_bd_wizard_next"), MODEST_WIZARD_DIALOG_NEXT);
    gtk_dialog_add_button (dialog, _HL("ecdg_bd_wizard_cancel"), MODEST_WIZARD_DIALOG_CANCEL);
#endif
#ifdef MODEST_TOOLKIT_HILDON2
    gtk_dialog_add_button (dialog, _HL("wdgt_bd_finish"), MODEST_WIZARD_DIALOG_FINISH);
    gtk_dialog_add_button (dialog, _HL("wdgt_bd_previous"), MODEST_WIZARD_DIALOG_PREVIOUS);
    gtk_dialog_add_button (dialog, _HL("wdgt_bd_next"), MODEST_WIZARD_DIALOG_NEXT);
#endif
#ifdef MODEST_TOOLKIT_GTK
    gtk_dialog_add_button (dialog, GTK_STOCK_SAVE, MODEST_WIZARD_DIALOG_FINISH);
    gtk_dialog_add_button (dialog, GTK_STOCK_GO_BACK, MODEST_WIZARD_DIALOG_PREVIOUS);
    gtk_dialog_add_button (dialog, GTK_STOCK_GO_FORWARD, MODEST_WIZARD_DIALOG_NEXT);
    gtk_dialog_add_button (dialog, GTK_STOCK_CANCEL, MODEST_WIZARD_DIALOG_CANCEL);
#endif

    /* Set initial button states: previous and finish buttons are disabled */
    make_buttons_sensitive (wizard_dialog, FALSE, FALSE, TRUE);

    gtk_widget_show (GTK_WIDGET (dialog->vbox));

    /* connect to dialog's response signal */
    g_signal_connect (G_OBJECT (dialog), "response",
            G_CALLBACK (response), NULL);

}

#if GTK_CHECK_VERSION(2, 10, 0) /* These signals were added in GTK+ 2.10: */
static void on_notebook_page_added(GtkNotebook *notebook, 
				   GtkWidget   *child,
				   guint        page_num,
				   gpointer     user_data)
{
	ModestWizardDialog* dialog = NULL;

	g_return_if_fail (MODEST_IS_WIZARD_DIALOG(user_data));
	dialog = MODEST_WIZARD_DIALOG(user_data);

	/* The title should show the total number of pages: */
	create_title (dialog);
}

static void on_notebook_page_removed(GtkNotebook *notebook, 
				     GtkWidget   *child,
				     guint        page_num,
				     gpointer     user_data)
{
	ModestWizardDialog* dialog = NULL;

	g_return_if_fail (MODEST_IS_WIZARD_DIALOG(user_data));
	dialog = MODEST_WIZARD_DIALOG(user_data);

	/* The title should show the total number of pages: */
	create_title (dialog);
}
#endif /* GTK_CHECK_VERSION */

static void
on_notebook_switch_page (GtkNotebook *notebook,
			 GtkNotebookPage *page,
			 guint page_num,
			 ModestWizardDialog *self)
{
	g_return_if_fail (MODEST_IS_WIZARD_DIALOG(self));

	create_title (self);
}

static void
connect_to_notebook_signals(ModestWizardDialog* dialog)
{
#if GTK_CHECK_VERSION(2, 10, 0) /* These signals were added in GTK+ 2.10: */
	ModestWizardDialogPrivate *priv = MODEST_WIZARD_DIALOG(dialog)->priv;
	g_return_if_fail (priv->notebook);
	
	/* Connect to the notebook signals,
	 * so we can update the title when necessary: */
	g_signal_connect (G_OBJECT (priv->notebook), "page-added",
		      G_CALLBACK (on_notebook_page_added), dialog);
	g_signal_connect (G_OBJECT (priv->notebook), "page-removed",
		      G_CALLBACK (on_notebook_page_removed), dialog);
#endif /* GTK_CHECK_VERSION */
	g_signal_connect_after (G_OBJECT (priv->notebook), "switch-page",
				G_CALLBACK (on_notebook_switch_page), dialog);
}


static void
set_property (GObject      *object, 
              guint        property_id,
              const GValue *value, 
              GParamSpec   *pspec)
{
    ModestWizardDialogPrivate *priv = MODEST_WIZARD_DIALOG(object)->priv;

    switch (property_id) {

        case PROP_WIZARD_AUTOTITLE:

            priv->autotitle = g_value_get_boolean (value);

            if (priv->autotitle && 
                priv->wizard_name && 
                priv->notebook)
                create_title (MODEST_WIZARD_DIALOG (object));
            else if (priv->wizard_name)
                gtk_window_set_title (GTK_WINDOW (object), priv->wizard_name);
            
            break;

        case PROP_WIZARD_NAME: 

            /* Set new wizard name. This name will appear in titlebar */
            if (priv->wizard_name)
                g_free (priv->wizard_name);

            gchar *str = (gchar *) g_value_get_string (value);
            g_return_if_fail (str != NULL);

            priv->wizard_name = g_strdup (str);

            /* We need notebook in order to create title, since page information
               is used in title generation */
            
            if (priv->notebook && priv->autotitle)
                create_title (MODEST_WIZARD_DIALOG (object));
    
            break;

        case PROP_WIZARD_NOTEBOOK: {

            GtkNotebook *book = GTK_NOTEBOOK (g_value_get_object (value));
            g_return_if_fail (book != NULL);

            priv->notebook = book;

            /* Set the default properties for the notebook (disable tabs,
             * and remove borders) to make it look like a nice wizard widget */
            gtk_notebook_set_show_tabs (priv->notebook, FALSE);
            gtk_notebook_set_show_border (priv->notebook, FALSE);
            gtk_box_pack_start (GTK_BOX( priv->box), GTK_WIDGET (priv->notebook), TRUE, TRUE, 0);

            /* Show the notebook so that a gtk_widget_show on the dialog is
             * all that is required to display the dialog correctly */
            gtk_widget_show ( GTK_WIDGET (priv->notebook));

            /* Update dialog title to reflect current page stats etc */ 
            ModestWizardDialog *wizard_dialog = MODEST_WIZARD_DIALOG (object);      
            if (priv->wizard_name && priv->autotitle)
                create_title (wizard_dialog);
                
            connect_to_notebook_signals (wizard_dialog);
            
            }break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
get_property (GObject      *object,
              guint        property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
    ModestWizardDialogPrivate *priv = MODEST_WIZARD_DIALOG (object)->priv;

    switch (property_id) {

        case PROP_WIZARD_NAME:
            g_value_set_string (value, priv->wizard_name);
            break;

        case PROP_WIZARD_NOTEBOOK:
            g_value_set_object (value, priv->notebook);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

/*
 * Creates the title of the dialog taking into account the current 
 * page of the notebook.
 */
static void
create_title (ModestWizardDialog *wizard_dialog)
{
    gchar *str = NULL;
    ModestWizardDialogPrivate *priv = NULL;
    GtkNotebook *notebook = NULL;
    gint pages, current;
    const gchar *steps;

    g_return_if_fail (MODEST_IS_WIZARD_DIALOG(wizard_dialog));
    g_return_if_fail (wizard_dialog->priv != NULL);

    priv = wizard_dialog->priv;
    notebook = priv->notebook;

    if (!notebook)
        return;

    /* Get page information, we'll need that when creating title */
    pages = gtk_notebook_get_n_pages (notebook);
    if (pages == 0)
	    return;

    current = gtk_notebook_get_current_page (priv->notebook);
    if (current < 0)
	    current = 0;

    steps = gtk_notebook_get_tab_label_text (notebook,
					     gtk_notebook_get_nth_page (notebook, current));

    str = g_strdup_printf ((steps&&*steps)?_HL("%s%s %s"):_HL("%s"),
			   priv->wizard_name, _HL("ecdg_ti_caption_separator"),
			   steps);

    /* Update the dialog to display the generated title */
    gtk_window_set_title (GTK_WINDOW (wizard_dialog), str);
    g_free (str);
}

/*
 * Response signal handler. This function is needed because GtkDialog's 
 * handler for this signal closes the dialog and we don't want that, we 
 * want to change pages and, dim certain response buttons. Overriding the 
 * virtual function would not work because that would be called after the 
 * signal handler implemented by GtkDialog.
 * FIXME: There is a much saner way to do that [MDK]
 */
static void 
response (ModestWizardDialog   *wizard_dialog,
          gint                 response_id,
          gpointer             unused)
{
    ModestWizardDialogPrivate *priv = wizard_dialog->priv;
    GtkNotebook *notebook = priv->notebook;
    gint current = 0;
    gboolean is_first, is_last;

    if (priv->override_func) {
	    if (priv->override_func (wizard_dialog, response_id, gtk_notebook_get_current_page (notebook))) {
		    /* Don't let the dialog close */
		    g_signal_stop_emission_by_name (wizard_dialog, "response");
		    
		    /* Force refresh of title */
		    if (priv->autotitle) 
			    create_title (wizard_dialog);
		    return;
	    }
    }
    
    switch (response_id) {
        
        case MODEST_WIZARD_DIALOG_PREVIOUS:
            gtk_notebook_prev_page (notebook); /* go to previous page */
            break;

        case MODEST_WIZARD_DIALOG_NEXT:
        	if (invoke_before_next_vfunc (wizard_dialog))
			gtk_notebook_next_page (notebook); /* go to next page */
            	
            break;

        case MODEST_WIZARD_DIALOG_CANCEL:
        	return;
        	break;      
        case MODEST_WIZARD_DIALOG_FINISH:
        	if (invoke_before_next_vfunc (wizard_dialog))
            	return;
            
            break;

    }

    current = gtk_notebook_get_current_page (notebook);
    gint last = gtk_notebook_get_n_pages (notebook) - 1;
    is_last = current == last;
    is_first = current == 0;

    /* If first page, previous and finish are disabled, 
       if last page, next is disabled */
    make_buttons_sensitive (wizard_dialog,
			    (is_first) ? FALSE : TRUE,
			    TRUE,
			    (is_last) ? FALSE : TRUE);

    /* Allow derived classes to disable buttons to prevent navigation,
     * according to their own validation logic: */
    invoke_enable_buttons_vfunc (wizard_dialog);

    /* Don't let the dialog close */
    g_signal_stop_emission_by_name (wizard_dialog, "response");

    /* We show the default image on first and last pages */
    last = gtk_notebook_get_n_pages (notebook) - 1;
    if (priv->image) {
	    if (current == last || current == 0)
		    gtk_widget_show (GTK_WIDGET(priv->image));
	    else
		    gtk_widget_hide (GTK_WIDGET(priv->image));
    }

    /* New page number may appear in the title, update it */
    if (priv->autotitle) 
        create_title (wizard_dialog);
}

/**
 * modest_wizard_dialog_new:
 * @parent: a #GtkWindow
 * @wizard_name: the name of dialog
 * @notebook: the notebook to be shown on the dialog
 *
 * Creates a new #ModestWizardDialog.
 *
 * Returns: a new #ModestWizardDialog
 */
GtkWidget*
modest_wizard_dialog_new (GtkWindow   *parent,
                          const char  *wizard_name,
                          GtkNotebook *notebook)
{
    GtkWidget *widget;

    g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), NULL);

    widget = GTK_WIDGET (g_object_new
            (MODEST_TYPE_WIZARD_DIALOG,
             "wizard-name", wizard_name,
             "wizard-notebook", notebook, NULL));

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW (widget), parent);

    return widget;
}

/**
 * modest_wizard_dialog_force_title_update:
 * @wizard_dialog: The wizard dialog
 *
 * Force the title to be rebuilt, for instance when you have added or 
 * removed notebook pages. This function is not necessary when using GTK+ 2.10, 
 * because that has GtkNotebook signals that will be used to update the title 
 * automatically.
 */
void
modest_wizard_dialog_force_title_update (ModestWizardDialog   *wizard_dialog)
{
	create_title (wizard_dialog);
}

static gboolean
invoke_before_next_vfunc (ModestWizardDialog *wizard_dialog)
{
	ModestWizardDialogClass *klass = MODEST_WIZARD_DIALOG_GET_CLASS (wizard_dialog);
	
	/* Call the vfunc, which may be overridden by derived classes: */
	if (klass->before_next) {
		ModestWizardDialogPrivate *priv = MODEST_WIZARD_DIALOG(wizard_dialog)->priv;
	
		gint current_page_num = gtk_notebook_get_current_page (priv->notebook);
		
		/* Get widgets for the two pages: */
		GtkWidget* current_page_widget = gtk_notebook_get_nth_page (priv->notebook, current_page_num);
		
		GtkWidget* next_page_widget = NULL;
		if ((current_page_num + 1) < gtk_notebook_get_n_pages (priv->notebook))
			next_page_widget = gtk_notebook_get_nth_page (priv->notebook, current_page_num + 1);

		MODEST_DEBUG_BLOCK (
		g_debug ("Switching to page %d (%s)",
			 gtk_notebook_page_num (priv->notebook, next_page_widget),
			 gtk_notebook_get_tab_label_text (priv->notebook, next_page_widget));

		{
			GtkWidget *p;
			gint i;
			g_debug ("\t***************");
			for (i=0; i<gtk_notebook_get_n_pages(priv->notebook);i++) {
				p = gtk_notebook_get_nth_page (priv->notebook, i);
				g_debug ("\t%d - %s", i, gtk_notebook_get_tab_label_text (priv->notebook, p));
			}
			g_debug ("\t***************");
		}
				    );
		
		/* Ask the vfunc implementation whether navigation should be allowed: */
		return (*(klass->before_next))(wizard_dialog, current_page_widget, next_page_widget);
	}
	
	/* Allow navigation by default if there is no vfunc implementation: */
	return TRUE;
}

static void
invoke_enable_buttons_vfunc (ModestWizardDialog *wizard_dialog)
{
	ModestWizardDialogClass *klass = MODEST_WIZARD_DIALOG_GET_CLASS (wizard_dialog);
	
	/* Call the vfunc, which may be overridden by derived classes: */
	if (klass->enable_buttons) {
		ModestWizardDialogPrivate *priv = MODEST_WIZARD_DIALOG(wizard_dialog)->priv;
	
		gint current_page_num = gtk_notebook_get_current_page (priv->notebook);
		
		GtkWidget* current_page_widget = gtk_notebook_get_nth_page (priv->notebook, current_page_num);
			
		(*(klass->enable_buttons))(wizard_dialog, current_page_widget);
	}
}

static void
invoke_update_model_vfunc (ModestWizardDialog *wizard_dialog)
{
	ModestWizardDialogClass *klass = MODEST_WIZARD_DIALOG_GET_CLASS (wizard_dialog);
	
	/* Call the vfunc, which may be overridden by derived classes: */
	if (klass->update_model) {
		(*(klass->update_model)) (wizard_dialog);
	}
}

static gboolean
invoke_save_vfunc (ModestWizardDialog *wizard_dialog)
{
	ModestWizardDialogClass *klass = MODEST_WIZARD_DIALOG_GET_CLASS (wizard_dialog);
	
	/* Call the vfunc, which may be overridden by derived classes: */
	if (klass->save) {
		return (*(klass->save)) (wizard_dialog);
	} else {
		return TRUE;
	}
}

void 
modest_wizard_dialog_set_response_override_handler (ModestWizardDialog *wizard_dialog,
						    ModestWizardDialogResponseOverrideFunc callback)
{
    ModestWizardDialogPrivate *priv = wizard_dialog->priv;

    priv->override_func = callback;
}

void
modest_wizard_dialog_update_model (ModestWizardDialog *wizard_dialog)
{
	g_return_if_fail (MODEST_IS_WIZARD_DIALOG (wizard_dialog));

	invoke_update_model_vfunc (wizard_dialog);
}

gboolean
modest_wizard_dialog_save (ModestWizardDialog *wizard_dialog)
{
	g_return_val_if_fail (MODEST_IS_WIZARD_DIALOG (wizard_dialog), FALSE);

	return invoke_save_vfunc (wizard_dialog);
}

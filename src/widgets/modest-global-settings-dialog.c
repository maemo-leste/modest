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
#include "widgets/modest-combo-box.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_global_settings_dialog_class_init (ModestGlobalSettingsDialogClass *klass);
static void modest_global_settings_dialog_init       (ModestGlobalSettingsDialog *obj);
static void modest_global_settings_dialog_finalize   (GObject *obj);

enum {
	MODEST_CONNECTED_VIA_WLAN,
	MODEST_CONNECTED_VIA_ANY
};

enum {
	MODEST_UPDATE_INTERVAL_5_MIN,
	MODEST_UPDATE_INTERVAL_10_MIN,
	MODEST_UPDATE_INTERVAL_15_MIN,
	MODEST_UPDATE_INTERVAL_30_MIN,
	MODEST_UPDATE_INTERVAL_1_HOUR,
	MODEST_UPDATE_INTERVAL_2_HOUR
};

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
/* 	GdkGeometry *geometry; */

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
	gtk_widget_show_all (priv->notebook);
        
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

	/* Set geometry */
/* 	geometry = g_malloc0(sizeof (GdkGeometry)); */
/* 	geometry->max_width = MODEST_DIALOG_WINDOW_MAX_WIDTH; */
/* 	geometry->min_width = MODEST_DIALOG_WINDOW_MIN_WIDTH; */
/* 	geometry->max_height = MODEST_DIALOG_WINDOW_MAX_HEIGHT; */
/* 	geometry->min_height = MODEST_DIALOG_WINDOW_MIN_HEIGHT; */
/* 	gtk_window_set_geometry_hints (GTK_WINDOW (self), */
/* 				       GTK_WIDGET (self), */
/* 				       geometry, */
/* 				       GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE); */
	gtk_widget_set_size_request (GTK_WIDGET (self), 
				     MODEST_DIALOG_WINDOW_MAX_WIDTH, 
				     MODEST_DIALOG_WINDOW_MAX_HEIGHT);
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

static void
add_to_table (GtkTable *table,
	      GtkWidget *left,
	      GtkWidget *right)
{
	guint n_rows = 0;

	g_object_get (G_OBJECT (table), "n-rows", &n_rows,NULL);

	/* Create label */
	gtk_misc_set_alignment (GTK_MISC (left), 1.0, 0.0);

	/* Create value */
/* 	gtk_misc_set_alignment (GTK_MISC (right), 0.0, 0.0); */

	/* Attach label and value */
	gtk_table_attach (table, 
			  left, 0, 1, 
			  n_rows, n_rows + 1, 
			  GTK_SHRINK|GTK_FILL, 
			  GTK_SHRINK|GTK_FILL, 
			  0, 0);
	gtk_table_attach (table, 
			  right, 1, 2, 
			  n_rows, n_rows + 1, 
			  GTK_EXPAND|GTK_FILL, 
			  GTK_SHRINK|GTK_FILL, 
			  0, 0);
}


static void
add_to_modest_pair_list (const gint num, const gchar *str, GSList **list)
{
	guint *number;
	ModestPair *pair;

	number = g_malloc0 (sizeof (guint));
	*number = num;
	pair = modest_pair_new (number, g_strdup (str), TRUE);
	*list = g_slist_prepend (*list, pair);
}

static ModestPairList *
get_connected_via (void)
{
	GSList *list = NULL;

	add_to_modest_pair_list (MODEST_CONNECTED_VIA_WLAN, 
				 _("mcen_va_options_connectiontype_wlan"), 
				 &list);
	add_to_modest_pair_list (MODEST_CONNECTED_VIA_ANY, 
				 _("mcen_va_options_connectiontype_all"), 
				 &list);

	return (ModestPairList *) g_slist_reverse (list);
}

static ModestPairList *
get_update_interval (void)
{
	GSList *list = NULL;

	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_5_MIN, 
				 _("mcen_va_options_updateinterval_5min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_10_MIN, 
				 _("mcen_va_options_updateinterval_10min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_15_MIN, 
				 _("mcen_va_options_updateinterval_15min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_30_MIN, 
				 _("mcen_va_options_updateinterval_30min"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_1_HOUR, 
				 _("mcen_va_options_updateinterval_1h"), 
				 &list);
	add_to_modest_pair_list (MODEST_UPDATE_INTERVAL_2_HOUR, 
				 _("mcen_va_options_updateinterval_2h"), 
				 &list);

	return (ModestPairList *) g_slist_reverse (list);
}

/* We need this because the translations are comming without ":" */
static GtkWidget *
create_label (const gchar *text)
{
	gchar *label_name;
	GtkWidget *label;

	label_name = g_strdup_printf ("%s:", text);
	label = gtk_label_new (label_name);
	g_free (label_name);

	return label;
}

static GtkWidget*
create_updating_page (void)
{
	GtkWidget *vbox, *table_update;
	GtkWidget *label, *check, *combo;
	ModestPairList *list;

	vbox = gtk_vbox_new (FALSE, MODEST_MARGIN_DEFAULT);
	table_update = gtk_table_new (3, 2, FALSE);
	/* FIXME: set proper values */
	gtk_table_set_row_spacings (GTK_TABLE (table_update), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table_update), 12);

	/* Autoupdate */
	label = create_label (_("mcen_fi_options_autoupdate"));
	check = gtk_check_button_new ();
	add_to_table (GTK_TABLE (table_update), label, check);

	/* Connected via */
	label = create_label (_("mcen_fi_options_connectiontype"));
	list = get_connected_via ();
	combo = modest_combo_box_new (list, g_int_equal);
	modest_pair_list_free (list);
	add_to_table (GTK_TABLE (table_update), label, combo);

	/* Update interval */
	label = create_label (_("mcen_fi_options_updateinterval"));
	list = get_update_interval ();
	combo = modest_combo_box_new (list, g_int_equal);
	modest_pair_list_free (list);
	add_to_table (GTK_TABLE (table_update), label, combo);

	/* Add to vbox */
	gtk_box_pack_start (GTK_BOX (vbox), table_update, FALSE, FALSE, MODEST_MARGIN_HALF);

	/* Separator */
	gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, MODEST_MARGIN_HALF);

	return vbox;
}

static GtkWidget* 
create_composing_page (void)
{
	GtkWidget *box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	box = gtk_vbox_new (FALSE, MODEST_MARGIN_NONE);

	return box;
}

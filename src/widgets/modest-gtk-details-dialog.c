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

#include "modest-details-dialog.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktable.h>
#include <gtk/gtkstock.h>
#include <gtk/gtklabel.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-header-view.h>
#include <tny-folder-store.h>
#include <modest-tny-folder.h>
#include <modest-tny-account.h>
#include <modest-text-utils.h>
#include "modest-gtk-details-dialog.h"
#include <modest-scrollable.h>
#include <modest-ui-constants.h>
#include <modest-runtime.h>

static void modest_gtk_details_dialog_create_container_default (ModestDetailsDialog *self);


G_DEFINE_TYPE (ModestGtkDetailsDialog, 
	       modest_gtk_details_dialog, 
	       MODEST_TYPE_DETAILS_DIALOG);

#define MODEST_GTK_DETAILS_DIALOG_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_DETAILS_DIALOG, ModestGtkDetailsDialogPrivate))


typedef struct _ModestGtkDetailsDialogPrivate ModestGtkDetailsDialogPrivate;

struct _ModestGtkDetailsDialogPrivate
{
	GtkWidget *props_table;
};

static void
modest_gtk_details_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (modest_gtk_details_dialog_parent_class)->finalize (object);
}

static void
modest_gtk_details_dialog_class_init (ModestGtkDetailsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (ModestGtkDetailsDialogPrivate));
	object_class->finalize = modest_gtk_details_dialog_finalize;

	MODEST_DETAILS_DIALOG_CLASS (klass)->create_container_func = 
		modest_gtk_details_dialog_create_container_default;
}

static void
modest_gtk_details_dialog_init (ModestGtkDetailsDialog *self)
{
}

GtkWidget*
modest_gtk_details_dialog_new_with_header (GtkWindow *parent, 
					       TnyHeader *header,
					       gboolean get_size)
{
	ModestDetailsDialog *dialog;

	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);
	g_return_val_if_fail (TNY_IS_HEADER (header), NULL);

	dialog = (ModestDetailsDialog *) (g_object_new (MODEST_TYPE_GTK_DETAILS_DIALOG, 
							"transient-for", parent, 
							NULL));

	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->create_container_func (dialog);
	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->set_header_func (dialog, header, get_size);

	return GTK_WIDGET (dialog);
}

GtkWidget* 
modest_gtk_details_dialog_new_with_folder  (GtkWindow *parent, 
						TnyFolder *folder)
{
	ModestDetailsDialog *dialog;

	g_return_val_if_fail (GTK_IS_WINDOW (parent), NULL);
	g_return_val_if_fail (TNY_IS_FOLDER (folder), NULL);

	dialog = (ModestDetailsDialog *) (g_object_new (MODEST_TYPE_GTK_DETAILS_DIALOG, 
							"transient-for", parent, 
							NULL));

	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->create_container_func (dialog);
	MODEST_DETAILS_DIALOG_GET_CLASS (dialog)->set_folder_func (dialog, folder);

	return GTK_WIDGET (dialog);
}


static void
modest_gtk_details_dialog_create_container_default (ModestDetailsDialog *self)
{
	ModestGtkDetailsDialogPrivate *priv;
	GtkWidget *scrollable;
	GtkWidget *align;

	priv = MODEST_GTK_DETAILS_DIALOG_GET_PRIVATE (self);

	gtk_window_set_default_size (GTK_WINDOW (self), -1, MODEST_DIALOG_WINDOW_MAX_HEIGHT);

	priv->props_table = gtk_table_new (0, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (priv->props_table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (priv->props_table), 1);

	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DEFAULT);

	scrollable = modest_toolkit_factory_create_scrollable (modest_runtime_get_toolkit_factory ());
	modest_scrollable_set_vertical_policy (MODEST_SCROLLABLE (scrollable), GTK_POLICY_AUTOMATIC);
	modest_scrollable_set_horizontal_policy (MODEST_SCROLLABLE (scrollable), GTK_POLICY_NEVER);

	gtk_container_add (GTK_CONTAINER (align), priv->props_table);
	modest_scrollable_add_with_viewport (MODEST_SCROLLABLE (scrollable), 
					     GTK_WIDGET (align));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self)->vbox), scrollable);

	gtk_dialog_set_has_separator (GTK_DIALOG (self), FALSE);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
}

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

#include <glib.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <string.h> /* for strlen */
#include <modest-runtime.h>

#include <modest-defs.h>
#include <modest-text-utils.h>
#include "modest-toolkit-utils.h"
#include "modest-ui-constants.h"
#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon.h>
#endif
#ifdef MODEST_TOOLKIT_GTK
#include <modest-gtk-window-mgr.h>
#endif

/* Label child of a captioned */
#define CAPTIONED_LABEL_CHILD "captioned-label"


GtkWidget *
modest_toolkit_utils_get_manager_menubar_as_menu (GtkUIManager *manager,
						  const gchar *item_name)
{
	GtkWidget *new_menu;
	GtkWidget *menubar;
	GList *children, *iter;

	menubar = gtk_ui_manager_get_widget (manager, item_name);
	new_menu = gtk_menu_new ();

	children = gtk_container_get_children (GTK_CONTAINER (menubar));
	for (iter = children; iter != NULL; iter = g_list_next (iter)) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent (menu, new_menu);
	}
	
	g_list_free (children);

	return new_menu;
}

/**
 * modest_toolkit_utils_create_captioned:
 * @title_size_group: a #GtkSizeGroup
 * @value_size_group: a #GtkSizeGroup
 * @title: a string
 * @control: a #GtkWidget
 *
 * this creates a widget (a #GtkHBox) with a control, and a label
 * (@string) captioning it. It also uses the proper size groups for title
 * and control.
 *
 * Returns: a widget containing the control and a proper label.
 */
GtkWidget *
modest_toolkit_utils_create_captioned    (GtkSizeGroup *title_size_group,
					  GtkSizeGroup *value_size_group,
					  const gchar *title,
					  gboolean use_markup,
					  GtkWidget *control)
{
	return modest_toolkit_utils_create_captioned_with_size_type (title_size_group,
								     value_size_group,
								     title,
								     use_markup,
								     control,
								     0);
}

GtkWidget *
modest_toolkit_utils_create_vcaptioned    (GtkSizeGroup *size_group,
					   const gchar *title,
					   gboolean use_markup,
					   GtkWidget *control)
{
	return modest_toolkit_utils_create_vcaptioned_with_size_type (size_group,
								      title,
								      use_markup,
								      control,
								      0);
}

/**
 * modest_toolkit_utils_create_captioned_with_size_type:
 * @title_size_group: a #GtkSizeGroup
 * @value_size_group: a #GtkSizeGroup
 * @title: a string
 * @control: a #GtkWidget
 * @size_type: a #HildonSizeType
 *
 * this creates a widget (a #GtkHBox) with a control, and a label
 * (@string) captioning it. It also uses the proper size groups for title
 * and control.
 *
 * Returns: a widget containing the control and a proper label.
 */
GtkWidget *
modest_toolkit_utils_create_captioned_with_size_type    (GtkSizeGroup *title_size_group,
							 GtkSizeGroup *value_size_group,
							 const gchar *title,
							 gboolean use_markup,
							 GtkWidget *control,
							 ModestToolkitSizeType size_type)
{
 	GtkWidget *label;
	GtkWidget *align;
	GtkWidget *box;
  
	if (use_markup) {
		label = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (label), title);
	} else {
		label = gtk_label_new (title);
	}
	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (label, HILDON_SIZE_FINGER_HEIGHT);
#endif
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);
	gtk_widget_show (align);
	box = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (align), label);
	gtk_box_pack_start (GTK_BOX (box), align, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
	if (title_size_group)
		gtk_size_group_add_widget (title_size_group, label);
	if (value_size_group)
		gtk_size_group_add_widget (value_size_group, control);

#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (control, size_type);
#endif

	g_object_set_data (G_OBJECT (box), CAPTIONED_LABEL_CHILD, label);

	return box;
}

GtkWidget *
modest_toolkit_utils_create_vcaptioned_with_size_type    (GtkSizeGroup *size_group,
							  const gchar *title,
							  gboolean use_markup,
							  GtkWidget *control,
							  ModestToolkitSizeType size_type)
{
 	GtkWidget *label;
	GtkWidget *align;
	GtkWidget *box;
  
	if (use_markup) {
		label = gtk_label_new (NULL);
		gtk_label_set_markup (GTK_LABEL (label), title);
	} else {
		label = gtk_label_new (title);
	}
	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, MODEST_MARGIN_DOUBLE, MODEST_MARGIN_DOUBLE);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (label, HILDON_SIZE_FINGER_HEIGHT);
#endif
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);
	gtk_widget_show (align);
	box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (align), label);
	gtk_box_pack_start (GTK_BOX (box), align, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), control, TRUE, TRUE, 0);
	if (size_group) {
		gtk_size_group_add_widget (size_group, label);
		gtk_size_group_add_widget (size_group, control);
	}

#ifdef MAEMO_CHANGES
	hildon_gtk_widget_set_theme_size (control, size_type);
#endif

	g_object_set_data (G_OBJECT (box), CAPTIONED_LABEL_CHILD, label);

	return box;
}

/**
 * modest_toolkit_utils_captioned_set_label:
 * @captioned: a #GtkWidget built as captioned
 * @new_label: a string
 * @use_markup: a #gboolean
 *
 * set a new label for the captioned
 */
void
modest_toolkit_utils_captioned_set_label (GtkWidget *captioned,
					  const gchar *new_label,
					  gboolean use_markup)
{
	GtkWidget *label;

	g_return_if_fail (GTK_IS_WIDGET (captioned));

	label = g_object_get_data (G_OBJECT (captioned), CAPTIONED_LABEL_CHILD);
	g_return_if_fail (GTK_IS_LABEL (label));

	if (use_markup) {
		gtk_label_set_markup (GTK_LABEL (label), new_label);
	} else {
		gtk_label_set_text (GTK_LABEL (label), new_label);
	}
}

/**
 * modest_toolkit_utils_captioned_get_label_widget:
 * @captioned: a #GtkWidget built as captioned
 *
 * obtains the label widget for the captioned
 *
 * Returns: a #GtkLabel
 */
GtkWidget *
modest_toolkit_utils_captioned_get_label_widget (GtkWidget *captioned)
{
	GtkWidget *label;

	g_return_val_if_fail (GTK_IS_WIDGET (captioned), NULL);

	label = g_object_get_data (G_OBJECT (captioned), CAPTIONED_LABEL_CHILD);
	g_return_val_if_fail (GTK_IS_LABEL (label), NULL);

	return label;

}

/**
 * modest_toolkit_utils_set_hbutton_layout:
 * @title_sizegroup: a #GtkSizeGroup, or %NULL
 * @value_sizegroup: a #GtkSizeGroup, or %NULL
 * @title: a string
 * @button: a #HildonButton
 *
 * Configures the alignment and layout of @button. If @title_sizegroup is provided,
 * the title will be aligned to the left using it. If @value_sizegroup is provided,
 * the value will be aligned to the left using it. It also sets the title
 * of the button.
 *
 * The alignment is left for the title and for the value.
 */
void
modest_toolkit_utils_set_hbutton_layout (GtkSizeGroup *title_sizegroup, 
				       GtkSizeGroup *value_sizegroup,
				       const gchar *title, 
				       GtkWidget *button)
{
#ifdef MODEST_TOOLKIT_HILDON2
	hildon_button_set_title (HILDON_BUTTON (button), title);
	if (title_sizegroup)
		hildon_button_add_title_size_group (HILDON_BUTTON (button), title_sizegroup);
	if (value_sizegroup)
		hildon_button_add_value_size_group (HILDON_BUTTON (button), value_sizegroup);
	hildon_button_set_alignment (HILDON_BUTTON (button), 0.0, 0.5, 1.0, 0.0);
	hildon_button_set_title_alignment (HILDON_BUTTON (button), 0.0, 0.5);
	hildon_button_set_value_alignment (HILDON_BUTTON (button), 0.0, 0.5);
#endif
}

void
modest_toolkit_utils_set_vbutton_layout (GtkSizeGroup *sizegroup, 
					 const gchar *title, 
					 GtkWidget *button)
{
#ifdef MODEST_TOOLKIT_HILDON2
	hildon_button_set_title (HILDON_BUTTON (button), title);
	if (sizegroup) {
		hildon_button_add_title_size_group (HILDON_BUTTON (button), sizegroup);
		hildon_button_add_value_size_group (HILDON_BUTTON (button), sizegroup);
	}
	hildon_button_set_alignment (HILDON_BUTTON (button), 0.0, 0.5, 1.0, 0.0);
	hildon_button_set_title_alignment (HILDON_BUTTON (button), 0.0, 0.5);
	hildon_button_set_value_alignment (HILDON_BUTTON (button), 0.0, 0.5);
#endif
}

GtkWidget *
modest_toolkit_utils_create_group_box (const gchar *label_text, GtkWidget *contents)
{
	GtkWidget *label;
	GtkWidget *box;

	label = gtk_label_new (label_text);
	gtk_widget_show (label);

	box = gtk_vbox_new (FALSE, MODEST_MARGIN_HALF);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), contents, TRUE, TRUE, 0);
	gtk_widget_show (box);

	return box;
}

#ifdef MODEST_TOOLKIT_HILDON2
static gboolean match_all (TnyList *list, GObject *item, gpointer match_data)
{
	return TRUE;
}
#endif

gboolean
modest_toolkit_utils_select_attachments (GtkWindow *window, TnyList *att_list, gboolean include_msgs)
{
#ifdef MODEST_TOOLKIT_HILDON2
	GtkTreeModel *model;
	TnyIterator *iterator;
	GtkWidget *selector;
	GtkCellRenderer *renderer;
	GtkWidget *dialog;
	gint response;
	gboolean result = TRUE;
	gint attachments_added = 0;

	model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_OBJECT));
	for (iterator = tny_list_create_iterator (att_list);
	     !tny_iterator_is_done (iterator);
	     tny_iterator_next (iterator)) {
		GtkTreeIter iter;
		TnyMimePart *part;
		gchar *filename = NULL;

		part = (TnyMimePart *) tny_iterator_get_current (iterator);

		/* Ignore purged attachments and messages if ignore is
		   set to TRUE */
		if (!(tny_mime_part_is_purged (part) ||
		      (TNY_IS_MSG (part) && !include_msgs))) {

			if (TNY_IS_MSG (part)) {
				TnyHeader *header = tny_msg_get_header (TNY_MSG (part));
				filename = tny_header_dup_subject (header);
				g_object_unref (header);
			} else {
				filename = g_strdup (tny_mime_part_get_filename (part));
			}
			if ((filename == NULL) || (filename[0] == '\0')) {
				g_free (filename);
				filename = g_strdup (_("mail_va_no_subject"));
			}
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, filename, 1, part, -1);
			attachments_added ++;
			g_free (filename);
		}
		g_object_unref (part);
	}
	g_object_unref (iterator);

	selector = GTK_WIDGET (hildon_touch_selector_new ());
	renderer = gtk_cell_renderer_text_new ();
	g_object_set((GObject *) renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	hildon_touch_selector_append_column ((HildonTouchSelector *) selector, model, renderer,
					     "text", 0, NULL);
	hildon_touch_selector_set_column_selection_mode ((HildonTouchSelector *) selector, 
							 HILDON_TOUCH_SELECTOR_SELECTION_MODE_MULTIPLE);

	dialog = hildon_picker_dialog_new (window);
	gtk_window_set_title (GTK_WINDOW (dialog), (attachments_added > 1)?
			      _("mcen_ti_select_attachments_title"):_("mcen_ti_select_attachment_title"));
	hildon_picker_dialog_set_selector (HILDON_PICKER_DIALOG (dialog), (HildonTouchSelector *) selector);
	hildon_touch_selector_unselect_all ((HildonTouchSelector *) selector, 0);
	hildon_picker_dialog_set_done_label (HILDON_PICKER_DIALOG (dialog), _HL("wdgt_bd_done"));

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_OK) {
		GList *selected_rows, *node;

		tny_list_remove_matches (att_list, match_all, NULL);
		selected_rows = hildon_touch_selector_get_selected_rows ((HildonTouchSelector *) selector, 0);
		for (node = selected_rows; node != NULL; node = g_list_next (node)) {
			GtkTreePath *path;
			GObject *selected;
			GtkTreeIter iter;

			path = (GtkTreePath *) node->data;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, 1, &selected, -1);
			tny_list_append (att_list, selected);
		}
		if (tny_list_get_length (att_list) == 0)
			result = FALSE;

		g_list_foreach (selected_rows, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (selected_rows);
	} else {
		result = FALSE;
	}

	gtk_widget_destroy (dialog);

	g_object_unref (model);

	return result;
#else
	return FALSE;
#endif
}

/* Copyright (c) 2007, Nokia Corporation
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

/* modest-address-book.c */

#include <config.h>
#include <glib/gi18n.h>
#include <modest-address-book.h>
#include <modest-text-utils.h>
#include <libebook/e-book.h>
#include <libebook/e-book-view.h>
#include <libosso-abook/osso-abook.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-banner.h>
#include <string.h>
#include <gtk/gtksizegroup.h>
#include <gtk/gtkbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkentry.h>

static OssoABookContactModel *contact_model =  NULL;
static EBook *book = NULL;
static EBookView * book_view = NULL;

static GSList *get_recipients_for_given_contact(EContact * contact);
static void commit_contact(EContact * contact);
static gchar *get_email_addr_from_user(const gchar * given_name);
static gchar *ui_get_formatted_email_id(gchar * current_given_name,
					gchar * current_sur_name, gchar * current_email_id);
static gchar *run_add_email_addr_to_contact_dlg(const gchar * contact_name);
static GSList *select_email_addrs_for_contact(GList * email_addr_list);


static void
get_book_view_cb (EBook *book, EBookStatus status, EBookView *bookview, gpointer data)
{
	if (status != E_BOOK_ERROR_OK) {
		g_object_unref (book);
		book = NULL;
		return;
	}
	book_view = bookview;

	if (contact_model)
		osso_abook_tree_model_set_book_view (OSSO_ABOOK_TREE_MODEL (contact_model),
						     book_view);

	e_book_view_start (book_view);
}

static void
book_open_cb (EBook *view, EBookStatus status, gpointer data)
{
	EBookQuery *query = NULL;

	if (status != E_BOOK_ERROR_OK) {
		g_object_unref (book);
		book = NULL;
		return;
	}
	query = e_book_query_any_field_contains ("");
	e_book_async_get_book_view (book, query, NULL, -1, get_book_view_cb, NULL);
	e_book_query_unref (query);
}

static gboolean 
open_addressbook ()
{
	book = e_book_new_system_addressbook (NULL);
	if (!book)
		return FALSE;

	e_book_async_open (book, FALSE, book_open_cb, NULL);

	return TRUE; /* FIXME */	
}

void
modest_address_book_add_address (const gchar *address)
{
	OssoABookAccount *account = NULL;
	GtkWidget *dialog = NULL;

	contact_model = osso_abook_contact_model_new ();
	if (!open_addressbook ()) {
		if (contact_model) {
			g_object_unref (contact_model);
			contact_model = NULL;
		}
		return;
	}
	
	account = osso_abook_account_get (EVC_EMAIL, NULL, address);
	dialog = osso_abook_add_to_contacts_dialog_new (contact_model, account);
	g_object_unref (account);
	gtk_dialog_run (GTK_DIALOG (dialog));

	if (contact_model) {
		g_object_unref (contact_model);
		contact_model = NULL;
	}

	gtk_widget_destroy (dialog);

}

void
modest_address_book_select_addresses (ModestRecptEditor *recpt_editor)
{
	GtkWidget *contact_view = NULL;
	GList *contacts_list = NULL;
	GtkWidget *contact_dialog;
	GSList *email_addrs_per_contact = NULL;
	gchar *econtact_id;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));

	contact_model = osso_abook_contact_model_new ();
	if (!open_addressbook ()) {
		if (contact_model) {
			g_object_unref (contact_model);
			contact_model = NULL;
		}
		return;
	}

	contact_view = osso_abook_contact_selector_new_basic (contact_model);
	osso_abook_contact_selector_set_minimum_selection (OSSO_ABOOK_CONTACT_SELECTOR (contact_view), 1);

	contact_dialog = osso_abook_select_dialog_new (OSSO_ABOOK_TREE_VIEW (contact_view));
	osso_abook_select_dialog_set_new_contact (OSSO_ABOOK_SELECT_DIALOG (contact_dialog), TRUE);

	gtk_widget_show (contact_dialog);

	if (gtk_dialog_run (GTK_DIALOG (contact_dialog)) == GTK_RESPONSE_OK) {
		contacts_list = 
			osso_abook_contact_selector_get_extended_selection (OSSO_ABOOK_CONTACT_SELECTOR
									     (contact_view));
	}
	
	if (contacts_list) {
		GList *node;

		for (node = contacts_list; node != NULL; node = g_list_next (node)) {
			EContact *contact = (EContact *) node->data;

			email_addrs_per_contact = get_recipients_for_given_contact (contact);
			if (email_addrs_per_contact) {
				econtact_id = (gchar *) e_contact_get_const (contact, E_CONTACT_UID);
				modest_recpt_editor_add_resolved_recipient (MODEST_RECPT_EDITOR (recpt_editor), 
									    email_addrs_per_contact, econtact_id);
				g_slist_foreach (email_addrs_per_contact, (GFunc) g_free, NULL);
				g_slist_free (email_addrs_per_contact);
				email_addrs_per_contact = NULL;
			}
		}
		g_list_free (contacts_list);
	}

	if (contact_view) {
		g_object_unref (contact_view);
		contact_view = NULL;
	}

	if (contact_model) {
		g_object_unref (contact_model);
		contact_model = NULL;
	}

	gtk_widget_destroy (contact_dialog);

}

/**
 * This function returns the resolved recipients for a given EContact.
 * If no e-mail address is defined, it launches 'Add e-mail address to contact'
 * dialog to obtain one. If multiple e-mail addresses are found, it launches
 * 'Select e-mail address' dialog to allow user to select one or more e-mail
 * addresses for that contact.
 *
 * @param  Contact of type #EContact
 * @return List of resolved recipient strings, to be freed by calling function.
 */
static GSList *get_recipients_for_given_contact(EContact * contact)
{
	gchar *givenname = NULL;
	gchar *familyname = NULL;
	gchar *nickname = NULL;
	gchar *emailid = NULL;
	const gchar *display_name = NULL;
	GList *list = NULL;
	gchar *formatted_string = NULL;
	gboolean email_not_present = FALSE;
	GSList *formattedlist = NULL, *selected_email_addr_list = NULL, *node = NULL;

	if (!contact) {
		return NULL;
	}

	givenname = (gchar *) e_contact_get_const(contact, E_CONTACT_GIVEN_NAME);
	familyname = (gchar *) e_contact_get_const(contact, E_CONTACT_FAMILY_NAME);
	nickname = (gchar *) e_contact_get_const(contact, E_CONTACT_NICKNAME);
	if (!nickname)
		nickname = "";
	list = (GList *) e_contact_get(contact, E_CONTACT_EMAIL);

	if (!list) {
		email_not_present = TRUE;
	}

	if (list && g_list_length(list) == 1) {
		if (list->data == NULL || g_utf8_strlen(list->data, -1) == 0) {
			email_not_present = TRUE;
		} else {
			emailid = g_strstrip(g_strdup(list->data));
			if (g_utf8_strlen(emailid, -1) == 0) {
				g_free(emailid);
				email_not_present = TRUE;
			}
		}
	}

	/*Launch the 'Add e-mail addr to contact' dialog if required */
	if (email_not_present) {
		display_name = osso_abook_contact_get_display_name(contact);
		emailid = get_email_addr_from_user(display_name);

		if (emailid) {
			e_contact_set(contact, E_CONTACT_EMAIL_1, emailid);
			commit_contact(contact);
		}
	}

	if (emailid) {
		if (givenname || familyname)
			formatted_string =
			    ui_get_formatted_email_id(givenname, familyname, emailid);
		else
			formatted_string = g_strdup(emailid);
		formattedlist = g_slist_append(formattedlist, formatted_string);
		g_free(emailid);
	}

	/*Launch the 'Select e-mail address' dialog if required */
	if (g_list_length(list) > 1) {
		selected_email_addr_list = select_email_addrs_for_contact(list);
		for (node = selected_email_addr_list; node != NULL; node = node->next) {
			if (givenname || familyname)
				formatted_string =
				    ui_get_formatted_email_id(givenname, familyname, node->data);
			else
				formatted_string = g_strdup(node->data);
			formattedlist = g_slist_append(formattedlist, formatted_string);
		}
		if (selected_email_addr_list) {
			g_slist_foreach(selected_email_addr_list, (GFunc) g_free, NULL);
			g_slist_free(selected_email_addr_list);
		}
	}

	if (list) {
		g_list_foreach(list, (GFunc) g_free, NULL);
		g_list_free(list);
	}

	return formattedlist;
}

/**
 * This is a helper function to commit a EContact to Address_Book application.
 *
 * @param  contact  Contact of type #EContact
 * @return void
 */
static void 
commit_contact(EContact * contact)
{
	if (!contact || !book)
		return;

	osso_abook_contact_commit(contact, FALSE, book);
}

/**
 * This is a helper function used to launch 'Add e-mail address to contact' dialog
 * after showing appropriate notification, when there is no e-mail address defined
 * for a selected contact.
 *
 * @param  given_name  Given name of the contact
 * @param  family_name  Family name of the contact
 * @return E-mail address string entered by user, to be freed by calling function.
 */
static gchar *
get_email_addr_from_user(const gchar * given_name)
{
	gchar *notification = NULL;
	gchar *email_addr = NULL;
	GtkWidget *note;
	gboolean note_response;


	notification = g_strdup_printf(_("mcen_nc_email_address_not_defined"), given_name);

	note = hildon_note_new_confirmation (NULL, notification);
	note_response = gtk_dialog_run (GTK_DIALOG(note));
	gtk_widget_destroy (note);
	g_free(notification);

	if (note_response == GTK_RESPONSE_OK) {
		email_addr = run_add_email_addr_to_contact_dlg(given_name);
	}

	return email_addr;
}

/**
This function is used to get the formated email id with given name and sur name
in the format "GIVENNAME SURNAME <EMAIL ADDRESS>".
@param current_given_name    to hold the given name
@param current_sur_name      to hold the sur name
@param current_email_id      to hold the email id. 
@return gchar* string to be freed by calling function
*/
static gchar *
ui_get_formatted_email_id(gchar * current_given_name,
			  gchar * current_sur_name, gchar * current_email_id)
{
	GString *email_id_str = NULL;
	gchar *email_id = NULL;

	email_id_str = g_string_new(NULL);

	if ((current_given_name != NULL) && ((strlen(current_given_name) != 0))
	    && (current_sur_name != NULL) && ((strlen(current_sur_name) != 0))) {
		g_string_printf(email_id_str, "%s %s %c%s%c", current_given_name,
				current_sur_name, '<', current_email_id, '>');
		email_id = g_strdup(email_id_str->str);
	} else if ((current_given_name != NULL) && (strlen(current_given_name) != 0)) {
		g_string_printf(email_id_str, "%s %c%s%c", current_given_name, '<',
				current_email_id, '>');
		email_id = g_strdup(email_id_str->str);
	} else if ((current_sur_name != NULL) && (strlen(current_sur_name) != 0)) {
		g_string_printf(email_id_str, "%s %c%s%c", current_sur_name, '<',
				current_email_id, '>');
		email_id = g_strdup(email_id_str->str);
	}
	g_string_free(email_id_str, TRUE);
	email_id_str = NULL;
	return (email_id);
}

/**
 * This is a helper function used to create & run 'Add e-mail address to contact' dialog.
 * It allows user to enter an e-mail address, and shows appropriate infonote if the
 * entered string is not a valid e-mail address.
 *
 * @param  contact_name  Full name of the contact
 * @return E-mail address string entered by user, to be freed by calling function.
 */
static gchar *
run_add_email_addr_to_contact_dlg(const gchar * contact_name)
{
	GtkWidget *add_email_addr_to_contact_dlg = NULL;
	GtkSizeGroup *size_group = NULL;
	GtkWidget *cptn_cntrl = NULL;
	GtkWidget *name_label = NULL;
	GtkWidget *email_entry = NULL;
	gint result = -1;
	gchar *new_email_addr = NULL;
	gboolean run_dialog = TRUE;

	add_email_addr_to_contact_dlg =
	    gtk_dialog_new_with_buttons(_("mcen_ti_add_email_title"), NULL,
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					_("mcen_bd_dialog_ok"), GTK_RESPONSE_ACCEPT,
					_("mcen_bd_dialog_cancel"), GTK_RESPONSE_REJECT, NULL);
	gtk_dialog_set_has_separator(GTK_DIALOG(add_email_addr_to_contact_dlg), FALSE);
	/*Set app_name & state_save related tags to the window */

	size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	name_label = gtk_label_new(contact_name);
	gtk_misc_set_alignment(GTK_MISC(name_label), 0, 0);
	cptn_cntrl =
	    hildon_caption_new(size_group, _("mcen_ia_add_email_name"), name_label, NULL,
			       HILDON_CAPTION_OPTIONAL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_email_addr_to_contact_dlg)->vbox), cptn_cntrl,
			   FALSE, FALSE, 0);

	email_entry = gtk_entry_new();
	cptn_cntrl =
	    hildon_caption_new(size_group, _("mcen_fi_add_email_name"), email_entry, NULL,
			       HILDON_CAPTION_OPTIONAL);
	hildon_gtk_entry_set_input_mode(GTK_ENTRY(email_entry), HILDON_GTK_INPUT_MODE_FULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_email_addr_to_contact_dlg)->vbox), cptn_cntrl,
			   TRUE, TRUE, 0);

	gtk_widget_show_all(add_email_addr_to_contact_dlg);

	while (run_dialog) {
		run_dialog = FALSE;
		gtk_widget_grab_focus(email_entry);
		result = gtk_dialog_run(GTK_DIALOG(add_email_addr_to_contact_dlg));

		if (result == GTK_RESPONSE_ACCEPT) {
			new_email_addr = g_strdup(gtk_entry_get_text(GTK_ENTRY(email_entry)));
			new_email_addr = g_strstrip(new_email_addr);
			if (!modest_text_utils_validate_email_address (new_email_addr)) {
				gtk_widget_grab_focus(email_entry);
				gtk_editable_select_region((GtkEditable *) email_entry, 0, -1);
				hildon_banner_show_information (add_email_addr_to_contact_dlg, NULL, _("mcen_ib_invalid_email"));
				run_dialog = TRUE;
				g_free(new_email_addr);
				new_email_addr = NULL;
			}
		}
	}

	gtk_widget_destroy(add_email_addr_to_contact_dlg);

	return new_email_addr;
}

/**
 * This is helper function to create & run 'Select e-mail address' dialog, used when
 * multiple e-mail addresses are found for a selected contact. It allows user to select
 * one or more e-mail addresses for that contact.
 *
 * @param  email_addr_list  List of e-mail addresses for that contact
 * @return List of user selected e-mail addresses, to be freed by calling function.
 */
static GSList *
select_email_addrs_for_contact(GList * email_addr_list)
{
	GtkWidget *select_email_addr_dlg = NULL;
	GtkWidget *view = NULL, *scrolledwindow = NULL;
	GtkTreeSelection *selection = NULL;
	GtkCellRenderer *renderer = NULL;
	GtkTreeViewColumn *col = NULL;
	GtkListStore *list_store = NULL;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	GList *pathslist = NULL, *node = NULL;
	gint result = -1;
	gchar *email_addr = NULL;
	GSList *selected_email_addr_list = NULL;

	if (!email_addr_list)
		return NULL;

	select_email_addr_dlg =
	    gtk_dialog_new_with_buttons(_("mcen_ti_select_email_title"),
					NULL,
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					_("mcen_bd_dialog_ok"), GTK_RESPONSE_ACCEPT,
					_("mcen_bd_dialog_cancel"), GTK_RESPONSE_REJECT, NULL);
	gtk_dialog_set_has_separator(GTK_DIALOG(select_email_addr_dlg), FALSE);

	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(select_email_addr_dlg)->vbox), scrolledwindow, TRUE,
			   TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	view = gtk_tree_view_new();
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), view);

	list_store = gtk_list_store_new(1, G_TYPE_STRING);
	model = GTK_TREE_MODEL(list_store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

	for (node = email_addr_list; node != NULL && node->data != NULL; node = node->next) {
		email_addr = g_strstrip(g_strdup(node->data));
		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter, 0, email_addr, -1);
		g_free(email_addr);
	}

	gtk_widget_show_all(select_email_addr_dlg);
	result = gtk_dialog_run(GTK_DIALOG(select_email_addr_dlg));

	if (result == GTK_RESPONSE_ACCEPT) {
		pathslist = gtk_tree_selection_get_selected_rows(selection, NULL);
		for (node = pathslist; node != NULL; node = node->next) {
			if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath *) node->data)) {
				gtk_tree_model_get(model, &iter, 0, &email_addr, -1);
				selected_email_addr_list =
				    g_slist_append(selected_email_addr_list, g_strdup(email_addr));
				g_free(email_addr);
			}
		}
	}

	gtk_list_store_clear(list_store);
	gtk_widget_destroy(select_email_addr_dlg);
	return selected_email_addr_list;
}

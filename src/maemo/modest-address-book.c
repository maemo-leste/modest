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
#include "modest-hildon-includes.h"
#include "modest-platform.h"
#include "modest-runtime.h"
#include "widgets/modest-window-mgr.h"
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
static void commit_contact(EContact * contact, gboolean is_new);
static gchar *get_email_addr_from_user(const gchar * given_name);
static gchar *ui_get_formatted_email_id(gchar * current_given_name,
					gchar * current_sur_name, gchar * current_email_id);
static gchar *run_add_email_addr_to_contact_dlg(const gchar * contact_name);
static GSList *select_email_addrs_for_contact(GList * email_addr_list);
static gboolean resolve_address (const gchar *address, GSList **resolved_addresses, gchar **contact_id);
static gchar *unquote_string (const gchar *str);

static void
unref_gobject (GObject *obj)
{
	if (obj)
		g_object_unref (obj);
}

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
#if MODEST_ABOOK_API < 4
		osso_abook_tree_model_set_book_view (OSSO_ABOOK_TREE_MODEL (contact_model),
						     book_view);
#else /* MODEST_ABOOK_API < 4 */
		osso_abook_list_store_set_book_view (OSSO_ABOOK_LIST_STORE (contact_model),
						     book_view);
#endif /* MODEST_ABOOK_API < 4 */

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

static gboolean
open_addressbook_sync ()
{
	book = e_book_new_system_addressbook (NULL);
	if (!book)
		return FALSE;

	e_book_open (book, FALSE, NULL);

	return TRUE;
}

void
modest_address_book_add_address (const gchar *address,
				 GtkWindow *parent)
{
	OssoABookAccount *account = NULL;
	GtkWidget *dialog = NULL;
	gchar *email_address = NULL;

	contact_model = osso_abook_contact_model_new ();
	if (!open_addressbook ()) {
		if (contact_model) {
			g_object_unref (contact_model);
			contact_model = NULL;
		}
		return;
	}

	email_address = modest_text_utils_get_email_address (address);

	account = osso_abook_account_get (EVC_EMAIL, NULL, email_address);
	g_free (email_address);
	if (account)
	{
		dialog = osso_abook_add_to_contacts_dialog_new (contact_model, account);
		g_object_unref (account);

		modest_window_mgr_set_modal (modest_runtime_get_window_mgr(),
					     (GtkWindow *) parent,
					     (GtkWindow *) dialog);

		gtk_dialog_run (GTK_DIALOG (dialog));

		if (contact_model) {
			g_object_unref (contact_model);
			contact_model = NULL;
		}

		gtk_widget_destroy (dialog);
	}

}

void
modest_address_book_select_addresses (ModestRecptEditor *recpt_editor,
				      GtkWindow *parent_window)
{
#if MODEST_ABOOK_API < 4
	GtkWidget *contact_view = NULL;
	GtkWidget *contact_dialog;
	GtkWidget *toplevel;
#else /* MODEST_ABOOK_API < 4 */
	OssoABookContactChooser *contact_chooser = NULL;
#endif /* MODEST_ABOOK_API < 4 */

	GList *contacts_list = NULL;
	GSList *email_addrs_per_contact = NULL;
	gchar *econtact_id;
	gboolean focus_recpt_editor = FALSE;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));

#if MODEST_ABOOK_API < 4
	if (!open_addressbook ()) {
		if (contact_model) {
			g_object_unref (contact_model);
			contact_model = NULL;
		}
		return;
	}
	contact_model = osso_abook_contact_model_new ();

	contact_view = osso_abook_contact_selector_new_basic (contact_model);
	osso_abook_contact_selector_set_minimum_selection (OSSO_ABOOK_CONTACT_SELECTOR (contact_view), 1);

	contact_dialog = osso_abook_select_dialog_new (OSSO_ABOOK_TREE_VIEW (contact_view));
	gtk_window_set_title (GTK_WINDOW (contact_dialog), _("mcen_ti_select_recipients"));
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (recpt_editor));
	modest_window_mgr_set_modal (modest_runtime_get_window_mgr (), GTK_WINDOW (contact_dialog), (GtkWindow *) toplevel);

	gtk_widget_show (contact_dialog);

	if (gtk_dialog_run (GTK_DIALOG (contact_dialog)) == GTK_RESPONSE_OK) {
		contacts_list = 
			osso_abook_contact_selector_get_extended_selection (OSSO_ABOOK_CONTACT_SELECTOR
									     (contact_view));
	}
#else /* MODEST_ABOOK_API < 4 */
	/* TODO: figure out how to make the contact chooser modal */
	contact_chooser = osso_abook_contact_chooser_new
		("title", _("mcen_ti_select_recipients"),
		 "help-topic", "",
		 "minimum-selection", 1, NULL);

	if (osso_abook_contact_chooser_run (contact_chooser) == GTK_RESPONSE_OK)
		contacts_list = osso_abook_contact_chooser_get_selection (contact_chooser);

	g_object_unref (contact_chooser);
#endif
	
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
				focus_recpt_editor = TRUE;
			}
		}
		g_list_free (contacts_list);
	}

#if MODEST_ABOOK_API < 4
	if (contact_model) {
		g_object_unref (contact_model);
		contact_model = NULL;
	}

	gtk_widget_destroy (contact_dialog);
#endif

	if (focus_recpt_editor)
		modest_recpt_editor_grab_focus (MODEST_RECPT_EDITOR (recpt_editor));

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
#if MODEST_ABOOK_API < 4
		display_name = osso_abook_contact_get_display_name(contact);		
#else
		OssoABookContact *abook_contact;
	       
		abook_contact = osso_abook_contact_new_from_template (contact);
		display_name = osso_abook_contact_get_display_name(abook_contact);
		g_object_unref (abook_contact);
#endif

		emailid = get_email_addr_from_user(display_name);
		if (emailid) {
			e_contact_set(contact, E_CONTACT_EMAIL_1, emailid);
			commit_contact(contact, FALSE);
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
commit_contact(EContact * contact, gboolean is_new)
{
	g_return_if_fail (contact);
	g_return_if_fail (book);
	
	if (!contact || !book)
		return;
	
#if MODEST_ABOOK_API < 2
	osso_abook_contact_commit(contact, is_new, book);
#else
	osso_abook_contact_commit(contact, is_new, book, NULL);
#endif /* MODEST_ABOOK_API < 2 */
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

	email_id_str = g_string_new(NULL);

	if ((current_given_name != NULL) && ((strlen(current_given_name) != 0))
	    && (current_sur_name != NULL) && ((strlen(current_sur_name) != 0))) {
		g_string_append_printf(email_id_str, "%s %s", current_given_name, current_sur_name);
	} else if ((current_given_name != NULL) && (strlen(current_given_name) != 0)) {
		g_string_append_printf(email_id_str, "%s", current_given_name);
	} else if ((current_sur_name != NULL) && (strlen(current_sur_name) != 0)) {
		g_string_append_printf(email_id_str, "%s", current_sur_name);
	}
	if (g_utf8_strchr (email_id_str->str, -1, ' ')) {
		g_string_prepend_c (email_id_str, '\"');
		g_string_append_c (email_id_str, '\"');
	}
	g_string_append_printf (email_id_str, " %c%s%c", '<', current_email_id, '>');
	return g_string_free (email_id_str, FALSE);
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
			const gchar *invalid_char_offset = NULL;
			new_email_addr = g_strdup(gtk_entry_get_text(GTK_ENTRY(email_entry)));
			new_email_addr = g_strstrip(new_email_addr);
			if (!modest_text_utils_validate_email_address (new_email_addr, &invalid_char_offset)) {
				gtk_widget_grab_focus(email_entry);
				if ((invalid_char_offset != NULL)&&(*invalid_char_offset != '\0')) {
					gchar *char_in_string = g_strdup_printf ("%c", *invalid_char_offset);
					gchar *message = g_strdup_printf(
						_CS("ckdg_ib_illegal_characters_entered"), 
						char_in_string);
					hildon_banner_show_information (
						add_email_addr_to_contact_dlg, NULL, message );
					g_free (message);
				} else {
					hildon_banner_show_information (add_email_addr_to_contact_dlg, NULL, _("mcen_ib_invalid_email"));
					run_dialog = TRUE;
				}
				gtk_editable_select_region((GtkEditable *) email_entry, 0, -1);
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

	/* Make the window approximately big enough, because it doesn't resize to be big enough 
	 * for the window title text: */
	gtk_window_set_default_size (GTK_WINDOW (select_email_addr_dlg), 400, -1);

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
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter);
	gtk_tree_selection_select_iter (selection, &iter);

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


static gboolean /* make this public? */
add_to_address_book (const gchar* address)
{
	EBookQuery *query;
	GList *contacts = NULL;
	GError *err = NULL;
	gchar *email;
	
	g_return_val_if_fail (address, FALSE);
	
	if (!book)
		open_addressbook ();
	
	g_return_val_if_fail (book, FALSE);

	email = modest_text_utils_get_email_address (address);
	
	query = e_book_query_field_test (E_CONTACT_EMAIL, E_BOOK_QUERY_IS, email);
	if (!e_book_get_contacts (book, query, &contacts, &err)) {
		g_printerr ("modest: failed to get contacts: %s",
			    err ? err->message : "<unknown>");
		if (err)
			g_error_free (err);
		return FALSE;
	}
	e_book_query_unref (query);
	
	/*  we need to 'commit' it, even if we already found the email
	 * address in the addressbook; thus, it will show up in the 'recent list' */
	if (contacts)  {		
		g_debug ("%s already in the address book", address);
		commit_contact ((EContact*)contacts->data, FALSE);
		
		g_list_foreach (contacts, (GFunc)unref_gobject, NULL);
		g_list_free (contacts);

	} else {
		/* it's not yet in the addressbook, add it now! */
		EContact *new_contact = e_contact_new ();
		gchar *display_address;
		display_address = g_strdup (address);
		if (display_address) {
			modest_text_utils_get_display_address (display_address);
			if ((display_address[0] != '\0') && (strlen (display_address) != strlen (address)))
				e_contact_set (new_contact, E_CONTACT_FULL_NAME, (const gpointer)display_address);
		}
		e_contact_set (new_contact, E_CONTACT_EMAIL_1, (const gpointer)email);
		g_free (display_address);
		commit_contact (new_contact, TRUE);
		g_debug ("%s added to address book", address);
		g_object_unref (new_contact);
	}

	g_free (email);

	return TRUE;
}

static gboolean
show_check_names_banner (gpointer userdata)
{
	GtkWidget **banner = (GtkWidget **) userdata;

	gdk_threads_enter ();
	*banner = modest_platform_animation_banner (NULL, NULL, _("mail_ib_checking_names"));
	gdk_threads_leave ();

	return FALSE;
}

static void
hide_check_names_banner (GtkWidget **banner, guint banner_timeout)
{
	if (*banner != NULL) {
		gtk_widget_destroy (*banner);
		*banner = NULL;
	} else {
		g_source_remove (banner_timeout);
	}

}

gboolean
modest_address_book_check_names (ModestRecptEditor *recpt_editor,
				 GSList **address_list)
{
	const gchar *recipients = NULL;
	GSList *start_indexes = NULL, *end_indexes = NULL;
	GSList *current_start, *current_end;
	gboolean result = TRUE;
	GtkTextBuffer *buffer;
	gint offset_delta = 0;
	gint last_length;
	GtkTextIter start_iter, end_iter;

	g_return_val_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor), FALSE);

	recipients = modest_recpt_editor_get_recipients (recpt_editor);
	last_length = g_utf8_strlen (recipients, -1);
	modest_text_utils_get_addresses_indexes (recipients, &start_indexes, &end_indexes);

	if (start_indexes == NULL) {
		if (last_length != 0) {
			hildon_banner_show_information (NULL, NULL, _("mcen_nc_no_matching_contacts"));
			return FALSE;
		} else {
			return TRUE;
		}
	}

	current_start = start_indexes;
	current_end = end_indexes;
	buffer = modest_recpt_editor_get_buffer (recpt_editor);

	while (current_start != NULL) {
		gchar *address;
		gchar *start_ptr, *end_ptr;
		gint start_pos, end_pos;
		const gchar *invalid_char_position = NULL;
		gboolean store_address = FALSE;

		start_pos = (*((gint*) current_start->data)) + offset_delta;
		end_pos = (*((gint*) current_end->data)) + offset_delta;
	       
		start_ptr = g_utf8_offset_to_pointer (recipients, start_pos);
		end_ptr = g_utf8_offset_to_pointer (recipients, end_pos);

		address = g_strstrip (g_strndup (start_ptr, end_ptr - start_ptr));
		gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, start_pos);
		gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, end_pos);
		gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);

		if (!modest_text_utils_validate_recipient (address, &invalid_char_position)) {
			if ((invalid_char_position != NULL) && (*invalid_char_position != '\0')) {
				gchar *char_in_string = g_strdup_printf("%c", *invalid_char_position);
				gchar *message = g_strdup_printf(
					_CS("ckdg_ib_illegal_characters_entered"), 
					char_in_string);
				g_free (char_in_string);
				hildon_banner_show_information (NULL, NULL, message );
				g_free (message);
				result = FALSE;
			} else if (strstr (address, "@") == NULL) {
				/* here goes searching in addressbook */
				gchar *contact_id = NULL;
				GSList *resolved_addresses = NULL;

				result = resolve_address (address, &resolved_addresses, &contact_id);

				if (result) {
					gint new_length;
					/* replace string */
					modest_recpt_editor_replace_with_resolved_recipient (recpt_editor,
											     &start_iter, &end_iter,
											     resolved_addresses, 
											     contact_id);
					g_free (contact_id);
					g_slist_foreach (resolved_addresses, (GFunc) g_free, NULL);
					g_slist_free (resolved_addresses);

					/* update offset delta */
					recipients = modest_recpt_editor_get_recipients (recpt_editor);
					new_length = g_utf8_strlen (recipients, -1);
					offset_delta = offset_delta + new_length - last_length;
					last_length = new_length;					
				}
			} else {
				/* this address is not valid, select it and return control to user showing banner */
				hildon_banner_show_information (NULL, NULL, _("mcen_ib_invalid_email"));
				result = FALSE;
			}
		} else {
			GSList *tags, *node;
			gboolean has_recipient = FALSE;

			tags = gtk_text_iter_get_tags (&start_iter);
			for (node = tags; node != NULL; node = g_slist_next (node)) {
				GtkTextTag *tag = GTK_TEXT_TAG (node->data);
				if (g_object_get_data (G_OBJECT (tag), "recipient-tag-id") != NULL) {
					has_recipient = TRUE;
					break;
				}
			}
			g_slist_free (tags);
			if (!has_recipient) {
				GSList * addr_list = NULL;

				addr_list = g_slist_prepend (addr_list, address);
				modest_recpt_editor_replace_with_resolved_recipient (recpt_editor,
										     &start_iter, &end_iter,
										     addr_list,
										     "");
				g_slist_free (addr_list);
				store_address = TRUE;
			}
		}

		/* so, it seems a valid address */
		/* note: adding it the to the addressbook if it did not exist yet,
		 * and adding it to the recent_list */
		if (result && address_list && store_address)
			*address_list = g_slist_prepend (*address_list, address);
		else
			g_free (address);

		if (result == FALSE)
			break;

		current_start = g_slist_next (current_start);
		current_end = g_slist_next (current_end);
	}

	/* Remove dup's */
	if (address_list && *address_list)
		*address_list = modest_text_utils_remove_duplicate_addresses_list (*address_list);

	if (current_start == NULL) {
		gtk_text_buffer_get_end_iter (buffer, &end_iter);
		gtk_text_buffer_place_cursor (buffer, &end_iter);
	}

	g_slist_foreach (start_indexes, (GFunc) g_free, NULL);
	g_slist_foreach (end_indexes, (GFunc) g_free, NULL);
	g_slist_free (start_indexes);
	g_slist_free (end_indexes);

	return result;

}

static GList *
get_contacts_for_name (const gchar *name)
{
	EBookQuery *full_name_book_query = NULL;
	GList *result;
	gchar *unquoted;

	if (name == NULL)
		return NULL;

	unquoted = unquote_string (name);
	full_name_book_query = e_book_query_field_test (E_CONTACT_FULL_NAME, E_BOOK_QUERY_CONTAINS, unquoted);
	g_free (unquoted);

	e_book_get_contacts (book, full_name_book_query, &result, NULL);
	e_book_query_unref (full_name_book_query);

	return result;
}

static GList *
select_contacts_for_name_dialog (const gchar *name)
{
	EBookQuery *full_name_book_query = NULL;
	EBookView *book_view = NULL;
	GList *result = NULL;
	gchar *unquoted;

	unquoted = unquote_string (name);
	full_name_book_query = e_book_query_field_test (E_CONTACT_FULL_NAME, E_BOOK_QUERY_CONTAINS, unquoted);
	g_free (unquoted);
	e_book_get_book_view (book, full_name_book_query, NULL, -1, &book_view, NULL);
	e_book_query_unref (full_name_book_query);

	if (book_view) {
		GtkWidget *contact_view = NULL;
		GtkWidget *contact_dialog = NULL;
#if MODEST_ABOOK_API < 4
		osso_abook_tree_model_set_book_view (OSSO_ABOOK_TREE_MODEL (contact_model), book_view);
#else /* MODEST_ABOOK_API < 4 */
		osso_abook_list_store_set_book_view (OSSO_ABOOK_LIST_STORE (contact_model), book_view);
#endif /* MODEST_ABOOK_API < 4 */
		e_book_view_start (book_view);
		
		contact_view = osso_abook_contact_selector_new_basic (contact_model);
		contact_dialog = osso_abook_select_dialog_new (OSSO_ABOOK_TREE_VIEW (contact_view));
		gtk_window_set_title (GTK_WINDOW (contact_dialog), _("mcen_ti_select_recipients"));

		if (gtk_dialog_run (GTK_DIALOG (contact_dialog)) == GTK_RESPONSE_OK) {
			result = osso_abook_contact_view_get_selection (OSSO_ABOOK_CONTACT_VIEW (contact_view));
		}
		e_book_view_stop (book_view);
		g_object_unref (book_view);
		gtk_widget_destroy (contact_dialog);
	}

	return result;
}

static gboolean
resolve_address (const gchar *address, GSList **resolved_addresses, gchar **contact_id)
{
	GList *resolved_contacts;
	guint banner_timeout;
	GtkWidget *banner = NULL;
	
	banner_timeout = g_timeout_add (500, show_check_names_banner, &banner);

	contact_model = osso_abook_contact_model_new ();
	if (!open_addressbook_sync ()) {
		if (contact_model) {
			g_object_unref (contact_model);
			contact_model = NULL;
		}
		return FALSE;
	}

	resolved_contacts = get_contacts_for_name (address);

	if (resolved_contacts == NULL) {
		/* no matching contacts for the search string */
		modest_platform_run_information_dialog (NULL, _("mcen_nc_no_matching_contacts"), FALSE);
		hide_check_names_banner (&banner, banner_timeout);
     
		return FALSE;
	}

	if (g_list_length (resolved_contacts) > 1) {
		/* show a dialog to select the contact from the resolved ones */
		g_list_free (resolved_contacts);

		hide_check_names_banner (&banner, banner_timeout);     
		resolved_contacts = select_contacts_for_name_dialog (address);
		banner_timeout = g_timeout_add (500, show_check_names_banner, &banner);

	}
	
	/* get the resolved contacts (can be no contact) */
	if (resolved_contacts) {
		gboolean found;
		EContact *contact = (EContact *) resolved_contacts->data;

		*resolved_addresses = get_recipients_for_given_contact (contact);
		hide_check_names_banner (&banner, banner_timeout);     
		if (*resolved_addresses) {
			*contact_id = g_strdup (e_contact_get_const (contact, E_CONTACT_UID));
			found = TRUE;
		} else {
			found = FALSE;
		}

		g_list_foreach (resolved_contacts, (GFunc)unref_gobject, NULL);
		g_list_free (resolved_contacts);

		return found;
	} else {
		/* cancelled dialog to select more than one contact or
		 * selected no contact */
		hide_check_names_banner (&banner, banner_timeout);     
		return FALSE;
	}

}

static gchar *
unquote_string (const gchar *str)
{
	GString *buffer;
	gchar *p;

	if (str == NULL)
		return NULL;

	buffer = g_string_new_len (NULL, strlen (str));
	for (p = (gchar *) str; *p != '\0'; p = g_utf8_next_char (p)) {
		if (*p == '"') {
			p = g_utf8_next_char (p);
			while ((*p != '\0')&&(*p != '"')) {
				if (*p == '\\') {
					g_string_append_unichar (buffer, g_utf8_get_char (p));
					p = g_utf8_next_char (p);
					
				}
				g_string_append_unichar (buffer, g_utf8_get_char (p));
				p = g_utf8_next_char (p);
			}
		} else {
			g_string_append_unichar (buffer, g_utf8_get_char (p));
		}
	}

	return g_string_free (buffer, FALSE);

}

gboolean
modest_address_book_has_address (const gchar *address)
{
	EBookQuery *query;
	GList *contacts = NULL;
	GError *err = NULL;
	gchar *email;
	gboolean result;

	g_return_val_if_fail (address, FALSE);
	
	if (!book) {
		if (!open_addressbook ()) {
			g_return_val_if_reached (FALSE);
		}
	}
	
	g_return_val_if_fail (book, FALSE);

	email = modest_text_utils_get_email_address (address);
	
	query = e_book_query_field_test (E_CONTACT_EMAIL, E_BOOK_QUERY_IS, email);
	if (!e_book_get_contacts (book, query, &contacts, &err)) {
		g_printerr ("modest: failed to get contacts: %s",
			    err ? err->message : "<unknown>");
		if (err)
			g_error_free (err);
		return FALSE;
	}
	e_book_query_unref (query);

	result = (contacts != NULL);
	if (contacts) {
		g_list_foreach (contacts, (GFunc)unref_gobject, NULL);
		g_list_free (contacts);
	}
	
	g_free (email);

	return result;
}

const gchar *
modest_address_book_get_my_name ()
{
	/* There is no support to get my own contact in this version */
	return NULL;
}


void
modest_address_book_init (void)
{
	open_addressbook ();
}

void
modest_address_book_add_address_list (GSList *address_list)
{

}

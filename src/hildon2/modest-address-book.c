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
#include <libebook/e-vcard.h>
#include "modest-hildon-includes.h"
#include <libosso-abook/osso-abook.h>
#include <libedataserver/e-data-server-util.h>
#include "modest-platform.h"
#include "modest-runtime.h"
#include "widgets/modest-window-mgr.h"
#include "widgets/modest-ui-constants.h"
#include <string.h>
#include <gtk/gtksizegroup.h>
#include <gtk/gtkbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkentry.h>
#include <modest-maemo-utils.h>

static OssoABookContactModel *contact_model =  NULL;
static EBook *book = NULL;
static EBookView * book_view = NULL;

static GSList *get_recipients_for_given_contact (EContact * contact, gboolean *canceled);
static gchar *get_email_addr_from_user(const gchar * given_name, gboolean *canceled);
static gchar *ui_get_formatted_email_id(gchar * current_given_name,
					gchar * current_sur_name, gchar * current_email_id);
static gchar *run_add_email_addr_to_contact_dlg(const gchar * contact_name, gboolean *canceled);
static GSList *select_email_addrs_for_contact(GList * email_addr_list);
static gboolean resolve_address (const gchar *address, GSList **resolved_addresses, GSList **contact_id, gboolean *canceled);
static gchar *unquote_string (const gchar *str);

static gboolean
open_addressbook ()
{
	OssoABookRoster *roster;
	GError *error = NULL;
	time_t init,end;

	if (book && book_view)
		return TRUE;

	roster = osso_abook_aggregator_get_default (&error);
	if (error)
		goto error;

	/* Wait until it's ready */
	init = time (NULL);
	osso_abook_waitable_run ((OssoABookWaitable *) roster,
				 g_main_context_default (),
				 &error);
	end = time (NULL);
	g_debug ("Opening addressbook lasted %ld seconds", (gint) end-init);

	if (error)
		goto error;

	if (!osso_abook_waitable_is_ready ((OssoABookWaitable *) roster,
					   &error))
		goto error;

	book = osso_abook_roster_get_book (roster);
	book_view = osso_abook_roster_get_book_view (roster);

	return TRUE;
 error:
	g_warning ("error opening addressbook %s", error->message);
	g_error_free (error);
	return FALSE;
}

void
modest_address_book_add_address (const gchar *address,
				 GtkWindow *parent)
{
	GtkWidget *dialog = NULL;
	gchar *email_address;
	EVCardAttribute *attribute;

	if (!open_addressbook ()) {
		return;
	}

	email_address = modest_text_utils_get_email_address (address);

	attribute = e_vcard_attribute_new (NULL, EVC_EMAIL);
	e_vcard_attribute_add_value (attribute, email_address);
	dialog = osso_abook_temporary_contact_dialog_new (parent, book, attribute, NULL);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	e_vcard_attribute_free (attribute);
	g_free (email_address);

}

void
modest_address_book_select_addresses (ModestRecptEditor *recpt_editor,
				      GtkWindow *parent_window)
{
	GtkWidget *contact_chooser = NULL;
	GList *contacts_list = NULL;
	GSList *email_addrs_per_contact = NULL;
	gchar *econtact_id;
	gboolean focus_recpt_editor = FALSE;

	g_return_if_fail (MODEST_IS_RECPT_EDITOR (recpt_editor));

	/* TODO: figure out how to make the contact chooser modal */
	contact_chooser = osso_abook_contact_chooser_new_with_capabilities (parent_window,
									    _AB("addr_ti_dia_select_contacts"),
									    OSSO_ABOOK_CAPS_EMAIL, 
									    OSSO_ABOOK_CONTACT_ORDER_NAME);
	/* Enable multiselection */
	osso_abook_contact_chooser_set_maximum_selection (OSSO_ABOOK_CONTACT_CHOOSER (contact_chooser),
							  G_MAXUINT);

	if (gtk_dialog_run (GTK_DIALOG (contact_chooser)) == GTK_RESPONSE_OK)
		contacts_list = osso_abook_contact_chooser_get_selection (OSSO_ABOOK_CONTACT_CHOOSER (contact_chooser));
	gtk_widget_destroy (contact_chooser);

	if (contacts_list) {
		GList *node;

		for (node = contacts_list; node != NULL; node = g_list_next (node)) {
			EContact *contact = (EContact *) node->data;
			gboolean canceled;

			email_addrs_per_contact = get_recipients_for_given_contact (contact, &canceled);
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
static GSList *
get_recipients_for_given_contact (EContact * contact,
				  gboolean *canceled)
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
		OssoABookContact *abook_contact;

		abook_contact = osso_abook_contact_new_from_template (contact);
		display_name = osso_abook_contact_get_display_name(abook_contact);

		emailid = get_email_addr_from_user(display_name, canceled);
		if (emailid) {
			list = g_list_append (list, g_strdup (emailid));
			e_contact_set(E_CONTACT (abook_contact), E_CONTACT_EMAIL, list);
			osso_abook_contact_commit (abook_contact, FALSE, NULL, NULL);
		}
		g_object_unref (abook_contact);
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
 * This is a helper function used to launch 'Add e-mail address to contact' dialog
 * after showing appropriate notification, when there is no e-mail address defined
 * for a selected contact.
 *
 * @param  given_name  Given name of the contact
 * @param  family_name  Family name of the contact
 * @return E-mail address string entered by user, to be freed by calling function.
 */
static gchar *
get_email_addr_from_user(const gchar * given_name, gboolean *canceled)
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
		email_addr = run_add_email_addr_to_contact_dlg (given_name, canceled);
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
	g_string_prepend_c (email_id_str, '\"');
	g_string_append_c (email_id_str, '\"');

	g_string_append_printf (email_id_str, " %c%s%c", '<', current_email_id, '>');
	return g_string_free (email_id_str, FALSE);
}

/**
 * This is a helper function used to create & run 'Add e-mail address to contact' dialog.
 * It allows user to enter an e-mail address, and shows appropriate infonote if the
 * entered string is not a valid e-mail address.
 *
 * It must return TRUE in canceled if the dialog was canceled by the user
 *
 * @param  contact_name  Full name of the contact
 * @return E-mail address string entered by user, to be freed by calling function.
 */
static gchar *
run_add_email_addr_to_contact_dlg(const gchar * contact_name,
				  gboolean *canceled)
{
	GtkWidget *add_email_addr_to_contact_dlg = NULL;
	GtkSizeGroup *size_group = NULL;
	GtkWidget *cptn_cntrl = NULL;
	GtkWidget *name_label = NULL;
	GtkWidget *email_entry = NULL;
	gint result = -1;
	gchar *new_email_addr = NULL;
	gboolean run_dialog = TRUE;

	g_return_val_if_fail (canceled, NULL);

	*canceled = FALSE;

	add_email_addr_to_contact_dlg =
	    gtk_dialog_new_with_buttons(_("mcen_ti_add_email_title"), NULL,
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					_HL("wdgt_bd_save"), GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_has_separator(GTK_DIALOG(add_email_addr_to_contact_dlg), FALSE);
#ifdef MODEST_TOOLKIT_HILDON2
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (add_email_addr_to_contact_dlg)->vbox), 
					HILDON_MARGIN_DOUBLE);
#endif
	/*Set app_name & state_save related tags to the window */

	size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	name_label = gtk_label_new(contact_name);
	gtk_misc_set_alignment(GTK_MISC(name_label), 0.0, 0.5);
	cptn_cntrl =
		modest_maemo_utils_create_captioned (size_group, NULL,
						     _("mcen_ia_add_email_name"), FALSE,
						     name_label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(add_email_addr_to_contact_dlg)->vbox), cptn_cntrl,
			   FALSE, FALSE, 0);

	email_entry = modest_toolkit_factory_create_entry (modest_runtime_get_toolkit_factory ());
	cptn_cntrl = modest_maemo_utils_create_captioned (size_group, NULL, 
							  _("mcen_fi_add_email_name"), FALSE,
							  email_entry);
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
			new_email_addr = g_strdup(modest_entry_get_text(email_entry));
			new_email_addr = g_strstrip(new_email_addr);
			if (!modest_text_utils_validate_email_address (new_email_addr, &invalid_char_offset)) {
				gtk_widget_grab_focus(email_entry);
				if ((invalid_char_offset != NULL)&&(*invalid_char_offset != '\0')) {
					gchar *char_in_string = g_strdup_printf ("%c", *invalid_char_offset);
					gchar *message = g_strdup_printf(
						_CS("ckdg_ib_illegal_characters_entered"), 
						char_in_string);
					g_free (char_in_string);
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
		} else {
			*canceled = TRUE;
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
	GSList *selected_email_addr_list = NULL;
	GList *node;
	GtkWidget *selector;
	gint result = -1;

	if (!email_addr_list)
		return NULL;

	select_email_addr_dlg = hildon_picker_dialog_new (NULL);
	gtk_window_set_title (GTK_WINDOW (select_email_addr_dlg), _("mcen_ti_select_email_title"));

	selector = hildon_touch_selector_new_text ();
	for (node = email_addr_list; node != NULL && node->data != NULL; node = node->next) {
		gchar *email_addr;
		email_addr = g_strstrip(g_strdup(node->data));
		hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (selector), email_addr);
		g_free(email_addr);
	}

	hildon_picker_dialog_set_selector (HILDON_PICKER_DIALOG (select_email_addr_dlg),
					   HILDON_TOUCH_SELECTOR (selector));
	gtk_window_set_default_size (GTK_WINDOW (select_email_addr_dlg), MODEST_DIALOG_WINDOW_MAX_HEIGHT, -1);

	gtk_widget_show_all(select_email_addr_dlg);
	result = gtk_dialog_run(GTK_DIALOG(select_email_addr_dlg));

	if (result == GTK_RESPONSE_OK) {
		gchar *current_text;

		current_text = hildon_touch_selector_get_current_text (HILDON_TOUCH_SELECTOR (selector));
		selected_email_addr_list = g_slist_append (selected_email_addr_list, current_text);
	}

	gtk_widget_destroy(select_email_addr_dlg);
	return selected_email_addr_list;
}

/* Assumes that the second argument (the user provided one) is a pure
   email address without name */
static gint
compare_addresses (const gchar *address1,
		   const gchar *mail2)
{
	gint retval;
	gchar *mail1, *mail1_down, *mail2_down;

	/* Perform a case insensitive comparison */
	mail1 = modest_text_utils_get_email_address (address1);
	mail1_down = g_ascii_strdown (mail1, -1);
	mail2_down = g_ascii_strdown (mail2, -1);
	retval = g_strcmp0 (mail1_down, mail2_down);
	g_free (mail1);
	g_free (mail1_down);
	g_free (mail2_down);

	return retval;
}

static EContact *
get_contact_for_address (GList *contacts,
			 const gchar *address)
{
	EContact *retval = NULL, *contact;
	GList *iter;
	gchar *email;

	email = modest_text_utils_get_email_address (address);
	iter = contacts;
	while (iter && !retval) {
		GList *emails = NULL;

		contact = E_CONTACT (iter->data);
		emails = e_contact_get (contact, E_CONTACT_EMAIL);
		if (emails) {
			/* Look for the email address */
			if (g_list_find_custom (emails, email, (GCompareFunc) compare_addresses))
				retval = contact;

			/* Free the list */
			g_list_foreach (emails, (GFunc) g_free, NULL);
			g_list_free (emails);
		}
		iter = g_list_next (iter);
	}
	g_free (email);

	return retval;
}

static void
async_get_contacts_cb (EBook *book,
		       EBookStatus status,
		       GList *contacts,
		       gpointer closure)
{
	GSList *addresses, *iter;
	GList *to_commit_contacts, *to_add_contacts;
	EContact *self_contact;

	addresses = (GSList *) closure;

	/* Check errors */
	if (status != E_BOOK_ERROR_OK)
		goto frees;

	self_contact = (EContact *) osso_abook_self_contact_get_default ();
	if (self_contact) {
		contacts = g_list_prepend (contacts, self_contact);
	}

	iter = addresses;
	to_commit_contacts = NULL;
	to_add_contacts = NULL;
	while (iter) {
		EContact *contact;
		const gchar *address;

		/* Look for a contact with such address. We perform
		   this kind of search because we assume that users
		   don't usually send emails to tons of addresses */
		address = (const gchar *) iter->data;
		contact = get_contact_for_address (contacts, address);

		/* Add new or commit existing contact */
		if (contact) {
			to_commit_contacts = g_list_prepend (to_commit_contacts, contact);
			g_debug ("----Preparing to commit contact %s", address);
		} else {
			gchar *email_address, *display_address;
			GList *email_list = NULL;

			/* Create new contact and add it to the list */
			contact = e_contact_new ();
			email_address = modest_text_utils_get_email_address (address);
			email_list = g_list_append (email_list, email_address);
			e_contact_set (contact, E_CONTACT_EMAIL, email_list);
			g_free (email_address);
			g_list_free (email_list);

			display_address = g_strdup (address);
			if (display_address) {
				modest_text_utils_get_display_address (display_address);
				if ((display_address[0] != '\0') && (strlen (display_address) != strlen (address)))
					e_contact_set (contact, E_CONTACT_FULL_NAME, (const gpointer)display_address);
				g_free (display_address);
			}

			to_add_contacts = g_list_prepend (to_add_contacts, contact);
			g_debug ("----Preparing to add contact %s", address);
		}

		iter = g_slist_next (iter);
	}

	/* Asynchronously add contacts */
	if (to_add_contacts)
		e_book_async_add_contacts (book, to_add_contacts, NULL, NULL);

	/* Asynchronously commit contacts */
	if (to_commit_contacts)
		e_book_async_commit_contacts (book, to_commit_contacts, NULL, NULL);

	/* Free lists */
	g_list_free (to_add_contacts);
	g_list_free (to_commit_contacts);

 frees:
	if (addresses) {
		g_slist_foreach (addresses, (GFunc) g_free, NULL);
		g_slist_free (addresses);
	}
	if (contacts)
		g_list_free (contacts);
}

typedef struct _CheckNamesInfo {
	GtkWidget *banner;
	guint show_banner_timeout;
	guint hide_banner_timeout;
	gboolean hide;
	gboolean free_info;
} CheckNamesInfo;

static void
hide_check_names_banner (CheckNamesInfo *info)
{
	if (info->show_banner_timeout > 0) {
		g_source_remove (info->show_banner_timeout);
		info->show_banner_timeout = 0;
	}
	if (info->hide_banner_timeout > 0) {
		info->hide = TRUE;
		return;
	}

	if (info->banner) {
		gtk_widget_destroy (info->banner);
		info->banner = NULL;
		info->hide = FALSE;
	}

	if (info->free_info) {
		g_slice_free (CheckNamesInfo, info);
	}
}

static gboolean hide_banner_timeout_handler (CheckNamesInfo *info)
{
	info->hide_banner_timeout = 0;
	if (info->hide) {
		gtk_widget_destroy (info->banner);
		info->banner = NULL;
	}
	if (info->free_info) {
		g_slice_free (CheckNamesInfo, info);
	}
	return FALSE;
}

static gboolean show_banner_timeout_handler (CheckNamesInfo *info)
{
	info->show_banner_timeout = 0;
	info->banner = hildon_banner_show_animation (NULL, NULL, _("mail_ib_checking_names"));
	info->hide_banner_timeout = g_timeout_add (1000, (GSourceFunc) hide_banner_timeout_handler, (gpointer) info);
	return FALSE;
}

static void show_check_names_banner (CheckNamesInfo *info)
{
	if (info->hide_banner_timeout > 0) {
		g_source_remove (info->hide_banner_timeout);
		info->hide_banner_timeout = 0;
	}

	info->hide = FALSE;
	if (info->show_banner_timeout > 0)
		return;

	if (info->banner == NULL) {
		info->show_banner_timeout = g_timeout_add (500, (GSourceFunc) show_banner_timeout_handler, (gpointer) info);
	}
}

static void clean_check_names_banner (CheckNamesInfo *info)
{
	if (info->hide_banner_timeout) {
		info->free_info = TRUE;
	} else {
		if (info->show_banner_timeout) {
			g_source_remove (info->show_banner_timeout);
		}
		if (info->banner)
			gtk_widget_destroy (info->banner);
		g_slice_free (CheckNamesInfo, info);
	}
}

void free_resolved_addresses_list (gpointer data,
				   gpointer ignored)
{
	GSList *list = (GSList *)data;
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
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
				gchar *message = 
					g_strdup_printf(_CS("ckdg_ib_illegal_characters_entered"), 
							char_in_string);
				g_free (char_in_string);
				hildon_banner_show_information (NULL, NULL, message );
				g_free (message);
				result = FALSE;
			} else if (strstr (address, "@") == NULL) {
				/* here goes searching in addressbook */
				gboolean canceled;
				GSList *contact_ids = NULL;
				GSList *resolved_addresses = NULL;

				result = resolve_address (address, &resolved_addresses, &contact_ids, &canceled);

				if (result) {
					gint new_length;

					modest_recpt_editor_replace_with_resolved_recipients (recpt_editor,
											      &start_iter, &end_iter,
											      resolved_addresses,
											      contact_ids);
					g_slist_foreach (contact_ids, (GFunc) g_free, NULL);
					g_slist_foreach (resolved_addresses, free_resolved_addresses_list, NULL);
					g_slist_free (contact_ids);
					g_slist_free (resolved_addresses);

					/* update offset delta */
					recipients = modest_recpt_editor_get_recipients (recpt_editor);
					new_length = g_utf8_strlen (recipients, -1);
					offset_delta = offset_delta + new_length - last_length;
					last_length = new_length;
				} else {
					if (canceled) {
						/* We have to remove the recipient if not resolved */
						modest_recpt_editor_replace_with_resolved_recipient (recpt_editor,
												     &start_iter, 
												     &end_iter,
												     NULL,
												     NULL);
					} else {
						/* There is no contact with that name so it's not
						   valid. Don't show any error because it'll be done
						   later */
						result = FALSE;
					}
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

typedef struct _GetContactsInfo {
	GMainLoop *mainloop;
	GList *result;
} GetContactsInfo;

static void 
get_contacts_for_name_cb (EBook *book, 
			  EBookStatus status, 
			  GList *list, 
			  gpointer userdata)
{
	GetContactsInfo *info = (GetContactsInfo *) userdata;

	if (status == E_BOOK_ERROR_OK)
		info->result = list;

	g_main_loop_quit (info->mainloop);
}

static GList *
get_contacts_for_name (const gchar *name)
{
	EBookQuery *book_query = NULL;
	GList *result;
	gchar *unquoted;
	GetContactsInfo *info;
	EBookQuery *queries[10];

	if (name == NULL)
		return NULL;

	unquoted = unquote_string (name);

	queries[0] = e_book_query_field_test (E_CONTACT_FULL_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[1] = e_book_query_field_test (E_CONTACT_GIVEN_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[2] = e_book_query_field_test (E_CONTACT_FAMILY_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[3] = e_book_query_field_test (E_CONTACT_NICKNAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[4] = e_book_query_field_test (E_CONTACT_EMAIL_1, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[5] = e_book_query_field_test (E_CONTACT_EMAIL_2, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[6] = e_book_query_field_test (E_CONTACT_EMAIL_3, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[7] = e_book_query_field_test (E_CONTACT_EMAIL_4, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[8] = e_book_query_field_test (E_CONTACT_EMAIL, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[9] = e_book_query_field_test (E_CONTACT_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	book_query = e_book_query_or (10, queries, TRUE);

	g_free (unquoted);

	/* TODO: Make it launch a mainloop */
	info = g_slice_new (GetContactsInfo);
	info->mainloop = g_main_loop_new (NULL, FALSE);
	info->result = NULL;
	if (e_book_async_get_contacts (book, book_query, get_contacts_for_name_cb, info) == 0) {
		GDK_THREADS_LEAVE ();
		g_main_loop_run (info->mainloop);
		GDK_THREADS_ENTER ();
	} 
	result = info->result;
	e_book_query_unref (book_query);
	g_main_loop_unref (info->mainloop);
	g_slice_free (GetContactsInfo, info);

	return result;
}


static GList *
select_contacts_for_name_dialog (const gchar *name)
{
	EBookQuery *book_query = NULL;
	EBookView *book_view = NULL;
	GList *result = NULL;
	gchar *unquoted;
	EBookQuery *queries[10];

	unquoted = unquote_string (name);

	queries[0] = e_book_query_field_test (E_CONTACT_FULL_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[1] = e_book_query_field_test (E_CONTACT_GIVEN_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[2] = e_book_query_field_test (E_CONTACT_FAMILY_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[3] = e_book_query_field_test (E_CONTACT_NICKNAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[4] = e_book_query_field_test (E_CONTACT_EMAIL_1, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[5] = e_book_query_field_test (E_CONTACT_EMAIL_2, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[6] = e_book_query_field_test (E_CONTACT_EMAIL_3, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[7] = e_book_query_field_test (E_CONTACT_EMAIL_4, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[8] = e_book_query_field_test (E_CONTACT_EMAIL, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	queries[9] = e_book_query_field_test (E_CONTACT_NAME, E_BOOK_QUERY_BEGINS_WITH, unquoted);
	book_query = e_book_query_or (10, queries, TRUE);
	e_book_get_book_view (book, book_query, NULL, -1, &book_view, NULL);
	e_book_query_unref (book_query);

	if (book_view) {
		GtkWidget *contact_dialog = NULL;
		osso_abook_list_store_set_book_view (OSSO_ABOOK_LIST_STORE (contact_model), book_view);
		e_book_view_start (book_view);

		/* TODO: figure out how to make the contact chooser modal */
		contact_dialog = osso_abook_contact_chooser_new_with_capabilities (NULL,
										   _AB("addr_ti_dia_select_contacts"),
										   OSSO_ABOOK_CAPS_EMAIL,
										   OSSO_ABOOK_CONTACT_ORDER_NAME);
		/* Enable multiselection */
		osso_abook_contact_chooser_set_maximum_selection (OSSO_ABOOK_CONTACT_CHOOSER (contact_dialog),
								  G_MAXUINT);
		osso_abook_contact_chooser_set_model (OSSO_ABOOK_CONTACT_CHOOSER (contact_dialog),
						      contact_model);

		if (gtk_dialog_run (GTK_DIALOG (contact_dialog)) == GTK_RESPONSE_OK)
			result = osso_abook_contact_chooser_get_selection (OSSO_ABOOK_CONTACT_CHOOSER (contact_dialog));
		e_book_view_stop (book_view);
		g_object_unref (book_view);
		gtk_widget_destroy (contact_dialog);
	}
	g_free (unquoted);

	return result;
}

static gboolean
resolve_address (const gchar *address, 
		 GSList **resolved_addresses, 
		 GSList **contact_ids,
		 gboolean *canceled)
{
	GList *resolved_contacts;
	CheckNamesInfo *info;;

	g_return_val_if_fail (canceled, FALSE);

	*resolved_addresses = NULL;
	*contact_ids = NULL;
	*canceled = FALSE;
	info = g_slice_new0 (CheckNamesInfo);
	show_check_names_banner (info);

	contact_model = osso_abook_contact_model_get_default ();
	if (!open_addressbook ()) {
		hide_check_names_banner (info);
		if (contact_model) {
			g_object_unref (contact_model);
			contact_model = NULL;
		}
		clean_check_names_banner (info);
		return FALSE;
	}

	resolved_contacts = get_contacts_for_name (address);
	hide_check_names_banner (info);

	if (resolved_contacts == NULL) {
		/* no matching contacts for the search string */
		modest_platform_run_information_dialog (NULL, _("mcen_nc_no_matching_contacts"), FALSE);
		clean_check_names_banner (info);
		return FALSE;
	}

	if (g_list_length (resolved_contacts) > 1) {
		/* show a dialog to select the contact from the resolved ones */
		g_list_free (resolved_contacts);

		resolved_contacts = select_contacts_for_name_dialog (address);
	}

	/* get the resolved contacts (can be no contact) */
	if (resolved_contacts) {
		GList *node;
		gboolean found = FALSE;

		for (node = resolved_contacts; node != NULL; node = g_list_next (node)) {
			EContact *contact = (EContact *) node->data;
			GSList *resolved;
			gchar *contact_id;

			resolved = get_recipients_for_given_contact (contact, canceled);
			if (resolved) {
				contact_id = g_strdup (e_contact_get_const (contact, E_CONTACT_UID));
				*contact_ids = g_slist_append (*contact_ids, contact_id);
				found = TRUE;
				*resolved_addresses = g_slist_append (*resolved_addresses, resolved);
			}
		}

		g_list_free (resolved_contacts);
		clean_check_names_banner (info);

		return found;
	} else {
		/* cancelled dialog to select more than one contact or
		 * selected no contact */
		clean_check_names_banner (info);
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
	GList *contacts = NULL;
	GError *err = NULL;
	gchar *email;
	gboolean result;
	OssoABookAggregator *roster;

	g_return_val_if_fail (address, FALSE);

	if (!book) {
		if (!open_addressbook ()) {
			g_return_val_if_reached (FALSE);
		}
	}
	g_return_val_if_fail (book, FALSE);

	email = modest_text_utils_get_email_address (address);

	roster = (OssoABookAggregator *) osso_abook_aggregator_get_default (NULL);
	contacts = osso_abook_aggregator_find_contacts_for_email_address (roster, email);
	if (!contacts) {
		if (err)
			g_error_free (err);
		g_free (email);
		return FALSE;
	}

	if (contacts) {
		g_list_free (contacts);
		result = TRUE;
	}

	g_free (email);

	return result;
}

const gchar *
modest_address_book_get_my_name ()
{
	OssoABookSelfContact *self_contact = osso_abook_self_contact_get_default ();

	/* We are not using osso_abook_contact_get_display_name
	   because that method fallbacks to another fields if the name
	   is not defined */
	if (self_contact)
		return e_contact_get ((EContact *) self_contact, E_CONTACT_FULL_NAME);
	else
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
	EBookQuery **queries, *composite_query;
	gint num_add, i;

	g_return_if_fail (address_list);

	if (!book)
		if (!open_addressbook ())
			g_return_if_reached ();

	/* Create the list of queries */
	num_add = g_slist_length (address_list);
	queries = g_malloc0 (sizeof (EBookQuery *) * num_add);
	for (i = 0; i < num_add; i++) {
		gchar *email;

		email = modest_text_utils_get_email_address (g_slist_nth_data (address_list, i));
		queries[i] = e_book_query_field_test (E_CONTACT_EMAIL, E_BOOK_QUERY_IS, email);
		g_free (email);
	}

	/* Create the query */
	composite_query = e_book_query_or (num_add, queries, TRUE);

	/* Asynchronously retrieve contacts */
	e_book_async_get_contacts (book, composite_query, async_get_contacts_cb, address_list);

	/* Frees. This will unref the subqueries as well */
	e_book_query_unref (composite_query);
}

static void
selector_on_response (GtkDialog *dialog,
		      gint       response_id,
		      gpointer   user_data)
{
	if (response_id == GTK_RESPONSE_OK) {
		gchar *current_selection = NULL;
		GtkTreePath *selected_row = NULL;
		HildonTouchSelector *selector;

		selector = hildon_picker_dialog_get_selector (HILDON_PICKER_DIALOG (dialog));
		selected_row = hildon_touch_selector_get_last_activated_row (selector, 0);
		if (selected_row) {
			GtkTreeIter iter;
			GtkTreeModel *model = hildon_touch_selector_get_model (selector, 0);
			if (gtk_tree_model_get_iter (model, &iter, selected_row)) {
				gtk_tree_model_get (model, &iter, 0, &current_selection, -1);
				modest_address_book_add_address (current_selection, user_data);
				g_debug ("Current selection : %s", current_selection);
				g_free (current_selection);
			}
		}
	}

	if (response_id != GTK_RESPONSE_DELETE_EVENT)
		gtk_widget_destroy ((GtkWidget *) dialog);
}

static void
selector_selection_changed (HildonTouchSelector * selector,
			    gint column,
			    gpointer *user_data)
{
	/* Close the dialog */
	gtk_dialog_response (GTK_DIALOG (user_data), GTK_RESPONSE_OK);
}

void
modest_address_book_add_address_list_with_selector (GSList *address_list, GtkWindow *parent)
{
	GtkWidget *picker_dialog;
	HildonTouchSelector *selector;
	GSList *node;
	GtkTreeModel *model;
	gboolean contacts_to_add = FALSE;

	/* We cannot use hildon_touch_selector_new_text() because
	   there is a bug in hildon that does not retrieve the current
	   selected text when using MODES_NORMAL. So we need a
	   temporary workaround here */
	selector = (HildonTouchSelector*) hildon_touch_selector_new ();

	model = (GtkTreeModel *) gtk_list_store_new (1,  G_TYPE_STRING);
	hildon_touch_selector_append_text_column (selector, model, TRUE);
	hildon_touch_selector_set_hildon_ui_mode (selector, HILDON_UI_MODE_NORMAL);
	g_object_unref (model);

	for (node = address_list; node != NULL; node = g_slist_next (node)) {
		const gchar *recipient = (const gchar *) node->data;
		if (modest_text_utils_validate_recipient (recipient, NULL)) {
			if (!modest_address_book_has_address (recipient)) {
				GtkTreeIter iter;
				gtk_list_store_append ((GtkListStore *) model, &iter);
				gtk_list_store_set ((GtkListStore *) model, &iter, 0, recipient, -1);
				contacts_to_add = TRUE;
			}
		}
	}

	if (contacts_to_add) {
		picker_dialog = hildon_picker_dialog_new (parent);
		gtk_window_set_title (GTK_WINDOW (picker_dialog), _("mcen_me_viewer_addtocontacts"));

		hildon_picker_dialog_set_selector (HILDON_PICKER_DIALOG (picker_dialog),
						   selector);

		g_signal_connect ((GObject*) selector, "changed",
				  G_CALLBACK (selector_selection_changed), picker_dialog);

		g_signal_connect ((GObject*) picker_dialog, "response",
				  G_CALLBACK (selector_on_response), parent);

		gtk_widget_show (picker_dialog);
	} else {
		gtk_widget_destroy ((GtkWidget *) selector);
	}
}

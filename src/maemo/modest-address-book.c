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

#include <modest-address-book.h>
#include <libebook/e-book.h>
#include <libebook/e-book-view.h>
#include <libosso-abook/osso-abook.h>

static OssoABookContactModel *contact_model =  NULL;
static EBook *book = NULL;
static EBookView * book_view = NULL;


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

gchar *
modest_address_book_select_addresses (void)
{
	g_message (__FUNCTION__);
}


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

/* modest-address-book.h */

#ifndef __MODEST_ADDRESS_BOOK_H__
#define __MODEST_ADDRESS_BOOK_H__

#include <gtk/gtk.h>
#include <glib.h>
#include <widgets/modest-recpt-editor.h>

/**
 * modest_address_book_init:
 *
 * initializes the addressbook
 */
void
modest_address_book_init (void);

/**
 * modest_address_book_add_address:
 * @address: a string
 *
 * launches the UI for adding @address to the addressbook
 */
void
modest_address_book_add_address (const gchar *address,
				 GtkWindow *parent);

/**
 * modest_address_book_select_addresses:
 * 
 * Shows a dialog to select some addresses from the 
 * address book. It adds them to the recipient editor
 *
 */
void
modest_address_book_select_addresses (ModestRecptEditor *editor,
				      GtkWindow *parent_window);

/**
 * modest_address_book_check_names:
 * @editor: a #ModestRecptEditor
 * @address_list: if it is not NULL, this list is filled with the
 * valid addresses ready to be added to the address book
 *
 * Performs verification of addresses in a recipient editor.
 *
 * Returns: %TRUE if all recipients are valid or there are
 * no recipients, %FALSE otherwise.
 */
gboolean
modest_address_book_check_names (ModestRecptEditor *editor,
				 GSList **address_list);

/**
 * modest_address_book_has_address:
 * @address: a string
 *
 * Checks if an address is already stored in addressbook.
 *
 * Returns: %TRUE is @address is in addressbook. %FALSE otherwise.
 */
gboolean
modest_address_book_has_address (const gchar *address);

/**
 * modest_address_book_get_my_name:
 * @:
 *
 * Returns user name from user's own vcard
 *
 * Returns: the user full name
 **/
const gchar *
modest_address_book_get_my_name ();

/**
 * modest_address_book_add_address_list:
 * @address_list: a list of email addresses to add to the addressbook
 *
 * Inserts a list of addresses in the addressbook
 **/
void
modest_address_book_add_address_list (GSList *address_list);

/**
 * modest_address_book_add_address_list_with_selector:
 * @address_list: a list of email addresses to add to the addressbook
 * @parent: the parent window. The UI elements shown to the user
 * (tipically a dialog) will have that window as parent
 *
 * Presents some UI to the users to allow them to select and then
 * insert a list of selected addresses in the addressbook.
 **/
void
modest_address_book_add_address_list_with_selector (GSList *address_list,
						    GtkWindow *parent);

#endif /* __MODEST_ADDRESS_BOOK_H__ */

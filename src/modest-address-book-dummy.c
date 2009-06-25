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

/*
 * modest-address-book-dummy's only purpose in life is the make sure we
 * can build modest without any addressbook support
 */

#include "modest-address-book.h"

void modest_address_book_add_address (const gchar *address)
{
	g_debug ("trying to add '%s' to non-existing address book",
		 address);
}


void modest_address_book_select_addresses (ModestRecptEditor *editor,
					   GtkWindow *parent_window)
{
	return;
}

gboolean modest_address_book_check_names (ModestRecptEditor *editor,
					  GSList **address_list)
{
	/* let's be optimistic */
	return TRUE;
}

gboolean modest_address_book_has_address (const gchar *address)
{
	/* let's be optimistic */
	return TRUE;
}

const gchar *
modest_address_book_get_my_name ()
{
	return NULL;
}

void
modest_address_book_init (void)
{
	/* Do nothing */
}

void
modest_address_book_add_address_list (GSList *address_list)
{
	/* Do nothing */
}

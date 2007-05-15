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
#include <glib/gi18n.h>
#include <string.h>
#include <modest-tny-folder.h>
#include <tny-camel-folder.h>
#include <camel/camel-folder.h>
#include <modest-protocol-info.h>

TnyFolderType
modest_tny_folder_guess_folder_type_from_name (const gchar* name)
{
	gint  type;
	gchar *folder;

	g_return_val_if_fail (name, TNY_FOLDER_TYPE_UNKNOWN);
	
	type = TNY_FOLDER_TYPE_UNKNOWN;
	folder = g_utf8_strdown (name, strlen(name));

	if (strcmp (folder, "inbox") == 0 ||
	    strcmp (folder, _("inbox")) == 0 ||
	    strcmp (folder, _("mcen_me_folder_inbox")) == 0)
		type = TNY_FOLDER_TYPE_INBOX;
	else if (strcmp (folder, "outbox") == 0 ||
		 strcmp (folder, _("outbox")) == 0 ||
		 strcmp (folder, _("mcen_me_folder_outbox")) == 0)
		type = TNY_FOLDER_TYPE_OUTBOX;
	else if (g_str_has_prefix(folder, "junk") ||
		 g_str_has_prefix(folder, _("junk")))
		type = TNY_FOLDER_TYPE_JUNK;
	else if (g_str_has_prefix(folder, "trash") ||
		 g_str_has_prefix(folder, _("trash")))
		type = TNY_FOLDER_TYPE_TRASH;
	else if (g_str_has_prefix(folder, "sent") ||
		 g_str_has_prefix(folder, _("sent")) ||
		 strcmp (folder, _("mcen_me_folder_sent")) == 0)
		type = TNY_FOLDER_TYPE_SENT;
	else if (g_str_has_prefix(folder, "draft") ||
		 g_str_has_prefix(folder, _("draft")) ||
		 strcmp (folder, _("mcen_me_folder_drafts")) == 0)
		type = TNY_FOLDER_TYPE_DRAFTS;
	else if (g_str_has_prefix(folder, "notes") ||
		 g_str_has_prefix(folder, _("notes")))
		type = TNY_FOLDER_TYPE_NOTES;
	else if (g_str_has_prefix(folder, "contacts") ||
		 g_str_has_prefix(folder, _("contacts")))
		type = TNY_FOLDER_TYPE_CONTACTS;
	else if (g_str_has_prefix(folder, "calendar") ||
		 g_str_has_prefix(folder, _("calendar")))
		type = TNY_FOLDER_TYPE_CALENDAR;
	
	g_free (folder);
	return type;
}



TnyFolderType
modest_tny_folder_guess_folder_type (const TnyFolder *folder)
{
	TnyFolderType type;

	g_return_val_if_fail (folder, TNY_FOLDER_TYPE_UNKNOWN);

	type = tny_folder_get_folder_type (TNY_FOLDER (folder));
	
	if (type == TNY_FOLDER_TYPE_UNKNOWN) {
		const gchar *folder_name;

		folder_name = tny_folder_get_name (TNY_FOLDER (folder));
		type =	modest_tny_folder_guess_folder_type_from_name (folder_name);
	}

	return type;
}


/* FIXME: encode all folder rules here */
ModestTnyFolderRules
modest_tny_folder_get_rules   (const TnyFolder *folder)
{
	ModestTnyFolderRules rules = 0;
	TnyFolderType type;

	g_return_val_if_fail (TNY_IS_FOLDER(folder), -1);

	if (modest_tny_folder_is_local_folder (folder)) {
	
		type = modest_tny_folder_get_local_folder_type (folder);
		
		switch (type) {
		case TNY_FOLDER_TYPE_DRAFTS:
		case TNY_FOLDER_TYPE_OUTBOX:
		case TNY_FOLDER_TYPE_SENT:
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;
		case TNY_FOLDER_TYPE_INBOX:
		case TNY_FOLDER_TYPE_JUNK:
		case TNY_FOLDER_TYPE_TRASH:
		default:
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE;
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE;
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE;
		}
	} else {
		ModestTransportStoreProtocol proto;
		TnyAccount *account =
			tny_folder_get_account ((TnyFolder*)folder);
		if (!account)
			return -1; /* no account: nothing is allowed */
		
		proto = modest_protocol_info_get_transport_store_protocol (tny_account_get_proto (account));

		if (proto == MODEST_PROTOCOL_STORE_IMAP) {
			rules = 0;
		} else {
			/* pop, nntp, ... */
			rules =
				MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE |
				MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE |
				MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE  |
				MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE;

		}
		g_object_unref (G_OBJECT(account));
	}
	return rules;
}


gboolean
modest_tny_folder_is_local_folder   (const TnyFolder *folder)
{
	TnyAccount*  account;
	const gchar* account_id;
	
	g_return_val_if_fail (folder, FALSE);
	
	account = tny_folder_get_account ((TnyFolder*)folder);
	if (!account)
		return FALSE;

	account_id = tny_account_get_id (account);
	if (!account_id)
		return FALSE;

	g_object_unref (G_OBJECT(account));
	
	return (strcmp (account_id, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) == 0);
}	


TnyFolderType
modest_tny_folder_get_local_folder_type  (const TnyFolder *folder)
{
	CamelFolder *camel_folder;
	const gchar *full_name;
	
	g_return_val_if_fail (folder, TNY_FOLDER_TYPE_UNKNOWN);
	g_return_val_if_fail (modest_tny_folder_is_local_folder(folder),
			      TNY_FOLDER_TYPE_UNKNOWN);

	/* we need to use the camel functions, because we want the
	 * _full name_, that is, the full path name of the folder,
	 * to distinguis between 'Outbox' and 'myfunkyfolder/Outbox'
	 */
	camel_folder = tny_camel_folder_get_folder (TNY_CAMEL_FOLDER(folder));
	if (!camel_folder)
		return TNY_FOLDER_TYPE_UNKNOWN;

	full_name = camel_folder_get_full_name (camel_folder);
	camel_object_unref (CAMEL_OBJECT(camel_folder));
	
	if (!full_name) 
		return TNY_FOLDER_TYPE_UNKNOWN;
	else 
		return modest_local_folder_info_get_type (full_name);
}

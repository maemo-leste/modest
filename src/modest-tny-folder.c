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
#include <modest-tny-outbox-account.h>
#include <tny-simple-list.h>
#include <tny-camel-folder.h>
#include <tny-merge-folder.h>
#include <modest-protocol-info.h>
#include <modest-runtime.h>
#include <modest-tny-account-store.h>

TnyFolderType
modest_tny_folder_guess_folder_type_from_name (const gchar* name)
{
	gint  type;
	gchar *folder;
	gchar *dc_outbox = NULL;
	gchar *dc_sent = NULL;
	gchar *dc_drafts = NULL;

	g_return_val_if_fail (name, TNY_FOLDER_TYPE_UNKNOWN);
	
	type = TNY_FOLDER_TYPE_UNKNOWN;
	folder = g_utf8_strdown (name, strlen(name));
	dc_outbox = g_utf8_strdown (_("mcen_me_folder_outbox"), -1);
	dc_sent = g_utf8_strdown (_("mcen_me_folder_sent"), -1);
	dc_drafts = g_utf8_strdown (_("mcen_me_folder_drafts"), -1);

//	if (strcmp (folder, "inbox") == 0 ||
//	    strcmp (folder, _("inbox")) == 0 ||
//	    strcmp (folder, _("mcen_me_folder_inbox")) == 0)
//		type = TNY_FOLDER_TYPE_INBOX;
	if (strcmp (folder, dc_outbox) == 0)
		type = TNY_FOLDER_TYPE_OUTBOX;
//	else if (g_str_has_prefix(folder, "junk") ||
//		 g_str_has_prefix(folder, _("junk")))
//		type = TNY_FOLDER_TYPE_JUNK;
//	else if (g_str_has_prefix(folder, "trash") ||
//		 g_str_has_prefix(folder, _("trash")))
//		type = TNY_FOLDER_TYPE_TRASH;
	else if (strcmp (folder, dc_sent) == 0)
		type = TNY_FOLDER_TYPE_SENT;
	else if (strcmp (folder, dc_drafts) == 0)
		type = TNY_FOLDER_TYPE_DRAFTS;
//	else if (g_str_has_prefix(folder, "notes") ||
//		 g_str_has_prefix(folder, _("notes")))
//		type = TNY_FOLDER_TYPE_NOTES;
//	else if (g_str_has_prefix(folder, "contacts") ||
//		 g_str_has_prefix(folder, _("contacts")))
//		type = TNY_FOLDER_TYPE_CONTACTS;
//	else if (g_str_has_prefix(folder, "calendar") ||
//		 g_str_has_prefix(folder, _("calendar")))
//		type = TNY_FOLDER_TYPE_CALENDAR;
	
	g_free (folder);
	g_free (dc_outbox);
	g_free (dc_sent);
	g_free (dc_drafts);
	return type;
}



TnyFolderType
modest_tny_folder_guess_folder_type (const TnyFolder *folder)
{
	TnyFolderType type;
	
	g_return_val_if_fail (TNY_IS_FOLDER(folder), TNY_FOLDER_TYPE_UNKNOWN);

	if (modest_tny_folder_is_local_folder ((TnyFolder*)folder))
		type = modest_tny_folder_get_local_or_mmc_folder_type ((TnyFolder*)folder);
	else
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
modest_tny_folder_get_rules   (TnyFolder *folder)
{
	ModestTnyFolderRules rules = 0;
	TnyFolderType type;

	g_return_val_if_fail (TNY_IS_FOLDER(folder), -1);

	if (modest_tny_folder_is_local_folder (folder) ||
	    modest_tny_folder_is_memory_card_folder (folder)) {
	
		type = modest_tny_folder_get_local_or_mmc_folder_type (folder);
		
		switch (type) {
		case TNY_FOLDER_TYPE_OUTBOX:
		case TNY_FOLDER_TYPE_SENT:
		case TNY_FOLDER_TYPE_DRAFTS:
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_WRITEABLE;
		case TNY_FOLDER_TYPE_INBOX:
		case TNY_FOLDER_TYPE_JUNK:
		case TNY_FOLDER_TYPE_TRASH:
		case TNY_FOLDER_TYPE_ROOT:
		case TNY_FOLDER_TYPE_NOTES:
		case TNY_FOLDER_TYPE_CONTACTS:
		case TNY_FOLDER_TYPE_CALENDAR:
		case TNY_FOLDER_TYPE_ARCHIVE:
		case TNY_FOLDER_TYPE_MERGE:
		case TNY_FOLDER_TYPE_NUM:
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE;
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE;
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE;
		default:
			break;
		}
	} else {
		ModestTransportStoreProtocol proto;
		TnyFolderType folder_type;
		TnyAccount *account;

		account = modest_tny_folder_get_account ((TnyFolder*)folder);
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

		/* Neither INBOX nor ROOT, nor ARCHIVE folders should me moveable */
		folder_type = modest_tny_folder_guess_folder_type (folder);
		if ((folder_type ==  TNY_FOLDER_TYPE_INBOX) ||
		    (folder_type == TNY_FOLDER_TYPE_ROOT) ||
		    (folder_type == TNY_FOLDER_TYPE_ARCHIVE)) {
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_DELETABLE;
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_MOVEABLE;
			rules |= MODEST_FOLDER_RULES_FOLDER_NON_RENAMEABLE;
		}
	}
	return rules;
}

	

gboolean
modest_tny_folder_is_local_folder   (TnyFolder *folder)
{
	g_return_val_if_fail (TNY_IS_FOLDER (folder), FALSE);
	
	/* The merge folder is a special case, 
	 * used to merge the per-account local outbox folders. 
	 * and can have no get_account() implementation.
	 * We should do something more sophisticated if we 
	 * ever use TnyMergeFolder for anything else.
	 */
	if (TNY_IS_MERGE_FOLDER (folder)) {
		return TRUE;
	}
	
	TnyAccount* account = tny_folder_get_account ((TnyFolder*)folder);
	if (!account) {
		g_warning ("folder without account");
		return FALSE;
	}
	
	/* Outbox is a special case, using a derived TnyAccount: */
	if (MODEST_IS_TNY_OUTBOX_ACCOUNT (account)) {
		//g_warning ("BUG: should not be reached");
		/* should be handled with the MERGE_FOLDER above*/
		g_object_unref (G_OBJECT(account));
		return TRUE;
	}

	const gchar* account_id = tny_account_get_id (account);
	if (!account_id) {
		g_warning ("BUG: account without account id");
		g_object_unref (G_OBJECT(account));
		return FALSE;
	}
	
	g_object_unref (G_OBJECT(account));
	return (strcmp (account_id, MODEST_LOCAL_FOLDERS_ACCOUNT_ID) == 0);
}

gboolean
modest_tny_folder_is_memory_card_folder   (TnyFolder *folder)
{
	g_return_val_if_fail (folder, FALSE);
	
	/* The merge folder is a special case, 
	 * used to merge the per-account local outbox folders. 
	 * and can have no get_account() implementation.
	 */
	if (TNY_IS_MERGE_FOLDER (folder))
		return FALSE;

	TnyAccount* account = tny_folder_get_account ((TnyFolder*)folder);
	if (!account)
		return FALSE;

	const gchar* account_id = tny_account_get_id (account);
	if (!account_id)
		return FALSE;

	g_object_unref (G_OBJECT(account));
	
	return (strcmp (account_id, MODEST_MMC_ACCOUNT_ID) == 0);
}	

gboolean
modest_tny_folder_is_remote_folder   (TnyFolder *folder)
{
	gboolean is_local = TRUE;

	g_return_val_if_fail (folder, FALSE);
	
	is_local = ((modest_tny_folder_is_local_folder(folder)) ||
		    (modest_tny_folder_is_memory_card_folder(folder)));


	return !is_local;
}

TnyFolderType
modest_tny_folder_get_local_or_mmc_folder_type  (TnyFolder *folder)
{
	g_return_val_if_fail (folder, TNY_FOLDER_TYPE_UNKNOWN);
	g_return_val_if_fail (modest_tny_folder_is_local_folder(folder)||
			      modest_tny_folder_is_memory_card_folder(folder),
			      TNY_FOLDER_TYPE_UNKNOWN);
	
	/* The merge folder is a special case, 
	 * used to merge the per-account local outbox folders. 
	 * and can have no get_account() implementation.
	 * We should do something more sophisticated if we 
	 * ever use TnyMergeFolder for anything else.
	 */
	if (TNY_IS_MERGE_FOLDER (folder))
		return TNY_FOLDER_TYPE_OUTBOX;
		
	/* Outbox is a special case, using a derived TnyAccount: */
	TnyAccount* parent_account = tny_folder_get_account (folder);
	if (parent_account && MODEST_IS_TNY_OUTBOX_ACCOUNT (parent_account)) {
		g_object_unref (parent_account);
		return TNY_FOLDER_TYPE_OUTBOX;  
	}

	if (parent_account) {
		g_object_unref (parent_account);
		parent_account = NULL;
	}
	
	/* we need to use the camel functions, because we want the
	 * _full name_, that is, the full path name of the folder,
	 * to distinguish between 'Outbox' and 'myfunkyfolder/Outbox'
	 */

	const gchar *full_name = tny_camel_folder_get_full_name (TNY_CAMEL_FOLDER (folder));
	/* printf ("DEBUG: %s: full_name=%s\n", __FUNCTION__, full_name); */
	
	if (!full_name) 
		return TNY_FOLDER_TYPE_UNKNOWN;
	else 
		return modest_local_folder_info_get_type (full_name);
}

gboolean
modest_tny_folder_is_outbox_for_account (TnyFolder *folder, TnyAccount *account)
{
	g_return_val_if_fail(folder, FALSE);
	g_return_val_if_fail(account, FALSE);
	
	if (modest_tny_folder_get_local_or_mmc_folder_type (folder) != TNY_FOLDER_TYPE_OUTBOX)
		return FALSE;
		
	return TRUE;
}

gchar* 
modest_tny_folder_get_header_unique_id (TnyHeader *header)
{
	TnyFolder *folder;
	gchar *url, *retval;
	const gchar *uid;

	g_return_val_if_fail (TNY_IS_HEADER (header), NULL);

	folder = tny_header_get_folder (header);
	if (!folder)
		return NULL;

	url = tny_folder_get_url_string (folder);
	uid = tny_header_get_uid (header);

	retval = g_strjoin ("/", url, uid, NULL);

	g_free (url);
	g_object_unref (folder);

	return retval;
}

TnyAccount *
modest_tny_folder_get_account (TnyFolder *folder)
{
	TnyAccount *account = NULL;
	
	if (TNY_IS_MERGE_FOLDER (folder)) {
		/* TnyMergeFolder does not support get_account(), 
		 * because it could be merging folders from multiple accounts.
		 * So we assume that this is the local folders account: */
		 
		account = modest_tny_account_store_get_local_folders_account (modest_runtime_get_account_store());
	} else {
		account = tny_folder_get_account (folder);
	}
	
	return account;
}

/*
 * It's probably better to use a query to get the folders that match
 * new_name but currently tinymail only provides a match by name using
 * regular expressions and we want an exact matching. We're not using
 * a regular expression for the exact name because we'd need first to
 * escape @new_name and it's not easy sometimes. 
 *
 * The code that uses the query is available in revision 3152.
 */
gboolean 
modest_tny_folder_has_subfolder_with_name (TnyFolderStore *parent,
					   const gchar *new_name)
{
	TnyList *subfolders = NULL;
	TnyIterator *iter = NULL;
	TnyFolder *folder = NULL;
	GError *err = NULL;
	gboolean same_subfolder = FALSE;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), FALSE);

	/* Get direct subfolders */
	subfolders = tny_simple_list_new ();
	tny_folder_store_get_folders (parent, subfolders, NULL, &err);

	/* Check names */
	iter = tny_list_create_iterator (subfolders);
	while (!tny_iterator_is_done (iter) && !same_subfolder) {
		const gchar *name = NULL;

		folder = TNY_FOLDER(tny_iterator_get_current (iter));
		name = tny_folder_get_name (folder);
		
		same_subfolder = !strcmp(name, new_name);

		g_object_unref (folder);
		tny_iterator_next(iter);
	}
	
	/* free */
	if (iter != NULL)
		g_object_unref (iter);
	if (subfolders != NULL)
		g_object_unref (subfolders);
		
	return same_subfolder;
}

gboolean 
modest_tny_folder_is_ancestor (TnyFolder *folder,
			       TnyFolderStore *ancestor)
{
	TnyFolderStore *tmp = NULL;
	gboolean found = FALSE;

	tmp = TNY_FOLDER_STORE (folder);
	while (!found && tmp && !TNY_IS_ACCOUNT (tmp)) {
		TnyFolderStore *folder_store;

		folder_store = tny_folder_get_folder_store (TNY_FOLDER (tmp));
		if (ancestor == folder_store)
			found = TRUE;
		else
			tmp = folder_store;
		g_object_unref (folder_store);
	}
	return found;
}

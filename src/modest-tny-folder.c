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
#include <modest-defs.h>
#include <modest-tny-folder.h>
#include <modest-tny-account.h>
#include <modest-tny-outbox-account.h>
#include <tny-simple-list.h>
#include <tny-camel-folder.h>
#include <tny-merge-folder.h>
#include <modest-runtime.h>
#include <modest-tny-account-store.h>
#include <modest-text-utils.h>


/* make sure you use the *full* name, because foo/drafts is not the same as drafts */
static TnyFolderType
modest_tny_folder_guess_folder_type_from_name (const gchar* full_name)
{
	g_return_val_if_fail (full_name, TNY_FOLDER_TYPE_INVALID);
	
	if (strcmp (full_name, modest_local_folder_info_get_type_name(TNY_FOLDER_TYPE_OUTBOX)) == 0)
		return TNY_FOLDER_TYPE_OUTBOX;
	else if (strcmp (full_name, modest_local_folder_info_get_type_name(TNY_FOLDER_TYPE_DRAFTS)) == 0)
		return TNY_FOLDER_TYPE_DRAFTS;
	return
		TNY_FOLDER_TYPE_NORMAL;
}



TnyFolderType
modest_tny_folder_guess_folder_type (TnyFolder *folder)
{
	TnyFolderType type;
	
	g_return_val_if_fail (TNY_IS_FOLDER(folder), TNY_FOLDER_TYPE_INVALID);

	if (modest_tny_folder_is_local_folder (folder) || 
	    modest_tny_folder_is_memory_card_folder (folder))
		type = modest_tny_folder_get_local_or_mmc_folder_type (folder);
	else
		type = tny_folder_get_folder_type (TNY_FOLDER (folder));

	/* Fallback code, some servers (Dovecot in some versions)
	   report incorrectly that the INBOX folder is a normal
	   folder. Really ugly code but... */
	if (type == TNY_FOLDER_TYPE_NORMAL) {
		TnyFolderStore *parent = tny_folder_get_folder_store (folder);
		if (parent) {
			if (TNY_IS_ACCOUNT (parent)) {
				gchar *downcase = 
					g_ascii_strdown (tny_camel_folder_get_full_name (TNY_CAMEL_FOLDER (folder)), 
							 -1);

				if ((strlen (downcase) == 5) &&
				    !strncmp (downcase, "inbox", 5))
					type = TNY_FOLDER_TYPE_INBOX;
				if ((strlen (downcase) == 7) &&
				    !strncmp (downcase, "archive", 7))
					type = TNY_FOLDER_TYPE_ARCHIVE;
				g_free (downcase);
			}
			g_object_unref (parent);
		}
	}
	
	if (type == TNY_FOLDER_TYPE_UNKNOWN) {
		const gchar *folder_name =
			tny_camel_folder_get_full_name (TNY_CAMEL_FOLDER (folder));
		type =	modest_tny_folder_guess_folder_type_from_name (folder_name);
	}

	if (type == TNY_FOLDER_TYPE_INVALID)
		g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);

	return type;
}


const gchar*
modest_tny_folder_get_help_id (TnyFolder *folder)
{
	TnyFolderType type;
	const gchar* help_id = NULL;
	
	g_return_val_if_fail (folder, NULL);
	g_return_val_if_fail (TNY_IS_FOLDER(folder), NULL);
	
	type = modest_tny_folder_guess_folder_type (TNY_FOLDER (folder));
	
	switch (type) {
	case TNY_FOLDER_TYPE_NORMAL:  help_id = "applications_email_managefolders"; break;
	case TNY_FOLDER_TYPE_INBOX:   help_id = "applications_email_inbox";break;
	case TNY_FOLDER_TYPE_OUTBOX:  help_id = "applications_email_outbox";break;
	case TNY_FOLDER_TYPE_SENT:    help_id = "applications_email_sent"; break;
	case TNY_FOLDER_TYPE_DRAFTS:  help_id = "applications_email_drafts";break;
	case TNY_FOLDER_TYPE_ARCHIVE: help_id = "applications_email_managefolders";break;

	case TNY_FOLDER_TYPE_INVALID: g_warning ("%s: BUG: TNY_FOLDER_TYPE_INVALID", __FUNCTION__);break;
	default: 	              g_warning ("%s: BUG: unexpected folder type (%d)", __FUNCTION__, type);
	}
	
	return help_id;
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
		g_return_val_if_fail (type != TNY_FOLDER_TYPE_INVALID, -1);
		
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
		ModestProtocolRegistry *protocol_registry;
		ModestProtocolType protocol_type;
		TnyFolderType folder_type;
		TnyAccount *account;

		protocol_registry = modest_runtime_get_protocol_registry ();

		account = modest_tny_folder_get_account ((TnyFolder*)folder);
		if (!account)
			return -1; /* no account: nothing is allowed */
		
		protocol_type = modest_tny_account_get_protocol_type (account);

		if (modest_protocol_registry_protocol_type_has_tag (protocol_registry, protocol_type, MODEST_PROTOCOL_REGISTRY_STORE_HAS_FOLDERS)) {
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
		g_return_val_if_fail (folder_type != TNY_FOLDER_TYPE_INVALID, -1);
		
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
	if (!account_id) {
		g_object_unref (account);
		return FALSE;
	}

	g_object_unref (account);

	return (strcmp (account_id, MODEST_MMC_ACCOUNT_ID) == 0);
}

gboolean
modest_tny_folder_is_remote_folder   (TnyFolder *folder)
{
	gboolean is_local = TRUE;

	g_return_val_if_fail (TNY_IS_FOLDER(folder), FALSE);
	
	is_local = ((modest_tny_folder_is_local_folder(folder)) ||
		    (modest_tny_folder_is_memory_card_folder(folder)));


	return !is_local;
}


TnyFolderType
modest_tny_folder_get_local_or_mmc_folder_type  (TnyFolder *folder)
{
	g_return_val_if_fail (folder, TNY_FOLDER_TYPE_INVALID);
	g_return_val_if_fail (modest_tny_folder_is_local_folder(folder)||
			      modest_tny_folder_is_memory_card_folder(folder),
			      TNY_FOLDER_TYPE_INVALID);
	
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
	gchar *uid;

	g_return_val_if_fail (TNY_IS_HEADER (header), NULL);

	folder = tny_header_get_folder (header);
	if (!folder)
		return NULL;

	url = tny_folder_get_url_string (folder);
	uid = tny_header_dup_uid (header);

	retval = g_strjoin ("/", url, uid, NULL);
	g_free (uid);

	g_free (url);
	g_object_unref (folder);

	return retval;
}

TnyAccount *
modest_tny_folder_get_account (TnyFolder *folder)
{
	TnyAccount *account = NULL;

	g_return_val_if_fail (TNY_IS_FOLDER(folder), NULL);
	
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
					   const gchar *new_name,
					   gboolean non_strict)
{
	TnyList *subfolders = NULL;
	TnyIterator *iter = NULL;
	TnyFolder *folder = NULL;
	GError *err = NULL;
	gboolean has_name = FALSE;

	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), FALSE);
	g_return_val_if_fail (new_name, FALSE);

	/* Get direct subfolders */
	subfolders = tny_simple_list_new ();
	tny_folder_store_get_folders (parent, subfolders, NULL, FALSE, &err);

	/* Check names */
	iter = tny_list_create_iterator (subfolders);
	while (!tny_iterator_is_done (iter) && !has_name) {

		const gchar *name;

		folder = (TnyFolder*)tny_iterator_get_current (iter);
		if (!folder || ! TNY_IS_FOLDER(folder)) {
			g_warning ("%s: invalid folder", __FUNCTION__);
			tny_iterator_next (iter);
			continue;
		}

		name = tny_folder_get_name (folder);
		if (!name) {
			g_warning ("%s: folder name == NULL", __FUNCTION__);
			g_object_unref (folder);
			tny_iterator_next (iter);
			continue;
		}

		/* is it simply the same folder name? */
		if (strcmp (name, new_name) == 0)
			has_name = TRUE;
		/* or is it the same when ignoring case (non-strict mode)? */
		else if (non_strict && modest_text_utils_utf8_strcmp (name, new_name, TRUE) == 0)
			has_name = TRUE;
		/* or is the name equal to the display name of some folder, in the current locale? */
		else if (non_strict) {
			TnyFolderType type = modest_tny_folder_guess_folder_type (folder);
			if (type != TNY_FOLDER_TYPE_INVALID && type != TNY_FOLDER_TYPE_NORMAL) 
				has_name = !(modest_text_utils_utf8_strcmp (modest_local_folder_info_get_type_display_name (type),
									    new_name,
									    TRUE));
		} else {
			has_name = FALSE;
		}

		g_object_unref (folder);
		tny_iterator_next(iter);
	}

	/* free */
	g_object_unref (iter);
	g_object_unref (subfolders);

	return has_name;
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

gchar * 
modest_tny_folder_get_display_name (TnyFolder *folder)
{
	TnyFolderType type = TNY_FOLDER_TYPE_UNKNOWN;
	gchar *fname;

	g_return_val_if_fail (TNY_IS_FOLDER (folder), NULL);

	fname = g_strdup (tny_folder_get_name (folder));
	type = tny_folder_get_folder_type (folder);

	if (modest_tny_folder_is_local_folder (TNY_FOLDER (folder)) ||
	    modest_tny_folder_is_memory_card_folder (TNY_FOLDER (folder))) {
		type = modest_tny_folder_get_local_or_mmc_folder_type (TNY_FOLDER (folder));
		if (type != TNY_FOLDER_TYPE_UNKNOWN) {
			g_free (fname);
				fname = g_strdup (modest_local_folder_info_get_type_display_name (type));
		}
	} else {
		/* Sometimes an special folder is reported by the server as
		   NORMAL, like some versions of Dovecot */
		if (type == TNY_FOLDER_TYPE_NORMAL ||
		    type == TNY_FOLDER_TYPE_UNKNOWN) {
			type = modest_tny_folder_guess_folder_type (TNY_FOLDER (folder));
		}
	}

	if (type == TNY_FOLDER_TYPE_INBOX) {
		g_free (fname);
		fname = g_strdup (_("mcen_me_folder_inbox"));
	}

	return fname;
}

TnyFolder *
modest_tny_folder_store_find_folder_from_uri (TnyFolderStore *folder_store, const gchar *uri)
{
	TnyList *children;
	TnyIterator *iterator;
	TnyFolder *result;
	gchar *uri_to_find, *slash;
	gint uri_lenght;

	if (uri == NULL)
		return NULL;

	slash = strrchr (uri, '/');
	if (slash == NULL)
		return NULL;

	result = NULL;
	children = TNY_LIST (tny_simple_list_new ());
	tny_folder_store_get_folders (folder_store, children, NULL, FALSE, NULL);

	uri_lenght = slash - uri + 1;
	uri_to_find = g_malloc0 (sizeof(char) * uri_lenght);
	strncpy (uri_to_find, uri, uri_lenght);
	uri_to_find[uri_lenght - 1] = '\0';

	for (iterator = tny_list_create_iterator (children);
	     iterator && !tny_iterator_is_done (iterator) && (result == NULL);
	     tny_iterator_next (iterator)) {
		TnyFolderStore *child;

		child = TNY_FOLDER_STORE (tny_iterator_get_current (iterator));
		if (!child) 
			continue;

		if (TNY_IS_FOLDER (child)) {
			gchar *folder_url;

			folder_url = tny_folder_get_url_string (TNY_FOLDER (child));
			if (uri_to_find && folder_url && !strcmp (folder_url, uri_to_find))
				result = TNY_FOLDER(g_object_ref (child));
			g_free (folder_url);
		}

		if ((result == NULL) && TNY_IS_FOLDER_STORE (child)) {
			result = modest_tny_folder_store_find_folder_from_uri (child, uri);
		}

		g_object_unref (child);
	}

	g_free (uri_to_find);
	g_object_unref (iterator);
	g_object_unref (children);

	return result;
}

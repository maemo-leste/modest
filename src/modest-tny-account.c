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

#include <modest-tny-platform-factory.h>
#include <modest-tny-account.h>
#include <modest-tny-account-store.h>
#include <modest-runtime.h>
#include <tny-simple-list.h>
#include <modest-tny-folder.h>


/* for now, ignore the account ===> the special folders are the same,
 * local folders for all accounts
 * this might change, ie, IMAP might have server-side sent-items
 */
TnyFolder *
modest_tny_account_get_special_folder (TnyAccount *account,
				       TnyFolderType special_type)
{
	TnyList *folders;
	TnyIterator *iter;
	TnyFolder *special_folder = NULL;
	TnyAccount *local_account;
	
	g_return_val_if_fail (account, NULL);
	g_return_val_if_fail (0 <= special_type && special_type < TNY_FOLDER_TYPE_NUM,
			      NULL);

	local_account = modest_tny_account_store_get_local_folders_account
		(modest_runtime_get_account_store());
	if (!local_account) {
		g_printerr ("modest: cannot get local account\n");
		return NULL;
	}

	folders = TNY_LIST (tny_simple_list_new ());

	/* no need to do this _async, as these are local folders */
	tny_folder_store_get_folders (TNY_FOLDER_STORE (local_account),
				      folders, NULL, NULL);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter)) {
		TnyFolder *folder =
			TNY_FOLDER (tny_iterator_get_current (iter));
		if (modest_tny_folder_get_local_folder_type (folder) == special_type) {
			special_folder = folder;
			break;
		}
		g_object_unref (G_OBJECT(folder));
		tny_iterator_next (iter);
	}
	g_object_unref (G_OBJECT (folders));
	g_object_unref (G_OBJECT (iter));

	return special_folder;
}



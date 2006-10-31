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

#include <tny-msg.h>
#include <tny-folder.h>			
#include <tny-folder-store.h>
#include <tny-folder-store-query.h>
#include <tny-header.h>
#include <tny-account.h>	
#include <tny-account-store.h>
#include <tny-store-account.h>	
#include <tny-store-account.h>
#include <tny-stream-camel.h>
#include <string.h>
#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>


#include <glib.h>
#include "modest-tny-store-actions.h"


void
modest_tny_store_actions_update_folders (TnyStoreAccount *storage_account)
{

// FIXME TODO: This results in failure on folder change.	

	TnyList *folders;
	TnyIterator *ifolders;
	const TnyFolder *cur_folder;
	TnyFolderStoreQuery *query;

/* 	folders = tny_store_account_get_folders (storage_account,  */
/* 						       TNY_STORE_ACCOUNT_FOLDER_TYPE_SUBSCRIBED); */
	query = tny_folder_store_query_new ();
	tny_folder_store_query_add_item (query, NULL, TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED);
	tny_folder_store_get_folders (TNY_FOLDER_STORE (storage_account),
				      folders, NULL);
	g_object_unref (query);
	
	ifolders = tny_list_create_iterator (folders);
	
	for (tny_iterator_first (ifolders); 
	     !tny_iterator_is_done (ifolders); 
	     tny_iterator_next (ifolders)) {
		
		cur_folder = TNY_FOLDER(tny_iterator_get_current (ifolders));
		tny_folder_refresh (cur_folder);
	}
	
	g_object_unref (ifolders);
}


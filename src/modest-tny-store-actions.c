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

#include <string.h>
#include <glib.h>
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
#include "modest-tny-store-actions.h"

TnyFolder *
modest_tny_store_actions_create_folder (TnyFolderStore *parent,
					const gchar *name)
{
	g_return_val_if_fail (TNY_IS_FOLDER_STORE (parent), NULL);
	g_return_val_if_fail (name, NULL);

	TnyFolder *new_folder = NULL;
	TnyStoreAccount *store_account;

	/* Create the folder */
	new_folder = tny_folder_store_create_folder (parent, name);
	if (!new_folder) 
		return NULL;

	/* Subscribe to folder */
	if (!tny_folder_is_subscribed (new_folder)) {
		store_account = tny_folder_get_account (TNY_FOLDER (parent));
		tny_store_account_subscribe (store_account, new_folder);
	}

	return new_folder;
}

void
modest_tny_store_actions_remove_folder (TnyFolder *folder)
{
	TnyFolderStore *folder_store;

	g_return_if_fail (TNY_IS_FOLDER (folder));

	/* Get folder store */
	folder_store = TNY_FOLDER_STORE (tny_folder_get_account (folder));

	/* Remove folder */
	tny_folder_store_remove_folder (folder_store, folder);

	/* Free instance */
	g_object_unref (G_OBJECT (folder));
}

void modest_tny_store_actions_rename_folder (TnyFolder *folder, 
					     const gchar *name)
{
	g_return_if_fail (TNY_IS_FOLDER (folder));
	g_return_if_fail (name);

	tny_folder_set_name (folder, name);
}

void modest_tny_store_actions_move_folder (TnyFolder *folder, 
					   TnyFolderStore *parent)
{
	/* TODO: set parent as parent */
}

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
#include <string.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>
#include <tny-folder-store.h>
#include <modest-tny-platform-factory.h>


#include <modest-account-mgr.h>
#include <modest-mail-operation.h>

static gchar *cachedir=NULL;
static gboolean move=FALSE;
static const gchar *src_name = NULL;
static const gchar *dst_name = NULL;

static void
find_folders (TnyFolderStore *store, TnyFolderStoreQuery *query,
	      TnyFolder **folder_src, TnyFolder **folder_dst)
{
	TnyIterator *iter;
	TnyList *folders;

	if ((*folder_src != NULL) && (*folder_dst != NULL))
		return;

	folders = tny_simple_list_new ();
	tny_folder_store_get_folders (store, folders, query, NULL);
	iter = tny_list_create_iterator (folders);

	while (!tny_iterator_is_done (iter) && (!*folder_src || !*folder_dst))
	{
		TnyFolderStore *folder = (TnyFolderStore*) tny_iterator_get_current (iter);
		const gchar *folder_name = NULL;

		folder_name = tny_folder_get_name (TNY_FOLDER (folder));

		if (strcmp (folder_name, src_name) == 0)
		    *folder_src = g_object_ref (folder);
		
		if (!strcmp (folder_name, dst_name))
		    *folder_dst = g_object_ref (folder);

		find_folders (folder, query, folder_src, folder_dst);
	    
 		g_object_unref (G_OBJECT (folder));

		tny_iterator_next (iter);	    
	}

	 g_object_unref (G_OBJECT (iter));
	 g_object_unref (G_OBJECT (folders));
}

static const GOptionEntry options[] = {
		{ "from",  'f', 0, G_OPTION_ARG_STRING, &src_name,
		  "Source folder", NULL},
		{ "to", 't', 0, G_OPTION_ARG_STRING, &dst_name,
		  "Destination folder", NULL},
		{ "cachedir", 'c', 0, G_OPTION_ARG_STRING, &cachedir,
		  "Cache directory", NULL },
		{ "move", 'm', 0, G_OPTION_ARG_NONE, &move,
		  "Move the messages instead of copy them", NULL },
		{ NULL }
};

int 
main (int argc, char **argv)
{
	GOptionContext *context;
	TnyList *accounts, *src_headers;
	TnyStoreAccount *account;
	TnyIterator *iter;
	TnyFolder *folder_src = NULL, *folder_dst = NULL;
	TnyPlatformFactory *fact = NULL;
	ModestAccountMgr *acc_mgr = NULL;
	ModestMailOperation *mail_op = NULL;
	TnyAccountStore *account_store = NULL;
	guint src_num_headers = 0, dst_num_headers = 0;
	GError *err;
    
	g_type_init ();

    	context = g_option_context_new ("Test");
	g_option_context_add_main_entries (context, options, "Modest");
	if (!g_option_context_parse (context, &argc, &argv, &err)) {
		g_printerr ("Error in command line parameter(s): '%s', exiting\n",
			    err ? err->message : "");
		return 0;
	}
	g_option_context_free (context);

	fact = TNY_PLATFORM_FACTORY (modest_tny_platform_factory_get_instance ());
	acc_mgr = MODEST_ACCOUNT_MGR (modest_tny_platform_factory_get_modest_account_mgr_instance (fact));
	account_store = tny_platform_factory_new_account_store (fact);	

	if (cachedir)
		g_print ("Using %s as cache directory\n", cachedir);

	if (!src_name || !dst_name) {
		g_printerr ("Error in command line parameter(s), specify source and target folders\n");	
		return 0;
	}

	/* Get accounts */
	accounts = tny_simple_list_new ();

	tny_account_store_get_accounts (account_store, accounts,
	      TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
    
	iter = tny_list_create_iterator (accounts);
	account = (TnyStoreAccount*) tny_iterator_get_current (iter);

	g_object_unref (G_OBJECT (iter));
	g_object_unref (G_OBJECT (accounts));

	/* Find the two folders */
	find_folders (TNY_FOLDER_STORE (account), NULL,
		      &folder_src, &folder_dst);

	if (!folder_src || !folder_dst)
		goto cleanup;

	/* Refresh folders */
	tny_folder_refresh (folder_src, NULL);
	src_num_headers = tny_folder_get_all_count (folder_src);

	tny_folder_refresh (folder_dst, NULL);
	dst_num_headers = tny_folder_get_all_count (folder_dst);

	/* Get all the headers of the source & target folder */
	src_headers = tny_simple_list_new ();
	tny_folder_get_headers (folder_src, src_headers, TRUE, NULL);

	mail_op = modest_mail_operation_new ();
		
	if (move)
		modest_mail_operation_move_folder (mail_op, 
						   folder_src, 
						   TNY_FOLDER_STORE (folder_dst));
	else
		modest_mail_operation_copy_folder (mail_op, 
						   folder_src, 
						   TNY_FOLDER_STORE (folder_dst));
	
	g_object_unref (G_OBJECT (src_headers));
	g_object_unref (G_OBJECT (mail_op));

 cleanup:
	g_object_unref (account);
    
	return 0;
}

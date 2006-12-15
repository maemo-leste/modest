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
#include <unistd.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>
#include <tny-account-store.h>
#include <tny-store-account.h>
#include <tny-folder.h>
#include <tny-folder-store.h>

#include "modest-tny-platform-factory.h"
#include "modest-account-mgr.h"
#include "modest-mail-operation.h"
#include "modest-mail-operation-queue.h"

GMainLoop *main_loop;

static void
on_progress_changed (ModestMailOperation *mail_op, gpointer user_data)
{
	ModestMailOperationStatus status;
	ModestMailOperationQueue *queue = NULL;

	g_print ("Refreshed %d of %d\n", 
		 modest_mail_operation_get_task_done  (mail_op), 
		 modest_mail_operation_get_task_total (mail_op));

	if (modest_mail_operation_is_finished (mail_op)) {
		queue = MODEST_MAIL_OPERATION_QUEUE (user_data);
		modest_mail_operation_queue_remove (queue, mail_op);
		g_main_loop_quit (main_loop);
	}
}

static gboolean
func (gpointer_data) 
{
	TnyStoreAccount *account;
	TnyIterator *iter;
	TnyPlatformFactory *fact = NULL;
	ModestAccountMgr *acc_mgr = NULL;
	ModestMailOperation *mail_op = NULL;
	ModestMailOperationQueue *queue = NULL;
	TnyAccountStore *account_store = NULL;
	TnyList *accounts;

	fact = TNY_PLATFORM_FACTORY (modest_tny_platform_factory_get_instance ());
	acc_mgr = MODEST_ACCOUNT_MGR (modest_tny_platform_factory_get_modest_account_mgr_instance (fact));
	account_store = tny_platform_factory_new_account_store (fact);	

	/* Get accounts */
	accounts = tny_simple_list_new ();

	tny_account_store_get_accounts (account_store, accounts,
					TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
    
	iter = tny_list_create_iterator (accounts);
	account = TNY_STORE_ACCOUNT (tny_iterator_get_current (iter));

	g_object_unref (G_OBJECT (iter));
	g_object_unref (G_OBJECT (accounts));

	queue = modest_tny_platform_factory_get_modest_mail_operation_queue_instance (fact);
	mail_op = modest_mail_operation_new ();
	
	g_signal_connect (G_OBJECT (mail_op), "progress_changed", 
			  G_CALLBACK (on_progress_changed), queue);

	if (modest_mail_operation_update_account (mail_op, account))
		modest_mail_operation_queue_add (queue, mail_op);

	g_object_unref (G_OBJECT (mail_op));

	return FALSE;
}

int
main (int argc, char **argv)
{   
	guint id;

	g_type_init ();
	g_thread_init(NULL);

	main_loop = g_main_loop_new (NULL, FALSE);
        id = g_timeout_add(1000, func, main_loop);

	g_main_loop_run(main_loop);

	return 0;
}

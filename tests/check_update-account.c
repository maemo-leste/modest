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

#include <modest-runtime.h>
#include "modest-account-mgr.h"
#include "modest-mail-operation.h"
#include "modest-mail-operation-queue.h"
#include <modest-init.h>
#include <modest-tny-account.h>

/* seconds we will wait for test to finish */
#define TEST_TIMEOUT 60

GMainLoop *main_loop;
gint retval = 0;

static void
on_progress_changed (ModestMailOperation *mail_op, ModestMailOperationState *state, gpointer user_data)
{
	g_print ("Refreshed %d of %d\n", 
		 modest_mail_operation_get_task_done  (mail_op), 
		 modest_mail_operation_get_task_total (mail_op));

}

static void
update_account_cb (ModestMailOperation *self,
		   TnyList *new_headers,
		   gpointer userdata)
{
	ModestMailOperationQueue *queue;

	if (modest_mail_operation_get_error (self))
		retval = 1;
	
	if (modest_mail_operation_is_finished (self)) {
		queue = MODEST_MAIL_OPERATION_QUEUE (userdata);
		modest_mail_operation_queue_remove (queue, self);
		g_main_loop_quit (main_loop);
	}

}

static gboolean
func (gpointer_data) 
{
	TnyStoreAccount *account = NULL;
	ModestAccountMgr *acc_mgr = NULL;
	ModestMailOperation *mail_op = NULL;
	ModestMailOperationQueue *queue = NULL;

	modest_init (0, NULL);

	acc_mgr       = modest_runtime_get_account_mgr();

	queue   = modest_runtime_get_mail_operation_queue ();
	mail_op = modest_mail_operation_new (NULL);
	
	g_signal_connect (G_OBJECT (mail_op), "progress_changed", 
			  G_CALLBACK (on_progress_changed), queue);

	modest_mail_operation_update_account (mail_op, modest_account_mgr_get_default_account (acc_mgr),
					      TRUE, FALSE, update_account_cb, queue);
	modest_mail_operation_queue_add (queue, mail_op);

	g_object_unref (G_OBJECT (mail_op));
	
	if (account)
		g_object_unref (account);

	return FALSE;
}

static gboolean 
got_timeout (gpointer userdata)
{
	retval = 1;

	g_main_loop_quit (main_loop);
	return FALSE;
}

int
main (int argc, char **argv)
{   
	guint id;

	g_type_init ();
	g_thread_init(NULL);

	main_loop = g_main_loop_new (NULL, FALSE);
        id = g_timeout_add(10, func, main_loop);
	g_timeout_add_seconds (TEST_TIMEOUT, got_timeout, NULL);

	g_main_loop_run(main_loop);

	return retval;
}

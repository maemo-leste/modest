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

#include <config.h>
#include <gdk/gdk.h>
#include <string.h>
#include <glib.h>
#include "modest-runtime.h"
#include "modest-init.h"
#include "modest-platform.h"
#include "modest-ui-actions.h"

static gboolean show_ui = FALSE;
static gint shutdown_timeout = 0;
static GOptionEntry option_entries [] =
{
	{ "show-ui", 's', 0, G_OPTION_ARG_NONE, &show_ui, "Show UI immediately, so no wait for DBUS activation", NULL },
	{ "shutdown-timeout", 't', 0, G_OPTION_ARG_INT, &shutdown_timeout, "Timeout in minutes for running Modest in prestart mode", NULL },
	{ NULL }
};

static guint shutdown_timeout_id = 0;

typedef struct {
	gulong queue_handler;
	gulong window_list_handler;
	gulong get_password_handler;
} MainSignalHandlers;

static gboolean
on_idle_exit_modest (gpointer data)
{
	MainSignalHandlers *handlers;
	ModestMailOperationQueue *mail_op_queue;

	/* Protect the Gtk calls */
	gdk_threads_enter ();
	mail_op_queue = modest_runtime_get_mail_operation_queue ();

	if (modest_tny_account_store_is_shutdown (modest_runtime_get_account_store ()) &&
	    modest_mail_operation_queue_running_shutdown (mail_op_queue)) {

		/* Disconnect signals. Will be freed by the destroy notify */
		handlers = (MainSignalHandlers *) data;
		g_signal_handler_disconnect (modest_runtime_get_mail_operation_queue (),
					     handlers->queue_handler);
		g_signal_handler_disconnect (modest_runtime_get_window_mgr (),
					     handlers->window_list_handler);
		g_signal_handler_disconnect (modest_runtime_get_account_store (),
					     handlers->get_password_handler);
		g_free (handlers);

		/* Wait for remaining tasks */
		while (gtk_events_pending ())
			gtk_main_iteration ();

		gtk_main_quit ();
	} else {
		ModestMailOperation *mail_op;
		mail_op = modest_mail_operation_new (NULL);
		modest_mail_operation_queue_add (mail_op_queue, mail_op);
		modest_mail_operation_shutdown (mail_op, modest_runtime_get_account_store ());
		g_object_unref (mail_op);
	}

	gdk_threads_leave ();

	return FALSE;
}

static void
on_queue_empty (ModestMailOperationQueue *queue,
		gpointer user_data)
{
	ModestWindowMgr *mgr = modest_runtime_get_window_mgr ();

	if (!modest_runtime_get_allow_shutdown ())
		return;

	/* Exit if the queue is empty and there are no more
	   windows. We can exit as well if the main window is hidden
	   and it's the only one */
	if (modest_window_mgr_get_num_windows (mgr) == 0)
		g_idle_add_full (G_PRIORITY_LOW, on_idle_exit_modest, user_data, NULL);
}

static void
on_window_list_empty (ModestWindowMgr *window_mgr,
		      gpointer user_data)
{
	ModestMailOperationQueue *queue = modest_runtime_get_mail_operation_queue ();

	if (!modest_runtime_get_allow_shutdown ())
		return;

	/* Exit if there are no more windows and the queue is empty */
	if (modest_mail_operation_queue_num_elements (queue) == 0)
		g_idle_add_full (G_PRIORITY_LOW, on_idle_exit_modest, user_data, NULL);
}

static gboolean
shutdown_timeout_handler (gpointer userdata)
{
	modest_runtime_set_allow_shutdown (TRUE);
	return FALSE;
}

int
main (int argc, char *argv[])
{
	/* Usually we don't show the application at first,
	 * because we wait for the top_application D-Bus method to
	 * be called. But that's annoying when starting from the
	 * command line.: */
	gboolean show_ui_without_top_application_method = FALSE;
	int retval  = 0;
	MainSignalHandlers *handlers;
	ModestTnyAccountStore *acc_store;

	GError *error = NULL;
	GOptionContext *context;

	ModestWindowMgr *mgr;

	context = g_option_context_new ("- Modest email client");
	g_option_context_add_main_entries (context, option_entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_print ("option parsing failed: %s\n", error->message);
		g_option_context_free (context);
		exit (1);
	}
	g_option_context_free (context);

	show_ui_without_top_application_method = show_ui;

	if (!show_ui_without_top_application_method) {
		g_print ("modest: use 'modest -s' to start from cmdline  with UI\n");
	}

	if (!g_thread_supported())
		g_thread_init (NULL);

	gdk_threads_init ();
	gdk_threads_enter ();

	if (!getenv("DISPLAY")) {
		g_printerr ("modest: DISPLAY env variable is not set\n");
		retval = 1;
		goto cleanup;
	}

	if (!gtk_init_check (&argc, &argv)) {
		g_printerr ("modest: failed to initialize gtk\n");
		retval = 1;
		goto cleanup;
	}

	if (!modest_init (argc, argv)) {
		g_printerr ("modest: cannot init modest\n");
		retval = 1;
		goto cleanup;
	}

	/* Create the account store & launch send queues */
	acc_store = modest_runtime_get_account_store ();
	modest_tny_account_store_start_send_queues (acc_store);

	handlers = g_malloc0 (sizeof (MainSignalHandlers));
	/* Connect to the "queue-emtpy" signal */
	handlers->queue_handler =
		g_signal_connect (modest_runtime_get_mail_operation_queue (),
				  "queue-empty",
				  G_CALLBACK (on_queue_empty),
				  handlers);

	/* Connect to the "window-list-emtpy" signal */
	handlers->window_list_handler =
		g_signal_connect (modest_runtime_get_window_mgr (),
				  "window-list-empty",
				  G_CALLBACK (on_window_list_empty),
				  handlers);

	/* Connect to the "password-requested" signal */
	handlers->get_password_handler =
		g_signal_connect (acc_store,
				  "password_requested",
				  G_CALLBACK (modest_ui_actions_on_password_requested),
				  NULL);

	/* Create cached windows */
	mgr = modest_runtime_get_window_mgr ();
	modest_window_mgr_create_caches (mgr);

	/* Usually, we only show the UI when we get the "top_application" D-Bus method.
	 * This allows modest to start via D-Bus activation to provide a service,
	 * without showing the UI.
	 * The UI will be shown later (or just after starting if no otehr D-Bus method was used),
	 * when we receive the "top_application" D-Bus method.
	 */
	if (show_ui_without_top_application_method) {
		ModestWindow *window;

		modest_runtime_set_allow_shutdown (TRUE);
		mgr = modest_runtime_get_window_mgr();
		window = modest_window_mgr_show_initial_window (mgr);
		if (!window) {
			g_printerr ("modest: failed to get main window instance\n");
			retval = 1;
			goto cleanup;
		}
		/* Remove new mail notifications if exist */
		modest_platform_remove_new_mail_notifications (FALSE);
	} else {
		if (shutdown_timeout > 0) {
			modest_runtime_set_allow_shutdown (FALSE);
			shutdown_timeout_id = g_timeout_add_seconds (shutdown_timeout * 60, shutdown_timeout_handler, NULL);
		}
	}

	gtk_main ();

cleanup:
	gdk_threads_leave ();

	if (!modest_init_uninit ()) {
		g_printerr ("modest: modest_init_uninit failed\n");
		retval = 1;
	}

	return retval;
}

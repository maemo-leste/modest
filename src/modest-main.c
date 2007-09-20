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

#include <glib.h>
#include <modest-runtime.h>
#include <modest-init.h>
#include <gdk/gdk.h>
#include <widgets/modest-main-window.h>
#include <string.h>

int
main (int argc, char *argv[])
{
	/* Usually we don't show the application at first, 
	 * because we wait for the top_application D-Bus method to 
	 * be called. But that's annoying when starting from the 
	 * command line.: */
	gboolean show_ui_without_top_application_method = FALSE;
	if (argc >= 2) {
		if (strcmp (argv[1], "showui") == 0)
			show_ui_without_top_application_method = TRUE;
	}
	
	ModestWindow *win;
	int retval  = 0;
		
	if (!g_thread_supported())
		g_thread_init (NULL);

	gdk_threads_init ();
	gdk_threads_enter (); /* CHECKED */

	if (!gtk_init_check(&argc, &argv)) {
		g_printerr ("modest: failed to initialize gtk\n");
		retval = 1;
		goto cleanup;
	}

	if (!modest_init (argc, argv)) {
		g_printerr ("modest: cannot init modest\n");
		retval = 1;
		goto cleanup;
	}

	/* this will create & register the window */
	win = modest_window_mgr_get_main_window (modest_runtime_get_window_mgr());
	if (!win) {
		g_printerr ("modest: failed to get main window instance\n");
		retval = 1;
		goto cleanup;
	}
	
	/* Usually, we only show the UI when we get the "top_application" D-Bus method.
	 * This allows modest to start via D-Bus activation to provide a service, 
	 * without showing the UI.
	 * The UI will be shown later (or just after starting if no otehr D-Bus method was used),
	 * when we receive the "top_application" D-Bus method.
	 */
	if (show_ui_without_top_application_method)
		gtk_widget_show_all (GTK_WIDGET(win));
	
	
	gtk_main ();	
	retval = 0;

cleanup:
	gdk_threads_leave (); /* CHECKED */

	if (!modest_init_uninit ()) {
		g_printerr ("modest: modest_init_uninit failed\n");
		retval = 1;
	}

	return retval;
}


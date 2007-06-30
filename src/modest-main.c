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

int
main (int argc, char *argv[])
{
	ModestWindow *win;
	int retval  = 0;
		
	if (!g_thread_supported())
		g_thread_init (NULL);

	gdk_threads_init ();
	gdk_threads_enter ();

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

	win = modest_main_window_new ();
	gtk_widget_show_all (GTK_WIDGET(win));
		
	if (!win) {
		g_printerr ("modest: failed to create main window\n");
		retval = 1;
		goto cleanup;
	}
	
	modest_window_mgr_register_window (modest_runtime_get_window_mgr(), 
					   win);
	g_object_unref (win);
	
	gtk_main ();	
	retval = 1;

cleanup:
	gdk_threads_leave ();

	if (!modest_init_uninit ()) 
		g_printerr ("modest: modest_init_uninit failed\n");
	
	return retval;
}


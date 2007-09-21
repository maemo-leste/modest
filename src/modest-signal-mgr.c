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

#include "modest-signal-mgr.h"

typedef struct {
	guint handler_id;
	GObject *obj;
} SignalHandler;

GSList *
modest_signal_mgr_connect (GSList *lst, GObject *instance, const gchar *detail,
			   GCallback handler, gpointer data)
{
	SignalHandler *sighandler;
	
	g_return_val_if_fail (instance, lst);
	g_return_val_if_fail (detail, lst);
	g_return_val_if_fail (handler, lst);
	
	sighandler = g_new (SignalHandler, 1);
	sighandler->obj        = g_object_ref (instance);
	sighandler->handler_id = g_signal_connect (instance, detail, handler, data);
	
	return g_slist_prepend (lst, (gpointer)sighandler);
}


void
modest_signal_mgr_disconnect_all_and_destroy (GSList *lst)
{
	GSList *cursor;	
	for (cursor = lst; cursor; cursor = g_slist_next(cursor)) {
		SignalHandler *handler;
		handler = (SignalHandler*)cursor->data;
		if (handler && handler->obj && G_IS_OBJECT(handler->obj)) {
			if (g_signal_handler_is_connected (handler->obj, handler->handler_id)) {
				/* g_debug ("%p: disconnecting %d", handler->obj, handler->handler_id); */
				g_signal_handler_disconnect (handler->obj, handler->handler_id);
			}
			g_object_unref (handler->obj);
			handler->obj = NULL;
		}
		g_free (handler);
	}
	g_slist_free (lst);
}

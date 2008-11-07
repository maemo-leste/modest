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
#include <string.h>

typedef struct {
	guint handler_id;
	gchar *signal_name;
	GObject *obj;
} SignalHandler;

GSList *
modest_signal_mgr_connect (GSList *lst, 
			   GObject *instance, 
			   const gchar *signal_name,
			   GCallback handler,
			   gpointer data)
{
	SignalHandler *sighandler;
	
	g_return_val_if_fail (instance, lst);
	g_return_val_if_fail (signal_name, lst);
	g_return_val_if_fail (handler, lst);
	
	sighandler = g_new (SignalHandler, 1);
	sighandler->obj         = g_object_ref (instance);
	sighandler->handler_id  = g_signal_connect (instance, signal_name, handler, data);
	sighandler->signal_name = g_strdup (signal_name);
	
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
				g_signal_handler_disconnect (handler->obj, handler->handler_id);
			}
			g_object_unref (handler->obj);
			g_free (handler->signal_name);
			handler->obj = NULL;
		}
		g_free (handler);
	}
	g_slist_free (lst);
}

static gboolean
obj_in_a_signal_handler (gconstpointer a,
			 gconstpointer b)
{
	SignalHandler *handler, *list_item_handler;

	list_item_handler = (SignalHandler *) a;
	handler = (SignalHandler *) b;

	if (list_item_handler->obj == handler->obj &&
	    !strcmp (list_item_handler->signal_name, handler->signal_name))
		return FALSE;
	else 
		return TRUE;
}

GSList *
modest_signal_mgr_disconnect (GSList *list, 
			      GObject *instance,
			      const gchar *signal_name)
{
	GSList *item = NULL;
	SignalHandler *signal_handler = NULL, *tmp = NULL;

	tmp = g_new (SignalHandler, 1);
	tmp->obj = instance;
	tmp->signal_name = g_strdup (signal_name);

	/* Find the element */
	item = g_slist_find_custom (list, tmp, obj_in_a_signal_handler);
	g_return_val_if_fail (item != NULL, list);

	/* Disconnect the signal */
	signal_handler = (SignalHandler *) item->data;
	g_signal_handler_disconnect (signal_handler->obj, signal_handler->handler_id);

	/* Free the handlers */
	g_object_unref (signal_handler->obj);
	g_free (signal_handler->signal_name);
	g_free (signal_handler);
	g_free (tmp->signal_name);
	g_free (tmp);

	/* Remove item from list */
	return g_slist_delete_link (list, item);
}

gboolean  
modest_signal_mgr_is_connected (GSList *list, 
				GObject *instance,
				const gchar *signal_name)
{
	GSList *item = NULL;
	SignalHandler *tmp = NULL;

	/* Build the helper object */
	tmp = g_new (SignalHandler, 1);
	tmp->obj = instance;
	tmp->signal_name = g_strdup (signal_name);

	/* Find the element */
	item = g_slist_find_custom (list, tmp, obj_in_a_signal_handler);

	/* Free the handlers */
	g_free (tmp->signal_name);
	g_free (tmp);

	return (item != NULL) ? TRUE : FALSE;
}

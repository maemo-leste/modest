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


/* modest-window-mgr.c */

#include "modest-window-mgr.h"
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void                   modest_window_mgr_class_init    (ModestWindowMgrClass *klass);
static void                   modest_window_mgr_init          (ModestWindowMgr *obj);
static void                   modest_window_mgr_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_WINDOW_CLOSED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestWindowMgrPrivate ModestWindowMgrPrivate;
struct _ModestWindowMgrPrivate {
	GSList *open_windows;
	
};
#define MODEST_WINDOW_MGR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                               MODEST_TYPE_WINDOW_MGR, \
                                               ModestWindowMgrPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

GType
modest_window_mgr_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestWindowMgrClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_window_mgr_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestWindowMgr),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_window_mgr_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestWindowMgr",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_window_mgr_class_init (ModestWindowMgrClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_window_mgr_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestWindowMgrPrivate));

 	signals[LAST_WINDOW_CLOSED_SIGNAL] =
		g_signal_new ("last_window_closed",
			      G_TYPE_FROM_CLASS(gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET(ModestWindowMgrClass, last_window_closed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
modest_window_mgr_init (ModestWindowMgr *obj)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE(obj);
	priv->open_windows = NULL;
}

static void
modest_window_mgr_finalize (GObject *obj)
{
	ModestWindowMgrPrivate *priv = MODEST_WINDOW_MGR_GET_PRIVATE(obj);
	g_slist_free (priv->open_windows);
	priv->open_windows = NULL;	
}

GObject*
modest_window_mgr_new (void)
{
	return G_OBJECT(g_object_new(MODEST_TYPE_WINDOW_MGR, NULL));
}

/* insert many other interesting function implementations */
/* such as modest_window_mgr_do_something, or modest_window_mgr_has_foo */

gboolean
modest_window_mgr_register (ModestWindowMgr *self, GObject *win,
			    ModestWindowType type,
			    guint window_id)
{
	ModestOpenWindow *openwin = NULL;
	ModestWindowMgrPrivate *priv;

	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (type==MODEST_MAIN_WINDOW || type==MODEST_EDIT_WINDOW
				|| type == MODEST_VIEW_WINDOW, FALSE);

	priv = MODEST_WINDOW_MGR_GET_PRIVATE(self);

	openwin = g_new (ModestOpenWindow, 1);
	openwin->win  = win;
	openwin->type = type;
	openwin->id   = window_id;
	
	priv->open_windows = g_slist_prepend (priv->open_windows, openwin);

	return TRUE;
}



gboolean
modest_window_mgr_unregister (ModestWindowMgr *self, GObject *win)
{
	ModestWindowMgrPrivate *priv;
	GSList *cursor;
	gboolean found = FALSE;
	
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (win, FALSE);

	priv = MODEST_WINDOW_MGR_GET_PRIVATE(self);

	cursor = priv->open_windows;
	while (cursor) {
		if (((ModestOpenWindow*)cursor->data)->win == win) {
			priv->open_windows = g_slist_delete_link (priv->open_windows,
								  cursor);
			found = TRUE;
			break;
		}
		cursor = cursor->next;
	}
	if (found) {
		guint win_num = g_slist_length (priv->open_windows);
		if (win_num == 0) 
			g_signal_emit (self, signals[LAST_WINDOW_CLOSED_SIGNAL],
				       0);
	}

	return found;
}


GObject *
modest_window_mgr_find_by_type (ModestWindowMgr *self, ModestWindowType type)
{
	ModestWindowMgrPrivate *priv;
	GSList *cursor;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_WINDOW_MGR_GET_PRIVATE(self);
	cursor = priv->open_windows;
	while (cursor) {
		ModestOpenWindow *openwin = (ModestOpenWindow*)cursor->data;
		if (openwin->type == type)
			return openwin->win;
		cursor = cursor->next;
	}
	
	return NULL;
}


GObject *
modest_window_mgr_find_by_id (ModestWindowMgr *self, gint window_id)
{
	ModestWindowMgrPrivate *priv;
	GSList *cursor;

	g_return_val_if_fail (self, NULL);
	
	priv = MODEST_WINDOW_MGR_GET_PRIVATE(self);
	cursor = priv->open_windows;
	while (cursor) {
		ModestOpenWindow *openwin = (ModestOpenWindow*)cursor->data;
		if (openwin->id == window_id)
			return openwin->win;
		cursor = cursor->next;
	}
	return NULL;
}


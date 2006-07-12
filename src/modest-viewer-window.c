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


#include "modest-ui.h"
#include "modest-tny-msg-view.h"
#include "modest-viewer-window.h"


/* 'private'/'protected' functions */
static void                      modest_viewer_window_class_init    (ModestViewerWindowClass *klass);
static void                      modest_viewer_window_init          (ModestViewerWindow *obj);
static void                      modest_viewer_window_finalize      (GObject *obj);

/* list my signals */
enum {
	LAST_SIGNAL
};

typedef struct _ModestViewerWindowPrivate ModestViewerWindowPrivate;
struct _ModestViewerWindowPrivate {
	ModestTnyMsgView *msg_view;
	gpointer user_data;
};
#define MODEST_VIEWER_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                  MODEST_TYPE_VIEWER_WINDOW, \
                                                  ModestViewerWindowPrivate))
/* globals */
static GtkWindowClass *parent_class = NULL;


GType
modest_viewer_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestViewerWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_viewer_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestViewerWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_viewer_window_init,
		};
		my_type = g_type_register_static (GTK_TYPE_WINDOW,
		                                  "ModestViewerWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_viewer_window_class_init (ModestViewerWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_viewer_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestViewerWindowPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[MY_SIGNAL_1] = */
/* 		g_signal_new ("my_signal_1",....); */
/* 	signals[MY_SIGNAL_2] = */
/* 		g_signal_new ("my_signal_2",....); */
/* 	etc. */
}


static void
modest_viewer_window_init (ModestViewerWindow *obj)
{
	ModestViewerWindowPrivate *priv = MODEST_VIEWER_WINDOW_GET_PRIVATE(obj);

	priv->user_data = NULL;
	priv->msg_view = NULL;
}


static void
modest_viewer_window_finalize (GObject *obj)
{
	ModestViewerWindowPrivate *priv;

	priv = MODEST_VIEWER_WINDOW_GET_PRIVATE(obj);
	        
	if (priv->user_data)
		g_free(priv->user_data);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


GtkWidget*
modest_viewer_window_new (ModestUI *ui, TnyMsgIface *msg)
{
	GObject *self;
	ModestViewerWindowPrivate *priv;
	GtkWidget *w;
	gpointer data;
	GtkWidget *msg_view;

	self = G_OBJECT(g_object_new(MODEST_TYPE_VIEWER_WINDOW, NULL));
	priv = MODEST_VIEWER_WINDOW_GET_PRIVATE(self);

	msg_view = modest_tny_msg_view_new(msg);

	data = NULL;
	w = GTK_WIDGET(modest_ui_new_viewer_window(ui, msg_view, msg, &data));
	if (!w)
		return NULL;
	if (!data)
		g_message("viewer window user data is emtpy");

	gtk_container_add(GTK_CONTAINER(self), w);
	priv->user_data = data;
	priv->msg_view = MODEST_TNY_MSG_VIEW(msg_view);

	return GTK_WIDGET(self);
}


/*
 * return user defined data from a ModestViewerWindow instance
 * like e.g. a refernce to a GladeXML*
 */
gpointer
modest_viewer_window_get_data(ModestViewerWindow *viewer_win)
{
	ModestViewerWindowPrivate *priv;

	g_return_val_if_fail (viewer_win, NULL);

	priv = MODEST_VIEWER_WINDOW_GET_PRIVATE(viewer_win);

	return priv->user_data;
}


ModestTnyMsgView*
modest_viewer_window_get_tiny_msg_view(ModestViewerWindow *viewer_win)
{
	ModestViewerWindowPrivate *priv;

	g_return_val_if_fail (viewer_win, NULL);

	priv = MODEST_VIEWER_WINDOW_GET_PRIVATE(viewer_win);

	return priv->msg_view;
}

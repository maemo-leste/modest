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
#include "modest-editor-window.h"

/* 'private'/'protected' functions */
static void                      modest_editor_window_class_init    (ModestEditorWindowClass *klass);
static void                      modest_editor_window_init          (ModestEditorWindow *obj);
static void                      modest_editor_window_finalize      (GObject *obj);

/* list my signals */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ModestEditorWindowPrivate ModestEditorWindowPrivate;
struct _ModestEditorWindowPrivate {
	gpointer user_data;
	gboolean modified;
	GList *attachments;
	gchar *identity;
	gchar *transport;
};
#define MODEST_EDITOR_WINDOW_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                  MODEST_TYPE_EDITOR_WINDOW, \
                                                  ModestEditorWindowPrivate))
/* globals */
static GObjectClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

GType
modest_editor_window_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestEditorWindowClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_editor_window_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestEditorWindow),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_editor_window_init,
		};
		my_type = g_type_register_static (G_TYPE_OBJECT,
		                                  "ModestEditorWindow",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_editor_window_class_init (ModestEditorWindowClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_editor_window_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestEditorWindowPrivate));

}

static void
modest_editor_window_init (ModestEditorWindow *obj)
{
	ModestEditorWindowPrivate *priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(obj);

	priv->user_data = NULL;
	priv->modified = FALSE;
	priv->attachments = NULL;
	priv->identity = NULL;
	priv->transport = NULL;
	obj->window = NULL;
}

static void
modest_editor_window_finalize (GObject *obj)
{
	ModestEditorWindowPrivate *priv;

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(obj);

	if (priv->user_data)
		g_free(priv->user_data);

	modest_editor_window_set_attachments(MODEST_EDITOR_WINDOW(obj), NULL);
	g_free(priv->identity);
	g_free(priv->transport);
	g_object_unref (MODEST_EDITOR_WINDOW(obj)->window);
	MODEST_EDITOR_WINDOW(obj)->window = NULL;
	
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GObject*
modest_editor_window_new (ModestUI *ui)
{
	GObject *self;
	ModestEditorWindowPrivate *priv;
	GObject *edit_win;
	gpointer data;

	self = G_OBJECT(g_object_new(MODEST_TYPE_EDITOR_WINDOW, NULL));
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(self);

	/* for now create a local test-window */

	data = NULL;
	edit_win = G_OBJECT(modest_ui_new_editor_window(ui, &data));
	
	if (!edit_win)
		return NULL;
	if (!data)
		g_message("editor window user data is emtpy");

	MODEST_EDITOR_WINDOW(self)->window = GTK_WINDOW(edit_win);
	priv->user_data = data;
	
	return self;
}

/*
 * return user defined data from a ModestEditorWindow instance
 * like e.g. a refernce to a GladeXML*
 */
gpointer modest_editor_window_get_data(ModestEditorWindow *edit_win)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win) {
		return NULL;
	}
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	// g_message("get priv->data = %p", priv->user_data);

	return priv->user_data;
}

gboolean modest_editor_window_set_modified(ModestEditorWindow *edit_win, gboolean modified)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win) {
		return FALSE;
	}
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	priv->modified = modified;

	return priv->modified;
}

gboolean modest_editor_window_get_modified(ModestEditorWindow *edit_win)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win) {
		return FALSE;
	}
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	// g_message("get priv->data = %p", priv->user_data);

	return priv->modified;
}	

gboolean modest_editor_window_set_to_header(ModestEditorWindow *edit_win, const gchar *to)
{
	ModestEditorWindowPrivate *priv;

	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_to_header(to, priv->user_data);
}


gboolean modest_editor_window_set_cc_header(ModestEditorWindow *edit_win, const gchar *cc)
{
	ModestEditorWindowPrivate *priv;
	
	if (!edit_win)
		return FALSE;

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_cc_header(cc, priv->user_data);
}

gboolean modest_editor_window_set_bcc_header(ModestEditorWindow *edit_win, const gchar *bcc)
{
	ModestEditorWindowPrivate *priv;
	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_bcc_header(bcc, priv->user_data);
}

gboolean modest_editor_window_set_subject_header(ModestEditorWindow *edit_win, const gchar *subject)
{
	ModestEditorWindowPrivate *priv;
	
	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_subject_header(subject, priv->user_data);
}

gboolean modest_editor_window_set_body(ModestEditorWindow *edit_win, const gchar *body)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return modest_ui_editor_window_set_body(body, priv->user_data);
}


gboolean modest_editor_window_attach_file(ModestEditorWindow *edit_win,
					  ModestTnyAttachment *attachment)
{
	ModestEditorWindowPrivate *priv;

	if (!edit_win)
		return FALSE;
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);
	
	priv->attachments = g_list_append(priv->attachments, 
					  attachment);
	
	return modest_ui_editor_window_update_attachments(priv->user_data);
}

GList * modest_editor_window_set_attachments(ModestEditorWindow *edit_win, const GList* attachments)
{
	ModestEditorWindowPrivate *priv;
	GList *pos;

	g_return_val_if_fail(edit_win, NULL);
	
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	modest_tny_attachment_free_list(priv->attachments);
	priv->attachments = g_list_copy((GList *)attachments);
	for (pos = priv->attachments ; pos ; pos = pos->next )
		g_object_ref(pos->data);

	return priv->attachments;
}

GList * modest_editor_window_get_attachments(ModestEditorWindow *edit_win)
{
	ModestEditorWindowPrivate *priv;

	g_return_val_if_fail(edit_win, NULL);
	
	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);
	return priv->attachments;
}


void
modest_editor_window_set_identity(ModestEditorWindow *edit_win, const gchar *identity)
{
	ModestEditorWindowPrivate *priv;

	g_return_if_fail(edit_win);

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	g_free(priv->identity);
	priv->identity = g_strdup(identity);
}


const gchar *
modest_editor_window_get_identity(ModestEditorWindow *edit_win)
{
	ModestEditorWindowPrivate *priv;

	g_return_val_if_fail(edit_win, NULL);

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return priv->identity;
}	


void
modest_editor_window_set_transport(ModestEditorWindow *edit_win, const gchar *transport)
{
	ModestEditorWindowPrivate *priv;

	g_return_if_fail(edit_win);

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	g_free(priv->transport);
	priv->transport = g_strdup(transport);
}


const gchar *
modest_editor_window_get_transport(ModestEditorWindow *edit_win)
{
	ModestEditorWindowPrivate *priv;

	g_return_val_if_fail(edit_win, NULL);

	priv = MODEST_EDITOR_WINDOW_GET_PRIVATE(edit_win);

	return priv->transport;
}

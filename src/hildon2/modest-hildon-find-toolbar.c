/* Copyright (c) 2009, Igalia
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

#include <glib/gi18n.h>
#include "modest-hildon-find-toolbar.h"

typedef struct _ModestHildonFindToolbarPrivate ModestHildonFindToolbarPrivate;
struct _ModestHildonFindToolbarPrivate {
	gchar *current_prefix;
};


static void modest_hildon_find_toolbar_class_init (ModestHildonFindToolbarClass *klass);
static void modest_hildon_find_toolbar_init (ModestHildonFindToolbar *self);
static void modest_isearch_toolbar_iface_init (gpointer g, gpointer iface_data);

/* GObject interface */
static void modest_hildon_find_toolbar_finalize (GObject *obj);

/* ModestISearchToolbar interface */
static void modest_hildon_find_toolbar_highlight_entry (ModestISearchToolbar *self, gboolean focus);
static void modest_hildon_find_toolbar_highlight_entry_default (ModestISearchToolbar *self, gboolean focus);
static void modest_hildon_find_toolbar_set_label (ModestISearchToolbar *self, const gchar *label);
static void modest_hildon_find_toolbar_set_label_default (ModestISearchToolbar *self, const gchar *label);
static const gchar *modest_hildon_find_toolbar_get_search (ModestISearchToolbar *self);
static const gchar *modest_hildon_find_toolbar_get_search_default (ModestISearchToolbar *self);

/* signals */
static void on_find_toolbar_close (HildonFindToolbar *self, gpointer userdata);
static void on_find_toolbar_search (HildonFindToolbar *self, gpointer userdata);

#define MODEST_HILDON_FIND_TOOLBAR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
										    MODEST_TYPE_HILDON_FIND_TOOLBAR, \
										    ModestHildonFindToolbarPrivate))
/* globals */
static HildonFindToolbarClass *parent_class = NULL;

G_DEFINE_TYPE_EXTENDED    (ModestHildonFindToolbar,
			   modest_hildon_find_toolbar,
			   HILDON_TYPE_FIND_TOOLBAR,
			   0,
			   {
			     G_IMPLEMENT_INTERFACE (MODEST_TYPE_ISEARCH_TOOLBAR, modest_isearch_toolbar_iface_init);
			     g_type_interface_add_prerequisite (g_define_type_id, GTK_TYPE_TOOLBAR);
			   }
			   );

GtkWidget *
modest_hildon_find_toolbar_new                            (const gchar *label)
{
	GtkWidget *toolbar = g_object_new (MODEST_TYPE_HILDON_FIND_TOOLBAR, NULL);
	modest_isearch_toolbar_set_label (MODEST_ISEARCH_TOOLBAR (toolbar), label);

	return toolbar;
}

static void
modest_hildon_find_toolbar_class_init (ModestHildonFindToolbarClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class = (GObjectClass *) klass;
	gobject_class->finalize = modest_hildon_find_toolbar_finalize;

	klass->highlight_entry_func = modest_hildon_find_toolbar_highlight_entry_default;
	klass->set_label_func = modest_hildon_find_toolbar_set_label_default;
	klass->get_search_func = modest_hildon_find_toolbar_get_search_default;

	g_type_class_add_private (gobject_class, sizeof(ModestHildonFindToolbarPrivate));
}

static void
modest_hildon_find_toolbar_init (ModestHildonFindToolbar *self)
{
	ModestHildonFindToolbarPrivate *priv;

	priv = MODEST_HILDON_FIND_TOOLBAR_GET_PRIVATE (self);
	priv->current_prefix = NULL;
	g_signal_connect (G_OBJECT (self), "close", G_CALLBACK (on_find_toolbar_close), self);
	g_signal_connect (G_OBJECT (self), "search", G_CALLBACK (on_find_toolbar_search), self);
}

static void
modest_isearch_toolbar_iface_init (gpointer g, gpointer iface_data)
{
	ModestISearchToolbarIface *iface = (ModestISearchToolbarIface *) g;

	iface->highlight_entry = modest_hildon_find_toolbar_highlight_entry;
	iface->set_label = modest_hildon_find_toolbar_set_label;
	iface->get_search = modest_hildon_find_toolbar_get_search;
}

static void 
modest_hildon_find_toolbar_finalize (GObject *obj)
{
	ModestHildonFindToolbarPrivate *priv;

	priv = MODEST_HILDON_FIND_TOOLBAR_GET_PRIVATE (obj);
	g_free (priv->current_prefix);
	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}

static void
modest_hildon_find_toolbar_highlight_entry (ModestISearchToolbar *self,
					    gboolean get_focus)
{
	MODEST_HILDON_FIND_TOOLBAR_GET_CLASS (self)->highlight_entry_func (self, get_focus);
}

static void
modest_hildon_find_toolbar_highlight_entry_default (ModestISearchToolbar *self,
						    gboolean get_focus)
{
	hildon_find_toolbar_highlight_entry (HILDON_FIND_TOOLBAR (self), get_focus);
}

static void
modest_hildon_find_toolbar_set_label (ModestISearchToolbar *self,
				      const gchar *label)
{
	MODEST_HILDON_FIND_TOOLBAR_GET_CLASS (self)->set_label_func (self, label);
}

static void
modest_hildon_find_toolbar_set_label_default (ModestISearchToolbar *self,
					      const gchar *label)
{
	g_object_set (G_OBJECT (self), "label", label, NULL);
}

static const gchar *
modest_hildon_find_toolbar_get_search (ModestISearchToolbar *self)
{
	return MODEST_HILDON_FIND_TOOLBAR_GET_CLASS (self)->get_search_func (self);
}

static const gchar *
modest_hildon_find_toolbar_get_search_default (ModestISearchToolbar *self)
{
	ModestHildonFindToolbarPrivate *priv;

	priv = MODEST_HILDON_FIND_TOOLBAR_GET_PRIVATE (self);

	if (priv->current_prefix) {
		g_free (priv->current_prefix);
		priv->current_prefix = NULL;
	}

	g_object_get (G_OBJECT (self), "prefix", &(priv->current_prefix), NULL);

	return priv->current_prefix;
}

static void 
on_find_toolbar_close (HildonFindToolbar *self, gpointer userdata)
{
	g_signal_emit_by_name (G_OBJECT (self), "isearch-close");
}

static void
on_find_toolbar_search (HildonFindToolbar *self, gpointer userdata)
{
	g_signal_emit_by_name (G_OBJECT (self), "isearch-search");
}

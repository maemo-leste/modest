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
#include "modest-find-toolbar.h"

typedef struct _ModestFindToolbarPrivate ModestFindToolbarPrivate;
struct _ModestFindToolbarPrivate {
	GtkWidget *label;
	GtkWidget *entry;
};


static void modest_find_toolbar_class_init (ModestFindToolbarClass *klass);
static void modest_find_toolbar_init (ModestFindToolbar *self);
static void modest_isearch_toolbar_iface_init (gpointer g, gpointer iface_data);

/* GObject interface */
static void modest_find_toolbar_finalize (GObject *obj);

/* ModestISearchToolbar interface */
static void modest_find_toolbar_highlight_entry (ModestISearchToolbar *self, gboolean focus);
static void modest_find_toolbar_highlight_entry_default (ModestISearchToolbar *self, gboolean focus);
static void modest_find_toolbar_set_label (ModestISearchToolbar *self, const gchar *label);
static void modest_find_toolbar_set_label_default (ModestISearchToolbar *self, const gchar *label);
static const gchar *modest_find_toolbar_get_search (ModestISearchToolbar *self);
static const gchar *modest_find_toolbar_get_search_default (ModestISearchToolbar *self);

/* signals */
static void on_entry_activate (GtkEntry *entry, gpointer userdata);
static void on_close_clicked (GtkToolButton *button, gpointer userdata);

#define MODEST_FIND_TOOLBAR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
									     MODEST_TYPE_FIND_TOOLBAR, \
									     ModestFindToolbarPrivate))
/* globals */
static GtkToolbarClass *parent_class = NULL;

G_DEFINE_TYPE_EXTENDED    (ModestFindToolbar,
			   modest_find_toolbar,
			   GTK_TYPE_TOOLBAR,
			   0,
			   {
			     G_IMPLEMENT_INTERFACE (MODEST_TYPE_ISEARCH_TOOLBAR, modest_isearch_toolbar_iface_init);
			   }
			   );

GtkWidget *
modest_find_toolbar_new                            (const gchar *label)
{
	GtkWidget *toolbar = g_object_new (MODEST_TYPE_FIND_TOOLBAR, NULL);
	modest_isearch_toolbar_set_label (MODEST_ISEARCH_TOOLBAR (toolbar), label);

	return toolbar;
}

static void
modest_find_toolbar_class_init (ModestFindToolbarClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);
	gobject_class = (GObjectClass *) klass;
	gobject_class->finalize = modest_find_toolbar_finalize;

	klass->highlight_entry_func = modest_find_toolbar_highlight_entry_default;
	klass->set_label_func = modest_find_toolbar_set_label_default;
	klass->get_search_func = modest_find_toolbar_get_search_default;

	g_type_class_add_private (gobject_class, sizeof(ModestFindToolbarPrivate));
}

static void
modest_find_toolbar_init (ModestFindToolbar *self)
{
	ModestFindToolbarPrivate *priv;
	GtkToolItem *label_tool_item;
	GtkToolItem *entry_tool_item;
	GtkToolItem *close_tool_button;

	priv = MODEST_FIND_TOOLBAR_GET_PRIVATE (self);

	gtk_widget_set_no_show_all (GTK_WIDGET (self), TRUE);

	label_tool_item = gtk_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (label_tool_item));
	priv->label = gtk_label_new (NULL);
	gtk_widget_show (priv->label);
	gtk_container_add (GTK_CONTAINER (label_tool_item), priv->label);

	gtk_toolbar_insert (GTK_TOOLBAR (self), label_tool_item, -1);
	gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (label_tool_item), "expand", FALSE, NULL);
	g_object_set (GTK_TOOL_ITEM (label_tool_item), "is-important", TRUE, NULL);

	entry_tool_item = gtk_tool_item_new ();
	gtk_widget_show (GTK_WIDGET (entry_tool_item));
	priv->entry = gtk_entry_new ();
	gtk_widget_show (priv->entry);
	gtk_container_add (GTK_CONTAINER (entry_tool_item), priv->entry);

	gtk_toolbar_insert (GTK_TOOLBAR (self), entry_tool_item, -1);
	gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (entry_tool_item), "expand", TRUE, NULL);
	g_object_set (GTK_TOOL_ITEM (entry_tool_item), "is-important", TRUE, NULL);

	close_tool_button = gtk_tool_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_widget_show (GTK_WIDGET (close_tool_button));

	gtk_toolbar_insert (GTK_TOOLBAR (self), close_tool_button, -1);
	gtk_container_child_set (GTK_CONTAINER (self), GTK_WIDGET (close_tool_button), "expand", FALSE, NULL);
	g_object_set (GTK_TOOL_ITEM (close_tool_button), "is-important", TRUE, NULL);

	g_signal_connect (G_OBJECT (priv->entry), "activate", G_CALLBACK (on_entry_activate), self);
	g_signal_connect (G_OBJECT (close_tool_button), "clicked", G_CALLBACK (on_close_clicked), self);
}

static void
modest_isearch_toolbar_iface_init (gpointer g, gpointer iface_data)
{
	ModestISearchToolbarIface *iface = (ModestISearchToolbarIface *) g;

	iface->highlight_entry = modest_find_toolbar_highlight_entry;
	iface->set_label = modest_find_toolbar_set_label;
	iface->get_search = modest_find_toolbar_get_search;
}

static void 
modest_find_toolbar_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);		
}

static void
modest_find_toolbar_highlight_entry (ModestISearchToolbar *self,
				     gboolean get_focus)
{
	MODEST_FIND_TOOLBAR_GET_CLASS (self)->highlight_entry_func (self, get_focus);
}

static void
modest_find_toolbar_highlight_entry_default (ModestISearchToolbar *self,
					     gboolean get_focus)
{
	ModestFindToolbarPrivate *priv;

	priv = MODEST_FIND_TOOLBAR_GET_PRIVATE (self);

	gtk_widget_grab_focus (priv->entry);
}

static void
modest_find_toolbar_set_label (ModestISearchToolbar *self,
			       const gchar *label)
{
	MODEST_FIND_TOOLBAR_GET_CLASS (self)->set_label_func (self, label);
}

static void
modest_find_toolbar_set_label_default (ModestISearchToolbar *self,
				       const gchar *label)
{
	ModestFindToolbarPrivate *priv;

	priv = MODEST_FIND_TOOLBAR_GET_PRIVATE (self);
	gtk_label_set_text (GTK_LABEL (priv->label), label);
}

static const gchar *
modest_find_toolbar_get_search (ModestISearchToolbar *self)
{
	return MODEST_FIND_TOOLBAR_GET_CLASS (self)->get_search_func (self);
}

static const gchar *
modest_find_toolbar_get_search_default (ModestISearchToolbar *self)
{
	ModestFindToolbarPrivate *priv;

	priv = MODEST_FIND_TOOLBAR_GET_PRIVATE (self);

	return gtk_entry_get_text (GTK_ENTRY (priv->entry));
}

static void
on_entry_activate (GtkEntry *entry, gpointer userdata)
{
	g_signal_emit_by_name (G_OBJECT (userdata), "isearch-search");
}

static void
on_close_clicked (GtkToolButton *button, gpointer userdata)
{
	g_signal_emit_by_name (G_OBJECT (userdata), "isearch-close");
}

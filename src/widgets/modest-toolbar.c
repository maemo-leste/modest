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

#include <glib/gi18n.h>
#include "modest-toolbar.h"

/* 'private'/'protected' functions */
static void modest_toolbar_class_init (ModestToolbarClass *klass);
static void modest_toolbar_init       (ModestToolbar *obj);
static void modest_toolbar_finalize   (GObject *obj);

static void on_toolbutton_clicked (GtkToolButton *button, ModestToolbar *self);

/* list my signals  */
enum {
	BUTTON_CLICKED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestToolbarPrivate ModestToolbarPrivate;
struct _ModestToolbarPrivate {
	GtkTooltips	*tooltips;

};
#define MODEST_TOOLBAR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                            MODEST_TYPE_TOOLBAR, \
                                            ModestToolbarPrivate))
/* globals */
static GtkToolbarClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = {0};

GType
modest_toolbar_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestToolbarClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_toolbar_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestToolbar),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_toolbar_init,
			NULL
		};
		my_type = g_type_register_static (GTK_TYPE_TOOLBAR,
		                                  "ModestToolbar",
		                                  &my_info, 0);
	}
	return my_type;
}

static void
modest_toolbar_class_init (ModestToolbarClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_toolbar_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestToolbarPrivate));

	signals[BUTTON_CLICKED_SIGNAL] = 
		g_signal_new ("button_clicked",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (ModestToolbarClass, button_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
modest_toolbar_init (ModestToolbar *obj)
{
	ModestToolbarPrivate *priv;
	priv = MODEST_TOOLBAR_GET_PRIVATE(obj);

	priv->tooltips = NULL;
	
}

static void
modest_toolbar_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


/* don't free icon_name/label/tooltip, they're static */ 
static gboolean
data_for_button_id (ModestToolbarButton button_id,
		    gchar **icon_name, gchar **label, gchar **tooltip)
{
	switch (button_id) {
	case MODEST_TOOLBAR_BUTTON_MAIL_SEND:
		*label     = _("Send");
		*tooltip   = _("Send the current email message");
		*icon_name = MODEST_TOOLBAR_ICON_MAIL_SEND;
		break;
	case MODEST_TOOLBAR_BUTTON_NEW_MAIL:
		*label     = _("New mail");
		*tooltip   = _("Compose a new email message");
		*icon_name = MODEST_TOOLBAR_ICON_NEW_MAIL;
		break;
	case MODEST_TOOLBAR_BUTTON_SEND_RECEIVE:
		*label     = _("Send/Receive");
		*tooltip   = _("Send and receive messages");
		*icon_name = MODEST_TOOLBAR_ICON_SEND_RECEIVE;
		break;
	case MODEST_TOOLBAR_BUTTON_REPLY:
		*label     = _("Reply");
		*tooltip   = _("Reply to the selected email message");
		*icon_name = MODEST_TOOLBAR_ICON_REPLY;
		break;
	case MODEST_TOOLBAR_BUTTON_REPLY_ALL:
		*label    = _("Reply all");
		*tooltip   = _("Reply to all people the selected email was sent to");
		*icon_name = MODEST_TOOLBAR_ICON_REPLY_ALL;
		break;
	case MODEST_TOOLBAR_BUTTON_FORWARD:
		*label     = _("Forward");
		*tooltip   = _("Forward the selected email");
		*icon_name = MODEST_TOOLBAR_ICON_FORWARD;
		break;
	case MODEST_TOOLBAR_BUTTON_DELETE:
		*label     = _("Delete");
		*tooltip   = _("Delete the selected email message(s)");
		*icon_name = MODEST_TOOLBAR_ICON_DELETE;
		break;
	case MODEST_TOOLBAR_BUTTON_NEXT:
		*label     = _("Next");
		*tooltip   = _("Move to the next message");
		*icon_name = MODEST_TOOLBAR_ICON_NEXT;
		break;
	case MODEST_TOOLBAR_BUTTON_PREV:
		*label    = _("Previous");
		*tooltip   = _("Move to the previous message");
		*icon_name = MODEST_TOOLBAR_ICON_PREV;	
		break;
	case MODEST_TOOLBAR_BUTTON_STOP:
		*label    = _("Stop");
		*tooltip  = _("Stop whatever");	
		*icon_name = MODEST_TOOLBAR_ICON_STOP;
		break;
	default:
		g_printerr ("modest: not a valid button id: %d\n", 
			    button_id);
		return FALSE;
	}
	return TRUE;
}


static gboolean 
modest_toolbar_set_buttons (ModestToolbar *self, const GSList *buttons)
{
	const GSList *cursor;
	ModestToolbarPrivate *priv;

	priv = MODEST_TOOLBAR_GET_PRIVATE(self);
	
	priv->tooltips = gtk_tooltips_new ();
	gtk_tooltips_enable (priv->tooltips);
	gtk_toolbar_set_tooltips (GTK_TOOLBAR(self), TRUE);

	cursor = buttons;
	while (cursor) {
		ModestToolbarButton button_id =
			(ModestToolbarButton) GPOINTER_TO_INT(cursor->data);
		
		if (button_id == MODEST_TOOLBAR_SEPARATOR)
			gtk_toolbar_insert (GTK_TOOLBAR(self), 
					    gtk_separator_tool_item_new(), -1);
		else {
			gchar *icon_name, *label, *tooltip; /* don't free these */
			if (!data_for_button_id (button_id, &icon_name, &label, &tooltip))
				g_printerr ("modest: error getting data for toolbar button %d\n",
					    button_id);
			else {
				GtkWidget *icon = NULL;
				GtkToolItem *button = NULL;
				GdkPixbuf *pixbuf = NULL;
				
				pixbuf = modest_icon_factory_get_icon_at_size (icon_name, 24, 24);
				if (pixbuf)
					icon   = gtk_image_new_from_pixbuf ((GdkPixbuf*)pixbuf);
				button = gtk_tool_button_new (icon, label);
				g_object_set_data (G_OBJECT(button), "button_id",
						   GINT_TO_POINTER(button_id));
				g_signal_connect (G_OBJECT(button), "clicked", 
						  G_CALLBACK(on_toolbutton_clicked), self);

				gtk_tooltips_set_tip (priv->tooltips, GTK_WIDGET(button),
						      tooltip, NULL);
				gtk_widget_show_all (GTK_WIDGET(button));
				gtk_toolbar_insert (GTK_TOOLBAR(self), button, -1);
			}
		}
		cursor = cursor->next;
	}
	
	return TRUE; /* FIXME */   
}	


ModestToolbar*
modest_toolbar_new (const GSList *buttons)
{
	GObject *obj;

	obj = g_object_new(MODEST_TYPE_TOOLBAR, NULL);
	modest_toolbar_set_buttons (MODEST_TOOLBAR(obj), buttons);
	
	return MODEST_TOOLBAR(obj);
}

static void
on_toolbutton_clicked (GtkToolButton *button, ModestToolbar *self)
{
	gint button_id = GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(button), "button_id"));

	g_signal_emit (G_OBJECT(self), signals[BUTTON_CLICKED_SIGNAL],
		       0, button_id);
}



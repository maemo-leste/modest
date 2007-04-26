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
#include <gtk/gtk.h>
#include <widgets/modest-combo-box.h>
#include "modest-progress-bar-widget.h"
#include <string.h>

/* 'private'/'protected' functions */
static void modest_progress_bar_widget_class_init (ModestProgressBarWidgetClass *klass);
static void modest_progress_bar_widget_init       (ModestProgressBarWidget *obj);
static void modest_progress_bar_widget_finalize   (GObject *obj);

static void modest_progress_bar_add_operation    (ModestProgressObject *self,
						    ModestMailOperation  *mail_op);

static void modest_progress_bar_remove_operation (ModestProgressObject *self,
						    ModestMailOperation  *mail_op);

static void on_progress_changed                    (ModestMailOperation  *mail_op, 
						    ModestProgressBarWidget *self);

static gboolean     progressbar_clean        (GtkProgressBar *bar);

/* list my signals  */
/* enum { */
/* 	LAST_SIGNAL */
/* }; */

typedef struct _ObservableData ObservableData;
struct _ObservableData {
        guint signal_handler;
        ModestMailOperation *mail_op;
};

typedef struct _ModestProgressBarWidgetPrivate ModestProgressBarWidgetPrivate;
struct _ModestProgressBarWidgetPrivate {
        GSList              *observables;
        ModestMailOperation *current;

	GtkWidget *progress_bar;

};
#define MODEST_PROGRESS_BAR_WIDGET_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_PROGRESS_BAR_WIDGET, \
                                                 ModestProgressBarWidgetPrivate))
/* globals */
static GtkContainerClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

static void
modest_progress_object_init (gpointer g, gpointer iface_data)
{
	ModestProgressObjectIface *klass = (ModestProgressObjectIface *)g;

	klass->add_operation_func = modest_progress_bar_add_operation;
	klass->remove_operation_func = modest_progress_bar_remove_operation;
}


GType
modest_progress_bar_widget_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestProgressBarWidgetClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_progress_bar_widget_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestProgressBarWidget),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_progress_bar_widget_init,
			NULL
		};

		static const GInterfaceInfo modest_progress_object_info = 
		{
		  (GInterfaceInitFunc) modest_progress_object_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		my_type = g_type_register_static (GTK_TYPE_VBOX,
		                                  "ModestProgressBarWidget",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, MODEST_TYPE_PROGRESS_OBJECT, 
					     &modest_progress_object_info);
	}
	return my_type;
}

static void
modest_progress_bar_widget_class_init (ModestProgressBarWidgetClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_progress_bar_widget_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestProgressBarWidgetPrivate));

	/* signal definitions go here, e.g.: */
/* 	signals[DATA_CHANGED_SIGNAL] = */
/* 		g_signal_new ("data_changed", */
/* 			      G_TYPE_FROM_CLASS (klass), */
/* 			      G_SIGNAL_RUN_FIRST, */
/* 			      G_STRUCT_OFFSET(ModestProgressBarWidgetClass, data_changed), */
/* 			      NULL, NULL, */
/* 			      g_cclosure_marshal_VOID__VOID, */
/* 			      G_TYPE_NONE, 0); */
}

static void
modest_progress_bar_widget_init (ModestProgressBarWidget *obj)
{
 	ModestProgressBarWidgetPrivate *priv;
	
	priv = MODEST_PROGRESS_BAR_WIDGET_GET_PRIVATE(obj); 
	priv->progress_bar = NULL;
}

static void
destroy_observable_data (ObservableData *data)
{
	g_signal_handler_disconnect (data->mail_op, data->signal_handler);
	g_object_unref (data->mail_op);
}

static void
modest_progress_bar_widget_finalize (GObject *obj)
{
	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void 
modest_progress_bar_add_operation (ModestProgressObject *self,
				   ModestMailOperation  *mail_op)
{
	ModestProgressBarWidget *me;
	ObservableData *data;
	ModestProgressBarWidgetPrivate *priv;
	
	me = MODEST_PROGRESS_BAR_WIDGET (self);
	priv = MODEST_PROGRESS_BAR_WIDGET_GET_PRIVATE (me);

	data = g_malloc0 (sizeof (ObservableData));
	data->mail_op = g_object_ref (mail_op);
	data->signal_handler = g_signal_connect (data->mail_op, 
						 "progress-changed",
						 G_CALLBACK (on_progress_changed),
						 me);

	if (priv->observables == NULL) {
		priv->current = mail_op;
	}
	priv->observables = g_slist_append (priv->observables, data);
}

static gint
compare_observable_data (ObservableData *data1, ObservableData *data2)
{
	if (data1->mail_op == data2->mail_op)
		return 0;
	else 
		return 1;
}

static void 
modest_progress_bar_remove_operation (ModestProgressObject *self,
					ModestMailOperation  *mail_op)
{
	ModestProgressBarWidget *me;
	ModestProgressBarWidgetPrivate *priv;
	GSList *link;

	me = MODEST_PROGRESS_BAR_WIDGET (self);
	priv = MODEST_PROGRESS_BAR_WIDGET_GET_PRIVATE (me);

	link = g_slist_find_custom (priv->observables,
				    mail_op,
				    (GCompareFunc) compare_observable_data);
	
	/* Remove the item */
	if (link) {
		priv->observables = g_slist_remove_link (priv->observables, link);
		destroy_observable_data ((ObservableData *) link->data);
	}
	
	/* Update the current mail operation */
	if (priv->current == mail_op) {
		if (priv->observables)
			priv->current = ((ObservableData *) priv->observables->data)->mail_op;
		else
			priv->current = NULL;

		/* Refresh the view */
		progressbar_clean (GTK_PROGRESS_BAR (priv->progress_bar));
	}
}

static void 
on_progress_changed (ModestMailOperation  *mail_op, 
		     ModestProgressBarWidget *self)
{
	ModestProgressBarWidgetPrivate *priv;
	gboolean determined = FALSE;
	guint id = 0;

	priv = MODEST_PROGRESS_BAR_WIDGET_GET_PRIVATE (self);

	/* If the mail operation is the currently shown one */
	if (priv->current == mail_op) {
		gchar *msg = NULL;
		gint done = modest_mail_operation_get_task_done (mail_op);
		gint total = modest_mail_operation_get_task_total (mail_op);

		switch (id) {
		case STATUS_RECEIVING:		
			if (determined)
				msg = g_strdup_printf(_("mcen_me_receiving"), done, total);
			else 
				msg = g_strdup(_("mail_me_receiving"));
			break;
		case STATUS_SENDING:		
			if (determined)
				msg = g_strdup_printf(_("mcen_me_sending"), done, total);
			else 
				msg = g_strdup(_("mail_me_sending"));
			break;
			
		case STATUS_OPENING:		
			msg = g_strdup(_("mail_me_opening"));
			break;
		default:
			g_return_if_reached();
		}
		
		modest_progress_bar_widget_set_progress (self, msg, done, total);
		g_free (msg);
	}
}

static gboolean
progressbar_clean (GtkProgressBar *bar)
{
	gtk_progress_bar_set_fraction (bar, 0);
	gtk_progress_bar_set_text (bar, 0);
	return FALSE;
}


GtkWidget*
modest_progress_bar_widget_new ()
{
	GObject *obj;
	ModestProgressBarWidget *self;
	ModestProgressBarWidgetPrivate *priv;
	GtkRequisition req;
	

	obj = g_object_new(MODEST_TYPE_PROGRESS_BAR_WIDGET, NULL);
	self = MODEST_PROGRESS_BAR_WIDGET(obj);
	priv = MODEST_PROGRESS_BAR_WIDGET_GET_PRIVATE(self);
	
	/* Build GtkProgressBar */
	priv->progress_bar = gtk_progress_bar_new ();		
	req.width = 50;
	req.height = 64;
	gtk_progress_set_text_alignment (GTK_PROGRESS (priv->progress_bar), 0.0, 0.5);
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (priv->progress_bar), PANGO_ELLIPSIZE_END);
	gtk_widget_size_request (priv->progress_bar, &req);
	
	/* Add progress bar widget */
	gtk_box_pack_start (GTK_BOX(self), priv->progress_bar, TRUE, TRUE, 2);
	gtk_widget_show_all (GTK_WIDGET(self));

	return GTK_WIDGET(self);
}


void 
modest_progress_bar_widget_set_progress   (ModestProgressBarWidget *self,
					   const gchar *message,
					   gint done,
					   gint total)
{
	ModestProgressBarWidgetPrivate *priv;
	
	priv = MODEST_PROGRESS_BAR_WIDGET_GET_PRIVATE (self);
	
	/* Set progress */
	if (total != 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar),
					       (gdouble)done/(gdouble)total);
	else
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));

	/* Set text */
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), message);
}


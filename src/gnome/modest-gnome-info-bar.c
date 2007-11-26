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


#include "modest-gnome-info-bar.h"
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkstatusbar.h>
/* include other impl specific header files */

/* 'private'/'protected' functions */
static void modest_gnome_info_bar_class_init (ModestGnomeInfoBarClass *klass);
static void modest_gnome_info_bar_init       (ModestGnomeInfoBar *obj);
static void modest_gnome_info_bar_finalize   (GObject *obj);

static void modest_gnome_info_bar_add_operation    (ModestProgressObject *self,
						    ModestMailOperation  *mail_op);

static void modest_gnome_info_bar_remove_operation (ModestProgressObject *self,
						    ModestMailOperation  *mail_op);

static void on_progress_changed                    (ModestMailOperation  *mail_op, 
						    ModestMailOperationState *state,
						    ModestGnomeInfoBar *self);

static gboolean     progressbar_clean        (GtkProgressBar *bar);
static gboolean     statusbar_clean          (GtkStatusbar *bar);

/* list my signals  */
enum {
	/* MY_SIGNAL_1, */
	/* MY_SIGNAL_2, */
	LAST_SIGNAL
};

typedef struct _ObservableData ObservableData;
struct _ObservableData {
        guint signal_handler;
        ModestMailOperation *mail_op;
};

typedef struct _ModestGnomeInfoBarPrivate ModestGnomeInfoBarPrivate;
struct _ModestGnomeInfoBarPrivate {
        GSList              *observables;
        ModestMailOperation *current;

	GtkWidget *status_bar;
	GtkWidget *progress_bar;

	guint status_bar_timeout;
	guint progress_bar_timeout;
};

#define MODEST_GNOME_INFO_BAR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                              MODEST_TYPE_GNOME_INFO_BAR, \
                                              ModestGnomeInfoBarPrivate))
/* globals */
static GtkHBoxClass *parent_class = NULL;

/* uncomment the following if you have defined any signals */
/* static guint signals[LAST_SIGNAL] = {0}; */

static void
modest_progress_object_init (gpointer g, gpointer iface_data)
{
	ModestProgressObjectIface *klass = (ModestProgressObjectIface *)g;

	klass->add_operation_func = modest_gnome_info_bar_add_operation;
	klass->remove_operation_func = modest_gnome_info_bar_remove_operation;
}

GType
modest_gnome_info_bar_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestGnomeInfoBarClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_gnome_info_bar_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestGnomeInfoBar),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_gnome_info_bar_init,
			NULL
		};

		static const GInterfaceInfo modest_progress_object_info = 
		{
		  (GInterfaceInitFunc) modest_progress_object_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		my_type = g_type_register_static (GTK_TYPE_HBOX,
		                                  "ModestGnomeInfoBar",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, MODEST_TYPE_PROGRESS_OBJECT, 
					     &modest_progress_object_info);
	}
	return my_type;
}

static void
modest_gnome_info_bar_class_init (ModestGnomeInfoBarClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_gnome_info_bar_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestGnomeInfoBarPrivate));
}

static void
modest_gnome_info_bar_init (ModestGnomeInfoBar *obj)
{
	ModestGnomeInfoBarPrivate *priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE(obj);

	priv->observables = NULL;
	priv->current = NULL;

	/* Status bar */
	priv->status_bar = gtk_statusbar_new ();
        gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (priv->status_bar), FALSE);

	/* Progress bar */
	priv->progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), 1.0);
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (priv->progress_bar),
					PANGO_ELLIPSIZE_END);

	/* Timeouts */
	priv->status_bar_timeout = 0;
	priv->progress_bar_timeout = 0;

	/* Pack */
	gtk_box_pack_start (GTK_BOX (obj), priv->status_bar, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (obj), priv->progress_bar, FALSE, FALSE, 0);
}

static void
destroy_observable_data (ObservableData *data)
{
	g_signal_handler_disconnect (data->mail_op, data->signal_handler);
	g_object_unref (data->mail_op);
}

static void
modest_gnome_info_bar_finalize (GObject *obj)
{
	ModestGnomeInfoBarPrivate *priv;

	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE(obj);
	if (priv->observables) {
		GSList *tmp;

		for (tmp = priv->observables; tmp; tmp = g_slist_next (tmp)) {
			destroy_observable_data ((ObservableData *) tmp->data);
			g_free (tmp->data);
		}
		g_slist_free (priv->observables);
		priv->observables = NULL;
	}

	if (priv->status_bar_timeout > 0) {
		g_source_remove (priv->status_bar_timeout);
		priv->status_bar_timeout = 0;
	}

	if (priv->progress_bar_timeout > 0) {
		g_source_remove (priv->progress_bar_timeout);
		priv->progress_bar_timeout = 0;
	}

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}

GtkWidget *
modest_gnome_info_bar_new (void)
{
	return GTK_WIDGET (g_object_new (MODEST_TYPE_GNOME_INFO_BAR, NULL));
}

static void 
modest_gnome_info_bar_add_operation (ModestProgressObject *self,
				     ModestMailOperation  *mail_op)
{
	ModestGnomeInfoBar *me;
	ObservableData *data;
	ModestGnomeInfoBarPrivate *priv;

	me = MODEST_GNOME_INFO_BAR (self);
	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (me);

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
modest_gnome_info_bar_remove_operation (ModestProgressObject *self,
					ModestMailOperation  *mail_op)
{
	ModestGnomeInfoBar *me;
	ModestGnomeInfoBarPrivate *priv;
	GSList *link;

	me = MODEST_GNOME_INFO_BAR (self);
	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (me);

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
		     ModestMailOperationState *state,
		     ModestGnomeInfoBar *self)
{
	ModestGnomeInfoBarPrivate *priv;

	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (self);

	/* If the mail operation is the currently shown one */
	if (priv->current == mail_op) {
		gchar *msg = NULL;

		msg = g_strdup_printf ("Mail operation %d of %d",
				       modest_mail_operation_get_task_done (mail_op),
				       modest_mail_operation_get_task_total (mail_op));
		modest_gnome_info_bar_set_message (self, msg);
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

static gboolean
statusbar_clean (GtkStatusbar *bar)
{
	gtk_statusbar_push (bar, 0, "");
	return FALSE;
}

void 
modest_gnome_info_bar_set_message    (ModestGnomeInfoBar *self,
				      const gchar *message)
{
	ModestGnomeInfoBarPrivate *priv;

	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (self);

	/* Set a message. Clean it after 2.5 seconds */
	gtk_statusbar_push (GTK_STATUSBAR (priv->status_bar), 0, message);
	priv->status_bar_timeout = g_timeout_add (2500, 
						  (GSourceFunc) statusbar_clean, 
						  priv->status_bar);
}

void 
modest_gnome_info_bar_set_progress   (ModestGnomeInfoBar *self,
				      const gchar *message,
				      gint done,
				      gint total)
{
	ModestGnomeInfoBarPrivate *priv;

	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (self);

	/* Set progress */
	if (total != 0)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar),
					       (gdouble)done/(gdouble)total);
	else
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));

	/* Set text */
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), message);
}

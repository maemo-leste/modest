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

static void modest_gnome_info_bar_set_pulsating_mode (ModestGnomeInfoBar *self,
						      const gchar* msg,
						      gboolean is_pulsating);

static void on_progress_changed                    (ModestMailOperation  *mail_op, 
						    ModestMailOperationState *state,
						    ModestGnomeInfoBar *self);

static gboolean     progressbar_clean        (GtkProgressBar *bar);
static gboolean     statusbar_clean          (GtkStatusbar *bar);

#define MODEST_GNOME_INFO_BAR_PULSE_INTERVAL 125

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
	guint pulsating_timeout;
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
	ObservableData *tmp_data = NULL;
	gboolean is_current;

	me = MODEST_GNOME_INFO_BAR (self);
	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (me);

	is_current = (priv->current == mail_op);

	/* Find item */
	tmp_data = g_malloc0 (sizeof (ObservableData));
        tmp_data->mail_op = mail_op;
	link = g_slist_find_custom (priv->observables,
				    tmp_data,
				    (GCompareFunc) compare_observable_data);
	
	/* Remove the item */
	if (link) {
		ObservableData *ob_data = link->data;
		g_signal_handler_disconnect (ob_data->mail_op, ob_data->signal_handler);
		g_object_unref (ob_data->mail_op);
		g_free (ob_data);
		priv->observables = g_slist_delete_link (priv->observables, link);
		tmp_data->mail_op = NULL;
		link = NULL;
	}
	
	/* Update the current mail operation */
	if (is_current) {
		if (priv->observables)
			priv->current = ((ObservableData *) priv->observables->data)->mail_op;
		else
			priv->current = NULL;

		/* Refresh the view */
		modest_gnome_info_bar_set_pulsating_mode (me, NULL, FALSE);
		progressbar_clean (GTK_PROGRESS_BAR (priv->progress_bar));
	}
	
	/* free */
	g_free(tmp_data);
}

static void 
on_progress_changed (ModestMailOperation  *mail_op, 
		     ModestMailOperationState *state,
		     ModestGnomeInfoBar *self)
{
	ModestGnomeInfoBarPrivate *priv;
	gboolean determined = FALSE;

	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (self);

	/* If the mail operation is the currently shown one */
	if (priv->current == mail_op) {
		gchar *msg = NULL;
		
		determined = (state->done > 0 && state->total > 1) && 
			!(state->done == 1 && state->total == 100);

		switch (state->op_type) {
		case MODEST_MAIL_OPERATION_TYPE_RECEIVE:		
			if (determined)
 				msg = g_strdup_printf(_("mcen_me_receiving"),
						      state->done, state->total); 
			else 
 				msg = g_strdup(_("mail_me_receiving"));
			break;
		case MODEST_MAIL_OPERATION_TYPE_SEND:		
			if (determined)
				msg = g_strdup_printf(_("mcen_me_sending"), state->done,
						      state->total);
			else
				msg = g_strdup(_("mail_me_sending"));
			break;
			
		case MODEST_MAIL_OPERATION_TYPE_OPEN:		
			msg = g_strdup(_("mail_me_opening"));
			break;
		default:
			msg = g_strdup("");
		}
		
		/* If we have byte information use it */
		if ((state->bytes_done != 0) && (state->bytes_total != 0))
			modest_gnome_info_bar_set_progress (self, msg,
							    state->bytes_done,
							    state->bytes_total);
		else if ((state->done == 0) && (state->total == 0))
			modest_gnome_info_bar_set_pulsating_mode (self, msg, TRUE);
		else
			modest_gnome_info_bar_set_progress (self, msg,
							    state->done,
							    state->total);
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

static gboolean
modest_gnome_info_bar_is_pulsating (ModestGnomeInfoBar *self)
{
	ModestGnomeInfoBarPrivate *priv;

	g_return_val_if_fail (MODEST_IS_GNOME_INFO_BAR(self), FALSE);

	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (self);
	
	return priv->pulsating_timeout != 0;
}

void 
modest_gnome_info_bar_set_progress   (ModestGnomeInfoBar *self,
				      const gchar *message,
				      gint done,
				      gint total)
{
	ModestGnomeInfoBarPrivate *priv;
	gboolean determined = FALSE;

	g_return_if_fail (MODEST_IS_GNOME_INFO_BAR(self));
	g_return_if_fail (done <= total);
	
	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (self);

	if (modest_gnome_info_bar_is_pulsating (self))
		modest_gnome_info_bar_set_pulsating_mode (self, NULL, FALSE);

	/* Set progress. Tinymail sometimes returns us 1/100 when it
	   does not have any clue, NOTE that 1/100 could be also a
	   valid progress (we will loose it), but it will be recovered
	   once the done is greater than 1 */
	determined = (done > 0 && total > 1) && 
		!(done == 1 && total == 100);
	if (!determined) {
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));
	} else {
		gdouble percent = 0;
		if (total != 0) /* Avoid division by zero. */
			percent = (gdouble)done/(gdouble)total;

 		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar),
					       percent);
	}
	
	/* Set text */
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), 
				   (message && message[0] != '\0')?message:" ");
}

static gboolean
do_pulse (gpointer data)
{
	ModestGnomeInfoBarPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_GNOME_INFO_BAR(data), FALSE);
	
	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (data);
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));
	return TRUE;
}

static void
modest_gnome_info_bar_set_pulsating_mode (ModestGnomeInfoBar *self,
					  const gchar* msg,
					  gboolean is_pulsating)
{
	ModestGnomeInfoBarPrivate *priv;

	g_return_if_fail (MODEST_IS_GNOME_INFO_BAR (self));

	priv = MODEST_GNOME_INFO_BAR_GET_PRIVATE (self);
	
	if (msg != NULL)
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), msg);
	
	if (is_pulsating == (priv->pulsating_timeout != 0))
		return;
	else if (is_pulsating && (priv->pulsating_timeout == 0)) {
		/* enable */
		priv->pulsating_timeout = g_timeout_add (MODEST_GNOME_INFO_BAR_PULSE_INTERVAL,
							 do_pulse, self);
	} else if (!is_pulsating && (priv->pulsating_timeout != 0)) {
		/* disable */
		g_source_remove (priv->pulsating_timeout);
		priv->pulsating_timeout = 0;
	}
}

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

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "widgets/modest-progress-bar.h"
#include "modest-tny-account.h"
#include "modest-platform.h"
#include "modest-runtime.h"

/* 'private'/'protected' functions */
static void modest_progress_bar_class_init (ModestProgressBarClass *klass);
static void modest_progress_bar_init       (ModestProgressBar *obj);
static void modest_progress_bar_finalize   (GObject *obj);

static void modest_progress_bar_add_operation    (ModestProgressObject *self,
						    ModestMailOperation  *mail_op);

static void modest_progress_bar_remove_operation (ModestProgressObject *self,
						    ModestMailOperation  *mail_op);

static void modest_progress_bar_cancel_current_operation (ModestProgressObject *self);

static void modest_progress_bar_cancel_all_operations    (ModestProgressObject *self);

static guint modest_progress_bar_num_pending_operations (ModestProgressObject *self);

static void on_progress_changed                    (ModestMailOperation  *mail_op, 
						    ModestMailOperationState *state,
						    ModestProgressBar *self);

static gboolean     progressbar_clean        (GtkProgressBar *bar);

static gboolean modest_progress_bar_is_pulsating (ModestProgressBar *self);

static void modest_progress_bar_set_pulsating_mode (ModestProgressBar *self,
                                                           const gchar* msg,
                                                           gboolean is_pulsating);

static gchar *progress_string (ModestMailOperationTypeOperation op_type, guint done, guint total);

#define XALIGN 0.5
#define YALIGN 0.5
#define XSPACE 1
#define YSPACE 0

#define LOWER 0
#define UPPER 150

#define MODEST_PROGRESS_BAR_PULSE_INTERVAL 125

/* list my signals  */
/* enum { */
/* 	LAST_SIGNAL */
/* }; */

typedef struct _ObservableData ObservableData;
struct _ObservableData {
        guint signal_handler;
        ModestMailOperation *mail_op;
};

typedef struct _ModestProgressBarPrivate ModestProgressBarPrivate;
struct _ModestProgressBarPrivate {
	GSList              *observables;
	ModestMailOperation *current;
	guint count;
	GtkWidget *progress_bar;
	guint pulsating_timeout;
};
#define MODEST_PROGRESS_BAR_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                                 MODEST_TYPE_PROGRESS_BAR_WIDGET, \
                                                 ModestProgressBarPrivate))

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
	klass->cancel_current_operation_func = modest_progress_bar_cancel_current_operation;
	klass->cancel_all_operations_func = modest_progress_bar_cancel_all_operations;
	klass->num_pending_operations_func = modest_progress_bar_num_pending_operations;
}


GType
modest_progress_bar_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestProgressBarClass),
			NULL,		/* base init */
			NULL,		/* base finalize */
			(GClassInitFunc) modest_progress_bar_class_init,
			NULL,		/* class finalize */
			NULL,		/* class data */
			sizeof(ModestProgressBar),
			1,		/* n_preallocs */
			(GInstanceInitFunc) modest_progress_bar_init,
			NULL
		};

		static const GInterfaceInfo modest_progress_object_info = 
		{
		  (GInterfaceInitFunc) modest_progress_object_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		my_type = g_type_register_static (GTK_TYPE_VBOX,
		                                  "ModestProgressBar",
		                                  &my_info, 0);

		g_type_add_interface_static (my_type, MODEST_TYPE_PROGRESS_OBJECT, 
					     &modest_progress_object_info);
	}
	return my_type;
}

static void
modest_progress_bar_class_init (ModestProgressBarClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class            = g_type_class_peek_parent (klass);
	gobject_class->finalize = modest_progress_bar_finalize;

	g_type_class_add_private (gobject_class, sizeof(ModestProgressBarPrivate));
}

static void
modest_progress_bar_init (ModestProgressBar *self)
{
	
	ModestProgressBarPrivate *priv;
	GtkWidget *align = NULL;
	GtkRequisition req;
	GtkAdjustment *adj;

	priv = MODEST_PROGRESS_BAR_GET_PRIVATE(self);

	/* Alignment */
	align = gtk_alignment_new(XALIGN, YALIGN, XSPACE, YSPACE);

	/* Build GtkProgressBar */
	adj = (GtkAdjustment *) gtk_adjustment_new (0, LOWER, UPPER, 0, 0, 0);
	priv->progress_bar = gtk_progress_bar_new ();
	g_object_set (priv->progress_bar, "adjustment", adj, NULL);
	req.width = 228;
	req.height = 64;
	gtk_progress_set_text_alignment (GTK_PROGRESS (priv->progress_bar), 0, 0.5);
	gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (priv->progress_bar), PANGO_ELLIPSIZE_END);
	gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.25);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), " ");
	gtk_widget_size_request (priv->progress_bar, &req);
	gtk_container_add (GTK_CONTAINER (align), priv->progress_bar);
	gtk_widget_size_request (align, &req);

	/* Add progress bar widget */	
	gtk_box_pack_start (GTK_BOX(self), align, TRUE, TRUE, 0);
	gtk_widget_show_all (GTK_WIDGET(self));       
	
	priv->pulsating_timeout = 0;
}

static void
modest_progress_bar_finalize (GObject *obj)
{
	ModestProgressBarPrivate *priv;

	priv = MODEST_PROGRESS_BAR_GET_PRIVATE(obj);
	if (priv->observables) {
		GSList *tmp;

		for (tmp = priv->observables; tmp; tmp = g_slist_next (tmp)) {
			ObservableData *ob_data = tmp->data;
			g_signal_handler_disconnect (ob_data->mail_op, ob_data->signal_handler);
			g_object_unref (ob_data->mail_op);
			g_free (ob_data);
		}
		g_slist_free (priv->observables);
		priv->observables = NULL;
	}
	
	/* remove timeout */
	if (priv->pulsating_timeout != 0)
		g_source_remove (priv->pulsating_timeout);

	G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void 
modest_progress_bar_add_operation (ModestProgressObject *self,
				   ModestMailOperation  *mail_op)
{
	ModestProgressBar *me = NULL;
	ObservableData *data = NULL;
	ModestProgressBarPrivate *priv = NULL;
	
	me = MODEST_PROGRESS_BAR (self);
	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (me);

	data = g_malloc0 (sizeof (ObservableData));
	data->mail_op = g_object_ref (mail_op);
	data->signal_handler = g_signal_connect (data->mail_op,
						 "progress-changed",
						 G_CALLBACK (on_progress_changed),
						 me);
	/* Set curent operation */
	if (priv->current == NULL) {
		priv->current = mail_op;
		
		priv->count = 0;
		
		/* Call progress_change handler to initialize progress message */
/* 		modest_progress_bar_set_undetermined_progress (MODEST_PROGRESS_BAR(self), mail_op); */
	}

	/* Add operation to obserbable objects list */
	priv->observables = g_slist_prepend (priv->observables, data);
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
	ModestProgressBar *me;
	ModestProgressBarPrivate *priv;
	GSList *link;
	ObservableData *tmp_data = NULL;
	gboolean is_current;

	me = MODEST_PROGRESS_BAR (self);
	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (me);

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
		if (priv->observables) {
			gchar *msg;
			priv->current = ((ObservableData *) priv->observables->data)->mail_op;
			msg = progress_string (modest_mail_operation_get_type_operation (MODEST_MAIL_OPERATION (priv->current)), 0, 0);
			modest_progress_bar_set_pulsating_mode (me, msg, TRUE);
			g_free (msg);
		} else {
			priv->current = NULL;
			modest_progress_bar_set_pulsating_mode (me, NULL, FALSE);
			progressbar_clean (GTK_PROGRESS_BAR (priv->progress_bar));
		}

		/* Refresh the view */
	}
	
	/* free */
	g_free(tmp_data);
}

static guint
modest_progress_bar_num_pending_operations (ModestProgressObject *self)
{
	ModestProgressBar *me;
	ModestProgressBarPrivate *priv;
	
	me = MODEST_PROGRESS_BAR (self);
	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (me);
	
	return g_slist_length(priv->observables);
}

static void 
modest_progress_bar_cancel_current_operation (ModestProgressObject *self)
{
	ModestProgressBar *me;
	ModestProgressBarPrivate *priv;

	me = MODEST_PROGRESS_BAR (self);
	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (me);

	if (priv->current == NULL) return;

	modest_mail_operation_cancel (priv->current);
}

static void 
modest_progress_bar_cancel_all_operations (ModestProgressObject *self)
{

	/* Cancel all the mail operations */
	modest_mail_operation_queue_cancel_all (modest_runtime_get_mail_operation_queue ());
}

static gchar *
progress_string (ModestMailOperationTypeOperation op_type, guint done, guint total)
{
	gboolean determined = FALSE;

	gchar *msg = NULL;

	determined = (done > 0 && total > 1) && 
		!(done == 1 && total == 100);

	switch (op_type) {
	case MODEST_MAIL_OPERATION_TYPE_SEND_AND_RECEIVE:		
	case MODEST_MAIL_OPERATION_TYPE_RECEIVE:		
		if (determined)
			msg = g_strdup_printf(_("mcen_me_receiving"),
					      done, total); 
		else 
			msg = g_strdup(_("mail_me_receiving"));
		break;
	case MODEST_MAIL_OPERATION_TYPE_SEND:		
		if (determined)
			msg = g_strdup_printf(_("mcen_me_sending"), done,
					      total);
		else
			msg = g_strdup(_("mail_me_sending"));
		break;
		
	default:
		msg = g_strdup("");
	}

	return msg;
}

static void 
on_progress_changed (ModestMailOperation  *mail_op, 
		     ModestMailOperationState *state,
		     ModestProgressBar *self)
{
	ModestProgressBarPrivate *priv;

	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (self);

	/* If the mail operation is the currently shown one */
	if (priv->current == mail_op) {
		gchar *msg = NULL;
		
		msg = progress_string (state->op_type, state->done, state->total);
		
		/* If we have byte information use it */
		if ((state->bytes_done != 0) && (state->bytes_total != 0))
			modest_progress_bar_set_progress (self, msg,
								 state->bytes_done,
								 state->bytes_total);
		else if ((state->done == 0) && (state->total == 0))
			modest_progress_bar_set_pulsating_mode (self, msg, TRUE);
		else
			modest_progress_bar_set_progress (self, msg,
								 state->done,
								 state->total);
		g_free (msg);
	}
}

static gboolean
progressbar_clean (GtkProgressBar *bar)
{

	gtk_progress_bar_set_fraction (bar, 0);
	gtk_progress_bar_set_text (bar, " ");

	return FALSE;
}


GtkWidget*
modest_progress_bar_new ()
{
	return GTK_WIDGET (g_object_new (MODEST_TYPE_PROGRESS_BAR_WIDGET, NULL));
}


void 
modest_progress_bar_set_progress (ModestProgressBar *self,
					 const gchar *message,
					 gint done,
					 gint total)
{
	ModestProgressBarPrivate *priv;
	gboolean determined = FALSE;

	g_return_if_fail (MODEST_IS_PROGRESS_BAR_WIDGET(self));
	g_return_if_fail (done <= total);
	
	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (self);

	priv->count++;

	if (modest_progress_bar_is_pulsating (self))
		modest_progress_bar_set_pulsating_mode (self, NULL, FALSE);

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


void
modest_progress_bar_set_undetermined_progress (ModestProgressBar *self,
						      ModestMailOperation *mail_op)
{
	ModestMailOperationState *state = NULL;

	state = g_malloc0(sizeof(ModestMailOperationState));
	state->done = 0;
	state->total = 0;
	state->op_type = modest_mail_operation_get_type_operation (mail_op);
	on_progress_changed (mail_op, state, self);
	g_free(state);
}

/* this has to be explicitly removed */
static gboolean
do_pulse (gpointer data)
{
	ModestProgressBarPrivate *priv;
	
	g_return_val_if_fail (MODEST_IS_PROGRESS_BAR_WIDGET(data), FALSE);
	
	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (data);
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (priv->progress_bar));
	return TRUE;
}

gboolean
modest_progress_bar_is_pulsating (ModestProgressBar *self)
{
	ModestProgressBarPrivate *priv;

	g_return_val_if_fail (MODEST_IS_PROGRESS_BAR_WIDGET(self), FALSE);

	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (self);
	
	return priv->pulsating_timeout != 0;
}

void
modest_progress_bar_set_pulsating_mode (ModestProgressBar *self,
                                               const gchar* msg,
                                               gboolean is_pulsating)
{
	ModestProgressBarPrivate *priv;

	g_return_if_fail (MODEST_IS_PROGRESS_BAR_WIDGET(self));

	priv = MODEST_PROGRESS_BAR_GET_PRIVATE (self);
	
	if (msg != NULL)
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), msg);
	
	if (is_pulsating == (priv->pulsating_timeout != 0))
		return;
	else if (is_pulsating && (priv->pulsating_timeout == 0)) {
		/* enable */
		priv->pulsating_timeout = g_timeout_add (MODEST_PROGRESS_BAR_PULSE_INTERVAL,
				do_pulse, self);
	} else if (!is_pulsating && (priv->pulsating_timeout != 0)) {
		/* disable */
		g_source_remove (priv->pulsating_timeout);
		priv->pulsating_timeout = 0;
	}
}

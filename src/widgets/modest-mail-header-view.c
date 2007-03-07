/* Copyright (c) 2007, Nokia Corporation
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

#include <config.h>
//#include <glib/gi18n-lib.h>

#include <string.h>
#include <gtk/gtk.h>
#include <modest-text-utils.h>
#include <modest-mail-header-view.h>
#include <modest-tny-folder.h>

static GObjectClass *parent_class = NULL;

/* signals */
enum {
	RECPT_ACTIVATED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _ModestMailHeaderViewPriv ModestMailHeaderViewPriv;

struct _ModestMailHeaderViewPriv
{
	GtkWidget    *fromto_label;
	GtkWidget    *fromto_contents;
	GtkWidget    *main_vbox;
	GtkWidget    *expander;
	GtkWidget    *headers_vbox;
	GtkSizeGroup *labels_size_group;
	gboolean     is_sent;
	TnyHeader    *header;
};

#define MODEST_MAIL_HEADER_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_MAIL_HEADER_VIEW, ModestMailHeaderViewPriv))

static guint signals[LAST_SIGNAL] = {0};

static void
activate_recpt (GtkWidget *recpt_view, const gchar *address, gpointer user_data)
{
	ModestMailHeaderView * view = MODEST_MAIL_HEADER_VIEW (user_data);

	g_signal_emit (G_OBJECT (view), signals[RECPT_ACTIVATED_SIGNAL], 0, address);
}

static void
add_header (ModestMailHeaderView *widget, const gchar *field, const gchar *value)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (widget);
	GtkWidget *hbox;
	GtkWidget *label_field, *label_value;

	hbox = gtk_hbox_new (FALSE, 12);
	label_field = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label_field), field);
	gtk_misc_set_alignment (GTK_MISC (label_field), 0.0, 0.0);
	label_value = gtk_label_new (NULL);
	gtk_label_set_text (GTK_LABEL (label_value), value);
	gtk_label_set_selectable (GTK_LABEL (label_value), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label_value), 0.0, 0.0);

	gtk_box_pack_start (GTK_BOX (hbox), label_field, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label_value, TRUE, TRUE, 0);
	gtk_size_group_add_widget (priv->labels_size_group, label_field);
	
	gtk_box_pack_start (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	
}

static void
add_recpt_header (ModestMailHeaderView *widget, const gchar *field, const gchar *value)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (widget);
	GtkWidget *hbox;
	GtkWidget *label_field, *label_value;

	hbox = gtk_hbox_new (FALSE, 12);
	label_field = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label_field), field);
	gtk_misc_set_alignment (GTK_MISC (label_field), 0.0, 0.0);
	label_value = modest_recpt_view_new ();
	modest_recpt_view_set_recipients (MODEST_RECPT_VIEW(label_value), value);
	g_signal_connect (G_OBJECT (label_value), "activate", G_CALLBACK (activate_recpt), widget);

	gtk_box_pack_start (GTK_BOX (hbox), label_field, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label_value, TRUE, TRUE, 0);
	gtk_size_group_add_widget (priv->labels_size_group, label_field);
	
	gtk_box_pack_start (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
}

static void
clean_headers (GtkWidget *vbox)
{
	GList *headers_list, *node;

	headers_list = gtk_container_get_children (GTK_CONTAINER (vbox));

	for (node = headers_list; node != NULL; node = g_list_next (node)) {
		gtk_widget_destroy (GTK_WIDGET (node->data));
	}
	g_list_free (headers_list);
}

static void 
modest_mail_header_view_set_header (TnyHeaderView *self, TnyHeader *header)
{
	MODEST_MAIL_HEADER_VIEW_GET_CLASS (self)->set_header_func (self, header);
	return;
}

static void
modest_mail_header_view_update_is_sent (TnyHeaderView *self)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	TnyFolder *folder = NULL;
     
	priv->is_sent = FALSE;

	if (priv->header == NULL)
		return;
	
	folder = tny_header_get_folder (priv->header);

	if (folder) {
		TnyFolderType folder_type;
		folder_type = tny_folder_get_folder_type (folder);
		if (folder_type == TNY_FOLDER_TYPE_NORMAL || folder_type == TNY_FOLDER_TYPE_UNKNOWN) {
			const gchar *fname = tny_folder_get_name (folder);
			folder_type = modest_tny_folder_guess_folder_type_from_name (fname);
		}

		switch (folder_type) {
		case TNY_FOLDER_TYPE_OUTBOX:
		case TNY_FOLDER_TYPE_SENT:
		case TNY_FOLDER_TYPE_DRAFTS:
			priv->is_sent = TRUE;
			break;
		default:
			priv->is_sent = FALSE;
		}

		g_object_unref (folder);
	}
}

static void 
modest_mail_header_view_set_header_default (TnyHeaderView *self, TnyHeader *header)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (header)
		g_assert (TNY_IS_HEADER (header));

	if (G_LIKELY (priv->header))
 		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	clean_headers (priv->headers_vbox);

	if (header && G_IS_OBJECT (header))
	{
		const gchar *to, *from, *subject, *bcc, *cc;

		g_object_ref (G_OBJECT (header)); 
		priv->header = header;

		modest_mail_header_view_update_is_sent (self);


		to = tny_header_get_to (header);
		from = tny_header_get_from (header);
		subject = tny_header_get_subject (header);
		cc = tny_header_get_cc (header);
		bcc = tny_header_get_bcc (header);

		if (subject)
			add_header (MODEST_MAIL_HEADER_VIEW (self), _("<b>Subject:</b>"), subject);
		if (priv->is_sent) {
			gchar *sent = modest_text_utils_get_display_date (tny_header_get_date_sent (header));
			gtk_label_set_markup (GTK_LABEL (priv->fromto_label), _("<b>To:</b>"));
			if (to)
				modest_recpt_view_set_recipients (MODEST_RECPT_VIEW (priv->fromto_contents), to);
			add_header (MODEST_MAIL_HEADER_VIEW (self), _("<b>Sent:</b>"), sent);
			g_free (sent);
		} else {
			gchar *received = modest_text_utils_get_display_date (tny_header_get_date_received (header));
			gtk_label_set_markup (GTK_LABEL (priv->fromto_label), _("<b>From:</b>"));
			if (from)
				modest_recpt_view_set_recipients (MODEST_RECPT_VIEW (priv->fromto_contents), from);
			add_header (MODEST_MAIL_HEADER_VIEW (self), _("<b>Received:</b>"), received);
			g_free (received);
		}
		if (cc)
			add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("<b>Cc:</b>"), cc);
		if (bcc)
			add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("<b>Bcc:</b>"), bcc);
	}

	gtk_widget_show_all (GTK_WIDGET (self));

	return;
}

static void 
modest_mail_header_view_clear (TnyHeaderView *self)
{
	MODEST_MAIL_HEADER_VIEW_GET_CLASS (self)->clear_func (self);
	return;
}

static void 
modest_mail_header_view_clear_default (TnyHeaderView *self)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (G_LIKELY (priv->header))
		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	clean_headers (priv->headers_vbox);

	gtk_label_set_text (GTK_LABEL (priv->fromto_label), "");
	modest_recpt_view_set_recipients (MODEST_RECPT_VIEW (priv->fromto_contents), "");

	gtk_widget_hide (GTK_WIDGET(self));

	return;
}

static void
expander_activate (GtkWidget *expander, ModestMailHeaderView *header_view)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (header_view);

	gtk_widget_queue_resize (GTK_WIDGET (header_view));
	gtk_widget_queue_draw (GTK_WIDGET (header_view));

	if (gtk_expander_get_expanded (GTK_EXPANDER (expander))) {
		if (gtk_widget_get_parent (priv->headers_vbox) == NULL) {
			gtk_box_pack_start (GTK_BOX(priv->main_vbox), priv->headers_vbox, TRUE, TRUE, 0);
			gtk_widget_show_all (GTK_WIDGET (priv->headers_vbox));
		}
	} else {
		if (gtk_widget_get_parent (priv->headers_vbox) != NULL) {
			gtk_container_remove (GTK_CONTAINER (priv->main_vbox), priv->headers_vbox);
		}
	}
	gtk_widget_queue_resize (GTK_WIDGET (priv->expander));
	gtk_widget_queue_draw (GTK_WIDGET (priv->expander));
}

/**
 * modest_mail_header_view_new:
 *
 * Return value: a new #ModestHeaderView instance implemented for Gtk+
 **/
TnyHeaderView*
modest_mail_header_view_new (void)
{
	ModestMailHeaderView *self = g_object_new (MODEST_TYPE_MAIL_HEADER_VIEW, NULL);

	return TNY_HEADER_VIEW (self);
}

static void
modest_mail_header_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestMailHeaderView *self = (ModestMailHeaderView *)instance;
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	GtkWidget *fromto_hbox = NULL;
	GtkSizeGroup *expander_group = NULL;

	priv->header = NULL;

	priv->expander = gtk_expander_new (NULL);
	priv->main_vbox = gtk_vbox_new (FALSE, 1);
	gtk_box_pack_start (GTK_BOX (instance), priv->expander, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (instance), priv->main_vbox, TRUE, TRUE, 0);
	g_signal_connect_after (G_OBJECT (priv->expander), "activate", G_CALLBACK (expander_activate), instance);

	fromto_hbox = gtk_hbox_new (FALSE, 12);
	priv->fromto_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->fromto_label), 0.0, 0.0);
	priv->fromto_contents = modest_recpt_view_new ();
	g_signal_connect (G_OBJECT (priv->fromto_contents), "activate", G_CALLBACK (activate_recpt), instance);

	gtk_box_pack_start (GTK_BOX (fromto_hbox), priv->fromto_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (fromto_hbox), priv->fromto_contents, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), fromto_hbox, FALSE, FALSE, 0);

	priv->labels_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (priv->labels_size_group, priv->fromto_label);
	
	priv->headers_vbox = gtk_vbox_new (FALSE, 0);
	g_object_ref (priv->headers_vbox);

	expander_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (expander_group, priv->headers_vbox);
	gtk_size_group_add_widget (expander_group, fromto_hbox);
	g_object_unref (expander_group);

	gtk_container_set_reallocate_redraws (GTK_CONTAINER (instance), TRUE);

	priv->is_sent = FALSE;

	return;
}

static void
modest_mail_header_view_finalize (GObject *object)
{
	ModestMailHeaderView *self = (ModestMailHeaderView *)object;	
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (G_LIKELY (priv->header))
		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	if (G_LIKELY (priv->headers_vbox))
		g_object_unref (G_OBJECT (priv->headers_vbox));

	priv->headers_vbox = NULL;

	g_object_unref (priv->labels_size_group);

	(*parent_class->finalize) (object);

	return;
}

static void
tny_header_view_init (gpointer g, gpointer iface_data)
{
	TnyHeaderViewIface *klass = (TnyHeaderViewIface *)g;

	klass->set_header_func = modest_mail_header_view_set_header;
	klass->clear_func = modest_mail_header_view_clear;

	return;
}

static void 
modest_mail_header_view_class_init (ModestMailHeaderViewClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	klass->set_header_func = modest_mail_header_view_set_header_default;
	klass->clear_func = modest_mail_header_view_clear_default;

	object_class->finalize = modest_mail_header_view_finalize;

	klass->recpt_activated = NULL;

	g_type_class_add_private (object_class, sizeof (ModestMailHeaderViewPriv));

 	signals[RECPT_ACTIVATED_SIGNAL] =
 		g_signal_new ("recpt-activated",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET(ModestMailHeaderViewClass, recpt_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, 
			      G_TYPE_STRING);


	return;
}

GType 
modest_mail_header_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestMailHeaderViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_mail_header_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestMailHeaderView),
		  0,      /* n_preallocs */
		  modest_mail_header_view_instance_init    /* instance_init */
		};

		static const GInterfaceInfo tny_header_view_info = 
		{
		  (GInterfaceInitFunc) tny_header_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
			"ModestMailHeaderView",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_HEADER_VIEW, 
			&tny_header_view_info);

	}

	return type;
}

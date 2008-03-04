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
	GtkWidget    *subject_box;
	GtkWidget    *priority_icon;
	GtkSizeGroup *labels_size_group;
	gboolean     is_outgoing;
	gboolean     is_draft;
	TnyHeader    *header;
	TnyHeaderFlags priority_flags;
};

#define MODEST_MAIL_HEADER_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_MAIL_HEADER_VIEW, ModestMailHeaderViewPriv))

static guint signals[LAST_SIGNAL] = {0};

static void
add_date_time_header (ModestMailHeaderView *mail_header, const gchar *name, time_t date)
{
	const guint BUF_SIZE = 64; 
	gchar date_buf [BUF_SIZE];
	gchar time_buf [BUF_SIZE];

	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (mail_header);
	GtkWidget *hbox, *date_hbox, *time_hbox;
	GtkWidget *label;

	modest_text_utils_strftime (date_buf, BUF_SIZE, "%x", date);
	modest_text_utils_strftime (time_buf, BUF_SIZE, "%X", date);

	hbox = gtk_hbox_new (FALSE, 48);
	date_hbox = gtk_hbox_new (FALSE, 12);
	time_hbox = gtk_hbox_new (FALSE, 12);

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), name);
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);
	gtk_box_pack_start (GTK_BOX (date_hbox), label, FALSE, FALSE, 0);
	gtk_size_group_add_widget (priv->labels_size_group, label);

	label = gtk_label_new(date_buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (date_hbox), label, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), date_hbox, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_label_set_markup (GTK_LABEL (label), _("mail_va_time"));
	gtk_box_pack_start (GTK_BOX (time_hbox), label, FALSE, FALSE, 0);

	label = gtk_label_new(time_buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (time_hbox), label, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), time_hbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
}

static void
activate_recpt (GtkWidget *recpt_view, const gchar *address, gpointer user_data)
{
	ModestMailHeaderView * view = MODEST_MAIL_HEADER_VIEW (user_data);

	g_signal_emit (G_OBJECT (view), signals[RECPT_ACTIVATED_SIGNAL], 0, address);
}

#if 0 /* This function is not used. murrayc. */
static void
add_header (ModestMailHeaderView *widget, const gchar *field, const gchar *value)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (widget);
	GtkWidget *hbox;
	GtkWidget *label_field, *label_value;
	GtkWidget *scroll_text;
	GtkTextBuffer *text_buffer;

	hbox = gtk_hbox_new (FALSE, 12);
	label_field = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label_field), field);
	gtk_misc_set_alignment (GTK_MISC (label_field), 1.0, 0.0);
	scroll_text = modest_scroll_text_new (NULL, 2);
	label_value = (GtkWidget *) modest_scroll_text_get_text_view (MODEST_SCROLL_TEXT (scroll_text));
	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (label_value));
	gtk_text_buffer_set_text (text_buffer, value, -1);

	gtk_box_pack_start (GTK_BOX (hbox), label_field, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), scroll_text, TRUE, TRUE, 0);
	gtk_size_group_add_widget (priv->labels_size_group, label_field);
	
	gtk_box_pack_start (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	
}
#endif


static void
add_recpt_header (ModestMailHeaderView *widget, const gchar *field, const gchar *value)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (widget);
	GtkWidget *hbox;
	GtkWidget *label_field, *label_value;

	hbox = gtk_hbox_new (FALSE, 12);
	label_field = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label_field), field);
	gtk_misc_set_alignment (GTK_MISC (label_field), 1.0, 0.0);
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
modest_mail_header_view_update_is_outgoing (TnyHeaderView *self)
{
	ModestMailHeaderViewPriv *priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	TnyFolder *folder = NULL;
     
	priv->is_outgoing = FALSE;

	if (priv->header == NULL)
		return;
	
	folder = tny_header_get_folder (priv->header);

	if (folder) {
		TnyFolderType folder_type = tny_folder_get_folder_type (folder);

		switch (folder_type) {
		case TNY_FOLDER_TYPE_DRAFTS:
		case TNY_FOLDER_TYPE_OUTBOX:
		case TNY_FOLDER_TYPE_SENT:
			priv->is_outgoing = TRUE;
			break;
		default:
			priv->is_outgoing = FALSE;
		}
		priv->is_draft = (folder_type == TNY_FOLDER_TYPE_DRAFTS);

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
		GtkWidget *subject_label;

		g_object_ref (G_OBJECT (header)); 
		priv->header = header;

		modest_mail_header_view_update_is_outgoing (self);


		to = tny_header_get_to (header);
		from = tny_header_get_from (header);
		subject = tny_header_get_subject (header);
		cc = tny_header_get_cc (header);
		bcc = tny_header_get_bcc (header);

		priv->subject_box = gtk_hbox_new (FALSE, 0);
		subject_label = gtk_label_new (NULL);
		if (subject && (subject[0] != '\0'))
			gtk_label_set_text (GTK_LABEL (subject_label), subject);
		else
			gtk_label_set_text (GTK_LABEL (subject_label), _("mail_va_no_subject"));
		gtk_label_set_single_line_mode (GTK_LABEL (subject_label), TRUE);
		gtk_label_set_ellipsize (GTK_LABEL (subject_label), PANGO_ELLIPSIZE_END);
		gtk_label_set_selectable (GTK_LABEL (subject_label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (subject_label), 0.0, 0.0);

		/* TODO: code disabled until we can get real priority information from message */
/* 		if (tny_header_get_priority (header)) { */
/* 			GtkWidget *priority_icon = gtk_image_new_from_icon_name ("qgn_list_messaging_high", GTK_ICON_SIZE_MENU); */
/* 			gtk_box_pack_start (GTK_BOX (subject_box), priority_icon, FALSE, FALSE, 0); */
/* 		} */
		priv->priority_icon = NULL;
		gtk_box_pack_end (GTK_BOX (priv->subject_box), subject_label, TRUE, TRUE, 0);
		if (priv->is_outgoing) {
			gtk_label_set_markup (GTK_LABEL (priv->fromto_label), _("mail_va_to"));
			if (to)
				modest_recpt_view_set_recipients (MODEST_RECPT_VIEW (priv->fromto_contents), to);
			if (cc)
				add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_cc"), cc);
			if (bcc)
				add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_hotfix1"), bcc);
			if (priv->is_draft&& from)
				add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_from"), from);
			modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_subject"),
								   priv->subject_box, TRUE, TRUE);
			if (priv->is_draft)
				add_date_time_header (MODEST_MAIL_HEADER_VIEW (self), _("mcen_fi_message_properties_created"),
						      tny_header_get_date_sent (header));
			else
				add_date_time_header (MODEST_MAIL_HEADER_VIEW (self), _("mcen_fi_message_properties_sent"),
						      tny_header_get_date_sent (header));
		} else {
			gtk_label_set_markup (GTK_LABEL (priv->fromto_label), _("mail_va_from"));
			if (from)
				modest_recpt_view_set_recipients (MODEST_RECPT_VIEW (priv->fromto_contents), from);
			if (to)
				add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_to"), to);
			if (cc)
				add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_cc"), cc);
			if (bcc)
				add_recpt_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_hotfix1"), bcc);
			modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_subject"),
								   priv->subject_box, TRUE, TRUE);
			add_date_time_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_date"),
					      tny_header_get_date_received (header));
		}
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

const GtkWidget *
modest_mail_header_view_add_custom_header (ModestMailHeaderView *header_view,
					   const gchar *label,
					   GtkWidget *custom_widget,
					   gboolean with_expander,
					   gboolean start)
{
	ModestMailHeaderViewPriv *priv;
	g_return_val_if_fail (MODEST_IS_MAIL_HEADER_VIEW (header_view), NULL);
	GtkWidget *hbox;
	GtkWidget *label_field;

	priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (header_view);
	hbox = gtk_hbox_new (FALSE, 12);
	label_field = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label_field), label);
	gtk_misc_set_alignment (GTK_MISC (label_field), 1.0, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label_field, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), custom_widget, TRUE, TRUE, 0);
	gtk_size_group_add_widget (priv->labels_size_group, label_field);

	if (with_expander) {
		if (start)
			gtk_box_pack_start (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);
		else
			gtk_box_pack_end (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);
	} else {
		if (start)
			gtk_box_pack_start (GTK_BOX (priv->main_vbox), hbox, FALSE, FALSE, 0);
		else
			gtk_box_pack_end (GTK_BOX (priv->main_vbox), hbox, FALSE, FALSE, 0);
	}

	return hbox;
}

/**
 * modest_mail_header_view_new:
 *
 * Return value: a new #ModestHeaderView instance implemented for Gtk+
 **/
TnyHeaderView*
modest_mail_header_view_new (gboolean expanded)
{
	ModestMailHeaderViewPriv *priv;
	ModestMailHeaderView *self = g_object_new (MODEST_TYPE_MAIL_HEADER_VIEW, NULL);

	priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	gtk_expander_set_expanded (GTK_EXPANDER (priv->expander), expanded);
	expander_activate (priv->expander, self);

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
	gtk_misc_set_alignment (GTK_MISC (priv->fromto_label), 1.0, 0.0);
	priv->fromto_contents = modest_recpt_view_new ();
	g_signal_connect (G_OBJECT (priv->fromto_contents), "activate", G_CALLBACK (activate_recpt), instance);

	gtk_box_pack_start (GTK_BOX (fromto_hbox), priv->fromto_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (fromto_hbox), priv->fromto_contents, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (priv->main_vbox), fromto_hbox, FALSE, FALSE, 0);

	priv->labels_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (priv->labels_size_group, priv->fromto_label);
	
	priv->headers_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_focus_chain (GTK_CONTAINER (priv->headers_vbox), NULL);
	g_object_ref (priv->headers_vbox);

	expander_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (expander_group, priv->headers_vbox);
	gtk_size_group_add_widget (expander_group, fromto_hbox);
	g_object_unref (expander_group);

	gtk_container_set_reallocate_redraws (GTK_CONTAINER (instance), TRUE);

	priv->is_outgoing = FALSE;
	priv->is_draft = FALSE;

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

	klass->set_header = modest_mail_header_view_set_header;
	klass->clear = modest_mail_header_view_clear;

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
 		g_signal_new ("recpt_activated",
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

TnyHeaderFlags
modest_mail_header_view_get_priority (ModestMailHeaderView *headers_view)
{
	ModestMailHeaderViewPriv *priv;

	g_return_val_if_fail (MODEST_IS_MAIL_HEADER_VIEW (headers_view), 0);
	priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	return priv->priority_flags;
}

void
modest_mail_header_view_set_priority (ModestMailHeaderView *headers_view,
				      TnyHeaderFlags flags)
{
	ModestMailHeaderViewPriv *priv;

	g_return_if_fail (MODEST_IS_MAIL_HEADER_VIEW (headers_view));
	priv = MODEST_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	priv->priority_flags = flags & TNY_HEADER_FLAG_PRIORITY_MASK ;

	if (priv->priority_flags == TNY_HEADER_FLAG_NORMAL_PRIORITY) {
		if (priv->priority_icon != NULL) {
			gtk_widget_destroy (priv->priority_icon);
			priv->priority_icon = NULL;
		}
	} else if (priv->priority_flags == TNY_HEADER_FLAG_HIGH_PRIORITY) {
		priv->priority_icon = gtk_image_new_from_icon_name ("qgn_list_messaging_high", GTK_ICON_SIZE_MENU);
		gtk_box_pack_start (GTK_BOX (priv->subject_box), priv->priority_icon, FALSE, FALSE, 0);
		gtk_widget_show (priv->priority_icon);
	} else if (priv->priority_flags == TNY_HEADER_FLAG_LOW_PRIORITY) {
		priv->priority_icon = gtk_image_new_from_icon_name ("qgn_list_messaging_low", GTK_ICON_SIZE_MENU);
		gtk_box_pack_start (GTK_BOX (priv->subject_box), priv->priority_icon, FALSE, FALSE, 0);
		gtk_widget_show (priv->priority_icon);
	}
}

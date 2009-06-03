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
#include <modest-defs.h>
#include <modest-text-utils.h>
#include <modest-expander-mail-header-view.h>
#include <modest-tny-folder.h>
#include <modest-recpt-view.h>
#include <modest-icon-names.h>
#include <modest-datetime-formatter.h>

static GObjectClass *parent_class = NULL;

typedef struct _ModestExpanderMailHeaderViewPriv ModestExpanderMailHeaderViewPriv;

struct _ModestExpanderMailHeaderViewPriv
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
	gboolean     is_loading;
	TnyHeader    *header;
	TnyHeaderFlags priority_flags;
	ModestDatetimeFormatter *datetime_formatter;
};

#define MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW, ModestExpanderMailHeaderViewPriv))

/* GObject */
static void modest_expander_mail_header_view_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_expander_mail_header_view_class_init (ModestExpanderMailHeaderViewClass *klass);
static void modest_expander_mail_header_view_finalize (GObject *object);

/* TnyHeaderView interface */
static void tny_header_view_init (gpointer g, gpointer iface_data);
static void modest_expander_mail_header_view_set_header (TnyHeaderView *self, TnyHeader *header);
static void modest_expander_mail_header_view_update_is_outgoing (TnyHeaderView *self);
static void modest_expander_mail_header_view_set_header_default (TnyHeaderView *self, TnyHeader *header);
static void modest_expander_mail_header_view_clear (TnyHeaderView *self);
static void modest_expander_mail_header_view_clear_default (TnyHeaderView *self);

/* ModestMailHeaderView interface */
static void modest_mail_header_view_init (gpointer g, gpointer iface_data);
static TnyHeaderFlags modest_expander_mail_header_view_get_priority (ModestMailHeaderView *self);
static TnyHeaderFlags modest_expander_mail_header_view_get_priority_default (ModestMailHeaderView *headers_view);
static void modest_expander_mail_header_view_set_priority (ModestMailHeaderView *self, TnyHeaderFlags flags);
static void modest_expander_mail_header_view_set_priority_default (ModestMailHeaderView *headers_view,
								   TnyHeaderFlags flags);
static gboolean modest_expander_mail_header_view_get_loading (ModestMailHeaderView *headers_view);
static gboolean modest_expander_mail_header_view_get_loading_default (ModestMailHeaderView *headers_view);
static void modest_expander_mail_header_view_set_branding (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static void modest_expander_mail_header_view_set_branding_default (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static void modest_expander_mail_header_view_set_loading (ModestMailHeaderView *headers_view, gboolean is_loading);
static void modest_expander_mail_header_view_set_loading_default (ModestMailHeaderView *headers_view, gboolean is_loading);
static const GtkWidget *modest_expander_mail_header_view_add_custom_header (ModestMailHeaderView *self,
									    const gchar *label,
									    GtkWidget *custom_widget,
									    gboolean with_expander,
									    gboolean start);
static const GtkWidget *modest_expander_mail_header_view_add_custom_header_default (ModestMailHeaderView *self,
										    const gchar *label,
										    GtkWidget *custom_widget,
										    gboolean with_expander,
										    gboolean start);

/* internal */
static void add_date_time_header (ModestExpanderMailHeaderView *mail_header, const gchar *name, time_t date);
static void activate_recpt (GtkWidget *recpt_view, const gchar *address, gpointer user_data);
static void add_recpt_header (ModestExpanderMailHeaderView *widget, const gchar *field, const gchar *value);
static void clean_headers (GtkWidget *vbox);
static void expander_activate (GtkWidget *expander, ModestExpanderMailHeaderView *header_view);



static void
add_date_time_header (ModestExpanderMailHeaderView *mail_header, const gchar *name, time_t date)
{
	const gchar *date_buf;
	const gchar *time_buf;

	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (mail_header);
	GtkWidget *hbox, *date_hbox, *time_hbox;
	GtkWidget *label;

	date_buf = modest_datetime_formatter_format_date (priv->datetime_formatter, date);
	time_buf = modest_datetime_formatter_format_time (priv->datetime_formatter, date);


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
	ModestExpanderMailHeaderView * view = MODEST_EXPANDER_MAIL_HEADER_VIEW (user_data);

	g_signal_emit_by_name (G_OBJECT (view), "recpt-activated", address);
}


static void
add_recpt_header (ModestExpanderMailHeaderView *widget, const gchar *field, const gchar *value)
{
	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (widget);
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
modest_expander_mail_header_view_set_header (TnyHeaderView *self, TnyHeader *header)
{
	MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (self)->set_header_func (self, header);
	return;
}

static void
modest_expander_mail_header_view_update_is_outgoing (TnyHeaderView *self)
{
	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (self);
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
modest_expander_mail_header_view_set_header_default (TnyHeaderView *self, TnyHeader *header)
{
	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (header)
		g_assert (TNY_IS_HEADER (header));

	if (G_LIKELY (priv->header))
 		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	clean_headers (priv->headers_vbox);

	if (header && G_IS_OBJECT (header))
	{
		gchar *to, *from, *subject, *bcc, *cc;
		GtkWidget *subject_label;

		g_object_ref (G_OBJECT (header)); 
		priv->header = header;

		modest_expander_mail_header_view_update_is_outgoing (self);


		to = tny_header_dup_to (header);
		from = tny_header_dup_from (header);
		subject = tny_header_dup_subject (header);
		cc = tny_header_dup_cc (header);
		bcc = tny_header_dup_bcc (header);

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

		priv->priority_icon = NULL;
		gtk_box_pack_end (GTK_BOX (priv->subject_box), subject_label, TRUE, TRUE, 0);
		if (priv->is_outgoing) {
			gtk_label_set_markup (GTK_LABEL (priv->fromto_label), _("mail_va_to"));
			if (to)
				modest_recpt_view_set_recipients (MODEST_RECPT_VIEW (priv->fromto_contents), to);
			if (cc)
				add_recpt_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mail_va_cc"), cc);
			if (bcc)
				add_recpt_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mail_va_hotfix1"), bcc);
			if (priv->is_draft&& from)
				add_recpt_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mail_va_from"), from);
			modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_subject"),
								   priv->subject_box, TRUE, TRUE);
			if (priv->is_draft)
				add_date_time_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mcen_fi_message_properties_created"),
						      tny_header_get_date_sent (header));
			else
				add_date_time_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mcen_fi_message_properties_sent"),
						      tny_header_get_date_sent (header));
		} else {
			gtk_label_set_markup (GTK_LABEL (priv->fromto_label), _("mail_va_from"));
			if (from)
				modest_recpt_view_set_recipients (MODEST_RECPT_VIEW (priv->fromto_contents), from);
			if (to)
				add_recpt_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mail_va_to"), to);
			if (cc)
				add_recpt_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mail_va_cc"), cc);
			if (bcc)
				add_recpt_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mail_va_hotfix1"), bcc);
			modest_mail_header_view_add_custom_header (MODEST_MAIL_HEADER_VIEW (self), _("mail_va_subject"),
								   priv->subject_box, TRUE, TRUE);
			add_date_time_header (MODEST_EXPANDER_MAIL_HEADER_VIEW (self), _("mail_va_date"),
					      tny_header_get_date_received (header));
		}
		g_free (subject);
		g_free (to);
		g_free (from);
		g_free (cc);
		g_free (bcc);
	}

	gtk_widget_show_all (GTK_WIDGET (self));

	return;
}

static void 
modest_expander_mail_header_view_clear (TnyHeaderView *self)
{
	MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (self)->clear_func (self);
	return;
}

static void 
modest_expander_mail_header_view_clear_default (TnyHeaderView *self)
{
	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (self);

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
expander_activate (GtkWidget *expander, ModestExpanderMailHeaderView *header_view)
{
	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (header_view);

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

static const GtkWidget *
modest_expander_mail_header_view_add_custom_header (ModestMailHeaderView *self,
						    const gchar *label,
						    GtkWidget *custom_widget,
						    gboolean with_expander,
						    gboolean start)
{
	return MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (self)->add_custom_header_func (self, label,
											  custom_widget, with_expander,
											  start);
}

static const GtkWidget *
modest_expander_mail_header_view_add_custom_header_default (ModestMailHeaderView *self,
							    const gchar *label,
							    GtkWidget *custom_widget,
							    gboolean with_expander,
							    gboolean start)
{
	ModestExpanderMailHeaderViewPriv *priv;
	g_return_val_if_fail (MODEST_IS_EXPANDER_MAIL_HEADER_VIEW (self), NULL);
	GtkWidget *hbox;
	GtkWidget *label_field;

	priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (self);
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
 * modest_expander_mail_header_view_new:
 *
 * Return value: a new #ModestHeaderView instance implemented for Gtk+
 **/
TnyHeaderView*
modest_expander_mail_header_view_new (gboolean expanded)
{
	ModestExpanderMailHeaderViewPriv *priv;
	ModestExpanderMailHeaderView *self = g_object_new (MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW, NULL);

	priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	gtk_expander_set_expanded (GTK_EXPANDER (priv->expander), expanded);
	expander_activate (priv->expander, self);

	return TNY_HEADER_VIEW (self);
}

static void
modest_expander_mail_header_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestExpanderMailHeaderView *self = (ModestExpanderMailHeaderView *)instance;
	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	GtkWidget *fromto_hbox = NULL;
	GtkSizeGroup *expander_group = NULL;

	priv->header = NULL;

	priv->datetime_formatter = modest_datetime_formatter_new ();

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

	priv->is_loading = FALSE;

	return;
}

static void
modest_expander_mail_header_view_finalize (GObject *object)
{
	ModestExpanderMailHeaderView *self = (ModestExpanderMailHeaderView *)object;	
	ModestExpanderMailHeaderViewPriv *priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (priv->datetime_formatter) {
		g_object_unref (priv->datetime_formatter);
		priv->datetime_formatter = NULL;
	}

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

	klass->set_header = modest_expander_mail_header_view_set_header;
	klass->clear = modest_expander_mail_header_view_clear;

	return;
}

static void
modest_mail_header_view_init (gpointer g, gpointer iface_data)
{
	ModestMailHeaderViewIface *klass = (ModestMailHeaderViewIface *)g;

	klass->get_priority = modest_expander_mail_header_view_get_priority;
	klass->set_priority = modest_expander_mail_header_view_set_priority;
	klass->get_loading = modest_expander_mail_header_view_get_loading;
	klass->set_loading = modest_expander_mail_header_view_set_loading;
	klass->set_branding = modest_expander_mail_header_view_set_branding;
	klass->add_custom_header = modest_expander_mail_header_view_add_custom_header;

	return;
}

static void 
modest_expander_mail_header_view_class_init (ModestExpanderMailHeaderViewClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	klass->set_header_func = modest_expander_mail_header_view_set_header_default;
	klass->clear_func = modest_expander_mail_header_view_clear_default;
	klass->set_priority_func = modest_expander_mail_header_view_set_priority_default;
	klass->get_priority_func = modest_expander_mail_header_view_get_priority_default;
	klass->set_loading_func = modest_expander_mail_header_view_set_loading_default;
	klass->get_loading_func = modest_expander_mail_header_view_get_loading_default;
	klass->set_branding_func = modest_expander_mail_header_view_set_branding_default;
	klass->add_custom_header_func = modest_expander_mail_header_view_add_custom_header_default;

	object_class->finalize = modest_expander_mail_header_view_finalize;

	g_type_class_add_private (object_class, sizeof (ModestExpanderMailHeaderViewPriv));

	return;
}

GType 
modest_expander_mail_header_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestExpanderMailHeaderViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_expander_mail_header_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestExpanderMailHeaderView),
		  0,      /* n_preallocs */
		  modest_expander_mail_header_view_instance_init    /* instance_init */
		};

		static const GInterfaceInfo tny_header_view_info = 
		{
		  (GInterfaceInitFunc) tny_header_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		static const GInterfaceInfo modest_mail_header_view_info = 
		{
		  (GInterfaceInitFunc) modest_mail_header_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
			"ModestExpanderMailHeaderView",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_HEADER_VIEW, 
			&tny_header_view_info);

		g_type_add_interface_static (type, MODEST_TYPE_MAIL_HEADER_VIEW, 
			&modest_mail_header_view_info);

	}

	return type;
}

static TnyHeaderFlags
modest_expander_mail_header_view_get_priority (ModestMailHeaderView *self)
{
	return MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (self)->get_priority_func (self);
}

static TnyHeaderFlags
modest_expander_mail_header_view_get_priority_default (ModestMailHeaderView *headers_view)
{
	ModestExpanderMailHeaderViewPriv *priv;

	g_return_val_if_fail (MODEST_IS_EXPANDER_MAIL_HEADER_VIEW (headers_view), 0);
	priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	return priv->priority_flags;
}

static void 
modest_expander_mail_header_view_set_priority (ModestMailHeaderView *self, TnyHeaderFlags flags)
{
	MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (self)->set_priority_func (self, flags);
	return;
}

static void
modest_expander_mail_header_view_set_priority_default (ModestMailHeaderView *headers_view,
						       TnyHeaderFlags flags)
{
	ModestExpanderMailHeaderViewPriv *priv;

	g_return_if_fail (MODEST_IS_EXPANDER_MAIL_HEADER_VIEW (headers_view));
	priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	priv->priority_flags = flags & TNY_HEADER_FLAG_PRIORITY_MASK ;

	if (priv->priority_flags == TNY_HEADER_FLAG_NORMAL_PRIORITY) {
		if (priv->priority_icon != NULL) {
			gtk_widget_destroy (priv->priority_icon);
			priv->priority_icon = NULL;
		}
	} else if (priv->priority_flags == TNY_HEADER_FLAG_HIGH_PRIORITY) {
		priv->priority_icon = gtk_image_new_from_icon_name (MODEST_HEADER_ICON_HIGH, GTK_ICON_SIZE_MENU);
		gtk_box_pack_start (GTK_BOX (priv->subject_box), priv->priority_icon, FALSE, FALSE, 0);
		gtk_widget_show (priv->priority_icon);
	} else if (priv->priority_flags == TNY_HEADER_FLAG_LOW_PRIORITY) {
		priv->priority_icon = gtk_image_new_from_icon_name (MODEST_HEADER_ICON_LOW, GTK_ICON_SIZE_MENU);
		gtk_box_pack_start (GTK_BOX (priv->subject_box), priv->priority_icon, FALSE, FALSE, 0);
		gtk_widget_show (priv->priority_icon);
	}
}

static gboolean
modest_expander_mail_header_view_get_loading (ModestMailHeaderView *headers_view)
{
	return MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (headers_view)->get_loading_func (headers_view);
}

static gboolean
modest_expander_mail_header_view_get_loading_default (ModestMailHeaderView *headers_view)
{
	ModestExpanderMailHeaderViewPriv *priv;

	g_return_val_if_fail (MODEST_IS_EXPANDER_MAIL_HEADER_VIEW (headers_view), FALSE);
	priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	return priv->is_loading;
}

static void
modest_expander_mail_header_view_set_loading (ModestMailHeaderView *headers_view, gboolean is_loading)
{
	MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (headers_view)->set_loading_func (headers_view, is_loading);
}

static void
modest_expander_mail_header_view_set_loading_default (ModestMailHeaderView *headers_view, gboolean is_loading)
{
	ModestExpanderMailHeaderViewPriv *priv;

	g_return_if_fail (MODEST_IS_EXPANDER_MAIL_HEADER_VIEW (headers_view));
	priv = MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	priv->is_loading = is_loading;
}

static void
modest_expander_mail_header_view_set_branding (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS (headers_view)->set_branding_func (headers_view, brand_name, brand_icon);
}

static void
modest_expander_mail_header_view_set_branding_default (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	g_return_if_fail (MODEST_IS_EXPANDER_MAIL_HEADER_VIEW (headers_view));

	/* Empty implementation */
	return;
}

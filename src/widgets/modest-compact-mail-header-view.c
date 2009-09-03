/* Copyright (c) 2008, Nokia Corporation
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
#include <modest-compact-mail-header-view.h>
#include <modest-tny-folder.h>
#include <modest-ui-constants.h>
#include <modest-icon-names.h>
#include <modest-datetime-formatter.h>
#ifdef MODEST_TOOLKIT_HILDON2
#include <hildon/hildon-gtk.h>
#include <hildon/hildon-helper.h>
#endif

static GObjectClass *parent_class = NULL;

typedef struct _ModestCompactMailHeaderViewPriv ModestCompactMailHeaderViewPriv;

struct _ModestCompactMailHeaderViewPriv
{
	GtkWidget    *headers_vbox;
	GtkWidget    *event_box;

	GtkWidget    *fromto_label;
	GtkWidget    *fromto_contents;
	GtkWidget    *time_label;
	GtkWidget    *date_label;
	GtkWidget    *brand_label;
	GtkWidget    *brand_image;

	GSList       *custom_labels;

	gboolean     is_outgoing;
	gboolean     is_draft;
	
	TnyHeader    *header;
	TnyHeaderFlags priority_flags;

	gboolean     is_loading;

	time_t       date_to_show;
	ModestDatetimeFormatter *datetime_formatter;
};

#define MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_COMPACT_MAIL_HEADER_VIEW, ModestCompactMailHeaderViewPriv))

/* GObject */
static void modest_compact_mail_header_view_class_init (ModestCompactMailHeaderViewClass *klass);
static void modest_compact_mail_header_view_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_compact_mail_header_view_finalize (GObject *object);

/* TnyHeaderView interface */
static void tny_header_view_init (gpointer g, gpointer iface_data);
static void modest_compact_mail_header_view_set_header (TnyHeaderView *self, TnyHeader *header);
static void modest_compact_mail_header_view_update_is_outgoing (TnyHeaderView *self);
static void modest_compact_mail_header_view_set_header_default (TnyHeaderView *self, TnyHeader *header);
static void modest_compact_mail_header_view_clear (TnyHeaderView *self);
static void modest_compact_mail_header_view_clear_default (TnyHeaderView *self);

/* ModestMailHeaderView interface */
static void modest_mail_header_view_init (gpointer g, gpointer iface_data);
static TnyHeaderFlags modest_compact_mail_header_view_get_priority (ModestMailHeaderView *self);
static TnyHeaderFlags modest_compact_mail_header_view_get_priority_default (ModestMailHeaderView *headers_view);
static void modest_compact_mail_header_view_set_priority (ModestMailHeaderView *self, TnyHeaderFlags flags);
static void modest_compact_mail_header_view_set_priority_default (ModestMailHeaderView *headers_view,
								   TnyHeaderFlags flags);
static gboolean modest_compact_mail_header_view_get_loading (ModestMailHeaderView *headers_view);
static gboolean modest_compact_mail_header_view_get_loading_default (ModestMailHeaderView *headers_view);
static void modest_compact_mail_header_view_set_loading (ModestMailHeaderView *headers_view, gboolean is_loading);
static void modest_compact_mail_header_view_set_loading_default (ModestMailHeaderView *headers_view, gboolean is_loading);
static void modest_compact_mail_header_view_set_branding (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static void modest_compact_mail_header_view_set_branding_default (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon);
static const GtkWidget *modest_compact_mail_header_view_add_custom_header (ModestMailHeaderView *self,
									    const gchar *label,
									    GtkWidget *custom_widget,
									    gboolean with_expander,
									    gboolean start);
static const GtkWidget *modest_compact_mail_header_view_add_custom_header_default (ModestMailHeaderView *self,
										    const gchar *label,
										    GtkWidget *custom_widget,
										    gboolean with_expander,
										    gboolean start);

/* internal */
static void on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata);
static void update_style (ModestCompactMailHeaderView *self);
static void set_date_time (ModestCompactMailHeaderView *compact_mail_header);
static void fill_address (ModestCompactMailHeaderView *self);

static void
set_date_time (ModestCompactMailHeaderView *compact_mail_header)
{
	ModestCompactMailHeaderViewPriv *priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (compact_mail_header);

	if (priv->date_to_show == 0) {
		gtk_label_set_text (GTK_LABEL (priv->time_label), "");
		gtk_label_set_text (GTK_LABEL (priv->date_label), "");
	} else {
		const guint BUF_SIZE = 64; 
		const gchar *date_str;
		gchar date_buf[BUF_SIZE];
		GString *buffer = g_string_new ("");

		modest_text_utils_strftime (date_buf, BUF_SIZE, _HL("wdgt_va_week"), priv->date_to_show);
		buffer = g_string_append (buffer, date_buf);
		buffer = g_string_append_c (buffer, ' ');
		buffer = g_string_append_unichar (buffer, 0x2015);
		buffer = g_string_append_c (buffer, ' ');
		date_str = modest_datetime_formatter_format_time (priv->datetime_formatter, priv->date_to_show);
		buffer = g_string_append (buffer, date_str);
		gtk_label_set_text (GTK_LABEL (priv->time_label), buffer->str);
		g_string_free  (buffer, TRUE);
		buffer = g_string_new ("");
		modest_text_utils_strftime (date_buf, BUF_SIZE, _HL("wdgt_va_date_medium"), priv->date_to_show);
		buffer = g_string_append (buffer, date_buf);

		gtk_label_set_text (GTK_LABEL (priv->date_label), buffer->str);
		g_string_free (buffer, TRUE);
	}

}

/* static void */
/* activate_recpt (GtkWidget *recpt_view, const gchar *address, gpointer user_data) */
/* { */
/* 	ModestCompactMailHeaderView * view = MODEST_COMPACT_MAIL_HEADER_VIEW (user_data); */

/* 	g_signal_emit (G_OBJECT (view), signals[RECPT_ACTIVATED_SIGNAL], 0, address); */
/* } */


static void 
modest_compact_mail_header_view_set_header (TnyHeaderView *self, TnyHeader *header)
{
	MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (self)->set_header_func (self, header);
	return;
}

static void
modest_compact_mail_header_view_update_is_outgoing (TnyHeaderView *self)
{
	ModestCompactMailHeaderViewPriv *priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);
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
modest_compact_mail_header_view_set_header_default (TnyHeaderView *self, TnyHeader *header)
{
	ModestCompactMailHeaderViewPriv *priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (header)
		g_assert (TNY_IS_HEADER (header));

	if (G_LIKELY (priv->header))
 		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	if (header && G_IS_OBJECT (header))
	{
		gchar *subject;

		g_object_ref (G_OBJECT (header)); 
		priv->header = header;

		modest_compact_mail_header_view_update_is_outgoing (self);

		subject = tny_header_dup_subject (header);

		if (priv->is_outgoing && priv->is_draft) {
			priv->date_to_show = tny_header_get_date_sent (header);
		} else {
			priv->date_to_show = tny_header_get_date_received (header);
		}
		set_date_time (MODEST_COMPACT_MAIL_HEADER_VIEW (self));

		fill_address (MODEST_COMPACT_MAIL_HEADER_VIEW (self));

		g_free (subject);
	}

	modest_mail_header_view_set_priority (MODEST_MAIL_HEADER_VIEW (self), 0);
	update_style (MODEST_COMPACT_MAIL_HEADER_VIEW (self));
	gtk_widget_show_all (GTK_WIDGET (self));

	return;
}

static void 
modest_compact_mail_header_view_clear (TnyHeaderView *self)
{
	MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (self)->clear_func (self);
	return;
}

static void 
modest_compact_mail_header_view_clear_default (TnyHeaderView *self)
{
	ModestCompactMailHeaderViewPriv *priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (G_LIKELY (priv->header))
		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	gtk_label_set_text (GTK_LABEL (priv->fromto_label), "");
	gtk_label_set_text (GTK_LABEL (priv->fromto_contents), "");

	gtk_widget_hide (GTK_WIDGET(self));

	return;
}

static const GtkWidget *
modest_compact_mail_header_view_add_custom_header (ModestMailHeaderView *self,
						    const gchar *label,
						    GtkWidget *custom_widget,
						    gboolean with_expander,
						    gboolean start)
{
	return MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (self)->add_custom_header_func (self, label,
											 custom_widget, with_expander,
											 start);
}

static const GtkWidget *
modest_compact_mail_header_view_add_custom_header_default (ModestMailHeaderView *header_view,
							   const gchar *label,
							   GtkWidget *custom_widget,
							   gboolean with_expander,
							   gboolean start)
{
	ModestCompactMailHeaderViewPriv *priv;
	g_return_val_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (header_view), NULL);
	GtkWidget *hbox;
	GtkWidget *label_field;

	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (header_view);
	hbox = gtk_hbox_new (FALSE, 12);
	label_field = gtk_label_new (NULL);
	gtk_label_set_text (GTK_LABEL (label_field), label);
	priv->custom_labels = g_slist_prepend (priv->custom_labels, (gpointer) label_field);
	gtk_misc_set_alignment (GTK_MISC (label_field), 1.0, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), label_field, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), custom_widget, TRUE, TRUE, 0);

	if (start)
		gtk_box_pack_start (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);
	else
		gtk_box_pack_end (GTK_BOX (priv->headers_vbox), hbox, FALSE, FALSE, 0);

	update_style (MODEST_COMPACT_MAIL_HEADER_VIEW (header_view));
	return hbox;
}

/**
 * modest_compact_mail_header_view_new:
 *
 * Return value: a new #ModestHeaderView instance implemented for Gtk+
 **/
TnyHeaderView*
modest_compact_mail_header_view_new ()
{
	ModestCompactMailHeaderViewPriv *priv;
	ModestCompactMailHeaderView *self = g_object_new (MODEST_TYPE_COMPACT_MAIL_HEADER_VIEW, NULL);

	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	return TNY_HEADER_VIEW (self);
}

static void
datetime_format_changed (ModestDatetimeFormatter *formatter,
			 ModestCompactMailHeaderView *self)
{
	set_date_time (self);
}

static void
unref_event_box (ModestCompactMailHeaderView *self,
		 GtkWidget *event_box)
{
	ModestCompactMailHeaderViewPriv *priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	priv->event_box = NULL;
}

static void
modest_compact_mail_header_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestCompactMailHeaderView *self = (ModestCompactMailHeaderView *)instance;
	ModestCompactMailHeaderViewPriv *priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);
	GtkWidget *from_date_hbox, *vbox, *main_vbox;
	GtkWidget *main_align;
	GtkWidget *headers_date_hbox;
	GtkWidget *date_brand_vbox, *brand_hbox;

	priv->header = NULL;
	priv->custom_labels = NULL;

	main_vbox = gtk_vbox_new (FALSE, 0);
	vbox = gtk_vbox_new (FALSE, 0);

	/* We set here the style for widgets using standard text color. For
	 * widgets with secondary text color, we set them in update_style,
	 * as we want to track the style changes and update the color properly */

	from_date_hbox = gtk_hbox_new (FALSE, MODEST_MARGIN_DOUBLE);
	headers_date_hbox = gtk_hbox_new (FALSE, MODEST_MARGIN_DOUBLE);

	priv->fromto_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->fromto_label), 0.0, 1.0);

	priv->fromto_contents = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->fromto_contents), 0.0, 1.0);
	gtk_label_set_ellipsize (GTK_LABEL (priv->fromto_contents), PANGO_ELLIPSIZE_END);

	priv->date_label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (priv->date_label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (priv->date_label), 1.0, 0.0);
	gtk_misc_set_padding (GTK_MISC (priv->date_label), MODEST_MARGIN_DOUBLE, 0);

	priv->time_label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (priv->time_label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (priv->time_label), 1.0, 1.0);
	gtk_misc_set_padding (GTK_MISC (priv->time_label), MODEST_MARGIN_DOUBLE, 0);

	priv->brand_label = gtk_label_new (NULL);
#ifdef MODEST_TOOLKIT_HILDON2
	hildon_helper_set_logical_font (priv->brand_label, "SmallSystemFont");
#endif
	gtk_label_set_justify (GTK_LABEL (priv->brand_label), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (priv->brand_label), 1.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (priv->brand_label), MODEST_MARGIN_DOUBLE, 0);
	gtk_widget_set_no_show_all (priv->brand_label, TRUE);

	priv->brand_image = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->brand_image), 0.5, 0.5);
	gtk_widget_set_no_show_all (priv->brand_image, TRUE);

	gtk_box_pack_start (GTK_BOX (from_date_hbox), priv->fromto_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (from_date_hbox), priv->fromto_contents, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (from_date_hbox), priv->time_label, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), from_date_hbox, FALSE, FALSE, 0);

	priv->datetime_formatter = modest_datetime_formatter_new ();
	g_signal_connect (G_OBJECT (priv->datetime_formatter), "format-changed", 
			  G_CALLBACK (datetime_format_changed), (gpointer) self);

	priv->headers_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_focus_chain (GTK_CONTAINER (priv->headers_vbox), NULL);
	g_object_ref (priv->headers_vbox);

	brand_hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (brand_hbox), priv->brand_image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (brand_hbox), priv->brand_label, TRUE, TRUE, 0);

	date_brand_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (date_brand_vbox), priv->date_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (date_brand_vbox), brand_hbox, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (headers_date_hbox), priv->headers_vbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (headers_date_hbox), date_brand_vbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), headers_date_hbox, FALSE, FALSE, 0);

	main_align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (main_align), 0, MODEST_MARGIN_HALF, MODEST_MARGIN_DOUBLE, 0);
	priv->event_box = gtk_event_box_new ();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (priv->event_box), TRUE);
	gtk_container_add (GTK_CONTAINER (main_align), main_vbox);
	gtk_container_add (GTK_CONTAINER (priv->event_box), main_align);
	gtk_box_pack_start (GTK_BOX (self), priv->event_box, TRUE, TRUE, 0);
	g_object_weak_ref (G_OBJECT (priv->event_box), (GWeakNotify) unref_event_box, (gpointer) self);

	update_style (self);

	g_signal_connect (G_OBJECT (self), "notify::style", G_CALLBACK (on_notify_style), (gpointer) self);

	gtk_widget_show_all (priv->event_box);

	priv->is_outgoing = FALSE;
	priv->is_draft = FALSE;
	priv->is_loading = FALSE;

	return;
}

static void
modest_compact_mail_header_view_finalize (GObject *object)
{
	ModestCompactMailHeaderView *self = (ModestCompactMailHeaderView *)object;	
	ModestCompactMailHeaderViewPriv *priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (priv->event_box) {
		g_object_weak_unref (G_OBJECT (priv->event_box), (GWeakNotify) unref_event_box, (gpointer) self);
		priv->event_box = NULL;
	}

	if (priv->datetime_formatter) {
		g_object_unref (priv->datetime_formatter);
		priv->datetime_formatter = NULL;
	}

	if (priv->custom_labels) {
		g_slist_free (priv->custom_labels);
		priv->custom_labels = NULL;
	}

	if (G_LIKELY (priv->header))
		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;

	if (G_LIKELY (priv->headers_vbox))
		g_object_unref (G_OBJECT (priv->headers_vbox));

	priv->headers_vbox = NULL;

	(*parent_class->finalize) (object);

	return;
}

static void
tny_header_view_init (gpointer g, gpointer iface_data)
{
	TnyHeaderViewIface *klass = (TnyHeaderViewIface *)g;

	klass->set_header = modest_compact_mail_header_view_set_header;
	klass->clear = modest_compact_mail_header_view_clear;

	return;
}

static void
modest_mail_header_view_init (gpointer g, gpointer iface_data)
{
	ModestMailHeaderViewIface *klass = (ModestMailHeaderViewIface *)g;

	klass->get_priority = modest_compact_mail_header_view_get_priority;
	klass->set_priority = modest_compact_mail_header_view_set_priority;
	klass->get_loading = modest_compact_mail_header_view_get_loading;
	klass->set_loading = modest_compact_mail_header_view_set_loading;
	klass->set_branding = modest_compact_mail_header_view_set_branding;
	klass->add_custom_header = modest_compact_mail_header_view_add_custom_header;

	return;
}

static void 
modest_compact_mail_header_view_class_init (ModestCompactMailHeaderViewClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	klass->set_header_func = modest_compact_mail_header_view_set_header_default;
	klass->clear_func = modest_compact_mail_header_view_clear_default;
	klass->set_priority_func = modest_compact_mail_header_view_set_priority_default;
	klass->get_priority_func = modest_compact_mail_header_view_get_priority_default;
	klass->set_loading_func = modest_compact_mail_header_view_set_loading_default;
	klass->get_loading_func = modest_compact_mail_header_view_get_loading_default;
	klass->set_branding_func = modest_compact_mail_header_view_set_branding_default;
	klass->add_custom_header_func = modest_compact_mail_header_view_add_custom_header_default;
	object_class->finalize = modest_compact_mail_header_view_finalize;

	g_type_class_add_private (object_class, sizeof (ModestCompactMailHeaderViewPriv));

	return;
}

GType 
modest_compact_mail_header_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestCompactMailHeaderViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_compact_mail_header_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestCompactMailHeaderView),
		  0,      /* n_preallocs */
		  modest_compact_mail_header_view_instance_init    /* instance_init */
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
			"ModestCompactMailHeaderView",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_HEADER_VIEW, 
			&tny_header_view_info);
		g_type_add_interface_static (type, MODEST_TYPE_MAIL_HEADER_VIEW, 
			&modest_mail_header_view_info);

	}

	return type;
}

static TnyHeaderFlags
modest_compact_mail_header_view_get_priority (ModestMailHeaderView *self)
{
	return MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (self)->get_priority_func (self);
}

static TnyHeaderFlags
modest_compact_mail_header_view_get_priority_default (ModestMailHeaderView *headers_view)
{
	ModestCompactMailHeaderViewPriv *priv;

	g_return_val_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (headers_view), 0);
	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	return priv->priority_flags;
}

static void 
modest_compact_mail_header_view_set_priority (ModestMailHeaderView *self, TnyHeaderFlags flags)
{
	MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (self)->set_priority_func (self, flags);
	return;
}

static void
modest_compact_mail_header_view_set_priority_default (ModestMailHeaderView *headers_view,
						      TnyHeaderFlags flags)
{
	ModestCompactMailHeaderViewPriv *priv;

	g_return_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (headers_view));
	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	priv->priority_flags = flags & TNY_HEADER_FLAG_PRIORITY_MASK ;
}

static gboolean
modest_compact_mail_header_view_get_loading (ModestMailHeaderView *headers_view)
{
	return MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (headers_view)->get_loading_func (headers_view);
}

static gboolean
modest_compact_mail_header_view_get_loading_default (ModestMailHeaderView *headers_view)
{
	ModestCompactMailHeaderViewPriv *priv;

	g_return_val_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (headers_view), FALSE);
	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	return priv->is_loading;
}

static void
modest_compact_mail_header_view_set_loading (ModestMailHeaderView *headers_view, gboolean is_loading)
{
	MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (headers_view)->set_loading_func (headers_view, is_loading);
}

static void
modest_compact_mail_header_view_set_loading_default (ModestMailHeaderView *headers_view, gboolean is_loading)
{
	ModestCompactMailHeaderViewPriv *priv;

	g_return_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (headers_view));
	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	priv->is_loading = is_loading;
}

static void
modest_compact_mail_header_view_set_branding (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	MODEST_COMPACT_MAIL_HEADER_VIEW_GET_CLASS (headers_view)->set_branding_func (headers_view, brand_name, brand_icon);
}

static void
modest_compact_mail_header_view_set_branding_default (ModestMailHeaderView *headers_view, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	ModestCompactMailHeaderViewPriv *priv;

	g_return_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (headers_view));
	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (headers_view);

	if (brand_name) {
		gtk_label_set_text (GTK_LABEL (priv->brand_label), brand_name);
		gtk_widget_show (priv->brand_label);
	} else {
		gtk_widget_hide (priv->brand_label);
	}

	if (brand_icon) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (priv->brand_image), (GdkPixbuf *) brand_icon);
		gtk_widget_show (priv->brand_image);
	} else {
		gtk_widget_hide (priv->brand_image);
	}


}

static void 
on_notify_style (GObject *obj, GParamSpec *spec, gpointer userdata)
{
	if (strcmp ("style", spec->name) == 0) {
		update_style (MODEST_COMPACT_MAIL_HEADER_VIEW (obj));
		gtk_widget_queue_draw (GTK_WIDGET (obj));
	} 
}

/* This method updates the color (and other style settings) of widgets using secondary text color,
 * tracking the gtk style */
static void
update_style (ModestCompactMailHeaderView *self)
{
	ModestCompactMailHeaderViewPriv *priv;
	GdkColor style_color;
	PangoColor color;
	PangoAttrList *attr_list;
	GSList *node;
	GdkColor bg_color;

	g_return_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (self));
	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (gtk_style_lookup_color (GTK_WIDGET (self)->style, "SecondaryTextColor", &style_color)) {
		color.red = style_color.red;
		color.green = style_color.green;
		color.blue = style_color.blue;
	} else {
		if (!pango_color_parse (&color, "grey")) {
			g_warning ("Failed to parse color grey");
			color.red = 0xc0c0;
			color.green = 0xc0c0;
			color.blue = 0xc0c0;
		}
	}

	/* style of from:/to: label */
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_foreground_new (color.red, color.green, color.blue));
	gtk_label_set_attributes (GTK_LABEL (priv->fromto_label), attr_list);
	pango_attr_list_unref (attr_list);

	/* style of date time label */
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_foreground_new (color.red, color.green, color.blue));
	pango_attr_list_insert (attr_list, pango_attr_scale_new (PANGO_SCALE_SMALL));
	gtk_label_set_attributes (GTK_LABEL (priv->date_label), attr_list);
	gtk_label_set_attributes (GTK_LABEL (priv->time_label), attr_list);
	pango_attr_list_unref (attr_list);

	/* set style of custom headers */
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_foreground_new (color.red, color.green, color.blue));
	for (node = priv->custom_labels; node != NULL; node = g_slist_next (node)) {
		gtk_label_set_attributes (GTK_LABEL (node->data), attr_list);
	}
	pango_attr_list_unref (attr_list);

#ifdef MODEST_COMPACT_HEADER_BG
	gdk_color_parse (MODEST_COMPACT_HEADER_BG, &bg_color);
#endif
	gtk_widget_modify_bg (GTK_WIDGET (priv->event_box), GTK_STATE_NORMAL, &bg_color);
}

static void
fill_address (ModestCompactMailHeaderView *self)
{
	ModestCompactMailHeaderViewPriv *priv;
	gchar *recipients;
	const gchar *label;

	g_return_if_fail (MODEST_IS_COMPACT_MAIL_HEADER_VIEW (self));
	priv = MODEST_COMPACT_MAIL_HEADER_VIEW_GET_PRIVATE (self);

	if (priv->is_outgoing) {
		label = _("mail_va_to");
		recipients = tny_header_dup_to (TNY_HEADER (priv->header));
	} else {
		label = _("mail_va_from");
		recipients = tny_header_dup_from (TNY_HEADER (priv->header));
	}

	/* Set label */
	gtk_label_set_text (GTK_LABEL (priv->fromto_label), label);

	/* Set recipients */
	if (recipients) {
		gchar *addresses;

		addresses = modest_text_utils_get_display_addresses ((const gchar *) recipients);
		gtk_label_set_text (GTK_LABEL (priv->fromto_contents), 
				    (addresses) ? addresses : _("mail_va_no_to"));
		g_free (addresses);
		g_free (recipients);
	}
}


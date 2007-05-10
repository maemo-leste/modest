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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <glib/gi18n.h>

#include <string.h>
#include <modest-attachment-view.h>
#include <modest-platform.h>
#include <modest-text-utils.h>
#include <tny-msg.h>

static GObjectClass *parent_class = NULL;

typedef struct _ModestAttachmentViewPriv ModestAttachmentViewPriv;

struct _ModestAttachmentViewPriv
{
	TnyMimePart *mime_part;

	GtkWidget *icon;
	GtkWidget *filename_view;
	GtkWidget *size_view;

	guint get_size_idle_id;
	TnyStream *get_size_stream;
	guint64 size;

	PangoLayout *layout_full_filename;

};

#define UNKNOWN_FILE_ICON "qgn_list_gene_unknown_file"
#define GET_SIZE_BUFFER_SIZE 128

#define MODEST_ATTACHMENT_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_ATTACHMENT_VIEW, ModestAttachmentViewPriv))

/* TnyMimePartView functions */
static TnyMimePart *modest_attachment_view_get_part (TnyMimePartView *self);
static TnyMimePart *modest_attachment_view_get_part_default (TnyMimePartView *self);
static void modest_attachment_view_set_part (TnyMimePartView *self, TnyMimePart *mime_part);
static void modest_attachment_view_set_part_default (TnyMimePartView *self, TnyMimePart *mime_part);
static void modest_attachment_view_clear (TnyMimePartView *self);
static void modest_attachment_view_clear_default (TnyMimePartView *self);

/* Gtk events */
static void size_allocate (GtkWidget *widget, GtkAllocation *allocation);

/* GObject and GInterface management */
static void modest_attachment_view_instance_init (GTypeInstance *instance, gpointer g_class);
static void modest_attachment_view_finalize (GObject *object);
static void modest_attachment_view_class_init (ModestAttachmentViewClass *klass);
static void tny_mime_part_view_init (gpointer g, gpointer iface_data);



static gboolean get_size_idle_func (gpointer data);
static void update_filename_request (ModestAttachmentView *self);



static TnyMimePart *
modest_attachment_view_get_part (TnyMimePartView *self)
{
	return MODEST_ATTACHMENT_VIEW_GET_CLASS (self)->get_part_func (self);
}

static TnyMimePart *
modest_attachment_view_get_part_default (TnyMimePartView *self)
{
	ModestAttachmentViewPriv *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	if (priv->mime_part)
		return TNY_MIME_PART (g_object_ref (priv->mime_part));
	else
		return NULL;
}

static void
update_filename_request (ModestAttachmentView *self)
{
	ModestAttachmentViewPriv *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);
	/* gint width, height; */
	
	pango_layout_set_text (PANGO_LAYOUT (priv->layout_full_filename), 
			       gtk_label_get_text (GTK_LABEL (priv->filename_view)), -1);


}

static void
modest_attachment_view_set_part (TnyMimePartView *self, TnyMimePart *mime_part)
{
	MODEST_ATTACHMENT_VIEW_GET_CLASS (self)->set_part_func (self, mime_part);
	return;
}


static gboolean
get_size_idle_func (gpointer data)
{
	ModestAttachmentView *self = (ModestAttachmentView *) data;
	ModestAttachmentViewPriv *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);
	gssize readed_size;
	gchar read_buffer[GET_SIZE_BUFFER_SIZE];
	gchar *size_string;

	if (priv->get_size_stream == NULL) {
		priv->get_size_stream = tny_mime_part_get_stream (priv->mime_part);
	}

	readed_size = tny_stream_read (priv->get_size_stream, read_buffer, GET_SIZE_BUFFER_SIZE);
	priv->size += readed_size;

	if (tny_stream_is_eos (priv->get_size_stream)) {
		gchar *display_size;

		display_size = modest_text_utils_get_display_size (priv->size);
		size_string = g_strdup_printf (" (%s)", display_size);
		g_free (display_size);
		gtk_label_set_text (GTK_LABEL (priv->size_view), size_string);
		g_free (size_string);

		g_object_unref (priv->get_size_stream);

		gtk_widget_queue_resize (priv->size_view);
		priv->get_size_stream = NULL;
		priv->get_size_idle_id = 0;
	}

	return (priv->get_size_stream != NULL);
	
}

static void
modest_attachment_view_set_part_default (TnyMimePartView *self, TnyMimePart *mime_part)
{
	ModestAttachmentViewPriv *priv = NULL;
	const gchar *filename = NULL;
	gchar *file_icon_name = NULL;
	
	g_return_if_fail (TNY_IS_MIME_PART_VIEW (self));
	g_return_if_fail (TNY_IS_MIME_PART (mime_part));
	priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	if (priv->mime_part != NULL) {
		g_object_unref (priv->mime_part);
	}

	priv->mime_part = mime_part;

	if (priv->get_size_idle_id != 0) {
		g_source_remove (priv->get_size_idle_id);
		priv->get_size_idle_id = 0;
	}

	if (priv->get_size_stream != NULL) {
		g_object_unref (priv->get_size_stream);
		priv->get_size_stream = NULL;
	}

	priv->size = 0;

	if (TNY_IS_MSG (mime_part)) {
		TnyHeader *header = tny_msg_get_header (TNY_MSG (mime_part));
		if (TNY_IS_HEADER (header)) {
			filename = tny_header_get_subject (header);
			if (filename == NULL)
				filename = _("mail_va_no_subject");
			file_icon_name = modest_platform_get_file_icon_name (NULL, tny_mime_part_get_content_type (mime_part), NULL);
			g_object_unref (header);
		}
	} else {
		filename = tny_mime_part_get_filename (mime_part);
		file_icon_name = modest_platform_get_file_icon_name (filename, 
								     tny_mime_part_get_content_type (mime_part), 
								     NULL);
	}

	if (file_icon_name) {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), file_icon_name, GTK_ICON_SIZE_MENU);
		g_free (file_icon_name);
	} else {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), UNKNOWN_FILE_ICON, GTK_ICON_SIZE_MENU);
	}

	gtk_label_set_text (GTK_LABEL (priv->filename_view), filename);
	update_filename_request (MODEST_ATTACHMENT_VIEW (self));

	gtk_label_set_text (GTK_LABEL (priv->size_view), " ");

	priv->get_size_idle_id = g_idle_add ((GSourceFunc) get_size_idle_func, (gpointer) self);

	gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
modest_attachment_view_clear (TnyMimePartView *self)
{
	MODEST_ATTACHMENT_VIEW_GET_CLASS (self)->clear_func (self);
	return;
}

static void
modest_attachment_view_clear_default (TnyMimePartView *self)
{
	ModestAttachmentViewPriv *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	if (priv->mime_part != NULL) {
		g_object_unref (priv->mime_part);
		priv->mime_part = NULL;
	}

	if (priv->get_size_idle_id != 0) {
		g_source_remove (priv->get_size_idle_id);
		priv->get_size_idle_id = 0;
	}

	if (priv->get_size_stream != NULL) {
		g_object_unref (priv->get_size_stream);
		priv->get_size_stream = NULL;
	}

	priv->size = 0;

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), 
				      UNKNOWN_FILE_ICON,
				      GTK_ICON_SIZE_MENU);
	gtk_label_set_text (GTK_LABEL (priv->filename_view), "");
	update_filename_request (MODEST_ATTACHMENT_VIEW(self));
	gtk_label_set_text (GTK_LABEL (priv->size_view), " ");

	gtk_widget_queue_draw (GTK_WIDGET (self));
}


/**
 * modest_attachment_view_new:
 * @mime_part: a #TnyMimePart
 *
 * Constructor for attachment view widget.
 *
 * Return value: a new #ModestAttachmentView instance implemented for Gtk+
 **/
GtkWidget*
modest_attachment_view_new (TnyMimePart *mime_part)
{
	ModestAttachmentView *self = g_object_new (MODEST_TYPE_ATTACHMENT_VIEW, 
						   NULL);

	modest_attachment_view_set_part (TNY_MIME_PART_VIEW (self), mime_part);

	return GTK_WIDGET (self);
}

static void
modest_attachment_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestAttachmentViewPriv *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (instance);
	PangoContext *context;
	GtkWidget *box = NULL;

	priv->mime_part = NULL;
	priv->icon = gtk_image_new ();
	priv->filename_view = gtk_label_new ("");
	gtk_label_set_line_wrap (GTK_LABEL (priv->filename_view), FALSE);
	gtk_label_set_ellipsize (GTK_LABEL (priv->filename_view), PANGO_ELLIPSIZE_END);
	gtk_label_set_single_line_mode (GTK_LABEL (priv->filename_view), TRUE);
	gtk_label_set_selectable (GTK_LABEL (priv->filename_view), FALSE);
	priv->size_view = gtk_label_new (" ");
	gtk_label_set_line_wrap (GTK_LABEL (priv->size_view), FALSE);
	gtk_label_set_selectable (GTK_LABEL (priv->size_view), FALSE);
	gtk_misc_set_alignment (GTK_MISC (priv->size_view), 0.0, 0.5);
	gtk_misc_set_alignment (GTK_MISC (priv->filename_view), 0.0, 0.5);

	priv->get_size_idle_id = 0;
	priv->get_size_stream = NULL;
	priv->size = 0;

	box = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), priv->icon, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), priv->filename_view, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), priv->size_view, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (instance), box);

/* 	gtk_widget_get_style */
/* 	gtk_widget_modify_bg (instance, GTK_STATE_SELECTED, selection_color); */

	context = gtk_widget_get_pango_context (priv->filename_view);
	priv->layout_full_filename = pango_layout_new (context);
	
	pango_layout_set_ellipsize (priv->layout_full_filename, PANGO_ELLIPSIZE_NONE);

	gtk_event_box_set_above_child (GTK_EVENT_BOX (instance), TRUE);

	GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (instance), GTK_CAN_FOCUS);

	return;
}

static void
modest_attachment_view_finalize (GObject *object)
{
	ModestAttachmentViewPriv *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (object);

	if (priv->get_size_idle_id) {
		g_source_remove (priv->get_size_idle_id);
		priv->get_size_idle_id = 0;
	}

	if (priv->get_size_stream != NULL) {
		g_object_unref (priv->get_size_stream);
		priv->get_size_stream = NULL;
	}

	if (G_LIKELY (priv->mime_part)) {
		g_object_unref (G_OBJECT (priv->mime_part));
		priv->mime_part = NULL;
	}

	(*parent_class->finalize) (object);

	return;
}

static void
size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	ModestAttachmentViewPriv *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (widget);
	gint width, width_diff;

	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
	pango_layout_set_font_description (priv->layout_full_filename, pango_context_get_font_description(pango_layout_get_context (gtk_label_get_layout (GTK_LABEL (priv->filename_view)))));

	pango_layout_get_pixel_size (priv->layout_full_filename, &width, NULL);
	width_diff = priv->filename_view->allocation.width - width;
	if (width_diff > 0) {
		GtkAllocation filename_alloc, filesize_alloc;
		filename_alloc = priv->filename_view->allocation;
		filesize_alloc = priv->size_view->allocation;
		filename_alloc.width -= width_diff;
		filesize_alloc.width += width_diff;
		filesize_alloc.x -= width_diff;
		gtk_widget_size_allocate (priv->filename_view, &filename_alloc);
		gtk_widget_size_allocate (priv->size_view, &filesize_alloc);
	}
	
}


static void 
modest_attachment_view_class_init (ModestAttachmentViewClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = modest_attachment_view_finalize;

	klass->get_part_func = modest_attachment_view_get_part_default;
	klass->set_part_func = modest_attachment_view_set_part_default;
	klass->clear_func = modest_attachment_view_clear_default;

	widget_class->size_allocate = size_allocate;

	g_type_class_add_private (object_class, sizeof (ModestAttachmentViewPriv));

	return;
}

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
        TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

        klass->get_part_func = modest_attachment_view_get_part;
        klass->set_part_func = modest_attachment_view_set_part;
        klass->clear_func = modest_attachment_view_clear;

        return;
}

GType 
modest_attachment_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestAttachmentViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) modest_attachment_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (ModestAttachmentView),
		  0,      /* n_preallocs */
		  modest_attachment_view_instance_init    /* instance_init */
		};

		static const GInterfaceInfo tny_mime_part_view_info =
		{
			(GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
			NULL,        /* interface_finalize */
			NULL         /* interface_data */
		};

		type = g_type_register_static (GTK_TYPE_EVENT_BOX,
			"ModestAttachmentView",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_MIME_PART_VIEW,
					     &tny_mime_part_view_info);

	}

	return type;
}

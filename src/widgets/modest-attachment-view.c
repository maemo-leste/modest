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
#include <modest-tny-mime-part.h>
#include <tny-msg.h>
#include <modest-mail-operation.h>
#include <modest-mail-operation-queue.h>
#include <modest-runtime.h>
#include <modest-count-stream.h>
#include <modest-ui-constants.h>

#define GET_SIZE_BUFFER_SIZE 128

static GObjectClass *parent_class = NULL;

typedef struct _ModestAttachmentViewPrivate ModestAttachmentViewPrivate;

struct _ModestAttachmentViewPrivate
{
	TnyMimePart *mime_part;

	GtkWidget *icon;
	GtkWidget *filename_view;
	GtkWidget *size_view;

	gboolean detect_size;
	TnyStream *get_size_stream;
	guint64 size;

	PangoLayout *layout_full_filename;
	gboolean is_purged;

};

#ifdef MODEST_TOOLKIT_HILDON2
#define UNKNOWN_FILE_ICON "filemanager_unknown_file"
#else
#define UNKNOWN_FILE_ICON "qgn_list_gene_unknown_file"
#endif

#define MODEST_ATTACHMENT_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), MODEST_TYPE_ATTACHMENT_VIEW, ModestAttachmentViewPrivate))

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



static void update_filename_request (ModestAttachmentView *self);

static void update_size_label (ModestAttachmentView *self)
{
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);
	gchar *size_str;
	gchar *label_text;

	size_str = modest_text_utils_get_display_size (priv->size);
	label_text = g_strdup_printf (" (%s)", size_str);
	g_free (size_str);
	gtk_label_set_text (GTK_LABEL (priv->size_view), label_text);
	g_free (label_text);
}

static gboolean
idle_get_mime_part_size_cb (gpointer userdata)
{
	ModestAttachmentView *view = (ModestAttachmentView *) userdata;
	gdk_threads_enter ();

	if (GTK_WIDGET_VISIBLE (view)) {
		update_size_label (view);
	}

	gdk_threads_leave ();

	g_object_unref (view);

	return FALSE;
}

static gpointer
get_mime_part_size_thread (gpointer thr_user_data)
{
	ModestAttachmentView *view =  (ModestAttachmentView *) thr_user_data;
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (view);
	gsize total = 0;
	gssize result = 0;

	result = tny_mime_part_decode_to_stream (priv->mime_part, priv->get_size_stream, NULL);
	total = modest_count_stream_get_count(MODEST_COUNT_STREAM (priv->get_size_stream));
	if (total == 0) {
		modest_count_stream_reset_count(MODEST_COUNT_STREAM (priv->get_size_stream));
		result = tny_mime_part_write_to_stream (priv->mime_part, priv->get_size_stream, NULL);
		total = modest_count_stream_get_count(MODEST_COUNT_STREAM (priv->get_size_stream));
	}
	
	/* if there was an error, don't set the size (this is pretty uncommon) */
	if (result < 0) {
		g_warning ("%s: error while writing mime part to stream\n", __FUNCTION__);
	} else {
		priv->size = (guint64)total;
		g_idle_add (idle_get_mime_part_size_cb, g_object_ref (view));
	}
	g_object_unref (view);

	return NULL;
}

void
modest_attachment_view_set_detect_size (ModestAttachmentView *self, gboolean detect_size)
{
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	priv->detect_size = detect_size;
	
}

void
modest_attachment_view_set_size (ModestAttachmentView *self, guint64 size)
{
	ModestAttachmentViewPrivate *priv;

	g_return_if_fail (MODEST_IS_ATTACHMENT_VIEW (self));
	priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	if (!priv->detect_size) {
		priv->size = size;
		update_size_label (self);
	} else {
		g_assert ("Shouldn't set the size of the attachment view if detect size is enabled");
	}
}

guint64
modest_attachment_view_get_size (ModestAttachmentView *self)
{
	ModestAttachmentViewPrivate *priv;

	g_return_val_if_fail (MODEST_IS_ATTACHMENT_VIEW (self), 0);
	priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	return priv->size;
}

static TnyMimePart *
modest_attachment_view_get_part (TnyMimePartView *self)
{
	return MODEST_ATTACHMENT_VIEW_GET_CLASS (self)->get_part_func (self);
}

static TnyMimePart *
modest_attachment_view_get_part_default (TnyMimePartView *self)
{
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	if (priv->mime_part)
		return TNY_MIME_PART (g_object_ref (priv->mime_part));
	else
		return NULL;
}

static void
update_filename_request (ModestAttachmentView *self)
{
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);
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


static void
modest_attachment_view_set_part_default (TnyMimePartView *self, TnyMimePart *mime_part)
{
	ModestAttachmentViewPrivate *priv = NULL;
	gchar *filename = NULL;
	gchar *file_icon_name = NULL;
	gboolean show_size = FALSE;
	
	g_return_if_fail (TNY_IS_MIME_PART_VIEW (self));
	g_return_if_fail (TNY_IS_MIME_PART (mime_part));
	priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	if (priv->mime_part != NULL) {
		g_object_unref (priv->mime_part);
	}

	priv->mime_part = g_object_ref (mime_part);

	priv->size = 0;
	priv->is_purged = tny_mime_part_is_purged (mime_part);

	if (TNY_IS_MSG (mime_part)) {
		TnyHeader *header = tny_msg_get_header (TNY_MSG (mime_part));
		if (TNY_IS_HEADER (header)) {
			filename = g_strdup (tny_mime_part_get_filename (mime_part));
			if (!filename)
				filename = tny_header_dup_subject (header);
			if (filename == NULL || filename[0] == '\0') {
				if (filename)
					g_free (filename);
				filename = g_strdup (_("mail_va_no_subject"));
			}
			if (priv->is_purged) {
				file_icon_name = modest_platform_get_file_icon_name (NULL, NULL, NULL);
			} else {
				gchar *header_content_type;
				header_content_type = modest_tny_mime_part_get_content_type (mime_part);
				if ((g_str_has_prefix (header_content_type, "message/rfc822") ||
				     g_str_has_prefix (header_content_type, "multipart/"))) {
					file_icon_name = 
						modest_platform_get_file_icon_name (
							NULL, "message/rfc822", NULL);
				} else if (g_str_has_prefix (header_content_type, "text/")) {
					file_icon_name = 
						modest_platform_get_file_icon_name (
							NULL, tny_mime_part_get_content_type (mime_part), NULL);
				} else {
					file_icon_name = 
						modest_platform_get_file_icon_name (
							NULL, header_content_type, NULL);
				}
				g_free (header_content_type);
			}
			g_object_unref (header);
		}
	} else {
		gboolean is_other_body = FALSE;
		filename = g_strdup (tny_mime_part_get_filename (mime_part));
		if (filename == NULL) {
			gchar *description;
			description = modest_tny_mime_part_get_header_value (mime_part, "Content-Description");
			if (description) {
				g_strstrip (description);
				filename = description;
			}
			if (!filename || filename[0] == '\0') {
				g_free (filename);
				filename = g_strdup (_("mail_va_no_subject"));
			}
			is_other_body = TRUE;
		}
		if (priv->is_purged) {
			file_icon_name = modest_platform_get_file_icon_name (NULL, NULL, NULL);
		} else {
			if (is_other_body) {
				file_icon_name = modest_platform_get_file_icon_name (NULL, "message/rfc822", NULL);
			} else {
				file_icon_name = modest_platform_get_file_icon_name (
					filename, modest_tny_mime_part_get_content_type (mime_part), NULL);
				show_size = TRUE;
			}
		}
	}

	if (file_icon_name) {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), file_icon_name, GTK_ICON_SIZE_MENU);
		g_free (file_icon_name);
	} else {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), UNKNOWN_FILE_ICON, GTK_ICON_SIZE_MENU);
	}

	if (priv->is_purged) {
		gchar * label_str = g_markup_printf_escaped(
			"<span style='italic' foreground='grey'>%s</span>",
			filename);
		gtk_label_set_markup (GTK_LABEL (priv->filename_view), label_str);
		g_free (label_str);
	} else {
		gtk_label_set_text (GTK_LABEL (priv->filename_view), filename);
	}
	g_free (filename);
	update_filename_request (MODEST_ATTACHMENT_VIEW (self));

	gtk_label_set_text (GTK_LABEL (priv->size_view), "");

	if (show_size && priv->detect_size) {
		gchar *disposition;

		disposition = modest_tny_mime_part_get_header_value (mime_part, "Content-Disposition");
		if (disposition) {
			const gchar *size_tmp;
			size_tmp = strstr (disposition, "size=");
			if (size_tmp) size_tmp += strlen("size=");
			if (size_tmp) {
				gchar *disposition_value;
				const gchar *size_end;
				size_end = strstr (size_tmp, ";");
				if (size_end == NULL) {
					disposition_value = g_strdup (size_tmp);
				} else {
					disposition_value = g_strndup (size_tmp, size_end - size_tmp);
				}
				if (disposition_value && disposition_value[0] != '\0') {
					priv->size = atoll (disposition_value);
					if (priv->size != 0) {
						show_size = FALSE;
						update_size_label (MODEST_ATTACHMENT_VIEW (self));
					}
				}
				g_free (disposition_value);
			}
			
			g_free (disposition);
		}
	}

	if (show_size && priv->detect_size) {
		g_object_ref (self);
		if (!priv->get_size_stream)
			priv->get_size_stream = modest_count_stream_new ();
		g_thread_create (get_mime_part_size_thread, self, FALSE, NULL);
	}

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
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (self);

	if (priv->mime_part != NULL) {
		g_object_unref (priv->mime_part);
		priv->mime_part = NULL;
	}

	if (priv->get_size_stream)
		modest_count_stream_reset_count(MODEST_COUNT_STREAM (priv->get_size_stream));

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
modest_attachment_view_new (TnyMimePart *mime_part, gboolean detect_size)
{
	ModestAttachmentView *self = g_object_new (MODEST_TYPE_ATTACHMENT_VIEW, 
						   NULL);

	modest_attachment_view_set_detect_size (self, detect_size);

	modest_attachment_view_set_part (TNY_MIME_PART_VIEW (self), mime_part);

	return GTK_WIDGET (self);
}

static void
modest_attachment_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (instance);
	PangoContext *context;
	GtkWidget *box = NULL;
	GtkWidget *icon_alignment = NULL;

#ifdef MODEST_TOOLKIT_HILDON2
	PangoAttrList *attr_list;
	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));
#endif

	priv->mime_part = NULL;
	icon_alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (icon_alignment), 0, 0, 0, MODEST_MARGIN_DEFAULT);
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

#ifdef MODEST_TOOLKIT_HILDON2
	gtk_label_set_attributes (GTK_LABEL (priv->filename_view), attr_list);
	gtk_label_set_attributes (GTK_LABEL (priv->size_view), attr_list);
#endif

	priv->get_size_stream = NULL;
	priv->size = 0;
	priv->detect_size = TRUE;

	box = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (icon_alignment), priv->icon);
	gtk_box_pack_start (GTK_BOX (box), icon_alignment, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), priv->filename_view, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), priv->size_view, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (instance), box);

/* 	gtk_widget_get_style */
/* 	gtk_widget_modify_bg (instance, GTK_STATE_SELECTED, selection_color); */

	context = gtk_widget_get_pango_context (priv->filename_view);
	priv->layout_full_filename = pango_layout_new (context);
	
	pango_layout_set_ellipsize (priv->layout_full_filename, PANGO_ELLIPSIZE_NONE);

	gtk_event_box_set_above_child (GTK_EVENT_BOX (instance), FALSE);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (instance), TRUE);
	gtk_widget_set_events (GTK_WIDGET (instance), 0);

#ifdef MODEST_TOOLKIT_HILDON2
	pango_attr_list_unref (attr_list);
#endif

	GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET (instance), GTK_CAN_FOCUS);

	return;
}

static void
modest_attachment_view_finalize (GObject *object)
{
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (object);

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
	ModestAttachmentViewPrivate *priv = MODEST_ATTACHMENT_VIEW_GET_PRIVATE (widget);
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

	g_type_class_add_private (object_class, sizeof (ModestAttachmentViewPrivate));

	return;
}

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
        TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

        klass->get_part = modest_attachment_view_get_part;
        klass->set_part = modest_attachment_view_set_part;
        klass->clear = modest_attachment_view_clear;

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

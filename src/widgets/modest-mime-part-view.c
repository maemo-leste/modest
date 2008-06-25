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

#include <modest-mime-part-view.h>
#include <modest-marshal.h>

enum {
	ACTIVATE_LINK,
	LINK_HOVER,
	FETCH_URL,
	LAST_SIGNAL
};

static guint mime_part_view_signals[LAST_SIGNAL] = { 0 };

/**
 * modest_mime_part_view_is_empty:
 * @self: a #ModestMimePartView
 *
 * checks if the mail shown can be considered as empty or not.
 *
 * Returns: %TRUE if mail shown is empty, %FALSE otherwise.
 */
gboolean
modest_mime_part_view_is_empty (ModestMimePartView *self)
{
	return MODEST_MIME_PART_VIEW_GET_IFACE (self)->is_empty_func (self);
}

/**
 * modest_mime_part_view_get_view_images:
 * @self: a #ModestMimePartView
 *
 * checks if we have enabled capability to view contained images.
 *
 * Returns: %TRUE if we show images, %FALSE otherwise.
 */
gboolean
modest_mime_part_view_get_view_images (ModestMimePartView *self)
{
	return MODEST_MIME_PART_VIEW_GET_IFACE (self)->get_view_images_func (self);
}

/**
 * modest_mime_part_view_set_view_images:
 * @self: a #ModestMimePartView
 * @view_images: a #gboolean
 *
 * set if we want to show images or not.
 */
void
modest_mime_part_view_set_view_images (ModestMimePartView *self, gboolean view_images)
{
	MODEST_MIME_PART_VIEW_GET_IFACE (self)->set_view_images_func (self, view_images);
}

/**
 * modest_mime_part_view_has_external_images: 
 * @self: a #ModestMimePartView
 *
 * checks if there are external images in the mime part.
 *
 * Returns: %TRUE if there are external images, %FALSE otherwise.
 */
gboolean
modest_mime_part_view_has_external_images (ModestMimePartView *self)
{
	return MODEST_MIME_PART_VIEW_GET_IFACE (self)->has_external_images_func (self);
}

static void
modest_mime_part_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {

		/**
		 * ModestMimePartView::activate-link:
		 * @self: a #ModestMimePartView instance the signal is emitted
		 * @string: a string containing the URI
		 *
		 * This signal is emitted when a URI is activated
		 *
		 * Returns:
		 */
		mime_part_view_signals[ACTIVATE_LINK] =
			g_signal_new ("activate_link",
				      MODEST_TYPE_MIME_PART_VIEW,
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (ModestMimePartViewIface, activate_link),
				      NULL, NULL,
				      modest_marshal_BOOLEAN__STRING,
				      G_TYPE_BOOLEAN, 1,
				      G_TYPE_STRING);
		/**
		 * ModestMimePartView::link-hover:
		 * @self: a #ModestMimePartView instance the signal is emitted
		 * @string: a string containing the URI
		 *
		 * This signal is emitted when user passes the mouse over a link
		 *
		 * Returns:
		 */
		mime_part_view_signals[LINK_HOVER] =
			g_signal_new ("link_hover",
				      MODEST_TYPE_MIME_PART_VIEW,
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (ModestMimePartViewIface, link_hover),
				      NULL, NULL,
				      modest_marshal_BOOLEAN__STRING,
				      G_TYPE_BOOLEAN, 1,
				      G_TYPE_STRING);

		/**
		 * ModestMimePartView::fetch-url:
		 * @self: a #ModestMimePartView instance the signal is emitted
		 * @string: a string containing the URI
		 * @stream: a #TnyStream
		 *
		 * This signal is emitted when the page is rendered, and some
		 * url has to be resolved externally. You have to put the
		 * required information in the stream (images, etc).
		 *
		 * Returns:
		 */
		mime_part_view_signals[FETCH_URL] =
			g_signal_new ("fetch_url",
				      MODEST_TYPE_MIME_PART_VIEW,
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (ModestMimePartViewIface, fetch_url),
				      NULL, NULL,
				      modest_marshal_BOOLEAN__STRING_OBJECT,
				      G_TYPE_BOOLEAN, 2,
				      G_TYPE_STRING,
				      G_TYPE_OBJECT);
		initialized = TRUE;
	}
}

GType
modest_mime_part_view_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0)) 
	{
		static const GTypeInfo info = 
		{
		  sizeof (ModestMimePartViewIface),
		  modest_mime_part_view_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL,   /* instance_init */
		  NULL
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
			"ModestMimePartView", &info, 0);

	}

	return type;
}

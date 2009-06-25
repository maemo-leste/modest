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

#include <config.h>

#include <widgets/modest-msg-view.h>
#include <widgets/modest-isearch-view.h>
#include <modest-marshal.h>

enum {
	ATTACHMENT_CLICKED_SIGNAL,
	RECPT_ACTIVATED_SIGNAL,
	LINK_CONTEXTUAL_SIGNAL,
	FETCH_IMAGE_SIGNAL,
	SHOW_DETAILS_SIGNAL,
	LIMIT_ERROR_SIGNAL,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {0};

void
modest_msg_view_set_msg_with_other_body (ModestMsgView *self, TnyMsg *msg, TnyMimePart *part)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->set_msg_with_other_body_func (self, msg, part);
}

GtkAdjustment*
modest_msg_view_get_vadjustment (ModestMsgView *self)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->get_vadjustment_func (self);
}

GtkAdjustment*
modest_msg_view_get_hadjustment (ModestMsgView *self)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->get_hadjustment_func (self);
}

void
modest_msg_view_set_vadjustment (ModestMsgView *self, GtkAdjustment *vadj)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->set_vadjustment_func (self, vadj);
}

void
modest_msg_view_set_hadjustment (ModestMsgView *self, GtkAdjustment *hadj)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->set_hadjustment_func (self, hadj);
}

void
modest_msg_view_set_shadow_type (ModestMsgView *self, GtkShadowType type)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->set_shadow_type_func (self, type);
}

GtkShadowType
modest_msg_view_get_shadow_type (ModestMsgView *self)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->get_shadow_type_func (self);
}

TnyHeaderFlags
modest_msg_view_get_priority (ModestMsgView *self)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->get_priority_func (self);
}

void
modest_msg_view_set_priority (ModestMsgView *self, TnyHeaderFlags flags)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->set_priority_func (self, flags);
}

void
modest_msg_view_set_view_images (ModestMsgView *self, gboolean view_images)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->set_view_images_func (self, view_images);
}

TnyList*
modest_msg_view_get_selected_attachments (ModestMsgView *self)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->get_selected_attachments_func (self);
}

TnyList*
modest_msg_view_get_attachments (ModestMsgView *self)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->get_attachments_func (self);
}

void
modest_msg_view_grab_focus (ModestMsgView *self)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->grab_focus_func (self);
}

void
modest_msg_view_remove_attachment (ModestMsgView *self, TnyMimePart *attachment)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->remove_attachment_func (self, attachment);
}

void
modest_msg_view_set_branding (ModestMsgView *self, const gchar *brand_name, const GdkPixbuf *brand_icon)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->set_branding_func (self, brand_name, brand_icon);
}

void
modest_msg_view_request_fetch_images (ModestMsgView *self)
{
	MODEST_MSG_VIEW_GET_IFACE (self)->request_fetch_images_func (self);
}

gboolean
modest_msg_view_has_blocked_external_images (ModestMsgView *self)
{
	return MODEST_MSG_VIEW_GET_IFACE (self)->has_blocked_external_images_func (self);
}

static void
modest_msg_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		
		signals[ATTACHMENT_CLICKED_SIGNAL] =
			g_signal_new ("attachment_clicked",
				      MODEST_TYPE_MSG_VIEW,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestMsgViewIface, attachment_clicked),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__OBJECT,
				      G_TYPE_NONE, 1, G_TYPE_OBJECT);
		
		signals[RECPT_ACTIVATED_SIGNAL] =
			g_signal_new ("recpt_activated",
				      MODEST_TYPE_MSG_VIEW,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestMsgViewIface, recpt_activated),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__STRING,
				      G_TYPE_NONE, 1, G_TYPE_STRING);
		
		signals[LINK_CONTEXTUAL_SIGNAL] =
			g_signal_new ("link_contextual",
				      MODEST_TYPE_MSG_VIEW,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestMsgViewIface, link_contextual),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__STRING,
				      G_TYPE_NONE, 1, G_TYPE_STRING);
		
		signals[FETCH_IMAGE_SIGNAL] =
			g_signal_new ("fetch_image",
				      MODEST_TYPE_MSG_VIEW,
				      G_SIGNAL_ACTION | G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET(ModestMsgViewIface, fetch_image),
				      NULL, NULL,
				      modest_marshal_BOOLEAN__STRING_OBJECT,
				      G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_OBJECT);
		
		signals[SHOW_DETAILS_SIGNAL] =
			g_signal_new ("show_details",
				      MODEST_TYPE_MSG_VIEW,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestMsgViewIface, show_details),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);
		
		signals[LIMIT_ERROR_SIGNAL] =
			g_signal_new ("limit_error",
				      MODEST_TYPE_MSG_VIEW,
				      G_SIGNAL_RUN_FIRST,
				      G_STRUCT_OFFSET(ModestMsgViewIface, limit_error),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__VOID,
				      G_TYPE_NONE, 0);
		
		initialized = TRUE;
	}
}

GType
modest_msg_view_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(ModestMsgViewIface),
			modest_msg_view_base_init,   /* base init */
			NULL,		/* base finalize */
			NULL,           /* class_init */
			NULL,		/* class finalize */
			NULL,		/* class data */
			0,
			0,		/* n_preallocs */
			NULL,           /* instance_init */
			NULL
		};

 		my_type = g_type_register_static (G_TYPE_INTERFACE,
		                                  "ModestMsgView",
		                                  &my_info, 0);

		g_type_interface_add_prerequisite (my_type,
						   MODEST_TYPE_ZOOMABLE);
		g_type_interface_add_prerequisite (my_type,
						   MODEST_TYPE_ISEARCH_VIEW);
		g_type_interface_add_prerequisite (my_type,
						   TNY_TYPE_MIME_PART_VIEW);
		g_type_interface_add_prerequisite (my_type,
						   TNY_TYPE_HEADER_VIEW);
		g_type_interface_add_prerequisite (my_type,
						   MODEST_TYPE_MIME_PART_VIEW);

	}
	return my_type;
}

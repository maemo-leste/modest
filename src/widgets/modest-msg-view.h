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

#ifndef __MODEST_MSG_VIEW_H__
#define __MODEST_MSG_VIEW_H__

#include <tny-stream.h>
#include <tny-msg.h>
#include <tny-mime-part.h>
#include <tny-msg-view.h>
#include <tny-header-view.h>
#include <widgets/modest-recpt-view.h>
#include <widgets/modest-zoomable.h>
#include <widgets/modest-isearch-view.h>
#include <widgets/modest-mime-part-view.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MSG_VIEW             (modest_msg_view_get_type())
#define MODEST_MSG_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MSG_VIEW,ModestMsgView))
#define MODEST_IS_MSG_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MSG_VIEW))
#define MODEST_MSG_VIEW_GET_IFACE(obj)   (G_TYPE_INSTANCE_GET_INTERFACE((obj),MODEST_TYPE_MSG_VIEW,ModestMsgViewIface))

typedef struct _ModestMsgView      ModestMsgView;
typedef struct _ModestMsgViewIface ModestMsgViewIface;

struct _ModestMsgViewIface {
	GTypeInterface parent;

	void (*set_msg_with_other_body_func) (ModestMsgView *self, TnyMsg *msg, TnyMimePart *other_body);
	GtkAdjustment* (*get_vadjustment_func) (ModestMsgView *self);
	GtkAdjustment* (*get_hadjustment_func) (ModestMsgView *self);
	void (*set_vadjustment_func) (ModestMsgView *self, GtkAdjustment *vadj);
	void (*set_hadjustment_func) (ModestMsgView *self, GtkAdjustment *vadj);
	void (*set_shadow_type_func) (ModestMsgView *self, GtkShadowType type);
	GtkShadowType (*get_shadow_type_func) (ModestMsgView *self);
	TnyHeaderFlags (*get_priority_func) (ModestMsgView *self);
	void (*set_priority_func) (ModestMsgView *self, TnyHeaderFlags flags);
	void (*set_view_images_func) (ModestMsgView *self, gboolean view_images);
	TnyList * (*get_selected_attachments_func) (ModestMsgView *self);
	TnyList * (*get_attachments_func) (ModestMsgView *self);
	void (*grab_focus_func) (ModestMsgView *self);
	void (*remove_attachment_func) (ModestMsgView *view, TnyMimePart *attachment);
	void (*set_branding_func) (ModestMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);

	/* signals */
	void (*set_scroll_adjustments)      (ModestMsgView *msg_view,
					     GtkAdjustment *hadj,
					     GtkAdjustment *vadj);
	
	void (*link_contextual)    (ModestMsgView *msgview, const gchar* link,
				    gpointer user_data);
	void (*attachment_clicked) (ModestMsgView *msgview, TnyMimePart *mime_part,
				    gpointer user_data);
	void (*recpt_activated)    (ModestMsgView *msgview, const gchar *address,
				    gpointer user_data);
	gboolean (*fetch_image)    (ModestMsgView *msgview, const gchar *uri,
				    TnyStream *stream);
	void (*show_details)       (ModestMsgView *msgview, gpointer userdata);

	void (*request_fetch_images_func) (ModestMsgView *msgview);
	gboolean (*has_blocked_external_images_func) (ModestMsgView *msgview);
	void (*limit_error)        (ModestMsgView *msgview);
	void (*handle_calendar)    (ModestMsgView *msgview, TnyMimePart *calendar_part, GtkContainer *container);
};


/**
 *
 * modest_msg_view_get_type
 *
 * get the GType for the this interface
 *
 * Returns: the GType for this interface
 */
GType        modest_msg_view_get_type    (void) G_GNUC_CONST;

void modest_msg_view_set_msg_with_other_body (ModestMsgView *self, TnyMsg *msg, TnyMimePart *part);
GtkAdjustment *modest_msg_view_get_vadjustment (ModestMsgView *self);
GtkAdjustment *modest_msg_view_get_hadjustment (ModestMsgView *self);
void modest_msg_view_set_vadjustment (ModestMsgView *self, GtkAdjustment *vadj);
void modest_msg_view_set_hadjustment (ModestMsgView *self, GtkAdjustment *hadj);
void modest_msg_view_set_shadow_type (ModestMsgView *self, GtkShadowType type);
GtkShadowType modest_msg_view_get_shadow_type (ModestMsgView *self);

TnyHeaderFlags modest_msg_view_get_priority (ModestMsgView *self);
void modest_msg_view_set_priority (ModestMsgView *self, TnyHeaderFlags flags);
TnyList *modest_msg_view_get_selected_attachments (ModestMsgView *self);
TnyList *modest_msg_view_get_attachments (ModestMsgView *self);
void modest_msg_view_grab_focus (ModestMsgView *self);
void modest_msg_view_remove_attachment (ModestMsgView *view, TnyMimePart *attachment);
void modest_msg_view_set_branding (ModestMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);
void modest_msg_view_set_view_images (ModestMsgView *view, gboolean view_images);
void modest_msg_view_request_fetch_images (ModestMsgView *view);
gboolean modest_msg_view_has_blocked_external_images (ModestMsgView *view);


G_END_DECLS

#endif /* __MODEST_MSG_VIEW_H__ */

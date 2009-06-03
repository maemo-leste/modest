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

#ifndef __MODEST_MOZEMBED_MSG_VIEW_H__
#define __MODEST_MOZEMBED_MSG_VIEW_H__

#include <tny-stream.h>
#include <tny-msg.h>
#include <tny-mime-part.h>
#include <tny-msg-view.h>
#include <widgets/modest-zoomable.h>
#include <widgets/modest-isearch-view.h>
#include <widgets/modest-mime-part-view.h>
#include <widgets/modest-msg-view.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MOZEMBED_MSG_VIEW             (modest_mozembed_msg_view_get_type())
#define MODEST_MOZEMBED_MSG_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MOZEMBED_MSG_VIEW,ModestMozembedMsgView))
#define MODEST_MOZEMBED_MSG_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_MOZEMBED_MSG_VIEW,ModestMozembedMsgViewClass))
#define MODEST_IS_MOZEMBED_MSG_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MOZEMBED_MSG_VIEW))
#define MODEST_IS_MOZEMBED_MSG_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_MOZEMBED_MSG_VIEW))
#define MODEST_MOZEMBED_MSG_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_MOZEMBED_MSG_VIEW,ModestMozembedMsgViewClass))

typedef struct _ModestMozembedMsgView      ModestMozembedMsgView;
typedef struct _ModestMozembedMsgViewClass ModestMozembedMsgViewClass;

struct _ModestMozembedMsgView {
	GtkScrolledWindow parent;
};

struct _ModestMozembedMsgViewClass {
	GtkScrolledWindowClass parent_class;

	/* TnyHeaderView interface */
	void (*set_header_func) (TnyHeaderView *self, TnyHeader *header);
	void (*clear_header_func) (TnyHeaderView *self);
	/* TnyMimePartView interface */
	TnyMimePart* (*get_part_func) (TnyMimePartView *self);
	void (*set_part_func) (TnyMimePartView *self, TnyMimePart *part);
	/* ModestMimePartView interface methods */
	gboolean (*is_empty_func) (ModestMimePartView *self);
	/* TnyMsgView interface */
	TnyMsg* (*get_msg_func) (TnyMsgView *self);
	void (*set_msg_func) (TnyMsgView *self, TnyMsg *msg);
	void (*set_unavailable_func) (TnyMsgView *self);
	void (*clear_func) (TnyMsgView *self);
	TnyMimePartView* (*create_mime_part_view_for_func) (TnyMsgView *self, TnyMimePart *part);
	TnyMsgView* (*create_new_inline_viewer_func) (TnyMsgView *self);
	/* ModestZoomable interface */
	gdouble (*get_zoom_func) (ModestZoomable *self);
	void (*set_zoom_func) (ModestZoomable *self, gdouble value);
	gboolean (*zoom_minus_func) (ModestZoomable *self);
	gboolean (*zoom_plus_func) (ModestZoomable *self);
	/* ModestISearchView interface methods */
	gboolean (*search_func)             (ModestISearchView *self, const gchar *string);
	gboolean (*search_next_func)        (ModestISearchView *self);
	/* ModestMsgView interface methods */
	void (*set_msg_with_other_body) (ModestMsgView *self, TnyMsg *msg, TnyMimePart *part);
	GtkAdjustment* (*get_vadjustment_func) (ModestMsgView *self);
	GtkAdjustment* (*get_hadjustment_func) (ModestMsgView *self);
	void (*set_vadjustment_func) (ModestMsgView *self, GtkAdjustment *vadj);
	void (*set_hadjustment_func) (ModestMsgView *self, GtkAdjustment *vadj);
	void (*set_shadow_type_func) (ModestMsgView *self, GtkShadowType type);
	GtkShadowType (*get_shadow_type_func) (ModestMsgView *self);
	TnyHeaderFlags (*get_priority_func) (ModestMsgView *self);
	void (*set_priority_func) (ModestMsgView *self, TnyHeaderFlags flags);
	TnyList * (*get_selected_attachments_func) (ModestMsgView *self);
	TnyList * (*get_attachments_func) (ModestMsgView *self);
	void (*grab_focus_func) (ModestMsgView *self);
	void (*remove_attachment_func) (ModestMsgView *view, TnyMimePart *attachment);
	void (*request_fetch_images_func) (ModestMsgView *view);
	void (*set_branding_func) (ModestMsgView *view, const gchar *brand_name, const GdkPixbuf *brand_icon);
	gboolean (*has_blocked_external_images_func) (ModestMsgView *view);

	void (*set_scroll_adjustments) (ModestMozembedMsgView *msg_view, GtkAdjustment *hadj, GtkAdjustment *vadj);
};


/**
 * modest_mozembed_msg_view_get_type:
 *
 * get the GType for the this class
 *
 * Returns: the GType for this class
 */
GType        modest_mozembed_msg_view_get_type    (void) G_GNUC_CONST;


/**
 * modest_mozembed_msg_view_new 
 * @tny_msg: a TnyMsg instance, or NULL
 *
 * create a new ModestMozembedMsgView widget,
 * and display the @tny_msg e-mail message in it. If @tny_msg is NULL,
 * then a blank page will be displayed
 *  
 * Returns: a new ModestMozembedMsgView widget, or NULL if there's an error
 */
GtkWidget*   modest_mozembed_msg_view_new          (TnyMsg *tny_msg);

G_END_DECLS

#endif /* __MODEST_MOZEMBED_MSG_VIEW_H__ */

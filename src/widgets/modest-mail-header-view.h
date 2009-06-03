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

#ifndef __MODEST_MAIL_HEADER_VIEW_H__
#define __MODEST_MAIL_HEADER_VIEW_H__

#include <tny-header-view.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_MAIL_HEADER_VIEW             (modest_mail_header_view_get_type())
#define MODEST_MAIL_HEADER_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_MAIL_HEADER_VIEW,ModestMailHeaderView))
#define MODEST_IS_MAIL_HEADER_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_MAIL_HEADER_VIEW))
#define MODEST_MAIL_HEADER_VIEW_GET_IFACE(obj)   (G_TYPE_INSTANCE_GET_INTERFACE((obj),MODEST_TYPE_MAIL_HEADER_VIEW,ModestMailHeaderViewIface))

typedef struct _ModestMailHeaderView      ModestMailHeaderView;
typedef struct _ModestMailHeaderViewIface ModestMailHeaderViewIface;

struct _ModestMailHeaderViewIface {
	GTypeInterface parent;

	TnyHeaderFlags (*get_priority) (ModestMailHeaderView *self);
	void (*set_priority) (ModestMailHeaderView *self, TnyHeaderFlags flags);
	const GtkWidget *(*add_custom_header) (ModestMailHeaderView *self,
					       const gchar *label,
					       GtkWidget *custom_widget,
					       gboolean with_expander,
					       gboolean start);
	void (*set_loading) (ModestMailHeaderView *self, gboolean is_loading);
	gboolean (*get_loading) (ModestMailHeaderView *self);
	void (*set_branding) (ModestMailHeaderView *self, const gchar *brand_name, const GdkPixbuf *brand_icon);
	
	/* signals */
	void (*show_details) (ModestMailHeaderView *msgview,
			      gpointer user_data);
	void (*recpt_activated)    (ModestMailHeaderView *msgview, const gchar *address,
				    gpointer user_data);
};


/**
 *
 * modest_mail_header_view_get_type
 *
 * get the GType for the this interface
 *
 * Returns: the GType for this interface
 */
GType        modest_mail_header_view_get_type    (void) G_GNUC_CONST;

TnyHeaderFlags modest_mail_header_view_get_priority (ModestMailHeaderView *self);
void modest_mail_header_view_set_priority (ModestMailHeaderView *self, TnyHeaderFlags flags);
const GtkWidget *modest_mail_header_view_add_custom_header (ModestMailHeaderView *header_view,
							    const gchar *label,
							    GtkWidget *custom_widget,
							    gboolean with_expander,
							    gboolean start);
void modest_mail_header_view_set_loading (ModestMailHeaderView *self, gboolean is_loading);
gboolean modest_mail_header_view_get_loading (ModestMailHeaderView *self);
void modest_mail_header_view_set_branding (ModestMailHeaderView *self, const gchar *brand_name, const GdkPixbuf *brand_icon);


G_END_DECLS

#endif /* __MODEST_MAIL_HEADER_VIEW_H__ */

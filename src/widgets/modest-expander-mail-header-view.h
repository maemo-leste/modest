/* Copyright (c) 2007, 2008, Nokia Corporation
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

#ifndef MODEST_EXPANDER_MAIL_HEADER_VIEW_H
#define MODEST_EXPANDER_MAIL_HEADER_VIEW_H

#include <gtk/gtk.h>
#include <glib-object.h>

#include <tny-header-view.h>
#include <modest-mail-header-view.h>

G_BEGIN_DECLS

#define MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW             (modest_expander_mail_header_view_get_type ())
#define MODEST_EXPANDER_MAIL_HEADER_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW, ModestExpanderMailHeaderView))
#define MODEST_EXPANDER_MAIL_HEADER_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW, ModestExpanderMailHeaderViewClass))
#define MODEST_IS_EXPANDER_MAIL_HEADER_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW))
#define MODEST_IS_EXPANDER_MAIL_HEADER_VIEW_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW))
#define MODEST_EXPANDER_MAIL_HEADER_VIEW_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MODEST_TYPE_EXPANDER_MAIL_HEADER_VIEW, ModestExpanderMailHeaderViewClass))

typedef struct _ModestExpanderMailHeaderView ModestExpanderMailHeaderView;
typedef struct _ModestExpanderMailHeaderViewClass ModestExpanderMailHeaderViewClass;

struct _ModestExpanderMailHeaderView
{
	GtkHBox parent;

};

struct _ModestExpanderMailHeaderViewClass
{
	GtkHBoxClass parent_class;

	/* virtual methods */
	void (*set_header_func) (TnyHeaderView *self, TnyHeader *header);
	void (*clear_func) (TnyHeaderView *self);
	void (*set_priority_func) (ModestMailHeaderView *self, TnyHeaderFlags flags);
	TnyHeaderFlags (*get_priority_func) (ModestMailHeaderView *self);
	void (*set_loading_func) (ModestMailHeaderView *self, gboolean is_loading);
	gboolean (*get_loading_func) (ModestMailHeaderView *self);
	void (*set_branding_func) (ModestMailHeaderView *self, const gchar *brand_name, const GdkPixbuf *brand_icon);
	const GtkWidget *(*add_custom_header_func) (ModestMailHeaderView *self,
						    const gchar *label,
						    GtkWidget *custom_widget,
						    gboolean with_expander,
						    gboolean start);
};

GType modest_expander_mail_header_view_get_type (void);
TnyHeaderView* modest_expander_mail_header_view_new (gboolean expanded);						     

G_END_DECLS

#endif

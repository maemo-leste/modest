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

#ifndef MODEST_MAIL_HEADER_VIEW_H
#define MODEST_MAIL_HEADER_VIEW_H

#include <gtk/gtk.h>
#include <glib-object.h>

#include <tny-header-view.h>
#include <modest-recpt-view.h>

G_BEGIN_DECLS

#define MODEST_TYPE_MAIL_HEADER_VIEW             (modest_mail_header_view_get_type ())
#define MODEST_MAIL_HEADER_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_MAIL_HEADER_VIEW, ModestMailHeaderView))
#define MODEST_MAIL_HEADER_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MODEST_TYPE_MAIL_HEADER_VIEW, ModestMailHeaderViewClass))
#define MODEST_IS_MAIL_HEADER_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_MAIL_HEADER_VIEW))
#define MODEST_IS_MAIL_HEADER_VIEW_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MODEST_TYPE_MAIL_HEADER_VIEW))
#define MODEST_MAIL_HEADER_VIEW_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MODEST_TYPE_MAIL_HEADER_VIEW, ModestMailHeaderViewClass))

typedef struct _ModestMailHeaderView ModestMailHeaderView;
typedef struct _ModestMailHeaderViewClass ModestMailHeaderViewClass;

struct _ModestMailHeaderView
{
	GtkHBox parent;

};

struct _ModestMailHeaderViewClass
{
	GtkHBoxClass parent_class;

	/* virtual methods */
	void (*set_header_func) (TnyHeaderView *self, TnyHeader *header);
	void (*clear_func) (TnyHeaderView *self);

	/* signals */
	void (*recpt_activated) (const gchar *address);
};

GType modest_mail_header_view_get_type (void);
TnyHeaderView* modest_mail_header_view_new (void);

const GtkWidget *modest_mail_header_view_add_custom_header (ModestMailHeaderView *header_view,
							    const gchar *label,
							    GtkWidget *custom_widget,
							    gboolean with_expander,
							    gboolean start);

G_END_DECLS

#endif

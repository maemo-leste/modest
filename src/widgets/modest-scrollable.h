/* Copyright (c) 2009, Igalia
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

#ifndef MODEST_SCROLLABLE_H
#define MODEST_SCROLLABLE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODEST_TYPE_SCROLLABLE            (modest_scrollable_get_type ())
#define MODEST_SCROLLABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_SCROLLABLE, ModestScrollable))
#define MODEST_IS_SCROLLABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_SCROLLABLE))
#define MODEST_SCROLLABLE_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), MODEST_TYPE_SCROLLABLE, ModestScrollableIface))

typedef struct _ModestScrollable ModestScrollable;
typedef struct _ModestScrollableIface ModestScrollableIface;

struct _ModestScrollableIface
{
	GTypeInterface parent;

	void (*add_with_viewport) (ModestScrollable *self, GtkWidget *widget);
	GtkAdjustment * (*get_vadjustment) (ModestScrollable *self);
	GtkAdjustment * (*get_hadjustment) (ModestScrollable *self);
	void (*scroll_to) (ModestScrollable *self, const gint x, const gint y);
	void (*jump_to) (ModestScrollable *self, const gint x, const gint y);

	/* properties */
	/* hscrollbar-policy; */
	/* initial-hint; */
	/* vscrollbar-policy; */	
};

GType modest_scrollable_get_type (void);

void modest_scrollable_add_with_viewport (ModestScrollable *self, GtkWidget *widget);
GtkAdjustment * modest_scrollable_get_vadjustment (ModestScrollable *self);
GtkAdjustment * modest_scrollable_get_hadjustment (ModestScrollable *self);
void modest_scrollable_scroll_to (ModestScrollable *self, const gint x, const gint y);
void modest_scrollable_jump_to (ModestScrollable *self, const gint x, const gint y);


G_END_DECLS

#endif

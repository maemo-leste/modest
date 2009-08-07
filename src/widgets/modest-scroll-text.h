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

#ifndef MODEST_SCROLL_TEXT_H
#define MODEST_SCROLL_TEXT_H
#include <gtk/gtk.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MODEST_TYPE_SCROLL_TEXT             (modest_scroll_text_get_type ())
#define MODEST_SCROLL_TEXT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_SCROLL_TEXT, ModestScrollText))
#define MODEST_SCROLL_TEXT_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MODEST_TYPE_SCROLL_TEXT, ModestScrollTextClass))
#define MODEST_IS_SCROLL_TEXT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_SCROLL_TEXT))
#define MODEST_IS_SCROLL_TEXT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MODEST_TYPE_SCROLL_TEXT))
#define MODEST_SCROLL_TEXT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MODEST_TYPE_SCROLL_TEXT, ModestScrollTextClass))

typedef struct _ModestScrollText ModestScrollText;
typedef struct _ModestScrollTextClass ModestScrollTextClass;

struct _ModestScrollText
{
	GtkScrolledWindow parent;

};

struct _ModestScrollTextClass
{
	GtkScrolledWindowClass parent_class;
};

GType modest_scroll_text_get_type (void);

GtkWidget* modest_scroll_text_new (GtkTextView *text_view, guint line_limit);

void modest_scroll_text_set_text_view (ModestScrollText *scroll_text,
				       GtkTextView *text_view);
void modest_scroll_text_set_line_limit (ModestScrollText *scroll_text,
					guint line_limit);

const GtkWidget *modest_scroll_text_get_text_view (ModestScrollText *scroll_text);
guint modest_scroll_text_get_line_limit (ModestScrollText *scroll_text);

G_END_DECLS

#endif

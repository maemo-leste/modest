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

#ifndef __MODEST_FORMATTER_H__
#define __MODEST_FORMATTER_H__

#include <tny-msg.h>
#include <tny-header.h>
#include <tny-mime-part.h>

G_BEGIN_DECLS

#define MODEST_TYPE_FORMATTER             (modest_formatter_get_type ())
#define MODEST_FORMATTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODEST_TYPE_FORMATTER, ModestFormatter))
#define MODEST_FORMATTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MODEST_TYPE_FORMATTER, ModestFormatterClass))
#define MODEST_IS_FORMATTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODEST_TYPE_FORMATTER))
#define MODEST_IS_FORMATTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MODEST_TYPE_FORMATTER))
#define MODEST_FORMATTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MODEST_TYPE_FORMATTER, ModestFormatterClass))

typedef struct _ModestFormatter ModestFormatter;
typedef struct _ModestFormatterClass ModestFormatterClass;

struct _ModestFormatter
{
	GObject parent;
};

struct _ModestFormatterClass 
{
	GObjectClass parent;
};

GType modest_formatter_get_type (void);

ModestFormatter* modest_formatter_new (const gchar *content_type);

TnyMsg * modest_formatter_cite   (ModestFormatter *self, TnyMimePart *part, TnyHeader *header);
TnyMsg * modest_formatter_quote  (ModestFormatter *self, TnyMimePart *part, TnyHeader *header);
TnyMsg * modest_formatter_inline (ModestFormatter *self, TnyMimePart *part, TnyHeader *header);
TnyMsg * modest_formatter_attach (ModestFormatter *self, TnyMimePart *part, TnyHeader *header);


G_END_DECLS

#endif

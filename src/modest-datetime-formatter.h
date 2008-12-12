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


/* modest-text-utils.h */

#ifndef __MODEST_DATETIME_FORMATTER_H__
#define __MODEST_DATETIME_FORMATTER_H__

#include <glib-object.h>
#include <time.h>
G_BEGIN_DECLS

/* convenience macros */
#define MODEST_TYPE_DATETIME_FORMATTER             (modest_datetime_formatter_get_type())
#define MODEST_DATETIME_FORMATTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MODEST_TYPE_DATETIME_FORMATTER,ModestDatetimeFormatter))
#define MODEST_DATETIME_FORMATTER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MODEST_TYPE_DATETIME_FORMATTER,ModestDatetimeFormatterClass))
#define MODEST_IS_DATETIME_FORMATTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MODEST_TYPE_DATETIME_FORMATTER))
#define MODEST_IS_DATETIME_FORMATTER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MODEST_TYPE_DATETIME_FORMATTER))
#define MODEST_DATETIME_FORMATTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MODEST_TYPE_DATETIME_FORMATTER,ModestDatetimeFormatterClass))

typedef struct _ModestDatetimeFormatter      ModestDatetimeFormatter;
typedef struct _ModestDatetimeFormatterClass ModestDatetimeFormatterClass;

struct _ModestDatetimeFormatter {
	GObject parent;
};

struct _ModestDatetimeFormatterClass {
	GObjectClass parent_class;

	/* signals */
	void (*format_changed) (ModestDatetimeFormatter *self,
				gpointer userdata);
};

/**
 * modest_datetime_formatter_get_type:
 *
 * Returns: GType of the datetime formatter
 */
GType  modest_datetime_formatter_get_type   (void) G_GNUC_CONST;

ModestDatetimeFormatter *modest_datetime_formatter_new ();

const gchar *modest_datetime_formatter_format_date (ModestDatetimeFormatter *self,
						    time_t date);
const gchar *modest_datetime_formatter_format_time (ModestDatetimeFormatter *self,
						    time_t date);
const gchar *modest_datetime_formatter_display_datetime (ModestDatetimeFormatter *self,
							 time_t date);
G_END_DECLS

#endif

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

#ifndef __MODEST_WIDGET_MEMORY_PRIV_H__
#define __MODEST_WIDGET_MEMORY_PRIV_H__

G_BEGIN_DECLS


#define MODEST_WIDGET_MEMORY_PARAM_X             "x"
#define MODEST_WIDGET_MEMORY_PARAM_Y             "y"
#define MODEST_WIDGET_MEMORY_PARAM_HEIGHT        "height"
#define MODEST_WIDGET_MEMORY_PARAM_WIDTH         "width"
#define MODEST_WIDGET_MEMORY_PARAM_POS           "pos"
#define MODEST_WIDGET_MEMORY_PARAM_COLUMN_WIDTH  "column-width"
#define MODEST_WIDGET_MEMORY_PARAM_WINDOW_STYLE  "window-style"

/* private functions, only for use in modest-widget-memory and modest-init */
gchar* _modest_widget_memory_get_keyname           (const gchar *name, const gchar *param);
gchar* _modest_widget_memory_get_keyname_with_type (const gchar *name, guint type,
						    const gchar *param);
gchar* _modest_widget_memory_get_keyname_with_double_type (const gchar *name,
							   guint type1, guint type2,
							   const gchar *param);
G_END_DECLS

#endif /*__MODEST_WIDGET_MEMORY_PRIV_H__*/





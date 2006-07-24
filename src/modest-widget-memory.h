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

#ifndef __MODEST_WIDGET_MEMORY_H__
#define __MODEST_WIDGET_MEMORY_H__

#include <gtk/gtk.h>
#include "modest-conf.h"
#include "modest-conf-keys.h"

G_BEGIN_DECLS

/**
 * modest_widget_memory_save_settings:
 * @self: a ModestConf instance
 * @widget: the widget to save the settings for
 * @name: the unique name for this widget
 * 
 * store a the settings for a widget in the configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 */
gboolean modest_widget_memory_save_settings (ModestConf *conf, GtkWidget *widget,
					     const gchar *name);

/**
 * modest_widget_memory_restore_settings:
 * @self: a ModestConf instance
 * @widget: the widget to save the settings for
 * @name: the unique name for this widget
 *
 * restore the settings for a widget configuration system
 * 
 * Returns: TRUE if succeeded or FALSE in case of error.
 */
gboolean modest_widget_memory_restore_settings (ModestConf *conf, GtkWidget *widget,
						const gchar *name);
G_END_DECLS

#endif /*__MODEST_WIDGET_MEMORY_H__*/
